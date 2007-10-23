/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * GimpColorManaged interface
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "gimpcolortypes.h"

#include "gimpcolormanaged.h"


enum
{
  PROFILE_CHANGED,
  LAST_SIGNAL
};


static void  gimp_color_managed_base_init (GimpColorManagedInterface *iface);


static guint gimp_color_managed_signals[LAST_SIGNAL] = { 0 };


GType
gimp_color_managed_interface_get_type (void)
{
  static GType iface_type = 0;

  if (! iface_type)
    {
      const GTypeInfo iface_info =
      {
        sizeof (GimpColorManagedInterface),
        (GBaseInitFunc)     gimp_color_managed_base_init,
        (GBaseFinalizeFunc) NULL,
      };

      iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                           "GimpColorManagedInterface",
                                           &iface_info, 0);

      g_type_interface_add_prerequisite (iface_type, G_TYPE_OBJECT);
    }

  return iface_type;
}

static void
gimp_color_managed_base_init (GimpColorManagedInterface *iface)
{
  static gboolean initialized = FALSE;

  if (! initialized)
    {
      gimp_color_managed_signals[PROFILE_CHANGED] =
        g_signal_new ("profile-changed",
                      G_TYPE_FROM_INTERFACE (iface),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimpColorManagedInterface,
                                       profile_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

      iface->get_icc_profile = NULL;
      iface->profile_changed = NULL;

      initialized = TRUE;
    }
}

/**
 * gimp_color_managed_get_icc_profile:
 * @managed: an object the implements the #GimpColorManaged interface
 * @len:     return location for the number of bytes in the profile data
 *
 * Return value: A pointer to a blob of data that represents an ICC
 *               color profile.
 *
 * Since: GIMP 2.4
 **/
const guint8 *
gimp_color_managed_get_icc_profile (GimpColorManaged *managed,
                                    gsize            *len)
{
  GimpColorManagedInterface *iface;

  g_return_val_if_fail (GIMP_IS_COLOR_MANAGED (managed), NULL);
  g_return_val_if_fail (len != NULL, NULL);

  *len = 0;

  iface = GIMP_COLOR_MANAGED_GET_INTERFACE (managed);

  if (iface->get_icc_profile)
    return iface->get_icc_profile (managed, len);

  return NULL;
}

/**
 * gimp_color_managed_profile_changed:
 * @managed: an object the implements the #GimpColorManaged interface
 *
 * Emits the "profile-changed" signal.
 *
 * Since: GIMP 2.4
 **/
void
gimp_color_managed_profile_changed (GimpColorManaged *managed)
{
  g_return_if_fail (GIMP_IS_COLOR_MANAGED (managed));

  g_signal_emit (managed, gimp_color_managed_signals[PROFILE_CHANGED], 0);
}
