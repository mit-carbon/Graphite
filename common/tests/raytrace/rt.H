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
 *	rt.h
 *
 * DESCRIPTION
 *	This file contains all the general definitions for constants, macros,
 *	data types, data structures, and variables global to the ray tracer
 *	(but not necessarily in global shared memory).
 *
 */


#define huge

/*
 *	Bring in the correct parallel environment header.
 */

#ifdef	MAIN
MAIN_ENV
#else
EXTERN_ENV
#endif

#define M_PI_2	1.57079632679489661923

/*
 *	Define general constants.
 */

#define NO		0
#define OFF		0
#define FALSE		0
#define YES		1
#define ON		1
#define TRUE		1

#define VOID		void

#define X_AXIS		1		/* Rotation axis.		     */
#define Y_AXIS		2
#define Z_AXIS		3

#define MAX_X		1280		/* Max # pixels along x-axis.	     */
#define MAX_Y		1024		/* Max # pixels along y-axis.	     */
#define MAX_PROCS	4096		/* Max # of processors. 	     */
#define MAX_VERTS	100		/* Max # of vertices in a polygon.   */
#define MAX_LIGHTS	20		/* Max # of lights in a scene.	     */
#define MAX_AA_ROW	9		/* Max antialias super sample is 9x9 */
#define MAX_AA_COL	9

#define INT_PAD_SIZE	256		/* padding size for 32-bit
	   quantities to avoid false-sharing. Pads are inserted in two
	   situations: (i) when variables are really private to a processor
	   but are declared in a shared array owing to the lack of a private
	   space in the sproc lightweight threads model, and (ii) when
	   there are simple situations of "control" variables that are written
	   by only one processor but read by several, and declared in a
	   shared array indexed by process id, e.g. wpstat and workpool
	   declared in this file */

#define MAX_SUBDIV_LEVEL 3		/* Max HUG subdivision level.	     */
#define MAX_RAYINFO	(MAX_SUBDIV_LEVEL + 1)

#define NAME_LEN	32
#define ISECT_MAX	2


#define RAYEPS		1e-07		/* Roundoff error tolerance.	     */
#define HUGE_REAL	1e+32		/* A number we consider infinity.    */
#if 0
#define PI		3.141592654
#define PI_over_2	1.570796327
#else
#define PI_over_2	M_PI_2
#endif
#define PIINV		0.318309886
#define TWOPIINV	0.159154943
#define LOG_CONV	3.321928094	/* Log base 2 of 10 (for base conv). */


/*
 *	Define database traversal type codes.
 */

#define TT_LIST 	0		/* Linked list traversal.	     */
#define TT_HUG		1		/* Hierarchical uniform grid trav.   */


/*
 *	Define data file type codes.
 */

#define DT_ASCII	0		/* Data is in ascii format.	     */
#define DT_BINARY	1		/* Data is in binary format.	     */

/*
 *	Define work pool status codes.
 */

#define WPS_EMPTY	0		/* Work pool is empty.		     */
#define WPS_VALID	1		/* Valid job is available.	     */


/*
 *	Define ray tree status codes.
 */

#define RTS_EMPTY	0		/* Ray tree stack is empty.	     */
#define RTS_VALID	1		/* Valid ray job is available.	     */


/*
 *	Define projection type codes.
 */

#define PT_PERSP	0		/* Perspective projection.	     */
#define PT_ORTHO	1		/* Orthographic projection.	     */


/*
 *	Define object types for ObjectMalloc() and ObjectFree().
 */

#define OT_GRID 	0
#define OT_VOXEL	1
#define OT_HASHTABLE	2
#define OT_EMPTYCELLS	3
#define OT_PRUNEDCELLS	4
#define OT_PELLIST	5
#define OT_BINTREE	6
#define OT_PEPARRAY	7


/*
 *	Define cell status values.
 */

#define MSB		(((UINT)1) << (sizeof(UINT)*8 - 1))
#define NONEMPTY	0
#define EMPTY		1
#define UNPRUNED	0
#define PRUNED		1
#define NO_STEP 	0
#define STEP		1


/*
 *	Define voxel types.
 */

#define LOCAL_VOXEL	0		/* Not currently in use.	     */
#define LOCAL_GRID	1		/* Not currently in use.	     */
#define GSM_VOXEL	2
#define GSM_GRID	3
#define REMOTE_VOXEL	4
#define REMOTE_GRID	5


/*
 *	Define ray status values.
 */

#define EXITED_WORLD	0
#define SENT_TO_REMOTE	1
#define IN_WORLD	2



/*
 *	Define general macros.
 */

//#define Min(A, B)		( (A) < (B) ? (A) :  (B) )
#define Min(A, B)		( (A) - (B) < 0.0 ? (A) :  (B) )

//#define Max(A, B)		( (A) > (B) ? (A) :  (B) )
#define Max(A, B)		( (A) - (B) > 0.0 ? (A) :  (B) )

#define ABS(A)			( (A) > 0.0 ? (A) : -(A) )

#define Sign(A) 		( (A) > 0.0 ? 1.0 : -1.0 )


/*
 *	Define vector manipulation macros.
 */

#define VecZero(A)		{ A[0] = 0.0;  A[1] = 0.0;  A[2] = 0.0; }

#define VecLen(A)		( sqrt( A[0]*A[0] + A[1]*A[1] + A[2]*A[2] ))

#define VecDot(A, B)		( A[0]*B[0] + A[1]*B[1] + A[2]*B[2] )

#define VecCross(C, A, B)	{ C[0] = A[1]*B[2] - A[2]*B[1]; \
				  C[1] = A[2]*B[0] - A[0]*B[2]; \
				  C[2] = A[0]*B[1] - A[1]*B[0]; }

#define VecNegate(B, A) 	{ B[0] = -A[0];  B[1] = -A[1];	B[2] = -A[2]; }

#define VecCopy(B, A)		{ B[0] =  A[0];  B[1] =  A[1];	B[2] =	A[2]; }

#define VecAdd(C, A, B) 	{ C[0] = A[0] + B[0]; \
				  C[1] = A[1] + B[1]; \
				  C[2] = A[2] + B[2]; }

#define VecSub(C, A, B) 	{ C[0] = A[0] - B[0]; \
				  C[1] = A[1] - B[1]; \
				  C[2] = A[2] - B[2]; }


#define VecScale(B, scale, A)	{ B[0] = scale * A[0]; \
				  B[1] = scale * A[1]; \
				  B[2] = scale * A[2]; }


/*
 *	Define ray trace macros.
 */

#define IsectAdd(hit, tval, P)	{ (hit)->t = tval;  \
				  (hit)->pelem = P; }

#define RayPoint(pi, ray, tval) { pi[0] = ray->P[0] + ray->D[0]*tval; \
				  pi[1] = ray->P[1] + ray->D[1]*tval; \
				  pi[2] = ray->P[2] + ray->D[2]*tval; }



/*
 *	Define data types.
 */

typedef 		char		CHAR;
typedef 		char		S8;
typedef unsigned	char		UCHAR;
typedef unsigned	char		U8;

typedef 		short		SHORT;
typedef 		short		S16;
typedef unsigned	short		USHORT;
typedef unsigned	short		U16;

typedef 		long		INT;
typedef unsigned	long		UINT;
typedef unsigned	long		BOOL;

typedef 		long		LONG;
typedef 		long		S32;
typedef unsigned	long		ULONG;
typedef unsigned	long		U32;

typedef 		float		FLOAT;
typedef 		float		R32;
typedef 		double		DOUBLE;
typedef 		double		R64;
typedef 		double		REAL;


typedef REAL	UV[2];			/* A u, v coordinate.		     */
typedef REAL	VEC3[3];		/* A 3-vector.			     */
typedef REAL	VEC4[4];		/* A 4-vector.			     */
typedef REAL	MATRIX[4][4];		/* A 4x4 matrix.		     */

typedef VEC4	POINT;			/* An x, y, z, w point. 	     */
typedef VEC3	COLOR;			/* An r, g, b color.		     */



/*
 *	Define primitive procedures.
 */

typedef struct	pprocs
	{
	CHAR	    *(*name)(); 	/* Primitive name.		     */
	VOID	    (*print)(); 	/* Primitive print.		     */
	VOID	    (*read)();		/* Read from model file.	     */
	VOID	    (*binread)();	/* Binary read from model file.      */
	VOID	    (*transform)();	/* Transform primitive. 	     */
	INT	    (*intersect)();	/* Intersect object with a ray.      */
	INT	    (*pe_intersect)();	/* Intersect primelement with a ray. */
	VOID	    (*normal)();	/* Compute normal vector.	     */
	VOID	    (*normalize)();	/* Data normalization to unit cube.  */
	VOID	    (*bbox)();		/* Bounding box constructor.	     */
	}
	PPROCS;


/*
 *	Define surface properties structure.
 */

typedef struct	surf
	{
	COLOR		fcolor; 	/* Front facing color.		     */
	COLOR		bcolor; 	/* Back facing color.		     */
	REAL		kdiff;		/* Diffuse coefficient. 	     */
	REAL		kspec;		/* Specular coefficient.	     */
	REAL		ktran;		/* Transmission coefficient.	     */
	REAL		refrindex;	/* Index of refraction. 	     */
	REAL		kspecn; 	/* Specular distribution coeff.      */
	}
	SURF;



/*
 *	Define light source structure.
 */

typedef struct	light
	{
	VEC4		pos;		/* Light position.		     */
	COLOR		col;		/* Light color. 		     */
	BOOL		shadow; 	/* Send shadow rays?		     */
	struct light	*next;		/* Ptr to next light.		     */
	}
	LIGHT;



/*
 *	Define pixel data structure.
 */

typedef struct	pixel
	{
	R32	r;
	R32	g;
	R32	b;
	}
	PIXEL;



/*
 *	Define voxel structure.
 */

typedef struct voxel
	{
	INT		id;		/* id = index1D 		     */
	INT		numelements;	/* Number of elements in voxel.      */
	struct voxel	*next;		/* Hashtable bucket list.	     */
	CHAR		*cell;		/* Ptr to grid or ptr to ElemPtr list*/
	CHAR		celltype;	/* 0 => local voxel, 1 => local grid */
					/* 2 => GSM voxel, 3 => GSM grid     */
					/* 4 => remote voxel, 5 =>remote grid*/
	CHAR		dummy[sizeof(INT) - 1];
	}
	VOXEL;



/*
 *	Define structure for bounding box (slab approach).
 */

typedef struct	bbox
	{
	REAL	dnear[3];
	REAL	dfar[3];
	}
	BBOX;



/*
 *	Define viewing variables.
 */

typedef struct	view
	{
	POINT	eye;			/* Eye position.		     */
	POINT	coi;			/* Center position		     */
	MATRIX	vtrans; 		/* Viewing transformation	     */
	MATRIX	vtransInv;		/* Inverse viewing transformation    */
	MATRIX	model;			/* Global model transformation	     */
	COLOR	bkg;			/* Background color.		     */
	INT	projection;		/* Projection type.		     */
	REAL	vang;			/* View angle.			     */
	COLOR	ambient;		/* Ambient light.		     */
	BOOL	shad;			/* Shadow indicator.		     */
	BOOL	shading;		/* Shading indicator.		     */
	}
	VIEW;


/*
 *	Define display variables.
 */

typedef struct	display
	{
	INT	maxlevel;	/* Maximum raytrace level.		     */
	INT	maxAAsubdiv;	/* Maximum antialiasing subdivision.	     */
	INT	aarow;		/* Max anti row index for super-sampling.    */
	INT	aacol;		/* Max anti column index for super-sampling. */
	REAL	aatolerance;	/* Antialiasing tolerance color difference.  */
	INT	xres,yres;	/* Display resolution.			     */
	INT	numpixels;	/* Total number of pixels in framebuffer.    */
	REAL	minweight;	/* Minimum ray weight.			     */
	REAL	scrDist;	/* Screen distance from eye.		     */
	REAL	scrWidth;	/* Screen width.			     */
	REAL	scrHeight;	/* Screen height.			     */
	REAL	scrHalfWidth;	/* Screen half width.			     */
	REAL	scrHalfHeight;	/* Screen half height.			     */
	REAL	vWscale;	/* Screen width scale.			     */
	REAL	vHscale;	/* Screen height scale. 		     */
	PIXEL	*framebuffer;	/* Ptr to the framebuffer.		     */
	}
	DISPLAY;



/*
 *	Define structure for a primitive element.
 */

typedef struct element
	{
	INT		index;
	struct object	*parent;	/* Ptr back to parent object.	     */
	CHAR		*data;		/* Pointer to data info.	     */
	BBOX		bv;		/* Element bounding volume.	     */
	}
	ELEMENT;


/*
 *	Define structure for a primitive object.
 */

typedef struct object
	{
	INT		index;
	CHAR		name[NAME_LEN]; /* Name of object.		     */
	BBOX		bv;		/* Bound volume.		     */
	ELEMENT 	*pelem; 	/* Pointer to prim element list.     */
	INT		numelements;	/* Number of primitive elements.     */
	PPROCS		*procs; 	/* Pointer to primitive procs.	     */
	SURF		*surf;		/* Pointer to surface properties.    */
	struct object	*next;		/* Next primitive (linked list).     */
	}
	OBJECT;


/*
 *	Define intersection record structure.
 */

typedef struct	irecord
	{
	REAL	t;			/* Line parameter at intersection.   */
	ELEMENT *pelem; 		/* Primitve element.		     */
	REAL	b1;			/* Barycentric coordinates of	     */
	REAL	b2;			/* intersection for triangles.	     */
	REAL	b3;
	}
	IRECORD;



/*
 *	Define binary tree node structure.
 */

typedef struct btnode
	{
	ELEMENT **pe;		/* Array of primitive element ptrs in node.  */
	REAL	p[3];		/* Lower left corner of bounding box	     */
				/* of space represented by node.	     */
	INT	nprims; 	/* # prims in node primElem list.	     */
	INT	n[3];		/* Gridsizes for this box.		     */
	INT	i[3];		/* Indices of cell (lower left corner	     */
				/* if not a leaf) in grid.		     */
	INT	axis;		/* subdiv axis, 0,1,2 => x,y,z		     */
	INT	totalPrimsAllocated;  /* This is used for garbage allocation.*/
	struct	btnode *btn[2]; /* Ptrs to children.			     */
	}
	BTNODE;



typedef struct grid
	{
	INT		id;
	VOXEL		**hashtable;	/* hashtable[ num_buckets ] is indexed	*/
					/* by index1D mod num_buckets,		*/
					/* num_buckets and n, the  # of cells/	*/
					/* axis, should be relatively prime,	*/
					/* grids at different  levels may	*/
					/* have different num_buckets.		*/
	UINT		*emptycells;	/* emptycells[ ceil( NumCells		*/
					/* sizeof(unsigned) ) ], a packed	*/
					/* array of bits indicating for 	*/
					/* each cell if it is empty,		*/
					/* 1 => empty,				*/
					/* grids at different levels may	*/
					/* have different NumCells.		*/
	ELEMENT 	**pepa; 	/* prim element pointer list		*/
	INT		num_prims;	/* number of prims on prim element	*/
					/* list 				*/
	INT		indx_inc[3];	/* if n is # of cells per axis, 	*/
					/* NumCells is n**3,			*/
					/* indx_inc[0,1,2] = 1, n, n**2;	*/
					/* inc for index1D.			*/
	INT		num_buckets;	/* # buckets in hashtable		*/
	REAL		min[3]; 	/* cell min boundary, world coord	*/
	REAL		cellsize[3];	/* cellsize of voxels in this grid	*/
					/* in world coord			*/
	INT		subdiv_level;	/* # levels of space subdiv		*/
					/* to reach this grid,			*/
					/* 0 is top level.			*/
	BTNODE		*bintree;	/* root of bintree for this grid	*/
	struct grid	*next;		/* grid list				*/
	}
	GRID;



/*
 *	RAYINFO is ray information that is specific to a particular grid and
 *	is pushed onto a stack when the ray descends a level in the space
 *	subdivision heirarchy.	Distances are in world coordinate units.
 */

typedef struct	rayinfo
	{
	GRID	*grid;		/* Grid for this rayinfo.		     */
	REAL	d[3];		/* Dist along ray from world coord origin to */
				/* next voxel boundary. 		     */
	INT	entry_plane;	/* Entry plane for current voxel,	     */
				/* (0,1,2) => (x,y,z).			     */
	REAL	t_in;		/* Dist along ray from world coord origin to */
				/* entry point of current voxel.	     */
	INT	exit_plane;	/* Exit plane for current voxel,	     */
				/* (0,1,2) => (x,y,z).			     */
	REAL	t_out;		/* Dist along ray from world coord origin to */
				/* exit point of current voxel. 	     */
	REAL	delta[3];	/* Dist along ray between voxel boundaries.  */
	INT	index3D[3];	/* Current cell in cell units wrt grid origin*/
	INT	index1D;	/* Index1D = i + j * n + k * n**2	     */
				/* where  index3D[] = i,j,k and n is the     */
				/* # of divisions per axis.		     */
	INT	indx_inc1D[3];	/* Including sign of ray direction.	     */
	struct	rayinfo *next;	/* Ptr to next structure.		     */
	}
	RAYINFO;



/*
 *	Define ray message information structure.
 */

typedef struct	ray
	{
	LONG		id;			/* Ray id.		     */
	INT		x, y;			/* Pixel ray is part of.     */
	VEC3		P;			/* Position (origin).	     */
	VEC3		D;			/* Direction.		     */
	INT		level;			/* Level of ray in ray tree. */
	RAYINFO 	*ri;			/* Grid dependent ray info.  */
	INT		ri_indx;
	INT		indx_inc3D[3];		/* Incl sign of ray direction*/
	RAYINFO 	rinfo[MAX_RAYINFO + 1];
	R32		weight; 		/* Ray weight.		     */
	R32		dummy;
	}
	RAY;



/*
 *	Define ray job bundle structure.
 */

typedef struct	rayjob
	{
	INT	x, y;			/* Primary ray pixel start address.  */
	INT	xlen, ylen;		/* Length of scanline bundle.	     */
	INT	xcurr, ycurr;		/* Current ray pixel address.	     */
	}
	RAYJOB;



/*
 *	Define shared workpool job entry structure.
 */

typedef struct	wpjob
	{
	INT	ypix, xpix;		/* Primary ray pixel address.	     */
	INT	xdim, ydim;		/* Pixel bundle size.		     */
	struct	wpjob	*next;
	}
	WPJOB;



/*
 *	Define heap node header structure (the arena).
 */

typedef struct	node
	{
	struct	node	huge	*next;	/* Ptr to next free node.	     */
	UINT	size;			/* Size of node in bytes excl header.*/
	BOOL	free;			/* TRUE = free, FALSE = in use.      */
	UINT	cksm;			/* Arena integrity check.	     */
	}
	NODE;



/*
 *	Define global memory structure.
 */

typedef struct	gmem
	{
	INT	nprocs; 		/* Number of processes. 	     */
	INT	pid;			/* Global process id counter.	     */
	INT	rid;			/* Global ray id counter.	     */
	OBJECT	*modelroot;		/* Root of model list.		     */
	GRID	*world_level_grid;	/* Zero level grid pointer.	     */
	NODE	huge *freelist; 	/* Ptr to global free memory heap.   */
	INT	wpstat[MAX_PROCS][INT_PAD_SIZE]; /* Shared work pool
					   status hints.  Padded to avoid
					   false-sharing */
	WPJOB	*workpool[MAX_PROCS][INT_PAD_SIZE]; /* Ptr to heads of
					   shared work pools.  Padded to
					   avoid false-sharing */

	BARDEC(start)			/* Barrier for startup sync.	     */
	LOCKDEC(pidlock)		/* Lock to increment pid.	     */
	LOCKDEC(ridlock)		/* Lock to increment rid.	     */
	LOCKDEC(memlock)		/* Lock for memory manager.	     */
	ALOCKDEC(wplock, MAX_PROCS)	/* Locks for shared work pools.      */
    UINT par_start_time;
    UINT partime[MAX_PROCS];
	}
	GMEM;



/*
 *      Define polygon data structure.
 */

typedef struct poly
        {
        INT     nverts;                 /* Number of vertices in polygon. */
        VEC3    norm;                   /* Face normal. */
        REAL    d;                      /* Plane eqn D. */
        VEC3    *vptr;                  /* Global vertex list pointer. */
        INT     *vindex;                /* Index of vertices. */
        INT     axis_proj;              /* Best axis for projection. */
        }
        POLY;



/*
 *      Define sphere data structure.
 */

typedef struct  sphere
        {
        POINT   center;                 /* Center of sphere. */
        REAL    rad;                    /* Radius of sphere. */
        REAL    rad2;                   /* Radius squared of sphere. */
        }
        SPHERE;



typedef struct  tri
        {
        VEC3    norm;                   /* Face normal. */
        REAL    d;                      /* Plane equation D. */
        VEC3    *vptr;                  /* Global vertex list pointer. */
        VEC3    *nptr;                  /* Global normal list pointer. */
        INT     vindex[3];              /* Index of vertices. */
        INT     indx;                   /* Normal component max flag. */
        BOOL    norminterp;             /* Do normal interpolation? */
        BOOL    vorder;                 /* Vertex order orientation. */
        }
        TRI;



/*
 *	Define flags and associated types.
 */

INT	DataType;			/* Ascii or binary geometry file.    */

INT	TraversalType;			/* Linked list or HUG traversal.     */

INT	bundlex, bundley;		/* Bundle sizes for workpools.	     */
INT	blockx, blocky; 		/* Block sizes for workpools.	     */
INT	NumSubRays;			/* Number of subpixel rays to calc.  */

BOOL	GeoFile;			/* TRUE if geometry file name found. */
BOOL	PicFile;			/* TRUE if picture file name found.  */
BOOL	ModelNorm;			/* TRUE if model must be normalized. */
BOOL	ModelTransform; 		/* TRUE if model transform present.  */
BOOL	AntiAlias;			/* TRUE if antialiasing enabled.     */

CHAR	*ProgName;			/* The program name.		     */
CHAR	GeoFileName[80];		/* Geometry file name.		     */
CHAR	PicFileName[80];		/* Picture file name.		     */

VIEW	View;				/* Viewing parameters.		     */
DISPLAY Display;			/* Display parameters.		     */
LIGHT	*lights;			/* Ptr to light list.		     */
INT	nlights;			/* Number of lights in scene.	     */

GMEM	*gm;				/* Ptr to global memory structure.   */



GRID	*world_level_grid;		/* Zero level grid pointer.	     */
GRID	*gridlist;

INT	hu_max_prims_cell;		/* Max # of prims per cell.	     */
INT	hu_gridsize;			/* Grid size.			     */
INT	hu_numbuckets;			/* Hash table bucket size.	     */
INT	hu_max_subdiv_level;		/* Maximum level of hierarchy.	     */
INT	hu_lazy;			/* Lazy evaluation indicator.	     */

INT	prim_obj_cnt;			/* Totals for model.		     */
INT	prim_elem_cnt;
INT	subdiv_cnt;
INT	bintree_cnt;

INT	grids;
INT	total_cells;
INT	empty_voxels;
INT	nonempty_voxels;
INT	nonempty_leafs;
INT	prims_in_leafs;

UINT	empty_masks[sizeof(UINT)*8];
UINT	nonempty_masks[sizeof(UINT)*8];



/*
 *	Function prototypes.
 */

/* bbox.c */
void InquireBoundBoxValues(BBOX *pbb, REAL *minx, REAL *miny, REAL *minz, REAL *maxx, REAL *maxy, REAL *maxz);
void NormalizeBoundBox(BBOX *pbb, MATRIX mat);

/* cr.c */
void prn_gridlist(void);
void prn_ds_stats(void);
void init_masks(void);
GRID *init_world_grid(VOXEL *v, ELEMENT **pepa, INT num_pe);
VOXEL *init_world_voxel(ELEMENT **pepa, INT numelements);
void mark_empty(INT index1D, GRID *g);
void mark_nonempty(INT index1D, GRID *g);
void insert_in_hashtable(VOXEL *v, GRID *g);
ELEMENT **prims_in_box2(ELEMENT **pepa, INT n_in, BBOX b, INT *n);
BTNODE *init_bintree(GRID *ng);
void subdiv_bintree(BTNODE *btn, GRID *g);
void create_bintree(BTNODE *root, GRID *g);
ELEMENT **bintree_lookup(BTNODE *root, INT i, INT j, INT k, GRID *g, INT *n);
void deleteBinTree(BTNODE *binTree);
GRID *create_grid(VOXEL *v, GRID *g, INT num_prims);

/* env.c */
void PrintEnv(void);
void InitEnv(void);
void InitLights(void);
void InitDisplay(void);
BOOL VerifyColorRange(COLOR c);
void TransformLights(MATRIX m);
void ViewRotate(MATRIX M, REAL x, REAL y, REAL z);
void CreateViewMatrix(void);
void TransformViewRay(POINT tray);
void NormalizeEnv(MATRIX normMat);
CHAR LookupCommand(CHAR *s);
void ReadEnvFile(CHAR *EnvFileName);

/* fbuf.c */
void RunLengthEncode(FILE *pf, PIXEL *fb, INT xsize);
void OpenFrameBuffer(void);
void AddPixelColor(COLOR c, INT x, INT y);
void CloseFrameBuffer(CHAR *PicFileName);

/* geo.c */
ELEMENT **MakeElementArray(INT *totalElements);
void PrintGeo(OBJECT *po);
void NormalizeGeo(OBJECT *po, MATRIX model, MATRIX modelInvT);
void ReadGeoFile(CHAR *GeoFileName);

/* huprn.c */
void prn_voxel(VOXEL *v);
void prn_grid(GRID *g);
void prn_ray(RAY *r);
void prn_PrimElem(ELEMENT *p);
void prn_bintree_node(BTNODE *b);
void prn_bintree_leaves(BTNODE *root);
void prn_pepa_prim_list(ELEMENT **pepa, INT nprims);

/* husetup.c */
void Huniform_defaults(void);
void BuildHierarchy_Uniform(void);
void IntersectHuniformPrimlist(INT *intersectPrim, IRECORD *hit, VOXEL *v, RAY *r, INT pid);
REAL HuniformShadowIntersect(RAY *r, REAL lightlength, ELEMENT *pe, INT pid);
BOOL TraverseHierarchyUniform(RAY *r, IRECORD *hit, INT pid);

/* hutv.c */
void prn_tv_stats(void);
INT send_ray(RAY *r, VOXEL *v);
VOXEL *lookup_hashtable(INT indx, GRID *g);
INT lookup_emptycells(INT indx, GRID *g);
void pop_up_a_grid(RAY *r);
void push_down_grid(RAY *r, VOXEL *v);
INT step_grid(RAY *r);
INT next_voxel(RAY *r);
VOXEL *next_nonempty_voxel(RAY *r);
VOXEL *next_nonempty_leaf(RAY *r, INT step, INT *status);
VOXEL *init_ray(RAY *r, GRID *g);

/* isect.c */
BOOL Intersect(RAY *pr, IRECORD *hit);
REAL ShadowIntersect(RAY *pr, REAL lightdist, ELEMENT *pe);

/* main.c */
void Usage(void);
void PrintStatistics(void);
void StartRayTrace(void);

/* matrix.c */
void VecNorm(POINT V);
void VecMatMult(POINT Vt, MATRIX M, POINT V);
void PrintMatrix(MATRIX M, CHAR *s);
void MatrixIdentity(MATRIX M);
void MatrixCopy(MATRIX A, MATRIX B);
void MatrixTranspose(MATRIX MT, MATRIX M);
void MatrixMult(MATRIX C, MATRIX A, MATRIX B);
void MatrixInverse(MATRIX Minv, MATRIX Mat);
void Translate(MATRIX M, REAL dx, REAL dy, REAL dz);
void Scale(MATRIX M, REAL sx, REAL sy, REAL sz);
void Rotate(INT axis, MATRIX M, REAL angle);

/* memory.c */
void *LocalMalloc(UINT n, CHAR *msg);
void LocalFree(void *p);
void GlobalHeapWalk(void);
BOOL GlobalHeapInit(UINT size);
void *GlobalMalloc(UINT size, CHAR *msg);
void *GlobalCalloc(UINT n, UINT size);
void *GlobalRealloc(void *p, UINT size);
void GlobalFree(void *p);
UINT GlobalMemAvl(void);
UINT GlobalMemMax(void);
void *ObjectMalloc(INT ObjectType, INT count);
void ObjectFree(INT ObjectType, INT count, void *p);
RAYINFO *ma_rayinfo(RAY *r);
void free_rayinfo(RAY *r);
void reset_rayinfo(RAY *r);
void ma_print(void);

/* poly.c */
CHAR *PolyName(void);
void PolyPrint(OBJECT *po);
void PolyElementBoundBox(ELEMENT *pe, POLY *pp);
void PolyBoundBox(OBJECT *po);
void PolyNormal(IRECORD *hit, POINT Pi, POINT Ni);
void PolyDataNormalize(OBJECT *po, MATRIX normMat);
INT PolyPeIntersect(RAY *pr, ELEMENT *pe, IRECORD *hit);
INT PolyIntersect(RAY *pr, OBJECT *po, IRECORD *hit);
void PolyTransform(OBJECT *po, MATRIX xtrans, MATRIX xinvT);
void PolyRead(OBJECT *po, FILE *pf);

/* raystack.c */
void CopyRayMsg(RAY *rdst, RAY *rsrc);
void InitRayTreeStack(INT TreeDepth, INT pid);
unsigned long powint(long i, long j);
void PushRayTreeStack(RAY *rmsg, INT pid);
INT PopRayTreeStack(RAY *rmsg, INT pid);

/* shade.c */
void SpecularDirection(POINT R, POINT N, POINT I);
BOOL TransmissionDirection(POINT T, POINT N, POINT I, REAL kn);
void Shade(VEC3 iP, VEC3 N, RAY *ray, IRECORD *hit, INT pid);

/* sph.c */
CHAR *SphName(void);
void SphPrint(OBJECT *po);
void SphElementBoundBox(ELEMENT *pe, SPHERE *ps);
void SphBoundBox(OBJECT *po);
void SphNormal(IRECORD *hit, POINT Pi, POINT Ni);
void SphDataNormalize(OBJECT *po, MATRIX normMat);
INT SphPeIntersect(RAY *pr, ELEMENT *pe, IRECORD *hit);
INT SphIntersect(RAY *pr, OBJECT *po, IRECORD *hit);
void SphTransform(OBJECT *po, MATRIX xtrans, MATRIX xinvT);
void SphRead(OBJECT *po, FILE *pf);

/* trace.c */
REAL frand(void);
BOOL GetRayJobFromBundle(RAYJOB *job, INT *x, INT *y);
void ConvertPrimRayJobToRayMsg(RAY *ray, REAL x, REAL y);
void RayTrace(INT pid);

/* tri.c */
CHAR *TriName(void);
void TriPrint(OBJECT *po);
void TriElementBoundBox(ELEMENT *pe, TRI *pt);
void TriBoundBox(OBJECT *po);
void TriNormal(IRECORD *hit, POINT Pi, POINT Ni);
void TriDataNormalize(OBJECT *po, MATRIX normMat);
INT TriPeIntersect(RAY *pr, ELEMENT *pe, IRECORD *hit);
INT TriIntersect(RAY *pr, OBJECT *po, IRECORD *hit);
void TriTransform(OBJECT *po, MATRIX xtrans, MATRIX xinvT);
void TriRead(OBJECT *po, FILE *pf);

/* workpool.c */
void PutJob(INT xs, INT ys, INT xe, INT ye, INT xbe, INT ybe, INT pid);
INT GetJob(RAYJOB *job, INT pid);
INT GetJobs(RAYJOB *job, INT pid);
void PrintWorkPool(INT pid);
void InitWorkPool(INT pid);
