# -----------------------------------
# Base image: dependencies only
# -----------------------------------
FROM ubuntu:20.04 AS base

# ------------------------------------------------------------------------------------------
# Definitions
ENV TZ=Europe/Minsk
ENV CMAKE_PREFIX_PATH=/opt/ros/noetic/share
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=en_US.UTF-8
RUN apt-get update && apt-get install -y tzdata locales && \
    locale-gen en_US.UTF-8 && update-locale LANG=en_US.UTF-8

# ------------------------------------------------------------------------------------------
# [0] Install compiler and tools
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    wget \
    software-properties-common \
    nano \
    git \
    curl \
    unzip \
    pkg-config \
    cmake \
    htop \
    gedit \
    gcc-10 g++-10 \
    libstdc++-10-dev \
    libtsan0

# Make gcc-10/g++-10 the defaults
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 100

# Ensure builds default to GCC 10 unless explicitly overridden
ENV CC=/usr/bin/gcc-10
ENV CXX=/usr/bin/g++-10

# ------------------------------------------------------------------------------------------
# [1] Use system OpenCV (4.2) and Python 3.8 (default in Ubuntu 20.04)
RUN apt-get update && apt-get install -y \
    python3 \
    python3-dev \
    python3-pip \
    python3-numpy \
    python3-empy \
    libopencv-dev \
    python3-opencv

# Upgrade pip if needed
RUN python3 -m pip install --upgrade pip

# ------------------------------------------------------------------------------------------
# [2] Eigen3
RUN apt-get update && apt-get install -y libeigen3-dev

# ------------------------------------------------------------------------------------------
# [3] Pangolin (from source, because Ubuntu’s package is outdated/incomplete)
RUN echo "[Dockerfile Info] Installing PANGOLIN from source ......"
RUN apt-get update && apt-get install -y \
        libgl1-mesa-dev \
        libglew-dev \
        ffmpeg \
        libavcodec-dev \
        libavutil-dev \
        libavformat-dev \
        libswscale-dev \
        libavdevice-dev \
        libdc1394-22-dev \
        libraw1394-dev \
        libjpeg-dev \
        libtiff-dev \
        libopenexr-dev

RUN mkdir -p /root/pangolin && \
        cd /root/pangolin && \
        git clone --branch v0.6 https://github.com/stevenlovegrove/Pangolin.git && \
        cd Pangolin && \
        mkdir build && cd build && \
        cmake .. -DEigen3_DIR=/usr/lib/cmake/eigen3 && \
        make -j$(nproc) && \
        make install

# ------------------------------------------------------------------------------------------
# [4] Install ROS
RUN apt-get update && \
    apt-get install -y gnupg2 lsb-release

RUN sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list'
RUN curl -s https://raw.githubusercontent.com/ros/rosdistro/master/ros.key | apt-key add -
RUN apt-get update && apt-get install -y ros-noetic-desktop-full

# Setup ROS environment
RUN echo "source /opt/ros/noetic/setup.bash" >> /root/.bashrc
ENV ROS_DISTRO=noetic

# Initialize rosdep
RUN apt-get update && apt-get install -y python3-rosdep && rosdep init || true && rosdep update

# usb_cam package
RUN apt-get update && apt-get install -y ros-noetic-usb-cam

# vision_opencv (cv_bridge, image_geometry, etc.) packaged
RUN apt-get update && apt-get install -y ros-noetic-vision-opencv

# ------------------------------------------------------------------------------------------
# [5] Install SLAM CARV dependencies
RUN apt-get update && apt-get install -y \
    libpcap-dev \
    libssl-dev \
    libopenblas-dev \
    libcgal-dev \
    libcgal-qt5-dev \
    ros-noetic-libg2o \
    libzmq3-dev


# debug: install valgrind from source

# RUN echo "[Dockerfile Info] Installing valgrind from source ......"
# RUN apt-get update && \
#     apt-get install -y build-essential wget libc6-dbg && \
#     cd /root && \
#     wget https://sourceware.org/pub/valgrind/valgrind-3.21.0.tar.bz2 && \
#     tar -xjf valgrind-3.21.0.tar.bz2 && \
#     cd valgrind-3.21.0 && \
#     ./configure && \
#     make -j$(nproc) && \
#     make install

# -----------------------------------
# Runtime image: adds source & builds
# -----------------------------------
FROM base AS runtime

WORKDIR /root/orbslam3/SLAM_CARV
COPY . /root/orbslam3/SLAM_CARV

RUN echo "[Dockerfile Info] Installing ORB-SLAM3 from local source ......"

RUN cd /root/orbslam3/SLAM_CARV && \
	rm -rf build && \
    chmod +x build.sh && \
    ./build.sh && \
    chmod +x /root/orbslam3/SLAM_CARV/run.sh /root/orbslam3/SLAM_CARV/valrun.sh

ENV LD_LIBRARY_PATH=/root/orbslam3/SLAM_CARV/lib:$LD_LIBRARY_PATH

RUN chmod +x /root/orbslam3/SLAM_CARV/usb_video.sh /root/orbslam3/SLAM_CARV/droidcam_video.sh /root/orbslam3/SLAM_CARV/mp4_video.sh /root/orbslam3/SLAM_CARV/tum_video.sh



RUN echo "[Dockerfile Info] Setting up ROS packages ......"

# Set the default shell to bash
SHELL ["/bin/bash", "-c"]

    # set up ROS for runtime
RUN printf 'source /opt/ros/noetic/setup.bash\n' >> /root/.bashrc && \
    printf 'export ROS_PACKAGE_PATH=/root/orbslam3/SLAM_CARV/Examples/ROS:/root/orbslam3/SLAM_CARV/ros_cam_publishers:$ROS_PACKAGE_PATH\n' >> /root/.bashrc && \
    # Do it again for the build because sourcing .bashrc doesnt work for whatever reason
    source /opt/ros/noetic/setup.bash && \
    export ROS_PACKAGE_PATH=/root/orbslam3/SLAM_CARV/Examples/ROS:/root/orbslam3/SLAM_CARV/ros_cam_publishers:$ROS_PACKAGE_PATH && \
    # verify packages can be seen
    rospack profile && \
    rospack find ORB_CARV_Pub >/dev/null && \
    rospack find ros_cam_publishers >/dev/null && \
    # build main package
    cd /root/orbslam3/SLAM_CARV && \
    chmod +x build_ros.sh && \
    ./build_ros.sh && \
    # Make Python work
    chmod +x /root/orbslam3/SLAM_CARV/ros_cam_publishers/scripts/mjpeg_camera_node.py && \
    chmod +x /root/orbslam3/SLAM_CARV/ros_cam_publishers/scripts/mp4_camera_node.py && \
    chmod +x /root/orbslam3/SLAM_CARV/ros_cam_publishers/scripts/tum_camera_node.py



RUN echo "[Dockerfile Info] Finishing up environment setup and VNC ......"

SHELL ["/bin/sh", "-c"]

# change default location to orbslamcarv
RUN echo "cd /root/orbslam3/SLAM_CARV" >> /root/.bashrc

# set up VNC server
EXPOSE 5900
ENV X11VNC_P="password"
RUN apt-get update -y
RUN apt-get install -y xvfb fluxbox wget gnupg && \
    wget https://downloads.sourceforge.net/project/turbovnc/3.1/turbovnc_3.1_amd64.deb && \
    apt-get install -y ./turbovnc_3.1_amd64.deb && \
    rm turbovnc_3.1_amd64.deb
ADD ./Docker/entrypoint.sh /opt/entrypoint.sh
RUN chmod +x /opt/entrypoint.sh

# reduce image size
RUN rm -rf /var/lib/apt/lists/*

ENTRYPOINT /opt/entrypoint.sh
