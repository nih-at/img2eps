/*
  $NiH: exceptions.c,v 1.2 2002/09/10 14:05:50 dillo Exp $

  exceptions.c -- exception (catch/throw) system
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
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
    vasprintf((char **)s, fmt, argp);
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
