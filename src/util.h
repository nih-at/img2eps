#ifndef HAD_UTIL_H
#define HAD_UTIL_H

/*
  $NiH$

  util.h -- utility functions
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/



struct _num_name {
    int num;
    const char *name;
};

int name2num(const struct _num_name *t, const char *n);
const char *num2name(const struct _num_name *t, int n, int which);

#endif /* util.h */
