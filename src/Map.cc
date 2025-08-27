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
    mspMapPoints.erase(pMP);

    //carv: remove point in modeler
    mpModeler->AddDeletePointEntry(pMP);

    // TODO: This only erase the pointer.
    // Delete the MapPoint
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
    mvpReferenceMapPoints = vpMPs;
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

// KeyFrame * Map::GetKeyFrameById(long unsigned int kf_id)
// {
//
// }

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

vector<MapPoint*> Map::GetReferenceMapPoints()
{
    unique_lock<mutex> lock(mMutexMap);
    return mvpReferenceMapPoints;
}

long unsigned int Map::GetMaxKFid()
{
    unique_lock<mutex> lock(mMutexMap);
    return mnMaxKFid;
}

void Map::clear()
{
    // 1) Block tracker/mapper updates, then take the map container lock.
    {
        std::unique_lock<std::shared_mutex> lkUpdate(mMutexMapUpdate);
        std::unique_lock<std::mutex> lk(mMutexMap);

        // 2) Do NOT delete MapPoints here. Mark them bad; deletion is deferred.
        //    This avoids racing with readers that still hold cv::Mat headers
        //    into MapPoint data (the cv::Mat::release() crash you saw).
        for (MapPoint* pMP : mspMapPoints)
        {
            if (pMP) pMP->SetBadFlag();  // queues into mTrash via DeferErase()
        }

        // 3) With points detached from KFs, it’s safe to delete KeyFrames under the lock.
        for (KeyFrame* pKF : mspKeyFrames)
        {
            delete pKF;
        }

        mspMapPoints.clear();
        mspKeyFrames.clear();
        mnMaxKFid = 0;
        mvpReferenceMapPoints.clear();
        mvpKeyFrameOrigins.clear();
        // locks released at scope end
    }

    // 4) Actually free MapPoints behind a barrier on the update mutex.
    //    This prevents races with Tracking still holding cv::Mat headers.
    CollectTrash();
}

void Map::DeferErase(MapPoint* pMP)
{
    // pMP is already detached & marked bad by SetBadFlag()/Replace()
    std::lock_guard<std::mutex> lk(mMutexTrash);
    mTrash.push_back(pMP);
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
        delete p;
    }
}


} //namespace ORB_SLAM
