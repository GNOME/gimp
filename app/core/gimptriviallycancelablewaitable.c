/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatriviallycancelablewaitable.c
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

#include "ligmacancelable.h"
#include "ligmatriviallycancelablewaitable.h"
#include "ligmawaitable.h"


/*  local function prototypes  */

static void   ligma_trivially_cancelable_waitable_cancelable_iface_init (LigmaCancelableInterface *iface);

static void   ligma_trivially_cancelable_waitable_cancel                (LigmaCancelable          *cancelable);


G_DEFINE_TYPE_WITH_CODE (LigmaTriviallyCancelableWaitable, ligma_trivially_cancelable_waitable, LIGMA_TYPE_UNCANCELABLE_WAITABLE,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CANCELABLE,
                                                ligma_trivially_cancelable_waitable_cancelable_iface_init))

#define parent_class ligma_trivially_cancelable_waitable_parent_class


/*  private functions  */


static void
ligma_trivially_cancelable_waitable_class_init (LigmaTriviallyCancelableWaitableClass *klass)
{
}

static void
ligma_trivially_cancelable_waitable_cancelable_iface_init (LigmaCancelableInterface *iface)
{
  iface->cancel = ligma_trivially_cancelable_waitable_cancel;
}

static void
ligma_trivially_cancelable_waitable_init (LigmaTriviallyCancelableWaitable *trivially_cancelable_waitable)
{
}

static void
ligma_trivially_cancelable_waitable_cancel (LigmaCancelable *cancelable)
{
  LigmaUncancelableWaitable *uncancelable_waitable =
    LIGMA_UNCANCELABLE_WAITABLE (cancelable);

  g_clear_object (&uncancelable_waitable->waitable);
}


/*  public functions  */


LigmaWaitable *
ligma_trivially_cancelable_waitable_new (LigmaWaitable *waitable)
{
  LigmaUncancelableWaitable *uncancelable_waitable;

  g_return_val_if_fail (LIGMA_IS_WAITABLE (waitable), NULL);

  uncancelable_waitable = g_object_new (LIGMA_TYPE_TRIVIALLY_CANCELABLE_WAITABLE,
                                        NULL);

  uncancelable_waitable->waitable = g_object_ref (waitable);

  return LIGMA_WAITABLE (uncancelable_waitable);
}
