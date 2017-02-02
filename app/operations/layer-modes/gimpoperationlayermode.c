/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationlayermode.c
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

#include <gegl-plugin.h>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "libgimpcolor/gimpcolor.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "../operations-types.h"

#include "gimpoperationlayermode.h"


enum
{
  PROP_0,
  PROP_LAYER_MODE,
  PROP_LINEAR,
  PROP_OPACITY,
  PROP_BLEND_SPACE,
  PROP_COMPOSITE_SPACE,
  PROP_COMPOSITE_MODE
};


static void     gimp_operation_layer_mode_set_property (GObject              *object,
                                                        guint                 property_id,
                                                        const GValue         *value,
                                                        GParamSpec           *pspec);
static void     gimp_operation_layer_mode_get_property (GObject              *object,
                                                        guint                 property_id,
                                                        GValue               *value,
                                                        GParamSpec           *pspec);

static void     gimp_operation_layer_mode_prepare      (GeglOperation        *operation);
static gboolean gimp_operation_layer_mode_process      (GeglOperation        *operation,
                                                        GeglOperationContext *context,
                                                        const gchar          *output_prop,
                                                        const GeglRectangle  *result,
                                                        gint                  level);

G_DEFINE_TYPE (GimpOperationLayerMode, gimp_operation_layer_mode,
               GEGL_TYPE_OPERATION_POINT_COMPOSER3)

#define parent_class gimp_operation_layer_mode_parent_class


const Babl *_gimp_fish_rgba_to_perceptual = NULL;
const Babl *_gimp_fish_perceptual_to_rgba = NULL;
const Babl *_gimp_fish_perceptual_to_laba = NULL;
const Babl *_gimp_fish_rgba_to_laba = NULL;
const Babl *_gimp_fish_laba_to_rgba = NULL;
const Babl *_gimp_fish_laba_to_perceptual = NULL;

typedef void (*CompFun)(gfloat *in,
                        gfloat *layer,
                        gfloat *comp,
                        gfloat *mask,
                        float opacity,
                        gfloat *out,
                        gint samples);


static inline void compfun_src_atop (gfloat *in,
                                     gfloat *layer,
                                     gfloat *comp,
                                     gfloat *mask,
                                     gfloat  opacity,
                                     gfloat *out,
                                     gint    samples);
static inline void compfun_dst_atop (gfloat *in,
                                     gfloat *layer,
                                     gfloat *comp,
                                     gfloat *mask,
                                     gfloat  opacity,
                                     gfloat *out,
                                     gint    samples);
static inline void compfun_src_in   (gfloat *in,
                                     gfloat *layer,
                                     gfloat *comp,
                                     gfloat *mask,
                                     gfloat  opacity,
                                     gfloat *out,
                                     gint    samples);
static inline void compfun_src_over (gfloat *in,
                                     gfloat *layer,
                                     gfloat *comp,
                                     gfloat *mask,
                                     gfloat  opacity,
                                     gfloat *out,
                                     gint    samples);

static CompFun compfun_src_atop_dispatch = compfun_src_atop;
static CompFun compfun_dst_atop_dispatch = compfun_dst_atop;
static CompFun compfun_src_in_dispatch   = compfun_src_in;
static CompFun compfun_src_over_dispatch = compfun_src_over;
#if COMPILE_SSE2_INTRINISICS

static inline void compfun_src_atop_sse2 (gfloat *in,
                                          gfloat *layer,
                                          gfloat *comp,
                                          gfloat *mask,
                                          gfloat  opacity,
                                          gfloat *out,
                                          gint    samples);
#endif

static void
gimp_operation_layer_mode_class_init (GimpOperationLayerModeClass *klass)
{
  GObjectClass                     *object_class;
  GeglOperationClass               *operation_class;
  GeglOperationPointComposer3Class *point_composer3_class;

#if COMPILE_SSE2_INTRINISICS
  if (gimp_cpu_accel_get_support () & GIMP_CPU_ACCEL_X86_SSE2)
    compfun_src_atop_dispatch = compfun_src_atop_sse2;
#endif

  object_class          = G_OBJECT_CLASS (klass);
  operation_class       = GEGL_OPERATION_CLASS (klass);
  point_composer3_class = GEGL_OPERATION_POINT_COMPOSER3_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name", "gimp:layer-mode", NULL);

  object_class->set_property = gimp_operation_layer_mode_set_property;
  object_class->get_property = gimp_operation_layer_mode_get_property;

  operation_class->prepare       = gimp_operation_layer_mode_prepare;
  operation_class->process       = gimp_operation_layer_mode_process;
  point_composer3_class->process = gimp_operation_layer_mode_process_pixels;

  g_object_class_install_property (object_class, PROP_LAYER_MODE,
                                   g_param_spec_enum ("layer-mode",
                                                      NULL, NULL,
                                                      GIMP_TYPE_LAYER_MODE,
                                                      GIMP_LAYER_MODE_NORMAL,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_LINEAR,
                                   g_param_spec_boolean ("linear",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_OPACITY,
                                   g_param_spec_double ("opacity",
                                                        NULL, NULL,
                                                        0.0, 1.0, 1.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_BLEND_SPACE,
                                   g_param_spec_enum ("blend-space",
                                                      NULL, NULL,
                                                      GIMP_TYPE_LAYER_COLOR_SPACE,
                                                      GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));


  g_object_class_install_property (object_class, PROP_COMPOSITE_SPACE,
                                   g_param_spec_enum ("composite-space",
                                                      NULL, NULL,
                                                      GIMP_TYPE_LAYER_COLOR_SPACE,
                                                      GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_COMPOSITE_MODE,
                                   g_param_spec_enum ("composite-mode",
                                                      NULL, NULL,
                                                      GIMP_TYPE_LAYER_COMPOSITE_MODE,
                                                      GIMP_LAYER_COMPOSITE_SRC_OVER,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  _gimp_fish_rgba_to_perceptual = babl_fish ("RGBA float", "R'G'B'A float");
  _gimp_fish_perceptual_to_rgba = babl_fish ("R'G'B'A float", "RGBA float");
  _gimp_fish_perceptual_to_laba = babl_fish ("R'G'B'A float", "CIE Lab alpha float");

  _gimp_fish_rgba_to_laba       = babl_fish ("RGBA float", "CIE Lab alpha float");
  _gimp_fish_laba_to_rgba       = babl_fish ("CIE Lab alpha float", "RGBA float");
  _gimp_fish_laba_to_perceptual = babl_fish ("CIE Lab alpha float", "R'G'B'A float");
}

static void
gimp_operation_layer_mode_init (GimpOperationLayerMode *self)
{
}

static void
gimp_operation_layer_mode_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GimpOperationLayerMode *self = GIMP_OPERATION_LAYER_MODE (object);

  switch (property_id)
    {
    case PROP_LAYER_MODE:
      self->layer_mode = g_value_get_enum (value);
      break;

    case PROP_LINEAR:
      self->linear = g_value_get_boolean (value);
      break;

    case PROP_OPACITY:
      self->opacity = g_value_get_double (value);
      break;

    case PROP_BLEND_SPACE:
      self->blend_space = g_value_get_enum (value);
      break;

    case PROP_COMPOSITE_SPACE:
      self->composite_space = g_value_get_enum (value);
      break;

    case PROP_COMPOSITE_MODE:
      self->composite_mode = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_layer_mode_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GimpOperationLayerMode *self = GIMP_OPERATION_LAYER_MODE (object);

  switch (property_id)
    {
    case PROP_LAYER_MODE:
      g_value_set_enum (value, self->layer_mode);
      break;

    case PROP_LINEAR:
      g_value_set_boolean (value, self->linear);
      break;

    case PROP_OPACITY:
      g_value_set_double (value, self->opacity);
      break;

    case PROP_BLEND_SPACE:
      g_value_set_enum (value, self->blend_space);
      break;

    case PROP_COMPOSITE_SPACE:
      g_value_set_enum (value, self->composite_space);
      break;

    case PROP_COMPOSITE_MODE:
      g_value_set_enum (value, self->composite_mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_layer_mode_prepare (GeglOperation *operation)
{
  GimpOperationLayerMode *self = GIMP_OPERATION_LAYER_MODE (operation);
  const Babl             *format;

  if (self->linear)
    format = babl_format ("RGBA float");
  else
    format = babl_format ("R'G'B'A float");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
  gegl_operation_set_format (operation, "aux",    format);
  gegl_operation_set_format (operation, "aux2",   babl_format ("Y float"));
}

static gboolean
gimp_operation_layer_mode_process (GeglOperation        *operation,
                                   GeglOperationContext *context,
                                   const gchar          *output_prop,
                                   const GeglRectangle  *result,
                                   gint                  level)
{
  GimpOperationLayerMode *point = GIMP_OPERATION_LAYER_MODE (operation);

  if (point->opacity == 0.0 ||
      ! gegl_operation_context_get_object (context, "aux"))
    {
      GObject *input;

      /* get the raw values, this does not increase the reference count */
      input = gegl_operation_context_get_object (context, "input");

      if (input)
        {
          gegl_operation_context_set_object (context, "output", input);
          return TRUE;
        }
    }

  /* chain up, which will create the needed buffers for our actual
   * process function
   */
  return GEGL_OPERATION_CLASS (parent_class)->process (operation, context,
                                                       output_prop, result,
                                                       level);
}


static inline GimpBlendFunc gimp_layer_mode_get_blend_fun (GimpLayerMode mode);

static inline void gimp_composite_blend (gpointer       op,
                                         gfloat        *in,
                                         gfloat        *layer,
                                         gfloat        *mask,
                                         gfloat        *out,
                                         glong          samples,
                                         GimpBlendFunc  blend_func);
gboolean
gimp_operation_layer_mode_process_pixels (GeglOperation       *operation,
                                          void                *in,
                                          void                *layer,
                                          void                *mask,
                                          void                *out,
                                          glong                samples,
                                          const GeglRectangle *roi,
                                          gint                 level)
{
  GimpOperationLayerMode *layer_mode = (GimpOperationLayerMode*)(operation);
  gimp_composite_blend (operation, in, layer, mask, out, samples,
                        gimp_layer_mode_get_blend_fun (layer_mode->layer_mode));
  return TRUE;
}




static inline void
compfun_src_atop (gfloat *in,
                  gfloat *layer,
                  gfloat *comp,
                  gfloat *mask,
                  gfloat  opacity,
                  gfloat *out,
                  gint    samples)
{
  while (samples--)
    {
      gfloat layer_alpha = layer[ALPHA] * opacity;

      if (mask)
        layer_alpha *= *mask;

      if (layer_alpha == 0.0f)
        {
          out[RED]   = in[RED];
          out[GREEN] = in[GREEN];
          out[BLUE]  = in[BLUE];
        }
      else
        {
          gint b;

          for (b = RED; b < ALPHA; b++)
            out[b] = layer[b] * layer_alpha + in[b] * (1.0f - layer_alpha);
        }

      out[ALPHA] = in[ALPHA];

      in    += 4;
      out   += 4;
      layer += 4;

      if (mask)
        mask++;
    }
}


static inline void
compfun_src_over (gfloat *in,
                  gfloat *layer,
                  gfloat *comp,
                  gfloat *mask,
                  gfloat  opacity,
                  gfloat *out,
                  gint    samples)
{
  while (samples--)
    {
      gfloat new_alpha;
      gfloat layer_alpha = layer[ALPHA] * opacity;

      if (mask)
        layer_alpha *= *mask;

      new_alpha = layer_alpha + (1.0f - layer_alpha) * in[ALPHA];

      if (layer_alpha == 0.0f || new_alpha == 0.0f)
        {
          out[RED]   = in[RED];
          out[GREEN] = in[GREEN];
          out[BLUE]  = in[BLUE];
        }
      else
        {
          gfloat ratio = layer_alpha / new_alpha;
          gint   b;

          for (b = RED; b < ALPHA; b++)
            out[b] = ratio * (in[ALPHA] * (comp[b] - layer[b]) + layer[b] - in[b]) + in[b];
        }

      out[ALPHA] = new_alpha;

      in    += 4;
      layer += 4;
      comp  += 4;
      out   += 4;

      if (mask)
        mask++;
    }
}

static inline void
compfun_dst_atop (gfloat *in,
                  gfloat *layer,
                  gfloat *comp,
                  gfloat *mask,
                  gfloat  opacity,
                  gfloat *out,
                  gint    samples)
{
  while (samples--)
    {
      gfloat layer_alpha = layer[ALPHA] * opacity;

      if (mask)
        layer_alpha *= *mask;

      if (layer_alpha == 0.0f)
        {
          out[RED]   = in[RED];
          out[GREEN] = in[GREEN];
          out[BLUE]  = in[BLUE];
        }
      else
        {
          gint b;

          for (b = RED; b < ALPHA; b++)
            out[b] = comp[b] * in[ALPHA] + layer[b] * (1.0f - in[ALPHA]);
        }

      out[ALPHA] = layer_alpha;

      in    += 4;
      layer += 4;
      comp  += 4;
      out   += 4;

      if (mask)
        mask++;
    }
}

static inline void
compfun_src_in (gfloat *in,
                gfloat *layer,
                gfloat *comp,
                gfloat *mask,
                gfloat  opacity,
                gfloat *out,
                gint    samples)
{
  while (samples--)
    {
      gfloat new_alpha = in[ALPHA] * layer[ALPHA] * opacity;

      if (mask)
        new_alpha *= *mask;

      if (new_alpha == 0.0f)
        {
          out[RED]   = in[RED];
          out[GREEN] = in[GREEN];
          out[BLUE]  = in[BLUE];
        }
      else
        {
          out[RED]   = layer[RED];
          out[GREEN] = layer[GREEN];
          out[BLUE]  = layer[BLUE];
        }

      out[ALPHA] = new_alpha;

      in    += 4;
      out   += 4;
      layer += 4;

      if (mask)
        mask++;
    }
}

#if COMPILE_SSE2_INTRINISICS

#include <emmintrin.h>

static inline void
compfun_src_atop_sse2 (gfloat *in,
                       gfloat *layer,
                       gfloat *comp,
                       gfloat *mask,
                       gfloat  opacity,
                       gfloat *out,
                       gint    samples)
{
  if ((((uintptr_t)in)    | /* alignment check */
       ((uintptr_t)mask)  |
       ((uintptr_t)comp)  |
       ((uintptr_t)layer) |
       ((uintptr_t)out)    ) & 0x0F)
    {
      return compfun_src_atop (in, layer, comp, mask, opacity, out, samples);
    }
  else
    {
      const __v4sf *v_in      = (const __v4sf*) in;
      const __v4sf *v_layer   = (const __v4sf*) layer;
            __v4sf *v_out     = (__v4sf*) out;
      const __v4sf  v_one     =  _mm_set1_ps (1.0f);
      const __v4sf  v_opacity =  _mm_set1_ps (opacity);

      while (samples--)
      {
        __v4sf  alpha, rgba_in, rgba_layer;

        rgba_in    = *v_in ++;
        rgba_layer = *v_layer++;

        alpha = (__v4sf)_mm_shuffle_epi32((__m128i)rgba_layer,_MM_SHUFFLE(3,3,3,3)) * v_opacity;

        if (mask)
          {
            alpha = alpha * _mm_set1_ps (*mask++);
          }

        if (_mm_ucomigt_ss (alpha, _mm_setzero_ps ()))
          {
            __v4sf out_pixel, out_pixel_rbaa, out_alpha;

            out_alpha      = (__v4sf)_mm_shuffle_epi32((__m128i)rgba_in,_MM_SHUFFLE(3,3,3,3));
            out_pixel      = rgba_layer * alpha + rgba_in * (v_one - alpha);
            out_pixel_rbaa = _mm_shuffle_ps (out_pixel, out_alpha, _MM_SHUFFLE (3, 3, 2, 0));
            out_pixel      = _mm_shuffle_ps (out_pixel, out_pixel_rbaa, _MM_SHUFFLE (2, 1, 1, 0));

            *v_out++ = out_pixel;
          }
        else
          {
            *v_out ++ = rgba_in;
          }
      }
    }
}

#endif

static inline void
gimp_composite_blend (gpointer                op,
                      gfloat                 *in,
                      gfloat                 *layer,
                      gfloat                 *mask,
                      gfloat                 *out,
                      glong                   samples,
                      GimpBlendFunc           blend_func)
{
  GimpOperationLayerMode *layer_mode     = op;
  gfloat                  opacity        = layer_mode->opacity;
  GimpLayerColorSpace     blend_space    = layer_mode->blend_space;
  GimpLayerColorSpace     composite_space= layer_mode->composite_space;
  GimpLayerCompositeMode  composite_mode = layer_mode->composite_mode;

  gfloat *blend_in    = in;
  gfloat *blend_layer = layer;
  gfloat *blend_out   = out;

  gfloat *composite_in    = NULL;
  gfloat *composite_layer = NULL;

  gboolean composite_needs_in_color =
    composite_mode == GIMP_LAYER_COMPOSITE_SRC_OVER ||
    composite_mode == GIMP_LAYER_COMPOSITE_SRC_ATOP;
  gboolean composite_needs_layer_color =
    composite_mode == GIMP_LAYER_COMPOSITE_SRC_OVER ||
    composite_mode == GIMP_LAYER_COMPOSITE_DST_ATOP;

  const Babl *fish_to_blend       = NULL;
  const Babl *fish_to_composite   = NULL;
  const Babl *fish_from_composite = NULL;

  switch (blend_space)
    {
    default:
    case GIMP_LAYER_COLOR_SPACE_RGB_LINEAR:
      fish_to_blend   =  NULL;
      switch (composite_space)
        {
        case GIMP_LAYER_COLOR_SPACE_LAB:
          fish_to_composite   = _gimp_fish_rgba_to_laba;
          fish_from_composite = _gimp_fish_laba_to_rgba;
          break;
        default:
        case GIMP_LAYER_COLOR_SPACE_RGB_LINEAR:
          fish_to_composite   = NULL;
          fish_from_composite = NULL;
          break;
        case GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL:
          fish_to_composite   = _gimp_fish_rgba_to_perceptual;
          fish_from_composite = _gimp_fish_perceptual_to_rgba;
          break;
        }
      break;

    case GIMP_LAYER_COLOR_SPACE_LAB:
      fish_to_blend   = _gimp_fish_rgba_to_laba;
      switch (composite_space)
        {
        case GIMP_LAYER_COLOR_SPACE_LAB:
        default:
          fish_to_composite = NULL;
          fish_from_composite = _gimp_fish_laba_to_rgba;
          break;
        case GIMP_LAYER_COLOR_SPACE_RGB_LINEAR:
          fish_to_composite = _gimp_fish_laba_to_rgba;
          fish_from_composite = NULL;
          break;
        case GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL:
          fish_to_composite = _gimp_fish_laba_to_perceptual;
          fish_from_composite = _gimp_fish_perceptual_to_rgba;
          break;
        }
      break;

    case GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL:
      fish_to_blend = _gimp_fish_rgba_to_perceptual;
      switch (composite_space)
        {
        case GIMP_LAYER_COLOR_SPACE_LAB:
        default:
          fish_to_composite = _gimp_fish_perceptual_to_laba;
          fish_from_composite = NULL;
          break;
        case GIMP_LAYER_COLOR_SPACE_RGB_LINEAR:
          fish_to_composite = _gimp_fish_perceptual_to_rgba;
          fish_from_composite = NULL;
          break;
        case GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL:
          fish_to_composite = NULL;
          fish_from_composite = _gimp_fish_perceptual_to_rgba;
          break;
        }
      break;
    }

  if (in == out) /* in-place detected, avoid clobbering since we need to
                    read it for the compositing stage  */
    blend_out = g_alloca (sizeof (gfloat) * 4 * samples);

  if (fish_to_blend)
    {
      if (in != out || (composite_needs_in_color &&
                        composite_space == GIMP_LAYER_COLOR_SPACE_RGB_LINEAR))
        {
          /* don't convert input in-place if we're not doing in-place output,
           * or if we're going to need the original input for compositing.
           */
          blend_in = g_alloca (sizeof (gfloat) * 4 * samples);
        }
      blend_layer  = g_alloca (sizeof (gfloat) * 4 * samples);

      babl_process (fish_to_blend, in,    blend_in,    samples);
      babl_process (fish_to_blend, layer, blend_layer, samples);
    }

  blend_func (blend_in, blend_layer, blend_out, samples);

  composite_in    = blend_in;
  composite_layer = blend_layer;

  if (fish_to_composite)
    {
      if (composite_space == GIMP_LAYER_COLOR_SPACE_RGB_LINEAR)
        {
          composite_in    = in;
          composite_layer = layer;
        }
      else
        {
          if (composite_needs_in_color)
            {
              if (composite_in == in && in != out)
                composite_in = g_alloca (sizeof (gfloat) * 4 * samples);

              babl_process (fish_to_composite,
                            blend_in, composite_in, samples);
            }

          if (composite_needs_layer_color)
            {
              if (composite_layer == layer)
                composite_layer = g_alloca (sizeof (gfloat) * 4 * samples);

              babl_process (fish_to_composite,
                            blend_layer, composite_layer, samples);
            }
        }

      babl_process (fish_to_composite, blend_out, blend_out, samples);
    }

  switch (composite_mode)
    {
    case GIMP_LAYER_COMPOSITE_SRC_ATOP:
    default:
      compfun_src_atop_dispatch (composite_in, blend_out, NULL, mask, opacity, out, samples);
      break;

    case GIMP_LAYER_COMPOSITE_SRC_OVER:
      compfun_src_over_dispatch (composite_in, composite_layer, blend_out, mask, opacity, out, samples);
      break;

    case GIMP_LAYER_COMPOSITE_DST_ATOP:
      compfun_dst_atop_dispatch (composite_in, composite_layer, blend_out, mask, opacity, out, samples);
      break;

    case GIMP_LAYER_COMPOSITE_SRC_IN:
      compfun_src_in_dispatch (composite_in, blend_out, NULL, mask, opacity, out, samples);
      break;
    }

  if (fish_from_composite)
    {
      babl_process (fish_from_composite, out, out, samples);
    }
}

static inline void
blendfun_screen (const float *dest,
                 const float *src,
                 float       *out,
                 int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            out[c] = 1.0f - (1.0f - dest[c])   * (1.0f - src[c]);
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void /* aka linear_dodge */
blendfun_addition (const float *dest,
                   const float *src,
                   float       *out,
                   int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            out[c] = dest[c] + src[c];
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}


static inline void
blendfun_linear_burn (const float *dest,
                      const float *src,
                      float       *out,
                      int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            out[c] = dest[c] + src[c] - 1.0f;
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_subtract (const float *dest,
                   const float *src,
                   float       *out,
                   int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            out[c] = dest[c] - src[c];
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_multiply (const float *dest,
                   const float *src,
                   float       *out,
                   int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            out[c] = dest[c] * src[c];
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_normal (const float *dest,
                 const float *src,
                 float       *out,
                 int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            out[c] = src[c];
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_burn (const float *dest,
               const float *src,
               float       *out,
               int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            {
              gfloat comp = 1.0f - (1.0f - dest[c]) / src[c];

              /* The CLAMP macro is deliberately inlined and written
               * to map comp == NAN (0 / 0) -> 1
               */
              out[c] = comp < 0 ? 0.0f : comp < 1.0f ? comp : 1.0f;
            }
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_darken_only (const float *dest,
                      const float *src,
                      float       *out,
                      int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            out[c] = MIN (dest[c], src[c]);
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_luminance_lighten_only (const float *dest,
                                 const float *src,
                                 float       *out,
                                 int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;
          float dest_luminance =
             GIMP_RGB_LUMINANCE(dest[0], dest[1], dest[2]);
          float src_luminance =
             GIMP_RGB_LUMINANCE(src[0], src[1], src[2]);

          if (dest_luminance >= src_luminance)
            for (c = 0; c < 3; c++)
              out[c] = dest[c];
          else
            for (c = 0; c < 3; c++)
              out[c] = src[c];
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_luminance_darken_only (const float *dest,
                                const float *src,
                                float       *out,
                                int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;
          float dest_luminance =
             GIMP_RGB_LUMINANCE(dest[0], dest[1], dest[2]);
          float src_luminance =
             GIMP_RGB_LUMINANCE(src[0], src[1], src[2]);

          if (dest_luminance <= src_luminance)
            for (c = 0; c < 3; c++)
              out[c] = dest[c];
          else
            for (c = 0; c < 3; c++)
              out[c] = src[c];
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_lighten_only (const float *dest,
                       const float *src,
                       float       *out,
                       int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            out[c] = MAX (dest[c], src[c]);
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_difference (const float *dest,
                     const float *src,
                     float       *out,
                     int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            {
              out[c] = dest[c] - src[c];

              if (out[c] < 0)
                out[c] = -out[c];
            }
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_divide (const float *dest,
                 const float *src,
                 float       *out,
                 int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            {
              gfloat comp = dest[c] / src[c];

              /* make infinities(or NaN) correspond to a high number,
               * to get more predictable math, ideally higher than 5.0
               * but it seems like some babl conversions might be
               * acting up then
               */
              if (!(comp > -42949672.0f && comp < 5.0f))
                comp = 5.0f;

              out[c] = comp;
            }
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_dodge (const float *dest,
                const float *src,
                float       *out,
                int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            {
              gfloat comp = dest[c] / (1.0f - src[c]);

              comp = MIN (comp, 1.0f);

              out[c] = comp;
            }
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_grain_extract (const float *dest,
                        const float *src,
                        float       *out,
                        int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            out[c] = dest[c] - src[c] + 0.5f;
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_grain_merge (const float *dest,
                      const float *src,
                      float       *out,
                      int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            out[c] = dest[c] + src[c] - 0.5f;
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_hardlight (const float *dest,
                    const float *src,
                    float       *out,
                    int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            {
              gfloat comp;

              if (src[c] > 0.5f)
                {
                  comp = (1.0f - dest[c]) * (1.0f - (src[c] - 0.5f) * 2.0f);
                  comp = MIN (1 - comp, 1);
                }
              else
                {
                  comp = dest[c] * (src[c] * 2.0f);
                  comp = MIN (comp, 1.0f);
                }

              out[c] = comp;
            }
        }
      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_softlight (const float *dest,
                    const float *src,
                    float       *out,
                    int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            {
              gfloat multiply = dest[c] * src[c];
              gfloat screen   = 1.0f - (1.0f - dest[c]) * (1.0f - src[c]);
              gfloat comp     = (1.0f - dest[c]) * multiply + dest[c] * screen;

              out[c] = comp;
            }
        }
      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_overlay (const float *dest,
                  const float *src,
                  float       *out,
                  int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            {
              gfloat comp;

              if (dest[c] < 0.5f)
                {
                  comp = 2.0f * dest[c] * src[c];
                }
              else
                {
                  comp = 1.0f - 2.0f * (1.0f - src[c]) * (1.0f - dest[c]);
                }

              out[c] = comp;
            }
        }
      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_hsv_color (const float *dest,
                    const float *src,
                    float       *out,
                    int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          GimpRGB dest_rgb = { dest[0], dest[1], dest[2] };
          GimpRGB src_rgb  = { src[0], src[1], src[2] };
          GimpHSL src_hsl, dest_hsl;

          gimp_rgb_to_hsl (&dest_rgb, &dest_hsl);
          gimp_rgb_to_hsl (&src_rgb, &src_hsl);

          dest_hsl.h = src_hsl.h;
          dest_hsl.s = src_hsl.s;

          gimp_hsl_to_rgb (&dest_hsl, &dest_rgb);

          out[RED]   = dest_rgb.r;
          out[GREEN] = dest_rgb.g;
          out[BLUE]  = dest_rgb.b;
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_hsv_hue (const float *dest,
                  const float *src,
                  float       *out,
                  int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          GimpRGB dest_rgb = { dest[0], dest[1], dest[2] };
          GimpRGB src_rgb  = { src[0],  src[1],  src[2] };
          GimpHSV src_hsv, dest_hsv;

          gimp_rgb_to_hsv (&dest_rgb, &dest_hsv);
          gimp_rgb_to_hsv (&src_rgb, &src_hsv);

          dest_hsv.h = src_hsv.h;
          gimp_hsv_to_rgb (&dest_hsv, &dest_rgb);

          out[RED]   = dest_rgb.r;
          out[GREEN] = dest_rgb.g;
          out[BLUE]  = dest_rgb.b;
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_hsv_saturation (const float *dest,
                         const float *src,
                         float       *out,
                         int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          GimpRGB dest_rgb = { dest[0], dest[1], dest[2] };
          GimpRGB src_rgb  = { src[0], src[1], src[2] };
          GimpHSV src_hsv, dest_hsv;

          gimp_rgb_to_hsv (&dest_rgb, &dest_hsv);
          gimp_rgb_to_hsv (&src_rgb, &src_hsv);

          dest_hsv.s = src_hsv.s;
          gimp_hsv_to_rgb (&dest_hsv, &dest_rgb);

          out[RED]   = dest_rgb.r;
          out[GREEN] = dest_rgb.g;
          out[BLUE]  = dest_rgb.b;
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_hsv_value (const float *dest,
                    const float *src,
                    float       *out,
                    int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          GimpRGB dest_rgb = { dest[0], dest[1], dest[2] };
          GimpRGB src_rgb  = { src[0], src[1], src[2] };
          GimpHSV src_hsv, dest_hsv;

          gimp_rgb_to_hsv (&dest_rgb, &dest_hsv);
          gimp_rgb_to_hsv (&src_rgb, &src_hsv);

          dest_hsv.v = src_hsv.v;
          gimp_hsv_to_rgb (&dest_hsv, &dest_rgb);

          out[RED]   = dest_rgb.r;
          out[GREEN] = dest_rgb.g;
          out[BLUE]  = dest_rgb.b;
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_lch_chroma (const float *dest,
                     const float *src,
                     float       *out,
                     int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gfloat A1 = dest[1];
          gfloat B1 = dest[2];
          gfloat c1 = hypotf (A1, B1);

          if (c1 != 0.0f)
            {
              gfloat A2 = src[1];
              gfloat B2 = src[2];
              gfloat c2 = hypotf (A2, B2);
              gfloat A  = c2 * A1 / c1;
              gfloat B  = c2 * B1 / c1;

              out[0] = dest[0];
              out[1] = A;
              out[2] = B;
            }
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_lch_color (const float *dest,
                    const float *src,
                    float       *out,
                    int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          out[0] = dest[0];
          out[1] = src[1];
          out[2] = src[2];
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
  }
}

static inline void
blendfun_lch_hue (const float *dest,
                  const float *src,
                  float       *out,
                  int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gfloat A2 = src[1];
          gfloat B2 = src[2];
          gfloat c2 = hypotf (A2, B2);

          if (c2 > 0.1f)
            {
              gfloat A1 = dest[1];
              gfloat B1 = dest[2];
              gfloat c1 = hypotf (A1, B1);
              gfloat A  = c1 * A2 / c2;
              gfloat B  = c1 * B2 / c2;

              out[0] = dest[0];
              out[1] = A;
              out[2] = B;
            }
          else
            {
              out[0] = dest[0];
              out[1] = dest[1];
              out[2] = dest[2];
            }
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_lch_lightness (const float *dest,
                        const float *src,
                        float       *out,
                        int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          out[0] = src[0];
          out[1] = dest[1];
          out[2] = dest[2];
        }

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}


static inline void
blendfun_copy (const float *dest,
               const float *src,
               float       *out,
               int          samples)
{
  while (samples--)
    {
      gint c;
      for (c = 0; c < 4; c++)
        out[c] = src[c];

      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

/* added according to:
    http://www.deepskycolors.com/archivo/2010/04/21/formulas-for-Photoshop-blending-modes.html */
static inline void
blendfun_vivid_light (const float *dest,
                      const float *src,
                      float       *out,
                      int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            {
              gfloat comp;

              if (src[c] > 0.5f)
                {
                  comp = (1.0f - (1.0f - dest[c]) / (2.0f * (src[c])));
                }
              else
                {
                  comp = dest[c] / (1.0f - 2.0f * (src[c] - 0.5));
                }
              comp = MIN (comp, 1.0f);

              out[c] = comp;
            }
        }
      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}


/* added according to:
    http://www.deepskycolors.com/archivo/2010/04/21/formulas-for-Photoshop-blending-modes.html */
static inline void
blendfun_linear_light (const float *dest,
                       const float *src,
                       float       *out,
                       int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            {
              gfloat comp;
              if (src[c] > 0.5f)
                {
                  comp = dest[c] + 2.0 * (src[c] - 0.5);
                }
              else
                {
                  comp = dest[c] + 2.0 * src[c] - 1.0;
                }
              out[c] = comp;
            }
        }
      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}


/* added according to:
    http://www.deepskycolors.com/archivo/2010/04/21/formulas-for-Photoshop-blending-modes.html */
static inline void
blendfun_pin_light (const float *dest,
                    const float *src,
                    float       *out,
                    int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            {
              gfloat comp;
              if (src[c] > 0.5f)
                {
                  comp = MAX(dest[c], 2 * (src[c] - 0.5));
                }
              else
                {
                  comp = MIN(dest[c], 2 * src[c]);
                }
              out[c] = comp;
            }
        }
      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void
blendfun_exclusion (const float *dest,
                    const float *src,
                    float       *out,
                    int          samples)
{
  while (samples--)
    {
      if (dest[ALPHA] != 0.0f && src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            {
              out[c] = 0.5f - 2.0f * (dest[c] - 0.5f) * (src[c] - 0.5f);
            }
        }
      out[ALPHA] = src[ALPHA];

      out  += 4;
      src  += 4;
      dest += 4;
    }
}

static inline void dummy_fun(void)
{
}

static inline GimpBlendFunc gimp_layer_mode_get_blend_fun (GimpLayerMode mode)
{
  switch (mode)
  {
    case GIMP_LAYER_MODE_SCREEN_LINEAR:
    case GIMP_LAYER_MODE_SCREEN:         return blendfun_screen;
    case GIMP_LAYER_MODE_ADDITION_LINEAR:
    case GIMP_LAYER_MODE_ADDITION:       return blendfun_addition;
    case GIMP_LAYER_MODE_SUBTRACT_LINEAR:
    case GIMP_LAYER_MODE_SUBTRACT:       return blendfun_subtract;
    case GIMP_LAYER_MODE_MULTIPLY_LINEAR:
    case GIMP_LAYER_MODE_MULTIPLY:       return blendfun_multiply;
    case GIMP_LAYER_MODE_NORMAL_LINEAR:
    case GIMP_LAYER_MODE_NORMAL:         return blendfun_normal;
    case GIMP_LAYER_MODE_BURN_LINEAR:
    case GIMP_LAYER_MODE_BURN:           return blendfun_burn;
    case GIMP_LAYER_MODE_GRAIN_MERGE_LINEAR:
    case GIMP_LAYER_MODE_GRAIN_MERGE:    return blendfun_grain_merge;
    case GIMP_LAYER_MODE_GRAIN_EXTRACT_LINEAR:
    case GIMP_LAYER_MODE_GRAIN_EXTRACT:  return blendfun_grain_extract;
    case GIMP_LAYER_MODE_DODGE_LINEAR:
    case GIMP_LAYER_MODE_DODGE:          return blendfun_dodge;
    case GIMP_LAYER_MODE_OVERLAY_LINEAR:
    case GIMP_LAYER_MODE_OVERLAY:        return blendfun_overlay;
    case GIMP_LAYER_MODE_HSV_COLOR:      return blendfun_hsv_color;
    case GIMP_LAYER_MODE_HSV_HUE:        return blendfun_hsv_hue;
    case GIMP_LAYER_MODE_HSV_SATURATION: return blendfun_hsv_saturation;
    case GIMP_LAYER_MODE_HSV_VALUE:      return blendfun_hsv_value;
    case GIMP_LAYER_MODE_LCH_CHROMA:     return blendfun_lch_chroma;
    case GIMP_LAYER_MODE_LCH_COLOR:      return blendfun_lch_color;
    case GIMP_LAYER_MODE_LCH_HUE:        return blendfun_lch_hue;
    case GIMP_LAYER_MODE_LCH_LIGHTNESS:  return blendfun_lch_lightness;
    case GIMP_LAYER_MODE_HARDLIGHT_LINEAR:
    case GIMP_LAYER_MODE_HARDLIGHT:      return blendfun_hardlight;
    case GIMP_LAYER_MODE_SOFTLIGHT_LINEAR:
    case GIMP_LAYER_MODE_SOFTLIGHT:      return blendfun_softlight;
    case GIMP_LAYER_MODE_DIVIDE:
    case GIMP_LAYER_MODE_DIVIDE_LINEAR:  return blendfun_divide;
    case GIMP_LAYER_MODE_DIFFERENCE_LINEAR:
    case GIMP_LAYER_MODE_DIFFERENCE:     return blendfun_difference;
    case GIMP_LAYER_MODE_DARKEN_ONLY:    return blendfun_darken_only;
    case GIMP_LAYER_MODE_LIGHTEN_ONLY:   return blendfun_lighten_only;
    case GIMP_LAYER_MODE_LUMINANCE_DARKEN_ONLY:
    case GIMP_LAYER_MODE_LUMA_DARKEN_ONLY:  return blendfun_luminance_darken_only;
    case GIMP_LAYER_MODE_LUMINANCE_LIGHTEN_ONLY:
    case GIMP_LAYER_MODE_LUMA_LIGHTEN_ONLY: return blendfun_luminance_lighten_only;
    case GIMP_LAYER_MODE_VIVID_LIGHT_LINEAR:
    case GIMP_LAYER_MODE_VIVID_LIGHT:  return blendfun_vivid_light;
    case GIMP_LAYER_MODE_PIN_LIGHT_LINEAR:
    case GIMP_LAYER_MODE_PIN_LIGHT:    return blendfun_pin_light;
    case GIMP_LAYER_MODE_LINEAR_LIGHT_LINEAR:
    case GIMP_LAYER_MODE_LINEAR_LIGHT: return blendfun_linear_light;
    case GIMP_LAYER_MODE_EXCLUSION_LINEAR:
    case GIMP_LAYER_MODE_EXCLUSION:    return blendfun_exclusion;
    case GIMP_LAYER_MODE_LINEAR_BURN_LINEAR:
    case GIMP_LAYER_MODE_LINEAR_BURN:  return blendfun_linear_burn;

    case GIMP_LAYER_MODE_DISSOLVE:
    case GIMP_LAYER_MODE_BEHIND:
    case GIMP_LAYER_MODE_BEHIND_LINEAR:
    case GIMP_LAYER_MODE_MULTIPLY_LEGACY:
    case GIMP_LAYER_MODE_SCREEN_LEGACY:
    case GIMP_LAYER_MODE_OVERLAY_LEGACY:
    case GIMP_LAYER_MODE_DIFFERENCE_LEGACY:
    case GIMP_LAYER_MODE_ADDITION_LEGACY:
    case GIMP_LAYER_MODE_SUBTRACT_LEGACY:
    case GIMP_LAYER_MODE_DARKEN_ONLY_LEGACY:
    case GIMP_LAYER_MODE_LIGHTEN_ONLY_LEGACY:
    case GIMP_LAYER_MODE_HSV_HUE_LEGACY:
    case GIMP_LAYER_MODE_HSV_SATURATION_LEGACY:
    case GIMP_LAYER_MODE_HSV_COLOR_LEGACY:
    case GIMP_LAYER_MODE_HSV_VALUE_LEGACY:
    case GIMP_LAYER_MODE_DIVIDE_LEGACY:
    case GIMP_LAYER_MODE_DODGE_LEGACY:
    case GIMP_LAYER_MODE_BURN_LEGACY:
    case GIMP_LAYER_MODE_HARDLIGHT_LEGACY:
    case GIMP_LAYER_MODE_SOFTLIGHT_LEGACY:
    case GIMP_LAYER_MODE_GRAIN_EXTRACT_LEGACY:
    case GIMP_LAYER_MODE_GRAIN_MERGE_LEGACY:
    case GIMP_LAYER_MODE_COLOR_ERASE:
    case GIMP_LAYER_MODE_ERASE:
    case GIMP_LAYER_MODE_REPLACE:
    case GIMP_LAYER_MODE_ANTI_ERASE:
      return (void*)dummy_fun;
  }
  return (void*)dummy_fun;
}
