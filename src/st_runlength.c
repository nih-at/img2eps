/*
  $NiH: st_runlength.c,v 1.1 2002/09/09 12:42:34 dillo Exp $

  st_runlength.c -- RunLengthEncode stream
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
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
stream_runlength_open(stream *ost)
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
