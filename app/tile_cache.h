#ifndef __TILE_CACHE_H__
#define __TILE_CACHE_H__


#include "tile.h"


void  tile_cache_insert   (Tile          *tile);
void  tile_cache_flush    (Tile          *tile);
void  tile_cache_set_size (unsigned long  cache_size);


#endif /* __TILE_CACHE_H__ */
