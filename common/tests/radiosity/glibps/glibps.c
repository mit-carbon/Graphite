/* -*-mode:c-*- */
/***************************************************************
*
*       Radiosity
*
*	Graphic driver for PostScript
*
*
***************************************************************/

#include <stdio.h>
#include <math.h>
#include "pslib.h"

#define SCREEN_WIDTH   (6.0*72)
#define SCREEN_HEIGHT  (4.8*72)
#define SCREEN_DEPTH   (65536)
#define ASPECT_RATIO  ((float)SCREEN_WIDTH/(float)SCREEN_HEIGHT)

#define PRE_CAT  (1)
#define POST_CAT (0)

#define DEFAULT_WINDOW_HEIGHT (2000.0)
#define DEFAULT_WINDOW_WIDTH  (DEFAULT_WINDOW_HEIGHT*ASPECT_RATIO)
#define DEFAULT_FRONT_PLANE_Z (2000.0)
#define DEFAULT_BACK_PLANE_Z  (-4000.0)
#define DEFAULT_PRP_Z         (10000.0)    /* Projection point Z coord. */

/**************************************************
*
*    Globals
*
***************************************************/

static Matrix  trans_mtx ;		/* WC -> DC */
static Vertex2  prp ;			/* Projection point */
static Vertex2  active_prp ;		/* Projection point in effect (WC) */
static float   view_rotx, view_roty ;	/* Viewing */
static float   view_zoom ;

static float   clip_right, clip_left ;	/* View volume (X) */
static float   clip_top, clip_bottom ;	/*             (Y) */
static float   clip_front, clip_back ;	/*             (Z) */


static FILE *ps_fd ;

static void setup_transformation(void);
static void init_transformation(void);
static void gset_unit_matrix(Matrix *mtx);
static void gconcatenate_matrix(long precat, Matrix *m1, Matrix *m2);
static void gscale_matrix(long precat, Matrix *m1, float sx, float sy, float sz);
static void gtranslate_matrix(long precat, Matrix *m1, float tx, float ty, float tz);
static void grotate_x_matrix(long precat, Matrix *m1, float rot);
static void grotate_y_matrix(long precat, Matrix *m1, float rot);
static void gtransform(Vertex2 *v1, Vertex2 *v2, Matrix *mtx);
static void ginverse_matrix(Matrix *m1, Matrix *m2);
static double det(Matrix *m);
static double cdet(Matrix *m, long r0, long r1, long r2, long c0, long c1, long c2);

/************************************************************************
*
*    ps_open()
*    ps_close()
*
*************************************************************************/



long ps_open(char *file)
{
      if( (ps_fd = fopen( file, "w" )) == 0 )
      {
	    perror( file ) ;
	    return( 0 ) ;
      }

      /* Print out preamble */
      fprintf( ps_fd, "%%!PS-Adobe-1.0\n" ) ;
      fprintf( ps_fd, "%%%%EndComments\n" ) ;
      fprintf( ps_fd, "%%%%Pages: 1\n" ) ;
      fprintf( ps_fd, "%%%%EndProlog\n" ) ;
      fprintf( ps_fd, "%%%%Page: 1 1\n" ) ;
      fprintf( ps_fd, "\n" ) ;

      /* Default line cap/join */
      fprintf( ps_fd, "1 setlinecap 1 setlinejoin\n" ) ;

      /* Initialize transformation */
      init_transformation() ;
      setup_transformation() ;
      return(0);
}



void ps_close()
{
      if( ps_fd == 0 )
	    return ;


      fprintf( ps_fd, "showpage\n" ) ;
      fprintf( ps_fd, "%%%%Trailer\n" ) ;
      fclose( ps_fd ) ;

      ps_fd = 0 ;
}



/**************************************************
*
*    ps_linewidth()
*
***************************************************/

void ps_linewidth(float w)
{
      if( ps_fd == 0 )
	    return ;

      fprintf( ps_fd, "%f setlinewidth\n", w ) ;
}



/**************************************************
*
*    ps_line()
*
***************************************************/

void ps_line(Vertex *p1, Vertex *p2)
{
      Vertex2  v1, v2 ;
      float x1, y1, x2, y2 ;

      if( ps_fd == 0 )
	    return ;

      v1.v[0] = p1->x ;  v1.v[1] = p1->y ;  v1.v[2] = p1->z ;  v1.v[3] = 1.0 ;
      v2.v[0] = p2->x ;  v2.v[1] = p2->y ;  v2.v[2] = p2->z ;  v2.v[3] = 1.0 ;
      gtransform( &v1, &v1, &trans_mtx ) ;
      gtransform( &v2, &v2, &trans_mtx ) ;
      x1 = v1.v[0] / v1.v[3] ;
      y1 = v1.v[1] / v1.v[3] ;
      x2 = v2.v[0] / v2.v[3] ;
      y2 = v2.v[1] / v2.v[3] ;


      fprintf( ps_fd, "newpath\n%f %f moveto\n", x1, y1 ) ;
      fprintf( ps_fd, "%f %f lineto\nstroke\n", x2, y2 ) ;
}


/**************************************************
*
*    ps_polygonedge()
*
***************************************************/

void ps_polygonedge(long n, Vertex *p_list)
{
      float dcx, dcy ;
      Vertex2 v ;
      long i ;

      if( ps_fd == 0 )
	    return ;

      /* Transform */
      v.v[0] = p_list[0].x ;
      v.v[1] = p_list[0].y ;
      v.v[2] = p_list[0].z ;
      v.v[3] = 1.0 ;
      gtransform( &v, &v, &trans_mtx ) ;
      dcx = v.v[0] / v.v[3] ;
      dcy = v.v[1] / v.v[3] ;
      fprintf( ps_fd, "newpath\n%f %f moveto\n", dcx, dcy ) ;

      for( i = 1 ; i < n ; i++ )
      {
	    /* Transform */
	    v.v[0] = p_list[i].x ;
	    v.v[1] = p_list[i].y ;
	    v.v[2] = p_list[i].z ;
	    v.v[3] = 1.0 ;
	    gtransform( &v, &v, &trans_mtx ) ;
	    dcx = v.v[0] / v.v[3] ;
	    dcy = v.v[1] / v.v[3] ;

	    fprintf( ps_fd, "%f %f lineto\n", dcx, dcy ) ;
      }

      fprintf( ps_fd, "closepath stroke\n" ) ;
}


/**************************************************
*
*    ps_polygon()
*
***************************************************/

void ps_polygon(long n, Vertex *p_list)
{
      float dcx, dcy ;
      Vertex2 v ;
      long i ;

      if( ps_fd == 0 )
	    return ;

      /* Transform */
      v.v[0] = p_list[0].x ;
      v.v[1] = p_list[0].y ;
      v.v[2] = p_list[0].z ;
      v.v[3] = 1.0 ;
      gtransform( &v, &v, &trans_mtx ) ;
      dcx = v.v[0] / v.v[3] ;
      dcy = v.v[1] / v.v[3] ;
      fprintf( ps_fd, "newpath\n%f %f moveto\n", dcx, dcy ) ;

      for( i = 1 ; i < n ; i++ )
      {
	    /* Transform */
	    v.v[0] = p_list[i].x ;
	    v.v[1] = p_list[i].y ;
	    v.v[2] = p_list[i].z ;
	    v.v[3] = 1.0 ;
	    gtransform( &v, &v, &trans_mtx ) ;
	    dcx = v.v[0] / v.v[3] ;
	    dcy = v.v[1] / v.v[3] ;

	    fprintf( ps_fd, "%f %f lineto\n", dcx, dcy ) ;
      }

      fprintf( ps_fd, "closepath fill\n" ) ;
}


/**************************************************
*
*    ps_spolygon()
*
***************************************************/

void ps_spolygon(long n, Vertex *p_list, Rgb *c_list)
{
      float dcx, dcy ;
      Vertex2 v ;
      long i ;
      float gray_scale ;

      if( ps_fd == 0 )
	    return ;

      /* Transform */
      v.v[0] = p_list[0].x ;
      v.v[1] = p_list[0].y ;
      v.v[2] = p_list[0].z ;
      v.v[3] = 1.0 ;
      gtransform( &v, &v, &trans_mtx ) ;
      dcx = v.v[0] / v.v[3] ;
      dcy = v.v[1] / v.v[3] ;
      fprintf( ps_fd, "newpath\n%f %f moveto\n", dcx, dcy ) ;

      for( i = 1 ; i < n ; i++ )
      {
	    /* Transform */
	    v.v[0] = p_list[i].x ;
	    v.v[1] = p_list[i].y ;
	    v.v[2] = p_list[i].z ;
	    v.v[3] = 1.0 ;
	    gtransform( &v, &v, &trans_mtx ) ;
	    dcx = v.v[0] / v.v[3] ;
	    dcy = v.v[1] / v.v[3] ;

	    fprintf( ps_fd, "%f %f lineto\n", dcx, dcy ) ;
      }

      gray_scale = c_list[0].g ;
      if( gray_scale > 1.0 )
	    gray_scale = 1.0 ;
      else if( gray_scale < 0.0 )
	    gray_scale = 0.0 ;

      fprintf( ps_fd, "closepath %f setgray fill\n", gray_scale ) ;
}


/**************************************************
*
*    ps_clear()
*
***************************************************/

void ps_clear()
{
}



/**************************************************
*
*    ps_setup_view()
*
***************************************************/

void ps_setup_view(float rot_x, float rot_y, float dist, float zoom)
{
      prp.v[0] = 0.0 ;
      prp.v[1] = 0.0 ;
      prp.v[2] = (float)dist ;
      prp.v[3] = 0.0 ;
      view_rotx = rot_x ;
      view_roty = rot_y ;
      view_zoom = zoom  ;

      setup_transformation() ;
}



/**************************************************
*
*    setup_transformation()
*
***************************************************/

static void setup_transformation()
{
      float cf_z, cb_z ;
      Matrix pmat ;

      /* Set to unit matrix */
      gset_unit_matrix( &trans_mtx ) ;

      /* View orientation matrix */
      grotate_x_matrix( POST_CAT, &trans_mtx, view_rotx ) ;
      grotate_y_matrix( POST_CAT, &trans_mtx, view_roty ) ;

      /* Compute active (currently effective) projection point */
      ginverse_matrix( &pmat, &trans_mtx ) ;
      gtransform( &active_prp, &prp, &pmat ) ;

      /* Perspective projection */
      gset_unit_matrix( &pmat ) ;
      pmat.m[2][3] = - 1 / prp.v[2] ;
      gconcatenate_matrix( POST_CAT, &trans_mtx, &pmat ) ;

      cf_z = prp.v[2] * clip_front / ( prp.v[2] - clip_front ) ;
      cb_z = prp.v[2] * clip_back  / ( prp.v[2] - clip_back  ) ;

      /* Window-Viewport */
      gscale_matrix( POST_CAT, &trans_mtx,
		   (float)SCREEN_WIDTH  / (clip_right - clip_left),
		   (float)SCREEN_HEIGHT / (clip_top - clip_bottom),
		   (float)SCREEN_DEPTH  / (cf_z - cb_z) ) ;

      gtranslate_matrix( POST_CAT, &trans_mtx,
	      -(float)SCREEN_WIDTH * clip_left / (clip_right - clip_left),
	      -(float)SCREEN_HEIGHT* clip_top  / (clip_bottom - clip_top),
	      -(float)SCREEN_DEPTH * cb_z / (cf_z - cb_z) ) ;

      gtranslate_matrix( POST_CAT, &trans_mtx,
			(float)(1.0*72), (float)(0.5*72), 0 ) ;
}




/**************************************************
*
*    init_transformation()
*
***************************************************/

static void init_transformation()
{
      /* Initialize matrix, just in case */
      gset_unit_matrix( &trans_mtx ) ;

      /* Initialize Projection point */
      prp.v[0] = 0.0 ;
      prp.v[1] = 0.0 ;
      prp.v[2] = DEFAULT_PRP_Z ;
      prp.v[3] = 0.0 ;

      /* Viewing */
      view_rotx = view_roty = 0.0 ;
      view_zoom = 1.0 ;

      /* Initialize view volume boundary */
      clip_right =  DEFAULT_WINDOW_WIDTH / 2.0 ;
      clip_left  = -DEFAULT_WINDOW_WIDTH / 2.0 ;
      clip_top   =  DEFAULT_WINDOW_HEIGHT / 2.0 ;
      clip_bottom= -DEFAULT_WINDOW_HEIGHT / 2.0 ;
      clip_front =  DEFAULT_FRONT_PLANE_Z ;
      clip_back  =  DEFAULT_BACK_PLANE_Z ;
}


/********************************************
*
*    set_unit_matrix()
*
*********************************************/

static void  gset_unit_matrix(Matrix *mtx)
{
      long  row, col ;

      /* Clear the matrix */
      for( row = 0 ; row < 4 ; row++ )
	    for( col = 0 ; col < 4 ; col++ )
		  mtx->m[row][col] = 0.0 ;

      /* Set 1.0s along diagonal line */
      for( row = 0 ; row < 4 ; row++ )
	    mtx->m[row][row] = 1.0 ;
}




/********************************************
*
*    concatenate_matrix()
*
*          m1 <- m1 * m2 (precat = 1)
*          m1 <- m2 * m1 (precat = 0)
*
*********************************************/

static void  gconcatenate_matrix(long precat, Matrix *m1, Matrix *m2)
{
      long  row, col, scan ;
      Matrix *dest ;
      Matrix temp ;


      /* Swap pointer according to the concatenation mode */
      dest = m1 ;
      if( precat == 1 )
      {
	    m1 = m2 ;
	    m2 = dest ;
      }

      /* concatenate it */
      for( row = 0 ; row < 4 ; row++ )
	    for( col = 0 ; col < 4 ; col++ )
	    {
		  temp.m[row][col] = 0.0 ;
		  for( scan = 0 ; scan < 4 ; scan++ )
			temp.m[row][col] +=
			      m1->m[row][scan] * m2->m[scan][col];
	    }

      *dest = temp ;
}


/********************************************
*
*    scale_matrix()
*
*          m1 <- SCALE * m1 (precat = 1)
*          m1 <- m1 * SCALE (precat = 0)
*
*********************************************/

static void  gscale_matrix(long precat, Matrix *m1, float sx, float sy, float sz)
{
      Matrix smat ;

      /* Initialize to unit matrix */
      gset_unit_matrix( &smat ) ;

      /* Set scale values */
      smat.m[0][0] = sx ;
      smat.m[1][1] = sy ;
      smat.m[2][2] = sz ;

      /* concatenate */
      gconcatenate_matrix( precat, m1, &smat ) ;
}



/********************************************
*
*    translate_matrix()
*
*          m1 <- T * m1 (precat = 1)
*          m1 <- m1 * T (precat = 0)
*
*********************************************/

static void  gtranslate_matrix(long precat, Matrix *m1, float tx, float ty, float tz)
{
      Matrix tmat ;

      /* Initialize to unit matrix */
      gset_unit_matrix( &tmat ) ;

      /* Set scale values */
      tmat.m[3][0] = tx ;
      tmat.m[3][1] = ty ;
      tmat.m[3][2] = tz ;

      /* concatenate */
      gconcatenate_matrix( precat, m1, &tmat ) ;
}



/********************************************
*
*    rotate_x_matrix()
*    rotate_y_matrix()
*    rotate_z_matrix()
*
*          m1 <- ROT * m1 (precat = 1)
*          m1 <- m1 * ROT (precat = 0)
*
*********************************************/

static void  grotate_x_matrix(long precat, Matrix *m1, float rot)
{
      Matrix rmat ;
      float s_val, c_val ;

      /* Initialize to unit matrix */
      gset_unit_matrix( &rmat ) ;

      /* Set scale values */
      s_val = sin( rot * M_PI / 180.0 ) ;
      c_val = cos( rot * M_PI / 180.0 ) ;
      rmat.m[1][1] = c_val ;
      rmat.m[1][2] = s_val ;
      rmat.m[2][1] = -s_val ;
      rmat.m[2][2] = c_val ;

      /* concatenate */
      gconcatenate_matrix( precat, m1, &rmat ) ;
}




static void  grotate_y_matrix(long precat, Matrix *m1, float rot)
{
      Matrix rmat ;
      float s_val, c_val ;

      /* Initialize to unit matrix */
      gset_unit_matrix( &rmat ) ;

      /* Set scale values */
      s_val = sin( rot * M_PI / 180.0 ) ;
      c_val = cos( rot * M_PI / 180.0 ) ;
      rmat.m[0][0] = c_val ;
      rmat.m[0][2] = -s_val ;
      rmat.m[2][0] = s_val ;
      rmat.m[2][2] = c_val ;

      /* concatenate */
      gconcatenate_matrix( precat, m1, &rmat ) ;
}




/********************************************
*
*    transform()
*
*          v1 <- v2 * mtx
*
*********************************************/

static void gtransform(Vertex2 *v1, Vertex2 *v2, Matrix *mtx)
{
      float x, y, z, w ;

      x  = v2->v[0] * mtx->m[0][0] ;
       y  = v2->v[0] * mtx->m[0][1] ;
        z  = v2->v[0] * mtx->m[0][2] ;
         w  = v2->v[0] * mtx->m[0][3] ;

      x += v2->v[1] * mtx->m[1][0] ;
       y += v2->v[1] * mtx->m[1][1] ;
        z += v2->v[1] * mtx->m[1][2] ;
         w += v2->v[1] * mtx->m[1][3] ;

      x += v2->v[2] * mtx->m[2][0] ;
       y += v2->v[2] * mtx->m[2][1] ;
        z += v2->v[2] * mtx->m[2][2] ;
         w += v2->v[2] * mtx->m[2][3] ;

      x += v2->v[3] * mtx->m[3][0] ;
       y += v2->v[3] * mtx->m[3][1] ;
        z += v2->v[3] * mtx->m[3][2] ;
         w += v2->v[3] * mtx->m[3][3] ;

      v1->v[0] = x ;
       v1->v[1] = y ;
        v1->v[2] = z ;
         v1->v[3] = w ;
}


/********************************************
*
*    inverse_matrix()
*
*          m1 <- inv(m2)
*
*********************************************/


static void ginverse_matrix(Matrix *m1, Matrix *m2)
{
      double detval ;

      /* det(m2) */
      detval = det( m2 ) ;

      /* Clamel's solution */
      m1->m[0][0] =  cdet( m2, 1,2,3, 1,2,3 ) / detval ;
      m1->m[0][1] = -cdet( m2, 0,2,3, 1,2,3 ) / detval ;
      m1->m[0][2] =  cdet( m2, 0,1,3, 1,2,3 ) / detval ;
      m1->m[0][3] = -cdet( m2, 0,1,2, 1,2,3 ) / detval ;

      m1->m[1][0] = -cdet( m2, 1,2,3, 0,2,3 ) / detval ;
      m1->m[1][1] =  cdet( m2, 0,2,3, 0,2,3 ) / detval ;
      m1->m[1][2] = -cdet( m2, 0,1,3, 0,2,3 ) / detval ;
      m1->m[1][3] =  cdet( m2, 0,1,2, 0,2,3 ) / detval ;

      m1->m[2][0] =  cdet( m2, 1,2,3, 0,1,3 ) / detval ;
      m1->m[2][1] = -cdet( m2, 0,2,3, 0,1,3 ) / detval ;
      m1->m[2][2] =  cdet( m2, 0,1,3, 0,1,3 ) / detval ;
      m1->m[2][3] = -cdet( m2, 0,1,2, 0,1,3 ) / detval ;

      m1->m[3][0] = -cdet( m2, 1,2,3, 0,1,2 ) / detval ;
      m1->m[3][1] =  cdet( m2, 0,2,3, 0,1,2 ) / detval ;
      m1->m[3][2] = -cdet( m2, 0,1,3, 0,1,2 ) / detval ;
      m1->m[3][3] =  cdet( m2, 0,1,2, 0,1,2 ) / detval ;
}



static double det(Matrix *m)
{
      double det_sum ;

      /* Expand with respect to column 4 */
      det_sum = 0.0 ;
      if( m->m[0][3] != 0.0 )
	    det_sum -= m->m[0][3] * cdet( m, 1, 2, 3,  0, 1, 2 ) ;
      if( m->m[1][3] != 0.0 )
	    det_sum += m->m[1][3] * cdet( m, 0, 2, 3,  0, 1, 2 ) ;
      if( m->m[2][3] != 0.0 )
	    det_sum -= m->m[2][3] * cdet( m, 0, 1, 3,  0, 1, 2 ) ;
      if( m->m[3][3] != 0.0 )
	    det_sum += m->m[3][3] * cdet( m, 0, 1, 2,  0, 1, 2 ) ;

      return( det_sum ) ;
}


static double cdet(Matrix *m, long r0, long r1, long r2, long c0, long c1, long c2)
{
        double temp ;

        temp  = m->m[r0][c0] * m->m[r1][c1] * m->m[r2][c2] ;
        temp += m->m[r1][c0] * m->m[r2][c1] * m->m[r0][c2] ;
        temp += m->m[r2][c0] * m->m[r0][c1] * m->m[r1][c2] ;

        temp -= m->m[r2][c0] * m->m[r1][c1] * m->m[r0][c2] ;
        temp -= m->m[r1][c0] * m->m[r0][c1] * m->m[r2][c2] ;
        temp -= m->m[r0][c0] * m->m[r2][c1] * m->m[r1][c2] ;

        return( temp ) ;
}
