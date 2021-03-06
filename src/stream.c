/*
  $NiH: stream.c,v 1.10 2002/10/12 00:02:13 dillo Exp $

  stream.c -- general stream functions
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

/* keep in sync with enum image_compression in image.h */
stream *(*_cmp_tab[])(stream *, void *) = {
    NULL,
    NULL,
    stream_runlength_open,
    stream_lzw_open,
    stream_flate_open,
    stream_ccitt_open,
    stream_dct_open
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
