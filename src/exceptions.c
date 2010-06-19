/*
  $NiH: exceptions.c,v 1.4 2002/09/10 21:40:47 dillo Exp $

  exceptions.c -- exception (catch/throw) system
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



#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exceptions.h"

#define _throw(ex)	(longjmp((ex)->buf, 1))


struct stack {
    struct stack *next;
    exception *ex;
};

static struct stack stack = { NULL };



int
_catch(int phase, exception *ex)
{
    struct stack *st;

    if (phase == 0) {
	if ((st=malloc(sizeof(*st))) == NULL) {
	    ex->code = ENOMEM;
	    ex->data = "out of memory allocating catch frame";
	    _throw(ex);
	}

	ex->code = 0;
	st->ex = ex;
	st->next = stack.next;
	stack.next = st;

	return 0;
    }
    else {
	st = stack.next;
	ex = st->ex;
	stack.next = st->next;
	free(st);
	return ex->code;
    }
}



void
drop(void)
{
    struct stack *st;

    st = stack.next;

    if (st == NULL) {
	/* XXX: too many drops */
	return;
    }

    stack.next = st->next;
    free(st);
}



void
throw(const exception *ex)
{
    if (stack.next == NULL) {
	/* XXX: provide hook */
	fprintf(stderr, "uncaught exception <%d> (%s)\n",
		ex->code, strerror(ex->code));
	exit(3);
    }

    stack.next->ex->code = ex->code;
    stack.next->ex->data = ex->data;
    _throw(stack.next->ex);
}



void
throwf(int code, const char *fmt, ...)
{
    static exception nomemex = {
	ENOMEM, "out of memory allocating exception message"
    };

    char *s;
    va_list argp;

    va_start(argp, fmt);
    vasprintf(&s, fmt, argp);
    va_end(argp);

    if (s == NULL)
	throw(&nomemex);

    throws(code, s);
}



void
throws(int code, const char *msg)
{
    static exception nomemex = {
	ENOMEM, "out of memory allocating exception"
    };
    
    exception *ex;

    if ((ex=malloc(sizeof(*ex))) == NULL)
	throw(&nomemex);

    ex->code = code;
    ex->data = (void *)msg;

    throw(ex);
}
