/*
  $NiH: st_ccitt.c,v 1.2 2005/07/24 04:50:58 dillo Exp $

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
#include "util.h"
#include "xmalloc.h"



struct stream_ccitt {
    stream st;

    int width;
    int flags;
    int k;
    int k_max;

    int *ce[2];
    int *ref;
    int remain;
    int len;
};

STREAM_DECLARE(ccitt);



#define BIT_WHITE	1
#define BIT_BLACK	0

#define CODE_MAX_TERM		  64
#define CODE_MAX_MAKEUP		(1728+CODE_MAX_TERM)
#define CODE_MAX_EXTENDED	(2560+CODE_MAX_TERM)
#define CODE_MAX_VERTICAL	   4

struct code {
    unsigned short len;
    unsigned short code;
};

static const struct code code_eol = {
    12, 0x0001
};

static const struct code code_pass = {
    4, 0x0001
};

static const struct code code_horizontal = {
    3, 0x0001
};

static const struct code code_vertical_0[] = {
    { 7, 0x0003 },
    { 6, 0x0003 },
    { 3, 0x0003 },
    { 1, 0x0001 },
    { 3, 0x0002 },
    { 6, 0x0002 },
    { 7, 0x0002 }
};

/* so we can use the difference as index */
static const struct code *code_vertical = code_vertical_0+CODE_MAX_VERTICAL-1;

static const struct code code_term[2][CODE_MAX_TERM] = {
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

static const struct code code_makeup[2][27] = {
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

static const struct code code_extended[] = {
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
static void put_eofb(stream_ccitt *);
static void put_pad(stream_ccitt *);
static void put_run(stream_ccitt *, int, int);
static void put_rtc(stream_ccitt *);




int
ccitt_close(stream_ccitt *st)
{
    if (st->flags & IMAGE_CMP_CCITT_EOB) {
	if (IMAGE_CMP_CCITT_MODE(st->flags) == IMAGE_CMP_CCITT_G4)
	    put_eofb(st);
	else
	    put_rtc(st);
    }

    put_pad(st);

    free(st->ce[0]);
    free(st->ce[1]);
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
    st->flags = info->compression_flags;
    st->remain = 0;
    st->len = 0;
    if (IMAGE_CMP_CCITT_MODE(st->flags) == IMAGE_CMP_CCITT_G31D)
	st->ce[0] = st->ce[1] = NULL;
    else {
	st->ce[0] = xmalloc(sizeof(st->ce[0][0])*(st->width+1));
	st->ce[1] = xmalloc(sizeof(st->ce[1][0])*(st->width+1));
	st->ref = st->ce[0];
    }

    switch (IMAGE_CMP_CCITT_MODE(st->flags)) {
    case IMAGE_CMP_CCITT_G32D:
	/* end of line markers are required in Group 3 2-D */
	st->flags |= IMAGE_CMP_CCITT_EOL;
	/* XXX: proper values */
	st->k_max = 4;
	st->k = 0;
	break;

    case IMAGE_CMP_CCITT_G4:
	st->flags &= ~IMAGE_CMP_CCITT_EOL;
	/* init reference line to all white */
	st->ref[0] = info->width;
    }
    
    return (stream *)st;
}



int
ccitt_write(stream_ccitt *st, const char *b, int n)
{
    struct code code_mode;
    int hori;
    
    /* XXX: assumes one scanline at a time */

    if (st->flags & IMAGE_CMP_CCITT_EOL)
	put_code(st, &code_eol);

    switch (IMAGE_CMP_CCITT_MODE(st->flags)) {
    case IMAGE_CMP_CCITT_G31D:
	hori = 1;
	break;
	
    case IMAGE_CMP_CCITT_G32D:
	/* keep track of which lines to encode 1-D/2-D */
	hori = (st->k == 0);
	if (st->k++ == st->k_max)
	    st->k = 0;
	
	/* write 1-D/2-D indicator */
	code_mode.len = 1;
	code_mode.code = hori;
	put_code(st, &code_mode);
	break;
	
    case IMAGE_CMP_CCITT_G4:
	hori = 0;
	break;
    }

    if (hori) {
	int i, x, bit, len;

	i = x = 0;
	bit = BIT_WHITE;
	while (x < st->width) {
	    len = bitspan(b, x, st->width, bit);
	    put_run(st, bit, len);
	    if (st->ce[0])
		st->ref[i++] = x;
	    x += len;
	    bit = !bit;
	}
	if (st->ce[0])
	    st->ref[i] = x;
    }
    else {
	int i, j, done, ce, next_ce;
	int *next_ref;

	next_ref = (st->ref == st->ce[0] ? st->ce[1] : st->ce[0]);

	done = 0;
	ce = bitspan(b, 0, st->width, BIT_WHITE);
	j = i = 0;

	while (done < st->width) {
	    if (st->ref[i] < st->width && st->ref[i+1] < ce) {
		/* this span doesn't exist in new line */
		put_code(st, &code_pass);
		done = st->ref[i+1];
		i += 2;
		
		/* we're not past this change element yet */
		continue;
	    }
	    else {
		if (abs(st->ref[i]-ce) < CODE_MAX_VERTICAL) {
		    /* vertical coding */
		    put_code(st, &code_vertical[st->ref[i]-ce]);
		    if (st->ref[i] < st->width)
			i++;
		    done = ce;
		}
		else {
		    /* horizontal coding */
		    put_code(st, &code_horizontal);
		    next_ce = ce + bitspan(b, ce, st->width, -1);
		    put_run(st, (j%2 ? BIT_BLACK : BIT_WHITE), ce - done);
		    put_run(st, (j%2 ? BIT_WHITE : BIT_BLACK), next_ce - ce);
		    next_ref[j++] = ce;
		    done = ce = next_ce;

		    if (done >= st->width)
			break;
		}
	    }

	    /* skip ref ces left of done */
	    while (st->ref[i] <= done) {
		if (st->ref[i+1] == st->width)
		    i++;
		else
		    i += 2;
	    }
	    next_ref[j++] = ce;
	    ce += bitspan(b, ce, st->width, -1);
	}

	next_ref[j] = done;
	st->ref = next_ref;
    }

    if (st->flags & IMAGE_CMP_CCITT_ALIGN)
	put_pad(st);

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
put_eofb(stream_ccitt *st)
{
    int i;

    for (i=0; i<2; i++)
	put_code(st, &code_eol);
}



static void
put_pad(stream_ccitt *st)
{
    struct code pad;
    
    if (st->len % 8) {
	pad.code = 0;
	pad.len = 8-st->len%8;
	put_code(st, &pad);
    }
}



static void
put_run(stream_ccitt *st, int bit, int len)
{
    while (len >= CODE_MAX_EXTENDED) {
	put_code(st, &code_extended[CODE_MAX_EXTENDED/CODE_MAX_TERM-1]);
	len -= CODE_MAX_EXTENDED;
    }
    if (len >= CODE_MAX_MAKEUP) {
	put_code(st, &code_extended[(len-CODE_MAX_MAKEUP)/CODE_MAX_TERM]);
    }
    else if (len >= CODE_MAX_TERM) {
	put_code(st, &code_makeup[bit][(len-CODE_MAX_TERM)/CODE_MAX_TERM]);
    }
    put_code(st, &code_term[bit][len%CODE_MAX_TERM]);
}



static void
put_rtc(stream_ccitt *st)
{
    int i;

    for (i=0; i<6; i++)
	put_code(st, &code_eol);
}
