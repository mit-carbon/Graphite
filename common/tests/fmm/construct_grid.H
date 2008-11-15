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

#ifndef _Construct_Grid_H
#define _Construct_Grid_H 1

extern void ConstructGrid(long my_id, time_info *local_time, long time_all);
extern void ConstructLists(long my_id, time_info *local_time, long time_all);
extern void DestroyGrid(long my_id, time_info *local_time, long time_all);
extern void PrintGrid(long my_id);

#endif /* _Construct_Grid_H */
