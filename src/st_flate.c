/*
  $NiH: st_flate.c,v 1.3 2002/10/06 16:08:49 dillo Exp $

  st_flate.c -- FlateEncode stream
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <errno.h>
#include <stddef.h>

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
	st->z.next_out = st->b;
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
    st->z.next_in = (char *)b;
    st->z.avail_in = n;

    while (st->z.avail_in) {
	st->z.next_out = st->b;
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
