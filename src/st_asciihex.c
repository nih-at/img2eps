/*
  $NiH: st_asciihex.c,v 1.3 2002/09/10 14:05:52 dillo Exp $

  st_asciihex.c -- ASCIIHexEncode stream
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



#include "exceptions.h"
#include "stream.h"
#include "stream_types.h"



struct stream_asciihex {
    stream st;

    stream *lst;	/* line stream */
    int eodmark;	/* wether to end stream with an EOD marker */
};

STREAM_DECLARE(asciihex);


#define BLKSIZE 2048	/* buffer size */

static const char hex[16] = "0123456789ABCDEF";



int
asciihex_close(stream_asciihex *st)
{
    stream_close(st->lst);
    
    if (st->eodmark)
	stream_puts(">\n", st->st.st);

    stream_free((stream *)st);

    return 0;
}



stream *
stream_asciihex_open(stream *ost, int eodmark)
{
    stream_asciihex *st;
    stream *lst;
    exception ex;

    lst = stream_line_open(ost);

    if (catch(&ex) == 0) {
	st = stream_create(asciihex, ost);
	drop();
    }
    else {
	stream_close(lst);
	throw(&ex);
    }

    st->lst = lst;
    st->eodmark = eodmark;

    return (stream *)st;
}



int
asciihex_write(stream_asciihex *st, const char *b, int n)
{
    char a[BLKSIZE];	/* room for line breaks */
    int j, i;

    j = 0;
    for (i=0; i<n; i++) {
	a[j++] = hex[((unsigned char)b[i]) >> 4];
	a[j++] = hex[((unsigned char)b[i]) & 0xf];
	if (j >= BLKSIZE-2) {
	    stream_write(st->lst, a, j);
	    j = 0;
	}
    }

    if (j>0)
	stream_write(st->lst, a, j);

    return 0;
}
