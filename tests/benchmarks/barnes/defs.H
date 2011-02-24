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

#ifndef _DEFS_H_
#define _DEFS_H_

//#include <assert.h>

#define MAX_PROC 1024
#define MAX_BODIES_PER_LEAF 10
#define MAXLOCK 2048            	/* maximum number of locks on DASH */
#define PAGE_SIZE 4096			/* in bytes */

#define NSUB (1 << NDIM)        /* subcells per cell */

/*
 * BODY and CELL data structures are used to represent the tree:
 *
 *         +-----------------------------------------------------------+
 * root--> | CELL: mass, pos, cost, quad, /, o, /, /, /, /, o, /, done |
 *         +---------------------------------|--------------|----------+
 *                                           |              |
 *    +--------------------------------------+              |
 *    |                                                     |
 *    |    +--------------------------------------+         |
 *    +--> | BODY: mass, pos, cost, vel, acc, phi |         |
 *         +--------------------------------------+         |
 *                                                          |
 *    +-----------------------------------------------------+
 *    |
 *    |    +-----------------------------------------------------------+
 *    +--> | CELL: mass, pos, cost, quad, o, /, /, o, /, /, o, /, done |
 *         +------------------------------|--------|--------|----------+
 *                                       etc      etc      etc
 */

/*
 * NODE: data common to BODY and CELL structures.
 */

typedef struct _node {
   long type;                 /* code for node type: body or cell */
   real mass;                  /* total mass of node */
   vector pos;                 /* position of node */
   long cost;                   /* number of interactions computed */
   long level;
   struct _node *parent;       /* ptr to parent of this node in tree */
   long child_num;              /* Index that this node should be put
				  at in parent cell */
} node;

typedef node* nodeptr;

#define Type(x) (((nodeptr) (x))->type)
#define Mass(x) (((nodeptr) (x))->mass)
#define Pos(x)  (((nodeptr) (x))->pos)
#define Cost(x) (((nodeptr) (x))->cost)
#define Level(x) (((nodeptr) (x))->level)
#define Parent(x) (((nodeptr) (x))->parent)
#define ChildNum(x) (((nodeptr) (x))->child_num)

/*
 * BODY: data structure used to represent particles.
 */

typedef struct _body* bodyptr;
typedef struct _leaf* leafptr;
typedef struct _cell* cellptr;

#define BODY 01                 /* type code for bodies */

typedef struct _body {
   long type;
   real mass;                  /* mass of body */
   vector pos;                 /* position of body */
   long cost;                   /* number of interactions computed */
   long level;
   leafptr parent;
   long child_num;              /* Index that this node should be put */
   vector vel;                 /* velocity of body */
   vector acc;                 /* acceleration of body */
   real phi;                   /* potential at body */
} body;

#define Vel(x)  (((bodyptr) (x))->vel)
#define Acc(x)  (((bodyptr) (x))->acc)
#define Phi(x)  (((bodyptr) (x))->phi)

/*
 * CELL: structure used to represent internal nodes of tree.
 */

#define CELL 02                 /* type code for cells */

typedef struct _cell {
   long type;
   real mass;                  /* total mass of cell */
   vector pos;                 /* cm. position of cell */
   long cost;                   /* number of interactions computed */
   long level;
   cellptr parent;
   long child_num;              /* Index [0..8] that this node should be put */
   long processor;		/* Used by partition code */
   struct _cell *next, *prev;    /* Used in the partition array */
   long seqnum;
#ifdef QUADPOLE
   matrix quad;                /* quad. moment of cell */
#endif
   volatile long done;          /* flag to tell when the c.of.m is ready */
   nodeptr subp[NSUB];         /* descendents of cell */
} cell;

#define Subp(x) (((cellptr) (x))->subp)

/*
 * LEAF: structure used to represent leaf nodes of tree.
 */

#define LEAF 03                 /* type code for leaves */

typedef struct _leaf {
   long type;
   real mass;                  /* total mass of leaf */
   vector pos;                 /* cm. position of leaf */
   long cost;                   /* number of interactions computed */
   long level;
   cellptr parent;
   long child_num;              /* Index [0..8] that this node should be put */
   long processor;		/* Used by partition code */
   struct _leaf *next, *prev;    /* Used in the partition array */
   long seqnum;
#ifdef QUADPOLE
   matrix quad;                /* quad. moment of leaf */
#endif
   volatile long done;          /* flag to tell when the c.of.m is ready */
   long num_bodies;
   bodyptr bodyp[MAX_BODIES_PER_LEAF];         /* bodies of leaf */
} leaf;

#define Bodyp(x)  (((leafptr) (x))->bodyp)

#ifdef QUADPOLE
#define Quad(x) (((cellptr) (x))->quad)
#endif
#define Done(x) (((cellptr) (x))->done)

/*
 * Integerized coordinates: used to mantain body-tree.
 */

#define MAXLEVEL ((8L * (long)sizeof(long)) - 2L)
#define IMAX  (1L << MAXLEVEL)    /* highest bit of int coord */

#endif

