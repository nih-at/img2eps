/*
  $NiH$

  st_line.c -- break ASCII stream into lines stream
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
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

    if ((st=stream_create(line, ost)) == NULL)
	return NULL;

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
	
	if (stream_write(st->st.st, b, nn) != 0)
	    return -1;
	if (nl)
	    if (stream_puts("\n", st->st.st) != 0)
		return -1;

	b += nn;
	st->llen = (st->llen + nn) % LINELEN;
    }

    return 0;
}
