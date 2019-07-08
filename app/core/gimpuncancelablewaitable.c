/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpuncancelablewaitable.c
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

#include "gimpuncancelablewaitable.h"
#include "gimpwaitable.h"


/*  local function prototypes  */

static void       gimp_uncancelable_waitable_waitable_iface_init (GimpWaitableInterface   *iface);

static void       gimp_uncancelable_waitable_finalize            (GObject                 *object);

static void       gimp_uncancelable_waitable_wait                (GimpWaitable            *waitable);
static gboolean   gimp_uncancelable_waitable_try_wait            (GimpWaitable            *waitable);
static gboolean   gimp_uncancelable_waitable_wait_until          (GimpWaitable            *waitable,
                                                                  gint64                   end_time);


G_DEFINE_TYPE_WITH_CODE (GimpUncancelableWaitable, gimp_uncancelable_waitable, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_WAITABLE,
                                                gimp_uncancelable_waitable_waitable_iface_init))

#define parent_class gimp_uncancelable_waitable_parent_class


/*  private functions  */


static void
gimp_uncancelable_waitable_class_init (GimpUncancelableWaitableClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_uncancelable_waitable_finalize;
}

static void
gimp_uncancelable_waitable_waitable_iface_init (GimpWaitableInterface *iface)
{
  iface->wait       = gimp_uncancelable_waitable_wait;
  iface->try_wait   = gimp_uncancelable_waitable_try_wait;
  iface->wait_until = gimp_uncancelable_waitable_wait_until;
}

static void
gimp_uncancelable_waitable_init (GimpUncancelableWaitable *uncancelable_waitable)
{
}

static void
gimp_uncancelable_waitable_finalize (GObject *object)
{
  GimpUncancelableWaitable *uncancelable_waitable =
    GIMP_UNCANCELABLE_WAITABLE (object);

  g_clear_object (&uncancelable_waitable->waitable);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_uncancelable_waitable_wait (GimpWaitable *waitable)
{
  GimpUncancelableWaitable *uncancelable_waitable =
    GIMP_UNCANCELABLE_WAITABLE (waitable);

  if (uncancelable_waitable->waitable)
    gimp_waitable_wait (uncancelable_waitable->waitable);
}

static gboolean
gimp_uncancelable_waitable_try_wait (GimpWaitable *waitable)
{
  GimpUncancelableWaitable *uncancelable_waitable =
    GIMP_UNCANCELABLE_WAITABLE (waitable);

  if (uncancelable_waitable->waitable)
    return gimp_waitable_try_wait (uncancelable_waitable->waitable);
  else
    return TRUE;
}

static gboolean
gimp_uncancelable_waitable_wait_until (GimpWaitable *waitable,
                                gint64        end_time)
{
  GimpUncancelableWaitable *uncancelable_waitable =
    GIMP_UNCANCELABLE_WAITABLE (waitable);

  if (uncancelable_waitable->waitable)
    return gimp_waitable_wait_until (uncancelable_waitable->waitable, end_time);
  else
    return TRUE;
}


/*  public functions  */


GimpWaitable *
gimp_uncancelable_waitable_new (GimpWaitable *waitable)
{
  GimpUncancelableWaitable *uncancelable_waitable;

  g_return_val_if_fail (GIMP_IS_WAITABLE (waitable), NULL);

  uncancelable_waitable = g_object_new (GIMP_TYPE_UNCANCELABLE_WAITABLE, NULL);

  uncancelable_waitable->waitable = g_object_ref (waitable);

  return GIMP_WAITABLE (uncancelable_waitable);
}
