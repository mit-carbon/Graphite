/************************************************************************/
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

#include "tiff_rgba_io.h"
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <stdlib.h>

typedef short SHORT;
typedef long LONG;
typedef char CHAR;
typedef long INT;
typedef float REAL;
typedef void VOID;
typedef unsigned short USHORT;
typedef unsigned char BYTE;
typedef unsigned char BOOL;
typedef unsigned char UCHAR;
typedef unsigned long UINT;
typedef unsigned long ULONG;

#define FALSE 0
#define TRUE 1

typedef struct
	{
	SHORT	sFileType;		/* Mark what kind of file we have.   */
	SHORT	sBpp;			/* Bits per pixel, 1, 4, 8, 24, 32.  */
	SHORT	sRows;			/* Up to 1024.			     */
	SHORT	sCols;			/* Up to 1024.			     */
	SHORT	sVectorType;		/* So far, 0 = rectangular blocks.   */
	SHORT	sVectorSize;		/* So far, 16 for 4x4 square blocks. */
	LONG	lCntVectors;		/* Up to 65536. 		     */
	SHORT	sBrows; 		/* Number of block rows.	     */
	SHORT	sBcols; 		/* Number of block cols.	     */
	SHORT	sVrows; 		/* Number of vector rows.	     */
	SHORT	sVcols; 		/* Number of vector cols.	     */
	CHAR	achReserved[104];	/* Pad variable space to 128 bytes.  */
	CHAR	achUser[128];		/* User application space.	     */
	CHAR	achComment[256];	/* Rest of 1st block for comments.   */
	}
	ISPHDR;


typedef struct
	{
	USHORT	usRes1;
	USHORT	usRes2;
	USHORT	usRes3;
	USHORT	usRes4;
	USHORT	usRes5;
	USHORT	usRes6;
	USHORT	usResX;
	USHORT	usResY;
	USHORT	usRes7;
	}
	TGAHDR;


typedef struct
	{
	BYTE	b;
	BYTE	g;
	BYTE	r;
	}
	TGAPIX;


typedef struct
	{
	BYTE	r;
	BYTE	g;
	BYTE	b;
	BYTE	count;
	}
	SPAPIX;


/*
USHORT	ausBase[]    =
	{
	HST_BASE2,
	HST_BASE1,
	HST_BASE0
	};
*/

CHAR	*pchProgName = "rltotiff";         /* The program name.                 */

BOOL	fFirst	     = TRUE;		/* TRUE if processing first file.    */
BOOL	fCenter      = FALSE;		/* TRUE if -c option specified.      */
BOOL	fDebug	     = FALSE;		/* TRUE if -D option specified.      */
BOOL	fFlipBytes   = FALSE;		/* TRUE if -f option specified.      */
BOOL	fMerge	     = FALSE;		/* TRUE if -m option specified.      */
BOOL	fIspFmt      = FALSE;		/* TRUE if -I option specified.      */
BOOL	fPaulsFmt    = FALSE;		/* TRUE if -P option specified.      */
BOOL	fRleFmt      = FALSE;		/* TRUE if -R option specified.      */
BOOL	fSingleScr   = FALSE;		/* TRUE if -s option specified.      */
BOOL	fSpachFmt    = FALSE;		/* TRUE if -S option specified.      */
BOOL	fTranspose   = FALSE;		/* TRUE if -t option specified.      */

INT	iCntResX     = 0;
INT	iCntResY     = 0;
INT	iOffsetX     = 0;
INT	iOffsetY     = 0;
REAL	rGamma	     = 1.0;

INT	iCntDcX      = 1024;
INT	iCntDcY      = 768;
INT	iCntDcR      = 256;
INT	iCntDcG      = 256;
INT	iCntDcB      = 256;


ISPHDR	ih;				/* ISP header buffer.		     */
TGAHDR	th;				/* TGA header buffer.		     */

SPAPIX	sp;				/* Spach pixel buffer.		     */
TGAPIX	tp;				/* TGA pixel buffer.		     */


static long *gbRGBA;
static long gbWidth;
static long gbHeight;

VOID	ProcessSpachFile(FILE	*pf, CHAR	*pchFileName);
void configRGBABuf(void);
void SetPixel24(INT i, INT j,  BYTE r,  BYTE g, BYTE b);

int main(int argc, char **argv)
{
    FILE *fp;
    char *spachfile, *tiffile;

    if (argc < 3) {
	fprintf(stderr, "spachtotiff <infile> <outfile>\n");
	exit(1);
    }
    spachfile = argv[1];
    tiffile = argv[2];

    fp = fopen(spachfile, "rb");

    if (!fp) {
	fprintf(stderr, "spachtotiff: could not open file %s\n", spachfile);
	exit(1);
    }

    ProcessSpachFile(fp, spachfile);
    tiff_save_rgba(tiffile, gbRGBA, gbWidth, gbHeight);
    return(0);
}



void
configRGBABuf()
{
    gbRGBA = (long *)malloc(gbWidth*gbHeight*sizeof(long));
}



void
SetPixel24(INT i, INT j,  BYTE r,  BYTE g, BYTE b)
{
    if (i+j*gbWidth >= gbWidth*gbHeight)
	fprintf(stderr, "Bug!\n");

    gbRGBA[i+(gbHeight-j-1)*gbWidth] =
	((UINT)r)*256*256 + ((UINT)g)*256 + ((UINT)b);
}


/*
 * NAME
 *	ProcessSpachFile - process the file data as needed
 *
 * SYNOPSIS
 *	VOID	ProcessSpachFile(pf)
 *	FILE	*pf;			// Pointer to stream FILE handle.
 *	CHAR	*pchFileName;		// Pointer to filename.
 *
 * RETURNS
 *	Nothing.
 */

VOID	ProcessSpachFile(FILE	*pf, CHAR	*pchFileName)
	{
	INT	i;
	INT	j;
	INT	k;
	INT	count;
	UINT	ui;
	LONG	lPixCnt;

	ui	 = getc(pf);
	ui	 = getc(pf);
	ui	 = getc(pf);
	iCntResX = ui*256 + (UINT)getc(pf);

	ui	 = getc(pf);
	ui	 = getc(pf);
	ui	 = getc(pf);
	iCntResY = ui*256 + (UINT)getc(pf);

	gbWidth = iCntResX;
	gbHeight = iCntResY;

	lPixCnt  = (ULONG)iCntResX*(ULONG)iCntResY;

	configRGBABuf();


	if (fCenter)
		{
		iOffsetX = (iCntDcX - iCntResX)/2;
		iOffsetY = (iCntDcY - iCntResY)/2;
		}

	for (i = 0, j = 0; lPixCnt > 0; lPixCnt -= count)
		{
		if (fread(&sp, 1, sizeof(sp), pf) != sizeof(sp))
			{
			fprintf(stderr, "%s: Unexpected EOF in file \"%s\".\n", pchProgName, pchFileName);
			exit(1);
			}

		count = (UINT)sp.count + 1;

		for (k = 0; k < count; k++, i++)
			{
			if (i >= iCntResX)
				{
				i = 0;
				j++;
				}

			if (fDebug)
				printf("%ld\t%ld\t0x%02X\t0x%02X\t0x%02X\t0x%02X\n",
					i, j, sp.count, sp.r, sp.g, sp.b);

			SetPixel24(i, j, sp.r, sp.g, sp.b);
			}
		}
	}


