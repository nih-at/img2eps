#ifndef _HAD_STREAM_TYPES_H
#define _HAD_STREAM_TYPES_H

/*
  $NiH: stream_types.h,v 1.3 2002/09/08 21:31:47 dillo Exp $

  stream_types.h -- specific stream open functions
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <stdio.h>

#include "image.h"

stream *stream_ascii85_open(stream *st, int eodmark);
stream *stream_asciihex_open(stream *st, int eodmark);
stream *stream_file_fopen(FILE *f, int closep);
stream *stream_file_open(char *fname);
stream *stream_line_open(stream *st);
stream *stream_runlength_open(stream *ost);

#endif /* stream_types.h */
