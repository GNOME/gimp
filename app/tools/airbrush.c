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

#include <stdlib.h>
#include "appenv.h"
#include "gimpbrushlist.h"
#include "gimpbrushpipe.h"
#include "gradient.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h"
#include "paint_funcs.h"
#include "paint_core.h"
#include "paint_options.h"
#include "palette.h"
#include "airbrush.h"
#include "selection.h"
#include "tool_options_ui.h"
#include "tools.h"
#include "gimage.h"

#include "libgimp/gimpintl.h"

/*  The maximum amount of pressure that can be exerted  */
#define MAX_PRESSURE  0.075

/* Default pressure setting */
#define AIRBRUSH_PRESSURE_DEFAULT    10.0
#define AIRBRUSH_INCREMENTAL_DEFAULT FALSE

#define OFF           0
#define ON            1

/*  the airbrush structures  */

typedef struct _AirbrushTimeout AirbrushTimeout;
struct _AirbrushTimeout
{
  PaintCore    *paint_core;
  GimpDrawable *drawable;
};

typedef struct _AirbrushOptions AirbrushOptions;
struct _AirbrushOptions
{
  PaintOptions paint_options;

  double       rate;
  double       rate_d;
  GtkObject   *rate_w;

  double       pressure;
  double       pressure_d;
  GtkObject   *pressure_w;
};


/*  the airbrush tool options  */
static AirbrushOptions *airbrush_options = NULL; 

/*  local variables  */
static gint             timer;  /*  timer for successive paint applications  */
static int              timer_state = OFF;       /*  state of airbrush tool  */
static AirbrushTimeout  airbrush_timeout;

static double           non_gui_pressure;
static gboolean         non_gui_incremental;

/*  forward function declarations  */
static void         airbrush_motion   (PaintCore *, GimpDrawable *, PaintPressureOptions *,
				       double, PaintApplicationMode);
static gint         airbrush_time_out (gpointer);


/*  functions  */

static void
airbrush_options_reset (void)
{
  AirbrushOptions *options = airbrush_options;

  paint_options_reset ((PaintOptions *) options);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->rate_w),
			    options->rate_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->pressure_w),
			    options->pressure_d);
}

static AirbrushOptions *
airbrush_options_new (void)
{
  AirbrushOptions *options;

  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *scale;

  /*  the new airbrush tool options structure  */
  options = (AirbrushOptions *) g_malloc (sizeof (AirbrushOptions));
  paint_options_init ((PaintOptions *) options,
		      AIRBRUSH,
		      airbrush_options_reset);
  options->rate     = options->rate_d     = 80.0;
  options->pressure = options->pressure_d = AIRBRUSH_PRESSURE_DEFAULT;

  /*  the main vbox  */
  vbox = ((ToolOptions *) options)->main_vbox;

  /*  the rate scale  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 1);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  label = gtk_label_new (_("Rate:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  options->rate_w =
    gtk_adjustment_new (options->rate_d, 0.0, 150.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->rate_w));
  gtk_table_attach_defaults (GTK_TABLE (table), scale, 1, 2, 0, 1);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->rate_w), "value_changed",
		      (GtkSignalFunc) tool_options_double_adjustment_update,
		      &options->rate);
  gtk_widget_show (scale);

  /*  the pressure scale  */
  label = gtk_label_new (_("Pressure:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  options->pressure_w =
    gtk_adjustment_new (options->pressure_d, 0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->pressure_w));
  gtk_table_attach_defaults (GTK_TABLE (table), scale, 1, 2, 1, 2);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->pressure_w), "value_changed",
		      (GtkSignalFunc) tool_options_double_adjustment_update,
		      &options->pressure);
  gtk_widget_show (scale);

  gtk_widget_show (table);

  return options;
}

Tool *
tools_new_airbrush (void)
{
  Tool * tool;
  PaintCore * private;

  /*  The tool options  */
  if (! airbrush_options)
    {
      airbrush_options = airbrush_options_new ();
      tools_register (AIRBRUSH, (ToolOptions *) airbrush_options);
    }

  tool = paint_core_new (AIRBRUSH);

  private = (PaintCore *) tool->private;
  private->paint_func = airbrush_paint_func;
  private->pick_colors = TRUE;
  private->flags |= TOOL_CAN_HANDLE_CHANGING_BRUSH;

  return tool;
}

void *
airbrush_paint_func (PaintCore    *paint_core,
		     GimpDrawable *drawable,
		     int           state)
{
  GimpBrushP brush;

  if (!drawable) 
    return NULL;

  brush = get_active_brush ();
  switch (state)
    {
    case INIT_PAINT :
      /* timer_state = OFF; */
      if (timer_state == ON)
      {
	g_warning (_("killing stray timer, please report to lewing@gimp.org"));
	gtk_timeout_remove (timer);
      }
      timer_state = OFF;
      break;

    case MOTION_PAINT :
      if (timer_state == ON)
	gtk_timeout_remove (timer);
      timer_state = OFF;

      airbrush_motion (paint_core, drawable,
		       airbrush_options->paint_options.pressure_options,
		       airbrush_options->pressure,
		       airbrush_options->paint_options.incremental ?
		       INCREMENTAL : CONSTANT);

      if (airbrush_options->rate != 0.0)
	{
	  airbrush_timeout.paint_core = paint_core;
	  airbrush_timeout.drawable = drawable;
	  timer = gtk_timeout_add ((10000 / airbrush_options->rate),
				   airbrush_time_out, NULL);
	  timer_state = ON;
	}
      break;

    case FINISH_PAINT :
      if (timer_state == ON)
	gtk_timeout_remove (timer);
      timer_state = OFF;
      break;

    default :
      break;
    }

  return NULL;
}


void
tools_free_airbrush (Tool *tool)
{
  if (timer_state == ON)
    gtk_timeout_remove (timer);
  timer_state = OFF;

  paint_core_free (tool);
}


static gint
airbrush_time_out (gpointer client_data)
{
  /*  service the timer  */
  airbrush_motion (airbrush_timeout.paint_core,
		   airbrush_timeout.drawable,
		   airbrush_options->paint_options.pressure_options,
		   airbrush_options->pressure,
		   airbrush_options->paint_options.incremental ?
		   INCREMENTAL : CONSTANT);
  gdisplays_flush ();

  /*  restart the timer  */
  if (airbrush_options->rate != 0.0)
    return TRUE;
  else
    return FALSE;
}


static void
airbrush_motion (PaintCore	      *paint_core,
		 GimpDrawable	      *drawable,
		 PaintPressureOptions *pressure_options,
		 double		       pressure,
		 PaintApplicationMode  mode)
{
  GImage *gimage;
  TempBuf * area;
  unsigned char col[MAX_CHANNELS];
  gdouble scale;

  if (!drawable) 
    return;

  if (! (gimage = drawable_gimage (drawable)))
    return;

  if (pressure_options->size)
    scale = paint_core->curpressure;
  else
    scale = 1.0;

  if (! (area = paint_core_get_paint_area (paint_core, drawable, scale)))
    return;

  if (GIMP_IS_BRUSH_PIXMAP (paint_core->brush))
    {
      mode = INCREMENTAL;
      paint_core_color_area_with_pixmap (paint_core, gimage, drawable, area, 
					 scale, SOFT);
    }
  else
    {
      /*  color the pixels  */
      if (pressure_options->color)
	{
	  gdouble r, g, b, a;

	  grad_get_color_at (paint_core->curpressure, &r, &g, &b, &a);
	  col[0] = r * 255.0;
	  col[1] = g * 255.0;
	  col[2] = b * 255.0;
	  col[3] = a * 255.0;
	  mode = INCREMENTAL;
	}
      else
	{      
	  gimage_get_foreground (gimage, drawable, col);
	  col[area->bytes - 1] = OPAQUE_OPACITY;
	}
      color_pixels (temp_buf_data (area), col,
		    area->width * area->height, area->bytes);
    }

  if (pressure_options->pressure)
    pressure = pressure * 2.0 * paint_core->curpressure;

  /*  paste the newly painted area to the image  */
  paint_core_paste_canvas (paint_core, drawable,
			   MIN (pressure, 255),
			   (gint) (gimp_context_get_opacity (NULL) * 255),
			   gimp_context_get_paint_mode (NULL),
			   SOFT, scale, mode);
}

static void *
airbrush_non_gui_paint_func (PaintCore    *paint_core,
			     GimpDrawable *drawable,
			     int           state)
{
  airbrush_motion (paint_core, drawable, &non_gui_pressure_options, 
		   non_gui_pressure, non_gui_incremental);

  return NULL;
}

gboolean
airbrush_non_gui_default (GimpDrawable *drawable,
			  int           num_strokes,
			  double       *stroke_array)
{
  AirbrushOptions *options = airbrush_options;
  gdouble pressure = AIRBRUSH_PRESSURE_DEFAULT;

  if(options)
    pressure = options->pressure;

  return airbrush_non_gui (drawable, pressure, num_strokes, stroke_array);
}

gboolean
airbrush_non_gui (GimpDrawable *drawable,
    		  double        pressure,
		  int           num_strokes,
		  double       *stroke_array)
{
  int i;

  if (paint_core_init (&non_gui_paint_core, drawable,
		       stroke_array[0], stroke_array[1]))
    {
      /* Set the paint core's paint func */
      non_gui_paint_core.paint_func = airbrush_non_gui_paint_func;

      non_gui_pressure = pressure;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      airbrush_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

      for (i = 1; i < num_strokes; i++)
	{
	  non_gui_paint_core.curx = stroke_array[i * 2 + 0];
	  non_gui_paint_core.cury = stroke_array[i * 2 + 1];

	  paint_core_interpolate (&non_gui_paint_core, drawable);

	  non_gui_paint_core.lastx = non_gui_paint_core.curx;
	  non_gui_paint_core.lasty = non_gui_paint_core.cury;
	}

      /* Finish the painting */
      paint_core_finish (&non_gui_paint_core, drawable, -1);

      /* Cleanup */
      paint_core_cleanup ();
      return TRUE;
    }
  else
    return FALSE;
}
