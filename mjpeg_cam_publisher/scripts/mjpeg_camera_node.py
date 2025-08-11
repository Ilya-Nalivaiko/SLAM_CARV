#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import rospy
import cv2
from sensor_msgs.msg import Image
from cv_bridge import CvBridge

def main():
    rospy.init_node("mjpeg_camera_node")
    pub = rospy.Publisher("/camera/image_raw", Image, queue_size=1)
    bridge = CvBridge()

    # Get the IP parameter, default fallback if not set
    ip_addr = rospy.get_param("~ip", "192.168.1.72")
    url = "http://{0}:4747/video".format(ip_addr)
    rospy.loginfo("Connecting to MJPEG stream at {0}".format(url)) # need to use .format because this runs on python 3.5 (before f strings)

    cap = cv2.VideoCapture(url)
    if not cap.isOpened():
        rospy.logerr("Failed to open MJPEG stream at {0}".format(url))
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
