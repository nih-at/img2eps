/*
  $NiH: stream.c,v 1.1 2002/09/07 20:58:01 dillo Exp $

  stream.c -- general stream functions
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stream.h"



stream *
_stream_create(struct stream_functions *f, size_t size, stream *ost)
{
    stream *st;

    if ((st=malloc(size)) == NULL)
	return NULL;

    st->f = f;
    st->st = ost;

    return st;
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
