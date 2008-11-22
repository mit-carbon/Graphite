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
 *	memory.c
 *
 * DESCRIPTION
 *	This file contains routines that handle all local and global memory
 *	allocation and deallocation for the ray tracer.
 *
 *	Various statistics are maintained and printed by these routines as
 *	well.
 *
 */


#include <stdio.h>
#include <math.h>
#include "rt.h"



#define CKSM		0x55AA55AA
#define PAGESIZE	(4*1024)
#define THRESHOLD	(sizeof(NODE) + 16)
#define ROUND_UP(x)	((((x) + 7) >> 3) << 3)
#define ROUND_DN(x)	(x & 0xFFFFFFF8)
#define NODE_ADD(x, y)	(NODE huge *)((U8 huge *)(x) + (y))



/*
 *	Define variables local to this module.
 */

UINT		nodesize;		/* Size of a node structure.	    */
NODE	huge	*begmem;		/* Ptr to first byte in heap.	    */
NODE	huge	*endmem;		/* Prt to last byte in heap.	    */



/*
 *	Allocate storage for memory usage statistics counters.
 */

INT	mem_grid;
INT	maxmem_grid;

INT	mem_voxel;
INT	maxmem_voxel;

INT	mem_hashtable;
INT	maxmem_hashtable;

INT	mem_emptycells;
INT	maxmem_emptycells;

INT	mem_bintree;
INT	maxmem_bintree;

INT	mem_pepArray;
INT	maxmem_pepArray;



/*
 * NAME
 *	LocalMalloc - allocate local memory and check validity
 *
 * SYNOPSIS
 *	VOID	*LocalMalloc(n, msg)
 *	UINT	n;			// Number of bytes to allocate.
 *	CHAR	*msg;			// String to print if it fails.
 *
 * DESCRIPTION
 *	Malloc is simply provided as a single point of control for local
 *	memory allocation.  Because checks for the validity of the allocation
 *	are made in this routine, they need not be performed by the caller.
 *	If the call fails, the caller can identify the context by including
 *	an appropriate identification string pointed to by msg.  This string
 *	will become a part of the RIP message.
 *
 * RETURNS
 *	A pointer to the new memory if successful, otherwise the routine
 *	generates an error exit.
 */

VOID	*LocalMalloc(UINT n, CHAR *msg)
	{
	VOID	*p;

	p = (VOID *) /*malloc*/G_MALLOC(n);
	if (!p)
		{
		printf("%s: %s cannot allocate local memory.\n", ProgName, msg);
		exit(-1);
		}

	return (p);
	}



/*
 * NAME
 *	LocalFree - deallocate local memory
 *
 * SYNOPSIS
 *	VOID	LocalFree(n)
 *	VOID	*p;
 *
 * DESCRIPTION
 *	As currently implemented, LocalFree() simply calls the standard
 *	library function free(), and thus is not strictly needed.  However, it
 *	does provide single point control for local memory deallocation such
 *	that future changes to this routine may make it more useful.
 *
 * RETURNS
 *	Nothing.
 */

VOID	LocalFree(VOID *p)
	{
	free(p);

	}



/*
 * NAME
 *	GlobalHeapWalk - walk the global heap and print info
 *
 * SYNOPSIS
 *	VOID	GlobalHeapWalk()
 *
 * DESCRIPTION
 *
 * RETURNS
 *	Nothing.
 *
 */

VOID	GlobalHeapWalk()
	{
	NODE	huge	*curr;

	LOCK(gm->memlock)
	curr = begmem;

	printf("freelist ->\t0x%08lX\n\n", (U32)gm->freelist);

	printf("node addr \tnode->next\tnode->size\tnode->free\tnode->cksm\n");
	printf("==========\t==========\t==========\t==========\t==========\n");

	while (curr < endmem)
		{
		printf("0x%08lX\t0x%08lX\t%10ld\t%s\t\t0x%08lX\n",
			(U32)curr, (U32)curr->next, curr->size,
			(curr->free ? "FREE" : "    "), curr->cksm);

		if (curr->cksm != CKSM)
			{
			fprintf(stderr, "GlobalHeapWalk: Invalid checksum in node.\n");
			exit(1);
			}

		curr = NODE_ADD(curr, curr->size + nodesize);
		}

	UNLOCK(gm->memlock)
	}



/*
 * NAME
 *	GlobalHeapInit - initialize the global memory management system
 *
 * SYNOPSIS
 *	BOOL	GlobalHeapInit(size)
 *	UINT	size;			// Size in bytes of global memory.
 *
 * DESCRIPTION
 *	GlobalHeapInit initializes the global memory management system by
 *	setting up the heap.  It MUST be called by one (and only one)
 *	processor before any of the other memory management functions are
 *	called.  Typically, this is done by the main() routine just after the
 *	MAIN INITENV call is made.
 *
 *	GlobalHeapInit attempts to allocate size bytes of global shared memory
 *	from the multiprocessor OS.  If the request is satisfied, the memory
 *	block becomes the first node on the free list.
 *
 * RETURNS
 *	TRUE if the global memory heap of the requested size could be allocated;
 *	FALSE otherwise.
 */

BOOL	GlobalHeapInit(UINT size)
	{
	size	     = ROUND_UP(size);
	gm->freelist = (NODE huge *)G_MALLOC(size);

	if (!gm->freelist)
		return (FALSE);

	nodesize = sizeof(NODE);
	begmem	 = gm->freelist;
	endmem	 = NODE_ADD(gm->freelist, size);

	gm->freelist->size = size - nodesize;
	gm->freelist->next = NULL;
	gm->freelist->free = TRUE;
	gm->freelist->cksm = CKSM;

/* NOTE TO USERS: Here's where one can allocate the memory segment from
	begmem to endmem round-robin among memories or however one desires */

	return (TRUE);
	}



/*
 * NAME
 *	GlobalMalloc - allocate a node of given size from global memory
 *
 * SYNOPSIS
 *	VOID	*GlobalMalloc(size, msg)
 *	UINT	size;			// Size of block in bytes.
 *	CHAR	*msg;			// Size of block in bytes.
 *
 * DESCRIPTION
 *	The GlobalMalloc() function allocates space for an object of size
 *	bytes and returns a pointer to the space.  This function does not
 *	modify the memory it allocates.  The storage space pointed to by the
 *	return value is guaranteed to be suitably aligned for storage of any
 *	type of object.
 *
 * NOTES
 *	Because checks for the validity of the allocation are made in this
 *	routine, they need not be performed by the caller.  If the call fails,
 *	the caller can identify the context by including an appropriate
 *	identification string pointed to by msg.  This string will become a
 *	part of the RIP message.
 *
 * RETURNS
 *	A pointer to the allocated space if successful.  NULL if size is 0.
 *	Generates error exit if there is insufficient memory available.
 */

VOID	*GlobalMalloc(UINT size, CHAR *msg)
	{
	NODE	huge   *prev;
	NODE	huge   *curr;
	NODE	huge   *next;

	if (!size)
		return (NULL);

	LOCK(gm->memlock)

	prev = NULL;
	curr = gm->freelist;
	size = ROUND_UP(size);

	/*
	 *	Scan through list for large enough node (first fit).
	 */

	while (curr && curr->size < size)
		{
		if (curr->cksm != CKSM)
			{
			fprintf(stderr, "GlobalMalloc: Invalid checksum in node.\n");
			exit(1);
			}

		if (curr->free != TRUE)
			{
			fprintf(stderr, "GlobalMalloc: Node in free list not marked as free.\n");
			exit(1);
			}

		prev = curr;
		curr = curr->next;
		}


	if (!curr)
		{
		fprintf(stderr, "%s: %s cannot allocate global memory.\n", ProgName, msg);
		exit(-1);
		}


	/*
	 *	If node is larger than needed, free extra space at end
	 *	by inserting remaining space into free list.
	 */

	if (curr->size - size > THRESHOLD)
		{
		next	    = NODE_ADD(curr, nodesize + size);
		next->size  = curr->size - nodesize - size;
		next->next  = curr->next;
		next->free  = TRUE;
		next->cksm  = CKSM;
		curr->size  = size;
		}
	else
		next = curr->next;


	if (!prev)
		gm->freelist = next;
	else
		prev->next   = next;


	UNLOCK(gm->memlock)
	curr->next = NULL;
	curr->free = FALSE;
	curr	   = NODE_ADD(curr, nodesize);

	return ((VOID *)curr);
	}



/*
 * NAME
 *	GlobalCalloc - allocate n elements of given size and initialize to 0
 *
 * SYNOPSIS
 *	VOID	*GlobalCalloc(n, size)
 *	UINT	n;			// Number of elements to allocate.
 *	UINT	size;			// Size of each element in bytes.
 *
 * DESCRIPTION
 *	The GlobalCalloc() function allocates space for n elements of size
 *	bytes each.  It initializes each element to 0, and returns a pointer
 *	to the allocated space.  The storage space pointed to by the return
 *	value is guaranteed to be suitably aligned for storage of any type of
 *	object.
 *
 * RETURNS
 *	A pointer to the allocated space if successful.  NULL if n or size are
 *	0.  Generates error exit if there is insufficient memory available.
 *
 */

VOID	*GlobalCalloc(UINT n, UINT size)
	{
	UINT	nbytes;
	UINT	huge	*p;
	VOID	huge	*q;

	nbytes = ROUND_UP(n*size);

	p = q = GlobalMalloc(nbytes, "GlobalCalloc");

	nbytes >>= 2;				/* Need size in words.	     */
	while (nbytes--)
		*p++ = 0;

	return (q);
	}



/*
 * NAME
 *	GlobalRealloc - reallocate a node to a new size
 *
 * SYNOPSIS
 *	VOID	*GlobalRealloc(p, size)
 *	VOID	*p;			// Ptr to object to change.
 *	UINT	size;			// New size in bytes of node.
 *
 * DESCRIPTION
 *	The GlobalRealloc() function changes the size of the allocated memory
 *	pointed to by p to the number of bytes specified by size.  The
 *	contents of the memory space (up to the lesser of the old and new
 *	sizes) is not changed.
 *
 *	There are three main cases to consider:
 *
 *	Allocating a new block:
 *
 *		If p is NULL, then GlobalRealloc() behaves like
 *		GlobalMalloc().  In other words, it attempts to allocate the
 *		requested number of bytes.
 *
 *	Shrinking an old block:
 *
 *		Shrinking a block is always possible (assuming valid
 *		arguments).  If the new size is less than the old size, the
 *		memory block is truncated at the new size and the remaining
 *		memory is freed.
 *
 *		If size = 0 and p is not NULL, then the entire memory block is
 *		freed and NULL is returned.
 *
 *	Expanding an old block:
 *
 *		If the new size is larger than the old size, this function
 *		first attempts to append more contiguous memory on the end of
 *		the old block.	If successful, the return value is the same as
 *		p.
 *
 *		If there is no free space following the old block, this
 *		function then tries allocate space of the new size elsewhere
 *		in memory.  If successful, the old block will be moved
 *		(copied) to the new block.  The old block will be freed, and a
 *		pointer to the new block returned.
 *
 *		If the new space cannot be allocated, the original memory
 *		space is not changed and the return value is NULL.
 *
 *
 *	If p points to an unallocated space, an error exit occurs.
 *
 * RETURNS
 *	As stated above.
 */

VOID	*GlobalRealloc(VOID *p, UINT size)
	{
	UINT		oldsize;
	UINT		newsize;
	UINT		totsize;
	VOID	huge	*q;
	UINT	huge	*r;
	UINT	huge	*s;
	NODE	huge	*pn;
	NODE	huge	*prev;
	NODE	huge	*curr;
	NODE	huge	*next;
	NODE	huge	*node;

	if (!size)
		{
		GlobalFree(p);
		return (NULL);
		}

	if (!p)
		return (GlobalMalloc(size, "GlobalRealloc"));


	pn = NODE_ADD(p, -nodesize);		/* Adjust ptr back to arena. */

	if (pn->cksm != CKSM)
		{
		fprintf(stderr, "GlobalRealloc: Attempted to realloc node with invalid checksum.\n");
		exit(1);
		}

	if (pn->free)
		{
		fprintf(stderr, "GlobalRealloc: Attempted to realloc an unallocated node.\n");
		exit(1);
		}


	newsize = ROUND_UP(size);
	oldsize = pn->size;


	/*
	 *	If new size is less than current node size, truncate the node
	 *	and return end to free list.
	 */

	if (newsize <= oldsize)
		{
		if (oldsize - newsize < THRESHOLD)
			return (p);

		pn->size    = newsize;

		next	    = NODE_ADD(p, newsize);
		next->size  = oldsize - nodesize - newsize;
		next->next  = NULL;
		next->free  = FALSE;
		next->cksm  = CKSM;
		next	    = NODE_ADD(next, nodesize);

		GlobalFree(next);
		return (p);
		}


	/*
	 *	New size is bigger than current node.  Try to expand next node
	 *	in list.
	 */

	next	= NODE_ADD(p, oldsize);
	totsize = oldsize + nodesize + next->size;

	LOCK(gm->memlock)
	if (next < endmem && next->free && totsize >= newsize)
		{
		/* Find next in free list. */

		prev = NULL;
		curr = gm->freelist;

		while (curr && curr < next && curr < endmem)
			{
			prev = curr;
			curr = curr->next;
			}

		if (curr != next)
			{
			fprintf(stderr, "GlobalRealloc: Could not find next node in free list.\n");
			exit(1);
			}

		if (totsize - newsize < THRESHOLD)
			{
			/* Just remove next from free list. */

			if (!prev)
				gm->freelist = next->next;
			else
				prev->next   = next->next;

			next->next = NULL;
			next->free = FALSE;
			pn->size   = totsize;

			UNLOCK(gm->memlock)
			return (p);
			}
		else
			{
			/* Remove next from free list while adding node. */

			node	   = NODE_ADD(p, newsize);
			node->next = next->next;
			node->size = totsize - nodesize - newsize;
			node->free = TRUE;
			node->cksm = CKSM;

			if (!prev)
				gm->freelist = node;
			else
				prev->next   = node;

			next->next = NULL;
			next->free = FALSE;
			pn->size   = newsize;

			UNLOCK(gm->memlock)
			return (p);
			}
		}


	/*
	 *	New size is bigger than current node, but next node in list
	 *	could not be expanded.	Try to allocate new node and move data
	 *	to new location.
	 */

	UNLOCK(gm->memlock)

	s = q = GlobalMalloc(newsize, "GlobalRealloc");
	if (!q)
		return (NULL);

	r = (UINT huge *)p;
	oldsize >>= 2;

	while (oldsize--)
		*s++ = *r++;

	GlobalFree(p);
	return (q);
	}



/*
 * NAME
 *	GlobalFree - return a node to the free list
 *
 * SYNOPSIS
 *	VOID	GlobalFree(p)
 *	VOID	*p;
 *
 * DESCRIPTION
 *	The GlobalFree() function deallocates memory space (pointed to by p)
 *	that was previously allocated by a call to GlobalMalloc().  This makes
 *	the memory space available again.  An attempt to free unallocated
 *	memory generates an error exit.
 *
 * RETURNS
 *	Nothing.
 */

VOID	GlobalFree(VOID *p)
	{
	BOOL		pcom;			/* TRUE if prev can combine. */
	BOOL		ncom;			/* TRUE if next can combine. */
	NODE	huge	*pn;
	NODE	huge	*prev;			/* Pointer to previous node. */
	NODE	huge	*curr;			/* Pointer to this node.     */
	NODE	huge	*next;			/* Pointer to next node.     */

	if (!begmem)
		return;


	pn = NODE_ADD(p, -nodesize);		/* Adjust ptr back to arena. */

	if (pn->cksm != CKSM)
		{
		fprintf(stderr, "GlobalFree: Attempted to free node with invalid checksum.\n");
		exit(1);
		}

	if (pn->free)
		{
		fprintf(stderr, "GlobalFree: Attempted to free unallocated node.\n");
		exit(1);
		}


	pcom = FALSE;
	prev = NULL;

	LOCK(gm->memlock)
	if (gm->freelist)
		{
		/*
		 *	Search the memory arena blocks for previous free neighbor.
		 */

		curr = gm->freelist;

		while (curr < pn && curr < endmem)
			{
			if (curr->cksm != CKSM)
				{
				fprintf(stderr, "GlobalFree: Invalid checksum in previous node.\n");
				exit(1);
				}

			if (curr->free)
				{
				prev = curr;
				pcom = TRUE;
				}
			else
				pcom = FALSE;

			curr = NODE_ADD(curr, curr->size + nodesize);
			}


		/*
		 *	Make sure we found the original node.
		 */

		if (curr >= endmem)
			{
			fprintf(stdout, "freelist=0x%p, curr=0x%p, size=0x%lu, pn=0x%p, endmem=0x%p\n", gm->freelist, curr, curr->size, pn, endmem);
			fprintf(stderr, "GlobalFree: Search for previous block fell off end of memory.\n");
			exit(1);
			}
		}


	/*
	 *	Search the memory arena blocks for next free neighbor.
	 */

	ncom = TRUE;
	next = NULL;
	curr = NODE_ADD(pn, pn->size + nodesize);

	while (!next && curr < endmem)
		{
		if (curr->cksm != CKSM)
			{
			fprintf(stderr, "GlobalFree: Invalid checksum in next node.\n");
			exit(1);
			}

		if (curr->free)
			next = curr;
		else
			ncom = FALSE;

		curr = NODE_ADD(curr, curr->size + nodesize);
		}


	if (!next)				/* Loop may have fallen thru.*/
		ncom = FALSE;

	curr = pn;
	curr->free = TRUE;			/* Mark NODE as free.	     */


	/*
	 *	Attempt to combine the three nodes (prev, current, next).
	 *	There are 9 cases to consider (16 total, but 7 are degenerate).
	 */


	if (next && !ncom && pcom)
		{
		prev->next  = next;
		prev->size += curr->size + nodesize;
		}
	else
	if (next && !ncom && prev && !pcom)
		{
		prev->next  = curr;
		curr->next  = next;
		}
	else
	if (next && !ncom && !prev)
		{
		gm->freelist = curr;
		curr->next   = next;
		}
	else
	if (ncom && pcom)
		{
		prev->next  = next->next;
		prev->size += curr->size + next->size + 2*nodesize;
		}
	else
	if (ncom && prev && !pcom)
		{
		prev->next  = curr;
		curr->next  = next->next;
		curr->size += next->size + nodesize;
		}
	else
	if (ncom && !prev)
		{
		gm->freelist = curr;
		curr->next   = next->next;
		curr->size  += next->size + nodesize;
		}
	else
	if (!next && pcom)
		{
		prev->next  = NULL;
		prev->size += curr->size + nodesize;
		}
	else
	if (!next && prev && !pcom)
		{
		prev->next  = curr;
		curr->next  = NULL;
		}
	else
	if (!next && !prev)
		{
		gm->freelist = curr;
		curr->next   = NULL;
		}

	UNLOCK(gm->memlock)
	return;
	}



/*
 * NAME
 *	GlobalMemAvl - return total memory that remains available for allocation
 *
 * SYNOPSIS
 *	UINT	GlobalMemAvl()
 *
 * DESCRIPTION
 *	This function walks the free list and returns the approximate size in
 *	bytes of the memory available for dynamic memory allocation.
 *
 * RETURNS
 *	As stated above.
 */

UINT	GlobalMemAvl()
	{
	UINT	total;
	NODE	huge	*curr;

	LOCK(gm->memlock)
	total = 0;
	curr  = gm->freelist;

	while (curr)
		{
		total += curr->size;
		curr   = curr->next;
		}

	total = ROUND_DN(total);

	UNLOCK(gm->memlock)
	return (total);
	}



/*
 * NAME
 *	GlobalMemMax - return size of largest block that can be allocated
 *
 * SYNOPSIS
 *	UINT	GlobalMemMax()
 *
 * DESCRIPTION
 *	This function walks the free list and returns the size in bytes of the
 *	largest contiguous block of memory that can be allocated from the
 *	heap.
 *
 * RETURNS
 *	As stated above.
 */

UINT	GlobalMemMax()
	{
	UINT	max;
	NODE	huge	*curr;

	LOCK(gm->memlock)
	max  = 0;
	curr = gm->freelist;

	while (curr)
		{
		max  = (curr->size > max ? curr->size : max);
		curr =	curr->next;
		}

	max = ROUND_DN(max);

	UNLOCK(gm->memlock)
	return (max);
	}



/*
 * NAME
 *	ObjectMalloc - allocate various global memory objects
 *
 * SYNOPSIS
 *	VOID	*ObjectMalloc(ObjectType, count)
 *	INT	ObjectType;		// Type of object to allocate.
 *	INT	count;			// Number of objects to allocate.
 *
 * DESCRIPTION
 *	ObjectMalloc provides a way to allocate various ray tracer objects
 *	from global memory.  It computes the size in bytes required for the
 *	objects and also maintains memory usage statistics for each object
 *	type.
 *
 * RETURNS
 *	A pointer to the new object if successful, otherwise the routine
 *	generates an error exit.
 */

VOID	*ObjectMalloc(INT ObjectType, INT count)
	{
	INT	n;
	VOID	*p;

	switch (ObjectType)
		{
		case OT_GRID:
			n = count*sizeof(GRID);
			p = GlobalMalloc(n, "GRID");

			mem_grid += n;
			maxmem_grid = Max(mem_grid, maxmem_grid);
			break;

		case OT_VOXEL:
			n = count*sizeof(VOXEL);
			p = GlobalMalloc(n, "VOXEL");

			mem_voxel += n;
			maxmem_voxel = Max(mem_voxel, maxmem_voxel);
			break;

		case OT_HASHTABLE:
			{
			INT	i;
			VOXEL	**x;

			n = count*sizeof(VOXEL *);
			p = GlobalMalloc(n, "HASHTABLE");
			x = p;

			for (i = 0; i < count; i++)
				x[i] = NULL;

			mem_hashtable += n;
			maxmem_hashtable = Max(mem_hashtable, maxmem_hashtable);
			}
			break;

		case OT_EMPTYCELLS:
			{
			INT	i, w;
			UINT	*x;

			w = 1 + count/(8*sizeof(UINT));
			n = w*sizeof(UINT);
			p = GlobalMalloc(n, "EMPTYCELLS");
			x = p;

			for (i = 0; i < w; i++)
				x[i] = ~0;		/* 1 => empty */

			mem_emptycells += n;
			maxmem_emptycells = Max(mem_emptycells, maxmem_emptycells);
			}
			break;

		case OT_BINTREE:
			n = count*sizeof(BTNODE);
			p = GlobalMalloc(n, "BINTREE");

			mem_bintree += n;
			maxmem_bintree = Max(mem_bintree, maxmem_bintree);
			break;

		case OT_PEPARRAY:
			n = count*sizeof(ELEMENT *);
			p = GlobalMalloc(n, "PEPARRAY");

			mem_pepArray += n;
			maxmem_pepArray = Max(mem_pepArray, maxmem_pepArray);
			break;

		default:
			printf("ObjectMalloc: Unknown object type: %ld\n", ObjectType);
			exit(-1);
		}

	return (p);
	}



/*
 * NAME
 *	ObjectFree - deallocate various global memory objects
 *
 * SYNOPSIS
 *	VOID	*ObjectFree(ObjectType, count, p)
 *	INT	ObjectType;		// Type of object to deallocate.
 *	INT	count;			// Number of objects to deallocate.
 *	VOID	*p;			// The object pointer to deallocate.
 *
 * DESCRIPTION
 *	ObjectFree simply deallocates objects which were obtained by
 *	ObjectMalloc(), updating usage statistics appropriately.
 *
 * RETURNS
 *	Nothing.
 */

VOID	ObjectFree(INT ObjectType, INT count, VOID *p)
	{
	INT	n;

	GlobalFree(p);

	switch (ObjectType)
		{
		case OT_GRID:
			n = count*sizeof(GRID);
			mem_grid -= n;
			break;

		case OT_VOXEL:
			n = count*sizeof(VOXEL);
			mem_voxel -= n;
			break;

		case OT_HASHTABLE:
			n = count*sizeof(VOXEL *);
			mem_hashtable -= n;
			break;

		case OT_EMPTYCELLS:
			n = 1 + count/(8*sizeof(UINT));
			n = n*sizeof(UINT);
			mem_emptycells -= n;
			break;

		case OT_BINTREE:
			n = count*sizeof(BTNODE);
			mem_bintree -= n;
			break;

		case OT_PEPARRAY:
			n = count*sizeof(ELEMENT *);
			mem_pepArray -= n;
			break;

		default:
			printf("ObjectFree: Unknown object type: %ld\n", ObjectType);
			exit(-1);
		}
	}




RAYINFO *ma_rayinfo(RAY *r)
	{
	RAYINFO *p;

	if (r->ri_indx + 1 > MAX_RAYINFO + 1)
		{
		fprintf(stderr, "error ma_rayinfo \n");
		exit(-1);
		}

	p = (RAYINFO *)(&(r->rinfo[r->ri_indx]));

	/*
	 *	It is assumed that rayinfos are allocated and released in a
	 *	stack fashion, i.e. the one released is the one most recently
	 *	allocated.
	 */

	r->ri_indx += 1;

	return (p);
	}



VOID   free_rayinfo(RAY *r)
	{
	r->ri_indx -= 1;
	}



VOID	reset_rayinfo(RAY *r)
	{
	r->ri_indx = 0;
	}



VOID	ma_print()
	{
	INT	mem_total;
	INT	maxmem_total;

	mem_total     = mem_grid + mem_hashtable + mem_emptycells;
	mem_total    += mem_voxel + mem_bintree;

	maxmem_total  = maxmem_grid + maxmem_hashtable + maxmem_emptycells;
	maxmem_total += maxmem_voxel + maxmem_bintree;

	fprintf(stdout, "\n****** Hierarchial uniform grid memory allocation summary ******* \n\n");
	fprintf(stdout, "     < struct >:            < current >   < maximum >    < sizeof > \n");
	fprintf(stdout, "     <  bytes >:             <  bytes >   <   bytes >    <  bytes > \n\n");
	fprintf(stdout, "     grid:                %11ld   %11ld   %11ld \n", mem_grid,        maxmem_grid,        sizeof(GRID)   );
	fprintf(stdout, "     hashtable entries:   %11ld   %11ld   %11ld \n", mem_hashtable,   maxmem_hashtable,   sizeof(VOXEL**));
	fprintf(stdout, "     emptycell entries:   %11ld   %11ld   %11ld \n", mem_emptycells,  maxmem_emptycells,  sizeof(UINT)   );
	fprintf(stdout, "     voxel:               %11ld   %11ld   %11ld \n", mem_voxel,       maxmem_voxel,       sizeof(VOXEL)  );
	fprintf(stdout, "     bintree_node:        %11ld   %11ld   %11ld \n", mem_bintree,     maxmem_bintree,     sizeof(BTNODE) );

	fprintf(stdout, "\n");
	fprintf(stdout, "     Totals:              %11ld   %11ld      \n\n", mem_total,       maxmem_total);
	}

