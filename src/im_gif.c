/*
  $NiH: im_gif.c,v 1.9 2002/10/12 00:02:07 dillo Exp $

  im_gif.c -- GIF image handling
  Copyright (C) 2002 Dieter Baron

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



#include "config.h"

#ifdef USE_GIF

#include <errno.h>
#include <stdio.h>

#include <gif_lib.h>

#define NOSUPP_CSPACE
#define NOSUPP_SCALE
#define NOSUPP_RAW

#include "exceptions.h"
#include "image.h"
#include "xmalloc.h"



struct image_gif {
    image im;

    GifFileType *gif;

    char *pal;

    int interlace;	/* whether image is interlaced */
    int n;		/* size of one row */
    int row;		/* current row */
    char *p;		/* pointer to image data */
};

IMAGE_DECLARE(gif);

static const char *_errstr(int err);



void
gif_close(image_gif *im)
{
    free(im->pal);

    if (DGifCloseFile(im->gif) != GIF_OK)
	throwf(EIO, "error closing: %s", _errstr(GifLastError()));

    image_free((image *)im);
}



char *
gif_get_palette(image_gif *im)
{
    char *pal;
    int n, m;
    
    pal = (char *)im->gif->SColorMap->Colors;

    if (im->gif->SColorMap->ColorCount != 1<<im->im.i.cspace.ncol) {
	free(im->pal);
	n = image_cspace_palette_size(&im->im.i.cspace);
	m = im->gif->SColorMap->ColorCount * 3;
	im->pal = xmalloc(n);
	memcpy(im->pal, pal, m);
	memset(im->pal+m, 0, n-m);
	pal = im->pal;
    }
    return pal;
}



image *
gif_open(char *fname)
{
    image_gif *im;
    GifFileType *gif;
    exception ex;

    if ((gif=DGifOpenFileName(fname)) == NULL)
	return NULL;

    if (catch(&ex) == 0) {
	im = image_create(gif, fname);
	drop();
    }
    else {
	DGifCloseFile(im->gif);
	throw(&ex);
    }

    im->gif = gif;
    im->pal = NULL;

    im->im.i.width = gif->SWidth;
    im->im.i.height = gif->SHeight;
    im->im.i.cspace.type = IMAGE_CS_INDEXED;
    im->im.i.cspace.depth = 8;
    im->im.i.cspace.base_type = IMAGE_CS_RGB;
    im->im.i.cspace.base_depth = 8;
    im->im.i.cspace.ncol = 1<<gif->SColorResolution;
    im->im.oi = im->im.i;

    /* no raw reading */
    im->im.i.compression = IMAGE_CMP_NONE;

    return (image *)im;
}



int
gif_read(image_gif *im, char **bp)
{
    int row;

    if (!im->interlace)
	row = im->row;
    else {
	switch (im->row%8) {
	case 0:
	    row = im->row/8;
	    break;
	case 4:
	    row = im->row/8 + (im->im.i.height-1)/8 + 1;
	    break;
	case 2:
	case 6:
	    row = im->row/4 + (im->im.i.height-1)/4 + 1;
	    break;
	case 1:
	case 3:
	case 5:
	case 7:
	    row = im->row/2 + (im->im.i.height-1)/2 + 1;
	    break;
	}
    }

    *bp = im->p + row*im->n;
    im->row++;
    return -1;
}



void
gif_read_start(image_gif *im)
{
    /*
     * Fear and Loathing: Thank's to the clean implementation and the
     * superbly detailed documentation, I can't figure out how
     * DGifGetLine() is supposed to work.  (Maybe it's supposed to
     * give errors on all inputs, wouldn't surprise me.)  So damned be
     * memory, slurp it all in.
     */

    if (DGifSlurp(im->gif) != GIF_OK)
	throwf(EIO, "error reading image: %s",
	       _errstr(GifLastError()));

    if (im->gif->ImageCount != 1)
	throws(EOPNOTSUPP, "multi-image GIFs not supported");

    /* XXX: check that image uses whole screen */

    im->interlace = im->gif->SavedImages[0].ImageDesc.Interlace;
    im->n = image_get_row_size((image *)im);
    im->p = im->gif->SavedImages[0].RasterBits;
    im->row = 0;
    
    return;
}



void
gif_read_finish(image_gif *im, int abortp)
{
    return;
}



static const char *
_errstr(int err)
{
    static char b[128];

    /*
     * Fear and Loathing continues: giflib's desinger, in his infinite
     * wisdom, provided no interface to get an error number's
     * description, only to print it to stdout.  But then again, who
     * in his right mind would ever want to do anything else with
     * it?
     */

    switch (err) {
    case D_GIF_ERR_OPEN_FAILED:
	return "cannot open file";
    case D_GIF_ERR_READ_FAILED:
	return "read error";
    case D_GIF_ERR_NOT_GIF_FILE:
	return "not a GIF file";
    case D_GIF_ERR_NO_SCRN_DSCR:
	return "no screen descriptor detected";
    case D_GIF_ERR_NO_IMAG_DSCR:
	return "no image descriptor detected";
    case D_GIF_ERR_NO_COLOR_MAP:
	return "neither global nor local color map detected";
    case D_GIF_ERR_WRONG_RECORD:
	return "wrong record type detected";
    case D_GIF_ERR_DATA_TOO_BIG:
	return "too many pixels";
    case D_GIF_ERR_NOT_ENOUGH_MEM:
	return "out of memory";
    case D_GIF_ERR_CLOSE_FAILED:
	return "cannot close file";
    case D_GIF_ERR_NOT_READABLE:
	return "file not opened for reading";
    case D_GIF_ERR_IMAGE_DEFECT:
	return "image is defective, decoding aborted";
    case D_GIF_ERR_EOF_TOO_SOON:
	return "premature end of file";

    default:
	sprintf(b, "unknown error %d", err);
	return b;
    }
}

#endif /* USE_GIF */
