/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimpdynamics.h"
#include "gimpdynamics-load.h"


GList *
gimp_dynamics_load (GimpContext  *context,
                    const gchar  *filename,
                    GError      **error)
{
  GimpDynamics *dynamics;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (filename), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  dynamics = g_object_new (GIMP_TYPE_DYNAMICS, NULL);

  if (gimp_config_deserialize_file (GIMP_CONFIG (dynamics),
                                    filename,
                                    NULL, error))
    {
      return g_list_prepend (NULL, dynamics);
    }

  g_object_unref (dynamics);

  return NULL;
}
