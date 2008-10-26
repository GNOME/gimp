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


#define R RED
#define G GREEN
#define B BLUE
#define A ALPHA

#define EACH_CHANNEL(expr)      \
  for (c = RED; c < ALPHA; c++) \
    {                           \
      expr;                     \
    }


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

  object_class->set_property   = gimp_operation_point_layer_mode_set_property;
  object_class->get_property   = gimp_operation_point_layer_mode_get_property;

  operation_class->name        = "gimp:point-layer-mode";
  operation_class->description = "GIMP point layer mode operation";
  operation_class->categories  = "compositors";

  operation_class->prepare     = gimp_operation_point_layer_mode_prepare;

  point_class->process         = gimp_operation_point_layer_mode_process;

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

  gfloat *in  = in_buf;  /* composite of layers below */
  gfloat *lay = aux_buf; /* layer */
  gfloat *out = out_buf; /* resulting composite */
  gint    c   = 0;

  while (samples--)
    {
      switch (self->blend_mode)
        {
        case GIMP_NORMAL_MODE:
          /* Porter-Duff A over B */
          EACH_CHANNEL (out[c] = lay[c] + in[c] * (1 - lay[A]));
          break;

        case GIMP_BEHIND_MODE:
          /* Porter-Duff B over A */
          EACH_CHANNEL (out[c] = in[c] + lay[c] * (1 - in[A]));
          break;


        case GIMP_MULTIPLY_MODE:
          /* SVG 1.2 multiply */
          EACH_CHANNEL (out[c] = lay[c] * in[c] + lay[c] * (1 - in[A]) + in[c] * (1 - lay[A]));
          break;

        case GIMP_SCREEN_MODE:
          /* SVG 1.2 screen */
          EACH_CHANNEL (out[c] = lay[c] + in[c] - lay[c] * in[c]);
          break;

        case GIMP_OVERLAY_MODE:
          /* SVG 1.2 overlay */
          EACH_CHANNEL (if (2 * in[c] < in[A])
                          out[c] = 2 * lay[c] * in[c] + lay[c] * (1 - in[A]) + in[c] * (1 - lay[A]);
                        else
                          out[c] = lay[A] * in[A] - 2 * (in[A] - in[c]) * (lay[A] - lay[c]) + lay[c] * (1 - in[A]) + in[c] * (1 - lay[A]));
          break;

        case GIMP_DIFFERENCE_MODE:
          /* SVG 1.2 difference */
          EACH_CHANNEL (out[c] = in[c] + lay[c] - 2 * MIN (lay[c] * in[A], in[c] * lay[A]));
          break;

        case GIMP_DARKEN_ONLY_MODE:
          /* SVG 1.2 darken */
          EACH_CHANNEL (out[c] = MIN (lay[c] * in[A], in[c] * lay[A]) + lay[c] * (1 - in[A]) + in[c] * (1 - lay[A]));
          break;

        case GIMP_LIGHTEN_ONLY_MODE:
          /* SVG 1.2 lighten */
          EACH_CHANNEL (out[c] = MAX (lay[c] * in[A], in[c] * lay[A]) + lay[c] * (1 - in[A]) + in[c] * (1 - lay[A]));
          break;

        case GIMP_DODGE_MODE:
          /* SVG 1.2 color-dodge */
          EACH_CHANNEL (if (lay[c] * in[A] + in[c] * lay[A] >= lay[A] * in[A])
                          out[c] = lay[A] * in[A] + lay[c] * (1 - in[A]) + in[c] * (1 - lay[A]);
                        else
                          out[c] = in[c] * lay[A] / (1 - lay[c] / lay[A]) + lay[c] * (1 - in[A]) + in[c] * (1 - lay[A]));
          break;

        case GIMP_BURN_MODE:
          /* SVG 1.2 color-burn */
          EACH_CHANNEL (if (lay[c] * in[A] + in[c] * lay[A] <= lay[A] * in[A])
                          out[c] = lay[c] * (1 - in[A]) + in[c] * (1 - lay[A]);
                        else
                          out[c] = lay[A] * (lay[c] * in[A] + in[c] * lay[A] - lay[A] * in[A])/lay[c] + lay[c] * (1 - in[A]) + in[c] * (1 - lay[A]));
          break;

        case GIMP_HARDLIGHT_MODE:
          /* SVG 1.2 hard-light */
          EACH_CHANNEL (if (2 * lay[c] < lay[A])
                          out[c] = 2 * lay[c] * in[c] + lay[c] * (1 - in[A]) + in[c] * (1 - lay[A]);
                        else
                          out[c] = lay[A] * in[A] - 2 * (in[A] - in[c]) * (lay[A] - lay[c]) + lay[c] * (1 - in[A]) + in[c] * (1 - lay[A]));
          break;

        case GIMP_SOFTLIGHT_MODE:
          /* SVG 1.2 soft-light */
          /* XXX: Why is the result so different from legacy Soft Light? */
          EACH_CHANNEL (if (2 * lay[c] < lay[A])
                          out[c] = in[c] * (lay[A] - (1 - in[c]/in[A]) * (2 * lay[c] - lay[A])) + lay[c] * (1 - in[A]) + in[c] * (1 - lay[A]);
                        else if (8 * in[c] <= in[A])
                          out[c] = in[c] * (lay[A] - (1 - in[c]/in[A]) * (2 * lay[c] - lay[A]) * (3 - 8 * in[c]/in[A])) + lay[c] * (1 - in[A]) + in[c] * (1 - lay[A]);
                        else
                          out[c] = (in[c] * lay[A] + (sqrt (in[c] / in[A]) * in[A] - in[c]) * (2 * lay[c] - lay[A])) + lay[c] * (1 - in[A]) + in[c] * (1 - lay[A]));
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
          EACH_CHANNEL (out[c] = in[c] + lay[c]);
          break;

        case GIMP_SUBTRACT_MODE:
          /* Derieved from SVG 1.2 formulas, f(Sc, Dc) = Dc - Sc */
          EACH_CHANNEL (out[c] = in[c] + lay[c] - 2 * lay[c] * in[A]);
          break;

        case GIMP_GRAIN_EXTRACT_MODE:
          /* Derieved from SVG 1.2 formulas, f(Sc, Dc) = Dc - Sc + 0.5 */
          EACH_CHANNEL (out[c] = in[c] + lay[c] - 2 * lay[c] * in[A] + 0.5 * in[A] * lay[A]);
          break;

        case GIMP_GRAIN_MERGE_MODE:
          /* Derieved from SVG 1.2 formulas, f(Sc, Dc) = Dc + Sc - 0.5 */
          EACH_CHANNEL (out[c] = in[c] + lay[c] - 0.5 * in[A] * lay[A]);
          break;

        case GIMP_DIVIDE_MODE:
          /* Derieved from SVG 1.2 formulas, f(Sc, Dc) = Dc / Sc */
          EACH_CHANNEL (out[c] = in[c] * lay[A] * lay[A] / lay[c] + lay[c] * (1 - in[A]) + in[c] * (1 - lay[A]));
          break;

        case GIMP_HUE_MODE:
        case GIMP_SATURATION_MODE:
        case GIMP_COLOR_MODE:
        case GIMP_VALUE_MODE:
          /* TODO */
          break;


        case GIMP_ERASE_MODE:
        case GIMP_ANTI_ERASE_MODE:
        case GIMP_COLOR_ERASE_MODE:
        case GIMP_REPLACE_MODE:
          /* Icky eraser and paint modes */
          break;


        case GIMP_DISSOLVE_MODE:
          /* Not a point filter and cannot be implemented here */
          /* g_assert_not_reached (); */
          break;

        default:
          g_error ("Unknown layer mode");
          break;
        }

      /* Alpha is treated the same */
      out[A] = lay[A] + in[A] - lay[A] * in[A];

      in  += 4;
      lay += 4;
      out += 4;
    }

  return TRUE;
}
