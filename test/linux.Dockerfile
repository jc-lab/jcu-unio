FROM ubuntu:20.04 as builder

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get -y update && \
    apt-get install -y \
    build-essential git cmake libc6-dev libssl-dev gcc-multilib g++-multilib \
    lcov gcovr

RUN mkdir -p /work/src /work/build
ADD [ ".", "/work/src" ]

ARG CMAKE_BUILD_TYPE=Debug
ARG ENABLE_COVERAGE=ON

WORKDIR /work/build
RUN cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DJCU_UNIO_ENABLE_TESTING=ON -DJCU_UNIO_ENABLE_COVERAGE=ON -DCODE_COVERAGE_VERBOSE=${ENABLE_COVERAGE} /work/src && \
    cmake --build . --config ${CMAKE_BUILD_TYPE} -- -j4

RUN cmake --build . --config Debug --target jcu_unio_coverage

FROM scratch
COPY --from=builder /work/build/jcu_unio_coverage.xml /
