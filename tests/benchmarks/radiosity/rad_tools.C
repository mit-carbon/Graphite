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

/**************************************************************
 *
 *	Utility package
 *
 ***************************************************************/

#include <stdio.h>

EXTERN_ENV;

include(radiosity.h)

static void clear_element_radiosity(Element *elem, long dummy, long process_id);

/***************************************
 *
 *    Global variables
 *
 ****************************************/

#define MAX_INTERACTION_PER_ELEMENT (100)

long total_patches ;
long total_elements ;
long total_equiv_elements ;
long total_interactions ;
long total_comp_visible_interactions ;
long total_invisible_interactions ;
long total_match3, total_match2, total_match1, total_match0 ;


typedef struct
{
    long count ;
    float area ;
} Elem_Interaction ;

Elem_Interaction elem_interaction[MAX_INTERACTION_PER_ELEMENT+1] ;
Elem_Interaction many_interaction ;


/***************************************
 *
 *    Prinit statistics
 *
 ****************************************/

void print_statistics(FILE *fd, long process_id)
{
    long i ;

    /* Initialize information */
    total_patches = 0 ;
    total_elements = 0 ;
    total_equiv_elements = 0 ;
    total_interactions = 0 ;
    total_comp_visible_interactions = 0 ;
    total_invisible_interactions = 0 ;
    total_match3 = 0 ;
    total_match2 = 0 ;
    total_match1 = 0 ;
    total_match0 = 0 ;

    for( i = 0 ; i < MAX_INTERACTION_PER_ELEMENT ; i++ )
        {
            elem_interaction[i].count = 0 ;
            elem_interaction[i].area = 0 ;
        }
    many_interaction.count = 0 ;
    many_interaction.area  = 0 ;

    foreach_patch_in_bsp( (void (*)())get_patch_stat,  0, 0 ) ;

    fprintf( fd, "Radiosity Statistics\n\n" ) ;

    fprintf( fd, "    Histogram of interactions/elem\n" ) ;
    fprintf( fd, "\t Interactions  Occurrence\n" ) ;
    fprintf( fd, "\t -------------------------------\n" ) ;
    if( many_interaction.count > 0 )
        {
            fprintf( fd, "\t (Over %d)      %ld (%f)\n",
                    MAX_INTERACTION_PER_ELEMENT,
                    many_interaction.count,
                    many_interaction.area / many_interaction.count ) ;
        }
    for( i = MAX_INTERACTION_PER_ELEMENT ;
        elem_interaction[i].count == 0 ; i-- ) ;
    for( ; i >= 0 ; i-- )
        {
            if( elem_interaction[i].count == 0 )
                continue ;

            if( elem_interaction[i].count == 0 )
                fprintf( fd, "\t    %ld          %ld (---)\n",
                        i, elem_interaction[i].count ) ;

            else
                fprintf( fd, "\t    %ld          %ld (%f)\n",
                        i, elem_interaction[i].count,
                        elem_interaction[i].area / elem_interaction[i].count);
        }

    fprintf( fd, "    Configurations\n" ) ;
#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_COSTBASED
    fprintf( fd, "\tPatch assignment: Costbased\n" ) ;
    fprintf( fd, "\tUsing non-greedy cost-based algorithm\n") ;
#endif
#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_STATIC
    fprintf( fd, "\tPatch assignment: Static equal number\n" ) ;
#endif

    fprintf( fd, "\tAlways inserting at top of list for visibility testing (not sorted)\n" ) ;
    fprintf( fd, "\tRecursive pruning enabled for BSP tree traversal\n" ) ;
    fprintf( fd, "\tPatch cache:      Enabled\n" ) ;
    fprintf( fd, "\tAlways check all other queues when task stealing (not neighbor scheme)\n" ) ;


    fprintf( fd, "    Parameters\n" ) ;
    fprintf( fd, "\tNumber of processors:    %ld\n", n_processors ) ;
    fprintf( fd, "\tNumber of task queues:   %ld\n", n_taskqueues ) ;
    fprintf( fd, "\tNumber of tasks / queue: %ld\n", n_tasks_per_queue ) ;
    fprintf( fd, "\tArea epsilon:            %f\n", Area_epsilon ) ;
    fprintf( fd, "\t#inter parallel refine:  %ld\n",
            N_inter_parallel_bf_refine);
    fprintf( fd, "\t#visibility comp / task: %ld\n", N_visibility_per_task ) ;
    fprintf( fd, "\tBF epsilon:              %f\n", BFepsilon ) ;
    fprintf( fd, "\tEnergy convergence:      %f\n", Energy_epsilon ) ;

    fprintf( fd, "    Iterations to converge:   %ld times\n",
            global->iteration_count ) ;

    fprintf( fd, "    Resource Usage\n" ) ;
    fprintf( fd, "\tNumber of patches:            %ld\n", total_patches ) ;
    fprintf( fd, "\tTotal number of elements:     %ld\n", total_elements ) ;
    fprintf( fd, "\tTotal number of interactions: %ld\n", total_interactions);
    fprintf( fd, "\t          completely visible: %ld\n",
            total_comp_visible_interactions ) ;
    fprintf( fd, "\t        completely invisible: %ld\n",
            total_invisible_interactions ) ;
    fprintf( fd, "\t           partially visible: %ld\n",
            total_interactions - total_comp_visible_interactions
            - total_invisible_interactions ) ;
    fprintf( fd, "\tInteraction coherence (root interaction not counted)\n");
    fprintf( fd, "\t       Common for 4 siblings: %ld\n", total_match3 ) ;
    fprintf( fd, "\t       Common for 3 siblings: %ld\n", total_match2 ) ;
    fprintf( fd, "\t       Common for 2 siblings: %ld\n", total_match1 ) ;
    fprintf( fd, "\t       Common for no sibling: %ld\n", total_match0 ) ;
    fprintf( fd, "\tAvg. elements per patch:      %.1f\n",
            (float)total_elements / (float)total_patches ) ;
    fprintf( fd, "\tAvg. interactions per patch:  %.1f\n",
            (float)total_interactions / (float)total_patches ) ;
    fprintf( fd, "\tAvg. interactions per element:%.1f\n",
            (float)total_interactions / (float)total_elements ) ;
    fprintf( fd, "\tNumber of elements in equivalent uniform mesh: %ld\n",
            total_equiv_elements ) ;
    fprintf( fd, "\tElem(hierarchical)/Elem(uniform): %.2f%%\n",
            (float)total_elements / (float)total_equiv_elements * 100.0 ) ;
}




/**********************************************************
 *
 *    print_per_process_info()
 *
 ***********************************************************/

void print_per_process_info(FILE *fd, long process)
{
    long cache_line ;
    long iteration ;
    StatisticalInfo *ps ;
    Element *e ;

    ps = &global->stat_info[process] ;

    fprintf( fd, "\t\tModeling tasks:            %ld\n",
    ps->total_modeling_tasks ) ;
    fprintf( fd, "\t\tDefine patch tasks:        %ld\n",
    ps->total_def_patch_tasks ) ;
    fprintf( fd, "\t\tFF refinement tasks:       %ld\n",
    ps->total_ff_ref_tasks ) ;
    fprintf( fd, "\t\tRay processing tasks:      %ld\n",
    ps->total_ray_tasks ) ;
    fprintf( fd, "\t\tRadiosity Avg/Norm tasks:  %ld\n",
    ps->total_radavg_tasks ) ;
    fprintf( fd, "\t\tInteraction computations:  %ld\n",
    ps->total_interaction_comp ) ;
    fprintf( fd, "\t\tVisibility computations:   %ld\n",
    ps->total_visibility_comp ) ;
    fprintf( fd, "\t\t   (%ld of %ld were partially visible)\n",
    ps->partially_visible,
    ps->total_visibility_comp ) ;
    fprintf( fd, "\t\tRay intersection tests:    %ld\n",
    ps->total_ray_intersect_test ) ;
    fprintf( fd, "\t\tPatch cache hit ratio:     %.2f%%\n",
    ps->total_patch_cache_hit * 100 /
    (ps->total_patch_cache_check + 0.01) ) ;
    for( cache_line = 0 ; cache_line < PATCH_CACHE_SIZE ; cache_line++ )
    fprintf( fd, "\t\t    (level %ld):             %.2f%%\n",
    cache_line,
    ps->patch_cache_hit[cache_line] * 100 /
    (ps->total_patch_cache_check + 0.01));

    /* Per iteration info */
    fprintf( fd, "\t\tPer iteration info.\n" ) ;
    for( iteration = 0 ; iteration < global->iteration_count ; iteration++ )
    {
        fprintf( fd, "\t\t     [%ld]  Interaction comp:   %ld\n",
        iteration, ps->per_iteration[iteration].visibility_comp ) ;
        fprintf( fd, "\t\t          Ray Intersection:   %ld\n",
        ps->per_iteration[iteration].ray_intersect_test ) ;
        fprintf( fd, "\t\t          Tasks from my Q:    %ld\n",
        ps->per_iteration[iteration].tasks_from_myq ) ;
        fprintf( fd, "\t\t          Tasks from other Q: %ld\n",
        ps->per_iteration[iteration].tasks_from_otherq ) ;
        fprintf( fd, "\t\t     Process_task wait count: %ld\n",
        ps->per_iteration[iteration].process_tasks_wait ) ;
        e = ps->per_iteration[iteration].last_pr_task ;
        if( e == 0 )
        continue ;
        if( e->parent == 0 )
        {
            fprintf( fd, "\t\t          Last task: Patch level\n" ) ;
            fprintf( fd, "\t\t           (%ld root inter)\n",
            e->n_interactions ) ;
        }
        else
        {
            fprintf( fd, "\t\t          Last task: Elem level\n" ) ;
            fprintf( fd, "\t\t           (%ld inter, %.3f Elem/Patch)\n",
            e->n_interactions, e->area / e->patch->area ) ;
        }
    }
}



/**********************************************************
*
*    get_patch_stat()
*
***********************************************************/

long   n_elements_in_patch ;
long   n_equiv_elem_in_patch ;
float min_elem_area ;
long   n_interactions_in_patch ;
long   n_comp_visible_interactions ;
long   n_invisible_interactions ;


void get_patch_stat(Patch *patch, long dummy, long process_id)
{
    /* Initialize stat info for element */
    n_elements_in_patch = 0 ;
    n_equiv_elem_in_patch = 1 ;
    min_elem_area = patch->area ;
    n_interactions_in_patch = 0 ;
    n_comp_visible_interactions = 0 ;
    n_invisible_interactions = 0 ;

    /* Traverse the quad tree */
    foreach_element_in_patch( patch, (void (*)())get_elem_stat, 0, process_id ) ;

    /* Update global stat variables */
    total_patches++ ;
    total_elements += n_elements_in_patch ;
    total_equiv_elements += n_equiv_elem_in_patch ;
    total_interactions += n_interactions_in_patch ;
    total_comp_visible_interactions += n_comp_visible_interactions ;
    total_invisible_interactions += n_invisible_interactions ;

#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_COSTBASED
    if( n_interactions_in_patch
    != global->patch_cost[patch->seq_no].n_total_inter )
    {
        printf( "Error: patch(%d) Inter counted: %d (n_total_inter %d)\n",
        patch->seq_no,
        n_interactions_in_patch,
        global->patch_cost[patch->seq_no].n_total_inter ) ;
    }
#endif
}

void get_elem_stat(Element *elem, long dummy, long process_id)
{
    Interaction *pi ;
    long p_visible = 0 ;
    long c_visible = 0 ;
    long i_visible = 0 ;
    long match0, match1, match2, match3 ;

    n_elements_in_patch++ ;

    while( elem->area < min_elem_area )
    {
        min_elem_area *= 0.25 ;
        n_equiv_elem_in_patch *= 4 ;
    }

    /* Classify visibility */
    n_interactions_in_patch += elem->n_interactions ;
    for( pi = elem->interactions ; pi ; pi = pi->next )
    {
        if( pi->visibility == 0.0 )
        i_visible++ ;
        else if( pi->visibility == 1.0 )
        c_visible++ ;
        else
        p_visible++ ;
    }
    if( i_visible + c_visible + p_visible != elem->n_interactions )
    printf( "Fatal: Interactions count miss match\n" ) ;
    if( elem->n_vis_undef_inter != 0 )
    printf( "Fatal: Visibility undef list count non zero(%ld)\n",
    elem->n_vis_undef_inter ) ;
    if( elem->vis_undef_inter != 0 )
    printf( "Fatal: Visibility undef list not empty\n" ) ;

    n_comp_visible_interactions += c_visible ;
    n_invisible_interactions    += i_visible ;


    /* Count interactions / element */
    if( elem->n_interactions > MAX_INTERACTION_PER_ELEMENT )
    {
        many_interaction.count++ ;
        many_interaction.area += elem->area ;
    }
    else
    {
        elem_interaction[ elem->n_interactions ].count++ ;
        elem_interaction[ elem->n_interactions ].area += elem->area ;
    }

    /* Analyze object coherence */
    if( ! LEAF_ELEMENT( elem ) )
    {
        match0 = match1 = match2 = match3 = 0 ;

        count_interaction(elem->center, elem->top, elem->right, elem->left,
        &match3, &match2, &match1, &match0, process_id ) ;
        count_interaction(elem->top, elem->right, elem->left, elem->center,
        &match3, &match2, &match1, &match0, process_id ) ;
        count_interaction(elem->right, elem->left, elem->center, elem->top,
        &match3, &match2, &match1, &match0, process_id ) ;
        count_interaction(elem->left, elem->center, elem->top, elem->right,
        &match3, &match2, &match1, &match0, process_id ) ;

        total_match3 += match3 ;
        total_match2 += match2 ;
        total_match1 += match1 ;
        total_match0 += match0 ;
    }
}



void count_interaction(Element *es, Element *e1, Element *e2, Element *e3, long *c3, long *c2, long *c1, long *c0, long process_id)
{
    Interaction *pi ;
    long occurrence ;

    for( pi = es->interactions ; pi ; pi = pi->next )
    {
        occurrence  = search_intearction( e1->interactions, pi, process_id ) ;
        occurrence += search_intearction( e2->interactions, pi, process_id ) ;
        occurrence += search_intearction( e3->interactions, pi, process_id ) ;
        switch( occurrence )
        {
            case 0:  (*c0)++ ; break ;
            case 1:  (*c1)++ ; break ;
            case 2:  (*c2)++ ; break ;
            case 3:  (*c3)++ ; break ;
        }
    }
}

long search_intearction(Interaction *int_list, Interaction *inter, long process_id)
{
    while( int_list )
    {
        if( int_list->destination == inter->destination )
        return( 1 ) ;

        int_list = int_list->next ;
    }

    return( 0 ) ;
}

/***************************************
*
*    Prinit running time
*
****************************************/

void print_running_time(long process_id)
{
    long time_diff, time_diff1 ;

    time_diff = time_rad_end - time_rad_start ;
    time_diff1 = time_rad_end - timing[0]->rad_start;
    if( time_diff < 0 )
    time_diff += CLOCK_MAX_VAL ;
    if( time_diff1 < 0 )
    time_diff1 += CLOCK_MAX_VAL ;

    printf( "\tOverall start time\t%20lu\n", time_rad_start);
    printf( "\tOverall end time\t%20lu\n", time_rad_end);
    printf( "\tTotal time with initialization\t%20lu\n", time_diff);
    printf( "\tTotal time without initialization\t%20lu\n", time_diff1);
}


/***************************************
*
*    Print process creation overhead
*
****************************************/

void print_fork_time(long process_id)
{
    long pid ;

    if( n_processors <= 1 )
    return ;

    printf( "\tProcess fork overhead\n" ) ;
    for( pid = 0 ; pid < n_processors-1 ; pid++ )
    {
        printf( "\t Process %ld  %.2f mS\n",
        pid,
        (timing[pid]->rad_start - time_rad_start) / 1000.0 ) ;
    }

    printf( "\t (total)    %.2f mS\n",
    (time_process_start[n_processors-2] - time_rad_start) / 1000.0 ) ;
}


/***************************************
*
*    Initialize statistical info
*
****************************************/

void init_stat_info(long process_id)
{
    long pid ;
    long i ;
    StatisticalInfo *ps ;

    for( pid = 0 ; pid < MAX_PROCESSORS ; pid++ )
    {
        ps = &global->stat_info[ pid ] ;
        ps->total_modeling_tasks    = 0 ;
        ps->total_def_patch_tasks   = 0 ;
        ps->total_ff_ref_tasks      = 0 ;
        ps->total_ray_tasks         = 0 ;
        ps->total_radavg_tasks      = 0 ;
        ps->total_interaction_comp  = 0 ;
        ps->total_visibility_comp   = 0 ;
        ps->partially_visible       = 0 ;
        ps->total_ray_intersect_test= 0 ;
        ps->total_patch_cache_check = 0 ;
        ps->total_patch_cache_hit   = 0 ;

        for( i = 0 ; i < PATCH_CACHE_SIZE ; i++ )
        ps->patch_cache_hit[i]   = 0 ;

        for( i = 0 ; i < MAX_ITERATION_INFO ; i++ )
        {
            ps->per_iteration[ i ].visibility_comp    = 0 ;
            ps->per_iteration[ i ].ray_intersect_test = 0 ;
            ps->per_iteration[ i ].tasks_from_myq     = 0 ;
            ps->per_iteration[ i ].tasks_from_otherq  = 0 ;
            ps->per_iteration[ i ].process_tasks_wait = 0 ;
            ps->per_iteration[ i ].last_pr_task       = 0 ;
        }
    }
}


/**********************************************************
*
*    clear_radiosity()
*
***********************************************************/


void clear_radiosity(long process_id)
{
    foreach_patch_in_bsp( (void (*)())clear_patch_radiosity,  0, 0 ) ;
}


void clear_patch_radiosity(Patch *patch, long dummy, long process_id)
{
    foreach_element_in_patch( patch, (void (*)())clear_element_radiosity, 0, process_id ) ;
}


static void clear_element_radiosity(Element *elem, long dummy, long process_id)
{
    elem->rad.r = 0 ;
    elem->rad.g = 0 ;
    elem->rad.b = 0 ;

    elem->rad_subtree.r = 0 ;
    elem->rad_subtree.g = 0 ;
    elem->rad_subtree.b = 0 ;

    global->prev_total_energy = global->total_energy ;
    global->total_energy.r = 0 ;
    global->total_energy.g = 0 ;
    global->total_energy.b = 0 ;
}
