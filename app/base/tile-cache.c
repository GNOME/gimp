#include <glib.h>
#include "gimprc.h"
#include "tile_cache.h"
#include "tile_swap.h"

/*  This is the percentage of the maximum cache size that should be cleared
 *   from the cache when an eviction is necessary
 */
#define FREE_QUANTUM 0.1

static void  tile_cache_init       (void);
static void  tile_cache_zorch_next (void);
static guint tile_cache_hash       (Tile *tile);


static int initialize = TRUE;
static GHashTable *tile_hash_table = NULL;
static GList *tile_list_head = NULL;
static GList *tile_list_tail = NULL;
static unsigned long max_tile_size = TILE_WIDTH * TILE_HEIGHT * 4;
static unsigned long cur_cache_size = 0;
static unsigned long max_cache_size = 0;


void
tile_cache_insert (Tile *tile)
{
  GList *tmp;

  if (initialize)
    tile_cache_init ();

  /* First check and see if the tile is already
   *  in the cache. In that case we will simply place
   *  it at the end of the tile list to indicate that
   *  it was the most recently accessed tile.
   */
  tmp = g_hash_table_lookup (tile_hash_table, tile);

  if (tmp)
    {
      /* The tile was already in the cache. Place it at
       *  the end of the tile list.
       */
      if (tmp == tile_list_tail)
	tile_list_tail = tile_list_tail->prev;
      tile_list_head = g_list_remove_link (tile_list_head, tmp);
      if (!tile_list_head)
	tile_list_tail = NULL;
      g_list_free (tmp);

      /* Remove the old reference to the tiles list node
       *  in the tile hash table.
       */
      g_hash_table_remove (tile_hash_table, tile);

      tile_list_tail = g_list_append (tile_list_tail, tile);
      if (!tile_list_head)
	tile_list_head = tile_list_tail;
      tile_list_tail = g_list_last (tile_list_tail);

      /* Add the tiles list node to the tile hash table. The
       *  list node is indexed by the tile itself. This makes
       *  for a quick lookup of which list node the tile is in.
       */
      g_hash_table_insert (tile_hash_table, tile, tile_list_tail);
    }
  else
    {
      /* The tile was not in the cache. First check and see
       *  if there is room in the cache. If not then we'll have
       *  to make room first. Note: it might be the case that the
       *  cache is smaller than the size of a tile in which case
       *  it won't be possible to put it in the cache.
       */
      if ((cur_cache_size + max_tile_size) > max_cache_size)
	{
	  while (tile_list_head && (cur_cache_size + max_cache_size * FREE_QUANTUM) > max_cache_size)
	    tile_cache_zorch_next ();

	  if ((cur_cache_size + max_tile_size) > max_cache_size)
	    return;
	}

      /* Place the tile at the end of the tile list.
       */
      tile_list_tail = g_list_append (tile_list_tail, tile);
      if (!tile_list_head)
	tile_list_head = tile_list_tail;
      tile_list_tail = g_list_last (tile_list_tail);

      /* Add the tiles list node to the tile hash table.
       */
      g_hash_table_insert (tile_hash_table, tile, tile_list_tail);

      /* Note the increase in the number of bytes the cache
       *  is referencing.
       */
      cur_cache_size += tile_size (tile);

      /* Reference the tile so that it won't be swapped out
       *  to disk. Swap the tile in if necessary.
       * "tile_ref" cannot be used here since it calls this
       *  function.
       */
      tile->ref_count += 1;
      {
	extern int tile_ref_count;
	tile_ref_count += 1;
      }
      if (tile->ref_count == 1)
	{
	  tile_swap_in (tile);

	  /*  the tile must be clean  */
	  tile->dirty = FALSE;
	}
    }
}

void
tile_cache_flush (Tile *tile)
{
  GList *tmp;

  if (initialize)
    tile_cache_init ();

  /* Find where the tile is in the cache.
   */
  tmp = g_hash_table_lookup (tile_hash_table, tile);

  if (tmp)
    {
      /* If the tile is in the cache, then remove it from the
       *  tile list.
       */
      if (tmp == tile_list_tail)
	tile_list_tail = tile_list_tail->prev;
      tile_list_head = g_list_remove_link (tile_list_head, tmp);
      if (!tile_list_head)
	tile_list_tail = NULL;
      g_list_free (tmp);

      /* Remove the tile from the tile hash table.
       */
      g_hash_table_remove (tile_hash_table, tile);

      /* Note the decrease in the number of bytes the cache
       *  is referencing.
       */
      cur_cache_size -= tile_size (tile);

      /* Unreference the tile.
       * "tile_unref" may be used here since it does not call
       *  this function (or any of the cache functions).
       */
      tile_unref (tile, FALSE);
    }
}

void
tile_cache_set_size (unsigned long cache_size)
{
  if (initialize)
    tile_cache_init ();

  max_cache_size = cache_size;

  if (max_cache_size >= max_tile_size)
    {
      while (cur_cache_size > max_cache_size)
	tile_cache_zorch_next ();
    }
}


static void
tile_cache_init ()
{
  if (initialize)
    {
      initialize = FALSE;

      tile_hash_table = g_hash_table_new ((GHashFunc) tile_cache_hash, NULL);

      max_cache_size = tile_cache_size;
    }
}

static void
tile_cache_zorch_next ()
{
  if (tile_list_head)
    tile_cache_flush ((Tile*) tile_list_head->data);
}

static guint
tile_cache_hash (Tile *tile)
{
  return (gulong) tile;
}
