/*
  $NiH: img2eps.c,v 1.23 2006/07/25 10:37:19 dillo Exp $

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
  -h, --help             display this help message\n\
  -V, --version          display version number\n\
  -1                     use language level 1\n\
  -2                     use language level 2\n\
  -3                     use language level 3\n\
  -a, --ascii ENC        use ENC ASCII encoding for binary data\n\
      --bottom-margin M  set bottom margin to M\n\
  -C, --compress C       use compression method C for image data\n\
  -c, --stdout           write EPSF to stdout\n\
  -G, --gravity G        set gravity to G\n\
  -g, --gray             force image to gray scale\n\
      --height H         set image height to H\n\
      --ignore-order     ignore pixel order information from image\n\
      --left-margin M    set left margin to M\n\
      --level L          use language level L\n\
  -m, --margin M         set all margins to M\n\
  -n, --dry-run          print info only, don't create EPSF\n\
  -O, --orientation O    set orientation to O\n\
  -o, --output FILE      write EPSF to FILE\n\
  -P, --paper P          set paper size to P\n\
  -r, --resolution R     set resolution to R\n\
      --right-margin M   set right margin to M\n\
  -R, --recompress       force recompression of image data\n\
  -S, --size S           set image size to S\n\
      --top-margin M     set top margin to M\n\
  -v, --verbose          be verbose\n\
      --width W          set image width to W\n\
\n"
#if defined(USE_EXIF) || defined(USE_GIF) || defined(USE_JPEG) || defined(USE_JPEG2000) || defined(USE_PNG) || defined(USE_TIFF)
"Support for the following optional features is included:\n\
\t"
#ifdef USE_EXIF
" exif"
#endif
#ifdef USE_GIF
" gif"
#endif
#ifdef USE_JPEG
" jpeg"
#endif
#ifdef USE_JPEG2000
" jpeg2000"
#endif
#ifdef USE_PNG
" png"
#endif
#ifdef USE_TIFF
" tiff"
#endif
"\n"
#endif /* any optional feature */
"\n\
Report bugs to <dillo@giga.or.at>.\n";

static const char usage[] =
"usage: %s [-hV] [-123cgnv] [-a asc] [-C comp] [-G grav] [-m marg] [-O ori] [-o file] [-P paper] [-S size] file [...]\n";


enum {
    OPT_BOTTOMM = 256,
    OPT_HEIGHT,
    OPT_IGNORE_ORDER,
    OPT_LEFTM,
    OPT_LEVEL,
    OPT_RIGHTM,
    OPT_TOPM,
    OPT_WIDTH
};

#define OPTIONS "hV123a:cC:gG:m:no:O:P:r:RS:v"

static const struct option options[] = {
    { "help",          0, 0, 'h' },
    { "version",       0, 0, 'V' },
    { "ascii",         1, 0, 'a' },
    { "bottom-margin", 1, 0, OPT_BOTTOMM },
    { "compress",      1, 0, 'C' },
    { "compression",   1, 0, 'C' },
    { "dry-run",       0, 0, 'n' },
    { "gravity",       1, 0, 'G' },
    { "gray",          0, 0, 'g' },
    { "grey",          0, 0, 'g' },
    { "height",        1, 0, OPT_HEIGHT },
    { "ignore-order",  0, 0, OPT_IGNORE_ORDER },
    { "left-margin",   1, 0, OPT_LEFTM },
    { "level",         1, 0, OPT_LEVEL },
    { "margin",        1, 0, 'm' },
    { "orientation",   1, 0, 'O' },
    { "output",        1, 0, 'o' },
    { "paper",         1, 0, 'P' },
    { "resolution",    1, 0, 'r' },
    { "recompress",    1, 0, 'R' },
    { "right-margin",  1, 0, OPT_RIGHTM },
    { "size",          1, 0, 'S' },
    { "top-margin",    1, 0, OPT_TOPM },
    { "stdout",        0, 0, 'c' },
    { "verbose",       0, 0, 'v' },
    { "width",         1, 0, OPT_WIDTH },
    
    { NULL, 0, 0, 0 }
};

static char *extsubst(const char *, const char *);
static void illegal_argument(const char *, const char *);



int
main(int argc, char *argv[])
{
    int c, i, ret;
    stream *st;
    epsf *par;
    char *outfile, *end;
    exception ex;
    int cat, dry_run;

    prg = argv[0];

    par = epsf_create_defaults();
    cat = 0;
    outfile = NULL;
    dry_run = 0;
    
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
	    if (par->ascii == EPSF_ASC_UNKNOWN)
		illegal_argument("ASCII encoding", optarg);
	    break;
	case 'C':
	    par->i.compression = image_compression_num(optarg);
	    if (par->i.compression == IMAGE_CMP_UNKNOWN)
		illegal_argument("compression method", optarg);
	    break;
	case 'c':
	    cat = 1;
	    break;
	case 'G':
	    if (epsf_set_gravity(par, optarg) < 0)
		illegal_argument("gravity", optarg);
	    break;
	case 'g':
	    par->i.cspace.type = IMAGE_CS_GRAY;
	    break;
	case 'm':
	    if (epsf_set_margins(par, optarg, EPSF_MARG_ALL) < 0)
		illegal_argument("margin", optarg);
	    break;
	case 'n':
	    par->flags |= EPSF_FLAG_VERBOSE;
	    dry_run = 1;
	    break;
	case 'O':
	    if (epsf_set_orientation(par, optarg) < 0)
		illegal_argument("orientation", optarg);
	    break;
	case 'o':
	    outfile = optarg;
	    break;
	case 'P':
	    if (epsf_set_paper(par, optarg) < 0)
		illegal_argument("paper size", optarg);
	    break;
	case 'r':
	    if (epsf_set_resolution(par, optarg) < 0)
		illegal_argument("resolution", optarg);
	    break;
	case 'R':
	    par->flags &= ~EPSF_FLAG_DIRECT_COPY;
	    break;
	case 'S':
	    if (epsf_set_image_size(par, optarg, EPSF_SIZE_BOTH) < 0)
		illegal_argument("image size", optarg);
	    break;
	case 'v':
	    par->flags |= EPSF_FLAG_VERBOSE;
	    break;
	case OPT_BOTTOMM:
	    if (epsf_set_margins(par, optarg, EPSF_MARG_BOTTOM) < 0)
		illegal_argument("margin", optarg);
	    break;
	case OPT_HEIGHT:
	    if (epsf_set_image_size(par, optarg, EPSF_SIZE_HEIGHT) < 0)
		illegal_argument("image height", optarg);
	    break;
	case OPT_IGNORE_ORDER:
	    par->flags |= EPSF_FLAG_IGNORE_ORDER;
	    break;
	case OPT_LEFTM:
	    if (epsf_set_margins(par, optarg, EPSF_MARG_LEFT) < 0)
		illegal_argument("margin", optarg);
	    break;
	case OPT_LEVEL:
	    i = strtoul(optarg, &end, 10);
	    if (i<1 || i>3 || (end && *end))
		illegal_argument("language level", optarg);
	    par->level = i;
	    break;
	case OPT_RIGHTM:
	    if (epsf_set_margins(par, optarg, EPSF_MARG_RIGHT) < 0)
		illegal_argument("margin", optarg);
	    break;
	case OPT_TOPM:
	    if (epsf_set_margins(par, optarg, EPSF_MARG_TOP) < 0)
		illegal_argument("margin", optarg);
	    break;
	case OPT_WIDTH:
	    if (epsf_set_image_size(par, optarg, EPSF_SIZE_WIDTH) < 0)
		illegal_argument("image width", optarg);
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
	    if (!dry_run) {
		if (cat)
		    st = stream_file_fopen(stdout, 0);
		else
		    st = stream_file_open(outfile);
	    }
	    
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
		if (!dry_run)
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
extsubst(const char *fname, const char *newext)
{
    const char *p, *q;
    char *s, *r;

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
    r = s + (q-p);
    *(r++) = '.';
    strcpy(r, newext);

    return s;
}



static void
illegal_argument(const char *type, const char *arg)
{
    fprintf(stderr, "%s: illegal %s `%s'\n",
	    prg, type, arg);
    exit(1);
}
