# Engineering log

Dated entries as things broke and how I fixed them. Each entry records the
symptom, the root cause, the options I weighed, the fix I chose and why, and how
I verified it. Phase 7 folds this into the debug report PDF.

## 2026-07-17 Phase 0: pinning LLVM and the find_package version trap

**Symptom.** `find_package(LLVM 21 REQUIRED CONFIG)` failed with "Could not find
a configuration file for package LLVM that is compatible with requested version
21", even though `LLVMConfig.cmake` reporting version 21.1.8 sat right there and
was one of the files "considered but not accepted".

**Root cause.** LLVM's generated `LLVMConfigVersion.cmake` does not treat a bare
major request of `21` as compatible with the concrete `21.1.8` it advertises.
The version compatibility shim wants the request to match more precisely, so a
major only pin is rejected outright rather than rounded up.

**Options.** (1) Request the full `21.1.8` string, which then hard codes a patch
level that changes on every apt update. (2) Drop the version from
`find_package` and enforce the pin myself against `LLVM_VERSION_MAJOR`.

**Fix.** Option 2. `find_package(LLVM REQUIRED CONFIG)` takes whatever
`LLVM_DIR` points at, and a `FATAL_ERROR` guard fails the configure if
`LLVM_VERSION_MAJOR` is not the pinned major. This keeps the pin at major
granularity, survives patch bumps, and still fails loudly on the wrong
toolchain.

**Verified.** Configure succeeds and prints "opf: found LLVM 21.1.8"; forcing a
mismatched `OPF_LLVM_VERSION_MAJOR` triggers the guard as intended.

## 2026-07-17 Phase 0: lit RUN lines and spaces in the checkout path

**Symptom.** The plugin built and `opt -load-pass-plugin=... -passes=my-noop`
worked by hand, but the lit smoke test failed with `FileCheck: Too many
positional arguments` and `Could not load library '.../Corrected'`.

**Root cause.** The checkout lives under a path containing spaces (a normal
Windows Documents folder mounted into WSL at `/mnt/c/.../Corrected Projects/LLVM
Optimization Pass Framework`). lit substitutes `%s` and `%opflib` as raw text
into a RUN line that is then parsed by the shell, which split each path on its
spaces into multiple arguments.

**Options.** (1) Move the whole project to a path without spaces, fighting the
user's chosen location. (2) Quote every substitution in every RUN line so the
shell keeps each path as one argument.

**Fix.** Option 2, adopted as a project convention: RUN lines write
`"%opflib"`, `"%s"`, and any tool path in double quotes. This is strictly more
robust anyway, since it also survives a user cloning into a spaced path, which
LLVM's own in tree tests never have to worry about.

**Verified.** `lit -v build/tests/lit` reports `PASS: opf :: smoke_noop.ll`.
