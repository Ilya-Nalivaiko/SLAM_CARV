FROM ubuntu:16.04
MAINTAINER Islam A. Ali <islam.a.mustafa@gmail.com>



# ------------------------------------------------------------------------------------------
# Definitions
ENV OPENCV_VERSION 3.4.1
ENV OPENCV_DOWNLOAD_URL https://github.com/opencv/opencv/archive/$OPENCV_VERSION.zip
ENV OpenCV_DIR opencv-$OPENCV_VERSION
ENV EIGEN_VERSION 3.3.2
ENV EIGEN_DOWNLOAD_URL http://bitbucket.org/eigen/eigen/get/$EIGEN_VERSION.tar.gz
ENV PANGOLIN_VERSION 0.6
ENV PANGOLIN_DOWNLOAD_PATH https://github.com/stevenlovegrove/Pangolin/archive/refs/tags/v$PANGOLIN_VERSION.zip

ENV TZ=Europe/Minsk
ENV CMAKE_PREFIX_PATH=/opt/ros/kinetic/share
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# ------------------------------------------------------------------------------------------
# [0] Install newer compiler
# Update package lists and install essential build tools
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    wget \
    software-properties-common

# Add GCC 7 repository and install GCC 7
RUN add-apt-repository ppa:ubuntu-toolchain-r/test && \
    apt-get update && \
    apt-get install -y gcc-7 g++-7 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 60 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 60

# Set GCC 7 as the default compiler
RUN update-alternatives --set gcc /usr/bin/gcc-7 && \
    update-alternatives --set g++ /usr/bin/g++-7


# ------------------------------------------------------------------------------------------
# [1] General Dependencies
RUN echo "[Dockerfile Info] Installing GENERAL Dependencies ......"
RUN apt-get update && apt-get install -y \
	 	build-essential \
		cmake \
		pkg-config \
		htop \
		gedit \
		wget \
		git \
		unzip \
		curl \
		software-properties-common
		

# ------------------------------------------------------------------------------------------                                 
# [2] OpenCV

RUN echo "[Dockerfile Info] Installing OPENCV Dependencies ......"
RUN apt-get update && apt-get install -y \
	build-essential \
	libgtk2.0-dev\
	libavcodec-dev\
	libavformat-dev\ 
	libswscale-dev \
	python3.5-dev \
	python3-numpy \
	libtbb2 \
	libtbb-dev
	
RUN apt-add-repository "deb http://security.ubuntu.com/ubuntu xenial-security main"
RUN apt-get update && apt-get install -y \
	libjasper1\
	libjasper-dev \
	libjpeg-dev \
	libpng-dev \
	libtiff5-dev \
	libjasper-dev \
	libdc1394-22-dev \
	#libeigen3-dev \
	libtheora-dev \
	libvorbis-dev \
	libxvidcore-dev \
	libx264-dev \
	sphinx-common \
	libtbb-dev \
	yasm \
	libfaac-dev \
	libopencore-amrnb-dev \
	libopencore-amrwb-dev \
	libopenexr-dev \
	libgstreamer-plugins-base1.0-dev \
	libavutil-dev \
	libavfilter-dev \
	libavresample-dev

RUN echo "[Dockerfile Info] Installing OPENCV ......"
RUN cd /opt && \
	wget "$OPENCV_DOWNLOAD_URL" && \
	unzip 3.4.1.zip && \
	cd opencv-3.4.1 && \
	mkdir build
RUN cd /opt/opencv-3.4.1/build && \ 
	cmake -D CMAKE_BUILD_TYPE=RELEASE -D WITH_CUDA=OFF -D WITH_OPENGL=OFF .. && \
	make -j8 && \
	make install

# ------------------------------------------------------------------------------------------
# [4] Eigen3
RUN echo "[Dockerfile Info] Installing EIGEN3 Dependencies ......"
#none

RUN echo "[Dockerfile Info] Installing EIGEN3 ......"
RUN mkdir -p ~/eigen3 && \
# installed in the pre-requisits of the opencv Library libeigen3-dev
        cd ~/eigen3 && \
        git clone https://github.com/libigl/eigen.git && \
        cd eigen && \
        mkdir build_dir && \
        cd build_dir && \
          cmake ../ && \
          make install

# ------------------------------------------------------------------------------------------
# [3] Pangolin
RUN echo "[Dockerfile Info] Installing PNGOLIN Dependencies ......"
RUN apt-get update && apt-get install -y \
		libgl1-mesa-dev \
		libglew-dev \
		libpython2.7-dev \
		ffmpeg libavcodec-dev libavutil-dev libavformat-dev libswscale-dev libavdevice-dev \
		libdc1394-22-dev libraw1394-dev \		
		libjpeg-dev libpng12-dev libtiff5-dev libopenexr-dev


RUN echo "[Dockerfile Info] Installing PNGOLIN ......"
RUN mkdir -p ~/pangolin && \
		cd ~/pangolin && \
		wget "$PANGOLIN_DOWNLOAD_PATH" && \
		mkdir $PANGOLIN_VERSION && \
		ls && \
		unzip v$PANGOLIN_VERSION.zip -d $PANGOLIN_VERSION && \
		cd $PANGOLIN_VERSION && \
		cd Pangolin-$PANGOLIN_VERSION && \
		mkdir build && \
		cd build && \
		cmake .. -DEigen3_DIR=/usr/lib/cmake/eigen3 && \
		cmake --build .

# ------------------------------------------------------------------------------------------
# [5] Install ROS
# Install necessary packages to set up ROS Kinetic
RUN apt-get update && \
    apt-get install -y \
    gnupg2 \
    lsb-release \
    && rm -rf /var/lib/apt/lists/*

# Add ROS repository
RUN sh -c 'echo "deb http://packages.ros.org/ros/ubuntu xenial main" > /etc/apt/sources.list.d/ros-latest.list' && \
    apt-key adv --keyserver 'hkp://keyserver.ubuntu.com:80' --recv-key C1CF6E31E6BADE8868B172B4F42ED6FBAB17C654

# Install ROS Kinetic full desktop
RUN apt-get update && \
    apt-get install -y \
    ros-kinetic-desktop-full \
    && rm -rf /var/lib/apt/lists/*

# Initialize rosdep
RUN rosdep init && \
    rosdep update

# Source ROS setup script
RUN echo "source /opt/ros/kinetic/setup.bash" >> /root/.bashrc

# install the usb_cam package (kinetic version)
RUN apt-get update && \
    apt-get install -y \
    ros-kinetic-usb-cam

# # ------------------------------------------------------------------------------------------
# # [6] Install flir camera drivers to docker 

# # Echo installation message
# RUN echo "[Dockerfile Info] Installing FLIR camera driver ......"

# # Create the target directory
# RUN mkdir -p ~/apps

# # Copy the compressed file to the target directory
# COPY spinnaker-3.2.0.62-amd64-pkg.20.04.tar.xz root/apps/

# # List the contents of the target directory to verify the file copy
# RUN ls -l root/apps/

# # Extract the compressed file
# RUN tar -xJf root/apps/spinnaker-3.2.0.62-amd64-pkg.20.04.tar.xz -C root/apps/


# ENV DEBIAN_FRONTEND noninteractive
# ENV DEBIAN_FRONTEND teletype

# # Change to the extracted directory and run installation scripts
# RUN cd root/apps/spinnaker-3.2.0.62-amd64-pkg.20.04/spinnaker-3.2.0.62-amd64 && \
#     yes | ./install_spinnaker.sh && \
#     ./configure_usbfs.sh

# ------------------------------------------------------------------------------------------
# [7] Install flir camera ros driver and launch files

# ------------------------------------------------------------------------------------------
# [8] Install SLAM CARV

RUN apt-get update && apt-get install -y \
	libpcap-dev \
	libssl-dev \ 
	libboost-dev \
	libboost-serialization-dev \
	libopenblas-dev \
	libcgal-dev \ 
    libcgal-qt5-dev \
	ros-kinetic-libg2o \
	ros-kinetic-cv-bridge


RUN echo "[Dockerfile Info] Installing ORB-SLAM3 ......"
RUN mkdir -p ~/orbslam3 && \
	cd ~/orbslam3 && \
	git clone --depth 1 https://github.com/IslamAAli/SLAM_CARV.git && \
	cd SLAM_CARV && \
	chmod +x build.sh && \
	./build.sh   

# Set the default shell to bash
SHELL ["/bin/bash", "-c"]

RUN echo "export ROS_PACKAGE_PATH=\${ROS_PACKAGE_PATH}:~/orbslam3/SLAM_CARV/Examples/ROS" >> /root/.bashrc \
    && source /opt/ros/kinetic/setup.bash \
	&& source /root/.bashrc \
	&& export ROS_PACKAGE_PATH=$ROS_PACKAGE_PATH:~/orbslam3/SLAM_CARV/Examples/ROS \
	&& cd ~/orbslam3 \
	&& cd SLAM_CARV \
	&& chmod +x build_ros.sh \
	&& ./build_ros.sh

SHELL ["/bin/sh", "-c"]

RUN echo "TESTING2"
# ############################################################################################
# ############################################################################################
# ############################################################################################
# ============================================================================================
# -----------------------------------------------------------------------------------------
# The following part is adapted partially from : https://github.com/jupemara/x11vnc-docker
# -----------------------------------------------------------------------------------------
# ============================================================================================
RUN echo "TESTING2"
EXPOSE 5900
ENV \
  DEBIAN_FRONTEND="nonintractive" \
  X11VNC_PASSWORD="password"
RUN apt-get update -y
RUN apt-get install -y xvfb x11vnc fluxbox
ADD ./entrypoint.sh /opt/entrypoint.sh
ENTRYPOINT /opt/entrypoint.sh
