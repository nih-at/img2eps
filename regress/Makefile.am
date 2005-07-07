EXTRA_DIST=runtest ${TESTCASES} ${INPUTS} ${OUTPUTS}

TESTCASES=	gif.test \
		idx1.test \
		idx2.test \
		idx4.test \
		jpg.test \
		jpg-gray.test \
		jpg-lzw.test \
		level1.test \
		order.test \
		ori-auto.test \
		pal-gray.test \
		pal-gray12.test\
		png-1.test \
		png-gray1.test \
		png-2.test \
		scale.test \
		recompress.test \
		tiff.test \
		tiff-flate.test \
		tiff-inv1.test \
		tiff-inv2.test \
		tiff-indexed.test \
		tiff-jpeg.test \
		tiff-lzw.test \
		xpm-flate.test \
		xpm-gray.test \
		xpm-rle.test

INPUTS=		col.jpg \
		col.tif \
		dump.tif \
		exif.jpg \
		fax2d.tif \
		flate.tif \
		idx1.xpm \
		idx2.xpm \
		inv.tif \
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
		level1.eps \
		order.eps \
		order-ignore.eps \
		ori-auto.eps \
		pal-gray.eps \
		pal-gray12.eps \
		png-1.eps \
		png-gray1.eps \
		png-2.eps \
		recompress.eps \
		scale.eps \
		tiff.eps \
		tiff-flate.eps \
		tiff-indexed.eps \
		tiff-inv1.eps \
		tiff-inv2.eps \
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
