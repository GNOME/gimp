/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcancelable.c
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


enum
{
  CANCEL,
  LAST_SIGNAL
};


G_DEFINE_INTERFACE (GimpCancelable, gimp_cancelable, G_TYPE_OBJECT)


static guint cancelable_signals[LAST_SIGNAL] = { 0 };


/*  private functions  */


static void
gimp_cancelable_default_init (GimpCancelableInterface *iface)
{
  cancelable_signals[CANCEL] =
    g_signal_new ("cancel",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpCancelableInterface, cancel),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}


/*  public functions  */


void
gimp_cancelable_cancel (GimpCancelable *cancelable)
{
  g_return_if_fail (GIMP_IS_CANCELABLE (cancelable));

  g_signal_emit (cancelable, cancelable_signals[CANCEL], 0);
}
