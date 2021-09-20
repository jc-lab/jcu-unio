cmake_minimum_required(VERSION 3.16)
project(jcu-unio)

############################## OPTIONS ##############################
set(JCU_UNIO_USE_OPENSSL ON CACHE BOOL "jcu_unio: TLS Support with OpenSSL")

add_subdirectory(thirdparty)
add_subdirectory(unio)
add_subdirectory(example)
