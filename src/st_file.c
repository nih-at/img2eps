/*
  $NiH: st_file.c,v 1.2 2002/09/08 00:27:49 dillo Exp $

  st_file.c -- stdio FILE stream
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "exceptions.h"
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
	if (fclose(st->f) == EOF)
	    throwf(errno, "write error: %s", strerror(errno));
    
    stream_free((stream *)st);

    return 0;
}



stream *
stream_file_fopen(FILE *f, int closep)
{
    stream_file *st;

    st = stream_create(file, NULL);

    st->f = f;
    st->closep = closep;

    return (stream *)st;
}



stream *
stream_file_open(char *fname)
{
    FILE *f;
    stream *st;
    exception ex;

    if ((f=fopen(fname, "w")) == NULL)
	throwf(errno, "cannot create `%s': %s", fname, strerror(errno));


    if (catch(&ex) == 0) {
	st = stream_file_fopen(f, 1);
	drop();
    }
    else {
	fclose(f);
	throw(&ex);
    }

    return st;
}



int
file_write(stream_file *st, const char *b, int n)
{
    const char *end;
    int nn;

    end = b+n;

    while (b < end) {
	if ((nn=fwrite(b, 1, end-b, st->f)) < 0)
	    throwf(errno, "write error: %s", strerror(errno));

	b += nn;
    }

    return 0;
}
