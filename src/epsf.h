#ifndef HAD_EPSF_H
#define HAD_EPSF_H

/*
  $NiH: epsf.h,v 1.5 2002/09/11 22:44:18 dillo Exp $

  epsf.h -- EPS file fragments
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include "stream.h"
#include "image.h"

/* keep in sync with _asc_tab[] in stream.c */
enum epsf_ascii {
    EPSF_ASC_UNKNOWN,
    EPSF_ASC_NONE,
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

struct _epsf_nn {
    int num;
    char *name;
};

extern const struct _epsf_nn _epsf_nn_asc[];
extern const struct _epsf_nn _epsf_nn_cspace[];
extern const struct _epsf_nn _epsf_nn_compression[];



int _epsf_name_num(const struct _epsf_nn *t, const char *n);
char *_epsf_num_name(const struct _epsf_nn *t, int n);

#define epsf_asc_name(a)	(_epsf_num_name(_epsf_nn_asc, (a)))
#define epsf_cspace_name(a)	(_epsf_num_name(_epsf_nn_cspace, (a)))
#define epsf_compression_name(a)	\
				(_epsf_num_name(_epsf_nn_compression, (a)))
#define epsf_asc_num(a)		(_epsf_name_num(_epsf_nn_asc, (a)))
#define epsf_cspace_num(a)	(_epsf_name_num(_epsf_nn_cspace, (a)))
#define epsf_compression_num(a)	(_epsf_name_num(_epsf_nn_compression, (a)))

int epsf_asc_langlevel(epsf_ascii asc);
int epsf_calculate_parameters(epsf *ep);
int epsf_compression_langlevel(image_compression cmp);
epsf *epsf_create(const epsf *par, stream *st, image *im);
epsf *epsf_create_defaults(void);
int epsf_cspace_langlevel(const image_cspace *cs);
void epsf_free(epsf *ep);
int epsf_parse_dimen(const char *d);
int epsf_process(stream *st, const char *fname, const epsf *par);
void epsf_set_margins(epsf *ep, int l, int r, int t, int b);
int epsf_set_paper(epsf *ep, const char *paper);
int epsf_write_data(epsf *ep);
int epsf_write_header(epsf *ep);
int epsf_write_setup(epsf *ep);
int epsf_write_trailer(epsf *ep);

#endif /* epsf.h */

