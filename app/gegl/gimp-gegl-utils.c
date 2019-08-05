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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gegl-plugin.h>

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
gimp_gegl_color_new (const GimpRGB *rgb,
                     const Babl    *space)
{
  GeglColor *color;

  g_return_val_if_fail (rgb != NULL, NULL);

  color = gegl_color_new (NULL);
  gegl_color_set_pixel (color,
                        babl_format_with_space ("R'G'B'A double", space),
                        rgb);

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
gimp_gegl_node_is_source_operation (GeglNode *node)
{
  GeglOperation *operation;

  g_return_val_if_fail (GEGL_IS_NODE (node), FALSE);

  operation = gegl_node_get_gegl_operation (node);

  if (! operation)
    return FALSE;

  return GEGL_IS_OPERATION_SOURCE (operation);
}

gboolean
gimp_gegl_node_is_point_operation (GeglNode *node)
{
  GeglOperation *operation;

  g_return_val_if_fail (GEGL_IS_NODE (node), FALSE);

  operation = gegl_node_get_gegl_operation (node);

  if (! operation)
    return FALSE;

  return GEGL_IS_OPERATION_POINT_RENDER    (operation) ||
         GEGL_IS_OPERATION_POINT_FILTER    (operation) ||
         GEGL_IS_OPERATION_POINT_COMPOSER  (operation) ||
         GEGL_IS_OPERATION_POINT_COMPOSER3 (operation);
}

gboolean
gimp_gegl_node_is_area_filter_operation (GeglNode *node)
{
  GeglOperation *operation;

  g_return_val_if_fail (GEGL_IS_NODE (node), FALSE);

  operation = gegl_node_get_gegl_operation (node);

  if (! operation)
    return FALSE;

  return GEGL_IS_OPERATION_AREA_FILTER (operation) ||
         /* be conservative and return TRUE for meta ops, since they may
          * involve an area op
          */
         GEGL_IS_OPERATION_META (operation);
}

const Babl *
gimp_gegl_node_get_format (GeglNode    *node,
                           const gchar *pad_name)
{
  GeglOperation *op;
  const Babl    *format = NULL;

  g_return_val_if_fail (GEGL_IS_NODE (node), NULL);
  g_return_val_if_fail (pad_name != NULL, NULL);

  g_object_get (node, "gegl-operation", &op, NULL);

  if (op)
    {
      format = gegl_operation_get_format (op, pad_name);

      g_object_unref (op);
    }

  if (! format)
    format = babl_format ("RGBA float");

  return format;
}

void
gimp_gegl_node_set_underlying_operation (GeglNode *node,
                                         GeglNode *operation)
{
  g_return_if_fail (GEGL_IS_NODE (node));
  g_return_if_fail (operation == NULL || GEGL_IS_NODE (operation));

  g_object_set_data (G_OBJECT (node),
                     "gimp-gegl-node-underlying-operation", operation);
}

GeglNode *
gimp_gegl_node_get_underlying_operation (GeglNode *node)
{
  GeglNode *operation;

  g_return_val_if_fail (GEGL_IS_NODE (node), NULL);

  operation = g_object_get_data (G_OBJECT (node),
                                 "gimp-gegl-node-underlying-operation");

  if (operation)
    return gimp_gegl_node_get_underlying_operation (operation);
  else
    return node;
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
