/*
  $NiH: st_asciihex.c,v 1.1 2002/09/07 20:58:01 dillo Exp $

  st_asciihex.c -- ASCIIHexEncode stream
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

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

    if ((lst=stream_line_open(ost)) == NULL)
	return NULL;

    if ((st=stream_create(asciihex, ost)) == NULL)
	return NULL;

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
	    if (stream_write(st->lst, a, j) != 0)
		return -1;
	    j = 0;
	}
    }

    if (j>0) {
	if (stream_write(st->lst, a, j) != 0)
	    return -1;
    }

    return 0;
}
