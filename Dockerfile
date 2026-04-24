FROM nvcr.io/nvidia/deepstream:7.1-gc-triton-devel

ENV DEBIAN_FRONTEND=noninteractive
SHELL ["/bin/bash", "-c"]

RUN apt-get update && apt-get install -y --no-install-recommends \
    locales \
    curl \
    gnupg2 \
    lsb-release \
    software-properties-common \
    libboost-thread-dev
 && locale-gen zh_CN zh_CN.UTF-8 \
 && update-locale LC_ALL=zh_CN.UTF-8 LANG=zh_CN.UTF-8 \
 && rm -rf /var/lib/apt/lists/*

ENV LANG=zh_CN.UTF-8
ENV LC_ALL=zh_CN.UTF-8

RUN mkdir -p /etc/apt/keyrings \
 && curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key \
    | gpg --dearmor -o /etc/apt/keyrings/ros-archive-keyring.gpg \
 && (grep -RIl "librealsense.intel.com" /etc/apt/sources.list /etc/apt/sources.list.d 2>/dev/null \
     | xargs -r sed -i '/librealsense\.intel\.com/s/^/# /') \
 && echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(. /etc/os-release && echo ${UBUNTU_CODENAME}) main" \
    > /etc/apt/sources.list.d/ros2.list \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
    ros-humble-ros-base \
 && rm -rf /var/lib/apt/lists/*

RUN if ! command -v pip3 >/dev/null 2>&1; then \
      apt-get update && apt-get install -y --no-install-recommends python3-pip && rm -rf /var/lib/apt/lists/*; \
    fi \
 && python3 -m pip install --no-cache-dir --upgrade setuptools==58.2.0

RUN echo "source /opt/ros/humble/setup.bash" >> /etc/bash.bashrc

CMD ["/bin/bash"]
