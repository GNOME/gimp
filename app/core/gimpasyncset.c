/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpasyncset.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpasync.h"
#include "gimpasyncset.h"
#include "gimpcancelable.h"
#include "gimpwaitable.h"


enum
{
  PROP_0,
  PROP_EMPTY
};


struct _GimpAsyncSetPrivate
{
  GHashTable *asyncs;
};


/*  local function prototypes  */

static void       gimp_async_set_waitable_iface_init   (GimpWaitableInterface   *iface);

static void       gimp_async_set_cancelable_iface_init (GimpCancelableInterface *iface);

static void       gimp_async_set_dispose               (GObject                 *object);
static void       gimp_async_set_finalize              (GObject                 *object);
static void       gimp_async_set_set_property          (GObject                 *object,
                                                        guint                    property_id,
                                                        const GValue            *value,
                                                        GParamSpec              *pspec);
static void       gimp_async_set_get_property          (GObject                 *object,
                                                        guint                    property_id,
                                                        GValue                  *value,
                                                        GParamSpec              *pspec);

static void       gimp_async_set_wait                  (GimpWaitable            *waitable);
static gboolean   gimp_async_set_try_wait              (GimpWaitable            *waitable);
static gboolean   gimp_async_set_wait_until            (GimpWaitable            *waitable,
                                                        gint64                   end_time);

static void       gimp_async_set_cancel                (GimpCancelable          *cancelable);

static void       gimp_async_set_async_callback        (GimpAsync               *async,
                                                        GimpAsyncSet            *async_set);


G_DEFINE_TYPE_WITH_CODE (GimpAsyncSet, gimp_async_set, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (GimpAsyncSet)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_WAITABLE,
                                                gimp_async_set_waitable_iface_init)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CANCELABLE,
                                                gimp_async_set_cancelable_iface_init))

#define parent_class gimp_async_set_parent_class


/*  private functions  */


static void
gimp_async_set_class_init (GimpAsyncSetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = gimp_async_set_dispose;
  object_class->finalize     = gimp_async_set_finalize;
  object_class->set_property = gimp_async_set_set_property;
  object_class->get_property = gimp_async_set_get_property;

  g_object_class_install_property (object_class, PROP_EMPTY,
                                   g_param_spec_boolean ("empty",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READABLE));
}

static void
gimp_async_set_waitable_iface_init (GimpWaitableInterface *iface)
{
  iface->wait       = gimp_async_set_wait;
  iface->try_wait   = gimp_async_set_try_wait;
  iface->wait_until = gimp_async_set_wait_until;
}

static void
gimp_async_set_cancelable_iface_init (GimpCancelableInterface *iface)
{
  iface->cancel = gimp_async_set_cancel;
}

static void
gimp_async_set_init (GimpAsyncSet *async_set)
{
  async_set->priv = gimp_async_set_get_instance_private (async_set);

  async_set->priv->asyncs = g_hash_table_new (NULL, NULL);
}

static void
gimp_async_set_dispose (GObject *object)
{
  GimpAsyncSet *async_set = GIMP_ASYNC_SET (object);

  gimp_async_set_clear (async_set);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_async_set_finalize (GObject *object)
{
  GimpAsyncSet *async_set = GIMP_ASYNC_SET (object);

  g_hash_table_unref (async_set->priv->asyncs);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_async_set_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_async_set_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GimpAsyncSet *async_set = GIMP_ASYNC_SET (object);

  switch (property_id)
    {
    case PROP_EMPTY:
      g_value_set_boolean (value, gimp_async_set_is_empty (async_set));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_async_set_wait (GimpWaitable *waitable)
{
  GimpAsyncSet *async_set = GIMP_ASYNC_SET (waitable);

  while (! gimp_async_set_is_empty (async_set))
    {
      GimpAsync      *async;
      GHashTableIter  iter;

      g_hash_table_iter_init (&iter, async_set->priv->asyncs);

      g_hash_table_iter_next (&iter, (gpointer *) &async, NULL);

      gimp_waitable_wait (GIMP_WAITABLE (async));
    }
}

static gboolean
gimp_async_set_try_wait (GimpWaitable *waitable)
{
  GimpAsyncSet *async_set = GIMP_ASYNC_SET (waitable);

  while (! gimp_async_set_is_empty (async_set))
    {
      GimpAsync      *async;
      GHashTableIter  iter;

      g_hash_table_iter_init (&iter, async_set->priv->asyncs);

      g_hash_table_iter_next (&iter, (gpointer *) &async, NULL);

      if (! gimp_waitable_try_wait (GIMP_WAITABLE (async)))
        return FALSE;
    }

  return TRUE;
}

static gboolean
gimp_async_set_wait_until (GimpWaitable *waitable,
                           gint64        end_time)
{
  GimpAsyncSet *async_set = GIMP_ASYNC_SET (waitable);

  while (! gimp_async_set_is_empty (async_set))
    {
      GimpAsync      *async;
      GHashTableIter  iter;

      g_hash_table_iter_init (&iter, async_set->priv->asyncs);

      g_hash_table_iter_next (&iter, (gpointer *) &async, NULL);

      if (! gimp_waitable_wait_until (GIMP_WAITABLE (async), end_time))
        return FALSE;
    }

  return TRUE;
}

static void
gimp_async_set_cancel (GimpCancelable *cancelable)
{
  GimpAsyncSet *async_set = GIMP_ASYNC_SET (cancelable);
  GList        *list;

  list = g_hash_table_get_keys (async_set->priv->asyncs);

  g_list_foreach (list, (GFunc) g_object_ref, NULL);

  g_list_foreach (list, (GFunc) gimp_cancelable_cancel, NULL);

  g_list_free_full (list, g_object_unref);
}

static void
gimp_async_set_async_callback (GimpAsync    *async,
                               GimpAsyncSet *async_set)
{
  g_hash_table_remove (async_set->priv->asyncs, async);

  if (gimp_async_set_is_empty (async_set))
    g_object_notify (G_OBJECT (async_set), "empty");
}


/*  public functions  */


GimpAsyncSet *
gimp_async_set_new (void)
{
  return g_object_new (GIMP_TYPE_ASYNC_SET,
                       NULL);
}

void
gimp_async_set_add (GimpAsyncSet *async_set,
                    GimpAsync    *async)
{
  g_return_if_fail (GIMP_IS_ASYNC_SET (async_set));
  g_return_if_fail (GIMP_IS_ASYNC (async));

  if (g_hash_table_add (async_set->priv->asyncs, async))
    {
      if (g_hash_table_size (async_set->priv->asyncs) == 1)
        g_object_notify (G_OBJECT (async_set), "empty");

      gimp_async_add_callback (
        async,
        (GimpAsyncCallback) gimp_async_set_async_callback,
        async_set);
    }
}

void
gimp_async_set_remove (GimpAsyncSet *async_set,
                       GimpAsync    *async)
{
  g_return_if_fail (GIMP_IS_ASYNC_SET (async_set));
  g_return_if_fail (GIMP_IS_ASYNC (async));

  if (g_hash_table_remove (async_set->priv->asyncs, async))
    {
      gimp_async_remove_callback (
        async,
        (GimpAsyncCallback) gimp_async_set_async_callback,
        async_set);

      if (g_hash_table_size (async_set->priv->asyncs) == 0)
        g_object_notify (G_OBJECT (async_set), "empty");
    }
}

void
gimp_async_set_clear (GimpAsyncSet *async_set)
{
  GimpAsync      *async;
  GHashTableIter  iter;

  g_return_if_fail (GIMP_IS_ASYNC_SET (async_set));

  if (gimp_async_set_is_empty (async_set))
    return;

  g_hash_table_iter_init (&iter, async_set->priv->asyncs);

  while (g_hash_table_iter_next (&iter, (gpointer *) &async, NULL))
    {
      gimp_async_remove_callback (
        async,
        (GimpAsyncCallback) gimp_async_set_async_callback,
        async_set);
    }

  g_hash_table_remove_all (async_set->priv->asyncs);

  g_object_notify (G_OBJECT (async_set), "empty");
}

gboolean
gimp_async_set_is_empty (GimpAsyncSet *async_set)
{
  g_return_val_if_fail (GIMP_IS_ASYNC_SET (async_set), FALSE);

  return g_hash_table_size (async_set->priv->asyncs) == 0;
}
