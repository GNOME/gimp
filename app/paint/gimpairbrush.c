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

#include "paint-types.h"

#include "base/temp-buf.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"

#include "gimpairbrush.h"


#define AIRBRUSH_DEFAULT_RATE     80.0
#define AIRBRUSH_DEFAULT_PRESSURE 10.0


typedef struct _AirbrushTimeout AirbrushTimeout;

struct _AirbrushTimeout
{
  GimpPaintCore    *paint_core;
  GimpDrawable     *drawable;
  GimpPaintOptions *paint_options;
};


static void       gimp_airbrush_class_init (GimpAirbrushClass  *klass);
static void       gimp_airbrush_init       (GimpAirbrush       *airbrush);

static void       gimp_airbrush_finalize   (GObject            *object);

static void       gimp_airbrush_paint      (GimpPaintCore      *paint_core,
                                            GimpDrawable       *drawable,
                                            GimpPaintOptions   *paint_options,
                                            GimpPaintCoreState  paint_state);

static void       gimp_airbrush_motion     (GimpPaintCore      *paint_core,
                                            GimpDrawable       *drawable,
                                            GimpPaintOptions   *paint_options);
static gboolean   gimp_airbrush_timeout    (gpointer            data);


static guint               timeout_id = 0;
static AirbrushTimeout     airbrush_timeout;

static GimpPaintCoreClass *parent_class = NULL;


void
gimp_airbrush_register (Gimp                      *gimp,
                        GimpPaintRegisterCallback  callback)
{
  (* callback) (gimp, GIMP_TYPE_AIRBRUSH);
}

GType
gimp_airbrush_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpAirbrushClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_airbrush_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpAirbrush),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_airbrush_init,
      };

      type = g_type_register_static (GIMP_TYPE_PAINT_CORE,
                                     "GimpAirbrush",
                                     &info, 0);
    }

  return type;
}

static void 
gimp_airbrush_class_init (GimpAirbrushClass *klass)
{
  GObjectClass       *object_class;
  GimpPaintCoreClass *paint_core_class;

  object_class     = G_OBJECT_CLASS (klass);
  paint_core_class = GIMP_PAINT_CORE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize  = gimp_airbrush_finalize;

  paint_core_class->paint = gimp_airbrush_paint;
}

static void
gimp_airbrush_init (GimpAirbrush *airbrush)
{
  GimpPaintCore *paint_core;

  paint_core = GIMP_PAINT_CORE (airbrush);

  paint_core->flags |= CORE_HANDLES_CHANGING_BRUSH;
}

static void
gimp_airbrush_finalize (GObject *object)
{
  if (timeout_id)
    {
      g_source_remove (timeout_id);
      timeout_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_airbrush_paint (GimpPaintCore      *paint_core,
                     GimpDrawable       *drawable,
                     GimpPaintOptions   *paint_options,
                     GimpPaintCoreState  paint_state)
{
  GimpAirbrushOptions *options;

  options = (GimpAirbrushOptions *) paint_options;

  switch (paint_state)
    {
    case INIT_PAINT:
      if (timeout_id)
	{
	  g_warning ("killing stray timer, please report to lewing@gimp.org");
	  g_source_remove (timeout_id);
	  timeout_id = 0;
	}
      break;

    case MOTION_PAINT:
      if (timeout_id)
	{
	  g_source_remove (timeout_id);
	  timeout_id = 0;
	}

      gimp_airbrush_motion (paint_core,
                            drawable,
                            paint_options);

      if (options->rate != 0.0)
	{
	  gdouble timeout;

	  airbrush_timeout.paint_core    = paint_core;
	  airbrush_timeout.drawable      = drawable;
          airbrush_timeout.paint_options = paint_options;

	  timeout = (paint_options->pressure_options->rate ? 
		     (10000 / (options->rate * 2.0 * paint_core->cur_coords.pressure)) : 
		     (10000 / options->rate));

	  timeout_id = g_timeout_add (timeout,
                                      gimp_airbrush_timeout,
                                      NULL);
	}
      break;

    case FINISH_PAINT:
      if (timeout_id)
	{
	  g_source_remove (timeout_id);
	  timeout_id = 0;
	}
      break;

    default:
      break;
    }
}

static gboolean
gimp_airbrush_timeout (gpointer client_data)
{
  gdouble rate;

  gimp_airbrush_motion (airbrush_timeout.paint_core,
                        airbrush_timeout.drawable,
                        airbrush_timeout.paint_options);

#ifdef __GNUC__
#warning: FIXME: gdisplays_flush()
#endif
  gdisplays_flush ();

  rate = ((GimpAirbrushOptions *) airbrush_timeout.paint_options)->rate;

  /*  restart the timer  */
  if (rate != 0.0)
    {
      if (airbrush_timeout.paint_options->pressure_options->rate)
	{
          if (timeout_id)
            g_source_remove (timeout_id);

	  timeout_id = g_timeout_add ((10000 /
                                       (rate * 2.0 *
                                        airbrush_timeout.paint_core->cur_coords.pressure)), 
                                      gimp_airbrush_timeout,
                                      NULL);
	  return FALSE;
	}

      return TRUE;
    }

  return FALSE;
}

static void
gimp_airbrush_motion (GimpPaintCore    *paint_core,
                      GimpDrawable     *drawable,
                      GimpPaintOptions *paint_options)
{
  GimpImage                *gimage;
  GimpContext              *context;
  TempBuf                  *area;
  guchar                    col[MAX_CHANNELS];
  gdouble                   scale;
  GimpPaintApplicationMode  paint_appl_mode;
  gdouble                   pressure;

  if (! (gimage = gimp_item_get_image (GIMP_ITEM (drawable))))
    return;

  context = gimp_get_current_context (gimage->gimp);

  paint_appl_mode = (paint_options->incremental ? 
                     GIMP_PAINT_INCREMENTAL : GIMP_PAINT_CONSTANT);

  pressure = ((GimpAirbrushOptions *) paint_options)->pressure / 100.0;

  if (paint_options->pressure_options->size)
    scale = paint_core->cur_coords.pressure;
  else
    scale = 1.0;

  if (! (area = gimp_paint_core_get_paint_area (paint_core, drawable, scale)))
    return;

  /*  color the pixels  */
  if (paint_options->pressure_options->color)
    {
      GimpRGB  color;

      gimp_gradient_get_color_at (gimp_context_get_gradient (context),
				  paint_core->cur_coords.pressure, &color);

      gimp_rgba_get_uchar (&color,
			   &col[RED_PIX],
			   &col[GREEN_PIX],
			   &col[BLUE_PIX],
			   &col[ALPHA_PIX]);

      paint_appl_mode = GIMP_PAINT_INCREMENTAL;

      color_pixels (temp_buf_data (area), col,
		    area->width * area->height,
                    area->bytes);
    }
  else if (paint_core->brush && paint_core->brush->pixmap)
    {
      paint_appl_mode = GIMP_PAINT_INCREMENTAL;

      gimp_paint_core_color_area_with_pixmap (paint_core, gimage,
					      drawable, area, 
					      scale, GIMP_BRUSH_SOFT);
    }
  else
    {
      gimp_image_get_foreground (gimage, drawable, col);
      col[area->bytes - 1] = OPAQUE_OPACITY;
      color_pixels (temp_buf_data (area), col,
		    area->width * area->height, area->bytes);
    }

  if (paint_options->pressure_options->pressure)
    pressure = pressure * 2.0 * paint_core->cur_coords.pressure;

  /*  paste the newly painted area to the image  */
  gimp_paint_core_paste_canvas (paint_core, drawable,
				MIN (pressure, GIMP_OPACITY_OPAQUE),
				gimp_context_get_opacity (context),
				gimp_context_get_paint_mode (context),
				GIMP_BRUSH_SOFT, scale, paint_appl_mode);
}


/*  paint options stuff  */

GimpAirbrushOptions *
gimp_airbrush_options_new (void)
{
  GimpAirbrushOptions *options;

  options = g_new0 (GimpAirbrushOptions, 1);

  gimp_paint_options_init ((GimpPaintOptions *) options);

  options->rate     = options->rate_d     = AIRBRUSH_DEFAULT_RATE;
  options->pressure = options->pressure_d = AIRBRUSH_DEFAULT_PRESSURE;

  return options;
}
