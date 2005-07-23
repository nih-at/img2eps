/*
  $NiH$

  im_xbm.c -- XBM (X Bitmap) image handling
  Copyright (C) 2002, 2005 Dieter Baron

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



#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

/*
#include "config.h"

#ifdef USE_XBM
*/

#define NOSUPP_CSPACE
#define NOSUPP_RAW
#define NOSUPP_SCALE

#include "exceptions.h"
#include "image.h"
#include "xmalloc.h"



#define LINESIZE	1024

struct image_xbm {
    image im;

    FILE *f;		/* file to read from */
    unsigned char *row;	/* row of image data */
    char *line;		/* line of file data */
    char *p;		/* pointer to next unused byte in buf, NULL for none */
};

IMAGE_DECLARE(xbm);

static int _get_byte(char **);
static int _get_int(char **);



void
xbm_close(image_xbm *im)
{
    free(im->line);
    free(im->row);
    fclose(im->f);
    
    image_free((image *)im);
}



char *
xbm_get_palette(image_xbm *im)
{
    return NULL;
}



image *
xbm_open(char *fname)
{
    image_xbm *im;
    exception ex;
    FILE *f;
    char b1[LINESIZE], b2[LINESIZE], *name, *p;
    int name_len;
    int width, height;

    if ((f=fopen(fname, "r")) == NULL)
	return NULL;

    if (fgets(b1, sizeof(b1), f) == NULL
	|| strncmp(b1, "#define ", 8) != 0
	|| (p=strchr(b1+8, '_')) == NULL
	|| strncmp(p, "_width ", 7) != 0) {
	fclose(f);
	return NULL;
    }
    name = b1+8;
    name_len = p-(b1+8);
    p += 7;
    width = _get_int(&p);
    if (p == NULL) {
	fclose(f);
	return NULL;
    }

    if (fgets(b2, sizeof(b2), f) == NULL
	|| strncmp(b2, b1, name_len+8) != 0
	|| strncmp(b2+name_len+8, "_height ", 8) != 0) {
	fclose(f);
	return NULL;
    }
    p = b2+name_len+16;
    height = _get_int(&p);
    if (p == NULL) {
	fclose(f);
	return NULL;
    }
    if (fgets(b2, sizeof(b2), f) == NULL
	|| strncmp(b2, "static unsigned char ", 21) != 0
	|| strncmp(b2+21, b1+8, name_len) != 0
	|| strcmp(b2+21+name_len, "_bits[] = {\n") != 0) {
	fclose(f);
	return NULL;
    }
    
    if (catch(&ex) == 0) {
	im = image_create(xbm, fname);
	drop();
    }
    else {
	fclose(f);
	throw(&ex);
    }

    im->im.i.width = width;
    im->im.i.height = height;
    im->im.i.cspace.type = IMAGE_CS_GRAY;
    im->im.i.cspace.depth = 1;
    im->f = f;
    im->line = NULL;
    im->row = NULL;

    return (image *)im;
}



int
xbm_read(image_xbm *im, char **bp)
{
    int i;

    for (i=0; i<(im->im.i.width+7)/8; i++) {
	if (im->p == NULL) {
	    if (fgets(im->line, LINESIZE, im->f) == NULL)
		throws(EINVAL, "xbm: premature EOF");
	    im->p = im->line+strspn(im->line, " \t");
	}
	im->row[i] = mirror_byte(_get_byte(&im->p)) ^ 0xff;
    }

    *bp = im->row;
    return 0;
}



void
xbm_read_start(image_xbm *im)
{
    im->row = xmalloc(image_get_row_size((image *)im));
    im->line = xmalloc(LINESIZE);
    im->p = NULL;
}



void
xbm_read_finish(image_xbm *im, int abortp)
{
    free(im->row);
    im->row = NULL;
    free(im->line);
    im->line = NULL;
}



static int
_get_byte(char **pp)
{
    char *p, *end;
    int c;

    p = *pp;
    
    if (p[0] != '0' || p[1] != 'x')
	throws(EINVAL, "xbm: hex number expected");
    p += 2;    
    c = strtol(p, &end, 16);
    if (p == end)
	throws(EINVAL, "xbm: hex number expected");

    end += strspn(end, " \t");
    switch (*end) {
    case '\n':
    case '\0':
	end = NULL;
	break;

    case ',':
    case '}':
	end += strspn(end+1, " \t") + 1;
	if (*end == '\n')
	    end = NULL;
	break;

    default:
	throwf(EINVAL, "xbm: illegal character `%c' in image data", *end);
    }

    *pp = end;
    return c;
}



static int
_get_int(char **pp)
{
    char *end;
    int i;

    if (**pp == '\0') {
	*pp = NULL;
	return 0;
    }

    i = strtol(*pp, &end, 10);
    if (*pp == end) {
	*pp = NULL;
	return 0;
    }

    *pp = end;

    return i;
}

/* #endif / * USE_XBM */
