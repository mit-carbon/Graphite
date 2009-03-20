/*************************************************************************/
/*                                                                       */
/*  Copyright (c) 1994 Stanford University                               */
/*                                                                       */
/*  All rights reserved.                                                 */
/*                                                                       */
/*  Permission is given to use, copy, and modify this software for any   */
/*  non-commercial purpose as long as this copyright notice is not       */
/*  removed.  All other uses, including redistribution in whole or in    */
/*  part, are forbidden without prior written permission.                */
/*                                                                       */
/*  This software is provided with absolutely no warranty and no         */
/*  support.                                                             */
/*                                                                       */
/*************************************************************************/

#ifndef USE_PROTOTYPES
#define USE_PROTOTYPES 1
#endif

#ifndef lint
static char sccsid[] = "@(#)savemap.c	1.3 2/6/9q";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <tiffio.h>
#include "tiff_rgba_io.h"

#define	streq(a,b)	(strcmp(a,b) == 0)

#define BYTESWAP

typedef unsigned char  u_char;
typedef unsigned long  u_long;
typedef unsigned short ushort;
typedef unsigned short u_short;

static u_char	rbuf[2048];
static u_char	gbuf[2048];
static u_char	bbuf[2048];
static u_char	abuf[2048];
static u_char	*scanline = NULL;

static long	rowsperstrip = -1;
static long	compression = COMPRESSION_LZW;
static long	config = PLANARCONFIG_CONTIG;
static long	orientation = ORIENTATION_BOTLEFT;

#define	MIN(a,b)	((a)<(b)?(a):(b))
#define	ABS(x)		((x)<0?-(x):(x))


long
tiff_save_rgba(char *name, long *pixels, long width, long height)
{
	TIFF *tif;
	long xsize, ysize;
	long y;
	long *pos;

	xsize = width;
	ysize = height;

	tif = TIFFOpen(name, "w");
	if (tif == NULL)
	    return 0;

	TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, xsize);
	TIFFSetField(tif, TIFFTAG_IMAGELENGTH, ysize);
	TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 4);
	TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
	TIFFSetField(tif, TIFFTAG_PLANARCONFIG, config);
	TIFFSetField(tif, TIFFTAG_COMPRESSION, compression);
	TIFFSetField(tif, TIFFTAG_ORIENTATION, orientation);
	if (rowsperstrip <= 0)
		rowsperstrip = (8*1024)/TIFFScanlineSize(tif);
	TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP,
	    rowsperstrip == 0 ? 1 : rowsperstrip);
	if (config == PLANARCONFIG_CONTIG)
		scanline = (u_char *)malloc(TIFFScanlineSize(tif));

	for (y = 0, pos = pixels; y < ysize;
			y++, pos += xsize) {

	    if (config == PLANARCONFIG_CONTIG) {
#ifdef BYTESWAP
		register char *sp = (char *) pos;
		register char *tp = (char *) scanline;
		register long x;

		for (x = 0; x < xsize; x++) {
		    tp[3] = sp[0];
		    tp[2] = sp[1];
		    tp[1] = sp[2];
		    tp[0] = sp[3];
			sp += 4;
			tp += 4;
		}
#endif
#ifndef BYTESWAP
                bcopy(pos, scanline, xsize*4);
#endif
		if (TIFFWriteScanline(tif, scanline, y, 0) < 0)
			break;
	    }
	    else if (config == PLANARCONFIG_SEPARATE) {

		register char *pp = (char *) pos;
		register long x;

		for (x = 0; x < xsize; x++) {
		    rbuf[x] = pp[0];
		    gbuf[x] = pp[1];
		    bbuf[x] = pp[2];
		    abuf[x] = pp[3];
			pp += 4;
		}
		if (TIFFWriteScanline(tif, rbuf, y, 0) < 0 ||
		    TIFFWriteScanline(tif, gbuf, y, 1) < 0 ||
		    TIFFWriteScanline(tif, bbuf, y, 2) < 0 ||
		    TIFFWriteScanline(tif, abuf, y, 3) < 0)
			break;
	    }
	}
    (void) TIFFClose(tif);
    return 1;
}


long
tiff_load_rgba(char *file, long **pixels, long *width, long *height)
{
	TIFF *tif;
	u_short bitspersample, samplesperpixel;
	u_long xsize, ysize;
	register long x, y, rowbytes;
	u_char *buf;
        long temp;
	ushort orient;
        register char *tp ;
        register char *sp ;

	if ((tif = TIFFOpen(file, "r")) == NULL) {
	    fprintf(stderr, "%s: error opening file.\n", file);
	    return 0;
	}

	TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);
	switch (bitspersample) {
	case 8:
		break;
	default:
		fprintf(stderr, "%s: Can not image a %d-bit/sample image.\n",
		    file, bitspersample);
	        return 0;
	}
	TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
        switch (samplesperpixel) {
	case 3: case 4:
		break;
	default:
		fprintf(stderr, "%s: Can not image a %d samples/pixel image.\n",
		    file, bitspersample);
	        return 0;
	}
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &xsize);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &ysize);
	TIFFGetField(tif, TIFFTAG_ORIENTATION, &orient);
	switch (orient) {
	case ORIENTATION_TOPLEFT:  case ORIENTATION_BOTLEFT:
	    break;
	default:
	    fprintf(stderr, "%s: Unsupported orientation.  Will assume ORIENTATION_TOPLEFT.\n" , file);
	    break;
	}

	rowbytes =TIFFScanlineSize(tif);

	*width = xsize;
	*height = ysize;
	*pixels = (long *) malloc(ysize*xsize*sizeof(long));


        switch (samplesperpixel) {
	    case 3:
		tp = (char *) (*pixels);
		if (orient != ORIENTATION_BOTLEFT)
		    tp += 4 * (*height-1) * *width;
		buf = (u_char *) malloc(rowbytes);

		for (y = 0; y < ysize; y++) {
		    if (TIFFReadScanline(tif, buf, y, 0) < 0)
			break;

#ifdef BYTESWAP
		    sp = (char *) buf;

		    for (x = 0; x < xsize; x++) {
			tp[3] = sp[0];
			tp[2] = sp[1];
			tp[1] = sp[2];
			tp[0] = 0;
			tp += 4;
			sp += 3;
		    }
#endif

		if (orient != ORIENTATION_BOTLEFT)
		    tp -= 8* *width;
		}
		break;

	    case 4:
		buf = (u_char *) (*pixels);
		if (orient != ORIENTATION_BOTLEFT)
		    buf += (*height-1) * rowbytes;
		for (y = 0; y < ysize; y++) {
		    if (TIFFReadScanline(tif, buf, y, 0) < 0)
			break;


#ifdef BYTESWAP
		    tp = (char *) buf;
		    sp = (char *) &temp;

		    for (x = 0; x < xsize; x++) {
			temp = *((long *)tp);
			tp[3] = sp[0];
			tp[2] = sp[1];
			tp[1] = sp[2];
			tp[0] = sp[3];
			tp += 4;
		    }
#endif
		    if (orient != ORIENTATION_BOTLEFT)
			buf -= rowbytes;
		    else
			buf += rowbytes;
		}
		break;

	    default:
		fprintf(stderr, "%s: Can not image a %d samples/pixel image.\n",
		    file, bitspersample);
	        return 0;
	}

	(void) TIFFClose(tif);
	return 1;
}
