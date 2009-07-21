/*************************************************************************/
/*                                                                       */
/*  SPLASH Ocean Code                                                    */
/*                                                                       */
/*  This application studies the role of eddy and boundary currents in   */
/*  influencing large-scale ocean movements.  This implementation uses   */
/*  statically allocated two-dimensional arrays for grid data storage.   */
/*                                                                       */
/*  Command line options:                                                */
/*                                                                       */
/*     -nN : Simulate NxN ocean.  N must be (power of 2)+2.              */
/*     -pP : P = number of processors.  P must be power of 2.            */
/*     -eE : E = error tolerance for iterative relaxation.               */
/*     -rR : R = distance between grid points in meters.                 */
/*     -tT : T = timestep in seconds.                                    */
/*     -s  : Print timing statistics.                                    */
/*     -o  : Print out relaxation residual values.                       */
/*     -h  : Print out command line options.                             */
/*                                                                       */
/*  Default: OCEAN -n130 -p1 -e1e-7 -r20000.0 -t28800.0                  */
/*                                                                       */
/*  NOTE: This code works under both the FORK and SPROC models.          */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include "decs.h"

MAIN_ENV

#define DEFAULT_N      258
#define DEFAULT_P        1
#define DEFAULT_E        1e-7
#define DEFAULT_T    28800.0
#define DEFAULT_R    20000.0
#define INPROCS         16         /* Maximum number of processors */
#define IMAX           258
#define JMAX           258
#define MAX_LEVELS       9
#define PAGE_SIZE     4096

struct global_struct *global;
struct fields_struct *fields;
struct fields2_struct *fields2;
struct wrk1_struct *wrk1;
struct wrk3_struct *wrk3;
struct wrk2_struct *wrk2;
struct wrk4_struct *wrk4;
struct wrk6_struct *wrk6;
struct wrk5_struct *wrk5;
struct frcng_struct *frcng;
struct iter_struct *iter;
struct guess_struct *guess;
struct multi_struct *multi;
struct locks_struct *locks;
struct bars_struct *bars;

long startcol[2][INPROCS];
long nprocs = DEFAULT_P;
long startrow[2][INPROCS];
double h1 = 1000.0;
double h3 = 4000.0;
double h = 5000.0;
double lf = -5.12e11;
double eps = 0;
double res = DEFAULT_R;
double dtau = DEFAULT_T;
double f0 = 8.3e-5;
double beta = 2.0e-11;
double gpr = 0.02;
long im = DEFAULT_N;
long jm;
double tolerance = DEFAULT_E;
double eig2;
double ysca;
long jmm1;
double pi;
double t0 = 0.5e-4 ;
double outday0 = 1.0;
double outday1 = 2.0;
double outday2 = 2.0;
double outday3 = 2.0;
double factjacob;
double factlap;
long numlev;
long minlev;
long imx[MAX_LEVELS];
long jmx[MAX_LEVELS];
double lev_res[MAX_LEVELS];
double lev_tol[MAX_LEVELS];
double maxwork = 10000.0;

struct Global_Private *gp;

double i_int_coeff[MAX_LEVELS];
double j_int_coeff[MAX_LEVELS];
long xprocs;
long yprocs;
long do_stats = 0;
long do_output = 0;

int main(int argc, char *argv[])
{
   long i;
   long j;
   long xextra;
   long xportion;
   long yextra;
   long yportion;
   long lower;
   double procsqrt;
   long k;
   long logtest;
   long my_num;
   unsigned long computeend;
   double min_total;
   double max_total;
   double avg_total;
   double min_multi;
   double max_multi;
   double avg_multi;
   double min_frac;
   double max_frac;
   double avg_frac;
   extern char *optarg;
   long ch;
   unsigned long start;

   CLOCK(start)

   while ((ch = getopt(argc, argv, "n:p:e:r:t:soh")) != -1) {
     switch(ch) {
     case 'n': im = atoi(optarg);
               if (im > IMAX) {
                 printerr("Max grid size exceeded\n");
                 exit(-1);
               }
               if (log_2(im-2) == -1) {
                 printerr("Grid must be ((power of 2)+2) in each dimension\n");
                 exit(-1);
               }
               break;
     case 'p': nprocs = atoi(optarg);
               if (nprocs < 1) {
                 printerr("P must be >= 1\n");
                 exit(-1);
               }
               if (log_2(nprocs) == -1) {
                 printerr("P must be a power of 2\n");
                 exit(-1);
               }
               break;
     case 'e': tolerance = atof(optarg); break;
     case 'r': res = atof(optarg); break;
     case 't': dtau = atof(optarg); break;
     case 's': do_stats = !do_stats; break;
     case 'o': do_output = !do_output; break;
     case 'h': printf("Usage: OCEAN <options>\n\n");
               printf("options:\n");
               printf("  -nN : Simulate NxN ocean.  N must be (power of 2)+2.\n");
               printf("  -pP : P = number of processors.  P must be power of 2.\n");
               printf("  -eE : E = error tolerance for iterative relaxation.\n");
               printf("  -rR : R = distance between grid points in meters.\n");
               printf("  -tT : T = timestep in seconds.\n");
               printf("  -s  : Print timing statistics.\n");
               printf("  -o  : Print out relaxation residual values.\n");
               printf("  -h  : Print out command line options.\n\n");
               printf("Default: OCEAN -n%1d -p%1d -e%1g -r%1g -t%1g\n",
                       DEFAULT_N,DEFAULT_P,DEFAULT_E,DEFAULT_R,DEFAULT_T);
               exit(0);
               break;
     }
   }

   MAIN_INITENV(,60000000)

   logtest = im-2;
   numlev = 1;
   while (logtest != 1) {
     if (logtest%2 != 0) {
       printerr("Cannot determine number of multigrid levels\n");
       exit(-1);
     }
     logtest = logtest / 2;
     numlev++;
   }

   if (numlev > MAX_LEVELS) {
     printerr("Max grid levels exceeded for multigrid\n");
     exit(-1);
   }

   jm = im;
   printf("\n");
   printf("Ocean simulation with W-cycle multigrid solver\n");
   printf("    Processors                         : %1ld\n",nprocs);
   printf("    Grid size                          : %1ld x %1ld\n",im,jm);
   printf("    Grid resolution (meters)           : %0.2f\n",res);
   printf("    Time between relaxations (seconds) : %0.0f\n",dtau);
   printf("    Error tolerance                    : %0.7g\n",tolerance);
   printf("\n");

   gp = (struct Global_Private *) G_MALLOC((nprocs+1)*sizeof(struct Global_Private));
   for (i=0;i<nprocs;i++) {
     gp[i].multi_time = 0;
     gp[i].total_time = 0;
   }
   global = (struct global_struct *) G_MALLOC(sizeof(struct global_struct));
   fields = (struct fields_struct *) G_MALLOC(sizeof(struct fields_struct));
   fields2 = (struct fields2_struct *) G_MALLOC(sizeof(struct fields2_struct));
   wrk1 = (struct wrk1_struct *) G_MALLOC(sizeof(struct wrk1_struct));
   wrk3 = (struct wrk3_struct *) G_MALLOC(sizeof(struct wrk3_struct));
   wrk2 = (struct wrk2_struct *) G_MALLOC(sizeof(struct wrk2_struct));
   wrk4 = (struct wrk4_struct *) G_MALLOC(sizeof(struct wrk4_struct));
   wrk6 = (struct wrk6_struct *) G_MALLOC(sizeof(struct wrk6_struct));
   wrk5 = (struct wrk5_struct *) G_MALLOC(sizeof(struct wrk5_struct));
   frcng = (struct frcng_struct *) G_MALLOC(sizeof(struct frcng_struct));
   iter = (struct iter_struct *) G_MALLOC(sizeof(struct iter_struct));
   guess = (struct guess_struct *) G_MALLOC(sizeof(struct guess_struct));
   multi = (struct multi_struct *) G_MALLOC(sizeof(struct multi_struct));
   locks = (struct locks_struct *) G_MALLOC(sizeof(struct locks_struct));
   bars = (struct bars_struct *) G_MALLOC(sizeof(struct bars_struct));

   LOCKINIT(locks->idlock)
   LOCKINIT(locks->psiailock)
   LOCKINIT(locks->psibilock)
   LOCKINIT(locks->donelock)
   LOCKINIT(locks->error_lock)
   LOCKINIT(locks->bar_lock)

#if defined(MULTIPLE_BARRIERS)
   BARINIT(bars->iteration, nprocs)
   BARINIT(bars->gsudn, nprocs)
   BARINIT(bars->p_setup, nprocs)
   BARINIT(bars->p_redph, nprocs)
   BARINIT(bars->p_soln, nprocs)
   BARINIT(bars->p_subph, nprocs)
   BARINIT(bars->sl_prini, nprocs)
   BARINIT(bars->sl_psini, nprocs)
   BARINIT(bars->sl_onetime, nprocs)
   BARINIT(bars->sl_phase_1, nprocs)
   BARINIT(bars->sl_phase_2, nprocs)
   BARINIT(bars->sl_phase_3, nprocs)
   BARINIT(bars->sl_phase_4, nprocs)
   BARINIT(bars->sl_phase_5, nprocs)
   BARINIT(bars->sl_phase_6, nprocs)
   BARINIT(bars->sl_phase_7, nprocs)
   BARINIT(bars->sl_phase_8, nprocs)
   BARINIT(bars->sl_phase_9, nprocs)
   BARINIT(bars->sl_phase_10, nprocs)
   BARINIT(bars->error_barrier, nprocs)
#else
   BARINIT(bars->barrier, nprocs)
#endif

   imx[numlev-1] = im;
   jmx[numlev-1] = jm;
   lev_res[numlev-1] = res;
   lev_tol[numlev-1] = tolerance;
   multi->err_multi = 0.0;
   multi->numspin = 0;
   for (i=0;i<nprocs;i++) {
     multi->spinflag[i] = 0;
   }

   for (i=numlev-2;i>=0;i--) {
     imx[i] = ((imx[i+1] - 2) / 2) + 2;
     jmx[i] = ((jmx[i+1] - 2) / 2) + 2;
     lev_res[i] = lev_res[i+1] * 2;
   }

   xprocs = 0;
   yprocs = 0;
   procsqrt = sqrt((double) nprocs);
   j = (long) procsqrt;
   while ((xprocs == 0) && (j > 0)) {
     k = nprocs / j;
     if (k * j == nprocs) {
       if (k > j) {
         xprocs = j;
         yprocs = k;
       } else {
         xprocs = k;
         yprocs = j;
       }
     }
     j--;
   }
   if (xprocs == 0) {
     printerr("Could not find factors for subblocking\n");
     exit(-1);
   }

/* Determine starting coord and number of points to process in     */
/* each direction                                                  */

   for (i=0;i<numlev;i++) {
     xportion = (jmx[i] - 2) / xprocs;
     xextra = (jmx[i] - 2) % xprocs;
     for (j=0;j<xprocs;j++) {
       if (xextra == 0) {
         for (k=0;k<yprocs;k++) {
           gp[k*xprocs+j].rel_start_x[i] = j * xportion + 1;
           gp[k*xprocs+j].rel_num_x[i] = xportion;
         }
       } else {
         if (j + 1 > xextra) {
           for (k=0;k<yprocs;k++) {
             lower = xextra * (xportion + 1);
             gp[k*xprocs+j].rel_start_x[i] = lower + (j - xextra) * xportion + 1;
             gp[k*xprocs+j].rel_num_x[i] = xportion;
           }
         } else {
           for (k=0;k<yprocs;k++) {
             gp[k*xprocs+j].rel_start_x[i] = j * (xportion + 1) + 1;
             gp[k*xprocs+j].rel_num_x[i] = xportion + 1;
           }
         }
       }
     }
     yportion = (imx[i] - 2) / yprocs;
     yextra = (imx[i] - 2) % yprocs;
     for (j=0;j<yprocs;j++) {
       if (yextra == 0) {
         for (k=0;k<xprocs;k++) {
           gp[j*xprocs+k].rel_start_y[i] = j * yportion + 1;
           gp[j*xprocs+k].rel_num_y[i] = yportion;
         }
       } else {
         if (j + 1 > yextra) {
           for (k=0;k<xprocs;k++) {
             lower = yextra * (yportion + 1);
             gp[j*xprocs+k].rel_start_y[i] = lower + (j - yextra) * yportion + 1;
             gp[j*xprocs+k].rel_num_y[i] = yportion;
           }
         } else {
           for (k=0;k<xprocs;k++) {
             gp[j*xprocs+k].rel_start_y[i] = j * (yportion + 1) + 1;
             gp[j*xprocs+k].rel_num_y[i] = yportion + 1;
           }
         }
       }
     }
   }

   i_int_coeff[0] = 0.0;
   j_int_coeff[0] = 0.0;
   for (i=0;i<numlev;i++) {
     i_int_coeff[i] = 1.0/(imx[i]-1);
     j_int_coeff[i] = 1.0/(jmx[i]-1);
   }

   for (my_num=0;my_num<nprocs;my_num++) {
     for (i=0;i<numlev;i++) {
       gp[my_num].rlist[i] = gp[my_num].rel_start_y[i];
       gp[my_num].rljst[i] = gp[my_num].rel_start_x[i];
       gp[my_num].rlien[i] = gp[my_num].rlist[i] + gp[my_num].rel_num_y[i] - 1;
       gp[my_num].rljen[i] = gp[my_num].rljst[i] + gp[my_num].rel_num_x[i] - 1;
       gp[my_num].iist[i] = gp[my_num].rel_start_y[i];
       gp[my_num].ijst[i] = gp[my_num].rel_start_x[i];
       gp[my_num].iien[i] = gp[my_num].iist[i] + gp[my_num].rel_num_y[i] - 1;
       gp[my_num].ijen[i] = gp[my_num].ijst[i] + gp[my_num].rel_num_x[i] - 1;
       gp[my_num].pist[i] = gp[my_num].rel_start_y[i];
       gp[my_num].pjst[i] = gp[my_num].rel_start_x[i];
       gp[my_num].pien[i] = gp[my_num].pist[i] + gp[my_num].rel_num_y[i] - 1;
       gp[my_num].pjen[i] = gp[my_num].pjst[i] + gp[my_num].rel_num_x[i] - 1;

       if (gp[my_num].pist[i] == 1) {
         gp[my_num].pist[i] = 0;
       }
       if (gp[my_num].pjst[i] == 1) {
         gp[my_num].pjst[i] = 0;
       }
       if (gp[my_num].pien[i] == imx[i] - 2) {
         gp[my_num].pien[i] = imx[i]-1;
       }
       if (gp[my_num].pjen[i] == jmx[i] - 2) {
         gp[my_num].pjen[i] = jmx[i]-1;
       }

       if (gp[my_num].rlist[i] % 2 == 0) {
         gp[my_num].eist[i] = gp[my_num].rlist[i];
         gp[my_num].oist[i] = gp[my_num].rlist[i] + 1;
       } else {
         gp[my_num].eist[i] = gp[my_num].rlist[i] + 1;
         gp[my_num].oist[i] = gp[my_num].rlist[i];
       }
       if (gp[my_num].rljst[i] % 2 == 0) {
         gp[my_num].ejst[i] = gp[my_num].rljst[i];
         gp[my_num].ojst[i] = gp[my_num].rljst[i] + 1;
       } else {
         gp[my_num].ejst[i] = gp[my_num].rljst[i] + 1;
         gp[my_num].ojst[i] = gp[my_num].rljst[i];
       }
       if (gp[my_num].rlien[i] == imx[i]-2) {
         gp[my_num].rlien[i] = gp[my_num].rlien[i] - 1;
         if (gp[my_num].rlien[i] % 2 == 0) {
           gp[my_num].ojest[i] = gp[my_num].ojst[i];
           gp[my_num].ejest[i] = gp[my_num].ejst[i];
         } else {
           gp[my_num].ojest[i] = gp[my_num].ejst[i];
           gp[my_num].ejest[i] = gp[my_num].ojst[i];
         }
       }
       if (gp[my_num].rljen[i] == jmx[i]-2) {
         gp[my_num].rljen[i] = gp[my_num].rljen[i] - 1;
         if (gp[my_num].rljen[i] % 2 == 0) {
           gp[my_num].oiest[i] = gp[my_num].oist[i];
           gp[my_num].eiest[i] = gp[my_num].eist[i];
         } else {
           gp[my_num].oiest[i] = gp[my_num].eist[i];
           gp[my_num].eiest[i] = gp[my_num].oist[i];
         }
       }
     }
   }

/* initialize constants and variables

   id is a global shared variable that has fetch-and-add operations
   performed on it by processes to obtain their pids.   */

   global->id = 0;
   global->psibi = 0.0;
   pi = atan(1.0);
   pi = 4.*pi;

   factjacob = -1./(12.*res*res);
   factlap = 1./(res*res);
   eig2 = -h*f0*f0/(h1*h3*gpr);
   jmm1 = jm-1 ;
   ysca = ((double) jmm1)*res ;
   for (i=0;i<im;i++) {
     for (j=0;j<jm;j++) {
       guess->oldga[i][j] = 0.0;
       guess->oldgb[i][j] = 0.0;
     }
   }

   if (do_output) {
     printf("                       MULTIGRID OUTPUTS\n");
   }

   CREATE(slave, nprocs);
   WAIT_FOR_END(nprocs);
   CLOCK(computeend)

   printf("\n");
   printf("                       PROCESS STATISTICS\n");
   printf("                  Total          Multigrid         Multigrid\n");
   printf(" Proc             Time             Time            Fraction\n");
   printf("    0   %15.0f    %15.0f        %10.3f\n", gp[0].total_time,gp[0].multi_time, gp[0].multi_time/gp[0].total_time);

   if (do_stats) {
     min_total = max_total = avg_total = gp[0].total_time;
     min_multi = max_multi = avg_multi = gp[0].multi_time;
     min_frac = max_frac = avg_frac = gp[0].multi_time/gp[0].total_time;
     for (i=1;i<nprocs;i++) {
       if (gp[i].total_time > max_total) {
         max_total = gp[i].total_time;
       }
       if (gp[i].total_time < min_total) {
         min_total = gp[i].total_time;
       }
       if (gp[i].multi_time > max_multi) {
         max_multi = gp[i].multi_time;
       }
       if (gp[i].multi_time < min_multi) {
         min_multi = gp[i].multi_time;
       }
       if (gp[i].multi_time/gp[i].total_time > max_frac) {
         max_frac = gp[i].multi_time/gp[i].total_time;
       }
       if (gp[i].multi_time/gp[i].total_time < min_frac) {
         min_frac = gp[i].multi_time/gp[i].total_time;
       }
       avg_total += gp[i].total_time;
       avg_multi += gp[i].multi_time;
       avg_frac += gp[i].multi_time/gp[i].total_time;
     }
     avg_total = avg_total / nprocs;
     avg_multi = avg_multi / nprocs;
     avg_frac = avg_frac / nprocs;
     for (i=1;i<nprocs;i++) {
       printf("  %3ld   %15.0f    %15.0f        %10.3f\n", i, gp[i].total_time, gp[i].multi_time, gp[i].multi_time/gp[i].total_time);
     }
     printf("  Avg   %15.0f    %15.0f        %10.3f\n", avg_total,avg_multi,avg_frac);
     printf("  Min   %15.0f    %15.0f        %10.3f\n", min_total,min_multi,min_frac);
     printf("  Max   %15.0f    %15.0f        %10.3f\n", max_total,max_multi,max_frac);
   }
   printf("\n");

   global->starttime = start;
   printf("                       TIMING INFORMATION\n");
   printf("Start time                        : %16lu\n", global->starttime);
   printf("Initialization finish time        : %16lu\n", global->trackstart);
   printf("Overall finish time               : %16lu\n", computeend);
   printf("Total time with initialization    : %16lu\n", computeend-global->starttime);
   printf("Total time without initialization : %16lu\n", computeend-global->trackstart);
   printf("    (excludes first timestep)\n");
   printf("\n");

   MAIN_END
}

long log_2(long number)
{
  long cumulative = 1;
  long out = 0;
  long done = 0;

  while ((cumulative < number) && (!done) && (out < 50)) {
    if (cumulative == number) {
      done = 1;
    } else {
      cumulative = cumulative * 2;
      out ++;
    }
  }

  if (cumulative == number) {
    return(out);
  } else {
    return(-1);
  }
}

void printerr(char *s)
{
  fprintf(stderr,"ERROR: %s\n",s);
}
