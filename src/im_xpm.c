/*
  $NiH$

  im_xpm.c -- XPM (X Pixmap) image handling
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <errno.h>
#include <stdio.h>
#include <string.h>

/*
#include "config.h"

#ifdef USE_XPM
*/

#define NOSUPP_SCALE

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



int
xpm_close(image_xpm *im)
{
    int ret;

    free(im->pal);
    free(im->cname);
    free(im->col);
    free(im->buf);
    ret = fclose(im->f);
    
    image_free((image *)im);

    return ret;
}



char *
xpm_get_palette(image_xpm *im)
{
    int i, n;
    char *p;

    if (!IMAGE_CS_IS_INDEXED(im->im.i.cspace))
	return NULL;

    n = image_get_palette_size((image *)im);
    p = im->pal = xmalloc(n);
    
    for (i=0; i<im->ncol; i++) {
	if (im->im.i.cspace == IMAGE_CS_INDEXED_RGB) {
	    *p++ = im->col[i].r;
	    *p++ = im->col[i].g;
	    *p++ = im->col[i].b;
	}
	else if (im->flags & GOT_GRAY)
	    *p++ = im->col[i].gr;
	else
	    *p++ = im->col[i].g4 * 17;
    }
    if (p < im->pal+n)
	memset(p, 0, im->pal+n-p);

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
	im->im.i.cspace = IMAGE_CS_GRAY;
	im->im.i.depth = 1;
    }
    else {
	im->im.i.cspace = (im->flags & GOT_RGB
			   ? IMAGE_CS_INDEXED_RGB
			   : IMAGE_CS_INDEXED_GRAY);
	
	for (i=1; im->ncol > (1<<i); i*=2)
	    if (i>16)
		throws(EINVAL, "xpm: more than 64k colors");
	im->im.i.depth = i;
    }
    im->im.i.compression = IMAGE_CMP_NONE;
    
    return (image *)im;
}



int
xpm_read(image_xpm *im, char **bp)
{
    char b[8192], *s, *t;
    int i, c, cn, cpp, depth;
    struct col *col;

    cpp = im->cpp;
    depth = im->im.i.depth;
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
	
	if (im->im.i.cspace == IMAGE_CS_GRAY) {
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



int
xpm_read_start(image_xpm *im)
{
    im->buf = xmalloc(image_get_row_size((image *)im));
    return 0;
}



int
xpm_read_finish(image_xpm *im, int abortp)
{
    free (im->buf);
    im->buf = NULL;

    return 0;
}



int
xpm_set_cspace_depth(image_xpm *im, image_cspace cspace, int depth)
{
    int supp;

    supp = 0;
    switch (cspace) {
    case IMAGE_CS_GRAY:
	if (depth == 1 && im->flags&GOT_MONO)
	    supp = 1;
	break;

    case IMAGE_CS_INDEXED_GRAY:
	if (im->ncol <= (1<<depth)
	    && (im->flags&GOT_GRAY || im->flags&GOT_GRAY4))
	    supp = 1;
	break;

    case IMAGE_CS_INDEXED_RGB:
	if (im->ncol <= (1<<depth) && im->flags&GOT_RGB)
	    supp = 1;
	break;

    default:
	break;
    }

    if (!supp)
	return -1;

    im->im.i.cspace = cspace;
    im->im.i.depth = depth;

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
    int i, cpp, key, val, flags;
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

		default:
		    throwf(EINVAL, "xpm: unrecognized RBG color code %c",
			   p[-1]);
		}
		break;
	
		
#if 0
	    case 'g4':
		im->col[i].g4 = _get_int(&p);
		flags |= GOT_GRAY4;
		break;
#endif


	    case 'g':
		im->col[i].gr = _get_int(&p);
		flags |= GOT_GRAY;
		break;
	
		
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
