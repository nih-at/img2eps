man_MANS=img2eps.1

EXTRA_DIST=img2eps.mdoc img2eps.man image.mdoc image.ps stream.mdoc stream.ps

DISTCLEANFILES=img2eps.1

SUFFIXES=.ps .mdoc

all-local: image.ps stream.ps

.mdoc.ps:
	groff -Tps -mandoc $< > $@
