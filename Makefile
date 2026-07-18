# Convenience wrapper over the scripts. Everything runs on Linux (native or
# WSL2) against the pinned distribution LLVM. Override the version with
# OPF_LLVM_MAJOR if a different pinned major is installed.
OPF_LLVM_MAJOR ?= 21
export OPF_LLVM_MAJOR

BUILD := build

.PHONY: all build test check-style format bench report report-debug clean help

help:
	@echo "targets:"
	@echo "  build         configure and build the plugin"
	@echo "  test          build then run unit + lit suites"
	@echo "  check-style   dash lint + clang-format check"
	@echo "  format        apply clang-format in place"
	@echo "  bench         run the benchmark suite"
	@echo "  report        build the main report PDF"
	@echo "  report-debug  build the debug report PDF"
	@echo "  clean         remove the build tree"

all: build

build:
	bash scripts/build.sh

test:
	bash scripts/run_tests.sh

check-style:
	python3 scripts/check_no_dashes.py
	@echo "=== clang-format check ==="
	@bash scripts/check_format.sh
	@echo "=== ruff (if available) ==="
	@bash scripts/check_python.sh

format:
	@bash scripts/check_format.sh --fix

bench:
	bash scripts/run_benchmarks.sh

report:
	$(MAKE) -C report

report-debug:
	$(MAKE) -C report_debug

clean:
	rm -rf $(BUILD)
