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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimpcontext.h"
#include "gimptoolpreset.h"
#include "gimptoolpreset-load.h"

#include "gimp-intl.h"


GList *
gimp_tool_preset_load (GimpContext  *context,
                       const gchar  *filename,
                       GError      **error)
{
  GimpToolPreset *tool_preset;

  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (filename), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  tool_preset = g_object_new (GIMP_TYPE_TOOL_PRESET,
                              "gimp", context->gimp,
                              NULL);

  if (gimp_config_deserialize_file (GIMP_CONFIG (tool_preset),
                                    filename,
                                    NULL, error))
    {
      if (GIMP_IS_CONTEXT (tool_preset->tool_options))
        {
          return g_list_prepend (NULL, tool_preset);
        }
      else
        {
          g_set_error (error, GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_PARSE,
                       _("Error while parsing '%s'"),
                       gimp_filename_to_utf8 (filename));
        }
    }

  g_object_unref (tool_preset);

  return NULL;
}
