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

#include <gegl.h>

#include "gimp-gegl-types.h"

#include "gimp-gegl-nodes.h"
#include "gimpapplicator.h"


static void   gimp_applicator_finalize     (GObject      *object);
static void   gimp_applicator_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);
static void   gimp_applicator_get_property (GObject      *object,
                                            guint         property_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);


G_DEFINE_TYPE (GimpApplicator, gimp_applicator, G_TYPE_OBJECT)

#define parent_class gimp_applicator_parent_class


static void
gimp_applicator_class_init (GimpApplicatorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_applicator_finalize;
  object_class->set_property = gimp_applicator_set_property;
  object_class->get_property = gimp_applicator_get_property;
}

static void
gimp_applicator_init (GimpApplicator *core)
{
}

static void
gimp_applicator_finalize (GObject *object)
{
  GimpApplicator *applicator = GIMP_APPLICATOR (object);

  if (applicator->node)
    {
      g_object_unref (applicator->node);
      applicator->node = NULL;
    }

  if (applicator->processor)
    {
      g_object_unref (applicator->processor);
      applicator->processor = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_applicator_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_applicator_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GimpApplicator *
gimp_applicator_new (GeglBuffer        *dest_buffer,
                     GimpComponentMask  affect,
                     GeglBuffer        *mask_buffer,
                     gint               mask_offset_x,
                     gint               mask_offset_y)
{
  GimpApplicator *applicator;

  g_return_val_if_fail (GEGL_IS_BUFFER (dest_buffer), NULL);
  g_return_val_if_fail (mask_buffer == NULL || GEGL_IS_BUFFER (mask_buffer),
                        NULL);

  applicator = g_object_new (GIMP_TYPE_APPLICATOR, NULL);

  applicator->node = gegl_node_new ();

  applicator->mode_node = gegl_node_new_child (applicator->node,
                                               "operation", "gimp:normal-mode",
                                               NULL);

  applicator->src_node =
    gegl_node_new_child (applicator->node,
                         "operation", "gegl:buffer-source",
                         NULL);

  gegl_node_connect_to (applicator->src_node,  "output",
                        applicator->mode_node, "input");

  applicator->apply_src_node =
    gegl_node_new_child (applicator->node,
                         "operation", "gegl:buffer-source",
                         NULL);

  applicator->apply_offset_node =
    gegl_node_new_child (applicator->node,
                         "operation", "gegl:translate",
                         NULL);

  gegl_node_connect_to (applicator->apply_src_node,    "output",
                        applicator->apply_offset_node, "input");
  gegl_node_connect_to (applicator->apply_offset_node, "output",
                        applicator->mode_node,         "aux");

  if (mask_buffer)
    {
      GeglNode *mask_src;

      mask_src = gegl_node_new_child (applicator->node,
                                      "operation", "gegl:buffer-source",
                                      "buffer",    mask_buffer,
                                      NULL);

      if (mask_offset_x != 0 || mask_offset_y != 0)
        {
          GeglNode *offset;

          offset = gegl_node_new_child (applicator->node,
                                        "operation", "gegl:translate",
                                        "x",         (gdouble) mask_offset_x,
                                        "y",         (gdouble) mask_offset_y,
                                        NULL);

          gegl_node_connect_to (mask_src,        "output",
                                offset,          "input");
          gegl_node_connect_to (offset,          "output",
                                applicator->mode_node, "aux2");
        }
      else
        {
          gegl_node_connect_to (mask_src,        "output",
                                applicator->mode_node, "aux2");
        }
    }

  applicator->dest_node =
    gegl_node_new_child (applicator->node,
                         "operation", "gegl:write-buffer",
                         "buffer",    dest_buffer,
                         NULL);

  if (affect == GIMP_COMPONENT_ALL)
    {
      gegl_node_connect_to (applicator->mode_node, "output",
                            applicator->dest_node, "input");
    }
  else
    {
      GeglNode *affect_node;

      affect_node = gegl_node_new_child (applicator->node,
                                         "operation", "gimp:mask-components",
                                         "mask",      affect,
                                         NULL);

      gegl_node_connect_to (applicator->src_node,  "output",
                            affect_node,     "input");
      gegl_node_connect_to (applicator->mode_node, "output",
                            affect_node,     "aux");
      gegl_node_connect_to (affect_node,     "output",
                            applicator->dest_node, "input");
    }

  applicator->processor = gegl_node_new_processor (applicator->dest_node, NULL);

  return applicator;
}

void
gimp_applicator_apply (GimpApplicator           *applicator,
                       GeglBuffer               *src_buffer,
                       GeglBuffer               *apply_buffer,
                       gint                      apply_buffer_x,
                       gint                      apply_buffer_y,
                       gdouble                   opacity,
                       GimpLayerModeEffects      paint_mode)
{
  gint width;
  gint height;

  gegl_node_set (applicator->src_node,
                 "buffer", src_buffer,
                 NULL);

  gegl_node_set (applicator->apply_src_node,
                 "buffer", apply_buffer,
                 NULL);

  gegl_node_set (applicator->apply_offset_node,
                 "x", (gdouble) apply_buffer_x,
                 "y", (gdouble) apply_buffer_y,
                 NULL);

  gimp_gegl_mode_node_set (applicator->mode_node,
                           paint_mode, opacity, FALSE);

  width  = gegl_buffer_get_width  (apply_buffer);
  height = gegl_buffer_get_height (apply_buffer);

  gegl_processor_set_rectangle (applicator->processor,
                                GEGL_RECTANGLE (apply_buffer_x,
                                                apply_buffer_y,
                                                width, height));

  while (gegl_processor_work (applicator->processor, NULL));
}
