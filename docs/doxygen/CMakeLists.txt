CPMAddPackage(
    NAME doxygenawesome
    GITHUB_REPOSITORY jothepro/doxygen-awesome-css
    GIT_TAG 82d315e34ac419a81caccf470cc2fbc2ec8ee524 # main @ 2026-07-17
    DOWNLOAD_ONLY YES
)

set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/docs/build/Doxyfile)

# Version label shown in the docs (PROJECT_NUMBER). Defaults to the project
# version; the CI docs job overrides it per release/dev build.
if(NOT DEFINED SOUND_BAKERY_DOXYGEN_VERSION)
  set(SOUND_BAKERY_DOXYGEN_VERSION "v${PROJECT_VERSION}")
endif()

configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
message(STATUS "Creating doxyfile")

add_custom_target(doc_doxygen ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
)