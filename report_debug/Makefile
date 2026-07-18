# Build the debug report PDF. It has no generated figures or bibliography, so it
# needs only a LaTeX toolchain.
MAIN := debug_report

.PHONY: all clean

all: $(MAIN).pdf

$(MAIN).pdf: $(MAIN).tex $(wildcard sections/*.tex)
	latexmk -pdf -interaction=nonstopmode -halt-on-error $(MAIN).tex

clean:
	latexmk -C
	rm -f $(MAIN).pdf
