/*
  $NiH: im_xpm.c,v 1.5 2002/09/14 02:27:39 dillo Exp $

  im_xpm.c -- XPM (X Pixmap) image handling
  Copyright (C) 2002 Dieter Baron

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

#ifdef USE_XPM
*/

#define NOSUPP_SCALE
#define NOSUPP_RAW

#include "exceptions.h"
#include "image.h"
#include "xmalloc.h"



#define XPM_MAGIC	"/* XPM */\n"
#define XPM_MAGIC2	"static char *"

#define GOT_MONO	1
#define GOT_GRAY4	2
#define GOT_GRAY	4
#define GOT_RGB		8

struct col {
    char *name;
    int m, g4, gr;	/* monochrome, gray4, gray */
    int r, g, b;	/* red/green/blue for rgb */
};

struct image_xpm {
    image im;

    FILE *f;		/* file to read from */
    char *cname;	/* color names */
    char *pal;		/* palette string */
    char *buf;		/* image data buffer */
    struct col *col;	/* palette */
    int ncol;		/* number of entries in palette */
    int flags;		/* color types present */
    int cpp;		/* characters per pixel */
};

IMAGE_DECLARE(xpm);

static int _colcmp(const void *v1, const void *v2);
static int _colsearch(const void *k, const void *v);
static int _get_int(char **pp);
static char *_get_line(char *b, size_t n, FILE *f, char *fname);
void _parse_colors(image_xpm *im);



void
xpm_close(image_xpm *im)
{
    free(im->pal);
    free(im->cname);
    free(im->col);
    free(im->buf);
    fclose(im->f);
    
    image_free((image *)im);
}



char *
xpm_get_palette(image_xpm *im)
{
    int i, n;
    char *p;

    if (im->im.i.cspace.type != IMAGE_CS_INDEXED)
	return NULL;

    n = image_cspace_palette_size(&im->im.i.cspace);
    p = im->pal = xmalloc(n);
    memset(p, 0, n);

    for (i=0; i<im->ncol; i++) {
	if (im->im.i.cspace.base_type == IMAGE_CS_RGB) {
	    *p++ = im->col[i].r;
	    *p++ = im->col[i].g;
	    *p++ = im->col[i].b;
	}
	else {
	    if (im->im.i.cspace.base_depth == 8) {
		if (im->flags & GOT_GRAY)
		    *p++ = im->col[i].gr;
		else
		    *p++ = im->col[i].g4 * 17;
	    }
	    else		
		p[i/4] = im->col[i].g4 << (6-(i%4)*2);
	}
    }

    return im->pal;
}



image *
xpm_open(char *fname)
{
    image_xpm *im;
    exception ex;
    FILE *f;
    char b[128], *p;
    int i;

    if ((f=fopen(fname, "r")) == NULL)
	return NULL;

    if (fgets(b, sizeof(b), f) == NULL
	|| strcmp(b, XPM_MAGIC) != 0) {
	fclose(f);
	return NULL;
    }
    if (fgets(b, sizeof(b), f) == NULL
	|| strncmp(b, XPM_MAGIC2, strlen(XPM_MAGIC2)) != 0) {
	fclose(f);
	return NULL;
    }

    if (catch(&ex) == 0) {
	im = image_create(xpm, fname);
	drop();
    }
    else {
	fclose(f);
	throw(&ex);
    }

    im->f = f;
    im->col = NULL;
    im->cname = NULL;
    im->pal = NULL;
    im->buf = NULL;
    im->flags = 0;

    if (catch(&ex) == 0) {
	p = _get_line(b, sizeof(b), f, fname);
	im->im.i.width = _get_int(&p);
	im->im.i.height = _get_int(&p);
	im->ncol = _get_int(&p);
	im->cpp = _get_int(&p);

	drop ();
    }
    else {
	image_close((image *)im);
	throw(&ex);
    }
    _parse_colors(im);

    if (im->flags == GOT_MONO) {
	/* mono only */
	im->im.i.cspace.type = IMAGE_CS_GRAY;
	im->im.i.cspace.depth = 1;
    }
    else {
	im->im.i.cspace.type = IMAGE_CS_INDEXED;
	im->im.i.cspace.base_type = (im->flags & GOT_RGB
				     ? IMAGE_CS_RGB
				     : IMAGE_CS_GRAY);
	im->im.i.cspace.base_depth = (im->flags & (GOT_RGB|GOT_GRAY) ? 8 : 4);
	
	for (i=1; im->ncol > (1<<i); i*=2)
	    if (i>16)
		throws(EINVAL, "xpm: more than 64k colors");
	im->im.i.cspace.depth = i;
    }
    im->im.i.compression = IMAGE_CMP_NONE;

    im->im.oi = im->im.i;

    return (image *)im;
}



int
xpm_read(image_xpm *im, char **bp)
{
    char b[8192], *s, *t;
    int i, c, cn, cpp, depth;
    struct col *col;

    cpp = im->cpp;
    depth = im->im.i.cspace.depth;
    t = im->buf;

    memset(t, 0, image_get_row_size((image *)im));
    s = _get_line(b, sizeof(b), im->f, im->im.fname);

    for (i=0; i<im->im.i.width; i++) {
	c = s[cpp];
	s[cpp] = '\0';
	col = bsearch(s, im->col, im->ncol, sizeof(im->col[0]), _colsearch);
	s[cpp] = c;
	s += cpp;
	
	if (col == NULL)
	    throws(EINVAL, "xpm: unknown color string");
	
	if (im->im.i.cspace.type == IMAGE_CS_GRAY) {
		t[i/8] |= col->m << (7-(i%8));
	}
	else {
	    cn = col-im->col;

	    switch (depth) {
	    case 1:
		t[i/8] |= cn << (7-(i%8));
		break;
	    case 2:
		t[i/4] |= cn << (6-(i%4)*2);
		break;
	    case 4:
		t[i/2] |= cn << (4-(i%2)*4);
		break;
	    case 8:
		t[i] = cn;
		break;
	    case 16:
		t[i*2] = cn >> 8;
		t[i*2+1] = cn & 0xff;
		break;
	    }
	}
    }

    *bp = t;
    return 0;
}



void
xpm_read_start(image_xpm *im)
{
    im->buf = xmalloc(image_get_row_size((image *)im));
}



void
xpm_read_finish(image_xpm *im, int abortp)
{
    free (im->buf);
    im->buf = NULL;
}



int
xpm_set_cspace(image_xpm *im, int mask, const image_cspace *cspace)
{
    int i, depth, base_depth;

    mask = image_cspace_diffs(&im->im.i.cspace, mask, cspace);

    base_depth = (mask&IMAGE_INF_BASE_DEPTH)
	? cspace->base_depth : im->im.i.cspace.base_depth;
    depth = (mask&IMAGE_INF_DEPTH) ? cspace->depth : im->im.i.cspace.depth;
    
    switch ((mask&IMAGE_INF_TYPE) ? cspace->type : im->im.i.cspace.type) {
    case IMAGE_CS_GRAY:
	if (depth != 1 || !(im->flags&GOT_MONO))
	    return mask;
	break;

    case IMAGE_CS_INDEXED:
	if (im->ncol > (1<<depth))
	    return mask;
	switch ((mask&IMAGE_INF_BASE_TYPE)
		? im->im.i.cspace.base_type : cspace->base_type) {
	case IMAGE_CS_GRAY:
	    if ((im->flags&(GOT_GRAY|GOT_GRAY4)) == 0)
		return mask;
	    switch (base_depth) {
	    case 0:
		base_depth = im->flags&GOT_GRAY ? 8 : 4;
		break;
	    case 2:
		if ((im->flags&GOT_GRAY4) == 0)
		    return mask;
		break;
	    case 8:
		break;
	    default:
		return mask;
	    }
	    break;

	case IMAGE_CS_RGB:
	    if ((im->flags&GOT_RGB) == 0
		|| (base_depth != 0 && base_depth != 8))
		return mask;
	    base_depth = 8;
	    break;

	default:
	    return mask;
	}

	if ((mask&IMAGE_INF_DEPTH) == 0) {
	    for (i=1; im->ncol > (1<<i); i*=2)
		;
	    depth = i;
	}
	break;

    default:
	return mask;
    }

    image_cspace_merge(&im->im.i.cspace,
		       mask&(IMAGE_INF_TYPE|IMAGE_INF_DEPTH
			     |IMAGE_INF_BASE_TYPE
			     |IMAGE_INF_BASE_DEPTH), cspace);

    if (depth)
	im->im.i.cspace.depth = depth;
    if (base_depth)
	im->im.i.cspace.depth = base_depth;

    return 0;
}



static int
_colcmp(const void *v1, const void *v2)
{
    return strcmp(((struct col *)v1)->name,
		  ((struct col *)v2)->name);
}



static int
_colsearch(const void *k, const void *v)
{
    return strcmp((char *)k, ((struct col *)v)->name);
}



static int
_get_int(char **pp)
{
    char *end;
    int i;

    *pp += strspn(*pp, " \t");

    if (**pp == '\0')
	throws(EINVAL, "xpm: premature end of line");

    i = strtol(*pp, &end, 10);
    if (*pp == end)
	throws(EINVAL, "xpm: number expected");

    *pp = end;

    return i;
}



static char *
_get_line(char *b, size_t n, FILE *f, char *fname)
{
    char *p, *q;
    
    if (fgets(b, n, f) == NULL)
	throwf(errno, "read error on `%s': %s",
	       fname, strerror(errno));

    if ((p=strchr(b, '"')) == NULL)
	throws(EINVAL, "xpm: start of string not found");
    
    if ((q=strchr(p+1, '"')) == NULL)
	throws(EINVAL, "xpm: end of string not found");

    *q = '\0';
    return p+1;
}



#define SKIP(p)	((p) += strspn((p), " \t"));

void
_parse_colors(image_xpm *im)
{
    int i, cpp, key, val, flags, four;
    char b[1024], *p, *end;

    cpp = im->cpp;
    im->col = xmalloc(sizeof(*im->col) * im->ncol);
    im->cname = xmalloc((cpp+1)*im->ncol);

    for (i=0; i<im->ncol; i++) {
	p = _get_line(b, sizeof(b), im->f, im->im.fname);

	/* color name */
	
	im->col[i].name = im->cname + (cpp+1)*i;
	strncpy(im->col[i].name, p, cpp);
	im->col[i].name[cpp] = '\0';
	p += cpp;
	SKIP(p);

	flags = 0;
	while (*p) {
	    key = *p++;
	    four = (*p == '4');
	    SKIP(p);
	    switch (key) {
	    case 'c':
		flags |= GOT_RGB;
		switch (*p++) {
		case '#':
		    val = strtol(p, &end, 16);
		    if (end != p+6)
			throwf(EINVAL, "xpm: invalid RGB color `%.*s'",
			       p, end-p);
		    im->col[i].r = val>>16;
		    im->col[i].g = (val>>8) & 0xff;
		    im->col[i].b = val & 0xff;
		    p = end;
		    break;

		case '%':
		    throws(EOPNOTSUPP, "xpm: HSV colors not supported");

		case 'n':
		case 'N':
		    if (strncasecmp(p, "one", 3) == 0
			&& (p[3] == '\0' || isspace(p[3]))) {
			/* XXX: mark as transparent */
			im->col[i].r = 0xff;
			im->col[i].g = 0xff;
			im->col[i].b = 0xff;
			break;
		    }
		    /* fallthrough */

		default:
		    throwf(EINVAL, "xpm: unrecognized RBG color code %c",
			   p[-1]);
		}
		break;


	    case 'g':
		if (four) {
		    im->col[i].g4 = _get_int(&p);
		    flags |= GOT_GRAY4;
		}
		else {
		    im->col[i].gr = _get_int(&p);
		    flags |= GOT_GRAY;
		    break;
		}
	
		
	    case 'm':
		flags |= GOT_MONO;
		
		if (strncasecmp(p, "white", 5) == 0)
		    im->col[i].m = 0;
		else if (strncasecmp(p, "black", 5) == 0)
		    im->col[i].m = 1;
		else
		    throws(EINVAL, "xpm: unrecoginzed monochrom color name");
		p += 5;
		break;

	    case 's':
		break;
	    }
	}

	if (flags == 0)
	    throws(EOPNOTSUPP, "xpm: no supported color specification found");
	if (im->flags && im->flags != flags)
	    throws(EINVAL,
		   "xpm: set of color specification types inconsistent");
	else
	    im->flags = flags;
    }

    qsort(im->col, im->ncol, sizeof(im->col[0]), _colcmp);
}

/* #endif / * USE_XPM */
