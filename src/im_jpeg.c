/*
  $NiH$

  im_jpeg.c -- JPEg image handling
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include "config.h"

#ifdef USE_JPEG

#include <setjmp.h>
#include <stdio.h>

#include <jpeglib.h>

#include "image.h"

static image_cspace cspace_jpg2img(int cs);
static void error_exit(j_common_ptr cinfo);



struct image_jpeg {
    image im;

    struct jpeg_decompress_struct *cinfo;
    struct jpeg_error_mgr *jerr;
    FILE *f;
    jmp_buf jmp;
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



image *
jpeg_open(char *fname)
{
    image_jpeg *im;
    FILE *f;

    if ((f=fopen(fname, "rb")) == NULL)
	return NULL;

    if ((im=image_create(jpeg, fname)) == NULL) {
	fclose(f);
	return NULL;
    }

    im->f = f;
    im->cinfo = NULL;
    im->jerr = NULL;
    im->buf = NULL;

    if ((im->cinfo=malloc(sizeof(*im->cinfo))) == NULL) {
	image_free((image *)im);
	return NULL;
    }
    if ((im->jerr=malloc(sizeof(*im->jerr))) == NULL) {
	image_free((image *)im);
	return NULL;
    }
    
    im->cinfo->err = jpeg_std_error(im->jerr);
    im->cinfo->client_data = im;
    im->jerr->error_exit = error_exit;
    jpeg_create_decompress(im->cinfo);
    jpeg_stdio_src(im->cinfo, im->f);

    if (setjmp(im->jmp) == 0) {
	if (jpeg_read_header(im->cinfo, TRUE) < 0) {
	    image_free((image *)im);
	    return NULL;
	}
    }
    else {
	image_free((image *)im);
	return NULL;
    }

    im->im.i.width = im->cinfo->image_width;
    im->im.i.height = im->cinfo->image_height;
    im->im.i.compression = IMAGE_CMP_DCT;
    im->im.i.depth = 8; /* XXX: correct for compressed? */
    im->im.i.cspace = cspace_jpg2img(im->cinfo->jpeg_color_space);
    if (im->im.i.cspace == IMAGE_CS_UNKNOWN) {
	/*
	  color space not supported by PostScript,
	  we have to decompress
	*/
	im->im.i.cspace = cspace_jpg2img(im->cinfo->out_color_space);
	im->im.i.compression = IMAGE_CMP_NONE;
    }

    return (image *)im;
}



int
jpeg_read(image_jpeg *im, char **bp)
{
    char *a[1];

    if (im->cinfo->output_scanline >= im->cinfo->output_height)
	return -1;

    if (setjmp(im->jmp) == 0) {
	a[0] = im->buf;
	jpeg_read_scanlines(im->cinfo, (JSAMPARRAY)a, 1);
    }
    else
	return -1;

    *bp = im->buf;
    return im->buflen;
}



int
jpeg_read_finish(image_jpeg *im, int abortp)
{
    free(im->buf);
    im->buf = 0;

    if (setjmp(im->jmp) == 0) {
	if (abortp)
	    jpeg_abort_decompress(im->cinfo);
	else
	    jpeg_finish_decompress(im->cinfo);
    }
    else
	return -1;

    return 0;
}



int
jpeg_read_start(image_jpeg *im)
{
    if (setjmp(im->jmp) == 0)
	jpeg_start_decompress(im->cinfo);
    else
	return -1;

    im->buflen = im->im.i.width*image_cspace_components(im->im.i.cspace);
    if ((im->buf=malloc(im->buflen))
	== NULL)
	return -1;

    return 0;
}		



static image_cspace
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
    longjmp(((image_jpeg *)cinfo->client_data)->jmp, 1);
}

#endif /* USE_JPEG */
