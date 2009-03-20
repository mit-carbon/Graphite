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

/*************************************************************************
*                                                                        *
*     address.h: Definitions used for addressing maps.                   *
*                                                                        *
*************************************************************************/


				/* Fast subscripted access to map            */
				/*   (assumes mins = 0 and chars)            */
#define MAP_ADDRESS(IZ,IY,IX)	(map_address+\
				 ((IZ)*map_len[Y]+(IY))*map_len[X]+(IX))
#define MAP(IZ,IY,IX)		(*MAP_ADDRESS(IZ,IY,IX))


				/* Fast subscripted access to map            */
				/*   (assumes mins = 0 and chars)            */
#define GMAG_ADDRESS(IZ,IY,IX)	(gmag_address+\
				 ((IZ)*gmag_len[Y]+(IY))*gmag_len[X]+(IX))
#define GMAG(IZ,IY,IX)		(*GMAG_ADDRESS(IZ,IY,IX))


				/* Subscripted access to normal map          */
				/*   (NM GRADIENTs at each IX,IY,IZ)         */

#define TADDR(IZ,IY,IX)	        (LOOKUP_HSIZE+((IZ)*LOOKUP_PREC+(IY))*2+(IX))

#define NORM_ADDRESS(IZ,IY,IX,D) (norm_address+\
				 ((IZ)*norm_len[Y]+(IY))*norm_len[X]+(IX))

#define NORM(IZ,IY,IX,D)		(*NORM_ADDRESS(IZ,IY,IX,D))


				/* Subscripted access to opacity map         */
				/*   (1 char at each IX,IY,IZ)               */
#define OPC_ADDRESS(IZ,IY,IX)	(opc_address+\
				 ((IZ)*opc_len[Y]+(IY))*opc_len[X]+(IX))
#define OPC(IZ,IY,IX)		(*OPC_ADDRESS(IZ,IY,IX))


				/* Subscripted access to normal map          */
				/*   (NM GRADIENTs at each IX,IY,IZ)         */
#define VOX_ADDRESS(IZ,IY,IX,D)	(vox_address+\
				 (((IZ)*vox_len[Y]+(IY))*vox_len[X]+(IX))*\
				  2+(D))
#define VOX(IZ,IY,IX,D)		(*VOX_ADDRESS(IZ,IY,IX,D))


#define PYR_ADDRESS(ILEVEL,IZ,IY,IX)\
				(pyr_offset1=\
				 ((IZ)*pyr_len[ILEVEL][Y]+(IY))*\
				 pyr_len[ILEVEL][X]+(IX),\
				 pyr_offset2=pyr_offset1&7,\
				 pyr_address2=pyr_address[ILEVEL]+\
				 (pyr_offset1>>3))
#define PYR(ILEVEL,IZ,IY,IX)\
				(PYR_ADDRESS(ILEVEL,IZ,IY,IX),\
				 (*pyr_address2>>pyr_offset2)&1)


#define IMAGE_ADDRESS(IY,IX)	(image_address+(IY)*image_len[X]+(IX))
#define IMAGE(IY,IX)		(*IMAGE_ADDRESS(IY,IX))

				/* Access to mask image                      */
#define MASK_IMAGE_ADDRESS(IY,IX)	\
				(mask_image_address+\
				 (IY)*mask_image_len[X]+(IX))
#define MASK_IMAGE(IY,IX)	(*MASK_IMAGE_ADDRESS(IY,IX))
