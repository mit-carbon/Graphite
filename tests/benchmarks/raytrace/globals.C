#include "rt.h"


INT	DataType;			/* Ascii or binary geometry file.    */

INT	TraversalType;			/* Linked list or HUG traversal.     */

INT	bundlex, bundley;		/* Bundle sizes for workpools.	     */
INT	blockx, blocky; 		/* Block sizes for workpools.	     */

BOOL	GeoFile;			/* TRUE if geometry file name found. */
BOOL	PicFile;			/* TRUE if picture file name found.  */
BOOL	ModelNorm;			/* TRUE if model must be normalized. */
BOOL	ModelTransform; 		/* TRUE if model transform present.  */
BOOL	AntiAlias;			/* TRUE if antialiasing enabled.     */

CHAR	GeoFileName[80];		/* Geometry file name.		     */
CHAR	PicFileName[80];		/* Picture file name.		     */

VIEW	View;				/* Viewing parameters.		     */
DISPLAY Display;			/* Display parameters.		     */
LIGHT	*lights;			/* Ptr to light list.		     */
INT	nlights;			/* Number of lights in scene.	     */

GMEM	*gm;				/* Ptr to global memory structure.   */



GRID	*world_level_grid;		/* Zero level grid pointer.	     */

INT	hu_max_prims_cell;		/* Max # of prims per cell.	     */
INT	hu_gridsize;			/* Grid size.			     */
INT	hu_numbuckets;			/* Hash table bucket size.	     */
INT	hu_max_subdiv_level;		/* Maximum level of hierarchy.	     */
INT	hu_lazy;			/* Lazy evaluation indicator.	     */

INT	prim_obj_cnt;			/* Totals for model.		     */
INT	prim_elem_cnt;
INT	subdiv_cnt;
INT	bintree_cnt;

INT	grids;
INT	total_cells;
INT	empty_voxels;
INT	nonempty_voxels;
INT	nonempty_leafs;
INT	prims_in_leafs;

UINT	empty_masks[sizeof(UINT)*8];
UINT	nonempty_masks[sizeof(UINT)*8];

