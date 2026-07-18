cmake_minimum_required(VERSION 3.20)

# One knob pins the LLVM major version for the entire project. Every script and
# CI job passes the matching llvm-config, and the build fails loudly if the
# discovered LLVM disagrees with this pin.
set(OPF_LLVM_VERSION_MAJOR "21" CACHE STRING "Pinned LLVM major version")

project(opf
  VERSION 1.0.0
  DESCRIPTION "Out of tree LLVM new pass manager optimization framework"
  LANGUAGES C CXX)

# LLVM's exported targets reference zstd::libzstd_shared because the
# distribution was built with zstd. Make that imported target resolvable before
# LLVM's config is read, with a shim for any system whose zstd ships no CMake
# package (keeps CI portable).
find_package(zstd QUIET)
if(NOT TARGET zstd::libzstd_shared)
  find_library(OPF_ZSTD_LIB NAMES zstd)
  if(OPF_ZSTD_LIB)
    add_library(zstd::libzstd_shared UNKNOWN IMPORTED)
    set_target_properties(zstd::libzstd_shared PROPERTIES
      IMPORTED_LOCATION "${OPF_ZSTD_LIB}")
  endif()
endif()

# LLVM's version config rejects a bare major such as "21" as incompatible with
# "21.1.8", so we take whatever config LLVM_DIR points at and enforce the pin
# ourselves against LLVM_VERSION_MAJOR below.
find_package(LLVM REQUIRED CONFIG)

message(STATUS "opf: found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "opf: LLVM_DIR = ${LLVM_DIR}")
message(STATUS "opf: LLVM_INCLUDE_DIRS = ${LLVM_INCLUDE_DIRS}")

if(NOT LLVM_VERSION_MAJOR STREQUAL OPF_LLVM_VERSION_MAJOR)
  message(FATAL_ERROR
    "Pinned LLVM major is ${OPF_LLVM_VERSION_MAJOR} but find_package found "
    "${LLVM_VERSION_MAJOR}. Point LLVM_DIR at the pinned toolchain.")
endif()

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# LLVM headers assume the same RTTI and definition set the library was built
# with. Mismatching -frtti against a no-RTTI LLVM causes link and ODR grief, so
# mirror what the distribution reports.
add_definitions(${LLVM_DEFINITIONS})

add_subdirectory(lib)

enable_testing()
add_subdirectory(tests)

# Surface the pinned tool paths to test configuration (lit) and to callers.
set(OPF_LLVM_TOOLS_BINDIR "${LLVM_TOOLS_BINARY_DIR}" CACHE PATH "LLVM tool dir")
