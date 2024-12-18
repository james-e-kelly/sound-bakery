# Lists all external projects used by Sound Bakery

include(FetchContent)

    # Resource Embedding
FetchContent_Declare(
  cmrc
  GIT_REPOSITORY https://github.com/vector-of-bool/cmrc.git
  GIT_TAG        2.0.1
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)

    # Using TheCherno version for custom titlebar on Windows
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/TheCherno/glfw.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
  OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        docking
  GIT_SHALLOW    TRUE
  OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
    implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
  nfd
  GIT_REPOSITORY https://github.com/mlabbe/nativefiledialog.git
  GIT_TAG        67345b80ebb429ecc2aeda94c478b3bcc5f7888e
  GIT_SHALLOW    TRUE
  OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
  fonts
  GIT_REPOSITORY https://github.com/juliettef/IconFontCppHeaders.git
  GIT_TAG        main
  GIT_SHALLOW    TRUE
  OVERRIDE_FIND_PACKAGE
)

# Need fmt library for better cross compiler support
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        10.2.1
  GIT_SHALLOW    TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  doxygenawesome
  GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
  GIT_TAG        main
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
  cppdelegates
  GIT_REPOSITORY https://github.com/KarateKidzz/CppDelegates.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)


FetchContent_Declare(
  rttr
  GIT_REPOSITORY https://github.com/KarateKidzz/rttr.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
  yaml
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG        0.8.0
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
  concurrencpp
  GIT_REPOSITORY https://github.com/David-Haim/concurrencpp.git
  GIT_TAG        v.0.1.7
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.14.1
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
  miniaudio
  GIT_REPOSITORY https://github.com/mackron/miniaudio.git
  GIT_TAG        0.11.21
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
  ogg
  GIT_REPOSITORY https://github.com/xiph/ogg.git
  GIT_TAG        v1.3.5
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
  vorbis
  GIT_REPOSITORY https://github.com/xiph/vorbis.git
  GIT_TAG        v1.3.7
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
  opus
  GIT_REPOSITORY https://github.com/xiph/opus.git
  GIT_TAG        v1.5.2
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)

FetchContent_Declare(
  opusfile
  GIT_REPOSITORY https://github.com/KarateKidzz/opusfile.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
)