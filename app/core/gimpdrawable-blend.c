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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <cairo.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/pixel-processor.h"
#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gegl/gimp-gegl-utils.h"

#include "gimp.h"
#include "gimp-apply-operation.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpdrawable-blend.h"
#include "gimpgradient.h"
#include "gimpimage.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


typedef struct
{
  GimpGradient     *gradient;
  GimpContext      *context;
  gboolean          reverse;
  gdouble           offset;
  gdouble           sx, sy;
  GimpBlendMode     blend_mode;
  GimpGradientType  gradient_type;
  GimpRGB           fg, bg;
  gdouble           dist;
  gdouble           vec[2];
  GimpRepeatMode    repeat;
  GRand            *seed;
  GeglBuffer       *dist_buffer;
} RenderBlendData;

typedef struct
{
  PixelRegion *PR;
  guchar      *row_data;
  gint         bytes;
  gint         width;
  GRand       *dither_rand;
} PutPixelData;


/*  local function prototypes  */

static gdouble  gradient_calc_conical_sym_factor  (gdouble   dist,
                                                   gdouble  *axis,
                                                   gdouble   offset,
                                                   gdouble   x,
                                                   gdouble   y);
static gdouble  gradient_calc_conical_asym_factor (gdouble   dist,
                                                   gdouble  *axis,
                                                   gdouble   offset,
                                                   gdouble   x,
                                                   gdouble   y);
static gdouble  gradient_calc_square_factor       (gdouble   dist,
                                                   gdouble   offset,
                                                   gdouble   x,
                                                   gdouble   y);
static gdouble  gradient_calc_radial_factor             (gdouble   dist,
                                                   gdouble   offset,
                                                   gdouble   x,
                                                   gdouble   y);
static gdouble  gradient_calc_linear_factor             (gdouble   dist,
                                                   gdouble  *vec,
                                                   gdouble   offset,
                                                   gdouble   x,
                                                   gdouble   y);
static gdouble  gradient_calc_bilinear_factor           (gdouble   dist,
                                                   gdouble  *vec,
                                                   gdouble   offset,
                                                   gdouble   x,
                                                   gdouble   y);
static gdouble  gradient_calc_spiral_factor       (gdouble   dist,
                                                   gdouble  *axis,
                                                   gdouble   offset,
                                                   gdouble   x,
                                                   gdouble   y,
                                                   gboolean  clockwise);

static gdouble  gradient_calc_shapeburst_angular_factor   (GeglBuffer *dist_buffer,
                                                           gdouble     x,
                                                           gdouble     y);
static gdouble  gradient_calc_shapeburst_spherical_factor (GeglBuffer *dist_buffer,
                                                           gdouble     x,
                                                           gdouble     y);
static gdouble  gradient_calc_shapeburst_dimpled_factor   (GeglBuffer *dist_buffer,
                                                           gdouble     x,
                                                           gdouble     y);

static GeglBuffer * gradient_precalc_shapeburst (GimpImage        *image,
                                                 GimpDrawable     *drawable,
                                                 PixelRegion      *PR,
                                                 gdouble           dist,
                                                 GimpProgress     *progress);

static void     gradient_render_pixel       (gdouble           x,
                                             gdouble           y,
                                             GimpRGB          *color,
                                             gpointer          render_data);
static void     gradient_put_pixel          (gint              x,
                                             gint              y,
                                             GimpRGB          *color,
                                             gpointer          put_pixel_data);

static void     gradient_fill_region        (GimpImage        *image,
                                             GimpDrawable     *drawable,
                                             GimpContext      *context,
                                             PixelRegion      *PR,
                                             gint              width,
                                             gint              height,
                                             GimpBlendMode     blend_mode,
                                             GimpGradientType  gradient_type,
                                             gdouble           offset,
                                             GimpRepeatMode    repeat,
                                             gboolean          reverse,
                                             gboolean          supersample,
                                             gint              max_depth,
                                             gdouble           threshold,
                                             gboolean          dither,
                                             gdouble           sx,
                                             gdouble           sy,
                                             gdouble           ex,
                                             gdouble           ey,
                                             GimpProgress     *progress);

static void     gradient_fill_single_region_rgb         (RenderBlendData *rbd,
                                                         PixelRegion     *PR);
static void     gradient_fill_single_region_rgb_dither  (RenderBlendData *rbd,
                                                         PixelRegion     *PR);
static void     gradient_fill_single_region_gray        (RenderBlendData *rbd,
                                                         PixelRegion     *PR);
static void     gradient_fill_single_region_gray_dither (RenderBlendData *rbd,
                                                         PixelRegion     *PR);


/*  public functions  */

void
gimp_drawable_blend (GimpDrawable         *drawable,
                     GimpContext          *context,
                     GimpBlendMode         blend_mode,
                     GimpLayerModeEffects  paint_mode,
                     GimpGradientType      gradient_type,
                     gdouble               opacity,
                     gdouble               offset,
                     GimpRepeatMode        repeat,
                     gboolean              reverse,
                     gboolean              supersample,
                     gint                  max_depth,
                     gdouble               threshold,
                     gboolean              dither,
                     gdouble               startx,
                     gdouble               starty,
                     gdouble               endx,
                     gdouble               endy,
                     GimpProgress         *progress)
{
  GimpImage   *image;
  GeglBuffer  *buffer;
  PixelRegion  bufPR;
  gint         x, y, width, height;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable), &x, &y, &width, &height))
    return;

  gimp_set_busy (image->gimp);

  /*  Always create an alpha temp buf (for generality) */
  buffer = gimp_gegl_buffer_new (GIMP_GEGL_RECT (0, 0, width, height),
                                 gimp_drawable_get_format_with_alpha (drawable));

  pixel_region_init (&bufPR, gimp_gegl_buffer_get_tiles (buffer),
                     0, 0, width, height, TRUE);

  gradient_fill_region (image, drawable, context,
                        &bufPR, width, height,
                        blend_mode, gradient_type, offset, repeat, reverse,
                        supersample, max_depth, threshold, dither,
                        (startx - x), (starty - y),
                        (endx - x), (endy - y),
                        progress);

  gimp_gegl_buffer_refetch_tiles (buffer);

  gimp_drawable_apply_buffer (drawable, buffer,
                              GIMP_GEGL_RECT (0, 0, width, height),
                              TRUE, C_("undo-type", "Blend"),
                              opacity, paint_mode,
                              NULL, x, y,
                              NULL, x, y);

  /*  update the image  */
  gimp_drawable_update (drawable, x, y, width, height);

  /*  free the temporary buffer  */
  g_object_unref (buffer);

  gimp_unset_busy (image->gimp);
}

static gdouble
gradient_calc_conical_sym_factor (gdouble  dist,
                                  gdouble *axis,
                                  gdouble  offset,
                                  gdouble  x,
                                  gdouble  y)
{
  if (dist == 0.0)
    {
      return 0.0;
    }
  else if ((x != 0) || (y != 0))
    {
      gdouble vec[2];
      gdouble r;
      gdouble rat;

      /* Calculate offset from the start in pixels */

      r = sqrt (SQR (x) + SQR (y));

      vec[0] = x / r;
      vec[1] = y / r;

      rat = axis[0] * vec[0] + axis[1] * vec[1]; /* Dot product */

      if (rat > 1.0)
        rat = 1.0;
      else if (rat < -1.0)
        rat = -1.0;

      /* This cool idea is courtesy Josh MacDonald,
       * Ali Rahimi --- two more XCF losers.  */

      rat = acos (rat) / G_PI;
      rat = pow (rat, (offset / 10.0) + 1.0);

      return CLAMP (rat, 0.0, 1.0);
    }
  else
    {
      return 0.5;
    }
}

static gdouble
gradient_calc_conical_asym_factor (gdouble  dist,
                                   gdouble *axis,
                                   gdouble  offset,
                                   gdouble  x,
                                   gdouble  y)
{
  if (dist == 0.0)
    {
      return 0.0;
    }
  else if (x != 0 || y != 0)
    {
      gdouble ang0, ang1;
      gdouble ang;
      gdouble rat;

      ang0 = atan2 (axis[0], axis[1]) + G_PI;

      ang1 = atan2 (x, y) + G_PI;

      ang = ang1 - ang0;

      if (ang < 0.0)
        ang += (2.0 * G_PI);

      rat = ang / (2.0 * G_PI);
      rat = pow (rat, (offset / 10.0) + 1.0);

      return CLAMP (rat, 0.0, 1.0);
    }
  else
    {
      return 0.5; /* We are on middle point */
    }
}

static gdouble
gradient_calc_square_factor (gdouble dist,
                             gdouble offset,
                             gdouble x,
                             gdouble y)
{
  if (dist == 0.0)
    {
      return 0.0;
    }
  else
    {
      gdouble r;
      gdouble rat;

      /* Calculate offset from start as a value in [0, 1] */

      offset = offset / 100.0;

      r   = MAX (abs (x), abs (y));
      rat = r / dist;

      if (rat < offset)
        return 0.0;
      else if (offset == 1.0)
        return (rat >= 1.0) ? 1.0 : 0.0;
      else
        return (rat - offset) / (1.0 - offset);
    }
}

static gdouble
gradient_calc_radial_factor (gdouble dist,
                             gdouble offset,
                             gdouble x,
                             gdouble y)
{
  if (dist == 0.0)
    {
      return 0.0;
    }
  else
    {
      gdouble r;
      gdouble rat;

      /* Calculate radial offset from start as a value in [0, 1] */

      offset = offset / 100.0;

      r   = sqrt (SQR (x) + SQR (y));
      rat = r / dist;

      if (rat < offset)
        return 0.0;
      else if (offset == 1.0)
        return (rat >= 1.0) ? 1.0 : 0.0;
      else
        return (rat - offset) / (1.0 - offset);
    }
}

static gdouble
gradient_calc_linear_factor (gdouble  dist,
                             gdouble *vec,
                             gdouble  offset,
                             gdouble  x,
                             gdouble  y)
{
  if (dist == 0.0)
    {
      return 0.0;
    }
  else
    {
      gdouble r;
      gdouble rat;

      offset = offset / 100.0;

      r   = vec[0] * x + vec[1] * y;
      rat = r / dist;

      if (rat >= 0.0 && rat < offset)
        return 0.0;
      else if (offset == 1.0)
        return (rat >= 1.0) ? 1.0 : 0.0;
      else if (rat < 0.0)
        return rat / (1.0 - offset);
      else
        return (rat - offset) / (1.0 - offset);
    }
}

static gdouble
gradient_calc_bilinear_factor (gdouble  dist,
                               gdouble *vec,
                               gdouble  offset,
                               gdouble  x,
                               gdouble  y)
{
  if (dist == 0.0)
    {
      return 0.0;
    }
  else
    {
      gdouble r;
      gdouble rat;

      /* Calculate linear offset from the start line outward */

      offset = offset / 100.0;

      r   = vec[0] * x + vec[1] * y;
      rat = r / dist;

      if (fabs (rat) < offset)
        return 0.0;
      else if (offset == 1.0)
        return (rat == 1.0) ? 1.0 : 0.0;
      else
        return (fabs (rat) - offset) / (1.0 - offset);
    }
}

static gdouble
gradient_calc_spiral_factor (gdouble   dist,
                             gdouble  *axis,
                             gdouble   offset,
                             gdouble   x,
                             gdouble   y,
                             gboolean  clockwise)
{
  if (dist == 0.0)
    {
      return 0.0;
    }
  else if (x != 0.0 || y != 0.0)
    {
      gdouble ang0, ang1;
      gdouble ang;
      double  r;

      ang0 = atan2 (axis[0], axis[1]) + G_PI;
      ang1 = atan2 (x, y) + G_PI;

      if (clockwise)
        ang = ang1 - ang0;
      else
        ang = ang0 - ang1;

      if (ang < 0.0)
        ang += (2.0 * G_PI);

      r = sqrt (SQR (x) + SQR (y)) / dist;

      return fmod (ang / (2.0 * G_PI) + r + offset, 1.0);
    }
  else
    {
      return 0.5 ; /* We are on the middle point */
    }
}

static gdouble
gradient_calc_shapeburst_angular_factor (GeglBuffer *dist_buffer,
                                         gdouble     x,
                                         gdouble     y)
{
  gint   ix = CLAMP (x, 0.0, gegl_buffer_get_width  (dist_buffer) - 0.7);
  gint   iy = CLAMP (y, 0.0, gegl_buffer_get_height (dist_buffer) - 0.7);
  gfloat value;

  gegl_buffer_get (dist_buffer, GIMP_GEGL_RECT (ix, iy, 1, 1), 1.0,
                   NULL, &value,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  value = 1.0 - value;

  return value;
}


static gdouble
gradient_calc_shapeburst_spherical_factor (GeglBuffer *dist_buffer,
                                           gdouble     x,
                                           gdouble     y)
{
  gint   ix = CLAMP (x, 0.0, gegl_buffer_get_width  (dist_buffer) - 0.7);
  gint   iy = CLAMP (y, 0.0, gegl_buffer_get_height (dist_buffer) - 0.7);
  gfloat value;

  gegl_buffer_get (dist_buffer, GIMP_GEGL_RECT (ix, iy, 1, 1), 1.0,
                   NULL, &value,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  value = 1.0 - sin (0.5 * G_PI * value);

  return value;
}


static gdouble
gradient_calc_shapeburst_dimpled_factor (GeglBuffer *dist_buffer,
                                         gdouble     x,
                                         gdouble     y)
{
  gint   ix = CLAMP (x, 0.0, gegl_buffer_get_width  (dist_buffer) - 0.7);
  gint   iy = CLAMP (y, 0.0, gegl_buffer_get_height (dist_buffer) - 0.7);
  gfloat value;

  gegl_buffer_get (dist_buffer, GIMP_GEGL_RECT (ix, iy, 1, 1), 1.0,
                   NULL, &value,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  value = cos (0.5 * G_PI * value);

  return value;
}

static GeglBuffer *
gradient_precalc_shapeburst (GimpImage    *image,
                             GimpDrawable *drawable,
                             PixelRegion  *PR,
                             gdouble       dist,
                             GimpProgress *progress)
{
  GimpChannel *mask;
  GeglBuffer  *dist_buffer;
  GeglBuffer  *temp_buffer;
  GeglNode    *shapeburst;
  gdouble      max;
  gfloat       max_iteration;

  /*  allocate the distance map  */
  dist_buffer = gegl_buffer_new (GIMP_GEGL_RECT (0, 0, PR->w, PR->h),
                                 babl_format ("Y float"));

  /*  allocate the selection mask copy  */
  temp_buffer = gimp_gegl_buffer_new (GIMP_GEGL_RECT (0, 0, PR->w, PR->h),
                                      babl_format ("Y u8"));

  mask = gimp_image_get_mask (image);

  /*  If the image mask is not empty, use it as the shape burst source  */
  if (! gimp_channel_is_empty (mask))
    {
      gint           x, y, width, height;
      gint           off_x, off_y;

      gimp_item_mask_intersect (GIMP_ITEM (drawable), &x, &y, &width, &height);
      gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

      /*  copy the mask to the temp mask  */
      gegl_buffer_copy (gimp_drawable_get_buffer (GIMP_DRAWABLE (mask)),
                        GIMP_GEGL_RECT (x+off_x, y + off_y, width, height),
                        temp_buffer,
                        GIMP_GEGL_RECT (0,0,0,0));
    }
  else
    {
      /*  If the intended drawable has an alpha channel, use that  */
      if (gimp_drawable_has_alpha (drawable))
        {
          /*  extract the aplha into the temp mask  */
          gegl_buffer_set_format (temp_buffer, babl_format ("A u8"));
          gegl_buffer_copy (gimp_drawable_get_buffer (drawable),
                            GIMP_GEGL_RECT (PR->x, PR->y, PR->w, PR->h),
                            temp_buffer,
                            GIMP_GEGL_RECT (0, 0, 0, 0));
          gegl_buffer_set_format (temp_buffer, NULL);
        }
      else
        {
          GeglColor *white = gegl_color_new ("white");

          /*  Otherwise, just fill the shapeburst to white  */
          gegl_buffer_set_color (temp_buffer, NULL, white);
          g_object_unref (white);
        }
    }

  shapeburst = gegl_node_new_child (NULL,
                                    "operation", "gimp:shapeburst",
                                    NULL);

  gimp_apply_operation (temp_buffer, NULL, NULL,
                        shapeburst,
                        dist_buffer, NULL);

  gegl_node_get (shapeburst, "max-iterations", &max, NULL);

  g_object_unref (shapeburst);

  max_iteration = max;

  g_object_unref (temp_buffer);

  /*  normalize the shapeburst with the max iteration  */
  if (max_iteration > 0)
    {
      GeglBufferIterator *iter;

      iter = gegl_buffer_iterator_new (dist_buffer, NULL, 0, NULL,
                                       GEGL_BUFFER_READWRITE, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (iter))
        {
          gfloat *data = iter->data[0];

          while (iter->length--)
            *data++ /= max_iteration;
        }
    }

  return dist_buffer;
}


static void
gradient_render_pixel (gdouble   x,
                       gdouble   y,
                       GimpRGB  *color,
                       gpointer  render_data)
{
  RenderBlendData *rbd = render_data;
  gdouble          factor;

  /* Calculate blending factor */

  switch (rbd->gradient_type)
    {
    case GIMP_GRADIENT_LINEAR:
      factor = gradient_calc_linear_factor (rbd->dist,
                                            rbd->vec, rbd->offset,
                                            x - rbd->sx, y - rbd->sy);
      break;

    case GIMP_GRADIENT_BILINEAR:
      factor = gradient_calc_bilinear_factor (rbd->dist,
                                              rbd->vec, rbd->offset,
                                              x - rbd->sx, y - rbd->sy);
      break;

    case GIMP_GRADIENT_RADIAL:
      factor = gradient_calc_radial_factor (rbd->dist,
                                            rbd->offset,
                                            x - rbd->sx, y - rbd->sy);
      break;

    case GIMP_GRADIENT_SQUARE:
      factor = gradient_calc_square_factor (rbd->dist, rbd->offset,
                                            x - rbd->sx, y - rbd->sy);
      break;

    case GIMP_GRADIENT_CONICAL_SYMMETRIC:
      factor = gradient_calc_conical_sym_factor (rbd->dist,
                                                 rbd->vec, rbd->offset,
                                                 x - rbd->sx, y - rbd->sy);
      break;

    case GIMP_GRADIENT_CONICAL_ASYMMETRIC:
      factor = gradient_calc_conical_asym_factor (rbd->dist,
                                                  rbd->vec, rbd->offset,
                                                  x - rbd->sx, y - rbd->sy);
      break;

    case GIMP_GRADIENT_SHAPEBURST_ANGULAR:
      factor = gradient_calc_shapeburst_angular_factor (rbd->dist_buffer, x, y);
      break;

    case GIMP_GRADIENT_SHAPEBURST_SPHERICAL:
      factor = gradient_calc_shapeburst_spherical_factor (rbd->dist_buffer, x, y);
      break;

    case GIMP_GRADIENT_SHAPEBURST_DIMPLED:
      factor = gradient_calc_shapeburst_dimpled_factor (rbd->dist_buffer, x, y);
      break;

    case GIMP_GRADIENT_SPIRAL_CLOCKWISE:
      factor = gradient_calc_spiral_factor (rbd->dist,
                                            rbd->vec, rbd->offset,
                                            x - rbd->sx, y - rbd->sy, TRUE);
      break;

    case GIMP_GRADIENT_SPIRAL_ANTICLOCKWISE:
      factor = gradient_calc_spiral_factor (rbd->dist,
                                            rbd->vec, rbd->offset,
                                            x - rbd->sx, y - rbd->sy, FALSE);
      break;

    default:
      g_assert_not_reached ();
      return;
    }

  /* Adjust for repeat */

  switch (rbd->repeat)
    {
    case GIMP_REPEAT_NONE:
      factor = CLAMP (factor, 0.0, 1.0);
      break;

    case GIMP_REPEAT_SAWTOOTH:
      factor = factor - floor (factor);
      break;

    case GIMP_REPEAT_TRIANGULAR:
      {
        guint ifactor;

        if (factor < 0.0)
          factor = -factor;

        ifactor = (guint) factor;
        factor = factor - floor (factor);

        if (ifactor & 1)
          factor = 1.0 - factor;
      }
      break;
    }

  /* Blend the colors */

  if (rbd->blend_mode == GIMP_CUSTOM_MODE)
    {
      gimp_gradient_get_color_at (rbd->gradient, rbd->context, NULL,
                                  factor, rbd->reverse, color);
    }
  else
    {
      /* Blend values */

      if (rbd->reverse)
        factor = 1.0 - factor;

      color->r = rbd->fg.r + (rbd->bg.r - rbd->fg.r) * factor;
      color->g = rbd->fg.g + (rbd->bg.g - rbd->fg.g) * factor;
      color->b = rbd->fg.b + (rbd->bg.b - rbd->fg.b) * factor;
      color->a = rbd->fg.a + (rbd->bg.a - rbd->fg.a) * factor;

      if (rbd->blend_mode == GIMP_FG_BG_HSV_MODE)
        {
          GimpHSV hsv = *((GimpHSV *) color);

          gimp_hsv_to_rgb (&hsv, color);
        }
    }
}

static void
gradient_put_pixel (gint      x,
                    gint      y,
                    GimpRGB  *color,
                    gpointer  put_pixel_data)
{
  PutPixelData  *ppd  = put_pixel_data;
  guchar        *dest = ppd->row_data + ppd->bytes * x;

  if (ppd->bytes >= 3)
    {
      if (ppd->dither_rand)
        {
          gint i = g_rand_int (ppd->dither_rand);

          *dest++ = color->r * 255.0 + (gdouble) (i & 0xff) / 256.0; i >>= 8;
          *dest++ = color->g * 255.0 + (gdouble) (i & 0xff) / 256.0; i >>= 8;
          *dest++ = color->b * 255.0 + (gdouble) (i & 0xff) / 256.0; i >>= 8;
          *dest++ = color->a * 255.0 + (gdouble) (i & 0xff) / 256.0;
        }
      else
        {
          *dest++ = ROUND (color->r * 255.0);
          *dest++ = ROUND (color->g * 255.0);
          *dest++ = ROUND (color->b * 255.0);
          *dest++ = ROUND (color->a * 255.0);
        }
    }
  else
    {
      /* Convert to grayscale */
      gdouble gray = gimp_rgb_luminance (color);

      if (ppd->dither_rand)
        {
          gint i = g_rand_int (ppd->dither_rand);

          *dest++ = gray     * 255.0 + (gdouble) (i & 0xff) / 256.0; i >>= 8;
          *dest++ = color->a * 255.0 + (gdouble) (i & 0xff) / 256.0;
        }
      else
        {
          *dest++ = ROUND (gray     * 255.0);
          *dest++ = ROUND (color->a * 255.0);
        }
    }

  /* Paint whole row if we are on the rightmost pixel */

  if (x == (ppd->width - 1))
    pixel_region_set_row (ppd->PR, 0, y, ppd->width, ppd->row_data);
}

static void
gradient_fill_region (GimpImage        *image,
                      GimpDrawable     *drawable,
                      GimpContext      *context,
                      PixelRegion      *PR,
                      gint              width,
                      gint              height,
                      GimpBlendMode     blend_mode,
                      GimpGradientType  gradient_type,
                      gdouble           offset,
                      GimpRepeatMode    repeat,
                      gboolean          reverse,
                      gboolean          supersample,
                      gint              max_depth,
                      gdouble           threshold,
                      gboolean          dither,
                      gdouble           sx,
                      gdouble           sy,
                      gdouble           ex,
                      gdouble           ey,
                      GimpProgress     *progress)
{
  RenderBlendData rbd = { 0, };

  rbd.gradient = gimp_context_get_gradient (context);
  rbd.context  = context;
  rbd.reverse  = reverse;

  if (gimp_gradient_has_fg_bg_segments (rbd.gradient))
    rbd.gradient = gimp_gradient_flatten (rbd.gradient, context);
  else
    rbd.gradient = g_object_ref (rbd.gradient);

  gimp_context_get_foreground (context, &rbd.fg);
  gimp_context_get_background (context, &rbd.bg);

  switch (blend_mode)
    {
    case GIMP_FG_BG_RGB_MODE:
      break;

    case GIMP_FG_BG_HSV_MODE:
      /* Convert to HSV */
      {
        GimpHSV fg_hsv;
        GimpHSV bg_hsv;

        gimp_rgb_to_hsv (&rbd.fg, &fg_hsv);
        gimp_rgb_to_hsv (&rbd.bg, &bg_hsv);

        memcpy (&rbd.fg, &fg_hsv, sizeof (GimpRGB));
        memcpy (&rbd.bg, &bg_hsv, sizeof (GimpRGB));
      }
      break;

    case GIMP_FG_TRANSPARENT_MODE:
      /* Color does not change, just the opacity */

      rbd.bg   = rbd.fg;
      rbd.bg.a = GIMP_OPACITY_TRANSPARENT;
      break;

    case GIMP_CUSTOM_MODE:
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  /* Calculate type-specific parameters */

  switch (gradient_type)
    {
    case GIMP_GRADIENT_RADIAL:
      rbd.dist = sqrt (SQR (ex - sx) + SQR (ey - sy));
      break;

    case GIMP_GRADIENT_SQUARE:
      rbd.dist = MAX (fabs (ex - sx), fabs (ey - sy));
      break;

    case GIMP_GRADIENT_CONICAL_SYMMETRIC:
    case GIMP_GRADIENT_CONICAL_ASYMMETRIC:
    case GIMP_GRADIENT_SPIRAL_CLOCKWISE:
    case GIMP_GRADIENT_SPIRAL_ANTICLOCKWISE:
    case GIMP_GRADIENT_LINEAR:
    case GIMP_GRADIENT_BILINEAR:
      rbd.dist = sqrt (SQR (ex - sx) + SQR (ey - sy));

      if (rbd.dist > 0.0)
        {
          rbd.vec[0] = (ex - sx) / rbd.dist;
          rbd.vec[1] = (ey - sy) / rbd.dist;
        }

      break;

    case GIMP_GRADIENT_SHAPEBURST_ANGULAR:
    case GIMP_GRADIENT_SHAPEBURST_SPHERICAL:
    case GIMP_GRADIENT_SHAPEBURST_DIMPLED:
      rbd.dist = sqrt (SQR (ex - sx) + SQR (ey - sy));
      rbd.dist_buffer = gradient_precalc_shapeburst (image, drawable,
                                                     PR, rbd.dist, progress);
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  /* Initialize render data */

  rbd.offset        = offset;
  rbd.sx            = sx;
  rbd.sy            = sy;
  rbd.blend_mode    = blend_mode;
  rbd.gradient_type = gradient_type;
  rbd.repeat        = repeat;

  /* Render the gradient! */

  if (supersample)
    {
      PutPixelData  ppd;

      ppd.PR          = PR;
      ppd.row_data    = g_malloc (width * PR->bytes);
      ppd.bytes       = PR->bytes;
      ppd.width       = width;
      ppd.dither_rand = g_rand_new ();

      gimp_adaptive_supersample_area (0, 0, (width - 1), (height - 1),
                                      max_depth, threshold,
                                      gradient_render_pixel, &rbd,
                                      gradient_put_pixel, &ppd,
                                      progress ?
                                      gimp_progress_update_and_flush : NULL,
                                      progress);

      g_rand_free (ppd.dither_rand);
      g_free (ppd.row_data);
    }
  else
    {
      PixelProcessorFunc          func;
      PixelProcessorProgressFunc  progress_func = NULL;

      if (dither)
        {
          rbd.seed = g_rand_new ();

          if (PR->bytes >= 3)
            func = (PixelProcessorFunc) gradient_fill_single_region_rgb_dither;
          else
            func = (PixelProcessorFunc) gradient_fill_single_region_gray_dither;
        }
      else
        {
          if (PR->bytes >= 3)
            func = (PixelProcessorFunc) gradient_fill_single_region_rgb;
          else
            func = (PixelProcessorFunc) gradient_fill_single_region_gray;
        }

      if (progress)
        progress_func = (PixelProcessorProgressFunc) gimp_progress_set_value;

      pixel_regions_process_parallel_progress (func, &rbd,
                                               progress_func, progress,
                                               1, PR);

      if (dither)
        g_rand_free (rbd.seed);
    }

  g_object_unref (rbd.gradient);

  if (rbd.dist_buffer)
    g_object_unref (rbd.dist_buffer);
}

static void
gradient_fill_single_region_rgb (RenderBlendData *rbd,
                                 PixelRegion     *PR)
{
  guchar *dest = PR->data;
  gint    endx = PR->x + PR->w;
  gint    endy = PR->y + PR->h;
  gint    x, y;

  for (y = PR->y; y < endy; y++)
    for (x = PR->x; x < endx; x++)
      {
        GimpRGB  color;

        gradient_render_pixel (x, y, &color, rbd);

        *dest++ = ROUND (color.r * 255.0);
        *dest++ = ROUND (color.g * 255.0);
        *dest++ = ROUND (color.b * 255.0);
        *dest++ = ROUND (color.a * 255.0);
      }
}

static void
gradient_fill_single_region_rgb_dither (RenderBlendData *rbd,
                                        PixelRegion     *PR)
{
  GRand  *dither_rand = g_rand_new_with_seed (g_rand_int (rbd->seed));
  guchar *dest        = PR->data;
  gint    endx        = PR->x + PR->w;
  gint    endy        = PR->y + PR->h;
  gint    x, y;

  for (y = PR->y; y < endy; y++)
    for (x = PR->x; x < endx; x++)
      {
        GimpRGB  color;
        gint     i = g_rand_int (dither_rand);

        gradient_render_pixel (x, y, &color, rbd);

        *dest++ = color.r * 255.0 + (gdouble) (i & 0xff) / 256.0; i >>= 8;
        *dest++ = color.g * 255.0 + (gdouble) (i & 0xff) / 256.0; i >>= 8;
        *dest++ = color.b * 255.0 + (gdouble) (i & 0xff) / 256.0; i >>= 8;
        *dest++ = color.a * 255.0 + (gdouble) (i & 0xff) / 256.0;
      }

  g_rand_free (dither_rand);
}

static void
gradient_fill_single_region_gray (RenderBlendData *rbd,
                                  PixelRegion     *PR)
{
  guchar *dest = PR->data;
  gint    endx = PR->x + PR->w;
  gint    endy = PR->y + PR->h;
  gint    x, y;

  for (y = PR->y; y < endy; y++)
    for (x = PR->x; x < endx; x++)
      {
        GimpRGB  color;

        gradient_render_pixel (x, y, &color, rbd);

        *dest++ = gimp_rgb_luminance_uchar (&color);
        *dest++ = ROUND (color.a * 255.0);
      }
}

static void
gradient_fill_single_region_gray_dither (RenderBlendData *rbd,
                                         PixelRegion     *PR)
{
  GRand  *dither_rand = g_rand_new_with_seed (g_rand_int (rbd->seed));
  guchar *dest        = PR->data;
  gint    endx        = PR->x + PR->w;
  gint    endy        = PR->y + PR->h;
  gint    x, y;

  for (y = PR->y; y < endy; y++)
    for (x = PR->x; x < endx; x++)
      {
        GimpRGB  color;
        gdouble  gray;
        gint     i = g_rand_int (dither_rand);

        gradient_render_pixel (x, y, &color, rbd);

        gray = gimp_rgb_luminance (&color);

        *dest++ = gray    * 255.0 + (gdouble) (i & 0xff) / 256.0; i >>= 8;
        *dest++ = color.a * 255.0 + (gdouble) (i & 0xff) / 256.0;
      }

  g_rand_free (dither_rand);
}
