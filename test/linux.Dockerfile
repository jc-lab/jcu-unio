FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get -y update && \
    apt-get install -y build-essential git cmake libc6-dev libssl-dev gcc-multilib g++-multilib

RUN mkdir -p /work/src /work/build
ADD [ ".", "/work/src" ]

ARG CMAKE_BUILD_TYPE=RelWithDebInfo

WORKDIR /work/build
RUN cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} /work/src && \
    cmake --build . --config ${CMAKE_BUILD_TYPE}
