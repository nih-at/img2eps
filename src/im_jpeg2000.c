/*
  $NiH: im_jpeg2000.c,v 1.1 2002/11/13 01:41:39 dillo Exp $

  im_jpeg2000.c -- JPEG 2000 image handling
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>

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
 
  THIS SOFTWARE IS PROVIDED BY DIETER BARON ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL DIETER BARON BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#include "config.h"

#ifdef USE_JPEG2000

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <jasper/jasper.h>

/* compatibility defines for versions < 1.600 */
#ifndef JAS_IMAGE_CS_GRAY
#define JAS_IMAGE_CS_UNKNOWN	JAS_IMAGE_CM_UNKNOWN
#define JAS_IMAGE_CS_GRAY	JAS_IMAGE_CM_GRAY
#define JAS_IMAGE_CS_RGB	JAS_IMAGE_CM_RGB
#endif

#define NOSUPP_CSPACE
#define NOSUPP_SCALE
#define NOSUPP_RAW

#include "exceptions.h"
#include "image.h"
#include "xmalloc.h"

#define BUFSIZE		4096	/* buffer size for raw reads */

static image_cs_type cspace_jp2img(int cs);

static int jasper_inited = 0;



struct image_jpeg2000 {
    image im;

    jas_stream_t *st;		/* jasper input stream */
    jas_image_t *jp;		/* jasper image */

    jas_matrix_t *mat;		/* pixel matrices */
    char *buf;			/* row data buffer */
    int row;			/* row count */
};

IMAGE_DECLARE(jpeg2000);



void
jpeg2000_close(image_jpeg2000 *im)
{
    jas_image_destroy(im->jp);
    jas_stream_close(im->st);
    free(im->buf);
    if (im->mat)
	jas_matrix_destroy(im->mat);
    image_free((image *)im);
}



char *
jpeg2000_get_palette(image_jpeg2000 *im)
{
    return NULL;
}



image *
jpeg2000_open(char *fname)
{
    image_jpeg2000 *im;
    jas_stream_t *st;
    jas_image_t *jp;
    exception ex;
    int fmt;
    char *fmtstr;

    if (!jasper_inited) {
	if (jas_init())
	    return NULL;
	jas_setdbglevel(0);
	jasper_inited = 1;
    }

    if ((st=jas_stream_fopen(fname, "rb")) == NULL)
	return NULL;

    if ((fmt=jas_image_getfmt(st)) < 0) {
	jas_stream_close(st);
	return NULL;
    }
    if ((fmtstr=jas_image_fmttostr(fmt)) == NULL
	|| (strcmp(fmtstr, "jp2") != 0
	    && strcmp(fmtstr, "jpc") != 0
	    && strcmp(fmtstr, "pgx") != 0)) {
	jas_stream_close(st);
	return NULL;
    }

    if ((jp=jas_image_decode(st, fmt, NULL)) == NULL) {
	jas_stream_close(st);
	return NULL;
    }

    if (jas_image_cmptprec(jp, 0) != 8 && jas_image_cmptprec(jp, 0) != 16) {
	jas_image_destroy(jp);
	jas_stream_close(st);
	throws(EOPNOTSUPP, "only depths 8 and 16 are supported");
    }
    
    if (catch(&ex) == 0) {
	im = image_create(jpeg2000, fname);
	drop();
    }
    else {
	jas_image_destroy(jp);
	jas_stream_close(st);
	throw(&ex);
    }

    im->st = st;
    im->jp = jp;
    im->mat = NULL;
    im->buf = NULL;

    im->im.i.width = jas_image_cmptwidth(jp, 0);
    im->im.i.height = jas_image_cmptheight(jp, 0);
    im->im.i.compression = IMAGE_CMP_NONE;
    im->im.i.cspace.type = cspace_jp2img(jas_image_colorspace(jp));
    im->im.i.cspace.depth = jas_image_cmptprec(jp, 0);
    /* XXX: alpha */

    im->im.oi = im->im.i;

    /* not technically correct, but close enough */
    im->im.oi.compression = IMAGE_CMP_DCT;

    return (image *)im;
}



int
jpeg2000_read(image_jpeg2000 *im, char **bp)
{
    int i, j, nc, w;
    unsigned short *sp;
    unsigned char *cp;

    cp = (unsigned char *)im->buf;
    sp = (unsigned short *)im->buf;
    nc = image_cspace_components(&im->im.i.cspace, 0);
    w = im->im.i.width;
    
    for (i=0; i<nc; i++) {
	if (jas_image_readcmpt(im->jp, i, 0, im->row, w, 1, im->mat) < 0)
	    throwf(EINVAL, "error reading component %d of row %d",
		   i, im->row);

	if (im->im.i.cspace.depth == 16) {
	    for (j=0; j<w; j++)
		sp[j*nc+i] = jas_matrix_getv(im->mat,j);
	}
	else {
	    for (j=0; j<w; j++)
		cp[j*nc+i] = jas_matrix_getv(im->mat,j);
	}
    }
    im->row++;

    *bp = im->buf;
    return im->im.i.width;
}



void
jpeg2000_read_finish(image_jpeg2000 *im, int abortp)
{
    free(im->buf);
    jas_matrix_destroy(im->mat);
    im->mat = NULL;

    im->buf = NULL;
}



void
jpeg2000_read_start(image_jpeg2000 *im)
{
    if ((im->mat=jas_matrix_create(1, im->im.i.width)) == NULL)
	throws(EINVAL, "cannot create pixel matrix");
    im->buf = xmalloc(image_get_row_size((image *)im));
    im->row = 0;
}		



static image_cs_type
cspace_jp2img(int cs)
{
    switch (cs) {
    case JAS_IMAGE_CS_GRAY:
	return IMAGE_CS_GRAY;
    case JAS_IMAGE_CS_RGB:
    case JAS_IMAGE_CS_YCBCR:
	return IMAGE_CS_RGB;

    default:
	return IMAGE_CS_UNKNOWN;
    }
}

#endif /* USE_JPEG2000 */
