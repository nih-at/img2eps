/*
  $NiH: st_runlength.c,v 1.3 2002/09/14 02:27:40 dillo Exp $

  st_runlength.c -- RunLengthEncode stream
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



#include "stream.h"
#include "stream_types.h"



struct stream_runlength {
    stream st;

    int run;		/* length of current run */
    int byte;		/* byte of current run */
    char b[132];	/* buffer for literals */
    int count;		/* fill count of b */
};

STREAM_DECLARE(runlength);

static void _write_literal(stream_runlength *st, char *b, int n);
static void _write_run(stream_runlength *st, int b, int n);



int
runlength_close(stream_runlength *st)
{
    if (st->run == 1) {
	st->b[st->count++] = st->byte;
	st->run = 0;
    }
    _write_literal(st, st->b, st->count);
    _write_run(st, st->byte, st->run);

    stream_putc(128, st->st.st);
    
    stream_free((stream *)st);

    return 0;
}



stream *
stream_runlength_open(stream *ost, void *params)
{
    stream_runlength *st;

    st = stream_create(runlength, ost);

    st->count = 0;
    st->run = 0;
    st->byte = -1;

    return (stream *)st;
}



int
runlength_write(stream_runlength *st, const char *b, int n)
{
    const char *end;
    int count, run, byte, rest;
    char *buf;

    count = st->count;
    run = st->run;
    byte = st->byte;
    buf = st->b;

    end = b+n;
    while (b<end) {
	if (*b == byte) {
	    if (++run > 128) {
		_write_literal(st, buf, count);
		count = 0;
		_write_run(st, byte, 128);
		run -= 128;
	    }
	}
	else {
	    if (run > 3) {
		_write_literal(st, buf, count);
		count = 0;
		_write_run(st, byte, run);
		run = 0;
	    }
	    else {
		while (run-- > 0)
		    buf[count++] = byte;
		if (count >= 128) {
		    _write_literal(st, buf, 128);
		    rest = count - 128;
		    count = 0;
		    while (count < rest) {
			buf[count] = buf[count+128];
			count++;
		    }
		}
	    }
	    byte = *b;
	    run = 1;
	}
	b++;
    }
    st->count = count;
    st->run = run;
    st->byte = byte;

    return 0;
}



static void
_write_literal(stream_runlength *st, char *b, int n)
{
    if (n == 0)
	return;

    stream_putc(n-1, st->st.st);
    stream_write(st->st.st, b, n);
}



static void
_write_run(stream_runlength *st, int c, int n)
{
    char b[2];

    if (n == 0)
	return;

    b[0] = 257-n;
    b[1] = c;

    stream_write(st->st.st, b, 2);
}
