/*
  $NiH: im_convert.c,v 1.7 2002/10/09 14:46:43 dillo Exp $

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



struct conv_info {
    int sncomp, dncomp;		/* number of components */
    int snbits, dnbits;		/* number of bits per component */
    image_cs_type stype, dtype;	/* cspace type */
};

struct image_conv {
    image im;

    image *oim;
    int mask;		/* which conversions are needed */

    char *pal;		/* palette */

    struct conv_info ci[2];	/* conversion info (image, palette) */
};

IMAGE_DECLARE(conv);

#define MAKE2(i, j)	((i)<<16 | (j))

static void _convert(image_conv *im, char *pdc, char *psc, int n, int basep);
static void _update_ci(image_conv *im);



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
    char *pal;

    if (im->im.i.cspace.type != IMAGE_CS_INDEXED)
	return NULL;

    pal = image_get_palette(im->oim);

    if (im->mask & (IMAGE_INF_BASE_TYPE|IMAGE_INF_BASE_DEPTH)) {
	free(im->pal);
	im->pal = xmalloc(image_cspace_palette_size(&im->im.i.cspace));
	_convert(im, im->pal, pal, 1<<im->im.i.cspace.depth, 1);
	pal = im->pal;
    }
    
    return pal;
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
	    if ((mask & IMAGE_INF_BASE_TYPE)
		&& ((oim->i.cspace.base_type != IMAGE_CS_GRAY
		     && oim->i.cspace.base_type != IMAGE_CS_RGB)
		    || (i->cspace.base_type != IMAGE_CS_GRAY
			&& i->cspace.base_type != IMAGE_CS_RGB)))
		throws(EOPNOTSUPP, "palette type conversion not supported");
	    if ((mask & IMAGE_INF_BASE_DEPTH)
		&& ((oim->i.cspace.base_depth != 16
		     && oim->i.cspace.base_depth != 8)
		    || (i->cspace.base_depth != 16
			&& i->cspace.base_depth != 8)))
		throws(EOPNOTSUPP, "palette depth conversion not supported");

	    if ((mask & IMAGE_INF_CSPACE) & ~(IMAGE_INF_BASE_TYPE
					      |IMAGE_INF_BASE_DEPTH))
		throws(EOPNOTSUPP,
		       "color space / depth conversion not supported");
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

	_update_ci(im);

	return (image *)im;
    }
    else
	return oim;
}



static void
_convert(image_conv *im, char *pdc, char *psc, int n, int basep)
{
    int c[4];
    int i, j;
    unsigned char *ps, *pd;

    ps = (unsigned char *)psc;
    pd = (unsigned char *)pdc;

    for (i=0; i<n; i++) {
	/* extract source colour */
	for (j=0; j<im->ci[basep].sncomp; j++) {
	    switch(im->ci[basep].snbits) {
	    case 8:
		c[j] = (*ps<<8)+*ps;
		ps++;
		break;
	    case 16:
		c[j] = *(unsigned short *)ps;
		ps += 2;
		break;
	    }
	}

	/* convert */
	switch (MAKE2(im->ci[basep].stype, im->ci[basep].dtype)) {
	case MAKE2(IMAGE_CS_RGB, IMAGE_CS_GRAY):
	    c[0] = (c[0]*7+c[1]*38+c[2]*19)/64;
	    break;
	case MAKE2(IMAGE_CS_GRAY, IMAGE_CS_RGB):
	    c[1] = c[2] = c[0];
	    break;
	}

	/* store destination colour */
	for (j=0; j<im->ci[basep].dncomp; j++) {
	    switch(im->ci[basep].dnbits) {
	    case 8:
		*pd = c[j]>>8;
		pd++;
		break;
	    case 16:
		*(unsigned short *)pd = c[j];
		pd += 2;
		break;
	    }
	}
    }
}



static void
_update_ci(image_conv *im)
{
    int i;

    for (i=0; i<2; i++) {
	im->ci[i].sncomp = image_cspace_components(&im->oim->i.cspace, i);
	im->ci[i].dncomp = image_cspace_components(&im->im.i.cspace, i);
    }
    im->ci[0].snbits = im->oim->i.cspace.depth;
    im->ci[0].dnbits = im->im.i.cspace.depth;
    im->ci[1].snbits = im->oim->i.cspace.base_depth;
    im->ci[1].dnbits = im->im.i.cspace.base_depth;
    im->ci[0].stype = im->oim->i.cspace.type;
    im->ci[0].dtype = im->im.i.cspace.type;
    im->ci[1].stype = im->oim->i.cspace.base_type;
    im->ci[1].dtype = im->im.i.cspace.base_type;
}
