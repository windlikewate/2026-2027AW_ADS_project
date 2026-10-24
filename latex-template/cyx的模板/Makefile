# ============================================================
#  LaTeX 通用编译脚本
#  用法:
#    make                  编译全部 .tex 文件
#    make homeworkTemplate 编译指定文件
#    make clean            清除中间文件
#    make rebuild          清除中间文件后重新编译
#    make distclean        清除所有产物（含 PDF）
# ============================================================

# ---------- 编译引擎（按需修改）----------
TEX      = xelatex
TEXFLAGS = -interaction=nonstopmode -halt-on-error -file-line-error
BIBTOOL  = biber

# ---------- 自动发现所有 .tex ----------
TEXS := $(wildcard *.tex)
PDFS := $(TEXS:.tex=.pdf)

# ---------- 中间文件后缀 ----------
AUX_EXTS := .aux .log .out .toc .lof .lot \
            .bbl .blg .bcf .run.xml \
            .synctex.gz .synctex \
            .nav .snm .vrb .fls .fdb_latexmk \
            .idx .ind .ilg .glo .gls .ist \
            .acn .acr .alg .glg

AUX_FILES := $(foreach ext,$(AUX_EXTS),$(TEXS:.tex=$(ext)))

# ---------- 伪目标 ----------
.PHONY: all clean rebuild distclean $(TEXS:.tex=)

# ---------- 默认目标 ----------
all: $(PDFS)

# ---------- 单个文件编译：xelatex x2 ----------
#   跳过空文件或不含 \documentclass 的片段
%.pdf: %.tex
	@if ! grep -q '\\documentclass' $< 2>/dev/null; then \
		echo "  跳过 $< (无 \\documentclass)"; \
	else \
		echo "==============================="; \
		echo "  编译: $*.pdf"; \
		echo "==============================="; \
		$(TEX) $(TEXFLAGS) $< || true; \
		if [ -f $*.bcf ]; then $(BIBTOOL) $* || true; fi; \
		$(TEX) $(TEXFLAGS) $<; \
		echo "  => $*.pdf 完成 ✓"; \
	fi

# ---------- 清除中间文件（保留 PDF）----------
clean:
	@echo "清除中间文件..."
	@rm -f *.aux *.log *.out *.toc *.lof *.lot *.bbl *.blg *.bcf *.run.xml *.synctex.gz *.synctex *.nav *.snm *.vrb *.fls *.fdb_latexmk *.idx *.ind *.ilg *.glo *.gls *.ist *.acn *.acr *.alg *.glg
	@echo "完成 ✓"

# ---------- 清除中间文件后重新编译 ----------
rebuild:
	@$(MAKE) distclean
	@$(MAKE) all

# ---------- 彻底清除（含 PDF）----------
distclean:
	@$(MAKE) clean
	@rm -f $(PDFS)
	@echo "已清除所有产物 ✓"

# ---------- 快捷方式: make homeworkTemplate ----------
$(TEXS:.tex=): %: %.pdf
