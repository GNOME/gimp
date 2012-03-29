/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationpointlayermode.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2008 Martin Nordholts <martinn@svn.gnome.org>
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

#include <cairo.h>
#include <gegl-plugin.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimp-gegl-types.h"

#include "paint-funcs/paint-funcs.h"

#include "gimpoperationpointlayermode.h"


/* The size of the area of which an evenly transparent Dissolve layer
 * repeats its dissolve pattern
 */
#define DISSOLVE_REPEAT_WIDTH  400
#define DISSOLVE_REPEAT_HEIGHT 300

#define DISSOLVE_SEED          737893334


#define R     (RED)
#define G     (GREEN)
#define B     (BLUE)
#define A     (ALPHA)
#define L     0
#define C     1
#define H     2
#define inA   (in[A])
#define inCa  (in[c])
#define inC   (in[A]  ? in[c]  / in[A]  : 0.0)
#define layA  (lay[A])
#define layCa (lay[c])
#define layC  (lay[A] ? lay[c] / lay[A] : 0.0)
#define outCa (out[c])
#define outA  (out[A])
#define outC  (out[A] ? out[c] / out[A] : 0.0)
#define newCa (new[c])

#define EACH_CHANNEL(expr)            \
        for (c = RED; c < ALPHA; c++) \
          {                           \
            expr;                     \
          }


enum
{
  PROP_0,
  PROP_BLEND_MODE
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
                                                              const GeglRectangle *roi,
                                                              gint                 level);


G_DEFINE_TYPE (GimpOperationPointLayerMode, gimp_operation_point_layer_mode,
               GEGL_TYPE_OPERATION_POINT_COMPOSER)


static guint32 dissolve_lut[DISSOLVE_REPEAT_WIDTH * DISSOLVE_REPEAT_HEIGHT];


static void
gimp_operation_point_layer_mode_class_init (GimpOperationPointLayerModeClass *klass)
{
  GObjectClass                    *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass              *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointComposerClass *point_class     = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);
  GRand                           *rand            = g_rand_new_with_seed (DISSOLVE_SEED);
  int                              i;

  object_class->set_property   = gimp_operation_point_layer_mode_set_property;
  object_class->get_property   = gimp_operation_point_layer_mode_get_property;

  gegl_operation_class_set_keys (operation_class,
           "name"       , "gimp:point-layer-mode",
           "description", "GIMP point layer mode operation",
           "categories" , "compositors",
           NULL);

  operation_class->prepare     = gimp_operation_point_layer_mode_prepare;

  point_class->process         = gimp_operation_point_layer_mode_process;

  g_object_class_install_property (object_class, PROP_BLEND_MODE,
                                   g_param_spec_enum ("blend-mode", NULL, NULL,
                                                      GIMP_TYPE_LAYER_MODE_EFFECTS,
                                                      GIMP_NORMAL_MODE,
                                                      GIMP_PARAM_READWRITE));

  for (i = 0; i < DISSOLVE_REPEAT_WIDTH * DISSOLVE_REPEAT_HEIGHT; i++)
    dissolve_lut[i] = g_rand_int (rand);

  g_rand_free (rand);
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
  const Babl *format = babl_format ("RaGaBaA float");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
  gegl_operation_set_format (operation, "aux",    format);
}

static void
gimp_operation_point_layer_mode_get_new_color_lchab (GimpLayerModeEffects  blend_mode,
                                                     const gfloat         *in,
                                                     const gfloat         *lay,
                                                     gfloat               *new)
{
  float in_lchab[3];
  float lay_lchab[3];
  float new_lchab[3];
  const Babl *ragabaa_to_lchab = babl_fish (babl_format ("RaGaBaA float"),
                                            babl_format ("CIE LCH(ab) float"));
  const Babl *lchab_to_ragabaa = babl_fish (babl_format ("CIE LCH(ab) float"),
                                            babl_format ("RaGaBaA float"));

  babl_process (ragabaa_to_lchab, (void*)in,  (void*)in_lchab,  1);
  babl_process (ragabaa_to_lchab, (void*)lay, (void*)lay_lchab, 1);

  switch (blend_mode)
    {
    case GIMP_HUE_MODE:
      new_lchab[L] = in_lchab[L];
      new_lchab[C] = in_lchab[C];
      new_lchab[H] = lay_lchab[H];
      break;

    case GIMP_SATURATION_MODE:
      new_lchab[L] = in_lchab[L];
      new_lchab[C] = lay_lchab[C];
      new_lchab[H] = in_lchab[H];
      break;

    case GIMP_COLOR_MODE:
      new_lchab[L] = in_lchab[L];
      new_lchab[C] = lay_lchab[C];
      new_lchab[H] = lay_lchab[H];
      break;

    case GIMP_VALUE_MODE: /* GIMP_LIGHTNESS_MODE */
      new_lchab[L] = lay_lchab[L];
      new_lchab[C] = in_lchab[C];
      new_lchab[H] = in_lchab[H];
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  babl_process (lchab_to_ragabaa, new_lchab, new, 1);
}

static void
gimp_operation_point_layer_mode_get_color_erase_color (const gfloat *in,
                                                       const gfloat *lay,
                                                       gfloat       *out)
{
  GimpRGB inRGB;
  GimpRGB layRGB;

  if (inA <= 0.0)
    gimp_rgba_set (&inRGB, 0.0, 0.0, 0.0, inA);
  else
    gimp_rgba_set (&inRGB, in[R] / inA, in[G] / inA, in[B] / inA, inA);

  if (layA <= 0.0)
    gimp_rgba_set (&layRGB, 0.0, 0.0, 0.0, layA);
  else
    gimp_rgba_set (&layRGB, lay[R] / layA, lay[G] / layA, lay[B] / layA, layA);

  paint_funcs_color_erase_helper (&inRGB, &layRGB);

  out[A] = inRGB.a;
  out[R] = inRGB.r * out[A];
  out[G] = inRGB.g * out[A];
  out[B] = inRGB.b * out[A];
}

static gboolean
gimp_operation_point_layer_mode_process (GeglOperation       *operation,
                                         void                *in_buf,
                                         void                *aux_buf,
                                         void                *out_buf,
                                         glong                samples,
                                         const GeglRectangle *roi,
                                         gint                 level)
{
  GimpOperationPointLayerMode *self       = GIMP_OPERATION_POINT_LAYER_MODE (operation);
  GimpLayerModeEffects         blend_mode = self->blend_mode;

  gfloat *in     = in_buf;     /* composite of layers below */
  gfloat *lay    = aux_buf;    /* layer */
  gfloat *out    = out_buf;    /* resulting composite */
  glong   sample = samples;
  gint    c      = 0;
  gint    x      = 0;
  gint    y      = 0;
  gfloat  new[3] = { 0.0, 0.0, 0.0 };

  while (sample--)
    {
      switch (blend_mode)
        {
        case GIMP_ERASE_MODE:
        case GIMP_ANTI_ERASE_MODE:
        case GIMP_COLOR_ERASE_MODE:
        case GIMP_REPLACE_MODE:
        case GIMP_DISSOLVE_MODE:
          /* These modes handle alpha themselves */
          break;

        default:
          /* Porter-Duff model for the rest */
          outA = layA + inA - layA * inA;
        }

      switch (blend_mode)
        {
        case GIMP_ERASE_MODE:
          /* Eraser mode */
          outA = inA - inA * layA;
          if (inA <= 0.0)
            EACH_CHANNEL (
            outCa = 0.0)
          else
            EACH_CHANNEL (
            outCa = inC * outA);
          break;

        case GIMP_ANTI_ERASE_MODE:
          /* Eraser mode */
          outA = inA + (1 - inA) * layA;
          if (inA <= 0.0)
            EACH_CHANNEL (
            outCa = 0.0)
          else
            EACH_CHANNEL (
            outCa = inC * outA);
          break;

        case GIMP_COLOR_ERASE_MODE:
          /* Paint mode */
          gimp_operation_point_layer_mode_get_color_erase_color (in, lay, out);
          break;

        case GIMP_REPLACE_MODE:
          /* Filter fade mode */
          outA = layA;
          EACH_CHANNEL(
          outCa = layCa);
          break;

        case GIMP_DISSOLVE_MODE:
          /* The layer pixel has layA probability of being composited
           * with 100% opacity, else not all
           */
          x = (roi->x + sample - (sample / roi->width) * roi->width) % DISSOLVE_REPEAT_WIDTH;
          y = (roi->y + sample / roi->width)                         % DISSOLVE_REPEAT_HEIGHT;

          if (layA * G_MAXUINT32 >= dissolve_lut[y * DISSOLVE_REPEAT_WIDTH + x])
            {
              outA = 1.0;
              EACH_CHANNEL (
              outCa = layC);
            }
          else
            {
              outA = inA;
              EACH_CHANNEL (
              outCa = inCa);
            }
          break;

        case GIMP_NORMAL_MODE:
          /* Porter-Duff A over B */
          EACH_CHANNEL (
          outCa = layCa + inCa * (1 - layA));
          break;

        case GIMP_BEHIND_MODE:
          /* Porter-Duff B over A */
          EACH_CHANNEL (
          outCa = inCa + layCa * (1 - inA));
          break;

        case GIMP_MULTIPLY_MODE:
          /* SVG 1.2 multiply */
          EACH_CHANNEL (
          outCa = layCa * inCa + layCa * (1 - inA) + inCa * (1 - layA));
          break;

        case GIMP_SCREEN_MODE:
          /* SVG 1.2 screen */
          EACH_CHANNEL (
          outCa = layCa + inCa - layCa * inCa);
          break;

        case GIMP_DIFFERENCE_MODE:
          /* SVG 1.2 difference */
          EACH_CHANNEL (
          outCa = inCa + layCa - 2 * MIN (layCa * inA, inCa * layA));
          break;

        case GIMP_DARKEN_ONLY_MODE:
          /* SVG 1.2 darken */
          EACH_CHANNEL (
          outCa = MIN (layCa * inA, inCa * layA) + layCa * (1 - inA) + inCa * (1 - layA));
          break;

        case GIMP_LIGHTEN_ONLY_MODE:
          /* SVG 1.2 lighten */
          EACH_CHANNEL (
          outCa = MAX (layCa * inA, inCa * layA) + layCa * (1 - inA) + inCa * (1 - layA));
          break;

        case GIMP_OVERLAY_MODE:
          /* SVG 1.2 overlay */
          EACH_CHANNEL (
          if (2 * inCa < inA)
            outCa = 2 * layCa * inCa + layCa * (1 - inA) + inCa * (1 - layA);
          else
            outCa = layA * inA - 2 * (inA - inCa) * (layA - layCa) + layCa * (1 - inA) + inCa * (1 - layA));
          break;

        case GIMP_DODGE_MODE:
          /* SVG 1.2 color-dodge */
          EACH_CHANNEL (
          if (layCa * inA + inCa * layA >= layA * inA)
            outCa = layA * inA + layCa * (1 - inA) + inCa * (1 - layA);
          else
            outCa = inCa * layA / (1 - layC) + layCa * (1 - inA) + inCa * (1 - layA));
          break;

        case GIMP_BURN_MODE:
          /* SVG 1.2 color-burn */
          EACH_CHANNEL (
          if (layCa * inA + inCa * layA <= layA * inA)
            outCa = layCa * (1 - inA) + inCa * (1 - layA);
          else
            outCa = layA * (layCa * inA + inCa * layA - layA * inA) / layCa + layCa * (1 - inA) + inCa * (1 - layA));
          break;

        case GIMP_HARDLIGHT_MODE:
          /* SVG 1.2 hard-light */
          EACH_CHANNEL (
          if (2 * layCa < layA)
            outCa = 2 * layCa * inCa + layCa * (1 - inA) + inCa * (1 - layA);
          else
            outCa = layA * inA - 2 * (inA - inCa) * (layA - layCa) + layCa * (1 - inA) + inCa * (1 - layA));
          break;

        case GIMP_SOFTLIGHT_MODE:
          /* Custom SVG 1.2:
           *
           * f(Sc, Dc) = Dc * (Dc + (2 * Sc * (1 - Dc)))
           */
          EACH_CHANNEL (
          outCa = inCa * (layA * inC + (2 * layCa * (1 - inC))) + layCa * (1 - inA) + inCa * (1 - layA));
          break;

        case GIMP_ADDITION_MODE:
          /* Custom SVG 1.2:
           *
           * if Dc + Sc >= 1
           *   f(Sc, Dc) = 1
           * otherwise
           *   f(Sc, Dc) = Dc + Sc
           */
          EACH_CHANNEL (
          if (layCa * inA + inCa * layA >= layA * inA)
            outCa = layA * inA + layCa * (1 - inA) + inCa * (1 - layA);
          else
            outCa = inCa + layCa);
          break;

        case GIMP_SUBTRACT_MODE:
          /* Custom SVG 1.2:
           *
           * if Dc - Sc <= 0
           *   f(Sc, Dc) = 0
           * otherwise
           *   f(Sc, Dc) = Dc - Sc
           */
          EACH_CHANNEL (
          if (inCa * layA - layCa * inA <= 0)
            outCa = layCa * (1 - inA) + inCa * (1 - layA);
          else
            outCa = inCa + layCa - 2 * layCa * inA);
          break;

        case GIMP_GRAIN_EXTRACT_MODE:
          /* Custom SVG 1.2:
           *
           * if Dc - Sc + 0.5 >= 1
           *   f(Sc, Dc) = 1
           * otherwise if Dc - Sc + 0.5 <= 0
           *   f(Sc, Dc) = 0
           * otherwise
           *   f(Sc, Dc) = f(Sc, Dc) = Dc - Sc + 0.5
           */
          EACH_CHANNEL (
          if (inCa * layA - layCa * inA + 0.5 * layA * inA >= layA * inA)
            outCa = layA * inA + layCa * (1 - inA) + inCa * (1 - layA);
          else if (inCa * layA - layCa * inA + 0.5 * layA * inA <= 0)
            outCa = layCa * (1 - inA) + inCa * (1 - layA);
          else
            outCa = inCa + layCa - 2 * layCa * inA + 0.5 * inA * layA);
          break;

        case GIMP_GRAIN_MERGE_MODE:
          /* Custom SVG 1.2:
           *
           * if Dc + Sc - 0.5 >= 1
           *   f(Sc, Dc) = 1
           * otherwise if Dc + Sc - 0.5 <= 0
           *   f(Sc, Dc) = 0
           * otherwise
           *   f(Sc, Dc) = f(Sc, Dc) = Dc + Sc - 0.5
           */
          EACH_CHANNEL (
          if (inCa * layA + layCa * inA - 0.5 * layA * inA >= layA * inA)
            outCa = layA * inA + layCa * (1 - inA) + inCa * (1 - layA);
          else if (inCa * layA + layCa * inA - 0.5 * layA * inA <= 0)
            outCa = layCa * (1 - inA) + inCa * (1 - layA);
          else
            outCa = inCa + layCa - 0.5 * inA * layA);
          break;

        case GIMP_DIVIDE_MODE:
          /* Custom SVG 1.2:
           *
           * if Dc / Sc > 1
           *   f(Sc, Dc) = 1
           * otherwise
           *   f(Sc, Dc) = Dc / Sc
           */
          EACH_CHANNEL (
          if (layA == 0.0 || inCa / layCa > inA / layA)
            outCa = layA * inA + layCa * (1 - inA) + inCa * (1 - layA);
          else
            outCa = inCa * layA * layA / layCa + layCa * (1 - inA) + inCa * (1 - layA));
          break;

        case GIMP_HUE_MODE:
        case GIMP_SATURATION_MODE:
        case GIMP_COLOR_MODE:
        case GIMP_VALUE_MODE: /* GIMP_LIGHTNESS_MODE */
          /* Custom SVG 1.2:
           *
           * f(Sc, Dc) = New color
           */

          /* FIXME: Doing this call for each pixel is very slow, we
           * should make conversions on larger chunks of data
           */
          gimp_operation_point_layer_mode_get_new_color_lchab (blend_mode,
                                                               in,
                                                               lay,
                                                               new);
          EACH_CHANNEL (
          outCa = newCa * layA * inA + layCa * (1 - inA) + inCa * (1 - layA));
          break;

        default:
          g_error ("Unknown layer mode");
          break;
        }

      in  += 4;
      lay += 4;
      out += 4;
    }

  return TRUE;
}
