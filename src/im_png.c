/*
  $NiH: im_png.c,v 1.1 2002/09/08 21:31:46 dillo Exp $

  im_png.c -- PNG image handling
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include "config.h"

#ifdef USE_PNG

#include <setjmp.h>
#include <stdio.h>

#include <png.h>

#define NOSUPP_SCALE
#include "exceptions.h"
#include "image.h"



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
    
    if (!IMAGE_CS_IS_INDEXED(im->im.i.cspace))
	return NULL;

    if (im->pal)
	return im->pal;

    png_get_PLTE(im->png, im->info, (png_colorp *)&plte, &n);
    if (n < 1<<im->im.i.depth) {
	sz = image_get_palette_size((image *)im);
	if ((im->pal=malloc(sz)) == NULL)
	    return NULL;
	n *= image_cspace_components(im->im.i.cspace);
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
	return NULL;
    }

    if ((im->info=png_create_info_struct(im->png)) == NULL) {
	image_close(im);
	return NULL;
    }

    if ((im->endinfo=png_create_info_struct(im->png)) == NULL) {
	image_close(im);
	return NULL;
    }

    if (setjmp(png_jmpbuf(im->png)) != 0) {
	image_close(im);
	return NULL;
    }
    
    png_init_io(im->png, im->f);
    png_set_sig_bytes(im->png, 8);

    png_read_info(im->png, im->info);

    im->pinfo.width = png_get_image_width(im->png, im->info);
    im->pinfo.height = png_get_image_height(im->png, im->info);
    im->pinfo.depth = png_get_bit_depth(im->png, im->info);
    switch (png_get_color_type(im->png, im->info)) {
    case PNG_COLOR_TYPE_GRAY:
    case PNG_COLOR_TYPE_GRAY_ALPHA:
	im->pinfo.cspace = IMAGE_CS_GRAY;
	break;
    case PNG_COLOR_TYPE_PALETTE:
	im->pinfo.cspace = IMAGE_CS_INDEXED_RGB;
	break;
    case PNG_COLOR_TYPE_RGB:
    case PNG_COLOR_TYPE_RGB_ALPHA:
	im->pinfo.cspace = IMAGE_CS_RGB;
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

    im->im.i = im->pinfo;

    return (image *)im;
}



int
png_read(image_png *im, char **bp)
{
    if (im->rows)
	*bp = im->rows[im->currow++];
    else {
	if (setjmp(png_jmpbuf(im->png)) != 0)
	    return -1;

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

    if (setjmp(png_jmpbuf(im->png)) != 0) {
	free(im->rows);
	im->rows = NULL;
	free(im->buf);
	im->buf = NULL;
	return -1;
    }
    
    if (im->pinfo.cspace == IMAGE_CS_INDEXED_RGB
	&& im->im.i.cspace != im->pinfo.cspace)
	png_set_palette_to_rgb(im->png);

    if (im->pinfo.depth != im->im.i.depth) {
	if (im->pinfo.cspace == IMAGE_CS_GRAY && im->pinfo.depth)
	    png_set_gray_1_2_4_to_8(im->png);
	else if (im->pinfo.depth == 16)
	    png_set_strip_16(im->png);
    }
    
    /* XXX: combine with background? */
    png_set_strip_alpha(im->png);

    if (im->pinfo.cspace != im->im.i.cspace) {
	if (im->im.i.cspace == IMAGE_CS_GRAY)
	    png_set_rgb_to_gray_fixed(im->png, 1, -1, -1);
	else if (im->im.i.cspace == IMAGE_CS_RGB)
	    png_set_gray_to_rgb(im->png);
    }

    /* XXX: bit/byte order? */

    n = image_get_row_size((image *)im);
    if (png_get_interlace_type(im->png, im->info) != PNG_INTERLACE_NONE) {
	if ((im->buf=malloc(n*im->im.i.height)) == NULL)
	    return -1;
	if ((im->rows=malloc(im->im.i.height*sizeof(*im->rows))) == NULL) {
	    free(im->buf);
	    im->buf = NULL;
	    return -1;
	}
	for (i=0; i<im->im.i.height; i++)
	    im->rows[i] = im->buf+n*i;
	im->currow = 0;
	png_read_image(im->png, (png_bytep *)im->rows);
    }
    else {
	if ((im->buf=malloc(n)) == NULL)
	    return -1;
    }
    
    return 0;
}		



int
png_set_cspace(image_png *im, image_cspace cspace)
{
    switch (cspace) {
    case IMAGE_CS_GRAY:
    case IMAGE_CS_RGB:
	im->im.i.cspace = cspace;
	return 0;

    default:
	return -1;
    }
}



int
png_set_depth(image_png *im, int depth)
{
    if (depth == im->pinfo.depth || depth == 8) {
	im->im.i.depth = depth;
	return 0;
    }
    else
	return -1;
}

#endif /* USE_PNG */
