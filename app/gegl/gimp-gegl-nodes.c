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
                                     gdouble     opacity,
                                     /* offsets *into* the mask */
                                     gint        mask_offset_x,
                                     gint        mask_offset_y)
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
                                             -mask_offset_x,
                                             -mask_offset_y);

  gegl_node_connect_to (input,        "output",
                        opacity_node, "input");
  gegl_node_connect_to (mask_source,  "output",
                        opacity_node, "aux");
  gegl_node_connect_to (opacity_node, "output",
                        output,       "input");

  return node;
}

GeglNode *
gimp_gegl_create_apply_buffer_node (GeglBuffer           *buffer,
                                    gint                  buffer_offset_x,
                                    gint                  buffer_offset_y,
                                    gint                  src_offset_x,
                                    gint                  src_offset_y,
                                    gint                  dest_offset_x,
                                    gint                  dest_offset_y,
                                    GeglBuffer           *mask,
                                    gint                  mask_offset_x,
                                    gint                  mask_offset_y,
                                    gdouble               opacity,
                                    GimpLayerModeEffects  mode)

{
  GeglNode *node;
  GeglNode *input;
  GeglNode *output;
  GeglNode *buffer_source;
  GeglNode *mask_source;
  GeglNode *opacity_node;
  GeglNode *mode_node;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (mask == NULL || GEGL_IS_BUFFER (mask), NULL);

  node = gegl_node_new ();

  buffer_source = gimp_gegl_add_buffer_source (node, buffer,
                                               buffer_offset_x,
                                               buffer_offset_y);

  input = gegl_node_get_input_proxy (node, "input");

  if (src_offset_x != 0 || src_offset_y != 0)
    {
      GeglNode *translate =
        gegl_node_new_child (node,
                             "operation", "gegl:translate",
                             "x",         (gdouble) src_offset_x,
                             "y",         (gdouble) src_offset_y,
                             NULL);

      gegl_node_connect_to (input,     "output",
                            translate, "input");

      input = translate;
    }

  output = gegl_node_get_output_proxy (node, "output");

  if (dest_offset_x != 0 || dest_offset_y != 0)
    {
      GeglNode *translate =
        gegl_node_new_child (node,
                             "operation", "gegl:translate",
                             "x",         (gdouble) dest_offset_x,
                             "y",         (gdouble) dest_offset_y,
                             NULL);

      gegl_node_connect_to (translate, "output",
                            output,    "input");

      output = translate;
    }

  if (mask)
    mask_source = gimp_gegl_add_buffer_source (node, mask,
                                               mask_offset_x,
                                               mask_offset_y);
  else
    mask_source = NULL;

  opacity_node = gegl_node_new_child (node,
                                      "operation", "gegl:opacity",
                                      "value",     opacity,
                                      NULL);

  gegl_node_connect_to (buffer_source, "output",
                        opacity_node,  "input");

  if (mask_source)
    gegl_node_connect_to (mask_source,   "output",
                          opacity_node,  "aux");

  mode_node = gegl_node_new_child (node,
                                   "operation",
                                   gimp_layer_mode_to_gegl_operation (mode),
                                   NULL);

  gegl_node_connect_to (opacity_node, "output",
                        mode_node,    "aux");
  gegl_node_connect_to (input,        "output",
                        mode_node,    "input");
  gegl_node_connect_to (mode_node,    "output",
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
