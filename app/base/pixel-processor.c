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
			  PixelRegion *);
typedef void (* p2_func) (gpointer     data,
			  PixelRegion * ,
			  PixelRegion *);
typedef void (* p3_func) (gpointer     data,
			  PixelRegion *,
			  PixelRegion *,
			  PixelRegion *);
typedef void (* p4_func) (gpointer     data,
			  PixelRegion *,
			  PixelRegion *,
			  PixelRegion *,
			  PixelRegion *);


typedef struct _PixelProcessor PixelProcessor;

struct _PixelProcessor
{
  gpointer             data;
  PixelProcessorFunc   f;
  PixelRegionIterator *PRI;

#ifdef ENABLE_MP
  pthread_mutex_t      mutex;
  gint                 nthreads;
#endif

  gint                 n_regions;
  PixelRegion         *r[4];
};


static void  pixel_processor_free (PixelProcessor *pp);


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
      for (i = 0; i < processor->n_regions; i++)
	if (processor->r[i])
	  {
	    memcpy (&tr[i], processor->r[i], sizeof (PixelRegion));
	    if (tr[i].tiles)
	      tile_lock(tr[i].curtile);
	  }

      pthread_mutex_unlock (&processor->mutex);
      n_tiles++;

      switch(processor->n_regions)
	{
	case 1:
	  ((p1_func) processor->f) (processor->data,
                                    processor->r[0] ? &tr[0] : NULL);
	  break;

	case 2:
	  ((p2_func) processor->f) (processor->data,
                                    processor->r[0] ? &tr[0] : NULL,
                                    processor->r[1] ? &tr[1] : NULL);
	  break;

	case 3:
	  ((p3_func) processor->f) (processor->data,
                                    processor->r[0] ? &tr[0] : NULL,
                                    processor->r[1] ? &tr[1] : NULL,
                                    processor->r[2] ? &tr[2] : NULL);
	  break;

	case 4:
	  ((p4_func) processor->f) (processor->data,
                                    processor->r[0] ? &tr[0] : NULL,
                                    processor->r[1] ? &tr[1] : NULL,
                                    processor->r[2] ? &tr[2] : NULL,
                                    processor->r[3] ? &tr[3] : NULL);
	  break;

	default:
	  g_warning ("do_parallel_regions: Bad number of regions %d\n",
                     processor->n_regions);
          break;
    }

    pthread_mutex_lock (&processor->mutex);

    for (i = 0; i < processor->n_regions; i++)
      if (processor->r[i])
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
      switch (processor->n_regions)
        {
        case 1:
          ((p1_func) processor->f) (processor->data,
                                    processor->r[0]);
          break;

        case 2:
          ((p2_func) processor->f) (processor->data,
                                    processor->r[0],
                                    processor->r[1]);
          break;

        case 3:
          ((p3_func) processor->f) (processor->data,
                                    processor->r[0],
                                    processor->r[1],
                                    processor->r[2]);
          break;

        case 4:
          ((p4_func) processor->f) (processor->data,
                                    processor->r[0],
                                    processor->r[1],
                                    processor->r[2],
                                    processor->r[3]);
          break;

        default:
          g_warning ("do_parallel_regions_single: Bad number of regions %d\n",
                     processor->n_regions);
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

static PixelProcessor *
pixel_regions_process_parallel_valist (PixelProcessorFunc  f,
                                       gpointer            data,
                                       gint                num_regions,
                                       va_list             ap)
{
  PixelProcessor *processor = g_new (PixelProcessor, 1);
  gint            i;

  for (i = 0; i < num_regions; i++)
    processor->r[i] = va_arg (ap, PixelRegion *);

  switch(num_regions)
    {
    case 1:
      processor->PRI = pixel_regions_register (num_regions,
                                               processor->r[0]);
      break;

    case 2:
      processor->PRI = pixel_regions_register (num_regions,
                                               processor->r[0],
                                               processor->r[1]);
      break;

    case 3:
      processor->PRI = pixel_regions_register (num_regions,
                                               processor->r[0],
                                               processor->r[1],
                                               processor->r[2]);
      break;

    case 4:
      processor->PRI = pixel_regions_register (num_regions,
                                               processor->r[0],
                                               processor->r[1],
                                               processor->r[2],
                                               processor->r[3]);
      break;

    default:
      g_warning ("pixel_regions_process_parallel:"
                 "Bad number of regions %d\n", processor->n_regions);
      pixel_processor_free (processor);
      return NULL;
    }

  processor->f = f;
  processor->data = data;
  processor->n_regions = num_regions;

#ifdef ENABLE_MP
  pthread_mutex_init (&processor->mutex, NULL);
  processor->nthreads = 0;
#endif

  pixel_regions_do_parallel (processor);

  if (processor->PRI)
    return processor;

#ifdef ENABLE_MP
  pthread_mutex_destroy (&processor->mutex);
#endif

  pixel_processor_free (processor);

  return NULL;
}

void
pixel_regions_process_parallel (PixelProcessorFunc  f,
				gpointer            data,
				gint                num_regions,
				...)
{
  va_list va;

  va_start (va, num_regions);

  pixel_regions_process_parallel_valist (f, data, num_regions, va);

  va_end (va);
}

static void
pixel_processor_free (PixelProcessor *processor)
{
  if (processor->PRI)
    pixel_regions_process_stop (processor->PRI);

  g_free (processor);
}
