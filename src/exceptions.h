#ifndef HAD_EXCEPTIONS_H
#define HAD_EXCEPTIONS_H

/*
  $NiH: exceptions.h,v 1.4 2002/11/13 01:35:48 dillo Exp $

  exceptions.h -- exceptions (catch/throw) header
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <img2eps@nih.at>

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
 
  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#include <setjmp.h>

struct exception_s {
    int code;
    void *data;
    jmp_buf buf;
};

typedef struct exception_s exception;

#define catch(ex)	(_catch(setjmp((ex)->buf), (ex)))
int _catch(int, exception *);
void drop(void);
void throw(const exception *);
void throwf(int, const char *, ...);
void throws(int, const char *);

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
