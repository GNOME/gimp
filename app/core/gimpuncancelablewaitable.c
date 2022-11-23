/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmauncancelablewaitable.c
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

#include "core-types.h"

#include "ligmauncancelablewaitable.h"
#include "ligmawaitable.h"


/*  local function prototypes  */

static void       ligma_uncancelable_waitable_waitable_iface_init (LigmaWaitableInterface   *iface);

static void       ligma_uncancelable_waitable_finalize            (GObject                 *object);

static void       ligma_uncancelable_waitable_wait                (LigmaWaitable            *waitable);
static gboolean   ligma_uncancelable_waitable_try_wait            (LigmaWaitable            *waitable);
static gboolean   ligma_uncancelable_waitable_wait_until          (LigmaWaitable            *waitable,
                                                                  gint64                   end_time);


G_DEFINE_TYPE_WITH_CODE (LigmaUncancelableWaitable, ligma_uncancelable_waitable, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_WAITABLE,
                                                ligma_uncancelable_waitable_waitable_iface_init))

#define parent_class ligma_uncancelable_waitable_parent_class


/*  private functions  */


static void
ligma_uncancelable_waitable_class_init (LigmaUncancelableWaitableClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ligma_uncancelable_waitable_finalize;
}

static void
ligma_uncancelable_waitable_waitable_iface_init (LigmaWaitableInterface *iface)
{
  iface->wait       = ligma_uncancelable_waitable_wait;
  iface->try_wait   = ligma_uncancelable_waitable_try_wait;
  iface->wait_until = ligma_uncancelable_waitable_wait_until;
}

static void
ligma_uncancelable_waitable_init (LigmaUncancelableWaitable *uncancelable_waitable)
{
}

static void
ligma_uncancelable_waitable_finalize (GObject *object)
{
  LigmaUncancelableWaitable *uncancelable_waitable =
    LIGMA_UNCANCELABLE_WAITABLE (object);

  g_clear_object (&uncancelable_waitable->waitable);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_uncancelable_waitable_wait (LigmaWaitable *waitable)
{
  LigmaUncancelableWaitable *uncancelable_waitable =
    LIGMA_UNCANCELABLE_WAITABLE (waitable);

  if (uncancelable_waitable->waitable)
    ligma_waitable_wait (uncancelable_waitable->waitable);
}

static gboolean
ligma_uncancelable_waitable_try_wait (LigmaWaitable *waitable)
{
  LigmaUncancelableWaitable *uncancelable_waitable =
    LIGMA_UNCANCELABLE_WAITABLE (waitable);

  if (uncancelable_waitable->waitable)
    return ligma_waitable_try_wait (uncancelable_waitable->waitable);
  else
    return TRUE;
}

static gboolean
ligma_uncancelable_waitable_wait_until (LigmaWaitable *waitable,
                                gint64        end_time)
{
  LigmaUncancelableWaitable *uncancelable_waitable =
    LIGMA_UNCANCELABLE_WAITABLE (waitable);

  if (uncancelable_waitable->waitable)
    return ligma_waitable_wait_until (uncancelable_waitable->waitable, end_time);
  else
    return TRUE;
}


/*  public functions  */


LigmaWaitable *
ligma_uncancelable_waitable_new (LigmaWaitable *waitable)
{
  LigmaUncancelableWaitable *uncancelable_waitable;

  g_return_val_if_fail (LIGMA_IS_WAITABLE (waitable), NULL);

  uncancelable_waitable = g_object_new (LIGMA_TYPE_UNCANCELABLE_WAITABLE, NULL);

  uncancelable_waitable->waitable = g_object_ref (waitable);

  return LIGMA_WAITABLE (uncancelable_waitable);
}
