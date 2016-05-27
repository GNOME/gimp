/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-gegl-utils.h
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gegl.h>

#include "gimp-gegl-types.h"

#include "core/gimpprogress.h"

#include "gimp-gegl-utils.h"


GType
gimp_gegl_get_op_enum_type (const gchar *operation,
                            const gchar *property)
{
  GeglNode   *node;
  GObject    *op;
  GParamSpec *pspec;

  g_return_val_if_fail (operation != NULL, G_TYPE_NONE);
  g_return_val_if_fail (property != NULL, G_TYPE_NONE);

  node = g_object_new (GEGL_TYPE_NODE,
                       "operation", operation,
                       NULL);
  g_object_get (node, "gegl-operation", &op, NULL);
  g_object_unref (node);

  g_return_val_if_fail (op != NULL, G_TYPE_NONE);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (op), property);

  g_return_val_if_fail (G_IS_PARAM_SPEC_ENUM (pspec), G_TYPE_NONE);

  g_object_unref (op);

  return G_TYPE_FROM_CLASS (G_PARAM_SPEC_ENUM (pspec)->enum_class);
}

GeglColor *
gimp_gegl_color_new (const GimpRGB *rgb)
{
  GeglColor *color;

  g_return_val_if_fail (rgb != NULL, NULL);

  color = gegl_color_new (NULL);
  gegl_color_set_pixel (color, babl_format ("R'G'B'A double"), rgb);

  return color;
}

static void
gimp_gegl_progress_callback (GObject      *object,
                             gdouble       value,
                             GimpProgress *progress)
{
  if (value == 0.0)
    {
      const gchar *text = g_object_get_data (object, "gimp-progress-text");

      if (gimp_progress_is_active (progress))
        gimp_progress_set_text (progress, "%s", text);
      else
        gimp_progress_start (progress, FALSE, "%s", text);
    }
  else
    {
      gimp_progress_set_value (progress, value);

      if (value == 1.0)
        gimp_progress_end (progress);
    }
}

void
gimp_gegl_progress_connect (GeglNode     *node,
                            GimpProgress *progress,
                            const gchar  *text)
{
  g_return_if_fail (GEGL_IS_NODE (node));
  g_return_if_fail (GIMP_IS_PROGRESS (progress));
  g_return_if_fail (text != NULL);

  g_signal_connect (node, "progress",
                    G_CALLBACK (gimp_gegl_progress_callback),
                    progress);

  g_object_set_data_full (G_OBJECT (node),
                          "gimp-progress-text", g_strdup (text),
                          (GDestroyNotify) g_free);
}

gboolean
gimp_gegl_param_spec_has_key (GParamSpec  *pspec,
                              const gchar *key,
                              const gchar *value)
{
  const gchar *v = gegl_param_spec_get_property_key (pspec, key);

  if (v && ! strcmp (v, value))
    return TRUE;

  return FALSE;
}
