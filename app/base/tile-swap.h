#ifndef __TILE_SWAP_H__
#define __TILE_SWAP_H__


#include "tile.h"


typedef enum {
  SWAP_IN = 1,
  SWAP_IN_ASYNC,
  SWAP_OUT,
  SWAP_DELETE,
  SWAP_COMPRESS
} SwapCommand;

typedef int (*SwapFunc) (int       fd,
			 Tile     *tile,
			 int       cmd,
			 gpointer  user_data);


void tile_swap_exit     (void);
int  tile_swap_add      (char      *filename,
		         SwapFunc   swap_func,
		         gpointer   user_data);
void tile_swap_remove   (int        swap_num);
void tile_swap_in       (Tile      *tile);
void tile_swap_in_async (Tile      *tile);
void tile_swap_out      (Tile      *tile);
void tile_swap_delete   (Tile      *tile);
void tile_swap_compress (int        swap_num);


#endif /* __TILE_SWAP_H__ */
