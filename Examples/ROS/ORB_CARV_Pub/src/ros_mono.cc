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


#include<iostream>
#include<algorithm>
#include<fstream>
#include<chrono>
#include<mutex>

#include<ros/ros.h>
#include <std_msgs/Header.h>
#include "std_msgs/String.h"
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include<opencv2/core/core.hpp>

#include"../../../include/System.h"

#include "../../../include/KeyFrame.h"
// #include "../../../include/MapPoint.h"
// #include "../../../include/Converter.h"
// #include "../../../include/Map.h"
// #include "../../../include/MapPoint.h"

#include "../../../include/NetworkIntegration/HttpService.h"

using namespace std;



class ImageGrabber
{
public:
    ImageGrabber(ORB_SLAM2::System* pSLAM):mpSLAM(pSLAM){}

    void GrabImage(const sensor_msgs::ImageConstPtr& msg);

    ORB_SLAM2::System* mpSLAM;
};

int max_kfId;
ros::Publisher pubTask;
ros::Publisher pubCARVScripts;
int main(int argc, char **argv)
{
    max_kfId=0;

    ros::init(argc, argv, "Mono");
    ros::start();

    if(argc != 8)
    {
        cerr << endl << "Usage: rosrun ORB_CARV_Pub Mono path_to_vocabulary path_to_settings own_ip http_port unity_ip unity_zmq_port image_topic" << endl;
        ros::shutdown();
        return 1;
    }

    std::string ownIp = argv[3];
    int httpPort = std::stoi(argv[4]);
    std::string unityIp = argv[5];
    int zmqPort = std::stoi(argv[6]);
    std::string imageTopic = argv[7];

    std::string ownAddress = ownIp + ":" + std::to_string(httpPort);
    std::string unityAddress = unityIp + ":" + std::to_string(zmqPort);

    // Create ChunkCache and HTTP Server
    ChunkCache cache;
    HttpService server(httpPort, cache);
    server.start();

    // Create SLAM system. It initializes all system threads and gets ready to process frames.
    ORB_SLAM2::System SLAM(argv[1], argv[2], ORB_SLAM2::System::MONOCULAR, false);

    SLAM.SetNetworkingInfo(ownAddress, unityAddress, &cache);

    ImageGrabber igb(&SLAM);

    ros::NodeHandle nodeHandler;
    image_transport::ImageTransport it(nodeHandler);
    image_transport::TransportHints th("raw", ros::TransportHints().tcpNoDelay());
    image_transport::Subscriber sub = it.subscribe(imageTopic, 5, &ImageGrabber::GrabImage, &igb, th);

    pubTask = nodeHandler.advertise<std_msgs::String>("/chris/twc", 1);
    pubCARVScripts = nodeHandler.advertise<std_msgs::String>("/carv/script", 1);
    ros::spin();

    // Stop all threads
    SLAM.Shutdown();

    // Stop HTTP Server
    server.stop();

    // Save camera trajectory
    SLAM.SaveKeyFrameTrajectoryTUM("KeyFrameTrajectory.txt");

    ros::shutdown();

    return 0;
}


void ImageGrabber::GrabImage(const sensor_msgs::ImageConstPtr& msg)
{
    // Copy the ROS image to cv::Mat
    cv_bridge::CvImageConstPtr cv_ptr;
    try {
        cv_ptr = cv_bridge::toCvShare(msg);
    } catch (cv_bridge::Exception& e) {
        ROS_ERROR("cv_bridge exception: %s", e.what());
        return;
    }

    // Run tracking
    mpSLAM->TrackMonocular(cv_ptr->image, cv_ptr->header.stamp.toSec());

    // --- FIX: read newestKeyFrame under the same mutex writers use ---
    ORB_SLAM2::KeyFrame* pKF = nullptr;
    {
        std::unique_lock<std::mutex> lk(mpSLAM->mpMap->mMutexMap);
        pKF = mpSLAM->mpMap->newestKeyFrame;
    }

    if (pKF && pKF->mnId > max_kfId)
    {
        cv::Mat TWC = pKF->GetPoseInverse(); // thread-safe inside KeyFrame

        std_msgs::String msg;              // publish KF id, timestamp, pose
        std::stringstream ss;
        ss << pKF->mnId << ",";
        ss << std::setprecision(15) << pKF->mTimeStamp << ",";
        for (int r = 0; r < TWC.rows; ++r)
            for (int c = 0; c < TWC.cols; ++c)
                ss << TWC.at<float>(r, c) << ",";

        msg.data = ss.str();
        pubTask.publish(msg);

        max_kfId = pKF->mnId;              // update last-published id
    }

    // Publish CARV transcript safely (Modeler provides its own lock)
    std_msgs::String msgScript;
    msgScript.data = mpSLAM->mpModeler->GetNewCommand();
    if (!msgScript.data.empty())
        pubCARVScripts.publish(msgScript);
}