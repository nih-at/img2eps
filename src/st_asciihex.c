/*
  $NiH: st_asciihex.c,v 1.2 2002/09/08 00:27:49 dillo Exp $

  st_asciihex.c -- ASCIIHexEncode stream
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
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
