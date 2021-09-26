FROM abrarov/msvc-2019@sha256:a5b141c6e333369d1fa362df1567184f46756e4c39c2d672937a9241883b79b3

RUN mkdir "\\work" && mkdir "\\work\\src" && mkdir "\\work\\build"
ADD [ ".", "\\work\\src" ]

ARG CMAKE_BUILD_TYPE=RelWithDebInfo

WORKDIR "\\work\\build"
RUN cmake "C:\\work\\src" && \
    cmake --build . --config $ENV{RelWithDebInfo}
