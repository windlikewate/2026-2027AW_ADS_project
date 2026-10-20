TEX      = xelatex
TEXFLAGS = -interaction=nonstopmode -halt-on-error -file-line-error
BIBTOOL  = biber

.PHONY: all clean rebuild

all: main.pdf

main.pdf: main.tex
	$(TEX) $(TEXFLAGS) $<
	@if [ -f main.bcf ]; then $(BIBTOOL) main || true; $(TEX) $(TEXFLAGS) $< || true; fi
	$(TEX) $(TEXFLAGS) $<

clean:
	rm -f *.aux *.log *.out *.toc *.nav *.snm *.vrb *.bbl *.blg *.bcf *.run.xml *.synctex.gz

rebuild: clean all
