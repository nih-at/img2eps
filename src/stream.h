#ifndef _HAD_STREAM_H
#define _HAD_STREAM_H

/*
  $NiH: stream.h,v 1.2 2002/09/08 21:31:47 dillo Exp $

  stream.h -- stream header
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <stddef.h>

struct stream {
    struct stream_functions *f;	/* method function pointer */
    struct stream *st;		/* underlying stream */
};

typedef struct stream stream;

struct stream_functions {
    int (*close)(void *);
    int (*write)(void *, const char *, int);
};

#define STREAM_DECLARE(name)				\
typedef struct stream_##name stream_##name;		\
int name##_close(stream_##name *);			\
int name##_write(stream_##name *, const char *, int);	\
struct stream_functions name##_functions  = {		\
    (int (*)())name##_close,				\
    (int (*)())name##_write				\
}

#define stream_create(name, st)	((stream_##name *)_stream_create(	\
					&name##_functions,		\
					sizeof(stream_##name),		\
					st))

/* create and initialize stream structure */
stream *_stream_create(struct stream_functions *f, size_t size, stream *st);
void stream_free(stream *st);

/* external interface */

int stream_printf(stream *st, const char *fmt, ...);
int stream_putc(int c, stream *st);
int stream_puts(const char *s, stream *st);

#define stream_close(st)	(((stream *)(st))->f->close(st))
#define stream_write(st, b, n)	(((stream *)(st))->f->write((st), (b), (n)))

#endif /* stream.h */
