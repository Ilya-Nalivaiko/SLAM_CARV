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

#include "LoopClosing.h"

#include "Sim3Solver.h"

#include "Converter.h"

#include "Optimizer.h"

#include "ORBmatcher.h"

#include<mutex>
#include<thread>


namespace ORB_SLAM2
{

LoopClosing::LoopClosing(Map *pMap, KeyFrameDatabase *pDB, ORBVocabulary *pVoc, const bool bFixScale):
    mbResetRequested(false), mbFinishRequested(false), mbFinished(true), mpMap(pMap),
    mpKeyFrameDB(pDB), mpORBVocabulary(pVoc), mpMatchedKF(NULL), mLastLoopKFid(0), mbRunningGBA(false), mbFinishedGBA(true),
    mbStopGBA(false), mpThreadGBA(NULL), mbFixScale(bFixScale), mnFullBAIdx(0)
{
    mnCovisibilityConsistencyTh = 3;
}

void LoopClosing::SetTracker(Tracking *pTracker)
{
    mpTracker=pTracker;
}

void LoopClosing::SetLocalMapper(LocalMapping *pLocalMapper)
{
    mpLocalMapper=pLocalMapper;
}


void LoopClosing::Run()
{
    mbFinished =false;

    while(1)
    {
        if (Stop())
        {
            while (isStopped() && !CheckFinish())
            {
                usleep(1000);
            }
            if (CheckFinish())
                break;
        }

        if (CheckFinish()) break;
        // Check if there are keyframes in the queue
        if(CheckNewKeyFrames())
        {
            // Detect loop candidates and check covisibility consistency
            if(DetectLoop())
            {
               // Compute similarity transformation [sR|t]
               // In the stereo/RGBD case s=1
               if(ComputeSim3())
               {
                   // Perform loop fusion and pose graph optimization
                   CorrectLoop();
               }
            }
        }       

        ResetIfRequested();

        if(CheckFinish())
            break;

        usleep(5000);
    }

    SetFinish();
}

void LoopClosing::InsertKeyFrame(KeyFrame *pKF)
{
    if (CheckFinish()) return;
    unique_lock<mutex> lock(mMutexLoopQueue);
    if(pKF->mnId!=0)
        mlpLoopKeyFrameQueue.push_back(pKF);
}

bool LoopClosing::CheckNewKeyFrames()
{
    unique_lock<mutex> lock(mMutexLoopQueue);
    return(!mlpLoopKeyFrameQueue.empty());
}

bool LoopClosing::DetectLoop()
{
    if (CheckFinish()) return false;
    {
        unique_lock<mutex> lock(mMutexLoopQueue);
        mpCurrentKF = mlpLoopKeyFrameQueue.front();
        mlpLoopKeyFrameQueue.pop_front();
        // Avoid that a keyframe can be erased while it is being process by this thread
        mpCurrentKF->SetNotErase();
    }

    //If the map contains less than 10 KF or less than 10 KF have passed from last loop detection
    if(mpCurrentKF->mnId<mLastLoopKFid+10)
    {
        mpKeyFrameDB->add(mpCurrentKF);
        mpCurrentKF->SetErase();
        return false;
    }

    // Compute reference BoW similarity score
    // This is the lowest score to a connected keyframe in the covisibility graph
    // We will impose loop candidates to have a higher similarity than this
    const vector<KeyFrame*> vpConnectedKeyFrames = mpCurrentKF->GetVectorCovisibleKeyFrames();
    const DBoW2::BowVector &CurrentBowVec = mpCurrentKF->mBowVec;
    float minScore = 1;
    for(size_t i=0; i<vpConnectedKeyFrames.size(); i++)
    {
        KeyFrame* pKF = vpConnectedKeyFrames[i];
        if(pKF->isBad())
            continue;
        const DBoW2::BowVector &BowVec = pKF->mBowVec;

        float score = mpORBVocabulary->score(CurrentBowVec, BowVec);

        if(score<minScore)
            minScore = score;
    }

    // Query the database imposing the minimum score
    vector<KeyFrame*> vpCandidateKFs = mpKeyFrameDB->DetectLoopCandidates(mpCurrentKF, minScore);

    // If there are no loop candidates, just add new keyframe and return false
    if(vpCandidateKFs.empty())
    {
        mpKeyFrameDB->add(mpCurrentKF);
        mvConsistentGroups.clear();
        mpCurrentKF->SetErase();
        return false;
    }

    // For each loop candidate check consistency with previous loop candidates
    // Each candidate expands a covisibility group (keyframes connected to the loop candidate in the covisibility graph)
    // A group is consistent with a previous group if they share at least a keyframe
    // We must detect a consistent loop in several consecutive keyframes to accept it
    mvpEnoughConsistentCandidates.clear();

    vector<ConsistentGroup> vCurrentConsistentGroups;
    vector<bool> vbConsistentGroup(mvConsistentGroups.size(),false);
    for(size_t i=0, iend=vpCandidateKFs.size(); i<iend; i++)
    {
        KeyFrame* pCandidateKF = vpCandidateKFs[i];

        set<KeyFrame*> spCandidateGroup = pCandidateKF->GetConnectedKeyFrames();
        spCandidateGroup.insert(pCandidateKF);

        bool bEnoughConsistent = false;
        bool bConsistentForSomeGroup = false;
        for(size_t iG=0, iendG=mvConsistentGroups.size(); iG<iendG; iG++)
        {
            set<KeyFrame*> sPreviousGroup = mvConsistentGroups[iG].first;

            bool bConsistent = false;
            for(set<KeyFrame*>::iterator sit=spCandidateGroup.begin(), send=spCandidateGroup.end(); sit!=send;sit++)
            {
                if(sPreviousGroup.count(*sit))
                {
                    bConsistent=true;
                    bConsistentForSomeGroup=true;
                    break;
                }
            }

            if(bConsistent)
            {
                int nPreviousConsistency = mvConsistentGroups[iG].second;
                int nCurrentConsistency = nPreviousConsistency + 1;
                if(!vbConsistentGroup[iG])
                {
                    ConsistentGroup cg = make_pair(spCandidateGroup,nCurrentConsistency);
                    vCurrentConsistentGroups.push_back(cg);
                    vbConsistentGroup[iG]=true; //this avoid to include the same group more than once
                }
                if(nCurrentConsistency>=mnCovisibilityConsistencyTh && !bEnoughConsistent)
                {
                    mvpEnoughConsistentCandidates.push_back(pCandidateKF);
                    bEnoughConsistent=true; //this avoid to insert the same candidate more than once
                }
            }
        }

        // If the group is not consistent with any previous group insert with consistency counter set to zero
        if(!bConsistentForSomeGroup)
        {
            ConsistentGroup cg = make_pair(spCandidateGroup,0);
            vCurrentConsistentGroups.push_back(cg);
        }
    }

    // Update Covisibility Consistent Groups
    mvConsistentGroups = vCurrentConsistentGroups;


    // Add Current Keyframe to database
    if(!CheckFinish()) mpKeyFrameDB->add(mpCurrentKF);

    if(mvpEnoughConsistentCandidates.empty())
    {
        mpCurrentKF->SetErase();
        return false;
    }
    else
    {
        return true;
    }

    mpCurrentKF->SetErase();
    return false;
}

bool LoopClosing::ComputeSim3()
{
    // For each consistent loop candidate we try to compute a Sim3

    const int nInitialCandidates = mvpEnoughConsistentCandidates.size();

    // We compute first ORB matches for each candidate
    // If enough matches are found, we setup a Sim3Solver
    ORBmatcher matcher(0.75,true);

    vector<Sim3Solver*> vpSim3Solvers;
    vpSim3Solvers.resize(nInitialCandidates);

    vector<vector<MapPoint*> > vvpMapPointMatches;
    vvpMapPointMatches.resize(nInitialCandidates);

    vector<bool> vbDiscarded;
    vbDiscarded.resize(nInitialCandidates);

    int nCandidates=0; //candidates with enough matches

    for(int i=0; i<nInitialCandidates; i++)
    {
        KeyFrame* pKF = mvpEnoughConsistentCandidates[i];

        // avoid that local mapping erase it while it is being processed in this thread
        pKF->SetNotErase();

        if(pKF->isBad())
        {
            vbDiscarded[i] = true;
            continue;
        }

        int nmatches = matcher.SearchByBoW(mpCurrentKF,pKF,vvpMapPointMatches[i]);

        if(nmatches<20)
        {
            vbDiscarded[i] = true;
            continue;
        }
        else
        {
            Sim3Solver* pSolver = new Sim3Solver(mpCurrentKF,pKF,vvpMapPointMatches[i],mbFixScale);
            pSolver->SetRansacParameters(0.99,20,300);
            vpSim3Solvers[i] = pSolver;
        }

        nCandidates++;
    }

    bool bMatch = false;

    // Perform alternatively RANSAC iterations for each candidate
    // until one is succesful or all fail
    while(nCandidates>0 && !bMatch)
    {
        for(int i=0; i<nInitialCandidates; i++)
        {
            if(vbDiscarded[i])
                continue;

            KeyFrame* pKF = mvpEnoughConsistentCandidates[i];

            // Perform 5 Ransac Iterations
            vector<bool> vbInliers;
            int nInliers;
            bool bNoMore;

            Sim3Solver* pSolver = vpSim3Solvers[i];
            cv::Mat Scm  = pSolver->iterate(5,bNoMore,vbInliers,nInliers);

            // If Ransac reachs max. iterations discard keyframe
            if(bNoMore)
            {
                vbDiscarded[i]=true;
                nCandidates--;
            }

            // If RANSAC returns a Sim3, perform a guided matching and optimize with all correspondences
            if(!Scm.empty())
            {
                vector<MapPoint*> vpMapPointMatches(vvpMapPointMatches[i].size(), static_cast<MapPoint*>(NULL));
                for(size_t j=0, jend=vbInliers.size(); j<jend; j++)
                {
                    if(vbInliers[j])
                       vpMapPointMatches[j]=vvpMapPointMatches[i][j];
                }

                cv::Mat R = pSolver->GetEstimatedRotation();
                cv::Mat t = pSolver->GetEstimatedTranslation();
                const float s = pSolver->GetEstimatedScale();
                matcher.SearchBySim3(mpCurrentKF,pKF,vpMapPointMatches,s,R,t,7.5);

                g2o::Sim3 gScm(Converter::toMatrix3d(R),Converter::toVector3d(t),s);
                const int nInliers = Optimizer::OptimizeSim3(mpCurrentKF, pKF, vpMapPointMatches, gScm, 10, mbFixScale);

                // If optimization is succesful stop ransacs and continue
                if(nInliers>=20)
                {
                    bMatch = true;
                    mpMatchedKF = pKF;
                    g2o::Sim3 gSmw(Converter::toMatrix3d(pKF->GetRotation()),Converter::toVector3d(pKF->GetTranslation()),1.0);
                    mg2oScw = gScm*gSmw;
                    mScw = Converter::toCvMat(mg2oScw);

                    mvpCurrentMatchedPoints = vpMapPointMatches;
                    break;
                }
            }
        }
    }

    if(!bMatch)
    {
        for(int i=0; i<nInitialCandidates; i++)
             mvpEnoughConsistentCandidates[i]->SetErase();
        mpCurrentKF->SetErase();
        return false;
    }

    // Retrieve MapPoints seen in Loop Keyframe and neighbors
    vector<KeyFrame*> vpLoopConnectedKFs = mpMatchedKF->GetVectorCovisibleKeyFrames();
    vpLoopConnectedKFs.push_back(mpMatchedKF);
    mvpLoopMapPoints.clear();
    
    Map::ReadGuard rg(mpMap);
    for(vector<KeyFrame*>::iterator vit=vpLoopConnectedKFs.begin(); vit!=vpLoopConnectedKFs.end(); vit++)
    {
        KeyFrame* pKF = *vit;
        vector<MapPoint*> vpMapPoints = pKF->GetMapPointMatches();
        for(size_t i=0, iend=vpMapPoints.size(); i<iend; i++)
        {
            MapPoint* pMP = vpMapPoints[i];
            if(pMP)
            {
                if(!pMP->isBad() && pMP->mnLoopPointForKF!=mpCurrentKF->mnId)
                {
                    mvpLoopMapPoints.push_back(pMP);
                    pMP->mnLoopPointForKF=mpCurrentKF->mnId;
                }
            }
        }
    }

    // Find more matches projecting with the computed Sim3
    matcher.SearchByProjection(mpCurrentKF, mScw, mvpLoopMapPoints, mvpCurrentMatchedPoints,10);

    // If enough matches accept Loop
    int nTotalMatches = 0;
    for(size_t i=0; i<mvpCurrentMatchedPoints.size(); i++)
    {
        if(mvpCurrentMatchedPoints[i])
            nTotalMatches++;
    }

    if(nTotalMatches>=40)
    {
        for(int i=0; i<nInitialCandidates; i++)
            if(mvpEnoughConsistentCandidates[i]!=mpMatchedKF)
                mvpEnoughConsistentCandidates[i]->SetErase();
        return true;
    }
    else
    {
        for(int i=0; i<nInitialCandidates; i++)
            mvpEnoughConsistentCandidates[i]->SetErase();
        mpCurrentKF->SetErase();
        return false;
    }

}

void LoopClosing::CorrectLoop()
{
    cout << "Loop detected!" << endl;

    // Stop Local Mapping while we correct
    mpLocalMapper->RequestStop();

    // Abort running GBA if any
    if (isRunningGBA())
    {
        unique_lock<mutex> lock(mMutexGBA);
        mbStopGBA = true;
        mnFullBAIdx += 1;
        if (mpThreadGBA)
        {
            mpThreadGBA->detach();
            delete mpThreadGBA;
            mpThreadGBA = nullptr;
        }
    }

    // Wait until Local Mapping has effectively stopped
    while (!mpLocalMapper->isStopped())
    {
        usleep(1000);
    }

    // update under read guard
    cv::Mat Twc_cur;
    KeyFrameAndPose CorrectedSim3, NonCorrectedSim3;
    {
        Map::ReadGuard rg(mpMap);

        // Ensure current KF is updated
        mpCurrentKF->UpdateConnections();

        // Connected KFs (current + covisibles)
        mvpCurrentConnectedKFs = mpCurrentKF->GetVectorCovisibleKeyFrames();
        mvpCurrentConnectedKFs.push_back(mpCurrentKF);

        // Sim3 containers
        CorrectedSim3[mpCurrentKF] = mg2oScw;

        // Twc(current) for Tic = Tiw * Twc
        Twc_cur = mpCurrentKF->GetPoseInverse();
    }
    // ===== PURE COMPUTATION (no map writes) =====

    // Fill CorrectedSim3 / NonCorrectedSim3 (read-only per-KF math)
    for (KeyFrame* pKFi : mvpCurrentConnectedKFs)
    {
        const cv::Mat Tiw = pKFi->GetPose();
        if (pKFi != mpCurrentKF)
        {
            const cv::Mat Tic = Tiw * Twc_cur;
            const cv::Mat Ric = Tic.rowRange(0,3).colRange(0,3);
            const cv::Mat tic = Tic.rowRange(0,3).col(3);
            g2o::Sim3 g2oSic(Converter::toMatrix3d(Ric), Converter::toVector3d(tic), 1.0);
            g2o::Sim3 g2oCorrectedSiw = g2oSic * mg2oScw;
            CorrectedSim3[pKFi] = g2oCorrectedSiw;
        }
        const cv::Mat Riw = Tiw.rowRange(0,3).colRange(0,3);
        const cv::Mat tiw = Tiw.rowRange(0,3).col(3);
        g2o::Sim3 g2oSiw(Converter::toMatrix3d(Riw), Converter::toVector3d(tiw), 1.0);
        NonCorrectedSim3[pKFi] = g2oSiw;
    }

    // Convert corrected Sim3 → SE3 Tiw to apply later
    std::map<KeyFrame*, cv::Mat> correctedTiwByKF;
    for (auto &kv : CorrectedSim3)
    {
        KeyFrame* pKFi = kv.first;
        const g2o::Sim3& g2oCorrectedSiw = kv.second;
        Eigen::Matrix3d eigR = g2oCorrectedSiw.rotation().toRotationMatrix();
        Eigen::Vector3d eigt = g2oCorrectedSiw.translation();
        const double s = g2oCorrectedSiw.scale();
        eigt *= (1.0 / s); // [R, t/s]
        correctedTiwByKF[pKFi] = Converter::toCvSE3(eigR, eigt);
    }

    // Precompute all MapPoint new positions
    struct MPUpdate { MapPoint* pMP; cv::Mat newPos; KeyFrame* refKF; };
    std::vector<MPUpdate> pendingMPUpdates;
    pendingMPUpdates.reserve(1024);
    for (auto &kv : CorrectedSim3)
    {
        KeyFrame* pKFi = kv.first;
        const g2o::Sim3& g2oCorrectedSiw = kv.second;
        const g2o::Sim3  g2oCorrectedSwi = g2oCorrectedSiw.inverse();
        const g2o::Sim3& g2oSiw          = NonCorrectedSim3[pKFi];

        const std::vector<MapPoint*> vpMPsi = pKFi->GetMapPointMatches();
        for (MapPoint* pMPi : vpMPsi)
        {
            if (!pMPi || pMPi->isBad()) continue;
            const cv::Mat  P3Dw       = pMPi->GetWorldPos();
            const auto     eigP3Dw    = Converter::toVector3d(P3Dw);
            const auto     eigNewP3Dw = g2oCorrectedSwi.map(g2oSiw.map(eigP3Dw));
            pendingMPUpdates.push_back({ pMPi, Converter::toCvMat(eigNewP3Dw), pKFi });
        }
    }

    // ===== APPLY MUTATIONS (short lock) =====
    {
        unique_lock<shared_mutex> lock(mpMap->mMutexMapUpdate);

        // MapPoints
        for (const MPUpdate& up : pendingMPUpdates)
        {
            MapPoint* pMPi = up.pMP;
            if (!pMPi || pMPi->isBad()) continue;
            if (pMPi->mnCorrectedByKF == mpCurrentKF->mnId) continue;
            pMPi->SetWorldPos(up.newPos);
            pMPi->mnCorrectedByKF      = mpCurrentKF->mnId;
            pMPi->mnCorrectedReference = up.refKF->mnId;
            pMPi->UpdateNormalAndDepth();
        }

        // KeyFrames
        for (auto &kvSE3 : correctedTiwByKF)
        {
            KeyFrame* pKFi = kvSE3.first;
            const cv::Mat& correctedTiw = kvSE3.second;
            pKFi->SetPose(correctedTiw);
            pKFi->UpdateConnections();
        }

        // Loop Fusion: update/replace matched points
        for (size_t i = 0; i < mvpCurrentMatchedPoints.size(); i++)
        {
            if (mvpCurrentMatchedPoints[i])
            {
                MapPoint* pLoopMP = mvpCurrentMatchedPoints[i];
                MapPoint* pCurMP  = mpCurrentKF->GetMapPoint(i);
                if (pCurMP)
                    pCurMP->Replace(pLoopMP);
                else
                {
                    mpCurrentKF->AddMapPoint(pLoopMP, i);
                    pLoopMP->AddObservation(mpCurrentKF, i);
                    pLoopMP->ComputeDistinctiveDescriptors();
                }
            }
        }
    }

    // Project & fuse neighbors using corrected poses
    SearchAndFuse(CorrectedSim3);

    // Build LoopConnections (new links only)
    std::map<KeyFrame*, std::set<KeyFrame*>> LoopConnections;
    for (vector<KeyFrame*>::iterator vit = mvpCurrentConnectedKFs.begin(), vend = mvpCurrentConnectedKFs.end(); vit != vend; vit++)
    {
        KeyFrame* pKFi = *vit;
        vector<KeyFrame*> vpPreviousNeighbors = pKFi->GetVectorCovisibleKeyFrames();

        {
            Map::ReadGuard rg(mpMap);
            // After corrections & fusion, UpdateConnections then subtract previous neighbors and connected set
            pKFi->UpdateConnections();
            LoopConnections[pKFi] = pKFi->GetConnectedKeyFrames();
        }

        for (vector<KeyFrame*>::iterator vit_prev = vpPreviousNeighbors.begin(), vend_prev = vpPreviousNeighbors.end(); vit_prev != vend_prev; vit_prev++)
            LoopConnections[pKFi].erase(*vit_prev);

        for (vector<KeyFrame*>::iterator vit2 = mvpCurrentConnectedKFs.begin(), vend2 = mvpCurrentConnectedKFs.end(); vit2 != vend2; vit2++)
            LoopConnections[pKFi].erase(*vit2);
    }

    // Optimize essential graph with new loop links
    Optimizer::OptimizeEssentialGraph(mpMap, mpMatchedKF, mpCurrentKF,
                                      NonCorrectedSim3, CorrectedSim3, LoopConnections, mbFixScale);

    mpMap->InformNewBigChange();

    // Add loop edge
    mpMatchedKF->AddLoopEdge(mpCurrentKF);
    mpCurrentKF->AddLoopEdge(mpMatchedKF);

    // Launch GBA
    mbRunningGBA = true; mbFinishedGBA = false; mbStopGBA = false;
    mpThreadGBA = new thread(&LoopClosing::RunGlobalBundleAdjustment, this, mpCurrentKF->mnId);

    // Release Local Mapping
    mpLocalMapper->Release();
    mLastLoopKFid = mpCurrentKF->mnId;
}

void LoopClosing::SearchAndFuse(const KeyFrameAndPose &CorrectedPosesMap)
{
    ORBmatcher matcher(0.8);

    for(KeyFrameAndPose::const_iterator mit=CorrectedPosesMap.begin(), mend=CorrectedPosesMap.end(); mit!=mend;mit++)
    {
        KeyFrame* pKF = mit->first;

        g2o::Sim3 g2oScw = mit->second;
        cv::Mat cvScw = Converter::toCvMat(g2oScw);

        vector<MapPoint*> vpReplacePoints(mvpLoopMapPoints.size(),static_cast<MapPoint*>(NULL));
        matcher.Fuse(pKF,cvScw,mvpLoopMapPoints,4,vpReplacePoints);

        // Get Map Mutex
        unique_lock<shared_mutex> lock(mpMap->mMutexMapUpdate);
        const int nLP = mvpLoopMapPoints.size();
        for(int i=0; i<nLP;i++)
        {
            MapPoint* pRep = vpReplacePoints[i];
            if(pRep)
            {
                pRep->Replace(mvpLoopMapPoints[i]);
            }
        }
    }
}


void LoopClosing::RequestReset()
{
    {
        unique_lock<mutex> lock(mMutexReset);
        mbResetRequested = true;
    }

    while(1)
    {
        if (CheckFinish()) break;
        {
        unique_lock<mutex> lock2(mMutexReset);
        if(!mbResetRequested)
            break;
        }
        usleep(5000);
    }
}

void LoopClosing::ResetIfRequested()
{
    unique_lock<mutex> lock(mMutexReset);
    if(mbResetRequested)
    {
        mlpLoopKeyFrameQueue.clear();
        mLastLoopKFid=0;
        mbResetRequested=false;
    }
}

void LoopClosing::RunGlobalBundleAdjustment(unsigned long nLoopKF)
{
    cout << "Starting Global Bundle Adjustment" << endl;

    const int idx = mnFullBAIdx;
    Optimizer::GlobalBundleAdjustemnt(mpMap, 10, &mbStopGBA, nLoopKF, false);

    // For logging
    std::set<KeyFrame*> sBAKF;
    std::set<MapPoint*> sBAMP;

    // Update all MapPoints and KeyFrames
    // Local Mapping was active during BA; propagate correction through the spanning tree.
    {
        unique_lock<mutex> lock(mMutexGBA);
        if (idx != mnFullBAIdx) return;

        if (!mbStopGBA)
        {
            cout << "Global Bundle Adjustment finished" << endl;
            cout << "Updating map ..." << endl;

            mpLocalMapper->RequestStop();
            while (!mpLocalMapper->isStopped() && !mpLocalMapper->isFinished())
                usleep(1000);

            // ===== PURE COMPUTATION: collect updates outside map lock =====

            // Propagate corrected poses (BFS over origins) and collect new Tcw for each KF
            std::vector<std::pair<KeyFrame*, cv::Mat>> kfPoseUpdates;
            kfPoseUpdates.reserve(256);

            std::list<KeyFrame*> lpKFtoCheck(mpMap->mvpKeyFrameOrigins.begin(), mpMap->mvpKeyFrameOrigins.end());
            while (!lpKFtoCheck.empty())
            {
                KeyFrame* pKF = lpKFtoCheck.front();
                const set<KeyFrame*> sChilds = pKF->GetChilds();
                const cv::Mat Twc = pKF->GetPoseInverse();
                for (KeyFrame* pChild : sChilds)
                {
                    if (pChild->mnBAGlobalForKF != nLoopKF)
                    {
                        const cv::Mat Tchildc = pChild->GetPose() * Twc;
                        pChild->mTcwGBA = Tchildc * pKF->mTcwGBA;
                        pChild->mnBAGlobalForKF = nLoopKF;
                    }
                    lpKFtoCheck.push_back(pChild);
                }
                pKF->mTcwBefGBA = pKF->GetPose();
                kfPoseUpdates.emplace_back(pKF, pKF->mTcwGBA);
                lpKFtoCheck.pop_front();
            }

            // Precompute MapPoint position updates (either mPosGBA or propagated via ref KF)
            std::vector<std::pair<MapPoint*, cv::Mat>> mpPosUpdates;
            {
                const std::vector<MapPoint*> vpMPs = mpMap->GetAllMapPoints();
                mpPosUpdates.reserve(vpMPs.size());
                for (MapPoint* pMP : vpMPs)
                {
                    if (!pMP || pMP->isBad()) continue;

                    if (pMP->mnBAGlobalForKF == nLoopKF)
                    {
                        mpPosUpdates.emplace_back(pMP, pMP->mPosGBA);
                        continue;
                    }

                    KeyFrame* pRefKF = pMP->GetReferenceKeyFrame();
                    if (!pRefKF || pRefKF->mnBAGlobalForKF != nLoopKF) continue;

                    // Map to pre-BA camera
                    const cv::Mat Rcw = pRefKF->mTcwBefGBA.rowRange(0,3).colRange(0,3);
                    const cv::Mat tcw = pRefKF->mTcwBefGBA.rowRange(0,3).col(3);
                    const cv::Mat Xc  = Rcw * pMP->GetWorldPos() + tcw;

                    // Backproject using corrected camera
                    const cv::Mat Twc = pRefKF->GetPoseInverse();
                    const cv::Mat Rwc = Twc.rowRange(0,3).colRange(0,3);
                    const cv::Mat twc = Twc.rowRange(0,3).col(3);

                    mpPosUpdates.emplace_back(pMP, Rwc * Xc + twc);
                    sBAMP.insert(pMP);
                }
            }

            // ===== APPLY MUTATIONS (short lock) =====
            {
                unique_lock<shared_mutex> lockMap(mpMap->mMutexMapUpdate);

                // KeyFrames
                for (auto &kv : kfPoseUpdates)
                {
                    KeyFrame* pKF = kv.first;
                    pKF->SetPose(kv.second);
                    sBAKF.insert(pKF);
                }

                // MapPoints
                for (auto &kv : mpPosUpdates)
                    kv.first->SetWorldPos(kv.second);

                // Log/update
                mpMap->mpModeler->AddAdjustmentEntry(sBAKF, sBAMP);
                mpMap->InformNewBigChange();
                mpLocalMapper->Release();

                cout << "Map updated!" << endl;
            }
        }
    }

    mbFinishedGBA = true;
    mbRunningGBA  = false;
}

void LoopClosing::RequestFinish()
{
    unique_lock<mutex> lock(mMutexFinish);
    mbFinishRequested = true;
}

bool LoopClosing::CheckFinish()
{
    unique_lock<mutex> lock(mMutexFinish);
    return mbFinishRequested;
}

void LoopClosing::SetFinish()
{
    unique_lock<mutex> lock(mMutexFinish);
    mbFinished = true;
}

bool LoopClosing::isFinished()
{
    unique_lock<mutex> lock(mMutexFinish);
    return mbFinished;
}



bool LoopClosing::Stop()
{
    std::unique_lock<std::mutex> lock(mMutexStop);
    if(mbStopRequested)
    {
        mbStopped = true;
        return true;
    }
    return false;
}

void LoopClosing::RequestStop()
{
    std::unique_lock<std::mutex> lock(mMutexStop);
    mbStopRequested = true;
}

bool LoopClosing::isStopped()
{
    std::unique_lock<std::mutex> lock(mMutexStop);
    return mbStopped;
}

bool LoopClosing::stopRequested()
{
    std::unique_lock<std::mutex> lock(mMutexStop);
    return mbStopRequested;
}

void LoopClosing::Release()
{
    std::unique_lock<std::mutex> lock(mMutexStop);
    if(mbFinished) return;
    mbStopped = false;
    mbStopRequested = false;
}

} //namespace ORB_SLAM
