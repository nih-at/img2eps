/*
  $NiH: epsf.c,v 1.9 2002/09/12 13:52:14 dillo Exp $

  epsf.c -- EPS file fragments
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
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

#define DEFAULT_PAPER	"a4"
#define DEFAULT_MARGIN	20

#ifdef USE_LZW_COMPRESS
#define DEFAULT_L2_CMP	IMAGE_CMP_LZW
#else
#define DEFAULT_L2_CMP	IMAGE_CMP_RLE
#endif
#define DEFAULT_L3_CMP	IMAGE_CMP_FLATE

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
    { "cm",  7200,  254 },
    { "in",    72,    1 },
    { "mm", 72000, 2540 },
    { "pt",     1,    1 },
    { NULL, 0, 0 }
};



/*
  In the following arrays, the first entry of each num must be the
  name of the corresponding PostScript filters.
*/

const struct _epsf_nn _epsf_nn_asc[] = {
    { EPSF_ASC_HEX, "ASCIIHex" },
    { EPSF_ASC_85,  "ASCII85" },

    { EPSF_ASC_HEX, "hex" },
    { EPSF_ASC_85,  "85" },

    { EPSF_ASC_UNKNOWN, NULL }
};

const struct _epsf_nn _epsf_nn_cspace[] = {
    { IMAGE_CS_GRAY,         "DeviceGray" },
    { IMAGE_CS_RGB,          "DeviceRGB" },
    { IMAGE_CS_CMYK,         "DeviceCMYK" },
    { IMAGE_CS_INDEXED,      "Indexed" },

    { IMAGE_CS_GRAY,         "gray" },
    { IMAGE_CS_GRAY,         "grey" },
    { IMAGE_CS_RGB,          "rgb" },
    { IMAGE_CS_CMYK,         "cmyk" },
    { IMAGE_CS_HSV,          "hsv" },

    { IMAGE_CS_UNKNOWN, NULL }
};

const struct _epsf_nn _epsf_nn_compression[] = {
    { IMAGE_CMP_NONE,        "none" },
    { IMAGE_CMP_RLE,         "RunLength" },
    { IMAGE_CMP_LZW,         "LZW" },
    { IMAGE_CMP_FLATE,       "Flate" },
    { IMAGE_CMP_CCITT,       "CCITTFax" },
    { IMAGE_CMP_DCT,         "DCT" },

    { IMAGE_CMP_RLE,         "rle" },
    { IMAGE_CMP_LZW,         "gif" },
    { IMAGE_CMP_FLATE,       "zlib" },
    { IMAGE_CMP_FLATE,       "png" },
    { IMAGE_CMP_CCITT,       "ccitt" },
    { IMAGE_CMP_CCITT,       "fax" },
    { IMAGE_CMP_DCT,         "jpeg" },

    { IMAGE_CMP_UNKNOWN, NULL }
};

static void _calculate_bbox(epsf *ep);
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

    /* determine color space */

    if (ep->i.cspace.type == IMAGE_CS_UNKNOWN) {
	/* no /Indexed in LanguageLevel 1 */
	if (ep->level == 1 && ep->im->i.cspace.type == IMAGE_CS_INDEXED)
	    ep->i.cspace.type = ep->im->i.cspace.base_type;
	else
	    ep->i.cspace.type = ep->im->i.cspace.type;
    }

    /* XXX: depth, unsupported color space types */

    level = epsf_cspace_langlevel(&ep->i.cspace);
    if (ep->level && ep->level < level)
	throwf(1, "color space %s not supported in LanguageLevel %d",
	       epsf_cspace_name(ep->i.cspace.type), ep->level);


    /* determine level needed by ascii encoding */

    level2 = epsf_asc_langlevel(ep->ascii);
    if (ep->level && ep->level < level2)
	throwf(1, "encoding %s not supported in LanguageLevel %d",
	       epsf_asc_name(ep->ascii), ep->level);
    
    level = level2 > level ? level2 : level;

    
    /* determine compression method */

    if (ep->i.compression == IMAGE_CMP_UNKNOWN) {
	/* default to method of image */
	ep->i.compression = ep->im->i.compression;

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
	if (ep->i.compression == IMAGE_CMP_NONE)
	    switch (ep->level ? ep->level : level) {
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
	throwf(1, "compression method %s not supported in LanguageLevel %d",
	       epsf_compression_name(ep->i.compression), ep->level);

    level = level2 > level ? level2 : level;
    
    
    /* use best available encoding method */

    if (ep->ascii == EPSF_ASC_UNKNOWN) {
	ep->ascii = ((ep->level ? ep->level : level) > 1)
	    ? EPSF_ASC_85 : EPSF_ASC_HEX;
	level2 = epsf_asc_langlevel(ep->ascii);
	level = level2 > level ? level2 : level;
    }

    /* set required LanguageLevel */
	  
    ep->level = level;
    
    
    /* set up image conversion */

    ep->im = image_convert(ep->im, image_info_mask(&ep->i), &ep->i);

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
    ep->paper_width = par->paper_width;
    ep->paper_height = par->paper_height;
    ep->paper_bbox = par->paper_bbox;
    ep->level = par->level;
    ep->i = par->i;

    return ep;
}



epsf *
epsf_create_defaults(void)
{
    epsf *ep;

    ep = xmalloc(sizeof(*ep));

    ep->ascii = EPSF_ASC_UNKNOWN;
    epsf_set_paper(ep, DEFAULT_PAPER);
    epsf_set_margins(ep,
		     DEFAULT_MARGIN, DEFAULT_MARGIN,
		     DEFAULT_MARGIN, DEFAULT_MARGIN);
    ep->level = 0;
    image_init_info(&ep->i);

    return ep;
}



int
epsf_cspace_langlevel(const image_cspace *cs)
{
    switch (cs->type) {
    case IMAGE_CS_UNKNOWN:
    case IMAGE_CS_GRAY:
    case IMAGE_CS_RGB:
    case IMAGE_CS_CMYK:
	return 1;
	
    case IMAGE_CS_INDEXED:
	return 2;

    default:
	return 0;
    }

    /* XXX depth */
}



void
epsf_free(epsf *ep)
{
    free(ep);
}



int
epsf_parse_dimen(const char *d)
{
    int i, l;
    char *end;

    l = strtol(d, &end, 10);

    if (*end) {
	for (i=0; dimen[i].name; i++) {
	    if (strcasecmp(dimen[i].name, end) == 0)
		return (l*dimen[i].nom)/dimen[i].denom;
	}
	return -1;
    }

    return l;
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
	epsf_write_header(ep);
	epsf_write_setup(ep);
	epsf_write_data(ep);
	epsf_write_trailer(ep);
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



void
epsf_set_margins(epsf *ep, int l, int r, int t, int b)
{
    if (l >= 0)
	ep->paper_bbox.llx = l;
    if (r >= 0)
	ep->paper_bbox.urx = ep->paper_width-r;
    if (b >= 0)
	ep->paper_bbox.lly = b;
    if (t >= 0)
	ep->paper_bbox.ury = ep->paper_height-t;
}



int
epsf_set_paper(epsf *ep, const char *paper)
{
    int i, pw, ph;
    
    pw = -1;
    for (i=0; papersize[i].name; i++) {
	if (strcasecmp(paper, papersize[i].name) == 0) {
	    pw = papersize[i].width;
	    ph = papersize[i].height;
	    break;
	}
    }
    if (pw == -1)
	return -1;

    ep->paper_bbox.urx += pw-ep->paper_width;
    ep->paper_bbox.ury += ph-ep->paper_height;

    ep->paper_width = pw;
    ep->paper_height = ph;

    return 0;
}



int
epsf_write_data(epsf *ep)
{
    int i, n, raw;
    char *b;
    stream *st, *st2;
    volatile int imread_open;
    exception ex, ex2, *exp;

    raw = (ep->i.compression != IMAGE_CMP_NONE
	   && ep->i.compression == ep->im->i.compression);

    st = st2 = NULL;
    imread_open = 0;
    if (catch(&ex) == 0) {
	st2 = stream_ascii_open(ep->st, ep->ascii, ep->level > 1);

	if (raw) {
	    image_raw_read_start(ep->im);
	    imread_open = 1;

	    while ((n=image_raw_read(ep->im, &b)) > 0)
		stream_write(st2, b, n);
	    
	    imread_open = 0;
	    image_raw_read_finish(ep->im, 0);
	}
	else {
	    st = stream_compression_open(st2, ep->i.compression, NULL);

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
	    if (raw)
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
    stream_puts("save 20 dict begin\n", ep->st);
    stream_printf(ep->st, "%d %d translate %d %d scale\n",
		  ep->bbox.llx, ep->bbox.lly,
		  ep->bbox.urx - ep->bbox.llx,
		  ep->bbox.ury - ep->bbox.lly);

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



int
_epsf_name_num(const struct _epsf_nn *t, const char *n)
{
    int i;

    for (i=0; t[i].name; i++)
	if (strcasecmp(t[i].name, n) == 0)
	    break;

    return t[i].num;
}



char *
_epsf_num_name(const struct _epsf_nn *t, int n)
{
    int i;

    for (i=0; t[i].name; i++)
	if (t[i].num == n)
	    break;

    return t[i].name;
}



static void
_calculate_bbox(epsf *ep)
{
    int pw, ph, imw, imh;
    double scale, sh;
    
    pw = ep->paper_bbox.urx - ep->paper_bbox.llx; 
    ph = ep->paper_bbox.ury - ep->paper_bbox.lly; 

    imw = ep->im->i.width;
    imh = ep->im->i.height;

    scale = pw/(double)imw;
    sh = ph/(double)imh;
    if (sh < scale)
	scale = sh;

    imw *= scale;
    imh *= scale;

    ep->bbox.llx = (pw-imw)/2 + ep->paper_bbox.llx;
    ep->bbox.lly = (ph-imh)/2 + ep->paper_bbox.lly;
    ep->bbox.urx = ep->bbox.llx + imw;
    ep->bbox.ury = ep->bbox.lly + imh;
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
	    stream_puts("0 1 ", ep->st);
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
	"0hw00h", "0hW000", "0Hw0wh", "0HW0w0"
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
