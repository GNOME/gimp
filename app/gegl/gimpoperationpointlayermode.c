/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationpointcomposer.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2008 Martin Nordholts <martinn@svn.gnome.org>
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

#include <gegl-plugin.h>

#include "gegl-types.h"

#include "gimpoperationpointlayermode.h"


enum
{
  PROP_0,
  PROP_BLEND_MODE
};


struct _GimpOperationPointLayerModeClass
{
  GeglOperationPointComposerClass  parent_class;
};

struct _GimpOperationPointLayerMode
{
  GeglOperationPointComposer  parent_instance;

  GimpLayerModeEffects        blend_mode;
};


static void     gimp_operation_point_layer_mode_set_property (GObject             *object,
                                                              guint                property_id,
                                                              const GValue        *value,
                                                              GParamSpec          *pspec);
static void     gimp_operation_point_layer_mode_get_property (GObject             *object,
                                                              guint                property_id,
                                                              GValue              *value,
                                                              GParamSpec          *pspec);

static void     gimp_operation_point_layer_mode_prepare      (GeglOperation       *operation);
static gboolean gimp_operation_point_layer_mode_process      (GeglOperation       *operation,
                                                              void                *in_buf,
                                                              void                *aux_buf,
                                                              void                *out_buf,
                                                              glong                samples,
                                                              const GeglRectangle *roi);


G_DEFINE_TYPE (GimpOperationPointLayerMode, gimp_operation_point_layer_mode,
               GEGL_TYPE_OPERATION_POINT_COMPOSER)


static void
gimp_operation_point_layer_mode_class_init (GimpOperationPointLayerModeClass *klass)
{
  GObjectClass                    *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass              *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointComposerClass *point_class     = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  object_class->set_property  = gimp_operation_point_layer_mode_set_property;
  object_class->get_property  = gimp_operation_point_layer_mode_get_property;

  operation_class->name        = "gimp:layer-mode";
  operation_class->description = "GIMP layer mode operation";
  operation_class->categories  = "compositors";

  operation_class->prepare    = gimp_operation_point_layer_mode_prepare;

  point_class->process        = gimp_operation_point_layer_mode_process;

  g_object_class_install_property (object_class, PROP_BLEND_MODE,
                                   g_param_spec_enum ("blend-mode", NULL, NULL,
                                                      GIMP_TYPE_LAYER_MODE_EFFECTS,
                                                      GIMP_NORMAL_MODE,
                                                      GIMP_PARAM_READWRITE));
}

static void
gimp_operation_point_layer_mode_init (GimpOperationPointLayerMode *self)
{
}

static void
gimp_operation_point_layer_mode_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GimpOperationPointLayerMode *self = GIMP_OPERATION_POINT_LAYER_MODE (object);

  switch (property_id)
    {
    case PROP_BLEND_MODE:
      self->blend_mode = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_point_layer_mode_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GimpOperationPointLayerMode *self = GIMP_OPERATION_POINT_LAYER_MODE (object);

  switch (property_id)
    {
    case PROP_BLEND_MODE:
      g_value_set_enum (value, self->blend_mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_point_layer_mode_prepare (GeglOperation *operation)
{
  Babl *format = babl_format ("RaGaBaA float");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
  gegl_operation_set_format (operation, "aux",    format);
}

static gboolean
gimp_operation_point_layer_mode_process (GeglOperation       *operation,
                                   void                *in_buf,
                                   void                *aux_buf,
                                   void                *out_buf,
                                   glong                samples,
                                   const GeglRectangle *roi)
{
  GimpOperationPointLayerMode *self = GIMP_OPERATION_POINT_LAYER_MODE (operation);

  gfloat *in    = in_buf;
  gfloat *layer = aux_buf;
  gfloat *out   = out_buf;

  while (samples--)
    {
      out[ALPHA] = in[ALPHA] + layer[ALPHA] - in[ALPHA] * layer[ALPHA];

      switch (self->blend_mode)
        {
        case GIMP_NORMAL_MODE:
          out[RED]   = layer[RED]   + in[RED]   * (1 - layer[ALPHA]);
          out[GREEN] = layer[GREEN] + in[GREEN] * (1 - layer[ALPHA]);
          out[BLUE]  = layer[BLUE]  + in[BLUE]  * (1 - layer[ALPHA]);
          break;

        case GIMP_DISSOLVE_MODE:
          g_warning ("Not a point filter and cannot be implemented here.");
          break;
          
        case GIMP_BEHIND_MODE:
        case GIMP_MULTIPLY_MODE:
        case GIMP_SCREEN_MODE:
        case GIMP_OVERLAY_MODE:
        case GIMP_DIFFERENCE_MODE:
          /* TODO */
          break;

        case GIMP_ADDITION_MODE:
          /* To be more mathematically correct we would have to either
           * adjust the formula for the resulting opacity or adapt the
           * other channels to the change in opacity. Compare to the
           * 'plus' compositing operation in SVG 1.2.
           *
           * Since this doesn't matter for completely opaque layers, and
           * since consistency in how the alpha channel of layers is
           * interpreted is more important than mathematically correct
           * results, we don't bother.
           */
          out[RED]   = in[RED]   + layer[RED];
          out[GREEN] = in[GREEN] + layer[GREEN];
          out[BLUE]  = in[BLUE]  + layer[BLUE];
          break;

        case GIMP_SUBTRACT_MODE:
        case GIMP_DARKEN_ONLY_MODE:
        case GIMP_LIGHTEN_ONLY_MODE:
        case GIMP_HUE_MODE:
        case GIMP_SATURATION_MODE:
        case GIMP_COLOR_MODE:
        case GIMP_VALUE_MODE:
        case GIMP_DIVIDE_MODE:
        case GIMP_DODGE_MODE:
        case GIMP_BURN_MODE:
        case GIMP_HARDLIGHT_MODE:
        case GIMP_SOFTLIGHT_MODE:
        case GIMP_GRAIN_EXTRACT_MODE:
        case GIMP_GRAIN_MERGE_MODE:
        case GIMP_COLOR_ERASE_MODE:
        case GIMP_ERASE_MODE:
        case GIMP_REPLACE_MODE:
        case GIMP_ANTI_ERASE_MODE:
          /* TODO */
          break;

        default:
          g_error ("Unknown layer mode");
          break;
        }

      in    += 4;
      layer += 4;
      out   += 4;
    }

  return TRUE;
}
