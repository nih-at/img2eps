/*
  $NiH: im_convert.c,v 1.6 2002/09/14 02:27:38 dillo Exp $

  im_convert.c -- image conversion handling
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <errno.h>

#include "config.h"
#include "exceptions.h"
#include "image.h"
#include "xmalloc.h"



struct image_conv {
    image im;

    image *oim;
    int mask;		/* which conversions are needed */

    char *pal;		/* palette */
};

IMAGE_DECLARE(conv);

static void _depth16to8(char *pd, char *ps, int n);



void
conv_close(image_conv *im)
{
    free (im->pal);
    image_close(im->oim);
    image_free((image *)im);
}



char *
conv_get_palette(image_conv *im)
{
    if (im->im.i.cspace.type != IMAGE_CS_INDEXED)
	return NULL;

    if (im->mask & IMAGE_INF_BASE_DEPTH) {
	/* XXX: currently only 16->8 */
	free(im->pal);
	im->pal = xmalloc(image_cspace_palette_size(&im->im.i.cspace));
	_depth16to8(im->pal, image_get_palette(im->oim),
		    image_cspace_palette_size(&im->im.i.cspace));
	return im->pal;
    }
    else
	return image_get_palette(im->oim);
}



image *
conv_open(char *fname)
{
    throwf(EOPNOTSUPP, "cannot open image conversion from file");

    return NULL;
}



int
conv_raw_read(image_conv *im, char **bp)
{
    return image_raw_read(im->oim, bp);
}



void
conv_raw_read_start(image_conv *im)
{
    image_raw_read_start(im->oim);
}



void
conv_raw_read_finish(image_conv *im, int abortp)
{
    image_raw_read_finish(im->oim, abortp);
}



int
conv_read(image_conv *im, char **bp)
{
    return image_read(im->oim, bp);
}



void
conv_read_start(image_conv *im)
{
    image_read_start(im->oim);
}



void
conv_read_finish(image_conv *im, int abortp)
{
    image_read_finish(im->oim, abortp);
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

	if (mask & IMAGE_INF_CSPACE) {
	    if ((m2 & IMAGE_INF_CSPACE) != IMAGE_INF_BASE_DEPTH
		|| oim->i.cspace.base_depth != 16 || i->cspace.base_depth != 8)
	    /* XXX: not yet */
	    throwf(EOPNOTSUPP, "color space / depth conversion not supported");
	}
    }

    if (mask) {
	im = image_create(conv, oim->fname);

	im->mask = mask;
	im->oim = oim;
	im->im.oi = oim->oi;
	im->im.i = oim->i;
	image_cspace_merge(&im->im.i.cspace, m2, &i->cspace);

	if (mask & ~(IMAGE_INF_BASE_TYPE|IMAGE_INF_BASE_DEPTH))
	    im->im.i.compression = IMAGE_CMP_NONE;

	im->pal = NULL;

	return (image *)im;
    }
    else
	return oim;
}



static void
_depth16to8(char *pdc, char *psc, int n)
{
    int i;
    unsigned short *ps;
    unsigned char *pd;

    ps = (unsigned short *)psc;
    pd = (unsigned char *)pdc;

    for (i=0; i<n; i++)
	pd[i] = ps[i]>>8;
}
