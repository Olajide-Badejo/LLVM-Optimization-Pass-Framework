# The plugin path is only known at generate time, so it flows through a
# generator expression: configure_file plants the literal expression, then
# file(GENERATE) evaluates it into the final lit.site.cfg.py.
set(OPF_PLUGIN_PATH "$<TARGET_FILE:OPFPasses>")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in"
  "${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py.configured"
  @ONLY)

file(GENERATE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py"
  INPUT "${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py.configured")

find_program(OPF_LIT
  NAMES llvm-lit-${OPF_LLVM_VERSION_MAJOR} llvm-lit lit
  HINTS "${LLVM_TOOLS_BINARY_DIR}")

if(OPF_LIT)
  add_custom_target(check-lit
    COMMAND "${OPF_LIT}" -v "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS OPFPasses
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Running lit/FileCheck IR tests"
    USES_TERMINAL)

  add_test(NAME lit
    COMMAND "${OPF_LIT}" -v "${CMAKE_CURRENT_BINARY_DIR}")
else()
  message(WARNING "opf: lit not found; the FileCheck suite will not run")
endif()
