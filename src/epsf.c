/*
  $NiH: epsf.c,v 1.2 2002/09/08 00:27:48 dillo Exp $

  epsf.c -- EPS file fragments
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <time.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "epsf.h"
#include "stream.h"
#include "stream_types.h"

#define DEFAULT_PAPER	"a4"
#define DEFAULT_MARGIN	20

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

static void _calculate_bbox(epsf *ep);
static int _write_image_dict(epsf *ep);
static int _write_image_l1(epsf *ep);
static int _write_image_matrix(epsf *ep);
static int _write_palette_array(epsf *ep);



char *
epsf_asc_name(epsf_ascii asc)
{
    switch (asc) {
    case EPSF_ASC_HEX:
	return "/ASCIIHEX";
    case EPSF_ASC_85:
	return "/ASCII85";

    default:
	return NULL;
    }
}



int
epsf_calculate_parameters(epsf *ep)
{
    int level;
    image *im;

    if (ep->level == 1) {
	if (IMAGE_CS_IS_INDEXED(ep->im->i.cspace)
	    && ep->i.cspace == IMAGE_CS_UNKNOWN)
	    ep->i.cspace = IMAGE_CS_BASE(ep->im->i.cspace);
    }

    if ((im=image_convert(ep->im, &ep->i)) == NULL)
	return -1;
    ep->im = im;

    level = epsf_cspace_langlevel(ep->im->i.cspace);

    if (ep->level < level) {
	if (ep->level) {
	    /* requested unsupported features */
	    return -1;
	}
	else
	    ep->level = level;
    }

    if (ep->ascii == EPSF_ASC_UNKNOWN)
	ep->ascii = (ep->level > 1) ? EPSF_ASC_85 : EPSF_ASC_HEX;

    _calculate_bbox(ep);

    return 0;
}



epsf *
epsf_create(epsf *par, stream *st, image *im)
{
    epsf *ep;

    if ((ep=malloc(sizeof(*ep))) == NULL)
	return NULL;

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

    if ((ep=malloc(sizeof(*ep))) == NULL)
	return NULL;

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
epsf_cspace_langlevel(image_cspace cs)
{
    switch (cs) {
    case IMAGE_CS_UNKNOWN:
    case IMAGE_CS_GRAY:
    case IMAGE_CS_RGB:
    case IMAGE_CS_CMYK:
	return 1;
	
    case IMAGE_CS_INDEXED_GRAY:
    case IMAGE_CS_INDEXED_RGB:
    case IMAGE_CS_INDEXED_CMYK:
	return 2;

    default:
	return 0;
    }
}



char *
epsf_cspace_name(image_cspace cspace)
{
    switch (cspace) {
    case IMAGE_CS_GRAY:
	return "/DeviceGray";
    case IMAGE_CS_RGB:
	return "/DeviceRGB";
    case IMAGE_CS_CMYK:
	return "/DeviceCMYK";
    case IMAGE_CS_INDEXED_GRAY:
    case IMAGE_CS_INDEXED_RGB:
    case IMAGE_CS_INDEXED_CMYK:
	return "/Indexed";

    default:
    return NULL;
    }
}



void
epsf_free(epsf *ep)
{
    free(ep);
}



int
epsf_process(stream *st, char *fname, epsf *par)
{
    epsf *ep;
    image *im;
    
    if ((im=image_open(fname)) == NULL)
	return -1;

    ep = epsf_create(par, st, im);
    epsf_calculate_parameters(ep);
    epsf_write_header(ep);
    epsf_write_setup(ep);
    epsf_write_data(ep);
    epsf_write_trailer(ep);
    
    epsf_free(ep);
    image_close(im);

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
epsf_set_paper(epsf *ep, char *paper)
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
    int i, n;
    char *b;
    stream *st;

    if (ep->ascii == EPSF_ASC_HEX)
	st = stream_asciihex_open(ep->st, 0);
    else
	st = stream_ascii85_open(ep->st, 1);
    if (st == NULL)
	return -1;

    if (image_read_start(ep->im) < 0) {
	stream_close(st);
	return -1;
    }

    n = image_get_row_size(ep->im);
    for (i=0; i<ep->im->i.height; i++) {
	if (image_read(ep->im, &b) < 0) {
	    image_read_finish(ep->im, 1);
	    stream_close(st);
	    return -1;
	}
	stream_write(st, b, n);
    }

    image_read_finish(ep->im, 0);
    stream_close(st);

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
    if (IMAGE_CS_IS_INDEXED(ep->im->i.cspace))
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

    if (IMAGE_CS_IS_INDEXED(i->cspace)) {
	stream_printf(ep->st, "[/Indexed %s %d <~\n", 
		      epsf_cspace_name(IMAGE_CS_BASE(i->cspace)),
		      (1<<i->depth) - 1);
	_write_palette_array(ep);
	stream_puts("] setcolorspace\n", ep->st);
    }
    else
	stream_printf(ep->st, "%s setcolorspace\n",
		      epsf_cspace_name(i->cspace));

    stream_printf(ep->st, "<<\n\
/ImageType 1\n\
/Width %d\n\
/Height %d\n\
/BitsPerComponent %d\n\
/DataSource currentfile %sDecode filter\n\
/ImageMatrix ",
		  i->width, i->height, i->depth,
		  epsf_asc_name(ep->ascii));
    _write_image_matrix(ep);
    stream_puts("\n/Decode [ ", ep->st);
    if (IMAGE_CS_IS_INDEXED(i->cspace))
	stream_printf(ep->st, "0 %d ", (1<<i->depth)-1);
    else
	for (j=0; j<image_cspace_components(i->cspace); j++)
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
		  ep->im->i.depth);
    _write_image_matrix(ep);
    stream_puts("\n", ep->st);
    if (ep->ascii == EPSF_ASC_HEX) {
	stream_printf(ep->st, "/picstr %d string def\n",
		      image_get_row_size(ep->im));
	stream_puts("{ currentfile picstr readhexstring pop } bind\n",
		    ep->st);
    }
    else
	stream_puts("currentfile /ASCII85Decode filter\n", ep->st);
    if (ep->im->i.cspace == IMAGE_CS_GRAY)
	stream_puts("image\n", ep->st);
    else
	stream_printf(ep->st, "false %d colorimage\n",
		      image_cspace_components(ep->im->i.cspace));

    return 0;
}



static int
_write_image_matrix(epsf *ep)
{
    /* keep in sync with enum image_order in image.h */
    static const char *matrix[] = {
	"w00H0h", "w00h00", "W00Hwh", "W00hw0",
	"0hw00h", "0hW000", "0Hw0wh", "0HW0w0"
    };

    char b[128], *s;
    const char *p;

    strcpy(b, "[");
    s = b+1;
    for (p=matrix[ep->i.order]; *p; p++) {
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



static int
_write_palette_array(epsf *ep)
{
    stream *st;
    int ret;
    char *pal;

    if ((pal=image_get_palette(ep->im)) == NULL)
	return -1;
    
    if ((st=stream_ascii85_open(ep->st, 1)) == NULL)
	ret = -1;
    else
	ret = stream_write(st, pal, image_get_palette_size(ep->im));
			   

    stream_close(st);
    return ret;
}
