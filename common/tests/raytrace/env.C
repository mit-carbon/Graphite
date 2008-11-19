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
 *	env.c
 *
 * DESCRIPTION
 *	This file contains routines that read environment description files
 *	as well as routines to setup and manipulate the viewing transform,
 *	routines to manipulate lights, colors, and to print the environment.
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "rt.h"



/*
 *	Define environment specification opcodes.
 */

#define OP_EYE			'a'
#define OP_COI			'b'
#define OP_BKGCOL		'c'
#define OP_VANG 		'd'
#define OP_LIGHT		'e'
#define OP_RES			'f'
#define OP_MAXLEVEL		'g'
#define OP_MINWEIGHT		'h'
#define OP_PROJECT		'i'
#define OP_ANTILEVEL		'j'
#define OP_ANTITOL		'k'
#define OP_MODELMAT		'l'
#define OP_SHAD 		'm'
#define OP_SHADING		'n'
#define OP_DISPLAY_IN		'o'
#define OP_DISPLAY_OUT		'p'
#define OP_GEOM_FILE		'q'
#define OP_TEX_FILE		'r'
#define OP_RL_FILE		's'
#define OP_TEXTURE		't'
#define OP_IMAGE		'u'
#define OP_FOOTPRINT		'v'
#define OP_TRAVERSAL		'w'
#define OP_AMBIENT		'x'
#define OP_EXCELL_PRIM		'y'
#define OP_EXCELL_DIR		'z'
#define OP_EXCELL_SUBDIV	'9'
#define OP_HIER_SUBLEVEL	'1'
#define OP_HIER_PRIMNUM 	'2'
#define OP_PREVIEW_BKCULL	'3'
#define OP_PREVIEW_FILL 	'4'
#define OP_PREVIEW_SPHTESS	'5'
#define OP_ERROR		'0'
#define OP_NORM_DB		'6'
#define OP_DATA_TYPE		'7'
#define OP_HU_MAX_PRIMS_CELL	'8'
#define OP_HU_GRIDSIZE		'@'
#define OP_HU_NUMBUCKETS	'#'
#define OP_HU_MAX_SUBDIV	'$'
#define OP_HU_LAZY		'*'
#define OP_BUNDLE		'+'
#define OP_BLOCK		'%'



/*
 *	Define structure for a command table entry (CTE).
 */

typedef struct
	{
	CHAR	*command;		/* Command name.		     */
	CHAR	opcode; 		/* Command code.		     */
	}
	CTE;



/*
 *	Allocate and initialize the command table.
 */

CTE	cmdtab[] =
	{
	{"eye",                 OP_EYE                  },
	{"center",              OP_COI                  },
	{"light",               OP_LIGHT                },
	{"resolution",          OP_RES                  },
	{"shadows",             OP_SHAD                 },
	{"background",          OP_BKGCOL               },
	{"viewangle",           OP_VANG                 },
	{"antilevel",           OP_ANTILEVEL            },
	{"minweight",           OP_MINWEIGHT            },
	{"project",             OP_PROJECT              },
	{"antitolerance",       OP_ANTITOL              },
	{"maxlevel",            OP_MAXLEVEL             },
	{"modelxform",          OP_MODELMAT             },
	{"shading",             OP_SHADING              },
	{"displayin",           OP_DISPLAY_IN           },
	{"displayout",          OP_DISPLAY_OUT          },
	{"geometry",            OP_GEOM_FILE            },
	{"texturetype",         OP_TEXTURE              },
	{"texturefile",         OP_TEX_FILE             },
	{"image",               OP_IMAGE                },
	{"footprint",           OP_FOOTPRINT            },
	{"traversal",           OP_TRAVERSAL            },
	{"rlfile",              OP_RL_FILE              },
	{"ambient",             OP_AMBIENT              },
	{"excellprim",          OP_EXCELL_PRIM          },
	{"excelldir",           OP_EXCELL_DIR           },
	{"excellsubdiv",        OP_EXCELL_SUBDIV        },
	{"hsublevel",           OP_HIER_SUBLEVEL        },
	{"hprim",               OP_HIER_PRIMNUM         },
	{"bfcull",              OP_PREVIEW_BKCULL       },
	{"fill",                OP_PREVIEW_FILL         },
	{"spheretess",          OP_PREVIEW_SPHTESS      },
	{"normdata",            OP_NORM_DB              },
	{"datatype",            OP_DATA_TYPE            },
	{"hu_maxprims",         OP_HU_MAX_PRIMS_CELL    },
	{"hu_gridsize",         OP_HU_GRIDSIZE          },
	{"hu_numbuckets",       OP_HU_NUMBUCKETS        },
	{"hu_maxsubdiv",        OP_HU_MAX_SUBDIV        },
	{"hu_lazy",             OP_HU_LAZY              },
	{"bundle",              OP_BUNDLE               },
	{"block",               OP_BLOCK                },
	{" ",                   OP_ERROR                },
	};



#define NUM_COMMANDS	(sizeof(cmdtab)/sizeof(CTE))



/*
 * NAME
 *	PrintEnv - print environment and lights to stdout
 *
 * SYNOPSIS
 *	VOID	PrintEnv()
 *
 * RETURNS
 *	Nothing.
 */

VOID	PrintEnv()
	{
	INT	i;
	LIGHT	*lp;

	printf("\nEnvironment:\n");
	printf("\tEye pos:   \t %f %f %f\n",      View.eye[0], View.eye[1], View.eye[2]);
	printf("\tCenter pos:\t %f %f %f\n",      View.coi[0], View.coi[1], View.coi[2]);
	printf("\tBackground:\t %f %f %f\n",      View.bkg[0], View.bkg[1], View.bkg[2]);
	printf("\tView Angle:\t %f\n",            View.vang);
	printf("\nAmbient Light:\t\t %f %f %f\n", View.ambient[0], View.ambient[1], View.ambient[2]);
	printf("\nLights:\n");

	lp = lights;
	for (i = 0; i < nlights; i++)
		{
		printf("\t[%ld] Pos:\t %f %f %f\n", i, lp->pos[0], lp->pos[1], lp->pos[2]);
		printf("\t    Col:\t %f %f %f\n",      lp->col[0], lp->col[1], lp->col[2]);
		printf("\t    Shadow:\t %ld\n",        lp->shadow);

		lp = lp->next;
		}

	printf("\n");
	printf("Options:\n");
	printf("\tTraversal:\t\t\t");

	switch (TraversalType)
		{
		case TT_LIST:
			printf("list\n");
			break;

		case TT_HUG:
			printf("uniform grid hierarchy\n");
			printf("\t\t\t\t\t   grid size    %ld\n", hu_gridsize);
			printf("\t\t\t\t\t   max prims    %ld\n", hu_max_prims_cell);
			printf("\t\t\t\t\t   max sublevel %ld\n", hu_max_subdiv_level);
			printf("\t\t\t\t\t   buckets      %ld\n", hu_numbuckets);
			printf("\t\t\t\t\t   lazy         %ld\n", hu_lazy);
			break;
		}

	printf("\tNormalization DB:\t\t");
	printf(ModelNorm ? "yes\n" : "no\n");

	printf("\tProjection type:\t\t");
	printf(View.projection == PT_PERSP ? "perspective\n" : "orthographic\n");

	printf("\tShadows:\t\t\t");
	printf(View.shad ? "on\n" : "off\n");

	printf("\tShading:\t\t\t");
	printf(View.shading ? "on\n" : "off\n");

	printf("\tResolution:\t\t\t%ld %ld\n",        Display.xres, Display.yres);
	printf("\tMin Ray Weight:\t\t\t%f\n",         Display.minweight);
	printf("\tMax Anti Subdivison Level:\t%ld\n", Display.maxAAsubdiv);
	printf("\tAnti tolerance:\t\t\t%f\n",         Display.aatolerance);

	printf("\tBundle: \t\t\t%ld %ld\n", bundlex, bundley);
	printf("\tBlock:  \t\t\t%ld %ld\n", blockx,  blocky);

	if (GeoFile)
		printf("\tGeometry file:\t\t\t%s\n", GeoFileName);

	if (PicFile)
		printf("\tImage file:\t\t\t%s\n",    PicFileName);

	printf("\n");
	}



/*
 * NAME
 *	InitEnv - setup the default environment variables
 *
 * SYNOPSIS
 *	VOID	InitEnv()
 *
 * RETURNS
 *	Nothing.
 */

VOID	InitEnv()
	{
	/* Default eye position. */
	View.eye[0] =  0.5;
	View.eye[1] =  0.5;
	View.eye[2] = -1.5;

	/* Default center position. */
	View.coi[0] =  0.5;
	View.coi[1] =  0.5;
	View.coi[2] =  0.5;

	/* Default background color. */
	View.bkg[0] =  0.0;
	View.bkg[1] =  0.0;
	View.bkg[2] =  0.8;

	/* Default viewing angle. */
	View.vang   =  60.0;

	/* Default ambient light. */
	View.ambient[0] = 0.1;
	View.ambient[1] = 0.1;
	View.ambient[2] = 0.1;

	/* Shadows and shading. */
	View.shad    = TRUE;
	View.shading = TRUE;

	/* Perspective projection. */
	View.projection = PT_PERSP;

	/*  Default resolution is 100 x 100  */
	Display.xres = 100;
	Display.yres = 100;
	Display.numpixels = 100*100;

	/* Default maximum raytrace level. */
	Display.maxlevel = 5;

	/* Default anti-aliasing. */
	Display.maxAAsubdiv = 0;
	Display.aatolerance = .020;
	Display.aarow = MAX_AA_ROW - 1;
	Display.aacol = MAX_AA_COL - 1;

	/* Default ray minimum weight is 0.001. */
	Display.minweight = 0.001;

	/* Default screen size. */
	Display.scrDist   = 164.5;
	Display.scrWidth  = 190.0;
	Display.scrHeight = 190.0;

	Display.scrHalfWidth  = Display.scrWidth * 0.5;
	Display.scrHalfHeight = Display.scrHeight* 0.5;

	Display.vWscale = Display.scrWidth/(REAL)Display.xres;
	Display.vHscale = Display.scrHeight/(REAL)Display.yres;

	/* Init transformation matrices. */
	MatrixIdentity(View.vtrans);
	MatrixIdentity(View.model);

	GeoFile        = FALSE;
	PicFile        = FALSE;
	ModelNorm      = TRUE;
	ModelTransform = FALSE;
	DataType       = DT_ASCII;
	TraversalType  = TT_LIST;

	/* Init statistics.
	 *
	 *	Stats.total_rays	  = 0;
	 *	Stats.prim_rays 	  = 0;
	 *	Stats.shad_rays 	  = 0;
	 *	Stats.shad_rays_not_hit   = 0;
	 *	Stats.shad_rays_hit	  = 0;
	 *	Stats.shad_coherence_rays = 0;
	 *	Stats.refl_rays 	  = 0;
	 *	Stats.trans_rays	  = 0;
	 *	Stats.aa_rays		  = 0;
	 *	Stats.bound_tests	  = 0;
	 *	Stats.bound_hits	  = 0;
	 *	Stats.bound_misses	  = 0;
	 *	Stats.ray_obj_hit	  = 0;
	 *	Stats.ray_obj_miss	  = 0;
	 *	Stats.max_tree_depth	  = 0;
	 *	Stats.max_objs_ray	  = 0;
	 *	Stats.max_rays_pixel	  = 0;
	 *	Stats.max_objs_pixel	  = 0;
	 *	Stats.total_objs_tested   = 0;
	 *	Stats.coverage		  = 0;
	 */

	lights	= (LIGHT*)GlobalMalloc(sizeof(LIGHT), "env.c");

	bundlex = 25;
	bundley = 25;
	blockx	= 2;
	blocky	= 2;
	}




/*
 * NAME
 *	InitLights - setup the default light position and color
 *
 * SYNOPSIS
 *	VOID	InitLights()
 *
 * RETURNS
 *	Nothing.
 */

VOID	InitLights()
	{
	nlights = 1;

	lights->pos[0] =  0.0;
	lights->pos[1] =  0.0;
	lights->pos[2] = -2000.0;
	lights->pos[3] =  1.0;
	lights->col[0] =  1.0;
	lights->col[1] =  1.0;
	lights->col[2] =  1.0;
	lights->shadow =  TRUE;
	lights->next   =  NULL;
	}



/*
 * NAME
 *	InitDisplay - setup display parameters
 *
 * SYNOPSIS
 *	VOID	InitDisplay()
 *
 * RETURNS
 *	Nothing.
 */

VOID	InitDisplay()
	{
	REAL	aspect; 		/* Resolution aspect ratio.	     */
	REAL	theta;			/* View angle in radians.	     */

	aspect = (REAL)Display.xres/(REAL)Display.yres;
	theta  = (View.vang*0.5)*0.0175;

	Display.scrWidth      = Display.scrHeight*aspect;
	Display.scrDist       = (0.5*Display.scrHeight)/tan(theta);
	Display.scrHalfWidth  = Display.scrWidth*0.5;
	Display.scrHalfHeight = Display.scrHeight*0.5;
	Display.vWscale       = Display.scrWidth/(REAL)Display.xres;
	Display.vHscale       = Display.scrHeight/(REAL)Display.yres;
	}



/*
 * NAME
 *	VerifyColorRange - verify if a color value lies between 0 and 1
 *
 * SYNOPSIS
 *	BOOL	VerifyColorRange(c)
 *	COLOR	c;
 *
 * RETURNS
 *	TRUE if color is within range, FALSE otherwise.
 */

BOOL	VerifyColorRange(COLOR c)
	{
	if (c[0] < 0.0 || c[0] > 1.0  ||
	    c[1] < 0.0 || c[1] > 1.0  ||
	    c[2] < 0.0 || c[2] > 1.0)
		{
		printf("Invalid color %f %f %f.\n", c[0], c[1], c[2]);
		return (FALSE);
		}
	else
		return (TRUE);
	}



/*
 * NAME
 *	TransformLights - transform lights to a new position given a matrix
 *
 * SYNOPSIS
 *	VOID	TransformLights(m)
 *	MATRIX	m;			// Transformation matrix.
 *
 * RETURNS
 *	Nothing.
 */

VOID	TransformLights(MATRIX m)
	{
	INT	i;
	LIGHT	*lp;

	lp = lights;
	for (i = 0; i < nlights; i++)
		{
		VecMatMult(lp->pos, m, lp->pos);
		lp = lp->next;
		}
	}



/*
 * NAME
 *	ViewRotate - compute transform that aligns direction x,y,z with +z axis
 *
 * SYNOPSIS
 *	VOID	ViewRotate(M, x, y, z)
 *	MATRIX	M;
 *	REAL	x, y, z;
 *
 * RETURNS
 *	Nothing.
 */

VOID	ViewRotate(MATRIX M, REAL x, REAL y, REAL z)
	{
	REAL	r, rx;

	rx = sqrt(x*x + z*z);

	if (ABS(rx) < RAYEPS)
		{
		MatrixIdentity(M);
		Rotate(X_AXIS, M, PI_over_2 * (Sign(y)));
		return;
		}

	r = sqrt(x*x + y*y + z*z);

	M[0][0] = z/rx;
	M[0][1] = -x*y/(r*rx);
	M[0][2] = x/r;
	M[0][3] = 0.0;

	M[1][0] = 0.0;
	M[1][1] = rx/r;
	M[1][2] = y/r;
	M[1][3] = 0.0;

	M[2][0] = -x/rx;
	M[2][1] = -y*z/(r*rx);
	M[2][2] = z/r;
	M[2][3] = 0.0;

	M[3][0] = 0.0;
	M[3][1] = 0.0;
	M[3][2] = 0.0;
	M[3][3] = 1.0;
	}



/*
 * NAME
 *	CreateViewMatrix - compute view transform matrix and put in View.vtrans
 *
 * SYNOPSIS
 *	VOID	CreateViewMatrix()
 *
 * NOTES
 *	Taken from David Kurlander's program.
 *
 * RETURNS
 *	Nothing.
 */

VOID	CreateViewMatrix()
	{
	MATRIX	T, R;				/* Temporary matrices. */

	/* Put eye at origin. */

	Translate(T, -View.eye[0], -View.eye[1], -View.eye[2]);
	MatrixMult(View.vtrans, View.vtrans, T);

	/* Align view direction with Z axis. */

	ViewRotate(R, View.coi[0] - View.eye[0], View.coi[1] - View.eye[1], View.coi[2] - View.eye[2]);
	MatrixMult(View.vtrans, View.vtrans, R);
	}



/*
 * NAME
 *	TransformViewRay - put ray back in world coordinate system from view coordinate system
 *
 * SYNOPSIS
 *	VOID	TransformViewRay(tray)
 *	POINT	tray;			// Ray.
 *
 * RETURNS
 *	Nothing.
 */

VOID	TransformViewRay(POINT tray)
	{
	VecMatMult(tray, View.vtransInv, tray);
	}



/*
 * NAME
 *	NormalizeEnv - normalize eye, center of intersect and light positions
 *
 * SYNOPSIS
 *	VOID	NormalizeEnv(normMat)
 *	MATRIX	normMat;		// Normalization matrix.
 *
 * RETURNS
 *	Nothing.
 */

VOID	NormalizeEnv(MATRIX normMat)
	{
	View.eye[3] = 1.0;
	VecMatMult(View.eye, normMat, View.eye);

	View.coi[3] = 1.0;
	VecMatMult(View.coi, normMat, View.coi);

	TransformLights(normMat);
	}



/*
 * NAME
 *	LookupCommand - find environment command in table and return opcode
 *
 * SYNOPSIS
 *	CHAR	LookupCommand(s)
 *	CHAR	*s;
 *
 * RETURNS
 *	The corresponding opcode.
 */

CHAR	LookupCommand(CHAR *s)
	{
	INT	i;

	for (i = 0; i < NUM_COMMANDS; i++)
		if (strcmp(s, cmdtab[i].command) == 0)
			return (cmdtab[i].opcode);

	printf("\n\nInvalid command string %s.\n", s);
	return (OP_ERROR);
	}



/*
 * NAME
 *	ReadEnvFile - read and parse environment file
 *
 * SYNOPSIS
 *	VOID	ReadEnvFile(EnvFileName)
 *	CHAR	*EnvFileName;		// Environment filename.
 *
 * RETURNS
 *	Nothing.
 */

VOID	ReadEnvFile(CHAR *EnvFileName)
	{
	INT	i, j;				/* Indices.		     */
	INT	stat;				/* Input var status counter. */
	INT	dummy;
	CHAR	opcode; 			/* Environment spec opcode.  */
	CHAR	command[30];			/* Environment spec command. */
	CHAR	opparam[30];			/* Command parameter.	     */
	CHAR	dummy_char[60];
	CHAR	datafile[10];
	BOOL	lights_set;			/* Lights set?		     */
	FILE	*pf;				/* Input file pointer.	     */
	LIGHT	*lptr, *lastlight;		/* Light node pointers.      */


	/* Open command file. */

	pf = fopen(EnvFileName, "r");
	if (!pf)
		{
		printf("Unable to open environment file %s.\n", EnvFileName);
		exit(-1);
		}

	InitEnv();			/* Set defaults. */

	nlights    = 0;
	lights_set = FALSE;


	/* Process command file according to opcodes. */

	while (fscanf(pf, "%s", command) != EOF)
		{
		opcode = LookupCommand(command);

		switch (opcode)
			{
			/* Eye position. */
			case OP_EYE:
				stat = fscanf(pf, "%lf %lf %lf", &(View.eye[0]), &(View.eye[1]), &(View.eye[2]));
				if (stat != 3)
					{
					printf("error: eye position.\n");
					exit(-1);
					}
				break;

			/* Center of interest position. */
			case OP_COI:
				stat = fscanf(pf, "%lf %lf %lf", &(View.coi[0]), &(View.coi[1]), &(View.coi[2]));
				if (stat != 3)
					{
					printf("error: coi position.\n");
					exit(-1);
					}
				break;

			/* Background color. */
			case OP_BKGCOL:
				stat = fscanf(pf, "%lf %lf %lf", &(View.bkg[0]), &(View.bkg[1]), &(View.bkg[2]));
				if (stat != 3)
					{
					printf("error: background color.\n");
					exit(-1);
					}

				if (!VerifyColorRange(View.bkg))
					exit(-1);
				break;

			/* Viewing angle in degrees. */
			case OP_VANG:
				stat = fscanf(pf, "%lf", &(View.vang));
				if (stat != 1)
					{
					printf("error: viewing angle.\n");
					exit(-1);
					}

				if (View.vang < 0.0 || View.vang > 100.0)
					{
					printf("Invalid angle %f.\n", View.vang);
					exit(-1);
					}
				break;

			/* Ambient. */
			case OP_AMBIENT:
				stat = fscanf(pf, "%lf %lf %lf", &(View.ambient[0]), &(View.ambient[1]), &(View.ambient[2]));
				if (stat != 3)
					{
					printf("error: ambient.\n");
					exit(-1);
					}

				if (!VerifyColorRange(View.ambient))
					exit(-1);
				break;

			/* Anti-aliasing level. */
			case OP_ANTILEVEL:
				stat = fscanf(pf, "%ld", &(Display.maxAAsubdiv));
				if (stat != 1)
					{
					printf("View error: antialias level.\n");
					exit(-1);
					}

				if (Display.maxAAsubdiv < 0 || Display.maxAAsubdiv > 3)
					{
					printf("error: antialias level %ld.\n", Display.maxAAsubdiv);
					exit(-1);
					}
				break;

			/* Recursion level. */
			case OP_MAXLEVEL:
				stat = fscanf(pf, "%ld", &(Display.maxlevel));
				printf("maxlevel of ray recursion = %ld\n",Display.maxlevel);
				fflush(stdout);
				if (stat != 1)
					{
					printf("error: recursion level.\n");
					exit(-1);
					}

				if (Display.maxlevel > 5 || Display.maxlevel < 0)
					{
					printf("error: recursion level %ld.\n", Display.maxlevel);
					exit(-1);
					}
				break;

			/* Mininum ray weight. */
			case OP_MINWEIGHT:
				stat = fscanf(pf, "%lf", &(Display.minweight));
				if (stat != 1)
					{
					printf("error: miniumum ray weight.\n");
					exit(-1);
					}

				if (Display.minweight < 0.0 || Display.minweight > 1.0)
					{
					printf("error: invalid ray weight %f.\n", Display.minweight);
					exit(-1);
					}
				break;

			/* Anti tolerance weight. */
			case OP_ANTITOL:
				stat = fscanf(pf, "%lf", &(Display.aatolerance));
				if (stat != 1)
					{
					printf("error: anti tolerance weight.\n");
					exit(-1);
					}

				if (Display.aatolerance < 0.0 || Display.aatolerance > 1.0)
					{
					printf("error: invalid anti tolerance weight %f.\n", Display.aatolerance);
					exit(-1);
					}
				break;

			/* Resolution. */
			case OP_RES:
				stat = fscanf(pf, "%ld %ld", &(Display.xres), &(Display.yres));
				if (stat != 2)
					{
					printf("error: resolution.\n");
					exit(-1);
					}
				break;

			/* Light positions and colors. */
			case OP_LIGHT:
				lights_set = TRUE;
				if (nlights > 0)
					lptr = (LIGHT*)GlobalMalloc(sizeof(LIGHT), "env.c");
				else
					lptr = lights;

				stat = fscanf(pf, "%lf %lf %lf %lf %lf %lf",
					      &(lptr->pos[0]),
					      &(lptr->pos[1]),
					      &(lptr->pos[2]),
					      &(lptr->col[0]),
					      &(lptr->col[1]),
					      &(lptr->col[2]));

				if (stat != 6)
					{
					printf("error: Lights.\n");
					exit(-1);
					}

				if (!VerifyColorRange(lptr->col))
					exit(-1);

				lptr->pos[3] = 1.0;
				stat = fscanf(pf, "%ld", &(lptr->shadow));
				if (stat != 1)
					{
					printf("error: Lights shadow indicator.\n");
					exit(-1);
					}

				lptr->next = NULL;
				if (nlights > 0)
					lastlight->next = lptr;

				nlights++;
				lastlight = lptr;
				break;

			/* Model transformation matrix. */
			case OP_MODELMAT:
				for (i = 0; i < 4; i++)
					for (j = 0; j < 4; j++)
						{
						stat = fscanf(pf, "%lf", &(View.model[i][j]));
						if (stat != 1)
							{
							printf("Error in matrix.\n");
							exit(-1);
							}
						}

				ModelTransform = TRUE;
				break;

			/* Shadow info. */
			case OP_SHAD:
				stat = fscanf(pf, "%s", opparam);
				if (stat != 1)
					{
					printf("error: shadow.\n");
					exit(-1);
					}

				if (strcmp(opparam, "on") == 0)
					View.shad = TRUE;
				else
					View.shad = FALSE;
				break;

			/* Shading info. */
			case OP_SHADING:
				stat = fscanf(pf, "%s", opparam);
				if (stat != 1)
					{
					printf("error: shading %s.\n", opparam);
					exit(-1);
					}

				if (strcmp(opparam, "on") == 0)
					View.shading = TRUE;
				else
					View.shading = FALSE;
				break;

			/* Projection type. */
			case OP_PROJECT:
				stat = fscanf(pf, "%s", opparam);
				if (stat != 1)
					{
					printf("error: projection %s.\n", opparam);
					exit(-1);
					}

				if (strcmp(opparam, "perspective") == 0)
					View.projection = PT_PERSP;
				else
				if (strcmp(opparam, "orthographic") == 0)
					View.projection = PT_ORTHO;
				else
					{
					printf("Invalid projection %s.\n", opparam);
					exit(-1);
					}
				break;

			/* Database traversal info. */
			case OP_TRAVERSAL:
				stat = fscanf(pf, "%s", opparam);
				if (stat != 1)
					{
					printf("error: traversal %s.\n", opparam);
					exit(-1);
					}

				if (strcmp(opparam, "list") == 0)
					TraversalType = TT_LIST;
				else
				if (strcmp(opparam, "huniform") == 0)
					TraversalType = TT_HUG;
				else
					{
					printf("Invalid traversal code %s.\n", opparam);
					exit(-1);
					}
				break;

			/* Geometry file. */
			case OP_GEOM_FILE:
				stat = fscanf(pf, " %s", GeoFileName);
				if (stat != 1)
					{
					printf("error: geometry file.\n");
					exit(-1);
					}

				GeoFile = TRUE;
				break;

			/* Runlength file. */
			case OP_RL_FILE:
				stat = fscanf(pf, " %s", PicFileName);
				if (stat != 1)
					{
					printf("error: runlength file.\n");
					exit(-1);
					}

				PicFile = TRUE;
				break;

			case OP_PREVIEW_BKCULL:
				stat = fscanf(pf, "%ld", &dummy);
				if (stat != 1)
					{
					printf("error: Preview bkcull.\n");
					exit(-1);
					}
				break;

			case OP_PREVIEW_FILL:
				stat = fscanf(pf, "%ld", &dummy);
				if (stat != 1)
					{
					printf("error: Preview fill.\n");
					exit(-1);
					}
				break;

			case OP_PREVIEW_SPHTESS:
				stat = fscanf(pf, "%s", dummy_char);
				if (stat != 1)
					{
					printf("error: sphere tess.\n");
					exit(-1);
					}
				break;

			case OP_NORM_DB:
				stat = fscanf(pf, "%s", opparam);
				if (stat != 1)
					{
					printf("error: norm database.\n");
					exit(-1);
					}

				if (strcmp(opparam, "no") == 0)
					ModelNorm = FALSE;

				break;

			case OP_DATA_TYPE:
				stat = fscanf(pf, "%s", datafile);
				if (stat != 1)
					{
					printf("error: datatype.\n");
					exit(-1);
					}

				if (strcmp(datafile, "binary") == 0)
					DataType = DT_BINARY;

				break;

			case OP_HU_MAX_PRIMS_CELL:
				stat = fscanf(pf, "%ld", &hu_max_prims_cell);
				if (stat != 1)
					{
					printf("error: Huniform prims per cell.\n");
					exit(-1);
					}
				break;

			case OP_HU_GRIDSIZE:
				stat = fscanf(pf, "%ld", &hu_gridsize);
				if (stat != 1)
					{
					printf("error: Huniform gridsize.\n");
					exit(-1);
					}
				break;

			case OP_HU_NUMBUCKETS:
				stat = fscanf(pf, "%ld", &hu_numbuckets);
				if (stat != 1)
					{
					printf("error: Huniform numbuckets.\n");
					exit(-1);
					}
				break;

			case OP_HU_MAX_SUBDIV:
				stat = fscanf(pf, "%ld", &hu_max_subdiv_level);
				if (stat != 1  || hu_max_subdiv_level > 3)
					{
					printf("error: Huniform max subdiv level.\n");
					exit(-1);
					}
				break;

			case OP_HU_LAZY:
				stat = fscanf(pf, "%ld", &hu_lazy);
				if (stat != 1)
					{
					printf("error: Huniform lazy.\n");
					exit(-1);
					}
				break;

			case OP_BUNDLE:
				stat = fscanf(pf, "%ld %ld", &bundlex, &bundley);
				if (stat != 2 )
					{
					printf("error: bundle.\n");
					exit(-1);
					}
				break;

			case OP_BLOCK:
				stat = fscanf(pf, "%ld %ld", &blockx, &blocky);
				if (stat != 2 )
					{
					printf("error: block.\n");
					exit(-1);
					}
				break;

			default:
				printf("Warning: unrecognized env command: %s.\n", command);
				break;
			}
		}

	fclose(pf);

	/* Display parameters reset. */
	Display.numpixels = Display.xres*Display.yres;
	Display.vWscale   = Display.scrWidth/Display.xres;
	Display.vHscale   = Display.scrHeight/Display.yres;


	/* If no light information given, set default. */
	if (!lights_set)
		InitLights();

	/* Set up screen parameters. */
	InitDisplay();

	/* Parameter check; think about lifting this restriction. */
	if ((TraversalType != TT_LIST) && ModelNorm == FALSE)
		{
		printf("Data must be normalized with this traversal method!.\n");
		ModelNorm = TRUE;
		}
	}

