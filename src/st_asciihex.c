/*
  $NiH$

  st_asciihex.c -- ASCIIHexEncode stream
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include "stream.h"



struct stream_asciihex {
    stream st;

    int llen;		/* length of partial line */
    int eodmark;	/* wether to end stream with an EOD marker */
};

STREAM_DECLARE(asciihex);


#define BLKSIZE 2048	/* buffer size */
#define LINELEN 72	/* line length */

static const char hex[16] = "0123456789ABCDEF";



int
asciihex_close(stream_asciihex *st)
{
    if (st->llen)
	stream_write(st->st.st, "\n", 1);
    
    if (st->eodmark)
	stream_write(st->st.st, ">\n", 2);

    stream_free((stream *)st);

    return 0;
}



stream *
stream_asciihex_open(stream *ost, int eodmark)
{
    stream_asciihex *st;

    if ((st=stream_create(asciihex, ost)) == NULL)
	return NULL;

    st->eodmark = eodmark;
    st->llen = 0;

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
	if ((j+st->llen) % LINELEN == 0) {
	    a[j++] = '\n';
	    --st->llen;
	}
	if (j >= BLKSIZE-3) {
	    if (stream_write(st->st.st, a, j) != 0)
		return -1;
	    st->llen = (j+st->llen) % LINELEN;
	    j = 0;
	}
    }

    if (j>0) {
	if (stream_write(st->st.st, a, j) != 0)
	    return -1;
    }

    st->llen = (j+st->llen) % LINELEN;
    return 0;
}
