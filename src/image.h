#ifndef _HAD_IMAGE_H
#define _HAD_IMAGE_H

/*
  $NiH: image.h,v 1.2 2002/09/08 21:31:47 dillo Exp $

  image.h -- image header
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <stdlib.h>

#define IMAGE_CS_INDEX_OFFSET	256
#define DEF_IMAGE_CS_INDEXED(c)	\
	IMAGE_CS_INDEXED_##c = IMAGE_CS_##c+IMAGE_CS_INDEX_OFFSET

#define IMAGE_CS_IS_INDEXED(cs)	((cs) > IMAGE_CS_INDEX_OFFSET)
#define IMAGE_CS_BASE(cs)	(IMAGE_CS_IS_INDEXED(cs)	\
				 ? (cs)-IMAGE_CS_INDEX_OFFSET	\
				 : (cs))

enum image_cspace {
    IMAGE_CS_UNKNOWN,
    IMAGE_CS_GRAY,
    IMAGE_CS_RGB,
    IMAGE_CS_CMYK,
    DEF_IMAGE_CS_INDEXED(GRAY),
    DEF_IMAGE_CS_INDEXED(RGB),
    DEF_IMAGE_CS_INDEXED(CMYK)
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
    IMAGE_ORD_UNKNOWN,
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
    int (*close)(image *);
    char *(*get_palette)(image *);
    image *(*open)(char *);
    int (*read)(image *, char **);
    int (*read_finish)(image *, int);
    int (*read_start)(image *);
    int (*set_cspace_depth)(image *, image_cspace, int);
    int (*set_size)(image *, int, int);
};

int _image_notsup(image *, int, int);

#ifdef NOSUPP_CSPACE
#define IMAGE_DECL_CSPACE(name)
#define IMAGE_METH_CSPACE(name)	(int (*)())_image_notsup
#else
#define IMAGE_DECL_CSPACE(name)			\
int name##_set_cspace_depth(image_##name *, image_cspace, int);
#define IMAGE_METH_CSPACE(name)	(int (*)())name##_set_cspace_depth
#endif
#ifdef NOSUPP_SCALE
#define IMAGE_DECL_SCALE(name)
#define IMAGE_METH_SCALE(name)	_image_notsup
#else
#define IMAGE_DECL_SCALE(name)			\
int name##_set_size(image_##name *, int, int);
#define IMAGE_METH_SCALE(name)	(int (*)())name##_set_size
#endif

#define IMAGE_DECLARE(name)			\
typedef struct image_##name image_##name;	\
int name##_close(image_##name *);		\
char *name##_get_palette(image_##name *);	\
image *name##_open(char *);			\
int name##_read(image_##name *, char **);	\
int name##_read_finish(image_##name *, int);	\
int name##_read_start(image_##name *);		\
IMAGE_DECL_CSPACE(name)				\
IMAGE_DECL_SCALE(name)				\
struct image_functions name##_functions  = {	\
    (int (*)())name##_close,			\
    (char *(*)())name##_get_palette,		\
    name##_open,				\
    (int (*)())name##_read,			\
    (int (*)())name##_read_finish,		\
    (int (*)())name##_read_start,		\
    IMAGE_METH_CSPACE(name),			\
    IMAGE_METH_SCALE(name)			\
}

#define image_create(name, fname)	((image_##name *)_image_create(	\
						&name##_functions,	\
						sizeof(image_##name),	\
						fname))


/* create and initialize image structure */
image *_image_create(struct image_functions *f, size_t size, char *fname);

int image_cspace_components(image_cspace cspace);
void image_free(image *im);
int image_get_palette_size(image *im);
int image_get_row_size(image *im);

/* external interface */

image *image_convert(image *oim, image_info *i);
void image_init_info(image_info *i);
image *image_open(char *fname);
int image_set_cspace(image *im, image_cspace cspace);
int image_set_depth(image *im, int depth);

#define IMAGE_METH0(name, im)		(((image *)(im))->f->name\
						((image *)(im)))
#define IMAGE_METH1(name, im, a0)	(((image *)(im))->f->name\
						((image *)(im), (a0)))
#define IMAGE_METH2(name, im, a0, a1)	(((image *)(im))->f->name\
						((image *)(im), (a0), (a1)))

#define image_close(im)		      IMAGE_METH0(close, (im))
#define image_get_palette(im)	      IMAGE_METH0(get_palette, (im))
#define image_read(im, bp)	      IMAGE_METH1(read, (im), (bp))
#define image_read_finish(im, ab)     IMAGE_METH1(read_finish, (im), (ab))
#define image_read_start(im)	      IMAGE_METH0(read_start, (im))
#define image_set_cspace_depth(im, cs, d)\
  				IMAGE_METH2(set_cspace_depth, (im), (cs), (d))
#define image_set_size(im, w, h)      IMAGE_METH2(set_size, (im), (w), (h))

#endif /* image.h */
