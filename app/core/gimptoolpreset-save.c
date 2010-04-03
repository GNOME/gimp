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

#include "gimptoolpreset.h"
#include "gimptoolpreset-save.h"


gboolean
gimp_tool_preset_save (GimpData  *data,
                       GError   **error)
{
  g_return_val_if_fail (GIMP_IS_TOOL_PRESET (data), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return gimp_config_serialize_to_file (GIMP_CONFIG (data),
                                        gimp_data_get_filename (data),
                                        "GIMP tool preset file",
                                        "end of GIMP tool preset file",
                                        NULL, error);
}
