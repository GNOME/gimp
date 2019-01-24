/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationsemiflatten.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Ported from the semi-flatten plug-in
 * by Adam D. Moss, adam@foxbox.org.  1998/01/27
 */

#include "config.h"

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"

#include "operations-types.h"

#include "gimpoperationsemiflatten.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_COLOR
};


static void       gimp_operation_semi_flatten_get_property (GObject             *object,
                                                            guint                property_id,
                                                            GValue              *value,
                                                            GParamSpec          *pspec);
static void       gimp_operation_semi_flatten_set_property (GObject             *object,
                                                            guint                property_id,
                                                            const GValue        *value,
                                                            GParamSpec          *pspec);

static void       gimp_operation_semi_flatten_prepare      (GeglOperation       *operation);
static gboolean   gimp_operation_semi_flatten_process      (GeglOperation       *operation,
                                                            void                *in_buf,
                                                            void                *out_buf,
                                                            glong                samples,
                                                            const GeglRectangle *roi,
                                                            gint                 level);


G_DEFINE_TYPE (GimpOperationSemiFlatten, gimp_operation_semi_flatten,
               GEGL_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_semi_flatten_parent_class


static void
gimp_operation_semi_flatten_class_init (GimpOperationSemiFlattenClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);
  GimpRGB                        white;

  object_class->set_property = gimp_operation_semi_flatten_set_property;
  object_class->get_property = gimp_operation_semi_flatten_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:semi-flatten",
                                 "categories",  "color",
                                 "description", _("Replace partial transparency with a color"),
                                 NULL);

  operation_class->prepare = gimp_operation_semi_flatten_prepare;

  point_class->process     = gimp_operation_semi_flatten_process;

  gimp_rgba_set (&white, 1.0, 1.0, 1.0, 1.0);

  g_object_class_install_property (object_class, PROP_COLOR,
                                   gimp_param_spec_rgb ("color",
                                                        _("Color"),
                                                        _("The color"),
                                                        FALSE, &white,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_operation_semi_flatten_init (GimpOperationSemiFlatten *self)
{
}

static void
gimp_operation_semi_flatten_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  GimpOperationSemiFlatten *self = GIMP_OPERATION_SEMI_FLATTEN (object);

  switch (property_id)
    {
    case PROP_COLOR:
      gimp_value_set_rgb (value, &self->color);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_semi_flatten_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  GimpOperationSemiFlatten *self = GIMP_OPERATION_SEMI_FLATTEN (object);

  switch (property_id)
    {
    case PROP_COLOR:
      gimp_value_get_rgb (value, &self->color);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_semi_flatten_prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  gegl_operation_set_format (operation, "input",  babl_format_with_space ("RGBA float", space));
  gegl_operation_set_format (operation, "output", babl_format_with_space ("RGBA float", space));
}

static gboolean
gimp_operation_semi_flatten_process (GeglOperation       *operation,
                                     void                *in_buf,
                                     void                *out_buf,
                                     glong                samples,
                                     const GeglRectangle *roi,
                                     gint                 level)
{
  GimpOperationSemiFlatten *self = GIMP_OPERATION_SEMI_FLATTEN (operation);
  gfloat                   *src  = in_buf;
  gfloat                   *dest = out_buf;

  while (samples--)
    {
      gfloat alpha = src[ALPHA];

      if (alpha <= 0.0 || alpha >= 1.0)
        {
          dest[RED]   = src[RED];
          dest[GREEN] = src[GREEN];
          dest[BLUE]  = src[BLUE];
          dest[ALPHA] = alpha;
        }
      else
        {
          dest[RED]   = src[RED]   * alpha + self->color.r * (1.0 - alpha);
          dest[GREEN] = src[GREEN] * alpha + self->color.g * (1.0 - alpha);
          dest[BLUE]  = src[BLUE]  * alpha + self->color.b * (1.0 - alpha);
          dest[ALPHA] = 1.0;
        }

      src  += 4;
      dest += 4;
    }

  return TRUE;
}
