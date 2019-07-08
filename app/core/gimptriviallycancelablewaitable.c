/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptriviallycancelablewaitable.c
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

#include "gimpcancelable.h"
#include "gimptriviallycancelablewaitable.h"
#include "gimpwaitable.h"


/*  local function prototypes  */

static void   gimp_trivially_cancelable_waitable_cancelable_iface_init (GimpCancelableInterface *iface);

static void   gimp_trivially_cancelable_waitable_cancel                (GimpCancelable          *cancelable);


G_DEFINE_TYPE_WITH_CODE (GimpTriviallyCancelableWaitable, gimp_trivially_cancelable_waitable, GIMP_TYPE_UNCANCELABLE_WAITABLE,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CANCELABLE,
                                                gimp_trivially_cancelable_waitable_cancelable_iface_init))

#define parent_class gimp_trivially_cancelable_waitable_parent_class


/*  private functions  */


static void
gimp_trivially_cancelable_waitable_class_init (GimpTriviallyCancelableWaitableClass *klass)
{
}

static void
gimp_trivially_cancelable_waitable_cancelable_iface_init (GimpCancelableInterface *iface)
{
  iface->cancel = gimp_trivially_cancelable_waitable_cancel;
}

static void
gimp_trivially_cancelable_waitable_init (GimpTriviallyCancelableWaitable *trivially_cancelable_waitable)
{
}

static void
gimp_trivially_cancelable_waitable_cancel (GimpCancelable *cancelable)
{
  GimpUncancelableWaitable *uncancelable_waitable =
    GIMP_UNCANCELABLE_WAITABLE (cancelable);

  g_clear_object (&uncancelable_waitable->waitable);
}


/*  public functions  */


GimpWaitable *
gimp_trivially_cancelable_waitable_new (GimpWaitable *waitable)
{
  GimpUncancelableWaitable *uncancelable_waitable;

  g_return_val_if_fail (GIMP_IS_WAITABLE (waitable), NULL);

  uncancelable_waitable = g_object_new (GIMP_TYPE_TRIVIALLY_CANCELABLE_WAITABLE,
                                        NULL);

  uncancelable_waitable->waitable = g_object_ref (waitable);

  return GIMP_WAITABLE (uncancelable_waitable);
}
