# ============================================================
#  LaTeX Build Script for ADS Project 4 Report
# ============================================================

TEX      = xelatex
TEXFLAGS = -interaction=nonstopmode -halt-on-error -file-line-error
BIBTOOL  = biber

TEXS := $(wildcard *.tex)
PDFS := $(TEXS:.tex=.pdf)

.PHONY: all clean rebuild distclean $(TEXS:.tex=)

all: $(PDFS)

%.pdf: %.tex
	@if ! grep -q '\\documentclass' $< 2>/dev/null; then \
		echo "  Skip $< (no \\documentclass)"; \
	else \
		echo "==============================="; \
		echo "  Building: $*.pdf"; \
		echo "==============================="; \
		$(TEX) $(TEXFLAGS) $<; \
		if [ -f $*.bcf ]; then $(BIBTOOL) $* && $(TEX) $(TEXFLAGS) $<; fi; \
		$(TEX) $(TEXFLAGS) $<; \
		echo "  => $*.pdf built successfully"; \
	fi

clean:
	@rm -f *.aux *.log *.out *.toc *.lof *.lot *.bbl *.blg *.bcf *.run.xml *.synctex.gz *.synctex *.nav *.snm *.vrb *.fls *.fdb_latexmk

rebuild:
	@$(MAKE) distclean
	@$(MAKE) all

distclean:
	@$(MAKE) clean
	@rm -f $(PDFS)

$(TEXS:.tex=): %: %.pdf
