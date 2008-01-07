/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationlevels.c
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "gegl/gegl-types.h"
#include <gegl/buffer/gegl-buffer.h>

#include "gegl-types.h"

#include "gimpoperationlevels.h"


enum
{
  PROP_0,
  PROP_HUE,
  PROP_SATURATION,
  PROP_LIGHTNESS
};


static void     gimp_operation_levels_get_property (GObject       *object,
                                                    guint          property_id,
                                                    GValue        *value,
                                                    GParamSpec    *pspec);
static void     gimp_operation_levels_set_property (GObject       *object,
                                                    guint          property_id,
                                                    const GValue  *value,
                                                    GParamSpec    *pspec);

static gboolean gimp_operation_levels_process      (GeglOperation *operation,
                                                    void          *in_buf,
                                                    void          *out_buf,
                                                    glong          samples);


G_DEFINE_TYPE (GimpOperationLevels, gimp_operation_levels,
               GEGL_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_levels_parent_class


static void
gimp_operation_levels_class_init (GimpOperationLevelsClass * klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property = gimp_operation_levels_set_property;
  object_class->get_property = gimp_operation_levels_get_property;

  point_class->process       = gimp_operation_levels_process;

  gegl_operation_class_set_name (operation_class, "gimp-levels");
}

static void
gimp_operation_levels_init (GimpOperationLevels *self)
{
}

static void
gimp_operation_levels_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  /* GimpOperationLevels *self = GIMP_OPERATION_LEVELS (object); */

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_levels_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  /* GimpOperationLevels *self = GIMP_OPERATION_LEVELS (object); */

  switch (property_id)
    {
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static inline gfloat
gimp_operation_levels_map (gfloat value,
                           gfloat low_input,
                           gfloat high_input,
                           gfloat gamma,
                           gfloat low_output,
                           gfloat high_output)
{
  /*  determine input intensity  */
  if (high_input != low_input)
    {
      value = (value - low_input) / (high_input - low_input);
    }
  else
    {
      value = (value - low_input);
    }

  if (gamma != 0.0)
    {
      if (value >= 0.0)
        value =  pow ( value, 1.0 / gamma);
      else
        value = -pow (-value, 1.0 / gamma);
    }

  /*  determine the output intensity  */
  if (high_output >= low_output)
    value = value * (high_output - low_output) + low_output;
  else if (high_output < low_output)
    value = low_output - value * (low_output - high_output);

  return value;
}

static gboolean
gimp_operation_levels_process (GeglOperation *operation,
                               void          *in_buf,
                               void          *out_buf,
                               glong          samples)
{
  GimpOperationLevels *self = GIMP_OPERATION_LEVELS (operation);
  gfloat              *src  = in_buf;
  gfloat              *dest = out_buf;
  glong                sample;

  for (sample = 0; sample < samples; sample++)
    {
      gint channel;

      for (channel = 0; channel < 4; channel++)
        {
          gfloat value;

          value = gimp_operation_levels_map (src[channel],
                                             self->low_input[channel + 1],
                                             self->high_input[channel + 1],
                                             self->gamma[channel + 1],
                                             self->low_output[channel + 1],
                                             self->high_output[channel + 1]);

          /* don't apply the overall curve to the alpha channel */
          if (channel != 3)
            value = gimp_operation_levels_map (value,
                                               self->low_input[0],
                                               self->high_input[0],
                                               self->gamma[0],
                                               self->low_output[0],
                                               self->high_output[0]);

          dest[channel] = value;
        }

      src  += 4;
      dest += 4;
    }

  return TRUE;
}
