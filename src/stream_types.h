#ifndef _HAD_STREAM_TYPES_H
#define _HAD_STREAM_TYPES_H

/*
  $NiH$

  stream_types.h -- specific stream open functions
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <stdio.h>

stream *stream_file_open(FILE *f);
stream *stream_asciihex_open(stream *st, int eodmark);

#endif /* stream_types.h */
