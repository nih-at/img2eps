/*
  $NiH: im_tiff.c,v 1.10 2002/10/10 10:58:40 dillo Exp $

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



struct image_tiff {
    image im;

    TIFF *tif;

    int row;		/* current row (stripe in raw read) */
    char *buf;		/* buffer */
    int buf_size;	/* size of buffer (only valid in raw read) */
    char *pal;		/* palette */
};

IMAGE_DECLARE(tiff);

static void _error_handler(const char* module, const char* fmt, va_list ap);
static void _get_cspace(image_tiff *im);
#if 0
static image_order _order_tiff2img(int or);
#endif



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
    uint16 u16;
    uint32 u32;
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
	_get_cspace(im);

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



static void
_error_handler(const char* module, const char* fmt, va_list ap)
{
    char *s;
    
    vasprintf(&s, fmt, ap);
    throws(EINVAL, s);
}



static void
_get_cspace(image_tiff *im)
{
    uint32 u32;
    uint16 u16, type, cmp, ocmp, nex, *ex;

    /* size */

    if (TIFFGetField(im->tif, TIFFTAG_IMAGEWIDTH, &u32) == 0)
	throws(EINVAL, "image width not defined");
    im->im.i.width = u32;
    
    if (TIFFGetField(im->tif, TIFFTAG_IMAGELENGTH, &u32) == 0)
	throws(EINVAL, "image height not defined");
    im->im.i.height = u32;

    
    /* depth */

    if (TIFFGetField(im->tif, TIFFTAG_BITSPERSAMPLE, &u16) == 0)
	throws(EINVAL, "image depth not defined");
    im->im.i.cspace.depth = u16;

    
    /* orientation */
#if 0
    /* XXX: does libtiff take care of this?! */
    if (TIFFGetField(im->tif, TIFFTAG_ORIENTATION, &u16) == 0) {
	im->im.i.order = _order_tiff2img(u16);
	if (im->im.i.order == IMAGE_ORD_UNKNOWN)
	    throwf(EINVAL, "unknown image order `%d'", u16);
    }
#endif

    
    if (TIFFGetField(im->tif, TIFFTAG_PHOTOMETRIC, &type) == 0)
	throws(EINVAL, "image colour space type not defined");
    if (TIFFGetField(im->tif, TIFFTAG_COMPRESSION, &cmp) == 0)
	cmp = COMPRESSION_NONE;


    /* alpha channel */

    if (TIFFGetField(im->tif, TIFFTAG_EXTRASAMPLES, &nex, &ex) == 1) {
	if (nex > 1)
	    throws(EOPNOTSUPP, "more than one extra sample not supported");
	if (ex[0] == EXTRASAMPLE_UNASSALPHA) {
	    im->im.i.cspace.transparency = IMAGE_TR_ALPHA;
	}
	else
	    throwf(EINVAL, "unknown extra sample type `%d'", ex[0]);
    }

    /* colour space type */

    switch(type) {
    case PHOTOMETRIC_MINISWHITE:
    case PHOTOMETRIC_MINISBLACK:
	/* XXX: handle reversed grayscale */
	type = IMAGE_CS_GRAY;
	break;
    case PHOTOMETRIC_RGB:
	type = IMAGE_CS_RGB;
	break;
    case PHOTOMETRIC_PALETTE:
	type = IMAGE_CS_INDEXED;
	break;
    case PHOTOMETRIC_SEPARATED:
	type = IMAGE_CS_CMYK;
	break;
    case PHOTOMETRIC_YCBCR:
	if (cmp == COMPRESSION_OJPEG || cmp == COMPRESSION_JPEG)
	    type = IMAGE_CS_RGB;
	else
	    throwf(EINVAL, "unknown colour space type `%d'", type);
	break;
    default:
	throwf(EINVAL, "unknown colour space type `%d'", type);
    }
    
    if (type == IMAGE_CS_INDEXED) {
	im->im.i.cspace.base_type = IMAGE_CS_RGB;
	im->im.i.cspace.base_depth = 16;
	im->im.i.cspace.ncol = 1<<im->im.i.cspace.depth;
    }
    im->im.i.cspace.type = type;


    /* compression */

    switch(cmp) {
	/* XXX: CCITT? */
    case COMPRESSION_LZW:
	ocmp = cmp = IMAGE_CMP_LZW;
	break;
    case COMPRESSION_JPEG:
    case COMPRESSION_OJPEG:
	ocmp = cmp = IMAGE_CMP_DCT;
	break;
    case COMPRESSION_PACKBITS:
	ocmp = cmp = IMAGE_CMP_RLE;
	break;
    case COMPRESSION_ADOBE_DEFLATE:
    case COMPRESSION_DEFLATE:
	ocmp = cmp = IMAGE_CMP_FLATE;
	break;
    default:
	ocmp = cmp = IMAGE_CMP_NONE;
    }
    
    if (TIFFNumberOfStrips(im->tif) > 1	&& cmp != IMAGE_CMP_RLE)
	cmp = IMAGE_CMP_NONE;

    im->im.oi = im->im.i;
    im->im.i.compression = cmp;
    im->im.oi.compression = ocmp;
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
