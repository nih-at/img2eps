/*
  $NiH$

  im_gif.c -- GIF image handling
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include "config.h"

#ifdef USE_GIF

#include <gif_lib.h>

#include "image.h"



struct image_gif {
    image im;

    GifFileType *gif;
};

IMAGE_DECLARE(gif);



int
gif_close(image_gif *im)
{
    int ret;

    ret = (DGifCloseFile(im->gif) == GIF_OK) ? 0 : -1;

    image_free((image *)im);

    return ret;
}



image *
gif_open(char *fname)
{
    image_gif *im;
    GifFileType *gif;

    if ((gif=DGifOpenFileName(fname)) == NULL)
	return NULL;

    if ((im=image_create(gif, fname)) == NULL) {
	DGifCloseFile(im->gif);
	return NULL;
    }

    im->im.i.width = gif->SWidth;
    im->im.i.height = gif->SHeight;
    im->gif = gif;
    return (image *)im;
}



int
gif_read(image_gif *im, char **bp)
{
    return -1;
}



int
gif_read_start(image_gif *im)
{
    return 0;
}



int
gif_read_finish(image_gif *im, int abortp)
{
    return 0;
}

#endif /* USE_GIF */
