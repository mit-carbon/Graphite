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


/*  This file contains many constant definitions that control the execution
of the program, as well as lobal data structure declarations */

#ifndef _RADIOSITY_H
#define _RADIOSITY_H

#include <math.h>
include(parallel.h)
include(patch.h)
include(model.h)
include(task.h)
#include "glib.h"
#include "pslib.h"


/****************************************
*
*    Configuration Parameters
*
*****************************************/

/*************************************************************************
*
*    Task scheduling & Load balancing (1)
*       --- Assignment of the patches to the processors
*
*    This macro specifies how patches are assigned to the task queues (ie,
*    processors).
*    - PATCH_ASSIGNMENT_STATIC assigns the same set of patches to the same
*    queue repeatedly over iterations.
*    - PATCH_ASSIGNMENT_COSTBASED assigns patches to queues based on the
*    work associated with those patches in previous iterations, in order
*    to try to balance the initial workload assignment among processors
*    and hence reduce task stealing.
*
**************************************************************************/

#define PATCH_ASSIGNMENT_STATIC    (1)
#define PATCH_ASSIGNMENT_COSTBASED (3)

#if !defined(PATCH_ASSIGNMENT)
#define PATCH_ASSIGNMENT PATCH_ASSIGNMENT_STATIC
#endif


/****************************************
*
*    Constants
*
*****************************************/


#define F_ZERO  (1.0e-6)

#if defined(SIMULATOR)
#define MAX_PROCESSORS (128)	      /* Maximum number of processors
(i.e., processes) created */
#define MAX_TASKQUEUES (128)	      /* Maximum number of task queues */
#define MAX_TASKS    (32768)	      /* # of available task descriptors */
#define MAX_PATCHES  (1024)	      /* # of available patch objects */
#define MAX_ELEMENTS (80000)	      /* # of available element objects */
#define MAX_INTERACTIONS (640000)     /* # of available interaction objs */
#define MAX_ELEMVERTICES  (65536)     /* # of available ElemVertex objs */
#define MAX_EDGES  (65536)            /* # of available Edge objs */
#endif

#if defined(DASH)
#define MAX_PROCESSORS (64)	      /* Maximum number of processors
(i.e., processes) created */
#define MAX_TASKQUEUES (64)	      /* Maximum number of task queues */
#define MAX_TASKS    (32768)	      /* # of available task descriptors */
#define MAX_PATCHES  (1024)	      /* # of available patch objects */
#define MAX_ELEMENTS (80000)	      /* # of available element objects */
#define MAX_INTERACTIONS (640000)     /* # of available interaction objs */
#define MAX_ELEMVERTICES  (65536)     /* # of available ElemVertex objs */
#define MAX_EDGES  (65536)            /* # of available Edge objs */
#endif

#if defined(SGI_GL)
#define MAX_PROCESSORS (8)	      /* Maximum number of processors
(i.e., processes) created */
#define MAX_TASKQUEUES (8)	      /* Maximum number of task queues */
#define MAX_TASKS    (8192)	      /* # of available task descriptors */
#define MAX_PATCHES  (1024)	      /* # of available patch objects */
#define MAX_ELEMENTS (40000)	      /* # of available element objects */
#define MAX_INTERACTIONS (320000)     /* # of available interaction objs */
#define MAX_ELEMVERTICES  (16384)     /* # of available ElemVertex objs */
#define MAX_EDGES  (65536)            /* # of available Edge objs */
#endif

#if defined(SUN4)
#define MAX_PROCESSORS (1)	      /* Maximum number of processors
(i.e., processes) created */
#define MAX_TASKQUEUES (1)	      /* Maximum number of task queues */
#define MAX_TASKS    (1024)	      /* # of available task descriptors */
#define MAX_PATCHES  (1024)	      /* # of available patch objects */
#define MAX_ELEMENTS (20000)	      /* # of available element objects */
#define MAX_INTERACTIONS (160000)     /* # of available interaction objs */
#define MAX_ELEMVERTICES  (16384)     /* # of available ElemVertex objs */
#define MAX_EDGES  (32768)            /* # of available Edge objs */
#endif

#if (!defined(SIMULATOR) && !defined(DASH) && !defined(SGI_GL) && !defined(SUN4))
#define MAX_PROCESSORS (128)	      /* Maximum number of processors
(i.e., processes) created */
#define MAX_TASKQUEUES (128)	      /* Maximum number of task queues */
#define MAX_TASKS    (32768)	      /* # of available task descriptors */
#define MAX_PATCHES  (1024)	      /* # of available patch objects */
#define MAX_ELEMENTS (80000)	      /* # of available element objects */
#define MAX_INTERACTIONS (640000)     /* # of available interaction objs */
#define MAX_ELEMVERTICES  (65536)     /* # of available ElemVertex objs */
#define MAX_EDGES  (65536)            /* # of available Edge objs */
#endif

#define MAX_SHARED_LOCK (3900)	      /* Maximum locks allocated. Objects
share these locks */

#if defined(SGI_GL) || defined(DASH) || defined(SIMULATOR)
#define CLOCK_MAX_VAL (2048*1000000)  /* ANL macro clock max value */
#elif defined(SUN4)
#define CLOCK_MAX_VAL (65536*1000000)  /* ANL macro clock max value */
#else
#define CLOCK_MAX_VAL (2048*1000000)  /* ANL macro clock max value */
#endif



/****************************************
*
*    System defaults
*
*****************************************/

#define DEFAULT_N_PROCESSORS (1)
#define DEFAULT_N_TASKQUEUES (1)
#define DEFAULT_N_TASKS_PER_QUEUE (200)
/* Create new tasks if number of tasks currently
in the queue is less than this number */
#define DEFAULT_N_INTER_PARALLEL_BF_REFINEMENT (5)
/* If the number of interactions is greater than
or equal to this value, BF-refinement is
performed in parallel */
#define DEFAULT_N_VISIBILITY_PER_TASK (4)
/* Number of visibility computations per
visibility task */
#define DEFAULT_AREA_EPSILON (2000.0)
/* If element is smaller than this value,
no further subdivision takes place */
#define DEFAULT_ENERGY_EPSILON (0.005)
/* Terminate radiosity iteration if the
difference of total energy is less than this
value. */
#define DEFAULT_BFEPSILON (0.015)
/* BF refinement threshold level. If the estimated
error of BF (due to FF error and error due to
constant approximation within an element) is
larger than this value, then subdivide */

#define DFLT_VIEW_ROT_X (10.0)
#define DFLT_VIEW_ROT_Y (0.0)
#define DFLT_VIEW_DIST  (8000.0)
#define DFLT_VIEW_ZOOM  (1.0)


/****************************************
*
*    Display mode
*
*****************************************/

#define DISPLAY_FILLED   (0)
#define DISPLAY_SHADED   (1)
#define DISPLAY_EDGEONLY (2)

#define DISPLAY_ALL_INTERACTIONS  (0)
#define DISPLAY_HALF_INTERACTIONS (1)



/****************************************
*
*    Statistical Measure
*
*****************************************/

#define MAX_ITERATION_INFO (16)

struct _element ;

typedef struct
{
    long visibility_comp ;
    long ray_intersect_test ;
    long tasks_from_myq ;
    long tasks_from_otherq ;
    long process_tasks_wait ;
    struct _element *last_pr_task ;
} PerIterationInfo ;


typedef struct
{
    char pad1[PAGE_SIZE];	 	/* padding to avoid false-sharing
    and allow page-placement */
    long total_modeling_tasks ;
    long total_def_patch_tasks ;
    long total_ff_ref_tasks ;
    long total_ray_tasks ;
    long total_radavg_tasks ;
    long total_direct_radavg_tasks ;
    long total_interaction_comp ;
    long total_visibility_comp ;
    long partially_visible ;
    long total_ray_intersect_test ;
    long total_patch_cache_check ;
    long total_patch_cache_hit ;
    long patch_cache_hit[PATCH_CACHE_SIZE] ;
    PerIterationInfo per_iteration[ MAX_ITERATION_INFO ] ;
    char pad2[PAGE_SIZE];	 	/* padding to avoid false-sharing
    and allow page-placement */
} StatisticalInfo ;

/****************************************
*
*    Shared data structure definition.
*
*****************************************/

typedef struct
{
    long rad_start, rad_time, refine_time, wait_time, vertex_time;
} Timing;

typedef struct
{

    /* Task queue */
    /* ***** */ long index;
    /* ***** */	LOCKDEC(index_lock)
    Task_Queue task_queue[ MAX_TASKQUEUES ] ;
    Task task_buf[ MAX_TASKS ] ;

    /* BSP tree root */
    LOCKDEC(bsp_tree_lock)
    Patch *bsp_root ;

    /* Average radiosity value */
    LOCKDEC(avg_radiosity_lock)
    long   converged ;
    Rgb   prev_total_energy ;
    Rgb   total_energy ;
    float total_patch_area ;
    long   iteration_count ;

    /* Computation cost estimate */
    LOCKDEC(cost_sum_lock)
    long cost_sum ;
    long cost_estimate_sum ;
    Patch_Cost patch_cost[ MAX_PATCHES ] ;

    /* Barrier */
    BARDEC(barrier)

    /* Private varrier */
    long pbar_count ;
    LOCKDEC(pbar_lock)

    /* Task initializer counter */
    long task_counter ;
    LOCKDEC(task_counter_lock)

    /* Resource buffers */
    LOCKDEC(free_patch_lock)
    Patch *free_patch ;
    long   n_total_patches ;
    long   n_free_patches ;
    Patch patch_buf[ MAX_PATCHES ] ;

    LOCKDEC(free_element_lock)
    Element *free_element ;
    long     n_free_elements ;
    Element element_buf[ MAX_ELEMENTS ] ;

    LOCKDEC(free_interaction_lock)
    Interaction *free_interaction ;
    long         n_free_interactions ;
    Interaction interaction_buf[ MAX_INTERACTIONS ] ;

    LOCKDEC(free_elemvertex_lock)
    long         free_elemvertex ;
    ElemVertex  elemvertex_buf[ MAX_ELEMVERTICES ] ;

    LOCKDEC(free_edge_lock)
    long   free_edge ;
    Edge  edge_buf[ MAX_EDGES ] ;

    Shared_Lock sh_lock[ MAX_SHARED_LOCK ] ;

    StatisticalInfo stat_info[ MAX_PROCESSORS ] ;

} Global ;


/****************************************
*
*    Global variables
*
*****************************************/

extern Timing **timing ;
extern Global *global ;
extern long    n_processors ;
extern long    n_taskqueues ;
extern long    n_tasks_per_queue ;

extern long    N_inter_parallel_bf_refine ;
extern long    N_visibility_per_task ;
extern float  Area_epsilon ;
extern float  Energy_epsilon ;
extern float  BFepsilon ;

extern long batch_mode, verbose_mode ;
extern long taskqueue_id[] ;

extern long time_rad_start, time_rad_end, time_process_start[] ;


/****************************************
*
*    Global function names & types
*
*****************************************/

/*
 * display.C
 */
void radiosity_averaging(Element *elem, long mode, long process_id);
void setup_view(float rot_x, float rot_y, float dist, float zoom, long process_id);
void display_scene(long fill_sw, long patch_sw, long mesh_sw, long interaction_sw, long process_id);
void display_patch(Patch *patch, long mode, long process_id);
void display_patches_in_bsp_tree(long mode, long process_id);
void display_element(Element *element, long mode, long process_id);
void display_elements_in_patch(Patch *patch, long mode, long process_id);
void display_elements_in_bsp_tree(long mode, long process_id);
void display_interactions_in_element(Element *elem, long mode, long process_id);
void display_interactions_in_patch(Patch *patch, long mode, long process_id);
void display_interactions_in_bsp_tree(long process_id);
void ps_display_scene(long fill_sw, long patch_sw, long mesh_sw, long interaction_sw, long process_id);
void ps_display_patch(Patch *patch, long mode, long process_id);
void ps_display_patches_in_bsp_tree(long mode, long process_id);
void ps_display_element(Element *element, long mode, long process_id);
void ps_display_elements_in_patch(Patch *patch, long mode, long process_id);
void ps_display_elements_in_bsp_tree(long mode, long process_id);
void ps_display_interactions_in_element(Element *elem, long mode, long process_id);
void ps_display_interactions_in_patch(Patch *patch, long mode, long process_id);
void ps_display_interactions_in_bsp_tree(long process_id);

/*
 * elemman.C
 */
void foreach_element_in_patch(Patch *patch, void (*func)(), long arg1, long process_id);
void foreach_leaf_element_in_patch(Patch *patch, void (*func)(), long arg1, long process_id);
void ff_refine_elements(Element *e1, Element *e2, long level, long process_id);
long error_analysis(Element *e1, Element *e2, Interaction *inter12, Interaction *inter21, long process_id);
void bf_error_analysis_list(Element *elem, Interaction *i_list, long process_id);
long bf_error_analysis(Element *elem, Interaction *inter, long process_id);
long radiosity_converged(long process_id);
void subdivide_element(Element *e, long process_id);
void process_rays(Element *e, long process_id);
long element_completely_invisible(Element *e1, Element *e2, long process_id);
Element *get_element(long process_id);
long leaf_element(Element *elem, long process_id);
void init_elemlist(long process_id);
void print_element(Element *elem, long process_id);
void foreach_interaction_in_element(Element *elem, void (*func)(), long arg1, long process_id);
void compute_formfactor(Element *e_src, Element *e_dst, Interaction *inter, long process_id);
void compute_interaction(Element *e_src, Element *e_dst, Interaction *inter, long subdiv, long process_id);
void insert_interaction(Element *elem, Interaction *inter, long process_id);
void delete_interaction(Element *elem, Interaction *prev, Interaction *inter, long process_id);
void insert_vis_undef_interaction(Element *elem, Interaction *inter, long process_id);
void delete_vis_undef_interaction(Element *elem, Interaction *prev, Interaction *inter, long process_id);
Interaction *get_interaction(long process_id);
void free_interaction(Interaction *interaction, long process_id);
void init_interactionlist(long process_id);
void print_interaction(Interaction *inter, long process_id);

/*
 * rad_main.C
 */
void start_radiosity(long val);
void change_display(long val);
void change_view_x(long val);
void change_view_y(long val);
void change_view_zoom(long val);
void change_BFepsilon(long val);
void change_area_epsilon(long val);
void select_model(long val);
void utility_tools(long val);
void radiosity(void);
long init_ray_tasks(long process_id);
void init_radavg_tasks(long mode, long process_id);
void init_global(long process_id);
void print_usage(void);

/*
 * rad_tools.C
 */
void print_statistics(FILE *fd, long process_id);
void print_per_process_info(FILE *fd, long process);
void get_patch_stat(Patch *patch, long dummy, long process_id);
void get_elem_stat(Element *elem, long dummy, long process_id);
void count_interaction(Element *es, Element *e1, Element *e2, Element *e3, long *c3, long *c2, long *c1, long *c0, long process_id);
long search_intearction(Interaction *int_list, Interaction *inter, long process_id);
void print_running_time(long process_id);
void print_fork_time(long process_id);
void init_stat_info(long process_id);
void clear_radiosity(long process_id);
void clear_patch_radiosity(Patch *patch, long dummy, long process_id);

/*
 * smallobj.C
 */
float vector_length(Vertex *v);
float distance(Vertex *p1, Vertex *p2);
float normalize_vector(Vertex *v1, Vertex *v2);
float inner_product(Vertex *v1, Vertex *v2);
void cross_product(Vertex *vc, Vertex *v1, Vertex *v2);
float plane_normal(Vertex *vc, Vertex *p1, Vertex *p2, Vertex *p3);
void center_point(Vertex *p1, Vertex *p2, Vertex *p3, Vertex *pc);
void four_center_points(Vertex *p1, Vertex *p2, Vertex *p3, Vertex *pc, Vertex *pc1, Vertex *pc2, Vertex *pc3);
void print_point(Vertex *point);
void print_rgb(Rgb *rgb);
ElemVertex *create_elemvertex(Vertex *p, long process_id);
ElemVertex *get_elemvertex(long process_id);
void init_elemvertex(long process_id);
void foreach_leaf_edge(Edge *edge, long reverse, void (*func)(), long arg1, long arg2, long process_id);
Edge *create_edge(ElemVertex *v1, ElemVertex *v2, long process_id);
void subdivide_edge(Edge *e, float a_ratio, long process_id);
Edge *get_edge(long process_id);
void init_edge(long process_id);
void init_sharedlock(long process_id);
Shared_Lock *get_sharedlock(long segment, long process_id);

/*
 * visible.C
 */
void init_visibility_module(long process_id);
void get_test_rays(Vertex *p_src, Ray *v, long no, long process_id);
long v_intersect(Patch *patch, Vertex *p, Ray *ray, float t);
long traverse_bsp(Patch *src_node, Vertex *p, Ray *ray, float r_min, float r_max, long process_id);
long traverse_subtree(Patch *node, Vertex *p, Ray *ray, float r_min, float r_max, long process_id);
long intersection_type(Patch *patch, Vertex *p, Ray *ray, float *tval, float range_min, float range_max);
long test_intersection(Patch *patch, Vertex *p, Ray *ray, float tval, long process_id);
void update_patch_cache(Patch *patch, long process_id);
long check_patch_cache(Vertex *p, Ray *ray, float r_min, float r_max, long process_id);
void init_patch_cache(long process_id);
long patch_tested(Patch *p, long process_id);
float visibility(Element *e1, Element *e2, long n_rays, long process_id);
void compute_visibility_values(Element *elem, Interaction *inter, long n_inter, long process_id);
void visibility_task(Element *elem, Interaction *inter, long n_inter, void (*k)(), long process_id);

#endif
