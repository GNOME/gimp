/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpapplicator.c
 * Copyright (C) 2012-2013 Michael Natterer <mitch@gimp.org>
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
gimp_applicator_init (GimpApplicator *applicator)
{
  applicator->opacity    = 1.0;
  applicator->paint_mode = GIMP_NORMAL_MODE;
  applicator->affect     = GIMP_COMPONENT_ALL;
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
gimp_applicator_new (GeglNode *parent,
                     gboolean  linear)
{
  GimpApplicator *applicator;

  g_return_val_if_fail (parent == NULL || GEGL_IS_NODE (parent), NULL);

  applicator = g_object_new (GIMP_TYPE_APPLICATOR, NULL);

  applicator->linear = linear;

  if (parent)
    applicator->node = g_object_ref (parent);
  else
    applicator->node = gegl_node_new ();

  applicator->input_node =
    gegl_node_get_input_proxy  (applicator->node, "input");

  applicator->aux_node =
    gegl_node_get_input_proxy  (applicator->node, "aux");

  applicator->output_node =
    gegl_node_get_output_proxy (applicator->node, "output");

  applicator->mode_node = gegl_node_new_child (applicator->node,
                                               "operation", "gimp:normal-mode",
                                               NULL);

  gimp_gegl_mode_node_set_mode (applicator->mode_node,
                                applicator->paint_mode,
                                applicator->linear);
  gimp_gegl_mode_node_set_opacity (applicator->mode_node,
                                   applicator->opacity);

  gegl_node_connect_to (applicator->input_node, "output",
                        applicator->mode_node,  "input");

  applicator->apply_offset_node =
    gegl_node_new_child (applicator->node,
                         "operation", "gegl:translate",
                         NULL);

  gegl_node_connect_to (applicator->aux_node,          "output",
                        applicator->apply_offset_node, "input");
  gegl_node_connect_to (applicator->apply_offset_node, "output",
                        applicator->mode_node,         "aux");

  applicator->mask_node =
    gegl_node_new_child (applicator->node,
                         "operation", "gegl:buffer-source",
                         NULL);

  applicator->mask_offset_node =
    gegl_node_new_child (applicator->node,
                         "operation", "gegl:translate",
                         NULL);

  gegl_node_connect_to (applicator->mask_node,        "output",
                        applicator->mask_offset_node, "input");
  /* don't connect the the mask offset node to mode's aux2 yet */

  applicator->affect_node =
    gegl_node_new_child (applicator->node,
                         "operation", "gimp:mask-components",
                         "mask",      applicator->affect,
                         NULL);

  gegl_node_connect_to (applicator->input_node,  "output",
                        applicator->affect_node, "input");
  gegl_node_connect_to (applicator->mode_node,   "output",
                        applicator->affect_node, "aux");
  gegl_node_connect_to (applicator->affect_node, "output",
                        applicator->output_node, "input");

  return applicator;
}

void
gimp_applicator_set_src_buffer (GimpApplicator *applicator,
                                GeglBuffer     *src_buffer)
{
  g_return_if_fail (src_buffer == NULL || GEGL_IS_BUFFER (src_buffer));

  if (src_buffer == applicator->src_buffer)
    return;

  if (src_buffer)
    {
      if (! applicator->src_node)
        {
          applicator->src_node =
            gegl_node_new_child (applicator->node,
                                 "operation", "gegl:buffer-source",
                                 "buffer",    src_buffer,
                                 NULL);
        }
      else
        {
          gegl_node_set (applicator->src_node,
                         "buffer", src_buffer,
                         NULL);
        }

      if (! applicator->src_buffer)
        {
          gegl_node_connect_to (applicator->src_node,    "output",
                                applicator->mode_node,   "input");
          gegl_node_connect_to (applicator->src_node,    "output",
                                applicator->affect_node, "input");
        }
    }
  else if (applicator->src_buffer)
    {
      gegl_node_connect_to (applicator->input_node,  "output",
                            applicator->mode_node,   "input");
      gegl_node_connect_to (applicator->input_node,  "output",
                            applicator->affect_node, "input");
    }

  applicator->src_buffer = src_buffer;
}

void
gimp_applicator_set_dest_buffer (GimpApplicator *applicator,
                                 GeglBuffer     *dest_buffer)
{
  g_return_if_fail (dest_buffer == NULL || GEGL_IS_BUFFER (dest_buffer));

  if (dest_buffer == applicator->dest_buffer)
    return;

  if (dest_buffer)
    {
      if (! applicator->dest_node)
        {
          applicator->dest_node =
            gegl_node_new_child (applicator->node,
                                 "operation", "gegl:write-buffer",
                                 "buffer",    dest_buffer,
                                 NULL);
        }
      else
        {
          gegl_node_set (applicator->dest_node,
                         "buffer", dest_buffer,
                         NULL);
        }

      if (! applicator->dest_buffer)
        {
          gegl_node_disconnect (applicator->output_node, "input");

          gegl_node_connect_to (applicator->affect_node, "output",
                                applicator->dest_node,   "input");
        }
    }
  else if (applicator->dest_buffer)
    {
      gegl_node_disconnect (applicator->dest_node, "input");

      gegl_node_connect_to (applicator->affect_node, "output",
                            applicator->output_node, "input");
    }

  applicator->dest_buffer = dest_buffer;
}

void
gimp_applicator_set_mask_buffer (GimpApplicator *applicator,
                                 GeglBuffer     *mask_buffer)
{
  if (applicator->mask_buffer == mask_buffer)
    return;

  gegl_node_set (applicator->mask_node,
                 "buffer", mask_buffer,
                 NULL);

  if (mask_buffer)
    {
      gegl_node_connect_to (applicator->mask_offset_node, "output",
                            applicator->mode_node,        "aux2");
    }
  else
    {
      gegl_node_disconnect (applicator->mode_node, "aux2");
    }

  applicator->mask_buffer = mask_buffer;
}

void
gimp_applicator_set_mask_offset (GimpApplicator *applicator,
                                 gint            mask_offset_x,
                                 gint            mask_offset_y)
{
  if (applicator->mask_offset_x != mask_offset_x ||
      applicator->mask_offset_y != mask_offset_y)
    {
      applicator->mask_offset_x = mask_offset_x;
      applicator->mask_offset_y = mask_offset_y;

      gegl_node_set (applicator->mask_offset_node,
                     "x", (gdouble) mask_offset_x,
                     "y", (gdouble) mask_offset_y,
                     NULL);
    }
}

void
gimp_applicator_set_apply_buffer (GimpApplicator *applicator,
                                  GeglBuffer     *apply_buffer)
{
  g_return_if_fail (apply_buffer == NULL || GEGL_IS_BUFFER (apply_buffer));

  if (apply_buffer == applicator->apply_buffer)
    return;

  if (apply_buffer)
    {
      if (! applicator->apply_src_node)
        {
          applicator->apply_src_node =
            gegl_node_new_child (applicator->node,
                                 "operation", "gegl:buffer-source",
                                 "buffer",    apply_buffer,
                                 NULL);
        }
      else
        {
          gegl_node_set (applicator->apply_src_node,
                         "buffer", apply_buffer,
                         NULL);
        }

      if (! applicator->apply_buffer)
        {
          gegl_node_connect_to (applicator->apply_src_node,    "output",
                                applicator->apply_offset_node, "input");
        }
    }
  else if (applicator->apply_buffer)
    {
      gegl_node_connect_to (applicator->aux_node,          "output",
                            applicator->apply_offset_node, "input");
    }

  applicator->apply_buffer = apply_buffer;
}

void
gimp_applicator_set_apply_offset (GimpApplicator *applicator,
                                  gint            apply_offset_x,
                                  gint            apply_offset_y)
{
  if (applicator->apply_offset_x != apply_offset_x ||
      applicator->apply_offset_y != apply_offset_y)
    {
      applicator->apply_offset_x = apply_offset_x;
      applicator->apply_offset_y = apply_offset_y;

      gegl_node_set (applicator->apply_offset_node,
                     "x", (gdouble) apply_offset_x,
                     "y", (gdouble) apply_offset_y,
                     NULL);
    }
}

void
gimp_applicator_set_mode (GimpApplicator       *applicator,
                          gdouble               opacity,
                          GimpLayerModeEffects  paint_mode)
{
  if (applicator->opacity != opacity)
    {
      applicator->opacity = opacity;

      gimp_gegl_mode_node_set_opacity (applicator->mode_node,
                                       opacity);
    }

  if (applicator->paint_mode != paint_mode)
    {
      applicator->paint_mode = paint_mode;

      gimp_gegl_mode_node_set_mode (applicator->mode_node,
                                    paint_mode, applicator->linear);
    }
}

void
gimp_applicator_set_affect (GimpApplicator    *applicator,
                            GimpComponentMask  affect)
{
  if (applicator->affect != affect)
    {
      applicator->affect = affect;

      gegl_node_set (applicator->affect_node,
                     "mask", affect,
                     NULL);
    }
}

void
gimp_applicator_blit (GimpApplicator      *applicator,
                      const GeglRectangle *rect)
{
  gegl_node_blit (applicator->dest_node, 1.0, rect,
                  NULL, NULL, 0, GEGL_BLIT_DEFAULT);
}

GeglBuffer *
gimp_applicator_dup_apply_buffer (GimpApplicator      *applicator,
                                  const GeglRectangle *rect)
{
  GeglBuffer *buffer;
  GeglNode   *offset;
  GeglNode   *dest;

  buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, rect->width, rect->height),
                            babl_format ("RGBA float"));

  offset = gegl_node_new_child (applicator->node,
                                "operation", "gegl:translate",
                                "x",
                                (gdouble) - applicator->apply_offset_x - rect->x,
                                "y",
                                (gdouble) - applicator->apply_offset_y - rect->y,
                                NULL);

  dest = gegl_node_new_child (applicator->node,
                              "operation", "gegl:write-buffer",
                              "buffer",    buffer,
                              NULL);

  gegl_node_link_many (applicator->apply_offset_node,
                       offset,
                       dest,
                       NULL);

  gegl_node_blit (dest, 1.0, GEGL_RECTANGLE (0, 0, rect->width, rect->height),
                  NULL, NULL, 0, GEGL_BLIT_DEFAULT);

  gegl_node_disconnect (offset, "input");

  gegl_node_remove_child (applicator->node, offset);
  gegl_node_remove_child (applicator->node, dest);

  return buffer;
}
