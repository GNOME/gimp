/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "appenv.h"
#include "pixel_region.h"
#include "gimprc.h"

#include "tile_manager_pvt.h"
#include "tile.h"			/* ick. */

#ifdef ENABLE_MP

#include <pthread.h>

#define IF_THREAD(statement) statement

#else /* !USE_PTHREADS */

#define IF_THREAD(statement)

#endif /* !USE_PTHREADS */


typedef struct _PixelRegionHolder PixelRegionHolder;

struct _PixelRegionHolder
{
  PixelRegion *PR;
  unsigned char *original_data;
  int startx, starty;
  int count;
};


typedef struct _PixelRegionIterator PixelRegionIterator;

struct _PixelRegionIterator
{
  GSList *pixel_regions;
  int region_width;
  int region_height;
  int portion_width;
  int portion_height;
  int process_count;
};

/*********************/
/*  Local Variables  */



/*********************/
/*  Local Functions  */

static int     get_portion_width       (PixelRegionIterator *);
static int     get_portion_height      (PixelRegionIterator *);
static void *  pixel_regions_configure (PixelRegionIterator *);
static void    pixel_region_configure  (PixelRegionHolder *, PixelRegionIterator *);


/**************************/
/*  Function definitions  */

void
pixel_region_init (PR, tiles, x, y, w, h, dirty)
     PixelRegion *PR;
     TileManager *tiles;
     int x, y;
     int w, h;
     int dirty;
{
  PR->tiles = tiles;
  PR->curtile = NULL;
  PR->data = NULL;
  PR->bytes = tiles->bpp;
  PR->rowstride = PR->bytes * TILE_WIDTH;
  PR->x = x;
  PR->y = y;
  PR->w = w;
  PR->h = h;
  PR->dirty = dirty;
}


void
pixel_region_resize (PR, x, y, w, h)
     PixelRegion *PR;
     int x, y;
     int w, h;
{
  /*  If the data is non-null, data is contiguous--need to advance  */
  if (PR->data != NULL)
    {
      PR->data += (y - PR->y) * PR->rowstride + (x - PR->x) * PR->bytes;
    }

  /*  update sizes for both contiguous and tiled regions  */
  PR->x = x;
  PR->y = y;
  PR->w = w;
  PR->h = h;
}

/* request that tiles within a region be fetched asynchronously
 */
void
pixel_region_get_async (PR, ulx, uly, lrx, lry)
    PixelRegion *PR;
    int ulx;
    int uly;
    int lrx;
    int lry;
{
  int x, y;

  for (y = uly; y < lry; y += TILE_HEIGHT)
    for (x = ulx; x < lrx; x += TILE_WIDTH)
      tile_manager_get_async (PR->tiles, x, y);
}

void
pixel_region_get_row (PR, x, y, w, data, subsample)
     PixelRegion *PR;
     int x, y;
     int w;
     unsigned char *data;
     int subsample;
{
  Tile *tile;
  unsigned char *tile_data;
  int inc;
  int end;
  int boundary;
  int b;
  int npixels;

  end = x + w;

  pixel_region_get_async (PR, x, y, end, y);

  while (x < end)
    {
      tile = tile_manager_get_tile (PR->tiles, x, y, TRUE, FALSE);
      tile_data = tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT);
      npixels = tile_ewidth (tile) - (x % TILE_WIDTH);

      if ((x + npixels) > end) /* make sure we don't write past the end */
	npixels = end - x;

      if (subsample == 1) /* optimize for the common case */
      {
	memcpy(data, tile_data, tile_bpp(tile)*npixels);
	data += tile_bpp(tile)*npixels;
	x += npixels;
      }
      else
      {
	boundary = x + npixels;
	inc = subsample * tile_bpp(tile);
	for ( ; x < boundary; x += subsample)
	{
	  for (b = 0; b < tile_bpp(tile); b++)
	    *data++ = tile_data[b];
	  tile_data += inc;
	}
      }
      tile_release (tile, FALSE);
    }
}


void
pixel_region_set_row (PR, x, y, w, data)
     PixelRegion *PR;
     int x, y;
     int w;
     unsigned char *data;
{
  Tile *tile;
  unsigned char *tile_data;
  int end;
  int npixels;

  end = x + w;

  pixel_region_get_async (PR, x, y, end, y);

  while (x < end)
    {
      tile = tile_manager_get_tile (PR->tiles, x, y, TRUE, TRUE);
      tile_data = tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT);

      npixels = tile_ewidth(tile) - (x % TILE_WIDTH);

      if ((x + npixels) > end) /* make sure we don't write past the end */
	npixels = end - x;

      memcpy(tile_data, data, tile_bpp(tile)*npixels);

      data += tile_bpp(tile)*npixels;
      x += npixels;

      tile_release (tile, TRUE);
    }
}


void
pixel_region_get_col (PR, x, y, h, data, subsample)
     PixelRegion *PR;
     int x, y;
     int h;
     unsigned char *data;
     int subsample;
{
  Tile *tile;
  unsigned char *tile_data;
  int inc;
  int end;
  int boundary;
  int b;

  end = y + h;

  pixel_region_get_async (PR, x, y, x, end);

  while (y < end)
    {
      tile = tile_manager_get_tile (PR->tiles, x, y, TRUE, FALSE);
      tile_data = tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT);
      boundary = y + (tile_eheight(tile) - (y % TILE_HEIGHT));
      if (boundary > end) /* make sure we don't write past the end */
	boundary = end;

      inc = subsample * tile_bpp(tile) * tile_ewidth(tile);

      for ( ; y < boundary; y += subsample)
	{
	  for (b = 0; b < tile_bpp(tile); b++)
	    *data++ = tile_data[b];
	  tile_data += inc;
	}

      tile_release (tile, FALSE);
    }
}


void
pixel_region_set_col (PR, x, y, h, data)
     PixelRegion *PR;
     int x, y;
     int h;
     unsigned char *data;
{
  Tile *tile;
  unsigned char *tile_data;
  int inc;
  int end;
  int boundary;
  int b;

  end = y + h;

  pixel_region_get_async (PR, x, y, x, end);

  while (y < end)
    {
      tile = tile_manager_get_tile (PR->tiles, x, y, TRUE, TRUE);
      tile_data = tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT);
      boundary = y + (tile_eheight(tile) - (y % TILE_HEIGHT));
      inc = tile_bpp(tile) * tile_ewidth(tile);

      if (boundary > end) /* make sure we don't write past the end */
	boundary = end;

      for ( ; y < boundary; y++)
	{
	  for (b = 0; b < tile_bpp(tile); b++)
	    tile_data[b] = *data++;
	  tile_data += inc;
	}

      tile_release (tile, TRUE);
    }
}

typedef void (*p1_func)(void *, PixelRegion *);
typedef void (*p2_func)(void *, PixelRegion * ,PixelRegion *);
typedef void (*p3_func)(void *, PixelRegion * ,PixelRegion *, PixelRegion *);
typedef void (*p4_func)(void *, PixelRegion * ,PixelRegion *, PixelRegion *,
			PixelRegion *);

struct parallel_struct
{
  void *data;
  p_func f;
  PixelRegionIterator *PRI;
  IF_THREAD(pthread_mutex_t mutex;)
  int nthreads;
  int n_regions;
  PixelRegion *r[4];
};

static void *
do_parallel_regions(struct parallel_struct *p_s)
{
  PixelRegion tr[4];
  int ntiles = 0;
  int i;

  IF_THREAD(pthread_mutex_lock(&p_s->mutex);)
  if (p_s->nthreads != 0 && p_s->PRI)
    p_s->PRI =  (PixelRegionIterator*)pixel_regions_process(p_s->PRI);
  if (p_s->PRI == NULL)
  {
    IF_THREAD(pthread_mutex_unlock(&p_s->mutex);)
    return NULL;
  }
  p_s->nthreads++;

  do
  {
    for (i = 0; i < p_s->n_regions; i++)
      if (p_s->r[i])
	memcpy(&tr[i], p_s->r[i], sizeof(PixelRegion));
    IF_THREAD(pthread_mutex_unlock(&p_s->mutex);)
    ntiles++;
    switch(p_s->n_regions)
    {
     case 1:
       ((p1_func)p_s->f)(p_s->data,
			 p_s->r[0] ? &tr[0] : NULL);
       break;
     case 2:
       ((p2_func)p_s->f)(p_s->data,
			 p_s->r[0] ? &tr[0] : NULL,
			 p_s->r[1] ? &tr[1] : NULL);
       break;
     case 3:
       ((p3_func)p_s->f)(p_s->data,
			 p_s->r[0] ? &tr[0] : NULL,
			 p_s->r[1] ? &tr[1] : NULL,
			 p_s->r[2] ? &tr[2] : NULL);
       break;
     case 4:
       ((p4_func)p_s->f)(p_s->data,
			 p_s->r[0] ? &tr[0] : NULL,
			 p_s->r[1] ? &tr[1] : NULL,
			 p_s->r[2] ? &tr[2] : NULL,
			 p_s->r[3] ? &tr[3] : NULL);
       break;
     default:
       g_message("do_parallel_regions: Bad number of regions %d\n",
		 p_s->n_regions);
    }
    IF_THREAD(pthread_mutex_lock(&p_s->mutex);)
  }  while (p_s->PRI && (p_s->PRI =  (PixelRegionIterator*)pixel_regions_process(p_s->PRI)));
  p_s->nthreads--;
  /*  fprintf(stderr, "processed %d tiles\n", ntiles); */
  IF_THREAD(pthread_mutex_unlock(&p_s->mutex);)
  return NULL;
}

#define MAX_THREADS 30

void pixel_regions_process_parallel(p_func f, void *data, int num_regions, ...)
{
  int i;
  PixelRegionIterator *PRI;
  struct parallel_struct p_s;
  IF_THREAD(pthread_t threads[MAX_THREADS];)
  IF_THREAD(pthread_attr_t pthread_attr;)
  int nthreads;
  va_list ap;

  va_start (ap, num_regions);
  for (i = 0; i < num_regions; i++)
    p_s.r[i] = va_arg (ap, PixelRegion *);
  va_end (ap);

  switch(num_regions)
  {
   case 1:
    p_s.PRI = (PixelRegionIterator *) pixel_regions_register(num_regions,
							     p_s.r[0]);
    break;
   case 2:
    p_s.PRI = (PixelRegionIterator *) pixel_regions_register(num_regions,
							     p_s.r[0],
							     p_s.r[1]);
    break;
   case 3:
    p_s.PRI = (PixelRegionIterator *) pixel_regions_register(num_regions,
							     p_s.r[0],
							     p_s.r[1],
							     p_s.r[2]);
    break;
   case 4:
    p_s.PRI = (PixelRegionIterator *) pixel_regions_register(num_regions,
							     p_s.r[0],
							     p_s.r[1],
							     p_s.r[2],
							     p_s.r[3]);
    break;
   default:
     g_message("pixel_regions_process_parallel: Bad number of regions %d\n",
	       p_s.n_regions);
  }
  if (!p_s.PRI)
    return;
  p_s.f = f;
  p_s.data = data;
  p_s.n_regions = num_regions;
  IF_THREAD(pthread_mutex_init(&p_s.mutex, NULL);)
  p_s.nthreads = 0;

  IF_THREAD(
    nthreads = MIN(num_processors,
		   (p_s.PRI->region_width * p_s.PRI->region_height) /(64*64));
    if (nthreads > 1)
    {
      pthread_attr_init (&pthread_attr);
      for (i = 0; i < nthreads; i++)
      {
	pthread_create (&threads[i], &pthread_attr,
			(void *(*)(void *)) do_parallel_regions,
			&p_s);
      }
      for (i = 0; i < nthreads; i++)
      {
	pthread_join(threads[i], NULL);
      }
    }
    else
    )
    do_parallel_regions(&p_s);
  if (p_s.nthreads != 0)
    fprintf(stderr, "Ack, we've lost a thread.");
}

void *
pixel_regions_register (int num_regions, ...)
{
  PixelRegion *PR;
  PixelRegionHolder *PRH;
  PixelRegionIterator *PRI;
  int found;
  va_list ap;

  PRI = (PixelRegionIterator *) g_malloc (sizeof (PixelRegionIterator));
  PRI->pixel_regions = NULL;
  PRI->process_count = 0;

  if (num_regions < 1)
    return FALSE;

  va_start (ap, num_regions);

  found = FALSE;
  while (num_regions --)
    {
      PR = va_arg (ap, PixelRegion *);
      PRH = (PixelRegionHolder *) g_malloc (sizeof (PixelRegionHolder));
      PRH->PR = PR;

      if (PR != NULL)
	{
	  /*  If there is a defined value for data, make sure tiles is NULL  */
	  if (PR->data)
	    PR->tiles = NULL;
	  PRH->original_data = PR->data;
	  PRH->startx = PR->x;
	  PRH->starty = PR->y;
	  PRH->PR->process_count = 0;

	  if (!found)
	    {
	      found = TRUE;
	      PRI->region_width = PR->w;
	      PRI->region_height = PR->h;
	    }
	}

      /*  Add the pixel region holder to the list  */
      PRI->pixel_regions = g_slist_prepend (PRI->pixel_regions, PRH);
    }

  va_end (ap);

  return pixel_regions_configure (PRI);
}


void *
pixel_regions_process (PRI_ptr)
     void *PRI_ptr;
{
  GSList *list;
  PixelRegionHolder *PRH;
  PixelRegionIterator *PRI;

  PRI = (PixelRegionIterator *) PRI_ptr;
  PRI->process_count++;

  /*  Unref all referenced tiles and increment the offsets  */

  list = PRI->pixel_regions;
  while (list)
    {
      PRH = (PixelRegionHolder *) list->data;

      if ((PRH->PR != NULL) && (PRH->PR->process_count != PRI->process_count))
	{
	  /*  This eliminates the possibility of incrementing the
	   *  same region twice
	   */
	  PRH->PR->process_count++;

	  /*  Unref the last referenced tile if the underlying region is a tile manager  */
	  if (PRH->PR->tiles)
	    {
	      tile_release (PRH->PR->curtile, PRH->PR->dirty);
	      PRH->PR->curtile = NULL;
	    }

	  PRH->PR->x += PRI->portion_width;

	  if ((PRH->PR->x - PRH->startx) >= PRI->region_width)
	    {
	      PRH->PR->x = PRH->startx;
	      PRH->PR->y += PRI->portion_height;
	    }
	}

      list = g_slist_next (list);
    }

  return pixel_regions_configure (PRI);
}

void
pixel_regions_process_stop (PRI_ptr)
     void *PRI_ptr;
{
  GSList *list;
  PixelRegionHolder *PRH;
  PixelRegionIterator *PRI;

  PRI = (PixelRegionIterator *) PRI_ptr;
  PRI->process_count++;

  /*  Unref all referenced tiles and increment the offsets  */

  list = PRI->pixel_regions;
  while (list)
    {
      PRH = (PixelRegionHolder *) list->data;

      if ((PRH->PR != NULL) && (PRH->PR->process_count != PRI->process_count))
	{
	  /*  This eliminates the possibility of incrementing the
	   *  same region twice
	   */
	  PRH->PR->process_count++;

	  /*  Unref the last referenced tile if the underlying region is a tile manager  */
	  if (PRH->PR->tiles)
	    {
	      tile_release (PRH->PR->curtile, PRH->PR->dirty);
	      PRH->PR->curtile = NULL;
	    }
	}

      list = g_slist_next (list);
    }

  if (PRI->pixel_regions)
    {
      list = PRI->pixel_regions;
      while (list)
	{
	  g_free (list->data);
	  list = g_slist_next (list);
	}
      g_slist_free (PRI->pixel_regions);
      g_free (PRI);
    }
}


/*********************************/
/*  Static Function Definitions  */

static int
get_portion_height (PRI)
     PixelRegionIterator *PRI;
{
  GSList *list;
  PixelRegionHolder *PRH;
  int min_height = G_MAXINT;
  int height;

  /*  Find the minimum height to the next vertical tile (in the case of a tile manager)
   *  or to the end of the pixel region (in the case of no tile manager)
   */

  list = PRI->pixel_regions;
  while (list)
    {
      PRH = (PixelRegionHolder *) list->data;

      if (PRH->PR)
	{
	  /*  Check if we're past the point of no return  */
	  if ((PRH->PR->y - PRH->starty) >= PRI->region_height)
	    return 0;

	  if (PRH->PR->tiles)
	    {
	      height = TILE_HEIGHT - (PRH->PR->y % TILE_HEIGHT);
	      height = BOUNDS (height, 0, (PRI->region_height - (PRH->PR->y - PRH->starty)));
	    }
	  else
	    height = (PRI->region_height - (PRH->PR->y - PRH->starty));

	  if (height < min_height)
	    min_height = height;
	}

      list = g_slist_next (list);
    }

  return min_height;
}


static int
get_portion_width (PRI)
     PixelRegionIterator *PRI;
{
  GSList *list;
  PixelRegionHolder *PRH;
  int min_width = G_MAXINT;
  int width;

  /*  Find the minimum width to the next vertical tile (in the case of a tile manager)
   *  or to the end of the pixel region (in the case of no tile manager)
   */

  list = PRI->pixel_regions;
  while (list)
    {
      PRH = (PixelRegionHolder *) list->data;

      if (PRH->PR)
	{
	  /*  Check if we're past the point of no return  */
	  if ((PRH->PR->x - PRH->startx) >= PRI->region_width)
	    return 0;

	  if (PRH->PR->tiles)
	    {
	      width = TILE_WIDTH - (PRH->PR->x % TILE_WIDTH);
	      width = BOUNDS (width, 0, (PRI->region_width - (PRH->PR->x - PRH->startx)));
	    }
	  else
	    width = (PRI->region_width - (PRH->PR->x - PRH->startx));

	  if (width < min_width)
	    min_width = width;
	}

      list = g_slist_next (list);
    }

  return min_width;
}


static void *
pixel_regions_configure (PRI)
     PixelRegionIterator *PRI;
{
  PixelRegionHolder *PRH;
  GSList *list;

  /*  Determine the portion width and height  */
  PRI->portion_width = get_portion_width (PRI);
  PRI->portion_height = get_portion_height (PRI);

  if (PRI->portion_width == 0 || PRI->portion_height == 0)
    {
      /*  free the pixel regions list  */
      if (PRI->pixel_regions)
	{
	  list = PRI->pixel_regions;
	  while (list)
	    {
	      g_free (list->data);
	      list = g_slist_next (list);
	    }
	  g_slist_free (PRI->pixel_regions);
	  g_free (PRI);
	}

      return NULL;
    }

  PRI->process_count++;

  list = PRI->pixel_regions;
  while (list)
    {
      PRH = (PixelRegionHolder *) list->data;

      if ((PRH->PR != NULL) && (PRH->PR->process_count != PRI->process_count))
	{
	  PRH->PR->process_count++;
	  pixel_region_configure (PRH, PRI);
	}

      list = g_slist_next (list);
    }

  return PRI;
}


static void
pixel_region_configure (PRH, PRI)
     PixelRegionHolder *PRH;
     PixelRegionIterator *PRI;
{
  /*  Configure the rowstride and data pointer for the pixel region
   *  based on the current offsets into the region and whether the
   *  region is represented by a tile manager or not
   */
  if (PRH->PR->tiles)
    {
      PRH->PR->curtile = tile_manager_get_tile (PRH->PR->tiles, PRH->PR->x, PRH->PR->y, TRUE, PRH->PR->dirty);

      PRH->PR->offx = PRH->PR->x % TILE_WIDTH;
      PRH->PR->offy = PRH->PR->y % TILE_HEIGHT;

      PRH->PR->rowstride = tile_ewidth(PRH->PR->curtile) * PRH->PR->bytes;
      PRH->PR->data = tile_data_pointer(PRH->PR->curtile, 
					PRH->PR->offx, PRH->PR->offy);
    }
  else
    {
      PRH->PR->data = PRH->original_data +
	(PRH->PR->y - PRH->starty) * PRH->PR->rowstride +
	(PRH->PR->x - PRH->startx) * PRH->PR->bytes;
    }

  PRH->PR->w = PRI->portion_width;
  PRH->PR->h = PRI->portion_height;
}
