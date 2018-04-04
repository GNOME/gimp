/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-parallel.c
 * Copyright (C) 2018 Ell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "config.h"

#include <gio/gio.h>
#include <gegl.h>

extern "C"
{

#include "core-types.h"

#include "config/gimpgeglconfig.h"

#include "gimp.h"
#include "gimp-parallel.h"


#define GIMP_PARALLEL_MAX_THREADS 64


typedef struct
{
  GimpParallelDistributeFunc func;
  gint                       n;
  gpointer                   user_data;
} GimpParallelTask;

typedef struct
{
  GThread          *thread;
  GMutex            mutex;
  GCond             cond;

  gboolean          quit;

  GimpParallelTask *volatile task;
  volatile gint     i;
} GimpParallelThread;


/*  local function prototypes  */

static void       gimp_parallel_notify_num_processors (GimpGeglConfig     *config);

static void       gimp_parallel_set_n_threads         (gint                n_threads);
static gpointer   gimp_parallel_thread_func           (GimpParallelThread *thread);


/*  local variables  */

static gint               gimp_parallel_n_threads = 1;
static GimpParallelThread gimp_parallel_threads[GIMP_PARALLEL_MAX_THREADS];

static GMutex             gimp_parallel_completion_mutex;
static GCond              gimp_parallel_completion_cond;
static volatile gint      gimp_parallel_completion_counter;
static volatile gint      gimp_parallel_busy;


/*  public functions  */


void
gimp_parallel_init (Gimp *gimp)
{
  GimpGeglConfig *config;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  config = GIMP_GEGL_CONFIG (gimp->config);

  g_signal_connect (config, "notify::num-processors",
                    G_CALLBACK (gimp_parallel_notify_num_processors),
                    NULL);

  gimp_parallel_notify_num_processors (config);
}

void
gimp_parallel_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  g_signal_handlers_disconnect_by_func (gimp->config,
                                        (gpointer) gimp_parallel_notify_num_processors,
                                        NULL);

  /* stop all threads */
  gimp_parallel_set_n_threads (1);
}

void
gimp_parallel_distribute (gint                       max_n,
                          GimpParallelDistributeFunc func,
                          gpointer                   user_data)
{
  GimpParallelTask task;
  gint             i;

  g_return_if_fail (func != NULL);

  if (max_n == 0)
    return;

  if (max_n < 0)
    max_n = gimp_parallel_n_threads;
  else
    max_n = MIN (max_n, gimp_parallel_n_threads);

  if (max_n == 1 ||
      ! g_atomic_int_compare_and_exchange (&gimp_parallel_busy, 0, 1))
    {
      func (0, 1, user_data);

      return;
    }

  task.n         = max_n;
  task.func      = func;
  task.user_data = user_data;

  g_atomic_int_set (&gimp_parallel_completion_counter, task.n - 1);

  for (i = 0; i < task.n - 1; i++)
    {
      GimpParallelThread *thread = &gimp_parallel_threads[i];

      g_mutex_lock (&thread->mutex);

      thread->task = &task;
      thread->i    = i;

      g_cond_signal (&thread->cond);

      g_mutex_unlock (&thread->mutex);
    }

  func (i, task.n, user_data);

  if (g_atomic_int_get (&gimp_parallel_completion_counter))
    {
      g_mutex_lock (&gimp_parallel_completion_mutex);

      while (g_atomic_int_get (&gimp_parallel_completion_counter))
        {
          g_cond_wait (&gimp_parallel_completion_cond,
                       &gimp_parallel_completion_mutex);
        }

      g_mutex_unlock (&gimp_parallel_completion_mutex);
    }

  g_atomic_int_set (&gimp_parallel_busy, 0);
}

void
gimp_parallel_distribute_range (gsize                           size,
                                gsize                           min_sub_size,
                                GimpParallelDistributeRangeFunc func,
                                gpointer                        user_data)
{
  gsize n = size;

  g_return_if_fail (func != NULL);

  if (size == 0)
    return;

  if (min_sub_size > 1)
    n /= min_sub_size;

  n = CLAMP (n, 1, gimp_parallel_n_threads);

  gimp_parallel_distribute (n, [=] (gint i, gint n)
    {
      gsize offset;
      gsize sub_size;

      offset   = (2 * i       * size + n) / (2 * n);
      sub_size = (2 * (i + 1) * size + n) / (2 * n) - offset;

      func (offset, sub_size, user_data);
    });
}

void
gimp_parallel_distribute_area (const GeglRectangle            *area,
                               gsize                           min_sub_area,
                               GimpParallelDistributeAreaFunc  func,
                               gpointer                        user_data)
{
  gsize n;

  g_return_if_fail (area != NULL);
  g_return_if_fail (func != NULL);

  if (area->width <= 0 || area->height <= 0)
    return;

  n = (gsize) area->width * (gsize) area->height;

  if (min_sub_area > 1)
    n /= min_sub_area;

  n = CLAMP (n, 1, gimp_parallel_n_threads);

  gimp_parallel_distribute (n, [=] (gint i, gint n)
    {
      GeglRectangle sub_area;

      if (area->width <= area->height)
        {
          sub_area.x       = area->x;
          sub_area.width   = area->width;

          sub_area.y       = (2 * i       * area->height + n) / (2 * n);
          sub_area.height  = (2 * (i + 1) * area->height + n) / (2 * n);

          sub_area.height -= sub_area.y;
          sub_area.y      += area->y;
        }
      else
        {
          sub_area.y       = area->y;
          sub_area.height  = area->height;

          sub_area.x       = (2 * i       * area->width + n) / (2 * n);
          sub_area.width   = (2 * (i + 1) * area->width + n) / (2 * n);

          sub_area.width  -= sub_area.x;
          sub_area.x      += area->x;
        }

      func (&sub_area, user_data);
    });
}


/*  private functions  */


static void
gimp_parallel_notify_num_processors (GimpGeglConfig *config)
{
  gimp_parallel_set_n_threads (config->num_processors);
}

static void
gimp_parallel_set_n_threads (gint n_threads)
{
  gint i;

  if (! g_atomic_int_compare_and_exchange (&gimp_parallel_busy, 0, 1))
    g_return_if_reached ();

  n_threads = CLAMP (n_threads, 1, GIMP_PARALLEL_MAX_THREADS + 1);

  if (n_threads > gimp_parallel_n_threads) /* need more threads */
    {
      for (i = gimp_parallel_n_threads - 1; i < n_threads - 1; i++)
        {
          GimpParallelThread *thread = &gimp_parallel_threads[i];

          thread->quit = FALSE;
          thread->task = NULL;

          thread->thread = g_thread_new ("worker",
                                         (GThreadFunc) gimp_parallel_thread_func,
                                         thread);
        }
    }
  else if (n_threads < gimp_parallel_n_threads) /* need less threads */
    {
      for (i = n_threads - 1; i < gimp_parallel_n_threads - 1; i++)
        {
          GimpParallelThread *thread = &gimp_parallel_threads[i];

          g_mutex_lock (&thread->mutex);

          thread->quit = TRUE;
          g_cond_signal (&thread->cond);

          g_mutex_unlock (&thread->mutex);
        }

      for (i = n_threads - 1; i < gimp_parallel_n_threads - 1; i++)
        {
          GimpParallelThread *thread = &gimp_parallel_threads[i];

          g_thread_join (thread->thread);
        }
    }

  gimp_parallel_n_threads = n_threads;

  g_atomic_int_set (&gimp_parallel_busy, 0);
}

static gpointer
gimp_parallel_thread_func (GimpParallelThread *thread)
{
  g_mutex_lock (&thread->mutex);

  while (TRUE)
    {
      g_cond_wait (&thread->cond, &thread->mutex);

      if (thread->quit)
        {
          break;
        }
      else if (thread->task)
        {
          thread->task->func (thread->i, thread->task->n,
                              thread->task->user_data);

          if (g_atomic_int_dec_and_test (&gimp_parallel_completion_counter))
            {
              g_mutex_lock (&gimp_parallel_completion_mutex);

              g_cond_signal (&gimp_parallel_completion_cond);

              g_mutex_unlock (&gimp_parallel_completion_mutex);
            }

          thread->task = NULL;
        }
    }

  g_mutex_unlock (&thread->mutex);

  return NULL;
}

} /* extern "C" */
