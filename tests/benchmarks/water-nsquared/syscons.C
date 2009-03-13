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

EXTERN_ENV
#include "stdio.h"
#include <math.h>

#include "parameters.h"
#include "mdvar.h"
#include "water.h"
#include "wwpot.h"
#include "cnst.h"
#include "mddata.h"
#include "global.h"

void SYSCNS()                    /* sets up some system constants */
{
    TSTEP=TSTEP/UNITT;        /* time between steps */
    NATMO=NATOMS*NMOL;        /* total number of atoms in system */
    NATMO3=NATMO*3; /* number of atoms * number of spatial dimensions */
    FPOT= UNITM * pow((UNITL/UNITT),2.0) / (BOLTZ*TEMP*NATMO);
    FKIN=FPOT*0.50/(TSTEP*TSTEP);
    BOXL= pow( (NMOL*WTMOL*UNITM/RHO),(1.00/3.00));  /* computed
                                                        length of the cubical "box".  Note that box size is
                                                        computed as being large enough to handle the input
                                                        number of water molecules */

    BOXL=BOXL/UNITL;    /* normalized length of computational box */

    BOXH=BOXL*0.50; /* half the box length, used in
                       computing cutoff radius */

    if (CUTOFF == 0.0) {
        CUTOFF=max(BOXH,CUTOFF);    /* cutoff radius is max of BOXH
                                       and default (= 0); i.e. CUTOFF
                                       radius is set to half the normalized
                                       box length */
    }
    if (CUTOFF > 11.0) CUTOFF = 11.0; /* cutoff never greater than 11
					  Angstrom*/

    REF1= -QQ/(CUTOFF*CUTOFF*CUTOFF);
    REF2=2.00*REF1;
    REF4=2.00*REF2;
    CUT2=CUTOFF*CUTOFF;       /* square of cutoff radius,  used
                                 to actually decide whether an
                                 interaction should be computed in
                                 INTERF and POTENG */
    FHM=(TSTEP*TSTEP*0.50)/HMAS;
    FOM=(TSTEP*TSTEP*0.50)/OMAS;
    NMOL1=NMOL-1;
}       /* end of subroutine SYSCNS */
