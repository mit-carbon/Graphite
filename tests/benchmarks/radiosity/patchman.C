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
 *	Patch management.
 *
 *       This module has the following functions:
 *       (1) Create/initialize a new instance of the patch object.
 *       (2) Management of BSP tree (insertion,traversal)
 *
 *************************************************************************/

#include <stdio.h>

EXTERN_ENV;

include(radiosity.h)

static void _foreach_patch(Patch *node, void (*func)(), long arg1, long process_id);
static void _foreach_d_s_patch(Vertex *svec, Patch *node, void (*func)(), long arg1, long process_id);
static void split_into_3(Patch *patch, ElemVertex *ev1, ElemVertex *ev2, ElemVertex *ev3, Edge *e12, Edge *e23, Edge *e31, Patch *parent, long process_id);
static void split_into_2(Patch *patch, ElemVertex *ev1, ElemVertex *ev2, ElemVertex *ev3, Edge *e12, Edge *e23, Edge *e31, Patch *parent, long process_id);

/***************************************************************************
 ****************************************************************************
 *
 *    Methods of Patch object
 *
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 *
 *    foreach_patch_in_bsp()
 *
 *    General purpose driver. For each patch in the BSP tree, apply specified
 *    function.  The function is passed a pointer to the patch.
 *    Traversal is in-order, that is, subtree(-) -> node -> subtree(+).
 *
 ****************************************************************************/

void foreach_patch_in_bsp(void (*func)(), long  arg1, long process_id)
{
    _foreach_patch( global->bsp_root, func, arg1, process_id ) ;
}


static void _foreach_patch(Patch *node, void (*func)(), long   arg1, long process_id)
{
    if( node == 0 )
        return ;

    /* Process subtree(-) */
    if( node->bsp_negative )
        _foreach_patch( node->bsp_negative, func, arg1, process_id ) ;

    void (*_func)(Patch *, long, long) = (void (*)(Patch *, long, long))func;

    /* Apply function to this node */
    _func( node, arg1, process_id ) ;

    /* Process subtree(+) */
    if( node->bsp_positive )
        _foreach_patch( node->bsp_positive, func, arg1, process_id ) ;
}




/***************************************************************************
 *
 *    foreach_depth_sorted_patch()
 *
 *    For each patch in the BSP tree, apply specified function.  In the depth
 *    sorted order along the given vector (from tail to arrow head of the
 *    vector).
 *    The function is passed a pointer to the patch.
 *
 ****************************************************************************/

void foreach_depth_sorted_patch(Vertex *sort_vec, void (*func)(), long  arg1, long process_id)
{
    _foreach_d_s_patch( sort_vec, global->bsp_root, func, arg1, process_id ) ;
}


static void _foreach_d_s_patch(Vertex *svec, Patch *node, void (*func)(), long arg1, long process_id)
{
    float sign ;

    if( node == 0 )
        return ;

    /* Compute inner product */
    sign = inner_product( svec, &node->plane_equ.n ) ;

    if( sign >= 0.0 )
        {
            /* The vector is approaching from the negative side of the patch */

            /* Process subtree(-) */
            if( node->bsp_negative )
                _foreach_d_s_patch( svec, node->bsp_negative, func, arg1, process_id ) ;

            void (*_func)(Patch *, long, long) = (void (*)(Patch *, long, long))func;

            /* Apply function to this node */
            _func( node, arg1, process_id ) ;

            /* Process subtree(+) */
            if( node->bsp_positive )
                _foreach_d_s_patch( svec, node->bsp_positive, func, arg1, process_id ) ;
        }
    else
        {
            /* Process subtree(+) */
            if( node->bsp_positive )
                _foreach_d_s_patch( svec, node->bsp_positive, func, arg1, process_id ) ;

            void (*_func)( Patch *, long, long) = (void (*)(Patch *, long, long))func;

            /* Apply function to this node */
            _func( node, arg1, process_id ) ;

            /* Process subtree(-) */
            if( node->bsp_negative )
                _foreach_d_s_patch( svec, node->bsp_negative, func, arg1, process_id ) ;
        }
}




/***************************************************************************
 *
 *    define_patch()
 *
 *    Insert a new patch in the BSP tree and put refinement task in the queue.
 *
 ****************************************************************************/

void define_patch(Patch *patch, Patch *root, long process_id)
{
    Patch *parent = root ;
    long xing_code ;

    /* Lock the BSP tree */
    LOCK(global->bsp_tree_lock);

    /* If this is the first patch, link directly */
    if( parent == 0 )
        {
            if( global->bsp_root == 0 )
                {
                    /* This is really the first patch */
                    global->bsp_root = patch ;
                    patch->bsp_positive = 0 ;
                    patch->bsp_negative = 0 ;
                    patch->bsp_parent   = 0 ;
                    attach_element( patch, process_id ) ;
                    UNLOCK(global->bsp_tree_lock);

                    return ;
                }
            else
                /* Race condition. The root was NULL when the task was
                   created */
                parent = global->bsp_root ;
        }

    /* Traverse the BSP tree and get to the leaf node */
    while( 1 )
        {
            /* Check the sign */
            xing_code = patch_intersection( &parent->plane_equ, &patch->p1,
                                           &patch->p2, &patch->p3, process_id ) ;

            /* Traverse down the tree according to the sign */
            if( POSITIVE_SIDE( xing_code ) )
                {
                    if( parent->bsp_positive == 0 )
                        {
                            /* Insert the patch */
                            parent->bsp_positive = patch ;
                            patch->bsp_parent = parent ;
                            attach_element( patch, process_id ) ;
                            UNLOCK(global->bsp_tree_lock);

                            foreach_patch_in_bsp( (void (*)())refine_newpatch, (long)patch, process_id ) ;
                            return ;
                        }
                    else
                        /* Traverse down to the subtree(+) */
                        parent = parent->bsp_positive ;
                }
            else if( NEGATIVE_SIDE( xing_code ) )
                {
                    if( parent->bsp_negative == 0 )
                        {
                            /* Insert the patch */
                            parent->bsp_negative = patch ;
                            patch->bsp_parent = parent ;
                            attach_element( patch, process_id ) ;
                            UNLOCK(global->bsp_tree_lock);

                            foreach_patch_in_bsp( (void (*)())refine_newpatch, (long)patch, process_id ) ;
                            return ;
                        }
                    else
                        /* Traverse down to the subtree(-) */
                        parent = parent->bsp_negative ;
                }
            else
                {
                    /* The patch must be split. Insertion is taken care of by
                       split_patch(). */
                    UNLOCK(global->bsp_tree_lock);
                    split_patch( patch, parent, xing_code, process_id ) ;
                    return ;
                }
        }
}


/***************************************************************************
 *
 *    split_patch()
 *    split_into_3()
 *    split_into_2()
 *
 *    Split a patch and insert in the BSP tree.
 *    split_into_3()  Split a patch into 3 patches. The routine assumes both
 *                    P1-P2 and P1-P3 intersect the plane.
 *    split_into_2()  Split a patch into 2 patches. The routine assuems P1 is
 *                    on the plane and P2-P3 intersects the plane.
 *    split_patch()   Classify intersection type, rename vertices, and
 *                    call split_into_X().
 *
 ****************************************************************************/

void split_patch(Patch *patch, Patch *node, long xing_code, long process_id)
{
    long   c1, c2, c3 ;


    c1 = P1_CODE( xing_code ) ;
    c2 = P2_CODE( xing_code ) ;
    c3 = P3_CODE( xing_code ) ;

    /* Classify intersection type */
    if( c1 == c2 )
        /* P3 is on the oposite side */
        split_into_3( patch, patch->ev3, patch->ev1, patch->ev2,
                     patch->e31, patch->e12, patch->e23, node, process_id) ;
    else if( c1 == c3 )
        /* P2 is on the oposite side */
        split_into_3( patch, patch->ev2, patch->ev3, patch->ev1,
                     patch->e23, patch->e31, patch->e12, node, process_id ) ;
    else if( c2 == c3 )
        /* P1 is on the oposite side */
        split_into_3( patch, patch->ev1, patch->ev2,  patch->ev3,
                     patch->e12, patch->e23, patch->e31, node, process_id ) ;
    else if( c1 == POINT_ON_PLANE )
        /* P1 is on the plane. P2 and P3 are on the oposite side */
        split_into_2( patch, patch->ev1, patch->ev2, patch->ev3,
                     patch->e12, patch->e23, patch->e31, node, process_id ) ;
    else if( c2 == POINT_ON_PLANE )
        /* P2 is on the plane. P3 and P1 are on the oposite side */
        split_into_2( patch, patch->ev2, patch->ev3, patch->ev1,
                     patch->e23, patch->e31, patch->e12, node, process_id ) ;
    else
        /* P3 is on the plane. P1 and P2 are on the oposite side */
        split_into_2( patch, patch->ev3, patch->ev1, patch->ev2,
                     patch->e31, patch->e12, patch->e23, node, process_id ) ;
}



static void split_into_3(Patch *patch, ElemVertex *ev1, ElemVertex *ev2, ElemVertex *ev3, Edge *e12, Edge *e23, Edge *e31, Patch *parent, long process_id)
{
    ElemVertex *ev_a ;	   /* Intersection of P1-P2 & the patch */
    ElemVertex *ev_b ;	   /* Intersection of P1-P3 & the patch */
    float h1, h2, h3 ;
    float u2, u3 ;
    Patch *_new ;
    Edge  *e_ab, *e_3a ;
    long   rev_e12, rev_e31 ;


    /* Compute intersection in terms of parametarized distance from P1 */
    h1 = plane_equ( &parent->plane_equ, &ev1->p, process_id ) ;
    h2 = plane_equ( &parent->plane_equ, &ev2->p, process_id ) ;
    h3 = plane_equ( &parent->plane_equ, &ev3->p, process_id ) ;

    /* NOTE: Length of P1-P2 and P1-P3 are at least 2*F_COPLANAR.
       So, no check is necessary before division */
    u2 = h1 / (h1 - h2) ;
    if( (rev_e12 = EDGE_REVERSE( e12, ev1, ev2 )) )
        subdivide_edge( e12, u2, process_id ) ;
    else
        subdivide_edge( e12, (float)1.0 - u2, process_id ) ;
    ev_a = e12->ea->pb ;

    u3 = h1 / (h1 - h3) ;
    if( (rev_e31 = EDGE_REVERSE( e31, ev3, ev1 )) )
        subdivide_edge( e31, (float)1.0 - u3, process_id ) ;
    else
        subdivide_edge( e31, u3, process_id ) ;
    ev_b = e31->ea->pb ;

    /* Now insert patches in the tree */

    /* (1) Put P1-Pa-Pb */
    _new = get_patch(process_id) ;
    _new->p1        = ev1->p ;
    _new->p2        = ev_a->p ;
    _new->p3        = ev_b->p ;

    _new->ev1       = ev1 ;
    _new->ev2       = e12->ea->pb ;
    _new->ev3       = e31->ea->pb ;

    _new->e12       = (!rev_e12)? e12->ea : e12->eb ;
    _new->e23       = e_ab = create_edge(ev_a, ev_b, process_id ) ;
    _new->e31       = (!rev_e31)? e31->eb : e31->ea ;

    _new->plane_equ = patch->plane_equ ;
    _new->area      = u2 * u3 * patch->area ;
    _new->color     = patch->color ;
    _new->emittance = patch->emittance ;
    define_patch( _new, parent, process_id ) ;

    /* (2) Put Pa-P2-P3 */
    _new = get_patch(process_id) ;
    _new->p1        = ev_a->p ;
    _new->p2        = ev2->p ;
    _new->p3        = ev3->p ;

    _new->ev1       = ev_a ;
    _new->ev2       = ev2 ;
    _new->ev3       = ev3 ;

    _new->e12       = (!rev_e12)? e12->eb : e12->ea ;
    _new->e23       = e23 ;
    _new->e31       = e_3a = create_edge( ev3, ev_a, process_id ) ;

    _new->plane_equ = patch->plane_equ ;
    _new->area      = (1.0 - u2) * patch->area ;
    _new->color     = patch->color ;
    _new->emittance = patch->emittance ;
    define_patch( _new, parent, process_id ) ;

    /* (3) Put Pa-P3-Pb. Reuse the original patch */
    patch->p1      = ev_a->p ;
    patch->p2      = ev3->p ;
    patch->p3      = ev_b->p ;

    patch->ev1     = ev_a ;
    patch->ev2     = ev3 ;
    patch->ev3     = ev_b ;

    patch->e12     = e_3a ;
    patch->e23     = (!rev_e31)? e31->ea : e31->eb ;
    patch->e31     = e_ab ;

    patch->area    = u2 * (1.0 - u3) * patch->area ;
    define_patch( patch, parent, process_id ) ;
}


static void split_into_2(Patch *patch, ElemVertex *ev1, ElemVertex *ev2, ElemVertex *ev3, Edge *e12, Edge *e23, Edge *e31, Patch *parent, long process_id)
{
    ElemVertex *ev_a ;
    Edge *e_a1 ;
    float h2, h3 ;
    float u2 ;
    Patch *_new ;
    long  rev_e23 ;

    /* Compute intersection in terms of parameterized distance from P2 */
    h2 = plane_equ( &parent->plane_equ, &ev2->p, process_id ) ;
    h3 = plane_equ( &parent->plane_equ, &ev3->p, process_id ) ;

    /* NOTE: Length of P2-P3 is at least 2*F_COPLANAR.
       So, no check is necessary before division */
    u2 = h2 / (h2 - h3) ;
    if( (rev_e23 = EDGE_REVERSE( e23, ev2, ev3 )) )
        subdivide_edge( e23, u2, process_id ) ;
    else
        subdivide_edge( e23, (float)1.0 - u2, process_id ) ;
    ev_a = e23->ea->pb ;


    /* Now put patches in the tree */

    /* (1) Put P1-P2-Pa */
    _new = get_patch(process_id) ;

    _new->p1        = ev1->p ;
    _new->p2        = ev2->p ;
    _new->p3        = ev_a->p ;

    _new->ev1       = ev1 ;
    _new->ev2       = ev2 ;
    _new->ev3       = ev_a ;

    _new->e12       = e12 ;
    _new->e23       = (!rev_e23)? e23->ea : e23->eb ;
    _new->e31       = e_a1 = create_edge( ev_a, ev1, process_id ) ;

    _new->plane_equ = patch->plane_equ ;
    _new->area      = u2 * patch->area ;
    _new->color     = patch->color ;
    _new->emittance = patch->emittance ;
    define_patch( _new, parent, process_id ) ;

    /* (2) Put P1-Pa-P3.  Reuse the original patch */
    patch->p1      = ev1->p ;
    patch->p2      = ev_a->p ;
    patch->p3      = ev3->p ;

    patch->ev1     = ev1 ;
    patch->ev2     = ev_a ;
    patch->ev3     = ev3 ;

    patch->e12     = e_a1 ;
    patch->e23     = (!rev_e23)? e23->eb : e23->ea ;
    patch->e31     = e31 ;

    patch->area    = (1.0 - u2) * patch->area ;
    define_patch( patch, parent, process_id ) ;
}



/***************************************************************************
 *
 *    attach_element()
 *
 *    Attach an element to the patch. This element becomes the
 *    root of the quad tree.
 *
 ****************************************************************************/

void attach_element(Patch *patch, long process_id)
{
    Element *pelem ;

    /* Create and link an element to the patch */
    pelem = get_element(process_id) ;
    patch->el_root = pelem ;

    /* Initialization of the element */
    pelem->patch = patch ;
    pelem->ev1   = patch->ev1 ;
    pelem->ev2   = patch->ev2 ;
    pelem->ev3   = patch->ev3 ;

    pelem->e12   = patch->e12 ;
    pelem->e23   = patch->e23 ;
    pelem->e31   = patch->e31 ;

    pelem->area  = patch->area ;
    pelem->rad   = patch->emittance ;
}



/***************************************************************************
 *
 *    refine_newpatch()
 *
 *    Recursively subdivide
 *
 ****************************************************************************/


void refine_newpatch(Patch *patch, long newpatch, long process_id)
{
    long cc ;
    Patch *new_patch = (Patch *)newpatch ;

    /* Check sequence number */
    if( patch->seq_no >= new_patch->seq_no )
        /* Racing condition due to multiprocessing */
        return ;

    /* Check visibility */
    cc = patch_intersection( &patch->plane_equ,
                            &new_patch->p1, &new_patch->p2, &new_patch->p3, process_id ) ;
    if( NEGATIVE_SIDE(cc) )
        /* If negative or on the plane, then do nothing */
        return ;

    cc = patch_intersection( &new_patch->plane_equ,
                            &patch->p1, &patch->p2, &patch->p3, process_id ) ;
    if( NEGATIVE_SIDE(cc) )
        /* If negative or on the plane, then do nothing */
        return ;

    /* Create a new task or do it by itself */
    create_ff_refine_task( patch->el_root, new_patch->el_root, 0, process_id ) ;
}


/***************************************************************************
 *
 *    get_patch()
 *
 *    Returns a new instance of the Patch object.
 *
 ****************************************************************************/

Patch *get_patch(long process_id)
{
    Patch *p ;

    /* LOCK the free list */
    LOCK(global->free_patch_lock);

    /* Test pointer */
    if( global->free_patch == 0 )
        {
            printf( "Fatal: Ran out of patch buffer\n" ) ;
            UNLOCK(global->free_patch_lock);
            exit( 1 ) ;
        }

    /* Get a patch data structure */
    p = global->free_patch ;
    global->free_patch = p->bsp_positive ;
    global->n_total_patches++ ;
    global->n_free_patches-- ;

    /* Unlock the list */
    UNLOCK(global->free_patch_lock);

    /* Clear pointers just in case.. */
    p->el_root = 0 ;
    p->bsp_positive = 0 ;
    p->bsp_negative = 0 ;
    p->bsp_parent   = 0 ;

    return( p ) ;
}



/***************************************************************************
 *
 *    init_patchlist()
 *
 *    Initialize patch free list.
 *
 ****************************************************************************/

void init_patchlist(long process_id)
{
    long i ;

    /* Initialize Patch free list */
    for( i = 0 ; i < MAX_PATCHES-1 ; i++ )
        {
            global->patch_buf[i].bsp_positive = &global->patch_buf[i+1] ;
            global->patch_buf[i].seq_no = i ;
        }
    global->patch_buf[ MAX_PATCHES-1 ].bsp_positive = 0 ;
    global->patch_buf[ MAX_PATCHES-1 ].seq_no = MAX_PATCHES - 1 ;

    global->free_patch = global->patch_buf ;
    global->n_total_patches = 0 ;
    global->n_free_patches  = MAX_PATCHES ;
    LOCKINIT(global->free_patch_lock) ;

#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_COSTBASED
    /* Initialize Patch_Cost structure */
    for( i = 0 ; i < MAX_PATCHES ; i++ )
        {
            global->patch_cost[i].patch  = &global->patch_buf[i] ;
            global->patch_cost[i].cost_lock
                = get_sharedlock( SHARED_LOCK_SEGANY, process_id ) ;
            global->patch_cost[i].n_bsp_node      = 0 ;
            global->patch_cost[i].n_total_inter   = 0 ;
            global->patch_cost[i].cost_estimate   = 0 ;
            global->patch_cost[i].cost_history[0] = 0 ;
            global->patch_cost[i].cost_history[1] = 0 ;
            global->patch_cost[i].cost_history[2] = 0 ;
            global->patch_cost[i].cost_history[3] = 0 ;
        }
#endif
}


/***************************************************************************
 *
 *    print_patch()
 *
 *    Print patch information.
 *
 ****************************************************************************/

void print_patch(Patch *patch, long process_id)
{
    printf( "Patch (%ld)\n", (long)patch ) ;
    print_point( &patch->p1 ) ;
    print_point( &patch->p2 ) ;
    print_point( &patch->p3 ) ;
    print_plane_equ( &patch->plane_equ, process_id ) ;
    printf( "\tArea %f\n", patch->area ) ;
}



/***************************************************************************
 *
 *    print_bsp_tree()
 *
 *    Print BSP tree
 *
 ****************************************************************************/

void print_bsp_tree(long process_id)
{
    printf( "**** BSP TREE ***\n" ) ;
    foreach_patch_in_bsp( (void (*)())_pr_patch, 0, process_id ) ;
    printf( "\n\n" ) ;
}

void _pr_patch(Patch *patch, long dummy, long process_id)
{
    print_patch( patch, process_id ) ;
}


/***************************************************************************
 ****************************************************************************
 *
 *    Methods for PlaneEqu object
 *
 ****************************************************************************
 ****************************************************************************
 ****************************************************************************
 *
 *
 *    plane_equ()
 *
 *    Returns the value H(Px, Py, Pz)  where H is the equation of the plane
 *
 ****************************************************************************/

float plane_equ(PlaneEqu *plane, Vertex *point, long process_id)
{
    float h ;
    h = plane->c + point->x * plane->n.x
        + point->y * plane->n.y
            + point->z * plane->n.z ;

    return( h ) ;
}

/***************************************************************************
 *
 *    comp_plane_equ()
 *
 *    Compute plane equation from the three vertices on the plane.
 *
 ****************************************************************************/

float comp_plane_equ(PlaneEqu *pln, Vertex *p1, Vertex *p2, Vertex *p3, long process_id)
{
    float length ;

    /* Compute normal vector */
    length = plane_normal( &pln->n, p1, p2, p3 ) ;

    /* Calculate constant factor */
    pln->c = -inner_product( &pln->n, p1 ) ;

    return( length ) ;
}


/***************************************************************************
 *
 *    point_intersection()
 *    patch_intersection()
 *
 *    Returns the intersection code according to the relationship between the
 *    point/patch and the plane.
 *    point_intersection() returns 2 bits code that represents:
 *        01:  Point is on the positive side (H(x,y,z) > 0)
 *        10:  Point is on the negative side (H(x,y,z) < 0)
 *        00:  Point is on the plane         (H(x,y,z) = 0)
 *
 *    patch_intersection() returns 3 sets of 2 bits code each represents the
 *    relationship of each vertex of the triangle patch.
 *
 ****************************************************************************/

long point_intersection(PlaneEqu *plane, Vertex *point, long process_id)
{
    float h ;
    long result_code = 0 ;

    /* Compare H(x,y,z) against allowance */
    if( (h = plane_equ( plane, point, process_id )) < -F_COPLANAR )
        result_code |= POINT_NEGATIVE_SIDE ;
    if( h > F_COPLANAR )
        result_code |= POINT_POSITIVE_SIDE ;

    return( result_code ) ;
}



long patch_intersection(PlaneEqu *plane, Vertex *p1, Vertex *p2, Vertex *p3, long process_id)
{
    long c1, c2, c3 ;

    c1 = point_intersection( plane, p1, process_id ) ;
    c2 = point_intersection( plane, p2, process_id ) ;
    c3 = point_intersection( plane, p3, process_id ) ;

    return( (c3 << 4) | (c2 << 2) | c1 ) ;
}


/***************************************************************************
 *
 *    print_plane_equ()
 *
 *    Print plane equation
 *
 ****************************************************************************/

void print_plane_equ(PlaneEqu *peq, long process_id)
{
    printf( "\tPLN: %.3f x + %.3f y + %.3f z + %.3f\n",
           peq->n.x, peq->n.y, peq->n.z, peq->c ) ;
}


