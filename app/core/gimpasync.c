/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpasync.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "gimpasync.h"


/* GimpAsync represents an asynchronous task.  Both the public and the
 * protected interfaces are intentionally minimal at this point, to keep things
 * simple.  They may be extended in the future as needed.
 *
 * Upon creation, a GimpAsync object is in the "running" state.  Once the task
 * is complete (and before the object's destruction), it should be transitioned
 * to the "stopped" state, using either 'gimp_async_finish()' or
 * 'gimp_async_abort()'.
 *
 * Note that certain GimpAsync functions may be only be called during a certain
 * state, or on a certain thread, as detailed for each function.  When
 * referring to threads, the "main thread" is the thread running the main loop,
 * and the "async thread" is the thread calling 'gimp_async_finish()' or
 * 'gimp_async_abort()' (which may be the main thread).  The main thread is
 * said to be "synced" with the async thread if both are the same thread, or
 * after calling 'gimp_async_wait()' on the main thread.
 */


typedef struct _GimpAsyncCallbackInfo GimpAsyncCallbackInfo;


struct _GimpAsyncCallbackInfo
{
  GimpAsyncCallback callback;
  gpointer          data;
};

struct _GimpAsyncPrivate
{
  GMutex          mutex;
  GCond           cond;

  GSList         *callbacks;

  gpointer        result;
  GDestroyNotify  result_destroy_func;

  guint           idle_id;

  gboolean        stopped;
  gboolean        finished;
  gboolean        canceled;
};


/*  local function prototypes  */

static void       gimp_async_dispose       (GObject      *object);
static void       gimp_async_finalize      (GObject      *object);

static gboolean   gimp_async_idle          (GimpAsync    *async);

static void       gimp_async_stop          (GimpAsync    *async);


G_DEFINE_TYPE (GimpAsync, gimp_async, G_TYPE_OBJECT)

#define parent_class gimp_async_parent_class


/*  private functions  */


static void
gimp_async_class_init (GimpAsyncClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose  = gimp_async_dispose;
  object_class->finalize = gimp_async_finalize;

  g_type_class_add_private (klass, sizeof (GimpAsyncPrivate));
}

static void
gimp_async_init (GimpAsync *async)
{
  async->priv = G_TYPE_INSTANCE_GET_PRIVATE (async,
                                             GIMP_TYPE_ASYNC,
                                             GimpAsyncPrivate);

  g_mutex_init (&async->priv->mutex);
  g_cond_init  (&async->priv->cond);
}

static void
gimp_async_dispose (GObject *object)
{
  GimpAsync *async = GIMP_ASYNC (object);

  g_return_if_fail (async->priv->stopped);

  if (async->priv->finished &&
      async->priv->result   &&
      async->priv->result_destroy_func)
    {
      async->priv->result_destroy_func (async->priv->result);

      async->priv->result = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_async_finalize (GObject *object)
{
  GimpAsync *async = GIMP_ASYNC (object);

  g_cond_clear  (&async->priv->cond);
  g_mutex_clear (&async->priv->mutex);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_async_idle (GimpAsync *async)
{
  GSList *iter;

  async->priv->idle_id = 0;

  async->priv->callbacks = g_slist_reverse (async->priv->callbacks);

  for (iter = async->priv->callbacks; iter; iter = g_slist_next (iter))
    {
      GimpAsyncCallbackInfo *callback_info = iter->data;

      callback_info->callback (async, callback_info->data);

      g_slice_free (GimpAsyncCallbackInfo, callback_info);
    }

  g_slist_free (async->priv->callbacks);
  async->priv->callbacks = NULL;

  g_object_unref (async);

  return G_SOURCE_REMOVE;
}

static void
gimp_async_stop (GimpAsync *async)
{
  if (async->priv->callbacks)
    {
      g_object_ref (async);

      async->priv->idle_id = g_idle_add_full (G_PRIORITY_DEFAULT,
                                              (GSourceFunc) gimp_async_idle,
                                              async, NULL);
    }

  async->priv->stopped = TRUE;

  g_cond_broadcast (&async->priv->cond);
}


/*  public functions  */


/* creates a new GimpAsync object, initially placed in the "running" state. */
GimpAsync *
gimp_async_new (void)
{
  return g_object_new (GIMP_TYPE_ASYNC,
                       NULL);
}

/* checks if 'async' is in the "stopped" state */
gboolean
gimp_async_is_stopped (GimpAsync *async)
{
  g_return_val_if_fail (GIMP_IS_ASYNC (async), FALSE);

  return async->priv->stopped;
}

/* waits for 'async' to transition to the "stopped" state.  if 'async' is
 * already stopped, returns immediately.
 *
 * after the call, all callbacks previously added through
 * 'gimp_async_add_callback()' are guaranteed to have been called.
 *
 * may only be called on the main thread.
 */
void
gimp_async_wait (GimpAsync *async)
{
  g_return_if_fail (GIMP_IS_ASYNC (async));

  g_mutex_lock (&async->priv->mutex);

  while (! async->priv->stopped)
    g_cond_wait (&async->priv->cond, &async->priv->mutex);

  g_mutex_unlock (&async->priv->mutex);

  if (async->priv->idle_id)
    {
      g_source_remove (async->priv->idle_id);

      gimp_async_idle (async);
    }
}

/* registers a callback to be called when 'async' transitions to the "stopped"
 * state.  if 'async' is already stopped, the callback is called directly.
 *
 * callbacks added before 'async' was stopped are called in the order in which
 * they were added.  'async' is guaranteed to be kept alive, even without an
 * external reference, between the point where it was stopped, and until all
 * previously added callbacks have been called.
 *
 * the callback is guaranteed to be called on the main thread.
 *
 * may only be called on the main thread.
 */
void
gimp_async_add_callback (GimpAsync         *async,
                         GimpAsyncCallback  callback,
                         gpointer           data)
{
  GimpAsyncCallbackInfo *callback_info;

  g_return_if_fail (GIMP_IS_ASYNC (async));
  g_return_if_fail (callback != NULL);

  g_mutex_lock (&async->priv->mutex);

  if (async->priv->stopped && ! async->priv->idle_id)
    {
      g_mutex_unlock (&async->priv->mutex);

      callback (async, data);

      return;
    }

  callback_info           = g_slice_new (GimpAsyncCallbackInfo);
  callback_info->callback = callback;
  callback_info->data     = data;

  async->priv->callbacks = g_slist_prepend (async->priv->callbacks,
                                            callback_info);

  g_mutex_unlock (&async->priv->mutex);
}

/* transitions 'async' to the "stopped" state, indicating that the task
 * completed normally, possibly providing a result.
 *
 * 'async' shall be in the "running" state.
 *
 * may only be called on the async thread.
 */
void
gimp_async_finish (GimpAsync *async,
                   gpointer   result)
{
  gimp_async_finish_full (async, result, NULL);
}

/* same as 'gimp_async_finish()', taking an additional GDestroyNotify function,
 * used for freeing the result when 'async' is destroyed.
 */
void
gimp_async_finish_full (GimpAsync      *async,
                        gpointer        result,
                        GDestroyNotify  result_destroy_func)
{
  g_return_if_fail (GIMP_IS_ASYNC (async));
  g_return_if_fail (! async->priv->stopped);

  g_mutex_lock (&async->priv->mutex);

  async->priv->finished            = TRUE;
  async->priv->result              = result;
  async->priv->result_destroy_func = result_destroy_func;

  gimp_async_stop (async);

  g_mutex_unlock (&async->priv->mutex);
}

/* checks if 'async' completed normally, using 'gimp_async_finish()' (in
 * contrast to 'gimp_async_abort()').
 *
 * 'async' shall be in the "stopped" state.
 *
 * may only be called on the async thread, or on the main thread when synced
 * with the async thread.
 */
gboolean
gimp_async_is_finished (GimpAsync *async)
{
  g_return_val_if_fail (GIMP_IS_ASYNC (async), FALSE);

  return async->priv->finished;
}

/* returns the result of 'async', as passed to 'gimp_async_finish()'.
 *
 * 'async' shall be in the "stopped" state, and should have completed normally.
 *
 * may only be called on the async thread, or on the main thread when synced
 * with the async thread.
 */
gpointer
gimp_async_get_result (GimpAsync *async)
{
  g_return_val_if_fail (GIMP_IS_ASYNC (async), NULL);
  g_return_val_if_fail (async->priv->finished, NULL);

  return async->priv->result;
}

/* requests the cancellation of the task managed by 'async'. */
void
gimp_async_cancel (GimpAsync *async)
{
  g_return_if_fail (GIMP_IS_ASYNC (async));

  async->priv->canceled = TRUE;
}

/* checks if cancellation of 'async' have been requested.
 *
 * note that a return value of TRUE only indicates that 'gimp_async_cancel()'
 * has been called for 'async', and not that 'async' is stopped.
 */
gboolean
gimp_async_is_canceled (GimpAsync *async)
{
  g_return_val_if_fail (GIMP_IS_ASYNC (async), FALSE);

  return async->priv->canceled;
}

/* transitions 'async' to the "stopped" state, indicating that the task
 * was stopped before completion (normally, in response to a
 * 'gimp_async_cancel()' call).
 *
 * 'async' shall be in the "running" state.
 *
 * may only be called on the async thread.
 */
void
gimp_async_abort (GimpAsync *async)
{
  g_return_if_fail (GIMP_IS_ASYNC (async));
  g_return_if_fail (! async->priv->stopped);

  g_mutex_lock (&async->priv->mutex);

  gimp_async_stop (async);

  g_mutex_unlock (&async->priv->mutex);
}
