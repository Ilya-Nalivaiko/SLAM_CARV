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

#ifndef MAP_H
#define MAP_H

#include "MapPoint.h"
#include "KeyFrame.h"

#include <set>
#include <list>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <vector>
#include <opencv2/core/matx.hpp>   // cv::Vec3f

#include <iostream>

namespace ORB_SLAM2 {

    class MapPoint;

    class KeyFrame;

    // carv: declaration
    class Modeler;

    class Map {
    private:
        std::list<MapPoint*> mTrash;   // queued bad/replaced points
        std::mutex           mMutexTrash;

    public:
        // SAFETY: two‑phase deletion of MapPoints
        void DeferErase(MapPoint* pMP);
        void CollectTrash();

        Map();

        void AddKeyFrame(KeyFrame *pKF);

        void AddMapPoint(MapPoint* pMP);

        void EraseMapPoint(MapPoint *pMP);

        void EraseKeyFrame(KeyFrame *pKF);

        void SetReferenceMapPoints(const std::vector<MapPoint *> &vpMPs);

        void InformNewBigChange();
        int GetLastBigChangeIdx();

        std::vector<KeyFrame *> GetAllKeyFrames();


        std::vector<MapPoint *> GetAllMapPoints();

        std::vector<MapPoint *> GetReferenceMapPoints();

        // NEW: lifetime-safe snapshot for rendering (no raw pointers in the viewer)
        void SnapshotMapPoints(std::vector<cv::Vec3f>& nonRefOut,
                               std::vector<cv::Vec3f>& refOut);

        long unsigned int MapPointsInMap();

        long unsigned KeyFramesInMap();

        long unsigned int GetMaxKFid();

        std::shared_ptr<KeyFrame> GetNewestKeyFrame();

        void clear();

        vector<KeyFrame *> mvpKeyFrameOrigins;

        // Blocks CollectTrash() (unique) vs readers (shared)
        mutable std::shared_mutex mMutexMapUpdate;
        struct ReadGuard {
            std::shared_lock<std::shared_mutex> lk;
            explicit ReadGuard(Map* m) : lk(m->mMutexMapUpdate) {}
        };

        // This avoid that two points are created simultaneously in separate threads (id conflict)
        std::mutex mMutexPointCreation;

        // carv: pointer to modeler
        Modeler* mpModeler;
        void SetModeler(Modeler* pModeler){
            mpModeler = pModeler;
        }
        std::shared_ptr<KeyFrame> newestKeyFrame;

        std::mutex mMutexMap;
    protected:


        std::set<MapPoint *> mspMapPoints;
        std::set<KeyFrame *> mspKeyFrames;

        std::vector<MapPoint *> mvpReferenceMapPoints;

        long unsigned int mnMaxKFid;

        // Index related to a big change in the map (loop closure, global BA)
        int mnBigChangeIdx;

    };

} //namespace ORB_SLAM

#endif // MAP_H
