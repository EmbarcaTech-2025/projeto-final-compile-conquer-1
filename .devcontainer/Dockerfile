FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    libusb-1.0-0-dev \
    git \
    python3 \
    wget \
    curl \
    sudo \
    minicom \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

RUN useradd -m -s /bin/bash developer \
    && usermod -aG dialout,plugdev,root developer \
    && echo 'developer ALL=(ALL) NOPASSWD: ALL' >> /etc/sudoers.d/developer

USER developer
RUN echo 'export PICO_SDK_PATH=$HOME/.pico-sdk/pico-sdk' >> ~/.bashrc \
    && echo 'export PATH=$HOME/.pico-sdk/picotool/*/picotool:$PATH' >> ~/.bashrc \
    && echo 'export PATH=$HOME/.pico-sdk/cmake/*/bin:$PATH' >> ~/.bashrc \
    && echo 'export PATH=$HOME/.pico-sdk/ninja/*:$PATH' >> ~/.bashrc



