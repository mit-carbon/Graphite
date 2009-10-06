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

#define MASTER            0
#define RED_ITER          0
#define BLACK_ITER        1
#define UP                0
#define DOWN              1
#define LEFT              2
#define RIGHT             3
#define UPLEFT            4
#define UPRIGHT           5
#define DOWNLEFT          6
#define DOWNRIGHT         7
#define PAGE_SIZE      4096

struct multi_struct {
   double err_multi;
};

extern struct multi_struct *multi;

struct global_struct {
   long id;
   long starttime;
   long trackstart;
   double psiai;
   double psibi;
};

extern struct global_struct *global;

extern double eig2;
extern double ysca;
extern long jmm1;
extern double pi;
extern double t0;

extern double ****psi;
extern double ****psim;
extern double ***psium;
extern double ***psilm;
extern double ***psib;
extern double ***ga;
extern double ***gb;
extern double ****work1;
extern double ***work2;
extern double ***work3;
extern double ****work4;
extern double ****work5;
extern double ***work6;
extern double ****work7;
extern double ****temparray;
extern double ***tauz;
extern double ***oldga;
extern double ***oldgb;
extern double *f;
extern double ****q_multi;
extern double ****rhs_multi;

struct locks_struct {
   LOCKDEC(idlock)
   LOCKDEC(psiailock)
   LOCKDEC(psibilock)
   LOCKDEC(donelock)
   LOCKDEC(error_lock)
   LOCKDEC(bar_lock)
};

extern struct locks_struct *locks;

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

extern struct bars_struct *bars;

extern double factjacob;
extern double factlap;

struct Global_Private {
  char pad[PAGE_SIZE];
  long *rel_num_x;
  long *rel_num_y;
  long *eist;
  long *ejst;
  long *oist;
  long *ojst;
  long *rlist;
  long *rljst;
  long *rlien;
  long *rljen;
  long rownum;
  long colnum;
  long neighbors[8];
  double multi_time;
  double total_time;
};

extern struct Global_Private *gp;

extern double *i_int_coeff;
extern double *j_int_coeff;
extern long xprocs;
extern long yprocs;

extern long numlev;
extern long *imx;
extern long *jmx;
extern double *lev_res;
extern double *lev_tol;
extern double maxwork;
extern long *xpts_per_proc;
extern long *ypts_per_proc;
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

/*
 * jacobcalc.C
 */
void jacobcalc(double ***x, double ***y, double ***z, long pid, long firstrow, long lastrow, long firstcol, long lastcol);

/*
 * jacobcalc2.C
 */
void jacobcalc2(double ****x, double ****y, double ****z, long psiindex, long pid, long firstrow, long lastrow, long firstcol, long lastcol);

/*
 * laplacalc.C
 */
void laplacalc(long procid, double ****x, double ****z, long psiindex, long firstrow, long lastrow, long firstcol, long lastcol);

/*
 * linkup.C
 */
void link_all(void);
void linkup(double **row_ptr);
void link_multi(void);

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
void copy_borders(long k, long pid);
void copy_rhs_borders(long k, long procid);
void copy_red(long k, long procid);
void copy_black(long k, long procid);

/*
 * slave1.C
 */
void slave(void);

/*
 * slave2.C
 */
void slave2(long procid, long firstrow, long lastrow, long numrows, long firstcol, long lastcol, long numcols);

/*
 * subblock.C
 */
void subblock(void);
