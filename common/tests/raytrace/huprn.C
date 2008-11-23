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
 *	huprn.c
 *
 * DESCRIPTION
 *	This file contains routines that walk and print most all data
 *	structures relating to the HU grid.
 */


#include <stdio.h>
#include <math.h>
#include "rt.h"

VOID	prn_voxel(VOXEL *v)
	{
	ELEMENT *pe, **pepa;
	GRID	*g;
	INT	i;

	fprintf(stderr, "    Print Voxel  id = %ld \n", v->id);
	fprintf(stderr, "        celltype %d \n", v->celltype);

	if (v->celltype == GSM_VOXEL)
		{
		fprintf(stderr, "        gsm_voxel \n");
		fprintf(stderr, "        primElement index list:\n");

		pepa = (ELEMENT**)v->cell;


		for (i=0; i < v->numelements; i++)
			{
			pe = pepa[i];
			fprintf(stderr, "            %ld \n", pe->index);
			}
		}

	if (v->celltype == GSM_GRID)
		{
		g = (GRID *)v->cell;
		prn_grid(g);
		fprintf(stderr, "        gsm_grid id %ld \n", g->id);
		}

	fprintf(stderr, "    End Voxel \n");
	}



VOID	prn_grid(GRID *g)
	{
	INT	i;
	INT	n;
	GRID	*ng;
	VOXEL	*v;

	fprintf(stderr, "    Print  Grid %ld \n",                      g->id);
	fprintf(stderr, "        num_prims = %ld \n",                  g->num_prims);
	fprintf(stderr, "        indx_inc[0,1,2] = %ld, %ld, %ld \n",  g->indx_inc[0], g->indx_inc[1], g->indx_inc[2]);
	fprintf(stderr, "        num_buckets = %ld \n",                g->num_buckets);
	fprintf(stderr, "        min[0,1,2] = %lf, %lf, %lf \n",       g->min[0], g->min[1], g->min[2] );
	fprintf(stderr, "        cellsize[0,1,2] = %lf, %lf, %lf \n",  g->cellsize[0], g->cellsize[1], g->cellsize[2]);
	fprintf(stderr, "        subdiv_level = %ld \n",               g->subdiv_level);

	if (g->next != NULL)
		{
		ng = g->next;
		fprintf(stderr, "        next grid id %ld \n", ng->id);
		}
	else
		fprintf(stderr, "        next grid id NULL \n");

	fprintf(stderr, "    Voxel List \n");

	n = g->indx_inc[1] * g->indx_inc[2];

	for (i = 0; i < n; i++)
		{
		if (lookup_emptycells(i, g) == EMPTY)
			fprintf(stderr, "        Voxel %ld is empty. \n", i);
		else
			{
			v = lookup_hashtable(i, g);
			prn_voxel(v);
			}
		}


	fprintf(stderr, "    End Grid \n");
	}



VOID	prn_ray(RAY *r)
	{
	RAYINFO *ri;
	GRID	*g;

	fprintf(stderr, "    Print Ray  id %ld \n",                             r->id );
	fprintf(stderr, "        origin:        ( %lf, %lf, %lf ) \n",          r->P[0], r->P[1], r->P[2]);
	fprintf(stderr, "        direction:     ( %lf, %lf, %lf ) \n",          r->D[0], r->D[1], r->D[2] );
	fprintf(stderr, "        indx_inc3D[0,1,2] = [ %ld, %ld, %ld ] \n",     r->indx_inc3D[0],r->indx_inc3D[1],r->indx_inc3D[2] );
	fprintf(stderr, "        ri_indx = %ld \n",                             r->ri_indx);
	fprintf(stderr, "        rayinfo: \n");

	ri = r->ri;
	g  = ri->grid;

	fprintf(stderr, "            ray is in grid %ld \n",                    g->id );
	fprintf(stderr, "            d[0,1,2] = [ %lf, %lf, %lf ] \n",          ri->d[0], ri->d[1], ri->d[2]);
	fprintf(stderr, "            entry_plane %ld \n",                       ri->entry_plane );
	fprintf(stderr, "            t_in = %lf \n",                            ri->t_in );
	fprintf(stderr, "            exit_plane %ld \n",                        ri->exit_plane );
	fprintf(stderr, "            t_out = %lf \n",                           ri->t_out );
	fprintf(stderr, "            delta[0,1,2] = [ %lf, %lf, %lf ] \n",      ri->delta[0], ri->delta[1], ri->delta[2]);
	fprintf(stderr, "            index3D[0,1,2] = [ %ld, %ld, %ld ] \n",    ri->index3D[0], ri->index3D[1], ri->index3D[2]);
	fprintf(stderr, "            index1D = %ld \n",                         ri->index1D );
	fprintf(stderr, "            indx_inc1D[0,1,2] = [ %ld, %ld, %ld ] \n", ri->indx_inc1D[0], ri->indx_inc1D[1], ri->indx_inc1D[2]);
	fprintf(stderr, "    End Ray \n");
	}



VOID	prn_PrimElem(ELEMENT *p)
	{
	BBOX	b;

	if (p == NULL)
		{
		fprintf(stderr, "%s: prn_PrimElem: Null pointer.\n", ProgName);
		exit(-1);
		}

	fprintf(stderr, "PrimElem: index %ld  ptr %p, PrimObj index %ld ptr %p \n",
		p->index, p,  p->parent->index,   p->parent);

	b = p->bv;

	fprintf(stderr, "   BBox: ( %lf, %lf, %lf ) -> \n         ( %lf, %lf, %lf ) \n",
		b.dnear[0],b.dnear[1],b.dnear[2],b.dfar[0],b.dfar[1],b.dfar[2] );
	}



VOID	prn_bintree_node(BTNODE *b)
	{
	INT	i;

	fprintf(stderr, "Bintree node: \n");
	fprintf(stderr, "    indecies of cell: ( %ld, %ld, %ld ) \n",           b->i[0], b->i[1], b->i[2]);
	fprintf(stderr, "    gridsizes: ( %ld, %ld, %ld ) \n",                  b->n[0], b->n[1], b->n[2]);
	fprintf(stderr, "    minimum point ( %lf, %lf, %lf ) \n",               b->p[0], b->p[1], b->p[2]);
	fprintf(stderr, "    subdiv axis %ld \n",                               b->axis);
	fprintf(stderr, "    number of primitives %ld \n",                      b->nprims);
	fprintf(stderr, "    Primitive element list: \n");

	if (b->nprims > 0)
		for (i = 0; i < b->nprims; i++)
			{
			fprintf(stderr, "  %ld", b->pe[i]->index);

			if (i % 8 == 7)
				fprintf(stderr, "\n");
			}

	fprintf(stderr, "\n    End of bintree node \n");
	}




VOID	prn_bintree_leaves(BTNODE *root)
	{
	BTNODE	*b;

	b = root;
	if (b->axis == -1)
		prn_bintree_node(b);
	else
		{
		prn_bintree_leaves(b->btn[0]);
		prn_bintree_leaves(b->btn[1]);
		}
	}


VOID	prn_pepa_prim_list(ELEMENT **pepa, INT nprims)
	{
	INT	i;

	if (nprims > 0)
		{
		for (i = 0; i < nprims; i++)
			{
			fprintf(stderr, "  %ld", pepa[i]->index);

			if (i % 8 == 7)
				fprintf(stderr, "\n");
			}
		}
	}
