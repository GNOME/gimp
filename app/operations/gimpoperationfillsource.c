/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationfillsource.c
 * Copyright (C) 2019 Ell
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

#include <cairo.h>
#include <gegl-plugin.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "operations-types.h"

#include "core/gimpdrawable.h"
#include "core/gimpfilloptions.h"

#include "gimpoperationfillsource.h"


enum
{
  PROP_0,
  PROP_OPTIONS,
  PROP_DRAWABLE,
  PROP_PATTERN_OFFSET_X,
  PROP_PATTERN_OFFSET_Y,
};


static void            gimp_operation_fill_source_dispose          (GObject              *object);
static void            gimp_operation_fill_source_get_property     (GObject              *object,
                                                                    guint                 property_id,
                                                                    GValue               *value,
                                                                    GParamSpec           *pspec);
static void            gimp_operation_fill_source_set_property     (GObject              *object,
                                                                    guint                 property_id,
                                                                    const GValue         *value,
                                                                    GParamSpec           *pspec);

static GeglRectangle   gimp_operation_fill_source_get_bounding_box (GeglOperation        *operation);
static void            gimp_operation_fill_source_prepare          (GeglOperation        *operation);
static gboolean        gimp_operation_fill_source_process          (GeglOperation        *operation,
                                                                    GeglOperationContext *context,
                                                                    const gchar          *output_pad,
                                                                    const GeglRectangle  *result,
                                                                    gint                  level);


G_DEFINE_TYPE (GimpOperationFillSource, gimp_operation_fill_source,
               GEGL_TYPE_OPERATION_SOURCE)

#define parent_class gimp_operation_fill_source_parent_class


static void
gimp_operation_fill_source_class_init (GimpOperationFillSourceClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->dispose             = gimp_operation_fill_source_dispose;
  object_class->set_property        = gimp_operation_fill_source_set_property;
  object_class->get_property        = gimp_operation_fill_source_get_property;

  operation_class->get_bounding_box = gimp_operation_fill_source_get_bounding_box;
  operation_class->prepare          = gimp_operation_fill_source_prepare;
  operation_class->process          = gimp_operation_fill_source_process;

  operation_class->threaded         = FALSE;
  operation_class->cache_policy     = GEGL_CACHE_POLICY_NEVER;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:fill-source",
                                 "categories",  "gimp",
                                 "description", "GIMP Fill Source operation",
                                 NULL);

  g_object_class_install_property (object_class, PROP_OPTIONS,
                                   g_param_spec_object ("options",
                                                        "Options",
                                                        "Fill options",
                                                        GIMP_TYPE_FILL_OPTIONS,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_DRAWABLE,
                                   g_param_spec_object ("drawable",
                                                        "Drawable",
                                                        "Fill drawable",
                                                        GIMP_TYPE_DRAWABLE,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_PATTERN_OFFSET_X,
                                   g_param_spec_int  ("pattern-offset-x",
                                                      "Pattern X-offset",
                                                      "Pattern X-offset",
                                                      G_MININT, G_MAXINT, 0,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PATTERN_OFFSET_Y,
                                   g_param_spec_int  ("pattern-offset-y",
                                                      "Pattern Y-offset",
                                                      "Pattern Y-offset",
                                                      G_MININT, G_MAXINT, 0,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));
}

static void
gimp_operation_fill_source_init (GimpOperationFillSource *self)
{
}

static void
gimp_operation_fill_source_dispose (GObject *object)
{
  GimpOperationFillSource *fill_source = GIMP_OPERATION_FILL_SOURCE (object);

  g_clear_object (&fill_source->options);
  g_clear_object (&fill_source->drawable);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_operation_fill_source_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  GimpOperationFillSource *fill_source = GIMP_OPERATION_FILL_SOURCE (object);

  switch (property_id)
    {
    case PROP_OPTIONS:
      g_value_set_object (value, fill_source->options);
      break;

    case PROP_DRAWABLE:
      g_value_set_object (value, fill_source->drawable);
      break;

    case PROP_PATTERN_OFFSET_X:
      g_value_set_int (value, fill_source->pattern_offset_x);
      break;

    case PROP_PATTERN_OFFSET_Y:
      g_value_set_int (value, fill_source->pattern_offset_y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_fill_source_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  GimpOperationFillSource *fill_source = GIMP_OPERATION_FILL_SOURCE (object);

  switch (property_id)
    {
    case PROP_OPTIONS:
      g_set_object (&fill_source->options, g_value_get_object (value));
      break;

    case PROP_DRAWABLE:
      g_set_object (&fill_source->drawable, g_value_get_object (value));
      break;

    case PROP_PATTERN_OFFSET_X:
      fill_source->pattern_offset_x = g_value_get_int (value);
      break;

    case PROP_PATTERN_OFFSET_Y:
      fill_source->pattern_offset_y = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GeglRectangle
gimp_operation_fill_source_get_bounding_box (GeglOperation *operation)
{
  return gegl_rectangle_infinite_plane ();
}

static void
gimp_operation_fill_source_prepare (GeglOperation *operation)
{
  GimpOperationFillSource *fill_source = GIMP_OPERATION_FILL_SOURCE (operation);
  const Babl              *format      = NULL;

  if (fill_source->options && fill_source->drawable)
    {
      format = gimp_fill_options_get_format (fill_source->options,
                                             fill_source->drawable);
    }

  gegl_operation_set_format (operation, "output", format);
}

static gboolean
gimp_operation_fill_source_process (GeglOperation        *operation,
                                    GeglOperationContext *context,
                                    const gchar          *output_pad,
                                    const GeglRectangle  *result,
                                    gint                  level)
{
  GimpOperationFillSource *fill_source = GIMP_OPERATION_FILL_SOURCE (operation);

  if (fill_source->options && fill_source->drawable)
    {
      GeglBuffer    *buffer;
      GeglRectangle  rect;

      gegl_rectangle_align_to_buffer (
        &rect, result,
        gimp_drawable_get_buffer (fill_source->drawable),
        GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

      buffer = gimp_fill_options_create_buffer (fill_source->options,
                                                fill_source->drawable,
                                                &rect,
                                                fill_source->pattern_offset_x,
                                                fill_source->pattern_offset_y);

      gegl_operation_context_take_object (context, "output", G_OBJECT (buffer));
    }

  return TRUE;
}
