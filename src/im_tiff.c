/*
  $NiH: im_tiff.c,v 1.6 2002/09/14 02:27:39 dillo Exp $

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

static image_compression _cmp_tiff2img(int cmp);
static image_cs_type _cspace_tiff2img(int cs);



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
    TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &u16);
    im->im.i.cspace.type = _cspace_tiff2img(u16);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &u16);
    im->im.i.cspace.depth = u16;
    TIFFGetField(tif, TIFFTAG_COMPRESSION, &u16);
    im->im.i.compression = _cmp_tiff2img(u16);

    if (im->im.i.cspace.type == IMAGE_CS_INDEXED) {
	im->im.i.cspace.base_type = IMAGE_CS_RGB;
	im->im.i.cspace.base_depth = 16;
	im->im.i.cspace.ncol = 1<<im->im.i.cspace.depth;
    }	

    im->im.oi = im->im.i;

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



static image_compression
_cmp_tiff2img(int cmp)
{
    /* XXX: COMPRESSION_OJPEG?, CCITT? */

    switch(cmp) {
    case COMPRESSION_LZW:
	return IMAGE_CMP_LZW;
    case COMPRESSION_JPEG:
	return IMAGE_CMP_DCT;
    case COMPRESSION_PACKBITS:
	return IMAGE_CMP_RLE;
    case COMPRESSION_ADOBE_DEFLATE:
	return IMAGE_CMP_FLATE;
    default:
	return IMAGE_CMP_NONE;
    }
}



static image_cs_type
_cspace_tiff2img(int cs)
{
    switch(cs) {
    case PHOTOMETRIC_MINISWHITE:
    case PHOTOMETRIC_MINISBLACK:
	/* XXX: handle reversed grayscale */
	return IMAGE_CS_GRAY;
    case PHOTOMETRIC_RGB:
	return IMAGE_CS_RGB;
    case PHOTOMETRIC_PALETTE:
	return IMAGE_CS_INDEXED;
    default:
	return IMAGE_CS_UNKNOWN;
    }
}


#endif /* USE_TIFF */
