/*
  $NiH: im_png.c,v 1.12 2002/10/12 18:18:23 dillo Exp $

  im_png.c -- PNG image handling
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <img2eps@nih.at>

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
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#include "config.h"

#ifdef USE_PNG

#include <errno.h>
#include <stdio.h>

#include <png.h>

#define NOSUPP_SCALE
#define NOSUPP_RAW
#include "exceptions.h"
#include "image.h"
#include "xmalloc.h"



struct image_png {
    image im;

    FILE *f;
    png_structp png;
    png_infop info, endinfo;

    int currow;
    char **rows;
    char *buf;
    char *pal;
};

IMAGE_DECLARE(png);

static void _error_fn(png_structp png, png_const_charp msg);
static void _warn_fn(png_structp png, png_const_charp msg);




void
png_close(image_png *im)
{
    if (im->png)
	png_destroy_read_struct(&im->png, &im->info, &im->endinfo);
    if (im->f)
	fclose(im->f);
    free(im->rows);
    free(im->buf);
    free(im->pal);

    image_free((image *)im);
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
	n *= image_cspace_components(&im->im.i.cspace, 1);
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
    unsigned char sig[8];
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
	
	im->im.i.width = png_get_image_width(im->png, im->info);
	im->im.i.height = png_get_image_height(im->png, im->info);
	im->im.i.cspace.depth = png_get_bit_depth(im->png, im->info);
	switch (png_get_color_type(im->png, im->info)) {
	case PNG_COLOR_TYPE_GRAY_ALPHA:
	    im->im.i.cspace.transparency = IMAGE_TR_ALPHA;
	    /* fallthrough */
	case PNG_COLOR_TYPE_GRAY:
	    im->im.i.cspace.type = IMAGE_CS_GRAY;
	    break;
	case PNG_COLOR_TYPE_PALETTE:
	    im->im.i.cspace.type = IMAGE_CS_INDEXED;
	    png_get_PLTE(im->png, im->info, &plte,
			 &im->im.i.cspace.ncol);
	    /* XXX: set num colors */
	    /* XXX: always correct? */
	    im->im.i.cspace.base_type = IMAGE_CS_RGB;
	    im->im.i.cspace.base_depth = 8;
	    break;
	case PNG_COLOR_TYPE_RGB_ALPHA:
	    im->im.i.cspace.transparency = IMAGE_TR_ALPHA;
	    /* fallthrough */
	case PNG_COLOR_TYPE_RGB:
	    im->im.i.cspace.type = IMAGE_CS_RGB;
	    break;
	}

	switch (png_get_compression_type(im->png, im->info)) {
	case PNG_COMPRESSION_TYPE_BASE:
	    im->im.i.compression = IMAGE_CMP_FLATE;
	    break;
	default:
	    im->im.i.compression = IMAGE_CMP_NONE;
	    break;
	}
	im->im.i.order = IMAGE_ORD_ROW_LT;

	im->im.oi = im->im.i;

	/* raw read not supported yet */
	im->im.i.compression = IMAGE_CMP_NONE;

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
	png_read_row(im->png, (unsigned char *)im->buf, NULL);
	*bp = im->buf;
    }

    return 0;
}



void
png_read_finish(image_png *im, int abortp)
{
    free(im->buf);
    im->buf = NULL;
    free(im->rows);
    im->rows = NULL;
}



void
png_read_start(image_png *im)
{
    int i, n;
    exception ex;

    if (catch(&ex) == 0) {
	if (im->im.oi.cspace.type == IMAGE_CS_INDEXED
	    && im->im.i.cspace.type != im->im.oi.cspace.type)
	    png_set_palette_to_rgb(im->png);
	
	if (im->im.oi.cspace.depth != im->im.i.cspace.depth) {
	    if (im->im.oi.cspace.type == IMAGE_CS_GRAY
		&& im->im.oi.cspace.depth /* XXX ?! */)
		png_set_expand_gray_1_2_4_to_8(im->png);
	    else if (im->im.oi.cspace.depth == 16)
		png_set_strip_16(im->png);
	}
	
	/* XXX: combine with background? */
	png_set_strip_alpha(im->png);
	
	if (im->im.oi.cspace.type != im->im.i.cspace.type) {
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
}		



int
png_set_cspace(image_png *im, int mask, const image_cspace *cspace)
{
    /* XXX: this routine is a mess, and it's broken.  indexed handling
       doesn't work together with other parameters */

    if (mask & IMAGE_INF_TYPE) {
	switch (cspace->type) {
	case IMAGE_CS_INDEXED:
	    if (im->im.oi.cspace.type == IMAGE_CS_INDEXED
		&& ((mask & IMAGE_INF_DEPTH) == 0
		    || cspace->depth == im->im.oi.cspace.depth)) {
		im->im.i.cspace.type = IMAGE_CS_INDEXED;
		im->im.i.cspace.depth = im->im.oi.cspace.depth;
		mask &= ~(IMAGE_INF_TYPE|IMAGE_INF_DEPTH);

		if ((mask & IMAGE_INF_BASE_TYPE) == 0
		    || cspace->base_type == im->im.oi.cspace.base_type) {
		    im->im.i.cspace.base_type = im->im.oi.cspace.base_type;
		    mask &= ~IMAGE_INF_BASE_TYPE;
		}
		if ((mask & IMAGE_INF_BASE_DEPTH) == 0
		    || cspace->base_depth == im->im.oi.cspace.base_depth) {
		    im->im.i.cspace.base_depth = im->im.oi.cspace.base_depth;
		    mask &= ~IMAGE_INF_BASE_DEPTH;
		}
	    }
	    break;
	    
	case IMAGE_CS_GRAY:
	case IMAGE_CS_RGB:
	    if (im->im.oi.cspace.type == IMAGE_CS_INDEXED
		&& cspace->type != im->im.oi.cspace.base_type) {
		/* palette to grayscale doesn't work reliably */
		break;
	    }
	    if ((mask & IMAGE_INF_DEPTH) == 0) {
		if (im->im.i.cspace.type == IMAGE_CS_INDEXED)
		    im->im.i.cspace.depth = im->im.oi.cspace.base_depth;
	    }
	    else {
		if (cspace->depth == 8
		    || (im->im.oi.cspace.type == IMAGE_CS_INDEXED
			&& cspace->depth == im->im.oi.cspace.base_depth)
		    || (im->im.oi.cspace.type != IMAGE_CS_INDEXED
			&& cspace->depth == im->im.oi.cspace.depth)) {
		    im->im.i.cspace.depth = cspace->depth;
		    mask &= ~IMAGE_INF_DEPTH;
		}
	    }
	    im->im.i.cspace.type = cspace->type;
	    mask &= ~IMAGE_INF_TYPE;
	    break;

	default:
	    ;
	}
    }

    if (mask & IMAGE_INF_TRANSPARENCY) {
	if ((cspace->transparency == im->im.oi.cspace.transparency
	     || cspace->transparency == IMAGE_TR_NONE)) {
	    im->im.i.cspace.transparency = cspace->transparency;
	    mask &= ~IMAGE_INF_TRANSPARENCY;
	}
    }

    return mask;
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

