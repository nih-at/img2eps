/*
  $NiH: im_convert.c,v 1.4 2002/09/11 22:44:18 dillo Exp $

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
    image_info i;
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
conv_set_cspace(image_conv *im, const image_cspace *cspace)
{
    return -1;
}



int
conv_set_size(image_conv *im, int w, int h)
{
    return -1;
}



image *
image_convert(image *oim, const image_info *i)
{
    image_conv *im;
    int need_conv;

    need_conv = 0;

    if (i->order != IMAGE_ORD_UNKNOWN && i->order != im->i.order)
	throwf(EOPNOTSUPP, "reordering of samples not supported");

    if ((i->width && i->width != oim->i.width)
	|| (i->height && i->height != oim->i.height)) {
	if (image_set_size(oim, i->width, i->height) < 0) {
	    need_conv = 1;
	    /* XXX: not yet */
	    throwf(EOPNOTSUPP, "scaling not supported");
	}
    }
    if ((i->cspace.type != IMAGE_CS_UNKNOWN
	 && i->cspace.type != im->i.cspace.type)
	|| (i->cspace.depth && i->cspace.depth != im->i.cspace.depth)) {
	/* XXX: base type & depth */
	if (image_set_cspace(oim, &i->cspace)) {
	    need_conv = 1;
	    /* XXX: not yet */
	    throwf(EOPNOTSUPP, "color space / depth conversion not supported");
	}
    }

    if (need_conv) {
	im = image_create(conv, oim->fname);

	/* XXX */

	return (image *)im;
    }
    else
	return oim;
}
