/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationlayermode-blend.c
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
 *               2017 Øyvind Kolås <pippin@gimp.org>
 *               2017 Ell
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

#include <gegl-plugin.h>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "../operations-types.h"

#include "gimpoperationlayermode-blend.h"


#define EPSILON      1e-6f

#define SAFE_DIV_MIN EPSILON
#define SAFE_DIV_MAX (1.0f / SAFE_DIV_MIN)


/*  local function prototypes  */

static inline gfloat   safe_div (gfloat a,
                                 gfloat b);


/*  private functions  */


/* returns a / b, clamped to [-SAFE_DIV_MAX, SAFE_DIV_MAX].
 * if -SAFE_DIV_MIN <= a <= SAFE_DIV_MIN, returns 0.
 */
static inline gfloat
safe_div (gfloat a,
          gfloat b)
{
  gfloat result = 0.0f;

  if (fabsf (a) > SAFE_DIV_MIN)
    {
      result = a / b;
      result = CLAMP (result, -SAFE_DIV_MAX, SAFE_DIV_MAX);
    }

  return result;
}


/*  public functions  */


/*  non-subtractive blending functions.  these functions must set comp[ALPHA]
 *  to the same value as layer[ALPHA].  when in[ALPHA] or layer[ALPHA] are
 *  zero, the value of comp[RED..BLUE] is unconstrained (in particular, it may
 *  be NaN).
 */


void /* aka linear_dodge */
gimp_operation_layer_mode_blend_addition (GeglOperation *operation,
                                          const gfloat  *in,
                                          const gfloat  *layer,
                                          gfloat        *comp,
                                          gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            comp[c] = in[c] + layer[c];
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_burn (GeglOperation *operation,
                                      const gfloat  *in,
                                      const gfloat  *layer,
                                      gfloat        *comp,
                                      gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            comp[c] = 1.0f - safe_div (1.0f - in[c], layer[c]);
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_darken_only (GeglOperation *operation,
                                             const gfloat  *in,
                                             const gfloat  *layer,
                                             gfloat        *comp,
                                             gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            comp[c] = MIN (in[c], layer[c]);
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_difference (GeglOperation *operation,
                                            const gfloat  *in,
                                            const gfloat  *layer,
                                            gfloat        *comp,
                                            gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            comp[c] = fabsf (in[c] - layer[c]);
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_divide (GeglOperation *operation,
                                        const gfloat  *in,
                                        const gfloat  *layer,
                                        gfloat        *comp,
                                        gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            comp[c] = safe_div (in[c], layer[c]);
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_dodge (GeglOperation *operation,
                                       const gfloat  *in,
                                       const gfloat  *layer,
                                       gfloat        *comp,
                                       gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            comp[c] = safe_div (in[c], 1.0f - layer[c]);
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_exclusion (GeglOperation *operation,
                                           const gfloat  *in,
                                           const gfloat  *layer,
                                           gfloat        *comp,
                                           gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            comp[c] = 0.5f - 2.0f * (in[c] - 0.5f) * (layer[c] - 0.5f);
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_grain_extract (GeglOperation *operation,
                                               const gfloat  *in,
                                               const gfloat  *layer,
                                               gfloat        *comp,
                                               gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            comp[c] = in[c] - layer[c] + 0.5f;
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_grain_merge (GeglOperation *operation,
                                             const gfloat  *in,
                                             const gfloat  *layer,
                                             gfloat        *comp,
                                             gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            comp[c] = in[c] + layer[c] - 0.5f;
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_hard_mix (GeglOperation *operation,
                                          const gfloat  *in,
                                          const gfloat  *layer,
                                          gfloat        *comp,
                                          gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            comp[c] = in[c] + layer[c] < 1.0f ? 0.0f : 1.0f;
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_hardlight (GeglOperation *operation,
                                           const gfloat  *in,
                                           const gfloat  *layer,
                                           gfloat        *comp,
                                           gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            {
              gfloat val;

              if (layer[c] > 0.5f)
                {
                  val = (1.0f - in[c]) * (1.0f - (layer[c] - 0.5f) * 2.0f);
                  val = MIN (1.0f - val, 1.0f);
                }
              else
                {
                  val = in[c] * (layer[c] * 2.0f);
                  val = MIN (val, 1.0f);
                }

              comp[c] = val;
            }
        }
      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_hsl_color (GeglOperation *operation,
                                           const gfloat  *in,
                                           const gfloat  *layer,
                                           gfloat        *comp,
                                           gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gfloat dest_min, dest_max, dest_l;
          gfloat src_min,  src_max,  src_l;

          dest_min = MIN (in[0],  in[1]);
          dest_min = MIN (dest_min, in[2]);
          dest_max = MAX (in[0],  in[1]);
          dest_max = MAX (dest_max, in[2]);
          dest_l   = (dest_min + dest_max) / 2.0f;

          src_min  = MIN (layer[0],  layer[1]);
          src_min  = MIN (src_min, layer[2]);
          src_max  = MAX (layer[0],  layer[1]);
          src_max  = MAX (src_max, layer[2]);
          src_l    = (src_min + src_max) / 2.0f;

          if (fabs (src_l) > EPSILON && fabs (1.0 - src_l) > EPSILON)
            {
              gboolean dest_high;
              gboolean src_high;
              gfloat   ratio;
              gfloat   offset;
              gint     c;

              dest_high = dest_l > 0.5f;
              src_high  = src_l  > 0.5f;

              dest_l = MIN (dest_l, 1.0f - dest_l);
              src_l  = MIN (src_l,  1.0f - src_l);

              ratio                  = dest_l / src_l;

              offset                 = 0.0f;
              if (dest_high) offset += 1.0f - 2.0f * dest_l;
              if (src_high)  offset += 2.0f * dest_l - ratio;

              for (c = 0; c < 3; c++)
                comp[c] = layer[c] * ratio + offset;
            }
          else
            {
              comp[RED]   = dest_l;
              comp[GREEN] = dest_l;
              comp[BLUE]  = dest_l;
            }
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_hsv_hue (GeglOperation *operation,
                                         const gfloat  *in,
                                         const gfloat  *layer,
                                         gfloat        *comp,
                                         gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gfloat src_min,  src_max,  src_delta;
          gfloat dest_min, dest_max, dest_delta, dest_s;

          src_min   = MIN (layer[0], layer[1]);
          src_min   = MIN (src_min, layer[2]);
          src_max   = MAX (layer[0], layer[1]);
          src_max   = MAX (src_max, layer[2]);
          src_delta = src_max - src_min;

          if (src_delta > EPSILON)
            {
              gfloat ratio;
              gfloat offset;
              gint   c;

              dest_min   = MIN (in[0], in[1]);
              dest_min   = MIN (dest_min, in[2]);
              dest_max   = MAX (in[0], in[1]);
              dest_max   = MAX (dest_max, in[2]);
              dest_delta = dest_max - dest_min;
              dest_s     = dest_max ? dest_delta / dest_max : 0.0f;

              ratio  = dest_s * dest_max / src_delta;
              offset = dest_max - src_max * ratio;

              for (c = 0; c < 3; c++)
                comp[c] = layer[c] * ratio + offset;
            }
          else
            {
              gint c;

              for (c = 0; c < 3; c++)
                comp[c] = in[c];
            }
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_hsv_saturation (GeglOperation *operation,
                                                const gfloat  *in,
                                                const gfloat  *layer,
                                                gfloat        *comp,
                                                gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gfloat src_min,  src_max,  src_delta, src_s;
          gfloat dest_min, dest_max, dest_delta;

          dest_min   = MIN (in[0], in[1]);
          dest_min   = MIN (dest_min, in[2]);
          dest_max   = MAX (in[0], in[1]);
          dest_max   = MAX (dest_max, in[2]);
          dest_delta = dest_max - dest_min;

          if (dest_delta > EPSILON)
            {
              gfloat ratio;
              gfloat offset;
              gint   c;

              src_min   = MIN (layer[0], layer[1]);
              src_min   = MIN (src_min, layer[2]);
              src_max   = MAX (layer[0], layer[1]);
              src_max   = MAX (src_max, layer[2]);
              src_delta = src_max - src_min;
              src_s     = src_max ? src_delta / src_max : 0.0f;

              ratio  = src_s * dest_max / dest_delta;
              offset = (1.0f - ratio) * dest_max;

              for (c = 0; c < 3; c++)
                comp[c] = in[c] * ratio + offset;
            }
          else
            {
              comp[RED]   = dest_max;
              comp[GREEN] = dest_max;
              comp[BLUE]  = dest_max;
            }
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_hsv_value (GeglOperation *operation,
                                           const gfloat  *in,
                                           const gfloat  *layer,
                                           gfloat        *comp,
                                           gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gfloat dest_v;
          gfloat src_v;

          dest_v = MAX (in[0], in[1]);
          dest_v = MAX (dest_v, in[2]);

          src_v  = MAX (layer[0], layer[1]);
          src_v  = MAX (src_v, layer[2]);

          if (fabs (dest_v) > EPSILON)
            {
              gfloat ratio = src_v / dest_v;
              gint   c;

              for (c = 0; c < 3; c++)
                comp[c] = in[c] * ratio;
            }
          else
            {
              comp[RED]   = src_v;
              comp[GREEN] = src_v;
              comp[BLUE]  = src_v;
            }
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_lch_chroma (GeglOperation *operation,
                                            const gfloat  *in,
                                            const gfloat  *layer,
                                            gfloat        *comp,
                                            gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gfloat A1 = in[1];
          gfloat B1 = in[2];
          gfloat c1 = hypotf (A1, B1);

          if (c1 > EPSILON)
            {
              gfloat A2 = layer[1];
              gfloat B2 = layer[2];
              gfloat c2 = hypotf (A2, B2);
              gfloat A  = c2 * A1 / c1;
              gfloat B  = c2 * B1 / c1;

              comp[0] = in[0];
              comp[1] = A;
              comp[2] = B;
            }
          else
            {
              comp[0] = in[0];
              comp[1] = in[1];
              comp[2] = in[2];
            }
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_lch_color (GeglOperation *operation,
                                           const gfloat  *in,
                                           const gfloat  *layer,
                                           gfloat        *comp,
                                           gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          comp[0] = in[0];
          comp[1] = layer[1];
          comp[2] = layer[2];
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
  }
}

void
gimp_operation_layer_mode_blend_lch_hue (GeglOperation *operation,
                                         const gfloat  *in,
                                         const gfloat  *layer,
                                         gfloat        *comp,
                                         gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gfloat A2 = layer[1];
          gfloat B2 = layer[2];
          gfloat c2 = hypotf (A2, B2);

          if (c2 > EPSILON)
            {
              gfloat A1 = in[1];
              gfloat B1 = in[2];
              gfloat c1 = hypotf (A1, B1);
              gfloat A  = c1 * A2 / c2;
              gfloat B  = c1 * B2 / c2;

              comp[0] = in[0];
              comp[1] = A;
              comp[2] = B;
            }
          else
            {
              comp[0] = in[0];
              comp[1] = in[1];
              comp[2] = in[2];
            }
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_lch_lightness (GeglOperation *operation,
                                               const gfloat  *in,
                                               const gfloat  *layer,
                                               gfloat        *comp,
                                               gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          comp[0] = layer[0];
          comp[1] = in[1];
          comp[2] = in[2];
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_lighten_only (GeglOperation *operation,
                                              const gfloat  *in,
                                              const gfloat  *layer,
                                              gfloat        *comp,
                                              gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            comp[c] = MAX (in[c], layer[c]);
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_linear_burn (GeglOperation *operation,
                                             const gfloat  *in,
                                             const gfloat  *layer,
                                             gfloat        *comp,
                                             gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            comp[c] = in[c] + layer[c] - 1.0f;
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

/* added according to:
    http://www.deepskycolors.com/archivo/2010/04/21/formulas-for-Photoshop-blending-modes.html */
void
gimp_operation_layer_mode_blend_linear_light (GeglOperation *operation,
                                              const gfloat  *in,
                                              const gfloat  *layer,
                                              gfloat        *comp,
                                              gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            {
              gfloat val;

              if (layer[c] <= 0.5f)
                val = in[c] + 2.0f * layer[c] - 1.0f;
              else
                val = in[c] + 2.0f * (layer[c] - 0.5f);

              comp[c] = val;
            }
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_luma_darken_only (GeglOperation *operation,
                                                  const gfloat  *in,
                                                  const gfloat  *layer,
                                                  gfloat        *comp,
                                                  gint           samples)
{
  const Babl *space  = gegl_operation_get_source_space (operation, "input");
  double red_luminance, green_luminance, blue_luminance;
  babl_space_get_rgb_luminance (space, 
    &red_luminance, &green_luminance, &blue_luminance);

  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gfloat dest_luminance;
          gfloat src_luminance;
          gint   c;

          dest_luminance  = (in[0]    * red_luminance)   +
                            (in[1]    * green_luminance) +
                            (in[2]    * blue_luminance);

          src_luminance   = (layer[0] * red_luminance)   +
                            (layer[1] * green_luminance) +
                            (layer[2] * blue_luminance);

          if (dest_luminance <= src_luminance)
            {
              for (c = 0; c < 3; c++)
                comp[c] = in[c];
            }
          else
            {
              for (c = 0; c < 3; c++)
                comp[c] = layer[c];
            }
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_luma_lighten_only (GeglOperation *operation,
                                                   const gfloat  *in,
                                                   const gfloat  *layer,
                                                   gfloat        *comp,
                                                   gint           samples)
{
  const Babl *space  = gegl_operation_get_source_space (operation, "input");
  double red_luminance, green_luminance, blue_luminance;
  babl_space_get_rgb_luminance (space, 
    &red_luminance, &green_luminance, &blue_luminance);

  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gfloat dest_luminance;
          gfloat src_luminance;
          gint   c;

          dest_luminance  = (in[0]    * red_luminance)   +
                            (in[1]    * green_luminance) +
                            (in[2]    * blue_luminance);

          src_luminance   = (layer[0] * red_luminance)   +
                            (layer[1] * green_luminance) +
                            (layer[2] * blue_luminance);

          if (dest_luminance >= src_luminance)
            {
              for (c = 0; c < 3; c++)
                comp[c] = in[c];
            }
          else
            {
              for (c = 0; c < 3; c++)
                comp[c] = layer[c];
            }
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_luminance (GeglOperation *operation,
                                           const gfloat  *in,
                                           const gfloat  *layer,
                                           gfloat        *comp,
                                           gint           samples)
{
  const Babl *fish;
  gfloat     *scratch;
  gfloat     *in_Y;
  gfloat     *layer_Y;
  const Babl *space = gegl_operation_get_source_space (operation, "input");

  fish = babl_fish (babl_format_with_space ("RGBA float", space),
                    babl_format_with_space ("Y float",    space));

  scratch = gegl_scratch_new (gfloat, 2 * samples);

  in_Y    = scratch;
  layer_Y = scratch + samples;

  babl_process (fish, in,    in_Y,    samples);
  babl_process (fish, layer, layer_Y, samples);

  while (samples--)
    {
      if (layer[ALPHA] != 0.0f && in[ALPHA] != 0.0f)
        {
          gfloat ratio = safe_div (layer_Y[0], in_Y[0]);
          gint   c;

          for (c = 0; c < 3; c ++)
            comp[c] = in[c] * ratio;
        }

      comp[ALPHA] = layer[ALPHA];

      comp    += 4;
      in      += 4;
      layer   += 4;
      in_Y    ++;
      layer_Y ++;
    }

  gegl_scratch_free (scratch);
}

void
gimp_operation_layer_mode_blend_multiply (GeglOperation *operation,
                                          const gfloat  *in,
                                          const gfloat  *layer,
                                          gfloat        *comp,
                                          gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            comp[c] = in[c] * layer[c];
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_overlay (GeglOperation *operation,
                                         const gfloat  *in,
                                         const gfloat  *layer,
                                         gfloat        *comp,
                                         gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            {
              gfloat val;

              if (in[c] < 0.5f)
                val = 2.0f * in[c] * layer[c];
              else
                val = 1.0f - 2.0f * (1.0f - layer[c]) * (1.0f - in[c]);

              comp[c] = val;
            }
        }
      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

/* added according to:
    http://www.deepskycolors.com/archivo/2010/04/21/formulas-for-Photoshop-blending-modes.html */
void
gimp_operation_layer_mode_blend_pin_light (GeglOperation *operation,
                                           const gfloat  *in,
                                           const gfloat  *layer,
                                           gfloat        *comp,
                                           gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            {
              gfloat val;

              if (layer[c] > 0.5f)
                val = MAX(in[c], 2.0f * (layer[c] - 0.5f));
              else
                val = MIN(in[c], 2.0f * layer[c]);

              comp[c] = val;
            }
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_screen (GeglOperation *operation,
                                        const gfloat  *in,
                                        const gfloat  *layer,
                                        gfloat        *comp,
                                        gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            comp[c] = 1.0f - (1.0f - in[c])   * (1.0f - layer[c]);
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_softlight (GeglOperation *operation,
                                           const gfloat  *in,
                                           const gfloat  *layer,
                                           gfloat        *comp,
                                           gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            {
              gfloat multiply = in[c] * layer[c];
              gfloat screen   = 1.0f - (1.0f - in[c]) * (1.0f - layer[c]);
              gfloat val      = (1.0f - in[c]) * multiply + in[c] * screen;

              comp[c] = val;
            }
        }
      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

void
gimp_operation_layer_mode_blend_subtract (GeglOperation *operation,
                                          const gfloat  *in,
                                          const gfloat  *layer,
                                          gfloat        *comp,
                                          gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            comp[c] = in[c] - layer[c];
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}

/* added according to:
    http://www.simplefilter.de/en/basics/mixmods.html */
void
gimp_operation_layer_mode_blend_vivid_light (GeglOperation *operation,
                                             const gfloat  *in,
                                             const gfloat  *layer,
                                             gfloat        *comp,
                                             gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          gint c;

          for (c = 0; c < 3; c++)
            {
              gfloat val;

              if (layer[c] <= 0.5f)
                {
                  val = 1.0f - safe_div (1.0f - in[c], 2.0f * layer[c]);
                  val = MAX (val, 0.0f);
                }
              else
                {
                  val = safe_div (in[c], 2.0f * (1.0f - layer[c]));
                  val = MIN (val, 1.0f);
                }

              comp[c] = val;
            }
        }

      comp[ALPHA] = layer[ALPHA];

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}


/*  subtractive blending functions.  these functions must set comp[ALPHA] to
 *  the modified alpha of the overlapping content, as a fraction of the
 *  original overlapping content (i.e., an alpha of 1.0 specifies that no
 *  content is subtracted.)  when in[ALPHA] or layer[ALPHA] are zero, the value
 *  of comp[RED..BLUE] is unconstrained (in particular, it may be NaN).
 */


void
gimp_operation_layer_mode_blend_color_erase (GeglOperation *operation,
                                             const gfloat  *in,
                                             const gfloat  *layer,
                                             gfloat        *comp,
                                             gint           samples)
{
  while (samples--)
    {
      if (in[ALPHA] != 0.0f && layer[ALPHA] != 0.0f)
        {
          const gfloat *color   = in;
          const gfloat *bgcolor = layer;
          gfloat       alpha;
          gint         c;

          alpha = 0.0f;

          for (c = 0; c < 3; c++)
            {
              gfloat col   = CLAMP (color[c],   0.0f, 1.0f);
              gfloat bgcol = CLAMP (bgcolor[c], 0.0f, 1.0f);

              if (fabs (col - bgcol) > EPSILON)
                {
                  gfloat a;

                  if (col > bgcol)
                    a = (col - bgcol) / (1.0f - bgcol);
                  else
                    a = (bgcol - col) / bgcol;

                  alpha = MAX (alpha, a);
                }
            }

          if (alpha > EPSILON)
            {
              gfloat alpha_inv = 1.0f / alpha;

              for (c = 0; c < 3; c++)
                comp[c] = (color[c] - bgcolor[c]) * alpha_inv + bgcolor[c];
            }
          else
            {
              comp[RED] = comp[GREEN] = comp[BLUE] = 0.0f;
            }

          comp[ALPHA] = alpha;
        }
      else
        comp[ALPHA] = 0.0f;

      comp  += 4;
      layer += 4;
      in    += 4;
    }
}
