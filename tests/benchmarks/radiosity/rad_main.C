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
 *       Parallel Hierarchical Radiosity
 *
 *	    Main program
 *
 ***************************************************************/

#include <stdio.h>
#include <string.h>
#if defined(SGI_GL)
#include <gl.h>
#if defined(GL_NASA)
#include <panel.h>
#endif
#endif

/* ANL macro initialization */

MAIN_ENV;

include(radiosity.h)



  /***************************************
   *
   *    Global shared variables
   *
   ****************************************/

  Global *global;
  Timing **timing;


  /***************************************
   *
   *    Global variables (not shared)
   *
   ****************************************/

  long   n_processors              = DEFAULT_N_PROCESSORS ;
  long   n_taskqueues              = DEFAULT_N_TASKQUEUES ;
  long   n_tasks_per_queue         = DEFAULT_N_TASKS_PER_QUEUE ;
  long   N_inter_parallel_bf_refine= DEFAULT_N_INTER_PARALLEL_BF_REFINEMENT ;
  long   N_visibility_per_task     = DEFAULT_N_VISIBILITY_PER_TASK ;
  float Area_epsilon              = DEFAULT_AREA_EPSILON ;
  float Energy_epsilon            = DEFAULT_ENERGY_EPSILON ;
  float BFepsilon                 = DEFAULT_BFEPSILON ;

  long batch_mode = 0 ;
  long verbose_mode = 0 ;

  /*
    in converting from a fork process model to an sproc (threads) model,
    taskqueue_id and time_process_start are converted to individual arrays
    without worrying about false sharing.  This is because taskqueue_id is
    read-only  once written by the parent process, and time_process_start
    is written only once by each process.
    */

  long taskqueue_id[MAX_PROCESSORS] ; 		/* Task queue ID */
  long time_rad_start, time_rad_end, time_process_start[MAX_PROCESSORS] ;


  /*********************************************************
   *
   *    Global variables (used only by the master process)
   *
   **********************************************************/

#define N_SLIDERS (5)

  slider sliders[N_SLIDERS] = {
      { "View(X)  deg ", -100,  100, (long)DFLT_VIEW_ROT_X,  5,  (void (*)())change_view_x },
      { "View(Y)  deg ", -100,  100, (long)DFLT_VIEW_ROT_Y,  5,  (void (*)())change_view_y },
      { "View(Zoom)   ",   0,  50, (long)DFLT_VIEW_ZOOM*10,6,  (void (*)())change_view_zoom },
      { "BF-e      0.1%",  0,  50,  (long)(DEFAULT_BFEPSILON *1000.0),
            11, (void (*)())change_BFepsilon },
      { "Area-e       ",   0, 5000, (long)DEFAULT_AREA_EPSILON,
            11, (void (*)())change_area_epsilon },
  } ;

#define N_CHOICES (4)

#define CHOICE_RAD_RUN    (0)
#define CHOICE_RAD_STEP   (1)
#define CHOICE_RAD_RESET  (2)

#define CHOICE_DISP_RADIOSITY   (0)
#define CHOICE_DISP_SHADED      (1)
#define CHOICE_DISP_PATCH       (2)
#define CHOICE_DISP_MESH        (3)
#define CHOICE_DISP_INTERACTION (4)

#define CHOICE_UTIL_PS        (0)
#define CHOICE_UTIL_STAT_CRT  (1)
#define CHOICE_UTIL_STAT_FILE (2)
#define CHOICE_UTIL_CLEAR_RAD (3)

  choice choices[N_CHOICES] = {
      { "Run",
            { "Run", "Step", "Reset", 0 },
            0, (void (*)())start_radiosity },
      { "Display",
            { "Filled",   "Smooth shading", "Show polygon edges",
                  "Show element edges",  "Show interactions", 0 },
            0, (void (*)())change_display },
      { "Models",
            { "Test", "Room", "LargeRoom", 0 },
            0, (void (*)())select_model },
      { "Tools",
            { "HardCopy(PS)", "Statistics", "Statistics(file)",
                  "Clear Radiosity Value", 0 },
            0, (void (*)())utility_tools },
  } ;

  /***************************************
   *
   *    Main function.
   *
   ****************************************/

static void change_view(void);
static void expose_callback(void);
static void _init_radavg_tasks(Patch *p, long mode, long process_id);
static void parse_args(int argc, char *argv[]);

static long dostats = 0;

int main(int argc, char *argv[])
{
    long i;
    long total_rad_time, max_rad_time, min_rad_time;
    long total_refine_time, max_refine_time, min_refine_time;
    long total_wait_time, max_wait_time, min_wait_time;
    long total_vertex_time, max_vertex_time, min_vertex_time;

    /* Parse arguments */
    parse_args(argc, argv) ;
    choices[2].init_value = model_selector ;

    /* Initialize graphic device */
    if( batch_mode == 0 )
        {
            g_init(argc, argv) ;
            setup_view( DFLT_VIEW_ROT_X, DFLT_VIEW_ROT_Y,
                       DFLT_VIEW_DIST, DFLT_VIEW_ZOOM,0 ) ;
        }

    /* Initialize ANL macro */
    MAIN_INITENV(,60000000) ;

    THREAD_INIT_FREE();

    /* Allocate global shared memory and initialize */
    global = (Global *) G_MALLOC(sizeof(Global)) ;
    if( global == 0 )
        {
            printf( "Can't allocate memory\n" ) ;
            exit(1) ;
        }
    init_global(0) ;

    timing = (Timing **) G_MALLOC(n_processors * sizeof(Timing *));
    for (i = 0; i < n_processors; i++)
        timing[i] = (Timing *) G_MALLOC(sizeof(Timing));

    /* Initialize shared lock */
    init_sharedlock(0) ;

    /* Initial random testing rays array for visibility test. */
    init_visibility_module(0) ;

/* POSSIBLE ENHANCEMENT:  Here is where one might distribute the
   sobj_struct, task_struct, and vis_struct data structures across
   physically distributed memories as desired.

   One way to place data is as follows:

   long i;

   for (i=0;i<n_processors;i++) {
     Place all addresses x such that
       &(sobj_struct[i]) <= x < &(sobj_struct[i+1]) on node i
     Place all addresses x such that
       &(task_struct[i]) <= x < &(task_struct[i+1]) on node i
     Place all addresses x such that
       &(vis_struct[i]) <= x < &(vis_struct[i+1]) on node i
   }

*/

    if( batch_mode )
        {
            /* In batch mode, create child processes and start immediately */

            /* Time stamp */
            CLOCK( time_rad_start );

            global->index = 0;
            for( i = 0 ; i < n_processors ; i++ )
                {
                    taskqueue_id[i] = assign_taskq(0) ;
                }

            // Enable Models at the start of parallel execution
            CarbonEnableModels();

            /* And start processing */
            CREATE(radiosity, n_processors);
            WAIT_FOR_END(n_processors);

            // Disable Models at the end of parallel execution
            CarbonDisableModels();

            /* Time stamp */
            CLOCK( time_rad_end );

            /* Print out running time */
            printf("TIMING STATISTICS MEASURED BY MAIN PROCESS:\n");

            print_running_time(0);

            if (dostats) {
                printf("\n\n\nPER-PROCESS STATISTICS:\n");

                printf("%8s%20s%20s%12s%12s\n","Proc","Total","Refine","Wait","Smooth");
                printf("%8s%20s%20s%12s%12s\n\n","","Time","Time","Time","Time");
                for (i = 0; i < n_processors; i++)
                    printf("%8ld%20lu%20lu%12lu%12lu\n",i,timing[i]->rad_time, timing[i]->refine_time, timing[i]->wait_time, timing[i]->vertex_time);

                total_rad_time = timing[0]->rad_time;
                max_rad_time = timing[0]->rad_time;
                min_rad_time = timing[0]->rad_time;

                total_refine_time = timing[0]->refine_time;
                max_refine_time = timing[0]->refine_time;
                min_refine_time = timing[0]->refine_time;

                total_wait_time = timing[0]->wait_time;
                max_wait_time = timing[0]->wait_time;
                min_wait_time = timing[0]->wait_time;

                total_vertex_time = timing[0]->vertex_time;
                max_vertex_time = timing[0]->vertex_time;
                min_vertex_time = timing[0]->vertex_time;

                for (i = 1; i < n_processors; i++) {
                    total_rad_time += timing[i]->rad_time;
                    if (timing[i]->rad_time > max_rad_time)
                        max_rad_time = timing[i]->rad_time;
                    if (timing[i]->rad_time < min_rad_time)
                        min_rad_time = timing[i]->rad_time;

                    total_refine_time += timing[i]->refine_time;
                    if (timing[i]->refine_time > max_refine_time)
                        max_refine_time = timing[i]->refine_time;
                    if (timing[i]->refine_time < min_refine_time)
                        min_refine_time = timing[i]->refine_time;

                    total_wait_time += timing[i]->wait_time;
                    if (timing[i]->wait_time > max_wait_time)
                        max_wait_time = timing[i]->wait_time;
                    if (timing[i]->wait_time < min_wait_time)
                        min_wait_time = timing[i]->wait_time;

                    total_vertex_time += timing[i]->vertex_time;
                    if (timing[i]->vertex_time > max_vertex_time)
                        max_vertex_time = timing[i]->vertex_time;
                    if (timing[i]->vertex_time < min_vertex_time)
                        min_vertex_time = timing[i]->vertex_time;
                }

                printf("\n\n%8s%20lu%20lu%12lu%12lu\n","Max", max_rad_time, max_refine_time, max_wait_time, max_vertex_time);
                printf("\n%8s%20lu%20lu%12lu%12lu\n","Min", min_rad_time, min_refine_time, min_wait_time, min_vertex_time);
                printf("\n%8s%20lu%20lu%12lu%12lu\n","Avg", (long) (((double) total_rad_time) / ((double) (1.0 * n_processors))), (long) (((double) total_refine_time) / ((double) (1.0 * n_processors))), (long) (((double) total_wait_time) / ((double) (1.0 * n_processors))), (long) (((double) total_vertex_time) / ((double) (1.0 * n_processors))));
                printf("\n\n");

            }

            /*	print_fork_time(0) ; */

            print_statistics( stdout, 0 ) ;
        }
    else
        {
            /* In interactive mode, start workers, and the master starts
               notification loop */

            /* Start notification loop */
            g_start( expose_callback,
                    N_SLIDERS, sliders, N_CHOICES, choices ) ;
        }
    MAIN_END;
    exit(0) ;
}



/***************************************
 *
 *    PANEL call back routine
 *
 *    start_radiosity()   (MASTER only)
 *
 ****************************************/

static long disp_fill_switch = 1 ;
static long disp_shade_switch = 0 ;
static long disp_fill_mode = 1 ;
static long disp_patch_switch = 0 ;
static long disp_mesh_switch  = 0 ;
static long disp_interaction_switch = 0 ;
static long disp_crnt_view_x = (long)DFLT_VIEW_ROT_X ;
  static long disp_crnt_view_y = (long)DFLT_VIEW_ROT_Y ;
  static float disp_crnt_zoom = DFLT_VIEW_ZOOM ;


#if defined(SGI_GL) && defined(GL_NASA)
void start_radiosity(Actuator *ap)
#else
void start_radiosity(long val)
#endif
{
    static long state = 0 ;
    long i;
    long total_rad_time, max_rad_time, min_rad_time;
    long total_refine_time, max_refine_time, min_refine_time;
    long total_wait_time, max_wait_time, min_wait_time;
    long total_vertex_time, max_vertex_time, min_vertex_time;

#if defined(SGI_GL) && defined(GL_NASA)
    long val ;

    val = g_get_choice_val( ap, &choices[0] ) ;
#endif

    if( val == CHOICE_RAD_RUN )
        {
            if( state == -1 )
                {
                    printf( "Please reset first\007\n" ) ;
                    return ;
                }

            /* Time stamp */
            CLOCK( time_rad_start ) ;


            global->index = 0;

            /* Create slave processes */
            for (i = 0 ; i < n_processors ; i++ )
                {
                    taskqueue_id[i] = assign_taskq(0) ;
                }

            /* And start processing */
            CREATE(radiosity, n_processors);
            WAIT_FOR_END(n_processors);
            /* Time stamp */
            CLOCK( time_rad_end );

            /* Print out running time */
            /* Print out running time */
            printf("TIMING STATISTICS MEASURED BY MAIN PROCESS:\n");

            print_running_time(0);

            if (dostats) {
                printf("\n\n\nPER-PROCESS STATISTICS:\n");

                printf("%8s%20s%20s%12s%12s\n","Proc","Total","Refine","Wait","Smooth");
                printf("%8s%20s%20s%12s%12s\n\n","","Time","Time","Time","Time")
                    ;
                for (i = 0; i < n_processors; i++)
                    printf("%8ld%20lu%20lu%12lu%12lu\n",i,timing[i]->rad_time, timing[i]->refine_time, timing[i]->wait_time, timing[i]->vertex_time);

                total_rad_time = timing[0]->rad_time;
                max_rad_time = timing[0]->rad_time;
                min_rad_time = timing[0]->rad_time;

                total_refine_time = timing[0]->refine_time;
                max_refine_time = timing[0]->refine_time;
                min_refine_time = timing[0]->refine_time;

                total_wait_time = timing[0]->wait_time;
                max_wait_time = timing[0]->wait_time;
                min_wait_time = timing[0]->wait_time;

                total_vertex_time = timing[0]->vertex_time;
                max_vertex_time = timing[0]->vertex_time;
                min_vertex_time = timing[0]->vertex_time;

                for (i = 1; i < n_processors; i++) {
                    total_rad_time += timing[i]->rad_time;
                    if (timing[i]->rad_time > max_rad_time)
                        max_rad_time = timing[i]->rad_time;
                    if (timing[i]->rad_time < min_rad_time)
                        min_rad_time = timing[i]->rad_time;

                    total_refine_time += timing[i]->refine_time;
                    if (timing[i]->refine_time > max_refine_time)
                        max_refine_time = timing[i]->refine_time;
                    if (timing[i]->refine_time < min_refine_time)
                        min_refine_time = timing[i]->refine_time;

                    total_wait_time += timing[i]->wait_time;
                    if (timing[i]->wait_time > max_wait_time)
                        max_wait_time = timing[i]->wait_time;
                    if (timing[i]->wait_time < min_wait_time)
                        min_wait_time = timing[i]->wait_time;

                    total_vertex_time += timing[i]->vertex_time;
                    if (timing[i]->vertex_time > max_vertex_time)
                        max_vertex_time = timing[i]->vertex_time;
                    if (timing[i]->vertex_time < min_vertex_time)
                        min_vertex_time = timing[i]->vertex_time;
                }


                printf("\n\n%8s%20lu%20lu%12lu%12lu\n","Max", max_rad_time, max_refine_time, max_wait_time, max_vertex_time);
                printf("\n%8s%20lu%20lu%12lu%12lu\n","Min", min_rad_time, min_refine_time, min_wait_time, min_vertex_time);
                printf("\n%8s%20lu%20lu%12lu%12lu\n","Avg", (long) (((double) total_rad_time) / ((double) (1.0 * n_processors))), (long) (((double) total_refine_time) / ((double) (1.0 * n_processors))), (long) (((double) total_wait_time) / ((double) (1.0 * n_processors))), (long) (((double) total_vertex_time) / ((double) (1.0 * n_processors))));
                printf("\n\n");

            }

            /*      print_fork_time(0) ; */

            print_statistics( stdout, 0 ) ;

            /* Display image */
            display_scene( disp_fill_mode, disp_patch_switch,
                          disp_mesh_switch, disp_interaction_switch, 0) ;

            state = -1 ;
        }

    else if( val == CHOICE_RAD_STEP )
        {
            if( state == -1 )
                {
                    printf( "Please reset first\007\n" ) ;
                    return ;
                }

            /* Step execution */
            switch( state )
                {
                case 0:
                    /* Step execute as a single process */

                    global->index = 1;
                    /* Create slave processes */
                    for ( i = 0 ; i < n_processors ; i++ )
                        {
                            taskqueue_id[i] = assign_taskq(0) ;
                        }

                    CREATE(radiosity, n_processors/* - 1*/);

                    /* Decompose model objects into patches and build
                       the BSP tree */
                    /* Create the first tasks (MASTER only) */
                    init_modeling_tasks(0) ;
                    process_tasks(0) ;
                    state ++ ;
                    break ;

                case 1:
                    if( init_ray_tasks(0) )
                        {
                            BARRIER(global->barrier, n_processors);
                            process_tasks(0) ;
                        }
                    else
                        state++ ;
                    break ;
                default:
                    BARRIER(global->barrier, n_processors);
                    init_radavg_tasks( RAD_AVERAGING_MODE, 0 ) ;
                    process_tasks(0) ;
                    init_radavg_tasks( RAD_NORMALIZING_MODE, 0 ) ;
                    process_tasks(0) ;

                    WAIT_FOR_END(n_processors/* - 1*/)
                        state = -1 ;
                }

            /* Display image */
            display_scene( disp_fill_mode, disp_patch_switch,
                          disp_mesh_switch, disp_interaction_switch, 0) ;
        }

    else if( val == CHOICE_RAD_RESET )
        {
            /* Initialize global variables again */
            init_global(0) ;
            init_visibility_module(0) ;
            g_clear() ;
            state = 0 ;
        }
}


/***************************************
 *
 *    PANEL call back routine
 *
 *    change_display()   (MASTER only)
 *
 ****************************************/


#if defined(SGI_GL) && defined(GL_NASA)
void change_display(Actuator *ap)
#else
void change_display(long val)
#endif
{
#if defined(SGI_GL) && defined(GL_NASA)
    long val ;

    val = g_get_choice_val( ap, &choices[1] ) ;
#endif

    /* Display image */
    switch( val )
        {
        case CHOICE_DISP_RADIOSITY:
            disp_fill_switch = (! disp_fill_switch) ;
            break ;
        case CHOICE_DISP_SHADED:
            disp_shade_switch = (! disp_shade_switch) ;
            break ;
        case CHOICE_DISP_PATCH:
            disp_patch_switch = (! disp_patch_switch) ;
            break ;
        case CHOICE_DISP_MESH:
            disp_mesh_switch = (! disp_mesh_switch) ;
            break ;
        case CHOICE_DISP_INTERACTION:
            disp_interaction_switch = (! disp_interaction_switch) ;
            break ;
        default:
            return ;
        }

    if( disp_fill_switch == 0 )
        disp_fill_mode = 0 ;
    else
        {
            if( disp_shade_switch == 0 )
                disp_fill_mode = 1 ;
            else
                disp_fill_mode = 2 ;
        }

    /* Display image */
    display_scene( disp_fill_mode, disp_patch_switch,
                  disp_mesh_switch, disp_interaction_switch, 0 ) ;
}


/*****************************************************
 *
 *    PANEL call back routine
 *
 *    change_view_y()            (MASTER only)
 *    change_BFepsilon()
 *    change_area_epsilon()
 *
 ******************************************************/

static void change_view()
{
    /* Change the view */
    setup_view( (float)disp_crnt_view_x, (float)disp_crnt_view_y,
               DFLT_VIEW_DIST, disp_crnt_zoom, 0 ) ;

    /* And redraw */
    display_scene( disp_fill_mode, disp_patch_switch,
                  disp_mesh_switch, disp_interaction_switch, 0 ) ;
}


#if defined(SGI_GL) && defined(GL_NASA)
void change_view_x(Actuator *ap)
#else
void change_view_x(long val)
#endif
{
#if defined(SGI_GL) && defined(GL_NASA)
    long val = g_get_slide_val( ap ) ;
#endif

    /* Save current rot-X value */
    disp_crnt_view_x = val ;
    change_view() ;
}


#if defined(SGI_GL) && defined(GL_NASA)
void change_view_y(Actuator *ap)
#else
void change_view_y(long val )
#endif
{
#if defined(SGI_GL) && defined(GL_NASA)
    long val = g_get_slide_val( ap ) ;
#endif

    /* Save current rot-Y value */
    disp_crnt_view_y = val ;
    change_view() ;
}


#if defined(SGI_GL) && defined(GL_NASA)
void change_view_zoom(Actuator *ap)
#else
void change_view_zoom(long val)
#endif
{
#if defined(SGI_GL) && defined(GL_NASA)
    long val = g_get_slide_val( ap ) ;
#endif

    /* Save current zoom value */
    disp_crnt_zoom = (float)val / 10.0 ;
    change_view() ;
}


#if defined(SGI_GL) && defined(GL_NASA)
void change_BFepsilon(Actuator *ap)
#else
void change_BFepsilon(long val)
#endif
{
#if defined(SGI_GL) && defined(GL_NASA)
    long val = g_get_slide_val( ap ) ;
#endif
    BFepsilon = (float)val / 1000.0 ;
}



#if defined(SGI_GL) && defined(GL_NASA)
void change_area_epsilon(Actuator *ap)
#else
void change_area_epsilon(long val)
#endif
{
#if defined(SGI_GL) && defined(GL_NASA)
    long val = g_get_slide_val( ap ) ;
#endif
    Area_epsilon = (float)val ;
}


/***************************************
 *
 *    PANEL call back routine
 *
 *    select_model()   (MASTER only)
 *
 ****************************************/

#if defined(SGI_GL) && defined(GL_NASA)
void select_model(Actuator *ap)
#else
void select_model(long val)
#endif
{
#if defined(SGI_GL) && defined(GL_NASA)
    long val = g_get_choice_val( ap, &choices[2] ) ;
#endif
    switch( val )
        {
        case MODEL_TEST_DATA:
            model_selector = MODEL_TEST_DATA ;
            break ;
        case MODEL_ROOM_DATA:
            model_selector = MODEL_ROOM_DATA ;
            break ;
        case MODEL_LARGEROOM_DATA:
            model_selector = MODEL_LARGEROOM_DATA ;
            break ;
        }
}



/***************************************
 *
 *    PANEL call back routine
 *
 *    utility_tools()   (MASTER only)
 *
 ****************************************/


#if defined(SGI_GL) && defined(GL_NASA)
void utility_tools(Actuator *ap)
#else
void utility_tools(long val)
#endif
{
    FILE *fd ;
#if defined(SGI_GL) && defined(GL_NASA)
    long val = g_get_choice_val( ap, &choices[3] ) ;
#endif

    switch( val )
        {
        case CHOICE_UTIL_PS:
            /* Open PS file */
            ps_open( "radiosity.ps" ) ;

            /* Change the view */
            ps_setup_view( DFLT_VIEW_ROT_X, (float)disp_crnt_view_y,
                          DFLT_VIEW_DIST, DFLT_VIEW_ZOOM) ;

            /* And redraw */
            ps_display_scene( disp_fill_mode, disp_patch_switch,
                             disp_mesh_switch, disp_interaction_switch, 0 ) ;

            /* Close */
            ps_close() ;
            break ;
        case CHOICE_UTIL_STAT_CRT:
            print_statistics( stdout, 0 ) ;
            break ;
        case CHOICE_UTIL_STAT_FILE:
            if( (fd = fopen( "radiosity_stat", "w" )) == 0 )
                {
                    perror( "radiosity_stat" ) ;
                    break ;
                }
            print_statistics( fd, 0 ) ;
            fclose( fd ) ;
            break ;
        case CHOICE_UTIL_CLEAR_RAD:
            clear_radiosity(0) ;
        }
}


/***************************************
 *
 *    Exposure call back
 *
 ****************************************/

static void expose_callback()
{
    /* Display image */
    display_scene( disp_fill_mode, disp_patch_switch,
                  disp_mesh_switch, disp_interaction_switch, 0 ) ;
}


/***************************************
 *
 *    radiosity()  Radiosity task main
 *
 ****************************************/


void radiosity()
{
    long process_id;
    long rad_start, refine_done, vertex_start, vertex_done;

    THREAD_INIT_FREE();

    LOCK(global->index_lock);
    process_id = global->index++;
    UNLOCK(global->index_lock);
    process_id = process_id % n_processors;

    BARINCLUDE(global->barrier);
    if ((process_id == 0) || (dostats))
        CLOCK(rad_start);

    /* POSSIBLE ENHANCEMENT:  Here is where one might pin processes to
       processors to avoid migration */

    /* POSSIBLE ENHANCEMENT:  Here is where one might reset the
       statistics that one is measuring about the parallel execution */

    /* Decompose model objects into patches and build the BSP tree */
    /* Create the initial tasks */
    init_modeling_tasks(process_id) ;
    process_tasks(process_id) ;

    /* Gather rays & do BF refinement */
    while( init_ray_tasks(process_id) )
        {
            /* Wait till tasks are put in the queue */
            BARRIER(global->barrier, n_processors);
            /* Then perform ray-gathering and BF-refinement till the
               solution converges */
            process_tasks(process_id) ;
        }

    if ((process_id == 0) || (dostats))
        CLOCK(refine_done);

    BARRIER(global->barrier, n_processors);

    if ((process_id == 0) || (dostats))
        CLOCK(vertex_start);

    /* Compute area-weighted radiosity value at each vertex */
    init_radavg_tasks( RAD_AVERAGING_MODE, process_id ) ;
    process_tasks(process_id) ;

    /* Then normalize the radiosity at vertices */
    init_radavg_tasks( RAD_NORMALIZING_MODE, process_id ) ;
    process_tasks(process_id) ;

    if ((process_id == 0) || (dostats))
        CLOCK(vertex_done);

    if ((process_id == 0) || (dostats)) {
        timing[process_id]->rad_start = rad_start;
        timing[process_id]->rad_time = vertex_done - rad_start;
        timing[process_id]->refine_time = refine_done - rad_start;
        timing[process_id]->vertex_time = vertex_done - vertex_start;
        timing[process_id]->wait_time = vertex_start - refine_done;
    }
}



/***************************************************************************
 *
 *    init_ray_tasks()
 *
 *    Create initial tasks to perform ray gathering.
 *
 ****************************************************************************/


#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_STATIC
static void _init_ray_tasks_static(Patch *p, long dummy, long process_id);
#define _INIT_RAY_TASK  _init_ray_tasks_static
#endif

#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_COSTBASED
static long avg_cost_of_q ;
static long avg_cost_of_patch ;
static long queue_cost[MAX_TASKQUEUES] ;

static void _init_ray_tasks_cost2(Patch *p, long layer, long process_id);
#define _INIT_RAY_TASK  _init_ray_tasks_cost2
#endif




long init_ray_tasks(long process_id)
{
    long conv ;

    /* If this is not the first process to initialize, then return */
    LOCK(global->avg_radiosity_lock);
    if( ! check_task_counter() )
        {
            conv = global->converged ;
            UNLOCK(global->avg_radiosity_lock);
            return( conv == 0 ) ;
        }

    /* Check radiosity convergence */
    conv = radiosity_converged(process_id) ;
    global->converged = conv ;

    /* Reset total energy variable */
    global->prev_total_energy = global->total_energy ;
    global->total_energy.r = 0.0 ;
    global->total_energy.g = 0.0 ;
    global->total_energy.b = 0.0 ;
    global->total_patch_area = 0.0 ;

    /* Increment iteration counter */
    global->iteration_count++ ;
    UNLOCK(global->avg_radiosity_lock);

    /* If radiosity converged, then return 0 */
    if( conv )
        return( 0 ) ;


#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_COSTBASED
    /* Compute average cost per queue. Also reset the cost variable.
       The 'cost_sum' is not locked since no one is processing rays
       at this moment */

    for( crnt_qid = 0 ; crnt_qid < n_taskqueues ; crnt_qid++ )
        queue_cost[ crnt_qid ] = 0 ;

    avg_cost_of_q = global->cost_estimate_sum / n_taskqueues ;
    avg_cost_of_patch = global->cost_estimate_sum / global->n_total_patches ;
    cost_of_this_q = 0 ;
    crnt_qid = 0 ;

    global->cost_sum = 0 ;
    global->cost_estimate_sum = 0 ;

    /* layered selection of tasks */
    foreach_patch_in_bsp( (void (*)())_INIT_RAY_TASK, 2, process_id ) ;
    foreach_patch_in_bsp( (void (*)())_INIT_RAY_TASK, 1, process_id ) ;
#endif

    /* Create BF refinement tasks */
    foreach_patch_in_bsp( (void (*)())_INIT_RAY_TASK, 0, process_id ) ;

    return( 1 ) ;
}


static void _init_ray_tasks_static(Patch *p, long dummy, long process_id)
{
    /* Clear incoming energy variable */
    p->el_root->rad_in.r = 0.0 ;
    p->el_root->rad_in.g = 0.0 ;
    p->el_root->rad_in.b = 0.0 ;

    enqueue_ray_task( (p->seq_no >> 2) % n_taskqueues, p->el_root,
                     TASK_APPEND, process_id ) ;
}

#if PATCH_ASSIGNMENT == PATCH_ASSIGNMENT_COSTBASED
static void _init_ray_tasks_cost2(Patch *p, long layer, long process_id)
{
    Patch_Cost *pc ;
    long c_est ;
    long qid ;
    long min_cost_q, min_cost ;


    pc = &global->patch_cost[ p->seq_no ] ;
    c_est = pc->cost_estimate ;

    if( c_est < 0 )
        /* Already processed */
        return ;

    if( c_est < avg_cost_of_patch * layer )
        return ;

    /* Find the first available queue */
    min_cost_q = 0 ;
    min_cost = queue_cost[ 0 ] ;
    for( qid = 0 ; qid < n_taskqueues ; qid++ )
        {
            if( (c_est + queue_cost[ qid ]) <= avg_cost_of_q )
                break ;

            if( min_cost > queue_cost[ qid ] )
                {
                    min_cost_q = qid ;
                    min_cost = queue_cost[ qid ] ;
                }
        }

    if( qid >= n_taskqueues )
        {
            /* All queues are nearly full. Put to min-cost queue */
            qid = min_cost_q ;
        }

    /* Update queue cost */
    queue_cost[ qid ] += c_est ;


    /* Clear incoming energy variable */
    p->el_root->rad_in.r = 0.0 ;
    p->el_root->rad_in.g = 0.0 ;
    p->el_root->rad_in.b = 0.0 ;

    /* Clear cost value */
    pc->cost_estimate = -1 ;
    pc->n_bsp_node    = 0 ;

    /* Enqueue */
    enqueue_ray_task( qid, p->el_root, TASK_APPEND, process_id ) ;

}
#endif



/***************************************************************************
 *
 *    init_radavg_tasks()
 *
 *    Create initial tasks to perform radiosity averaging.
 *
 ****************************************************************************/

void init_radavg_tasks(long mode, long process_id)
{

    /* If this is not the first process to initialize, then return */
    if( ! check_task_counter() )
        return ;

    /* Create RadAvg tasks */
    foreach_patch_in_bsp( (void (*)())_init_radavg_tasks, mode, process_id ) ;
}


static void _init_radavg_tasks(Patch *p, long mode, long process_id)
{
    enqueue_radavg_task( p->seq_no % n_taskqueues, p->el_root, mode, process_id  ) ;
}



/***************************************
 *
 *    init_global()
 *
 ****************************************/


void init_global(long process_id)
{
    /* Clear BSP root pointer */
    global->index = 1;  /* ****** */
    global->bsp_root = 0 ;
    LOCKINIT(global->index_lock);
    LOCKINIT(global->bsp_tree_lock);

    /* Initialize radiosity statistics variables */
    LOCKINIT(global->avg_radiosity_lock);
    global->converged = 0 ;
    global->prev_total_energy.r = 0.0 ;
    global->prev_total_energy.g = 0.0 ;
    global->prev_total_energy.b = 0.0 ;
    global->total_energy.r = 1.0 ;
    global->total_energy.g = 1.0 ;
    global->total_energy.b = 1.0 ;
    global->total_patch_area = 1.0 ;
    global->iteration_count = -1 ;     /* init_ray_task() increments to 0 */

    /* Initialize the cost sum */
    LOCKINIT(global->cost_sum_lock);
    global->cost_sum = 0 ;
    global->cost_estimate_sum = 0 ;

    /* Initialize the barrier */
    BARINIT(global->barrier, n_processors);
    LOCKINIT(global->pbar_lock);
    global->pbar_count = 0 ;

    /* Initialize task counter */
    global->task_counter = 0 ;
    LOCKINIT(global->task_counter_lock);

    /* Initialize task queue */
    init_taskq(process_id) ;

    /* Initialize Patch, Element, Interaction free lists */
    init_patchlist(process_id) ;
    init_elemlist(process_id) ;
    init_interactionlist(process_id) ;
    init_elemvertex(process_id) ;
    init_edge(process_id) ;

    /* Initialize statistical info */
    init_stat_info(process_id) ;

}


/*************************************************************
 *
 * parse_args()   Parse arguments
 *
 **************************************************************/

static void parse_args(int argc, char *argv[])
{
    long cnt ;

    /* Parse arguments */
    for( cnt = 1 ; cnt < argc ; cnt++ )
        {
            if( strcmp( argv[cnt], "-p" ) == 0 ) {
                sscanf( argv[++cnt], "%ld", &n_processors ) ;
                n_taskqueues = n_processors;
            }
            else if( strcmp( argv[cnt], "-tq" ) == 0 )
                sscanf( argv[++cnt], "%ld", &n_tasks_per_queue ) ;
            else if( strcmp( argv[cnt], "-ae" ) == 0 )
                sscanf( argv[++cnt], "%f", &Area_epsilon ) ;
            else if( strcmp( argv[cnt], "-pr" ) == 0 )
                sscanf( argv[++cnt], "%ld", &N_inter_parallel_bf_refine ) ;
            else if( strcmp( argv[cnt], "-pv" ) == 0 )
                sscanf( argv[++cnt], "%ld", &N_visibility_per_task ) ;
            else if( strcmp( argv[cnt], "-bf" ) == 0 )
                sscanf( argv[++cnt], "%f", &BFepsilon ) ;
            else if( strcmp( argv[cnt], "-en" ) == 0 )
                sscanf( argv[++cnt], "%f", &Energy_epsilon ) ;

            else if( strcmp( argv[cnt], "-batch" ) == 0 )
                batch_mode = 1 ;
            else if( strcmp( argv[cnt], "-verbose" ) == 0 )
                verbose_mode = 1 ;
            else if( strcmp( argv[cnt], "-s" ) == 0 )
                dostats = 1 ;
            else if( strcmp( argv[cnt], "-room" ) == 0 )
                model_selector = MODEL_ROOM_DATA ;
            else if( strcmp( argv[cnt], "-largeroom" ) == 0 )
                model_selector = MODEL_LARGEROOM_DATA ;
            else if(( strcmp( argv[cnt], "-help" ) == 0 ) || ( strcmp( argv[cnt], "-h" ) == 0 ) || ( strcmp( argv[cnt], "-H" ) == 0 ))	    {
                print_usage() ;
                exit(0) ;
            }
        }


    /* Then check the arguments */
    if( (n_processors < 1) || (MAX_PROCESSORS < n_processors) )
        {
            fprintf( stderr, "Bad number of processors: %ld\n",
                    n_processors ) ;
            exit(1) ;
        }
    if( (n_taskqueues < 1) || (MAX_TASKQUEUES < n_taskqueues) )
        {
            fprintf( stderr, "Bad number of task queues: %ld\n",
                    n_taskqueues ) ;
            exit(1) ;
        }
    /* Check epsilon values */
    if( Area_epsilon < 0.0 )
        {
            fprintf( stderr, "Area epsilon must be positive\n" ) ;
            exit(1) ;
        }
    if( BFepsilon < 0.0 )
        {
            fprintf( stderr, "BFepsilon must be within [0,1]\n" ) ;
            exit(1) ;
        }
}



/*************************************************************
 *
 *   print_usage()
 *
 **************************************************************/

void print_usage()
{
    fprintf( stderr, "Usage:  RADIOSITY  [options..]\n\n" ) ;
    fprintf( stderr, "\tNote: Must have a space between option label and numeric value, if any\n\n");
    fprintf( stderr, "   -p    (d)  # of processes\n" ) ;
    fprintf( stderr, "   -tq   (d)  # of tasks per queue: default (200) in code for SPLASH\n" ) ;
    fprintf( stderr, "   -ae   (f)  Area epsilon: default (2000.0) in code for SPLASH\n" ) ;
    fprintf( stderr, "   -pr   (d)  # of inter for parallel refinement: default (5) in code for SPLASH\n") ;
    fprintf( stderr, "   -pv   (d)  # of visibility comp in a task: default (4) in code for SPLASH\n") ;
    fprintf( stderr, "   -bf   (f)  BFepsilon (BF refinement): default (0.015) in code for SPLASH\n" ) ;
    fprintf( stderr, "   -en   (f)  Energy epsilon (convergence): default (0.005) in code for SPLASH\n" ) ;
    fprintf( stderr, "   -room      Use room model (default=test)\n" ) ;
    fprintf( stderr, "   -largeroom Use large room model\n" ) ;
    fprintf( stderr, "   -batch     Batch mode (use for SPLASH)\n" ) ;
    fprintf( stderr, "   -verbose   Verbose mode (don't use for SPLASH)\n" ) ;
    fprintf( stderr, "   -s   Measure per-process timing (don't use for SPLASH)\n" ) ;
}

