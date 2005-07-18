man1_MANS=img2eps.@MANFMT@

EXTRA_DIST=img2eps.mdoc img2eps.man image.mdoc image.ps stream.mdoc stream.ps

SUFFIXES=.ps .mdoc .man .html

all-local: image.ps stream.ps

.mdoc.ps:
	groff -Tps -mandoc $< > $@

.mdoc.man:
	mdoc2man $< > $@.$$$$ && diff -I NiH $@.$$$$ $@ || mv $@.$$$$ $@; \
	rm -f $@.$$$$

.mdoc.html:
	nroff -mdoc2html $< | sed -e "s,../html1/,," > $@.$$$$ && mv $@.$$$$ $@

update-man: img2eps.man
