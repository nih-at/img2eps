/*
  $NiH: im_convert.c,v 1.16 2002/10/15 03:03:49 dillo Exp $

  im_convert.c -- image conversion handling
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



#include <errno.h>

#include "config.h"
#include "exceptions.h"
#include "image.h"
#include "xmalloc.h"



struct conv_info {
    int palncomp;		/* number of components in palette */
    int sncomp, dncomp;		/* number of components */
    int snbits, dnbits;		/* number of bits per component */
    int invert;			/* if component value inversion is needed */
    image_cs_type stype, dtype;	/* cspace type */
};

struct image_conv {
    image im;

    image *oim;
    int mask;		/* which conversions are needed */

    char *pal;		/* palette */
    char *buf;		/* scanline buffer */

    image_cspace palcs;		/* cspace for palette */
    struct conv_info ci[2];	/* conversion info (image, palette) */
    unsigned short *conv_pal;	/* palette used in conversion */
};

IMAGE_DECLARE(conv);

#define MAKE2(i, j)	((i)<<16 | (j))
#define NEED_IMAGE_CONVERSION(im)	\
	((im)->mask & (IMAGE_INF_TYPE|IMAGE_INF_DEPTH) || (im)->ci[0].invert)

static int _convertable_cstype(image_cs_type stype, image_cs_type dtype);
static int _convertable_depth(int sdepth, int ddepth);
static void _convert(image_conv *im, char *pdc, char *psc, int n, int basep);
static char *_get_palette(image_conv *im, int intern);
static void _update_ci(image_conv *im);



void
conv_close(image_conv *im)
{
    free (im->buf);
    free (im->pal);
    if (im->oim)
	image_close(im->oim);
    image_free((image *)im);
}



char *
conv_get_palette(image_conv *im)
{
    return _get_palette(im, 0);
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
    char *b;
    
    image_read(im->oim, &b);

    if (NEED_IMAGE_CONVERSION(im)) {
	_convert(im, im->buf, b, im->im.i.width, 0);
	b = im->buf;
    }

    *bp = b;
    return 0;
}



void
conv_read_start(image_conv *im)
{
    image_read_start(im->oim);

    if (NEED_IMAGE_CONVERSION(im)) {
	im->buf = xmalloc(image_get_row_size((image *)im));
	if (im->mask & IMAGE_INF_TYPE
	    && im->oim->i.cspace.type == IMAGE_CS_INDEXED)
	    im->conv_pal = (unsigned short *)_get_palette(im, 1);
    }
}



void
conv_read_finish(image_conv *im, int abortp)
{
    free(im->pal);
    free(im->buf);
    im->buf = im->pal = NULL;
    im->conv_pal = NULL;
    
    image_read_finish(im->oim, abortp);
}



int
conv_set_cspace(image_conv *im, int mask, const image_cspace *cspace)
{
    int m2;
    
    m2 = image_set_cspace(im->oim, mask&IMAGE_INF_CSPACE, cspace);
    image_cspace_merge(&im->im.i.cspace, mask&~m2, &im->oim->i.cspace);
    mask = (mask&~IMAGE_INF_CSPACE) | m2;

    if (mask & IMAGE_INF_TYPEDEPTH) {
	if (((mask & IMAGE_INF_TYPE) ? cspace->type : im->oim->i.cspace.type)
	    == IMAGE_CS_INDEXED) {
	    /* conversion to indexed */
	    if (im->oim->i.cspace.type != IMAGE_CS_INDEXED)
		throws(EOPNOTSUPP,
		       "colour space conversion not supported");
	    if (((mask & (IMAGE_INF_BASE_TYPE|IMAGE_INF_BASE_DEPTH))
		 && (!_convertable_cstype(
			 im->oim->i.cspace.base_type,
			 (mask & IMAGE_INF_BASE_TYPE
			  ? cspace->base_type
			  : im->oim->i.cspace.base_type))
		     || (!_convertable_depth(
			     im->oim->i.cspace.base_depth,
			     (mask & IMAGE_INF_BASE_DEPTH
			      ? cspace->base_depth
			      : im->oim->i.cspace.base_depth))))))
		throws(EOPNOTSUPP, "palette conversion not supported");
	}
	else if (im->oim->i.cspace.type == IMAGE_CS_INDEXED) {
	    /* conversion from indexed to other */
	    if (!_convertable_cstype(im->oim->i.cspace.base_type,
				     cspace->type)
		|| !_convertable_depth(im->oim->i.cspace.base_depth, 16)
		|| !_convertable_depth(16,
				       (mask&IMAGE_INF_DEPTH
					? cspace->depth
					: im->oim->i.cspace.depth)))
		throws(EOPNOTSUPP,
		       "colour space conversion not supported");
	}
	else {
	    /* other to other */
	    if ((mask & (IMAGE_INF_TYPE|IMAGE_INF_DEPTH))
		&& (!_convertable_cstype(im->oim->i.cspace.type,
					 (mask & IMAGE_INF_TYPE
					  ? cspace->type
					  : im->oim->i.cspace.type))
		    || !_convertable_depth(im->oim->i.cspace.depth,
					   (mask & IMAGE_INF_DEPTH
					    ? cspace->depth
					    : im->oim->i.cspace.depth))))
		throws(EOPNOTSUPP,
		       "colour space conversion not supported");
	}
	
	if ((mask & IMAGE_INF_INVERTED)
	    && im->oim->i.cspace.inverted == cspace->inverted)
	    mask &= ~IMAGE_INF_INVERTED;
    }
    
    image_cspace_merge(&im->im.i.cspace, mask, cspace);

    im->mask = image_cspace_diffs(&im->oim->i.cspace,
				  IMAGE_INF_ALL, &im->im.i.cspace);

    if (im->mask & ~(IMAGE_INF_BASE_TYPE|IMAGE_INF_BASE_DEPTH))
	im->im.i.compression = IMAGE_CMP_NONE;
    else
	im->im.i.compression = im->oim->i.compression;

    _update_ci(im);

    return 0;
}



int
conv_set_size(image_conv *im, int w, int h)
{
    if (im->im.i.width == w && im->im.i.height == h)
	return 0;
    
    return image_set_size(im->oim, w, h);
}



image *
image_convert(image *oim, int mask, const image_info *i)
{
    image_conv *im;

    mask &= ~IMAGE_INF_COMPRESSION;

    if ((mask & IMAGE_INF_ORDER) && i->order != oim->i.order)
	throwf(EOPNOTSUPP, "reordering of samples not supported");

    im = image_create(conv, oim->fname);
    
    im->mask = 0;
    im->oim = oim;
    im->im.oi = oim->oi;
    im->im.i = oim->i;

    im->pal = NULL;
    im->buf = NULL;


    if (mask & IMAGE_INF_SIZE) {
	if (image_set_size((image *)im, i->width, i->height) == 0)
	    mask &= ~IMAGE_INF_SIZE;
	else {
	    /* XXX: not yet */
	    throwf(EOPNOTSUPP, "scaling not supported");
	}
    }

    if (mask & IMAGE_INF_CSPACE)
	image_set_cspace((image *)im, mask&IMAGE_INF_CSPACE, &i->cspace);

    if (im->mask)
	return (image *)im;
    else {
	im->oim = NULL;
	image_close((image *)im);
	return oim;
    }
}



static int
_convertable_cstype(image_cs_type stype, image_cs_type dtype)
{
    if (stype == dtype)
	return 1;
    
    switch (MAKE2(stype, dtype)) {
    case MAKE2(IMAGE_CS_RGB, IMAGE_CS_GRAY):
    case MAKE2(IMAGE_CS_GRAY, IMAGE_CS_RGB):
    case MAKE2(IMAGE_CS_GRAY, IMAGE_CS_CMYK):
    case MAKE2(IMAGE_CS_CMYK, IMAGE_CS_GRAY):
    case MAKE2(IMAGE_CS_CMYK, IMAGE_CS_RGB):
	return 1;
    default:
	return 0;
    }
}



static int
_convertable_depth(int sdepth, int ddepth)
{
    switch (sdepth) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 12:
    case 16:
	break;

    default:
	return 0;
    }
	
    switch (ddepth) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 12:
    case 16:
	break;

    default:
	return 0;
    }

    return 1;
}



static void
_convert(image_conv *im, char *pdc, char *psc, int n, int basep)
{
    static const int mask[4] = { 0x1, 0x3, 0, 0xf };
    static const unsigned short tab[4][16] = {
	{ 0x0000, 0xffff },
	{ 0x0000, 0x5555, 0xaaaa, 0xffff },
	{ 0 },
	{ 0x0000, 0x1111, 0x2222, 0x3333,
	  0x4444, 0x5555, 0x6666, 0x7777,
	  0x8888, 0x9999, 0xaaaa, 0xbbbb,
	  0xcccc, 0xdddd, 0xeeee, 0xffff }
    };

    int c[IMAGE_MAX_COMPONENTS];
    int i, j, idx, bs, bd;
    unsigned char *ps, *pd;
    struct conv_info *ci;

    ci = &im->ci[basep];
    ps = (unsigned char *)psc;
    pd = (unsigned char *)pdc;

    bs = bd = 0;
    for (i=0; i<n; i++) {
	/* extract source colour */
	for (j=0; j<ci->sncomp; j++) {
	    switch(ci->snbits) {
	    case 1:
	    case 2:
	    case 4:
		c[j] = tab[ci->snbits-1][(*ps>>(8-(1+bs)*ci->snbits))
					 & mask[ci->snbits-1]];
		if (++bs == 8/ci->snbits) {
		    bs = 0;
		    ps++;
		}
		break;
	    case 8:
		c[j] = (*ps<<8)+*ps;
		ps++;
		break;
	    case 12:
		if ((i*ci->sncomp+j)%2 == 0) {
		    c[j] = (*ps<<4) | (ps[1]>>4);
		    ps++;
		}
		else {
		    c[j] = ((*ps&0x0f)<<8) | ps[1];
		    ps += 2;
		}
		c[j] = (c[j]<<4) | c[j]>>8;
		break;
	    case 16:
		c[j] = *(unsigned short *)ps;
		ps += 2;
		break;
	    }
	}

	/* lookup index */
	if (ci->palncomp != -1) {
	    idx = c[0]>>(16-ci->snbits);
	    for (j=0; j<ci->palncomp; j++)
		c[j] = im->conv_pal[ci->dncomp*idx+j];
	}

	/* convert */
	switch (MAKE2(ci->stype, ci->dtype)) {
	case MAKE2(IMAGE_CS_RGB, IMAGE_CS_GRAY):
	    c[0] = (c[0]*7+c[1]*38+c[2]*19)/64;
	    break;
	case MAKE2(IMAGE_CS_GRAY, IMAGE_CS_RGB):
	    c[1] = c[2] = c[0];
	    break;
	case MAKE2(IMAGE_CS_GRAY, IMAGE_CS_CMYK):
	    c[3] = 0xffff-c[0];
	    c[0] = c[1] = c[2] = 0;
	    break;
	case MAKE2(IMAGE_CS_CMYK, IMAGE_CS_GRAY):
	    c[0] = (c[0]*19+c[1]*38+c[2]*7)/64 + c[3];
	    c[0] = 0xffff - (c[0]> 0xffff ? 0xffff : c[0]);
	    break;
	case MAKE2(IMAGE_CS_CMYK, IMAGE_CS_RGB):
	    c[0] = 0xffff - (c[0]+c[3] > 0xffff ? 0xffff : c[0]+c[3]);
	    c[1] = 0xffff - (c[2]+c[3] > 0xffff ? 0xffff : c[2]+c[3]);
	    c[2] = 0xffff - (c[2]+c[3] > 0xffff ? 0xffff : c[2]+c[3]);
	    break;
	}

	/* invert */
	/* XXX: may need to be done before type conversion */
	if (ci->invert)
	    for (j=0; j<ci->dncomp; j++)
		c[j] = 0xffff-c[j];

	/* store destination colour */
	for (j=0; j<ci->dncomp; j++) {
	    switch(ci->dnbits) {
	    case 1:
	    case 2:
	    case 4:
		if (bd == 0)
		    *pd = 0;
		*pd |= (c[j]>>(16-ci->dnbits))<<(8-(1+bd)*ci->dnbits);
		if (++bd == 8/ci->dnbits) {
		    bd = 0;
		    pd++;
		}
		break;
	    case 8:
		*pd = c[j]>>8;
		pd++;
		break;
	    case 12:
		if ((i*ci->dncomp+j)%2 == 0) {
		    pd[0] = c[j]>>8;
		    pd[1] = c[j] & 0xf0;
		    pd++;
		}
		else {
		    pd[0] |= c[j]>>12;
		    pd[1] = (c[j]>>4) & 0xff;
		    pd += 2;
		}
		break;
	    case 16:
		*(unsigned short *)pd = c[j];
		pd += 2;
		break;
	    }
	}
    }
}



static char *
_get_palette(image_conv *im, int intern)
{
    char *pal;

    if (!intern && im->im.i.cspace.type != IMAGE_CS_INDEXED)
	return NULL;

    pal = image_get_palette(im->oim);

    if (im->oim->i.cspace.base_type != im->ci[1].dtype
	|| im->oim->i.cspace.base_depth != im->ci[1].dnbits
	|| im->ci[1].invert) {
	free(im->pal);
	im->pal = xmalloc(image_cspace_palette_size(&im->palcs));
	_convert(im, im->pal, pal, 1<<im->palcs.depth, 1);
	pal = im->pal;
    }
    
    return pal;
}



static void
_update_ci(image_conv *im)
{
    if (im->oim->i.cspace.type == IMAGE_CS_INDEXED
	&& im->im.i.cspace.type != IMAGE_CS_INDEXED) {
	im->palcs = im->oim->i.cspace;
	im->palcs.base_depth = 16;
	im->palcs.base_type = im->im.i.cspace.type;

	im->ci[0].palncomp = image_cspace_components(&im->palcs, 1);
	im->ci[0].sncomp = 1;
	im->ci[1].stype = im->im.i.cspace.type;
    }
    else {
	im->ci[0].palncomp = -1;
	im->ci[0].sncomp = image_cspace_components(&im->oim->i.cspace, 0);
	im->ci[0].stype = im->oim->i.cspace.type;
    }
    im->ci[0].snbits = im->oim->i.cspace.depth;
    im->ci[0].dncomp = image_cspace_components(&im->im.i.cspace, 0);
    im->ci[0].dnbits = im->im.i.cspace.depth;
    im->ci[0].dtype = im->im.i.cspace.type;

    im->ci[1].palncomp = -1;
    if (im->im.i.cspace.type == IMAGE_CS_INDEXED)
	im->palcs = im->im.i.cspace;

    if (im->im.i.cspace.type == IMAGE_CS_INDEXED
	|| im->oim->i.cspace.type == IMAGE_CS_INDEXED) {
	im->ci[1].sncomp = image_cspace_components(&im->oim->i.cspace, 1);
	im->ci[1].dncomp = image_cspace_components(&im->palcs, 1);
	im->ci[1].snbits = im->oim->i.cspace.base_depth;
	im->ci[1].dnbits = im->palcs.base_depth;
	im->ci[1].stype = im->oim->i.cspace.base_type;
	im->ci[1].dtype = im->palcs.base_type;
    }

    if (im->mask & IMAGE_INF_INVERTED) {
	if (im->oim->i.cspace.type == IMAGE_CS_INDEXED) {
	    im->ci[0].invert = 0;
	    im->ci[1].invert = 1;
	}
	else {
	    im->ci[0].invert = 1;
	    im->ci[1].invert = 0;
	}
    }
    else {
	im->ci[0].invert = 0;
	im->ci[1].invert = 0;
    }
}
