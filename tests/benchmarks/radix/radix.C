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

/*************************************************************************/
/*                                                                       */
/*  Integer radix sort of non-negative integers.                         */
/*                                                                       */
/*  Command line options:                                                */
/*                                                                       */
/*  -pP : P = number of processors.                                      */
/*  -rR : R = radix for sorting.  Must be power of 2.                    */
/*  -nN : N = number of keys to sort.                                    */
/*  -mM : M = maximum key value.  Integer keys k will be generated such  */
/*        that 0 <= k <= M.                                              */
/*  -s  : Print individual processor timing statistics.                  */
/*  -t  : Check to make sure all keys are sorted correctly.              */
/*  -o  : Print out sorted keys.                                         */
/*  -h  : Print out command line options.                                */
/*                                                                       */
/*  Default: RADIX -p1 -n262144 -r1024 -m524288                          */
/*                                                                       */
/*  Note: This version works under both the FORK and SPROC models        */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>
#include <math.h>
#include <unistd.h>

#define DEFAULT_P                    1
#define DEFAULT_N               262144
#define DEFAULT_R                 1024 
#define DEFAULT_M             67108864
#define MAX_PROCESSORS            1024    
#define RADIX_S                8388608.0e0
#define RADIX           70368744177664.0e0
#define SEED                 314159265.0e0
#define RATIO               1220703125.0e0
#define PAGE_SIZE                 4096
#define PAGE_MASK     (~(PAGE_SIZE-1))
#define MAX_RADIX                 4096

MAIN_ENV

struct prefix_node {
   long densities[MAX_RADIX];
   long ranks[MAX_RADIX];
   PAUSEDEC(done)
   char pad[PAGE_SIZE];
};

struct global_memory {
   long Index;                             /* process ID */
   LOCKDEC(lock_Index)                    /* for fetch and add to get ID */
   LOCKDEC(rank_lock)                     /* for fetch and add to get ID */
/*   ALOCKDEC(section_lock,MAX_PROCESSORS)*/  /* key locks */
   BARDEC(barrier_rank)                   /* for ranking process */
   BARDEC(barrier_key)                    /* for key sorting process */
   double *ranktime;
   double *sorttime;
   double *totaltime;
   long final;
   unsigned long starttime;
   unsigned long rs;
   unsigned long rf;
   struct prefix_node prefix_tree[2 * MAX_PROCESSORS];
} *global;

struct global_private {
  char pad[PAGE_SIZE];
  long *rank_ff;         /* overall processor ranks */
} gp[MAX_PROCESSORS];

long *key[2];            /* sort from one index into the other */
long **rank_me;          /* individual processor ranks */
long *key_partition;     /* keys a processor works on */
long *rank_partition;    /* ranks a processor works on */

long number_of_processors = DEFAULT_P;
long max_num_digits;
long radix = DEFAULT_R;
long num_keys = DEFAULT_N;
long max_key = DEFAULT_M;
long log2_radix;
long log2_keys;
long dostats = 0;
long test_result = 0;
long doprint = 0;

void slave_sort(void);
double product_mod_46(double t1, double t2);
double ran_num_init(unsigned long k, double b, double t);
long get_max_digits(long max_key);
long get_log2_radix(long rad);
long get_log2_keys(long num_keys);
long log_2(long number);
void printerr(char *s);
void init(long key_start, long key_stop, long from);
void test_sort(long final);
void printout(void);

int main(int argc, char *argv[])
{
   long i;
   long p;
   long quotient;
   long remainder;
   long sum_i; 
   long sum_f;
   long size;
   long **temp;
   long **temp2;
   long *a;
   long c;
   double mint, maxt, avgt;
   double minrank, maxrank, avgrank;
   double minsort, maxsort, avgsort;
   unsigned long start;

   extern char *optarg;


   CLOCK(start)

   while ((c = getopt(argc, argv, "p:r:n:m:stoh")) != -1) {
     switch(c) {
      case 'p': number_of_processors = atoi(optarg);
                if (number_of_processors < 1) {
                  printerr("P must be >= 1\n");
                  exit(-1);
                }
                if (number_of_processors > MAX_PROCESSORS) {
                  printerr("Maximum processors (MAX_PROCESSORS) exceeded\n"); 
		  exit(-1);
		}
                break;
      case 'r': radix = atoi(optarg);
                if (radix < 1) {
                  printerr("Radix must be a power of 2 greater than 0\n");
                  exit(-1);
                }
                log2_radix = log_2(radix);
                if (log2_radix == -1) {
                  printerr("Radix must be a power of 2\n");
                  exit(-1);
                }
                break;
      case 'n': num_keys = atoi(optarg);
                if (num_keys < 1) {
                  printerr("Number of keys must be >= 1\n");
                  exit(-1);
                }
                break;
      case 'm': max_key = atoi(optarg);
                if (max_key < 1) {
                  printerr("Maximum key must be >= 1\n");
                  exit(-1);
                }
                break;
      case 's': dostats = !dostats;
                break;
      case 't': test_result = !test_result;
                break;
      case 'o': doprint = !doprint;
                break;
      case 'h': printf("Usage: RADIX <options>\n\n");
                printf("   -pP : P = number of processors.\n");
                printf("   -rR : R = radix for sorting.  Must be power of 2.\n");
                printf("   -nN : N = number of keys to sort.\n");
                printf("   -mM : M = maximum key value.  Integer keys k will be generated such\n");
                printf("         that 0 <= k <= M.\n");
                printf("   -s  : Print individual processor timing statistics.\n");
                printf("   -t  : Check to make sure all keys are sorted correctly.\n");
                printf("   -o  : Print out sorted keys.\n");
                printf("   -h  : Print out command line options.\n\n");
                printf("Default: RADIX -p%1d -n%1d -r%1d -m%1d\n",
                        DEFAULT_P,DEFAULT_N,DEFAULT_R,DEFAULT_M);
		exit(0);
     }
   }

   MAIN_INITENV(,80000000)

   log2_radix = log_2(radix); 
   log2_keys = log_2(num_keys);
   global = (struct global_memory *) G_MALLOC(sizeof(struct global_memory));
   if (global == NULL) {
	   fprintf(stderr,"ERROR: Cannot malloc enough memory for global\n");
	   exit(-1);
   }
   key[0] = (long *) G_MALLOC(num_keys*sizeof(long));
   key[1] = (long *) G_MALLOC(num_keys*sizeof(long));
   key_partition = (long *) G_MALLOC((number_of_processors+1)*sizeof(long));
   rank_partition = (long *) G_MALLOC((number_of_processors+1)*sizeof(long));
   global->ranktime = (double *) G_MALLOC(number_of_processors*sizeof(double));
   global->sorttime = (double *) G_MALLOC(number_of_processors*sizeof(double));
   global->totaltime = (double *) G_MALLOC(number_of_processors*sizeof(double));
   size = number_of_processors*(radix*sizeof(long)+sizeof(long *));
   rank_me = (long **) G_MALLOC(size);
   if ((key[0] == NULL) || (key[1] == NULL) || (key_partition == NULL) || (rank_partition == NULL) || 
       (global->ranktime == NULL) || (global->sorttime == NULL) || (global->totaltime == NULL) || (rank_me == NULL)) {
     fprintf(stderr,"ERROR: Cannot malloc enough memory\n");
     exit(-1); 
   }

   temp = rank_me;
   temp2 = temp + number_of_processors;
   a = (long *) temp2;
   for (i=0;i<number_of_processors;i++) {
     *temp = (long *) a;
     temp++;
     a += radix;
   }
   for (i=0;i<number_of_processors;i++) {
     gp[i].rank_ff = (long *) G_MALLOC(radix*sizeof(long)+PAGE_SIZE);
   }
   LOCKINIT(global->lock_Index)
   LOCKINIT(global->rank_lock)
/*   ALOCKINIT(global->section_lock,MAX_PROCESSORS)*/
   BARINIT(global->barrier_rank, number_of_processors)
   BARINIT(global->barrier_key, number_of_processors)
   
   for (i=0; i<2*number_of_processors; i++) {
     PAUSEINIT(global->prefix_tree[i].done);
   }

   global->Index = 0;
   max_num_digits = get_max_digits(max_key);
   printf("\n");
   printf("Integer Radix Sort\n");
   printf("     %ld Keys\n",num_keys);
   printf("     %ld Processors\n",number_of_processors);
   printf("     Radix = %ld\n",radix);
   printf("     Max key = %ld\n",max_key);
   printf("\n");

   quotient = num_keys / number_of_processors;
   remainder = num_keys % number_of_processors;
   sum_i = 0;
   sum_f = 0;
   p = 0;
   while (sum_i < num_keys) {
      key_partition[p] = sum_i;
      p++;
      sum_i = sum_i + quotient;
      sum_f = sum_f + remainder;
      sum_i = sum_i + sum_f / number_of_processors;
      sum_f = sum_f % number_of_processors;
   }
   key_partition[p] = num_keys;

   quotient = radix / number_of_processors;
   remainder = radix % number_of_processors;
   sum_i = 0;
   sum_f = 0;
   p = 0;
   while (sum_i < radix) {
      rank_partition[p] = sum_i;
      p++;
      sum_i = sum_i + quotient;
      sum_f = sum_f + remainder;
      sum_i = sum_i + sum_f / number_of_processors;
      sum_f = sum_f % number_of_processors;
   }
   rank_partition[p] = radix;

/* POSSIBLE ENHANCEMENT:  Here is where one might distribute the key,
   rank_me, rank, and gp data structures across physically 
   distributed memories as desired. 
   
   One way to place data is as follows:  
      
   for (i=0;i<number_of_processors;i++) {
     Place all addresses x such that:
       &(key[0][key_partition[i]]) <= x < &(key[0][key_partition[i+1]])
            on node i
       &(key[1][key_partition[i]]) <= x < &(key[1][key_partition[i+1]])
            on node i
       &(rank_me[i][0]) <= x < &(rank_me[i][radix-1]) on node i
       &(gp[i]) <= x < &(gp[i+1]) on node i
       &(gp[i].rank_ff[0]) <= x < &(gp[i].rank_ff[radix]) on node i
   }
   start_p = 0;
   i = 0;

   for (toffset = 0; toffset < number_of_processors; toffset ++) {
     offset = toffset;
     level = number_of_processors >> 1;
     base = number_of_processors;
     while ((offset & 0x1) != 0) {
       offset >>= 1;
       index = base + offset;
       Place all addresses x such that:
         &(global->prefix_tree[index]) <= x < 
              &(global->prefix_tree[index + 1]) on node toffset
       base += level;
       level >>= 1;
     }  
   }  */

   /* Fill the random-number array. */
   
   // Enable Architectural Models
   CarbonEnableModels();

   CREATE(slave_sort, number_of_processors);
   WAIT_FOR_END(number_of_processors);

   // Disable Architectural Models
   CarbonDisableModels();

   printf("\n");
   printf("                 PROCESS STATISTICS\n");
   printf("               Total            Rank            Sort\n");
   printf(" Proc          Time             Time            Time\n");
   printf("    0     %10.0f      %10.0f      %10.0f\n",
           global->totaltime[0],global->ranktime[0],
           global->sorttime[0]);
   if (dostats) {
     maxt = avgt = mint = global->totaltime[0];
     maxrank = avgrank = minrank = global->ranktime[0];
     maxsort = avgsort = minsort = global->sorttime[0];
     for (i=1; i<number_of_processors; i++) {
       if (global->totaltime[i] > maxt) {
         maxt = global->totaltime[i];
       }
       if (global->totaltime[i] < mint) {
         mint = global->totaltime[i];
       }
       if (global->ranktime[i] > maxrank) {
         maxrank = global->ranktime[i];
       }
       if (global->ranktime[i] < minrank) {
         minrank = global->ranktime[i];
       }
       if (global->sorttime[i] > maxsort) {
         maxsort = global->sorttime[i];
       }
       if (global->sorttime[i] < minsort) {
         minsort = global->sorttime[i];
       }
       avgt += global->totaltime[i];
       avgrank += global->ranktime[i];
       avgsort += global->sorttime[i];
     }
     avgt = avgt / number_of_processors;
     avgrank = avgrank / number_of_processors;
     avgsort = avgsort / number_of_processors;
     for (i=1; i<number_of_processors; i++) {
       printf("  %3ld     %10.0f      %10.0f      %10.0f\n",
               i,global->totaltime[i],global->ranktime[i],
               global->sorttime[i]);
     }
     printf("  Avg     %10.0f      %10.0f      %10.0f\n",avgt,avgrank,avgsort);
     printf("  Min     %10.0f      %10.0f      %10.0f\n",mint,minrank,minsort);
     printf("  Max     %10.0f      %10.0f      %10.0f\n",maxt,maxrank,maxsort);
     printf("\n");
   }

   printf("\n");
   global->starttime = start;
   printf("                 TIMING INFORMATION\n");
   printf("Start time                        : %16lu\n",
           global->starttime);
   printf("Initialization finish time        : %16lu\n",
           global->rs);
   printf("Overall finish time               : %16lu\n",
           global->rf);
   printf("Total time with initialization    : %16lu\n",
           global->rf-global->starttime);
   printf("Total time without initialization : %16lu\n",
           global->rf-global->rs);
   printf("\n");

   if (doprint) {
     printout();
   }
   if (test_result) {
     test_sort(global->final);  
   }
  
   MAIN_END;
}

void slave_sort()
{
   long i;
   long MyNum;
   long this_key;
   long tmp;
   long loopnum;
   long shiftnum;
   long bb;
   long my_key;
   long key_start;
   long key_stop;
   long rank_start;
   long rank_stop;
   long from=0;
   long to=1;
   long *key_density;       /* individual processor key densities */
   unsigned long time1;
   unsigned long time2;
   unsigned long time3;
   unsigned long time4;
   unsigned long time5;
   unsigned long time6;
   double ranktime=0;
   double sorttime=0;
   long *key_from;
   long *key_to;
   long *rank_me_mynum;
   long *rank_ff_mynum;
   long stats;
   struct prefix_node* n;
   struct prefix_node* r;
   struct prefix_node* l;
   struct prefix_node* my_node;
   struct prefix_node* their_node;
   long index;
   long level;
   long base;
   long offset;

   stats = dostats;

   LOCK(global->lock_Index)
     MyNum = global->Index;
     global->Index++;
   UNLOCK(global->lock_Index)

   BARINCLUDE(global->barrier_key);
   BARINCLUDE(global->barrier_rank);

/* POSSIBLE ENHANCEMENT:  Here is where one might pin processes to
   processors to avoid migration */

   key_density = (long *) G_MALLOC(radix*sizeof(long));

   /* Fill the random-number array. */

   key_start = key_partition[MyNum];
   key_stop = key_partition[MyNum + 1];
   rank_start = rank_partition[MyNum];
   rank_stop = rank_partition[MyNum + 1];
   if (rank_stop == radix) {
     rank_stop--;
   }

   init(key_start,key_stop,from);

   BARRIER(global->barrier_key, number_of_processors) 

/* POSSIBLE ENHANCEMENT:  Here is where one might reset the
   statistics that one is measuring about the parallel execution */
   
   BARRIER(global->barrier_key, number_of_processors) 

   if ((MyNum == 0) || (stats)) {
     CLOCK(time1)
   }

/* Do 1 iteration per digit.  */

   rank_me_mynum = rank_me[MyNum];
   rank_ff_mynum = gp[MyNum].rank_ff;
   for (loopnum=0;loopnum<max_num_digits;loopnum++) {
     shiftnum = (loopnum * log2_radix);
     bb = (radix-1) << shiftnum;

/* generate histograms based on one digit */

     if ((MyNum == 0) || (stats)) {
       CLOCK(time2)
     }

     for (i = 0; i < radix; i++) {
       rank_me_mynum[i] = 0;
     }  
     key_from = (long *) key[from];
     key_to = (long *) key[to];
     for (i=key_start;i<key_stop;i++) {
       my_key = key_from[i] & bb;
       my_key = my_key >> shiftnum;  
       rank_me_mynum[my_key]++;
     }
     key_density[0] = rank_me_mynum[0]; 
     for (i=1;i<radix;i++) {
       key_density[i] = key_density[i-1] + rank_me_mynum[i];  
     }

     BARRIER(global->barrier_rank, number_of_processors)  

     n = &(global->prefix_tree[MyNum]);
     for (i = 0; i < radix; i++) {
        n->densities[i] = key_density[i];
        n->ranks[i] = rank_me_mynum[i];
     }
     offset = MyNum;
     level = number_of_processors >> 1;
     base = number_of_processors;
     if ((MyNum & 0x1) == 0) {
        SETPAUSE(global->prefix_tree[base + (offset >> 1)].done);
     }
     while ((offset & 0x1) != 0) {
       offset >>= 1;
       r = n;
       l = n - 1;
       index = base + offset;
       n = &(global->prefix_tree[index]);
       WAITPAUSE(n->done);
       CLEARPAUSE(n->done);
       if (offset != (level - 1)) {
         for (i = 0; i < radix; i++) {
           n->densities[i] = r->densities[i] + l->densities[i];
           n->ranks[i] = r->ranks[i] + l->ranks[i];
         }
       } else {
         for (i = 0; i < radix; i++) {
           n->densities[i] = r->densities[i] + l->densities[i];
         }
       }
       base += level;
       level >>= 1;
       if ((offset & 0x1) == 0) {
         SETPAUSE(global->prefix_tree[base + (offset >> 1)].done);
       }
     }
     BARRIER(global->barrier_rank, number_of_processors);

     if (MyNum != (number_of_processors - 1)) {
       offset = MyNum;
       level = number_of_processors;
       base = 0;
       while ((offset & 0x1) != 0) {
         offset >>= 1;
         base += level;
         level >>= 1;
       }
       my_node = &(global->prefix_tree[base + offset]);
       offset >>= 1;
       base += level;
       level >>= 1;
       while ((offset & 0x1) != 0) {
         offset >>= 1;
         base += level;
         level >>= 1;
       }
       their_node = &(global->prefix_tree[base + offset]);
       WAITPAUSE(my_node->done);
       CLEARPAUSE(my_node->done);
       for (i = 0; i < radix; i++) {
         my_node->densities[i] = their_node->densities[i];
       }
     } else {
       my_node = &(global->prefix_tree[(2 * number_of_processors) - 2]);
     }
     offset = MyNum;
     level = number_of_processors;
     base = 0;
     while ((offset & 0x1) != 0) {
       SETPAUSE(global->prefix_tree[base + offset - 1].done);
       offset >>= 1;
       base += level;
       level >>= 1;
     }
     offset = MyNum;
     level = number_of_processors;
     base = 0;
     for(i = 0; i < radix; i++) {
       rank_ff_mynum[i] = 0;
     }
     while (offset != 0) {
       if ((offset & 0x1) != 0) {
         /* Add ranks of node to your left at this level */
         l = &(global->prefix_tree[base + offset - 1]);
         for (i = 0; i < radix; i++) {
           rank_ff_mynum[i] += l->ranks[i];
         }
       }
       base += level;
       level >>= 1;
       offset >>= 1;
     }
     for (i = 1; i < radix; i++) {
       rank_ff_mynum[i] += my_node->densities[i - 1];
     }

     if ((MyNum == 0) || (stats)) {
       CLOCK(time3);
     }

     BARRIER(global->barrier_rank, number_of_processors);

     if ((MyNum == 0) || (stats)) {
       CLOCK(time4);
     }

     /* put it in order according to this digit */

     for (i = key_start; i < key_stop; i++) {  
       this_key = key_from[i] & bb;
       this_key = this_key >> shiftnum;  
       tmp = rank_ff_mynum[this_key];
       key_to[tmp] = key_from[i];
       rank_ff_mynum[this_key]++;
     }   /*  i */  

     if ((MyNum == 0) || (stats)) {
       CLOCK(time5);
     }

     if (loopnum != max_num_digits-1) {
       from = from ^ 0x1;
       to = to ^ 0x1;
     }

     BARRIER(global->barrier_rank, number_of_processors)

     if ((MyNum == 0) || (stats)) {
       ranktime += (time3 - time2);
       sorttime += (time5 - time4);
     }
   } /* for */

   BARRIER(global->barrier_rank, number_of_processors)
   if ((MyNum == 0) || (stats)) {
     CLOCK(time6)
     global->ranktime[MyNum] = ranktime;
     global->sorttime[MyNum] = sorttime;
     global->totaltime[MyNum] = time6-time1;
   }
   if (MyNum == 0) {
     global->rs = time1;
     global->rf = time6;
     global->final = to;
   }
}

/*
 * product_mod_46() returns the product (mod 2^46) of t1 and t2.
 */
double product_mod_46(double t1, double t2)
{
   double a1;
   double b1;
   double a2;
   double b2;
			
   a1 = (double)((long)(t1 / RADIX_S));    /* Decompose the arguments.  */
   a2 = t1 - a1 * RADIX_S;
   b1 = (double)((long)(t2 / RADIX_S));
   b2 = t2 - b1 * RADIX_S;
   t1 = a1 * b2 + a2 * b1;      /* Multiply the arguments.  */
   t2 = (double)((long)(t1 / RADIX_S));
   t2 = t1 - t2 * RADIX_S;
   t1 = t2 * RADIX_S + a2 * b2;
   t2 = (double)((long)(t1 / RADIX));

   return (t1 - t2 * RADIX);    /* Return the product.  */
}

/*
 * finds the (k)th random number, given the seed, b, and the ratio, t.
 */
double ran_num_init(unsigned long k, double b, double t)
{
   unsigned long j;

   while (k != 0) {             /* while() is executed m times
				   such that 2^m > k.  */
      j = k >> 1;
      if ((j << 1) != k) {
         b = product_mod_46(b, t);
      }
      t = product_mod_46(t, t);
      k = j;
   }

   return b;
}

long get_max_digits(long max_key)
{
  long done = 0;
  long temp = 1;
  long key_val;

  key_val = max_key;
  while (!done) {
    key_val = key_val / radix;
    if (key_val == 0) {
      done = 1;
    } else {
      temp ++;
    }
  }
  return temp;
}

long get_log2_radix(long rad)
{
   long cumulative=1;
   long out;

   for (out = 0; out < 20; out++) {
     if (cumulative == rad) {
       return(out);
     } else {
       cumulative = cumulative * 2;
     }
   }
   fprintf(stderr,"ERROR: Radix %ld not a power of 2\n", rad);
   exit(-1);
}

long get_log2_keys(long num_keys)
{
   long cumulative=1;
   long out;

   for (out = 0; out < 30; out++) {
     if (cumulative == num_keys) {
       return(out);
     } else {
       cumulative = cumulative * 2;
     }
   }
   fprintf(stderr,"ERROR: Number of keys %ld not a power of 2\n", num_keys);
   exit(-1);
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

void init(long key_start, long key_stop, long from)
{
   double ran_num;
   double sum;
   long tmp;
   long i;
   long *key_from;

   ran_num = ran_num_init((key_start << 2) + 1, SEED, RATIO);
   sum = ran_num / RADIX;
   key_from = (long *) key[from];
   for (i = key_start; i < key_stop; i++) {
      ran_num = product_mod_46(ran_num, RATIO);
      sum = sum + ran_num / RADIX;
      ran_num = product_mod_46(ran_num, RATIO);
      sum = sum + ran_num / RADIX;
      ran_num = product_mod_46(ran_num, RATIO);
      sum = sum + ran_num / RADIX;
      key_from[i] = (long) ((sum / 4.0) *  max_key);
      tmp = (long) ((key_from[i])/100);
      ran_num = product_mod_46(ran_num, RATIO);
      sum = ran_num / RADIX;
   }
}

void test_sort(long final)
{
   long i;
   long mistake = 0;
   long *key_final;

   printf("\n");
   printf("                  TESTING RESULTS\n");
   key_final = key[final];
   for (i = 0; i < num_keys-1; i++) {
     if (key_final[i] > key_final[i + 1]) {
       fprintf(stderr,"error with key %ld, value %ld %ld \n",
        i,key_final[i],key_final[i + 1]);
       mistake++;
     }
   }

   if (mistake) {
      printf("FAILED: %ld keys out of place.\n", mistake);
   } else {
      printf("PASSED: All keys in place.\n");
   }
   printf("\n");
}

void printout()
{
   long i;
   long *key_final;

   key_final = (long *) key[global->final];
   printf("\n");
   printf("                 SORTED KEY VALUES\n");
   printf("%8ld ",key_final[0]);
   for (i = 0; i < num_keys-1; i++) {
     printf("%8ld ",key_final[i+1]);
     if ((i+2)%5 == 0) {
       printf("\n");
     }
   }
   printf("\n");
}

