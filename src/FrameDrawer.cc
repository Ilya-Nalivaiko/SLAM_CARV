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

#include "FrameDrawer.h"
#include "Tracking.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include<mutex>

namespace ORB_SLAM2
{

FrameDrawer::FrameDrawer(Map* pMap):mpMap(pMap)
{
    mState=Tracking::SYSTEM_NOT_READY;
    mIm = cv::Mat(480,640,CV_8UC3, cv::Scalar(0,0,0));
}

cv::Mat FrameDrawer::DrawFrame()
{
    cv::Mat imBGR;                         // final image we draw on (BGR)
    std::vector<cv::KeyPoint> vIniKeys;    // Initialization: ref frame keypoints
    std::vector<int> vMatches;             // Initialization matches
    std::vector<cv::KeyPoint> vCurrentKeys;
    std::vector<bool> vbVO, vbMap;
    int state;
    bool onlyTracking = false;             // <-- capture under lock

    // Snapshot all state with a single lock. Convert to BGR directly to avoid an
    // extra deep copy; this keeps the lock held for the conversion only.
    {
        std::unique_lock<std::mutex> lock(mMutex);

        state = mState;
        if (mState == Tracking::SYSTEM_NOT_READY)
            mState = Tracking::NO_IMAGES_YET;

        // capture the flag while locked (fixes TSAN race)
        onlyTracking = mbOnlyTracking;

        if (mIm.channels() == 1)
            cv::cvtColor(mIm, imBGR, cv::COLOR_GRAY2BGR);
        else
            mIm.copyTo(imBGR);

        if (mState == Tracking::NOT_INITIALIZED)
        {
            vCurrentKeys = mvCurrentKeys;
            vIniKeys     = mvIniKeys;
            vMatches     = mvIniMatches;
        }
        else if (mState == Tracking::OK)
        {
            vCurrentKeys = mvCurrentKeys;
            vbVO         = mvbVO;
            vbMap        = mvbMap;
        }
        else if (mState == Tracking::LOST)
        {
            vCurrentKeys = mvCurrentKeys;
        }
    } // unlock — everything below is on local copies

    // Draw features
    if (state == Tracking::NOT_INITIALIZED)
    {
        for (size_t i = 0; i < vMatches.size(); ++i)
        {
            if (vMatches[i] >= 0)
                cv::line(imBGR, vIniKeys[i].pt, vCurrentKeys[vMatches[i]].pt, cv::Scalar(0,255,0));
        }
    }
    else if (state == Tracking::OK)
    {
        mnTracked   = 0;
        mnTrackedVO = 0;
        const float r = 5.f;
        const int n = static_cast<int>(vCurrentKeys.size());
        for (int i = 0; i < n; ++i)
        {
            if (vbVO[i] || vbMap[i])
            {
                cv::Point2f pt1(vCurrentKeys[i].pt.x - r, vCurrentKeys[i].pt.y - r);
                cv::Point2f pt2(vCurrentKeys[i].pt.x + r, vCurrentKeys[i].pt.y + r);

                if (vbMap[i]) {
                    cv::rectangle(imBGR, pt1, pt2, cv::Scalar(0,255,0));
                    cv::circle(imBGR, vCurrentKeys[i].pt, 2, cv::Scalar(0,255,0), -1);
                    ++mnTracked;
                } else {
                    cv::rectangle(imBGR, pt1, pt2, cv::Scalar(255,0,0));
                    cv::circle(imBGR, vCurrentKeys[i].pt, 2, cv::Scalar(255,0,0), -1);
                    ++mnTrackedVO;
                }
            }
        }
    }

    // Status text bar
    std::stringstream ss;
    if (state == Tracking::NO_IMAGES_YET)         ss << " WAITING FOR IMAGES";
    else if (state == Tracking::NOT_INITIALIZED)  ss << " TRYING TO INITIALIZE ";
    else if (state == Tracking::OK) {
        if (!onlyTracking) ss << "SLAM MODE |  ";
        else               ss << "LOCALIZATION | ";
        const int nKFs = mpMap->KeyFramesInMap();
        const int nMPs = mpMap->MapPointsInMap();
        ss << "KFs: " << nKFs << ", MPs: " << nMPs << ", Matches: " << mnTracked;
        if (mnTrackedVO > 0) ss << ", + VO matches: " << mnTrackedVO;
    }
    else if (state == Tracking::LOST)             ss << " TRACK LOST. TRYING TO RELOCALIZE ";
    else if (state == Tracking::SYSTEM_NOT_READY) ss << " LOADING ORB VOCABULARY. PLEASE WAIT...";

    int baseline = 0;
    const cv::Size textSize = cv::getTextSize(ss.str(), cv::FONT_HERSHEY_PLAIN, 1, 1, &baseline);
    const int barH = textSize.height + 10;
    const int y0   = std::max(0, imBGR.rows - barH);

    cv::rectangle(imBGR, cv::Rect(0, y0, imBGR.cols, barH), cv::Scalar(0,0,0), cv::FILLED);
    cv::putText(imBGR, ss.str(), cv::Point(5, imBGR.rows - 5),
                cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(255,255,255), 1, 8);

    return imBGR;
}


void FrameDrawer::DrawTextInfo(cv::Mat &im, int nState, cv::Mat &imText)
{
    stringstream s;
    if(nState==Tracking::NO_IMAGES_YET)
        s << " WAITING FOR IMAGES";
    else if(nState==Tracking::NOT_INITIALIZED)
        s << " TRYING TO INITIALIZE ";
    else if(nState==Tracking::OK)
    {
        if(!mbOnlyTracking)
            s << "SLAM MODE |  ";
        else
            s << "LOCALIZATION | ";
        int nKFs = mpMap->KeyFramesInMap();
        int nMPs = mpMap->MapPointsInMap();
        s << "KFs: " << nKFs << ", MPs: " << nMPs << ", Matches: " << mnTracked;
        if(mnTrackedVO>0)
            s << ", + VO matches: " << mnTrackedVO;
    }
    else if(nState==Tracking::LOST)
    {
        s << " TRACK LOST. TRYING TO RELOCALIZE ";
    }
    else if(nState==Tracking::SYSTEM_NOT_READY)
    {
        s << " LOADING ORB VOCABULARY. PLEASE WAIT...";
    }

    int baseline=0;
    cv::Size textSize = cv::getTextSize(s.str(),cv::FONT_HERSHEY_PLAIN,1,1,&baseline);

    imText = cv::Mat(im.rows+textSize.height+10,im.cols,im.type());
    im.copyTo(imText.rowRange(0,im.rows).colRange(0,im.cols));
    imText.rowRange(im.rows,imText.rows) = cv::Mat::zeros(textSize.height+10,im.cols,im.type());
    cv::putText(imText,s.str(),cv::Point(5,imText.rows-5),cv::FONT_HERSHEY_PLAIN,1,cv::Scalar(255,255,255),1,8);

}

void FrameDrawer::Update(Tracking *pTracker)
{
    unique_lock<mutex> lock(mMutex);
    mIm = pTracker->mImGray.clone();
    mvCurrentKeys=pTracker->mCurrentFrame.mvKeys;
    N = mvCurrentKeys.size();
    mvbVO = vector<bool>(N,false);
    mvbMap = vector<bool>(N,false);
    mbOnlyTracking = pTracker->mbOnlyTracking;


    if(pTracker->mLastProcessedState==Tracking::NOT_INITIALIZED)
    {
        mvIniKeys=pTracker->mInitialFrame.mvKeys;
        mvIniMatches=pTracker->mvIniMatches;
    }
    else if(pTracker->mLastProcessedState==Tracking::OK)
    {
        Map::ReadGuard rg(mpMap);
        for(int i=0;i<N;i++)
        {
            MapPoint* pMP = pTracker->mCurrentFrame.mvpMapPoints[i];
            if(pMP)
            {
                if(!pTracker->mCurrentFrame.mvbOutlier[i])
                {
                    if(pMP->Observations()>0)
                        mvbMap[i]=true;
                    else
                        mvbVO[i]=true;
                }
            }
        }
    }
    mState=static_cast<int>(pTracker->mLastProcessedState);
}

} //namespace ORB_SLAM
