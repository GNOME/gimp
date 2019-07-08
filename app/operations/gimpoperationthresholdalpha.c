/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationthresholdalpha.c
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
 * Ported from the threshold-alpha plug-in
 * Copyright (C) 1997 Shuji Narazaki <narazaki@InetQ.or.jp>
 */

#include "config.h"

#include <gegl.h>

#include "operations-types.h"

#include "gimpoperationthresholdalpha.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_VALUE
};


static void       gimp_operation_threshold_alpha_get_property (GObject             *object,
                                                               guint                property_id,
                                                               GValue              *value,
                                                               GParamSpec          *pspec);
static void       gimp_operation_threshold_alpha_set_property (GObject             *object,
                                                               guint                property_id,
                                                               const GValue        *value,
                                                               GParamSpec          *pspec);

static void       gimp_operation_threshold_alpha_prepare      (GeglOperation       *operation);
static gboolean   gimp_operation_threshold_alpha_process      (GeglOperation       *operation,
                                                               void                *in_buf,
                                                               void                *out_buf,
                                                               glong                samples,
                                                               const GeglRectangle *roi,
                                                               gint                 level);


G_DEFINE_TYPE (GimpOperationThresholdAlpha, gimp_operation_threshold_alpha,
               GEGL_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_threshold_alpha_parent_class


static void
gimp_operation_threshold_alpha_class_init (GimpOperationThresholdAlphaClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property = gimp_operation_threshold_alpha_set_property;
  object_class->get_property = gimp_operation_threshold_alpha_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:threshold-alpha",
                                 "categories",  "color",
                                 "description",
                                 _("Make transparency all-or-nothing, by "
                                   "thresholding the alpha channel to a value"),
                                 NULL);

  operation_class->prepare = gimp_operation_threshold_alpha_prepare;

  point_class->process     = gimp_operation_threshold_alpha_process;

  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_double ("value",
                                                        _("Value"),
                                                        _("The alpha value"),
                                                        0.0, 1.0, 0.5,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gimp_operation_threshold_alpha_init (GimpOperationThresholdAlpha *self)
{
}

static void
gimp_operation_threshold_alpha_get_property (GObject    *object,
                                             guint       property_id,
                                             GValue     *value,
                                             GParamSpec *pspec)
{
  GimpOperationThresholdAlpha *self = GIMP_OPERATION_THRESHOLD_ALPHA (object);

  switch (property_id)
    {
    case PROP_VALUE:
      g_value_set_double (value, self->value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_threshold_alpha_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec)
{
  GimpOperationThresholdAlpha *self = GIMP_OPERATION_THRESHOLD_ALPHA (object);

  switch (property_id)
    {
    case PROP_VALUE:
      self->value = g_value_get_double (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_threshold_alpha_prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  gegl_operation_set_format (operation, "input",  babl_format_with_space ("RGBA float", space));
  gegl_operation_set_format (operation, "output", babl_format_with_space ("RGBA float", space));
}

static gboolean
gimp_operation_threshold_alpha_process (GeglOperation       *operation,
                                        void                *in_buf,
                                        void                *out_buf,
                                        glong                samples,
                                        const GeglRectangle *roi,
                                        gint                 level)
{
  GimpOperationThresholdAlpha *self = GIMP_OPERATION_THRESHOLD_ALPHA (operation);
  gfloat                      *src  = in_buf;
  gfloat                      *dest = out_buf;

  while (samples--)
    {
      dest[RED]   = src[RED];
      dest[GREEN] = src[GREEN];
      dest[BLUE]  = src[BLUE];

      if (src[ALPHA] > self->value)
        dest[ALPHA] = 1.0;
      else
        dest[ALPHA] = 0.0;

      src  += 4;
      dest += 4;
    }

  return TRUE;
}
