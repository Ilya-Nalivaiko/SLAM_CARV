# SLAM_CARV
newer version of SLAM and CARV engine

# Personal Notes (To Edit)
Export command 
```
export ROS_PACKAGE_PATH=${ROS_PACKAGE_PATH}:~/vr_ws/SLAM_CARV/Examples/ROS
```

# Differences from original repo
1. Working with c++14
2. removed build folders being in git repo for cleaner build.
3. Applied some fixes to the code from the original ORB-SLAM2
4. suppress all warnings so that we can only see errors and not warnings from deprecation in eigen.
5. Make a ```config_files``` directory, that has all yaml files and configuration files.
6. Change ```crono::monotonic_clock``` to ```chrono::steady_clock```

# Compilation Edits
1. Install ros melodic (Tutorial [Here](https://wiki.ros.org/melodic/Installation/Ubuntu]) )
2. Install ORB-SLAM2 dependencies
3. Install CGAL ``` sudo apt-get install libcgal-dev ```
4. Edit from original repo [Here](https://github.com/atlas-jj/ORB-SLAM-free-space-carving/tree/master)
   - Move the code to work with c++14 in order to avoid the error of not having proper access to ```std::decay_t```
5. change permission for build file of ORB-SLAM2 ```chmod +x ./build.sh```
6. build ORB-SLAM2 ```./build.sh```
7. change permission for build file of ROS modules ```chmod +x ./build_ros.sh```
8. build ros modules ```./build_ros.sh```

# Steps to run
1. Make sure that the usb camera ros package is installed in the docker file and it is working
```
# in a terminal (To start ros)
roscore

# Then in another terminal do the following
# ------------------------------------------
# To install (if not installed from the Docker file)
sudo apt-get install ros-kinetic-usb-cam

# To run and test 
roslaunch usb_cam usb_cam-test.launch
```