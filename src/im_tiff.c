/*
  $NiH: im_tiff.c,v 1.16 2002/10/13 01:25:07 dillo Exp $

  im_tiff.c -- TIFF image handling
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The name of the author may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.
 
  THIS SOFTWARE IS PROVIDED BY DIETER BARON ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL DIETER BARON BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

    int row, strip;	/* current row / strip (while reading) */
    int nrow, nstrip;	/* number of rows / strips */
    char *buf;		/* buffer */
    int row_size;	/* size of row */
    char *pal;		/* palette */
    uint32 *sizes;	/* sizes of raw strips */
};

IMAGE_DECLARE(tiff);

static void _error_handler(const char* module, const char* fmt, va_list ap);
static void _get_cspace(image_tiff *im);
static int _lzw_pad(char *buf, int n);
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
    if (TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &u16)
	&& u16 != PLANARCONFIG_CONTIG) {
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
    int n;

    if (im->strip >= im->nstrip)
	return 0;

    n = im->sizes[im->strip];
    if ((n=TIFFReadRawStrip(im->tif, im->strip, im->buf, n)) == -1)
	throwf(EIO, "tiff: error reading strip %d", im->strip);

    if (im->im.i.compression == IMAGE_CMP_LZW && im->strip < im->nstrip-1)
	n = _lzw_pad(im->buf, n);

    im->strip++;
    *bp = im->buf;
    
    return n;
}



void
tiff_raw_read_finish(image_tiff *im, int abortp)
{
    free(im->buf);
    im->buf = NULL;
}



void
tiff_raw_read_start(image_tiff *im)
{
    int i, n;
    
    im->strip = 0;
    im->nstrip = TIFFNumberOfStrips(im->tif);
    if (TIFFGetField(im->tif, TIFFTAG_STRIPBYTECOUNTS, &im->sizes) == 0)
	throws(EINVAL, "tiff: cannot get size of strips");

    n = 0;
    for (i=0; i<im->nstrip; i++) {
	if (im->sizes[i] > n)
	    n = im->sizes[i];
    }
    
    free(im->buf);
    /* 7 bytes room for lzw padding */
    im->buf = xmalloc(n+7);
}



int
tiff_read(image_tiff *im, char **bp)
{
    if (im->row >= im->im.i.height)
	return 0;
    
    if (im->row % im->nrow == 0) {
	if (TIFFReadEncodedStrip(im->tif, im->strip,
				 im->buf, (tsize_t)-1) == -1)
	    throwf(EIO, "tiff: error reading strip %d", im->strip);
	im->strip++;
    }

    *bp = im->buf+im->row_size*(im->row%im->nrow);
    im->row++;

    return 0;
}



void
tiff_read_finish(image_tiff *im, int abortp)
{
    free(im->buf);
    im->buf = NULL;
}



void
tiff_read_start(image_tiff *im)
{
    uint32 u32;
    
    free(im->buf);
    im->buf = xmalloc(TIFFStripSize(im->tif));
    im->row_size = image_get_row_size((image *)im);

    if (TIFFGetField(im->tif, TIFFTAG_ROWSPERSTRIP, &u32) == 0)
	throws(EINVAL, "unknown number of rows per strip");
    if (u32 == 0xffffffff)
	u32 = im->im.i.height;
    im->row = im->strip = 0;
    im->nrow = u32;
    im->nstrip = TIFFNumberOfStrips(im->tif);

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

    if (TIFFGetField(im->tif, TIFFTAG_EXTRASAMPLES, &nex, &ex) == 1
	&& nex > 0) {
	if (nex > 1)
	    throws(EOPNOTSUPP, "more than one extra sample not supported");
	if (ex[0] == EXTRASAMPLE_UNASSALPHA) {
	    im->im.i.cspace.transparency = IMAGE_TR_ALPHA;
	}
	else {
	    /* XXX: alpha is ignored, and setting it makes conversion work */
	    im->im.i.cspace.transparency = IMAGE_TR_ALPHA;
	    /* throwf(EINVAL, "unknown extra sample type `%d'", ex[0]); */
	}
    }

    /* colour space type */

    switch(type) {
    case PHOTOMETRIC_MINISWHITE:
	im->im.i.cspace.inverted = IMAGE_INV_BRIGHTLOW;
	/* fallthrough */
    case PHOTOMETRIC_MINISBLACK:
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
	if (TIFFGetField(im->tif, TIFFTAG_PREDICTOR, &u16) == 1 && u16 != 1) {
	    /* predictors not yet supported */
	    cmp = IMAGE_CMP_NONE;
	}
	break;
    case COMPRESSION_JPEG:
    case COMPRESSION_OJPEG:
	/* jpeg in tiff can be more complicated than a simple jpeg stream */
	ocmp = IMAGE_CMP_DCT;
	cmp = IMAGE_CMP_NONE;
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
    
    if (TIFFNumberOfStrips(im->tif) > 1 &&
	cmp != IMAGE_CMP_RLE && cmp != IMAGE_CMP_LZW) {
	/* simply concatenating compressed streams doesn't work for
	   most schemes */
	cmp = IMAGE_CMP_NONE;
    }

    im->im.oi = im->im.i;
    im->im.i.compression = cmp;
    im->im.oi.compression = ocmp;
}



/*
  post process LZW compressed strip so concatenation works:
  replace EOI with CLEAR, and pad to byte boundary with CLEARs
*/

static int
_lzw_pad(char *buf, int n)
{
    unsigned char *p;
    int b, c;

    p = (unsigned char *)buf+n-2;

    /* find EOI (and thus number of free bits in last byte) */
    c = p[0]<<8 | p[1];
    p++;
    for (b=0; b<8; b++) {
	if (((c>>b) & 0x1ff) == 0x101)
	    break;
    }
    if (b == 8)
	throws(EINVAL, "EOI code not found in LZW stream");

    /* convert EOI to CLEAR and clear free bits */
    *p = 0;

    /* pad to byte boundary with (9 bit) CLEAR codes */
    while (b) {
	*p++ |= 1<<--b;
	*p = 0;
	n++;
    }

    return n;
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
