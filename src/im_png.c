/*
  $NiH: im_png.c,v 1.5 2002/09/11 22:44:19 dillo Exp $

  im_png.c -- PNG image handling
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include "config.h"

#ifdef USE_PNG

#include <errno.h>
#include <stdio.h>

#include <png.h>

#define NOSUPP_SCALE
#include "exceptions.h"
#include "image.h"
#include "xmalloc.h"



struct image_png {
    image im;

    FILE *f;
    png_structp png;
    png_infop info, endinfo;
    image_info pinfo;

    int currow;
    char **rows;
    char *buf;
    char *pal;
};

IMAGE_DECLARE(png);

static void _error_fn(png_structp png, png_const_charp msg);
static void _warn_fn(png_structp png, png_const_charp msg);




int
png_close(image_png *im)
{
    if (setjmp(png_jmpbuf(im->png)) != 0)
	return -1;
    
    if (im->png)
	png_destroy_read_struct(&im->png, &im->info, &im->endinfo);
    if (im->f)
	fclose(im->f);
    free(im->rows);
    free(im->buf);
    free(im->pal);

    image_free((image *)im);

    return 0;
}



char *
png_get_palette(image_png *im)
{
    int n, sz;
    char *plte;
    
    if (im->im.i.cspace.type != IMAGE_CS_INDEXED)
	return NULL;

    if (im->pal)
	return im->pal;

    png_get_PLTE(im->png, im->info, (png_colorp *)&plte, &n);
    if (n < 1<<im->im.i.cspace.depth) {
	sz = image_cspace_palette_size(&im->im.i.cspace);
	im->pal = xmalloc(sz);
	n *= image_cspace_components(&im->im.i.cspace, 0);
	memcpy(im->pal, plte, n);
	memset(im->pal+n, 0, sz-n);
	return im->pal;
    }
    
    return plte;
}



image *
png_open(char *fname)
{
    image_png *im;
    char sig[8];
    FILE *f;
    exception ex;
    png_colorp plte;
    
    if ((f=fopen(fname, "rb")) == NULL)
	return NULL;

    if (fread(sig, 8, 1, f) != 1) {
	fclose(f);
	return NULL;
    }

    if (png_sig_cmp(sig, 0, 8) != 0) {
	fclose(f);
	return NULL;
    }

    if (catch(&ex) == 0) {
	im = image_create(png, fname);
	drop();
    }
    else {
	fclose(f);
	throw(&ex);
    }

    im->f = f;
    im->png = NULL;
    im->info = NULL;
    im->endinfo = NULL;
    im->rows = NULL;
    im->buf = NULL;
    im->pal = NULL;

    if ((im->png=png_create_read_struct(PNG_LIBPNG_VER_STRING,
					im, NULL, NULL)) == NULL) {
	image_close(im);
	throwf(ENOMEM, "out of memory allocating png structures");
    }

    if ((im->info=png_create_info_struct(im->png)) == NULL) {
	image_close(im);
	throwf(ENOMEM, "out of memory allocating png structures");
    }

    if ((im->endinfo=png_create_info_struct(im->png)) == NULL) {
	image_close(im);
	throwf(ENOMEM, "out of memory allocating png structures");
    }

    png_set_error_fn(im->png, NULL, _error_fn, _warn_fn);

    if (catch(&ex) == 0) {
	png_init_io(im->png, im->f);
	png_set_sig_bytes(im->png, 8);
	
	png_read_info(im->png, im->info);
	
	im->pinfo.width = png_get_image_width(im->png, im->info);
	im->pinfo.height = png_get_image_height(im->png, im->info);
	im->pinfo.cspace.depth = png_get_bit_depth(im->png, im->info);
	switch (png_get_color_type(im->png, im->info)) {
	case PNG_COLOR_TYPE_GRAY_ALPHA:
	    im->pinfo.cspace.transparency = IMAGE_TR_ALPHA;
	    /* fallthrough */
	case PNG_COLOR_TYPE_GRAY:
	    im->pinfo.cspace.type = IMAGE_CS_GRAY;
	    break;
	case PNG_COLOR_TYPE_PALETTE:
	    im->pinfo.cspace.type = IMAGE_CS_INDEXED;
	    png_get_PLTE(im->png, im->info, &plte,
			 &im->pinfo.cspace.ncol);
	    /* XXX: set num colors */
	    /* XXX: always correct? */
	    im->pinfo.cspace.base_type = IMAGE_CS_RGB;
	    im->pinfo.cspace.base_depth = 8;
	    break;
	case PNG_COLOR_TYPE_RGB_ALPHA:
	    im->pinfo.cspace.transparency = IMAGE_TR_ALPHA;
	    /* fallthrough */
	case PNG_COLOR_TYPE_RGB:
	    im->pinfo.cspace.type = IMAGE_CS_RGB;
	    break;
	}

	switch (png_get_compression_type(im->png, im->info)) {
	case PNG_COMPRESSION_TYPE_BASE:
	    im->pinfo.compression = IMAGE_CMP_FLATE;
	    break;
	default:
	    im->pinfo.compression = IMAGE_CMP_NONE;
	    break;
	}
	im->pinfo.order = IMAGE_ORD_ROW_LT;

	im->im.i = im->pinfo;

	drop();
    }
    else {
	image_close((image *)im);
	throw(&ex);
    }

    return (image *)im;
}



int
png_read(image_png *im, char **bp)
{
    if (im->rows)
	*bp = im->rows[im->currow++];
    else {
	png_read_row(im->png, im->buf, NULL);
	*bp = im->buf;
    }

    return 0;
}



int
png_read_finish(image_png *im, int abortp)
{
    free(im->buf);
    im->buf = NULL;
    free(im->rows);
    im->rows = NULL;

    return 0;
}



int
png_read_start(image_png *im)
{
    int i, n;
    exception ex;

    if (catch(&ex) == 0) {
	if (im->pinfo.cspace.type == IMAGE_CS_INDEXED
	    && im->im.i.cspace.type != im->pinfo.cspace.type)
	    png_set_palette_to_rgb(im->png);
	
	if (im->pinfo.cspace.depth != im->im.i.cspace.depth) {
	    if (im->pinfo.cspace.type == IMAGE_CS_GRAY
		&& im->pinfo.cspace.depth /* XXX ?! */)
		png_set_gray_1_2_4_to_8(im->png);
	    else if (im->pinfo.cspace.depth == 16)
		png_set_strip_16(im->png);
	}
	
	/* XXX: combine with background? */
	png_set_strip_alpha(im->png);
	
	if (im->pinfo.cspace.type != im->im.i.cspace.type) {
	    if (im->im.i.cspace.type == IMAGE_CS_GRAY)
		png_set_rgb_to_gray_fixed(im->png, 1, -1, -1);
	    else if (im->im.i.cspace.type == IMAGE_CS_RGB)
		png_set_gray_to_rgb(im->png);
	}
	
	/* XXX: bit/byte order? */
	
	n = image_get_row_size((image *)im);
	if (png_get_interlace_type(im->png, im->info) != PNG_INTERLACE_NONE) {
	    im->buf = xmalloc(n*im->im.i.height);
	    im->rows = xmalloc(im->im.i.height*sizeof(*im->rows));
	    for (i=0; i<im->im.i.height; i++)
		im->rows[i] = im->buf+n*i;
	    im->currow = 0;
	    png_read_image(im->png, (png_bytep *)im->rows);
	}
	else
	    im->buf = xmalloc(n);

	drop();
    }
    else {
	free(im->rows);
	im->rows = NULL;
	free(im->buf);
	im->buf = NULL;
	throw(&ex);
    }
    
    return 0;
}		



int
png_set_cspace(image_png *im, const image_cspace *cspace)
{
    int ret;

    ret = 0;

    /* XXX: this routine is a mess, and it's broken.  indexed handling
       doesn't work together with other parameters */
    switch (cspace->type) {
    case IMAGE_CS_INDEXED:
	if (im->pinfo.cspace.type != IMAGE_CS_INDEXED
	    || !IMAGE_CS_EQUAL(depth, cspace, &im->pinfo.cspace)
	    || !IMAGE_CS_EQUAL(base_type, cspace, &im->pinfo.cspace)
	    || !IMAGE_CS_EQUAL(base_depth, cspace, &im->pinfo.cspace))
	    ret = -1;
	else {
	    im->im.i.cspace.type = IMAGE_CS_INDEXED;
	    im->im.i.cspace.depth = im->pinfo.cspace.depth;
	    if (IMAGE_CS_EQUAL(transparency, cspace, &im->pinfo.cspace)
		|| cspace->transparency == IMAGE_TR_NONE)
		im->im.i.cspace.transparency = cspace->transparency;
	    else
		ret = -1;
	}
	break;

    case IMAGE_CS_UNKNOWN:
    case IMAGE_CS_GRAY:
    case IMAGE_CS_RGB:
	if (cspace->depth == 0) {
	    if (im->im.i.cspace.type == IMAGE_CS_INDEXED)
		im->im.i.cspace.depth = im->pinfo.cspace.base_depth;
	}
	else {
	    if (cspace->depth == 8
		|| (im->pinfo.cspace.type == IMAGE_CS_INDEXED
		    && cspace->depth == im->pinfo.cspace.base_depth)
		|| (im->pinfo.cspace.type != IMAGE_CS_INDEXED
		    && cspace->depth == im->pinfo.cspace.depth))
		im->im.i.cspace.depth = cspace->depth;
	    else
		ret = -1;
	}
	if (cspace->transparency != IMAGE_TR_UNKNOWN) {
	    if ((cspace->transparency == im->pinfo.cspace.transparency
		 || cspace->transparency == IMAGE_TR_NONE))
		im->im.i.cspace.transparency = cspace->transparency;
	    else
		ret = -1;
	}
	im->im.i.cspace.type = cspace->type;
	
	break;

    default:
	ret = -1;
    }

    return ret;
}



static void
_error_fn(png_structp png, png_const_charp msg)
{
    throws(errno ? errno : EINVAL, msg);
}



static void
_warn_fn(png_structp png, png_const_charp msg)
{
    return;
}

#endif /* USE_PNG */

