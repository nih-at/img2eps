#ifndef _HAD_STREAM_H
#define _HAD_STREAM_H

/*
  $NiH: stream.h,v 1.5 2002/10/12 00:02:13 dillo Exp $

  stream.h -- stream header
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
stream *_stream_create(struct stream_functions *, size_t, stream *);
void stream_free(stream *);

/* external interface */

stream *stream_ascii_open(stream *, int, int);
stream *stream_compression_open(stream *, int, void *);
int stream_printf(stream *, const char *, ...);
int stream_putc(int, stream *);
int stream_puts(const char *, stream *);

#define stream_close(st)	(((stream *)(st))->f->close(st))
#define stream_write(st, b, n)	(((stream *)(st))->f->write((st), (b), (n)))

#endif /* stream.h */
