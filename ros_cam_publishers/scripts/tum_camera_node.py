#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import rospy, cv2, os, glob, re, time
from sensor_msgs.msg import Image, CameraInfo
from cv_bridge import CvBridge

TS_RE = re.compile(r'(?P<ts>\d+\.\d+)\.(png|jpg|jpeg|bmp|tiff)$', re.IGNORECASE)

def load_tum_list(list_file, base_dir=None):
    items = []
    with open(list_file, 'r') as f:
        for line in f:
            line=line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split()
            if len(parts) >= 2:
                ts = float(parts[0])
                path = parts[1]
                if base_dir and not os.path.isabs(path):
                    path = os.path.join(base_dir, path)
                items.append((ts, path))
    return items

def load_from_filenames(directory, pattern):
    files = sorted(glob.glob(os.path.join(directory, pattern)))
    seq = []
    for fp in files:
        name = os.path.basename(fp)
        m = TS_RE.search(name)
        if not m:
            continue
        ts = float(m.group('ts'))
        seq.append((ts, fp))
    # sort by numeric timestamp (don’t trust lexicographic only)
    seq.sort(key=lambda x: x[0])
    return seq

def main():
    rospy.init_node("image_sequence_camera_node")
    pub_img  = rospy.Publisher("/camera/image_raw", Image, queue_size=1)
    pub_info = rospy.Publisher("/camera/camera_info", CameraInfo, queue_size=1, latch=True)
    bridge = CvBridge()

    # Params
    directory   = rospy.get_param("~dir", "")
    list_file   = rospy.get_param("~list_file", "")  # optional TUM rgb.txt
    pattern     = rospy.get_param("~pattern", "*.png")
    loop        = rospy.get_param("~loop", True)
    frame_id    = rospy.get_param("~frame_id", "camera")
    resize_w    = rospy.get_param("~resize_width", 0)
    resize_h    = rospy.get_param("~resize_height", 0)
    time_scale  = rospy.get_param("~time_scale", 1.0)  # >1 faster, <1 slower

    seq = []
    if list_file:
        base = directory if directory else os.path.dirname(list_file)
        seq = load_tum_list(list_file, base_dir=base)
        source_desc = "rgb.txt"
    else:
        if not directory:
            rospy.logerr("Provide ~dir (and optionally ~pattern), or a ~list_file.")
            return
        seq = load_from_filenames(directory, pattern)
        source_desc = "filenames"

    if len(seq) == 0:
        rospy.logerr("No frames found (dir=%s, pattern=%s, list_file=%s)", directory, pattern, list_file)
        return

    # Normalize to t=0 at first frame timestamp
    t0 = seq[0][0]
    seq = [ (ts - t0, path) for (ts, path) in seq ]
    rospy.loginfo("Loaded %d frames from %s. First ts=0.000", len(seq), source_desc)

    cam_info = CameraInfo()
    cam_info.header.frame_id = frame_id
    first_info_sent = False

    idx = 0
    start_wall = time.time()
    while not rospy.is_shutdown():
        t_rel, path = seq[idx]

        # Target wall time for this frame
        target_wall = start_wall + (t_rel / max(1e-9, time_scale))
        now = time.time()
        if target_wall > now:
            rospy.sleep(target_wall - now)

        img = cv2.imread(path, cv2.IMREAD_COLOR)
        if img is None:
            rospy.logwarn("Failed to read %s (skipping)", path)
        else:
            if resize_w > 0 and resize_h > 0:
                img = cv2.resize(img, (int(resize_w), int(resize_h)), interpolation=cv2.INTER_AREA)

            stamp = rospy.Time.now()

            if not first_info_sent:
                cam_info.width  = img.shape[1]
                cam_info.height = img.shape[0]
                pub_info.publish(cam_info)
                first_info_sent = True

            msg = bridge.cv2_to_imgmsg(img, encoding="bgr8")
            msg.header.stamp = stamp
            msg.header.frame_id = frame_id
            pub_img.publish(msg)

            cam_info.header.stamp = stamp
            pub_info.publish(cam_info)

        idx += 1
        if idx >= len(seq):
            if loop:
                idx = 0
                start_wall = time.time()
            else:
                rospy.loginfo("End of sequence; exiting.")
                break

if __name__ == "__main__":
    main()
