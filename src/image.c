/*
  $NiH: image.c,v 1.1 2002/09/07 20:58:00 dillo Exp $

  image.c -- general image functions
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <string.h>

#include "config.h"
#include "image.h"

#ifdef USE_GIF
image *gif_open(char *);
#endif
#ifdef USE_JPEG
image *jpeg_open(char *);
#endif
#ifdef USE_PNG
image *png_open(char *);
#endif
#ifdef USE_TIFF
image *tiff_open(char *);
#endif

image *(*open_tab[])(char *) = {
#ifdef USE_PNG
    png_open,
#endif
#ifdef USE_JPEG
    jpeg_open,
#endif
#ifdef USE_TIFF
    tiff_open,
#endif
#ifdef USE_GIF
    gif_open,
#endif
    NULL
};



image *
_image_create(struct image_functions *f, size_t size, char *fname)
{
    image *im;

    if ((im=malloc(size)) == NULL)
	return NULL;

    im->f = f;
    im->fname = strdup(fname);
    image_init_info(&im->i);

    return im;
}



int
_image_notsup(image *im, int a0, int a1)
{
    return -1;
}



int
image_cspace_components(image_cspace cspace)
{
    switch (cspace) {
    case IMAGE_CS_GRAY:
    case IMAGE_CS_INDEXED_GRAY:
    case IMAGE_CS_INDEXED_RGB:
    case IMAGE_CS_INDEXED_CMYK:
	return 1;
    case IMAGE_CS_RGB:
	return 3;
    case IMAGE_CS_CMYK:
	return 4;

    default:
	return 0;
    }
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
image_get_palette_size(image *im)
{
    if (!IMAGE_CS_IS_INDEXED(im->i.cspace))
	return 0;

    return image_cspace_components(IMAGE_CS_BASE(im->i.cspace))
	* (1<<im->i.depth);
}



int
image_get_row_size(image *im)
{
    int n;

    n = im->i.width * image_cspace_components(im->i.cspace);
    n = (n*8 + im->i.depth-1) / im->i.depth;

    return n;
}



void
image_init_info(image_info *i)
{
    i->width = i->height = 0;
    i->cspace = IMAGE_CS_UNKNOWN;
    i->depth = 0;
    i->compression = IMAGE_CMP_UNKNOWN;
    i->order = IMAGE_ORD_UNKNOWN;
}



image *
image_open(char *fname)
{
    image *im;
    int i;

    for (i=0; open_tab[i]; i++) {
	if ((im=open_tab[i](fname)) != NULL)
	    return im;
    }

    return NULL;
}
