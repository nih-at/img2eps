/*
  $NiH: img2eps.c,v 1.2 2002/09/08 00:27:49 dillo Exp $

  img2eps.c -- main function
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "epsf.h"
#include "image.h"
#include "stream.h"
#include "stream_types.h"

char *prg;

static char const help_version[] = PACKAGE " " VERSION "\n\
Copyright (C) 2002 Dieter Baron\n\
img2eps comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n";

static const char help_head[] =
PACKAGE " " VERSION " by Dieter Baron <dillo@giga.or.at>\n\n";

static const char help_tail[] = "\
  -h, --help        display this help message\n\
  -V, --version     display version number\n\
  -1                use language level 1\n\
  -2                use language level 2\n\
  -3                use language level 3\n\
      --level L     use language level L\n\
  -c, --stdout      write EPSF to stdout\n\
  -g, --gray        force image to gray scale\n\
  -m, --margin M    set all margins to M\n\
  -o, --output FILE write EPSF to FILE\n\
  -P, --paper P     set paper size to P\n\
\n\
Report bugs to <dillo@giga.or.at>.\n";

static const char usage[] =
"usage: %s [-hV] [-123c] [-m margin] [-o outfile] [-p paper] file [...]\n";


enum {
    OPT_LEVEL = 256
};

#define OPTIONS "hV123cgm:o:p:"

static const struct option options[] = {
    { "help",      0, 0, 'h' },
    { "version",   0, 0, 'V' },
    { "gray",      0, 0, 'g' },
    { "grey",      0, 0, 'g' },
    { "level",     1, 0, OPT_LEVEL },
    { "margin",    1, 0, 'm' },
    { "output",    1, 0, 'o' },
    { "paper",     1, 0, 'P' },
    { "stdout",    0, 0, 'c' },
    { NULL,        0, 0, 0   }
};

static char *extsubst(char *fname, char *newext);



int
main(int argc, char *argv[])
{
    int c, i;
    stream *st;
    epsf *par;
    char *outfile;
    int cat;

    prg = argv[0];

    par = epsf_create_defaults();
    cat = 0;
    outfile = NULL;
    
    opterr = 0;
    while ((c=getopt_long(argc, argv, OPTIONS, options, 0)) != EOF) {
	switch (c) {
	case '1':
	case '2':
	case '3':
	    par->level = c-'0';
	    break;
	case 'c':
	    cat = 1;
	    break;
	case 'g':
	    par->i.cspace = IMAGE_CS_GRAY;
	    break;
	case 'm':
	    i = atoi(optarg);
	    epsf_set_margins(par, i, i, i, i);
	    break;
	case 'o':
	    outfile = optarg;
	    break;
	case 'P':
	    epsf_set_paper(par, optarg);
	    break;
	case OPT_LEVEL:
	    i = atoi(optarg);
	    if (i<1 || i>3) {
		fprintf(stderr, usage, prg);
		exit(1);
	    }
	    par->level = i;
	    break;

	case 'V':
            fputs(help_version, stdout);
            exit(0);
        case 'h':
            fputs(help_head, stdout);
            printf(usage, prg);
            fputs(help_tail, stdout);
            exit(0);
        case '?':
            fprintf(stderr, usage, prg);
            exit(1);
	}
    }

    if (optind == argc) {
	fprintf(stderr, usage, prg);
	exit(1);
    }

    if (cat || outfile) {
	if (optind != argc-1) {
	    fprintf(stderr, "%s: cannot convert multpile files"
		    " with -c or -o\n",
		    prg);
	    exit(1);
	}
	if (cat && outfile) {
	    fprintf(stderr, "%s: cannot use both -c and -o\n",
		    prg);
	    exit(1);
	}

	if (cat) {
	    if ((st=stream_file_fopen(stdout, 0)) == NULL) {
		fprintf(stderr, "%s: cannot create stdout stream: %s\n",
			prg, strerror(errno));
		exit(1);
	    }
	}
	else {
	    if ((st=stream_file_open(outfile)) == NULL) {
		fprintf(stderr, "%s: cannot open `%s': %s\n",
			prg, outfile, strerror(errno));
		exit(1);
	    }
	}
	epsf_process(st, argv[optind], par);
	stream_close(st);
    }
    else {
	for (i=optind; i<argc; i++) {
	    outfile = extsubst(argv[i], "eps");

	    if ((st=stream_file_open(outfile)) == NULL) {
		fprintf(stderr, "%s: cannot open `%s': %s\n",
			prg, outfile, strerror(errno));
		exit(1);
	    }
	    epsf_process(st, argv[i], par);
	    stream_close(st);
	    free(outfile);
	}
    }
    
    exit(0);
}



static char *
extsubst(char *fname, char *newext)
{
    char *p, *q, *s;

    p = strrchr(fname, '/');
    if (p)
	p++;
    else
	p = fname;
    
    q = strrchr(p, '.');
    if (q == NULL)
	q = p+strlen(p);

    if ((s=malloc(q-p + strlen(newext) + 2)) == NULL)
	return NULL;

    strncpy(s, p, q-p);
    q = s+strlen(s);
    *(q++) = '.';
    strcpy(q, newext);

    return s;
}
