set(BUILD_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/build-inc)
make_directory(${BUILD_INCLUDE_DIR})
make_directory(${BUILD_INCLUDE_DIR}/jcu-unio)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/jcu-unio-config.h.in ${BUILD_INCLUDE_DIR}/jcu-unio/jcu-unio-config.h @ONLY)

add_library(jcu_unio
        STATIC
        ${BUILD_INCLUDE_DIR}/jcu-unio/jcu-unio-config.h
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/jcu-unio/log.h
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/jcu-unio/loop.h
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/jcu-unio/uv_helper.h
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/jcu-unio/buffer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/jcu-unio/event.h
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/jcu-unio/emitter.h
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/jcu-unio/timer.h
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/jcu-unio/net/socket.h
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/jcu-unio/net/stream_socket.h
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/jcu-unio/net/tcp_socket.h
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/jcu-unio/net/ssl_socket.h
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/jcu-unio/net/openssl_provider.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/log.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/log.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/loop.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/handle.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/event.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/buffer.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/timer.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/net/socket.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/net/stream_socket.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/net/tcp_socket.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ssl_context.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/net/ssl_socket.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/net/openssl_provider.cc
        )
target_compile_definitions(
        jcu_unio
        PUBLIC
        HAS_JCU_UNIO_CONFIG_H=1
)
target_include_directories(
        jcu_unio
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc$<SEMICOLON>${BUILD_INCLUDE_DIR}>
)
target_link_libraries(
        jcu_unio
        PUBLIC
        uv_a
        OpenSSL::SSL
        OpenSSL::Crypto
)

if (JCU_UNIO_ENABLE_TESTING)
    include(GoogleTest)
    add_executable(
            jcu_unio_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/test/unit_test_utils.h
            ${CMAKE_CURRENT_SOURCE_DIR}/test/unit_test_utils.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/src/emitter_unittest.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/src/loop_unittest.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/src/timer_unittest.cc
            ${CMAKE_CURRENT_SOURCE_DIR}/src/net/tcp_socket_unittest.cc
    )
    target_link_libraries(jcu_unio_tests
            PUBLIC
            jcu_unio
            gtest
            gtest_main
            )
    gtest_discover_tests(jcu_unio_tests)
    if (JCU_UNIO_ENABLE_COVERAGE)
        setup_target_for_coverage_gcovr_xml(
                NAME jcu_unio_coverage
                DEPENDENCIES jcu_unio_tests
                EXECUTABLE ctest -j ${PROCESSOR_COUNT}
                BASE_DIRECTORY "../"
                EXCLUDE "example/.*" "unio/test/.*" ".*_unittest\.cc"
        )
    endif()
endif()
