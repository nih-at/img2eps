#ifndef HAD_EXCEPTIONS_H
#define HAD_EXCEPTIONS_H

/*
  $NiH$

  exceptions.h -- exceptions (catch/throw) header
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <setjmp.h>

struct exception {
    int code;
    void *data;
    jmp_buf buf;
};

typedef struct exception exception;

#define catch(ex)	(_catch(setjmp((ex)->buf), (ex)))
int _catch(int phase, exception *ex);
void drop(void);
void throw(exception *ex);
void throwf(int code, char *fmt, ...);

/*
  example:

f()
{
    exception ex;

    if (catch(&ex) == 0) {
	g();
    }

    if (ex.code) {
        caught_exception();
    }
}

g()
{
    exception ex;

    ex.code = 1;
    ex.data = "foo";

    throw(&ex);
}

*/

#endif /* exceptions.h */
