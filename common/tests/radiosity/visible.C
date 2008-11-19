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
 *	Visibility testing
 *
 *
 ***************************************************************/

#include	<stdio.h>
#include        <math.h>

EXTERN_ENV;

include(radiosity.h)

#define VIS_RANGE_MARGIN (0.01)

#define		VISI_RAYS_MAX   (16)
#define         VISI_POOL_NO    (16)

#define FABS(x)  (((x) < 0)?-(x):(x))


float rand_ray1[VISI_RAYS_MAX][2], rand_ray2[VISI_RAYS_MAX][2] ;

struct v_struct {
    char pad1[PAGE_SIZE];	 	/* padding to avoid false-sharing
                                   and allow page-placement */
    Patch *v_src_patch, *v_dest_patch ;
    Vertex v_src_p1,   v_dest_p1 ;
    Vertex v_src_v12,  v_src_v13 ;
    Vertex v_dest_v12, v_dest_v13 ;

    long bsp_nodes_visited, total_bsp_nodes_visited ;
    Ray ray_pool[VISI_POOL_NO];
    Vertex point_pool[VISI_POOL_NO];
    long pool_dst_hits;	/* Number of rays that hit the destination  */
    Patch *patch_cache[PATCH_CACHE_SIZE] ;
    char pad2[PAGE_SIZE];	 	/* padding to avoid false-sharing
                                   and allow page-placement */
} vis_struct[MAX_PROCESSORS];


/*************************************************************
 *
 *	void init_visibility_module()
 *
 *       initialize the random test rays array.
 *
 *       Test ray parameters are precomputed as follows.
 *       (1) The triangles are divided into 16 small triangles.
 *       (2) Each small triangle shoots a ray to a small triangle on the
 *           destination. If N-rays are requested by get_test_ray(),
 *           N small triangles are chosen on the source and the destination
 *           and a ray is shot between N pairs of the small triangles.
 *       (3) Current scheme selects pairs of the small triangles in a static
 *           manner. The pairs are chosen such that rays are equally
 *           distributed.
 *
 *************************************************************/

void init_visibility_module(long process_id)
{
#define TTICK (1.0/12.0)

    /* Three corner triangles. P(i) -- Q(i) */
    rand_ray1[0][0] = TTICK ;      rand_ray1[0][1] = TTICK ;
    rand_ray1[1][0] = TTICK ;      rand_ray1[1][1] = TTICK * 10 ;
    rand_ray1[2][0] = TTICK * 10 ; rand_ray1[2][1] = TTICK ;
    rand_ray2[0][0] = TTICK ;      rand_ray2[0][1] = TTICK ;
    rand_ray2[1][0] = TTICK ;      rand_ray2[1][1] = TTICK * 10 ;
    rand_ray2[2][0] = TTICK * 10 ; rand_ray2[2][1] = TTICK ;

    /* Three triangles adjacent to the corners. RotLeft P(i)--> Q(i+1) */
    rand_ray1[3][0] = TTICK * 2 ;  rand_ray1[3][1] = TTICK * 2 ;
    rand_ray1[4][0] = TTICK * 2 ;  rand_ray1[4][1] = TTICK * 8 ;
    rand_ray1[5][0] = TTICK * 8 ;  rand_ray1[5][1] = TTICK * 2 ;
    rand_ray2[4][0] = TTICK * 2 ;  rand_ray2[4][1] = TTICK * 2 ;
    rand_ray2[5][0] = TTICK * 2 ;  rand_ray2[5][1] = TTICK * 8 ;
    rand_ray2[3][0] = TTICK * 8 ;  rand_ray2[3][1] = TTICK * 2 ;

    /* Three triangles adjacent to the center. RotRight P(i)--> Q(i-1) */
    rand_ray1[6][0] = TTICK * 2 ;  rand_ray1[6][1] = TTICK * 5 ;
    rand_ray1[7][0] = TTICK * 5 ;  rand_ray1[7][1] = TTICK * 5 ;
    rand_ray1[8][0] = TTICK * 5 ;  rand_ray1[8][1] = TTICK * 2 ;
    rand_ray2[8][0] = TTICK * 2 ;  rand_ray2[8][1] = TTICK * 5 ;
    rand_ray2[6][0] = TTICK * 5 ;  rand_ray2[6][1] = TTICK * 5 ;
    rand_ray2[7][0] = TTICK * 5 ;  rand_ray2[7][1] = TTICK * 2 ;

    /* Center triangle. Straight P(i) --> Q(i) */
    rand_ray1[9][0] = TTICK * 4 ;  rand_ray1[9][1] = TTICK * 4 ;
    rand_ray2[9][0] = TTICK * 4 ;  rand_ray2[9][1] = TTICK * 4 ;

    /* Along the pelimeter. RotRight P(i)--> Q(i-1) */
    rand_ray1[10][0] = TTICK * 1 ;  rand_ray1[10][1] = TTICK * 7 ;
    rand_ray1[11][0] = TTICK * 5 ;  rand_ray1[11][1] = TTICK * 4 ;
    rand_ray1[12][0] = TTICK * 4 ;  rand_ray1[12][1] = TTICK * 1 ;
    rand_ray2[12][0] = TTICK * 1 ;  rand_ray2[12][1] = TTICK * 7 ;
    rand_ray2[10][0] = TTICK * 5 ;  rand_ray2[10][1] = TTICK * 4 ;
    rand_ray2[11][0] = TTICK * 4 ;  rand_ray2[11][1] = TTICK * 1 ;

    /* Along the pelimeter. RotLeft P(i)--> Q(i+1) */
    rand_ray1[13][0] = TTICK * 1 ;  rand_ray1[13][1] = TTICK * 4 ;
    rand_ray1[14][0] = TTICK * 4 ;  rand_ray1[14][1] = TTICK * 7 ;
    rand_ray1[15][0] = TTICK * 7 ;  rand_ray1[15][1] = TTICK * 1 ;
    rand_ray2[14][0] = TTICK * 1 ;  rand_ray2[14][1] = TTICK * 4 ;
    rand_ray2[15][0] = TTICK * 4 ;  rand_ray2[15][1] = TTICK * 7 ;
    rand_ray2[13][0] = TTICK * 7 ;  rand_ray2[13][1] = TTICK * 1 ;

    /* Initialize patch cache */
    init_patch_cache(process_id) ;
}


/*************************************************************
 *
 *	void get_test_ray(): get a randomized ray vector and start point.
 *
 *	Place: main loop.
 *
 *	Storage: must keep in the invidiual processor.
 *
 *************************************************************/

void get_test_rays(Vertex *p_src, Ray *v, long no, long process_id)
{
    long g_index, i ;
    Vertex p_dst ;
    float fv1, fv2 ;

    if( no > VISI_RAYS_MAX )
        no = VISI_RAYS_MAX ;

    for (i = 0, g_index = 0 ; i < no; i++, g_index++) {

        fv1 = rand_ray1[ g_index ][0] ;
        fv2 = rand_ray1[ g_index ][1] ;
        p_src->x = vis_struct[process_id].v_src_p1.x + vis_struct[process_id].v_src_v12.x * fv1 + vis_struct[process_id].v_src_v13.x * fv2 ;
        p_src->y = vis_struct[process_id].v_src_p1.y + vis_struct[process_id].v_src_v12.y * fv1 + vis_struct[process_id].v_src_v13.y * fv2 ;
        p_src->z = vis_struct[process_id].v_src_p1.z + vis_struct[process_id].v_src_v12.z * fv1 + vis_struct[process_id].v_src_v13.z * fv2 ;

        fv1 = rand_ray2[ g_index ][0] ;
        fv2 = rand_ray2[ g_index ][1] ;
        p_dst.x = vis_struct[process_id].v_dest_p1.x + vis_struct[process_id].v_dest_v12.x * fv1 + vis_struct[process_id].v_dest_v13.x * fv2 ;
        p_dst.y = vis_struct[process_id].v_dest_p1.y + vis_struct[process_id].v_dest_v12.y * fv1 + vis_struct[process_id].v_dest_v13.y * fv2 ;
        p_dst.z = vis_struct[process_id].v_dest_p1.z + vis_struct[process_id].v_dest_v12.z * fv1 + vis_struct[process_id].v_dest_v13.z * fv2 ;

        v->x = p_dst.x - p_src->x;
        v->y = p_dst.y - p_src->y;
        v->z = p_dst.z - p_src->z;

        p_src++;
        v++;
    }
}


/************************************************************************
 *
 *	long v_intersect(): check if the ray intersects with the polygon.
 *
 *************************************************************************/


long v_intersect(Patch *patch, Vertex *p, Ray *ray, float t)
{
    float px, py, pz;
    float nx, ny, nz;
    float rx, ry, rz;
    float x, y, x1, x2, x3, y1, y2, y3;
    float a, b, c;
    long nc, sh, nsh;

    nx = patch->plane_equ.n.x;
    ny = patch->plane_equ.n.y;
    nz = patch->plane_equ.n.z;

    rx = ray->x;
    ry = ray->y;
    rz = ray->z;

    px = p->x;
    py = p->y;
    pz = p->z;


    a = FABS(nx); b = FABS(ny); c = FABS(nz);

    if (a > b)
        if (a > c) {
            x  = py + t * ry; y = pz + t * rz;
            x1 = patch->p1.y; y1 = patch->p1.z;
            x2 = patch->p2.y; y2 = patch->p2.z;
            x3 = patch->p3.y; y3 = patch->p3.z;
        } else {
            x  = px + t * rx; y = py + t * ry;
            x1 = patch->p1.x; y1 = patch->p1.y;
            x2 = patch->p2.x; y2 = patch->p2.y;
            x3 = patch->p3.x; y3 = patch->p3.y;
        }
    else if (b > c) {
        x  = px + t * rx; y = pz + t * rz;
        x1 = patch->p1.x; y1 = patch->p1.z;
        x2 = patch->p2.x; y2 = patch->p2.z;
        x3 = patch->p3.x; y3 = patch->p3.z;
    } else {
        x  = px + t * rx; y = py + t * ry;
        x1 = patch->p1.x; y1 = patch->p1.y;
        x2 = patch->p2.x; y2 = patch->p2.y;
        x3 = patch->p3.x; y3 = patch->p3.y;
    }


    x1 -= x; y1 -= y;
    x2 -= x; y2 -= y;
    x3 -= x; y3 -= y;

    nc = 0;

    if (y1 >= 0.0)
        sh = 1;
    else
        sh = -1;

    if (y2 >= 0.0)
        nsh = 1;
    else
        nsh = -1;

    if (sh != nsh) {
        if ((x1 >= 0.0) && (x2 >= 0.0))
            nc++;
        else if ((x1 >= 0.0) || (x2 >= 0.0))
            if ((x1 - y1 * (x2 - x1) / (y2 - y1)) > 0.0)
                nc++;
        sh = nsh;
    }

    if (y3 >= 0.0)
        nsh = 1;
    else
        nsh = -1;

    if (sh != nsh) {
        if ((x2 >= 0.0) && (x3 >= 0.0))
            nc++;
        else if ((x2 >= 0.0) || (x3 >= 0.0))
            if ((x2 - y2 * (x3 - x2) / (y3 - y2)) > 0.0)
                nc++;
        sh = nsh;
    }

    if (y1 >= 0.0)
        nsh = 1;
    else
        nsh = -1;

    if (sh != nsh) {
        if ((x3 >= 0.0) && (x1 >= 0.0))
            nc++;
        else if ((x3 >= 0.0) || (x1 >= 0.0))
            if ((x1 - y1 * (x1 - x3) / (y1 - y3)) > 0.0)
                nc++;
        sh = nsh;
    }

    if ((nc % 2) == 0)
        return(0);
    else {
        return(1);
    }

}




#define DEST_FOUND (1)
#define DEST_NOT_FOUND (0)

#define ON_THE_PLANE          (0)
#define POSITIVE_SUBTREE_ONLY (1)
#define NEGATIVE_SUBTREE_ONLY (2)
#define POSITIVE_SIDE_FIRST   (3)
#define NEGATIVE_SIDE_FIRST   (4)



/****************************************************************************
 *
 *    traverse_bsp()
 *    traverse_subtree()
 *
 *    Traverse the BSP tree. The traversal is in-order. Since the traversal
 *    of the BSP tree begins at the middle of the BSP tree (i.e., the source
 *    node), the traversal is performed as follows.
 *      1) Traverse the positive subtee of the start (source) node.
 *      2) For each ancestor node of the source node, visit it (immediate
 *         parent first).
 *         2.1) Test if the node intercepts the ray.
 *         2.2) Traverse the subtree of the node (i.e., the subtree that the
 *              source node does not belong to.
 *
 *    traverse_bsp() takes care of the traversal of ancestor nodes. Ordinary
 *    traversal of a subtree is done by traverse_subtree().
 *
 *****************************************************************************/


long traverse_bsp(Patch *src_node, Vertex *p, Ray *ray, float r_min, float r_max, long process_id)
{
    float t = 0.0 ;
    Patch *parent, *visited_child ;
    long advice ;


    /* (1) Check patch cache */
    if( check_patch_cache( p, ray, r_min, r_max, process_id ) )
        return( 1 ) ;

    /* (2) Check S+(src_node) */
    if( traverse_subtree( src_node->bsp_positive, p, ray, r_min, r_max, process_id ) )
        return( 1 ) ;

    /* (3) Continue in-order traversal till root is encountered */
    for( parent = src_node->bsp_parent, visited_child = src_node ;
        parent ;
        visited_child = parent, parent = parent->bsp_parent )
        {
            /* Check intersection at this node */
            advice = intersection_type( parent, p, ray, &t, r_min, r_max ) ;
            if( (advice != POSITIVE_SUBTREE_ONLY) && (advice != NEGATIVE_SUBTREE_ONLY) )
                {
                    if( test_intersection( parent, p, ray, t, process_id ) )
                        return( 1 ) ;

                    r_min = t - VIS_RANGE_MARGIN ;
                }

            /* Traverse unvisited subtree of the node */
            if(   (parent->bsp_positive == visited_child) && (advice != POSITIVE_SUBTREE_ONLY) )
                {
                    if( traverse_subtree( parent->bsp_negative, p, ray, r_min, r_max, process_id ) )
                        return( 1 ) ;
                }
            else if( (parent->bsp_positive != visited_child) && (advice != NEGATIVE_SUBTREE_ONLY) )
                {
                    if( traverse_subtree( parent->bsp_positive, p, ray, r_min, r_max, process_id ) )
                        return( 1 ) ;
                }
        }

    return( 0 ) ;
}




long traverse_subtree(Patch *node, Vertex *p, Ray *ray, float r_min, float r_max, long process_id)
  /*
   *    To minimize the length of the traversal of the BSP tree, a pruning
   *    algorithm is incorporated.
   *    One possibility (not used in this version) is to prune one of the
   *	 subtrees if the node in question intersects the ray outside of the
   *	 range defined by the source and the destination patches.
   *    Another possibility (used here) is more aggressive pruning. Like the above
   *    method, the intersection point is checked against the range to prune the
   *    subtree. But instead of using a constant source-destination range,
   *    the range itself is recursively subdivided so that the minimum range is
   *    applied the possibility of pruning maximized.
   */
{
    float t ;
    long advice ;


    if( node == 0 )
        return( 0 ) ;

    advice = intersection_type( node, p, ray, &t, r_min, r_max ) ;
    if( advice == POSITIVE_SIDE_FIRST )
        {
            /* The ray is approaching from the positive side of the patch */
            if( traverse_subtree( node->bsp_positive, p, ray,
                                 r_min, t + VIS_RANGE_MARGIN, process_id ) )
                return( 1 ) ;

            if( test_intersection( node, p, ray, t, process_id ) )
                return( 1 ) ;
            return( traverse_subtree( node->bsp_negative, p, ray,
                                     t - VIS_RANGE_MARGIN, r_max, process_id ) ) ;
        }
    else if( advice == NEGATIVE_SIDE_FIRST )
        {
            /* The ray is approaching from the negative side of the patch */
            if( traverse_subtree( node->bsp_negative, p, ray,
                                 r_min, t + VIS_RANGE_MARGIN, process_id ) )
                return( 1 ) ;
            if( test_intersection( node, p, ray, t, process_id ) )
                return( 1 ) ;

            return( traverse_subtree( node->bsp_positive, p, ray,
                                     t - VIS_RANGE_MARGIN, r_max, process_id ) ) ;
        }

    else if( advice == POSITIVE_SUBTREE_ONLY )
        return( traverse_subtree( node->bsp_positive, p, ray,
                                 r_min, r_max, process_id ) ) ;
    else if( advice == NEGATIVE_SUBTREE_ONLY )
        return( traverse_subtree( node->bsp_negative, p, ray,
                                 r_min, r_max, process_id ) ) ;
    else
        /* On the plane */
        return( 1 ) ;
}



/**************************************************************************
 *
 *	intersection_type()
 *
 *       Compute intersection coordinate as the barycentric coordinate
 *       w.r.t the ray vector. This value is returned to the caller through
 *       the variable passed by reference.
 *       intersection_type() also classifies the intersection type and
 *       returns the type as the "traversal advice" code.
 *       Possible types are:
 *       1) the patch and the ray are parallel
 *          --> POSITIVE_SUBTREE_ONLY, NEGATIVE_SUBTREE_ONLY, or ON_THE_PLANE
 *       2) intersects the ray outside of the specified range
 *          --> POSITIVE_SUBTREE_ONLY, NEGATIVE_SUBTREE_ONLY
 *       3) intersects within the range
 *          --> POSITIVE_SIDE_FIRST, NEGATIVE_SIDE_FIRST
 *
 ***************************************************************************/

long intersection_type(Patch  *patch, Vertex *p, Ray *ray, float *tval, float range_min, float range_max)
{
    float r_dot_n ;
    float dist ;
    float t ;
    float nx, ny, nz ;

#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_COSTBASED
    vis_struct[process_id].bsp_nodes_visited++ ;
#endif

    /* (R.N) */
    nx = patch->plane_equ.n.x ;
    ny = patch->plane_equ.n.y ;
    nz = patch->plane_equ.n.z ;

    r_dot_n = nx * ray->x + ny * ray->y + nz * ray->z ;
    dist = patch->plane_equ.c  +  p->x * nx  +  p->y * ny  +  p->z * nz ;

    if( (-(float)F_ZERO < r_dot_n) && (r_dot_n < (float)F_ZERO) )
        {
            if( dist > (float)F_COPLANAR )
                return( POSITIVE_SUBTREE_ONLY ) ;
            else if( dist < -F_COPLANAR )
                return( NEGATIVE_SUBTREE_ONLY ) ;

            return( ON_THE_PLANE ) ;
        }

    t = -dist / r_dot_n ;
    *tval = t ;

    if( t < range_min )
        {
            if( r_dot_n >= 0 )
                return( POSITIVE_SUBTREE_ONLY ) ;
            else
                return( NEGATIVE_SUBTREE_ONLY ) ;
        }
    else if ( t > range_max )
        {
            if( r_dot_n >= 0 )
                return( NEGATIVE_SUBTREE_ONLY ) ;
            else
                return( POSITIVE_SUBTREE_ONLY ) ;
        }
    else
        {
            if( r_dot_n >= 0 )
                return( NEGATIVE_SIDE_FIRST ) ;
            else
                return( POSITIVE_SIDE_FIRST ) ;
        }
}


/*************************************************************
 *
 *	test_intersection()
 *
 *************************************************************/

long test_intersection(Patch *patch, Vertex *p, Ray *ray, float tval, long process_id)
{
    /* Rays always hit the destination. Note that (R.Ndest) is already
       checked by visibility() */

    if( patch == vis_struct[process_id].v_dest_patch )
        {
            vis_struct[process_id].pool_dst_hits++ ;
            return( 1 ) ;
        }

    if( patch_tested( patch, process_id ) )
        return( 0 ) ;

    if( v_intersect( patch, p, ray, tval ) )
        {
            /* Store it in the patch-cache */
            update_patch_cache( patch, process_id ) ;
            return( 1 ) ;
        }

    return( 0 ) ;
}



/*************************************************************
 *
 *	patch_cache
 *
 *       update_patch_cache()
 *       check_patch_cache()
 *       init_patch_cache()
 *
 *    To exploit visibility coherency, a patch cache is used.
 *    Before traversing the BSP tree, the cache contents are tested to see
 *    if they intercept the ray in question. The size of the patch cache is
 *    defined by PATCH_CACHE_SIZE (in patch.H). Since the first two
 *    entries of the cache
 *    usually cover about 95% of the cache hits, increasing the cache size
 *    does not help much. Nevertheless, the program is written so that
 *    increasing cache size does not result in additional ray-intersection
 *    test.
 *
 *************************************************************/

void update_patch_cache(Patch *patch, long process_id)
{
    long i ;

    /* Shift current contents */
    for( i = PATCH_CACHE_SIZE-1 ; i > 0  ; i-- )
        vis_struct[process_id].patch_cache[i] = vis_struct[process_id].patch_cache[i-1] ;

    /* Store the new patch data */
    vis_struct[process_id].patch_cache[0] = patch ;
}



long check_patch_cache(Vertex *p, Ray *ray, float r_min, float r_max, long process_id)
{
    long i ;
    float t ;
    Patch *temp ;
    long advice ;

    for( i = 0 ; i < PATCH_CACHE_SIZE ; i++ )
        {
            if(   (vis_struct[process_id].patch_cache[i] == 0)
               || (vis_struct[process_id].patch_cache[i] == vis_struct[process_id].v_dest_patch)
               || (vis_struct[process_id].patch_cache[i] == vis_struct[process_id].v_src_patch) )
                continue ;

            advice = intersection_type( vis_struct[process_id].patch_cache[i],  p, ray, &t, r_min, r_max ) ;

            /* If no intersection, then skip */
            if(   (advice == POSITIVE_SUBTREE_ONLY)
               || (advice == NEGATIVE_SUBTREE_ONLY) )
                continue ;

            if(   (advice == ON_THE_PLANE) || v_intersect( vis_struct[process_id].patch_cache[i], p, ray, t ) )
                {
                    /* Change priority */
                    if( i > 0 )
                        {
                            temp = vis_struct[process_id].patch_cache[ i-1 ] ;
                            vis_struct[process_id].patch_cache[ i-1 ] = vis_struct[process_id].patch_cache[ i ] ;
                            vis_struct[process_id].patch_cache[ i ] = temp ;
                        }

                    return( 1 ) ;
                }
        }


    return( 0 ) ;
}



void init_patch_cache(long process_id)
{
    long i ;

    for( i = 0 ; i < PATCH_CACHE_SIZE ; i++ )
        vis_struct[process_id].patch_cache[ i ] = 0 ;
}


long patch_tested(Patch *p, long process_id)
{
    long i ;

    for( i = 0 ; i < PATCH_CACHE_SIZE ; i++ )
        {
            if( p == vis_struct[process_id].patch_cache[i] )
                return( 1 ) ;
        }

    return( 0 ) ;
}


/*************************************************************
 *
 *	float visibility(): checking if two patches are mutually invisible.
 *
 *************************************************************/


float visibility(Element *e1, Element *e2, long n_rays, long process_id)
{
    float range_max, range_min ;
    long i;
    Ray *r;
    long ray_reject ;

    vis_struct[process_id].v_src_patch  = e1->patch;
    vis_struct[process_id].v_dest_patch = e2->patch;

    vis_struct[process_id].v_src_p1 = e1->ev1->p ;
    vis_struct[process_id].v_src_v12.x = e1->ev2->p.x - vis_struct[process_id].v_src_p1.x ;
    vis_struct[process_id].v_src_v12.y = e1->ev2->p.y - vis_struct[process_id].v_src_p1.y ;
    vis_struct[process_id].v_src_v12.z = e1->ev2->p.z - vis_struct[process_id].v_src_p1.z ;
    vis_struct[process_id].v_src_v13.x = e1->ev3->p.x - vis_struct[process_id].v_src_p1.x ;
    vis_struct[process_id].v_src_v13.y = e1->ev3->p.y - vis_struct[process_id].v_src_p1.y ;
    vis_struct[process_id].v_src_v13.z = e1->ev3->p.z - vis_struct[process_id].v_src_p1.z ;

    vis_struct[process_id].v_dest_p1 = e2->ev1->p ;
    vis_struct[process_id].v_dest_v12.x = e2->ev2->p.x - vis_struct[process_id].v_dest_p1.x ;
    vis_struct[process_id].v_dest_v12.y = e2->ev2->p.y - vis_struct[process_id].v_dest_p1.y ;
    vis_struct[process_id].v_dest_v12.z = e2->ev2->p.z - vis_struct[process_id].v_dest_p1.z ;
    vis_struct[process_id].v_dest_v13.x = e2->ev3->p.x - vis_struct[process_id].v_dest_p1.x ;
    vis_struct[process_id].v_dest_v13.y = e2->ev3->p.y - vis_struct[process_id].v_dest_p1.y ;
    vis_struct[process_id].v_dest_v13.z = e2->ev3->p.z - vis_struct[process_id].v_dest_p1.z ;

    get_test_rays( vis_struct[process_id].point_pool, vis_struct[process_id].ray_pool, n_rays, process_id ) ;

    range_min = -VIS_RANGE_MARGIN ;
    range_max =  1.0 + VIS_RANGE_MARGIN ;

    vis_struct[process_id].pool_dst_hits = 0 ;
    ray_reject = 0 ;
    for ( i = 0 ; i < n_rays ; i++ )
        {
            r = &(vis_struct[process_id].ray_pool[i]);

            if (  (inner_product( (Vertex *)r, &(vis_struct[process_id].v_src_patch)->plane_equ.n ) <= 0.0 )
                ||(inner_product( (Vertex *)r, &(vis_struct[process_id].v_dest_patch)->plane_equ.n ) >= 0.0 ) )
                {
                    ray_reject++ ;
                    continue ;
                }

            traverse_bsp( vis_struct[process_id].v_src_patch, &vis_struct[process_id].point_pool[i], r, range_min, range_max, process_id ) ;
        }

    if (ray_reject == n_rays) {
        /* All rays have been trivially rejected. This can occur
           if no rays are shot between visible portion of the elements.
           Return partial visibility (1/No-of-rays). */

        /* Return partially visible result */
        vis_struct[process_id].pool_dst_hits = 1 ;
    }

    return( (float)vis_struct[process_id].pool_dst_hits / (float)n_rays ) ;
}



/*****************************************************************
 *
 *    compute_visibility_values()
 *
 *    Apply visibility() function to an interaction list.
 *
 ******************************************************************/

void compute_visibility_values(Element *elem, Interaction *inter, long n_inter, long process_id)
{
    for( ; n_inter > 0 ; inter = inter->next, n_inter-- )
        {
            if( inter->visibility != VISIBILITY_UNDEF )
                continue ;

            vis_struct[process_id].bsp_nodes_visited = 0 ;

            inter->visibility
                = visibility( elem, inter->destination,
                             N_VISIBILITY_TEST_RAYS, process_id ) ;

            vis_struct[process_id].total_bsp_nodes_visited += vis_struct[process_id].bsp_nodes_visited ;
        }
}


/*****************************************************************
 *
 *    visibility_task()
 *
 *    Compute visibility values and then call continuation when all
 *    the undefined visibility values have been computed.
 *
 ******************************************************************/

void visibility_task(Element *elem, Interaction *inter, long n_inter, void (*k)(), long process_id)
{
#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_COSTBASED
    Patch_Cost *pc ;
#endif
    long new_vis_undef_count ;

    /* Compute visibility */
    vis_struct[process_id].total_bsp_nodes_visited = 0 ;
    compute_visibility_values( elem, inter, n_inter, process_id ) ;

    /* Change visibility undef count */
    LOCK(elem->elem_lock->lock);
    elem->n_vis_undef_inter -= n_inter ;
    new_vis_undef_count = elem->n_vis_undef_inter ;
    UNLOCK(elem->elem_lock->lock);

#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_COSTBASED
    pc = &global->patch_cost[ elem->patch->seq_no ] ;
    LOCK(pc->cost_lock->lock);
    pc->n_bsp_node += vis_struct[process_id].total_bsp_nodes_visited ;
    UNLOCK(pc->cost_lock->lock);
#endif

    /* Call continuation if this is the last task finished. */
    if( new_vis_undef_count == 0 )
    {
       void (*_k)(Element *, long) = (void (*)(Element *, long))k;
        _k( elem, process_id ) ;
    }
}
