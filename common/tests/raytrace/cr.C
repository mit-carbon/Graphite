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
 *	cr.c
 *
 * DESCRIPTION
 *	This file contains routines that manage the creation of the HU grid
 *	structures.
 *
 */


#include <stdio.h>
#include <math.h>
#include "rt.h"



GRID	*gridlist = NULL;

/*
	Note: gridlist doesn't need to be an array since it is only used when
	building HUG, which is done before child process creation
*/



/*
 * NAME
 *	prn_gridlist - print HU grid to stdout
 *
 * SYNOPSIS
 *	VOID	prn_gridlist()
 *
 * RETURNS
 *	Nothing.
 */

VOID	prn_gridlist()
	{
	GRID	*g;

	fprintf(stderr, "    Print Gridlist \n");
	g = gridlist;

	while (g != NULL)
		{
		prn_grid(g);
		g = g->next;
		}

	fprintf(stderr, "    End Gridlist \n");
	}



/*
 * NAME
 *	prn_ds_stats - print HU grid data structure statistics to stdout
 *
 * SYNOPSIS
 *	VOID	prn_ds_stats()
 *
 * RETURNS
 *	Nothing.
 */

VOID	prn_ds_stats()
	{
	INT	leafs;
	INT	voxels;

	printf("\n");
	printf("****** Hierarchical uniform grid data structure statistics ******\n\n");

	printf("    Data structure evaluated as a preprocess.\n");

	printf("    gridsize                             %3ld \n", hu_gridsize);
	printf("    hashtable buckets                    %3ld \n", hu_numbuckets);
	printf("    maximum subdivision level            %3ld \n", hu_max_subdiv_level);
	printf("    maximum primitives / cell            %3ld \n", hu_max_prims_cell);
	printf("    grids                                %3ld \n", grids);

	voxels = empty_voxels + nonempty_voxels;
	leafs  = empty_voxels + nonempty_leafs;

	printf("    empty voxels                    %8ld    %3ld %% \n",          empty_voxels,    100*empty_voxels/voxels);
	printf("    nonempty voxels                 %8ld    %3ld %% \n",          nonempty_voxels, 100*nonempty_voxels/voxels);
	printf("    empty leafs                     %8ld    %3ld %% \n",          empty_voxels,    100*empty_voxels/leafs);
	printf("    nonempty leafs                  %8ld    %3ld %% \n",          nonempty_leafs,  100*nonempty_leafs/leafs);
	printf("    primitives / leaf                 %6.1lf \n",               (double)prims_in_leafs/leafs);
	printf("\n");
	}



/*
 * NAME
 *	init_masks - initialize empty and nonempty mask arrays
 *
 * SYNOPSIS
 *	VOID	init_masks()
 *
 * RETURNS
 *	Nothing.
 */

VOID	init_masks()
	{
	INT	n, i;

	n = sizeof(UINT)*8;

	for (i = 0; i < n; i++)
		{
		empty_masks[i]	  = (MSB >> i);
		nonempty_masks[i] = ~empty_masks[i];
		}

	}


/*
 * NAME
 *	init_world_grid -
 *
 * SYNOPSIS
 *	GRID	*init_world_grid(v, pepa, num_pe)
 *	VOXEL	*v;
 *	ELEMENT **pepa; 	// Prim elements in voxel containing grid.
 *	INT	num_pe; 	// # of prim elements in voxel containing grid.
 *
 * RETURNS
 *	A pointer to the grid.
 *
 */

GRID	*init_world_grid(VOXEL	*v, ELEMENT **pepa, INT	num_pe)
	{
	UINT	*ec;
	BBOX	wbox;
	GRID	*g;
	VOXEL	**ht;

	g  = ObjectMalloc(OT_GRID, 1);
	g->id = grids++;

	ht = ObjectMalloc(OT_HASHTABLE, 1);

	g->hashtable	= ht;
	g->hashtable[0] = v;

	ec = ObjectMalloc(OT_EMPTYCELLS, 1);

	g->emptycells	 = ec;
	g->emptycells[0] = 0;	    /* nonempty */

	g->indx_inc[0] = 1;
	g->indx_inc[1] = 1;
	g->indx_inc[2] = 1;
	g->num_buckets = 1;

	wbox.dnear[0] = 0.0;
	wbox.dnear[1] = 0.0;
	wbox.dnear[2] = 0.0;
	wbox.dfar[0]  = 1.0;
	wbox.dfar[1]  = 1.0;
	wbox.dfar[2]  = 1.0;

	g->min[0] = wbox.dnear[0];
	g->min[1] = wbox.dnear[1];
	g->min[2] = wbox.dnear[2];

	g->cellsize[0] = wbox.dfar[0] - wbox.dnear[0];
	g->cellsize[1] = wbox.dfar[1] - wbox.dnear[1];
	g->cellsize[2] = wbox.dfar[2] - wbox.dnear[2];

	g->subdiv_level = - 1;
	g->num_prims	= num_pe;
	g->pepa 	= pepa;
	g->next 	= NULL;

	gridlist = g;
	return (g);
	}



/*
 * NAME
 *	init_world_voxel -
 *
 * SYNOPSIS
 *	VOXEL	*init_world_voxel(pepa, numelements)
 *	ELEMENT **pepa;
 *	INT	numelements;
 *
 * RETURNS
 *	A pointer to the voxel.
 *
 */

VOXEL	*init_world_voxel(ELEMENT **pepa, INT	numelements)
	{
	VOXEL	*v;

	v = ObjectMalloc(OT_VOXEL, 1);

	nonempty_voxels++;

	v->id	       = 0;
	v->cell        = (CHAR *)pepa;
	v->numelements = numelements;
	v->celltype    = GSM_VOXEL;
	v->next        = NULL;

	return (v);
	}



/*
 * NAME
 *     mark_empty - mark grid as being empty
 *
 * SYNOPSIS
 *     VOID   mark_empty(index1D, g)
 *     INT    index1D;
 *     GRID   *g;
 *
 * RETURNS
 *	Nothing.
 */

VOID	mark_empty(INT	index1D, GRID	*g)
	{
	INT	i, r;
	UINT	w;

	i = index1D/(sizeof(UINT)*8);
	r = index1D - i*sizeof(UINT)*8;

	w = g->emptycells[i];
	w = w | empty_masks[r];
	g->emptycells[i] = w;
	}



/*
 * NAME
 *     mark_nonempty - mark grid as being nonempty
 *
 * SYNOPSIS
 *     VOID   mark_nonempty(index1D, g)
 *     INT    index1D;
 *     GRID   *g;
 *
 * RETURNS
 *	Nothing.
 */

VOID	mark_nonempty(INT index1D, GRID *g)
	{
	INT	i, r;
	UINT	w;

	i = index1D/(sizeof(UINT)*8);
	r = index1D - i*sizeof(UINT)*8;

	w = g->emptycells[i];
	w = w & nonempty_masks[r];
	g->emptycells[i] = w;
	}



/*
 * NAME
 *	insert_in_hashtable - insert voxel from grid into hashtable
 *
 * SYNOPSIS
 *	VOID	insert_in_hashtable(v, g)
 *	VOXEL	*v;
 *	GRID	*g;
 *
 * RETURNS
 *	Nothing.
 */

VOID	insert_in_hashtable(VOXEL *v, GRID *g)
	{
	INT	i, r;
	VOXEL	*vht;

	i = v->id/hu_numbuckets;
	r = v->id - i*hu_numbuckets;

	vht = g->hashtable[r];
	v->next = vht;
	g->hashtable[r] = v;
	}


/*
 * NAME
 *	**prims_in_box2 -
 *
 * SYNOPSIS
 *	ELEMENT **prims_in_box2(pepa, n_in, b, n)
 *	ELEMENT **pepa; 		// Array of element ptrs to check.
 *	INT	n_in;			// Number of elements in array.
 *	BBOX	b;			// Bounding box to check.
 *	INT	*n;			// Number of elements in the box.
 *
 * RETURNS
 *	An array of PrimElement pointers for those primitive elements in the
 *	bounding box.
 */

ELEMENT **prims_in_box2(ELEMENT **pepa, INT n_in, BBOX b, INT *n)
	{
	INT	ovlap, i, j, k;
	ELEMENT *pe;
	ELEMENT **npepa;
	BBOX	bb;
	REAL	max, side, eps;

	max  = b.dfar[0] - b.dnear[0];
	side = b.dfar[1] - b.dnear[1];

	max  = max > side ? max : side;
	side = b.dfar[2] - b.dnear[2];

	max  = max > side ? max : side;
	eps  = max * 1.0E-6;

	if (n_in > 0)
		npepa = ObjectMalloc(OT_PEPARRAY, n_in);
	else
		{
		npepa = NULL;
		*n = 0;
		return (npepa);
		}

	*n = 0;
	k  = 0;

	for (j = 0; j < n_in; j++)
		{
		pe = pepa[j];
		bb = pe->bv;
		ovlap = 1;

		for (i = 0; i < 1; i++)
			{
			if (b.dnear[0] > bb.dfar[0] + eps)
				{
				ovlap = 0;
				break;
				}

			if (b.dnear[1] > bb.dfar[1] + eps)
				{
				ovlap = 0;
				break;
				}

			if (b.dnear[2] > bb.dfar[2] + eps)
				{
				ovlap = 0;
				break;
				}

			if (b.dfar[0] < bb.dnear[0] - eps)
				{
				ovlap = 0;
				break;
				}

			if (b.dfar[1] < bb.dnear[1] - eps)
				{
				ovlap = 0;
				break;
				}

			if (b.dfar[2] < bb.dnear[2] - eps)
				{
				ovlap = 0;
				break;
				}
			}

		if (ovlap == 1)
			{
			npepa[k++] = pepa[j];
			(*n)++;
			}
		}

	return (npepa);
	}



/*
 * NAME
 *	init_bintree - initialize (create) rootnode of bintree
 *
 * SYNOPSIS
 *	BTNODE	*init_bintree(ng)
 *	GRID	*ng;
 *
 * RETURNS
 *	A pointer to the newly created bintree root.
 *
 */

BTNODE	*init_bintree(GRID *ng)
	{
	BTNODE	*btn;

	btn = ObjectMalloc(OT_BINTREE, 1);

	btn->btn[0] = NULL;
	btn->btn[1] = NULL;
	btn->axis   = -1;

	btn->p[0] = ng->min[0];
	btn->p[1] = ng->min[1];
	btn->p[2] = ng->min[2];

	btn->n[0] = ng->indx_inc[1];
	btn->n[1] = ng->indx_inc[1];
	btn->n[2] = ng->indx_inc[1];

	btn->i[0] = 0;
	btn->i[1] = 0;
	btn->i[2] = 0;

	btn->nprims = ng->num_prims;
	btn->totalPrimsAllocated = btn->nprims;

	if (ng->num_prims > 0)
		btn->pe = ng->pepa;
	else
		btn->pe = NULL;

	return (btn);
	}


/*
 * NAME
 *	subdiv_bintree
 *
 * SYNOPSIS
 *	VOID	subdiv_bintree(btn, g)
 *	BTNODE	*btn;			// Current bintree node.
 *	GRID	*g;			// Grid being created.
 *
 * DESCRIPTION
 *	The bintree node is subdivided at the largest dimension and the
 *	primitive element list is pruned to the two new nodes.
 *
 * RETURNS
 *	Nothing.
 *
 */

VOID	subdiv_bintree(BTNODE *btn, GRID *g)
	{
	BTNODE	*btn1, *btn2;
	INT	 n1, n2, imax, dmax;
	BBOX	 b1, b2;

	/* Find largest dimension. */

	imax = 0;
	dmax = btn->n[0];

	if (dmax < btn->n[1])
		{
		imax = 1;
		dmax = btn->n[1];
		}

	if (dmax < btn->n[2])
		{
		imax = 2;
		dmax = btn->n[2];
		}

	/* Subdiv largest dimension. */

	btn->axis   = imax;
	btn->btn[0] = NULL;
	btn->btn[1] = NULL;

	if (btn->n[imax] > 1)
		{
		n1 = btn->n[imax] / 2;
		n2 = btn->n[imax] - n1;

		btn1 = ObjectMalloc(OT_BINTREE, 1);
		btn2 = ObjectMalloc(OT_BINTREE, 1);

		btn->btn[0] = btn1;
		btn->btn[1] = btn2;

		btn1->btn[0] = NULL;
		btn1->btn[1] = NULL;
		btn1->axis   = -1;

		btn2->btn[0] = NULL;
		btn2->btn[1] = NULL;
		btn2->axis   = -1;

		btn1->i[0] = btn->i[0];
		btn1->i[1] = btn->i[1];
		btn1->i[2] = btn->i[2];

		btn2->i[0] = btn->i[0];
		btn2->i[1] = btn->i[1];
		btn2->i[2] = btn->i[2];
		btn2->i[imax] += n1;

		btn1->n[0] = btn->n[0];
		btn1->n[1] = btn->n[1];
		btn1->n[2] = btn->n[2];
		btn1->n[imax] = n1;

		btn2->n[0] = btn->n[0];
		btn2->n[1] = btn->n[1];
		btn2->n[2] = btn->n[2];
		btn2->n[imax] = n2;

		btn1->p[0] = btn->p[0];
		btn1->p[1] = btn->p[1];
		btn1->p[2] = btn->p[2];

		btn2->p[0] = btn->p[0];
		btn2->p[1] = btn->p[1];
		btn2->p[2] = btn->p[2];
		btn2->p[imax] = btn->p[imax] + n1 * g->cellsize[imax];

		b1.dnear[0] = btn1->p[0];
		b1.dnear[1] = btn1->p[1];
		b1.dnear[2] = btn1->p[2];
		b1.dfar[0]  = btn1->p[0] + btn1->n[0] * g->cellsize[0];
		b1.dfar[1]  = btn1->p[1] + btn1->n[1] * g->cellsize[1];
		b1.dfar[2]  = btn1->p[2] + btn1->n[2] * g->cellsize[2];

		btn1->pe    = prims_in_box2(btn->pe, btn->nprims, b1, &(btn1->nprims));
		btn1->totalPrimsAllocated = btn->nprims;

		b2.dnear[0] = btn2->p[0];
		b2.dnear[1] = btn2->p[1];
		b2.dnear[2] = btn2->p[2];
		b2.dfar[0]  = btn2->p[0] + btn2->n[0] * g->cellsize[0];
		b2.dfar[1]  = btn2->p[1] + btn2->n[1] * g->cellsize[1];
		b2.dfar[2]  = btn2->p[2] + btn2->n[2] * g->cellsize[2];

		btn2->pe    = prims_in_box2(btn->pe, btn->nprims, b2, &(btn2->nprims));
		btn2->totalPrimsAllocated = btn->nprims;

		}

	if (btn1->n[0] == 1 && btn1->n[1] == 1 && btn1->n[2] == 1)
		{
		/* further optimize the pe so no space is wasted !! */
		}

	if (btn2->n[0] == 1 && btn2->n[1] == 1 && btn2->n[2] == 1)
		{
		/* further optimize the pe so no space is wasted !! */
		}
	}



/*
 * NAME
 *	create_bintree - subdivide tree until all leaf nodes have gridsizes of
 *			 (1, 1, 1).
 *
 * SYNOPSIS
 *	VOID	create_bintree(root , g)
 *	BTNODE	*root;			// Root of bintree.
 *	GRID	*g;			// Grid being created.
 *
 * RETURNS
 *	Nothing.
 */

VOID	create_bintree(BTNODE *root, GRID *g)
	{
	BTNODE	*btn;

	/* It is assumed that root's children are NULL on entry. */

	btn = root;
	if (btn->n[0] != 1 || btn->n[1] != 1 || btn->n[2] != 1)
		{
		subdiv_bintree(btn, g);
		create_bintree(btn->btn[0], g);
		create_bintree(btn->btn[1], g);

		if ((btn->nprims) > 0)
			{
		/*	ObjectFree(OT_PEPARRAY, btn->totalPrimsAllocated, btn->pe);
			btn->pe = NULL;     */
			}
		}
	}



/*
 * NAME
 *	bintree_lookup -
 *
 * SYNOPSIS
 *	ELEMENT **bintree_lookup(root, i, j, k, g, n)
 *	BTNODE	*root;			// Root of bintree.
 *	INT	i, j, k;		// 3D indecies of cell in grid.
 *	GRID	*g;			// Grid for this bintree.
 *	INT	*n;			// # prims.
 *
 * DESCRIPTION
 *	Receive the 3D indices of the cell in the grid and return a pointer to
 *	an array of elments and the number of elements on the list.
 *
 * RETURNS
 *	An array of elements associated with the voxel
 *
 */

ELEMENT **bintree_lookup(BTNODE	*root, INT i, INT j, INT k, GRID *g, INT *n)
	{
	INT	ijk[3];
	INT	child;
	INT	idiv;
	ELEMENT **pepa;
	BTNODE	*btn;

	ijk[0] = i;
	ijk[1] = j;
	ijk[2] = k;

	btn = root;

	if (btn == NULL)
		{
		*n  = 0;
		return (NULL);
		}

	while (btn->n[0] != 1 || btn->n[1] != 1 || btn->n[2] != 1)
		{
		if (btn->axis == - 1)
			{
			/* Not a leaf & not subdivided but should be subdivided. */

			fprintf(stderr, "bintree_lookup: gridsizes (%ld, %ld, %ld), axis %ld & nprims %ld\n",
				btn->n[0], btn->n[1], btn->n[2], btn->axis, btn->nprims);

			exit(-1);
			}

		/* Descend the tree: find which branch. */

		child = 0;
		idiv  = (btn->n[btn->axis]/2);

		if (ijk[btn->axis] + 1 > idiv)
			{
			child = 1;
			ijk[btn->axis] -= idiv;
			}

		/* Child is now the correct branch. */

		btn = btn->btn[child];
		if (btn == NULL)
			{
			*n  = 0;
			return (NULL);
			}
		}

	/* Now at a leaf. */

	*n   = btn->nprims;
	pepa = btn->pe;

/*	if (btn->nprims == btn->totalPrimsAllocated)
		pepa = btn->pe;
	else
		{
		if (btn->pe)
			pepa = GlobalRealloc(btn->pe, btn->nprims*sizeof(ELEMENT *));
		else
			pepa = NULL;


		pepa = ObjectMalloc(OT_PEPARRAY, btn->nprims);
		for (x = 0; x < btn->nprims; x++)
			pepa[x] = btn->pe[x];

		ObjectFree(OT_PEPARRAY, btn->totalPrimsAllocated, btn->pe);
		btn->pe = NULL;
		}
*/
	return (pepa);
	}



/*
 * NAME
 *	deleteBinTree - delete the entire bintree and free its memory
 *
 * SYNOPSIS
 *	VOID	deleteBinTree(binTree)
 *	BTNODE	*binTree;
 *
 * DESCRIPTION
 *	Delete the entire bintree by recursively calling this procedure.
 *
 * RETURNS
 *	Nothing.
 */

VOID	deleteBinTree(BTNODE *binTree)
	{
	BTNODE *left, *right;

	if (binTree != NULL)
		{
		left  = binTree->btn[0];
		right = binTree->btn[1];

		deleteBinTree(left);
		deleteBinTree(right);
/*		ObjectFree(OT_BINTREE, 1, binTree);    */
		}
	}



/*
 * NAME
 *	create_grid -
 *
 * SYNOPSIS
 *	GRID	*create_grid(v, g, num_prims)
 *	VOXEL	*v;
 *	GRID	*g;
 *	INT	num_prims;	// # of prim elem in voxel to be subdivided.
 *
 * DESCRIPTION
 *
 *		Accept a voxel with an ELEMENT array and a grid and recursively
 *		uniformly subdivide the voxel to produce a new grid.  Create a
 *		new list of prim elements pruned to each new voxel.  If the
 *		list is non NULL create a voxel for it.
 *
 *	In all cases:
 *
 *		Place a pointer to the new grid in the input voxel and mark
 *		the celltype as GSM_GRID.  Return a pointer to the new grid.
 *		Link all new grids on to the list gridlist for debug purposes.
 *
 * RETURNS
 *	A pointer to the grid.
 */

GRID	*create_grid(VOXEL *v, GRID *g, INT num_prims)
	{
	INT	n;
	INT	i, j, k, r;
	INT	nprims;
	INT	index1D;
	UINT	*ec;
	R64	ncells;
	GRID	*ng, *nng;	/* New grid. */
	VOXEL	*nv;
	VOXEL	**ht;
	BTNODE	*bintree;
	ELEMENT **pepa;

	ng = ObjectMalloc(OT_GRID, 1);
	ng->id = grids++;

	ht = ObjectMalloc(OT_HASHTABLE, hu_numbuckets);
	ng->hashtable = ht;

	ncells = (R64)(hu_gridsize * hu_gridsize * hu_gridsize);
	total_cells += ncells;

	ec = ObjectMalloc(OT_EMPTYCELLS, (INT)ncells);
	ng->emptycells = ec;

	ng->num_prims	= num_prims;
	ng->pepa	= (ELEMENT**)v->cell;
	ng->indx_inc[0] = 1;
	ng->indx_inc[1] = hu_gridsize;
	ng->indx_inc[2] = hu_gridsize * hu_gridsize;
	ng->num_buckets = hu_numbuckets;

	k = v->id/g->indx_inc[2];
	r = v->id - k*g->indx_inc[2];
	j = r/g->indx_inc[1];
	i = r - j*g->indx_inc[1];

	ng->min[0] = g->min[0] + i * g->cellsize[0];
	ng->min[1] = g->min[1] + j * g->cellsize[1];
	ng->min[2] = g->min[2] + k * g->cellsize[2];

	ng->cellsize[0]  = g->cellsize[0]/ng->indx_inc[1];
	ng->cellsize[1]  = g->cellsize[1]/ng->indx_inc[1];
	ng->cellsize[2]  = g->cellsize[2]/ng->indx_inc[1];
	ng->subdiv_level = g->subdiv_level + 1;
	ng->next	 = gridlist;

	gridlist = ng;

	/* Calculate hierarchical grid */

	/* First create bintree. */

	bintree = init_bintree(ng);
	create_bintree(bintree, ng);

	index1D = 0;
	n = hu_gridsize;

	/*
	 *	For each cell in new grid, create an ELEMENT array for the
	 *	cell; if non empty create a voxel for it.  If nonempty
	 *	cell has more than MAX_PRIMS_PER_CELL (hu_max_prims_cell) primitives and
	 *	MAX_SUBDIV_LEVEL (hu_max_subdiv_level) has not been reached, create a
	 *	new grid (i.e. subdivide) and insert it in the hashtable.  If nonempty
	 *	cell has less than MAX_PRIMS_PER_CELL insert it in the
	 *	hashtable.  In any case make the appropriate entry in
	 *	emptycells.
	 */

	for (k = 0; k < n; k++)
		{
		for (j = 0; j < n; j++)
			{
			for (i = 0; i < n; i++)
				{
				pepa = bintree_lookup(bintree, i, j, k, ng, &nprims);

				if (pepa != NULL)
					{
					nonempty_voxels++;
					mark_nonempty(index1D, ng);

					nv = ObjectMalloc(OT_VOXEL, 1);

					nv->id		= index1D;
					nv->celltype	= GSM_VOXEL;
					nv->cell	= (CHAR*)pepa;
					nv->numelements = nprims;

					if (nprims > hu_max_prims_cell && ng->subdiv_level < hu_max_subdiv_level)
						nng = create_grid(nv, ng, nprims);
					else
						{
						/* Leaf cell. */

						nonempty_leafs++;
						prims_in_leafs += nprims;
						}

					insert_in_hashtable(nv, ng);
					}
				else
					{
					/* Empty cell. */

					empty_voxels++;
					mark_empty(index1D, ng);
					}

				index1D++;
				}
			}
		}

	/* Store new grid ptr in input voxel. */

	v->cell        = (CHAR *)ng;
	v->numelements = -1;	 /* this field doesn't make sence if cell is a GRID */
	v->celltype    = GSM_GRID;

	deleteBinTree(bintree);

	return (ng);
	}

