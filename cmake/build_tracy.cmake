include(ExternalProject)

ExternalProject_Add(tracy_profiler
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG        v0.13.1
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  profiler
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=Release
    BUILD_COMMAND
      ${CMAKE_COMMAND} --build . --config Release
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS
      ${CMAKE_CURRENT_BINARY_DIR}/t/src/tracy_profiler-build/Release/tracy-profiler.exe
    PREFIX ${CMAKE_BINARY_DIR}/t
)

function(setup_tracy_profiler target)
    add_dependencies(${target} tracy_profiler)

    ExternalProject_Get_Property(tracy_profiler BINARY_DIR)
    
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${BINARY_DIR}/Release/tracy-profiler${CMAKE_EXECUTABLE_SUFFIX}
            $<TARGET_FILE_DIR:${target}>
        COMMENT "Copying Tracy profiler next to ${target}"
    )
endfunction()