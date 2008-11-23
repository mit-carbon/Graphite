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


#ifndef _TASK_H
#define _TASK_H


/************************************************************************
*
*     Constants
*
*************************************************************************/

#define PAGE_SIZE 4096   /* page size of system, used for padding to
allow page placement of some logically
per-process data structures */

/*** Task types ***/
#define TASK_MODELING      (1)
#define TASK_BSP           (2)
#define TASK_FF_REFINEMENT (4)
#define TASK_RAY           (8)
#define TASK_RAD_AVERAGE   (16)
#define TASK_VISIBILITY    (32)


/*** Controling parallelism ***/

#define MAX_TASKGET_RETRY (32)	    /* Max # of retry get_task() can make */
#define N_ALLOCATE_LOCAL_TASK (8)   /* get_task() and free_task() transfer
this # of task objects to/from the
global shared queue at a time */


/************************************************************************
*
*     Task Descriptors
*
*************************************************************************/

/* Decompose modeling object into patches (B-reps) */
typedef struct {
    long   type ;		     /* Object type */
    Model *model ;		     /* Object to be decomposed */
} Modeling_Task ;


/* Insert a new patch to the BSP tree */
typedef struct {
    Patch *patch ;                 /* Patch to be inserted */
    Patch *parent ;		     /* Parent node in the BSP tree */
} BSP_Task ;


/* Refine element interaction based on FF value or BF value */
typedef struct {
    Element *e1, *e2 ;	     /* Interacting elements */
    float   visibility ;           /* Visibility of parent */
    long level ;		     /* Path length from the root element */
} Refinement_Task ;


typedef struct {
    long  ray_type ;
    Element *e ;		     /* The element we are interested in */
} Ray_Task ;


typedef struct {
    Element *e ;		     /* The element we are interested in */
    Interaction *inter ;	     /* Top of interactions */
    long   n_inter ;		     /* Number of interactions */
    void  (*k)() ;		     /* Continuation */
} Visibility_Task ;

/* Radiosity averaging task */

#define RAD_AVERAGING_MODE (0)
#define RAD_NORMALIZING_MODE (1)

typedef struct {
    Element *e ;
    long level ;
    long mode ;
} RadAvg_Task ;



/************************************************************************
*
*     Task Definition
*
*************************************************************************/


typedef struct _task {
    long task_type ;
    struct _task *next ;
    union {
        Modeling_Task   model ;
        BSP_Task        bsp ;
        Refinement_Task ref ;
        Ray_Task        ray ;
        Visibility_Task vis ;
        RadAvg_Task     rad ;
    } task ;
} Task ;


typedef struct {
    char pad1[PAGE_SIZE];	 	/* padding to avoid false-sharing
    and allow page-placement */
    LOCKDEC(q_lock)
    Task  *top, *tail ;
    long   n_tasks ;
    LOCKDEC(f_lock)
    long   n_free ;
    Task  *free ;
    char pad2[PAGE_SIZE];	 	/* padding to avoid false-sharing
    and allow page-placement */
} Task_Queue ;


#define TASK_APPEND (0)
#define TASK_INSERT (1)

#define taskq_length(q)   (q->n_tasks)
#define taskq_top(q)      (q->top)
#define taskq_too_long(q)  ((q)->n_tasks > n_tasks_per_queue)

/*
 * taskman.C
 */
void process_tasks(long process_id);
long _process_task_wait_loop(void);
void create_modeling_task(Model *model, long type, long process_id);
void create_bsp_task(Patch *patch, Patch *parent, long process_id);
void create_ff_refine_task(Element *e1, Element *e2, long level, long process_id);
void create_ray_task(Element *e, long process_id);
void enqueue_ray_task(long qid, Element *e, long mode, long process_id);
void create_visibility_tasks(Element *e, void (*k)(), long process_id);
void create_radavg_task(Element *e, long mode, long process_id);
void enqueue_radavg_task(long qid, Element *e, long mode, long process_id);
void enqueue_task(long qid, Task *task, long mode);
Task *dequeue_task(long qid, long max_visit, long process_id);
Task *get_task(long process_id);
void free_task(Task *task, long process_id);
void init_taskq(long process_id);
long check_task_counter(void);
long assign_taskq(long process_id);
void print_task(Task *task);
void print_taskq(Task_Queue *tq);

#endif

