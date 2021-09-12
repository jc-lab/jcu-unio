add_executable(jcu_unio_example_echo_test_client echo_test_client.cc)
target_link_libraries(jcu_unio_example_echo_test_client
        PRIVATE
        jcu_unio
        )
