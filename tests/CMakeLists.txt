# A single ctest run covers both suites: the GoogleTest unit tests over analysis
# math, and the lit/FileCheck IR tests.
add_subdirectory(unit)
add_subdirectory(lit)
