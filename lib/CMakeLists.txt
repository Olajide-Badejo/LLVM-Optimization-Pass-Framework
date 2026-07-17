# The framework ships as a single loadable module. opt resolves the LLVM
# symbols at dlopen time, so the plugin itself links no LLVM libraries; that is
# the standard out of tree plugin arrangement and keeps the .so small.
# This list grows one entry per phase as passes land. Keeping it explicit (no
# globbing) means the build never silently picks up a half written file.
set(OPF_SOURCES
  plugin_registration.cpp)

add_library(OPFPasses MODULE ${OPF_SOURCES})

target_include_directories(OPFPasses
  PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_include_directories(OPFPasses SYSTEM
  PRIVATE ${LLVM_INCLUDE_DIRS})

set_target_properties(OPFPasses PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF)

# Match the distribution's RTTI setting. Ubuntu LLVM ships without RTTI.
if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(OPFPasses PRIVATE -fno-rtti)
endif()

target_compile_options(OPFPasses PRIVATE
  -Wall -Wextra -fvisibility-inlines-hidden)

# Loadable modules legitimately carry undefined LLVM symbols resolved by the
# host tool. Do not let the linker reject them.
if(APPLE)
  target_link_options(OPFPasses PRIVATE -Wl,-undefined,dynamic_lookup)
endif()
