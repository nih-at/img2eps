/*
  $NiH: image.c,v 1.5 2002/09/11 22:44:19 dillo Exp $

  image.c -- general image functions
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "exceptions.h"
#include "image.h"
#include "xmalloc.h"

#ifdef USE_GIF
image *gif_open(const char *);
#endif
#ifdef USE_JPEG
image *jpeg_open(const char *);
#endif
#ifdef USE_PNG
image *png_open(const char *);
#endif
#ifdef USE_TIFF
image *tiff_open(const char *);
#endif
image *xpm_open(const char *);


image *(*open_tab[])(const char *) = {
#ifdef USE_PNG
    png_open,
#endif
#ifdef USE_JPEG
    jpeg_open,
#endif
#ifdef USE_TIFF
    tiff_open,
#endif
    xpm_open,
#ifdef USE_GIF
    gif_open,
#endif
    NULL
};



image *
_image_create(struct image_functions *f, size_t size, const char *fname)
{
    image *im;

    im = xmalloc(size);
    im->f = f;
    im->fname = strdup(fname);
    image_init_info(&im->i);

    im->i.cspace.transparency = IMAGE_TR_NONE;
    im->i.compression = IMAGE_CMP_NONE;
    im->i.order = IMAGE_ORD_ROW_LT;


    return im;
}



int
_image_notsup(image *im, int a0, int a1)
{
    return -1;
}



int
_image_notsup_raw(image *im, int a0, int a1)
{
    throws(EOPNOTSUPP, "raw reading not supported");
    return -1;
}



int
image_cspace_components(const image_cspace *cspace, int base)
{
    int n;
    switch (base ? cspace->base_type : cspace->type) {
    case IMAGE_CS_GRAY:
    case IMAGE_CS_INDEXED:
	n = 1;
	break;
    case IMAGE_CS_RGB:
    case IMAGE_CS_HSV:
	n = 3;
	break;
    case IMAGE_CS_CMYK:
	n = 4;
	break;

    default:
	return 0;
    }

    if (!base && cspace->transparency == IMAGE_TR_ALPHA)
	n += 1;

    return n;
}



void
image_cspace_merge(image_cspace *cst, const image_cspace *css)
{
    if (css->type != IMAGE_CS_UNKNOWN)
	cst->type = css->type;
    if (css->transparency != IMAGE_TR_UNKNOWN)
	cst->transparency = css->transparency;
    if (css->depth != 0)
	cst->depth = css->depth;
    if (css->type == IMAGE_CS_INDEXED) {
	if (css->base_type != IMAGE_CS_UNKNOWN)
	    cst->base_type = css->base_type;
	if (css->base_depth != 0)
	    cst->base_depth = css->base_depth;
    }
}



int
image_cspace_palette_size(const image_cspace *cspace)
{
    if (cspace->type != IMAGE_CS_INDEXED)
	return 0;

    return image_cspace_components(cspace, 1) * (1<<cspace->depth);
}



void
image_free(image *im)
{
    if (im == NULL)
	return;

    free(im->fname);
    free(im);
}



int
image_get_row_size(const image *im)
{
    int n;

    n = im->i.width * image_cspace_components(&im->i.cspace, 0);
    n = (n*im->i.cspace.depth+7) / 8;

    return n;
}



void
image_init_info(image_info *i)
{
    i->width = i->height = 0;
    i->cspace.type = IMAGE_CS_UNKNOWN;
    i->cspace.transparency = IMAGE_TR_UNKNOWN;
    i->cspace.depth = 0;
    i->cspace.base_type = IMAGE_CS_UNKNOWN;
    i->cspace.base_depth = 0;
    i->cspace.ncol = 0;
    i->compression = IMAGE_CMP_UNKNOWN;
    i->order = IMAGE_ORD_UNKNOWN;
}



image *
image_open(const char *fname)
{
    image *im;
    int i;

    if (access(fname, R_OK) == -1)
	throwf(errno, "cannot open image `%s': %s",
	       fname, strerror(errno));

    for (i=0; open_tab[i]; i++) {
	if ((im=open_tab[i](fname)) != NULL)
	    return im;
    }

    throwf(EOPNOTSUPP,
	   "cannot open image `%s': format not recognized", fname);

    return NULL;
}
