/* -*-mode:c-*- */
/**************************************************************
*
*        CS348C   Radiosity
*
*        Device independent graphics package.
*
*        May 6, 1991
*                                      Tsai, Tso-Sheng
*                                      Totsuka, Takashi
*
***************************************************************/

#ifndef _PSLIB_H
#define _PSLIB_H

#include "../structs.H"

#define M_PI           3.14159265358979323846

typedef struct
{
	float v[4] ;                   /* x, y, z, and w */
} Vertex2;

typedef struct
{
	float m[4][4] ;                /* m[row][column], row vector assumed */
} Matrix;

/****************************************
*
*    Library function type definition
*
*****************************************/

long ps_open(char *file);
void ps_close(void);
void ps_linewidth(float w);
void ps_line(Vertex *p1, Vertex *p2);
void ps_polygonedge(long n, Vertex *p_list);
void ps_polygon(long n, Vertex *p_list);
void ps_spolygon(long n, Vertex *p_list, Rgb *c_list);
void ps_clear(void);
void ps_setup_view(float rot_x, float rot_y, float dist, float zoom);

#endif
