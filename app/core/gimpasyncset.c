/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaasyncset.c
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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligmaasync.h"
#include "ligmaasyncset.h"
#include "ligmacancelable.h"
#include "ligmawaitable.h"


enum
{
  PROP_0,
  PROP_EMPTY
};


struct _LigmaAsyncSetPrivate
{
  GHashTable *asyncs;
};


/*  local function prototypes  */

static void       ligma_async_set_waitable_iface_init   (LigmaWaitableInterface   *iface);

static void       ligma_async_set_cancelable_iface_init (LigmaCancelableInterface *iface);

static void       ligma_async_set_dispose               (GObject                 *object);
static void       ligma_async_set_finalize              (GObject                 *object);
static void       ligma_async_set_set_property          (GObject                 *object,
                                                        guint                    property_id,
                                                        const GValue            *value,
                                                        GParamSpec              *pspec);
static void       ligma_async_set_get_property          (GObject                 *object,
                                                        guint                    property_id,
                                                        GValue                  *value,
                                                        GParamSpec              *pspec);

static void       ligma_async_set_wait                  (LigmaWaitable            *waitable);
static gboolean   ligma_async_set_try_wait              (LigmaWaitable            *waitable);
static gboolean   ligma_async_set_wait_until            (LigmaWaitable            *waitable,
                                                        gint64                   end_time);

static void       ligma_async_set_cancel                (LigmaCancelable          *cancelable);

static void       ligma_async_set_async_callback        (LigmaAsync               *async,
                                                        LigmaAsyncSet            *async_set);


G_DEFINE_TYPE_WITH_CODE (LigmaAsyncSet, ligma_async_set, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (LigmaAsyncSet)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_WAITABLE,
                                                ligma_async_set_waitable_iface_init)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CANCELABLE,
                                                ligma_async_set_cancelable_iface_init))

#define parent_class ligma_async_set_parent_class


/*  private functions  */


static void
ligma_async_set_class_init (LigmaAsyncSetClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = ligma_async_set_dispose;
  object_class->finalize     = ligma_async_set_finalize;
  object_class->set_property = ligma_async_set_set_property;
  object_class->get_property = ligma_async_set_get_property;

  g_object_class_install_property (object_class, PROP_EMPTY,
                                   g_param_spec_boolean ("empty",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READABLE));
}

static void
ligma_async_set_waitable_iface_init (LigmaWaitableInterface *iface)
{
  iface->wait       = ligma_async_set_wait;
  iface->try_wait   = ligma_async_set_try_wait;
  iface->wait_until = ligma_async_set_wait_until;
}

static void
ligma_async_set_cancelable_iface_init (LigmaCancelableInterface *iface)
{
  iface->cancel = ligma_async_set_cancel;
}

static void
ligma_async_set_init (LigmaAsyncSet *async_set)
{
  async_set->priv = ligma_async_set_get_instance_private (async_set);

  async_set->priv->asyncs = g_hash_table_new (NULL, NULL);
}

static void
ligma_async_set_dispose (GObject *object)
{
  LigmaAsyncSet *async_set = LIGMA_ASYNC_SET (object);

  ligma_async_set_clear (async_set);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_async_set_finalize (GObject *object)
{
  LigmaAsyncSet *async_set = LIGMA_ASYNC_SET (object);

  g_hash_table_unref (async_set->priv->asyncs);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_async_set_set_property (GObject      *object,
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
ligma_async_set_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  LigmaAsyncSet *async_set = LIGMA_ASYNC_SET (object);

  switch (property_id)
    {
    case PROP_EMPTY:
      g_value_set_boolean (value, ligma_async_set_is_empty (async_set));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_async_set_wait (LigmaWaitable *waitable)
{
  LigmaAsyncSet *async_set = LIGMA_ASYNC_SET (waitable);

  while (! ligma_async_set_is_empty (async_set))
    {
      LigmaAsync      *async;
      GHashTableIter  iter;

      g_hash_table_iter_init (&iter, async_set->priv->asyncs);

      g_hash_table_iter_next (&iter, (gpointer *) &async, NULL);

      ligma_waitable_wait (LIGMA_WAITABLE (async));
    }
}

static gboolean
ligma_async_set_try_wait (LigmaWaitable *waitable)
{
  LigmaAsyncSet *async_set = LIGMA_ASYNC_SET (waitable);

  while (! ligma_async_set_is_empty (async_set))
    {
      LigmaAsync      *async;
      GHashTableIter  iter;

      g_hash_table_iter_init (&iter, async_set->priv->asyncs);

      g_hash_table_iter_next (&iter, (gpointer *) &async, NULL);

      if (! ligma_waitable_try_wait (LIGMA_WAITABLE (async)))
        return FALSE;
    }

  return TRUE;
}

static gboolean
ligma_async_set_wait_until (LigmaWaitable *waitable,
                           gint64        end_time)
{
  LigmaAsyncSet *async_set = LIGMA_ASYNC_SET (waitable);

  while (! ligma_async_set_is_empty (async_set))
    {
      LigmaAsync      *async;
      GHashTableIter  iter;

      g_hash_table_iter_init (&iter, async_set->priv->asyncs);

      g_hash_table_iter_next (&iter, (gpointer *) &async, NULL);

      if (! ligma_waitable_wait_until (LIGMA_WAITABLE (async), end_time))
        return FALSE;
    }

  return TRUE;
}

static void
ligma_async_set_cancel (LigmaCancelable *cancelable)
{
  LigmaAsyncSet *async_set = LIGMA_ASYNC_SET (cancelable);
  GList        *list;

  list = g_hash_table_get_keys (async_set->priv->asyncs);

  g_list_foreach (list, (GFunc) g_object_ref, NULL);

  g_list_foreach (list, (GFunc) ligma_cancelable_cancel, NULL);

  g_list_free_full (list, g_object_unref);
}

static void
ligma_async_set_async_callback (LigmaAsync    *async,
                               LigmaAsyncSet *async_set)
{
  g_hash_table_remove (async_set->priv->asyncs, async);

  if (ligma_async_set_is_empty (async_set))
    g_object_notify (G_OBJECT (async_set), "empty");
}


/*  public functions  */


LigmaAsyncSet *
ligma_async_set_new (void)
{
  return g_object_new (LIGMA_TYPE_ASYNC_SET,
                       NULL);
}

void
ligma_async_set_add (LigmaAsyncSet *async_set,
                    LigmaAsync    *async)
{
  g_return_if_fail (LIGMA_IS_ASYNC_SET (async_set));
  g_return_if_fail (LIGMA_IS_ASYNC (async));

  if (g_hash_table_add (async_set->priv->asyncs, async))
    {
      if (g_hash_table_size (async_set->priv->asyncs) == 1)
        g_object_notify (G_OBJECT (async_set), "empty");

      ligma_async_add_callback (
        async,
        (LigmaAsyncCallback) ligma_async_set_async_callback,
        async_set);
    }
}

void
ligma_async_set_remove (LigmaAsyncSet *async_set,
                       LigmaAsync    *async)
{
  g_return_if_fail (LIGMA_IS_ASYNC_SET (async_set));
  g_return_if_fail (LIGMA_IS_ASYNC (async));

  if (g_hash_table_remove (async_set->priv->asyncs, async))
    {
      ligma_async_remove_callback (
        async,
        (LigmaAsyncCallback) ligma_async_set_async_callback,
        async_set);

      if (g_hash_table_size (async_set->priv->asyncs) == 0)
        g_object_notify (G_OBJECT (async_set), "empty");
    }
}

void
ligma_async_set_clear (LigmaAsyncSet *async_set)
{
  LigmaAsync      *async;
  GHashTableIter  iter;

  g_return_if_fail (LIGMA_IS_ASYNC_SET (async_set));

  if (ligma_async_set_is_empty (async_set))
    return;

  g_hash_table_iter_init (&iter, async_set->priv->asyncs);

  while (g_hash_table_iter_next (&iter, (gpointer *) &async, NULL))
    {
      ligma_async_remove_callback (
        async,
        (LigmaAsyncCallback) ligma_async_set_async_callback,
        async_set);
    }

  g_hash_table_remove_all (async_set->priv->asyncs);

  g_object_notify (G_OBJECT (async_set), "empty");
}

gboolean
ligma_async_set_is_empty (LigmaAsyncSet *async_set)
{
  g_return_val_if_fail (LIGMA_IS_ASYNC_SET (async_set), FALSE);

  return g_hash_table_size (async_set->priv->asyncs) == 0;
}
