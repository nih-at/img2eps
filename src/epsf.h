#ifndef HAD_EPSF_H
#define HAD_EPSF_H

/*
  $NiH: epsf.h,v 1.7 2002/10/11 00:53:45 dillo Exp $

  epsf.h -- EPS file fragments
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



#include "image.h"
#include "stream.h"
#include "util.h"

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
    int flags;				/* prossing flags */
};

typedef struct epsf epsf;

#define EPSF_FLAG_VERBOSE	1



extern const struct _num_name _epsf_nn_asc[];

#define epsf_asc_name(a)	(num2name(_epsf_nn_asc, (a), 1))
#define epsf_asc_num(a)		(name2num(_epsf_nn_asc, (a)))

#define epsf_cspace_name(a)	(num2name(_image_nn_cspace, (a), 1))
#define epsf_compression_name(a) (num2name(_image_nn_compression, (a), 1))

int epsf_asc_langlevel(epsf_ascii asc);
int epsf_calculate_parameters(epsf *ep);
int epsf_compression_langlevel(image_compression cmp);
epsf *epsf_create(const epsf *par, stream *st, image *im);
epsf *epsf_create_defaults(void);
int epsf_cspace_langlevel(const image_cspace *cs);
void epsf_free(epsf *ep);
int epsf_parse_dimen(const char *d);
void epsf_print_parameters(const epsf *ep);
int epsf_process(stream *st, const char *fname, const epsf *par);
void epsf_set_margins(epsf *ep, int l, int r, int t, int b);
int epsf_set_paper(epsf *ep, const char *paper);
int epsf_write_data(epsf *ep);
int epsf_write_header(epsf *ep);
int epsf_write_setup(epsf *ep);
int epsf_write_trailer(epsf *ep);

#endif /* epsf.h */

