/*
  $NiH$

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
#define DEFAULT_MARGIN	50

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

static void calculate_bbox(epsf *ep);
static int write_image_matrix(epsf *ep);



void
epsf_calculate_parameters(epsf *ep)
{
    if (ep->i.width == 0)
	ep->i.width = ep->im->i.width;
    if (ep->i.height == 0)
	ep->i.height = ep->im->i.height;
    if (ep->i.cspace == IMAGE_CS_UNKNOWN)
	ep->i.cspace = ep->im->i.cspace;
    if (ep->i.depth == 0)
	ep->i.depth = ep->im->i.depth;
    if (ep->i.compression == IMAGE_CMP_UNKNOWN)
	ep->i.compression = ep->im->i.compression;

    calculate_bbox(ep);
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

    epsf_set_paper(ep, DEFAULT_PAPER);
    epsf_set_margins(ep,
		     DEFAULT_MARGIN, DEFAULT_MARGIN,
		     DEFAULT_MARGIN, DEFAULT_MARGIN);
    ep->level = 0;
    image_init_info(&ep->i);

    return ep;
}



void
epsf_free(epsf *ep)
{
    free(ep);
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

    if ((st=stream_asciihex_open(ep->st, 0)) == NULL)
	return -1;

    if (image_read_start(ep->im) < 0) {
	stream_close(st);
	return -1;
    }

    n = ep->i.width * image_cspace_components(ep->i.cspace);
    n = ((n*8 + ep->i.depth-1) / ep->i.depth) * ep->i.height;

    while (n > 0) {
	if ((i=image_read(ep->im, &b)) < 0) {
	    image_read_finish(ep->im, 1);
	    stream_close(st);
	    return -1;
	}
	stream_write(st, b, i);
	n -= i;
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
    stream_puts(trailer, ep->st);

    return 0;
}



int
epsf_write_setup(epsf *ep)
{
    int slen;

    slen = ep->i.width * image_cspace_components(ep->i.cspace);
    slen = (slen*8 + ep->i.depth-1) / ep->i.depth;

    stream_puts("save 20 dict begin\n", ep->st);
    stream_printf(ep->st, "/picstr %d string def\n", slen);
    stream_printf(ep->st, "%d %d translate %d %d scale\n",
		  ep->bbox.llx, ep->bbox.lly,
		  ep->bbox.urx - ep->bbox.llx,
		  ep->bbox.ury - ep->bbox.lly);
    stream_printf(ep->st, "%d %d %d\n",
		  ep->i.width,
		  ep->i.height,
		  ep->i.depth);
    write_image_matrix(ep);
    stream_puts("\n{ currentfile picstr readhexstring pop } bind\n"
		"false 3 colorimage\n",
		ep->st);

    return 0;
}



int
epsf_write_trailer(epsf *ep)
{
    stream_puts("showpage end restore\n", ep->st);
    return stream_puts("%%EOF\n", ep->st);
}



static void
calculate_bbox(epsf *ep)
{
    int pw, ph, imw, imh;
    double scale, sh;
    
    pw = ep->paper_bbox.urx - ep->paper_bbox.llx; 
    ph = ep->paper_bbox.ury - ep->paper_bbox.lly; 

    imw = ep->i.width;
    imh = ep->i.height;

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
write_image_matrix(epsf *ep)
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
	    sprintf(s, " %d", ep->i.width);
	    break;
	case 'W':
	    sprintf(s, " %d", -ep->i.width);
	    break;
	case 'h':
	    sprintf(s, " %d", ep->i.height);
	    break;
	case 'H':
	    sprintf(s, " %d", -ep->i.height);
	    break;
	default:
	    strcpy(s, " 0");
	}
	s += strlen(s);
    }
    strcpy(s, " ]");

    return stream_puts(b, ep->st);
}
