/*
  $NiH: im_tiff.c,v 1.4 2002/09/11 22:44:19 dillo Exp $

  im_tiff.c -- TIFF image handling
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include "config.h"

#ifdef USE_TIFF

#include <setjmp.h>
#include <stdio.h>

#include <tiff.h>
#include <tiffio.h>

#define NOSUPP_CSPACE
#define NOSUPP_SCALE
#define NOSUPP_RAW
#include "image.h"



struct image_tiff {
    image im;

    TIFF *tif;
};

IMAGE_DECLARE(tiff);



void
tiff_close(image_tiff *im)
{
    TIFFClose(im->tif);
    image_free((image *)im);
}



char *
tiff_get_palette(image_tiff *im)
{
    return NULL;
}



image *
tiff_open(char *fname)
{
    image_tiff *im;
    TIFF *tif;
    uint32 u32;
    uint16 u16;

    TIFFSetErrorHandler(NULL);
    TIFFSetWarningHandler(NULL);

    if ((tif=TIFFOpen(fname, "r")) == NULL)
	return NULL;

    if ((im=image_create(tiff, fname)) == NULL) {
	TIFFClose(tif);
	return NULL;
    }

    im->tif = tif;

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &u32);
    im->im.i.width = u32;
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &u32);
    im->im.i.height = u32;
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &u16);
    im->im.i.cspace.depth = u16;

    return (image *)im;
}



int
tiff_read(image_tiff *im, char **bp)
{
    return -1;
}



void
tiff_read_finish(image_tiff *im, int abortp)
{
    return;
}



void
tiff_read_start(image_tiff *im)
{
    return;
}		

#endif /* USE_TIFF */
