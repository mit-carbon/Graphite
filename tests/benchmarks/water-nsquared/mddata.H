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

  /* this file contains the declarations of the main data
  structure types used by the program */

  /* per molecule, there are 3*8 = 24 bytes for the VM array */
  /* and 9*3*3*8 = 648 bytes for the F array 		   */
  /* that is, a total of 672 bytes		           */

  /* then there is some global memory, defined in global.H   */
  /* which mainly consists of an array of locks, one per mol */

  /* PRIVATE DATA:  Every process has a private array of computed  */
  /* interactions, PFORCES, defined in interf.C.  The size of this */
  /* is nmol*3*3*8 = 72 * nmol bytes				 */
  /* A processor only uses at most 72 * (nmol/2 + nmol/p) bytes of */
  /* this.                                                         */
  /* And, every process has six private arrays for use in interf   */
  /* These arrays are of size 15*8 = 120 bytes each, for a total   */
  /* of 720 bytes.  therefore per process data are                 */
  /* (72 * nmol) + 720 bytes.                                      */

typedef double vm_type[3];

typedef struct mol_dummy {
      vm_type VM;
      double F[MXOD2][NDIR][NATOM];
} molecule_type;

extern molecule_type *VAR;

extern double ****PFORCES;

extern double  TLC[100], FPOT, FKIN;

#define MAXPROCS 128

extern long StartMol[MAXPROCS+1];
extern long MolsPerProc;
extern long NumProcs;
