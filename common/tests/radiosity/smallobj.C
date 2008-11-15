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
 *	Class methods for small simple objects.
 *
 *
 *************************************************************************/

#include <stdio.h>

EXTERN_ENV;

include(radiosity.h)

struct {
    char pad1[PAGE_SIZE];	 	/* padding to avoid false-sharing
        and allow page-placement */
    long          n_local_free_elemvertex    ;
    ElemVertex  *local_free_elemvertex      ;
    long    n_local_free_edge    ;
    Edge  *local_free_edge      ;
    long lock_alloc_counter  ;
    char pad2[PAGE_SIZE];	 	/* padding to avoid false-sharing
        and allow page-placement */
} sobj_struct[MAX_PROCESSORS];


/***************************************************************************
****************************************************************************
*
*    Methods for Vertex object
*
****************************************************************************
****************************************************************************/

/***************************************************************************
 *
 *    vector_length()
 *
 *    Comute length of a vector represented by Vertex
 *    length = | v |
 *
 ****************************************************************************/

float vector_length(Vertex *v)
{
    double t0, t1, t2 ;

    t0 = v->x * v->x ;
    t1 = v->y * v->y ;
    t2 = v->z * v->z ;

    return( sqrt( t0 + t1 + t2 ) ) ;
}


/***************************************************************************
*
*    distance()
  *
  *    Comute distance of two points.
  *    dist = | P1 - P2 |
  *
  ****************************************************************************/

float distance(Vertex *p1, Vertex *p2)
{
    Vertex v12 ;

    v12.x = p2->x - p1->x ;
    v12.y = p2->y - p1->y ;
    v12.z = p2->z - p1->z ;

    return( vector_length( &v12 ) ) ;
}

/***************************************************************************
*
*    normalize_vector()
  *
  *    Normalize vector represented by Vertex
  *    v1 <- normalized( v2 )
  *
  ****************************************************************************/

float normalize_vector(Vertex *v1, Vertex *v2)
{
    float t0 ;
    float length ;

    length = vector_length( v2 ) ;
    t0 = (float)1.0 / length ;

    v1->x = v2->x * t0 ;
    v1->y = v2->y * t0 ;
    v1->z = v2->z * t0 ;

    return( length ) ;
}


/**************************************************************************
*
*    inner_product()
  *
  *          (v1.v2) <- inner_product( v1, v2 )
  *
  ***************************************************************************/

float inner_product(Vertex *v1, Vertex *v2)
{
    float ip ;

    ip  = v1->x * v2->x ;
    ip += v1->y * v2->y ;
    ip += v1->z * v2->z ;

    return( ip ) ;
}


/**************************************************************************
*
*    cross_product()
  *
  *          Vc = V1 X V2
  *
  ***************************************************************************/

void cross_product(Vertex *vc, Vertex *v1, Vertex *v2)
{
    vc->x = v1->y * v2->z  -  v1->z * v2->y ;
    vc->y = v1->z * v2->x  -  v1->x * v2->z ;
    vc->z = v1->x * v2->y  -  v1->y * v2->x ;
}


/**************************************************************************
*
*    plane_normal()
  *
  *          Vc = (P2-P1) X (P3-P1) /  |(P2-P1) X (P3-P1)|
  *
  ***************************************************************************/

float plane_normal(Vertex *vc, Vertex *p1, Vertex *p2, Vertex *p3)
{
    Vertex v1, v2 ;

    /* Compute vectors */
    v1.x = p2->x - p1->x ;
    v1.y = p2->y - p1->y ;
    v1.z = p2->z - p1->z ;

    v2.x = p3->x - p1->x ;
    v2.y = p3->y - p1->y ;
    v2.z = p3->z - p1->z ;

    /* Compute cross product and normalize */
    cross_product( vc, &v1, &v2 ) ;
    return( normalize_vector( vc, vc ) ) ;
}



/**************************************************************************
*
*    center_point()
  *
  *          P = (P1 + P2 + P3) / 3
  *
  ***************************************************************************/

void center_point(Vertex *p1, Vertex *p2, Vertex *p3, Vertex *pc)
{
    /* Compute mid point of the element */

    pc->x = (p1->x + p2->x + p3->x) * (float)(1.0/3.0) ;
    pc->y = (p1->y + p2->y + p3->y) * (float)(1.0/3.0) ;
    pc->z = (p1->z + p2->z + p3->z) * (float)(1.0/3.0) ;
}



/**************************************************************************
*
*    four_center_points()
  *
  *          P = (P1 + P2 + P3) / 3
  *
  ***************************************************************************/

void four_center_points(Vertex *p1, Vertex *p2, Vertex *p3, Vertex *pc, Vertex *pc1, Vertex *pc2, Vertex *pc3)
{
    /* Compute mid point of the element */
    pc->x  = (p1->x + p2->x + p3->x) * (float)(1.0/3.0) ;
    pc->y  = (p1->y + p2->y + p3->y) * (float)(1.0/3.0) ;
    pc->z  = (p1->z + p2->z + p3->z) * (float)(1.0/3.0) ;

    pc1->x = (p1->x * 4 + p2->x + p3->x) * (float)(1.0/6.0) ;
    pc1->y = (p1->y * 4 + p2->y + p3->y) * (float)(1.0/6.0) ;
    pc1->z = (p1->z * 4 + p2->z + p3->z) * (float)(1.0/6.0) ;

    pc2->x = (p1->x + p2->x * 4 + p3->x) * (float)(1.0/6.0) ;
    pc2->y = (p1->y + p2->y * 4 + p3->y) * (float)(1.0/6.0) ;
    pc2->z = (p1->z + p2->z * 4 + p3->z) * (float)(1.0/6.0) ;

    pc3->x = (p1->x + p2->x + p3->x * 4) * (float)(1.0/6.0) ;
    pc3->y = (p1->y + p2->y + p3->y * 4) * (float)(1.0/6.0) ;
    pc3->z = (p1->z + p2->z + p3->z * 4) * (float)(1.0/6.0) ;
}


/***************************************************************************
*
*    print_point()
  *
  *    Print point information.
  *
  ****************************************************************************/

void print_point(Vertex *point)
{
    printf( "\tP(%.2f, %.2f, %.2f)\n", point->x, point->y, point->z ) ;
}




/***************************************************************************
****************************************************************************
*
*    Methods for Rgb object
*
****************************************************************************
****************************************************************************
****************************************************************************
*
*    print_rgb()
  *
  *    Print RGB information.
  *
  ****************************************************************************/

void print_rgb(Rgb *rgb)
{
    printf( "\tRGB(%.2f, %.2f, %.2f)\n", rgb->r, rgb->g, rgb->b ) ;
}




/***************************************************************************
****************************************************************************
*
*    Methods for ElementVertex
*
****************************************************************************
****************************************************************************/
/***************************************************************************
*
*    create_elemvertex
*
*    Given Vertex, create and return a new ElemVertex object.
*
****************************************************************************/

ElemVertex *create_elemvertex(Vertex *p, long process_id)
{
    ElemVertex *ev_new ;

    ev_new = get_elemvertex(process_id) ;
    ev_new->p = *p ;

    return( ev_new ) ;
}


/***************************************************************************
*
*    get_elemvertex
*
*    Returns an ElementVertex object
*
****************************************************************************/



ElemVertex *get_elemvertex(long process_id)
{
    ElemVertex *ev ;

    if( sobj_struct[process_id].n_local_free_elemvertex == 0 )
        {
            LOCK(global->free_elemvertex_lock);
            if ( MAX_ELEMVERTICES - global->free_elemvertex
                < N_ELEMVERTEX_ALLOCATE )
                {
                    fprintf( stderr, "Fatal:Ran out of ElemVertex buffer\n" ) ;
                    UNLOCK(global->free_elemvertex_lock);
                    exit(1) ;
                }
            sobj_struct[process_id].n_local_free_elemvertex = N_ELEMVERTEX_ALLOCATE ;
            sobj_struct[process_id].local_free_elemvertex
                = &global->elemvertex_buf[ global->free_elemvertex ] ;
            global->free_elemvertex += N_ELEMVERTEX_ALLOCATE ;
            UNLOCK(global->free_elemvertex_lock);
        }

    ev = sobj_struct[process_id].local_free_elemvertex++ ;
    sobj_struct[process_id].n_local_free_elemvertex-- ;


    /* Initialize contents */
    ev->col.r  = 0.0 ;
    ev->col.g  = 0.0 ;
    ev->col.b  = 0.0 ;
    ev->weight = 0.0 ;

    return( ev ) ;
}


/***************************************************************************
*
*    init_elemvertex()
  *
  *    Initialize ElemVertex buffer.
  *    This routine must be called in single process state.
  *
  ****************************************************************************/


void init_elemvertex(long process_id)
{
    long ev_cnt ;

    /* Initialize global free list */
    LOCKINIT(global->free_elemvertex_lock);
    global->free_elemvertex = 0 ;

    /* Allocate locks */
    for( ev_cnt = 0 ; ev_cnt < MAX_ELEMVERTICES ; ev_cnt++ )
        global->elemvertex_buf[ ev_cnt ].ev_lock
            = get_sharedlock( SHARED_LOCK_SEGANY, process_id ) ;

    /* Initialize local free list */
    sobj_struct[process_id].n_local_free_elemvertex    = 0 ;
    sobj_struct[process_id].local_free_elemvertex      = 0 ;
}



/***************************************************************************
****************************************************************************
*
*    Methods for Edge
*
****************************************************************************
****************************************************************************/


/***************************************************************************
 *
 *    foreach_leaf_edge()
 *
 *    For each leaf edges of the binary edge tree, apply the specified
 *    function. Edges are traversed from A to B (i.e., from Pa of the root
 *    to the Pb of the root) if 'reverse' is 0. Otherwise, it is traversed
 *    from B to A.
 *
 ****************************************************************************/

void foreach_leaf_edge(Edge *edge, long reverse, void (*func)(), long arg1, long arg2, long process_id)
{
    Edge *first, *second ;

    if( edge == 0 )
        return ;

    if( (edge->ea == 0) && (edge->eb == 0) )
        func( edge, reverse, arg1, arg2, process_id ) ;
    else
        {
            if( reverse )
                {
                    first = edge->eb ;
                    second = edge->ea ;
                }
            else
                {
                    first = edge->ea ;
                    second = edge->eb ;
                }
            if( first )
                foreach_leaf_edge( first, reverse, func, arg1, arg2, process_id ) ;
            if( second )
                foreach_leaf_edge( second, reverse, func, arg1, arg2, process_id ) ;
        }
}


/***************************************************************************
*
*    create_edge()
  *
  *    Given two ElemVertices V1 and V2, create a new edge (V1,V2)
  *
  ****************************************************************************/

Edge *create_edge(ElemVertex *v1, ElemVertex *v2, long process_id)
{
    Edge *enew ;

    enew = get_edge(process_id) ;
    enew->pa = v1 ;
    enew->pb = v2 ;
    return( enew ) ;
}


/***************************************************************************
*
*    subdivide_edge()
  *
  *    Create child edges. If they already exist, do nothing.
  *
  ****************************************************************************/

void subdivide_edge(Edge *e, float a_ratio, long process_id)
{
    Edge *enew, *e_am ;
    ElemVertex *ev_middle ;
    float b_ratio ;

    /* Lock the element before checking the value */
    LOCK(e->edge_lock->lock);

    /* Check if the element already has children */
    if( ! _LEAF_EDGE(e) )
        {
            UNLOCK(e->edge_lock->lock);
            return ;
        }

    /* Create the subdivision point */
    b_ratio = (float)1.0 - a_ratio ;
    ev_middle = get_elemvertex(process_id) ;
    ev_middle->p.x = a_ratio * e->pa->p.x + b_ratio * e->pb->p.x ;
    ev_middle->p.y = a_ratio * e->pa->p.y + b_ratio * e->pb->p.y ;
    ev_middle->p.z = a_ratio * e->pa->p.z + b_ratio * e->pb->p.z ;

    /* (1) Create edge(A-middle) */
    enew = get_edge(process_id) ;
    e_am = enew ;
    enew->pa = e->pa ;
    enew->pb = ev_middle ;

    /* (2) Create edge(middle-B) */
    enew = get_edge(process_id) ;
    enew->pa = ev_middle ;
    enew->pb = e->pb ;
    e->eb = enew ;

    /* Finally, set e->ea */
    e->ea = e_am ;

    /* Unlock the element */
    UNLOCK(e->edge_lock->lock);
}


/***************************************************************************
*
*    get_edge
*
*    Returns an Edge object
*
****************************************************************************/



Edge *get_edge(long process_id)
{
    Edge *edge ;

    if( sobj_struct[process_id].n_local_free_edge == 0 )
        {
            LOCK(global->free_edge_lock);
            if ( MAX_EDGES - global->free_edge < N_EDGE_ALLOCATE )
                {
                    fprintf( stderr, "Fatal:Ran out of Edge buffer\n" ) ;
                    UNLOCK(global->free_edge_lock);
                    exit(1) ;
                }
            sobj_struct[process_id].n_local_free_edge = N_EDGE_ALLOCATE ;
            sobj_struct[process_id].local_free_edge
                = &global->edge_buf[ global->free_edge ] ;
            global->free_edge += N_EDGE_ALLOCATE ;
            UNLOCK(global->free_edge_lock);
        }

    edge = sobj_struct[process_id].local_free_edge++ ;
    sobj_struct[process_id].n_local_free_edge-- ;


    /* Initialize contents */
    edge->pa = 0 ;
    edge->pb = 0 ;
    edge->ea = 0 ;
    edge->eb = 0 ;

    return( edge ) ;
}


/***************************************************************************
*
*    init_edge()
  *
  *    Initialize Edge buffer.
  *    This routine must be called in single process state.
  *
  ****************************************************************************/


void init_edge(long process_id)
{
    long edge_cnt ;

    /* Initialize global free list */
    LOCKINIT(global->free_edge_lock);
    global->free_edge = 0 ;

    /* Allocate locks */
    for( edge_cnt = 0 ; edge_cnt < MAX_EDGES ; edge_cnt++ )
        global->edge_buf[ edge_cnt ].edge_lock
            = get_sharedlock( SHARED_LOCK_SEG0, process_id ) ;

    /* Initialize local free list */
    sobj_struct[process_id].n_local_free_edge    = 0 ;
    sobj_struct[process_id].local_free_edge      = 0 ;
}



/***************************************************************************
****************************************************************************
*
*    Methods for Shared_Lock
*
*    Some machines provide a limited number of lock variables due to hardware
*    constraints etc. This package controls the sharing of this limited number
*    of locks among objects.
*
****************************************************************************
****************************************************************************/
/***************************************************************************
*
*    init_sharedlock()
  *
  *    Initialize shared lock.
  *
  ****************************************************************************/


void init_sharedlock(long process_id)
{
    long i ;

    for( i = 0 ; i < MAX_SHARED_LOCK ; i++ )
        {
            LOCKINIT(global->sh_lock[i].lock);
        }

    sobj_struct[process_id].lock_alloc_counter = 0 ;
}


/***************************************************************************
*
*    get_sharedlock()
  *
  *    Return a shared lock variable. If SHARED_LOCK_SEG[01] is specified,
  *    the lock is selected from the specified segment. If SHARED_LOCK_SEGANY
  *    is specified, the lock is picked up from arbitrary segment.
  *
  ****************************************************************************/

Shared_Lock *get_sharedlock(long segment, long process_id)
{
    Shared_Lock *pshl ;
    long effective_lock_ctr ;

    /* Compute effective lock allocation counter value */
    switch( segment )
        {
        case SHARED_LOCK_SEG0:
            effective_lock_ctr = sobj_struct[process_id].lock_alloc_counter % SHARED_LOCK_SEG_SIZE ;
            break ;
        case SHARED_LOCK_SEG1:
            effective_lock_ctr = sobj_struct[process_id].lock_alloc_counter % SHARED_LOCK_SEG_SIZE
                + SHARED_LOCK_SEG_SIZE ;
            break ;
        default:
            effective_lock_ctr = sobj_struct[process_id].lock_alloc_counter ;
        }


    /* Get pointer to the lock */
    pshl = &global->sh_lock[ effective_lock_ctr ] ;

    /* Update the lock counter */
    sobj_struct[process_id].lock_alloc_counter++ ;
    if( sobj_struct[process_id].lock_alloc_counter >= MAX_SHARED_LOCK )
        sobj_struct[process_id].lock_alloc_counter = 0 ;

    return( pshl ) ;
}

