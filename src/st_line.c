/*
  $NiH: st_line.c,v 1.2 2002/09/10 14:05:53 dillo Exp $

  st_line.c -- break ASCII stream into lines stream
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>

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
 
  THIS SOFTWARE IS PROVIDED BY DIETER BARON ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL DIETER BARON BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#include "stream.h"
#include "stream_types.h"



struct stream_line {
    stream st;

    int llen;		/* length of partial line */
};

STREAM_DECLARE(line);

#define LINELEN 72	/* line length */



int
line_close(stream_line *st)
{
    if (st->llen)
	stream_puts("\n", st->st.st);
    
    stream_free((stream *)st);

    return 0;
}



stream *
stream_line_open(stream *ost)
{
    stream_line *st;

    st = stream_create(line, ost);

    st->llen = 0;

    return (stream *)st;
}



int
line_write(stream_line *st, const char *b, int n)
{
    const char *end;
    int nn, nl;

    end = b+n;
    nl = 1;
    while (b<end) {
	nn = LINELEN - st->llen;
	if (nn > end-b) {
	    nn = end-b;
	    nl = 0;
	}
	
	stream_write(st->st.st, b, nn);

	if (nl)
	    stream_puts("\n", st->st.st);

	b += nn;
	st->llen = (st->llen + nn) % LINELEN;
    }

    return 0;
}
