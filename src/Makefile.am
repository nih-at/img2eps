bin_PROGRAMS=img2eps

LDADD=@LIBOBJS@

img2eps_SOURCES=epsf.c \
		img2eps.c \
		image.c \
		im_gif.c \
		im_jpeg.c \
		stream.c \
		st_ascii85.c \
		st_asciihex.c \
		st_file.c \
		st_line.c

noinst_HEADERS=	epsf.h \
		image.h \
		stream.h \
		stream_types.h

EXTRA_DIST=	getopt.c \
		getopt.h \
		getopt1.c
