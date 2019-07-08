/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationlayermode-composite.c
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

#include "../operations-types.h"

#include "gimpoperationlayermode-composite.h"


/*  non-subtractive compositing functions.  these functions expect comp[ALPHA]
 *  to be the same as layer[ALPHA].  when in[ALPHA] or layer[ALPHA] are zero,
 *  the value of comp[RED..BLUE] is unconstrained (in particular, it may be
 *  NaN).
 */


void
gimp_operation_layer_mode_composite_union (const gfloat *in,
                                           const gfloat *layer,
                                           const gfloat *comp,
                                           const gfloat *mask,
                                           gfloat        opacity,
                                           gfloat       *out,
                                           gint          samples)
{
  while (samples--)
    {
      gfloat new_alpha;
      gfloat in_alpha    = in[ALPHA];
      gfloat layer_alpha = layer[ALPHA] * opacity;

      if (mask)
        layer_alpha *= *mask;

      new_alpha = layer_alpha + (1.0f - layer_alpha) * in_alpha;

      if (layer_alpha == 0.0f || new_alpha == 0.0f)
        {
          out[RED]   = in[RED];
          out[GREEN] = in[GREEN];
          out[BLUE]  = in[BLUE];
        }
      else if (in_alpha == 0.0f)
        {
          out[RED]   = layer[RED];
          out[GREEN] = layer[GREEN];
          out[BLUE]  = layer[BLUE];
        }
      else
        {
          gfloat ratio = layer_alpha / new_alpha;
          gint   b;

          for (b = RED; b < ALPHA; b++)
            out[b] = ratio * (in_alpha * (comp[b] - layer[b]) + layer[b] - in[b]) + in[b];
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

void
gimp_operation_layer_mode_composite_clip_to_backdrop (const gfloat *in,
                                                      const gfloat *layer,
                                                      const gfloat *comp,
                                                      const gfloat *mask,
                                                      gfloat        opacity,
                                                      gfloat       *out,
                                                      gint          samples)
{
  while (samples--)
    {
      gfloat layer_alpha = comp[ALPHA] * opacity;

      if (mask)
        layer_alpha *= *mask;

      if (in[ALPHA] == 0.0f || layer_alpha == 0.0f)
        {
          out[RED]   = in[RED];
          out[GREEN] = in[GREEN];
          out[BLUE]  = in[BLUE];
        }
      else
        {
          gint b;

          for (b = RED; b < ALPHA; b++)
            out[b] = comp[b] * layer_alpha + in[b] * (1.0f - layer_alpha);
        }

      out[ALPHA] = in[ALPHA];

      in   += 4;
      comp += 4;
      out  += 4;

      if (mask)
        mask++;
    }
}

void
gimp_operation_layer_mode_composite_clip_to_layer (const gfloat *in,
                                                   const gfloat *layer,
                                                   const gfloat *comp,
                                                   const gfloat *mask,
                                                   gfloat        opacity,
                                                   gfloat       *out,
                                                   gint          samples)
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
      else if (in[ALPHA] == 0.0f)
        {
          out[RED]   = layer[RED];
          out[GREEN] = layer[GREEN];
          out[BLUE]  = layer[BLUE];
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

void
gimp_operation_layer_mode_composite_intersection (const gfloat *in,
                                                  const gfloat *layer,
                                                  const gfloat *comp,
                                                  const gfloat *mask,
                                                  gfloat        opacity,
                                                  gfloat       *out,
                                                  gint          samples)
{
  while (samples--)
    {
      gfloat new_alpha = in[ALPHA] * comp[ALPHA] * opacity;

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
          out[RED]   = comp[RED];
          out[GREEN] = comp[GREEN];
          out[BLUE]  = comp[BLUE];
        }

      out[ALPHA] = new_alpha;

      in   += 4;
      comp += 4;
      out  += 4;

      if (mask)
        mask++;
    }
}

/*  subtractive compositing functions.  these functions expect comp[ALPHA] to
 *  specify the modified alpha of the overlapping content, as a fraction of the
 *  original overlapping content (i.e., an alpha of 1.0 specifies that no
 *  content is subtracted.)  when in[ALPHA] or layer[ALPHA] are zero, the value
 *  of comp[RED..BLUE] is unconstrained (in particular, it may be NaN).
 */

void
gimp_operation_layer_mode_composite_union_sub (const gfloat *in,
                                               const gfloat *layer,
                                               const gfloat *comp,
                                               const gfloat *mask,
                                               gfloat        opacity,
                                               gfloat       *out,
                                               gint          samples)
{
  while (samples--)
    {
      gfloat in_alpha    = in[ALPHA];
      gfloat layer_alpha = layer[ALPHA] * opacity;
      gfloat comp_alpha  = comp[ALPHA];
      gfloat new_alpha;

      if (mask)
        layer_alpha *= *mask;

      new_alpha = in_alpha + layer_alpha -
                  (2.0f - comp_alpha) * in_alpha * layer_alpha;

      if (layer_alpha == 0.0f || new_alpha == 0.0f)
        {
          out[RED]   = in[RED];
          out[GREEN] = in[GREEN];
          out[BLUE]  = in[BLUE];
        }
      else if (in_alpha == 0.0f)
        {
          out[RED]   = layer[RED];
          out[GREEN] = layer[GREEN];
          out[BLUE]  = layer[BLUE];
        }
      else
        {
          gfloat ratio       = in_alpha / new_alpha;
          gfloat layer_coeff = 1.0f / in_alpha - 1.0f;
          gint   b;

          for (b = RED; b < ALPHA; b++)
            out[b] = ratio * (layer_alpha * (comp_alpha * comp[b] + layer_coeff * layer[b] - in[b]) + in[b]);
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

void
gimp_operation_layer_mode_composite_clip_to_backdrop_sub (const gfloat *in,
                                                          const gfloat *layer,
                                                          const gfloat *comp,
                                                          const gfloat *mask,
                                                          gfloat        opacity,
                                                          gfloat       *out,
                                                          gint          samples)
{
  while (samples--)
    {
      gfloat layer_alpha = layer[ALPHA] * opacity;
      gfloat comp_alpha  = comp[ALPHA];
      gfloat new_alpha;

      if (mask)
        layer_alpha *= *mask;

      comp_alpha *= layer_alpha;

      new_alpha = 1.0f - layer_alpha + comp_alpha;

      if (in[ALPHA] == 0.0f || comp_alpha == 0.0f)
        {
          out[RED]   = in[RED];
          out[GREEN] = in[GREEN];
          out[BLUE]  = in[BLUE];
        }
      else
        {
          gfloat ratio = comp_alpha / new_alpha;
          gint   b;

          for (b = RED; b < ALPHA; b++)
            out[b] = comp[b] * ratio + in[b] * (1.0f - ratio);
        }

      new_alpha *= in[ALPHA];

      out[ALPHA] = new_alpha;

      in    += 4;
      layer += 4;
      comp  += 4;
      out   += 4;

      if (mask)
        mask++;
    }
}

void
gimp_operation_layer_mode_composite_clip_to_layer_sub (const gfloat *in,
                                                       const gfloat *layer,
                                                       const gfloat *comp,
                                                       const gfloat *mask,
                                                       gfloat        opacity,
                                                       gfloat       *out,
                                                       gint          samples)
{
  while (samples--)
    {
      gfloat in_alpha    = in[ALPHA];
      gfloat layer_alpha = layer[ALPHA] * opacity;
      gfloat comp_alpha  = comp[ALPHA];
      gfloat new_alpha;

      if (mask)
        layer_alpha *= *mask;

      comp_alpha *= in_alpha;

      new_alpha = 1.0f - in_alpha + comp_alpha;

      if (layer_alpha == 0.0f)
        {
          out[RED]   = in[RED];
          out[GREEN] = in[GREEN];
          out[BLUE]  = in[BLUE];
        }
      else if (in_alpha == 0.0f)
        {
          out[RED]   = layer[RED];
          out[GREEN] = layer[GREEN];
          out[BLUE]  = layer[BLUE];
        }
      else
        {
          gfloat ratio = comp_alpha / new_alpha;
          gint   b;

          for (b = RED; b < ALPHA; b++)
            out[b] = comp[b] * ratio + layer[b] * (1.0f - ratio);
        }

      new_alpha *= layer_alpha;

      out[ALPHA] = new_alpha;

      in    += 4;
      layer += 4;
      comp  += 4;
      out   += 4;

      if (mask)
        mask++;
    }
}

void
gimp_operation_layer_mode_composite_intersection_sub (const gfloat *in,
                                                      const gfloat *layer,
                                                      const gfloat *comp,
                                                      const gfloat *mask,
                                                      gfloat        opacity,
                                                      gfloat       *out,
                                                      gint          samples)
{
  while (samples--)
    {
      gfloat new_alpha = in[ALPHA] * layer[ALPHA] * comp[ALPHA] * opacity;

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
          out[RED]   = comp[RED];
          out[GREEN] = comp[GREEN];
          out[BLUE]  = comp[BLUE];
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
