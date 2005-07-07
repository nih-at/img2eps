/*
  $NiH: epsf.c,v 1.30 2005/07/06 14:43:06 dillo Exp $

  epsf.c -- EPS file fragments
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



#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "epsf.h"
#include "exceptions.h"
#include "stream.h"
#include "stream_types.h"
#include "xmalloc.h"

#define PAIR_SEPARATORS		"x*,"

#define DEFAULT_GRAVITY		"center"
#define DEFAULT_IMAGE_SIZE	"-1x-1"
#define DEFAULT_MARGIN		"20"
#define DEFAULT_ORIENTATION	"portrait"
#define DEFAULT_PAPER		"a4"
#define DEFAULT_RESOLUTION	"-1"
#define DEFAULT_FLAGS		EPSF_FLAG_DIRECT_COPY

#define DEFAULT_L2_CMP	IMAGE_CMP_LZW
#ifdef HAVE_LIBZ
#define DEFAULT_L3_CMP	IMAGE_CMP_FLATE
#else
#define DEFAULT_L3_CMP	IMAGE_CMP_LZW
#endif

struct papersize {
    const char *name;
    int width;
    int height;
};

struct papersize papersize[] = {
    { "10x14",      720, 1008 },
    { "a3",         842, 1190 },
    { "a4",         595,  842 },
    { "a5",         420,  595 },
    { "b4",         729, 1032 },
    { "b5",         516,  729 },
    { "executive",  540,  720 },
    { "folio",      612,  936 },
    { "ledger",    1224,  792 },
    { "legal",      612, 1008 },
    { "letter",     612,  792 },
    { "none",        -1,   -1 },
    { "quarto",     610,  780 },
    { "statement",  396,  612 },
    { "tabloid",    792, 1224 },
    { NULL, 0, 0 }
};

struct dimen {
    const char *name;	/* name of dimen */
    int nom, denom;	/* scaling ratio (nominator/denominator) */
};

struct dimen dimen[] = {
    { "cm",  3600, 127 },
    { "in",    72,   1 },
    { "mm",   360, 127 },
    { "pt",     1,   1 },
    { NULL, 0, 0 }
};

struct gravity {
    double x;
    double y;
};

static const struct gravity gravity[] = {
    { 0.,  0.  },
    { 0.5, 0.  },
    { 1.,  0.  },
    { 0.,  0.5 },
    { 0.5, 0.5 },
    { 1.,  0.5 },
    { 0.,  1.  },
    { 0.5, 1.  },
    { 1.,  1.  }
};



const struct _num_name _epsf_nn_grav[] = {
    { EPSF_GRAV_TOPLEFT,     "top-left" },
    { EPSF_GRAV_TOP,         "top" },
    { EPSF_GRAV_TOPRIGHT,    "top-right" },
    { EPSF_GRAV_LEFT,        "left" },
    { EPSF_GRAV_CENTER,      "center" },
    { EPSF_GRAV_RIGHT,       "right" },
    { EPSF_GRAV_BOTTOMLEFT,  "bottom-left" },
    { EPSF_GRAV_BOTTOM,      "bottom" },
    { EPSF_GRAV_BOTTOMRIGHT, "bottom-right" },

    { EPSF_GRAV_TOPLEFT,     "tl" },
    { EPSF_GRAV_TOP,         "tc" },
    { EPSF_GRAV_TOPRIGHT,    "tr" },
    { EPSF_GRAV_LEFT,        "cl" },
    { EPSF_GRAV_CENTER,      "cc" },
    { EPSF_GRAV_RIGHT,       "cr" },
    { EPSF_GRAV_BOTTOMLEFT,  "bl" },
    { EPSF_GRAV_BOTTOM,      "bc" },
    { EPSF_GRAV_BOTTOMRIGHT, "br" },

    { EPSF_GRAV_TOP,         "t" },
    { EPSF_GRAV_LEFT,        "l" },
    { EPSF_GRAV_CENTER,      "c" },
    { EPSF_GRAV_RIGHT,       "r" },
    { EPSF_GRAV_BOTTOM,      "b" },

    { EPSF_GRAV_UNKNOWN,   NULL }
};

const struct _num_name _epsf_nn_ori[] = {
    { EPSF_ORI_PORTRAIT,  "portrait" },
    { EPSF_ORI_LANDSCAPE, "landscape" },
    { EPSF_ORI_UPDOWN,    "upside-down" },
    { EPSF_ORI_SEASCAPE,  "seascape" },

    { EPSF_ORI_AUTO,      "auto" },

    { EPSF_ORI_UNKNOWN,   NULL }
};

/*
  In the following arrays, the first entry of each num must be the
  name of the corresponding PostScript filters.
*/

const struct _num_name _epsf_nn_asc[] = {
    { EPSF_ASC_HEX, "ASCIIHex" },
    { EPSF_ASC_85,  "ASCII85" },

    { EPSF_ASC_HEX, "hex" },
    { EPSF_ASC_85,  "85" },

    { EPSF_ASC_UNKNOWN, NULL }
};



static void _calculate_bbox(epsf *ep);
static double _calculate_scale(int, int, int, int);
static int _dimen_get(const char *);
static int _dimen_parse(const char *, int *);
static int _dimen_parse_pair(const char *, int *, int *);
static int _dimen_scale(int, double);
static int _fit_depth(int depth);
static int _write_image_dict(epsf *ep);
static int _write_image_l1(epsf *ep);
static int _write_image_matrix(epsf *ep);
static void _write_l1_datasrc(epsf *ep);
static int _write_palette_array(epsf *ep);



int
epsf_asc_langlevel(epsf_ascii asc)
{
    switch (asc) {
    case EPSF_ASC_HEX:
	return 1;
	
    case EPSF_ASC_85:
	return 2;
	
    default:
	return 0;
    }
}



int
epsf_calculate_parameters(epsf *ep)
{
    int level, level2;

    /* size */

    if (ep->i.width == 0)
	ep->i.width = ep->im->i.width;
    if (ep->i.height == 0)
	ep->i.height = ep->im->i.height;


    /* determine color space */

    if (ep->i.cspace.type == IMAGE_CS_UNKNOWN) {
	if (ep->level == 1 && ep->im->i.cspace.type == IMAGE_CS_INDEXED) {
	    /* no /Indexed in LanguageLevel 1 */
	    ep->i.cspace.type = ep->im->i.cspace.base_type;
	    if (ep->i.cspace.depth == 0)
		ep->i.cspace.depth = ep->im->i.cspace.base_depth;
	}
	else
	    ep->i.cspace.type = ep->im->i.cspace.type;
    }
    if (ep->i.cspace.type == IMAGE_CS_INDEXED) {
	ep->i.cspace.base_depth = 8;
	ep->i.cspace.base_type = ep->im->i.cspace.base_type;
    }
    else if (ep->i.cspace.type == IMAGE_CS_GRAY) {
	if (ep->level != 1 && ep->im->i.cspace.type == IMAGE_CS_INDEXED
	    && ep->im->i.cspace.depth < 8) {
	    /* select indexed grayscale if we can and it saves space */
	    ep->i.cspace.type = IMAGE_CS_INDEXED;
	    ep->i.cspace.base_type = IMAGE_CS_GRAY;
	    ep->i.cspace.base_depth = 8;
	}
    }

    /* check depth */

    if (ep->i.cspace.depth == 0) {
	if (ep->im->i.cspace.type == IMAGE_CS_INDEXED
	    && ep->i.cspace.type != IMAGE_CS_INDEXED)
	    ep->i.cspace.depth = _fit_depth(ep->im->i.cspace.base_depth);
	else
	    ep->i.cspace.depth = _fit_depth(ep->im->i.cspace.depth);

	if (ep->level == 1 && ep->i.cspace.depth == 12)
	    ep->i.cspace.depth = 8;
    }

    level = epsf_cspace_langlevel(&ep->i.cspace);
    if (ep->level && ep->level < level)
	throwf(EOPNOTSUPP,
	       "color space %s at depth %d not supported in LanguageLevel %d",
	       epsf_cspace_name(ep->i.cspace.type), ep->i.cspace.depth,
	       ep->level);


    /* transparency */

    ep->i.cspace.transparency = IMAGE_TR_NONE;


    /* determine level needed by ascii encoding */

    level2 = epsf_asc_langlevel(ep->ascii);
    if (ep->level && ep->level < level2)
	throwf(1, "encoding %s not supported in LanguageLevel %d",
	       epsf_asc_name(ep->ascii), ep->level);
    
    level = level2 > level ? level2 : level;

    
    /* set up image conversion */

    ep->im = image_convert(ep->im, image_info_mask(&ep->i), &ep->i);


    /* determine compression method */

    if (ep->i.compression == IMAGE_CMP_UNKNOWN) {
	/* if we can do raw copy, do so; if original was DCT, use that */
	if (ep->im->i.compression != IMAGE_CMP_NONE)
	    ep->i.compression = ep->im->i.compression;
	else if (ep->im->oi.compression == IMAGE_CMP_DCT)
	    ep->i.compression = ep->im->oi.compression;

	/* don't use methods unavailable at forced LanguageLevel */
	switch (ep->level) {
	case 1:
	    /* no compression in LanguageLevel 1 */
	    ep->i.compression = IMAGE_CMP_NONE;
	    break;
	case 2:
	    /* no /FlateDecode in LanguageLevel 2 */
	    if (ep->i.compression == IMAGE_CMP_FLATE)
		ep->i.compression = DEFAULT_L2_CMP;
	    break;
	}

	/* use best compression available */
	if (ep->i.compression == IMAGE_CMP_UNKNOWN)
	    switch (ep->level ? ep->level : level) {
	    case 1:
		ep->i.compression = IMAGE_CMP_NONE;
		break;
	    case 2:
		ep->i.compression = DEFAULT_L2_CMP;
		break;
	    case 3:
		ep->i.compression = DEFAULT_L3_CMP;
		break;
	    }
    }

    level2 = epsf_compression_langlevel(ep->i.compression);
    if (ep->level && ep->level < level2)
	throwf(EOPNOTSUPP,
	       "compression method %s not supported in LanguageLevel %d",
	       epsf_compression_name(ep->i.compression), ep->level);

    level = level2 > level ? level2 : level;
    
    /* check whether direct copy is possible */

    if (ep->i.compression == IMAGE_CMP_NONE
	|| ep->i.compression != ep->im->i.compression) {
	ep->flags &= ~EPSF_FLAG_DIRECT_COPY;
    }

    /* use best available encoding method */

    if (ep->ascii == EPSF_ASC_UNKNOWN) {
	ep->ascii = ((ep->level ? ep->level : level) > 1)
	    ? EPSF_ASC_85 : EPSF_ASC_HEX;
	level2 = epsf_asc_langlevel(ep->ascii);
	level = level2 > level ? level2 : level;
    }

    /* set required LanguageLevel */
	  
    ep->level = level;
    
    
    /* handle inverted images */

    if (ep->im->i.cspace.inverted == IMAGE_INV_BRIGHTLOW
	&& ((ep->level ? ep->level : level) == 1
	    || ep->i.cspace.type == IMAGE_CS_INDEXED)) {
	/* inversion doesn't work in language level 1 or for indexd */
	ep->i.cspace.inverted = IMAGE_INV_DARKLOW;
	ep->im = image_convert(ep->im, image_info_mask(&ep->i), &ep->i);
    }
    else
	ep->i.cspace.inverted = ep->im->i.cspace.inverted;


    /* calculate image placement and bounding box */

    _calculate_bbox(ep);

    return 0;
}



int
epsf_compression_langlevel(image_compression cmp)
{
    switch (cmp) {
    case IMAGE_CMP_NONE:
	return 1;
	
    case IMAGE_CMP_RLE:
    case IMAGE_CMP_LZW:
    case IMAGE_CMP_CCITT:
    case IMAGE_CMP_DCT:
	return 2;
	
    case IMAGE_CMP_FLATE:
	return 3;

    default:
	return 0;
    }
}



epsf *
epsf_create(const epsf *par, stream *st, image *im)
{
    epsf *ep;

    ep = xmalloc(sizeof(*ep));

    ep->st = st;
    ep->im = im;
    ep->bbox.llx = ep->bbox.lly = ep->bbox.urx = ep->bbox.ury = 0;

    ep->ascii = par->ascii;
    ep->placement = par->placement;
    ep->level = par->level;
    ep->i = par->i;
    ep->flags = par->flags;

    return ep;
}



epsf *
epsf_create_defaults(void)
{
    epsf *ep;

    ep = xmalloc(sizeof(*ep));

    epsf_set_paper(ep, DEFAULT_PAPER);
    epsf_set_margins(ep, DEFAULT_MARGIN, EPSF_MARG_ALL);
    epsf_set_image_size(ep, DEFAULT_IMAGE_SIZE, EPSF_SIZE_BOTH);
    epsf_set_resolution(ep, DEFAULT_RESOLUTION);
    epsf_set_orientation(ep, DEFAULT_ORIENTATION);
    epsf_set_gravity(ep, DEFAULT_GRAVITY);

    ep->ascii = EPSF_ASC_UNKNOWN;
    ep->level = 0;
    image_init_info(&ep->i);
    ep->flags = DEFAULT_FLAGS;

    return ep;
}



int
epsf_cspace_langlevel(const image_cspace *cs)
{
    int lcs, ld;
    
    switch (cs->type) {
    case IMAGE_CS_UNKNOWN:
    case IMAGE_CS_GRAY:
    case IMAGE_CS_RGB:
    case IMAGE_CS_CMYK:
	lcs = 1;
	break;
	
    case IMAGE_CS_INDEXED:
	lcs = 2;
	break;

    default:
	return 0;
    }

    switch (cs->depth) {
    case 1:
    case 2:
    case 4:
    case 8:
	ld = 1;
	break;

    case 12:
	ld = 2;
	break;

    default:
	return 0;
    }

    return (lcs>ld ? lcs : ld);
}



void
epsf_free(epsf *ep)
{
    free(ep);
}



void
epsf_print_parameters(const epsf *ep)
{
    printf("%s:\n", ep->im->fname);
    printf("   image: %s\n",
	   image_info_print(&ep->im->oi));
    printf("    EPSF: %s, %s, level %d%s\n",
	   image_info_print(&ep->i),
	   epsf_asc_name(ep->ascii),
	   ep->level,
	   (ep->flags & EPSF_FLAG_DIRECT_COPY ? ", direct copy" : ""));
    
    printf("    BBox: %d %d %d %d\n",
	   ep->bbox.llx, ep->bbox.lly,
	   ep->bbox.urx, ep->bbox.ury);
}



int
epsf_process(stream *st, const char *fname, const epsf *par)
{
    epsf *ep;
    image *im;
    exception ex;

    im = NULL;
    ep = NULL;

    if (catch(&ex) == 0) {
	im = image_open(fname);
	ep = epsf_create(par, st, im);

	epsf_calculate_parameters(ep);
	if (ep->flags & EPSF_FLAG_VERBOSE)
	    epsf_print_parameters(ep);
	if (st) {
	    epsf_write_header(ep);
	    epsf_write_setup(ep);
	    epsf_write_data(ep);
	    epsf_write_trailer(ep);
	}
	drop();
    }

    if (ep)
	epsf_free(ep);
    if (im)
	image_close(im);

    if (ex.code)
	throw(&ex);

    return 0;
}



int
epsf_set_gravity(epsf *ep, const char *g)
{
    int i;
    double x, y;
    char *p, *q;

    i = epsf_grav_num(g);

    if (i >= 0) {
	ep->placement.gravity_x = gravity[i].x;
	ep->placement.gravity_y = gravity[i].y;
	return 0;
    }

    x = strtod(g, &p);
    if (*p == '\0' || strchr(PAIR_SEPARATORS, *p) == NULL)
	return -1;
    y = strtod(p+1, &q);

    if (x < 0 || x > 1 || y < 0 || y > 1 || (q && *q) || p==g || q==p+1)
	return -1;

    ep->placement.gravity_x = x;
    ep->placement.gravity_y = y;

    return 0;
}



int
epsf_set_image_size(epsf *ep, const char *is, int which)
{
    if (which == EPSF_SIZE_BOTH)
	return _dimen_parse_pair(is,
				 &ep->placement.image_width,
				 &ep->placement.image_height);
    else
	return _dimen_parse(is,
			    (which & EPSF_SIZE_WIDTH
			     ? &ep->placement.image_width
			     : &ep->placement.image_height));
}



int
epsf_set_margins(epsf *ep, const char *m, int which)
{
    int i;

    if (_dimen_parse(m, &i) < 0)
	return -1;

    if (which & EPSF_MARG_TOP)
	ep->placement.top_margin = i;
    if (which & EPSF_MARG_LEFT)
	ep->placement.left_margin = i;
    if (which & EPSF_MARG_RIGHT)
	ep->placement.right_margin = i;
    if (which & EPSF_MARG_BOTTOM)
	ep->placement.bottom_margin = i;

    return 0;
}



int
epsf_set_orientation(epsf *ep, const char *orientation)
{
    int i;
    
    i = epsf_ori_num(orientation);

    if (i >= 0) {
	ep->placement.orientation = i;
	return 0;
    }

    return -1;
}



int
epsf_set_paper(epsf *ep, const char *paper)
{
    int i;
    
    for (i=0; papersize[i].name; i++) {
	if (strcasecmp(paper, papersize[i].name) == 0) {
	    ep->placement.paper_width = papersize[i].width;
	    ep->placement.paper_height = papersize[i].height;
	    return 0;
	}
    }

    return _dimen_parse_pair(paper,
			     &ep->placement.paper_width,
			     &ep->placement.paper_height);
}



int
epsf_set_resolution(epsf *ep, const char *r)
{
    int x, y;
    char *p, *q;

    x = strtoul(r, &p, 10);
    if (p == r)
	return -1;
    if (*p == '\0')
	y = x;
    else if (strchr(PAIR_SEPARATORS, *p) == NULL)
	return -1;
    else {
	y = strtoul(p+1, &q, 10);
	if ((q && *q) || q==p+1)
	    return -1;
    }

    ep->placement.resolution_x = x;
    ep->placement.resolution_y = y;

    return 0;
}



int
epsf_write_data(epsf *ep)
{
    int i, n;
    char *b;
    stream *st, *st2;
    volatile int imread_open;
    exception ex, ex2, *exp;

    st = st2 = NULL;
    imread_open = 0;
    if (catch(&ex) == 0) {
	st2 = stream_ascii_open(ep->st, ep->ascii, ep->level > 1);

	if (ep->flags & EPSF_FLAG_DIRECT_COPY) {
	    image_raw_read_start(ep->im);
	    imread_open = 1;

	    while ((n=image_raw_read(ep->im, &b)) > 0)
		stream_write(st2, b, n);
	    
	    imread_open = 0;
	    image_raw_read_finish(ep->im, 0);
	}
	else {
	    st = stream_compression_open(st2, ep->i.compression, &ep->im->i);

	    image_read_start(ep->im);
	    imread_open = 1;
	    
	    n = image_get_row_size(ep->im);
	    for (i=0; i<ep->im->i.height; i++) {
		image_read(ep->im, &b);
		
		stream_write(st, b, n);
	    }
	    
	    imread_open = 0;
	    image_read_finish(ep->im, 0);
	}
	drop();
    }

    exp = (ex.code ? &ex2 : &ex);
    if (catch(exp) == 0) {
	if (imread_open) {
	    if (ep->flags & EPSF_FLAG_DIRECT_COPY)
		image_raw_read_finish(ep->im, 1);
	    else
		image_read_finish(ep->im, 1);
	}
	if (st && st != st2)
	    stream_close(st);
	if (st2 && st2 != ep->st)
	    stream_close(st2);
	drop();
    }

    if (ex.code)
	throw(&ex);
    
    return 0;
}



int
epsf_write_header(epsf *ep)
{
    static const char *header = "\
%!PS-Adobe-3.0 EPSF-3.0\n\
%%Creator: " PACKAGE " " VERSION "\n\
%%DocumentData: Clean7Bit\n";
    static const char *trailer = "\
%%EndComments\n\
%%BeginProlog\n\
%%EndProlog\n\
%%Page: 1 1\n";

    time_t now;

    stream_puts(header, ep->st);
    stream_printf(ep->st, "%%%%Title: %s\n", ep->im->fname);
    now = time(NULL);
    stream_printf(ep->st, "%%%%CreationDate: %s", ctime(&now));
    stream_printf(ep->st, "%%%%BoundingBox: %d %d %d %d\n",
		  ep->bbox.llx, ep->bbox.lly,
		  ep->bbox.urx, ep->bbox.ury);
    if (ep->level > 1)
	stream_printf(ep->st, "%%%%LanguageLevel: %d\n", ep->level);
    stream_puts(trailer, ep->st);

    return 0;
}



int
epsf_write_setup(epsf *ep)
{
    int x0, y0, xd, yd, r;

    x0 = ep->bbox.llx;
    y0 = ep->bbox.lly;
    xd = ep->bbox.urx - ep->bbox.llx;
    yd = ep->bbox.ury - ep->bbox.lly;
    r = ep->orientation * 90;

    if (ep->orientation == EPSF_ORI_LANDSCAPE
	|| ep->orientation == EPSF_ORI_UPDOWN)
	x0 += xd;
    if (ep->orientation == EPSF_ORI_SEASCAPE
	|| ep->orientation == EPSF_ORI_UPDOWN)
	y0 += yd;

    stream_puts("save 20 dict begin\n", ep->st);
    stream_printf(ep->st, "%d %d translate %d %d scale\n",
		  x0, y0, xd, yd);
    if (r)
	stream_printf(ep->st, "%d rotate\n", r);

    /* XXX: handle other compression methods */
    if (ep->im->i.cspace.type == IMAGE_CS_INDEXED
	|| ep->i.compression != IMAGE_CMP_NONE)
	_write_image_dict(ep);
    else
	_write_image_l1(ep);
    return 0;
}



int
epsf_write_trailer(epsf *ep)
{
    stream_puts("showpage end restore\n", ep->st);
    return stream_puts("%%EOF\n", ep->st);
}



static void
_calculate_bbox(epsf *ep)
{
    int pw, ph, imw, imh;
    double scale, scale_l;

    ep->orientation = ep->placement.orientation;

    if (image_order_swapped(ep->im->i.order)) {
	imw = ep->im->i.height;
	imh = ep->im->i.width;
    }
    else {
	imw = ep->im->i.width;
	imh = ep->im->i.height;
    }
	
    if (ep->placement.paper_width > 0) {
	pw = ep->placement.paper_width
	    - ep->placement.left_margin - ep->placement.right_margin;
	ph = ep->placement.paper_height
	    - ep->placement.top_margin - ep->placement.bottom_margin;
    }
    else
	pw = ph = -1;
	
    if (ep->placement.image_width > 0 || ep->placement.image_height > 0) {
	if (ep->placement.resolution_x > 0)
	    throws(EINVAL, "both image size and resolution specified");

	if (ep->placement.image_width <= 0) {
	    imh *= ep->placement.image_width/(double)imw;
	    imw = ep->placement.image_width;
	}
	else if (ep->placement.image_height <= 0) {
	    imw *= ep->placement.image_height/(double)imh;
	    imh = ep->placement.image_height;
	}
	else {
	    imw = ep->placement.image_width;
	    imh = ep->placement.image_height;
	}
    }
    else {
	if (ep->placement.resolution_x >= 0) {
	    imw *= 72.0/(double)ep->placement.resolution_x;
	    imh *= 72.0/(double)ep->placement.resolution_y;
	}
	else {
	    if (pw < 0)
		throws(EINVAL, "neither paper size, image size, nor "
		       "resolution specified");

	    scale = _calculate_scale(pw, ph, imw, imh);
	    scale_l = _calculate_scale(pw, ph, imh, imw);
	    
	    if (ep->orientation == EPSF_ORI_AUTO) {
		if (scale_l > scale) {
		    scale = scale_l;
		    ep->orientation = EPSF_ORI_LANDSCAPE;
		}
		else
		    ep->orientation = EPSF_ORI_PORTRAIT;
	    }

	    imw *= scale;
	    imh *= scale;
	}
    }
    
    if (ep->orientation == EPSF_ORI_AUTO) {
	if ((imw > ph || imh > ph) && (imw <= ph && imh <= pw))
	    ep->orientation = EPSF_ORI_LANDSCAPE;
	else 
	    ep->orientation = EPSF_ORI_PORTRAIT;
    }

    if (ep->orientation == EPSF_ORI_LANDSCAPE
	|| ep->orientation == EPSF_ORI_SEASCAPE) {
	int t;
	t = imw;
	imw = imh;
	imh = t;
    }

    if (pw < 0)
	ep->bbox.llx = ep->bbox.llx = 0;
    else {
	ep->bbox.llx = ep->placement.left_margin;
	ep->bbox.lly = ep->placement.bottom_margin;

	if (imw > pw || imh > ph) {
	    /* XXX: warning: image bigger than printable area */
	}
	ep->bbox.llx += (pw-imw) * ep->placement.gravity_x;
	ep->bbox.lly += (ph-imh) * ep->placement.gravity_y;
    }

    ep->bbox.urx = ep->bbox.llx + imw;
    ep->bbox.ury = ep->bbox.lly + imh;
}



static double
_calculate_scale(int pw, int ph, int imw, int imh)
{
    double sw, sh;

    sw = pw/(double)imw;
    sh = ph/(double)imh;

    return (sw < sh ? sw : sh);
}



static int
_dimen_get(const char *s)
{
    int i;

    if (s == NULL || *s == '\0')
	s = "pt";

    for (i=0; dimen[i].name; i++) {
	if (strcasecmp(dimen[i].name, s) == 0)
	    return i;
    }
    return -1;
}



static int
_dimen_parse(const char *d, int *ip)
{
    int dim;
    double f;
    char *p;

    f = strtod(d, &p);

    if ((dim=_dimen_get(p)) < 0)
	return -1;

    *ip = _dimen_scale(dim, f);

    return 0;
}



static int
_dimen_parse_pair(const char *d, int *wp, int *hp)
{
    double fw, fh;
    char *p;
    int dim;

    fw = strtod(d, &p);
    if (*p == '\0' || strchr(PAIR_SEPARATORS, *p) == NULL)
	return -1;
    fh = strtod(p+1, &p);

    if ((dim=_dimen_get(p)) < 0)
	return -1;

    *wp = _dimen_scale(dim, fw);
    *hp = _dimen_scale(dim, fh);

    return 0;
}



static int
_dimen_scale(int dim, double f)
{
    return (int)(f*dimen[dim].nom)/dimen[dim].denom;
}



static int
_fit_depth(int depth)
{
    switch (depth) {
    case 1:
	return 1;
    case 2:
	return 2;
    case 3:
    case 4:
	return 4;
    case 5:
    case 6:
    case 7:
    case 8:
	return 8;
    default:
	return 12;
    }
}



static int
_write_image_dict(epsf *ep)
{
    image_info *i;
    int j;

    i = &ep->im->i;

    if (i->cspace.type == IMAGE_CS_INDEXED) {
	stream_printf(ep->st, "[/Indexed /%s %d ", 
		      epsf_cspace_name(i->cspace.base_type),
		      (1<<i->cspace.depth) - 1);
	_write_palette_array(ep);
	stream_puts("] setcolorspace\n", ep->st);
    }
    else
	stream_printf(ep->st, "/%s setcolorspace\n",
		      epsf_cspace_name(i->cspace.type));

    stream_printf(ep->st, "<<\n\
/ImageType 1\n\
/Width %d\n\
/Height %d\n\
/BitsPerComponent %d\n\
/DataSource currentfile /%sDecode filter",
		  i->width, i->height, i->cspace.depth,
		  epsf_asc_name(ep->ascii));
    /* XXX: handle other compression methods */
    if (ep->i.compression != IMAGE_CMP_NONE)
	stream_printf(ep->st, " /%sDecode filter",
		      epsf_compression_name(ep->i.compression));
    stream_puts("\n/ImageMatrix \n", ep->st);
    _write_image_matrix(ep);
    stream_puts("\n/Decode [ ", ep->st);
    if (i->cspace.type == IMAGE_CS_INDEXED)
	stream_printf(ep->st, "0 %d ", (1<<i->cspace.depth)-1);
    else
	for (j=0; j<image_cspace_components(&i->cspace, 0); j++)
	    stream_puts((i->cspace.inverted == IMAGE_INV_DARKLOW
			 ? "0 1 " : "1 0 "), ep->st);
    stream_puts("]\n>> image\n", ep->st);

    return 0;
}



static int
_write_image_l1(epsf *ep)
{
    stream_printf(ep->st, "%d %d %d\n",
		  ep->im->i.width,
		  ep->im->i.height,
		  ep->im->i.cspace.depth);
    _write_image_matrix(ep);
    stream_puts("\n", ep->st);
    if (ep->level == 1) {
	_write_l1_datasrc(ep);
    }
    else {
	stream_printf(ep->st, "currentfile /%sDecode filter\n",
		      epsf_asc_name(ep->ascii));
	if (ep->im->i.cspace.type == IMAGE_CS_GRAY)
	    stream_puts("image\n", ep->st);
	else
	    stream_printf(ep->st, "false %d colorimage\n",
			  image_cspace_components(&ep->im->i.cspace, 0));
    }

    return 0;
}



static int
_write_image_matrix(epsf *ep)
{
    /* keep in sync with enum image_order in image.h */
    static const char *matrix[] = {
	NULL, "w00H0h", "w00h00", "W00Hwh", "W00hw0",
	"0Hw00h", "0hw000", "0HW0wh", "0hW0w0"
    };

    char b[128], *s;
    const char *p;
    int order;

    order = ep->im->i.order;

    if (order < 0 || order >= sizeof(matrix)/sizeof(matrix[0])
	|| matrix[order] == NULL)
	throws(EINVAL, "unsupported pixel order");

    strcpy(b, "[");
    s = b+1;
    for (p=matrix[order]; *p; p++) {
	switch (*p) {
	case 'w':
	    sprintf(s, " %d", ep->im->i.width);
	    break;
	case 'W':
	    sprintf(s, " %d", -ep->im->i.width);
	    break;
	case 'h':
	    sprintf(s, " %d", ep->im->i.height);
	    break;
	case 'H':
	    sprintf(s, " %d", -ep->im->i.height);
	    break;
	default:
	    strcpy(s, " 0");
	}
	s += strlen(s);
    }
    strcpy(s, " ]");

    return stream_puts(b, ep->st);
}



static void
_write_l1_datasrc(epsf *ep)
{
    int nc, ng;

    nc = image_get_row_size(ep->im);
    ng = (ep->im->i.width * ep->im->i.cspace.depth + 7) / 8;
    
    stream_printf(ep->st, "/picstr %d string def\n", nc);

    switch (ep->im->i.cspace.type) {
    case IMAGE_CS_RGB:
	if (ep->im->i.cspace.depth == 8) {
	    stream_printf(ep->st, "\
/colorimage where {\n\
 pop { currentfile picstr readhexstring pop } bind false %d colorimage\n\
} {\n\
 /gstr %d string def\n\
 { currentfile picstr readhexstring pop 0 1 %d \n\
  { gstr exch dup 3 mul dup picstr exch get 27 mul\n\
    picstr 2 index 1 add get 91 mul add\n\
    picstr 2 index 2 add get 10 mul add\n\
    128 idiv exch pop put\n\
  } for gstr\n\
 } bind image\n\
} ifelse\n",
			  image_cspace_components(&ep->im->i.cspace, 0),
			  ng, ng-1);
	}
	return;

    case IMAGE_CS_GRAY:
	stream_puts("{ currentfile picstr readhexstring pop } bind image\n",
		    ep->st);
	return;

    default:
	break;
    }
    
    /* no emulation for this mode yet */
    stream_printf(ep->st, "\
{ currentfile picstr readhexstring pop } bind\n\
false %d colorimage\n",
		  image_cspace_components(&ep->im->i.cspace, 0));
}



static int
_write_palette_array(epsf *ep)
{
    stream *st;
    char *pal;
    int ret;

    if ((pal=image_get_palette(ep->im)) == NULL)
	return -1;

    if (ep->ascii == EPSF_ASC_HEX)
	stream_puts("<\n", ep->st);
    else
	stream_puts("<~\n", ep->st);

    st = stream_ascii_open(ep->st, ep->ascii, 1);
    
    ret = stream_write(st, pal, image_cspace_palette_size(&ep->im->i.cspace));

    stream_close(st);
    return ret;
}
