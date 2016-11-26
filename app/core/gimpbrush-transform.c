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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gegl/gimp-gegl-loops.h"

#include "gimpbrush.h"
#include "gimpbrush-transform.h"
#include "gimptempbuf.h"


#define MAX_BLUR_KERNEL 15


/*  local function prototypes  */

static void    gimp_brush_transform_bounding_box     (GimpBrush         *brush,
                                                      const GimpMatrix3 *matrix,
                                                      gint              *x,
                                                      gint              *y,
                                                      gint              *width,
                                                      gint              *height);

static gdouble gimp_brush_transform_array_sum        (gfloat            *arr,
                                                      gint               len);
static void    gimp_brush_transform_fill_blur_kernel (gfloat            *arr,
                                                      gint               len);
static gint    gimp_brush_transform_blur_kernel_size (gint               height,
                                                      gint               width,
                                                      gdouble            hardness);
static void    gimp_brush_transform_adjust_hardness_matrix
                                                     (gdouble      width,
                                                      gdouble      height,
                                                      gdouble      kernel_size,
                                                      GimpMatrix3 *matrix);


/*  public functions  */

void
gimp_brush_real_transform_size (GimpBrush *brush,
                                gdouble    scale,
                                gdouble    aspect_ratio,
                                gdouble    angle,
                                gint      *width,
                                gint      *height)
{
  GimpMatrix3 matrix;
  gint        x, y;

  gimp_brush_transform_matrix (gimp_brush_get_width  (brush),
                               gimp_brush_get_height (brush),
                               scale, aspect_ratio, angle, &matrix);

  gimp_brush_transform_bounding_box (brush, &matrix, &x, &y, width, height);
}

/*
 * Transforms the brush mask with bilinear interpolation.
 *
 * Rather than calculating the inverse transform for each point in the
 * transformed image, this algorithm uses the inverse transformed
 * corner points of the destination image to work out the starting
 * position in the source image and the U and V deltas in the source
 * image space.  It then uses a scan-line approach, looping through
 * rows and colummns in the transformed (destination) image while
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
                                gdouble    hardness)
{
  GimpTempBuf  *result;
  GimpTempBuf  *source;
  guchar       *dest;
  const guchar *src;
  GimpMatrix3   matrix;
  gint          src_width;
  gint          src_height;
  gint          src_width_minus_one;
  gint          src_height_minus_one;
  gint          dest_width;
  gint          dest_height;
  gint          kernel_size;
  gint          x, y;
  gdouble       blx, brx, tlx, trx;
  gdouble       bly, bry, tly, try;
  gdouble       src_tl_to_tr_delta_x;
  gdouble       src_tl_to_tr_delta_y;
  gdouble       src_tl_to_bl_delta_x;
  gdouble       src_tl_to_bl_delta_y;
  gint          src_walk_ux_i;
  gint          src_walk_uy_i;
  gint          src_walk_vx_i;
  gint          src_walk_vy_i;
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

  source = gimp_brush_get_mask (brush);

  src_width  = gimp_brush_get_width  (brush);
  src_height = gimp_brush_get_height (brush);

  gimp_brush_transform_matrix (src_width, src_height,
                               scale, aspect_ratio, angle, &matrix);

  if (gimp_matrix3_is_identity (&matrix) && hardness == 1.0)
    return gimp_temp_buf_copy (source);

  src_width_minus_one  = src_width  - 1;
  src_height_minus_one = src_height - 1;

  gimp_brush_transform_bounding_box (brush, &matrix,
                                     &x, &y, &dest_width, &dest_height);
  if (hardness < 1.0)
    {
      kernel_size =
        gimp_brush_transform_blur_kernel_size (dest_width,
                                               dest_height,
                                               hardness);

      gimp_brush_transform_adjust_hardness_matrix (dest_width, dest_height, kernel_size,
                                                   &matrix);
    }

  gimp_matrix3_translate (&matrix, -x, -y);
  gimp_matrix3_invert (&matrix);

  result = gimp_temp_buf_new (dest_width, dest_height,
                              gimp_temp_buf_get_format (source));

  dest = gimp_temp_buf_get_data (result);
  src  = gimp_temp_buf_get_data (source);

  /* prevent disappearance of 1x1 pixel brush at some rotations when
     scaling < 1 */
  /*
  if (src_width == 1 && src_height == 1 && scale_x < 1 && scale_y < 1 )
    {
      *dest = src[0];
      return result;
    }*/

  gimp_matrix3_transform_point (&matrix, 0,          0,           &tlx, &tly);
  gimp_matrix3_transform_point (&matrix, dest_width, 0,           &trx, &try);
  gimp_matrix3_transform_point (&matrix, 0,          dest_height, &blx, &bly);
  gimp_matrix3_transform_point (&matrix, dest_width, dest_height, &brx, &bry);


  /* in image space, calc U (what was horizontal originally)
   * note: double precision
   */
  src_tl_to_tr_delta_x = trx - tlx;
  src_tl_to_tr_delta_y = try - tly;

  /* in image space, calc V (what was vertical originally)
   * note: double precision
   */
  src_tl_to_bl_delta_x = blx - tlx;
  src_tl_to_bl_delta_y = bly - tly;

  /* speed optimized, note conversion to int precision */
  src_walk_ux_i = (gint) ((src_tl_to_tr_delta_x / dest_width)  * int_multiple);
  src_walk_uy_i = (gint) ((src_tl_to_tr_delta_y / dest_width)  * int_multiple);
  src_walk_vx_i = (gint) ((src_tl_to_bl_delta_x / dest_height) * int_multiple);
  src_walk_vy_i = (gint) ((src_tl_to_bl_delta_y / dest_height) * int_multiple);

  /* initialize current position in source space to the start position (tl)
   * speed optimized, note conversion to int precision
   */
  src_space_cur_pos_x_i   = (gint) (tlx* int_multiple);
  src_space_cur_pos_y_i   = (gint) (tly* int_multiple);
  src_space_cur_pos_x     = (gint) (src_space_cur_pos_x_i >> fraction_bits);
  src_space_cur_pos_y     = (gint) (src_space_cur_pos_y_i >> fraction_bits);
  src_space_row_start_x_i = (gint) (tlx* int_multiple);
  src_space_row_start_y_i = (gint) (tly* int_multiple);


  for (y = 0; y < dest_height; y++)
    {
      for (x = 0; x < dest_width; x++)
        {
          if (src_space_cur_pos_x > src_width_minus_one ||
              src_space_cur_pos_x < 0     ||
              src_space_cur_pos_y > src_height_minus_one ||
              src_space_cur_pos_y < 0)
              /* no corresponding pixel in source space */
            {
              *dest = 0;
            }
          else /* reverse transformed point hits source pixel */
            {
              src_walker = src
                        + src_space_cur_pos_y * src_width
                        + src_space_cur_pos_x;

              /* bottom right corner
               * no pixel below, reuse current pixel instead
               * no next pixel to the right so reuse current pixel instead
               */
              if (src_space_cur_pos_y == src_height_minus_one &&
                  src_space_cur_pos_x == src_width_minus_one )
                {
                  pixel_next       = src_walker;
                  pixel_below      = src_walker;
                  pixel_below_next = src_walker;
                }

              /* bottom edge pixel row, except rightmost corner
               * no pixel below, reuse current pixel instead  */
              else if (src_space_cur_pos_y == src_height_minus_one)
                {
                  pixel_next       = src_walker + 1;
                  pixel_below      = src_walker;
                  pixel_below_next = src_walker + 1;
                }

              /* right edge pixel column, except bottom corner
               * no next pixel to the right so reuse current pixel instead */
              else if (src_space_cur_pos_x == src_width_minus_one)
                {
                  pixel_next       = src_walker;
                  pixel_below      = src_walker + src_width;
                  pixel_below_next = pixel_below;
                }

              /* neither on bottom edge nor on right edge */
              else
                {
                  pixel_next       = src_walker + 1;
                  pixel_below      = src_walker + src_width;
                  pixel_below_next = pixel_below + 1;
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

          src_space_cur_pos_x_i+=src_walk_ux_i;
          src_space_cur_pos_y_i+=src_walk_uy_i;

          src_space_cur_pos_x = src_space_cur_pos_x_i >> fraction_bits;
          src_space_cur_pos_y = src_space_cur_pos_y_i >> fraction_bits;

          dest ++;
        } /* end for x */

        src_space_row_start_x_i +=src_walk_vx_i;
        src_space_row_start_y_i +=src_walk_vy_i;
        src_space_cur_pos_x_i = src_space_row_start_x_i;
        src_space_cur_pos_y_i = src_space_row_start_y_i;

        src_space_cur_pos_x = src_space_cur_pos_x_i >> fraction_bits;
        src_space_cur_pos_y = src_space_cur_pos_y_i >> fraction_bits;

    } /* end for y */

  if (hardness < 1.0)
    {
      GimpTempBuf *blur_src;
      GeglBuffer  *src_buffer;
      GeglBuffer  *dest_buffer;
      gint         kernel_len  = kernel_size * kernel_size;
      gfloat       blur_kernel[kernel_len];

      gimp_brush_transform_fill_blur_kernel (blur_kernel, kernel_len);

      blur_src = gimp_temp_buf_copy (result);

      src_buffer  = gimp_temp_buf_create_buffer (blur_src);
      dest_buffer = gimp_temp_buf_create_buffer (result);

      gimp_temp_buf_unref (blur_src);

      gimp_gegl_convolve (src_buffer,
                          GEGL_RECTANGLE (0, 0,
                                          gimp_temp_buf_get_width  (blur_src),
                                          gimp_temp_buf_get_height (blur_src)),
                          dest_buffer,
                          GEGL_RECTANGLE (0, 0,
                                          gimp_temp_buf_get_width  (result),
                                          gimp_temp_buf_get_height (result)),
                          blur_kernel, kernel_size,
                          gimp_brush_transform_array_sum (blur_kernel,
                                                          kernel_len),
                          GIMP_NORMAL_CONVOL, FALSE);

      g_object_unref (src_buffer);
      g_object_unref (dest_buffer);
    }

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
 * rows and colummns in the transformed (destination) image while
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
                                  gdouble    hardness)
{
  GimpTempBuf  *result;
  GimpTempBuf  *source;
  guchar       *dest;
  const guchar *src;
  GimpMatrix3   matrix;
  gint          src_width;
  gint          src_height;
  gint          src_width_minus_one;
  gint          src_height_minus_one;
  gint          dest_width;
  gint          dest_height;
  gint          kernel_size;
  gint          x, y;
  gdouble       blx, brx, tlx, trx;
  gdouble       bly, bry, tly, try;
  gdouble       src_tl_to_tr_delta_x;
  gdouble       src_tl_to_tr_delta_y;
  gdouble       src_tl_to_bl_delta_x;
  gdouble       src_tl_to_bl_delta_y;
  gint          src_walk_ux_i;
  gint          src_walk_uy_i;
  gint          src_walk_vx_i;
  gint          src_walk_vy_i;
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

  /*
   * tl, tr etc are used because it is easier to visualize top left,
   * top right etc corners of the forward transformed source image
   * rectangle.
   */
  const gint fraction_bits = 12;
  const gint int_multiple  = pow(2,fraction_bits);

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
  const guint fraction_bitmask = pow(2, fraction_bits)- 1 ;

  source = gimp_brush_get_pixmap (brush);

  src_width  = gimp_brush_get_width  (brush);
  src_height = gimp_brush_get_height (brush);

  gimp_brush_transform_matrix (src_width, src_height,
                               scale, aspect_ratio, angle, &matrix);

  if (gimp_matrix3_is_identity (&matrix) && hardness == 1.0)
    return gimp_temp_buf_copy (source);


  src_width_minus_one  = src_width  - 1;
  src_height_minus_one = src_height - 1;

  gimp_brush_transform_bounding_box (brush, &matrix,
                                     &x, &y, &dest_width, &dest_height);

    if (hardness < 1.0)
    {
      kernel_size =
        gimp_brush_transform_blur_kernel_size (dest_width,
                                               dest_height,
                                               hardness);

      gimp_brush_transform_adjust_hardness_matrix (dest_width, dest_height, kernel_size,
                                                   &matrix);
    }

  gimp_matrix3_translate (&matrix, -x, -y);
  gimp_matrix3_invert (&matrix);

  result = gimp_temp_buf_new (dest_width, dest_height,
                              gimp_temp_buf_get_format (source));

  dest = gimp_temp_buf_get_data (result);
  src  = gimp_temp_buf_get_data (source);

  gimp_matrix3_transform_point (&matrix, 0,          0,           &tlx, &tly);
  gimp_matrix3_transform_point (&matrix, dest_width, 0,           &trx, &try);
  gimp_matrix3_transform_point (&matrix, 0,          dest_height, &blx, &bly);
  gimp_matrix3_transform_point (&matrix, dest_width, dest_height, &brx, &bry);


  /* in image space, calc U (what was horizontal originally)
   * note: double precision
   */
  src_tl_to_tr_delta_x = trx - tlx;
  src_tl_to_tr_delta_y = try - tly;

  /* in image space, calc V (what was vertical originally)
   * note: double precision
   */
  src_tl_to_bl_delta_x = blx - tlx;
  src_tl_to_bl_delta_y = bly - tly;

  /* speed optimized, note conversion to int precision */
  src_walk_ux_i = (gint) ((src_tl_to_tr_delta_x / dest_width)* int_multiple);
  src_walk_uy_i = (gint) ((src_tl_to_tr_delta_y / dest_width)* int_multiple);
  src_walk_vx_i = (gint) ((src_tl_to_bl_delta_x / dest_height)* int_multiple);
  src_walk_vy_i = (gint) ((src_tl_to_bl_delta_y / dest_height)* int_multiple);

  /* initialize current position in source space to the start position (tl)
   * speed optimized, note conversion to int precision
   */
  src_space_cur_pos_x_i    = (gint) (tlx* int_multiple);
  src_space_cur_pos_y_i    = (gint) (tly* int_multiple);
  src_space_cur_pos_x      = (gint) (src_space_cur_pos_x_i >> fraction_bits);
  src_space_cur_pos_y      = (gint) (src_space_cur_pos_y_i >> fraction_bits);
  src_space_row_start_x_i  = (gint) (tlx* int_multiple);
  src_space_row_start_y_i  = (gint) (tly* int_multiple);


  for (y = 0; y < dest_height; y++)
    {
      for (x = 0; x < dest_width; x++)
        {
          if (src_space_cur_pos_x > src_width_minus_one  ||
              src_space_cur_pos_x < 0     ||
              src_space_cur_pos_y > src_height_minus_one ||
              src_space_cur_pos_y < 0)
              /* no corresponding pixel in source space */
            {
              dest[0] = 0;
              dest[1] = 0;
              dest[2] = 0;
            }
          else /* reverse transformed point hits source pixel */
            {
              src_walker = src
                        + 3 * (
                          src_space_cur_pos_y * src_width
                        + src_space_cur_pos_x);

              /* bottom right corner
               * no pixel below, reuse current pixel instead
               * no next pixel to the right so reuse current pixel instead
               */
              if (src_space_cur_pos_y == src_height_minus_one &&
                  src_space_cur_pos_x == src_width_minus_one )
                {
                  pixel_next  = src_walker;
                  pixel_below = src_walker;
                  pixel_below_next = src_walker;
                }

              /* bottom edge pixel row, except rightmost corner
               * no pixel below, reuse current pixel instead  */
              else if (src_space_cur_pos_y == src_height_minus_one)
                {
                  pixel_next  = src_walker + 3;
                  pixel_below = src_walker;
                  pixel_below_next = src_walker + 3;
                }

              /* right edge pixel column, except bottom corner
               * no next pixel to the right so reuse current pixel instead */
              else if (src_space_cur_pos_x == src_width_minus_one)
                {
                  pixel_next  = src_walker;
                  pixel_below = src_walker + src_width * 3;
                  pixel_below_next = pixel_below;
                }

              /* neither on bottom edge nor on right edge */
              else
                {
                  pixel_next  = src_walker + 3;
                  pixel_below = src_walker + src_width * 3;
                  pixel_below_next = pixel_below + 3;
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

          src_space_cur_pos_x = src_space_cur_pos_x_i >> fraction_bits;
          src_space_cur_pos_y = src_space_cur_pos_y_i >> fraction_bits;

          dest += 3;
        } /* end for x */

        src_space_row_start_x_i +=src_walk_vx_i;
        src_space_row_start_y_i +=src_walk_vy_i;
        src_space_cur_pos_x_i = src_space_row_start_x_i;
        src_space_cur_pos_y_i = src_space_row_start_y_i;

        src_space_cur_pos_x = src_space_cur_pos_x_i >> fraction_bits;
        src_space_cur_pos_y = src_space_cur_pos_y_i >> fraction_bits;
    } /* end for y */

  if (hardness < 1.0)
    {
      GimpTempBuf *blur_src;
      GeglBuffer  *src_buffer;
      GeglBuffer  *dest_buffer;
      gint         kernel_len  = kernel_size * kernel_size;
      gfloat       blur_kernel[kernel_len];

      gimp_brush_transform_fill_blur_kernel (blur_kernel, kernel_len);

      blur_src = gimp_temp_buf_copy (result);

      src_buffer  = gimp_temp_buf_create_buffer (blur_src);
      dest_buffer = gimp_temp_buf_create_buffer (result);

      gimp_temp_buf_unref (blur_src);

      gimp_gegl_convolve (src_buffer,
                          GEGL_RECTANGLE (0, 0,
                                          gimp_temp_buf_get_width  (blur_src),
                                          gimp_temp_buf_get_height (blur_src)),
                          dest_buffer,
                          GEGL_RECTANGLE (0, 0,
                                          gimp_temp_buf_get_width  (result),
                                          gimp_temp_buf_get_height (result)),
                          blur_kernel, kernel_size,
                          gimp_brush_transform_array_sum (blur_kernel,
                                                          kernel_len),
                          GIMP_NORMAL_CONVOL, FALSE);

      g_object_unref (src_buffer);
      g_object_unref (dest_buffer);
    }

  return result;
}

void
gimp_brush_transform_matrix (gdouble      width,
                             gdouble      height,
                             gdouble      scale,
                             gdouble      aspect_ratio,
                             gdouble      angle,
                             GimpMatrix3 *matrix)
{
  const gdouble center_x = width  / 2;
  const gdouble center_y = height / 2;
  gdouble       scale_x  = scale;
  gdouble       scale_y  = scale;

  if (aspect_ratio < 0.0)
    {
      scale_x = scale * (1.0 - (fabs (aspect_ratio) / 20.0));
      scale_y = scale;
    }
  else if (aspect_ratio > 0.0)
    {
      scale_x = scale;
      scale_y = scale * (1.0 - (aspect_ratio  / 20.0));
    }

  gimp_matrix3_identity (matrix);
  gimp_matrix3_scale (matrix, scale_x, scale_y);
  gimp_matrix3_translate (matrix, - center_x * scale_x, - center_y * scale_y);
  gimp_matrix3_rotate (matrix, -2 * G_PI * angle);
  gimp_matrix3_translate (matrix, center_x * scale_x, center_y * scale_y);
}

void
gimp_brush_transform_adjust_hardness_matrix (gdouble      width,
                                             gdouble      height,
                                             gdouble      kernel_size,
                                             GimpMatrix3 *matrix)
{

  gdouble       scale_x  = (width - 2 * kernel_size)/width;
  gdouble       scale_y  = (height - 2 * kernel_size)/height;


  gimp_matrix3_scale (matrix, scale_x, scale_y);
  gimp_matrix3_translate (matrix, kernel_size, kernel_size);
}

/*  private functions  */

static void
gimp_brush_transform_bounding_box (GimpBrush         *brush,
                                   const GimpMatrix3 *matrix,
                                   gint              *x,
                                   gint              *y,
                                   gint              *width,
                                   gint              *height)
{
  const gdouble  w = gimp_brush_get_width  (brush);
  const gdouble  h = gimp_brush_get_height (brush);
  gdouble        x1, x2, x3, x4;
  gdouble        y1, y2, y3, y4;
  gdouble        temp_x;
  gdouble        temp_y;

  gimp_matrix3_transform_point (matrix, 0, 0, &x1, &y1);
  gimp_matrix3_transform_point (matrix, w, 0, &x2, &y2);
  gimp_matrix3_transform_point (matrix, 0, h, &x3, &y3);
  gimp_matrix3_transform_point (matrix, w, h, &x4, &y4);

  temp_x = MIN (MIN (x1, x2), MIN (x3, x4));
  temp_y = MIN (MIN (y1, y2), MIN (y3, y4));

  *width  = (gint) ceil (MAX (MAX (x1, x2), MAX (x3, x4)) - temp_x);
  *height = (gint) ceil (MAX (MAX (y1, y2), MAX (y3, y4)) - temp_y);

  *x = floor (temp_x);
  *y = floor (temp_y);

  /* Transform size can not be less than 1 px */
  *width  = MAX (1, *width);
  *height = MAX (1, *height);
}

static gdouble
gimp_brush_transform_array_sum (gfloat *arr,
                                gint    len)
{
  gfloat total = 0;
  gint   i;

  for (i = 0; i < len; i++)
    {
      total += arr [i];
    }

  return total;
}

static void
gimp_brush_transform_fill_blur_kernel (gfloat *arr,
                                       gint    len)
{
  gint half_point = ((gint) len / 2) + 1;
  gint i;

  for (i = 0; i < len; i++)
    {
      if (i < half_point)
        arr [i] = half_point - i;
      else
        arr [i] = i - half_point;
    }
}

static gint
gimp_brush_transform_blur_kernel_size (gint    height,
                                       gint    width,
                                       gdouble hardness)
{
  gint kernel_size = (MIN (MAX_BLUR_KERNEL,
                           MIN (width, height)) *
                      ((MIN (width, height) * (1.0 - hardness)) /
                       MIN (width, height)));

  /* Kernel size must be odd */
  if (kernel_size % 2 == 0)
    kernel_size++;

  return kernel_size;
}
