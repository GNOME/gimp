/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-gegl-nodes.h
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>

#include "gimp-gegl-types.h"

#include "gimp-gegl-nodes.h"
#include "gimp-gegl-utils.h"


GeglNode *
gimp_gegl_create_flatten_node (const GimpRGB *background)
{
  GeglNode  *node;
  GeglNode  *input;
  GeglNode  *output;
  GeglNode  *color;
  GeglNode  *over;
  GeglColor *c;

  g_return_val_if_fail (background != NULL, NULL);

  node = gegl_node_new ();

  input  = gegl_node_get_input_proxy  (node, "input");
  output = gegl_node_get_output_proxy (node, "output");

  c = gimp_gegl_color_new (background);
  color = gegl_node_new_child (node,
                               "operation", "gegl:color",
                               "value",     c,
                               NULL);
  g_object_unref (c);

  over = gegl_node_new_child (node,
                              "operation", "gegl:over",
                              NULL);

  gegl_node_connect_to (input,  "output",
                        over,   "aux");
  gegl_node_connect_to (color,  "output",
                        over,   "input");
  gegl_node_connect_to (over,   "output",
                        output, "input");

  return node;
}

GeglNode *
gimp_gegl_create_apply_opacity_node (GeglBuffer *mask,
                                     gint        mask_offset_x,
                                     gint        mask_offset_y,
                                     gdouble     opacity)
{
  GeglNode  *node;
  GeglNode  *input;
  GeglNode  *output;
  GeglNode  *opacity_node;
  GeglNode  *mask_source;

  g_return_val_if_fail (GEGL_IS_BUFFER (mask), NULL);

  node = gegl_node_new ();

  input  = gegl_node_get_input_proxy  (node, "input");
  output = gegl_node_get_output_proxy (node, "output");

  opacity_node = gegl_node_new_child (node,
                                      "operation", "gegl:opacity",
                                      "value",     opacity,
                                      NULL);

  mask_source = gimp_gegl_add_buffer_source (node, mask,
                                             mask_offset_x,
                                             mask_offset_y);

  gegl_node_connect_to (input,        "output",
                        opacity_node, "input");
  gegl_node_connect_to (mask_source,  "output",
                        opacity_node, "aux");
  gegl_node_connect_to (opacity_node, "output",
                        output,       "input");

  return node;
}

GeglNode *
gimp_gegl_add_buffer_source (GeglNode   *parent,
                             GeglBuffer *buffer,
                             gint        offset_x,
                             gint        offset_y)
{
  GeglNode *buffer_source;

  g_return_val_if_fail (GEGL_IS_NODE (parent), NULL);
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  buffer_source = gegl_node_new_child (parent,
                                       "operation", "gegl:buffer-source",
                                       "buffer",    buffer,
                                       NULL);

  if (offset_x != 0 || offset_y != 0)
    {
      GeglNode *translate =
        gegl_node_new_child (parent,
                             "operation", "gegl:translate",
                             "x",         (gdouble) offset_x,
                             "y",         (gdouble) offset_y,
                             NULL);

      gegl_node_connect_to (buffer_source, "output",
                            translate,     "input");

      buffer_source = translate;
    }

  return buffer_source;
}

gboolean
gimp_gegl_mode_is_linear (GimpLayerMode mode)
{
  switch (mode)
    {
    case GIMP_LAYER_MODE_NORMAL:
    case GIMP_LAYER_MODE_DISSOLVE:
    case GIMP_LAYER_MODE_MULTIPLY_LINEAR:
    case GIMP_LAYER_MODE_BEHIND:
      return TRUE;
    case GIMP_LAYER_MODE_BEHIND_NON_LINEAR:
    case GIMP_LAYER_MODE_MULTIPLY:
    case GIMP_LAYER_MODE_MULTIPLY_LEGACY:
    case GIMP_LAYER_MODE_SCREEN:
    case GIMP_LAYER_MODE_SCREEN_LEGACY:
    case GIMP_LAYER_MODE_ADDITION:
    case GIMP_LAYER_MODE_SUBTRACT:
    case GIMP_LAYER_MODE_ADDITION_LEGACY:
    case GIMP_LAYER_MODE_SUBTRACT_LEGACY:
    case GIMP_LAYER_MODE_DARKEN_ONLY:
    case GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY:
    case GIMP_LAYER_MODE_LIGHTEN_ONLY:
    case GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY:
    case GIMP_LAYER_MODE_NORMAL_NON_LINEAR:
    case GIMP_LAYER_MODE_OVERLAY_LEGACY:
    case GIMP_LAYER_MODE_DIFFERENCE_LEGACY:
    case GIMP_LAYER_MODE_DIFFERENCE:
    case GIMP_LAYER_MODE_HSV_HUE:
    case GIMP_LAYER_MODE_HSV_SATURATION:
    case GIMP_LAYER_MODE_HSV_COLOR:
    case GIMP_LAYER_MODE_HSV_VALUE:
    case GIMP_LAYER_MODE_HSV_HUE_LEGACY:
    case GIMP_LAYER_MODE_HSV_SATURATION_LEGACY:
    case GIMP_LAYER_MODE_HSV_COLOR_LEGACY:
    case GIMP_LAYER_MODE_HSV_VALUE_LEGACY:
    case GIMP_LAYER_MODE_DIVIDE:
    case GIMP_LAYER_MODE_DIVIDE_LEGACY:
    case GIMP_LAYER_MODE_DODGE:
    case GIMP_LAYER_MODE_DODGE_LEGACY:
    case GIMP_LAYER_MODE_BURN:
    case GIMP_LAYER_MODE_BURN_LEGACY:
    case GIMP_LAYER_MODE_HARDLIGHT:
    case GIMP_LAYER_MODE_HARDLIGHT_LEGACY:
    case GIMP_LAYER_MODE_SOFTLIGHT:
    case GIMP_LAYER_MODE_SOFTLIGHT_LEGACY:
    case GIMP_LAYER_MODE_GRAIN_EXTRACT:
    case GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY:
    case GIMP_LAYER_MODE_GRAIN_MERGE:
    case GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY:
    case GIMP_LAYER_MODE_COLOR_ERASE:
    case GIMP_LAYER_MODE_OVERLAY:
    case GIMP_LAYER_MODE_ERASE:
    case GIMP_LAYER_MODE_REPLACE:
    case GIMP_LAYER_MODE_ANTI_ERASE:
    case GIMP_LAYER_MODE_LCH_HUE:
    case GIMP_LAYER_MODE_LCH_CHROMA:
    case GIMP_LAYER_MODE_LCH_COLOR:
    case GIMP_LAYER_MODE_LCH_LIGHTNESS:
      return FALSE;
  }
  return FALSE;
}

void
gimp_gegl_mode_node_set_mode (GeglNode      *node,
                              GimpLayerMode  mode,
                              gboolean       linear)
{
  const gchar *operation = "gimp:normal";
  gdouble      opacity;

  g_return_if_fail (GEGL_IS_NODE (node));

  switch (mode)
    {
    case GIMP_LAYER_MODE_NORMAL:
      operation = "gimp:normal";
      break;

    case GIMP_LAYER_MODE_DISSOLVE:
      operation = "gimp:dissolve";
      break;

    case GIMP_LAYER_MODE_BEHIND:
      operation = "gimp:behind";
      break;

    case GIMP_LAYER_MODE_MULTIPLY_LEGACY:
      operation = "gimp:multiply-legacy";
      break;

    case GIMP_LAYER_MODE_MULTIPLY:
    case GIMP_LAYER_MODE_MULTIPLY_LINEAR:
      operation = "gimp:multiply";
      break;

    case GIMP_LAYER_MODE_SCREEN:
      operation = "gimp:screen";
      break;

    case GIMP_LAYER_MODE_SCREEN_LEGACY:
      operation = "gimp:screen-legacy";
      break;

    case GIMP_LAYER_MODE_OVERLAY_LEGACY:
      operation = "gimp:softlight-legacy";
      break;

    case GIMP_LAYER_MODE_DIFFERENCE:
      operation = "gimp:difference";
      break;

    case GIMP_LAYER_MODE_DIFFERENCE_LEGACY:
      operation = "gimp:difference-legacy";
      break;

    case GIMP_LAYER_MODE_ADDITION:
      operation = "gimp:addition";
      break;

    case GIMP_LAYER_MODE_ADDITION_LEGACY:
      operation = "gimp:addition-legacy";
      break;

    case GIMP_LAYER_MODE_SUBTRACT:
      operation = "gimp:subtract";
      break;

    case GIMP_LAYER_MODE_SUBTRACT_LEGACY:
      operation = "gimp:subtract-legacy";
      break;

    case GIMP_LAYER_MODE_DARKEN_ONLY:
      operation = "gimp:darken-only";
      break;

    case GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY:
      operation = "gimp:darken-only-legacy";
      break;

    case GIMP_LAYER_MODE_LIGHTEN_ONLY:
      operation = "gimp:lighten-only";
      break;

    case GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY:
      operation = "gimp:lighten-only-legacy";
      break;

    case GIMP_LAYER_MODE_HSV_HUE:
      operation = "gimp:hsv-hue";
      break;

    case GIMP_LAYER_MODE_HSV_SATURATION:
      operation = "gimp:hsv-saturation";
      break;

    case GIMP_LAYER_MODE_HSV_COLOR:
      operation = "gimp:hsv-color";
      break;

    case GIMP_LAYER_MODE_HSV_VALUE:
      operation = "gimp:hsv-value";
      break;

    case GIMP_LAYER_MODE_HSV_HUE_LEGACY:
      operation = "gimp:hsv-hue-legacy";
      break;

    case GIMP_LAYER_MODE_HSV_SATURATION_LEGACY:
      operation = "gimp:hsv-saturation-legacy";
      break;

    case GIMP_LAYER_MODE_HSV_COLOR_LEGACY:
      operation = "gimp:hsv-color-legacy";
      break;

    case GIMP_LAYER_MODE_HSV_VALUE_LEGACY:
      operation = "gimp:hsv-value-legacy";
      break;

    case GIMP_LAYER_MODE_DIVIDE:
      operation = "gimp:divide";
      break;

    case GIMP_LAYER_MODE_DIVIDE_LEGACY:
      operation = "gimp:divide-legacy";
      break;

    case GIMP_LAYER_MODE_DODGE:
      operation = "gimp:dodge";
      break;

    case GIMP_LAYER_MODE_DODGE_LEGACY:
      operation = "gimp:dodge-legacy";
      break;

    case GIMP_LAYER_MODE_BURN:
      operation = "gimp:burn";
      break;

    case GIMP_LAYER_MODE_BURN_LEGACY:
      operation = "gimp:burn-legacy";
      break;

    case GIMP_LAYER_MODE_HARDLIGHT:
      operation = "gimp:hardlight";
      break;

    case GIMP_LAYER_MODE_HARDLIGHT_LEGACY:
      operation = "gimp:hardlight-legacy";
      break;

    case GIMP_LAYER_MODE_SOFTLIGHT:
      operation = "gimp:softlight";
      break;

    case GIMP_LAYER_MODE_SOFTLIGHT_LEGACY:
      operation = "gimp:softlight-legacy";
      break;

    case GIMP_LAYER_MODE_GRAIN_EXTRACT:
      operation = "gimp:grain-extract";
      break;

    case GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY:
      operation = "gimp:grain-extract-legacy";
      break;

    case GIMP_LAYER_MODE_GRAIN_MERGE:
      operation = "gimp:grain-merge";
      break;

    case GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY:
      operation = "gimp:grain-merge-legacy";
      break;

    case GIMP_LAYER_MODE_COLOR_ERASE:
      operation = "gimp:color-erase";
      break;

    case GIMP_LAYER_MODE_OVERLAY:
      operation = "gimp:overlay";
      break;

    case GIMP_LAYER_MODE_LCH_HUE:
      operation = "gimp:lch-hue";
      break;

    case GIMP_LAYER_MODE_LCH_CHROMA:
      operation = "gimp:lch-chroma";
      break;

    case GIMP_LAYER_MODE_LCH_COLOR:
      operation = "gimp:lch-color";
      break;

    case GIMP_LAYER_MODE_LCH_LIGHTNESS:
      operation = "gimp:lch-lightness";
      break;

    case GIMP_LAYER_MODE_ERASE:
      operation = "gimp:erase";
      break;

    case GIMP_LAYER_MODE_REPLACE:
      operation = "gimp:replace";
      break;

    case GIMP_LAYER_MODE_ANTI_ERASE:
      operation = "gimp:anti-erase";
      break;

    default:
      break;
    }

  gegl_node_get (node,
                 "opacity", &opacity,
                 NULL);

  /* setting the operation creates a new instance, so we have to set
   * all its properties
   */
  gegl_node_set (node,
                 "operation", operation,
                 "opacity",   opacity,
                 NULL);

  gegl_node_set (node, "linear", gimp_gegl_mode_is_linear (mode)?TRUE:FALSE,
                 NULL);
}

void
gimp_gegl_mode_node_set_opacity (GeglNode *node,
                                 gdouble   opacity)
{
  g_return_if_fail (GEGL_IS_NODE (node));

  gegl_node_set (node,
                 "opacity", opacity,
                 NULL);
}

void
gimp_gegl_node_set_matrix (GeglNode          *node,
                           const GimpMatrix3 *matrix)
{
  gchar *matrix_string;

  g_return_if_fail (GEGL_IS_NODE (node));
  g_return_if_fail (matrix != NULL);

  matrix_string = gegl_matrix3_to_string ((GeglMatrix3 *) matrix);

  gegl_node_set (node,
                 "transform", matrix_string,
                 NULL);

  g_free (matrix_string);
}

void
gimp_gegl_node_set_color (GeglNode      *node,
                          const GimpRGB *color)
{
  GeglColor *gegl_color;

  g_return_if_fail (GEGL_IS_NODE (node));
  g_return_if_fail (color != NULL);

  gegl_color = gimp_gegl_color_new (color);

  gegl_node_set (node,
                 "value", gegl_color,
                 NULL);

  g_object_unref (gegl_color);
}
