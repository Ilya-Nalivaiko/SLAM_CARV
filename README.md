# SLAM_CARV
newer version of SLAM and CARV engine

# Personal Notes (To Edit)
Export command 
```
export ROS_PACKAGE_PATH=${ROS_PACKAGE_PATH}:~/vr_ws/ORB-SLAM-free-space-carving/Examples/ROS
```
# Compilation Edits
1. Install ros melodic (Tutorial [Here](https://wiki.ros.org/melodic/Installation/Ubuntu]) )
2. Install ORB-SLAM2 dependencies
3. Install CGAL ``` sudo apt-get install libcgal-dev ```
4. Edit from original repo [Here](https://github.com/atlas-jj/ORB-SLAM-free-space-carving/tree/master)
   - Move the code to work with c++14 in order to avoid the error of not having proper access to ```std::decay_t```
5. 
