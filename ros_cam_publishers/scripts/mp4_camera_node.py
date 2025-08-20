#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import rospy
import cv2
from sensor_msgs.msg import Image, CameraInfo
from cv_bridge import CvBridge

def main():
    rospy.init_node("video_file_camera_node")
    pub_img = rospy.Publisher("/camera/image_raw", Image, queue_size=1)
    pub_info = rospy.Publisher("/camera/camera_info", CameraInfo, queue_size=1, latch=True)
    bridge = CvBridge()

    # Params
    path = rospy.get_param("~file", "/path/to/video.mp4")
    fps_param = rospy.get_param("~fps", 0.0)           # 0 => use file FPS if available
    loop = rospy.get_param("~loop", True)
    frame_id = rospy.get_param("~frame_id", "camera")
    resize_w = rospy.get_param("~resize_width", 0)     # 0 => no resize
    resize_h = rospy.get_param("~resize_height", 0)

    cap = cv2.VideoCapture(path)
    if not cap.isOpened():
        rospy.logerr("Failed to open video file at {0}".format(path))
        return

    # Discover properties
    file_fps = cap.get(cv2.CAP_PROP_FPS)
    if not file_fps or file_fps <= 1e-3:
        file_fps = 30.0  # fallback
    run_fps = fps_param if fps_param and fps_param > 0 else file_fps
    rate = rospy.Rate(run_fps)

    total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT)) if cap.get(cv2.CAP_PROP_FRAME_COUNT) > 0 else -1

    # Publish a basic CameraInfo once (optional; width/height updated on first frame)
    cam_info = CameraInfo()
    cam_info.header.frame_id = frame_id

    rospy.loginfo("Playing video {0} at {1:.2f} FPS (loop={2})".format(path, run_fps, loop))

    first_info_sent = False
    while not rospy.is_shutdown():
        ret, frame = cap.read()
        if not ret:
            if loop:
                # Seek to start and continue
                cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
                continue
            else:
                rospy.loginfo("End of video reached; exiting.")
                break

        if resize_w > 0 and resize_h > 0:
            frame = cv2.resize(frame, (int(resize_w), int(resize_h)), interpolation=cv2.INTER_AREA)

        # Fill CameraInfo (once we know frame size)
        if not first_info_sent:
            cam_info.width = frame.shape[1]
            cam_info.height = frame.shape[0]
            pub_info.publish(cam_info)
            first_info_sent = True

        # Publish image
        stamp = rospy.Time.now()
        msg = bridge.cv2_to_imgmsg(frame, encoding="bgr8")
        msg.header.stamp = stamp
        msg.header.frame_id = frame_id
        pub_img.publish(msg)

        # Update and republish CameraInfo with current timestamp for sync
        cam_info.header.stamp = stamp
        pub_info.publish(cam_info)

        rate.sleep()

    cap.release()

if __name__ == "__main__":
    main()
