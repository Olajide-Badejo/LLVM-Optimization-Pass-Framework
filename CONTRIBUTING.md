# Contributing

## Ground rules

- Every metric and timing in the docs and report comes from a recorded run on
  real hardware. No hand typed numbers in the report sources.
- A transformation that changes observable kernel output is a bug, no matter how
  much IR it removes. The differential execution check gates every transform.
- No em dashes or en dashes anywhere, and no `--` or `---` runs in LaTeX prose.
  `make check-style` enforces this; so does CI.
- Conventional Commits, one logical change per commit.

## Workflow

```
make build         # configure and build the plugin
make test          # unit + lit suites
make check-style   # dash lint + clang-format
make format        # apply clang-format in place
```

## Style

Modern C++ with RAII and no global state, small single responsibility passes,
everything under namespace `opf`. Formatting follows LLVM style through
`.clang-format`. Files are `snake_case`; inside pass code, types are
`CamelCase`, variables `camelBack`, constants `UPPER_SNAKE`, matching LLVM. See
`docs/design_decisions.md` for why the conventions split at the file boundary.

## Adding a pass

1. Header in `include/opf/{analysis,transforms}/`, implementation in the mirror
   `lib/` path, added to `lib/CMakeLists.txt`.
2. Register it in `lib/plugin_registration.cpp`.
3. A lit test per pattern in `tests/lit/`, including a negative test proving the
   unsafe case is left alone. Unit tests for any nontrivial metric arithmetic.
4. Document it in `docs/passes.md`.
