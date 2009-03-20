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

/************************************************************************
 *
 *	Task management.
 *
 *       This module has the following functions.
 *       (1) Allocate/free a task object.
 *       (2) Enqueue/decuque a task object.
 *
 *************************************************************************/

#include <stdio.h>

EXTERN_ENV;

include(radiosity.h)


struct {
    char pad1[PAGE_SIZE];	 	/* padding to avoid false-sharing
                                   and allow page-placement */
    long n_local_free_task ;
    Task *local_free_task ;
    long crnt_taskq_id ;
    char pad2[PAGE_SIZE];	 	/* padding to avoid false-sharing
                                   and allow page-placement */
}  task_struct[MAX_PROCESSORS];

/***************************************************************************
 ****************************************************************************
 *
 *    Methods for Task object
 *
 ****************************************************************************
 ****************************************************************************/
/***************************************************************************
 *
 *    process_tasks()
 *
 *    Process tasks in the task queue. Task type is specified by the mask.
 *    Multiple task types may be specified by bit-oring the task type.
 *
 ****************************************************************************/

#define QUEUES_VISITED (n_taskqueues)
#define DEQUEUE_TASK(q,v,p) (dequeue_task((q),(v),p))

void process_tasks(long process_id)
{
    Task *t ;

    t = DEQUEUE_TASK( taskqueue_id[process_id], QUEUES_VISITED, process_id ) ;

 retry_entry:
    while( t )
        {
            switch( t->task_type )
                {
                case TASK_MODELING:
                    process_model( t->task.model.model, t->task.model.type, process_id ) ;
                    break ;
                case TASK_BSP:
                    define_patch( t->task.bsp.patch, t->task.bsp.parent, process_id ) ;
                    break ;
                case TASK_FF_REFINEMENT:
                    ff_refine_elements( t->task.ref.e1, t->task.ref.e2, 0, process_id ) ;
                    break ;
                case TASK_RAY:
                    process_rays( t->task.ray.e, process_id ) ;
                    break ;
                case TASK_VISIBILITY:
                    visibility_task( t->task.vis.e, t->task.vis.inter,
                                    t->task.vis.n_inter, t->task.vis.k, process_id ) ;
                    break ;
                case TASK_RAD_AVERAGE:
                    radiosity_averaging( t->task.rad.e, t->task.rad.mode, process_id ) ;
                    break ;
                default:
                    fprintf( stderr, "Panic:process_tasks:Illegal task type\n" );
                }

            /* Free the task */
            free_task( t, process_id ) ;

            /* Get next task */
            t = DEQUEUE_TASK( taskqueue_id[process_id], QUEUES_VISITED, process_id ) ;
        }


    /* Barrier. While waiting for other processors to finish, poll the task
       queues and resume processing if there is any task */

    LOCK(global->pbar_lock);
    /* Reset the barrier counter if not initialized */
    if( global->pbar_count >= n_processors )
        global->pbar_count = 0 ;

    /* Increment the counter */
    global->pbar_count++ ;
    UNLOCK(global->pbar_lock);

    /* barrier spin-wait loop */
    while( global->pbar_count < n_processors )
        {
            /* Wait for a while and then retry dequeue */
            if( _process_task_wait_loop() )
                break ;

            /* Waited for a while but other processors are still running.
               Poll the task queue again */
            t = DEQUEUE_TASK( taskqueue_id[process_id], QUEUES_VISITED, process_id ) ;
            if( t )
                {
                    /* Task found. Exit the barrier and work on it */
                    LOCK(global->pbar_lock);
                    global->pbar_count-- ;
                    UNLOCK(global->pbar_lock);
                    goto retry_entry ;
                }

        }

    BARRIER(global->barrier, n_processors);
}


long _process_task_wait_loop()
{
    long  i ;
    long finished = 0 ;

    /* Wait for a while and then retry */
    for( i = 0 ; i < 1000 && ! finished ; i++ )
        {
            if(    ((i & 0xff) == 0) && ((volatile long)global->pbar_count >= n_processors) )

                finished = 1 ;
        }

    return( finished ) ;
}



/***************************************************************************
 *
 *    create_modeling_task()
 *    create_bsp_task()
 *    create_ff_refine_task()
 *    create_ray_task()
 *    create_visibility_task()
 *    create_radavg_task()
 *
 ****************************************************************************/


void create_modeling_task(Model *model, long type, long process_id)
{
    /* Implemented this way (routine just calls another routine)
       for historical reasons */

    process_model( model, type, process_id ) ;
    return ;
}


void create_bsp_task(Patch *patch, Patch *parent, long process_id)
{
    /* Implemented this way (routine just calls another routine) for historical reasons */
    define_patch( patch, parent, process_id ) ;
    return ;
}



void create_ff_refine_task(Element *e1, Element *e2, long level, long process_id)
{
    Task *t ;

    /* Check existing parallelism */
    if( taskq_too_long(&global->task_queue[ taskqueue_id[process_id] ]) )
        {
            /* Task queue is too long. Solve it immediately */
            ff_refine_elements( e1, e2, level, process_id ) ;
            return ;
        }

    /* Create a task */
    t = get_task(process_id) ;
    t->task_type = TASK_FF_REFINEMENT ;
    t->task.ref.e1              = e1 ;
    t->task.ref.e2              = e2 ;
    t->task.ref.level           = level ;

    /* Put in the queue */
    enqueue_task( taskqueue_id[process_id], t, TASK_INSERT ) ;
}




void create_ray_task(Element *e, long process_id)
{
    /* Check existing parallelism */
    if(   ((e->n_interactions + e->n_vis_undef_inter)
           < N_inter_parallel_bf_refine)
       || taskq_too_long(&global->task_queue[ taskqueue_id[process_id] ]) )
        {
            /* Task size is small, or the queue is too long.
               Solve it immediately */
            process_rays( e, process_id ) ;
            return ;
        }

    /* Put in the queue */
    enqueue_ray_task( taskqueue_id[process_id], e, TASK_INSERT, process_id ) ;
}


void enqueue_ray_task(long qid, Element *e, long mode, long process_id)
{
    Task *t ;

    /* Create task object */
    t = get_task(process_id) ;
    t->task_type = TASK_RAY ;
    t->task.ray.e     = e ;

    /* Put in the queue */
    enqueue_task( qid, t, mode ) ;
}


void create_visibility_tasks(Element *e, void (*k)(), long process_id)
{
    long n_tasks ;
    long remainder ;			     /* Residue of MOD(total_undefs)*/
    long i_cnt ;
    Interaction *top, *tail ;
    Task *t ;
    long total_undefs = 0 ;
    long tasks_created = 0 ;

    /* Check number of hard problems */
    for( top = e->vis_undef_inter ; top ; top = top->next )
        if( top->visibility == VISIBILITY_UNDEF )
            total_undefs++ ;

    if( total_undefs == 0 )
        {
            /* No process needs to be created. Call the continuation
               immediately */
            void (*_k)(Element *, long) = (void (*)(Element *, long))k;
            (*_k)( e, process_id ) ;
            return ;
        }

    /* Check existing parallelism */
    if(   (total_undefs < N_visibility_per_task)
       || taskq_too_long(&global->task_queue[ taskqueue_id[process_id] ]) )
        {
            /* Task size is small, or the queue is too long.
               Solve it immediately. */
            visibility_task( e, e->vis_undef_inter,
                            e->n_vis_undef_inter, k, process_id ) ;

            return ;
        }

    /* Create multiple tasks. Hard problems (i.e. where visibility comp is
       really necessary) are divided into 'n_tasks' groups by residue
       number division (or Bresenham's DDA) */
    /* Note: once the first task is enqueued, the vis-undef list may be
       modified while other tasks are being created. So, any information
       that is necessary in the for-loop must be read from the element
       and saved locally */

    n_tasks = (total_undefs + N_visibility_per_task - 1)
        / N_visibility_per_task ;
    remainder = 0 ;
    i_cnt = 0 ;
    for( top = e->vis_undef_inter, tail = top ; tail ; tail = tail->next )
        {
            i_cnt++ ;

            if( tail->visibility != VISIBILITY_UNDEF )
                continue ;

            remainder += n_tasks ;

            if( remainder >= total_undefs )
                {
                    /* Create a task */

                    /* For the last task, append following (easy) interactions
                       if there is any */
                    tasks_created++ ;
                    if( tasks_created >= n_tasks )
                        for( ; tail->next ; tail = tail->next, i_cnt++ ) ;

                    /* Set task descriptor */
                    t = get_task(process_id) ;
                    t->task_type = TASK_VISIBILITY ;
                    t->task.vis.e       = e ;
                    t->task.vis.inter   = top ;
                    t->task.vis.n_inter = i_cnt ;
                    t->task.vis.k       = k ;

                    /* Enqueue */
                    enqueue_task( taskqueue_id[process_id], t, TASK_INSERT ) ;

                    /* Update pointer and the residue variable */
                    top = tail->next ;
                    remainder -= total_undefs ;
                    i_cnt = 0 ;
                }
        }
}




void create_radavg_task(Element *e, long mode, long process_id)
{
    /* Check existing parallelism */
    if(   (e->n_interactions < N_inter_parallel_bf_refine)
       || taskq_too_long(&global->task_queue[ taskqueue_id[process_id] ]) )
        {
            /* Task size is too small or queue is too long.
               Solve it immediately */
            radiosity_averaging( e, mode, process_id ) ;
            return ;
        }

    /* Put in the queue */
    enqueue_radavg_task( taskqueue_id[process_id], e, mode, process_id ) ;
}


void enqueue_radavg_task(long qid, Element *e, long mode, long process_id)
{
    Task *t ;

    /* Create task object */
    t = get_task(process_id) ;
    t->task_type = TASK_RAD_AVERAGE ;
    t->task.rad.e     = e ;
    t->task.rad.mode  = mode ;

    /* Put in the queue */
    enqueue_task( qid, t, TASK_INSERT ) ;
}



/***************************************************************************
 *
 *    enqueue_task()
 *    dequeue_task()
 *
 ****************************************************************************/

void enqueue_task(long qid, Task *task, long  mode)
{
    Task_Queue *tq ;


    tq = &global->task_queue[ qid ] ;

    /* Lock the task queue */
    LOCK(tq->q_lock);

    if( tq->tail == 0 )
        {
            /* The first task in the queue */
            tq->tail    = task ;
            tq->top     = task ;
            tq->n_tasks = 1 ;
        }
    else
        {
            /* Usual case */
            if( mode == TASK_APPEND )
                {
                    tq->tail->next = task ;
                    tq->tail = task ;
                    tq->n_tasks++ ;
                }
            else
                {
                    task->next = tq->top ;
                    tq->top = task ;
                    tq->n_tasks++ ;
                }
        }

    /* Unlock the task queue */
    UNLOCK(tq->q_lock);
}




Task *dequeue_task(long qid, long max_visit, long process_id)
  /*
   *    Attempts to dequeue first from the specified queue (qid), but if no
   *	 task is found the routine searches max_visit other queues and returns
   *    a task. If a task is taken from another queue, the task is taken from
   *    the tail of the queue (usually, larger amount of work is involved than
   *    the task at the top of the queue and more locality can be exploited
   *  	 within the stolen task).
   */
{
    Task_Queue *tq ;
    Task *t = 0 ;
    Task *prev ;
    long visit_count = 0 ;
    long sign = -1 ;		      /* The first retry will go backward */
    long offset ;

    /* Check number of queues to be visited */
    if( max_visit > n_taskqueues )
        max_visit = n_taskqueues ;

    /* Get next task */
    while( visit_count < max_visit )
        {
            /* Select a task queue */
            tq = &global->task_queue[ qid ] ;

            /* Check the length (test-test&set) */
            if( tq->n_tasks > 0 )
                {
                    /* Lock the task queue */
                    LOCK(tq->q_lock);
                    if( tq->top )
                        {
                            if( qid == taskqueue_id[process_id] )
                                {
                                    t = tq->top ;
                                    tq->top = t->next ;
                                    if( tq->top == 0 )
                                        tq->tail = 0 ;
                                    tq->n_tasks-- ;
                                }
                            else
                                {
                                    /* Get tail */
                                    for( prev = 0, t = tq->top ; t->next ;
                                        prev = t, t = t->next ) ;

                                    if( prev == 0 )
                                        tq->top = 0 ;
                                    else
                                        prev->next = 0 ;
                                    tq->tail = prev ;
                                    tq->n_tasks-- ;
                                }
                        }
                    /* Unlock the task queue */
                    UNLOCK(tq->q_lock);
                    break ;
                }

            /* Update visit count */
            visit_count++ ;

            /* Compute next taskqueue ID */
            offset = (sign > 0)? visit_count : -visit_count ;
            sign = -sign ;

            qid += offset ;
            if( qid < 0 )
                qid += n_taskqueues ;
            else if( qid >= n_taskqueues )
                qid -= n_taskqueues ;
        }

    return( t ) ;
}


/***************************************************************************
 *
 *    get_task()    Create a new instance of Task
 *    free_task()   Free a Task object
 *
 ****************************************************************************/


Task *get_task(long process_id)
{
    Task *p ;
    Task_Queue *tq ;
    long i ;
    long q_id ;
    long retry_count = 0 ;

    /* First, check local task queue */
    if( task_struct[process_id].local_free_task == 0 )
        {
            /* If empty, allocate task objects from the shared list */
            q_id = taskqueue_id[process_id] ;

            while( task_struct[process_id].local_free_task == 0 )
                {
                    tq = &global->task_queue[ q_id ] ;

                    if( tq->n_free > 0 )
                        {
                            LOCK(tq->f_lock);
                            if( tq->free )
                                {
                                    /* Scan the free list */
                                    for( i = 1, p = tq->free ;
                                        (i < N_ALLOCATE_LOCAL_TASK) && p->next ;
                                        i++, p = p->next ) ;

                                    task_struct[process_id].local_free_task = tq->free ;
                                    task_struct[process_id].n_local_free_task = i ;
                                    tq->free = p->next ;
                                    tq->n_free -= i ;
                                    p->next = 0 ;
                                    UNLOCK(tq->f_lock);
                                    break ;
                                }
                            UNLOCK(tq->f_lock);
                        }

                    /* Try next task queue */
                    if( ++q_id >= n_taskqueues )
                        q_id = 0 ;

                    /* Check retry count */
                    if( ++retry_count > MAX_TASKGET_RETRY )
                        {
                            fprintf( stderr, "Panic(P%ld):No free task\n",
                                    process_id ) ;
                            fprintf( stderr, "  Local %ld\n", task_struct[process_id].n_local_free_task ) ;
                            fprintf( stderr, "  Q0 free %ld\n",
                                    global->task_queue[0].n_free ) ;
                            fprintf( stderr, "  Q0 task %ld\n",
                                    global->task_queue[0].n_tasks ) ;
                            exit(1) ;
                        }
                }
        }


    /* Delete from the queue */
    p = task_struct[process_id].local_free_task ;
    task_struct[process_id].local_free_task = p->next ;
    task_struct[process_id].n_local_free_task-- ;

    /* Clear pointer just in case.. */
    p->next = 0 ;


    return( p ) ;
}



void free_task(Task *task, long process_id)
{
    Task_Queue *tq ;
    Task *p, *top ;
    long i ;

    /* Insert to the local queue */
    task->next = task_struct[process_id].local_free_task ;
    task_struct[process_id].local_free_task = task ;
    task_struct[process_id].n_local_free_task++ ;

    /* If local list is too long, export some tasks */
    if( task_struct[process_id].n_local_free_task >= (N_ALLOCATE_LOCAL_TASK * 2) )
        {
            tq = &global->task_queue[ taskqueue_id[process_id] ] ;

            for( i = 1, p = task_struct[process_id].local_free_task ;
                i < N_ALLOCATE_LOCAL_TASK ;   i++, p = p->next ) ;

            /* Update local list */
            top = task_struct[process_id].local_free_task ;
            task_struct[process_id].local_free_task = p->next ;
            task_struct[process_id].n_local_free_task -= i ;

            /* Insert in the shared list */
            LOCK(tq->f_lock);
            p->next = tq->free ;
            tq->free = top ;
            tq->n_free += i ;
            UNLOCK(tq->f_lock);
        }
}



/***************************************************************************
 *
 *    init_taskq()
 *
 *    Initialize task free list and the task queue.
 *    This routine must be called when only one process is active.
 *
 ****************************************************************************/


void init_taskq(long process_id)
{
    long i ;
    long qid ;
    long task_index = 0 ;
    long task_per_queue ;
    long n_tasks ;

    /* Reset task assignment index */
    task_struct[process_id].crnt_taskq_id = 0 ;

    /* Initialize task queues */
    task_per_queue = (MAX_TASKS + n_taskqueues - 1) / n_taskqueues ;

    for( qid = 0 ; qid < n_taskqueues ; qid++ )
        {
            /* Initialize free list */
            if (task_index + task_per_queue > MAX_TASKS )
                n_tasks = MAX_TASKS - task_index ;
            else
                n_tasks = task_per_queue ;

            for( i = task_index ; i < task_index + n_tasks - 1 ; i++ )
                global->task_buf[i].next = &global->task_buf[i+1] ;
            global->task_buf[ i ].next = 0 ;

            global->task_queue[ qid ].free = &global->task_buf[ task_index ] ;
            global->task_queue[ qid ].n_free = n_tasks ;

            /* Initialize task queue */
            global->task_queue[ qid ].top     = 0 ;
            global->task_queue[ qid ].tail    = 0 ;
            global->task_queue[ qid ].n_tasks = 0 ;

            /* Initialize locks */
            LOCKINIT(global->task_queue[ qid ].q_lock);
            LOCKINIT(global->task_queue[ qid ].f_lock);

            /* Update index for next queue */
            task_index += n_tasks ;
        }

    /* Initialize local free lists */
    task_struct[process_id].n_local_free_task      = 0 ;
    task_struct[process_id].local_free_task        = 0 ;
}



/***************************************************************************
 *
 *    check_task_counter()
 *
 *    Check task counter and return TRUE if this is the first task.
 *
 ****************************************************************************/

long check_task_counter()
{
    long flag = 0 ;


    LOCK(global->task_counter_lock);

    if( global->task_counter == 0 )
        /* First processor */
        flag = 1 ;

    global->task_counter++ ;
    if( global->task_counter >= n_processors )
        global->task_counter = 0 ;

    UNLOCK(global->task_counter_lock);

    return( flag ) ;
}



/***************************************************************************
 *
 *    assign_taskq()
 *
 *    Assign process its task queue.
 *
 ****************************************************************************/

long assign_taskq(long process_id)
{
    long qid ;

    qid = task_struct[process_id].crnt_taskq_id++ ;

    if( task_struct[process_id].crnt_taskq_id >= n_taskqueues )
        task_struct[process_id].crnt_taskq_id = 0 ;

    return( qid ) ;
}


/***************************************************************************
 *
 *    print_task()
 *    print_taskq()
 *
 *    Print contents of a task.
 *
 ****************************************************************************/

void print_task(Task *task)
{
    if( task == 0 )
        {
            printf( "Task (NULL)\n" ) ;
            return ;
        }

    switch( task->task_type )
        {
        case TASK_MODELING:
            printf( "Task (Model)\n" ) ;
            break ;
        case TASK_BSP:
            printf( "Task (BSP)\n" ) ;
            break ;
        case TASK_FF_REFINEMENT:
            printf( "Task (FF Refinement)\n" ) ;
            break ;
        case TASK_RAY:
            printf( "Task (Ray)  (patch ID %ld)\n",
                   task->task.ray.e->patch->seq_no ) ;
            break ;
        case TASK_VISIBILITY:
            printf( "Task (Visibility)  (patch ID %ld)\n",
                   task->task.vis.e->patch->seq_no ) ;
            break ;
        case TASK_RAD_AVERAGE:
            printf( "Task (RadAvg)\n" ) ;
            break ;
        default:
            fprintf( stderr, "Task(Illegal task type %ld)\n", task->task_type );
        }
}


void print_taskq(Task_Queue *tq)
{
    Task *t ;

    printf( "TaskQ: %ld tasks in the queue\n", taskq_length(tq) ) ;
    for( t = taskq_top(tq) ; t ; t = t->next )
        {
            printf( "  " ) ;
            print_task( t ) ;
        }
}
