FROM abrarov/msvc-2019@sha256:a5b141c6e333369d1fa362df1567184f46756e4c39c2d672937a9241883b79b3

RUN mkdir "C:\\work" && mkdir "C:\\work\\src" && mkdir "C:\\work\\build"
ADD [ ".", "C:\\work\\src" ]

ARG CMAKE_BUILD_TYPE=RelWithDebInfo

WORKDIR "C:\\work\\build"
RUN cmake "C:\\work\\src" && \
    cmake --build . --config $ENV{CMAKE_BUILD_TYPE}
