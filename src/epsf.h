#ifndef HAD_EPSF_H
#define HAD_EPSF_H

/*
  $NiH: epsf.h,v 1.9 2003/12/14 09:50:42 dillo Exp $

  epsf.h -- EPS file fragments
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



#include "image.h"
#include "stream.h"
#include "util.h"

enum epsf_orientation {
    EPSF_ORI_UNKNOWN = -1,
    EPSF_ORI_PORTRAIT,
    EPSF_ORI_LANDSCAPE,
    EPSF_ORI_UPDOWN,
    EPSF_ORI_SEASCAPE,

    EPSF_ORI_AUTO
};

enum epsf_gravity {
    EPSF_GRAV_UNKNOWN = -1,
    EPSF_GRAV_TOPLEFT,
    EPSF_GRAV_TOP,
    EPSF_GRAV_TOPRIGHT,
    EPSF_GRAV_LEFT,
    EPSF_GRAV_CENTER,
    EPSF_GRAV_RIGHT,
    EPSF_GRAV_BOTTOMLEFT,
    EPSF_GRAV_BOTTOM,
    EPSF_GRAV_BOTTOMRIGHT
};

/* keep in sync with _asc_tab[] in stream.c */
enum epsf_ascii {
    EPSF_ASC_UNKNOWN,
    EPSF_ASC_NONE,
    EPSF_ASC_HEX,
    EPSF_ASC_85
};

typedef enum epsf_ascii epsf_ascii;
typedef enum epsf_gravity epsf_gravity;
typedef enum epsf_orientation epsf_orientation;

struct epsf_bbox {
    int llx, lly, urx, ury;
};

typedef struct epsf_bbox epsf_bbox;

struct epsf_placement {
    int paper_width, paper_height;
    int left_margin, right_margin, top_margin, bottom_margin;
    int resolution;
    epsf_orientation orientation;
    epsf_gravity gravity;
};

typedef struct epsf_placement epsf_placement;

struct epsf {
    stream *st;				/* stream to write EPSF to */
    image *im;				/* image to convert */
    epsf_placement placement;		/* how to place image on page */
    epsf_bbox bbox;			/* bounding box of image */
    epsf_orientation orientation;	/* orientation of image */
    epsf_ascii ascii;			/* desired ascii encoding */
    int level;				/* minimum LanguageLevel to support */
    image_info i;			/* desired image properties */
    int flags;				/* processing flags */
};

typedef struct epsf epsf;

#define EPSF_FLAG_VERBOSE	1



extern const struct _num_name _epsf_nn_asc[];
extern const struct _num_name _epsf_nn_ori[];

#define epsf_asc_name(a)	(num2name(_epsf_nn_asc, (a), 1))
#define epsf_asc_num(a)		(name2num(_epsf_nn_asc, (a)))

#define epsf_grav_name(a)	(num2name(_epsf_nn_grav, (a), 1))
#define epsf_grav_num(a)	(name2num(_epsf_nn_grav, (a)))

#define epsf_ori_name(a)	(num2name(_epsf_nn_ori, (a), 1))
#define epsf_ori_num(a)		(name2num(_epsf_nn_ori, (a)))

#define epsf_cspace_name(a)	(num2name(_image_nn_cspace, (a), 1))
#define epsf_compression_name(a) (num2name(_image_nn_compression, (a), 1))

int epsf_asc_langlevel(epsf_ascii);
int epsf_calculate_parameters(epsf *);
int epsf_compression_langlevel(image_compression);
epsf *epsf_create(const epsf *, stream *, image *);
epsf *epsf_create_defaults(void);
int epsf_cspace_langlevel(const image_cspace *);
void epsf_free(epsf *);
int epsf_parse_dimen(const char *);
void epsf_print_parameters(const epsf *);
int epsf_process(stream *, const char *, const epsf *);
int epsf_set_gravity(epsf *, const char *);
void epsf_set_margins(epsf *, int, int, int, int);
int epsf_set_orientation(epsf *, const char *);
void epsf_set_resolution(epsf *, int);
int epsf_set_paper(epsf *, const char *);
int epsf_write_data(epsf *);
int epsf_write_header(epsf *);
int epsf_write_setup(epsf *);
int epsf_write_trailer(epsf *);

#endif /* epsf.h */

