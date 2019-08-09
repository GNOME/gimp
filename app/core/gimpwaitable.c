/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpwaitable.c
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

#include "gimpwaitable.h"


G_DEFINE_INTERFACE (GimpWaitable, gimp_waitable, G_TYPE_OBJECT)


/*  private functions  */


static void
gimp_waitable_default_init (GimpWaitableInterface *iface)
{
}


/*  public functions  */


void
gimp_waitable_wait (GimpWaitable *waitable)
{
  GimpWaitableInterface *iface;

  g_return_if_fail (GIMP_IS_WAITABLE (waitable));

  iface = GIMP_WAITABLE_GET_IFACE (waitable);

  if (iface->wait)
    iface->wait (waitable);
}

gboolean
gimp_waitable_try_wait (GimpWaitable *waitable)
{
  GimpWaitableInterface *iface;

  g_return_val_if_fail (GIMP_IS_WAITABLE (waitable), FALSE);

  iface = GIMP_WAITABLE_GET_IFACE (waitable);

  if (iface->try_wait)
    {
      return iface->try_wait (waitable);
    }
  else
    {
      gimp_waitable_wait (waitable);

      return TRUE;
    }
}

gboolean
gimp_waitable_wait_until (GimpWaitable *waitable,
                          gint64        end_time)
{
  GimpWaitableInterface *iface;

  g_return_val_if_fail (GIMP_IS_WAITABLE (waitable), FALSE);

  iface = GIMP_WAITABLE_GET_IFACE (waitable);

  if (iface->wait_until)
    {
      return iface->wait_until (waitable, end_time);
    }
  else
    {
      gimp_waitable_wait (waitable);

      return TRUE;
    }
}

gboolean
gimp_waitable_wait_for (GimpWaitable *waitable,
                        gint64        wait_duration)
{
  g_return_val_if_fail (GIMP_IS_WAITABLE (waitable), FALSE);

  if (wait_duration <= 0)
    {
      return gimp_waitable_try_wait (waitable);
    }
  else
    {
      return gimp_waitable_wait_until (waitable,
                                       g_get_monotonic_time () + wait_duration);
    }
}
