/*
  $NiH: st_file.c,v 1.4 2002/09/11 22:44:20 dillo Exp $

  st_file.c -- stdio FILE stream
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
stream_file_open(const char *fname)
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
