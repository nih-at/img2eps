/*
  $NiH$

  exceptions.c -- exception (catch/throw) system
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "exceptions.h"

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
	    /* XXX: handle error */
	    return -1;
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
    stack.next = st->next;
    free(st);
}



void
throw(exception *ex)
{
    stack.next->ex->code = ex->code;
    stack.next->ex->data = ex->data;
    longjmp(stack.next->ex->buf, 1);
}



void
throwf(int code, char *fmt, ...)
{
    exception *ex;
    va_list argp;

    if ((ex=malloc(sizeof(*ex))) == NULL) {
	/* XXX: handle error */
	throw(NULL);
    }

    ex->code = code;
    
    va_start(argp, fmt);
    vasprintf((char **)&ex->data, fmt, argp);
    va_end(argp);

    throw(ex);
}
