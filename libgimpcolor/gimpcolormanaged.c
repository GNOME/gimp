/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * GimpColorManaged interface
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gio/gio.h>
#include <gegl.h>

#include "gimpcolortypes.h"

#include "gimpcolormanaged.h"
#include "gimpcolorprofile.h"


/**
 * SECTION: gimpcolormanaged
 * @title: GimpColorManaged
 * @short_description: An interface dealing with color profiles.
 *
 * An interface dealing with color profiles.
 **/


enum
{
  PROFILE_CHANGED,
  SIMULATION_PROFILE_CHANGED,
  SIMULATION_INTENT_CHANGED,
  SIMULATION_BPC_CHANGED,
  LAST_SIGNAL
};


G_DEFINE_INTERFACE (GimpColorManaged, gimp_color_managed, G_TYPE_OBJECT)


static guint gimp_color_managed_signals[LAST_SIGNAL] = { 0 };


/*  private functions  */


static void
gimp_color_managed_default_init (GimpColorManagedInterface *iface)
{
  gimp_color_managed_signals[PROFILE_CHANGED] =
    g_signal_new ("profile-changed",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorManagedInterface,
                                   profile_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_color_managed_signals[SIMULATION_PROFILE_CHANGED] =
    g_signal_new ("simulation-profile-changed",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorManagedInterface,
                                   simulation_profile_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_color_managed_signals[SIMULATION_INTENT_CHANGED] =
    g_signal_new ("simulation-intent-changed",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorManagedInterface,
                                   simulation_intent_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_color_managed_signals[SIMULATION_BPC_CHANGED] =
    g_signal_new ("simulation-bpc-changed",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpColorManagedInterface,
                                   simulation_bpc_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}


/*  public functions  */


/**
 * gimp_color_managed_get_icc_profile:
 * @managed: an object the implements the #GimpColorManaged interface
 * @len: (out): return location for the number of bytes in the profile data
 *
 * Returns: (array length=len): A blob of data that represents an ICC color
 *                              profile.
 *
 * Since: 2.4
 */
const guint8 *
gimp_color_managed_get_icc_profile (GimpColorManaged *managed,
                                    gsize            *len)
{
  GimpColorManagedInterface *iface;

  g_return_val_if_fail (GIMP_IS_COLOR_MANAGED (managed), NULL);
  g_return_val_if_fail (len != NULL, NULL);

  *len = 0;

  iface = GIMP_COLOR_MANAGED_GET_IFACE (managed);

  if (iface->get_icc_profile)
    return iface->get_icc_profile (managed, len);

  return NULL;
}

/**
 * gimp_color_managed_get_color_profile:
 * @managed: an object the implements the #GimpColorManaged interface
 *
 * This function always returns a #GimpColorProfile and falls back to
 * gimp_color_profile_new_rgb_srgb() if the method is not implemented.
 *
 * Returns: (transfer full): The @managed's #GimpColorProfile.
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_color_managed_get_color_profile (GimpColorManaged *managed)
{
  GimpColorManagedInterface *iface;

  g_return_val_if_fail (GIMP_IS_COLOR_MANAGED (managed), NULL);

  iface = GIMP_COLOR_MANAGED_GET_IFACE (managed);

  if (iface->get_color_profile)
    return iface->get_color_profile (managed);

  return NULL;
}

/**
 * gimp_color_managed_get_simulation_profile:
 * @managed: an object the implements the #GimpColorManaged interface
 *
 * This function always returns a #GimpColorProfile
 *
 * Returns: (transfer full): The @managed's simulation #GimpColorProfile.
 *
 * Since: 3.0
 **/
GimpColorProfile *
gimp_color_managed_get_simulation_profile (GimpColorManaged *managed)
{
  GimpColorManagedInterface *iface;

  g_return_val_if_fail (GIMP_IS_COLOR_MANAGED (managed), NULL);

  iface = GIMP_COLOR_MANAGED_GET_IFACE (managed);

  if (iface->get_simulation_profile)
    return iface->get_simulation_profile (managed);

  return NULL;
}

/**
 * gimp_color_managed_get_simulation_intent:
 * @managed: an object the implements the #GimpColorManaged interface
 *
 * This function always returns a #GimpColorRenderingIntent
 *
 * Returns: The @managed's simulation #GimpColorRenderingIntent.
 *
 * Since: 3.0
 **/
GimpColorRenderingIntent
gimp_color_managed_get_simulation_intent (GimpColorManaged *managed)
{
  GimpColorManagedInterface *iface;

  g_return_val_if_fail (GIMP_IS_COLOR_MANAGED (managed),
                        GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC);

  iface = GIMP_COLOR_MANAGED_GET_IFACE (managed);

  if (iface->get_simulation_intent)
    return iface->get_simulation_intent (managed);

  return GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC;
}

/**
 * gimp_color_managed_get_simulation_bpc:
 * @managed: an object the implements the #GimpColorManaged interface
 *
 * This function always returns a gboolean representing whether
 * Black Point Compensation is enabled
 *
 * Returns: The @managed's simulation Black Point Compensation value.
 *
 * Since: 3.0
 **/
gboolean
gimp_color_managed_get_simulation_bpc (GimpColorManaged *managed)
{
  GimpColorManagedInterface *iface;

  g_return_val_if_fail (GIMP_IS_COLOR_MANAGED (managed), FALSE);

  iface = GIMP_COLOR_MANAGED_GET_IFACE (managed);

  if (iface->get_simulation_bpc)
    return iface->get_simulation_bpc (managed);

  return FALSE;
}


/**
 * gimp_color_managed_profile_changed:
 * @managed: an object that implements the #GimpColorManaged interface
 *
 * Emits the "profile-changed" signal.
 *
 * Since: 2.4
 **/
void
gimp_color_managed_profile_changed (GimpColorManaged *managed)
{
  g_return_if_fail (GIMP_IS_COLOR_MANAGED (managed));

  g_signal_emit (managed, gimp_color_managed_signals[PROFILE_CHANGED], 0);
}

/**
 * gimp_color_managed_simulation_profile_changed:
 * @managed: an object that implements the #GimpColorManaged interface
 *
 * Emits the "simulation-profile-changed" signal.
 *
 * Since: 3.0
 **/
void
gimp_color_managed_simulation_profile_changed (GimpColorManaged *managed)
{
  g_return_if_fail (GIMP_IS_COLOR_MANAGED (managed));

  g_signal_emit (managed, gimp_color_managed_signals[SIMULATION_PROFILE_CHANGED], 0);
}

/**
 * gimp_color_managed_simulation_intent_changed:
 * @managed: an object that implements the #GimpColorManaged interface
 *
 * Emits the "simulation-intent-changed" signal.
 *
 * Since: 3.0
 **/
void
gimp_color_managed_simulation_intent_changed (GimpColorManaged *managed)
{
  g_return_if_fail (GIMP_IS_COLOR_MANAGED (managed));

  g_signal_emit (managed, gimp_color_managed_signals[SIMULATION_INTENT_CHANGED], 0);
}

/**
 * gimp_color_managed_simulation_bpc_changed:
 * @managed: an object that implements the #GimpColorManaged interface
 *
 * Emits the "simulation-bpc-changed" signal.
 *
 * Since: 3.0
 **/
void
gimp_color_managed_simulation_bpc_changed (GimpColorManaged *managed)
{
  g_return_if_fail (GIMP_IS_COLOR_MANAGED (managed));

  g_signal_emit (managed, gimp_color_managed_signals[SIMULATION_BPC_CHANGED], 0);
}
