EXTRA_DIST=runtest ${TESTCASES} ${INPUTS} ${OUTPUTS}

TESTCASES=	gif.test \
		idx1.test \
		idx2.test \
		idx4.test \
		jpg.test \
		jpg-gray.test \
		jpg-lzw.test \
		pal-gray.test \
		pal-gray12.test\
		png-1.test \
		png-2.test \
		tiff.test \
		tiff-indexed.test \
		tiff-jpeg.test \
		tiff-lzw.test \
		xpm-flate.test \
		xpm-gray.test \
		xpm-rle.test

INPUTS=		col.jpg \
		fax2d.tif \
		idx1.xpm \
		idx2.xpm \
		jello.tif \
		links.xpm \
		lzw.tif \
		nih.png \
		pal4.png \
		pal4.tif \
		porsche.gif \
		xpdf.xpm

OUTPUTS=	gif.eps \
		idx1.eps \
		idx2.eps \
		idx4.eps \
		jpg.eps \
		jpg-gray.eps \
		jpg-lzw.eps \
		pal-gray.eps \
		pal-gray12.eps \
		png-1.eps \
		png-2.eps \
		tiff.eps \
		tiff-indexed.eps \
		tiff-jpeg.eps \
		tiff-lzw.eps \
		xpm-flate.eps \
		xpm-gray.eps \
		xpm-rle.eps

check:
	@failed=0; all=0; \
	srcdir=$(srcdir); export srcdir; \
	for tst in ${TESTCASES}; do \
	  t=`basename $$tst .test`; \
	  if ${TESTS_ENVIRONMENT} $(srcdir)/runtest $$t; then \
	    all=`expr $$all + 1`; \
	    echo "PASS: $$tst"; \
	  elif test $$? -ne 77; then \
	    all=`expr $$all + 1`; \
	    failed=`expr $$failed + 1`; \
	    echo "FAIL: $$tst"; \
	  fi; \
	done; \
	if test "$$failed" -eq 0; then \
	  banner="All $$all tests passed"; \
	else \
	  banner="$$failed of $$all tests failed"; \
	fi; \
	dashes=`echo "$$banner" | sed s/./=/g`; \
	echo "$$dashes"; \
	echo "$$banner"; \
	echo "$$dashes"; \
	test "$$failed" -eq 0
