/*
  $NiH: im_convert.c,v 1.5 2002/09/12 12:31:13 dillo Exp $

  im_convert.c -- image conversion handling
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <errno.h>

#define NOSUPP_RAW

#include "config.h"
#include "exceptions.h"
#include "image.h"



struct image_conv {
    image im;

    image *oim;
    int mask;		/* which conversions are needed */
};

IMAGE_DECLARE(conv);



void
conv_close(image_conv *im)
{
    image_close(im->oim);
    image_free((image *)im);
}



char *
conv_get_palette(image_conv *im)
{
    if (im->im.i.cspace.type != IMAGE_CS_INDEXED)
	return NULL;

    return image_get_palette(im->oim);
}



image *
conv_open(char *fname)
{
    throwf(EOPNOTSUPP, "cannot open image conversion from file");

    return NULL;
}



int
conv_read(image_conv *im, char **bp)
{
    return -1;
}



void
conv_read_start(image_conv *im)
{
    return;
}



void
conv_read_finish(image_conv *im, int abortp)
{
    return;
}



int
conv_set_cspace(image_conv *im, int mask, const image_cspace *cspace)
{
    return mask;
}



int
conv_set_size(image_conv *im, int w, int h)
{
    return -1;
}



image *
image_convert(image *oim, int mask, const image_info *i)
{
    image_conv *im;
    int m2;

    mask &= ~IMAGE_INF_COMPRESSION;

    if ((mask & IMAGE_INF_ORDER) && i->order != im->im.i.order)
	throwf(EOPNOTSUPP, "reordering of samples not supported");

    if (mask & IMAGE_INF_SIZE) {
	if (image_set_size(oim, i->width, i->height) == 0)
	    mask &= ~IMAGE_INF_SIZE;
	else {
	    /* XXX: not yet */
	    throwf(EOPNOTSUPP, "scaling not supported");
	}
    }
    if (mask & IMAGE_INF_CSPACE) {
	m2 = image_set_cspace(oim, mask&IMAGE_INF_CSPACE, &i->cspace);

	mask = (mask&~IMAGE_INF_CSPACE) | m2;

	if (m2 & IMAGE_INF_CSPACE) {
	    /* XXX: not yet */
	    throwf(EOPNOTSUPP, "color space / depth conversion not supported");
	}
    }

    if (mask) {
	im = image_create(conv, oim->fname);

	/* XXX */

	return (image *)im;
    }
    else
	return oim;
}
