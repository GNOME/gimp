#ifndef __TILE_MANAGER_H__
#define __TILE_MANAGER_H__


#include "tile.h"


typedef struct _TileLevel    TileLevel;
typedef struct _TileManager  TileManager;

typedef void (*TileValidateProc) (TileManager *tm,
				  Tile        *tile,
				  int          level);

struct _TileLevel
{
  int width;                       /* the width of the tiled area */
  int height;                      /* the height of the tiled area */
  int bpp;                         /* the bpp of each tile */

  int ntile_rows;                  /* the number of tiles in each row */
  int ntile_cols;                  /* the number of tiles in each columns */

  Tile *tiles;                     /* the tiles for this level */
};

struct _TileManager
{
  int x, y;                        /* tile manager offsets  */
  int nlevels;                     /* the number of tile levels in the hierarchy */
  TileLevel *levels;               /* the hierarchy */
  TileValidateProc validate_proc;  /* this proc is called when an attempt to get an
				    *  invalid tile is made.
				    */
  void *user_data;                 /* hook for hanging data off of */
};


/* Creates a new tile manager with the specified
 *  width for the toplevel. The toplevel sizes is
 *  used to compute the number of levels and there
 *  size. Each level is 1/2 the width and height of
 *  the level above it.
 * The toplevel is level 0. The smallest level in the
 *  hierarchy is "nlevels - 1". That level will be smaller
 *  than TILE_WIDTH x TILE_HEIGHT
 */
TileManager* tile_manager_new (int toplevel_width,
			       int toplevel_height,
			       int bpp);

/* Destroy a tile manager and all the tiles it contains.
 */
void tile_manager_destroy (TileManager *tm);

/* Calculate the number of levels necessary to have a complete
 *  hierarchy. This procedure is normally called twice with
 *  the width and then height and the maximum value returned
 *  is then used as the number of levels an image needs.
 */
int tile_manager_calc_levels (int size,
			      int tile_size);

/* Set the number of levels this tile manager is managing.
 *  This procedure may destroy unnecessary levels in the
 *  tile manager if the new number of levels is less than
 *  the old number of levels.
 * Any newly added levels will consist of invalid tiles.
 */
void tile_manager_set_nlevels (TileManager *tm,
			       int          nlevels);

/* Set the validate procedure for the tile manager.
 *  The validate procedure is called when an invalid tile
 *  is referenced. If the procedure is NULL, then the tile
 *  is set to valid and its memory is allocated, but
 *  not initialized.
 */
void tile_manager_set_validate_proc (TileManager      *tm,
				     TileValidateProc  proc);

/* Get a specified tile from a tile manager. The tile
 *  is from the given level and contains the specified
 *  pixel. Be aware that the pixel coordinates are
 *  dependent on the level.
 */
Tile* tile_manager_get_tile (TileManager *tm,
			     int          xpixel,
			     int          ypixel,
			     int          level);

/* Get a specified tile from a tile manager.
 */
Tile* tile_manager_get (TileManager *tm,
			int          tile_num,
			int          level);

/* Validate a tiles memory.
 */
void tile_manager_validate (TileManager *tm,
			    Tile        *tile);

/* Given a toplevel tile, this procedure will invalidate
 *  (set the dirty bit) for all tiles in lower levels which
 *  contain this toplevel tile.
 * Note: if a level hasn't been created then the tile for that
 *       level won't be invalidated.
 */
void tile_manager_invalidate_tiles (TileManager *tm,
				    Tile        *toplevel_tile);

/* Invalidates all the tiles in the sublevels.
 */
void tile_manager_invalidate_sublevels (TileManager *tm);

/* Update a portion lower level tile given a toplevel tile.
 */
void tile_manager_update_tile (TileManager *tm,
			       Tile        *toplevel_tile,
			       int          level);


#endif /* __TILE_MANAGER_H__ */
