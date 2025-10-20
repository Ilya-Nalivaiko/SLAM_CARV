/**
* This file is part of ORB-SLAM2.
*
* Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
* For more information see <https://github.com/raulmur/ORB_SLAM2>
*
* ORB-SLAM2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM2 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ORB-SLAM2. If not, see <http://www.gnu.org/licenses/>.
*/

#include "Map.h"

// carv: include modeler to delete points and keyframes
#include "Modeler/Modeler.h"

#include<mutex>
#include <thread>

namespace ORB_SLAM2
{

Map::Map():mnMaxKFid(0),mnBigChangeIdx(0)
{
}

void Map::AddKeyFrame(KeyFrame *pKF)
{
    unique_lock<mutex> lock(mMutexMap);
    mspKeyFrames.insert(pKF);
    if(pKF->mnId>mnMaxKFid)
        mnMaxKFid=pKF->mnId;
    std::cout<<"new key frame inserted! now count: "<< mspKeyFrames.size()<<std::endl;
    newestKeyFrame.reset(pKF, [](KeyFrame*){});
}

std::shared_ptr<KeyFrame> Map::GetNewestKeyFrame()
{
    return newestKeyFrame;
}

void Map::AddMapPoint(MapPoint *pMP)
{
    unique_lock<mutex> lock(mMutexMap);
    mspMapPoints.insert(pMP);
}

void Map::EraseMapPoint(MapPoint *pMP)
{
    unique_lock<mutex> lock(mMutexMap);
    // 1) Erase from the main set of live points
    mspMapPoints.erase(pMP);

    // 2) Purge any lingering references so viewers don’t see stale pointers
    auto &refs = mvpReferenceMapPoints;
    refs.erase(std::remove(refs.begin(), refs.end(), pMP), refs.end());
    // 3) Inform the modeler; actual memory free is still deferred
    mpModeler->AddDeletePointEntry(pMP);

    // Note: pointer will be freed later by CollectTrash() after DeferErase().
}

void Map::EraseKeyFrame(KeyFrame *pKF)
{
    unique_lock<mutex> lock(mMutexMap);
    mspKeyFrames.erase(pKF);

    // TODO: This only erase the pointer.
    // Delete the MapPoint
}

void Map::SetReferenceMapPoints(const vector<MapPoint *> &vpMPs)
{
    unique_lock<mutex> lock(mMutexMap);
    mvpReferenceMapPoints.clear();
    for (MapPoint* p : vpMPs) {
        if (p && mspMapPoints.count(p) && !p->isBad())
            mvpReferenceMapPoints.push_back(p);
    }
}

void Map::InformNewBigChange()
{
    unique_lock<mutex> lock(mMutexMap);
    mnBigChangeIdx++;
}

int Map::GetLastBigChangeIdx()
{
    unique_lock<mutex> lock(mMutexMap);
    return mnBigChangeIdx;
}

vector<KeyFrame*> Map::GetAllKeyFrames()
{
    unique_lock<mutex> lock(mMutexMap);
    return vector<KeyFrame*>(mspKeyFrames.begin(),mspKeyFrames.end());
}

void Map::SnapshotMapPoints(std::vector<cv::Vec3f>& nonRefOut,
                            std::vector<cv::Vec3f>& refOut)
{
    nonRefOut.clear();
    refOut.clear();

    // Hold the update lock to block frees while we snapshot and materialize outputs.
    // (Writers should take this as unique; readers like us use shared.)
    std::shared_lock<std::shared_mutex> lkUpdate(mMutexMapUpdate);

    // snapshot containers under the map mutex, then release it
    std::vector<MapPoint*> vpMPs;
    std::vector<MapPoint*> vpRef;
    {
        std::unique_lock<std::mutex> lk(mMutexMap);

        vpMPs.reserve(mspMapPoints.size());
        for (MapPoint* pMP : mspMapPoints)
        {
            // mspMapPoints is the authoritative set; just copy pointers.
            if (pMP) vpMPs.push_back(pMP);
        }

        // Reference set is a subset/cached list provided by the map.
        vpRef = mvpReferenceMapPoints;
    } // <-- mMutexMap released here; do NOT reacquire it below

    // Build sets for fast membership checks without touching map locks again.
    std::unordered_set<MapPoint*> allSet(vpMPs.begin(), vpMPs.end());
    std::unordered_set<MapPoint*> refSet(vpRef.begin(), vpRef.end());

    nonRefOut.reserve(vpMPs.size());
    refOut.reserve(vpRef.size());

    // ---- Phase 2: emit non-reference points (no map locks inside this loop) ----
    for (MapPoint* mp : vpMPs)
    {
        if (!mp) continue;

        // Keep the MapPoint alive while we read its state.
        MapPoint::Pin guard(mp);

        // Guard feature-side flags/state; do NOT take the map mutex here.
        std::shared_lock<std::shared_mutex> lkFeat(mp->mMutexFeatures);

        // Skip bad points and those that belong to the reference set.
        if (mp->isBad()) continue;
        if (refSet.find(mp) != refSet.end()) continue;

        // Pose read (locks mp->mMutexPos internally).
        const cv::Mat P = mp->GetWorldPos();
        if (P.empty() || P.rows < 3) continue;

        nonRefOut.emplace_back(P.at<float>(0), P.at<float>(1), P.at<float>(2));
    }

    // ---- Phase 3: emit reference points (no map locks inside this loop) ----
    for (MapPoint* mp : vpRef)
    {
        if (!mp) continue;

        // Optional: ensure this ref is still in the main set using our snapshot.
        if (allSet.find(mp) == allSet.end()) continue;

        MapPoint::Pin guard(mp);
        std::shared_lock<std::shared_mutex> lkFeat(mp->mMutexFeatures);
        if (mp->isBad()) continue;

        const cv::Mat P = mp->GetWorldPos();
        if (P.empty() || P.rows < 3) continue;

        refOut.emplace_back(P.at<float>(0), P.at<float>(1), P.at<float>(2));
    }

    // lkUpdate released here; we return only POD data (Vec3f positions).
}

std::vector<MapPoint*> Map::GetAllMapPoints()
{
    std::unique_lock<std::mutex> lock(mMutexMap);
    std::vector<MapPoint*> v;
    v.reserve(mspMapPoints.size());
    for (MapPoint* pMP : mspMapPoints)
    {
        // Only publish fully usable points while holding the map lock
        if (pMP && !pMP->isBad())
            v.push_back(pMP);
    }
    return v;
}

long unsigned int Map::MapPointsInMap()
{
    unique_lock<mutex> lock(mMutexMap);
    return mspMapPoints.size();
}

long unsigned int Map::KeyFramesInMap()
{
    unique_lock<mutex> lock(mMutexMap);
    return mspKeyFrames.size();
}

// Map.cc
std::vector<MapPoint*> Map::GetReferenceMapPoints()
{
    std::unique_lock<std::mutex> lock(mMutexMap);
    std::vector<MapPoint*> out;
    out.reserve(mvpReferenceMapPoints.size());
    for (MapPoint* mp : mvpReferenceMapPoints) {
        if (mp && mspMapPoints.count(mp) && !mp->isBad())
            out.push_back(mp);
    }
    return out;
}

long unsigned int Map::GetMaxKFid()
{
    unique_lock<mutex> lock(mMutexMap);
    return mnMaxKFid;
}

void Map::clear(){
 
    // 1) Block tracker/mapper updates for the whole reset.
    std::unique_lock<std::shared_mutex> lkUpdate(mMutexMapUpdate);

    // 2) Snapshot pointers under the map mutex, then release it before SetBadFlag().
    std::vector<MapPoint*> points;
    std::vector<KeyFrame*> keyframes;
    {
        std::unique_lock<std::mutex> lk(mMutexMap);
        points.reserve(mspMapPoints.size());
        for (auto* p : mspMapPoints) {
            if (p) points.push_back(p);
        }
        keyframes.reserve(mspKeyFrames.size());
        for (auto* k : mspKeyFrames) {
            if (k) keyframes.push_back(k);
        }
    } // <-- mMutexMap released here. Important: avoid re-lock via SetBadFlag()->DeferErase()->EraseMapPoint().

    // 3) Detach map points without holding mMutexMap to prevent self-deadlock.
    //    SetBadFlag() will internally call DeferErase(), which takes mMutexMap.
    for (MapPoint* pMP : points) {
        if (pMP) pMP->SetBadFlag();
    }

    // 4) Now it is safe to delete keyframes (observations already removed by SetBadFlag()).
    for (KeyFrame* pKF : keyframes) {
        delete pKF;
    }

    // 5) Clear containers and indices under the map mutex.
    {
        std::unique_lock<std::mutex> lk(mMutexMap);
        mspMapPoints.clear();
        mspKeyFrames.clear();
        mnMaxKFid = 0;
        mvpReferenceMapPoints.clear();
        mvpKeyFrameOrigins.clear();
    }

    // 6) Finally free MapPoints when no one can still hold stale refs.
    CollectTrash();
}

void Map::DeferErase(MapPoint* pMP)
{
    // pMP is already detached & marked bad by SetBadFlag()/Replace()
    std::lock_guard<std::mutex> lk(mMutexTrash);
    mTrash.push_back(pMP);
    EraseMapPoint(pMP); //this does NOT free memory, just prevents the point from being sent out anymore

    //Ensure points that are marked bad are actually erased (with this function) eventually. currently this is done in setbadflag and replace
}

void Map::CollectTrash()
{
    // Stop the world for Tracking while we actually free memory.
    std::unique_lock<std::shared_mutex> lkUpdate(mMutexMapUpdate);

    std::list<MapPoint*> to_delete;
    {
        std::lock_guard<std::mutex> lk(mMutexTrash);
        to_delete.swap(mTrash);
    }
    // Now nobody else should hold references (Tracking is locked out and
    // LocalMapping already removed observations)
    for (MapPoint* p : to_delete) {
        while (p->mPins.load(std::memory_order_acquire) != 0) {
            std::this_thread::yield();
        }
        delete p;
    }
}


} //namespace ORB_SLAM
