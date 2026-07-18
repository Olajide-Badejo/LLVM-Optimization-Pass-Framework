# Build the main report PDF. The figures and tables it inputs are generated from
# the benchmark CSV by scripts/generate_plots.py and are committed, so this
# target needs only a LaTeX toolchain (latexmk, pdflatex, bibtex).
MAIN := main

.PHONY: all clean

all: $(MAIN).pdf

$(MAIN).pdf: $(MAIN).tex references.bib \
             $(wildcard sections/*.tex) \
             $(wildcard tables/*.tex) \
             $(wildcard figures/*.pdf)
	latexmk -pdf -interaction=nonstopmode -halt-on-error $(MAIN).tex

clean:
	latexmk -C
	rm -f $(MAIN).pdf
