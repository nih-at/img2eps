#ifndef HAD_XMALLOC_H
#define HAD_XMALLOC_H

/*
  $NiH: epsf.h,v 1.4 2002/09/09 12:42:32 dillo Exp $

  xmalloc.h -- allocation functions with exceptions
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <stddef.h>

void *xmalloc(size_t size);
char *xstrdup(const char *s);

#endif /* xmalloc.h */
