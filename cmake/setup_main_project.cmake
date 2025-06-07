include(setup_cpack)
include(setup_docs)
include(build_sound_bakery_project)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

setup_clang_format()
setup_clang_tidy()