/*
  $NiH$

  util.c -- utility functions
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <string.h>

#include "util.h"



int
name2num(const struct _num_name *t, const char *n)
{
    int i;

    for (i=0; t[i].name; i++)
	if (strcasecmp(t[i].name, n) == 0)
	    break;

    return t[i].num;
}



const char *
num2name(const struct _num_name *t, int n, int which)
{
    const char *name;
    int i;

    name = NULL;
    
    for (i=0; t[i].name; i++)
	if (t[i].num == n) {
	    name = t[i].name;
	    if (--which <= 0)
		break;
	}

    return name;
}
