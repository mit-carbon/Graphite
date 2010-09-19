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

#define INPROCS      1024
#define IMAX         1026
#define JMAX         1026
#define MAX_LEVELS     11
#define MASTER          0
#define RED_ITER        0
#define BLACK_ITER      1
#define PAGE_SIZE    4096

EXTERN_ENV

struct global_struct {
   long id;
   long starttime;
   long trackstart;
   double psiai;
   double psibi;
};

struct fields_struct {
   double psi[2][IMAX][JMAX];
   double psim[2][IMAX][JMAX];
};

struct fields2_struct {
   double psium[IMAX][JMAX];
   double psilm[IMAX][JMAX];
};

struct wrk1_struct {
   double psib[IMAX][JMAX];
   double ga[IMAX][JMAX];
   double gb[IMAX][JMAX];
};

struct wrk3_struct {
   double work1[2][IMAX][JMAX];
   double work2[IMAX][JMAX];
};

struct wrk2_struct {
   double work3[IMAX][JMAX];
   double f[IMAX];
};

struct wrk4_struct {
   double work4[2][IMAX][JMAX];
   double work5[2][IMAX][JMAX];
};

struct wrk6_struct {
   double work6[IMAX][JMAX];
};

struct wrk5_struct {
   double work7[2][IMAX][JMAX];
   double temparray[2][IMAX][JMAX];
};

struct frcng_struct {
   double tauz[IMAX][JMAX];
};

struct iter_struct {
   long notdone;
   double work8[IMAX][JMAX];
   double work9[IMAX][JMAX];
};

struct guess_struct {
   double oldga[IMAX][JMAX];
   double oldgb[IMAX][JMAX];
};

struct multi_struct {
   double q_multi[MAX_LEVELS][IMAX][JMAX];
   double rhs_multi[MAX_LEVELS][IMAX][JMAX];
   double err_multi;
   long numspin;
   long spinflag[INPROCS];
};

struct locks_struct {
   LOCKDEC(idlock)
   LOCKDEC(psiailock)
   LOCKDEC(psibilock)
   LOCKDEC(donelock)
   LOCKDEC(error_lock)
   LOCKDEC(bar_lock)
};

struct bars_struct {
#if defined(MULTIPLE_BARRIERS)
   BARDEC(iteration)
   BARDEC(gsudn)
   BARDEC(p_setup)
   BARDEC(p_redph)
   BARDEC(p_soln)
   BARDEC(p_subph)
   BARDEC(sl_prini)
   BARDEC(sl_psini)
   BARDEC(sl_onetime)
   BARDEC(sl_phase_1)
   BARDEC(sl_phase_2)
   BARDEC(sl_phase_3)
   BARDEC(sl_phase_4)
   BARDEC(sl_phase_5)
   BARDEC(sl_phase_6)
   BARDEC(sl_phase_7)
   BARDEC(sl_phase_8)
   BARDEC(sl_phase_9)
   BARDEC(sl_phase_10)
   BARDEC(error_barrier)
#else
   BARDEC(barrier)
#endif
};

extern struct global_struct *global;
extern struct fields_struct *fields;
extern struct fields2_struct *fields2;
extern struct wrk1_struct *wrk1;
extern struct wrk3_struct *wrk3;
extern struct wrk2_struct *wrk2;
extern struct wrk4_struct *wrk4;
extern struct wrk6_struct *wrk6;
extern struct wrk5_struct *wrk5;
extern struct frcng_struct *frcng;
extern struct iter_struct *iter;
extern struct guess_struct *guess;
extern struct multi_struct *multi;
extern struct locks_struct *locks;
extern struct bars_struct *bars;

extern double eig2;
extern double ysca;
extern long jmm1;
extern double pi;
extern double t0;

extern long *procmap;
extern long xprocs;
extern long yprocs;

extern long numlev;
extern long imx[MAX_LEVELS];
extern long jmx[MAX_LEVELS];
extern double lev_res[MAX_LEVELS];
extern double lev_tol[MAX_LEVELS];
extern double maxwork;
extern long minlevel;
extern double outday0;
extern double outday1;
extern double outday2;
extern double outday3;

extern long nprocs;

extern double h1;
extern double h3;
extern double h;
extern double lf;
extern double res;
extern double dtau;
extern double f0;
extern double beta;
extern double gpr;
extern long im;
extern long jm;
extern long do_stats;
extern long do_output;
extern long *multi_times;
extern long *total_times;
extern double factjacob;
extern double factlap;

struct Global_Private {
  char pad[PAGE_SIZE];
  double multi_time;
  double total_time;
  long rel_start_x[MAX_LEVELS];
  long rel_start_y[MAX_LEVELS];
  long rel_num_x[MAX_LEVELS];
  long rel_num_y[MAX_LEVELS];
  long eist[MAX_LEVELS];
  long ejst[MAX_LEVELS];
  long oist[MAX_LEVELS];
  long ojst[MAX_LEVELS];
  long eiest[MAX_LEVELS];
  long ejest[MAX_LEVELS];
  long oiest[MAX_LEVELS];
  long ojest[MAX_LEVELS];
  long rlist[MAX_LEVELS];
  long rljst[MAX_LEVELS];
  long rlien[MAX_LEVELS];
  long rljen[MAX_LEVELS];
  long iist[MAX_LEVELS];
  long ijst[MAX_LEVELS];
  long iien[MAX_LEVELS];
  long ijen[MAX_LEVELS];
  long pist[MAX_LEVELS];
  long pjst[MAX_LEVELS];
  long pien[MAX_LEVELS];
  long pjen[MAX_LEVELS];
};

extern struct Global_Private *gp;

extern double i_int_coeff[MAX_LEVELS];
extern double j_int_coeff[MAX_LEVELS];
extern long minlev;

/*
 * jacobcalc.C
 */
void jacobcalc(double x[IMAX][JMAX], double y[IMAX][JMAX], double z[IMAX][JMAX], long pid, long firstrow, long lastrow, long firstcol, long lastcol, long numrows, long numcols);

/*
 * laplacalc.C
 */
void laplacalc(double x[IMAX][JMAX], double z[IMAX][JMAX], long firstrow, long lastrow, long firstcol, long lastcol, long numrows, long numcols);

/*
 * main.C
 */
long log_2(long number);
void printerr(char *s);

/*
 * multi.C
 */
void multig(long my_id);
void relax(long k, double *err, long color, long my_num);
void rescal(long kf, long my_num);
void intadd(long kc, long my_num);
void putz(long k, long my_num);

/*
 * slave1.C
 */
void slave(void);

/*
 * slave2.C
 */
void slave2(long procid, long firstrow, long lastrow, long numrows, long firstcol, long lastcol, long numcols);
