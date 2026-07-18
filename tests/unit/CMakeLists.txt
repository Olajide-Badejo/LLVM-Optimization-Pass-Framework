# The unit tests link the analysis code directly (through the opf_core object
# library) and, being a standalone executable rather than a plugin, must link
# the LLVM libraries they parse and inspect IR with.
find_package(GTest REQUIRED)

# "passes" pulls in PassBuilder, which the invalidation test uses to set up the
# analysis managers exactly as opt does.
llvm_map_components_to_libnames(OPF_LLVM_LIBS
  core support asmparser irreader analysis passes)

add_executable(opf_unit
  analysis_metrics_test.cpp
  invalidation_test.cpp)

set_target_properties(opf_unit PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF)

target_link_libraries(opf_unit PRIVATE
  opf_core
  ${OPF_LLVM_LIBS}
  GTest::gtest
  GTest::gtest_main)

# One ctest entry runs the whole GoogleTest binary, which prints its own per
# test progress and fails the suite on any failure.
add_test(NAME unit COMMAND opf_unit)
