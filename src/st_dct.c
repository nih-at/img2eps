/*
  $NiH: st_dct.c,v 1.2 2002/10/12 00:02:11 dillo Exp $

  st_dct.c -- DCTEncode stream
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



#include <errno.h>
#include <stddef.h>

#include "config.h"
#include "epsf.h"
#include "exceptions.h"
#include "stream.h"
#include "stream_types.h"
#include "xmalloc.h"



#ifdef USE_JPEG

#include <jpeglib.h>

#define BLKSIZE		8192

struct stream_dct {
    stream st;

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    char b[BLKSIZE];
};

STREAM_DECLARE(dct);

static void error_exit(j_common_ptr cinfo);
static void jpeg_stream_dest(j_compress_ptr cinfo);
static void stdest_init(j_compress_ptr cinfo);
static boolean stdest_empty(j_compress_ptr cinfo);
static void stdest_term(j_compress_ptr cinfo);



int
dct_close(stream_dct *st)
{
    jpeg_finish_compress(&st->cinfo);
    jpeg_destroy_compress(&st->cinfo);

    stream_free((stream *)st);

    return 0;
    
}



stream *
stream_dct_open(stream *ost, void *params)
{
    stream_dct *st;
    image_info *info;
    int cspace;

    info = params;

    switch (info->cspace.type) {
    case IMAGE_CS_GRAY:
	cspace = JCS_GRAYSCALE;
	break;
    case IMAGE_CS_RGB:
	cspace = JCS_RGB;
	break;

    default:
	throwf(EOPNOTSUPP, "color space %s not supported by DCT",
	       epsf_cspace_name(info->cspace.type));
    }

    st = stream_create(dct, ost);

    st->cinfo.err = jpeg_std_error(&st->jerr);
    st->cinfo.client_data = st;
    st->jerr.error_exit = error_exit;
    jpeg_create_compress(&st->cinfo);
    jpeg_stream_dest(&st->cinfo);

    st->cinfo.image_width = info->width;
    st->cinfo.image_height = info->height;
    st->cinfo.input_components
	= image_cspace_components(&info->cspace, 0);
    st->cinfo.in_color_space = cspace;

    jpeg_set_defaults(&st->cinfo);
    jpeg_start_compress(&st->cinfo, TRUE);
    return (stream *)st;
}



int
dct_write(stream_dct *st, const char *b, int n)
{
    const char *a[1];

    /* XXX: assumes one scanline at a time. */
    
    a[0] = b;
    jpeg_write_scanlines(&st->cinfo, (JSAMPARRAY)a, 1);

    return 0;
}



static void
error_exit(j_common_ptr cinfo)
{
    char b[8192];

    cinfo->err->format_message(cinfo, b);
    throws(errno ? errno : EINVAL, xstrdup(b));
}



static void
jpeg_stream_dest(j_compress_ptr cinfo)
{
    struct jpeg_destination_mgr *dest;

    dest = xmalloc(sizeof(*dest));

    dest->init_destination = stdest_init;
    dest->empty_output_buffer = stdest_empty;
    dest->term_destination = stdest_term;
    cinfo->dest = dest;
}



static void
stdest_init(j_compress_ptr cinfo)
{
    stream_dct *st;

    st = cinfo->client_data;
    cinfo->dest->next_output_byte = (unsigned char *)st->b;
    cinfo->dest->free_in_buffer = BLKSIZE;
}



static boolean
stdest_empty(j_compress_ptr cinfo)
{
    stream_dct *st;

    st = cinfo->client_data;
    stream_write(st->st.st, st->b, BLKSIZE);
    cinfo->dest->next_output_byte = (unsigned char *)st->b;
    cinfo->dest->free_in_buffer = BLKSIZE;
    return TRUE;
}



static void
stdest_term(j_compress_ptr cinfo)
{
    stream_dct *st;

    st = cinfo->client_data;
    stream_write(st->st.st, st->b, BLKSIZE-cinfo->dest->free_in_buffer);
}

#else /* HAVE_LIBZ */

stream *
stream_dct_open(stream *ost, void *params)
{
    throws(EOPNOTSUPP, "DCT compression not supported (missing jpeg support)");
    return NULL;
}

#endif /* HAVE_LIBZ */
