/*
  $NiH$

  img2eps.c -- main function
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "epsf.h"
#include "image.h"
#include "stream.h"
#include "stream_types.h"

char *prg;



int
main(int argc, char *argv[])
{
    int i;
    image *im;
    stream *st;
    epsf *ep, *par;

    prg = argv[0];

    if ((st=stream_file_open(stdout)) == NULL) {
	fprintf(stderr, "%s: cannot create stdout stream: %s\n",
		prg, strerror(errno));
	exit(1);
    }

    par = epsf_create_defaults();

    for (i=1; i<argc; i++) {
	if ((im=image_open(argv[i])) == NULL) {
	    fprintf(stderr, "%s: cannot open image `%s': %s\n",
		    prg, argv[i], strerror(errno));
	    exit(1);
	}

	ep = epsf_create(par, st, im);
	epsf_calculate_parameters(ep);
	epsf_write_header(ep);
	epsf_write_setup(ep);
	epsf_write_data(ep);
	epsf_write_trailer(ep);

	epsf_free(ep);
	image_close(im);
    }
    
    exit(0);
}
