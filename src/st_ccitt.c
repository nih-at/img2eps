/*
  $NiH$

  st_ccitt.c -- CCITTFaxEncode stream
  Copyright (C) 2005 Dieter Baron

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



struct stream_ccitt {
    stream st;

    int width;

    int remain;
    int len;
};

STREAM_DECLARE(ccitt);



struct code {
    unsigned short len;
    unsigned short code;
};

static const struct code eol = {
    12, 0x0001
};

static const struct code term[2][64] = {
    { { 10, 0x0037 },
      {  3, 0x0002 },
      {  2, 0x0003 },
      {  2, 0x0002 },
      {  3, 0x0003 },
      {  4, 0x0003 },
      {  4, 0x0002 },
      {  5, 0x0003 },
      {  6, 0x0005 },
      {  6, 0x0004 },
      {  7, 0x0004 },
      {  7, 0x0005 },
      {  7, 0x0007 },
      {  8, 0x0004 },
      {  8, 0x0007 },
      {  9, 0x0018 },
      { 10, 0x0017 },
      { 10, 0x0018 },
      { 10, 0x0008 },
      { 11, 0x0067 },
      { 11, 0x0068 },
      { 11, 0x006c },
      { 11, 0x0037 },
      { 11, 0x0028 },
      { 11, 0x0017 },
      { 11, 0x0018 },
      { 12, 0x00ca },
      { 12, 0x00cb },
      { 12, 0x00cc },
      { 12, 0x00cd },
      { 12, 0x0068 },
      { 12, 0x0069 },
      { 12, 0x006a },
      { 12, 0x006b },
      { 12, 0x00d2 },
      { 12, 0x00d3 },
      { 12, 0x00d4 },
      { 12, 0x00d5 },
      { 12, 0x00d6 },
      { 12, 0x00d7 },
      { 12, 0x006c },
      { 12, 0x006d },
      { 12, 0x00da },
      { 12, 0x00db },
      { 12, 0x0054 },
      { 12, 0x0055 },
      { 12, 0x0056 },
      { 12, 0x0057 },
      { 12, 0x0064 },
      { 12, 0x0065 },
      { 12, 0x0052 },
      { 12, 0x0053 },
      { 12, 0x0024 },
      { 12, 0x0037 },
      { 12, 0x0038 },
      { 12, 0x0027 },
      { 12, 0x0028 },
      { 12, 0x0058 },
      { 12, 0x0059 },
      { 12, 0x002b },
      { 12, 0x002c },
      { 12, 0x005a },
      { 12, 0x0066 },
      { 12, 0x0067 }
    },
    { {  8, 0x0035 },
      {  6, 0x0007 },
      {  4, 0x0007 },
      {  4, 0x0008 },
      {  4, 0x000b },
      {  4, 0x000c },
      {  4, 0x000e },
      {  4, 0x000f },
      {  5, 0x0013 },
      {  5, 0x0014 },
      {  5, 0x0007 },
      {  5, 0x0008 },
      {  6, 0x0008 },
      {  6, 0x0003 },
      {  6, 0x0034 },
      {  6, 0x0035 },
      {  6, 0x002a },
      {  6, 0x002b },
      {  7, 0x0027 },
      {  7, 0x000c },
      {  7, 0x0008 },
      {  7, 0x0017 },
      {  7, 0x0003 },
      {  7, 0x0004 },
      {  7, 0x0028 },
      {  7, 0x002b },
      {  7, 0x0013 },
      {  7, 0x0024 },
      {  7, 0x0018 },
      {  8, 0x0002 },
      {  8, 0x0003 },
      {  8, 0x001a },
      {  8, 0x001b },
      {  8, 0x0012 },
      {  8, 0x0013 },
      {  8, 0x0014 },
      {  8, 0x0015 },
      {  8, 0x0016 },
      {  8, 0x0017 },
      {  8, 0x0028 },
      {  8, 0x0029 },
      {  8, 0x002a },
      {  8, 0x002b },
      {  8, 0x002c },
      {  8, 0x002d },
      {  8, 0x0004 },
      {  8, 0x0005 },
      {  8, 0x000a },
      {  8, 0x000b },
      {  8, 0x0052 },
      {  8, 0x0053 },
      {  8, 0x0054 },
      {  8, 0x0055 },
      {  8, 0x0024 },
      {  8, 0x0025 },
      {  8, 0x0058 },
      {  8, 0x0059 },
      {  8, 0x005a },
      {  8, 0x005b },
      {  8, 0x004a },
      {  8, 0x004b },
      {  8, 0x0032 },
      {  8, 0x0033 },
      {  8, 0x0034 }
    }
};

static const struct code makeup[2][27] = {
    { { 10, 0x000f },
      { 12, 0x00c8 },
      { 12, 0x00c9 },
      { 12, 0x005b },
      { 12, 0x0033 },
      { 12, 0x0034 },
      { 12, 0x0035 },
      { 13, 0x006c },
      { 13, 0x006d },
      { 13, 0x004a },
      { 13, 0x004b },
      { 13, 0x004c },
      { 13, 0x004d },
      { 13, 0x0072 },
      { 13, 0x0073 },
      { 13, 0x0074 },
      { 13, 0x0075 },
      { 13, 0x0076 },
      { 13, 0x0077 },
      { 13, 0x0052 },
      { 13, 0x0053 },
      { 13, 0x0054 },
      { 13, 0x0055 },
      { 13, 0x005a },
      { 13, 0x005b },
      { 13, 0x0064 },
      { 13, 0x0065 }
    },
    { {  5, 0x001b },
      {  5, 0x0012 },
      {  6, 0x0017 },
      {  7, 0x0037 },
      {  8, 0x0036 },
      {  8, 0x0037 },
      {  8, 0x0064 },
      {  8, 0x0065 },
      {  8, 0x0068 },
      {  8, 0x0067 },
      {  9, 0x00cc },
      {  9, 0x00cd },
      {  9, 0x00d2 },
      {  9, 0x00d3 },
      {  9, 0x00d4 },
      {  9, 0x00d5 },
      {  9, 0x00d6 },
      {  9, 0x00d7 },
      {  9, 0x00d8 },
      {  9, 0x00d9 },
      {  9, 0x00da },
      {  9, 0x00db },
      {  9, 0x0098 },
      {  9, 0x0099 },
      {  9, 0x009a },
      {  6, 0x0018 },
      {  9, 0x009b }
    }
};

static const struct code extended_makeup[] = {
    { 11, 0x0008 },
    { 11, 0x000c },
    { 11, 0x000d },
    { 12, 0x0012 },
    { 12, 0x0013 },
    { 12, 0x0014 },
    { 12, 0x0015 },
    { 12, 0x0016 },
    { 12, 0x0017 },
    { 12, 0x001c },
    { 12, 0x001d },
    { 12, 0x001e },
    { 12, 0x001f }
};



#define MASK(n)		((1<<(n))-1)

static void flush(stream_ccitt *);
static void put_code(stream_ccitt *, const struct code *);
static void put_run(stream_ccitt *, int, int);
static void put_rtc(stream_ccitt *);




int
ccitt_close(stream_ccitt *st)
{
    put_rtc(st);

    if (st->len > 0) {
	/* pad to byte boundary */
	if (st->len % 8)
	    st->remain <<= 8-st->len%8;
	/* write remaining bits */
	flush(st);
    }

    stream_free((stream *)st);

    return 0;
}



stream *
stream_ccitt_open(stream *ost, void *params)
{
    stream_ccitt *st;
    image_info *info;

    info = params;

    st = stream_create(ccitt, ost);

    /* XXX: check for bi-level image */
    /* XXX: check cspace->inverted */
    /* XXX: check bit order */

    st->width = info->width;
    st->remain = 0;
    st->len = 0;

    return (stream *)st;
}



#define BIT(b, x)	((((unsigned char *)(b))[(x)/8] >> (7-((x)%8))) & 1)

int
ccitt_write(stream_ccitt *st, const char *b, int n)
{
    int x;
    int bit, len;
    
    /* XXX: assumes one scanline at a time */

    put_code(st, &eol);

    len = 0;
    bit = 1;
    for (x=0; x<st->width; x++) {
	if (BIT(b, x) == bit)
	    len++;
	else {
	    put_run(st, bit, len);
	    len = 1;
	    bit = !bit;
	}
    }
    put_run(st, bit, len);

    return 0;
}



static void
flush(stream_ccitt *st)
{
    while (st->len > 7) {
	stream_putc((st->remain>>(st->len-8)) & 0xff, st->st.st);
	st->len -= 8;
    }
    st->remain &= MASK(st->len);
}



static void
put_code(stream_ccitt *st, const struct code *code)
{
    st->len += code->len;
    st->remain = (st->remain << code->len) | code->code;

    flush(st);
}



static void
put_run(stream_ccitt *st, int bit, int len)
{
    /* XXX: check that len is small enough */

    if (len >= 64)
	put_code(st, &makeup[bit][(len/64)-1]);
    put_code(st, &term[bit][len%64]);
}

static void
put_rtc(stream_ccitt *st)
{
    int i;

    for (i=0; i<6; i++)
	put_code(st, &eol);
}
