bin_PROGRAMS=img2eps

LDADD=@LIBOBJS@

img2eps_SOURCES=epsf.c \
		exceptions.c \
		img2eps.c \
		image.c \
		im_convert.c \
		im_gif.c \
		im_jpeg.c \
		im_jpeg2000.c \
		im_png.c \
		im_tiff.c \
		im_xbm.c \
		im_xpm.c \
		stream.c \
		st_ascii85.c \
		st_asciihex.c \
		st_ccitt.c \
		st_dct.c \
		st_file.c \
		st_flate.c \
		st_line.c \
		st_lzw.c \
		st_runlength.c \
		util.c \
		xmalloc.c

noinst_HEADERS=	epsf.h \
		exceptions.h \
		image.h \
		stream.h \
		stream_types.h \
		util.h \
		xmalloc.h

EXTRA_DIST=	getopt_long.c \
		getopt_long.h \
		snprintf.c
