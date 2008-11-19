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


/*
 * NAME
 *	fbuf.c
 *
 * DESCRIPTION
 *	This file contains the private data definitions and code for
 *	manipulating the virtual framebuffer.
 *
 *	Routines include:
 *		Run-length encoding a scanline.
 *		Allocating and initializing the framebuffer.
 *		Adding color to a pixel.
 *		Writing the framebuffer to a file.
 */


#include <stdio.h>
#include <math.h>
#include "rt.h"



static	struct				/* Runlength pixel definition.	     */
	{
	U8	r, g, b;		/* Color.			     */
	U8	count;			/* Pixel count for color.	     */
	}
	image[MAX_X];

/*
	image does not have  have to be made an array for tango-lite
	since it is only used in RunLengthEncode, which is only
	called by CloseFrameBuffer, which is only called after
	all children have terminated
*/



/*
 * NAME
 *	RunLengthEncode - rl encode a scanline and write the result to file
 *
 * SYNOPSIS
 *	VOID	RunLengthEncode(pf, fb, xsize)
 *	FILE	*pf;			// File pointer.
 *	PIXEL	*fb;			// Framebuffer pointer.
 *	INT	xsize;			// Size of scanline in x coordinates.
 *
 * RETURNS
 *	Nothing.
 */

VOID	RunLengthEncode(FILE *pf, PIXEL *fb, INT xsize)
	{
	INT	x;			/* Original buffer entry address.    */
	INT	rl;			/* Runlength buffer entry address.   */
	INT	numpixels;		/* Running count of identical pixels.*/
	U8	red, green, blue;	/* Color holders.		     */

	rl = 0;
	numpixels = 0;

	image[0].r = (U8)( (INT) (fb[0].r * 255.0));
	image[0].g = (U8)( (INT) (fb[0].g * 255.0));
	image[0].b = (U8)( (INT) (fb[0].b * 255.0));

	for (x = 1; x < xsize; x++)
		{
		red   = (U8)( (INT) (fb[x].r * 255.0));
		green = (U8)( (INT) (fb[x].g * 255.0));
		blue  = (U8)( (INT) (fb[x].b * 255.0));


		if (red   == image[rl].r  &&
		    green == image[rl].g  &&
		    blue  == image[rl].b  &&
		    numpixels < 255)
			numpixels++;
		else
			{
			image[rl].count = (U8)numpixels;
			rl++;

			image[rl].r = red;
			image[rl].g = green;
			image[rl].b = blue;
			numpixels   = 0;
			}
		}

	image[rl].count = (U8)numpixels;

	for (x = 0; x <= rl; x++)
		fprintf(pf, "%c%c%c%c", image[x].r, image[x].g, image[x].b, image[x].count);
	}



/*
 * NAME
 *	 OpenFrameBuffer - allocate and clear framebuffer
 *
 * SYNOPSIS
 *	 VOID	 OpenFrameBuffer()
 *
 * RETURNS
 *	Nothing.
 */

VOID	OpenFrameBuffer()
	{
	INT	i;
	PIXEL	*fb;

	fb = Display.framebuffer = (PIXEL*)GlobalMalloc(Display.numpixels*sizeof(PIXEL), "fbuf.c");

	for (i = 0; i < Display.numpixels; i++)
		{
		fb->r = 0.0;
		fb->g = 0.0;
		fb->b = 0.0;
		fb++;
		}
	}



/*
 * NAME
 *	 AddPixelColor - color is added to the frame buffer pixel at x,y
 *
 * SYNOPSIS
 *	VOID	AddPixelColor(c, x, y)
 *	COLOR	c;			// Pixel contribution color.
 *	INT	x;			// X coordinate of pixel.
 *	INT	y;			// Y coordinate of pixel.
 *
 * RETURNS
 *	Nothing.
 */

VOID	AddPixelColor(COLOR c, INT x, INT y)
	{
	INT	addr;			/* Index into framebuffer.	     */
	PIXEL	*fb;			/* Ptr to framebuffer.		     */

	addr = Display.xres * y + x;
	fb   = Display.framebuffer;

	fb[addr].r += c[0];
	fb[addr].g += c[1];
	fb[addr].b += c[2];
	}



/*
 * NAME
 *	CloseFrameBuffer - write the frame buffer pixels to a file
 *
 * SYNOPSIS
 *	VOID	CloseFrameBuffer(PicFileName, mode)
 *	CHAR	*PicFileName;		// Picture file name to create.
 *
 * RETURNS
 *	Nothing.
 */

VOID	CloseFrameBuffer(CHAR *PicFileName)
	{
	INT	x;
	INT	y;
	PIXEL	*fb;			/* Ptr to framebuffer.		     */
	FILE	*pf;			/* Ptr to picture file. 	     */

	pf = fopen(PicFileName, "wb");
	if (!pf)
		{
		printf("Unable to open picture file %s.\n", PicFileName);
		exit(-1);
		}


	/* Write file header. */

	x   = Display.xres;
	y   = Display.yres;
	fb  = Display.framebuffer;

	fprintf(pf, "%c%c%c%c%c%c%c%c",
		    (char)(0), (char)(0), (char)(x/256), (char) (x%256),
		    (char)(0), (char)(0), (char)(y/256), (char) (y%256));


	/* Clamp, run-length encode, and write out pixels on a scanline basis. */

	for (y = 0; y < Display.yres; y++)
		{
		for (x = 0; x < Display.xres; x++)
			{
			fb[x].r = Min(fb[x].r, 1.0);
			fb[x].g = Min(fb[x].g, 1.0);
			fb[x].b = Min(fb[x].b, 1.0);
			fb[x].r = Max(fb[x].r, 0.0);
			fb[x].g = Max(fb[x].g, 0.0);
			fb[x].b = Max(fb[x].b, 0.0);
			}

		RunLengthEncode(pf, fb, Display.xres);
		fb += Display.xres;
		}

	fclose(pf);
	GlobalFree(Display.framebuffer);
	}

