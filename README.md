# SLAM_CARV
newer version of SLAM and CARV engine with network model export

# Resources
FLIR camera setup with ros and linux
[Here](https://www.flir.ca/support-center/iis/machine-vision/application-note/using-ros-with-spinnaker/)

# Differences from original repo
1. Added functionality to share the rendered model over the network, to view in an external program
2. Support for HTTP published jpeg streams as camera feed (works well with DroidCam)
3. Working with c++17, and many updated dependencies
4. Modified compile workflow for faster code iteration
5. See the upstream repo's changelog for more differences from the original SLAM CARV engine

# Steps to run

## The Docker container
1. This code runs in a Docker container. Other than having the Docker engine installed and downloading this repository, there are no required dependencies
 ```
 $ ./docker_build.sh
 ```
2. Now you should be able to run the docker image with VNC capabilities. Ensure the parameters in run.sh are correct: (TODO explain args)
```
$ cd Docker
$ ./run.sh
```
3. In any VNC viewer, connect to the URL below. The password is "password"
```
localhost:5900
```
4. right click anywhere, and open a shell. You will need several of these
```
Application >> Shells >> Bash
```

## Camera Inpus
This code supports multiple camera inputs. Choose one of the below that suits you best

1. To use a traditional USB camera (such as a webcam), (TODO)

2. To use a phone through Droidcam, change the IP in video.sh, then run the below commands in **two separate** shells
```
$ roscore
$ ./video.sh
```

## Running CARV
1. In a new bash shell, run CARV
```
$ ./run.sh
```
2. ORB-SLAM2 will load the vocabulary dictionary for DBoW and then you should see that the SLAM window and SLAM image feed is working.
3. You will notice on the camera feed green tracking dots appearing. I recommend holding the camera steady watching a detailed surface, such as a keyboard, to have something to latch on to, then **slowly** looking around

## Remotely viewing the model
To export the model over the network,

(TODO update this)

1. press SEND_MODEL and use the Unity viewer (TODO add it to the repo?)




# Setting up the workspace

TODO update all of this for the new dependencies?


1. Install ros melodic (Tutorial [Here](https://wiki.ros.org/melodic/Installation/Ubuntu]) )
2. Install ORB-SLAM2 dependencies
3. Install CGAL ``` sudo apt-get install libcgal-dev ```
4. Edit from original repo [Here](https://github.com/atlas-jj/ORB-SLAM-free-space-carving/tree/master)
   - Move the code to work with c++14 in order to avoid the error of not having proper access to ```std::decay_t```
5. change permission for build file of ORB-SLAM2 ```chmod +x ./build.sh```
6. build ORB-SLAM2 ```./build.sh```
7. change permission for build file of ROS modules ```chmod +x ./build_ros.sh```
8. build ros modules ```./build_ros.sh```
