/*
  $NiH$

  im_convert.c -- image conversion handling
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include "config.h"

#include "image.h"



struct image_conv {
    image im;

    image *oim;
    image_info i;
};

IMAGE_DECLARE(conv);



int
conv_close(image_conv *im)
{
    int ret;
    
    ret = image_close(im->oim);
    
    image_free((image *)im);

    return ret;
}



char *
conv_get_palette(image_conv *im)
{
    if (!IMAGE_CS_IS_INDEXED(im->im.i.cspace))
	return NULL;

    return image_get_palette(im->oim);
}



image *
conv_open(char *fname)
{
    return NULL;
}



int
conv_read(image_conv *im, char **bp)
{
    return -1;
}



int
conv_read_start(image_conv *im)
{
    return 0;
}



int
conv_read_finish(image_conv *im, int abortp)
{
    return 0;
}



int
conv_set_cspace(image_conv *im, image_cspace cspace)
{
    return -1;
}



int
conv_set_depth(image_conv *im, int depth)
{
    return -1;
}



int
conv_set_size(image_conv *im, int w, int h)
{
    return -1;
}



image *
image_convert(image *oim, image_info *i)
{
    image_conv *im;
    int need_conv;

    need_conv = 0;

    if (i->order != IMAGE_ORD_UNKNOWN && i->order != im->i.order) {
	/* reordering of samples not supported */
	return NULL;
    }

    if ((i->width && i->width != oim->i.width)
	|| (i->height && i->height != oim->i.height)) {
	if (image_set_size(oim, i->width, i->height) < 0) {
	    need_conv = 1;
	    /* XXX: not yet */
	    return NULL;
	}
    }
    if (i->cspace != IMAGE_CS_UNKNOWN && i->cspace != im->i.cspace) {
	if (image_set_cspace(oim, i->cspace) < 0) {
	    need_conv = 1;
	    /* XXX: not yet */
	    return NULL;
	}
    }
    if (i->depth && i->depth != im->i.depth) {
	if (image_set_depth(oim, i->depth) < 0) {
	    need_conv = 1;
	    /* XXX: not yet */
	    return NULL;
	}
    }

    if (need_conv) {
	if ((im=image_create(conv, oim->fname)) == NULL) {
	    return NULL;
	}

	/* XXX */

	return (image *)im;
    }
    else
	return oim;
}
