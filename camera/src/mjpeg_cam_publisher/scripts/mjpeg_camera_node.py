#!/usr/bin/env python
# -*- coding: utf-8 -*-
import rospy
import cv2
import numpy as np
from sensor_msgs.msg import Image
from cv_bridge import CvBridge

MJPEG_URL = "http://192.168.1.109:4747/video"  # Replace with your phone's IP

def main():
    rospy.init_node("mjpeg_camera_node")
    pub = rospy.Publisher("/camera/image_raw", Image, queue_size=1)
    bridge = CvBridge()

    cap = cv2.VideoCapture(MJPEG_URL)

    if not cap.isOpened():
        rospy.logerr("Failed to open MJPEG stream")
        return

    rate = rospy.Rate(30)
    while not rospy.is_shutdown():
        ret, frame = cap.read()
        if not ret:
            rospy.logwarn("Failed to read frame")
            continue

        msg = bridge.cv2_to_imgmsg(frame, encoding="bgr8")
        pub.publish(msg)
        rate.sleep()

if __name__ == "__main__":
    main()
