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

## 2026-07-18 Phase 4: proving invalidation instead of assuming it

**Symptom.** Not a failure so much as a trap I wanted to close: it is easy to
return `PreservedAnalyses::all()` out of habit or convenience, and nothing at
build time catches it. A transform that quietly over claims preservation leaves
later passes reading stale analysis results, which is exactly the class of bug
this project is meant to demonstrate command over.

**Root cause.** The transforms genuinely change instructions but preserve the
CFG, so the honest answer is `preserveSet<CFGAnalyses>()`, not all. Getting that
one line right is the difference between a dependent analysis recomputing and it
serving a stale cache.

**Options.** (1) Trust the code review eye. (2) Write a test that would fail if a
transform over claimed preservation.

**Fix.** Option 2. The invalidation test caches an opcode count, runs the
algebraic simplifier through a real FunctionPassManager (so the invalidation
machinery actually runs), then requests the count again and asserts it dropped.
If the simplifier returned `all()`, the manager would hand back the stale count
and the test would fail on the spot.

**Verified.** `Invalidation.TransformForcesAnalysisRecompute` passes with the
count going from 4 to 1; flipping a transform to `PreservedAnalyses::all()`
locally made it fail as intended before I reverted the experiment.

## 2026-07-18 Phase 5: optnone made the whole pipeline a no-op

**Symptom.** The first time I pointed the harness at a real kernel, my-default
reported zero rewrites on functions I knew were full of identities, and the
transformed IR was identical to the baseline.

**Root cause.** `clang -O0` attaches the `optnone` attribute to every function,
and the new PassManager honors it by skipping optimization passes entirely. My
passes were loaded and registered correctly; they were simply never run on the
kernel functions.

**Options.** (1) Compile the kernels at `-O1` and accept that the frontend has
already optimized them, muddying the before picture. (2) Keep `-O0` for a clean
unoptimized baseline but strip optnone so passes run.

**Fix.** Option 2: `clang -O0 -Xclang -disable-O0-optnone`. This yields the
naive `-O0` IR I want as a baseline while letting `opt` actually transform it.
The harness sets it for every kernel and the manifest records it.

**Verified.** After the flag, my-default reports real rewrite counts and the
transformed IR shrinks; the differential checksum still matches the baseline.

## 2026-07-18 Phase 5: small kernels and timing noise

**Symptom.** Two of the four kernels show a runtime difference between baseline
and our pipeline that is smaller than the interquartile range of the samples.

**Root cause.** Not a bug. The kernels run in tens of milliseconds, and under
WSL2 there is no CPU governor control, so a few milliseconds of jitter is
expected. For `branchy` and `clean` the structural change is small or zero, so
there is genuinely little runtime to move.

**Options.** (1) Inflate the workloads until every kernel shows a runtime delta.
(2) Report the deltas honestly and label the noise dominated ones as structural
only.

**Fix.** Option 2, per the project's ground rules. `arith_redundant` shows a
real speedup well outside the IQR; the others are presented as structural
results with the timing noise stated plainly. Padding the workload to
manufacture a runtime story would be dishonest.

**Verified.** The results table and the runtime figure show the IQR as error
bars, so a reader can see for themselves which deltas are signal.
