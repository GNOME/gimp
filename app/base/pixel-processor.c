/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * pixel_processor.c: Copyright (C) 1999 Jay Cox <jaycox@earthlink.net>
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

#include "config.h"

#ifdef ENABLE_MP
#include <string.h>
#include <pthread.h>
#endif

#include <glib-object.h>

#include "base-types.h"

#include "config/gimpbaseconfig.h"

#include "pixel-processor.h"
#include "pixel-region.h"

#ifdef ENABLE_MP
#include "tile.h"
#endif

#ifdef __GNUC__
#warning FIXME: extern GimpBaseConfig *base_config;
#endif
extern GimpBaseConfig *base_config;


typedef void (* p1_func) (gpointer     data,
			  PixelRegion *region1);
typedef void (* p2_func) (gpointer     data,
			  PixelRegion *region1,
			  PixelRegion *region2);
typedef void (* p3_func) (gpointer     data,
			  PixelRegion *region1,
			  PixelRegion *region2,
			  PixelRegion *region3);
typedef void (* p4_func) (gpointer     data,
			  PixelRegion *region1,
			  PixelRegion *region2,
			  PixelRegion *region3,
			  PixelRegion *region4);


typedef struct _PixelProcessor PixelProcessor;

struct _PixelProcessor
{
  gpointer             data;
  PixelProcessorFunc   func;
  PixelRegionIterator *PRI;

#ifdef ENABLE_MP
  pthread_mutex_t      mutex;
  gint                 nthreads;
#endif

  gint                 num_regions;
  PixelRegion         *regions[4];
};


#ifdef ENABLE_MP
static void *
do_parallel_regions (PixelProcessor *processor)
{
  PixelRegion tr[4];
  gint        n_tiles = 0;
  gint        i;

  pthread_mutex_lock (&processor->mutex);

  if (processor->nthreads != 0 && processor->PRI)
    processor->PRI = pixel_regions_process (processor->PRI);

  if (processor->PRI == NULL)
    {
      pthread_mutex_unlock (&processor->mutex);
      return NULL;
    }

  processor->nthreads++;

  do
    {
      for (i = 0; i < processor->num_regions; i++)
	if (processor->regions[i])
	  {
	    memcpy (&tr[i], processor->regions[i], sizeof (PixelRegion));
	    if (tr[i].tiles)
	      tile_lock (tr[i].curtile);
	  }

      pthread_mutex_unlock (&processor->mutex);
      n_tiles++;

      switch(processor->num_regions)
	{
	case 1:
	  ((p1_func) processor->func) (processor->data,
                                       processor->regions[0] ? &tr[0] : NULL);
	  break;

	case 2:
	  ((p2_func) processor->func) (processor->data,
                                       processor->regions[0] ? &tr[0] : NULL,
                                       processor->regions[1] ? &tr[1] : NULL);
	  break;

	case 3:
	  ((p3_func) processor->func) (processor->data,
                                       processor->regions[0] ? &tr[0] : NULL,
                                       processor->regions[1] ? &tr[1] : NULL,
                                       processor->regions[2] ? &tr[2] : NULL);
	  break;

	case 4:
	  ((p4_func) processor->func) (processor->data,
                                       processor->regions[0] ? &tr[0] : NULL,
                                       processor->regions[1] ? &tr[1] : NULL,
                                       processor->regions[2] ? &tr[2] : NULL,
                                       processor->regions[3] ? &tr[3] : NULL);
	  break;

	default:
	  g_warning ("do_parallel_regions: Bad number of regions %d\n",
                     processor->num_regions);
          break;
    }

    pthread_mutex_lock (&processor->mutex);

    for (i = 0; i < processor->num_regions; i++)
      if (processor->regions[i])
	{
	  if (tr[i].tiles)
	    tile_release (tr[i].curtile, tr[i].dirty);
	}
    }

  while (processor->PRI &&
	 (processor->PRI = pixel_regions_process (processor->PRI)));

  processor->nthreads--;

  pthread_mutex_unlock (&processor->mutex);

  return NULL;
}
#endif

/*  do_parallel_regions_single is just like do_parallel_regions
 *   except that all the mutex and tile locks have been removed
 *
 * If we are processing with only a single thread we don't need to do the
 * mutex locks etc. and aditional tile locks even if we were
 * configured --with-mp
 */

static gpointer
do_parallel_regions_single (PixelProcessor *processor)
{
  do
    {
      switch (processor->num_regions)
        {
        case 1:
          ((p1_func) processor->func) (processor->data,
                                       processor->regions[0]);
          break;

        case 2:
          ((p2_func) processor->func) (processor->data,
                                       processor->regions[0],
                                       processor->regions[1]);
          break;

        case 3:
          ((p3_func) processor->func) (processor->data,
                                       processor->regions[0],
                                       processor->regions[1],
                                       processor->regions[2]);
          break;

        case 4:
          ((p4_func) processor->func) (processor->data,
                                       processor->regions[0],
                                       processor->regions[1],
                                       processor->regions[2],
                                       processor->regions[3]);
          break;

        default:
          g_warning ("do_parallel_regions_single: Bad number of regions %d\n",
                     processor->num_regions);
        }
    }

  while (processor->PRI &&
	 (processor->PRI = pixel_regions_process (processor->PRI)));

  return NULL;
}

#define MAX_THREADS 30

static void
pixel_regions_do_parallel (PixelProcessor *processor)
{
#ifdef ENABLE_MP
  gint  nthreads = MIN (base_config->num_processors, MAX_THREADS);

  /* make sure we have at least one tile per thread */
  nthreads = MIN (nthreads,
		  (processor->PRI->region_width *
                   processor->PRI->region_height) / (TILE_WIDTH * TILE_HEIGHT));

  if (nthreads > 1)
    {
      gint           i;
      pthread_t      threads[MAX_THREADS];
      pthread_attr_t pthread_attr;

      pthread_attr_init (&pthread_attr);

      for (i = 0; i < nthreads; i++)
        {
	  pthread_create (&threads[i], &pthread_attr,
			  (void *(*)(void *)) do_parallel_regions,
			  processor);
	}

      for (i = 0; i < nthreads; i++)
	{
	  gint ret = pthread_join (threads[i], NULL);

	  if (ret)
            g_printerr ("pixel_regions_do_parallel: "
                        "pthread_join returned: %d\n", ret);
	}

      if (processor->nthreads != 0)
	g_printerr ("pixel_regions_do_prarallel: we lost a thread\n");
    }
  else
#endif
    {
      do_parallel_regions_single (processor);
    }
}

static void
pixel_regions_process_parallel_valist (PixelProcessorFunc  func,
                                       gpointer            data,
                                       gint                num_regions,
                                       va_list             ap)
{
  PixelProcessor  processor = { NULL, };
  gint            i;

  for (i = 0; i < num_regions; i++)
    processor.regions[i] = va_arg (ap, PixelRegion *);

  switch(num_regions)
    {
    case 1:
      processor.PRI = pixel_regions_register (num_regions,
                                              processor.regions[0]);
      break;

    case 2:
      processor.PRI = pixel_regions_register (num_regions,
                                              processor.regions[0],
                                              processor.regions[1]);
      break;

    case 3:
      processor.PRI = pixel_regions_register (num_regions,
                                              processor.regions[0],
                                              processor.regions[1],
                                              processor.regions[2]);
      break;

    case 4:
      processor.PRI = pixel_regions_register (num_regions,
                                              processor.regions[0],
                                              processor.regions[1],
                                              processor.regions[2],
                                              processor.regions[3]);
      break;

    default:
      g_warning ("pixel_regions_process_parallel:"
                 "Bad number of regions %d\n", processor.num_regions);
    }

  if (! processor.PRI)
    {
      return;
    }

  processor.func        = func;
  processor.data        = data;
  processor.num_regions = num_regions;

#ifdef ENABLE_MP
  pthread_mutex_init (&processor.mutex, NULL);
  processor.nthreads = 0;
#endif

  pixel_regions_do_parallel (&processor);

#ifdef ENABLE_MP
  pthread_mutex_destroy (&processor.mutex);
#endif
}

void
pixel_regions_process_parallel (PixelProcessorFunc  func,
				gpointer            data,
				gint                num_regions,
				...)
{
  va_list va;

  va_start (va, num_regions);

  pixel_regions_process_parallel_valist (func, data, num_regions, va);

  va_end (va);
}
