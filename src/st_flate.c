/*
  $NiH: st_flate.c,v 1.6 2002/10/15 03:03:50 dillo Exp $

  st_flate.c -- FlateEncode stream
  Copyright (C) 2002, 2005 Dieter Baron

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
#include <stddef.h>
#include <string.h>

#include "config.h"
#include "exceptions.h"
#include "stream.h"
#include "stream_types.h"



#ifdef HAVE_LIBZ

#include <zlib.h>

#define BLKSIZE		8192

struct stream_flate {
    stream st;

    z_stream z;
    char b[BLKSIZE];
};

STREAM_DECLARE(flate);



int
flate_close(stream_flate *st)
{
    int end;
    
    st->z.next_in = NULL;
    st->z.avail_in = 0;

    end = 0;
    while (!end) {
	st->z.next_out = (unsigned char *)st->b;
	st->z.avail_out = BLKSIZE;
	switch (deflate(&st->z, Z_FINISH)) {
	case Z_OK:
	    break;

	case Z_STREAM_END:
	    end = 1;
	    break;

	default:
	    throwf(EINVAL, "deflate error: %s", st->z.msg);
	}

	if (st->z.avail_out != BLKSIZE)
	    stream_write(st->st.st, st->b, BLKSIZE-st->z.avail_out);
    }

    deflateEnd(&st->z);
    stream_free((stream *)st);

    return 0;
    
}



stream *
stream_flate_open(stream *ost, void *params)
{
    stream_flate *st;
    char *msg;

    st = stream_create(flate, ost);

    memset(&st->z, 0, sizeof(st->z));
    st->z.avail_in = 0;
    st->z.avail_out = 0;

    if (deflateInit(&st->z, Z_DEFAULT_COMPRESSION) != Z_OK) {
	msg = st->z.msg;
	free(st);
	throwf(EINVAL, "cannot initialize z stream: %s", msg);
    };
    
    return (stream *)st;
}



int
flate_write(stream_flate *st, const char *b, int n)
{
    st->z.next_in = (unsigned char *)b;
    st->z.avail_in = n;

    while (st->z.avail_in) {
	st->z.next_out = (unsigned char *)st->b;
	st->z.avail_out = BLKSIZE;
	if (deflate(&st->z, Z_NO_FLUSH) != Z_OK)
	    throwf(EINVAL, "deflate error: %s", st->z.msg);

	if (st->z.avail_out != BLKSIZE)
	    stream_write(st->st.st, st->b, BLKSIZE-st->z.avail_out);
    }

    return 0;
}

#else /* HAVE_LIBZ */

stream *
stream_flate_open(stream *ost, void *params)
{
    throws(EOPNOTSUPP, "flate compression not supported (missing zlib)");
    return NULL;
}

#endif /* HAVE_LIBZ */
