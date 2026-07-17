# Test wiring grows across phases. The lit suite (FileCheck over .ll) lands in
# Phase 3 and the GoogleTest unit suite in Phase 2; both are added here as they
# arrive so that a single ctest run covers everything.
add_subdirectory(lit)
