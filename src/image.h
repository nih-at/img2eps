#ifndef _HAD_IMAGE_H
#define _HAD_IMAGE_H

/*
  $NiH$

  image.h -- image header
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <stdlib.h>

enum image_cspace {
    IMAGE_CS_UNKNOWN,
    IMAGE_CS_GRAY,
    IMAGE_CS_RGB,
    IMAGE_CS_CMYK,
    IMAGE_CS_INDEXED_GRAY,
    IMAGE_CS_INDEXED_RGB,
    IMAGE_CS_INDEXED_CMYK
};

enum image_compression {
    IMAGE_CMP_UNKNOWN,
    IMAGE_CMP_NONE,
    IMAGE_CMP_RLE,
    IMAGE_CMP_LZW,
    IMAGE_CMP_FLATE,
    IMAGE_CMP_CCITT,
    IMAGE_CMP_DCT
};

/* keep in sync with enum epsf_write_image_matrix() in epsf.c */
enum image_order {
    IMAGE_ORD_ROW_LT,	/* row major, left to right, top to bottom */
    IMAGE_ORD_ROW_LB,	/* row major, left to right, bottom to top */
    IMAGE_ORD_ROW_RT,	/* row major, right to left, top to bottom */
    IMAGE_ORD_ROW_RB,	/* row major, right to left, bottom to top */
    IMAGE_ORD_COL_TL,	/* column major, top to bottom, left to right */
    IMAGE_ORD_COL_BL,	/* column major, bottom to top, left to right */
    IMAGE_ORD_COL_TR,	/* column major, top to bottom, right to left */
    IMAGE_ORD_COL_BR	/* column major, bottom to top, right to left */
};

typedef enum image_cspace image_cspace;
typedef enum image_compression image_compression;
typedef enum image_order image_order;

struct image_info {
    int width, height;			/* dimensions */
    image_cspace cspace;		/* used color space */
    int depth;				/* number of pixels per component */
    image_compression compression;	/* used compression scheme */
    image_order order;			/* order of samples in image data */
};    

typedef struct image_info image_info;

struct image {
    struct image_functions *f;	/* method function pointer */
    char *fname;		/* file name */
    image_info i;		/* info about image */
};

typedef struct image image;

struct image_functions {
    int (*close)(void *);
    image *(*open)(char *);
    int (*read)(void *, char **bp);
    int (*read_finish)(void *, int);
    int (*read_start)(void *);
};

#define IMAGE_DECLARE(name)			\
typedef struct image_##name image_##name;	\
int name##_close(image_##name *);		\
image *name##_open(char *);			\
int name##_read(image_##name *, char **);	\
int name##_read_finish(image_##name *, int);	\
int name##_read_start(image_##name *);		\
struct image_functions name##_functions  = {	\
    (int (*)())name##_close,			\
    name##_open,				\
    (int (*)())name##_read,			\
    (int (*)())name##_read_finish,		\
    (int (*)())name##_read_start		\
}

#define image_create(name, fname)	((image_##name *)_image_create(	\
						&name##_functions,	\
						sizeof(image_##name),	\
						fname))


/* create and initialize image structure */
image *_image_create(struct image_functions *f, size_t size, char *fname);

int image_cspace_components(image_cspace cspace);
void image_free(image *im);
#define image_read(im, bp)	((im)->f->read((im), (bp)))

/* external interface */

#define image_close(im)		((im)->f->close(im))
void image_init_info(image_info *i);
image *image_open(char *fname);
#define image_read(im, bp)		((im)->f->read((im), (bp)))
#define image_read_finish(im, abortp)	((im)->f->read_finish((im), (abortp)))
#define image_read_start(im)		((im)->f->read_start(im))
#endif /* image.h */
