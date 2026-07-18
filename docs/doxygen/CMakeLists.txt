include(FetchContent)

message(STATUS "Fetching doxygen awesome")
FetchContent_MakeAvailable(doxygenawesome)

set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/docs/build/Doxyfile)

configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
message(STATUS "Creating doxyfile")

add_custom_target(doc_doxygen ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
)