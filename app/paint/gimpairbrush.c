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
#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimpbrush.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"

#include "drawable.h"
#include "gdisplay.h"
#include "selection.h"
#include "temp_buf.h"

#include "gimpairbrushtool.h"
#include "paint_options.h"
#include "gimptool.h"
#include "tool_manager.h"

#include "libgimp/gimpintl.h"

#include "pixmaps2.h"


/*  The maximum amount of pressure that can be exerted  */
#define MAX_PRESSURE  0.075

/* Default pressure setting */
#define AIRBRUSH_DEFAULT_RATE        0.0
#define AIRBRUSH_DEFAULT_PRESSURE    10.0
#define AIRBRUSH_DEFAULT_INCREMENTAL FALSE

#define OFF 0
#define ON  1

/*  the airbrush structures  */

typedef struct _AirbrushTimeout AirbrushTimeout;

struct _AirbrushTimeout
{
  GimpPaintTool *paint_tool;
  GimpDrawable  *drawable;
};

typedef struct _AirbrushOptions AirbrushOptions;

struct _AirbrushOptions
{
  PaintOptions paint_options;

  gdouble      rate;
  gdouble      rate_d;
  GtkObject   *rate_w;

  gdouble      pressure;
  gdouble      pressure_d;
  GtkObject   *pressure_w;
};


/*  local function prototypes  */

static void   gimp_airbrush_tool_class_init (GimpAirbrushToolClass *klass);
static void   gimp_airbrush_tool_init       (GimpAirbrushTool      *airbrush);

static void   gimp_airbrush_tool_destroy    (GtkObject             *object);

static void   gimp_airbrush_tool_paint      (GimpPaintTool         *paint_tool,
					     GimpDrawable          *drawable,
					     PaintState             state);
static void   gimp_airbrush_tool_motion     (GimpPaintTool         *paint_tool,
					     GimpDrawable          *drawable,
					     PaintPressureOptions  *pressure_options,
					     gdouble                pressure,
					     gboolean               incremental);
static gint   airbrush_time_out             (gpointer               data);

static AirbrushOptions * airbrush_options_new   (void);
static void              airbrush_options_reset (ToolOptions *tool_options);


/*  local variables  */
static gint             timer;  /*  timer for successive paint applications  */
static gint             timer_state = OFF;       /*  state of airbrush tool  */
static AirbrushTimeout  airbrush_timeout;

static gdouble          non_gui_rate;
static gdouble          non_gui_pressure;
static gboolean         non_gui_incremental;

static AirbrushOptions *airbrush_options = NULL; 

static GimpPaintToolClass *parent_class = NULL;


/*  functions  */

void
gimp_airbrush_tool_register (void)
{
  tool_manager_register_tool (GIMP_TYPE_AIRBRUSH_TOOL,
                              TRUE,
                              "gimp:airbrush_tool",
                              _("Airbrush"),
                              _("Airbrush with variable pressure"),
                              N_("/Tools/Paint Tools/Airbrush"), "A",
                              NULL, "tools/airbrush.html",
                              (const gchar **) airbrush_bits);
}

GtkType
gimp_airbrush_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
        "GimpAirbrushTool",
        sizeof (GimpAirbrushTool),
        sizeof (GimpAirbrushToolClass),
        (GtkClassInitFunc) gimp_airbrush_tool_class_init,
        (GtkObjectInitFunc) gimp_airbrush_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        NULL
      };

      tool_type = gtk_type_unique (GIMP_TYPE_PAINT_TOOL, &tool_info);
    }

  return tool_type;
}

static void 
gimp_airbrush_tool_class_init (GimpAirbrushToolClass *klass)
{
  GtkObjectClass     *object_class;
  GimpPaintToolClass *paint_tool_class;

  object_class     = (GtkObjectClass *) klass;
  paint_tool_class = (GimpPaintToolClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_PAINT_TOOL);

  object_class->destroy   = gimp_airbrush_tool_destroy;

  paint_tool_class->paint = gimp_airbrush_tool_paint;
}

static void
gimp_airbrush_tool_init (GimpAirbrushTool *airbrush)
{
  GimpTool      *tool;
  GimpPaintTool *paint_tool;

  tool       = GIMP_TOOL (airbrush);
  paint_tool = GIMP_PAINT_TOOL (airbrush);

  if (! airbrush_options)
    {
      airbrush_options = airbrush_options_new ();

      tool_manager_register_tool_options (GIMP_TYPE_AIRBRUSH_TOOL,
                                          (ToolOptions *) airbrush_options);
     }

   tool->tool_cursor = GIMP_AIRBRUSH_TOOL_CURSOR;

   paint_tool->pick_colors  = TRUE;
   paint_tool->flags       |= TOOL_CAN_HANDLE_CHANGING_BRUSH;
}
             
static void
gimp_airbrush_tool_destroy (GtkObject *object)
{
  if (timer_state == ON)
    {
      gtk_timeout_remove (timer);
      timer_state = OFF;
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_airbrush_tool_paint (GimpPaintTool *paint_tool,
			  GimpDrawable  *drawable,
			  PaintState     state)
{
  PaintPressureOptions *pressure_options;
  gdouble               pressure;
  gdouble               rate;
  gboolean              incremental;
  GimpBrush            *brush;

  if (!drawable) 
    return;

  if (airbrush_options)
    {
      pressure_options = airbrush_options->paint_options.pressure_options;
      pressure         = airbrush_options->pressure;
      rate             = airbrush_options->rate;
      incremental      = airbrush_options->paint_options.incremental;
    }
  else
    {
      pressure_options = &non_gui_pressure_options;
      pressure         = non_gui_pressure;
      rate             = non_gui_rate;
      incremental      = non_gui_incremental;
    }

  brush = gimp_context_get_brush (NULL);

  switch (state)
    {
    case INIT_PAINT:
      if (timer_state == ON)
	{
	  g_warning ("killing stray timer, please report to lewing@gimp.org");
	  gtk_timeout_remove (timer);
	  timer_state = OFF;
	}
      break;

    case MOTION_PAINT:
      if (timer_state == ON)
	{
	  gtk_timeout_remove (timer);
	  timer_state = OFF;
	}

      gimp_airbrush_tool_motion (paint_tool,
				 drawable,
				 pressure_options,
				 pressure,
				 incremental);

      if (rate != 0.0)
	{
	  gdouble timeout;

	  airbrush_timeout.paint_tool = paint_tool;
	  airbrush_timeout.drawable   = drawable;

	  timeout = (pressure_options->rate ? 
		     (10000 / (rate * 2.0 * paint_tool->curpressure)) : 
		     (10000 / rate));

	  timer = gtk_timeout_add (timeout, airbrush_time_out, NULL);
	  timer_state = ON;
	}
      break;

    case FINISH_PAINT:
      if (timer_state == ON)
	{
	  gtk_timeout_remove (timer);
	  timer_state = OFF;
	}
      break;

    default:
      break;
    }
}

static gint
airbrush_time_out (gpointer client_data)
{
  PaintPressureOptions *pressure_options;
  gdouble               pressure;
  gdouble               rate;
  gboolean              incremental;

  if (airbrush_options)
    {
      pressure_options = airbrush_options->paint_options.pressure_options;
      pressure         = airbrush_options->pressure;
      rate             = airbrush_options->rate;
      incremental      = airbrush_options->paint_options.incremental;
    }
  else
    {
      pressure_options = &non_gui_pressure_options;
      pressure         = non_gui_pressure;
      rate             = non_gui_rate;
      incremental      = non_gui_incremental;
    }

  gimp_airbrush_tool_motion (airbrush_timeout.paint_tool,
			     airbrush_timeout.drawable,
			     pressure_options,
			     pressure,
			     incremental);
  gdisplays_flush ();

  /*  restart the timer  */
  if (rate != 0.0)
    {
      if (pressure_options->rate)
	{
	  /* set a new timer */
	  timer = gtk_timeout_add ((10000 / (rate * 2.0 * airbrush_timeout.paint_tool->curpressure)), 
				   airbrush_time_out, NULL);
	  return FALSE;
	}

      return TRUE;
    }

  return FALSE;
}

static void
gimp_airbrush_tool_motion (GimpPaintTool        *paint_tool,
			   GimpDrawable	        *drawable,
			   PaintPressureOptions *pressure_options,
			   gdouble               pressure,
			   gboolean              incremental)
{
  GimpImage            *gimage;
  TempBuf              *area;
  guchar                col[MAX_CHANNELS];
  gdouble               scale;
  PaintApplicationMode  paint_appl_mode = incremental ? INCREMENTAL : CONSTANT;

  if (!drawable) 
    return;

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return;

  if (pressure_options->size)
    scale = paint_tool->curpressure;
  else
    scale = 1.0;

  if (! (area = gimp_paint_tool_get_paint_area (paint_tool, drawable, scale)))
    return;

  /*  color the pixels  */
  if (pressure_options->color)
    {
      GimpRGB color;
      
      gimp_gradient_get_color_at (gimp_context_get_gradient (NULL),
				  paint_tool->curpressure, &color);

      gimp_rgba_get_uchar (&color,
			   &col[RED_PIX],
			   &col[GREEN_PIX],
			   &col[BLUE_PIX],
			   &col[ALPHA_PIX]);

      paint_appl_mode = INCREMENTAL;

      color_pixels (temp_buf_data (area), col,
		    area->width * area->height, area->bytes);
    }
  else if (paint_tool->brush && paint_tool->brush->pixmap)
    {
      paint_appl_mode = INCREMENTAL;

      gimp_paint_tool_color_area_with_pixmap (paint_tool, gimage,
					      drawable, area, 
					      scale, SOFT);
    }
  else
    {
      gimp_image_get_foreground (gimage, drawable, col);
      col[area->bytes - 1] = OPAQUE_OPACITY;
      color_pixels (temp_buf_data (area), col,
		    area->width * area->height, area->bytes);
    }

  if (pressure_options->pressure)
    pressure = pressure * 2.0 * paint_tool->curpressure;

  /*  paste the newly painted area to the image  */
  gimp_paint_tool_paste_canvas (paint_tool, drawable,
				MIN (pressure, 255),
				(gint) (gimp_context_get_opacity (NULL) * 255),
				gimp_context_get_paint_mode (NULL),
				SOFT, scale, paint_appl_mode);
}

gboolean
airbrush_non_gui_default (GimpDrawable *drawable,
			  gint          num_strokes,
			  gdouble      *stroke_array)
{
  AirbrushOptions *options = airbrush_options;

  gdouble pressure = AIRBRUSH_DEFAULT_PRESSURE;

  if (options)
    pressure = options->pressure;

  return airbrush_non_gui (drawable, pressure, num_strokes, stroke_array);
}

gboolean
airbrush_non_gui (GimpDrawable *drawable,
    		  gdouble       pressure,
		  gint          num_strokes,
		  gdouble      *stroke_array)
{
  static GimpAirbrushTool *non_gui_airbrush = NULL;

  GimpPaintTool *paint_tool;
  gint           i;

  if (! non_gui_airbrush)
    {
      non_gui_airbrush = gtk_type_new (GIMP_TYPE_AIRBRUSH_TOOL);
    }

  paint_tool = GIMP_PAINT_TOOL (non_gui_airbrush);

  if (gimp_paint_tool_start (paint_tool, drawable,
                             stroke_array[0],
                             stroke_array[1]))
    {
      non_gui_rate        = AIRBRUSH_DEFAULT_RATE;
      non_gui_pressure    = pressure;
      non_gui_incremental = AIRBRUSH_DEFAULT_INCREMENTAL;

      paint_tool->startx = paint_tool->lastx = stroke_array[0];
      paint_tool->starty = paint_tool->lasty = stroke_array[1];

      gimp_airbrush_tool_paint (paint_tool, drawable, MOTION_PAINT);

      for (i = 1; i < num_strokes; i++)
	{
	  paint_tool->curx = stroke_array[i * 2 + 0];
	  paint_tool->cury = stroke_array[i * 2 + 1];

	  gimp_paint_tool_interpolate (paint_tool, drawable);

	  paint_tool->lastx = paint_tool->curx;
	  paint_tool->lasty = paint_tool->cury;
	}

      gimp_paint_tool_finish (paint_tool, drawable);

      return TRUE;
    }

  return FALSE;
}

static AirbrushOptions *
airbrush_options_new (void)
{
  AirbrushOptions *options;
  GtkWidget       *vbox;
  GtkWidget       *table;
  GtkWidget       *scale;

  options = g_new0 (AirbrushOptions, 1);
  paint_options_init ((PaintOptions *) options,
		      GIMP_TYPE_AIRBRUSH_TOOL,
		      airbrush_options_reset);

  options->rate     = options->rate_d     = 80.0;
  options->pressure = options->pressure_d = AIRBRUSH_DEFAULT_PRESSURE;

  /*  the main vbox  */
  vbox = ((ToolOptions *) options)->main_vbox;

  /*  the rate scale  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 1);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  options->rate_w =
    gtk_adjustment_new (options->rate_d, 0.0, 150.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->rate_w));
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->rate_w), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &options->rate);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Rate:"), 1.0, 1.0,
			     scale, 1, FALSE);

  /*  the pressure scale  */
  options->pressure_w =
    gtk_adjustment_new (options->pressure_d, 0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->pressure_w));
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->pressure_w), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &options->pressure);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Pressure:"), 1.0, 1.0,
			     scale, 1, FALSE);

  gtk_widget_show (table);

  return options;
}

static void
airbrush_options_reset (ToolOptions *tool_options)
{
  AirbrushOptions *options;

  options = (AirbrushOptions *) tool_options;

  paint_options_reset (tool_options);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->rate_w),
			    options->rate_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->pressure_w),
			    options->pressure_d);
}
