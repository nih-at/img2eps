EXTRA_DIST=runtest ${TESTCASES} ${INPUTS} ${OUTPUTS}

TESTCASES=jpg.test jpg-gray.test jpg-lzw.test png-1.test png-2.test \
	tiff.test tiff-indexed.test xpm-flate.test xpm-rle.test
INPUTS=col.jpg fax2d.tif nih.png links.xpm
OUTPUTS=jpg.eps jpg-gray.eps jpg-lzw.eps png-1.eps png-2.eps \
	tiff.eps tiff-indexed.eps xpm-flate.eps xpm-rle.eps

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
