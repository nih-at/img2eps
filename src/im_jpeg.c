/*
  $NiH: im_jpeg.c,v 1.5 2002/09/10 21:40:47 dillo Exp $

  im_jpeg.c -- JPEG image handling
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include "config.h"

#ifdef USE_JPEG

#include <errno.h>
#include <setjmp.h>
#include <stdio.h>

#include <jpeglib.h>

#include "exceptions.h"
#include "image.h"
#include "xmalloc.h"

static image_cs_type cspace_jpg2img(int cs);
static void error_exit(j_common_ptr cinfo);



struct image_jpeg {
    image im;

    struct jpeg_decompress_struct *cinfo;
    struct jpeg_error_mgr *jerr;
    FILE *f;
    char *buf;
    int buflen;
};

IMAGE_DECLARE(jpeg);



int
jpeg_close(image_jpeg *im)
{
    int ret;

    if (im->cinfo)
	jpeg_destroy_decompress(im->cinfo);
    free(im->cinfo);
    free(im->jerr);
    ret = fclose(im->f);
    free(im->buf);
    image_free((image *)im);

    return ret;
}



char *
jpeg_get_palette(image_jpeg *im)
{
    return NULL;
}



image *
jpeg_open(char *fname)
{
    image_jpeg *im;
    FILE *f;
    exception ex;

    if ((f=fopen(fname, "rb")) == NULL)
	return NULL;

    if (catch(&ex) == 0) {
	im = image_create(jpeg, fname);
	drop();
    }
    else {
	fclose(f);
	throw(&ex);
    }

    im->f = f;
    im->cinfo = NULL;
    im->jerr = NULL;
    im->buf = NULL;

    if (catch(&ex) == 0) {
	im->cinfo = xmalloc(sizeof(*im->cinfo));
	im->jerr= xmalloc(sizeof(*im->jerr));
    
	im->cinfo->err = jpeg_std_error(im->jerr);
	im->cinfo->client_data = im;
	im->jerr->error_exit = error_exit;
	jpeg_create_decompress(im->cinfo);
	jpeg_stdio_src(im->cinfo, im->f);
	
	if (jpeg_read_header(im->cinfo, TRUE) < 0) {
	    image_free((image *)im);
	    return NULL;
	}

	drop();
    }
    else {
	image_free((image *)im);
	return NULL;
    }

    im->im.i.width = im->cinfo->image_width;
    im->im.i.height = im->cinfo->image_height;
    im->im.i.compression = IMAGE_CMP_DCT;
    im->im.i.cspace.type = cspace_jpg2img(im->cinfo->jpeg_color_space);
    im->im.i.cspace.depth = 8; /* XXX: correct for compressed? */
    /* XXX: alpha */

    return (image *)im;
}



int
jpeg_read(image_jpeg *im, char **bp)
{
    char *a[1];

    if (im->cinfo->output_scanline >= im->cinfo->output_height)
	return -1;

    a[0] = im->buf;
    jpeg_read_scanlines(im->cinfo, (JSAMPARRAY)a, 1);

    *bp = im->buf;
    return im->buflen;
}



int
jpeg_read_finish(image_jpeg *im, int abortp)
{
    free(im->buf);
    im->buf = NULL;

    if (abortp)
	jpeg_abort_decompress(im->cinfo);
    else
	jpeg_finish_decompress(im->cinfo);

    return 0;
}



int
jpeg_read_start(image_jpeg *im)
{
    jpeg_start_decompress(im->cinfo);

    im->buflen = image_get_row_size((image *)im);
    im->buf = xmalloc(im->buflen);

    return 0;
}		



int
jpeg_set_cspace(image_jpeg *im, const image_cspace *cspace)
{
    /* support for RGB->grayscale */
    /* XXX: more may be supported, need to check */

    if (cspace->depth && cspace->depth != im->im.i.cspace.depth)
	return -1;
    if (cspace->transparency != IMAGE_TR_UNKNOWN
	&& cspace->transparency != im->im.i.cspace.transparency)
	return -1;

    if (cspace->type == IMAGE_CS_UNKNOWN
	|| cspace->type == im->im.i.cspace.type)
	return 0;
    
    switch (cspace->type) {
    case IMAGE_CS_GRAY:
	im->cinfo->out_color_space = JCS_GRAYSCALE;
	break;

    default:
	return -1;
    }

    im->im.i.cspace.type = cspace->type;
    return 0;
}



int
jpeg_set_size(image_jpeg *im, int w, int h)
{
    int i;
    
    for (i=1; i<16; i*=2) {
	if (w*i == im->cinfo->image_width && h*i == im->cinfo->image_height) {
	    im->cinfo->scale_denom = i;
	    im->im.i.width = w;
	    im->im.i.height = h;
	    return 0;
	}
    }

    return -1;
}



static image_cs_type
cspace_jpg2img(int cs)
{
    switch (cs) {
    case JCS_GRAYSCALE:
	return IMAGE_CS_GRAY;
    case JCS_RGB:
    case JCS_YCbCr:
	return IMAGE_CS_RGB;
    case JCS_YCCK:
    case JCS_CMYK:
	return IMAGE_CS_CMYK;

    default:
	return IMAGE_CS_UNKNOWN;
    }
}



static void
error_exit(j_common_ptr cinfo)
{
    char b[8192];

    cinfo->err->format_message (cinfo, b);
    throws(errno ? errno : EINVAL, xstrdup(b));
}

#endif /* USE_JPEG */
