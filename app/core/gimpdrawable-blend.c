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
#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpdrawable.h"
#include "gimpdrawable-blend.h"
#include "gimpgradient.h"
#include "gimpimage.h"
#include "gimpimage-mask.h"


typedef gdouble (* RepeatFunc) (gdouble);


typedef struct
{
  GimpGradient *gradient;
  gdouble       offset;
  gdouble       sx, sy;
  BlendMode     blend_mode;
  GradientType  gradient_type;
  GimpRGB       fg, bg;
  gdouble       dist;
  gdouble       vec[2];
  RepeatFunc    repeat_func;
} RenderBlendData;

typedef struct
{
  PixelRegion *PR;
  guchar      *row_data;
  gint         bytes;
  gint         width;
} PutPixelData;


/*  local function prototypes  */

static gdouble gradient_calc_conical_sym_factor  (gdouble          dist,
						  gdouble         *axis,
						  gdouble          offset,
						  gdouble          x,
						  gdouble          y);
static gdouble gradient_calc_conical_asym_factor (gdouble          dist,
						  gdouble         *axis,
						  gdouble          offset,
						  gdouble          x,
						  gdouble          y);
static gdouble gradient_calc_square_factor       (gdouble          dist,
						  gdouble          offset,
						  gdouble          x,
						  gdouble          y);
static gdouble gradient_calc_radial_factor   	 (gdouble          dist,
						  gdouble          offset,
						  gdouble          x,
						  gdouble          y);
static gdouble gradient_calc_linear_factor   	 (gdouble          dist,
						  gdouble         *vec,
						  gdouble          offset,
						  gdouble          x,
						  gdouble          y);
static gdouble gradient_calc_bilinear_factor 	 (gdouble          dist,
						  gdouble         *vec,
						  gdouble          offset,
						  gdouble          x,
						  gdouble          y);
static gdouble gradient_calc_spiral_factor       (gdouble          dist,
						  gdouble         *axis,
						  gdouble          offset,
						  gdouble          x,
						  gdouble          y,
						  gint             cwise);

static gdouble gradient_calc_shapeburst_angular_factor   (gdouble x,
							  gdouble y);
static gdouble gradient_calc_shapeburst_spherical_factor (gdouble x,
							  gdouble y);
static gdouble gradient_calc_shapeburst_dimpled_factor   (gdouble x,
							  gdouble y);

static gdouble gradient_repeat_none              (gdouble       val);
static gdouble gradient_repeat_sawtooth          (gdouble       val);
static gdouble gradient_repeat_triangular        (gdouble       val);

static void    gradient_precalc_shapeburst       (GimpImage    *gimage,
						  GimpDrawable *drawable, 
						  PixelRegion  *PR,
						  gdouble       dist);

static void    gradient_render_pixel             (gdouble       x,
						  gdouble       y,
						  GimpRGB      *color,
						  gpointer      render_data);
static void    gradient_put_pixel                (gint          x,
						  gint          y,
						  GimpRGB      *color,
						  gpointer      put_pixel_data);

static void    gradient_fill_region              (GimpImage    *gimage,
						  GimpDrawable *drawable,
						  PixelRegion  *PR,
						  gint          width,
						  gint          height,
						  BlendMode     blend_mode,
						  GradientType  gradient_type,
						  gdouble       offset,
						  RepeatMode    repeat,
						  gint          supersample,
						  gint          max_depth,
						  gdouble       threshold,
						  gdouble       sx,
						  gdouble       sy,
						  gdouble       ex,
						  gdouble       ey,
						  GimpProgressFunc progress_callback,
						  gpointer      progress_data);


/*  variables for the shapeburst algs  */

static PixelRegion distR =
{
  NULL,  /* data */
  NULL,  /* tiles */
  0,     /* rowstride */
  0, 0,  /* w, h */
  0, 0,  /* x, y */
  4,     /* bytes */
  0      /* process count */
};


/*  public functions  */

void
gimp_drawable_blend (GimpDrawable     *drawable,
                     BlendMode         blend_mode,
                     int               paint_mode,
                     GradientType      gradient_type,
                     gdouble           opacity,
                     gdouble           offset,
                     RepeatMode        repeat,
                     gint              supersample,
                     gint              max_depth,
                     gdouble           threshold,
                     gdouble           startx,
                     gdouble           starty,
                     gdouble           endx,
                     gdouble           endy,
                     GimpProgressFunc  progress_callback,
                     gpointer          progress_data)
{
  GimpImage   *gimage;
  TileManager *buf_tiles;
  PixelRegion  bufPR;
  gint         has_alpha;
  gint         has_selection;
  gint         bytes;
  gint         x1, y1, x2, y2;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gradient_type >= LINEAR &&
                    gradient_type <= SPIRAL_ANTICLOCKWISE);
  g_return_if_fail (blend_mode >= FG_BG_RGB_MODE && blend_mode <= CUSTOM_MODE);
  g_return_if_fail (repeat >= REPEAT_NONE && repeat <= REPEAT_TRIANGULAR);

  gimage = gimp_drawable_gimage (drawable);

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  gimp_set_busy (gimage->gimp);

  has_selection = gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  has_alpha = gimp_drawable_has_alpha (drawable);
  bytes     = gimp_drawable_bytes (drawable);

  /*  Always create an alpha temp buf (for generality) */
  if (! has_alpha)
    {
      has_alpha = TRUE;
      bytes += 1;
    }

  buf_tiles = tile_manager_new ((x2 - x1), (y2 - y1), bytes);
  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), TRUE);

  gradient_fill_region (gimage, drawable,
  			&bufPR, (x2 - x1), (y2 - y1),
			blend_mode, gradient_type, offset, repeat,
			supersample, max_depth, threshold,
			(startx - x1), (starty - y1),
			(endx - x1), (endy - y1),
			progress_callback, progress_data);

  if (distR.tiles)
    {
      tile_manager_destroy (distR.tiles);
      distR.tiles = NULL;
    }

  pixel_region_init (&bufPR, buf_tiles, 0, 0, (x2 - x1), (y2 - y1), FALSE);
  gimp_image_apply_image (gimage, drawable, &bufPR, TRUE,
			  opacity * 255, paint_mode,
                          NULL, x1, y1);

  /*  update the image  */
  gimp_drawable_update (drawable, x1, y1, (x2 - x1), (y2 - y1));

  /*  free the temporary buffer  */
  tile_manager_destroy (buf_tiles);

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
      rat = pow (rat, (offset / 10) + 1);

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
	  ang0 = atan2(axis[0], axis[1]) + G_PI;
	  ang1 = atan2(x, y) + G_PI;

	  ang = ang1 - ang0;

	  if (ang < 0.0)
	    ang += (2.0 * G_PI);

	  rat = ang / (2.0 * G_PI);
	  rat = pow(rat, (offset / 10) + 1);

	  rat = CLAMP(rat, 0.0, 1.0);
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
      else if (offset == 1)
        rat = (rat>=1) ? 1 : 0;
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

      r   = sqrt(SQR(x) + SQR(y));
      rat = r / dist;

      if (rat < offset)
	rat = 0.0;
      else if (offset == 1)
        rat = (rat>=1) ? 1 : 0;
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
  double r;
  double rat;

  if (dist == 0.0)
    {
      rat = 0.0;
    }
  else
    {
      offset = offset / 100.0;

      r   = vec[0] * x + vec[1] * y;
      rat = r / dist;

      if (rat < offset)
	rat = 0.0;
      else if (offset == 1)
        rat = (rat>=1) ? 1 : 0;
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

      if (fabs(rat) < offset)
	rat = 0.0;
      else if (offset == 1)
        rat = (rat>=1) ? 1 : 0;
      else
	rat = (fabs(rat) - offset) / (1.0 - offset);
    }

  return rat;
}

static gdouble
gradient_calc_spiral_factor (gdouble  dist,
			     gdouble *axis,
			     gdouble  offset,
			     gdouble  x,
			     gdouble  y,
			     gint     cwise)
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
	  if(!cwise)
	    ang = ang0 - ang1;
	  else
	    ang = ang1 - ang0;

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

  ix = (int) CLAMP (x, 0, distR.w);
  iy = (int) CLAMP (y, 0, distR.h);
  tile = tile_manager_get_tile (distR.tiles, ix, iy, TRUE, FALSE);
  value = 1.0 - *((float *) tile_data_pointer (tile, ix % TILE_WIDTH, iy % TILE_HEIGHT));
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

  ix = (int) CLAMP (x, 0, distR.w);
  iy = (int) CLAMP (y, 0, distR.h);
  tile = tile_manager_get_tile (distR.tiles, ix, iy, TRUE, FALSE);
  value = *((gfloat *) tile_data_pointer (tile, ix % TILE_WIDTH, iy % TILE_HEIGHT));
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

  ix = (int) CLAMP (x, 0, distR.w);
  iy = (int) CLAMP (y, 0, distR.h);
  tile = tile_manager_get_tile (distR.tiles, ix, iy, TRUE, FALSE);
  value = *((float *) tile_data_pointer (tile, ix % TILE_WIDTH, iy % TILE_HEIGHT));
  value = cos (0.5 * G_PI * value);
  tile_release (tile, FALSE);

  return value;
}

static gdouble
gradient_repeat_none (gdouble val)
{
  return CLAMP (val, 0.0, 1.0);
}

static gdouble
gradient_repeat_sawtooth (gdouble val)
{
  if (val >= 0.0)
    return fmod (val, 1.0);
  else
    return 1.0 - fmod (-val, 1.0);
}

static gdouble
gradient_repeat_triangular (gdouble val)
{
  gint ival;

  if (val < 0.0)
    val = -val;

  ival = (int) val;

  if (ival & 1)
    return 1.0 - fmod (val, 1.0);
  else
    return fmod (val, 1.0);
}

/*****/
static void
gradient_precalc_shapeburst (GimpImage    *gimage,
			     GimpDrawable *drawable,
			     PixelRegion  *PR,
			     gdouble       dist)
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

  /*  If the gimage mask is not empty, use it as the shape burst source  */
  if (! gimp_image_mask_is_empty (gimage))
    {
      PixelRegion maskR;
      gint        x1, y1, x2, y2;
      gint        offx, offy;

      gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);
      gimp_drawable_offsets (drawable, &offx, &offy);

      /*  the selection mask  */
      mask = gimp_image_get_mask (gimage);
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
  max_iteration = shapeburst_region (&tempR, &distR);

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

  tile_manager_destroy (tempR.tiles);
}


static void
gradient_render_pixel (double    x, 
		       double    y, 
		       GimpRGB  *color, 
		       gpointer  render_data)
{
  RenderBlendData *rbd;
  gdouble          factor;

  rbd = render_data;

  /* Calculate blending factor */

  switch (rbd->gradient_type)
    {
    case LINEAR:
      factor = gradient_calc_linear_factor (rbd->dist, rbd->vec, rbd->offset,
					    x - rbd->sx, y - rbd->sy);
      break;

    case BILINEAR:
      factor = gradient_calc_bilinear_factor (rbd->dist, rbd->vec, rbd->offset,
					      x - rbd->sx, y - rbd->sy);
      break;

    case RADIAL:
      factor = gradient_calc_radial_factor (rbd->dist, rbd->offset,
					    x - rbd->sx, y - rbd->sy);
      break;

    case SQUARE:
      factor = gradient_calc_square_factor (rbd->dist, rbd->offset,
					    x - rbd->sx, y - rbd->sy);
      break;

    case CONICAL_SYMMETRIC:
      factor = gradient_calc_conical_sym_factor (rbd->dist, rbd->vec, rbd->offset,
						 x - rbd->sx, y - rbd->sy);
      break;

    case CONICAL_ASYMMETRIC:
      factor = gradient_calc_conical_asym_factor (rbd->dist, rbd->vec, rbd->offset,
						  x - rbd->sx, y - rbd->sy);
      break;

    case SHAPEBURST_ANGULAR:
      factor = gradient_calc_shapeburst_angular_factor (x, y);
      break;

    case SHAPEBURST_SPHERICAL:
      factor = gradient_calc_shapeburst_spherical_factor (x, y);
      break;

    case SHAPEBURST_DIMPLED:
      factor = gradient_calc_shapeburst_dimpled_factor (x, y);
      break;

    case SPIRAL_CLOCKWISE:
      factor = gradient_calc_spiral_factor (rbd->dist, rbd->vec, rbd->offset,
					    x - rbd->sx, y - rbd->sy,TRUE);
      break;

    case SPIRAL_ANTICLOCKWISE:
      factor = gradient_calc_spiral_factor (rbd->dist, rbd->vec, rbd->offset,
					    x - rbd->sx, y - rbd->sy,FALSE);
      break;

    default:
      g_assert_not_reached ();
      return;
    }

  /* Adjust for repeat */

  factor = (*rbd->repeat_func)(factor);

  /* Blend the colors */

  if (rbd->blend_mode == CUSTOM_MODE)
    {
      gimp_gradient_get_color_at (rbd->gradient, factor, color);
    }
  else
    {
      /* Blend values */

      color->r = rbd->fg.r + (rbd->bg.r - rbd->fg.r) * factor;
      color->g = rbd->fg.g + (rbd->bg.g - rbd->fg.g) * factor;
      color->b = rbd->fg.b + (rbd->bg.b - rbd->fg.b) * factor;
      color->a = rbd->fg.a + (rbd->bg.a - rbd->fg.a) * factor;

      if (rbd->blend_mode == FG_BG_HSV_MODE)
	gimp_hsv_to_rgb_double (&color->r, &color->g, &color->b);
    }
}

static void
gradient_put_pixel (int      x, 
		    int      y, 
		    GimpRGB *color, 
		    void    *put_pixel_data)
{
  PutPixelData  *ppd;
  guchar        *data;

  ppd = put_pixel_data;

  /* Paint */

  data = ppd->row_data + ppd->bytes * x;

  if (ppd->bytes >= 3)
    {
      *data++ = color->r * 255.0;
      *data++ = color->g * 255.0;
      *data++ = color->b * 255.0;
      *data++ = color->a * 255.0;
    }
  else
    {
      /* Convert to grayscale */

      *data++ = 255.0 * INTENSITY (color->r, color->g, color->b);
      *data++ = color->a * 255.0;
    }

  /* Paint whole row if we are on the rightmost pixel */

  if (x == (ppd->width - 1))
    pixel_region_set_row(ppd->PR, 0, y, ppd->width, ppd->row_data);
}

static void
gradient_fill_region (GimpImage        *gimage,
		      GimpDrawable     *drawable,
		      PixelRegion      *PR,
		      gint              width,
		      gint              height,
		      BlendMode         blend_mode,
		      GradientType      gradient_type,
		      gdouble           offset,
		      RepeatMode        repeat,
		      gint              supersample,
		      gint              max_depth,
		      gdouble           threshold,
		      gdouble           sx,
		      gdouble           sy,
		      gdouble           ex,
		      gdouble           ey,
		      GimpProgressFunc  progress_callback,
		      gpointer          progress_data)
{
  RenderBlendData  rbd;
  PutPixelData     ppd;
  gint             x, y;
  gint             endx, endy;
  gpointer         pr;
  guchar          *data;
  GimpRGB          color;
  GimpContext     *context;

  context = gimp_get_current_context (gimage->gimp);

  rbd.gradient = gimp_context_get_gradient (context);

  /* Get foreground and background colors, normalized */

  gimp_context_get_foreground (context, &rbd.fg);

  /* rbd.fg.a = 1.0; */ /* Foreground is always opaque */

  gimp_context_get_background (context, &rbd.bg);

  /* rbd.bg.a = 1.0; */ /* opaque, for now */

  switch (blend_mode)
    {
    case FG_BG_RGB_MODE:
      break;

    case FG_BG_HSV_MODE:
      /* Convert to HSV */

      gimp_rgb_to_hsv_double (&rbd.fg.r, &rbd.fg.g, &rbd.fg.b);
      gimp_rgb_to_hsv_double (&rbd.bg.r, &rbd.bg.g, &rbd.bg.b);

      break;

    case FG_TRANS_MODE:
      /* Color does not change, just the opacity */

      rbd.bg   = rbd.fg;
      rbd.bg.a = 0.0; /* transparent */

      break;

    case CUSTOM_MODE:
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  /* Calculate type-specific parameters */

  switch (gradient_type)
    {
    case RADIAL:
      rbd.dist = sqrt(SQR(ex - sx) + SQR(ey - sy));
      break;

    case SQUARE:
      rbd.dist = MAX (fabs (ex - sx), fabs (ey - sy));
      break;

    case CONICAL_SYMMETRIC:
    case CONICAL_ASYMMETRIC:
    case SPIRAL_CLOCKWISE:
    case SPIRAL_ANTICLOCKWISE:
    case LINEAR:
    case BILINEAR:
      rbd.dist = sqrt (SQR (ex - sx) + SQR (ey - sy));

      if (rbd.dist > 0.0)
	{
	  rbd.vec[0] = (ex - sx) / rbd.dist;
	  rbd.vec[1] = (ey - sy) / rbd.dist;
	}

      break;

    case SHAPEBURST_ANGULAR:
    case SHAPEBURST_SPHERICAL:
    case SHAPEBURST_DIMPLED:
      rbd.dist = sqrt (SQR (ex - sx) + SQR (ey - sy));
      gradient_precalc_shapeburst (gimage, drawable, PR, rbd.dist);
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  /* Set repeat function */

  switch (repeat)
    {
    case REPEAT_NONE:
      rbd.repeat_func = gradient_repeat_none;
      break;

    case REPEAT_SAWTOOTH:
      rbd.repeat_func = gradient_repeat_sawtooth;
      break;

    case REPEAT_TRIANGULAR:
      rbd.repeat_func = gradient_repeat_triangular;
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

  /* Render the gradient! */

  if (supersample)
    {
      /* Initialize put pixel data */

      ppd.PR       = PR;
      ppd.row_data = g_malloc (width * PR->bytes);
      ppd.bytes    = PR->bytes;
      ppd.width    = width;

      /* Render! */

      gimp_adaptive_supersample_area (0, 0, (width - 1), (height - 1),
				      max_depth, threshold,
				      gradient_render_pixel, &rbd,
				      gradient_put_pixel, &ppd,
				      progress_callback, progress_data);

      /* Clean up */

      g_free (ppd.row_data);
    }
  else
    {
      gint max_progress = PR->w * PR->h;
      gint progress = 0;

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
                      *data++ = color.r * 255.0;
                      *data++ = color.g * 255.0;
                      *data++ = color.b * 255.0;
                      *data++ = color.a * 255.0;
                    }
                  else
                    {
                      /* Convert to grayscale */

                      *data++ = 255.0 * INTENSITY (color.r, color.g, color.b);
                      *data++ = color.a * 255.0;
                    }
                }
            }

	  progress += PR->w * PR->h;
	  if (progress_callback)
	    (* progress_callback) (0, max_progress, progress, progress_data);
	}
    }
}
