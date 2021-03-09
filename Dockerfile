#Download base ubuntu image
FROM ubuntu:18.04
RUN apt-get update && \
    apt-get -y install build-essential ccache golang-go git wget sudo udev zip curl cmake software-properties-common

RUN dpkg --add-architecture i386
RUN apt-get update && \
    apt-get -y install libudev-dev libusb-1.0-0-dev && \
    apt-get -y install libc6:i386 libncurses5:i386 libstdc++6:i386 libc6-dev-i386 -y > /dev/null && \
    apt-get -y install binutils-arm-none-eabi && \
    apt-get -y install libcmocka-dev

ADD install_compiler.sh /tmp/install_compiler.sh
RUN /tmp/install_compiler.sh

# Install Python
RUN apt-get update && apt-get -y install python3 python3-pip
RUN ln -s /usr/bin/python3 /usr/bin/python
RUN pip3 install -U setuptools ledgerblue pillow

RUN adduser -u 1000 test

RUN git clone https://github.com/LedgerHQ/nanos-secure-sdk --branch master /opt/bolos/nanos-secure-sdk

#VOLUME
RUN echo "export BOLOS_SDK=/opt/bolos/nanos-secure-sdk" >> ~/.bashrc

# ENV

# START SCRIPT
