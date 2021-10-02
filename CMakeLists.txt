cmake_minimum_required(VERSION 3.16)
project(jcu-unio)

############################## OPTIONS ##############################
set(JCU_UNIO_USE_OPENSSL ON CACHE BOOL "jcu_unio: TLS Support with OpenSSL")
set(JCU_UNIO_ENABLE_TESTING ON CACHE BOOL "jcu_unio: Enable Testing")
set(JCU_UNIO_ENABLE_COVERAGE OFF CACHE BOOL "jcu_unio: Enable coverage")

if (JCU_UNIO_ENABLE_TESTING)
    enable_testing()
endif()

if (JCU_UNIO_ENABLE_COVERAGE)
    include(cmake/CodeCoverage.cmake)
    append_coverage_compiler_flags()
endif()

add_subdirectory(thirdparty)
add_subdirectory(unio)
add_subdirectory(example)
