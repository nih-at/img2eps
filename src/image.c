/*
  $NiH: image.c,v 1.3 2002/09/10 14:05:52 dillo Exp $

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
image *xpm_open(char *);


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
    xpm_open,
#ifdef USE_GIF
    gif_open,
#endif
    NULL
};



image *
_image_create(struct image_functions *f, size_t size, char *fname)
{
    image *im;

    im = xmalloc(size);
    im->f = f;
    im->fname = strdup(fname);
    image_init_info(&im->i);
    im->i.order = IMAGE_ORD_ROW_LT;

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
    n = (n*im->i.depth+7) / 8;

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



int
image_set_cspace(image *im, image_cspace cspace)
{
    return image_set_cspace_depth(im, cspace, im->i.depth);
}



int
image_set_depth(image *im, int depth)
{
    return image_set_cspace_depth(im, im->i.cspace, depth);
}
