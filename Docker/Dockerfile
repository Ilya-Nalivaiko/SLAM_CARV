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
    gedit

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
    chmod +x /root/orbslam3/SLAM_CARV/run.sh

ENV LD_LIBRARY_PATH=/root/orbslam3/SLAM_CARV/lib

RUN chmod +x /root/orbslam3/SLAM_CARV/usb_video.sh /root/orbslam3/SLAM_CARV/droidcam_video.sh

# Set the default shell to bash
SHELL ["/bin/bash", "-c"]

RUN echo "export ROS_PACKAGE_PATH=\${ROS_PACKAGE_PATH}:/root/orbslam3/SLAM_CARV/Examples/ROS" >> /root/.bashrc \
    && source /opt/ros/noetic/setup.bash \
    && source /root/.bashrc \
    && export ROS_PACKAGE_PATH=$ROS_PACKAGE_PATH:/root/orbslam3/SLAM_CARV/Examples/ROS \
    && cd /root/orbslam3/SLAM_CARV \
    && chmod +x build_ros.sh \
    && ./build_ros.sh \

RUN chmod +x /root/orbslam3/SLAM_CARV/mjpeg_cam_publisher/scripts/mjpeg_camera_node.py \
	&& echo "export ROS_PACKAGE_PATH=$ROS_PACKAGE_PATH:/root/orbslam3/SLAM_CARV/mjpeg_cam_publisher" >> /root/.bashrc

SHELL ["/bin/sh", "-c"]

# change default location to orbslamcarv
RUN echo "cd /root/orbslam3/SLAM_CARV" >> /root/.bashrc

# set up VNC server
EXPOSE 5900
ENV X11VNC_PASSWORD="password"
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
