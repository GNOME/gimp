/* GIMP - The GNU Image Manipulation Program
 * gimpblendcomposite
 * Copyright (C) 2017 Øyvind Kolås <pippin@gimp.org>
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

#ifndef __GIMP_BLEND_COMPOSITE_H__
#define __GIMP_BLEND_COMPOSITE_H__


#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"


extern const Babl *_gimp_fish_rgba_to_perceptual;
extern const Babl *_gimp_fish_perceptual_to_rgba;
extern const Babl *_gimp_fish_perceptual_to_laba;
extern const Babl *_gimp_fish_rgba_to_laba;
extern const Babl *_gimp_fish_laba_to_rgba;
extern const Babl *_gimp_fish_laba_to_perceptual;


static inline void
compfun_src_atop (gfloat *in,
                  gfloat *layer,
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
      compfun_src_atop (composite_in, blend_out, mask, opacity, out, samples);
      break;

    case GIMP_LAYER_COMPOSITE_SRC_OVER:
      compfun_src_over (composite_in, composite_layer, blend_out, mask, opacity, out, samples);
      break;

    case GIMP_LAYER_COMPOSITE_DST_ATOP:
      compfun_dst_atop (composite_in, composite_layer, blend_out, mask, opacity, out, samples);
      break;

    case GIMP_LAYER_COMPOSITE_SRC_IN:
      compfun_src_in (composite_in, blend_out, mask, opacity, out, samples);
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
blendfun_lighten_only (const float *dest,
                       const float *src,
                       float       *out,
                       int          samples)
{
  while (samples--)
    {
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            {
              gfloat comp;

              if (src[c] < 0.5f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
        {
          GimpRGB dest_rgb = { dest[0], dest[1], dest[2] };
          GimpRGB src_rgb  = { src[0], src[1], src[2] };
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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
      if (src[ALPHA] != 0.0f)
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

#endif /* __GIMP_BLEND_COMPOSITE_H__ */
