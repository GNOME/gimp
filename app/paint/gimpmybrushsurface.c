/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#include <gegl.h>
#include <stdint.h>

#include <mypaint-surface.h>

#include "paint-types.h"

#include "libgimpmath/gimpmath.h"

#include <cairo.h>
#ifdef _OPENMP
#include <omp.h>
#endif

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "fastapprox/fastpow.h"
#include "libgimpcolor/gimpcolor.h"

#include "gimpmybrushoptions.h"
#include "gimpmybrushsurface.h"

#define WGM_EPSILON 0.001

struct _GimpMybrushSurface
{
  MyPaintSurface2     surface;
  GeglBuffer         *buffer;
  GeglBuffer         *paint_mask;
  gint                paint_mask_x;
  gint                paint_mask_y;
  GeglRectangle       dirty;
  GimpComponentMask   component_mask;
  GimpMybrushOptions *options;
};

static const float T_MATRIX_SMALL[3][10] = {{0.026595621243689,0.049779426257903,0.022449850859496,-0.218453689278271
,-0.256894883201278,0.445881722194840,0.772365886289756,0.194498761382537
,0.014038157587820,0.007687264480513}
,{-0.032601672674412,-0.061021043498478,-0.052490001018404
,0.206659098273522,0.572496335158169,0.317837248815438,-0.021216624031211
,-0.019387668756117,-0.001521339050858,-0.000835181622534}
,{0.339475473216284,0.635401374177222,0.771520797089589,0.113222640692379
,-0.055251113343776,-0.048222578468680,-0.012966666339586
,-0.001523814504223,-0.000094718948810,-0.000051604594741}};

static const float spectral_r_small[10] = {0.009281362787953,0.009732627042016,0.011254252737167,0.015105578649573
,0.024797924177217,0.083622585502406,0.977865045723212,1.000000000000000
,0.999961046144372,0.999999992756822};

static const float spectral_g_small[10] = {0.002854127435775,0.003917589679914,0.012132151699187,0.748259205918013
,1.000000000000000,0.865695937531795,0.037477469241101,0.022816789725717
,0.021747419446456,0.021384940572308};

static const float spectral_b_small[10] = {0.537052150373386,0.546646402401469,0.575501819073983,0.258778829633924
,0.041709923751716,0.012662638828324,0.007485593127390,0.006766900622462
,0.006699764779016,0.006676219883241};

void
rgb_to_spectral                  (float     r,
                                  float     g,
                                  float     b,
                                  float    *spectral_);
void
spectral_to_rgb                  (float    *spectral,
                                  float    *rgb_);

float
spectral_blend_factor            (float     x);

void
get_color_pixels_legacy          (float     mask,
                                  float    *pixel,
                                  float    *sum_weight,
                                  float    *sum_r,
                                  float    *sum_g,
                                  float    *sum_b,
                                  float    *sum_a);

void
get_color_pixels_accumulate      (float     mask,
                                  float    *pixel,
                                  float    *sum_weight,
                                  float    *sum_r,
                                  float    *sum_g,
                                  float    *sum_b,
                                  float    *sum_a,
                                  float     paint,
                                  uint16_t  sample_interval,
                                  float     random_sample_rate,
                                  uint16_t  interval_counter);

void
draw_dab_pixels_BlendMode_Normal (float    *mask,
                                  float    *pixel,
                                  float     color_r,
                                  float     color_g,
                                  float     color_b,
                                  float     opacity);

void
draw_dab_pixels_BlendMode_Normal_Paint
                                 (float    *mask,
                                  float    *pixel,
                                  float     color_r,
                                  float     color_g,
                                  float     color_b,
                                  float     opacity);

void
draw_dab_pixels_BlendMode_Normal_and_Eraser
                                 (float    *mask,
                                  float    *pixel,
                                  float     color_r,
                                  float     color_g,
                                  float     color_b,
                                  float     color_a,
                                  float     opacity);

void
draw_dab_pixels_BlendMode_Normal_and_Eraser_Paint
                                 (float    *mask,
                                  float    *pixel,
                                  float     color_r,
                                  float     color_g,
                                  float     color_b,
                                  float     color_a,
                                  float     opacity);

void
draw_dab_pixels_BlendMode_LockAlpha
                                 (float    *mask,
                                  float    *pixel,
                                  float     color_r,
                                  float     color_g,
                                  float     color_b,
                                  float     opacity);

void
draw_dab_pixels_BlendMode_LockAlpha_Paint
                                 (float    *mask,
                                  float    *pixel,
                                  float     color_r,
                                  float     color_g,
                                  float     color_b,
                                  float     opacity);

void
draw_dab_pixels_BlendMode_Posterize
                                 (float    *mask,
                                  float    *pixel,
                                  float     opacity,
                                  float     posterize_num);

/* --- Taken from mypaint-tiled-surface.c --- */
static inline float
calculate_rr (int   xp,
              int   yp,
              float x,
              float y,
              float aspect_ratio,
              float sn,
              float cs,
              float one_over_radius2)
{
    /* code duplication, see brush::count_dabs_to() */
    const float yy = (yp + 0.5f - y);
    const float xx = (xp + 0.5f - x);
    const float yyr=(yy*cs-xx*sn)*aspect_ratio;
    const float xxr=yy*sn+xx*cs;
    const float rr = (yyr*yyr + xxr*xxr) * one_over_radius2;
    /* rr is in range 0.0..1.0*sqrt(2) */
    return rr;
}

static inline float
calculate_r_sample (float x,
                    float y,
                    float aspect_ratio,
                    float sn,
                    float cs)
{
    const float yyr=(y*cs-x*sn)*aspect_ratio;
    const float xxr=y*sn+x*cs;
    const float r = (yyr*yyr + xxr*xxr);
    return r;
}

static inline float
sign_point_in_line (float px,
                    float py,
                    float vx,
                    float vy)
{
    return (px - vx) * (-vy) - (vx) * (py - vy);
}

static inline void
closest_point_to_line (float  lx,
                       float  ly,
                       float  px,
                       float  py,
                       float *ox,
                       float *oy)
{
    const float l2 = lx*lx + ly*ly;
    const float ltp_dot = px*lx + py*ly;
    const float t = ltp_dot / l2;
    *ox = lx * t;
    *oy = ly * t;
}


/* This works by taking the visibility at the nearest point
 * and dividing by 1.0 + delta.
 *
 * - nearest point: point where the dab has more influence
 * - farthest point: point at a fixed distance away from
 *                   the nearest point
 * - delta: how much occluded is the farthest point relative
 *          to the nearest point
 */
static inline float
calculate_rr_antialiased (int  xp,
                          int  yp,
                          float x,
                          float y,
                          float aspect_ratio,
                          float sn,
                          float cs,
                          float one_over_radius2,
                          float r_aa_start)
{
    /* calculate pixel position and borders in a way
     * that the dab's center is always at zero */
    float pixel_right = x - (float)xp;
    float pixel_bottom = y - (float)yp;
    float pixel_center_x = pixel_right - 0.5f;
    float pixel_center_y = pixel_bottom - 0.5f;
    float pixel_left = pixel_right - 1.0f;
    float pixel_top = pixel_bottom - 1.0f;

    float nearest_x, nearest_y; /* nearest to origin, but still inside pixel */
    float farthest_x, farthest_y; /* farthest from origin, but still inside pixel */
    float r_near, r_far, rr_near, rr_far;
    float center_sign, rad_area_1, visibilityNear, delta, delta2;

    /* Dab's center is inside pixel? */
    if( pixel_left<0 && pixel_right>0 &&
        pixel_top<0 && pixel_bottom>0 )
    {
        nearest_x = 0;
        nearest_y = 0;
        r_near = rr_near = 0;
    }
    else
    {
        closest_point_to_line( cs, sn, pixel_center_x, pixel_center_y, &nearest_x, &nearest_y );
        nearest_x = CLAMP( nearest_x, pixel_left, pixel_right );
        nearest_y = CLAMP( nearest_y, pixel_top, pixel_bottom );
        /* XXX: precision of "nearest" values could be improved
         * by intersecting the line that goes from nearest_x/Y to 0
         * with the pixel's borders here, however the improvements
         * would probably not justify the perdormance cost.
         */
        r_near = calculate_r_sample( nearest_x, nearest_y, aspect_ratio, sn, cs );
        rr_near = r_near * one_over_radius2;
    }

    /* out of dab's reach? */
    if( rr_near > 1.0f )
        return rr_near;

    /* check on which side of the dab's line is the pixel center */
    center_sign = sign_point_in_line( pixel_center_x, pixel_center_y, cs, -sn );

    /* radius of a circle with area=1
     *   A = pi * r * r
     *   r = sqrt(1/pi)
     */
    rad_area_1 = sqrtf( 1.0f / M_PI );

    /* center is below dab */
    if( center_sign < 0 )
    {
        farthest_x = nearest_x - sn*rad_area_1;
        farthest_y = nearest_y + cs*rad_area_1;
    }
    /* above dab */
    else
    {
        farthest_x = nearest_x + sn*rad_area_1;
        farthest_y = nearest_y - cs*rad_area_1;
    }

    r_far = calculate_r_sample( farthest_x, farthest_y, aspect_ratio, sn, cs );
    rr_far = r_far * one_over_radius2;

    /* check if we can skip heavier AA */
    if( r_far < r_aa_start )
        return (rr_far+rr_near) * 0.5f;

    /* calculate AA approximate */
    visibilityNear = 1.0f - rr_near;
    delta = rr_far - rr_near;
    delta2 = 1.0f + delta;
    visibilityNear /= delta2;

    return 1.0f - visibilityNear;
}

/* -- Taken from helpers.c -- */
void
rgb_to_spectral (float  r,
                 float  g,
                 float  b,
                 float *spectral_)
{
  float offset = 1.0 - WGM_EPSILON;
  float spec_r[10] = {0};
  float spec_g[10] = {0};
  float spec_b[10] = {0};

  r = r * offset + WGM_EPSILON;
  g = g * offset + WGM_EPSILON;
  b = b * offset + WGM_EPSILON;
  /* upsample rgb to spectral primaries */

  for (int i = 0; i < 10; i++) {
    spec_r[i] = spectral_r_small[i] * r;
  }
  for (int i = 0; i < 10; i++) {
    spec_g[i] = spectral_g_small[i] * g;
  }
  for (int i = 0; i < 10; i++) {
    spec_b[i] = spectral_b_small[i] * b;
  }
  //collapse into one spd
  for (int i = 0; i < 10; i++) {
    spectral_[i] += spec_r[i] + spec_g[i] + spec_b[i];
  }
}

void
spectral_to_rgb (float *spectral,
                 float *rgb_)
{
  float offset = 1.0 - WGM_EPSILON;
  /* We need this tmp. array to allow auto vectorization. */
  float tmp[3] = {0};
  for (int i=0; i<10; i++) {
    tmp[0] += T_MATRIX_SMALL[0][i] * spectral[i];
    tmp[1] += T_MATRIX_SMALL[1][i] * spectral[i];
    tmp[2] += T_MATRIX_SMALL[2][i] * spectral[i];
  }
  for (int i=0; i<3; i++) {
    rgb_[i] = CLAMP((tmp[i] - WGM_EPSILON) / offset, 0.0f, 1.0f);
  }
}

/* -- Taken from brushmode.c -- */
void
get_color_pixels_legacy (float  mask,
                         float *pixel,
                         float *sum_weight,
                         float *sum_r,
                         float *sum_g,
                         float *sum_b,
                         float *sum_a)
{
  *sum_r += mask * pixel[RED];
  *sum_g += mask * pixel[GREEN];
  *sum_b += mask * pixel[BLUE];
  *sum_a += mask * pixel[ALPHA];
  *sum_weight += mask;
}

void
get_color_pixels_accumulate (float    mask,
                             float   *pixel,
                             float   *sum_weight,
                             float   *sum_r,
                             float   *sum_g,
                             float   *sum_b,
                             float   *sum_a,
                             float    paint,
                             uint16_t sample_interval,
                             float    random_sample_rate,
                             uint16_t interval_counter)
{
  const int random_sample_threshold = (int) (random_sample_rate * G_MAXINT);
  float     avg_spectral[10]        = {0};
  float     avg_rgb[3]              = {*sum_r, *sum_g, *sum_b};
  float     spec_rgb[3]             = {0};

  /* V1 Brush Code */
  if (paint < 0.0f)
    {
      get_color_pixels_legacy (mask, pixel, sum_weight,
                               sum_r, sum_g, sum_b, sum_a);
      return;
    }

  rgb_to_spectral (*sum_r, *sum_g, *sum_b, avg_spectral);

  if (interval_counter == 0 || rand() < random_sample_threshold)
    {
      float fac_a, fac_b;
      float a = mask * pixel[ALPHA];
      float alpha_sums = a + *sum_a;

      *sum_weight += mask;

      fac_a = fac_b = 1.0f;
      if (alpha_sums > 0.0f)
        {
          fac_a = a / alpha_sums;
          fac_b = 1.0 - fac_a;
        }

      if (paint > 0.0f && pixel[ALPHA] > 0)
        {
          float spectral[10] = {0};
          rgb_to_spectral (pixel[RED] / pixel[ALPHA],
                           pixel[GREEN] / pixel[ALPHA],
                           pixel[BLUE] / pixel[ALPHA],
                           spectral);

          for (int i = 0; i < 10; i++)
            avg_spectral[i] =
              fastpow (spectral[i], fac_a) * fastpow (avg_spectral[i], fac_b);
        }

      if (paint < 1.0f && pixel[ALPHA] > 0)
        {
          for (int i = 0; i < 3; i++)
            avg_rgb[i] = pixel[i] * fac_a / pixel[ALPHA] + avg_rgb[i] * fac_b;
        }

      *sum_a += a;
    }

  spectral_to_rgb (avg_spectral, spec_rgb);

  *sum_r = spec_rgb[0] * paint + (1.0 - paint) * avg_rgb[0];
  *sum_g = spec_rgb[1] * paint + (1.0 - paint) * avg_rgb[1];
  *sum_b = spec_rgb[2] * paint + (1.0 - paint) * avg_rgb[2];
}

// Fast sigmoid-like function with constant offsets, used to get a
// fairly smooth transition between additive and spectral blending.
float
spectral_blend_factor (float x)
{
  const float ver_fac = 1.65; // vertical compression factor
  const float hor_fac = 8.0f; // horizontal compression factor
  const float hor_offs = 3.0f; // horizontal offset (slightly left of center)
  const float b = x * hor_fac - hor_offs;
  return 0.5 + b / (1 + fabsf(b) * ver_fac);
}

void
draw_dab_pixels_BlendMode_Normal (float *mask,
                                  float *pixel,
                                  float  color_r,
                                  float  color_g,
                                  float  color_b,
                                  float  opacity)
{
  float src_term = *mask * opacity;
  float dst_term = 1.0f - src_term;

  pixel[RED]   = color_r * src_term + pixel[RED] * dst_term;
  pixel[GREEN] = color_g * src_term + pixel[GREEN] * dst_term;
  pixel[BLUE]  = color_b * src_term + pixel[BLUE] * dst_term;
}

void
draw_dab_pixels_BlendMode_Normal_Paint (float *mask,
                                        float *pixel,
                                        float  color_r,
                                        float  color_g,
                                        float  color_b,
                                        float  opacity)
{
  float spectral_a[10]      = {0};
  float spectral_b[10]      = {0};
  float spectral_result[10] = {0};
  float rgb_result[3]       = {0};
  float src_term;
  float dst_term;
  float fac_a;
  float fac_b;

  rgb_to_spectral (color_r, color_g, color_b, spectral_a);
  opacity = MAX (opacity, 1.5);

  src_term = *mask * opacity;
  dst_term = 1.0f - src_term;

  if (pixel[ALPHA] <= 0)
    {
      pixel[ALPHA] = src_term + dst_term * pixel[ALPHA];
      pixel[RED]   = src_term * color_r + dst_term * pixel[RED];
      pixel[GREEN] = src_term * color_g + dst_term * pixel[GREEN];
      pixel[BLUE]  = src_term * color_b + dst_term * pixel[BLUE];
    }
  else
    {
      fac_a = src_term / (src_term + dst_term * pixel[ALPHA]);
      fac_b = 1.0f - fac_a;

      rgb_to_spectral (pixel[RED] / pixel[ALPHA],
                       pixel[GREEN] / pixel[ALPHA],
                       pixel[BLUE] / pixel[ALPHA],
                       spectral_b);

      for (int i = 0; i < 10; i++)
        spectral_result[i] =
          fastpow (spectral_a[i], fac_a) * fastpow (spectral_b[i], fac_b);

      spectral_to_rgb (spectral_result, rgb_result);
      pixel[ALPHA] = src_term + dst_term * pixel[ALPHA];

      pixel[RED]   = (rgb_result[0] * pixel[ALPHA]);
      pixel[GREEN] = (rgb_result[1] * pixel[ALPHA]);
      pixel[BLUE]  = (rgb_result[2] * pixel[ALPHA]);
    }
}

void
draw_dab_pixels_BlendMode_Normal_and_Eraser (float *mask,
                                             float *pixel,
                                             float  color_r,
                                             float  color_g,
                                             float  color_b,
                                             float  color_a,
                                             float  opacity)
{
  float src_term = *mask * opacity;
  float dst_term = 1.0f - src_term;

  src_term *= color_a;

  pixel[RED]   = color_r * src_term + pixel[RED] * dst_term;
  pixel[GREEN] = color_g * src_term + pixel[GREEN] * dst_term;
  pixel[BLUE]  = color_b * src_term + pixel[BLUE] * dst_term;
}

void
draw_dab_pixels_BlendMode_Normal_and_Eraser_Paint (float *mask,
                                                   float *pixel,
                                                   float  color_r,
                                                   float  color_g,
                                                   float  color_b,
                                                   float  color_a,
                                                   float  opacity)
{
  float spectral_a[10]      = {0};
  float spectral_b[10]      = {0};
  float spectral_result[10] = {0};
  float rgb[3]              = {0};
  float rgb_result[3]       = {0};
  float src_term            = *mask * opacity;
  float dst_term            = 1.0f - src_term;
  float src_term2           = src_term * color_a;
  float out_term            = src_term2 + dst_term * pixel[ALPHA];
  float fac_a;
  float fac_b;
  float spectral_factor;
  float additive_factor;

  rgb_to_spectral (color_r, color_g, color_b, spectral_a);

  spectral_factor =
    CLAMP (spectral_blend_factor (pixel[ALPHA]), 0.0f, 1.0f);
  additive_factor = 1.0 - spectral_factor;
  if (additive_factor)
    {
      rgb[0] = src_term2 * color_r + dst_term * pixel[RED];
      rgb[1] = src_term2 * color_g + dst_term * pixel[GREEN];
      rgb[2] = src_term2 * color_b + dst_term * pixel[BLUE];
    }

  if (spectral_factor && pixel[ALPHA] != 0)
    {
      /* Convert straightened tile pixel color to a spectral */
      rgb_to_spectral (pixel[RED] / pixel[ALPHA],
                       pixel[GREEN] / pixel[ALPHA],
                       pixel[BLUE] / pixel[ALPHA],
                       spectral_b);

      fac_a = src_term / (src_term + dst_term * pixel[ALPHA]);
      fac_a *= color_a;
      fac_b = 1.0 - fac_a;

      /* Mix input and tile pixel colors using WGM */
      for (int i = 0; i < 10; i++) {
        spectral_result[i] =
          fastpow (spectral_a[i], fac_a) * fastpow (spectral_b[i], fac_b);
      }

      /* Convert back to RGB */
      spectral_to_rgb (spectral_result, rgb_result);

      for (int i = 0; i < 3; i++)
        rgb[i] = (additive_factor * rgb[i]) +
          (spectral_factor * rgb_result[i] * out_term);
    }

  pixel[ALPHA] = out_term;
  pixel[RED] = rgb[0];
  pixel[GREEN] = rgb[1];
  pixel[BLUE] = rgb[2];
}

void
draw_dab_pixels_BlendMode_LockAlpha (float *mask,
                                     float *pixel,
                                     float  color_r,
                                     float  color_g,
                                     float  color_b,
                                     float  opacity)
{
  float src_term = *mask * opacity;
  float dst_term = 1.0f - src_term;

  src_term /= pixel[ALPHA];

  pixel[RED]   = color_r * src_term + pixel[RED] * dst_term;
  pixel[GREEN] = color_g * src_term + pixel[GREEN] * dst_term;
  pixel[BLUE]  = color_b * src_term + pixel[BLUE] * dst_term;
}

void draw_dab_pixels_BlendMode_LockAlpha_Paint (float *mask,
                                                float    *pixel,
                                                float     color_r,
                                                float     color_g,
                                                float     color_b,
                                                float     opacity)
{
  float spectral_a[10]      = {0};
  float spectral_b[10]      = {0};
  float spectral_result[10] = {0};
  float rgb_result[3]       = {0};
  float src_term;
  float dst_term;
  float fac_a;
  float fac_b;

  rgb_to_spectral (color_r, color_g, color_b, spectral_a);
  opacity = MAX (opacity, 1.5);

  src_term = *mask * opacity;
  dst_term = 1.0f - src_term;
  src_term *= pixel[ALPHA];

  if (pixel[ALPHA] <= 0)
    {
      pixel[RED]   = src_term * color_r + dst_term * pixel[RED];
      pixel[GREEN] = src_term * color_g + dst_term * pixel[GREEN];
      pixel[BLUE]  = src_term * color_b + dst_term * pixel[BLUE];
    }
  else
    {
      fac_a = src_term / (src_term + dst_term * pixel[ALPHA]);
      fac_b = 1.0f - fac_a;

      rgb_to_spectral (pixel[RED] / pixel[ALPHA],
                       pixel[GREEN] / pixel[ALPHA],
                       pixel[BLUE] / pixel[ALPHA],
                       spectral_b);

      for (int i = 0; i < 10; i++)
        spectral_result[i] =
          fastpow (spectral_a[i], fac_a) * fastpow (spectral_b[i], fac_b);

      spectral_to_rgb (spectral_result, rgb_result);
      pixel[ALPHA] = src_term + dst_term * pixel[ALPHA];

      pixel[RED]   = rgb_result[0] * pixel[ALPHA];
      pixel[GREEN] = rgb_result[1] * pixel[ALPHA];
      pixel[BLUE]  = rgb_result[2] * pixel[ALPHA];
    }
}

void
draw_dab_pixels_BlendMode_Posterize (float *mask,
                                     float *pixel,
                                     float  opacity,
                                     float  posterize_num)
{
  float post_r = ROUND (pixel[RED] * posterize_num) / posterize_num;
  float post_g = ROUND (pixel[GREEN] * posterize_num) / posterize_num;
  float post_b = ROUND (pixel[BLUE] * posterize_num) / posterize_num;

  float src_term = *mask * opacity;
  float dst_term = 1 - src_term;

  pixel[RED]   = src_term * post_r + dst_term * pixel[RED];
  pixel[GREEN] = src_term * post_g + dst_term * pixel[GREEN];
  pixel[BLUE]  = src_term * post_b + dst_term * pixel[BLUE];
}


/* -- end mypaint code */

static inline float
calculate_alpha_for_rr (float rr,
                        float hardness,
                        float slope1,
                        float slope2)
{
  if (rr > 1.0f)
    return 0.0f;
  else if (rr <= hardness)
    return 1.0f + rr * slope1;
  else
    return rr * slope2 - slope2;
}

static GeglRectangle
calculate_dab_roi (float x,
                   float y,
                   float radius)
{
  int x0 = floor (x - radius);
  int x1 = ceil (x + radius);
  int y0 = floor (y - radius);
  int y1 = ceil (y + radius);

  return *GEGL_RECTANGLE (x0, y0, x1 - x0, y1 - y0);
}

static void
gimp_mypaint_surface_begin_atomic (MyPaintSurface *base_surface)
{

}

static void
gimp_mypaint_surface_destroy (MyPaintSurface *base_surface)
{
  GimpMybrushSurface *surface = (GimpMybrushSurface *)base_surface;

  g_clear_object (&surface->buffer);
  g_clear_object (&surface->paint_mask);
  g_free (surface);
}

/* MyPaintSurface2 implementation */
static int
gimp_mypaint_surface_draw_dab_2 (MyPaintSurface2 *base_surface,
                                float             x,
                                float             y,
                                float             radius,
                                float             color_r,
                                float             color_g,
                                float             color_b,
                                float             opaque,
                                float             hardness,
                                float             color_a,
                                float             aspect_ratio,
                                float             angle,
                                float             lock_alpha,
                                float             colorize,
                                float             posterize,
                                float             posterize_num,
                                float             paint)
{
  /* Placeholder - eventually implement here */
  GimpMybrushSurface *surface = (GimpMybrushSurface *) base_surface;
  GeglBufferIterator *iter;
  GeglRectangle       dabRect;
  GimpComponentMask   component_mask = surface->component_mask;

  const float one_over_radius2 = 1.0f / (radius * radius);
  const double angle_rad = angle / 360 * 2 * M_PI;
  const float cs = cos(angle_rad);
  const float sn = sin(angle_rad);
  float normal_mode;
  float segment1_slope;
  float segment2_slope;
  float r_aa_start;

  hardness = CLAMP (hardness, 0.0f, 1.0f);
  segment1_slope = -(1.0f / hardness - 1.0f);
  segment2_slope = -hardness / (1.0f - hardness);
  aspect_ratio = MAX (1.0f, aspect_ratio);

  r_aa_start = radius - 1.0f;
  r_aa_start = MAX (r_aa_start, 0);
  r_aa_start = (r_aa_start * r_aa_start) / aspect_ratio;

  posterize = CLAMP (posterize, 0.0f, 1.0f);
  normal_mode = opaque * (1.0f - colorize) * (1.0f - posterize);
  colorize = opaque * colorize;

  /* FIXME: This should use the real matrix values to trim aspect_ratio dabs */
  dabRect = calculate_dab_roi (x, y, radius);
  gegl_rectangle_intersect (&dabRect, &dabRect, gegl_buffer_get_extent (surface->buffer));

  if (dabRect.width <= 0 || dabRect.height <= 0)
    return 0;

  gegl_rectangle_bounding_box (&surface->dirty, &surface->dirty, &dabRect);

  iter = gegl_buffer_iterator_new (surface->buffer, &dabRect, 0,
                                   babl_format ("R'G'B'A float"),
                                   GEGL_BUFFER_READWRITE,
                                   GEGL_ABYSS_NONE, 2);
  if (surface->paint_mask)
    {
      GeglRectangle mask_roi = dabRect;
      mask_roi.x -= surface->paint_mask_x;
      mask_roi.y -= surface->paint_mask_y;
      gegl_buffer_iterator_add (iter, surface->paint_mask, &mask_roi, 0,
                                babl_format ("Y float"),
                                GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
    }

  while (gegl_buffer_iterator_next (iter))
    {
      float *pixel = (float *)iter->items[0].data;
      float *mask;
      int iy, ix;

      if (surface->paint_mask)
        mask = iter->items[1].data;
      else
        mask = NULL;

      for (iy = iter->items[0].roi.y; iy < iter->items[0].roi.y + iter->items[0].roi.height; iy++)
        {
          for (ix = iter->items[0].roi.x; ix < iter->items[0].roi.x +  iter->items[0].roi.width; ix++)
            {
              float rr, base_alpha, alpha, dst_alpha, r, g, b, a;
              if (radius < 3.0f)
                rr = calculate_rr_antialiased (ix, iy, x, y, aspect_ratio, sn, cs, one_over_radius2, r_aa_start);
              else
                rr = calculate_rr (ix, iy, x, y, aspect_ratio, sn, cs, one_over_radius2);
              base_alpha = calculate_alpha_for_rr (rr, hardness, segment1_slope, segment2_slope);
              alpha = base_alpha * normal_mode;
              if (mask)
                alpha *= *mask;
              dst_alpha = pixel[ALPHA];
              /* a = alpha * color_a + dst_alpha * (1.0f - alpha);
               * which converts to: */
              a = alpha * (color_a - dst_alpha) + dst_alpha;
              r = pixel[RED];
              g = pixel[GREEN];
              b = pixel[BLUE];

              /* v1 Brush code */
              if (paint < 1.0f)
                {
                  if (color_a == 1.0f)
                    draw_dab_pixels_BlendMode_Normal (&alpha, pixel,
                                                      color_r, color_g, color_b,
                                                      normal_mode * opaque * (1 - paint));
                  else
                    draw_dab_pixels_BlendMode_Normal_and_Eraser (&alpha, pixel,
                                                                 color_r, color_g,
                                                                 color_b, color_a,
                                                                 normal_mode * opaque * (1 - paint));

                  if (lock_alpha > 0.0f && color_a != 0)
                    draw_dab_pixels_BlendMode_LockAlpha (&alpha, pixel,
                                                         color_r, color_g, color_b,
                                                         lock_alpha * opaque * (1 - colorize) *
                                                         (1 - posterize) * (1 - paint));

                  r = pixel[RED];
                  g = pixel[GREEN];
                  b = pixel[BLUE];
                }

              /* v2 Brush code */
              if (paint > 0.0f)
                {
                  if (color_a == 1.0f)
                    draw_dab_pixels_BlendMode_Normal_Paint (&alpha, pixel,
                                                            color_r, color_g, color_b,
                                                            normal_mode * opaque * paint);
                  else
                    draw_dab_pixels_BlendMode_Normal_and_Eraser_Paint (&alpha, pixel,
                                                                       color_r, color_g,
                                                                       color_b, color_a,
                                                                       normal_mode * opaque * paint);

                  if (lock_alpha > 0.0f && color_a != 0)
                    draw_dab_pixels_BlendMode_LockAlpha_Paint (&alpha, pixel,
                                                               color_r, color_g, color_b,
                                                               lock_alpha * opaque * (1 - colorize) *
                                                               (1 - posterize) * paint);

                  r = pixel[RED];
                  g = pixel[GREEN];
                  b = pixel[BLUE];
                }

              if (colorize > 0.0f && base_alpha > 0.0f)
                {
                  alpha = base_alpha * colorize;
                  a = alpha + dst_alpha - alpha * dst_alpha;
                  if (a > 0.0f)
                    {
                      GimpHSL pixel_hsl, out_hsl;
                      GimpRGB pixel_rgb = {color_r, color_g, color_b};
                      GimpRGB out_rgb   = {r, g, b};
                      float src_term = alpha / a;
                      float dst_term = 1.0f - src_term;

                      gimp_rgb_to_hsl (&pixel_rgb, &pixel_hsl);
                      gimp_rgb_to_hsl (&out_rgb, &out_hsl);

                      out_hsl.h = pixel_hsl.h;
                      out_hsl.s = pixel_hsl.s;
                      gimp_hsl_to_rgb (&out_hsl, &out_rgb);

                      r = (float)out_rgb.r * src_term + r * dst_term;
                      g = (float)out_rgb.g * src_term + g * dst_term;
                      b = (float)out_rgb.b * src_term + b * dst_term;
                    }
                }

              if (posterize > 0.0f)
                draw_dab_pixels_BlendMode_Posterize (&alpha, pixel,
                                                     opaque, posterize_num);


              if (surface->options->no_erasing)
                a = MAX (a, pixel[ALPHA]);

              if (component_mask != GIMP_COMPONENT_MASK_ALL)
                {
                  if (component_mask & GIMP_COMPONENT_MASK_RED)
                    pixel[RED]   = r;
                  if (component_mask & GIMP_COMPONENT_MASK_GREEN)
                    pixel[GREEN] = g;
                  if (component_mask & GIMP_COMPONENT_MASK_BLUE)
                    pixel[BLUE]  = b;
                  if (component_mask & GIMP_COMPONENT_MASK_ALPHA)
                    pixel[ALPHA] = a;
                }
              else
                {
                  pixel[RED]   = r;
                  pixel[GREEN] = g;
                  pixel[BLUE]  = b;
                  pixel[ALPHA] = a;
                }

              pixel += 4;
              if (mask)
                mask += 1;
            }
        }
    }

  return 1;
}

static int
gimp_mypaint_surface_draw_dab_wrapper (MyPaintSurface *surface,
                                       float           x,
                                       float           y,
                                       float           radius,
                                       float           color_r,
                                       float           color_g,
                                       float           color_b,
                                       float           opaque,
                                       float           hardness,
                                       float           color_a,
                                       float           aspect_ratio,
                                       float           angle,
                                       float           lock_alpha,
                                       float           colorize)
{
  const gfloat posterize = 0.0;
  const gfloat posterize_num = 1.0;
  const gfloat pigment = 0.0;

  return gimp_mypaint_surface_draw_dab_2 ((MyPaintSurface2 *) surface, x, y, radius,
                                          color_r, color_g, color_b, opaque, hardness,
                                          color_a, aspect_ratio, angle, lock_alpha,
                                          colorize, posterize, posterize_num,
                                          pigment);
}

static void
gimp_mypaint_surface_get_color_2 (MyPaintSurface2 *base_surface,
                                  float            x,
                                  float            y,
                                  float            radius,
                                  float           *color_r,
                                  float           *color_g,
                                  float           *color_b,
                                  float           *color_a,
                                  float            paint)
{
  GimpMybrushSurface *surface = (GimpMybrushSurface *) base_surface;
  GeglRectangle       dabRect;

  if (radius < 1.0f)
    radius = 1.0f;

  dabRect = calculate_dab_roi (x, y, radius);

  *color_r = 0.0f;
  *color_g = 0.0f;
  *color_b = 0.0f;
  *color_a = 0.0f;

  if (dabRect.width > 0 || dabRect.height > 0)
  {
    const float one_over_radius2 = 1.0f / (radius * radius);
    const int sample_interval = radius <= 2.0f ? 1 : (int)(radius * 7);
    const float random_sample_rate = 1.0f / (7 * radius);
    float sum_weight = 0.0f;
    float sum_r = 0.0f;
    float sum_g = 0.0f;
    float sum_b = 0.0f;
    float sum_a = 0.0f;

    /* Read in clamp mode to avoid transparency bleeding in at the edges */
    GeglBufferIterator *iter = gegl_buffer_iterator_new (surface->buffer, &dabRect, 0,
                                                         babl_format ("R'aG'aB'aA float"),
                                                         GEGL_BUFFER_READ,
                                                         GEGL_ABYSS_CLAMP, 2);
    if (surface->paint_mask)
      {
        GeglRectangle mask_roi = dabRect;
        mask_roi.x -= surface->paint_mask_x;
        mask_roi.y -= surface->paint_mask_y;
        gegl_buffer_iterator_add (iter, surface->paint_mask, &mask_roi, 0,
                                  babl_format ("Y float"),
                                  GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
      }

    while (gegl_buffer_iterator_next (iter))
      {
        float *pixel = (float *)iter->items[0].data;
        float *mask;
        int iy, ix;
        uint16_t interval_counter = 0;

        if (surface->paint_mask)
          mask = iter->items[1].data;
        else
          mask = NULL;

        #ifdef _OPENMP
        #pragma omp parallel for schedule(static)
        #endif
        for (iy = iter->items[0].roi.y; iy < iter->items[0].roi.y + iter->items[0].roi.height; iy++)
          {
            float yy = (iy + 0.5f - y);
            for (ix = iter->items[0].roi.x; ix < iter->items[0].roi.x +  iter->items[0].roi.width; ix++)
              {
                /* pixel_weight == a standard dab with hardness = 0.5, aspect_ratio = 1.0, and angle = 0.0 */
                float xx = (ix + 0.5f - x);
                float rr = (yy * yy + xx * xx) * one_over_radius2;
                float pixel_weight = 0.0f;
                if (rr <= 1.0f)
                  pixel_weight = 1.0f - rr;
                if (mask)
                  pixel_weight *= *mask;

                #ifdef _OPENMP
                #pragma omp critical
                #endif
                get_color_pixels_accumulate (pixel_weight, pixel, &sum_weight,
                                             &sum_r, &sum_g, &sum_b, &sum_a, paint,
                                             sample_interval, random_sample_rate,
                                             interval_counter);

                interval_counter = (interval_counter + 1) % sample_interval;

                pixel += 4;
                if (mask)
                  mask += 1;
              }
          }
      }

    if (sum_a > 0.0f && sum_weight > 0.0f)
      {
        float demul;
        sum_a /= sum_weight;

        if (paint < 0.0f)
          {
            sum_r /= sum_weight;
            sum_g /= sum_weight;
            sum_b /= sum_weight;
          }

        demul = paint < 0.0 ? sum_a : 1.0;

        /* FIXME: Clamping is wrong because GEGL allows alpha > 1, this should probably re-multipy things */
        *color_r = CLAMP(sum_r / demul, 0.0f, 1.0f);
        *color_g = CLAMP(sum_g / demul, 0.0f, 1.0f);
        *color_b = CLAMP(sum_b / demul, 0.0f, 1.0f);
        *color_a = CLAMP(sum_a, 0.0f, 1.0f);
      }
  }
}

static void
gimp_mypaint_surface_get_color_wrapper (MyPaintSurface *surface,
                                        float           x,
                                        float           y,
                                        float           radius,
                                        float          *color_r,
                                        float          *color_g,
                                        float          *color_b,
                                        float          *color_a)
{
  return gimp_mypaint_surface_get_color_2 ((MyPaintSurface2 *) surface, x, y, radius,
                                           color_r, color_g, color_b, color_a,
                                           -1.0);
}


static void
gimp_mypaint_surface_end_atomic_2 (MyPaintSurface2    *base_surface,
                                   MyPaintRectangles  *roi)
{
  GimpMybrushSurface *surface = (GimpMybrushSurface *) base_surface;

  if (roi)
    {
      const gint roi_rects = roi->num_rectangles;

      for (gint i = 0; i < roi_rects; i++)
        {
          roi->rectangles[i].x         = surface->dirty.x;
          roi->rectangles[i].y         = surface->dirty.y;
          roi->rectangles[i].width     = surface->dirty.width;
          roi->rectangles[i].height    = surface->dirty.height;
          surface->dirty = *GEGL_RECTANGLE (0, 0, 0, 0);
        }
    }
}

static void
gimp_mypaint_surface_end_atomic_wrapper (MyPaintSurface   *surface,
                                         MyPaintRectangle *roi)
{
  MyPaintRectangles rois = {1, roi};
  gimp_mypaint_surface_end_atomic_2 ((MyPaintSurface2 *) surface, &rois);
}

GimpMybrushSurface *
gimp_mypaint_surface_new (GeglBuffer         *buffer,
                          GimpComponentMask   component_mask,
                          GeglBuffer         *paint_mask,
                          gint                paint_mask_x,
                          gint                paint_mask_y,
                          GimpMybrushOptions *options)
{
  GimpMybrushSurface *surface = g_malloc0 (sizeof (GimpMybrushSurface));
  MyPaintSurface2    *s;

  mypaint_surface_init (&surface->surface.parent);
  s = &surface->surface;

  s->get_color_pigment    = gimp_mypaint_surface_get_color_2;
  s->draw_dab_pigment     = gimp_mypaint_surface_draw_dab_2;
  s->parent.begin_atomic  = gimp_mypaint_surface_begin_atomic;
  s->end_atomic_multi     = gimp_mypaint_surface_end_atomic_2;

  s->parent.draw_dab      = gimp_mypaint_surface_draw_dab_wrapper;
  s->parent.get_color     = gimp_mypaint_surface_get_color_wrapper;
  s->parent.end_atomic    = gimp_mypaint_surface_end_atomic_wrapper;

  s->parent.destroy       = gimp_mypaint_surface_destroy;

  surface->component_mask = component_mask;
  surface->options        = options;
  surface->buffer         = g_object_ref (buffer);
  if (paint_mask)
    surface->paint_mask   = g_object_ref (paint_mask);

  surface->paint_mask_x   = paint_mask_x;
  surface->paint_mask_y   = paint_mask_y;
  surface->dirty          = *GEGL_RECTANGLE (0, 0, 0, 0);

  return surface;
}
