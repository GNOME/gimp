/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligma-parallel.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "config.h"

#include <gio/gio.h>
#include <gegl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef G_OS_WIN32
#include <windows.h>
#endif

extern "C"
{

#include "core-types.h"

#include "config/ligmageglconfig.h"

#include "ligma.h"
#include "ligma-parallel.h"
#include "ligmaasync.h"
#include "ligmacancelable.h"


#define LIGMA_PARALLEL_MAX_THREADS           64
#define LIGMA_PARALLEL_RUN_ASYNC_MAX_THREADS  1


typedef struct
{
  LigmaAsync        *async;
  gint              priority;
  LigmaRunAsyncFunc  func;
  gpointer          user_data;
  GDestroyNotify    user_data_destroy_func;
} LigmaParallelRunAsyncTask;

typedef struct
{
  GThread   *thread;

  gboolean   quit;

  LigmaAsync *current_async;
} LigmaParallelRunAsyncThread;


/*  local function prototypes  */

static void                       ligma_parallel_notify_num_processors   (LigmaGeglConfig             *config);

static void                       ligma_parallel_set_n_threads           (gint                        n_threads,
                                                                         gboolean                    finish_tasks);

static void                       ligma_parallel_run_async_set_n_threads (gint                        n_threads,
                                                                         gboolean                    finish_tasks);
static gpointer                   ligma_parallel_run_async_thread_func   (LigmaParallelRunAsyncThread *thread);
static void                       ligma_parallel_run_async_enqueue_task  (LigmaParallelRunAsyncTask   *task);
static LigmaParallelRunAsyncTask * ligma_parallel_run_async_dequeue_task  (void);
static gboolean                   ligma_parallel_run_async_execute_task  (LigmaParallelRunAsyncTask   *task);
static void                       ligma_parallel_run_async_abort_task    (LigmaParallelRunAsyncTask   *task);
static void                       ligma_parallel_run_async_cancel        (LigmaAsync                  *async);
static void                       ligma_parallel_run_async_waiting       (LigmaAsync                  *async);


/*  local variables  */

static gint                       ligma_parallel_run_async_n_threads = 0;
static LigmaParallelRunAsyncThread ligma_parallel_run_async_threads[LIGMA_PARALLEL_RUN_ASYNC_MAX_THREADS];

static GMutex                     ligma_parallel_run_async_mutex;
static GCond                      ligma_parallel_run_async_cond;
static GQueue                     ligma_parallel_run_async_queue = G_QUEUE_INIT;


/*  public functions  */


void
ligma_parallel_init (Ligma *ligma)
{
  LigmaGeglConfig *config;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  config = LIGMA_GEGL_CONFIG (ligma->config);

  g_signal_connect (config, "notify::num-processors",
                    G_CALLBACK (ligma_parallel_notify_num_processors),
                    NULL);

  ligma_parallel_notify_num_processors (config);
}

void
ligma_parallel_exit (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  g_signal_handlers_disconnect_by_func (ligma->config,
                                        (gpointer) ligma_parallel_notify_num_processors,
                                        NULL);

  /* stop all threads */
  ligma_parallel_set_n_threads (0, /* finish_tasks = */ FALSE);
}

LigmaAsync *
ligma_parallel_run_async (LigmaRunAsyncFunc func,
                         gpointer         user_data)
{
  return ligma_parallel_run_async_full (0, func, user_data, NULL);
}

LigmaAsync *
ligma_parallel_run_async_full (gint             priority,
                              LigmaRunAsyncFunc func,
                              gpointer         user_data,
                              GDestroyNotify   user_data_destroy_func)
{
  LigmaAsync                *async;
  LigmaParallelRunAsyncTask *task;

  g_return_val_if_fail (func != NULL, NULL);

  async = ligma_async_new ();

  task = g_slice_new (LigmaParallelRunAsyncTask);

  task->async                  = LIGMA_ASYNC (g_object_ref (async));
  task->priority               = priority;
  task->func                   = func;
  task->user_data              = user_data;
  task->user_data_destroy_func = user_data_destroy_func;

  if (ligma_parallel_run_async_n_threads > 0)
    {
      g_signal_connect_after (async, "cancel",
                              G_CALLBACK (ligma_parallel_run_async_cancel),
                              NULL);
      g_signal_connect_after (async, "waiting",
                              G_CALLBACK (ligma_parallel_run_async_waiting),
                              NULL);

      g_mutex_lock (&ligma_parallel_run_async_mutex);

      ligma_parallel_run_async_enqueue_task (task);

      g_cond_signal (&ligma_parallel_run_async_cond);

      g_mutex_unlock (&ligma_parallel_run_async_mutex);
    }
  else
    {
      while (ligma_parallel_run_async_execute_task (task));
    }

  return async;
}

LigmaAsync *
ligma_parallel_run_async_independent (LigmaRunAsyncFunc func,
                                     gpointer         user_data)
{
  return ligma_parallel_run_async_independent_full (0, func, user_data);
}

LigmaAsync *
ligma_parallel_run_async_independent_full (gint             priority,
                                          LigmaRunAsyncFunc func,
                                          gpointer         user_data)
{
  LigmaAsync                *async;
  LigmaParallelRunAsyncTask *task;
  GThread                  *thread;

  g_return_val_if_fail (func != NULL, NULL);

  async = ligma_async_new ();

  task = g_slice_new0 (LigmaParallelRunAsyncTask);

  task->async     = LIGMA_ASYNC (g_object_ref (async));
  task->priority  = priority;
  task->func      = func;
  task->user_data = user_data;

  thread = g_thread_new (
    "async-ind",
    [] (gpointer data) -> gpointer
    {
      LigmaParallelRunAsyncTask *task = (LigmaParallelRunAsyncTask *) data;

      /* adjust the thread's priority */
#if defined (G_OS_WIN32)
      if (task->priority < 0)
        {
          SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_ABOVE_NORMAL);
        }
      else if (task->priority > 0)
        {
          SetThreadPriority (GetCurrentThread (), THREAD_MODE_BACKGROUND_BEGIN);
        }
#elif defined (HAVE_UNISTD_H) && defined (__gnu_linux__)
      if (task->priority)
        {
          (nice (task->priority) != -1);
                              /* ^-- avoid "unused result" warning */
        }
#endif

      while (ligma_parallel_run_async_execute_task (task));

      return NULL;
    },
    task);

  ligma_async_add_callback (async,
                           [] (LigmaAsync *async,
                               gpointer   thread)
                           {
                             g_thread_join ((GThread *) thread);
                           },
                           thread);

  return async;
}


/*  private functions  */


static void
ligma_parallel_notify_num_processors (LigmaGeglConfig *config)
{
  ligma_parallel_set_n_threads (config->num_processors,
                               /* finish_tasks = */ TRUE);
}

static void
ligma_parallel_set_n_threads (gint     n_threads,
                             gboolean finish_tasks)
{
  ligma_parallel_run_async_set_n_threads (n_threads, finish_tasks);
}

static void
ligma_parallel_run_async_set_n_threads (gint     n_threads,
                                       gboolean finish_tasks)
{
  gint i;

  n_threads = CLAMP (n_threads, 0, LIGMA_PARALLEL_RUN_ASYNC_MAX_THREADS);

  if (n_threads > ligma_parallel_run_async_n_threads) /* need more threads */
    {
      for (i = ligma_parallel_run_async_n_threads; i < n_threads; i++)
        {
          LigmaParallelRunAsyncThread *thread =
            &ligma_parallel_run_async_threads[i];

          thread->quit = FALSE;

          thread->thread = g_thread_new (
            "async",
            (GThreadFunc) ligma_parallel_run_async_thread_func,
            thread);
        }
    }
  else if (n_threads < ligma_parallel_run_async_n_threads) /* need less threads */
    {
      g_mutex_lock (&ligma_parallel_run_async_mutex);

      for (i = n_threads; i < ligma_parallel_run_async_n_threads; i++)
        {
          LigmaParallelRunAsyncThread *thread =
            &ligma_parallel_run_async_threads[i];

          thread->quit = TRUE;

          if (thread->current_async && ! finish_tasks)
            ligma_cancelable_cancel (LIGMA_CANCELABLE (thread->current_async));
        }

      g_cond_broadcast (&ligma_parallel_run_async_cond);

      g_mutex_unlock (&ligma_parallel_run_async_mutex);

      for (i = n_threads; i < ligma_parallel_run_async_n_threads; i++)
        {
          LigmaParallelRunAsyncThread *thread =
            &ligma_parallel_run_async_threads[i];

          g_thread_join (thread->thread);
        }
    }

  ligma_parallel_run_async_n_threads = n_threads;

  if (n_threads == 0)
    {
      LigmaParallelRunAsyncTask *task;

      /* finish remaining tasks */
      while ((task = ligma_parallel_run_async_dequeue_task ()))
        {
          if (finish_tasks)
            while (ligma_parallel_run_async_execute_task (task));
          else
            ligma_parallel_run_async_abort_task (task);
        }
    }
}

static gpointer
ligma_parallel_run_async_thread_func (LigmaParallelRunAsyncThread *thread)
{
  g_mutex_lock (&ligma_parallel_run_async_mutex);

  while (TRUE)
    {
      LigmaParallelRunAsyncTask *task;

      while (! thread->quit &&
             (task = ligma_parallel_run_async_dequeue_task ()))
        {
          gboolean resume;

          thread->current_async = LIGMA_ASYNC (g_object_ref (task->async));

          do
            {
              g_mutex_unlock (&ligma_parallel_run_async_mutex);

              resume = ligma_parallel_run_async_execute_task (task);

              g_mutex_lock (&ligma_parallel_run_async_mutex);
            }
          while (resume &&
                 (g_queue_is_empty (&ligma_parallel_run_async_queue) ||
                  task->priority <
                  ((LigmaParallelRunAsyncTask *)
                     g_queue_peek_head (
                       &ligma_parallel_run_async_queue))->priority));

          g_clear_object (&thread->current_async);

          if (resume)
            ligma_parallel_run_async_enqueue_task (task);
        }

      if (thread->quit)
        break;

      g_cond_wait (&ligma_parallel_run_async_cond,
                   &ligma_parallel_run_async_mutex);
    }

  g_mutex_unlock (&ligma_parallel_run_async_mutex);

  return NULL;
}

static void
ligma_parallel_run_async_enqueue_task (LigmaParallelRunAsyncTask *task)
{
  GList *link;
  GList *iter;

  if (ligma_async_is_canceled (task->async))
    {
      ligma_parallel_run_async_abort_task (task);

      return;
    }

  link       = g_list_alloc ();
  link->data = task;

  g_object_set_data (G_OBJECT (task->async),
                     "ligma-parallel-run-async-link", link);

  for (iter = g_queue_peek_tail_link (&ligma_parallel_run_async_queue);
       iter;
       iter = g_list_previous (iter))
    {
      LigmaParallelRunAsyncTask *other_task =
        (LigmaParallelRunAsyncTask *) iter->data;

      if (other_task->priority <= task->priority)
        break;
    }

  if (iter)
    {
      link->prev = iter;
      link->next = iter->next;

      iter->next = link;

      if (link->next)
        link->next->prev = link;
      else
        ligma_parallel_run_async_queue.tail = link;

      ligma_parallel_run_async_queue.length++;
    }
  else
    {
      g_queue_push_head_link (&ligma_parallel_run_async_queue, link);
    }
}

static LigmaParallelRunAsyncTask *
ligma_parallel_run_async_dequeue_task (void)
{
  LigmaParallelRunAsyncTask *task;

  task = (LigmaParallelRunAsyncTask *) g_queue_pop_head (
                                        &ligma_parallel_run_async_queue);

  if (task)
    {
      g_object_set_data (G_OBJECT (task->async),
                         "ligma-parallel-run-async-link", NULL);
    }

  return task;
}

static gboolean
ligma_parallel_run_async_execute_task (LigmaParallelRunAsyncTask *task)
{
  if (ligma_async_is_canceled (task->async))
    {
      ligma_parallel_run_async_abort_task (task);

      return FALSE;
    }

  task->func (task->async, task->user_data);

  if (ligma_async_is_stopped (task->async))
    {
      g_object_unref (task->async);

      g_slice_free (LigmaParallelRunAsyncTask, task);

      return FALSE;
    }

  return TRUE;
}

static void
ligma_parallel_run_async_abort_task (LigmaParallelRunAsyncTask *task)
{
  if (task->user_data && task->user_data_destroy_func)
    task->user_data_destroy_func (task->user_data);

  ligma_async_abort (task->async);

  g_object_unref (task->async);

  g_slice_free (LigmaParallelRunAsyncTask, task);
}

static void
ligma_parallel_run_async_cancel (LigmaAsync *async)
{
  GList                    *link;
  LigmaParallelRunAsyncTask *task = NULL;

  link = (GList *) g_object_get_data (G_OBJECT (async),
                                      "ligma-parallel-run-async-link");

  if (! link)
    return;

  g_mutex_lock (&ligma_parallel_run_async_mutex);

  link = (GList *) g_object_get_data (G_OBJECT (async),
                                      "ligma-parallel-run-async-link");

  if (link)
    {
      g_object_set_data (G_OBJECT (async),
                         "ligma-parallel-run-async-link", NULL);

      task = (LigmaParallelRunAsyncTask *) link->data;

      g_queue_delete_link (&ligma_parallel_run_async_queue, link);
    }

  g_mutex_unlock (&ligma_parallel_run_async_mutex);

  if (task)
    ligma_parallel_run_async_abort_task (task);
}

static void
ligma_parallel_run_async_waiting (LigmaAsync *async)
{
  GList *link;

  link = (GList *) g_object_get_data (G_OBJECT (async),
                                      "ligma-parallel-run-async-link");

  if (! link)
    return;

  g_mutex_lock (&ligma_parallel_run_async_mutex);

  link = (GList *) g_object_get_data (G_OBJECT (async),
                                      "ligma-parallel-run-async-link");

  if (link)
    {
      LigmaParallelRunAsyncTask *task = (LigmaParallelRunAsyncTask *) link->data;

      task->priority = G_MININT;

      g_queue_unlink         (&ligma_parallel_run_async_queue, link);
      g_queue_push_head_link (&ligma_parallel_run_async_queue, link);
    }

  g_mutex_unlock (&ligma_parallel_run_async_mutex);
}

} /* extern "C" */
