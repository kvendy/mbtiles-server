FROM ubuntu:16.04
MAINTAINER Thomas Brüggemann <mail@thomasbrueggemann.com>
LABEL Description="Little C++ server that serves MBTiles vector tiles." Vendor="Thomas Brüggemann" Version="1.0.0"

# INSTALL DEPENDENCIES
RUN apt-get update -y && apt-get install -y  software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt-get update -y && apt-get install -y libboost-all-dev g++ build-essential
RUN apt-get -y install libsqlite3-dev libprotobuf-dev zlib1g-dev

# BUILD WHIRLWIND
COPY ./ /home
WORKDIR /home

RUN make
