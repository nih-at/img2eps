man1_MANS=img2eps.@MANFMT@

EXTRA_DIST=img2eps.mdoc img2eps.man image.mdoc image.ps stream.mdoc stream.ps

SUFFIXES=.ps .mdoc .man

all-local: image.ps stream.ps

.mdoc.ps:
	groff -Tps -mandoc $< > $@

.mdoc.man:
	mdoc2man $< > $@.$$$$ && diff -I NiH $@.$$$$ $@ || mv $@.$$$$ $@; \
	rm -f $@.$$$$

update-man: img2eps.man
