if (JCU_UNIO_USE_OPENSSL AND NOT TARGET OpenSSL::Crypto)
    if (NOT OPENSSL_FETCH_INFO)
        set(OPENSSL_FETCH_INFO
                URL https://www.openssl.org/source/openssl-1.1.1k.tar.gz
                URL_HASH SHA256=892a0875b9872acd04a9fde79b1f943075d5ea162415de3047c327df33fbaee5
                )
    endif()

    set(OPENSSL_USE_STATIC_LIBS ON)

    FetchContent_Declare(
            openssl
            GIT_REPOSITORY https://github.com/jc-lab/openssl-cmake.git
            GIT_TAG        c0294f418be47bc11467680c925d918e9f509e20
    )
    FetchContent_GetProperties(openssl)
    if (NOT openssl_POPULATED)
        FetchContent_Populate(openssl)
        add_subdirectory(${openssl_SOURCE_DIR} ${openssl_BINARY_DIR})
    endif ()
endif ()
