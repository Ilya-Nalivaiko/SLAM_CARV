# SLAM_CARV
newer version of SLAM and CARV engine with updated base system and network model export

# Differences from original repo
1. Added functionality to share the rendered model over the network, to view in an external program
2. Support for HTTP published mjpeg streams as camera feed (works well with DroidCam)
3. Updated base system to Ubuntu 20.04 (From 16.04), and with it most dependencies (OpenCV 4.2, newer compiler and build system, etc)
4. Faster build process, and Dev Container dependency isolation
5. See the upstream repo's changelog for more differences from the original SLAM CARV engine

# Steps to run

## The Docker container
1. This code runs in a Docker container. Other than having the Docker engine installed and downloading this repository, there are no required dependencies
 ```sh
 $ ./docker_build.sh
 ```
2. Now you should be able to run the docker image with VNC capabilities. Some parameters are explained below where relevant
```sh
$ cd Docker
$ ./run_local.sh
```
3. In any VNC viewer, connect to the URL below. The password is `password`
```
localhost:5901
```
Note that the Docker host uses X11. If you are connecing from a Wayland client, you may experience visual glitches on some VNC viewers (one that seems to avoid these issues is Remmina)

4. right click anywhere, and open a shell. You will need several of these
```
Application >> Shells >> Bash
```
5. The default working directory is `~/orbslam3/SLAM_CARV`. This is where the below commands should be executed in, unless otherwise stated

## Camera inputs
This code supports multiple camera inputs. Choose one of the below that suits you best

1. To use a traditional USB camera (such as a webcam), run
```sh
$ ./usb_video.sh
```
It is assumed your desired camera is `/dev/video0` on the host. If not, you need to change the argument below. Note this is difficult to get working on WSL, a native Linux host is recommended.
```sh
# in run_local.sh
(...)
--device=/dev/video0:/dev/video0 \
(...)
```

2. To use an MJPEG stream, such as a phone through DroidCam, change the IP in `droidcam_video.sh`, then run
```sh
$ ./droidcam_video.sh
```

## Running CARV
1. Edit `run.sh` last argument to `usb_cam/image_raw` if you used a USB camera, or `camera/image_raw` for MJPEG.
2. In a **new** bash shell (the one running your camera feed must stay alive), run it
```sh
$ ./run.sh
```
3. ORB-SLAM2 will load the vocabulary dictionary for DBoW and then you should see that the SLAM window and SLAM image feed is working.

## Notes on camera tracking
The tracking system can be flaky in suboptimal use cases (which are common). Below are notes from my personal experience:

- When the CARV camera feed starts, you should see green tracking dots/squares appearing. If you instead see green lines, or nothing at all, it means the system is failing to find tracking points.
- I recommend holding the camera steady watching a detailed surface, such as a keyboard, to have something to latch on to initially, then **slowly** looking around.
- If tracking cannot be established from the start (within a few seconds), it is unlikely it will start working, you should just close the shell window and re-run it. It may also just crash, in which case you can try running it again.
- If it loses tracking as you are moving, look back at something that has been successfully tracked previously to get it working again.
- The system cannot differentiate between a moving camera and a moving environment. Try keeping the environment static where possible.
- Mirrors and screens should be avoided.

As always, YMMV, but I hope this helps someone.

## Remotely viewing the model
To export the model over the network:

1. Ensure the correct port is bound in `./run.sh` for the HTTP server. The default is 8080, but if there is a conflict, you must adjust this. (Also remember to kill an old container before running a new one. check `docker ps` if you forgot)
2. Change the IP adresses in `./run.sh`.
  a. The first IP address is the IP adress the Unity server can use to access the CARV HTTP server, and its port (see 1.)
  b. The second IP address is of the Unity server and its ZMQ port, to send the update messages to.
3. Run the CARV model as above, and the Unity server.
4. (TODO temp) press the send model button in CARV to send the model to the Unity server (or more specifically, tell the Unity server to fetch it)

# Setting up the workspace

While the Docker container has the code and a GUI, actually coding it it is not optimal. If you are only interested in running the system rather than modifying it, you can ignore this section.

## Dev Container
This option is best for VS Code on Linux, and should also work in WSL. This lets you work in the same base environment as the Docker container the code will run in, without affecting your local system.
1. Install the Dev Containers extension (by Microsoft)
2. Open this project, then use `ctrl + shift + P` >> `Dev Containers: Reopen in Dev Container`
3. The configuration is stored in `.devcontainer/devcontainer.json`, but should be auto-detected
4. You may wish to add the following to your VS Code settings for this project (`.vscode/c_cpp_properties.json`, under "configurations")
```json
"includePath": [
    "${workspaceFolder}/**",
    "/usr/include/opencv4",
    "/usr/include/eigen3"
],
"compilerPath": "/usr/bin/gcc",
"cStandard": "c17",
"cppStandard": "c++17",
"intelliSenseMode": "linux-gcc-x64"
```

## Manual Configuration (not recommended)
Windows users may choose to instead configure their WSL environment to match that of the Docker container directly. You can use `Docker/Dockerfile` for reference as to what dependencies and system configurations are required. This takes more manual effort and maintenance, but should otherwise work without issues if done correctly.

Similar to WSL, some Linux users may choose to install required dependencies locally. This is **strongly not recommended**, as it may cause unexpected issues and conflicts with other things installed on your system, and if your base distro differs from that of the container (currently `Ubuntu 20.04`), packages may not be availiable and you would have to build some things from source and configure manually. You may consult `Docker/Dockerfile` if you wish, but proceed at your own risk.

# Other Resources
FLIR camera setup with ros and linux
[Here](https://www.flir.ca/support-center/iis/machine-vision/application-note/using-ros-with-spinnaker/)
