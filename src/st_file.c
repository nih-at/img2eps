/*
  $NiH$

  st_file.c -- stdio FILE stream
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <stdio.h>

#include "stream.h"



struct stream_file {
    stream st;

    FILE *f;
};

STREAM_DECLARE(file);



int
file_close(stream_file *st)
{
    stream_free((stream *)st);

    return 0;
}



stream *
stream_file_open(FILE *f)
{
    stream_file *st;

    if ((st=stream_create(file, NULL)) == NULL)
	return NULL;

    st->f = f;

    return (stream *)st;
}



int
file_write(stream_file *st, const char *b, int n)
{
    int nn;

    while (n>0) {
	if ((nn=fwrite(b, 1, n, st->f)) < 0)
	    return -1;

	n -= nn;
	b += nn;
    }

    return 0;
}
