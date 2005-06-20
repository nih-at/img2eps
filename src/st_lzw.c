/*
  $NiH: st_lzw.c,v 1.3 2002/10/15 03:03:50 dillo Exp $

  st_lzw.c -- LZWEncode filter

  This file is part of img2eps, an image to EPS file converter.
  The author can be contacted at <dillo@giga.or.at>
*/

/*	$NetBSD: zopen.c,v 1.7 2002/05/26 22:25:38 wiz Exp $	*/

/*-
 * Copyright (c) 1985, 1986, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Diomidis Spinellis and James A. Woods, derived from original
 * work by Spencer Thomas and Joseph Orost.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*-
 * fcompress.c - File compression ala IEEE Computer, June 1984.
 *
 * Compress authors:
 *		Spencer W. Thomas	(decvax!utah-cs!thomas)
 *		Jim McKie		(decvax!mcvax!jim)
 *		Steve Davies		(decvax!vax135!petsd!peora!srd)
 *		Ken Turkowski		(decvax!decwrl!turtlevax!ken)
 *		James A. Woods		(decvax!ihnp4!ames!jaw)
 *		Joe Orost		(decvax!vax135!petsd!joe)
 *
 * Cleaned up and converted to library returning I/O streams by
 * Diomidis Spinellis <dds@doc.ic.ac.uk>.
 *
 * Adapted to PostScript variant and img2eps stream framework by
 * Dieter Baron <dillo@giga.or.at>.
 */



#include <errno.h>
#include <stddef.h>

#include "config.h"
#include "exceptions.h"
#include "stream.h"
#include "stream_types.h"



#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

STREAM_DECLARE(lzw);

#define BUFSIZE		1024		/* output buffer size */
#define	BITS		12		/* Default bits. */
#define	HSIZE		69001		/* 95% occupancy */

/* A code_int must be able to hold 2**BITS values of type int, and also -1. */
typedef long code_int;
typedef long count_int;

typedef unsigned char char_type;

#define	BLOCK_MASK	0x80

/*
 * Masks 0x40 and 0x20 are free.  I think 0x20 should mean that there is
 * a fourth header byte (for expansion).
 */
#define	INIT_BITS 9			/* Initial number of bits/code. */

#define	MAXCODE(n_bits)	((1 << (n_bits)) - 1)

struct stream_lzw {
	stream st;

	char zs_mode;			/* r or w */
	enum {
		S_START, S_MIDDLE, S_EOF
	} zs_state;			/* State of computation */
	int zs_n_bits;			/* Number of bits/code. */
	int zs_maxbits;			/* User settable max # bits/code. */
	code_int zs_maxcode;		/* Maximum code, given n_bits. */
	code_int zs_maxmaxcode;		/* Should NEVER generate this code. */
	count_int zs_htab [HSIZE];
	u_short zs_codetab [HSIZE];
	code_int zs_hsize;		/* For dynamic table sizing. */
	code_int zs_free_ent;		/* First unused entry. */
	/*
	 * Block compression parameters -- after all codes are used up,
	 * and compression rate changes, start over.
	 */
	int zs_block_compress;
	int zs_clear_flg;
	long zs_ratio;
	count_int zs_checkpoint;
	int zs_offset;
	long zs_in_count;		/* Length of input. */
	long zs_bytes_out;		/* Length of compressed output. */
	long zs_out_count;		/* # of codes output (for debugging).*/
	char_type zs_buf[BUFSIZE];
	union {
		struct {
			long zs_fcode;
			code_int zs_ent;
			code_int zs_hsize_reg;
			int zs_hshift;
		} w;			/* Write paramenters */
	} u;
};

/* Definitions to retain old variable names */
#define	zmode		zs->zs_mode
#define	state		zs->zs_state
#define	n_bits		zs->zs_n_bits
#define	maxbits		zs->zs_maxbits
#define	maxcode		zs->zs_maxcode
#define	maxmaxcode	zs->zs_maxmaxcode
#define	htab		zs->zs_htab
#define	codetab		zs->zs_codetab
#define	hsize		zs->zs_hsize
#define	free_ent	zs->zs_free_ent
#define	block_compress	zs->zs_block_compress
#define	clear_flg	zs->zs_clear_flg
#define	ratio		zs->zs_ratio
#define	checkpoint	zs->zs_checkpoint
#define	offset		zs->zs_offset
#define	in_count	zs->zs_in_count
#define	bytes_out	zs->zs_bytes_out
#define	out_count	zs->zs_out_count
#define	buf		zs->zs_buf
#define	fcode		zs->u.w.zs_fcode
#define	hsize_reg	zs->u.w.zs_hsize_reg
#define	ent		zs->u.w.zs_ent
#define	hshift		zs->u.w.zs_hshift

/*
 * To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type as
 * the codetab.  The tab_suffix table needs 2**BITS characters.  We get this
 * from the beginning of htab.  The output stack uses the rest of htab, and
 * contains characters.  There is plenty of room for any possible stack
 * (stack used to be 8000 characters).
 */

#define	htabof(i)	htab[i]
#define	codetabof(i)	codetab[i]

#define	tab_prefixof(i)	codetabof(i)
#define	tab_suffixof(i)	((char_type *)(htab))[i]
#define	de_stack	((char_type *)&tab_suffixof(1 << BITS))

#define	CHECK_GAP 10000		/* Ratio check interval. */

/*
 * the next two codes should not be changed lightly, as they must not
 * lie within the contiguous general code space.
 */
#define	CLEAR	256		/* Table clear output code. */
#define EOD	257		/* end of data code. */
#define	FIRST	258		/* First free entry. */

static void	cl_hash(stream_lzw *, count_int);
static int	output(stream_lzw *st, code_int ocode);

/*-
 * Algorithm from "A Technique for High Performance Data Compression",
 * Terry A. Welch, IEEE Computer Vol 17, No 6 (June 1984), pp 8-19.
 *
 * Algorithm:
 * 	Modified Lempel-Ziv method (LZW).  Basically finds common
 * substrings and replaces them with a variable size code.  This is
 * deterministic, and can be done on the fly.  Thus, the decompression
 * procedure needs no input table, but tracks the way the table was built.
 */



int
lzw_close(stream_lzw *zs)
{
	output(zs, (code_int) ent);
	output(zs, (code_int) EOD);
	output(zs, (code_int) - 1);

	stream_free((stream *)zs);

	return 0;
}



stream *
stream_lzw_open(stream *ost, void *params)
{
    stream_lzw *zs;

    zs = stream_create(lzw, ost);

    maxbits = BITS;
    maxmaxcode = 1 << maxbits;	/* Should NEVER generate this code. */
    hsize = HSIZE;			/* For dynamic table sizing. */
    free_ent = 0;			/* First unused entry. */
    block_compress = BLOCK_MASK;
    clear_flg = 0;
    ratio = 0;
    checkpoint = CHECK_GAP;
    in_count = 1;			/* Length of input. */
    out_count = 0;			/* # of codes output (for debugging). */
    state = S_START;

    return (stream *)zs;
}



/*-
 * compress write
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */

int
lzw_write(stream_lzw *zs, const char *wbp, int num)
{
	code_int i;
	int c, disp;
	const u_char *bp;
	int count;

	if (num == 0)
		return (0);

	count = num;
	bp = (u_char *)wbp;
	if (state == S_MIDDLE)
		goto middle;
	state = S_MIDDLE;

	maxmaxcode = 1L << maxbits;

	offset = 0;
	bytes_out = 0;
	out_count = 0;
	clear_flg = 0;
	ratio = 0;
	in_count = 1;
	checkpoint = CHECK_GAP;
	maxcode = MAXCODE(n_bits = INIT_BITS);
	free_ent = ((block_compress) ? FIRST : 256);

	ent = *bp++;
	--count;

	hshift = 0;
	for (fcode = (long)hsize; fcode < 65536L; fcode *= 2L)
		hshift++;
	hshift = 8 - hshift;	/* Set hash code range bound. */

	hsize_reg = hsize;
	cl_hash(zs, (count_int)hsize_reg);	/* Clear hash table. */

	output(zs, (count_int) CLEAR);

middle:	for (i = 0; count--;) {
		c = *bp++;
		in_count++;
		fcode = (long)(((long)c << maxbits) + ent);
		i = ((c << hshift) ^ ent);	/* Xor hashing. */

		if (htabof(i) == fcode) {
			ent = codetabof(i);
			continue;
		} else if ((long)htabof(i) < 0)	/* Empty slot. */
			goto nomatch;
		disp = hsize_reg - i;	/* Secondary hash (after G. Knott). */
		if (i == 0)
			disp = 1;
probe:		if ((i -= disp) < 0)
			i += hsize_reg;

		if (htabof(i) == fcode) {
			ent = codetabof(i);
			continue;
		}
		if ((long)htabof(i) >= 0)
			goto probe;
nomatch:	output(zs, (code_int) ent);
		out_count++;
		ent = c;
		if (free_ent < maxmaxcode-1) {
			codetabof(i) = free_ent++;	/* code -> hashtable */
			htabof(i) = fcode;
		} else {
		    ratio = 0;
		    cl_hash(zs, (count_int) hsize);
		    free_ent = FIRST;
		    clear_flg = 1;
		    output(zs, (code_int) CLEAR);
		}
#if 0
		} else if ((count_int)in_count >=
		    checkpoint && block_compress) {
			if (cl_block(zs) == -1)
				return (-1);
		}
#endif
	}
	return 0;
}



/*-
 * Output the given code.
 * Inputs:
 * 	code:	A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *		that n_bits =< (long)wordsize - 1.
 * Outputs:
 * 	Outputs code to the file.
 * Assumptions:
 *	Chars are 8 bits long.
 * Algorithm:
 * 	Maintain a BITS character long buffer (so that 8 codes will
 * fit in it exactly).  Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.
 */

static char_type lmask[9] =
	{0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01, 0x00};
static char_type rmask[9] =
	{0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};

static int
output(stream_lzw *zs, code_int ocode)
{
	int bits, r_off;
	char_type *bp;

	r_off = offset;
	bits = n_bits;
	bp = buf;
	if (ocode >= 0) {
		/* Get to the first byte. */
		bp += (r_off >> 3);
		r_off &= 7;
		/*
		 * Since ocode is always >= 8 bits, only need to mask the first
		 * hunk on the left.
		 */
		*bp = (*bp & rmask[r_off])
		    | ((ocode >> (bits+r_off-8)) & lmask[r_off]);
		bp++;
		bits -= (8 - r_off);
		ocode &= ~(0xfff << bits);
		/* Get any 8 bit parts in the middle (<=1 for up to 16 bits) */
		if (bits >= 8) {
			*bp++ = ocode >> (bits-8);
			bits -= 8;
			ocode &= ~(0xfff << bits);
		}
		/* Last bits. */
		if (bits)
			*bp = ocode << (8-bits);
		offset += n_bits;
		if (offset >= (BUFSIZE-2)*8) {
			bp = buf;
			bits = offset/8;
			bytes_out += bits;
			stream_write(zs->st.st, (char *)bp, bits);
			/* move partial byte to beginning of buffer */
			bp[0] = bp[bits];
			bits = offset % 8;
			offset = bits;
		}
		/*
		 * If the next entry is going to be too big for the ocode size,
		 * then increase it, if possible.
		 */
		/* XXX: >= for EarlyChange, make configurable */
		if (free_ent >= maxcode || (clear_flg > 0)) {
#if 0
		       /*
			* Write the whole buffer, because the input side won't
			* discover the size increase until after it has read it.
			*/
			if (offset > 0)
			        stream_write(zs->st.st, bp, n_bits);
			offset = 0;
#endif

			if (clear_flg) {
				maxcode = MAXCODE(n_bits = INIT_BITS);
				clear_flg = 0;
			} else {
				n_bits++;
				if (n_bits == maxbits)
					maxcode = maxmaxcode;
				else
					maxcode = MAXCODE(n_bits);
			}
		}
	} else {
		/* At EOF, write the rest of the buffer. */
		if (offset > 0) {
			offset = (offset + 7) / 8;
			stream_write(zs->st.st, (char *)buf, offset);
			bytes_out += offset;
		}
		offset = 0;
	}
	return (0);
}



#if 0
static int
cl_block(stream_lzw *zs)		/* Table clear for block compress. */
{
	long rat;

	checkpoint = in_count + CHECK_GAP;

	if (in_count > 0x007fffff) {	/* Shift will overflow. */
		rat = bytes_out >> 8;
		if (rat == 0)		/* Don't divide by zero. */
			rat = 0x7fffffff;
		else
			rat = in_count / rat;
	} else
		rat = (in_count << 8) / bytes_out;	/* 8 fractional bits. */
	if (rat > ratio)
		ratio = rat;
	else {
		ratio = 0;
		cl_hash(zs, (count_int) hsize);
		free_ent = FIRST;
		clear_flg = 1;
		if (output(zs, (code_int) CLEAR) == -1)
			return (-1);
	}
	return (0);
}
#endif



static void
cl_hash(stream_lzw *zs, count_int cl_hsize)	/* Reset code table. */
{
	count_int *htab_p;
	long i, m1;

	m1 = -1;
	htab_p = htab + cl_hsize;
	i = cl_hsize - 16;
	do {			/* Might use Sys V memset(3) here. */
		*(htab_p - 16) = m1;
		*(htab_p - 15) = m1;
		*(htab_p - 14) = m1;
		*(htab_p - 13) = m1;
		*(htab_p - 12) = m1;
		*(htab_p - 11) = m1;
		*(htab_p - 10) = m1;
		*(htab_p - 9) = m1;
		*(htab_p - 8) = m1;
		*(htab_p - 7) = m1;
		*(htab_p - 6) = m1;
		*(htab_p - 5) = m1;
		*(htab_p - 4) = m1;
		*(htab_p - 3) = m1;
		*(htab_p - 2) = m1;
		*(htab_p - 1) = m1;
		htab_p -= 16;
	} while ((i -= 16) >= 0);
	for (i += 16; i > 0; i--)
		*--htab_p = m1;
}
