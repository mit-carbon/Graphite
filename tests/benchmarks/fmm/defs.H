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

#ifndef _Defs_H
#define _Defs_H 1

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <limits.h>

/* Define booleans */
#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif
#define NUM_DIMENSIONS 2
#define NUM_DIM_POW_2 4

#undef DBL_MIN
#define DBL_MIN        2.2250738585072014e-308 /* min > 0 val of "double" */

#define TIME_ALL 1  /* non-0 means time each phase within a time step */
#define MY_TIMING (Local[my_id].Timing)
#define MY_TIME_STEP (Local[my_id].Time_Step)

#define MAX_REAL DBL_MAX
#define MIN_REAL DBL_MIN
#define REAL_DIG __DBL_DIG__

#define MAX_PROCS 1024

/* Defines the maximum depth of the tree */
#define MAX_LEVEL 100
#define MAX_TIME_STEPS 10

#define COMPLEX_ADD(a,b,c) \
{ \
  a.r = b.r + c.r; \
  a.i = b.i + c.i; \
}

#define COMPLEX_SUB(a,b,c) \
{ \
  a.r = b.r - c.r; \
  a.i = b.i - c.i; \
}

#define COMPLEX_MUL(a,b,c) \
{ \
  complex _c_temp; \
  \
  _c_temp.r = (b.r * c.r) - (b.i * c.i); \
  _c_temp.i = (b.r * c.i) + (b.i * c.r); \
  a.r = _c_temp.r; \
  a.i = _c_temp.i; \
}

#define COMPLEX_DIV(a,b,c) \
{ \
  real _denom; \
  complex _c_temp; \
   \
  _denom = ((real) 1.0) / ((c.r * c.r) + (c.i * c.i)); \
  _c_temp.r = ((b.r * c.r) + (b.i * c.i)) * _denom; \
  _c_temp.i = ((b.i * c.r) - (b.r * c.i)) * _denom; \
  a.r = _c_temp.r; \
  a.i = _c_temp.i; \
}

#define COMPLEX_ABS(a) \
  sqrt((double) ((a.r * a.r) + (a.i * a.i)))

#define VECTOR_ADD(a,b,c) \
{ \
  a.x = b.x + c.x; \
  a.y = b.y + c.y; \
}

#define VECTOR_SUB(a,b,c) \
{ \
  a.x = b.x - c.x; \
  a.y = b.y - c.y; \
}

#define VECTOR_MUL(a,b,c) \
{ \
  a.x = b.x * c; \
  a.y = b.y * c; \
}

#define VECTOR_DIV(a,b,c) \
{ \
  a.x = b.x / c; \
  a.y = b.y / c; \
}

#define DOT_PRODUCT(a,b) \
((a.x * b.x) + (a.y * b.y))

#define ADD_COST 2
#define MUL_COST 5
#define DIV_COST 19
#define ABS_COST 1

#define U_LIST_COST(a,b) (1.06 * 79.2 * a * b)
#define V_LIST_COST(a) (1.08 * ((35.9 * a * a) + (133.6 * a)))
#define W_LIST_COST(a,b) (1.11 * 29.2 * a * b)
#define X_LIST_COST(a,b) (1.15 * 56.0 * a * b)
#define SELF_COST(a) (7.0 * 61.4 * a * a)

/* SWOO: Did I put this here? If so, you don't need it */
#define CACHE_SIZE 16  /* should be in bytes */

#define PAGE_SIZE 4096
#define PAD_SIZE (PAGE_SIZE / (sizeof(long)))

typedef enum { FALSE = 0, TRUE = 1 } bool_t;

/* These defintions sets the precision of the calculations. To use single
 * precision, simply change double to float and recompile! */
typedef double real;

typedef struct __Complex complex;
struct __Complex {
  real r;
  real i;
};

typedef struct _Vector vector;
struct _Vector {
  real x;
  real y;
};

typedef struct _Time_Info time_info;
struct _Time_Info {
  unsigned long construct_time;
  unsigned long list_time;
  unsigned long partition_time;
  unsigned long inter_time;
  unsigned long pass_time;
  unsigned long intra_time;
  unsigned long barrier_time;
  unsigned long other_time;
  unsigned long total_time;
};

extern unsigned long Number_Of_Processors;
extern double Timestep_Dur;
extern real Softening_Param;
extern long Expansion_Terms;

extern real RoundReal(real val);
extern void PrintComplexNum(complex *c);
extern void PrintVector(vector *v);
extern void LockedPrint(char *format, ...);

#endif /* _Defs_H */
