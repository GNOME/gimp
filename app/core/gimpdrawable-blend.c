/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
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
  gboolean          reverse;
  gdouble           offset;
  gdouble           sx, sy;
  GimpBlendMode     blend_mode;
  GimpGradientType  gradient_type;
  GimpRGB           fg, bg;
  gdouble           dist;
  gdouble           vec[2];
  GimpRepeatMode    repeat;
} RenderBlendData;

typedef struct
{
  PixelRegion *PR;
  guchar      *row_data;
  gint         bytes;
  gint         width;
  gboolean     dither;
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
static gdouble  gradient_calc_radial_factor   	  (gdouble   dist,
						   gdouble   offset,
                                                   gdouble   x,
						   gdouble   y);
static gdouble  gradient_calc_linear_factor   	  (gdouble   dist,
                                                   gdouble  *vec,
                                                   gdouble   offset,
                                                   gdouble   x,
                                                   gdouble   y);
static gdouble  gradient_calc_bilinear_factor 	  (gdouble   dist,
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

static gdouble  gradient_calc_shapeburst_angular_factor   (gdouble x,
                                                           gdouble y);
static gdouble  gradient_calc_shapeburst_spherical_factor (gdouble x,
                                                           gdouble y);
static gdouble  gradient_calc_shapeburst_dimpled_factor   (gdouble x,
                                                           gdouble y);

static void     gradient_precalc_shapeburst (GimpImage        *gimage,
					     GimpDrawable     *drawable,
                                             PixelRegion      *PR,
                                             gdouble           dist,
                                             GimpProgress     *progress);

static void     gradient_render_pixel       (gdouble       x,
                                             gdouble       y,
                                             GimpRGB      *color,
                                             gpointer      render_data);
static void     gradient_put_pixel          (gint          x,
                                             gint          y,
                                             GimpRGB      *color,
					     gpointer      put_pixel_data);

static void     gradient_fill_region        (GimpImage        *gimage,
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


/*  variables for the shapeburst algs  */

static PixelRegion distR =
{
  NULL,  /* data */
  NULL,  /* tiles */
  NULL,  /* curtile */
  0, 0,  /* offx, offy */
  0,     /* rowstride */
  0, 0,  /* x, y */
  0, 0,  /* w, h */
  4,     /* bytes */
  FALSE, /* dirty */
  0      /* process count */
};


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
  GimpImage   *gimage;
  TileManager *buf_tiles;
  PixelRegion  bufPR;
  gint         bytes;
  gint         x, y, width, height;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  if (! gimp_drawable_mask_intersect (drawable, &x, &y, &width, &height))
    return;

  gimp_set_busy (gimage->gimp);

  bytes = gimp_drawable_bytes (drawable);

  /*  Always create an alpha temp buf (for generality) */
  if (! gimp_drawable_has_alpha (drawable))
    {
      bytes += 1;
    }

  buf_tiles = tile_manager_new (width, height, bytes);
  pixel_region_init (&bufPR, buf_tiles, 0, 0, width, height, TRUE);

  gradient_fill_region (gimage, drawable, context,
			&bufPR, width, height,
			blend_mode, gradient_type, offset, repeat, reverse,
			supersample, max_depth, threshold, dither,
			(startx - x), (starty - y),
			(endx - x), (endy - y),
			progress);

  if (distR.tiles)
    {
      tile_manager_unref (distR.tiles);
      distR.tiles = NULL;
    }

  pixel_region_init (&bufPR, buf_tiles, 0, 0, width, height, FALSE);
  gimp_drawable_apply_region (drawable, &bufPR,
                              TRUE, _("Blend"),
                              opacity, paint_mode,
                              NULL, x, y);

  /*  update the image  */
  gimp_drawable_update (drawable, x, y, width, height);

  /*  free the temporary buffer  */
  tile_manager_unref (buf_tiles);

  gimp_unset_busy (gimage->gimp);
}

static gdouble
gradient_calc_conical_sym_factor (gdouble  dist,
				  gdouble *axis,
				  gdouble  offset,
				  gdouble  x,
				  gdouble  y)
{
  gdouble vec[2];
  gdouble r;
  gdouble rat;

  if (dist == 0.0)
    {
      rat = 0.0;
    }
  else if ((x != 0) || (y != 0))
    {
      /* Calculate offset from the start in pixels */

      r = sqrt (x * x + y * y);

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

      rat = CLAMP (rat, 0.0, 1.0);
    }
  else
    {
      rat = 0.5;
    }

  return rat;
}

static gdouble
gradient_calc_conical_asym_factor (gdouble  dist,
				   gdouble *axis,
				   gdouble  offset,
				   gdouble  x,
				   gdouble  y)
{
  gdouble ang0, ang1;
  gdouble ang;
  gdouble rat;

  if (dist == 0.0)
    {
      rat = 0.0;
    }
  else
    {
      if ((x != 0) || (y != 0))
	{
	  ang0 = atan2 (axis[0], axis[1]) + G_PI;
	  ang1 = atan2 (x, y) + G_PI;

	  ang = ang1 - ang0;

	  if (ang < 0.0)
	    ang += (2.0 * G_PI);

	  rat = ang / (2.0 * G_PI);
	  rat = pow (rat, (offset / 10.0) + 1.0);

	  rat = CLAMP (rat, 0.0, 1.0);
	}
      else
	{
	  rat = 0.5; /* We are on middle point */
	}
    }

  return rat;
}

static gdouble
gradient_calc_square_factor (gdouble dist,
			     gdouble offset,
			     gdouble x,
			     gdouble y)
{
  gdouble r;
  gdouble rat;

  if (dist == 0.0)
    {
      rat = 0.0;
    }
  else
    {
      /* Calculate offset from start as a value in [0, 1] */

      offset = offset / 100.0;

      r   = MAX (abs (x), abs (y));
      rat = r / dist;

      if (rat < offset)
	rat = 0.0;
      else if (offset == 1.0)
	rat = (rat >= 1.0) ? 1.0 : 0.0;
      else
	rat = (rat - offset) / (1.0 - offset);
    }

  return rat;
}

static gdouble
gradient_calc_radial_factor (gdouble dist,
			     gdouble offset,
			     gdouble x,
			     gdouble y)
{
  gdouble r;
  gdouble rat;

  if (dist == 0.0)
    {
      rat = 0.0;
    }
  else
    {
      /* Calculate radial offset from start as a value in [0, 1] */

      offset = offset / 100.0;

      r   = sqrt (SQR (x) + SQR (y));
      rat = r / dist;

      if (rat < offset)
	rat = 0.0;
      else if (offset == 1.0)
	rat = (rat >= 1.0) ? 1.0 : 0.0;
      else
	rat = (rat - offset) / (1.0 - offset);
    }

  return rat;
}

static gdouble
gradient_calc_linear_factor (gdouble  dist,
			     gdouble *vec,
			     gdouble  offset,
			     gdouble  x,
			     gdouble  y)
{
  gdouble r;
  gdouble rat;

  if (dist == 0.0)
    {
      rat = 0.0;
    }
  else
    {
      offset = offset / 100.0;

      r   = vec[0] * x + vec[1] * y;
      rat = r / dist;

      if (rat >= 0.0 && rat < offset)
	rat = 0.0;
      else if (offset == 1.0)
	rat = (rat >= 1.0) ? 1.0 : 0.0;
      else if (rat < 0.0)
	rat = rat / (1.0 - offset);
      else
	rat = (rat - offset) / (1.0 - offset);
    }

  return rat;
}

static gdouble
gradient_calc_bilinear_factor (gdouble  dist,
			       gdouble *vec,
			       gdouble  offset,
			       gdouble  x,
			       gdouble  y)
{
  gdouble r;
  gdouble rat;

  if (dist == 0.0)
    {
      rat = 0.0;
    }
  else
    {
      /* Calculate linear offset from the start line outward */

      offset = offset / 100.0;

      r   = vec[0] * x + vec[1] * y;
      rat = r / dist;

      if (fabs (rat) < offset)
	rat = 0.0;
      else if (offset == 1.0)
	rat = (rat == 1.0) ? 1.0 : 0.0;
      else
	rat = (fabs (rat) - offset) / (1.0 - offset);
    }

  return rat;
}

static gdouble
gradient_calc_spiral_factor (gdouble   dist,
			     gdouble  *axis,
			     gdouble   offset,
			     gdouble   x,
			     gdouble   y,
			     gboolean  clockwise)
{
  gdouble ang0, ang1;
  gdouble ang, r;
  gdouble rat;

  if (dist == 0.0)
    {
      rat = 0.0;
    }
  else
    {
      if (x != 0.0 || y != 0.0)
	{
	  ang0 = atan2 (axis[0], axis[1]) + G_PI;
	  ang1 = atan2 (x, y) + G_PI;

	  if (clockwise)
	    ang = ang1 - ang0;
	  else
	    ang = ang0 - ang1;

	  if (ang < 0.0)
	    ang += (2.0 * G_PI);

	  r = sqrt (x * x + y * y) / dist;
	  rat = ang / (2.0 * G_PI) + r + offset;
	  rat = fmod (rat, 1.0);
	}
      else
	rat = 0.5 ; /* We are on the middle point */
    }

  return rat;
}

static gdouble
gradient_calc_shapeburst_angular_factor (gdouble x,
					 gdouble y)
{
  gint    ix, iy;
  Tile   *tile;
  gfloat  value;

  ix = (gint) CLAMP (x, 0.0, distR.w - 0.7);
  iy = (gint) CLAMP (y, 0.0, distR.h - 0.7);
  tile = tile_manager_get_tile (distR.tiles, ix, iy, TRUE, FALSE);
  value = 1.0 - *((gfloat *) tile_data_pointer (tile,
                                                ix % TILE_WIDTH,
                                                iy % TILE_HEIGHT));
  tile_release (tile, FALSE);

  return value;
}


static gdouble
gradient_calc_shapeburst_spherical_factor (gdouble x,
					   gdouble y)
{
  gint    ix, iy;
  Tile   *tile;
  gfloat  value;

  ix = (gint) CLAMP (x, 0.0, distR.w - 0.7);
  iy = (gint) CLAMP (y, 0.0, distR.h - 0.7);
  tile = tile_manager_get_tile (distR.tiles, ix, iy, TRUE, FALSE);
  value = *((gfloat *) tile_data_pointer (tile,
                                          ix % TILE_WIDTH,
                                          iy % TILE_HEIGHT));
  value = 1.0 - sin (0.5 * G_PI * value);
  tile_release (tile, FALSE);

  return value;
}


static gdouble
gradient_calc_shapeburst_dimpled_factor (gdouble x,
					 gdouble y)
{
  gint    ix, iy;
  Tile   *tile;
  gfloat  value;

  ix = (gint) CLAMP (x, 0.0, distR.w - 0.7);
  iy = (gint) CLAMP (y, 0.0, distR.h - 0.7);
  tile = tile_manager_get_tile (distR.tiles, ix, iy, TRUE, FALSE);
  value = *((gfloat *) tile_data_pointer (tile,
                                          ix % TILE_WIDTH,
                                          iy % TILE_HEIGHT));
  value = cos (0.5 * G_PI * value);
  tile_release (tile, FALSE);

  return value;
}

static void
gradient_precalc_shapeburst (GimpImage    *gimage,
			     GimpDrawable *drawable,
			     PixelRegion  *PR,
			     gdouble       dist,
                             GimpProgress *progress)
{
  GimpChannel *mask;
  PixelRegion  tempR;
  gfloat       max_iteration;
  gfloat      *distp;
  gint         size;
  gpointer     pr;
  guchar       white[1] = { OPAQUE_OPACITY };

  /*  allocate the distance map  */
  distR.tiles = tile_manager_new (PR->w, PR->h, sizeof (gfloat));

  /*  allocate the selection mask copy  */
  tempR.tiles = tile_manager_new (PR->w, PR->h, 1);
  pixel_region_init (&tempR, tempR.tiles, 0, 0, PR->w, PR->h, TRUE);

  mask = gimp_image_get_mask (gimage);

  /*  If the gimage mask is not empty, use it as the shape burst source  */
  if (! gimp_channel_is_empty (mask))
    {
      PixelRegion maskR;
      gint        x1, y1, x2, y2;
      gint        offx, offy;

      gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);
      gimp_item_offsets (GIMP_ITEM (drawable), &offx, &offy);

      pixel_region_init (&maskR, gimp_drawable_data (GIMP_DRAWABLE (mask)),
			 x1 + offx, y1 + offy, (x2 - x1), (y2 - y1), FALSE);

      /*  copy the mask to the temp mask  */
      copy_region (&maskR, &tempR);
    }
  /*  otherwise...  */
  else
    {
      /*  If the intended drawable has an alpha channel, use that  */
      if (gimp_drawable_has_alpha (drawable))
	{
	  PixelRegion drawableR;

	  pixel_region_init (&drawableR, gimp_drawable_data (drawable),
			     PR->x, PR->y, PR->w, PR->h, FALSE);

	  extract_alpha_region (&drawableR, NULL, &tempR);
	}
      else
	{
	  /*  Otherwise, just fill the shapeburst to white  */
	  color_region (&tempR, white);
	}
    }

  pixel_region_init (&tempR, tempR.tiles, 0, 0, PR->w, PR->h, TRUE);
  pixel_region_init (&distR, distR.tiles, 0, 0, PR->w, PR->h, TRUE);
  max_iteration = shapeburst_region (&tempR, &distR,
                                     progress ?
                                     gimp_progress_update_and_flush : NULL,
                                     progress);

  /*  normalize the shapeburst with the max iteration  */
  if (max_iteration > 0)
    {
      pixel_region_init (&distR, distR.tiles, 0, 0, PR->w, PR->h, TRUE);

      for (pr = pixel_regions_register (1, &distR);
	   pr != NULL;
	   pr = pixel_regions_process (pr))
	{
	  distp = (gfloat *) distR.data;
	  size  = distR.w * distR.h;

	  while (size--)
	    *distp++ /= max_iteration;
	}

      pixel_region_init (&distR, distR.tiles, 0, 0, PR->w, PR->h, FALSE);
    }

  tile_manager_unref (tempR.tiles);
}


static void
gradient_render_pixel (double    x,
		       double    y,
		       GimpRGB  *color,
		       gpointer  render_data)
{
  RenderBlendData *rbd = render_data;
  gdouble          factor;

  /* Calculate blending factor */

  switch (rbd->gradient_type)
    {
    case GIMP_GRADIENT_LINEAR:
      factor = gradient_calc_linear_factor (rbd->dist, rbd->vec, rbd->offset,
					    x - rbd->sx, y - rbd->sy);
      break;

    case GIMP_GRADIENT_BILINEAR:
      factor = gradient_calc_bilinear_factor (rbd->dist, rbd->vec, rbd->offset,
					      x - rbd->sx, y - rbd->sy);
      break;

    case GIMP_GRADIENT_RADIAL:
      factor = gradient_calc_radial_factor (rbd->dist, rbd->offset,
					    x - rbd->sx, y - rbd->sy);
      break;

    case GIMP_GRADIENT_SQUARE:
      factor = gradient_calc_square_factor (rbd->dist, rbd->offset,
					    x - rbd->sx, y - rbd->sy);
      break;

    case GIMP_GRADIENT_CONICAL_SYMMETRIC:
      factor = gradient_calc_conical_sym_factor (rbd->dist, rbd->vec, rbd->offset,
						 x - rbd->sx, y - rbd->sy);
      break;

    case GIMP_GRADIENT_CONICAL_ASYMMETRIC:
      factor = gradient_calc_conical_asym_factor (rbd->dist, rbd->vec, rbd->offset,
						  x - rbd->sx, y - rbd->sy);
      break;

    case GIMP_GRADIENT_SHAPEBURST_ANGULAR:
      factor = gradient_calc_shapeburst_angular_factor (x, y);
      break;

    case GIMP_GRADIENT_SHAPEBURST_SPHERICAL:
      factor = gradient_calc_shapeburst_spherical_factor (x, y);
      break;

    case GIMP_GRADIENT_SHAPEBURST_DIMPLED:
      factor = gradient_calc_shapeburst_dimpled_factor (x, y);
      break;

    case GIMP_GRADIENT_SPIRAL_CLOCKWISE:
      factor = gradient_calc_spiral_factor (rbd->dist, rbd->vec, rbd->offset,
					    x - rbd->sx, y - rbd->sy,TRUE);
      break;

    case GIMP_GRADIENT_SPIRAL_ANTICLOCKWISE:
      factor = gradient_calc_spiral_factor (rbd->dist, rbd->vec, rbd->offset,
					    x - rbd->sx, y - rbd->sy,FALSE);
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
      gimp_gradient_get_color_at (rbd->gradient, factor, rbd->reverse, color);
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
  PutPixelData  *ppd = put_pixel_data;
  guchar        *data;

  /* Paint */

  data = ppd->row_data + ppd->bytes * x;

  if (ppd->bytes >= 3)
    {
      if (ppd->dither)
        {
	  gdouble dither_prob;
          gdouble ftmp;
          gint    itmp;

          ftmp = color->r * 255.0;
          itmp = ftmp;
          dither_prob = ftmp - itmp;

          if (g_rand_double (ppd->dither_rand) < dither_prob)
            color->r += (1.0 / 255.0);

          ftmp = color->g * 255.0;
          itmp = ftmp;
          dither_prob = ftmp - itmp;

          if (g_rand_double (ppd->dither_rand) < dither_prob)
            color->g += (1.0 / 255.0);

          ftmp = color->b * 255.0;
          itmp = ftmp;
          dither_prob = ftmp - itmp;

          if (g_rand_double (ppd->dither_rand) < dither_prob)
            color->b += (1.0 / 255.0);

          ftmp = color->a * 255.0;
          itmp = ftmp;
          dither_prob = ftmp - itmp;

          if (g_rand_double (ppd->dither_rand) < dither_prob)
            color->a += (1.0 / 255.0);

          if (color->r > 1.0) color->r = 1.0;
          if (color->g > 1.0) color->g = 1.0;
          if (color->b > 1.0) color->b = 1.0;
          if (color->a > 1.0) color->a = 1.0;
        }

      *data++ = color->r * 255.0;
      *data++ = color->g * 255.0;
      *data++ = color->b * 255.0;
      *data++ = color->a * 255.0;
    }
  else
    {
      /* Convert to grayscale */
      gdouble gray = gimp_rgb_intensity (color);

      if (ppd->dither)
        {
	  gdouble dither_prob;
          gdouble ftmp;
          gint    itmp;

          ftmp = gray * 255.0;
          itmp = ftmp;
          dither_prob = ftmp - itmp;

          if (g_rand_double (ppd->dither_rand) < dither_prob)
            gray += (1.0 / 255.0);

          ftmp = color->a * 255.0;
          itmp = ftmp;
          dither_prob = ftmp - itmp;

          if (g_rand_double (ppd->dither_rand) < dither_prob)
            color->a += (1.0 / 255.0);

          if (gray > 1.0)     gray     = 1.0;
          if (color->a > 1.0) color->a = 1.0;
        }

      *data++ = gray * 255.0;
      *data++ = color->a * 255.0;
    }

  /* Paint whole row if we are on the rightmost pixel */

  if (x == (ppd->width - 1))
    pixel_region_set_row (ppd->PR, 0, y, ppd->width, ppd->row_data);
}

static void
gradient_fill_region (GimpImage        *gimage,
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
  RenderBlendData  rbd;
  gint             x, y;
  gint             endx, endy;
  gpointer         pr;
  guchar          *data;
  GimpRGB          color;
  GRand           *dither_rand = NULL;

  rbd.gradient = gimp_context_get_gradient (context);
  rbd.reverse  = reverse;

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
      gradient_precalc_shapeburst (gimage, drawable, PR, rbd.dist, progress);
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

  if (dither)
    dither_rand = g_rand_new ();

  /* Render the gradient! */

  if (supersample)
    {
      PutPixelData  ppd;

      ppd.PR          = PR;
      ppd.row_data    = g_malloc (width * PR->bytes);
      ppd.bytes       = PR->bytes;
      ppd.width       = width;
      ppd.dither      = dither;
      ppd.dither_rand = dither_rand;

      gimp_adaptive_supersample_area (0, 0, (width - 1), (height - 1),
				      max_depth, threshold,
				      gradient_render_pixel, &rbd,
				      gradient_put_pixel, &ppd,
				      progress ?
                                      gimp_progress_update_and_flush : NULL,
                                      progress);

      g_free (ppd.row_data);
    }
  else
    {
      gint max_progress  = PR->w * PR->h;
      gint curr_progress = 0;

      for (pr = pixel_regions_register (1, PR);
	   pr != NULL;
	   pr = pixel_regions_process (pr))
	{
	  data = PR->data;
	  endx = PR->x + PR->w;
	  endy = PR->y + PR->h;

	  for (y = PR->y; y < endy; y++)
            {
              for (x = PR->x; x < endx; x++)
                {
                  gradient_render_pixel (x, y, &color, &rbd);

                  if (PR->bytes >= 3)
                    {
                      if (dither)
                        {
			  gdouble dither_prob;
                          gdouble ftmp;
                          gint    itmp;

                          ftmp = color.r * 255.0;
                          itmp = ftmp;
                          dither_prob = ftmp - itmp;

                          if (g_rand_double (dither_rand) < dither_prob)
                            color.r += (1.0 / 255.0);

                          ftmp = color.g * 255.0;
                          itmp = ftmp;
                          dither_prob = ftmp - itmp;

                          if (g_rand_double (dither_rand) < dither_prob)
                            color.g += (1.0 / 255.0);

                          ftmp = color.b * 255.0;
                          itmp = ftmp;
                          dither_prob = ftmp - itmp;

                          if (g_rand_double (dither_rand) < dither_prob)
                            color.b += (1.0 / 255.0);

                          ftmp = color.a * 255.0;
                          itmp = ftmp;
                          dither_prob = ftmp - itmp;

                          if (g_rand_double (dither_rand) < dither_prob)
                            color.a += (1.0 / 255.0);

                          if (color.r > 1.0) color.r = 1.0;
                          if (color.g > 1.0) color.g = 1.0;
                          if (color.b > 1.0) color.b = 1.0;
                          if (color.a > 1.0) color.a = 1.0;
                        }

                      *data++ = color.r * 255.0;
                      *data++ = color.g * 255.0;
                      *data++ = color.b * 255.0;
                      *data++ = color.a * 255.0;
                    }
                  else
                    {
                      /* Convert to grayscale */
                      gdouble gray = gimp_rgb_intensity (&color);

		      if (dither)
                        {
			  gdouble dither_prob;
                          gdouble ftmp;
                          gint    itmp;

                          ftmp = gray * 255.0;
                          itmp = ftmp;
                          dither_prob = ftmp - itmp;

                          if (g_rand_double (dither_rand) < dither_prob)
                            gray += (1.0 / 255.0);

                          ftmp = color.a * 255.0;
                          itmp = ftmp;
                          dither_prob = ftmp - itmp;

                          if (g_rand_double (dither_rand) < dither_prob)
                            color.a += (1.0 / 255.0);

                          if (gray > 1.0)    gray    = 1.0;
                          if (color.a > 1.0) color.a = 1.0;
                        }

                      *data++ = gray * 255.0;
                      *data++ = color.a * 255.0;
                    }
                }
            }

          if (progress)
            {
              curr_progress += PR->w * PR->h;

              gimp_progress_set_value (progress,
                                       (gdouble) curr_progress /
                                       (gdouble) max_progress);
            }
        }
    }

  if (dither)
    g_rand_free (dither_rand);
}
