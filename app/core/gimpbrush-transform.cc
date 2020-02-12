/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrush-transform.c
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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpmath/gimpmath.h"

extern "C"
{

#include "core-types.h"

#include "gegl/gimp-gegl-loops.h"

#include "gimpbrush.h"
#include "gimpbrush-mipmap.h"
#include "gimpbrush-transform.h"
#include "gimptempbuf.h"


#define PIXELS_PER_THREAD \
  (/* each thread costs as much as */ 64.0 * 64.0 /* pixels */)


/*  local function prototypes  */

static void    gimp_brush_transform_bounding_box           (const GimpTempBuf *temp_buf,
                                                            const GimpMatrix3 *matrix,
                                                            gint              *x,
                                                            gint              *y,
                                                            gint              *width,
                                                            gint              *height);

static void    gimp_brush_transform_blur                   (GimpTempBuf       *buf,
                                                            gint               r);
static gint    gimp_brush_transform_blur_radius            (gint               height,
                                                            gint               width,
                                                            gdouble            hardness);
static void    gimp_brush_transform_adjust_hardness_matrix (gdouble            width,
                                                            gdouble            height,
                                                            gdouble            blur_radius,
                                                            GimpMatrix3       *matrix);


/*  public functions  */

void
gimp_brush_real_transform_size (GimpBrush *brush,
                                gdouble    scale,
                                gdouble    aspect_ratio,
                                gdouble    angle,
                                gboolean   reflect,
                                gint      *width,
                                gint      *height)
{
  const GimpTempBuf *source;
  GimpMatrix3        matrix;
  gdouble            scale_x, scale_y;
  gint               x, y;

  gimp_brush_transform_get_scale (scale, aspect_ratio,
                                  &scale_x, &scale_y);

  source = gimp_brush_mipmap_get_mask (brush, &scale_x, &scale_y);

  gimp_brush_transform_matrix (gimp_temp_buf_get_width  (source),
                               gimp_temp_buf_get_height (source),
                               scale_x, scale_y, angle, reflect, &matrix);

  gimp_brush_transform_bounding_box (source, &matrix, &x, &y, width, height);
}

/*
 * Transforms the brush mask with bilinear interpolation.
 *
 * Rather than calculating the inverse transform for each point in the
 * transformed image, this algorithm uses the inverse transformed
 * corner points of the destination image to work out the starting
 * position in the source image and the U and V deltas in the source
 * image space.  It then uses a scan-line approach, looping through
 * rows and columns in the transformed (destination) image while
 * walking along the corresponding rows and columns (named U and V) in
 * the source image.
 *
 * The horizontal in destination space (transform result) is reverse
 * transformed into source image space to get U.  The vertical in
 * destination space (transform result) is reverse transformed into
 * source image space to get V.
 *
 * The strength of this particular algorithm is that calculation work
 * should depend more upon the final transformed brush size rather
 * than the input brush size.
 *
 * There are no floating point calculations in the inner loop for speed.
 *
 * Some variables end with the suffix _i to indicate they have been
 * premultiplied by int_multiple
 */
GimpTempBuf *
gimp_brush_real_transform_mask (GimpBrush *brush,
                                gdouble    scale,
                                gdouble    aspect_ratio,
                                gdouble    angle,
                                gboolean   reflect,
                                gdouble    hardness)
{
  GimpTempBuf       *result;
  const GimpTempBuf *source;
  const guchar      *src;
  GimpMatrix3        matrix;
  gdouble            scale_x, scale_y;
  gint               src_width;
  gint               src_height;
  gint               src_width_minus_one;
  gint               src_height_minus_one;
  gint               dest_width;
  gint               dest_height;
  gint               blur_radius;
  gint               x, y;
  gdouble            b_lx, b_rx, t_lx, t_rx;
  gdouble            b_ly, b_ry, t_ly, t_ry;
  gdouble            src_tl_to_tr_delta_x;
  gdouble            src_tl_to_tr_delta_y;
  gdouble            src_tl_to_bl_delta_x;
  gdouble            src_tl_to_bl_delta_y;
  gint               src_walk_ux_i;
  gint               src_walk_uy_i;
  gint               src_walk_vx_i;
  gint               src_walk_vy_i;
  gint               src_x_min_i;
  gint               src_y_min_i;
  gint               src_x_max_i;
  gint               src_y_max_i;

  /*
   * tl, tr etc are used because it is easier to visualize top left,
   * top right etc corners of the forward transformed source image
   * rectangle.
   */
  const gint fraction_bits = 12;
  const gint int_multiple  = pow (2, fraction_bits);

  /* In inner loop's bilinear calculation, two numbers that were each
   * previously multiplied by int_multiple are multiplied together.
   * To get back the right result, the multiplication result must be
   * divided *twice* by 2^fraction_bits, equivalent to bit shift right
   * by 2 * fraction_bits
   */
  const gint recovery_bits = 2 * fraction_bits;

  /*
   * example: suppose fraction_bits = 9
   * a 9-bit mask looks like this: 0001 1111 1111
   * and is given by:  2^fraction_bits - 1
   * demonstration:
   * 2^0     = 0000 0000 0001
   * 2^1     = 0000 0000 0010
   * :
   * 2^8     = 0001 0000 0000
   * 2^9     = 0010 0000 0000
   * 2^9 - 1 = 0001 1111 1111
   */
  const guint fraction_bitmask = pow(2, fraction_bits) - 1 ;

  gimp_brush_transform_get_scale (scale, aspect_ratio,
                                  &scale_x, &scale_y);

  source = gimp_brush_mipmap_get_mask (brush, &scale_x, &scale_y);

  src_width  = gimp_temp_buf_get_width  (source);
  src_height = gimp_temp_buf_get_height (source);

  gimp_brush_transform_matrix (src_width, src_height,
                               scale_x, scale_y, angle, reflect, &matrix);

  if (gimp_matrix3_is_identity (&matrix) && hardness == 1.0)
    return gimp_temp_buf_copy (source);

  src_width_minus_one  = src_width  - 1;
  src_height_minus_one = src_height - 1;

  gimp_brush_transform_bounding_box (source, &matrix,
                                     &x, &y, &dest_width, &dest_height);

  blur_radius = 0;

  if (hardness < 1.0)
    {
      GimpMatrix3 unrotated_matrix;
      gint        unrotated_x;
      gint        unrotated_y;
      gint        unrotated_dest_width;
      gint        unrotated_dest_height;

      gimp_brush_transform_matrix (src_width, src_height,
                                   scale_x, scale_y, 0.0, FALSE,
                                   &unrotated_matrix);

      gimp_brush_transform_bounding_box (source, &unrotated_matrix,
                                         &unrotated_x, &unrotated_y,
                                         &unrotated_dest_width,
                                         &unrotated_dest_height);

      blur_radius = gimp_brush_transform_blur_radius (unrotated_dest_width,
                                                      unrotated_dest_height,
                                                      hardness);

      gimp_brush_transform_adjust_hardness_matrix (dest_width, dest_height,
                                                   blur_radius, &matrix);
    }

  gimp_matrix3_translate (&matrix, -x, -y);
  gimp_matrix3_invert (&matrix);
  gimp_matrix3_translate (&matrix, -0.5, -0.5);

  result = gimp_temp_buf_new (dest_width, dest_height,
                              gimp_temp_buf_get_format (source));

  src = gimp_temp_buf_get_data (source);

  /* prevent disappearance of 1x1 pixel brush at some rotations when
     scaling < 1 */
  /*
  if (src_width == 1 && src_height == 1 && scale_x < 1 && scale_y < 1 )
    {
      *dest = src[0];
      return result;
    }*/

  gimp_matrix3_transform_point (&matrix,
                                0.5,              0.5,
                                &t_lx,            &t_ly);
  gimp_matrix3_transform_point (&matrix,
                                dest_width - 0.5, 0.5,
                                &t_rx,            &t_ry);
  gimp_matrix3_transform_point (&matrix,
                                0.5,              dest_height - 0.5,
                                &b_lx,            &b_ly);
  gimp_matrix3_transform_point (&matrix,
                                dest_width - 0.5, dest_height - 0.5,
                                &b_rx,            &b_ry);

  /* in image space, calc U (what was horizontal originally)
   * note: double precision
   */
  src_tl_to_tr_delta_x = t_rx - t_lx;
  src_tl_to_tr_delta_y = t_ry - t_ly;

  /* in image space, calc V (what was vertical originally)
   * note: double precision
   */
  src_tl_to_bl_delta_x = b_lx - t_lx;
  src_tl_to_bl_delta_y = b_ly - t_ly;

  /* speed optimized, note conversion to int precision */
  src_walk_ux_i = (gint) ((src_tl_to_tr_delta_x / MAX (dest_width  - 1, 1)) *
                          int_multiple);
  src_walk_uy_i = (gint) ((src_tl_to_tr_delta_y / MAX (dest_width  - 1, 1)) *
                          int_multiple);
  src_walk_vx_i = (gint) ((src_tl_to_bl_delta_x / MAX (dest_height - 1, 1)) *
                          int_multiple);
  src_walk_vy_i = (gint) ((src_tl_to_bl_delta_y / MAX (dest_height - 1, 1)) *
                          int_multiple);

  src_x_min_i = -int_multiple / 2;
  src_y_min_i = -int_multiple / 2;

  src_x_max_i = src_width  * int_multiple - int_multiple / 2;
  src_y_max_i = src_height * int_multiple - int_multiple / 2;

  gegl_parallel_distribute_area (
    GEGL_RECTANGLE (0, 0, dest_width, dest_height), PIXELS_PER_THREAD,
    [=] (const GeglRectangle *area)
    {
      guchar       *dest;
      gint          src_space_cur_pos_x;
      gint          src_space_cur_pos_y;
      gint          src_space_cur_pos_x_i;
      gint          src_space_cur_pos_y_i;
      gint          src_space_row_start_x_i;
      gint          src_space_row_start_y_i;
      const guchar *src_walker;
      const guchar *pixel_next;
      const guchar *pixel_below;
      const guchar *pixel_below_next;
      gint          opposite_x, distance_from_true_x;
      gint          opposite_y, distance_from_true_y;
      gint          u, v;

      dest = gimp_temp_buf_get_data (result) +
             dest_width * area->y + area->x;

      /* initialize current position in source space to the start position (tl)
       * speed optimized, note conversion to int precision
       */
      src_space_row_start_x_i = (gint) (t_lx * int_multiple) +
                                src_walk_vx_i * area->y      +
                                src_walk_ux_i * area->x;
      src_space_row_start_y_i = (gint) (t_ly * int_multiple) +
                                src_walk_vy_i * area->y      +
                                src_walk_uy_i * area->x;

      for (v = 0; v < area->height; v++)
        {
          src_space_cur_pos_x_i = src_space_row_start_x_i;
          src_space_cur_pos_y_i = src_space_row_start_y_i;

          for (u = 0; u < area->width; u++)
            {
              if (src_space_cur_pos_x_i <  src_x_min_i ||
                  src_space_cur_pos_x_i >= src_x_max_i ||
                  src_space_cur_pos_y_i <  src_y_min_i ||
                  src_space_cur_pos_y_i >= src_y_max_i)
                  /* no corresponding pixel in source space */
                {
                  *dest = 0;
                }
              else /* reverse transformed point hits source pixel */
                {
                  src_space_cur_pos_x = src_space_cur_pos_x_i >> fraction_bits;
                  src_space_cur_pos_y = src_space_cur_pos_y_i >> fraction_bits;

                  src_walker = src                             +
                               src_space_cur_pos_y * src_width +
                               src_space_cur_pos_x;

                  pixel_next       = src_walker + 1;
                  pixel_below      = src_walker + src_width;
                  pixel_below_next = pixel_below + 1;

                  if (src_space_cur_pos_x < 0)
                    {
                      src_walker  = pixel_next;
                      pixel_below = pixel_below_next;
                    }
                  else if (src_space_cur_pos_x >= src_width_minus_one)
                    {
                      pixel_next       = src_walker;
                      pixel_below_next = pixel_below;
                    }

                  if (src_space_cur_pos_y < 0)
                    {
                      src_walker = pixel_below;
                      pixel_next = pixel_below_next;
                    }
                  else if (src_space_cur_pos_y >= src_height_minus_one)
                    {
                      pixel_below      = src_walker;
                      pixel_below_next = pixel_next;
                    }

                  distance_from_true_x = src_space_cur_pos_x_i & fraction_bitmask;
                  distance_from_true_y = src_space_cur_pos_y_i & fraction_bitmask;
                  opposite_x =  int_multiple - distance_from_true_x;
                  opposite_y =  int_multiple - distance_from_true_y;

                  *dest = ((src_walker[0] * opposite_x +
                            pixel_next[0] * distance_from_true_x) * opposite_y +
                           (pixel_below[0] * opposite_x +
                            pixel_below_next[0] *distance_from_true_x) * distance_from_true_y
                           ) >> recovery_bits;
                }

              src_space_cur_pos_x_i += src_walk_ux_i;
              src_space_cur_pos_y_i += src_walk_uy_i;

              dest++;
            } /* end for x */

          src_space_row_start_x_i += src_walk_vx_i;
          src_space_row_start_y_i += src_walk_vy_i;

          dest += dest_width - area->width;
        } /* end for y */
    });

  gimp_brush_transform_blur (result, blur_radius);

  return result;
}

/*
 * Transforms the brush pixmap with bilinear interpolation.
 *
 * The algorithm used is exactly the same as for the brush mask
 * (gimp_brush_real_transform_mask) except it accounts for 3 color channels
 *  instead of 1 grayscale channel.
 *
 * Rather than calculating the inverse transform for each point in the
 * transformed image, this algorithm uses the inverse transformed
 * corner points of the destination image to work out the starting
 * position in the source image and the U and V deltas in the source
 * image space.  It then uses a scan-line approach, looping through
 * rows and columns in the transformed (destination) image while
 * walking along the corresponding rows and columns (named U and V) in
 * the source image.
 *
 * The horizontal in destination space (transform result) is reverse
 * transformed into source image space to get U.  The vertical in
 * destination space (transform result) is reverse transformed into
 * source image space to get V.
 *
 * The strength of this particular algorithm is that calculation work
 * should depend more upon the final transformed brush size rather
 * than the input brush size.
 *
 * There are no floating point calculations in the inner loop for speed.
 *
 * Some variables end with the suffix _i to indicate they have been
 * premultiplied by int_multiple
 */
GimpTempBuf *
gimp_brush_real_transform_pixmap (GimpBrush *brush,
                                  gdouble    scale,
                                  gdouble    aspect_ratio,
                                  gdouble    angle,
                                  gboolean   reflect,
                                  gdouble    hardness)
{
  GimpTempBuf       *result;
  const GimpTempBuf *source;
  const guchar      *src;
  GimpMatrix3        matrix;
  gdouble            scale_x, scale_y;
  gint               src_width;
  gint               src_height;
  gint               src_width_minus_one;
  gint               src_height_minus_one;
  gint               dest_width;
  gint               dest_height;
  gint               blur_radius;
  gint               x, y;
  gdouble            b_lx, b_rx, t_lx, t_rx;
  gdouble            b_ly, b_ry, t_ly, t_ry;
  gdouble            src_tl_to_tr_delta_x;
  gdouble            src_tl_to_tr_delta_y;
  gdouble            src_tl_to_bl_delta_x;
  gdouble            src_tl_to_bl_delta_y;
  gint               src_walk_ux_i;
  gint               src_walk_uy_i;
  gint               src_walk_vx_i;
  gint               src_walk_vy_i;
  gint               src_x_min_i;
  gint               src_y_min_i;
  gint               src_x_max_i;
  gint               src_y_max_i;

  /*
   * tl, tr etc are used because it is easier to visualize top left,
   * top right etc corners of the forward transformed source image
   * rectangle.
   */
  const gint fraction_bits = 12;
  const gint int_multiple  = pow (2, fraction_bits);

  /* In inner loop's bilinear calculation, two numbers that were each
   * previously multiplied by int_multiple are multiplied together.
   * To get back the right result, the multiplication result must be
   * divided *twice* by 2^fraction_bits, equivalent to bit shift right
   * by 2 * fraction_bits
   */
  const gint recovery_bits = 2 * fraction_bits;

  /*
   * example: suppose fraction_bits = 9
   * a 9-bit mask looks like this: 0001 1111 1111
   * and is given by:  2^fraction_bits - 1
   * demonstration:
   * 2^0     = 0000 0000 0001
   * 2^1     = 0000 0000 0010
   * :
   * 2^8     = 0001 0000 0000
   * 2^9     = 0010 0000 0000
   * 2^9 - 1 = 0001 1111 1111
   */
  const guint fraction_bitmask = pow(2, fraction_bits) - 1 ;

  gimp_brush_transform_get_scale (scale, aspect_ratio,
                                  &scale_x, &scale_y);

  source = gimp_brush_mipmap_get_pixmap (brush, &scale_x, &scale_y);

  src_width  = gimp_temp_buf_get_width  (source);
  src_height = gimp_temp_buf_get_height (source);

  gimp_brush_transform_matrix (src_width, src_height,
                               scale_x, scale_y, angle, reflect, &matrix);

  if (gimp_matrix3_is_identity (&matrix) && hardness == 1.0)
    return gimp_temp_buf_copy (source);

  src_width_minus_one  = src_width  - 1;
  src_height_minus_one = src_height - 1;

  gimp_brush_transform_bounding_box (source, &matrix,
                                     &x, &y, &dest_width, &dest_height);

  blur_radius = 0;

  if (hardness < 1.0)
    {
      GimpMatrix3 unrotated_matrix;
      gint        unrotated_x;
      gint        unrotated_y;
      gint        unrotated_dest_width;
      gint        unrotated_dest_height;

      gimp_brush_transform_matrix (src_width, src_height,
                                   scale_x, scale_y, 0.0, FALSE,
                                   &unrotated_matrix);

      gimp_brush_transform_bounding_box (source, &unrotated_matrix,
                                         &unrotated_x, &unrotated_y,
                                         &unrotated_dest_width,
                                         &unrotated_dest_height);

      blur_radius = gimp_brush_transform_blur_radius (unrotated_dest_width,
                                                      unrotated_dest_height,
                                                      hardness);

      gimp_brush_transform_adjust_hardness_matrix (dest_width, dest_height,
                                                   blur_radius, &matrix);
    }

  gimp_matrix3_translate (&matrix, -x, -y);
  gimp_matrix3_invert (&matrix);
  gimp_matrix3_translate (&matrix, -0.5, -0.5);

  result = gimp_temp_buf_new (dest_width, dest_height,
                              gimp_temp_buf_get_format (source));

  src = gimp_temp_buf_get_data (source);

  /* prevent disappearance of 1x1 pixel brush at some rotations when
     scaling < 1 */
  /*
  if (src_width == 1 && src_height == 1 && scale_x < 1 && scale_y < 1 )
    {
      *dest = src[0];
      return result;
    }*/

  gimp_matrix3_transform_point (&matrix,
                                0.5,              0.5,
                                &t_lx,            &t_ly);
  gimp_matrix3_transform_point (&matrix,
                                dest_width - 0.5, 0.5,
                                &t_rx,            &t_ry);
  gimp_matrix3_transform_point (&matrix,
                                0.5,              dest_height - 0.5,
                                &b_lx,            &b_ly);
  gimp_matrix3_transform_point (&matrix,
                                dest_width - 0.5, dest_height - 0.5,
                                &b_rx,            &b_ry);

  /* in image space, calc U (what was horizontal originally)
   * note: double precision
   */
  src_tl_to_tr_delta_x = t_rx - t_lx;
  src_tl_to_tr_delta_y = t_ry - t_ly;

  /* in image space, calc V (what was vertical originally)
   * note: double precision
   */
  src_tl_to_bl_delta_x = b_lx - t_lx;
  src_tl_to_bl_delta_y = b_ly - t_ly;

  /* speed optimized, note conversion to int precision */
  src_walk_ux_i = (gint) ((src_tl_to_tr_delta_x / MAX (dest_width  - 1, 1)) *
                          int_multiple);
  src_walk_uy_i = (gint) ((src_tl_to_tr_delta_y / MAX (dest_width  - 1, 1)) *
                          int_multiple);
  src_walk_vx_i = (gint) ((src_tl_to_bl_delta_x / MAX (dest_height - 1, 1)) *
                          int_multiple);
  src_walk_vy_i = (gint) ((src_tl_to_bl_delta_y / MAX (dest_height - 1, 1)) *
                          int_multiple);

  src_x_min_i = -int_multiple / 2;
  src_y_min_i = -int_multiple / 2;

  src_x_max_i = src_width  * int_multiple - int_multiple / 2;
  src_y_max_i = src_height * int_multiple - int_multiple / 2;

  gegl_parallel_distribute_area (
    GEGL_RECTANGLE (0, 0, dest_width, dest_height), PIXELS_PER_THREAD,
    [=] (const GeglRectangle *area)
    {
      guchar       *dest;
      gint          src_space_cur_pos_x;
      gint          src_space_cur_pos_y;
      gint          src_space_cur_pos_x_i;
      gint          src_space_cur_pos_y_i;
      gint          src_space_row_start_x_i;
      gint          src_space_row_start_y_i;
      const guchar *src_walker;
      const guchar *pixel_next;
      const guchar *pixel_below;
      const guchar *pixel_below_next;
      gint          opposite_x, distance_from_true_x;
      gint          opposite_y, distance_from_true_y;
      gint          u, v;

      dest = gimp_temp_buf_get_data (result) +
             3 * (dest_width * area->y + area->x);

      /* initialize current position in source space to the start position (tl)
       * speed optimized, note conversion to int precision
       */
      src_space_row_start_x_i = (gint) (t_lx * int_multiple) +
                                src_walk_vx_i * area->y      +
                                src_walk_ux_i * area->x;
      src_space_row_start_y_i = (gint) (t_ly * int_multiple) +
                                src_walk_vy_i * area->y      +
                                src_walk_uy_i * area->x;

      for (v = 0; v < area->height; v++)
        {
          src_space_cur_pos_x_i = src_space_row_start_x_i;
          src_space_cur_pos_y_i = src_space_row_start_y_i;

          for (u = 0; u < area->width; u++)
            {
              if (src_space_cur_pos_x_i <  src_x_min_i ||
                  src_space_cur_pos_x_i >= src_x_max_i ||
                  src_space_cur_pos_y_i <  src_y_min_i ||
                  src_space_cur_pos_y_i >= src_y_max_i)
                  /* no corresponding pixel in source space */
                {
                  dest[0] = 0;
                  dest[1] = 0;
                  dest[2] = 0;
                }
              else /* reverse transformed point hits source pixel */
                {
                  src_space_cur_pos_x = src_space_cur_pos_x_i >> fraction_bits;
                  src_space_cur_pos_y = src_space_cur_pos_y_i >> fraction_bits;

                  src_walker = src                                  +
                               3 * (src_space_cur_pos_y * src_width +
                                    src_space_cur_pos_x);

                  pixel_next       = src_walker + 3;
                  pixel_below      = src_walker + 3 * src_width;
                  pixel_below_next = pixel_below + 3;

                  if (src_space_cur_pos_x < 0)
                    {
                      src_walker  = pixel_next;
                      pixel_below = pixel_below_next;
                    }
                  else if (src_space_cur_pos_x >= src_width_minus_one)
                    {
                      pixel_next       = src_walker;
                      pixel_below_next = pixel_below;
                    }

                  if (src_space_cur_pos_y < 0)
                    {
                      src_walker = pixel_below;
                      pixel_next = pixel_below_next;
                    }
                  else if (src_space_cur_pos_y >= src_height_minus_one)
                    {
                      pixel_below      = src_walker;
                      pixel_below_next = pixel_next;
                    }

                  distance_from_true_x = src_space_cur_pos_x_i & fraction_bitmask;
                  distance_from_true_y = src_space_cur_pos_y_i & fraction_bitmask;
                  opposite_x =  int_multiple - distance_from_true_x;
                  opposite_y =  int_multiple - distance_from_true_y;

                  dest[0] = ((src_walker[0] * opposite_x +
                              pixel_next[0] * distance_from_true_x) * opposite_y +
                             (pixel_below[0] * opposite_x +
                              pixel_below_next[0] *distance_from_true_x) * distance_from_true_y
                            ) >> recovery_bits;

                  dest[1] = ((src_walker[1] * opposite_x +
                              pixel_next[1] * distance_from_true_x) * opposite_y +
                             (pixel_below[1] * opposite_x +
                              pixel_below_next[1] *distance_from_true_x) * distance_from_true_y
                            ) >> recovery_bits;

                  dest[2] = ((src_walker[2] * opposite_x +
                              pixel_next[2] * distance_from_true_x) * opposite_y +
                             (pixel_below[2] * opposite_x +
                              pixel_below_next[2] *distance_from_true_x) * distance_from_true_y
                            ) >> recovery_bits;
                }

              src_space_cur_pos_x_i += src_walk_ux_i;
              src_space_cur_pos_y_i += src_walk_uy_i;

              dest += 3;
            } /* end for x */

          src_space_row_start_x_i += src_walk_vx_i;
          src_space_row_start_y_i += src_walk_vy_i;

          dest += 3 * (dest_width - area->width);
        } /* end for y */
    });

  gimp_brush_transform_blur (result, blur_radius);

  return result;
}

void
gimp_brush_transform_get_scale (gdouble  scale,
                                gdouble  aspect_ratio,
                                gdouble *scale_x,
                                gdouble *scale_y)
{
  if (aspect_ratio < 0.0)
    {
      *scale_x = scale * (1.0 + (aspect_ratio / 20.0));
      *scale_y = scale;
    }
  else
    {
      *scale_x = scale;
      *scale_y = scale * (1.0 - (aspect_ratio / 20.0));
    }
}

void
gimp_brush_transform_matrix (gdouble      width,
                             gdouble      height,
                             gdouble      scale_x,
                             gdouble      scale_y,
                             gdouble      angle,
                             gboolean     reflect,
                             GimpMatrix3 *matrix)
{
  const gdouble center_x = width  / 2;
  const gdouble center_y = height / 2;

  gimp_matrix3_identity (matrix);
  gimp_matrix3_scale (matrix, scale_x, scale_y);
  gimp_matrix3_translate (matrix, - center_x * scale_x, - center_y * scale_y);
  gimp_matrix3_rotate (matrix, -2 * G_PI * angle);
  if (reflect)
    gimp_matrix3_scale (matrix, -1.0, 1.0);
  gimp_matrix3_translate (matrix, center_x * scale_x, center_y * scale_y);
}

/*  private functions  */

static void
gimp_brush_transform_bounding_box (const GimpTempBuf *temp_buf,
                                   const GimpMatrix3 *matrix,
                                   gint              *x,
                                   gint              *y,
                                   gint              *width,
                                   gint              *height)
{
  const gdouble  w = gimp_temp_buf_get_width  (temp_buf);
  const gdouble  h = gimp_temp_buf_get_height (temp_buf);
  gdouble        x1, x2, x3, x4;
  gdouble        y1, y2, y3, y4;

  gimp_matrix3_transform_point (matrix, 0, 0, &x1, &y1);
  gimp_matrix3_transform_point (matrix, w, 0, &x2, &y2);
  gimp_matrix3_transform_point (matrix, 0, h, &x3, &y3);
  gimp_matrix3_transform_point (matrix, w, h, &x4, &y4);

  *x = (gint) ceil (MIN (MIN (x1, x2), MIN (x3, x4)) - 0.5);
  *y = (gint) ceil (MIN (MIN (y1, y2), MIN (y3, y4)) - 0.5);

  *width  = (gint) ceil (MAX (MAX (x1, x2), MAX (x3, x4)) - 0.5) - *x;
  *height = (gint) ceil (MAX (MAX (y1, y2), MAX (y3, y4)) - 0.5) - *y;

  /* Transform size can not be less than 1 px */
  *width  = MAX (1, *width);
  *height = MAX (1, *height);
}

/* Blurs the brush mask/pixmap, in place, using a convolution of the form:
 *
 *   12  11  10   9   8
 *    7   6   5   4   3
 *    2   1   0   1   2
 *    3   4   5   6   7
 *    8   9  10  11  12
 *
 * (i.e., an array, wrapped into a matrix, whose i-th element is
 * `abs (i - a / 2)`, where `a` is the length of the array.)  `r` specifies the
 * convolution kernel's radius.
 */
static void
gimp_brush_transform_blur (GimpTempBuf *buf,
                           gint         r)
{
  typedef struct
  {
    gint sum;
    gint weighted_sum;
    gint middle_sum;
  } Sums;

  const Babl *format       = gimp_temp_buf_get_format (buf);
  gint        components   = babl_format_get_n_components (format);
  gint        components_r = components * r;
  gint        width        = gimp_temp_buf_get_width (buf);
  gint        height       = gimp_temp_buf_get_height (buf);
  gint        stride       = components * width;
  gint        stride_r     = stride * r;
  guchar     *data         = gimp_temp_buf_get_data (buf);
  gint        rw           = MIN (r, width - 1);
  gint        rh           = MIN (r, height - 1);
  gfloat      n            = 2 * r + 1;
  gfloat      n_r          = n * r;
  gfloat      weight       = floor (n * n / 2) * (floor (n * n / 2) + 1);
  gfloat      weight_inv   = 1 / weight;
  Sums       *sums;

  if (rw <= 0 || rh <= 0)
    return;

  sums = g_new (Sums, width * height * components);

  gegl_parallel_distribute_range (
    height, PIXELS_PER_THREAD / width,
    [=] (gint y0, gint height)
    {
      gint          x;
      gint          y;
      gint          c;
      const guchar *d;
      Sums         *s;

      d = data + y0 * stride;
      s = sums + y0 * stride;

      for (y = 0; y < height; y++)
        {
          const guchar *p;

          struct
          {
            gint sum;
            gint weighted_sum;
            gint leading_sum;
            gint leading_weighted_sum;
          } acc[components];

          memset (acc, 0, sizeof (acc));

          p = d;

          for (x = 0; x <= rw; x++)
            {
              for (c = 0; c < components; c++)
                {
                  acc[c].sum          +=      *p;
                  acc[c].weighted_sum += -x * *p;

                  p++;
                }
            }

          for (x = 0; x < width; x++)
            {
              for (c = 0; c < components; c++)
                {
                  if (x > 0)
                    {
                      acc[c].weighted_sum         += acc[c].sum;
                      acc[c].leading_weighted_sum += acc[c].leading_sum;

                      if (x < width - r)
                        {
                          acc[c].sum              +=      d[components_r];
                          acc[c].weighted_sum     += -r * d[components_r];
                        }
                    }

                  acc[c].leading_sum += d[0];

                  s->sum          = acc[c].sum;
                  s->weighted_sum = acc[c].weighted_sum;
                  s->middle_sum   = 2 * acc[c].leading_weighted_sum -
                                    acc[c].weighted_sum;

                  if (x >= r)
                    {
                      acc[c].sum                  -=     d[-components_r];
                      acc[c].weighted_sum         -= r * d[-components_r];
                      acc[c].leading_sum          -=     d[-components_r];
                      acc[c].leading_weighted_sum -= r * d[-components_r];
                    }

                  d++;
                  s++;
                }
            }
        }
    });

  gegl_parallel_distribute_range (
    width, PIXELS_PER_THREAD / height,
    [=] (gint x0, gint width)
    {
      gint        x;
      gint        y;
      gint        c;
      guchar     *d0;
      const Sums *s0;
      guchar     *d;
      const Sums *s;

      d0 = data + x0 * components;
      s0 = sums + x0 * components;

      for (x = 0; x < width; x++)
        {
          const Sums *p;
          gfloat      n_y;

          struct
          {
            gfloat weighted_sum;
            gint   leading_sum;
            gint   trailing_sum;
          } acc[components];

          memset (acc, 0, sizeof (acc));

          d = d0 + components * x;
          s = s0 + components * x;

          p = s + stride;

          for (y = 1, n_y = n; y <= rh; y++, n_y += n)
            {
              for (c = 0; c < components; c++)
                {
                  acc[c].weighted_sum += n_y * p->sum - p->weighted_sum;
                  acc[c].trailing_sum += p->sum;

                  p++;
                }

              p += stride - components;
            }

          for (y = 0; y < height; y++)
            {
              for (c = 0; c < components; c++)
                {
                  if (y > 0)
                    {
                      acc[c].weighted_sum += s->weighted_sum          +
                                             n * (acc[c].leading_sum  -
                                                  acc[c].trailing_sum);
                      acc[c].trailing_sum -= s->sum;

                      if (y < height - r)
                        {
                          acc[c].weighted_sum += n_r * s[stride_r].sum -
                                                 s[stride_r].weighted_sum;
                          acc[c].trailing_sum += s[stride_r].sum;
                        }
                    }

                  acc[c].leading_sum  += s->sum;

                  *d = (acc[c].weighted_sum + s->middle_sum) * weight_inv + 0.5f;

                  acc[c].weighted_sum += s->weighted_sum;

                  if (y >= r)
                    {
                      acc[c].weighted_sum -= n_r * s[-stride_r].sum +
                                             s[-stride_r].weighted_sum;
                      acc[c].leading_sum  -= s[-stride_r].sum;
                    }

                  d++;
                  s++;
                }

              d += stride - components;
              s += stride - components;
            }
        }
    });

  g_free (sums);
}

static gint
gimp_brush_transform_blur_radius (gint    height,
                                  gint    width,
                                  gdouble hardness)
{
  return floor ((1.0 - hardness) * (sqrt (0.5) - 0.5) * MIN (width, height));
}

static void
gimp_brush_transform_adjust_hardness_matrix (gdouble      width,
                                             gdouble      height,
                                             gdouble      blur_radius,
                                             GimpMatrix3 *matrix)
{
  gdouble scale;

  if (blur_radius == 0.0)
    return;

  scale = (MIN (width, height) - 2.0 * blur_radius) / MIN (width, height);

  gimp_matrix3_scale (matrix, scale, scale);
  gimp_matrix3_translate (matrix,
                          (1.0 - scale) * width  / 2.0,
                          (1.0 - scale) * height / 2.0);
}

} /* extern "C" */
