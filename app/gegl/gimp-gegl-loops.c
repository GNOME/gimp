/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-gegl-loops.c
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <lcms2.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"

#include "gimp-gegl-types.h"

#include "gimp-babl.h"
#include "gimp-gegl-loops.h"

#include "core/gimpprogress.h"


void
gimp_gegl_convolve (GeglBuffer          *src_buffer,
                    const GeglRectangle *src_rect,
                    GeglBuffer          *dest_buffer,
                    const GeglRectangle *dest_rect,
                    const gfloat        *kernel,
                    gint                 kernel_size,
                    gdouble              divisor,
                    GimpConvolutionType  mode,
                    gboolean             alpha_weighting)
{
  GeglBufferIterator *dest_iter;
  gfloat             *src;
  gint                src_rowstride;

  const Babl         *src_format;
  const Babl         *dest_format;
  gint                src_components;
  gint                dest_components;

  src_format = gegl_buffer_get_format (src_buffer);

  if (babl_format_is_palette (src_format))
    src_format = gimp_babl_format (GIMP_RGB,
                                   GIMP_PRECISION_FLOAT_LINEAR,
                                   babl_format_has_alpha (src_format));
  else
    src_format = gimp_babl_format (gimp_babl_format_get_base_type (src_format),
                                   GIMP_PRECISION_FLOAT_LINEAR,
                                   babl_format_has_alpha (src_format));

  dest_format = gegl_buffer_get_format (dest_buffer);

  if (babl_format_is_palette (dest_format))
    dest_format = gimp_babl_format (GIMP_RGB,
                                    GIMP_PRECISION_FLOAT_LINEAR,
                                    babl_format_has_alpha (dest_format));
  else
    dest_format = gimp_babl_format (gimp_babl_format_get_base_type (dest_format),
                                    GIMP_PRECISION_FLOAT_LINEAR,
                                    babl_format_has_alpha (dest_format));

  src_components  = babl_format_get_n_components (src_format);
  dest_components = babl_format_get_n_components (dest_format);

  /* Set up dest iterator */
  dest_iter = gegl_buffer_iterator_new (dest_buffer, dest_rect, 0, dest_format,
                                        GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  /* Get source pixel data */
  src_rowstride = src_components * src_rect->width;
  src = g_malloc (sizeof(gfloat) * src_rowstride * src_rect->height);
  gegl_buffer_get (src_buffer, src_rect, 1.0, src_format, src,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (dest_iter))
    {
      /*  Convolve the src image using the convolution kernel, writing
       *  to dest Convolve is not tile-enabled--use accordingly
       */
      gfloat       *dest        = dest_iter->data[0];
      const gint    components  = src_components;
      const gint    a_component = components - 1;
      const gint    margin      = kernel_size / 2;
      const gint    x1          = 0;
      const gint    y1          = 0;
      const gint    x2          = src_rect->width  - 1;
      const gint    y2          = src_rect->height - 1;
      const gint    dest_x1     = dest_iter->roi[0].x;
      const gint    dest_y1     = dest_iter->roi[0].y;
      const gint    dest_x2     = dest_iter->roi[0].x + dest_iter->roi[0].width;
      const gint    dest_y2     = dest_iter->roi[0].y + dest_iter->roi[0].height;
      gint          x, y;
      gfloat        offset;

      /*  If the mode is NEGATIVE_CONVOL, the offset should be 128  */
      if (mode == GIMP_NEGATIVE_CONVOL)
        {
          offset = 0.5;
          mode = GIMP_NORMAL_CONVOL;
        }
      else
        {
          offset = 0.0;
        }

      for (y = dest_y1; y < dest_y2; y++)
        {
          gfloat *d = dest;

          if (alpha_weighting)
            {
              for (x = dest_x1; x < dest_x2; x++)
                {
                  const gfloat *m                = kernel;
                  gdouble       total[4]         = { 0.0, 0.0, 0.0, 0.0 };
                  gdouble       weighted_divisor = 0.0;
                  gint          i, j, b;

                  for (j = y - margin; j <= y + margin; j++)
                    {
                      for (i = x - margin; i <= x + margin; i++, m++)
                        {
                          gint          xx = CLAMP (i, x1, x2);
                          gint          yy = CLAMP (j, y1, y2);
                          const gfloat *s  = src + yy * src_rowstride + xx * components;
                          const gfloat  a  = s[a_component];

                          if (a)
                            {
                              gdouble mult_alpha = *m * a;

                              weighted_divisor += mult_alpha;

                              for (b = 0; b < a_component; b++)
                                total[b] += mult_alpha * s[b];

                              total[a_component] += mult_alpha;
                            }
                        }
                    }

                  if (weighted_divisor == 0.0)
                    weighted_divisor = divisor;

                  for (b = 0; b < a_component; b++)
                    total[b] /= weighted_divisor;

                  total[a_component] /= divisor;

                  for (b = 0; b < components; b++)
                    {
                      total[b] += offset;

                      if (mode != GIMP_NORMAL_CONVOL && total[b] < 0.0)
                        total[b] = - total[b];

                      *d++ = CLAMP (total[b], 0.0, 1.0);
                    }
                }
            }
          else
            {
              for (x = dest_x1; x < dest_x2; x++)
                {
                  const gfloat *m        = kernel;
                  gdouble       total[4] = { 0.0, 0.0, 0.0, 0.0 };
                  gint          i, j, b;

                  for (j = y - margin; j <= y + margin; j++)
                    {
                      for (i = x - margin; i <= x + margin; i++, m++)
                        {
                          gint          xx = CLAMP (i, x1, x2);
                          gint          yy = CLAMP (j, y1, y2);
                          const gfloat *s  = src + yy * src_rowstride + xx * components;

                          for (b = 0; b < components; b++)
                            total[b] += *m * s[b];
                        }
                    }

                  for (b = 0; b < components; b++)
                    {
                      total[b] = total[b] / divisor + offset;

                      if (mode != GIMP_NORMAL_CONVOL && total[b] < 0.0)
                        total[b] = - total[b];

                      *d++ = CLAMP (total[b], 0.0, 1.0);
                    }
                }
            }

          dest += dest_iter->roi[0].width * dest_components;
        }
    }

  g_free (src);
}

void
gimp_gegl_dodgeburn (GeglBuffer          *src_buffer,
                     const GeglRectangle *src_rect,
                     GeglBuffer          *dest_buffer,
                     const GeglRectangle *dest_rect,
                     gdouble              exposure,
                     GimpDodgeBurnType    type,
                     GimpTransferMode     mode)
{
  GeglBufferIterator *iter;

  if (type == GIMP_DODGE_BURN_TYPE_BURN)
    exposure = -exposure;

  iter = gegl_buffer_iterator_new (src_buffer, src_rect, 0,
                                   babl_format ("R'G'B'A float"),
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, dest_buffer, dest_rect, 0,
                            babl_format ("R'G'B'A float"),
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  switch (mode)
    {
      gfloat factor;

    case GIMP_TRANSFER_HIGHLIGHTS:
      factor = 1.0 + exposure * (0.333333);

      while (gegl_buffer_iterator_next (iter))
        {
          gfloat *src   = iter->data[0];
          gfloat *dest  = iter->data[1];
          gint    count = iter->length;

          while (count--)
            {
              *dest++ = *src++ * factor;
              *dest++ = *src++ * factor;
              *dest++ = *src++ * factor;

              *dest++ = *src++;
            }
        }
      break;

    case GIMP_TRANSFER_MIDTONES:
      if (exposure < 0)
        factor = 1.0 - exposure * (0.333333);
      else
        factor = 1.0 / (1.0 + exposure);

      while (gegl_buffer_iterator_next (iter))
        {
          gfloat *src   = iter->data[0];
          gfloat *dest  = iter->data[1];
          gint    count = iter->length;

          while (count--)
            {
              *dest++ = pow (*src++, factor);
              *dest++ = pow (*src++, factor);
              *dest++ = pow (*src++, factor);

              *dest++ = *src++;
            }
        }
      break;

    case GIMP_TRANSFER_SHADOWS:
      if (exposure >= 0)
        factor = 0.333333 * exposure;
      else
        factor = -0.333333 * exposure;

      while (gegl_buffer_iterator_next (iter))
        {
          gfloat *src   = iter->data[0];
          gfloat *dest  = iter->data[1];
          gint    count = iter->length;

          while (count--)
            {
              if (exposure >= 0)
                {
                  gfloat s;

                  s = *src++; *dest++ = factor + s - factor * s;
                  s = *src++; *dest++ = factor + s - factor * s;
                  s = *src++; *dest++ = factor + s - factor * s;
                }
              else
                {
                  gfloat s;

                  s = *src++;
                  if (s < factor)
                    *dest++ = 0;
                  else /* factor <= value <=1 */
                    *dest++ = (s - factor) / (1.0 - factor);

                  s = *src++;
                  if (s < factor)
                    *dest++ = 0;
                  else /* factor <= value <=1 */
                    *dest++ = (s - factor) / (1.0 - factor);

                  s = *src++;
                  if (s < factor)
                    *dest++ = 0;
                  else /* factor <= value <=1 */
                    *dest++ = (s - factor) / (1.0 - factor);
                }

              *dest++ = *src++;
           }
        }
      break;
    }
}

/*
 * blend_pixels patched 8-24-05 to fix bug #163721.  Note that this change
 * causes the function to treat src1 and src2 asymmetrically.  This gives the
 * right behavior for the smudge tool, which is the only user of this function
 * at the time of patching.  If you want to use the function for something
 * else, caveat emptor.
 */
void
gimp_gegl_smudge_blend (GeglBuffer          *top_buffer,
                        const GeglRectangle *top_rect,
                        GeglBuffer          *bottom_buffer,
                        const GeglRectangle *bottom_rect,
                        GeglBuffer          *dest_buffer,
                        const GeglRectangle *dest_rect,
                        gdouble              blend)
{
  GeglBufferIterator *iter;

  iter = gegl_buffer_iterator_new (top_buffer, top_rect, 0,
                                   babl_format ("RGBA float"),
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, bottom_buffer, bottom_rect, 0,
                            babl_format ("RGBA float"),
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, dest_buffer, dest_rect, 0,
                            babl_format ("RGBA float"),
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      const gfloat *top    = iter->data[0];
      const gfloat *bottom = iter->data[1];
      gfloat       *dest   = iter->data[2];
      gint          count  = iter->length;
      const gfloat  blend1 = 1.0 - blend;
      const gfloat  blend2 = blend;

      while (count--)
        {
          const gfloat a1 = blend1 * bottom[3];
          const gfloat a2 = blend2 * top[3];
          const gfloat a  = a1 + a2;
          gint         b;

          if (a == 0)
            {
              for (b = 0; b < 4; b++)
                dest[b] = 0;
            }
          else
            {
              for (b = 0; b < 3; b++)
                dest[b] =
                  bottom[b] + (bottom[b] * a1 + top[b] * a2 - a * bottom[b]) / a;

              dest[3] = a;
            }

          top    += 4;
          bottom += 4;
          dest   += 4;
        }
    }
}

void
gimp_gegl_apply_mask (GeglBuffer          *mask_buffer,
                      const GeglRectangle *mask_rect,
                      GeglBuffer          *dest_buffer,
                      const GeglRectangle *dest_rect,
                      gdouble              opacity)
{
  GeglBufferIterator *iter;

  iter = gegl_buffer_iterator_new (mask_buffer, mask_rect, 0,
                                   babl_format ("Y float"),
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, dest_buffer, dest_rect, 0,
                            babl_format ("RGBA float"),
                            GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      const gfloat *mask  = iter->data[0];
      gfloat       *dest  = iter->data[1];
      gint          count = iter->length;

      while (count--)
        {
          dest[3] *= *mask * opacity;

          mask += 1;
          dest += 4;
        }
    }
}

void
gimp_gegl_combine_mask (GeglBuffer          *mask_buffer,
                        const GeglRectangle *mask_rect,
                        GeglBuffer          *dest_buffer,
                        const GeglRectangle *dest_rect,
                        gdouble              opacity)
{
  GeglBufferIterator *iter;

  iter = gegl_buffer_iterator_new (mask_buffer, mask_rect, 0,
                                   babl_format ("Y float"),
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, dest_buffer, dest_rect, 0,
                            babl_format ("Y float"),
                            GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      const gfloat *mask  = iter->data[0];
      gfloat       *dest  = iter->data[1];
      gint          count = iter->length;

      while (count--)
        {
          *dest *= *mask * opacity;

          mask += 1;
          dest += 1;
        }
    }
}

void
gimp_gegl_combine_mask_weird (GeglBuffer          *mask_buffer,
                              const GeglRectangle *mask_rect,
                              GeglBuffer          *dest_buffer,
                              const GeglRectangle *dest_rect,
                              gdouble              opacity,
                              gboolean             stipple)
{
  GeglBufferIterator *iter;

  iter = gegl_buffer_iterator_new (mask_buffer, mask_rect, 0,
                                   babl_format ("Y float"),
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, dest_buffer, dest_rect, 0,
                            babl_format ("Y float"),
                            GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      const gfloat *mask  = iter->data[0];
      gfloat       *dest  = iter->data[1];
      gint          count = iter->length;

      if (stipple)
        {
          while (count--)
            {
              dest[0] += (1.0 - dest[0]) * *mask * opacity;

              mask += 1;
              dest += 1;
            }
        }
      else
        {
          while (count--)
            {
              if (opacity > dest[0])
                dest[0] += (opacity - dest[0]) * *mask * opacity;

              mask += 1;
              dest += 1;
            }
        }
    }
}

void
gimp_gegl_replace (GeglBuffer          *top_buffer,
                   const GeglRectangle *top_rect,
                   GeglBuffer          *bottom_buffer,
                   const GeglRectangle *bottom_rect,
                   GeglBuffer          *mask_buffer,
                   const GeglRectangle *mask_rect,
                   GeglBuffer          *dest_buffer,
                   const GeglRectangle *dest_rect,
                   gdouble              opacity,
                   const gboolean      *affect)
{
  GeglBufferIterator *iter;

  iter = gegl_buffer_iterator_new (top_buffer, top_rect, 0,
                                   babl_format ("RGBA float"),
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, bottom_buffer, bottom_rect, 0,
                            babl_format ("RGBA float"),
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, mask_buffer, mask_rect, 0,
                            babl_format ("Y float"),
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, dest_buffer, dest_rect, 0,
                            babl_format ("RGBA float"),
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      const gfloat *top    = iter->data[0];
      const gfloat *bottom = iter->data[1];
      const gfloat *mask   = iter->data[2];
      gfloat       *dest   = iter->data[3];
      gint          count  = iter->length;

      while (count--)
        {
          gint    b;
          gdouble mask_val = *mask * opacity;

          /* calculate new alpha first. */
          gfloat   s1_a  = bottom[3];
          gfloat   s2_a  = top[3];
          gdouble  a_val = s1_a + mask_val * (s2_a - s1_a);

          if (a_val == 0.0)
            {
              /* In any case, write out versions of the blending
               * function that result when combinations of s1_a, s2_a,
               * and mask_val --> 0 (or mask_val -->1)
               */

              /* 1: s1_a, s2_a, AND mask_val all approach 0+: */
              /* 2: s1_a AND s2_a both approach 0+, regardless of mask_val: */
              if (s1_a + s2_a == 0.0)
                {
                  for (b = 0; b < 3; b++)
                    {
                      gfloat new_val;

                      new_val = bottom[b] + mask_val * (top[b] - bottom[b]);

                      dest[b] = affect[b] ? new_val : bottom[b];
                    }
                }

              /* 3: mask_val AND s1_a both approach 0+, regardless of s2_a  */
              else if (s1_a + mask_val == 0.0)
                {
                  for (b = 0; b < 3; b++)
                    dest[b] = bottom[b];
                }

              /* 4: mask_val -->1 AND s2_a -->0, regardless of s1_a */
              else if (1.0 - mask_val + s2_a == 0.0)
                {
                  for (b = 0; b < 3; b++)
                    dest[b] = affect[b] ? top[b] : bottom[b];
                }
            }
          else
            {
              gdouble a_recip = 1.0 / a_val;

              /* possible optimization: fold a_recip into s1_a and s2_a */
              for (b = 0; b < 3; b++)
                {
                  gfloat new_val = a_recip * (bottom[b] * s1_a + mask_val *
                                              (top[b] * s2_a - bottom[b] * s1_a));
                  dest[b] = affect[b] ? new_val : bottom[b];
                }
            }

          dest[3] = affect[3] ? a_val : s1_a;

          top    += 4;
          bottom += 4;
          mask   += 1;
          dest   += 4;
        }
    }
}

void
gimp_gegl_index_to_mask (GeglBuffer          *indexed_buffer,
                         const GeglRectangle *indexed_rect,
                         const Babl          *indexed_format,
                         GeglBuffer          *mask_buffer,
                         const GeglRectangle *mask_rect,
                         gint                 index)
{
  GeglBufferIterator *iter;

  iter = gegl_buffer_iterator_new (indexed_buffer, indexed_rect, 0,
                                   indexed_format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, mask_buffer, mask_rect, 0,
                            babl_format ("Y float"),
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      const guchar *indexed = iter->data[0];
      gfloat       *mask    = iter->data[1];
      gint          count   = iter->length;

      while (count--)
        {
          if (*indexed == index)
            *mask = 1.0;
          else
            *mask = 0.0;

          indexed++;
          mask++;
        }
    }
}

static gboolean
gimp_color_profile_can_gegl_copy (GimpColorProfile *src_profile,
                                  GimpColorProfile *dest_profile)
{
  static GimpColorProfile *srgb_profile        = NULL;
  static GimpColorProfile *srgb_linear_profile = NULL;
  static GimpColorProfile *gray_profile        = NULL;
  static GimpColorProfile *gray_linear_profile = NULL;

  if (gimp_color_profile_is_equal (src_profile, dest_profile))
    return TRUE;

  if (! srgb_profile)
    {
      srgb_profile        = gimp_color_profile_new_rgb_srgb ();
      srgb_linear_profile = gimp_color_profile_new_rgb_srgb_linear ();
      gray_profile        = gimp_color_profile_new_d65_gray_srgb_trc ();
      gray_linear_profile = gimp_color_profile_new_d65_gray_linear ();
    }

  if ((gimp_color_profile_is_equal (src_profile, srgb_profile)        ||
       gimp_color_profile_is_equal (src_profile, srgb_linear_profile) ||
       gimp_color_profile_is_equal (src_profile, gray_profile)        ||
       gimp_color_profile_is_equal (src_profile, gray_linear_profile))
      &&
      (gimp_color_profile_is_equal (dest_profile, srgb_profile)        ||
       gimp_color_profile_is_equal (dest_profile, srgb_linear_profile) ||
       gimp_color_profile_is_equal (dest_profile, gray_profile)        ||
       gimp_color_profile_is_equal (dest_profile, gray_linear_profile)))
    {
      return TRUE;
    }

  return FALSE;
}

void
gimp_gegl_convert_color_profile (GeglBuffer               *src_buffer,
                                 const GeglRectangle      *src_rect,
                                 GimpColorProfile         *src_profile,
                                 GeglBuffer               *dest_buffer,
                                 const GeglRectangle      *dest_rect,
                                 GimpColorProfile         *dest_profile,
                                 GimpColorRenderingIntent  intent,
                                 gboolean                  bpc,
                                 GimpProgress             *progress)
{
  const Babl       *src_format;
  const Babl       *dest_format;
  cmsHPROFILE       src_lcms;
  cmsHPROFILE       dest_lcms;
  cmsUInt32Number   lcms_src_format;
  cmsUInt32Number   lcms_dest_format;
  cmsUInt32Number   flags;
  cmsHTRANSFORM     transform;

  src_format  = gegl_buffer_get_format (src_buffer);
  dest_format = gegl_buffer_get_format (dest_buffer);

  if (gimp_color_profile_can_gegl_copy (src_profile, dest_profile))
    {
      gegl_buffer_copy (src_buffer,  src_rect, GEGL_ABYSS_NONE,
                        dest_buffer, dest_rect);
      return;
    }

  src_lcms  = gimp_color_profile_get_lcms_profile (src_profile);
  dest_lcms = gimp_color_profile_get_lcms_profile (dest_profile);

  src_format  = gimp_color_profile_get_format (src_format,  &lcms_src_format);
  dest_format = gimp_color_profile_get_format (dest_format, &lcms_dest_format);

  flags = cmsFLAGS_NOOPTIMIZE;

  if (bpc)
    flags |= cmsFLAGS_BLACKPOINTCOMPENSATION;

  transform = cmsCreateTransform (src_lcms,  lcms_src_format,
                                  dest_lcms, lcms_dest_format,
                                  intent, flags);

  if (transform)
    {
      gimp_gegl_convert_color_transform (src_buffer,  src_rect,  src_format,
                                         dest_buffer, dest_rect, dest_format,
                                         transform, progress);

      cmsDeleteTransform (transform);
    }
  else
    {
      /* FIXME: no idea if this ever happens */
      gegl_buffer_copy (src_buffer,  src_rect, GEGL_ABYSS_NONE,
                        dest_buffer, dest_rect);

      if (progress)
        gimp_progress_set_value (progress, 1.0);
    }
}

void
gimp_gegl_convert_color_transform (GeglBuffer          *src_buffer,
                                   const GeglRectangle *src_rect,
                                   const Babl          *src_format,
                                   GeglBuffer          *dest_buffer,
                                   const GeglRectangle *dest_rect,
                                   const Babl          *dest_format,
                                   GimpColorTransform   transform,
                                   GimpProgress        *progress)
{
  GeglBufferIterator *iter;
  gboolean            has_alpha;
  gint                total_pixels;
  gint                done_pixels = 0;

  if (src_rect)
    {
      total_pixels = src_rect->width * src_rect->height;
    }
  else
    {
      total_pixels = (gegl_buffer_get_width  (src_buffer) *
                      gegl_buffer_get_height (src_buffer));
    }

  has_alpha = babl_format_has_alpha (dest_format);

  /* make sure the alpha channel is copied too, lcms doesn't copy it */
  if (has_alpha)
    gegl_buffer_copy (src_buffer,  src_rect, GEGL_ABYSS_NONE,
                      dest_buffer, dest_rect);

  iter = gegl_buffer_iterator_new (src_buffer, src_rect, 0,
                                   src_format,
                                   GEGL_ACCESS_READ,
                                   GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, dest_buffer, dest_rect, 0,
                            dest_format,
                            /* use READWRITE for alpha surfaces
                             * because we must use the alpha channel
                             * that is already copied, see above
                             */
                            has_alpha ?
                            GEGL_ACCESS_READWRITE: GEGL_ACCESS_WRITE,
                            GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      cmsDoTransform (transform,
                      iter->data[0], iter->data[1], iter->length);

      done_pixels += iter->roi[0].width * iter->roi[0].height;

      if (progress)
        gimp_progress_set_value (progress,
                                 (gdouble) done_pixels /
                                 (gdouble) total_pixels);
    }

  if (progress)
    gimp_progress_set_value (progress, 1.0);
}
