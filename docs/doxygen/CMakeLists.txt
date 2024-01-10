include(FetchContent)

message(STATUS "Fetching doxygen awesome")

FetchContent_Declare(
  doxygenawesome
  GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
  GIT_TAG        main
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(doxygenawesome)

set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/docs/build/Doxyfile)

configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
message(STATUS "Creating doxyfile")

#execute_process(COMMAND ${DOXYGEN_EXECUTABLE} -w html header.html delete_me.html delete_me.css
 #   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docs
#)

add_custom_target(doc_doxygen ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
)

#execute_process(COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
 #   COMMENT "Generating doxygen documentation"
  #  VERBATIM
#)