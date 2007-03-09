/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * pixel_processor.c: Copyright (C) 1999 Jay Cox <jaycox@gimp.org>
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
#endif

#include <glib-object.h>

#include "base-types.h"

#include "config/gimpbaseconfig.h"

#include "pixel-processor.h"
#include "pixel-region.h"

#include "tile.h"


#define TILES_PER_THREAD  8
#define PROGRESS_TIMEOUT  64


static GThreadPool *pool       = NULL;
static GMutex      *pool_mutex = NULL;
static GCond       *pool_cond  = NULL;


typedef void  (* p1_func) (gpointer      data,
                           PixelRegion  *region1);
typedef void  (* p2_func) (gpointer      data,
                           PixelRegion  *region1,
                           PixelRegion  *region2);
typedef void  (* p3_func) (gpointer      data,
                           PixelRegion  *region1,
                           PixelRegion  *region2,
                           PixelRegion  *region3);
typedef void  (* p4_func) (gpointer      data,
                           PixelRegion  *region1,
                           PixelRegion  *region2,
                           PixelRegion  *region3,
                           PixelRegion  *region4);


typedef struct _PixelProcessor PixelProcessor;

struct _PixelProcessor
{
  PixelProcessorFunc   func;
  gpointer             data;

#ifdef ENABLE_MP
  GMutex              *mutex;
  gint                 threads;
  gboolean             first;
#endif

  PixelRegionIterator *PRI;
  gint                 num_regions;
  PixelRegion         *regions[4];

  gulong               progress;
};


#ifdef ENABLE_MP
static void
do_parallel_regions (PixelProcessor *processor)
{
  PixelRegion tr[4];
  gint        i;

  g_mutex_lock (processor->mutex);

  /*  the first thread getting here must not call pixel_regions_process()  */
  if (!processor->first && processor->PRI)
    processor->PRI = pixel_regions_process (processor->PRI);
  else
    processor->first = FALSE;

  while (processor->PRI)
    {
      guint pixels = (processor->PRI->portion_width *
                      processor->PRI->portion_height);

      for (i = 0; i < processor->num_regions; i++)
        if (processor->regions[i])
          {
            memcpy (&tr[i], processor->regions[i], sizeof (PixelRegion));
            if (tr[i].tiles)
              tile_lock (tr[i].curtile);
          }

      g_mutex_unlock (processor->mutex);

      switch (processor->num_regions)
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

      g_mutex_lock (processor->mutex);

      for (i = 0; i < processor->num_regions; i++)
        if (processor->regions[i])
          {
            if (tr[i].tiles)
              tile_release (tr[i].curtile, tr[i].dirty);
          }

      processor->progress += pixels;

      if (processor->PRI)
        processor->PRI = pixel_regions_process (processor->PRI);
    }

  processor->threads--;

  if (processor->threads == 0)
    {
      g_mutex_unlock (processor->mutex);

      g_mutex_lock (pool_mutex);
      g_cond_signal  (pool_cond);
      g_mutex_unlock (pool_mutex);
    }
  else
    {
      g_mutex_unlock (processor->mutex);
    }
}
#endif

/*  do_parallel_regions_single is just like do_parallel_regions
 *   except that all the mutex and tile locks have been removed
 *
 * If we are processing with only a single thread we don't need to do
 * the mutex locks etc. and aditional tile locks even if we were
 * configured --with-mp
 */

static gpointer
do_parallel_regions_single (PixelProcessor             *processor,
                            PixelProcessorProgressFunc  progress_func,
                            gpointer                    progress_data,
                            gulong                      total)
{
  GTimeVal  last_time;

  if (progress_func)
    g_get_current_time (&last_time);

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
          break;
        }

      if (progress_func)
        {
          GTimeVal  now;

          processor->progress += (processor->PRI->portion_width *
                                  processor->PRI->portion_height);

          g_get_current_time (&now);

          if (((now.tv_sec - last_time.tv_sec) * 1024 +
               (now.tv_usec - last_time.tv_usec) / 1024) > PROGRESS_TIMEOUT)
            {
              progress_func (progress_data,
                             (gdouble) processor->progress / (gdouble) total);

              last_time = now;
            }
        }
    }
  while (processor->PRI &&
         (processor->PRI = pixel_regions_process (processor->PRI)));

  return NULL;
}

static void
pixel_regions_do_parallel (PixelProcessor             *processor,
                           PixelProcessorProgressFunc  progress_func,
                           gpointer                    progress_data)
{
  gulong pixels = (processor->PRI->region_width *
                   processor->PRI->region_height);
  gulong tiles  = pixels / (TILE_WIDTH * TILE_HEIGHT);

#ifdef ENABLE_MP
  if (pool && tiles > TILES_PER_THREAD)
    {
      GError *error = NULL;
      gint    tasks = MIN (tiles / TILES_PER_THREAD,
                           g_thread_pool_get_max_threads (pool));

      /*
       * g_printerr ("pushing %d tasks into the thread pool (for %lu tiles)\n",
       *             tasks, tiles);
       */

      processor->first   = TRUE;
      processor->threads = tasks;
      processor->mutex   = g_mutex_new();

      g_mutex_lock (pool_mutex);

      while (tasks--)
        {
          g_thread_pool_push (pool, processor, &error);

          if (G_UNLIKELY (error))
            {
              g_warning ("thread creation failed: %s", error->message);
              g_clear_error (&error);
              processor->threads--;
            }
        }

      if (progress_func)
        {
          while (processor->threads != 0)
            {
              GTimeVal timeout;
              gulong   progress;

              g_get_current_time (&timeout);
              g_time_val_add (&timeout, PROGRESS_TIMEOUT * 1024);

              g_cond_timed_wait (pool_cond, pool_mutex, &timeout);

              g_mutex_lock (processor->mutex);
              progress = processor->progress;
              g_mutex_unlock (processor->mutex);

              progress_func (progress_data,
                             (gdouble) progress / (gdouble) pixels);
            }
        }
      else
        {
          while (processor->threads != 0)
            g_cond_wait (pool_cond, pool_mutex);
        }

      g_mutex_unlock (pool_mutex);

      g_mutex_free (processor->mutex);
    }
  else
#endif
    {
      do_parallel_regions_single (processor,
                                  progress_func, progress_data, pixels);
    }

  if (progress_func)
    progress_func (progress_data, 1.0);
}

static void
pixel_regions_process_parallel_valist (PixelProcessorFunc         func,
                                       gpointer                   data,
                                       PixelProcessorProgressFunc progress_func,
                                       gpointer                   progress_data,
                                       gint                       num_regions,
                                       va_list                    ap)
{
  PixelProcessor  processor = { NULL, };
  gint            i;

  for (i = 0; i < num_regions; i++)
    processor.regions[i] = va_arg (ap, PixelRegion *);

  switch (num_regions)
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
      g_warning ("pixel_regions_process_parallel: "
                 "bad number of regions (%d)", processor.num_regions);
      break;
    }

  if (! processor.PRI)
    return;

  processor.func        = func;
  processor.data        = data;
  processor.num_regions = num_regions;

#ifdef ENABLE_MP
  processor.threads     = 0;
#endif

  processor.progress    = 0;

  pixel_regions_do_parallel (&processor, progress_func, progress_data);
}

void
pixel_processor_init (gint num_threads)
{
  pixel_processor_set_num_threads (num_threads);
}

void
pixel_processor_set_num_threads (gint num_threads)
{
#ifdef ENABLE_MP

  g_return_if_fail (num_threads > 0 && num_threads <= GIMP_MAX_NUM_THREADS);

  if (num_threads < 2)
    {
      if (pool)
        {
          g_thread_pool_free (pool, TRUE, TRUE);
          pool = NULL;

          g_cond_free (pool_cond);
          pool_cond = NULL;

          g_mutex_free (pool_mutex);
          pool_mutex = NULL;
        }
    }
  else
    {
      GError *error = NULL;

      if (pool)
        {
          g_thread_pool_set_max_threads (pool, num_threads, &error);
        }
      else
        {
          pool = g_thread_pool_new ((GFunc) do_parallel_regions, NULL,
                                    num_threads, TRUE, &error);

          pool_mutex = g_mutex_new ();
          pool_cond  = g_cond_new ();
        }

      if (G_UNLIKELY (error))
        {
          g_warning ("changing the number of threads to %d failed: %s",
                     num_threads, error->message);
          g_clear_error (&error);
        }
    }
#endif
}

void
pixel_processor_exit (void)
{
  pixel_processor_set_num_threads (1);
}

void
pixel_regions_process_parallel (PixelProcessorFunc  func,
                                gpointer            data,
                                gint                num_regions,
                                ...)
{
  va_list va;

  va_start (va, num_regions);

  pixel_regions_process_parallel_valist (func, data,
                                         NULL, NULL,
                                         num_regions, va);

  va_end (va);
}

void
pixel_regions_process_parallel_progress (PixelProcessorFunc          func,
                                         gpointer                    data,
                                         PixelProcessorProgressFunc  progress_func,
                                         gpointer                    progress_data,
                                         gint                        num_regions,
                                         ...)
{
  va_list va;

  va_start (va, num_regions);

  pixel_regions_process_parallel_valist (func, data,
                                         progress_func, progress_data,
                                         num_regions, va);

  va_end (va);
}
