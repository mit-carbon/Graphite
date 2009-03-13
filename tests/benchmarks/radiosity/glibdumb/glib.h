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

#ifndef _GLIB_H
#define _GLIB_H

#include "../structs.H"

/****************************************
*
*    Color names
*
*****************************************/

#define G_BLACK   (256)
#define G_RED     (257)
#define G_GREEN   (258)
#define G_YELLOW  (259)
#define G_BLUE    (260)
#define G_MAGENTA (261)
#define G_CYAN    (262)
#define G_WHITE   (263)



/****************************************
*
*    Panel data structures
*
*****************************************/


typedef struct {
    char *name;
    long min, max;
    long init_value;
    long ticks;
    void (*callback)();
} slider;


#define MAX_POSSIBILITIES	32

typedef struct {
    char *name;
    char *possibilities[MAX_POSSIBILITIES];
    long init_value;
    void (*callback)();
} choice;

/****************************************
*
*    Library function type definition
*
*****************************************/

void g_init(int ac, char *av[]);
void g_start(void (*mouse_func)(void), long n_sliders, slider *slider_def, long n_choices, choice *choice_def);
void g_color(long color);
void g_rgb(Rgb color);
void g_line(Vertex *p1, Vertex *p2);
void g_polygon(long n, Vertex *p_list);
void g_spolygon(long n, Vertex *p_list, Rgb *c_list);
void g_clear(void);
void g_setup_view(float rot_x, float rot_y, float dist, float zoom);
void g_get_screen_size(long *u, long *v);
void g_flush(void);

#endif
