/*
  $NiH: im_tiff.c,v 1.8 2002/10/08 16:51:15 dillo Exp $

  im_tiff.c -- TIFF image handling
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include "config.h"

#ifdef USE_TIFF

#include <errno.h>
#include <setjmp.h>
#include <stdio.h>

#include <tiff.h>
#include <tiffio.h>

#define NOSUPP_CSPACE
#define NOSUPP_SCALE

#include "exceptions.h"
#include "image.h"
#include "xmalloc.h"

static image_compression _cmp_tiff2img(int cmp);
static image_cs_type _cspace_tiff2img(int cs);
static void _error_handler(const char* module, const char* fmt, va_list ap);
#if 0
static image_order _order_tiff2img(int or);
#endif



struct image_tiff {
    image im;

    TIFF *tif;

    int row;		/* current row (stripe in raw read) */
    char *buf;		/* buffer */
    int buf_size;	/* size of buffer (only valid in raw read) */
    char *pal;		/* palette */
};

IMAGE_DECLARE(tiff);



void
tiff_close(image_tiff *im)
{
    free(im->buf);
    free(im->pal);
    TIFFClose(im->tif);
    image_free((image *)im);
}



char *
tiff_get_palette(image_tiff *im)
{
    uint16 *pr, *pg, *pb, *p;
    int i;

    if (TIFFGetField(im->tif, TIFFTAG_COLORMAP, &pr, &pg, &pb) == 0)
	throws(EINVAL, "cannot get palette");
    p = xmalloc(image_cspace_palette_size(&im->im.i.cspace));

    for (i=0; i<im->im.i.cspace.ncol; i++) {
	p[3*i] = pr[i];
	p[3*i+1] = pg[i];
	p[3*i+2] = pb[i];
    }

    im->pal = (char *)p;
    return (char *)p;
}



image *
tiff_open(char *fname)
{
    image_tiff *im;
    TIFF *tif;
    uint32 u32;
    uint16 u16;
    exception ex; 

    TIFFSetErrorHandler(NULL);
    TIFFSetWarningHandler(NULL); /* XXX: handle warnings better */

    if ((tif=TIFFOpen(fname, "r")) == NULL)
	return NULL;

    TIFFSetErrorHandler(_error_handler);

    /* check for unsupported image types */
    if (TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &u16) && u16 != 1) {
	TIFFClose(tif);
	throws(EOPNOTSUPP, "unsupported planar config (not chunky)");
    }
    if (TIFFGetField(tif, TIFFTAG_TILEWIDTH, &u32)) {
	TIFFClose(tif);
	throws(EOPNOTSUPP, "tiled images not supported");
    }

    if ((im=image_create(tiff, fname)) == NULL) {
	TIFFClose(tif);
	return NULL;
    }

    im->tif = tif;
    im->pal = im->buf = NULL;

    if (catch(&ex) == 0) {
	if (TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &u32) == 0)
	    throws(EINVAL, "image width not defined");
	im->im.i.width = u32;
	if (TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &u32) == 0)
	    throws(EINVAL, "image height not defined");
	im->im.i.height = u32;
	if (TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &u16) == 0)
	    throws(EINVAL, "image colour space type not defined");
	im->im.i.cspace.type = _cspace_tiff2img(u16);
	if (TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &u16) == 0)
	    throws(EINVAL, "image depth not defined");
	im->im.i.cspace.depth = u16;
	if (TIFFGetField(tif, TIFFTAG_COMPRESSION, &u16) == 0)
	    im->im.i.compression = IMAGE_CMP_NONE;
	else
	    im->im.i.compression = _cmp_tiff2img(u16);
#if 0
	/* XXX: does libtiff take care of this?! */
	if (TIFFGetField(tif, TIFFTAG_ORIENTATION, &u16) == 0) {
	    im->im.i.order = _order_tiff2img(u16);
	    if (im->im.i.order == IMAGE_ORD_UNKNOWN)
		throwf(EINVAL, "unknown image order `%d'", u16);
	}
#endif

	if (TIFFNumberOfStrips(im->tif) > 1
	    && im->im.i.compression != IMAGE_CMP_RLE)
	    im->im.i.compression = IMAGE_CMP_NONE;

	if (im->im.i.cspace.type == IMAGE_CS_INDEXED) {
	    im->im.i.cspace.base_type = IMAGE_CS_RGB;
	    im->im.i.cspace.base_depth = 16;
	    im->im.i.cspace.ncol = 1<<im->im.i.cspace.depth;
	}

	if (image_get_row_size((image *)im) != TIFFScanlineSize(im->tif))
	    throwf(EINVAL, "scanline size mismatch: image: %d, tiff: %d",
		   image_get_row_size((image *)im),
		   TIFFScanlineSize(im->tif));

	drop();
    }
    else {
	image_close(im);
	throw(&ex);
    }

    im->im.oi = im->im.i;

    return (image *)im;
}



int
tiff_raw_read(image_tiff *im, char **bp)
{
    uint32 *bc;
    int n;

    if (im->row >= TIFFNumberOfStrips(im->tif))
	return 0;

    if (TIFFGetField(im->tif, TIFFTAG_STRIPBYTECOUNTS, &bc) == 0)
	throws(EINVAL, "tiff: cannot get size of strips");

    n = bc[im->row];
    if (n > im->buf_size) {
	free(im->buf);
	im->buf = xmalloc(n);
	im->buf_size = n;
    }
    if ((n=TIFFReadRawStrip(im->tif, im->row, im->buf, n)) == -1)
	throwf(EIO, "tiff: error reading strip %d", im->row);

    im->row++;
    *bp = im->buf;
    return n;
}



void
tiff_raw_read_finish(image_tiff *im, int abortp)
{
    free(im->buf);
    im->buf = NULL;
    im->buf_size = 0;
}



void
tiff_raw_read_start(image_tiff *im)
{
    free(im->buf);
    im->buf = NULL;
    im->buf_size = 0;
    im->row = 0;
}



int
tiff_read(image_tiff *im, char **bp)
{
    if (TIFFReadScanline(im->tif, im->buf, (uint32)im->row, 0) == -1)
	throwf(EIO, "tiff: error reading row %d", im->row);

    *bp = im->buf;
    im->row++;

    return 0;
}



void
tiff_read_finish(image_tiff *im, int abortp)
{
    free(im->buf);
    im->buf = NULL;

    return;
}



void
tiff_read_start(image_tiff *im)
{
    im->buf = xmalloc(image_get_row_size((image *)im));
    im->row = 0;

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
    case PHOTOMETRIC_SEPARATED:
	return IMAGE_CS_CMYK;
    default:
	return IMAGE_CS_UNKNOWN;
    }
}



static void
_error_handler(const char* module, const char* fmt, va_list ap)
{
    char *s;
    
    vasprintf(&s, fmt, ap);
    throws(EINVAL, s);
}



#if 0
static image_order
_order_tiff2img(int or)
{
    static const image_order t[] = {
	IMAGE_ORD_ROW_LT, IMAGE_ORD_ROW_RT, IMAGE_ORD_ROW_RB,
	IMAGE_ORD_ROW_LB, IMAGE_ORD_COL_TL, IMAGE_ORD_COL_TR,
	IMAGE_ORD_COL_BR, IMAGE_ORD_COL_BL
    };

    if (or < 1 || or > 8)
	return IMAGE_ORD_UNKNOWN;

    return t[or];
}
#endif

#endif /* USE_TIFF */
