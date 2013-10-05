man1_MANS=img2eps.@MANFMT@

EXTRA_DIST=img2eps.mdoc img2eps.man image.mdoc image.ps stream.mdoc stream.ps

SUFFIXES=.ps .mdoc .man .html

all-local: image.ps stream.ps

.mdoc.ps:
	groff -Tps -mandoc $< > $@

.mdoc.man:
	mandoc -T man $< | sed '/TH/s,"NetBSD [^"]*","NiH",' > $@.$$$$ && diff -u -I NiH $@ $@.$$$$ || mv $@.$$$$ $@; \
	rm -f $@.$$$$

.mdoc.html:
	mandoc -Thtml -Oman="%N.html",style=../nih-man.css $< > $@.$$$$ && mv $@.$$$$ $@

update-man: img2eps.man
