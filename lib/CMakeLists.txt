# The analysis and transform logic lives in an object library so two consumers
# can share it: the loadable module the tool dlopens, and the standalone unit
# test executable. Keeping the registration glue out of the object library means
# the unit tests link the code under test without pulling in the plugin entry
# point.
#
# This list grows one entry per phase as passes land. It stays explicit (no
# globbing) so the build never silently picks up a half written file.
set(OPF_CORE_SOURCES
  support/metrics_json.cpp
  support/instruction_key.cpp
  support/pipeline_logging.cpp
  analysis/opcode_stats.cpp
  analysis/block_stats.cpp
  analysis/dead_code_report.cpp
  analysis/simplify_opportunities.cpp
  transforms/canonicalize.cpp
  transforms/algebraic_simplify.cpp
  transforms/dce.cpp
  transforms/local_cse.cpp
  transforms/default_pipeline.cpp)

add_library(opf_core OBJECT ${OPF_CORE_SOURCES})

target_include_directories(opf_core PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_include_directories(opf_core SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS})

set_target_properties(opf_core PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  POSITION_INDEPENDENT_CODE ON)

if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(opf_core PUBLIC -fno-rtti)
endif()
target_compile_options(opf_core PRIVATE -Wall -Wextra)

# The loadable module. opt resolves the LLVM symbols at dlopen time, so the
# plugin links no LLVM libraries; that is the standard out of tree arrangement.
add_library(OPFPasses MODULE plugin_registration.cpp)
target_link_libraries(OPFPasses PRIVATE opf_core)

set_target_properties(OPFPasses PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

target_compile_options(OPFPasses PRIVATE
  -Wall -Wextra -fvisibility-inlines-hidden)

# Loadable modules legitimately carry undefined LLVM symbols resolved by the
# host tool. Do not let the linker reject them.
if(APPLE)
  target_link_options(OPFPasses PRIVATE -Wl,-undefined,dynamic_lookup)
endif()
