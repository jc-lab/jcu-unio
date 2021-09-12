if (NOT TARGET uv::uv-static AND NOT TARGET uv_a)
    if (NOT UV_GIT_TAG)
        set(UV_GIT_TAG 1dff88e5161cba5c59276d2070d2e304e4dcb242) # v1.41.0
    endif ()

    FetchContent_Declare(
            uv
            GIT_REPOSITORY https://github.com/libuv/libuv.git
            GIT_TAG ${UV_GIT_TAG}
    )
    FetchContent_GetProperties(uv)
    if (NOT uv_POPULATED)
        FetchContent_Populate(uv)
    endif ()
    add_subdirectory(${uv_SOURCE_DIR} third_party/uv)

    add_library(uv::uv-static ALIAS uv_a)
else(TARGET uv::uv-static AND NOT TARGET uv_a)
    add_library(uv::uv-static ALIAS uv_a)
endif()
