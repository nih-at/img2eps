/*
  $NiH: image.c,v 1.18 2005/07/23 00:17:06 dillo Exp $

  image.c -- general image functions
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
#include <stdio.h>
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
#ifdef USE_JPEG2000
image *jpeg2000_open(const char *);
#endif
#ifdef USE_PNG
image *png_open(const char *);
#endif
#ifdef USE_TIFF
image *tiff_open(const char *);
#endif
image *xbm_open(const char *);
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
    xbm_open,
#ifdef USE_GIF
    gif_open,
#endif
#ifdef USE_JPEG2000
    jpeg2000_open,
#endif
    NULL
};



const struct _num_name _image_nn_cspace[] = {
    { IMAGE_CS_GRAY,         "DeviceGray" },
    { IMAGE_CS_GRAY,         "gray" },
    { IMAGE_CS_GRAY,         "grey" },

    { IMAGE_CS_RGB,          "DeviceRGB" },
    { IMAGE_CS_RGB,          "rgb" },

    { IMAGE_CS_CMYK,         "DeviceCMYK" },
    { IMAGE_CS_CMYK,         "cmyk" },

    { IMAGE_CS_INDEXED,      "Indexed" },
    { IMAGE_CS_INDEXED,      "indexed" },

    { IMAGE_CS_HSV,          "hsv" },

    { IMAGE_CS_UNKNOWN, NULL }
};

const struct _num_name _image_nn_compression[] = {
    { IMAGE_CMP_NONE,        "none" },

    { IMAGE_CMP_RLE,         "RunLength" },
    { IMAGE_CMP_RLE,         "rle" },
    { IMAGE_CMP_RLE,         "PackBits" },

    { IMAGE_CMP_LZW,         "LZW" },
    { IMAGE_CMP_LZW,         "lzw" },
    { IMAGE_CMP_LZW,         "gif" },
    
    { IMAGE_CMP_FLATE,       "Flate" },
    { IMAGE_CMP_FLATE,       "flate" },
    { IMAGE_CMP_FLATE,       "zlib" },
    { IMAGE_CMP_FLATE,       "png" },

    { IMAGE_CMP_CCITT,       "CCITTFax" },
    { IMAGE_CMP_CCITT,       "ccitt" },
    { IMAGE_CMP_CCITT,       "fax" },

    { IMAGE_CMP_DCT,         "DCT" },
    { IMAGE_CMP_DCT,         "jpeg" },

    { IMAGE_CMP_UNKNOWN, NULL }
};



image *
_image_create(struct image_functions *f, size_t size, const char *fname)
{
    image *im;

    im = xmalloc(size);
    im->f = f;
    im->fname = xstrdup(fname);
    image_init_info(&im->i);

    im->i.cspace.transparency = IMAGE_TR_NONE;
    im->i.cspace.inverted = IMAGE_INV_DARKLOW;
    im->i.compression = IMAGE_CMP_NONE;
    im->i.compression_flags = 0;
    im->i.order = IMAGE_ORD_ROW_LT;
    im->oi = im->i;

    return im;
}



int
_image_notsup_cspace(image *im, int mask, const image_cspace *cspace)
{
    return image_cspace_diffs(&im->i.cspace, mask, cspace);
}



int
_image_notsup_raw(image *im, int a0, int a1)
{
    throws(EOPNOTSUPP, "raw reading not supported");
    return -1;
}



int
_image_notsup_scale(image *im, int w, int h)
{
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



int
image_cspace_diffs(const image_cspace *cst, int mask, const image_cspace *css)
{
    int m2;

    m2 = 0;
    if ((mask & IMAGE_INF_TYPE) && cst->type != css->type)
	m2 |= IMAGE_INF_TYPE;
    if ((mask & IMAGE_INF_DEPTH) && cst->depth != css->depth)
	m2 |= IMAGE_INF_DEPTH;
    if ((mask & IMAGE_INF_TRANSPARENCY)
	&& cst->transparency != css->transparency)
	m2 |= IMAGE_INF_TRANSPARENCY;
    if ((mask & IMAGE_INF_INVERTED)
	&& cst->inverted != css->inverted)
	m2 |= IMAGE_INF_INVERTED;
    if ((mask & IMAGE_INF_TYPE) && css->type == IMAGE_CS_INDEXED) {
	if ((mask & IMAGE_INF_BASE_TYPE) && cst->base_type != css->base_type)
	    m2 |= IMAGE_INF_BASE_TYPE;
	if ((mask & IMAGE_INF_BASE_DEPTH)
	    && cst->base_depth != css->base_depth)
	    m2 |= IMAGE_INF_BASE_DEPTH;
    }

    return m2;
}



void
image_cspace_merge(image_cspace *cst, int mask, const image_cspace *css)
{
    if (mask & IMAGE_INF_TYPE)
	cst->type = css->type;
    if (mask & IMAGE_INF_TRANSPARENCY)
	cst->transparency = css->transparency;
    if (mask & IMAGE_INF_INVERTED)
	cst->inverted = css->inverted;
    if (mask & IMAGE_INF_DEPTH)
	cst->depth = css->depth;
    if (css->type == IMAGE_CS_INDEXED) {
	if (mask & IMAGE_INF_BASE_TYPE)
	    cst->base_type = css->base_type;
	if (mask & IMAGE_INF_BASE_DEPTH)
	    cst->base_depth = css->base_depth;
    }
}



int
image_cspace_palette_size(const image_cspace *cspace)
{
    if (cspace->type != IMAGE_CS_INDEXED)
	return 0;

    return (image_cspace_components(cspace, 1)
	    * (1<<cspace->depth) * cspace->base_depth + 7) / 8;
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


int
image_info_mask(const image_info *i)
{
    int m;

    m = 0;
    
    if (i->width || i->height)
	m |= IMAGE_INF_SIZE;
    if (i->cspace.type != IMAGE_CS_UNKNOWN)
	m |= IMAGE_INF_TYPE;
    if (i->cspace.depth)
	m |= IMAGE_INF_DEPTH;
    if (i->cspace.transparency != IMAGE_TR_UNKNOWN)
	m |= IMAGE_INF_TRANSPARENCY;
    if (i->cspace.inverted != IMAGE_INV_UNKNOWN)
	m |= IMAGE_INF_INVERTED;
    if (i->cspace.type == IMAGE_CS_INDEXED) {
	if (i->cspace.base_type != IMAGE_CS_UNKNOWN)
	m |= IMAGE_INF_BASE_TYPE;
	if (i->cspace.base_depth)
	    m |= IMAGE_INF_BASE_DEPTH;
    }
    if (i->order != IMAGE_ORD_UNKNOWN)
	m |= IMAGE_INF_ORDER;

    return m;
}



const char *
image_info_print(const image_info *i)
{
    static char b[8192];

    char *p;

    p = b;
    sprintf(p, "%dx%d %s %dbps",
	    i->width, i->height,
	    image_cspace_name(i->cspace.type),
	    i->cspace.depth);
    p += strlen(p);
    if (i->cspace.type == IMAGE_CS_INDEXED) {
	sprintf(p, " (%s %dbps)",
		image_cspace_name(i->cspace.base_type),
		i->cspace.base_depth);
	p += strlen(p);
    }
    if (i->cspace.inverted == IMAGE_INV_BRIGHTLOW) {
	sprintf(p, ", inverted");
	p += strlen(p);
    }
    if (i->compression != IMAGE_CMP_NONE) {
	sprintf(p, ", %s compressed",
		image_compression_name(i->compression));
	p += strlen(p);
    }

    /* XXX: transparency */

    return b;
}



void
image_init_info(image_info *i)
{
    i->width = i->height = 0;
    i->cspace.type = IMAGE_CS_UNKNOWN;
    i->cspace.transparency = IMAGE_TR_UNKNOWN;
    i->cspace.depth = 0;
    i->cspace.inverted = IMAGE_INV_UNKNOWN;
    i->cspace.base_type = IMAGE_CS_UNKNOWN;
    i->cspace.base_depth = 0;
    i->cspace.ncol = 0;
    i->compression = IMAGE_CMP_UNKNOWN;
    i->compression_flags = 0;
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
