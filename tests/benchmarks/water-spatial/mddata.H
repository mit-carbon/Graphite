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

#define BOTH 2
typedef double vm_type[3];

typedef struct mol_dummy {
      vm_type VM;
      double F[MXOD2][NDIR][NATOM];
} molecule_type;

typedef struct link {
      molecule_type mol;
      struct link *next_mol;
} link_type;

typedef struct box_dummy {
      struct link *list;
      LOCKDEC(boxlock)
} box_type;

extern box_type ***BOX;

typedef struct array_dummy {
      long box[NDIR][BOTH];
} first_last_array;

extern first_last_array **start_end;

typedef struct list_of_boxes {
      long coord[3];
      struct list_of_boxes *next_box;
} box_list;

extern box_list **my_boxes;

extern double  TLC[100], FPOT, FKIN;
extern long IX[3*MXOD2+1], IRST,NVAR,NXYZ,NXV,IXF,IYF,IZF,IMY,IMZ;

extern long NumProcs;
extern long NumBoxes;
