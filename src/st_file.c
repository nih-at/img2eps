/*
  $NiH: st_file.c,v 1.1 2002/09/07 20:58:01 dillo Exp $

  st_file.c -- stdio FILE stream
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <stdio.h>

#include "stream.h"
#include "stream_types.h"



struct stream_file {
    stream st;

    FILE *f;		/* underlying file */
    int closep;		/* close f on stream close? */
};

STREAM_DECLARE(file);



int
file_close(stream_file *st)
{
    if (st->closep)
	fclose(st->f);
    
    stream_free((stream *)st);

    return 0;
}



stream *
stream_file_fopen(FILE *f, int closep)
{
    stream_file *st;

    if ((st=stream_create(file, NULL)) == NULL)
	return NULL;

    st->f = f;
    st->closep = closep;

    return (stream *)st;
}



stream *
stream_file_open(char *fname)
{
    FILE *f;
    stream *st;

    if ((f=fopen(fname, "w")) == NULL)
	return NULL;
    if ((st=stream_file_fopen(f, 1)) == NULL) {
	fclose(f);
	return NULL;
    }

    return st;
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
