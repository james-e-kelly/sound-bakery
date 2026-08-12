function(setup_review_dependencies target)
    CPMAddPackage(
        NAME sqlitecpp
        GITHUB_REPOSITORY SRombauts/SQLiteCpp
        GIT_TAG ff5f33c7ffe9347524251838e134fb4b9df5a263 # master @ 2026-07-17
        EXCLUDE_FROM_ALL YES
    )

    CPMAddPackage(
        NAME httplib
        GITHUB_REPOSITORY yhirose/cpp-httplib
        GIT_TAG 0c1cc8c9866bb567ff11c1cd0d09779e8c5f8585 # master @ 2026-07-17
        EXCLUDE_FROM_ALL YES
    )

    set(OPENSSL_ROOT_DIR ${PROJECT_SOURCE_DIR}/extern/openssl/x64)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
    find_package(OpenSSL REQUIRED)

    target_compile_definitions(${target} PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    target_include_directories(${target} PRIVATE ${OPENSSL_ROOT_DIR}/include)

    target_link_libraries(${target}
        PRIVATE
        SQLiteCpp
        sqlite3
        httplib::httplib
        OpenSSL::SSL
        OpenSSL::Crypto
    )
endfunction()
