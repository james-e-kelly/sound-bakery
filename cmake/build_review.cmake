function(setup_review_dependencies target)
    message(STATUS "Fetching SQLiteCpp")
    FetchContent_MakeAvailable(sqlitecpp)

    message(STATUS "Fetching magic_enum")
    FetchContent_MakeAvailable(magicenum)

    message(STATUS "Fetching httplib")
    FetchContent_MakeAvailable(httplib)

    set(OPENSSL_ROOT_DIR ${PROJECT_SOURCE_DIR}/extern/openssl/x64)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
    find_package(OpenSSL REQUIRED)

    target_compile_definitions(${target} PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    target_include_directories(${target} PRIVATE ${OPENSSL_ROOT_DIR}/include)

    target_link_libraries(${target}
        PRIVATE
        SQLiteCpp
        sqlite3
        magic_enum::magic_enum
        httplib::httplib
        OpenSSL::SSL
        OpenSSL::Crypto
    )
endfunction()
