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

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "paint-types.h"

#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"

#include "gimppaintbrush.h"
#include "gimppaintoptions.h"


static void   gimp_paintbrush_class_init (GimpPaintbrushClass *klass);
static void   gimp_paintbrush_init       (GimpPaintbrush      *paintbrush);

static void   gimp_paintbrush_paint      (GimpPaintCore       *paint_core,
                                          GimpDrawable        *drawable,
                                          GimpPaintOptions    *paint_options,
                                          GimpPaintCoreState   paint_state);

static void   gimp_paintbrush_motion     (GimpPaintCore       *paint_core,
                                          GimpDrawable        *drawable,
                                          GimpPressureOptions *pressure_options,
                                          GimpGradientOptions *gradient_options,
                                          gdouble              fade_out,
                                          gdouble              gradient_length,
                                          gboolean             incremental,
                                          GradientPaintMode    gradient_type);


static GimpPaintCoreClass *parent_class = NULL;


GType
gimp_paintbrush_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpPaintbrushClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_paintbrush_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpPaintbrush),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_paintbrush_init,
      };

      type = g_type_register_static (GIMP_TYPE_PAINT_CORE,
                                     "GimpPaintbrush",
                                     &info, 0);
    }

  return type;
}

static void
gimp_paintbrush_class_init (GimpPaintbrushClass *klass)
{
  GimpPaintCoreClass *paint_core_class;

  paint_core_class = GIMP_PAINT_CORE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  paint_core_class->paint = gimp_paintbrush_paint;
}

static void
gimp_paintbrush_init (GimpPaintbrush *paintbrush)
{
  GimpPaintCore *paint_core;

  paint_core = GIMP_PAINT_CORE (paintbrush);

  paint_core->flags |= CORE_HANDLES_CHANGING_BRUSH;
}

static void
gimp_paintbrush_paint (GimpPaintCore      *paint_core,
                       GimpDrawable       *drawable,
                       GimpPaintOptions   *paint_options,
                       GimpPaintCoreState  paint_state)
{
  GimpPressureOptions *pressure_options;
  GimpGradientOptions *gradient_options;
  gboolean             incremental;
  GimpImage           *gimage;
  gdouble              fade_out;
  gdouble              gradient_length;
  gdouble              unit_factor;

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  pressure_options = paint_options->pressure_options;
  gradient_options = paint_options->gradient_options;
  incremental      = paint_options->incremental;

  switch (paint_state)
    {
    case INIT_PAINT:
      break;

    case MOTION_PAINT:
      switch (gradient_options->fade_unit)
	{
	case GIMP_UNIT_PIXEL:
	  fade_out = gradient_options->fade_out;
	  break;
	case GIMP_UNIT_PERCENT:
	  fade_out = (MAX (gimage->width, gimage->height) *
		      gradient_options->fade_out / 100);
	  break;
	default:
	  unit_factor = gimp_unit_get_factor (gradient_options->fade_unit);
	  fade_out = (gradient_options->fade_out *
		      MAX (gimage->xresolution,
			   gimage->yresolution) / unit_factor);
	  break;
	}

      switch (gradient_options->gradient_unit)
	{
	case GIMP_UNIT_PIXEL:
	  gradient_length = gradient_options->gradient_length;
	  break;
	case GIMP_UNIT_PERCENT:
	  gradient_length = (MAX (gimage->width, gimage->height) *
			     gradient_options->gradient_length / 100);
	  break;
	default:
	  unit_factor = gimp_unit_get_factor (gradient_options->gradient_unit);
	  gradient_length = (gradient_options->gradient_length *
			     MAX (gimage->xresolution,
				  gimage->yresolution) / unit_factor);
	  break;
	}

      gimp_paintbrush_motion (paint_core, drawable,
                              pressure_options,
                              gradient_options,
                              gradient_options->use_fade ? fade_out : 0,
                              gradient_options->use_gradient ? gradient_length : 0,
                              incremental,
                              gradient_options->gradient_type);
      break;

    case FINISH_PAINT:
      break;

    default:
      break;
    }
}

static void
gimp_paintbrush_motion (GimpPaintCore       *paint_core,
                        GimpDrawable        *drawable,
                        GimpPressureOptions *pressure_options,
                        GimpGradientOptions *gradient_options,
                        gdouble              fade_out,
                        gdouble              gradient_length,
                        gboolean             incremental,
                        GradientPaintMode    gradient_type)
{
  GimpImage            *gimage;
  GimpContext          *context;
  TempBuf              *area;
  gdouble               x, paint_left;
  guchar                local_blend = OPAQUE_OPACITY;
  guchar                temp_blend  = OPAQUE_OPACITY;
  guchar                col[MAX_CHANNELS];
  GimpRGB               color;
  gint                  mode;
  gint                  opacity;
  gdouble               scale;
  PaintApplicationMode  paint_appl_mode = incremental ? INCREMENTAL : CONSTANT;

  gimage = gimp_item_get_image (GIMP_ITEM (drawable));

  context = gimp_get_current_context (gimage->gimp);

  if (pressure_options->size)
    scale = paint_core->cur_coords.pressure;
  else
    scale = 1.0;

  if (pressure_options->color)
    gradient_length = 1.0; /* not really used, only for if cases */

  /*  Get a region which can be used to paint to  */
  if (! (area = gimp_paint_core_get_paint_area (paint_core, drawable, scale)))
    return;

  /*  factor in the fade out value  */
  if (fade_out)
    {
      /*  Model the amount of paint left as a gaussian curve  */
      x = ((double) paint_core->pixel_dist / fade_out);
      paint_left = exp (- x * x * 5.541);    /*  ln (1/255)  */
      local_blend = (int) (255 * paint_left);
    }

  if (local_blend)
    {
      /*  set the alpha channel  */
      temp_blend = local_blend;
      mode = gradient_type;

      if (gradient_length)
	{
          GimpGradient *gradient;

          gradient = gimp_context_get_gradient (context);

	  if (pressure_options->color)
	    gimp_gradient_get_color_at (gradient,
					paint_core->cur_coords.pressure,
                                        &color);
	  else
	    gimp_paint_core_get_color_from_gradient (paint_core,
                                                     gradient,
                                                     gradient_length,
						     &color,
                                                     mode);

	  temp_blend = (gint) ((color.a * local_blend));

	  gimp_rgb_get_uchar (&color,
			      &col[RED_PIX],
			      &col[GREEN_PIX],
			      &col[BLUE_PIX]);
	  col[ALPHA_PIX] = OPAQUE_OPACITY;

	  /* always use incremental mode with gradients */
	  /* make the gui cool later */
	  paint_appl_mode = INCREMENTAL;
	  color_pixels (temp_buf_data (area), col,
			area->width * area->height, area->bytes);
	}
      /* we check to see if this is a pixmap, if so composite the
       * pixmap image into the area instead of the color
       */
      else if (paint_core->brush && paint_core->brush->pixmap)
	{
	  gimp_paint_core_color_area_with_pixmap (paint_core, gimage, drawable,
						  area,
						  scale, SOFT);
	  paint_appl_mode = INCREMENTAL;
	}
      else
	{
	  gimp_image_get_foreground (gimage, drawable, col);
	  col[area->bytes - 1] = OPAQUE_OPACITY;
	  color_pixels (temp_buf_data (area), col,
			area->width * area->height, area->bytes);
	}

      opacity = (gdouble) temp_blend;

      if (pressure_options->opacity)
	opacity = opacity * 2.0 * paint_core->cur_coords.pressure;

      gimp_paint_core_paste_canvas (paint_core, drawable,
				    MIN (opacity, 255),
				    gimp_context_get_opacity (context) * 255,
				    gimp_context_get_paint_mode (context),
				    pressure_options->pressure ? PRESSURE : SOFT,
				    scale, paint_appl_mode);
    }
}
