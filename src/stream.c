/*
  $NiH: stream.c,v 1.6 2002/09/14 02:27:41 dillo Exp $

  stream.c -- general stream functions
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exceptions.h"
#include "stream.h"
#include "stream_types.h"
#include "xmalloc.h"

/* keep in sync with enum epsf_ascii in epsf.h */
stream *(*_asc_tab[])(stream *, int) = {
    NULL,
    NULL,
    stream_asciihex_open,
    stream_ascii85_open
};

static stream *_cmp_unsupp(stream *, void *);

/* keep in sync with enum image_compression in image.h */
stream *(*_cmp_tab[])(stream *, void *) = {
    NULL,
    NULL,
    stream_runlength_open,
    _cmp_unsupp, /* lzw */
    stream_flate_open,
    _cmp_unsupp, /* ccitt */
    _cmp_unsupp, /* dct */
};



stream *
_stream_create(struct stream_functions *f, size_t size, stream *ost)
{
    stream *st;

    st = xmalloc(size);

    st->f = f;
    st->st = ost;

    return st;
}



stream *
stream_ascii_open(stream *st, int type, int eodmarker)
{
    if (type < 0 || type > sizeof(_asc_tab)/sizeof(_asc_tab[0]))
	throwf(EINVAL, "unknown ASCII encoding %d", type);

    if (_asc_tab[type] == NULL)
	return st;

    return _asc_tab[type](st, eodmarker);
}



stream *
stream_compression_open(stream *st, int type, void *params)
{
    if (type < 0 || type > sizeof(_cmp_tab)/sizeof(_cmp_tab[0]))
	throwf(EINVAL, "unknown compression type %d", type);

    if (_cmp_tab[type] == NULL)
	return st;

    return _cmp_tab[type](st, params);
}



void
stream_free(stream *st)
{
    if (st)
	free(st);
}



int
stream_printf(stream *st, const char *fmt, ...)
{
    char *s;
    va_list argp;
    int ret;

    va_start(argp, fmt);
    vasprintf(&s, fmt, argp);
    va_end(argp);

    if (s == NULL)
	throwf(ENOMEM, "out of memory");

    ret = stream_puts(s, st);
    free(s);

    return ret;
}



int
stream_putc(int c, stream *st)
{
    char b[1];

    b[0] = c;
    
    return stream_write(st, b, 1);
}



int
stream_puts(const char *s, stream *st)
{
    return stream_write(st, s, strlen(s));
}



static stream *
_cmp_unsupp(stream *st, void *params)
{
    throws(EOPNOTSUPP, "compression method not supported");
    return NULL;
}
