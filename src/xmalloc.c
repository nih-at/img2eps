/*
  $NiH: xmalloc.c,v 1.1 2002/09/10 14:05:54 dillo Exp $

  xmalloc.c -- allocation functions with exceptions
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "exceptions.h"
#include "xmalloc.h"



void *
xmalloc(size_t size)
{
    static errstr[128];
    void *p;

    if ((p=malloc(size)) == NULL) {
	sprintf(errstr, "out of memory allocating %ld bytes", (long)size);
	throws(ENOMEM, errstr);
    }

    return p;
}



char *
xstrdup(const char *s)
{
    char *t;

    t = xmalloc(strlen(s)+1);
    strcpy(t, s);

    return t;
}
