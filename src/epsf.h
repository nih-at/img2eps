#ifndef HAD_EPSF_H
#define HAD_EPSF_H

/*
  $NiH: epsf.h,v 1.2 2002/09/08 00:27:48 dillo Exp $

  epsf.h -- EPS file fragments
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include "stream.h"
#include "image.h"

enum epsf_ascii {
    EPSF_ASC_UNKNOWN,
    EPSF_ASC_HEX,
    EPSF_ASC_85
};

typedef enum epsf_ascii epsf_ascii;

struct epsf_bbox {
    int llx, lly, urx, ury;
};

typedef struct epsf_bbox epsf_bbox;

struct epsf {
    stream *st;				/* stream to write EPSF to */
    image *im;				/* image to convert */
    int paper_width, paper_height;	/* size of paper */
    epsf_bbox paper_bbox;		/* bounding box of printable area */
    epsf_bbox bbox;			/* bounding box of image */
    epsf_ascii ascii;			/* desired ascii encoding */
    int level;				/* minimum LanguageLevel to support */
    image_info i;			/* desired image properties */
};

typedef struct epsf epsf;



int epsf_calculate_parameters(epsf *ep);
epsf *epsf_create(epsf *par, stream *st, image *im);
epsf *epsf_create_defaults(void);
int epsf_cspace_langlevel(image_cspace cs);
void epsf_free(epsf *ep);
int epsf_process(stream *st, char *fname, epsf *par);
void epsf_set_margins(epsf *ep, int l, int r, int t, int b);
int epsf_set_paper(epsf *ep, char *paper);
int epsf_write_data(epsf *ep);
int epsf_write_header(epsf *ep);
int epsf_write_setup(epsf *ep);
int epsf_write_trailer(epsf *ep);

#endif /* epsf.h */

