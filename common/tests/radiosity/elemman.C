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
 *	Element management.
 *
 *       This module has the following functionalities.
 *       (1) Creation/initialization of a new instance of an element object.
 *       (2) Recursive refinement of elements.
 */


#include <stdio.h>

EXTERN_ENV;

include(radiosity.h)

static void _foreach_element(Element *elem, void (*func)(), long arg1, long process_id);
static void _foreach_leaf_element(Element *elem, void (*func)(), long arg1, long process_id);
static long bf_refine_element(long subdiv, Element *elem, Interaction *inter, long process_id);
static void process_rays2(Element *e, long process_id);
static void process_rays3(Element *e, long process_id);
static void elem_join_operation(Element *e, Element *ec, long process_id);
static void gather_rays(Element *elem, long process_id);
static float _diff_disc_formfactor(Vertex *p, Element *e_src, Vertex *p_disc, float area, Vertex *normal, long process_id);
static float compute_diff_disc_formfactor(Vertex *p, Element *e_src, Vertex *p_disc, Element *e_dst, long process_id);

/***************************************************************************
 ****************************************************************************
 *
 *    Methods for Element object
 *
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 *
 *    foreach_element_in_patch()
 *    foreach_leaf_element_in_patch()
 *
 *    General purpose driver. For each element of a patch, apply specified
 *    function.  The function is passed a pointer to the element.
 *    Traversal is DFS (post-order).
 *
 ****************************************************************************/

void foreach_element_in_patch(Patch *patch, void (*func)(), long  arg1, long process_id)
{
    _foreach_element( patch->el_root, func, arg1, process_id ) ;
}


static void _foreach_element(Element *elem, void (*func)(), long   arg1, long process_id)
{
    if( elem == 0 )
        return ;

    /* Process children */
    if( ! LEAF_ELEMENT( elem ) )
        {
            _foreach_element( elem->center, func, arg1, process_id ) ;
            _foreach_element( elem->top,    func, arg1, process_id ) ;
            _foreach_element( elem->left,   func, arg1, process_id ) ;
            _foreach_element( elem->right,  func, arg1, process_id ) ;
        }

    void (*_func)(Element *, long, long) = (void (*)(Element *, long, long))func;

    /* Apply function to this node */
    _func( elem, arg1, process_id ) ;
}


void foreach_leaf_element_in_patch(Patch *patch, void (*func)(), long  arg1, long process_id)
{
    _foreach_leaf_element( patch->el_root, func, arg1, process_id ) ;
}


static void _foreach_leaf_element(Element *elem, void (*func)(), long arg1, long process_id )
{
    if( elem == 0 )
        return ;

    /* Process children */
    if( LEAF_ELEMENT( elem ) )
    {
       void (*_func)(Element *, long, long) = (void (*)(Element *, long, long))func;
        _func( elem, arg1, process_id ) ;
    }
    else
        {
            _foreach_leaf_element( elem->center, func, arg1, process_id ) ;
            _foreach_leaf_element( elem->top,    func, arg1, process_id ) ;
            _foreach_leaf_element( elem->left,   func, arg1, process_id ) ;
            _foreach_leaf_element( elem->right,  func, arg1, process_id ) ;
        }
}



/***************************************************************************
 *
 *    ff_refine_elements()
 *
 *    Recursively refine the elements based on FF estimate.
 *
 ****************************************************************************/

void ff_refine_elements(Element *e1, Element *e2, long level, long process_id)
{
    long subdiv_advice ;
    Interaction i12, i21 ;
    Interaction *inter ;

#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_COSTBASED
#if defined(SUN4)
    Patch_Cost *pc1, *pc2 ;
#else
    volatile Patch_Cost *pc1, *pc2 ;
#endif
    long cost1, cost2 ;
#endif


    /* Now compute formfactor.
       As the BSP tree is being modified at this moment, don't test
       visibility. */
    compute_formfactor( e1, e2, &i12, process_id ) ;
    compute_formfactor( e2, e1, &i21, process_id ) ;

    /* Analyze the error of FF */
    subdiv_advice = error_analysis( e1, e2, &i12, &i21, process_id ) ;

    /* Execute subdivision procedure */
    if( NO_INTERACTION(subdiv_advice) )
        /* Two elements are mutually invisible. Do nothing */
        return ;

    else if( NO_REFINEMENT_NECESSARY(subdiv_advice) )
        {
            /* Create links and finish the job */
            inter = get_interaction(process_id) ;
            *inter = i12 ;
            inter->visibility = VISIBILITY_UNDEF ;
            insert_vis_undef_interaction( e1, inter, process_id ) ;

            inter = get_interaction(process_id) ;
            *inter = i21 ;
            inter->visibility = VISIBILITY_UNDEF ;
            insert_vis_undef_interaction( e2, inter, process_id ) ;

#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_COSTBASED
            /* Update cost variable */
            pc1 = &global->patch_cost[ e1->patch->seq_no ] ;
            pc2 = &global->patch_cost[ e2->patch->seq_no ] ;
            if( pc1->n_total_inter <= 13 )
                cost1 = (long)ceil(e1->area / Area_epsilon) ;
            else
                cost1 = 1 ;

            if( pc2->n_total_inter <= 13 )
                cost2 = (long)ceil(e2->area / Area_epsilon) ;
            else
                cost2 = 1 ;

            LOCK(global->cost_sum_lock);
            pc1->cost_estimate += cost1 ;
            pc1->n_total_inter++ ;
            pc2->cost_estimate += cost2 ;
            pc2->n_total_inter++ ;
            global->cost_estimate_sum += (cost1 + cost2) ;
            global->cost_sum += (cost1 + cost2) ;
            UNLOCK(global->cost_sum_lock);
#endif
        }

    else if( REFINE_PATCH_1(subdiv_advice) )
        {
            /* Refine patch 1 */
            subdivide_element( e1, process_id ) ;

            /* Locally solve it */
            ff_refine_elements( e1->top,    e2, level+1, process_id ) ;
            ff_refine_elements( e1->center, e2, level+1, process_id ) ;
            ff_refine_elements( e1->left,   e2, level+1, process_id ) ;
            ff_refine_elements( e1->right,  e2, level+1, process_id ) ;
        }
    else
        {
            /* Refine patch 2 */
            subdivide_element( e2, process_id ) ;

            /* Locally solve it */
            ff_refine_elements( e1, e2->top,    level+1, process_id ) ;
            ff_refine_elements( e1, e2->center, level+1, process_id ) ;
            ff_refine_elements( e1, e2->left,   level+1, process_id ) ;
            ff_refine_elements( e1, e2->right,  level+1, process_id ) ;
        }
}



/***************************************************************************
 *
 *    error_analysis()
 *
 *    Analyze FF error.
 *
 ****************************************************************************/

long error_analysis(Element *e1, Element *e2, Interaction *inter12, Interaction *inter21, long process_id)
{
    long cc ;

    /* Check visibility */
    cc = patch_intersection( &e1->patch->plane_equ,
                            &e2->ev1->p, &e2->ev2->p, &e2->ev3->p, process_id ) ;
    if( NEGATIVE_SIDE(cc) )
        /* If negative or on the plane, then do nothing */
        return( _NO_INTERACTION ) ;

    cc = patch_intersection( &e2->patch->plane_equ,
                            &e1->ev1->p, &e1->ev2->p, &e1->ev3->p, process_id ) ;
    if( NEGATIVE_SIDE(cc) )
        /* If negative or on the plane, then do nothing */
        return( _NO_INTERACTION ) ;

    return( _NO_REFINEMENT_NECESSARY ) ;
}





/***************************************************************************
 *
 *    bf_refine_element()
 *
 *    Recursively refine the elements based on BF estimate.
 *    Return value: number of interactions newly created.
 *
 ****************************************************************************/

static long  bf_refine_element(long subdiv, Element *elem, Interaction *inter, long process_id)
{
    Element *e_dst = inter->destination ;
    Interaction *pi ;
    float visibility_val ;
    long new_inter = 0 ;


    visibility_val = NO_VISIBILITY_NECESSARY(subdiv)?
        (float)1.0 : VISIBILITY_UNDEF ;

    if( REFINE_PATCH_1(subdiv) )
        {
            /* Refine this element */
            /* (1) Make sure it has children */
            subdivide_element( elem, process_id ) ;

            /* (2) For each of the patch, create an interaction */
            if( element_completely_invisible( elem->center, e_dst, process_id ) == 0 )
                {
                    pi = get_interaction(process_id) ;
                    compute_formfactor( elem->center, e_dst, pi, process_id ) ;
                    pi->visibility = visibility_val ;
                    insert_vis_undef_interaction( elem->center, pi, process_id ) ;
                    new_inter++ ;
                }
            if( element_completely_invisible( elem->top, e_dst, process_id ) == 0 )
                {
                    pi = get_interaction(process_id) ;
                    compute_formfactor( elem->top, e_dst, pi, process_id ) ;
                    pi->visibility = visibility_val ;
                    insert_vis_undef_interaction( elem->top, pi, process_id ) ;
                    new_inter++ ;
                }
            if( element_completely_invisible( elem->left, e_dst, process_id ) == 0 )
                {
                    pi = get_interaction(process_id) ;
                    compute_formfactor( elem->left, e_dst, pi, process_id ) ;
                    pi->visibility = visibility_val ;
                    insert_vis_undef_interaction( elem->left, pi, process_id ) ;
                    new_inter++ ;
                }
            if( element_completely_invisible( elem->right, e_dst, process_id ) == 0 )
                {
                    pi = get_interaction(process_id) ;
                    compute_formfactor( elem->right, e_dst, pi, process_id ) ;
                    pi->visibility = visibility_val ;
                    insert_vis_undef_interaction( elem->right, pi, process_id ) ;
                    new_inter++ ;
                }
        }
    else
        {
            /* Refine source element */
            /* (1) Make sure it has children */
            subdivide_element( e_dst, process_id ) ;

            /* (2) Insert four new interactions
               NOTE: Use *inter as a place holder to link 4 new interactions
               since *prev may be NULL */

            if( element_completely_invisible( elem, e_dst->center, process_id ) == 0 )
                {
                    pi = get_interaction(process_id) ;
                    compute_formfactor( elem, e_dst->center, pi, process_id ) ;
                    pi->visibility = visibility_val ;
                    insert_vis_undef_interaction( elem, pi, process_id ) ;
                    new_inter++ ;
                }
            if( element_completely_invisible( elem, e_dst->top, process_id ) == 0 )
                {
                    pi = get_interaction(process_id) ;
                    compute_formfactor( elem, e_dst->top, pi, process_id ) ;
                    pi->visibility = visibility_val ;
                    insert_vis_undef_interaction( elem, pi, process_id ) ;
                    new_inter++ ;
                }
            if( element_completely_invisible( elem, e_dst->left, process_id ) == 0 )
                {
                    pi = get_interaction(process_id) ;
                    compute_formfactor( elem, e_dst->left, pi, process_id ) ;
                    pi->visibility = visibility_val ;
                    insert_vis_undef_interaction( elem, pi, process_id ) ;
                    new_inter++ ;
                }
            if( element_completely_invisible( elem, e_dst->right, process_id ) == 0 )
                {
                    pi = get_interaction(process_id) ;
                    compute_formfactor( elem, e_dst->right, pi, process_id ) ;
                    pi->visibility = visibility_val ;
                    insert_vis_undef_interaction( elem, pi, process_id ) ;
                    new_inter++ ;
                }
        }

    return( new_inter ) ;
}




/***************************************************************************
 *
 *    bf_error_analysis_list()
 *
 *    For each interaction in the list, analyze error of BF value.
 *    If error is lower than the limit, the interaction is linked to the
 *    normal interaction list. Otherwise, BF-refinement is performed and
 *    the newly created interactions are linked to the vis-undef-list.
 *
 ****************************************************************************/

void bf_error_analysis_list(Element *elem, Interaction *i_list, long process_id)
{
    long subdiv_advice ;
    Interaction *prev = 0 ;
    Interaction *inter = i_list ;
    Interaction *refine_inter ;
    long i_len = 0 ;
    long delta_n_inter = 0 ;


    while( inter )
        {
            /* Analyze error */
            subdiv_advice = bf_error_analysis( elem, inter, process_id ) ;

            if( NO_REFINEMENT_NECESSARY(subdiv_advice) )
                {
                    /* Go on to the next interaction */
                    prev = inter ;
                    inter = inter->next ;
                    i_len++ ;
                }
            else
                {
                    /* Remove this interaction from the list */
                    refine_inter = inter ;
                    inter = inter->next ;
                    if( prev == 0 )
                        i_list = inter ;
                    else
                        prev->next = inter ;

                    /* Perform refine */
                    delta_n_inter += bf_refine_element( subdiv_advice,
                                                       elem, refine_inter, process_id ) ;

                    /* Delete this inter anyway */
                    free_interaction( refine_inter, process_id ) ;
                    delta_n_inter-- ;
                }
        }

    /* Link good interactions to elem->intearctions */
    if( i_len > 0 )
        {
            LOCK(elem->elem_lock->lock);
            prev->next = elem->interactions ;
            elem->interactions = i_list ;
            elem->n_interactions += i_len ;
            UNLOCK(elem->elem_lock->lock);
        }

#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_COSTBASED
    /* Update patch interaction count */
    if( delta_n_inter != 0 )
        {
            Patch_Cost *pc ;

            pc = &global->patch_cost[ elem->patch->seq_no ] ;
            LOCK(pc->cost_lock->lock);
            pc->n_total_inter += delta_n_inter ;
            UNLOCK(pc->cost_lock->lock);
        }
#endif
}



/***************************************************************************
 *
 *    bf_error_analysis()
 *
 *    Analyze error of BF value.
 *    Return value: subdivision advice code.
 *
 ****************************************************************************/

long bf_error_analysis(Element *elem, Interaction *inter, long process_id)
{
    float rad_avg ;		     /* Average radiosity */
    float total_error ;
    float visibility_error ;
    long   vis_code = 0 ;

    /* Compute amount of error associated with the BF.

       FFcomputed = (F + Ferr)(V + Verr) = FV + (Ferr V + F Verr + Ferr Verr)
       BFcomputed = BF + BFerr
       = B FV + B (Ferr V + F Verr + Ferr Verr)
       */

    if( (0.0 < inter->visibility) && (inter->visibility < 1.0) )
        visibility_error = 1.0 ;
    else
        visibility_error = FF_VISIBILITY_ERROR ;

    rad_avg =( inter->destination->rad.r
              + inter->destination->rad.g
              + inter->destination->rad.b ) * (float)(1.0 / 3.0) ;

    total_error = (inter->visibility * inter->formfactor_err
                   + visibility_error * inter->formfactor_out
                   + visibility_error * inter->formfactor_err) * rad_avg ;

    /* If BF is smaller than the threshold, then not subdivide */
    if( (total_error <= BFepsilon) && (elem->n_interactions <= 10) )
        return( _NO_REFINEMENT_NECESSARY ) ;
    else if( total_error <= BFepsilon * 0.5 )
        return( _NO_REFINEMENT_NECESSARY ) ;

    /* Subdivide light source or receiver whichever is larger */
    if( elem->area > inter->destination->area )
        {
            if( elem->area > Area_epsilon )
                /* Subdivide this element (receiver) */
                return( _REFINE_PATCH_1 | vis_code ) ;
        }
    else
        {
            if( inter->destination->area > Area_epsilon )

                /* Subdivide source element */
                return( _REFINE_PATCH_2 | vis_code ) ;
        }

    return( _NO_REFINEMENT_NECESSARY ) ;
}





/***************************************************************************
 *
 *    radiosity_converged()
 *
 *    Return TRUE(1) when the average radiosity value is converged.
 *
 ****************************************************************************/

long radiosity_converged(long process_id)
{
    float prev_total, current_total ;
    float difference ;
    Rgb rad ;

    /* Check radiosity value */
    prev_total = global->prev_total_energy.r + global->prev_total_energy.g
        + global->prev_total_energy.b ;
    current_total = global->total_energy.r + global->total_energy.g
        + global->total_energy.b ;

    /* Compute difference from the previous iteration */
    prev_total += 1.0e-4 ;
    difference = fabs( (current_total / prev_total) - (float)1.0 ) ;

    if( verbose_mode )
        {
            rad = global->total_energy ;
            rad.r /= global->total_patch_area ;
            rad.g /= global->total_patch_area ;
            rad.b /= global->total_patch_area ;
            printf( "Total energy:     " ) ;
            print_rgb( &global->total_energy ) ;
            printf( "Average radiosity:" ) ;
            print_rgb( &rad ) ;
            printf( "Difference %.2f%%\n", difference * 100.0 ) ;
        }

    if( difference <=  Energy_epsilon )
        return( 1 ) ;
    else
        return( 0 ) ;
}


/***************************************************************************
 *
 *    subdivide_element()
 *
 *    Create child elements. If they already exist, do nothing.
 *
 ****************************************************************************/

void subdivide_element(Element *e, long process_id)
{
    float quarter_area ;
    ElemVertex *ev_12, *ev_23, *ev_31 ;
    Edge *e_12_23, *e_23_31, *e_31_12 ;
    Element *enew, *ecenter ;
    long rev_12, rev_23, rev_31 ;

    /* Lock the element before checking the value */
    LOCK(e->elem_lock->lock);

    /* Check if the element already has children */
    if( ! _LEAF_ELEMENT(e) )
        {
            UNLOCK(e->elem_lock->lock);
            return ;
        }

    /* Subdivide edge structures */
    subdivide_edge( e->e12, (float)0.5, process_id ) ;
    subdivide_edge( e->e23, (float)0.5, process_id ) ;
    subdivide_edge( e->e31, (float)0.5, process_id ) ;
    ev_12 = e->e12->ea->pb ;
    ev_23 = e->e23->ea->pb ;
    ev_31 = e->e31->ea->pb ;

    /* Then create new edges */
    e_12_23 = create_edge( ev_12, ev_23, process_id ) ;
    e_23_31 = create_edge( ev_23, ev_31, process_id ) ;
    e_31_12 = create_edge( ev_31, ev_12, process_id ) ;

    /* Area parameters */
    quarter_area = e->area * (float)0.25 ;

    /* (1) Create the center patch */
    enew = get_element(process_id) ;
    ecenter = enew ;
    enew->parent= e ;
    enew->patch = e->patch ;
    enew->ev1   = ev_23 ;
    enew->ev2   = ev_31 ;
    enew->ev3   = ev_12 ;
    enew->e12   = e_23_31 ;
    enew->e23   = e_31_12 ;
    enew->e31   = e_12_23 ;
    enew->area  = quarter_area ;
    enew->rad   = e->rad ;

    /* (2) Create the top patch */
    rev_12 = EDGE_REVERSE( e->e12, e->ev1, e->ev2 ) ;
    rev_23 = EDGE_REVERSE( e->e23, e->ev2, e->ev3 ) ;
    rev_31 = EDGE_REVERSE( e->e31, e->ev3, e->ev1 ) ;

    enew = get_element(process_id) ;
    e->top = enew ;
    enew->parent= e ;
    enew->patch = e->patch ;
    enew->ev1   = e->ev1 ;
    enew->ev2   = ev_12 ;
    enew->ev3   = ev_31 ;
    enew->e12   = (!rev_12)? e->e12->ea : e->e12->eb ;
    enew->e23   = e_31_12 ;
    enew->e31   = (!rev_31)? e->e31->eb : e->e31->ea ;
    enew->area  = quarter_area ;
    enew->rad   = e->rad ;

    /* (3) Create the left patch */
    enew = get_element(process_id) ;
    e->left = enew ;
    enew->parent= e ;
    enew->patch = e->patch ;
    enew->ev1   = ev_12 ;
    enew->ev2   = e->ev2 ;
    enew->ev3   = ev_23 ;
    enew->e12   = (!rev_12)? e->e12->eb : e->e12->ea ;
    enew->e23   = (!rev_23)? e->e23->ea : e->e23->eb ;
    enew->e31   = e_12_23 ;
    enew->area  = quarter_area ;
    enew->rad   = e->rad ;

    /* (4) Create the right patch */
    enew = get_element(process_id) ;
    e->right = enew ;
    enew->parent= e ;
    enew->patch = e->patch ;
    enew->ev1   = ev_31 ;
    enew->ev2   = ev_23 ;
    enew->ev3   = e->ev3 ;
    enew->e12   = e_23_31 ;
    enew->e23   = (!rev_23)? e->e23->eb : e->e23->ea ;
    enew->e31   = (!rev_31)? e->e31->ea : e->e31->eb ;
    enew->area  = quarter_area ;
    enew->rad   = e->rad ;

    /* Finally, set e->center */
    e->center = ecenter ;

    /* Unlock the element */
    UNLOCK(e->elem_lock->lock);
}


/***************************************************************************
 *
 *    process_rays()
 *
 *    Gather rays.
 *
 *    Solution strategy:
 *            Original radiosity equation:    [I-M] B = E
 *            Iterative solution:                           2    3
 *                                            B = [I + M + M  + M  ...] E
 *            Sollution by this routine:      Bn = E + M Bn-1
 *                                            (B0 = E)
 *
 *
 *    process_rays() first checks the amount of error associated with each
 *    interaction. If it is large, then the interaction is refined.
 *    After all the interactions are examined, the light energy is
 *    gathered. This gathered energy and the energy passed from the ancestors
 *    are added and pushed down to descendants. The descendants in turn pop up
 *    the area weighted energy gathered at descendant level. Thus, the
 *    radiosity at level i is:
 *          Bi =  E
 *              + Sum(j=0 to i-1) gather(j)        --- gathered at ancestors
 *              + gather(i)                        --- gathered at this level
 *              + 1/4( Bi+1.right + Bi+1.left + Bi+1.top + Bi+1.center)
 *                                                 --- gathered at descendants
 *
 *    The implementation is slightly complicated due to many fork and join
 *    operations that occur when:
 *      (1) four children are visited and the results are added to Bi.
 *      (2) parallel visibility computation is done and then process_rays() resumed.
 *
 *    The former case would be a simple (non-tail)-recursive code in a
 *    uniprocessor program. However, because we create tasks to process lower
 *    levels and the creation of tasks does not mean the completion of
 *    processing at the lower level, we use a mechanism to suspend
 *    processing at this level and later resume it after the child tasks are
 *    finished.
 *    To be portable across many systems, in this implementation procedures
 *    are split at the point of task creation so that the subtask can call
 *    the next portion of the procedure on exit of the task.
 *    This mechanism can be best explained as the continuation-passing-style
 *    (CPS) where continuation means a lambda expression which represents 'the
 *    rest of the program'.
 *
 ****************************************************************************/


void process_rays(Element *e, long process_id)
{
    Interaction *i_list ;

    /* Detach interactions from the list */
    LOCK(e->elem_lock->lock);
    i_list = e->interactions ;
    e->interactions = (Interaction *)0 ;
    e->n_interactions = 0 ;
    UNLOCK(e->elem_lock->lock);

    /* For each interaction, do BF-error-analysis */
    bf_error_analysis_list( e, i_list, process_id ) ;

    if( e->n_vis_undef_inter == 0 )
        process_rays3( e, process_id ) ;
    else
        /* Some interactions were refined.
           Compute visibility and do BF-analysis again */
        create_visibility_tasks( e, (void (*)())process_rays2, process_id ) ;
}




static void process_rays2(Element *e, long process_id)
{
    Interaction *i_list ;

    /* Detach interactions from the vis-undef-list. They now have their
       visibility computed */
    LOCK(e->elem_lock->lock);
    i_list = e->vis_undef_inter ;
    e->vis_undef_inter = (Interaction *)0 ;
    e->n_vis_undef_inter = 0 ;
    UNLOCK(e->elem_lock->lock);

    /* For each interaction, do BF-error-analysis */
    bf_error_analysis_list( e, i_list, process_id ) ;

    if( e->n_vis_undef_inter == 0 )
        process_rays3( e, process_id ) ;
    else
        /* Some interactions were refined.
           Compute visibility and do BF-analysis again */
        create_visibility_tasks( e, (void (*)())process_rays2, process_id ) ;
}



static void process_rays3(Element *e, long process_id)
{
    Rgb rad_push ;		          /* Radiosity value pushed down */


    /* Gather light rays that impinge on this element */
    gather_rays( e, process_id ) ;

    /* Now visit children */
    if( ! LEAF_ELEMENT(e) )
        {
            /* Compute radiosity that is pushed to descendents */
            rad_push.r = e->rad_in.r + e->rad_subtree.r ;
            rad_push.g = e->rad_in.g + e->rad_subtree.g ;
            rad_push.b = e->rad_in.b + e->rad_subtree.b ;

            e->center->rad_in = rad_push ;
            e->top->   rad_in = rad_push ;
            e->right-> rad_in = rad_push ;
            e->left->  rad_in = rad_push ;
            e->join_counter = 4 ;

            /* Create tasks to process children */
            create_ray_task( e->center, process_id ) ;
            create_ray_task( e->top, process_id ) ;
            create_ray_task( e->left, process_id ) ;
            create_ray_task( e->right, process_id ) ;

            /* The rest of the job (pop up the computed radiosity) is
               handled by the continuation function, elem_join_operation().
               It is called when the lower level finishes.
               On exit, elem_join_operation() calls the continuation of the
               parent, which is also elem_join_operation().
               To be general, those tasks should be passed the continuation
               (elem_join_operation). But, since the continuation is obvious
               in this context, it is hardwired below */
        }
    else
        {
            /* Update element radiosity at the leaf level */
            e->rad.r = e->rad_in.r + e->rad_subtree.r + e->patch->emittance.r ;
            e->rad.g = e->rad_in.g + e->rad_subtree.g + e->patch->emittance.g ;
            e->rad.b = e->rad_in.b + e->rad_subtree.b + e->patch->emittance.b ;

            /* Ship out radiosity to the parent */
            elem_join_operation( e->parent, e, process_id ) ;
        }
}



/***************************************************************************
 *
 *    elem_join_operation()
 *
 *    This function performs the second half of the function of process_rays.
 *    That is, i)   add area weighted child radiosity
 *             ii)  update radiosity variable
 *             iii) call continuation of the parent (ie, elem_join_operation
 *                  itself)
 *
 *    Thus, in spirit, the function is a tail recursive routine although
 *    tail-recursion-elimination is manually performed in the code below.
 *
 ****************************************************************************/



static void elem_join_operation(Element *e, Element *ec, long process_id)
{
    long join_flag ;
#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_COSTBASED
    Patch_Cost *pc ;
#endif


    while( e != 0 )
        {
            /* Get radiosity of the child and add to my radiosity */
            LOCK(e->elem_lock->lock);
            e->rad_subtree.r += ec->rad_subtree.r * (float)0.25 ;
            e->rad_subtree.g += ec->rad_subtree.g * (float)0.25 ;
            e->rad_subtree.b += ec->rad_subtree.b * (float)0.25 ;
            e->join_counter-- ;
            join_flag = (e->join_counter == 0) ;
            UNLOCK(e->elem_lock->lock);

            if( join_flag == 0 )
                /* Other children are not finished. Return. */
                return ;

            /* This is the continuation called by the last (4th) subprocess.
               Perform JOIN at this level */
            /* Update element radiosity */
            e->rad.r = e->rad_in.r + e->rad_subtree.r + e->patch->emittance.r ;
            e->rad.g = e->rad_in.g + e->rad_subtree.g + e->patch->emittance.g ;
            e->rad.b = e->rad_in.b + e->rad_subtree.b + e->patch->emittance.b ;

            /* Traverse the tree one level up and repeat */
            ec = e ;
            e = e->parent ;
        }

    /* Process RAY root level finished. Update energy variable */
    LOCK(global->avg_radiosity_lock);
    global->total_energy.r += ec->rad.r * ec->area ;
    global->total_energy.g += ec->rad.g * ec->area ;
    global->total_energy.b += ec->rad.b * ec->area ;
    global->total_patch_area += ec->area ;
    UNLOCK(global->avg_radiosity_lock);

#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_COSTBASED
    /* Then update the cost variable of the patch */
    pc = &global->patch_cost[ ec->patch->seq_no ] ;
    pc->cost_history[3] = pc->cost_history[2] ;
    pc->cost_history[2] = pc->cost_history[1] ;
    pc->cost_history[1] = pc->cost_history[0] ;
    pc->cost_history[0] = PATCH_COST( pc ) ;
    pc->cost_estimate   = PATCH_COST_ESTIMATE( pc ) ;

    /* Also, update the global cost variable */
    LOCK(global->cost_sum_lock);
    global->cost_sum          += pc->cost_history[0] ;
    global->cost_estimate_sum += pc->cost_estimate ;
    UNLOCK(global->cost_sum_lock);
#endif

}



/***************************************************************************
 *
 *    gather_rays()
 *
 ****************************************************************************/

static void gather_rays(Element *elem, long process_id)
{
    Rgb rad_elem ;		/* Radiosity gathered by this elem */
    float ff_v ;		/* Form factor times visibility */
    float bf_r, bf_g, bf_b ;
    Interaction *inter ;


    /* Return immediately if there is no interaction */
    if( (inter = elem->interactions) == 0 )
        {
            elem->rad_subtree.r = 0.0 ;
            elem->rad_subtree.g = 0.0 ;
            elem->rad_subtree.b = 0.0 ;
            return ;
        }


    /* Gather rays of this element
       (do it directly without the driver function, 'Foreach-interaction') */
    rad_elem.r = 0.0 ;
    rad_elem.g = 0.0 ;
    rad_elem.b = 0.0 ;

    while( inter )
        {
            /* Be careful !
               Use FF(out) to compute incoming energy */
            ff_v = inter->formfactor_out * inter->visibility ;
            bf_r = ff_v * inter->destination->rad.r ;
            bf_g = ff_v * inter->destination->rad.g ;
            bf_b = ff_v * inter->destination->rad.b ;

            rad_elem.r += bf_r ;
            rad_elem.g += bf_g ;
            rad_elem.b += bf_b ;

            /* Update pointers */
            inter = inter->next ;
        }

    /* Multiply the gathered radiosity by the diffuse color of this element */
    rad_elem.r *= elem->patch->color.r ;
    rad_elem.g *= elem->patch->color.g ;
    rad_elem.b *= elem->patch->color.b ;

    /* Store the value at the initial value of 'rad_subtree' */
    elem->rad_subtree = rad_elem ;
}




/***************************************************************************
 *
 *    elememnt_completely_invisible()
 *
 *    Fast primary visibility test.
 *
 ****************************************************************************/

long element_completely_invisible(Element *e1, Element *e2, long process_id)
{
    long cc ;

    /* Check visibility */
    cc = patch_intersection( &e1->patch->plane_equ,
                            &e2->ev1->p, &e2->ev2->p, &e2->ev3->p, process_id ) ;
    if( NEGATIVE_SIDE(cc) )
        /* If negative or on the plane, then do nothing */
        return( 1 ) ;

    cc = patch_intersection( &e2->patch->plane_equ,
                            &e1->ev1->p, &e1->ev2->p, &e1->ev3->p, process_id ) ;
    if( NEGATIVE_SIDE(cc) )
        /* If negative or on the plane, then do nothing */
        return( 1 ) ;

    return( 0 ) ;
}


/***************************************************************************
 *
 *    get_element()
 *
 *    Create a new instance of Element
 *
 ****************************************************************************/

Element *get_element(long process_id)
{
    Element *p ;

    /* Lock the free list */
    LOCK(global->free_element_lock);

    /* Test pointer */
    if( global->free_element == 0 )
        {
            printf( "Fatal: Ran out of element buffer\n" ) ;
            UNLOCK(global->free_element_lock);
            exit( 1 ) ;
        }

    /* Get an element data structure */
    p = global->free_element ;
    global->free_element = p->center ;
    global->n_free_elements-- ;

    /* Unlock the list */
    UNLOCK(global->free_element_lock);

    /* Clear pointers just in case.. */
    p->parent             = 0 ;
    p->center             = 0 ;
    p->top                = 0 ;
    p->right              = 0 ;
    p->left               = 0 ;
    p->interactions       = 0 ;
    p->n_interactions     = 0 ;
    p->vis_undef_inter    = 0 ;
    p->n_vis_undef_inter  = 0 ;

    return( p ) ;
}



/***************************************************************************
 *
 *    leaf_element()
 *
 *    Returns TRUE(1) if this element is a leaf.
 *
 *    For strong and processor memory consistency models, this routine is not
 *    necessary. For weak and release consistency models, elem->center must be
 *    tested within the Lock.
 *
 ****************************************************************************/

long leaf_element(Element *elem, long process_id)
{
    long leaf ;

    LOCK(elem->elem_lock->lock);
    leaf  = _LEAF_ELEMENT(elem) ;
    UNLOCK(elem->elem_lock->lock);

    return( leaf ) ;
}


/***************************************************************************
 *
 *    init_elemlist()
 *
 *    Initialize Element free list
 *
 ****************************************************************************/

void init_elemlist(long process_id)
{
    long i ;

    /* Initialize Element free list */
    for( i = 0 ; i < MAX_ELEMENTS-1 ; i++ )
        {
            global->element_buf[i].center = &global->element_buf[i+1] ;
            /* Initialize lock variable */
            global->element_buf[i].elem_lock
                = get_sharedlock( SHARED_LOCK_SEG1, process_id ) ;
        }
    global->element_buf[ MAX_ELEMENTS-1 ].center = 0 ;
    global->element_buf[ MAX_ELEMENTS-1 ].elem_lock
        = get_sharedlock( SHARED_LOCK_SEG1, process_id ) ;

    global->free_element = global->element_buf ;
    global->n_free_elements = MAX_ELEMENTS ;
    LOCKINIT(global->free_element_lock);
}

/***************************************************************************
 *
 *    print_element()
 *
 *    Print contents of an element.
 *
 ****************************************************************************/

void print_element(Element *elem, long process_id)
{
    printf( "Element (%ld)\n", (long)elem ) ;

    print_point( &elem->ev1->p ) ;
    print_point( &elem->ev2->p ) ;
    print_point( &elem->ev3->p ) ;

    printf( "Radiosity:" ) ;     print_rgb( &elem->rad ) ;
}






/***************************************************************************
 ****************************************************************************
 *
 *    Methods for Interaction object
 *
 ****************************************************************************
 ****************************************************************************/
/***************************************************************************
 *
 *    foreach_interaction_in_element()
 *
 *    General purpose driver. For each interaction of the element, apply
 *    specified function.  The function is passed a pointer to the interaction.
 *    Traversal is linear.
 *
 ****************************************************************************/

void foreach_interaction_in_element(Element *elem, void (*func)(), long arg1, long process_id)
{
    Interaction *inter ;

    if( elem == 0 )
        return ;

    for( inter = elem->interactions ; inter ; inter = inter->next )
    {
       void (*_func)(Element *, Interaction *, long, long); 
       _func( elem, inter, arg1, process_id ) ;
    }
}



/***************************************************************************
 *
 *    compute_interaction()
 *    compute_formfactor()
 *
 *    compute_interaction() computes all the interaction parameters and fills
 *    in the entries of Interaction data structure.
 *    compute_formfactor() computes the formfactor only.
 *
 ****************************************************************************/

void compute_formfactor(Element *e_src, Element *e_dst, Interaction *inter, long process_id)
{
    float ff_c, ff_1, ff_2, ff_3 ;
    float ff_c1, ff_c2, ff_c3, ff_avg ;
    Vertex pc_src, pc_dst ;
    Vertex pc1_src, pc2_src, pc3_src ;
    float ff_min, ff_max, ff_err ;

    /* Estimate FF using disk approximation */
    /* (1) Compute FF(diff-disc) from the center of src to the destination */
    four_center_points( &e_src->ev1->p, &e_src->ev2->p, &e_src->ev3->p,
                       &pc_src, &pc1_src, &pc2_src, &pc3_src ) ;
    center_point( &e_dst->ev1->p, &e_dst->ev2->p, &e_dst->ev3->p, &pc_dst ) ;
    ff_c  = compute_diff_disc_formfactor( &pc_src,  e_src, &pc_dst, e_dst, process_id ) ;
    ff_c1 = compute_diff_disc_formfactor( &pc1_src, e_src, &pc_dst, e_dst, process_id ) ;
    ff_c2 = compute_diff_disc_formfactor( &pc2_src, e_src, &pc_dst, e_dst, process_id ) ;
    ff_c3 = compute_diff_disc_formfactor( &pc3_src, e_src, &pc_dst, e_dst, process_id ) ;
    if( ff_c  < 0 ) ff_c  = 0 ;
    if( ff_c1 < 0 ) ff_c1 = 0 ;
    if( ff_c2 < 0 ) ff_c2 = 0 ;
    if( ff_c3 < 0 ) ff_c3 = 0 ;
    ff_avg = (ff_c + ff_c1 + ff_c2 + ff_c3) * (float)0.25 ;
    ff_min = ff_max = ff_c ;
    if( ff_min > ff_c1 ) ff_min = ff_c1 ;
    if( ff_min > ff_c2 ) ff_min = ff_c2 ;
    if( ff_min > ff_c3 ) ff_min = ff_c3 ;
    if( ff_max < ff_c1 ) ff_max = ff_c1 ;
    if( ff_max < ff_c2 ) ff_max = ff_c2 ;
    if( ff_max < ff_c3 ) ff_max = ff_c3 ;

    /* (2) Compute FF(diff-disc) from the 3 vertices of the source */
    ff_1 = compute_diff_disc_formfactor( &e_src->ev1->p, e_src,
                                        &pc_dst, e_dst, process_id ) ;
    ff_2 = compute_diff_disc_formfactor( &e_src->ev2->p, e_src,
                                        &pc_dst, e_dst, process_id ) ;
    ff_3 = compute_diff_disc_formfactor( &e_src->ev3->p, e_src,
                                        &pc_dst, e_dst, process_id ) ;

    /* (3) Find FF min and max */
    ff_min = ff_max = ff_c ;
    if( ff_min > ff_1 ) ff_min = ff_1 ;
    if( ff_min > ff_2 ) ff_min = ff_2 ;
    if( ff_min > ff_3 ) ff_min = ff_3 ;
    if( ff_max < ff_1 ) ff_max = ff_1 ;
    if( ff_max < ff_2 ) ff_max = ff_2 ;
    if( ff_max < ff_3 ) ff_max = ff_3 ;

    /* (4) Clip FF(diff-disc) if it is negative */
    if( ff_avg < 0 )
        ff_avg = 0 ;
    inter->formfactor_out = ff_avg ;

    /* (5) Then find maximum difference from the FF at the center */
    ff_err = (ff_max - ff_avg) ;
    if( ff_err < (ff_avg - ff_min) )
        ff_err = ff_avg - ff_min ;
    inter->formfactor_err = ff_err ;

    /* (6) Correct visibility if partially visible */
    if( (ff_avg < 0) && (inter->visibility == 0) )
        /* All ray missed the visible portion of the elements.
           Set visibility to a non-zero value manually */
        /** inter->visibility = FF_VISIBILITY_ERROR **/ ;

    /* (7) Fill destination */
    inter->destination = e_dst ;
}


static float _diff_disc_formfactor(Vertex *p, Element *e_src, Vertex *p_disc, float area, Vertex *normal, long process_id)
{
    Vertex vec_sd ;
    float dist_sq ;
    float fnorm ;
    float  cos_s, cos_d, angle_factor ;

    vec_sd.x = p_disc->x - p->x ;
    vec_sd.y = p_disc->y - p->y ;
    vec_sd.z = p_disc->z - p->z ;
    dist_sq = vec_sd.x*vec_sd.x + vec_sd.y*vec_sd.y + vec_sd.z*vec_sd.z ;

    fnorm = area / ((float)M_PI * dist_sq  + area) ;

    /* (2) Now, consider angle to the other patch from the normal. */
    normalize_vector( &vec_sd, &vec_sd ) ;
    cos_s =  inner_product( &vec_sd, &e_src->patch->plane_equ.n ) ;
    cos_d = -inner_product( &vec_sd, normal ) ;
    angle_factor = cos_s * cos_d ;

    /* Return the form factor */
    return( fnorm * angle_factor ) ;
}


static float compute_diff_disc_formfactor(Vertex *p, Element *e_src, Vertex *p_disc, Element *e_dst, long process_id)
{
    Vertex p_c, p_c1, p_c2, p_c3 ;
    float quarter_area ;
    float ff_c, ff_c1, ff_c2, ff_c3 ;

    four_center_points( &e_dst->ev1->p, &e_dst->ev2->p, &e_dst->ev3->p,
                       &p_c, &p_c1, &p_c2, &p_c3 ) ;

    quarter_area = e_dst->area * (float)0.25 ;

    ff_c = _diff_disc_formfactor( p, e_src, &p_c,  quarter_area,
                                 &e_dst->patch->plane_equ.n, process_id ) ;
    ff_c1= _diff_disc_formfactor( p, e_src, &p_c1, quarter_area,
                                 &e_dst->patch->plane_equ.n, process_id ) ;
    ff_c2= _diff_disc_formfactor( p, e_src, &p_c2, quarter_area,
                                 &e_dst->patch->plane_equ.n, process_id ) ;
    ff_c3= _diff_disc_formfactor( p, e_src, &p_c3, quarter_area,
                                 &e_dst->patch->plane_equ.n, process_id ) ;

    if( ff_c  < 0 ) ff_c  = 0 ;
    if( ff_c1 < 0 ) ff_c1 = 0 ;
    if( ff_c2 < 0 ) ff_c2 = 0 ;
    if( ff_c3 < 0 ) ff_c3 = 0 ;

    return( ff_c + ff_c1 + ff_c2 + ff_c3 ) ;
}


void compute_interaction(Element *e_src, Element *e_dst, Interaction *inter, long subdiv, long process_id)
{
    /* (1) Check visibility. */
    if( NO_VISIBILITY_NECESSARY(subdiv) )
        inter->visibility = 1.0 ;
    else
        inter->visibility = VISIBILITY_UNDEF ;


    /* (2) Compute formfactor */
    compute_formfactor( e_src, e_dst, inter, process_id ) ;
}



/***************************************************************************
 *
 *    insert_interaction()
 *    delete_interaction()
 *    insert_vis_undef_interaction()
 *    delete_vis_undef_interaction()
 *
 *    Insert/Delete interaction from the interaction list.
 *
 ****************************************************************************/


void insert_interaction(Element *elem, Interaction *inter, long process_id)
{
    /* Link from patch 1 to patch 2 */
    LOCK(elem->elem_lock->lock);
    inter->next = elem->interactions ;
    elem->interactions = inter ;
    elem->n_interactions++ ;
    UNLOCK(elem->elem_lock->lock);
}



void delete_interaction(Element *elem, Interaction *prev, Interaction *inter, long process_id)
{
    /* Remove from the list */
    LOCK(elem->elem_lock->lock);
    if( prev == 0 )
        elem->interactions = inter->next ;
    else
        prev->next = inter->next ;
    elem->n_interactions-- ;
    UNLOCK(elem->elem_lock->lock);

    /* Return to the free list */
    free_interaction( inter, process_id ) ;
}



void insert_vis_undef_interaction(Element *elem, Interaction *inter, long process_id)
{
    /* Link from patch 1 to patch 2 */
    LOCK(elem->elem_lock->lock);
    inter->next = elem->vis_undef_inter ;
    elem->vis_undef_inter = inter ;
    elem->n_vis_undef_inter++ ;
    UNLOCK(elem->elem_lock->lock);
}

void delete_vis_undef_interaction(Element *elem, Interaction *prev, Interaction *inter, long process_id)
{
    /* Remove from the list */
    LOCK(elem->elem_lock->lock);
    if( prev == 0 )
        elem->vis_undef_inter = inter->next ;
    else
        prev->next = inter->next ;
    elem->n_vis_undef_inter-- ;
    UNLOCK(elem->elem_lock->lock);
}


/***************************************************************************
 *
 *    get_interaction()
 *    free_interaction()
 *
 *    Create/delete an instance of an interaction.
 *
 ****************************************************************************/

Interaction *get_interaction(long process_id)
{
    Interaction *p ;

    /* Lock the free list */
    LOCK(global->free_interaction_lock);

    /* Test pointer */
    if( global->free_interaction == 0 )
        {
            printf( "Fatal: Ran out of interaction buffer\n" ) ;
            UNLOCK(global->free_interaction_lock);
            exit( 1 ) ;
        }

    /* Get an element data structure */
    p = global->free_interaction ;
    global->free_interaction = p->next ;
    global->n_free_interactions-- ;

    /* Unlock the list */
    UNLOCK(global->free_interaction_lock);

    /* Clear pointers just in case.. */
    p->next   = 0 ;
    p->destination = 0 ;


    return( p ) ;
}



void free_interaction(Interaction *interaction, long process_id)
{
    /* Lock the free list */
    LOCK(global->free_interaction_lock);

    /* Get a task data structure */
    interaction->next = global->free_interaction ;
    global->free_interaction = interaction ;
    global->n_free_interactions++ ;

    /* Unlock the list */
    UNLOCK(global->free_interaction_lock);
}


/***************************************************************************
 *
 *    init_interactionlist()
 *
 *    Initialize Interaction free list
 *
 ****************************************************************************/

void init_interactionlist(long process_id)
{
    long i ;

    /* Initialize Interaction free list */
    for( i = 0 ; i < MAX_INTERACTIONS-1 ; i++ )
        global->interaction_buf[i].next = &global->interaction_buf[i+1] ;
    global->interaction_buf[ MAX_INTERACTIONS-1 ].next = 0 ;
    global->free_interaction = global->interaction_buf ;
    global->n_free_interactions = MAX_INTERACTIONS ;
    LOCKINIT(global->free_interaction_lock);
}


/***************************************************************************
 *
 *    print_interaction()
 *
 *    Print interaction data structure.
 *
 ****************************************************************************/

void print_interaction(Interaction *inter, long process_id)
{

    printf( "Interaction(0x%ld)\n", (long)inter ) ;
    printf( "    Dest: Elem (0x%ld) of patch %ld\n",
           (long)inter->destination, inter->destination->patch->seq_no ) ;
    printf( "    Fout: %f    Vis: %f\n",
           inter->formfactor_out,
           inter->visibility ) ;
    printf( "    Next: 0x%p\n", inter->next ) ;
}
