#ifndef _HAD_IMAGE_H
#define _HAD_IMAGE_H

/*
  $NiH: image.h,v 1.8 2002/10/08 16:46:59 dillo Exp $

  image.h -- image header
  Copyright (C) 2002 Dieter Baron

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

#include <stdlib.h>

#include "util.h"

enum image_cs_type {		/* type of color space */
    IMAGE_CS_UNKNOWN,
    IMAGE_CS_GRAY,		/* grayscale */
    IMAGE_CS_RGB,		/* red/green/blue */
    IMAGE_CS_CMYK,		/* cyan/magenta/yellow/black */
    IMAGE_CS_HSV,		/* hue/saturation/value */
    IMAGE_CS_INDEXED		/* index into palette */
};

enum image_tr_type {		/* type of transparency */
    IMAGE_TR_UNKNOWN,
    IMAGE_TR_NONE,		/* none */
    IMAGE_TR_COLOR,		/* specific color (range) */
    IMAGE_TR_MASK,		/* bit mask */
    IMAGE_TR_ALPHA		/* full alpha channel */
};

/* keep in sync with _cmp_tab[] in stream.c */
enum image_compression {
    IMAGE_CMP_UNKNOWN,
    IMAGE_CMP_NONE,
    IMAGE_CMP_RLE,
    IMAGE_CMP_LZW,
    IMAGE_CMP_FLATE,
    IMAGE_CMP_CCITT,
    IMAGE_CMP_DCT
};

/* keep in sync with epsf_write_image_matrix() in epsf.c */
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

typedef enum image_cs_type image_cs_type;
typedef enum image_tr_type image_tr_type;
typedef enum image_compression image_compression;
typedef enum image_order image_order;

struct image_cspace {
    image_cs_type type;		/* type of color space */
    image_tr_type transparency;	/* type of transarency information */
    int depth;			/* number of pixels per component */

    image_cs_type base_type;	/* type of base color space for indexed */
    int base_depth;		/* depth of base color space */
    int ncol;			/* number of colors in palette */
};

typedef struct image_cspace image_cspace;

#define IMAGE_INF_TYPE		0x001
#define IMAGE_INF_DEPTH		0x002
#define IMAGE_INF_TRANSPARENCY	0x004
#define IMAGE_INF_BASE_TYPE	0x008
#define IMAGE_INF_BASE_DEPTH	0x010
#define IMAGE_INF_CSPACE	0x01f	/* any member of cspace */
#define IMAGE_INF_SIZE		0x020
#define IMAGE_INF_ORDER		0x040
#define IMAGE_INF_COMPRESSION	0x080
#define IMAGE_INF_ALL		0x0ff	/* all of the above */

struct image_info {
    int width, height;			/* dimensions */
    image_cspace cspace;		/* used color space */
    image_compression compression;	/* used compression scheme */
    image_order order;			/* order of samples in image data */
};    

typedef struct image_info image_info;

struct image {
    struct image_functions *f;	/* method function pointer */
    char *fname;		/* file name */
    image_info i;		/* info about image */
    image_info oi;		/* info about original image (unconverted) */
};

typedef struct image image;

struct image_functions {
    void (*close)(image *);
    char *(*get_palette)(image *);
    image *(*open)(char *);
    int (*raw_read)(image *, char **);
    void (*raw_read_finish)(image *, int);
    void (*raw_read_start)(image *);
    int (*read)(image *, char **);
    void (*read_finish)(image *, int);
    void (*read_start)(image *);
    int (*set_cspace)(image *, int, const image_cspace *);
    int (*set_size)(image *, int, int);
};

#define IMAGE_CS_EQUAL(field, new, old) \
		    ((new)->field == 0 || (new)->field == (old)->field)

int _image_notsup_cspace(image *im, int mask, const image_cspace *cspace);
int _image_notsup_raw(image *, int, int);
int _image_notsup_scale(image *im, int w, int h);

#ifdef NOSUPP_CSPACE
#define IMAGE_DECL_CSPACE(name)
#define IMAGE_METH_CSPACE(name)	_image_notsup_cspace
#else
#define IMAGE_DECL_CSPACE(name)			\
int name##_set_cspace(image_##name *, int, const image_cspace *);
#define IMAGE_METH_CSPACE(name)	(int (*)())name##_set_cspace
#endif
#ifdef NOSUPP_SCALE
#define IMAGE_DECL_SCALE(name)
#define IMAGE_METH_SCALE(name)	_image_notsup_scale
#else
#define IMAGE_DECL_SCALE(name)			\
int name##_set_size(image_##name *, int, int);
#define IMAGE_METH_SCALE(name)	(int (*)())name##_set_size
#endif
#ifdef NOSUPP_RAW
#define IMAGE_DECL_RAWS(name)
#define IMAGE_DECL_RAW(name)
#define IMAGE_DECL_RAWF(name)
#define IMAGE_METH_RAWS(name)	(void (*)())_image_notsup_raw
#define IMAGE_METH_RAW(name)	(int (*)())_image_notsup_raw
#define IMAGE_METH_RAWF(name)	(void (*)())_image_notsup_raw
#else
#define IMAGE_DECL_RAWS(name)	\
void name##_raw_read_start(image_##name *);
#define IMAGE_DECL_RAW(name)	\
int name##_raw_read(image_##name *, char **);
#define IMAGE_DECL_RAWF(name)	\
void name##_raw_read_finish(image_##name *, int);
#define IMAGE_METH_RAWS(name)	(void (*)())name##_raw_read_start
#define IMAGE_METH_RAW(name)	(int (*)())name##_raw_read
#define IMAGE_METH_RAWF(name)	(void (*)())name##_raw_read_finish
#endif

#define IMAGE_DECLARE(name)			\
typedef struct image_##name image_##name;	\
void name##_close(image_##name *);		\
char *name##_get_palette(image_##name *);	\
image *name##_open(char *);			\
int name##_read(image_##name *, char **);	\
void name##_read_finish(image_##name *, int);	\
void name##_read_start(image_##name *);		\
IMAGE_DECL_CSPACE(name)				\
IMAGE_DECL_SCALE(name)				\
IMAGE_DECL_RAWS(name)				\
IMAGE_DECL_RAW(name)				\
IMAGE_DECL_RAWF(name)				\
struct image_functions name##_functions  = {	\
    (void (*)())name##_close,			\
    (char *(*)())name##_get_palette,		\
    name##_open,				\
    IMAGE_METH_RAW(name),			\
    IMAGE_METH_RAWF(name),			\
    IMAGE_METH_RAWS(name),			\
    (int (*)())name##_read,			\
    (void (*)())name##_read_finish,		\
    (void (*)())name##_read_start,		\
    IMAGE_METH_CSPACE(name),			\
    IMAGE_METH_SCALE(name)			\
}

#define image_create(name, fname)	((image_##name *)_image_create(	\
						&name##_functions,	\
						sizeof(image_##name),	\
						fname))


extern const struct _num_name _image_nn_cspace[];
extern const struct _num_name _image_nn_compression[];

#define image_cspace_name(a)	(num2name(_image_nn_cspace, (a), 2))
#define image_compression_name(a) (num2name(_image_nn_compression, (a), 2))
#define image_cspace_num(a)	(name2num(_image_nn_cspace, (a)))
#define image_compression_num(a) (name2num(_image_nn_compression, (a)))

/* create and initialize image structure */
image *_image_create(struct image_functions *f, size_t size,
		     const char *fname);

int image_cspace_components(const image_cspace *cspace, int base);
int image_cspace_diffs(const image_cspace *cst,
		       int mask, const image_cspace *css);
void image_cspace_merge(image_cspace *cst, int mask, const image_cspace *css);
int image_cspace_palette_size(const image_cspace *cspace);
void image_free(image *im);
int image_get_row_size(const image *im);
int image_info_mask(const image_info *cs);
char *image_info_print(const image_info *i);

/* external interface */

image *image_convert(image *oim, int mask, const image_info *i);
void image_init_info(image_info *i);
image *image_open(const char *fname);

#define IMAGE_METH0(name, im)		(((image *)(im))->f->name\
						((image *)(im)))
#define IMAGE_METH1(name, im, a0)	(((image *)(im))->f->name\
						((image *)(im), (a0)))
#define IMAGE_METH2(name, im, a0, a1)	(((image *)(im))->f->name\
						((image *)(im), (a0), (a1)))

#define image_close(im)		      IMAGE_METH0(close, (im))
#define image_get_palette(im)	      IMAGE_METH0(get_palette, (im))
#define image_raw_read(im, bp)	      IMAGE_METH1(raw_read, (im), (bp))
#define image_raw_read_finish(im, ab) IMAGE_METH1(raw_read_finish, (im), (ab))
#define image_raw_read_start(im)      IMAGE_METH0(raw_read_start, (im))
#define image_read(im, bp)	      IMAGE_METH1(read, (im), (bp))
#define image_read_finish(im, ab)     IMAGE_METH1(read_finish, (im), (ab))
#define image_read_start(im)	      IMAGE_METH0(read_start, (im))
#define image_set_cspace(im, m, cs)   IMAGE_METH2(set_cspace, (im), (m), (cs))
#define image_set_size(im, w, h)      IMAGE_METH2(set_size, (im), (w), (h))

#endif /* image.h */
