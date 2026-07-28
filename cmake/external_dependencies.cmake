# Lists all external projects used by Sound Bakery

include(FetchContent)

# CMake 4.0 removed compatibility with cmake_minimum_required(VERSION < 3.5).
# Some pinned third-party deps (e.g. cmrc 2.0.1, which is unmaintained upstream)
# still declare older minimums, so allow them to configure under CMake 4.x.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum CMake policy version for old dependencies")

# Resource Embedding
FetchContent_Declare(
  cmrc
  GIT_REPOSITORY https://github.com/vector-of-bool/cmrc.git
  GIT_TAG        2.0.1
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL 
)

FetchContent_Declare(
  nfd
  GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
  GIT_TAG        v1.2.1
  GIT_SHALLOW    TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL 
)

FetchContent_Declare(
  fonts
  GIT_REPOSITORY https://github.com/juliettef/IconFontCppHeaders.git
  GIT_TAG        bbe9ecd24203c4de10d1b9e3fdeb4edf5c6cb842
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL
)

# Need fmt library for better cross compiler support
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        12.2.0
  GIT_SHALLOW    TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  doxygenawesome
  GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
  GIT_TAG        82d315e34ac419a81caccf470cc2fbc2ec8ee524 # main @ 2026-07-17
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL 
)

FetchContent_Declare(
  cppdelegates
  GIT_REPOSITORY https://github.com/KarateKidzz/CppDelegates.git
  GIT_TAG        1f6e8c9352ca1c52b2c596520cba2241b9ecee31 # master @ 2026-07-17
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL 
)

FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        31c1ad37456438565541f4919958214b6e762fb4 # master @ 2026-07-17
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL 
)


FetchContent_Declare(
  rttr
  GIT_REPOSITORY https://github.com/KarateKidzz/rttr.git
  GIT_TAG        23850d1dd23952b7c29ba9398adb1548f6816c30 # master @ 2026-07-17
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL 
)

FetchContent_Declare(
  concurrencpp
  GIT_REPOSITORY https://github.com/james-e-kelly/concurrencpp.git
  GIT_TAG        53b007ba3721d99ec3aa6c39dd228638798d54e4 # master @ 2026-07-17
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  eabase
  GIT_REPOSITORY https://github.com/electronicarts/EABase.git
  GIT_TAG        0699a15efdfd20b6cecf02153bfa5663decb653c # matches EASTL 3.27.01's pinned EABase
  GIT_SUBMODULES ""
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  eastl
  GIT_REPOSITORY https://github.com/electronicarts/EASTL.git
  GIT_TAG        3.27.01
  GIT_SHALLOW    TRUE
  GIT_SUBMODULES ""
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.17.0
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL 
)

FetchContent_Declare(
  miniaudio
  GIT_REPOSITORY https://github.com/mackron/miniaudio.git
  GIT_TAG        0.11.21
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL 
)

FetchContent_Declare(
  ogg
  GIT_REPOSITORY https://github.com/xiph/ogg.git
  GIT_TAG        v1.3.5
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL 
)

FetchContent_Declare(
  vorbis
  GIT_REPOSITORY https://github.com/xiph/vorbis.git
  GIT_TAG        v1.3.7
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL 
)

FetchContent_Declare(
  opus
  GIT_REPOSITORY https://github.com/xiph/opus.git
  GIT_TAG        v1.5.2
  GIT_SHALLOW    OFF
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL 
)

FetchContent_Declare(
  opusfile
  GIT_REPOSITORY https://github.com/KarateKidzz/opusfile.git
  GIT_TAG        8cf05127fb1ae65735c2db6f65ad89e7a32ac74f # master @ 2026-07-17
  GIT_SHALLOW    OFF
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL 
)

FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG        v2.4.11
  GIT_SHALLOW    TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL 
)

FetchContent_Declare(
  bytesizelib
  GIT_REPOSITORY https://github.com/eudoxos/bytesize.git
  GIT_TAG        196542fed6b4234be6994eeb43984e3a3ba86b95 # master @ 2026-07-17
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL 
)

FetchContent_Declare(
  clap
  GIT_REPOSITORY https://github.com/free-audio/clap.git
  GIT_TAG 1.2.2
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  dirent
  GIT_REPOSITORY https://github.com/tronkko/dirent.git
  GIT_TAG      31db6474b5231c180bd4618c8c90e43af50c86d0 # HEAD @ 2026-07-17
  GIT_PROGRESS TRUE
  OVERRIDE_FIND_PACKAGE
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  boost
  URL https://github.com/boostorg/boost/releases/download/boost-1.87.0/boost-1.87.0-cmake.tar.xz
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  boost-yaml
  GIT_REPOSITORY https://github.com/james-e-kelly/yaml-archive.git
  GIT_TAG        fdbba62f97d16a0e8c28dcf04724423293752bc5 # HEAD @ 2026-07-17
  GIT_PROGRESS TRUE
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  sbk_rpmalloc_content
  GIT_REPOSITORY https://github.com/mjansson/rpmalloc.git
  GIT_TAG 1.4.5
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  platform-folders
  GIT_REPOSITORY https://github.com/sago007/PlatformFolders.git
  GIT_TAG f2625faed48bc891eb624187f8a0e4a1d1cd6ad6 # master @ 2026-07-17
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  sqlitecpp
  GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
  GIT_TAG ff5f33c7ffe9347524251838e134fb4b9df5a263 # master @ 2026-07-17
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  magicenum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG 6336b3a8295f9c9ff0522ed37f4f444f2e56c881 # master @ 2026-07-17
  EXCLUDE_FROM_ALL
)

FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG 0c1cc8c9866bb567ff11c1cd0d09779e8c5f8585 # master @ 2026-07-17
  EXCLUDE_FROM_ALL
)