/*
  $NiH: st_ascii85.c,v 1.2 2002/09/08 01:34:49 dillo Exp $

  st_ascii85.c -- ASCII85Encode stream
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include "exceptions.h"
#include "stream.h"
#include "stream_types.h"



struct stream_ascii85 {
    stream st;

    stream *lst;	/* line stream */
    unsigned long rest;	/* partial tupple */
    int nrest;		/* number of bytes in partial tupple */
    int eodmark;	/* wether to end stream with an EOD marker */
};

STREAM_DECLARE(ascii85);


#define BLKSIZE 2048	/* buffer size */

static const char hex[16] = "0123456789ABCDEF";



int
ascii85_close(stream_ascii85 *st)
{
    char a[5];
    int i;

    if (st->nrest) {
	for (i=st->nrest; i<4; i++)
	    st->rest <<= 8;
	for (i=4; i>=0; --i) {
	    a[i] = st->rest%85 + 33;
	    st->rest /= 85;
	}
	stream_write(st->lst, a, st->nrest+1);
    }

    stream_close(st->lst);
    
    if (st->eodmark)
	stream_puts("~>\n", st->st.st);

    stream_free((stream *)st);

    return 0;
}



stream *
stream_ascii85_open(stream *ost, int eodmark)
{
    stream_ascii85 *st;
    stream *lst;
    exception ex;

    lst = stream_line_open(ost);

    if (catch(&ex) == 0) {
	st = stream_create(ascii85, ost);
	drop();
    }
    else {
	stream_close(lst);
	throw(&ex);
    }

    st->lst = lst;
    st->eodmark = eodmark;
    st->rest = 0;
    st->nrest = 0;

    return (stream *)st;
}



int
ascii85_write(stream_ascii85 *st, const char *b, int n)
{
    const char *end;
    char a[BLKSIZE];
    int i, j, nrest; 
    unsigned long l;

    end = b+n;
    l = st->rest;
    nrest = st->nrest;
    i = 0;
    while (b<end) {
	l = l<<8 | (unsigned char)*(b++);
	nrest++;
	if (nrest == 4) {
	    if (l == 0)
		a[i++] = 'z';
	    else {
		for (j=4; j>=0; --j) {
		    a[i+j] = l%85 + 33;
		    l /= 85;
		}
		i += 5;
	    }
	    l = 0;
	    nrest = 0;
	    
	    if (i >= BLKSIZE-5) {
		stream_write(st->lst, a, i);
		i = 0;
	    }
	}
    }

    if (i>0)
	stream_write(st->lst, a, i);

    st->rest = l;
    st->nrest = nrest;

    return 0;
}
