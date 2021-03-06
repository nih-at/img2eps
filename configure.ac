dnl Process this file with autoconf to produce a configure script.

dnl init

AC_PREREQ(2.50)
AC_REVISION([$NiH: configure.in,v 1.22 2005/07/07 09:32:08 dillo Exp $])
AC_INIT([img2eps], [0.2])
AC_CONFIG_SRCDIR([src/im_gif.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE(foreign)

AC_CANONICAL_HOST

AC_ARG_WITH(exif,
    [  --without-exif          don't include EXIF support],,
    with_exif=yes)
AC_ARG_WITH(gif,
    [  --without-gif           don't include GIF support],,
    with_gif=yes)
AC_ARG_WITH(png,
    [  --without-png           don't include PNG support],,
    with_png=yes)
AC_ARG_WITH(jpeg,
    [  --without-jpeg          don't include JPEG support],,
    with_jpeg=yes)
AC_ARG_WITH(jpeg2000,
    [  --without-jpeg2000      don't include JPEG 2000 support],,
    with_jpeg2000=yes)
AC_ARG_WITH(tiff,
    [  --without-tiff          don't include TIFF support],,
    with_tiff=yes)

AC_PROG_CC

dnl programs

dnl libraries

AC_CHECK_LIB(z, deflate)

if test "$with_exif" = "yes"
then
  have_exif=no
  PKG_CHECK_MODULES(EXIF, libexif,
		    [CFLAGS="$EXIF_CFLAGS $CFLAGS"
		     LIBS="$EXIF_LIBS $LIBS"
		     have_exif=yes],
		    [have_exif=no])
fi
if test "$with_png" = "yes"
then
  have_png=yes
  PKG_CHECK_MODULES(PNG, libpng12,
		    [CFLAGS="$PNG_CFLAGS $CFLAGS"
		     LIBS="$PNG_LIBS $LIBS"],
		    [AC_CHECK_LIB(png, png_create_read_struct, ,
		    [AC_CHECK_LIB(png, png_destroy_read_struct,
	                          [AC_DEFINE(HAVE_LIBPNG)
			           LIBS="-lpng -lm $LIBS"],
				  [have_png=no], -lm)])])
fi
if test "$with_gif" = "yes"
then
  have_gif=yes
  AC_CHECK_LIB(gif, DGifOpenFileName, , [have_gif=no])
fi
if test "$with_jpeg" = "yes"
then
  have_jpeg=yes
  AC_CHECK_LIB(jpeg, jpeg_read_header, , [have_jpeg=no])
fi
if test "$with_tiff" = "yes"
then
  have_tiff=yes
  AC_CHECK_LIB(tiff, TIFFOpen, , [have_tiff=no])
fi
if test "$with_jpeg2000" = "yes"
then
  have_jpeg2000=yes
  AC_CHECK_LIB(jasper, jas_init, , [have_jpeg2000=no])
fi

dnl header files

dnl typedefs
dnl structures

dnl compiler characteristics

dnl library functions

AC_REPLACE_FUNCS(getopt_long)
AC_CHECK_FUNC(vasprintf, , [AC_LIBOBJ([snprintf])])

dnl system services

dnl output

dnl NIH_CHECK_SUPPORT(WHAT)
dnl check whether support for WHAT can and should be compiled in
AC_DEFUN([NIH_CHECK_SUPPORT], [
  AC_MSG_CHECKING([for $1 support])
  lc=`echo $1 | tr A-Z a-z`
  eval "support=\$have_$lc"
  eval "enabled=\$with_$lc"
  if test "$enabled" = "yes"
  then
    if test "$support" = "yes"
    then
	AC_DEFINE(USE_$1, , [define if $1 support should be compiled])
	AC_MSG_RESULT([yes])
    else
	AC_MSG_RESULT([no])
    fi
  else
    AC_MSG_RESULT([disabled])
  fi])

NIH_CHECK_SUPPORT(EXIF)
NIH_CHECK_SUPPORT(GIF)
NIH_CHECK_SUPPORT(JPEG)
NIH_CHECK_SUPPORT(JPEG2000)
NIH_CHECK_SUPPORT(PNG)
NIH_CHECK_SUPPORT(TIFF)

case $host_os
in
        *bsd*) MANFMT=mdoc;;
        *) MANFMT=man;;
esac
AC_SUBST([MANFMT])

AC_OUTPUT(Makefile doc/Makefile src/Makefile regress/Makefile)
