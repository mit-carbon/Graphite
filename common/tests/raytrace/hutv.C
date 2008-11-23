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
 *	hutv.c
 *
 * DESCRIPTION
 *
 */


#include <stdio.h>
#include <math.h>
#include "rt.h"

/*
 *	eps_t is a small increment in t to be added to t_in to insure that the
 *	point is inside a cell not on the boundary.
 */

REAL	eps_t	= 1.0E-10;




/*
 * NAME
 *	prn_tv_stats - print out HU traversal statistics -- not implemented in
 *		this version
 *
 * SYNOPSIS
 *	VOID	prn_tv_stats()
 *
 * RETURNS
 *	Nothing.
 */

VOID	prn_tv_stats()
	{
	}



/*
 * NAME
 *	send_ray(r, v) - send ray to remote cluster
 *
 * SYNOPSIS
 *	INT	send_ray(r, v)
 *	RAY	*r;
 *	VOXEL	*v;
 *
 * DESCRIPTION
 *	Not implemented yet.
 *
 * RETURNS
 *	Nothing, yet.
 */

INT	send_ray(RAY *r, VOXEL *v)
	{
		return(0);
	}



/*
 * NAME
 *	lookup_hashtable -
 *
 * SYNOPSIS
 *	VOXEL	*lookup_hashtable(indx, g)
 *	INT	indx;
 *	GRID	*g;
 *
 * DESCRIPTION
 *	Hashtable is a table of all non-empty cells in the grid.
 *	hashtable[num_buckets] is indexed by index1D mod num_buckets,
 *	num_buckets and n, the # of cells per axis, should be relatively
 *	prime, grids at different levels may have different num_buckets.  This
 *	routine assumes that there exists a non-empty voxel in the table.
 *
 * RETURNS
 *
 */

VOXEL	*lookup_hashtable(INT indx, GRID *g)
	{
	INT	i;
	VOXEL	*v;

	i = indx - ((indx / g->num_buckets) * g->num_buckets);
	v = g->hashtable[i];

	while (v->id != indx)
		{
		v = v->next;
		if (v == NULL)
			{
			fprintf(stderr, "hashtable error \n");
			exit(-1);
			}
		}

	return (v);
	}



/*
 * NAME
 *	lookup_emptycells -
 *
 * SYNOPSIS
 *	INT	lookup_emptycells(indx, g)
 *	INT	indx;
 *	GRID	*g;
 *
 * DESCRIPTION
 *	Emptycells is a packed array of bits, one bit per cell in the grid.  A
 *	1 indicates that the cell is empty and therefore there is no entry for
 *	that cell in the hashtable.
 *
 * RETURNS
 *
 */

INT	lookup_emptycells(INT indx, GRID *g)
	{
	INT	i, w, r, num_bits;
	UINT	p, b;

	num_bits = sizeof(UINT)*8;

	w = indx / num_bits;
	r = indx - w * num_bits;
	p = g->emptycells[w];
	b = p & (MSB >> r);
	i = b > 0 ? EMPTY : NONEMPTY;

	return (i);
	}



/*
 * NAME
 *	pop_up_a_grid -
 *
 * SYNOPSIS
 *	VOID	pop_up_a_grid(r)
 *	RAY	*r;
 *
 * DESCRIPTION
 *	Pop_up_a_grid takes RAYINFO & a grid pointer off the top of the stack
 *	and discards it which puts the ray in the voxel containing the grid
 *	being left.
 *
 * RETURNS
 *	Nothing.
 */

VOID	pop_up_a_grid(RAY *r)
	{
	RAYINFO *old_ri;

	old_ri = r->ri;
	r->ri  = old_ri->next;

	free_rayinfo(r);

	}



/*
 * NAME
 *	push_down_grid -
 *
 * SYNOPSIS
 *	VOID	push_down_grid(r, v)
 *	RAY	*r;
 *	VOXEL	*v;			Voxel containing the new grid.
 *
 * DESCRIPTION
 *	push_down_grid creates rayinfo for the new grid being entered and puts
 *	it on top of the stack.
 *
 * RETURNS
 *	Nothing.
 */

VOID	push_down_grid(RAY *r, VOXEL *v)
	{
	INT	n;		       /* # cells per axis in new grid.      */
	INT	small;
	INT	i_in, i_out, i, il, ih;
	REAL	wc[3];		       /* Entry point of grid in world coord.*/
	REAL	ti;
	REAL	del1, del2;
	REAL	t_in, t_out, tl, th;
	REAL	t[6], min[3];
	GRID	*new_g;
	RAYINFO *new_ri;
	RAYINFO *old_ri;

	old_ri = r->ri;
	new_g  = (GRID *)v->cell;

	new_ri = ma_rayinfo(r);
	r->ri  = new_ri;

	new_ri->next = old_ri;		/* Push new RAYINFO onto the stack.  */
	new_ri->grid = new_g;

	n = new_g->indx_inc[1];

	new_ri->delta[0] = old_ri->delta[0]/n;
	new_ri->delta[1] = old_ri->delta[1]/n;
	new_ri->delta[2] = old_ri->delta[2]/n;

	if (old_ri->t_in >= 0.0)
		{
		/* Ray origin outside the voxel. */

		new_ri->entry_plane = old_ri->entry_plane;
		new_ri->t_in = old_ri->t_in;
		ti = old_ri->t_in + eps_t;
		}
	else
		{
		/* Ray origin inside the voxel. */

		ti= 0.0;
		new_ri->entry_plane = -1;  /* no entry plane calculated */
		new_ri->t_in = old_ri->t_in;
		}

	wc[0] = ti*r->D[0] + r->P[0];
	wc[1] = ti*r->D[1] + r->P[1];
	wc[2] = ti*r->D[2] + r->P[2];


	new_ri->index3D[0] = (INT)((wc[0] - new_g->min[0]) / new_g->cellsize[0]);

	if (new_ri->index3D[0] < 0)
		new_ri->index3D[0] = 0;

	if (new_ri->index3D[0] >= n)
		new_ri->index3D[0] = n - 1;


	new_ri->index3D[1] = (INT)((wc[1] - new_g->min[1]) / new_g->cellsize[1]);

	if (new_ri->index3D[1] < 0)
		new_ri->index3D[1] = 0;

	if (new_ri->index3D[1] >= n)
		new_ri->index3D[1] = n - 1;


	new_ri->index3D[2] = (INT)((wc[2] - new_g->min[2]) / new_g->cellsize[2]);

	if (new_ri->index3D[2] < 0)
		new_ri->index3D[2] = 0;

	if (new_ri->index3D[2] >= n)
		new_ri->index3D[2] = n - 1;


	new_ri->index1D = new_ri->index3D[0] + new_ri->index3D[1]*n + new_ri->index3D[2]*new_g->indx_inc[2];

	new_ri->indx_inc1D[0] = r->indx_inc3D[0];
	new_ri->indx_inc1D[1] = r->indx_inc3D[1]*n;
	new_ri->indx_inc1D[2] = r->indx_inc3D[2]*new_g->indx_inc[2];

	switch (new_ri->entry_plane)
		{
		case 3: /* max X */
			new_ri->entry_plane = 0;

		case 0: /* min X */
			new_ri->d[0] = new_ri->t_in + new_ri->delta[0];

			del1 = fmod((double)(old_ri->d[1] - new_ri->t_in),
			(double)new_ri->delta[1]);

			if (del1 <= eps_t)
				new_ri->d[1] = new_ri->t_in + new_ri->delta[1];
			else
				new_ri->d[1] = new_ri->t_in + del1;

			del2= fmod((double)(old_ri->d[2] - new_ri->t_in),
			(double)new_ri->delta[2]);

			if (del2 <= eps_t)
				new_ri->d[2] = new_ri->t_in + new_ri->delta[2];
			else
				new_ri->d[2] = new_ri->t_in + del2;

			small = new_ri->d[0] <= new_ri->d[1] ? 0 : 1;
			small = new_ri->d[small] <= new_ri->d[2] ? small : 2;

			new_ri->t_out = new_ri->d[small];
			new_ri->exit_plane = small;
			break;

		case 4: /* max Y */
			new_ri->entry_plane = 1;

		case 1: /* min Y */
			new_ri->d[1] = new_ri->t_in + new_ri->delta[1];

			del1 = fmod((double)(old_ri->d[0] - new_ri->t_in),
			(double)new_ri->delta[0]);

			if (del1 <= eps_t)
				new_ri->d[0] = new_ri->t_in + new_ri->delta[0];
			else
				new_ri->d[0] = new_ri->t_in + del1;

			del2 = fmod((double)(old_ri->d[2] - new_ri->t_in),
			(double)new_ri->delta[2]);

			if (del2 <= eps_t)
				new_ri->d[2] = new_ri->t_in + new_ri->delta[2];
			else
				new_ri->d[2] = new_ri->t_in + del2;

			small = new_ri->d[0] <= new_ri->d[1] ? 0 : 1;
			small = new_ri->d[small] <= new_ri->d[2] ? small : 2;

			new_ri->t_out = new_ri->d[small];
			new_ri->exit_plane = small;
			break;

		case 5: /* max Z */
			new_ri->entry_plane = 2;

		case 2: /* min Z */
			new_ri->d[2] = new_ri->t_in + new_ri->delta[2];

			del1 = fmod((double)(old_ri->d[0] - new_ri->t_in),
			(double)new_ri->delta[0]);

			if (del1 <= eps_t)
				new_ri->d[0] = new_ri->t_in + new_ri->delta[0];
			else
				new_ri->d[0] = new_ri->t_in + del1;

			del2 = fmod((double)(old_ri->d[1] - new_ri->t_in),
			(double)new_ri->delta[1]);

			if (del2 <= eps_t)
				new_ri->d[1] = new_ri->t_in + new_ri->delta[1];
			else
				new_ri->d[1] = new_ri->t_in + del2;

			small = new_ri->d[0] <= new_ri->d[1] ? 0 : 1;
			small = new_ri->d[small] <= new_ri->d[2] ? small : 2;

			new_ri->t_out = new_ri->d[small];
			new_ri->exit_plane = small;
			break;

		case -1: /* No entry plane calculated, origin inside voxel. */

			min[0] = new_g->min[0] + new_ri->index3D[0]*new_g->cellsize[0];
			min[1] = new_g->min[1] + new_ri->index3D[1]*new_g->cellsize[1];
			min[2] = new_g->min[2] + new_ri->index3D[2]*new_g->cellsize[2];

			if (r->D[0] == 0.0)
				{
				if (r->P[0] >= min[0] && r->P[0] <=  min[0] + new_g->cellsize[0])
					t[0] = -HUGE_REAL;
				else
					t[0] =	HUGE_REAL;
				}
			else
				t[0] = (min[0] - r->P[0]) / r->D[0];


			if (r->D[1] == 0.0)
				{
				if (r->P[1] >= min[1] && r->P[1] <=  min[1] + new_g->cellsize[1])
					t[1] = -HUGE_REAL;
				else
					t[1] =	HUGE_REAL;
				}
			else
				t[1] = (min[1] - r->P[1]) / r->D[1];


			if (r->D[2] == 0.0)
				{
				if (r->P[2] >= min[2] && r->P[2] <=  min[2] + new_g->cellsize[2])
					t[2] = -HUGE_REAL;
				else
					t[2] =	HUGE_REAL;
				}
			else
				t[2] = (min[2] - r->P[2]) / r->D[2];


			if (r->D[0] == 0.0)
				{
				if (r->P[0] >= min[0] && r->P[0] <=  min[0] + new_g->cellsize[0])
					t[3] = HUGE_REAL;
				else
					t[3] = HUGE_REAL;
				}
			else
				t[3] = (min[0] + new_g->cellsize[0] - r->P[0]) / r->D[0];


			if (r->D[1] == 0.0)
				{
				if (r->P[1] >= min[1] && r->P[1] <=  min[1] + new_g->cellsize[1])
					t[4] = HUGE_REAL;
				else
					t[4] = HUGE_REAL;
				}
			else
				t[4] = (min[1] + new_g->cellsize[1] - r->P[1]) / r->D[1];

			if (r->D[2] == 0.0)
				{
				if (r->P[2] >= min[2] && r->P[2] <=  min[2] + new_g->cellsize[2])
					t[5] = HUGE_REAL;
				else
					t[5] = HUGE_REAL;
				}
			else
				t[5] = (min[2] + new_g->cellsize[2] - r->P[2]) / r->D[2];

			t_in  = -HUGE_REAL;
			i_in  = -1;
			t_out =  HUGE_REAL;
			i_out = -1;

			for (i = 0; i < 3; i++)
				{
				if (t[i] < t[i + 3])
					{
					tl = t[i];
					il = i;
					th = t[i + 3];
					ih = i + 3;
					}
				else
					{
					tl = t[i + 3];
					il = i + 3;
					th = t[i];
					ih = i;
					}

				new_ri->d[i] = th;
//				if (t_in < tl)			/* max min   */
				if (t_in - tl < 0.0)
					{
					t_in = tl;
					i_in = il;
					}

//				if (t_out > th) 		/* min max   */
				if (t_out - th > 0.0)
					{
					t_out = th;
					i_out = ih;
					}
				}

//			if ((t_in > t_out) || (t_out < 0.0))
			if ((t_in - t_out > 0.0) || (t_out < 0.0))
				{
				fprintf(stderr, "push_down_grid: Ray origin not in voxel \n");
				exit(-1);
				}

			if (i_in > 2)
				i_in -= 3;

			if (i_out > 2)
				i_out -= 3;

			new_ri->entry_plane = i_in;
			new_ri->t_in	    = t_in;
			new_ri->t_out	    = t_out;
			new_ri->exit_plane  = i_out;
			break;
		}
	}



/*
 * NAME
 *	step_grid -
 *
 * SYNOPSIS
 *	INT	step_grid(r)
 *	RAY	*r;
 *
 * DESCRIPTION
 *	Step to next voxel on grid and return index1D, updating RAYINFO in the
 *	process, return -1 if step carries off the current grid, index1D in
 *	RAYINFO is not valid if off the grid.  Assume that ray & RAYINFO are
 *	all initialized and that the current point along the ray is in the
 *	cell indentified by index1D & index3D[3].  The corresponding grid and
 *	RAYINFO are on the top of their stacks resp.
 *
 * RETURNS
 *	Nothing.
 */

INT	step_grid(RAY *r)
	{
	INT	n;			/* # cells per axis in grid.	     */
	INT	small;			/* Index of closest cell boundary.   */
	INT	*indx;
	RAY	*ra;
	GRID	*gr;
	RAYINFO *rinfo;

	ra    = r;
	rinfo = r->ri;
	gr    = rinfo->grid;
	indx  = gr->indx_inc;
	n     = indx[1];		/*  n = r->ri->grid->indx_inc[1];    */


	rinfo->t_in = rinfo->t_out;
	rinfo->index3D[rinfo->exit_plane] += r->indx_inc3D[rinfo->exit_plane];
	rinfo->entry_plane = rinfo->exit_plane;

	if (rinfo->index3D[rinfo->exit_plane] < 0 || rinfo->index3D[rinfo->exit_plane] >= n)
		{
		/* Out of range, off the grid. */

		return (-1);
		}
	else
		{
		/* Within current grid. */

		rinfo->d[rinfo->exit_plane] += rinfo->delta[rinfo->exit_plane];
		rinfo->index1D += rinfo->indx_inc1D[rinfo->exit_plane];

		/* Update exit info. */

		small = rinfo->d[0] <= rinfo->d[1] ? 0 : 1;
		small = rinfo->d[small] <= rinfo->d[2] ? small : 2;

		rinfo->t_out	  = rinfo->d[small];
		rinfo->exit_plane = small;

		return (rinfo->index1D);
		}
	}



/*
 * NAME
 *	next_voxel -
 *
 * SYNOPSIS
 *	INT	next_voxel(r)
 *	RAY	*r;
 *
 * DESCRIPTION
 *	Next_voxel() may move up or down in the heirarchy and make appropriate
 *	adjustments to the stack.  The routine returns the index1D of the next
 *	voxel.	An index == -1 indicates the ray exited the space.  Assume
 *	that ray & RAYINFO are all initialized and that the current point
 *	along the ray is in the cell indentified by index1D & index3D[3].  The
 *	corresponding grid and RAYINFO are on the top of their stacks resp.
 *
 * RETURNS
 *
 */

INT	next_voxel(RAY *r)
	{
	INT	indx;
	GRID	*gr;
	RAYINFO *rinfo;

	while ((indx = step_grid(r)) == -1)
		{
		/* Step carried off grid. */

		rinfo = r->ri;
		gr = rinfo->grid;

		if (gr->subdiv_level != 0)
			{
			pop_up_a_grid(r);
			indx = r->ri->index1D;
			}
		else
			{
			/* Top level & off grid. */
			/* Exited world space.	 */

			return (-1);
			}
		}

	return (indx);
	}



/*
 * NAME
 *	next_nonempty_voxel -
 *
 * SYNOPSIS
 *	VOXEL	*next_nonempty_voxel(r)
 *	RAY	*r;
 *
 * DESCRIPTION
 *	Next_nonempty_voxel() may move up or down in the heirarchy and make
 *	appropriate adjustments to the stack.  The routine returns a pointer
 *	to the next nonempty voxel.  A zero pointer indicates the ray exited
 *	the space.  Assume that ray & RAYINFO are all initialized and that the
 *	current point along the ray is in the cell indentified by index1D &
 *	index3D[3].  The corresponding grid and RAYINFO are on the top of
 *	their stacks resp.
 *
 * RETURNS
 *	A pointer to the next nonempty voxel.
 */

VOXEL	*next_nonempty_voxel(RAY *r)
	{
	INT	indx;
	VOXEL	*v;
	GRID	*gr;
	RAYINFO *rinfo;

	indx = next_voxel(r);
	if (indx < 0)
		return (NULL);

	rinfo = r->ri;
	gr    = rinfo->grid;

	while (lookup_emptycells(indx, gr) == EMPTY)
		{
		indx = next_voxel(r);

		if (indx < 0) {
			return (NULL);
			}

		rinfo = r->ri;
		gr = rinfo->grid;
		}

	/* Found a nonempty cell. */

	v = lookup_hashtable(indx, gr);

	return (v);
	}



/*
 * NAME
 *	next_nonempty_leaf -
 *
 * SYNOPSIS
 *	VOXEL	*next_nonempty_leaf(r , step, status)
 *	RAY	*r;
 *	INT	step;
 *	INT	*status;
 *
 *	Step = 1 implies step & move up/down in the heirarchy to a leaf node.
 *	Step = 0 implies move through the heirarchy to a nonempty leaf node,
 *	stepping to the next voxel if necessary.
 *
 * DESCRIPTION
 *	Next_nonempty_leaf steps to the next nonempty leaf node moving up or
 *	down in the heirarchy as required and makes appropriate adjustments to
 *	the stack.  If the routine is called with step = 0, then the ray must
 *	be in a nonempty voxel to begin with and will move to the first
 *	nonempty leaf without stepping if possible.
 *
 *	The routine returns a pointer to the next nonempty voxel.  A zero
 *	pointer indicates that a nonempty voxel was not found and the status
 *	word can be consulted to determine why.  If the voxel is in a remote
 *	cluster, the ray is sent to that cluster and NULL is returned.
 *
 *	Assume that ray & RAYINFO are all initialized and that the current
 *	point along the ray is in the cell indentified by index1D &
 *	index3D[3].  The corresponding RAYINFO are on the top of their stacks
 *	resp.
 *
 * RETURNS
 *	Nothing.
 */

VOXEL	*next_nonempty_leaf(RAY *r, INT step, INT *status)
	{
	INT	indx;
	VOXEL	*v;
	RAYINFO *rinfo;

	if (step == STEP)
		v = next_nonempty_voxel(r);
	else
		{
		/* Only used by init_ray when step == 0. */

		rinfo = r->ri;
		v = lookup_hashtable(rinfo->index1D, rinfo->grid);
		}

	if (v == NULL)
		{
		*status =  EXITED_WORLD;
		return (v);
		}

	/* Nonempty voxel either a GRID or a nonempty leaf. */

	while (v->celltype == REMOTE_GRID || v->celltype == GSM_GRID)
		{
		if (v->celltype == REMOTE_GRID)
			{
			send_ray(r, v);
			*status = SENT_TO_REMOTE;
			return (NULL);
			}

		push_down_grid(r, v);

		rinfo = r->ri;
		indx  = rinfo->index1D;

		if (lookup_emptycells(indx, rinfo->grid) != EMPTY)
			{
			v = lookup_hashtable(indx, rinfo->grid);
			if (v->celltype != REMOTE_GRID && v->celltype != GSM_GRID)
				{
				/* Nonempty leaf. */

				if (v->celltype == REMOTE_VOXEL)
					{
					send_ray(r, v);
					*status = SENT_TO_REMOTE;
					return (NULL);
					}

				return (v);
				}

			/* Nonempty grid so do another while loop. */
			}
		else
			{
			v = next_nonempty_leaf(r, STEP, status);

			return (v);
			}
		}

	return (v);
	}



/*
 * NAME
 *	init_ray -
 *
 * SYNOPSIS
 *	VOXEL	*init_ray(r, g)
 *	RAY	*r;
 *	GRID	*g;			// World grid.
 *
 * DESCRIPTION
 *	Add rayinfo to a ray & intersect with world grid and find the initial
 *	nonempty leaf voxel.
 *
 * RETURNS
 *	If the ray hits a nonempty leaf voxel a pointer to that voxel is
 *	returned, otherwise NULL is returned.
 */

VOXEL	*init_ray(RAY *r, GRID *g)
	{
	INT	status;
	INT	indx, grid_id;
	INT	i_in, i_out, i, il, ih;
	REAL	t_in, t_out, tl, th;
	REAL	t[6];
	VOXEL	*v;
	GRID	*gr;
	RAYINFO *ri;

	reset_rayinfo(r);

	if (r->D[0] == 0.0)
		{
		if (r->P[0] >= g->min[0] && r->P[0] <=	g->min[0] + g->cellsize[0])
			t[0] = -HUGE_REAL;
		else
			t[0] =	HUGE_REAL;
		}
	else
		t[0] = (g->min[0] - r->P[0]) / r->D[0];


	if (r->D[1] == 0.0)
		{
		if (r->P[1] >= g->min[1] && r->P[1] <=	g->min[1] + g->cellsize[1])
			t[1] = -HUGE_REAL;
		else
			t[1] =	HUGE_REAL;
		}
	else
		t[1] = (g->min[1] - r->P[1]) / r->D[1];


	if (r->D[2] == 0.0)
		{
		if (r->P[2] >= g->min[2] && r->P[2] <=	g->min[2] + g->cellsize[2])
			t[2] = -HUGE_REAL;
		else
			t[2] =	HUGE_REAL;
		}
	else
		t[2] = (g->min[2] - r->P[2]) / r->D[2];


	if (r->D[0] == 0.0)
		{
		if (r->P[0] >= g->min[0] && r->P[0] <=	g->min[0] + g->cellsize[0])
			t[3] = HUGE_REAL;
		else
			t[3] = HUGE_REAL;
		}
	else
		t[3] = (g->min[0] + g->cellsize[0] - r->P[0]) / r->D[0];


	if (r->D[1] == 0.0)
		{
		if (r->P[1] >= g->min[1] && r->P[1] <=	g->min[1] + g->cellsize[1])
			t[4] = HUGE_REAL;
		else
			t[4] = HUGE_REAL;
		}
	else
		t[4] = (g->min[1] + g->cellsize[1] - r->P[1]) / r->D[1];


	if (r->D[2] == 0.0)
		{
		if (r->P[2] >= g->min[2] && r->P[2] <=	g->min[2] + g->cellsize[2])
			t[5] = HUGE_REAL;
		else
			t[5] = HUGE_REAL;
		}
	else
		t[5] = (g->min[2] + g->cellsize[2] - r->P[2]) / r->D[2];


	t_in  = -HUGE_REAL;
	i_in  = -1;
	t_out =  HUGE_REAL;
	i_out = -1;

	for (i = 0 ; i < 3; i++)
		{
		if (t[i] < t[i + 3])
			{
			tl = t[i];
			il = i;
			th = t[i + 3];
			ih = i+3;
			}
		else
			{
			tl = t[i + 3];
			il = i + 3;
			th = t[i];
			ih = i;
			}

		if (t_in < tl)					/* max min   */
			{
			t_in = tl;
			i_in = il;
			}

		if (t_out > th) 				/* min max   */
			{
			t_out = th;
			i_out = ih;
			}
		}

	if (t_in >= t_out || t_out < 0.0)
		{
		return (NULL);			/* Ray missed world cube.    */
		}

	ri	 = ma_rayinfo(r);
	r->ri	 = ri;
	ri->grid = g;


	/* Store exit t[]s. */

	if (t[0] >= t[3])
		ri->d[0] = t[0];
	else
		ri->d[0] = t[3];


	if (t[1] >= t[4])
		ri->d[1] = t[1];
	else
		ri->d[1] = t[4];


	if (t[2] >= t[5])
		ri->d[2] = t[2];
	else
		ri->d[2] = t[5];


	if (i_in > 2)
		i_in  -= 3;

	if (i_out > 2)
		i_out -= 3;


	ri->entry_plane = i_in;
	ri->t_in	= t_in;
	ri->t_out	= t_out;
	ri->exit_plane	= i_out;

	ri->delta[0] =	r->D[0] == 0.0 ? HUGE_REAL : g->cellsize[0] / ABS(r->D[0]);
	ri->delta[1] =	r->D[1] == 0.0 ? HUGE_REAL : g->cellsize[1] / ABS(r->D[1]);
	ri->delta[2] =	r->D[2] == 0.0 ? HUGE_REAL : g->cellsize[2] / ABS(r->D[2]);

	ri->index3D[0] = 0;
	ri->index3D[1] = 0;
	ri->index3D[2] = 0;

	r->indx_inc3D[0] = r->D[0] >= 0.0 ? 1 : -1;
	r->indx_inc3D[1] = r->D[1] >= 0.0 ? 1 : -1;
	r->indx_inc3D[2] = r->D[2] >= 0.0 ? 1 : -1;

	ri->index1D = 0;

	ri->indx_inc1D[0] = r->D[0] >= 0.0 ? 1 : -1;
	ri->indx_inc1D[1] = r->D[1] >= 0.0 ? 1 : -1;
	ri->indx_inc1D[2] = r->D[2] >= 0.0 ? 1 : -1;
	ri->next = NULL;

	/*
	 *	At this point the ray is in the top grid in the hierarchy it
	 *	must now descend to a leaf cell.
	 */

	if ((v = next_nonempty_leaf(r, NO_STEP, &status)) != NULL)
		{
		ri	= r->ri;
		indx	= ri->index1D;
		gr	= ri->grid;
		grid_id = gr->id;
		}
	else
		{
		return (NULL);
		}

	return (v);
	}

