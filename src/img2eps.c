/*
  $NiH: img2eps.c,v 1.13 2003/12/14 09:17:38 dillo Exp $

  img2eps.c -- main function
  Copyright (C) 2002, 2005 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>

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
 
  THIS SOFTWARE IS PROVIDED BY DIETER BARON ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL DIETER BARON BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include "getopt_long.h"
#endif
#include "epsf.h"
#include "exceptions.h"
#include "image.h"
#include "stream.h"
#include "stream_types.h"
#include "xmalloc.h"

char *prg;

static char const help_version[] = PACKAGE " " VERSION "\n\
Copyright (C) 2002 Dieter Baron\n\
img2eps comes with ABSOLUTELY NO WARRANTY, to the extent permitted by law.\n";

static const char help_head[] =
PACKAGE " " VERSION " by Dieter Baron <dillo@giga.or.at>\n\n";

static const char help_tail[] = "\
  -h, --help           display this help message\n\
  -V, --version        display version number\n\
  -1                   use language level 1\n\
  -2                   use language level 2\n\
  -3                   use language level 3\n\
  -a, --ascii ENC      use ENC ASCII encoding for binary data\n\
      --level L        use language level L\n\
  -c, --stdout         write EPSF to stdout\n\
  -C, --compress C     use compression method C for image data\n\
  -g, --gray           force image to gray scale\n\
  -G, --gravity G      set gravity to G\n\
  -m, --margin M       set all margins to M\n\
  -o, --output FILE    write EPSF to FILE\n\
  -O, --orientation O  set orientation to O\n\
  -P, --paper P        set paper size to P\n\
  -r, --resolution R   set resolution to R\n\
  -v, --verbose        be verbose\n\
\n\
Report bugs to <dillo@giga.or.at>.\n";

static const char usage[] =
"usage: %s [-hV] [-123cv] [-a asc] [-C comp] [-G grav] [-m marg] [-o file] [-O ori] [-P paper] file [...]\n";


enum {
    OPT_LEVEL = 256
};

#define OPTIONS "hV123a:cC:gG:m:o:O:P:r:v"

static const struct option options[] = {
    { "help",         0, 0, 'h' },
    { "version",      0, 0, 'V' },
    { "ascii",        1, 0, 'a' },
    { "compress",     1, 0, 'C' },
    { "compression",  1, 0, 'C' },
    { "gravity",      1, 0, 'G' },
    { "gray",         0, 0, 'g' },
    { "grey",         0, 0, 'g' },
    { "level",        1, 0, OPT_LEVEL },
    { "margin",       1, 0, 'm' },
    { "orientation",  1, 0, 'O' },
    { "output",       1, 0, 'o' },
    { "paper",        1, 0, 'P' },
    { "resolution",   1, 0, 'r' },
    { "stdout",       0, 0, 'c' },
    { "verbose",      0, 0, 'v' },
    
    { NULL, 0, 0, 0 }
};

static char *extsubst(char *fname, char *newext);



int
main(int argc, char *argv[])
{
    int c, i, ret;
    stream *st;
    epsf *par;
    char *outfile;
    exception ex;
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
	case 'a':
	    par->ascii = epsf_asc_num(optarg);
	    if (par->ascii == EPSF_ASC_UNKNOWN) {
		fprintf(stderr, "%s: unknown ASCII encoding `%s'\n",
			prg, optarg);
		exit(1);
	    }
	    break;
	case 'c':
	    cat = 1;
	    break;
	case 'C':
	    par->i.compression = image_compression_num(optarg);
	    if (par->i.compression == IMAGE_CMP_UNKNOWN) {
		fprintf(stderr, "%s: unknown compression method `%s'\n",
			prg, optarg);
		exit(1);
	    }
	    break;
	case 'g':
	    par->i.cspace.type = IMAGE_CS_GRAY;
	    break;
	case 'G':
	    epsf_set_gravity(par, optarg);
	    /* XXX: check for error */
	    break;
	case 'm':
	    i = epsf_parse_dimen(optarg);
	    epsf_set_margins(par, i, i, i, i);
	    break;
	case 'o':
	    outfile = optarg;
	    break;
	case 'O':
	    epsf_set_orientation(par, optarg);
	    /* XXX: check for error */
	    break;
	case 'P':
	    epsf_set_paper(par, optarg);
	    /* XXX: check for error */
	    break;
	case 'r':
	    epsf_set_resolution(par, atoi(optarg));
	    break;
	case 'v':
	    par->flags |= EPSF_FLAG_VERBOSE;
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

    ret = 0;

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

	st = NULL;
	
	if (catch(&ex) == 0) {
	    if (cat)
		st = stream_file_fopen(stdout, 0);
	    else
		st = stream_file_open(outfile);

	    epsf_process(st, argv[optind], par);
	    drop();
	}
	
	if (st)
	    stream_close(st);
	
	if (ex.code) {
	    if (outfile)
		remove(outfile);
	    fprintf(stderr, "%s: %s: %s\n",
		    prg, argv[optind], (char *)ex.data);
	    /* XXX: not all strings are allocated */
	    /* free(ex.data); */
	    ret = 1;
	}
    }
    else {
	for (i=optind; i<argc; i++) {
	    st = NULL;
	    outfile = NULL;
		
	    if (catch(&ex) == 0) {
		outfile = extsubst(argv[i], "eps");
		st = stream_file_open(outfile);
		epsf_process(st, argv[i], par);
		drop();
	    }
	    if (st)
		stream_close(st);
	    free(outfile);

	    if (ex.code) {
		remove(outfile);
		fprintf(stderr, "%s: %s: %s\n",
			prg, argv[i], (char *)ex.data);
		/* XXX: not all strings are allocated */
		/* free(ex.data); */
		ret = 1;
	    }
	}
    }
    
    exit(ret);
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

    s = xmalloc(q-p + strlen(newext) + 2);

    strncpy(s, p, q-p);
    q = s + (q-p);
    *(q++) = '.';
    strcpy(q, newext);

    return s;
}
