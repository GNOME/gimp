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
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h"
#include "paint_funcs.h"
#include "paint_core.h"
#include "palette.h"
#include "airbrush.h"
#include "selection.h"
#include "tool_options_ui.h"
#include "tools.h"
#include "gimage.h"

#include "libgimp/gimpintl.h"

/*  The maximum amount of pressure that can be exerted  */
#define             MAX_PRESSURE  0.075

#define             OFF           0
#define             ON            1

/*  the airbrush structures  */

typedef struct _AirbrushTimeout AirbrushTimeout;
struct _AirbrushTimeout
{
  PaintCore *paint_core;
  GimpDrawable *drawable;
};

typedef struct _AirbrushOptions AirbrushOptions;
struct _AirbrushOptions
{
  ToolOptions  tool_options;

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


/*  forward function declarations  */
static void         airbrush_motion   (PaintCore *, GimpDrawable *, double);
static gint         airbrush_time_out (gpointer);
static Argument *   airbrush_invoker  (Argument *);


/*  functions  */

static void
airbrush_options_reset (void)
{
  AirbrushOptions *options = airbrush_options;

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
  tool_options_init ((ToolOptions *) options,
		     _("Airbrush Options"),
		     airbrush_options_reset);
  options->rate     = options->rate_d     = 80.0;
  options->pressure = options->pressure_d = 10.0;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  the rate scale  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 6);
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
tools_new_airbrush ()
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

  return tool;
}

void *
airbrush_paint_func (PaintCore *paint_core,
		     GimpDrawable *drawable,
		     int        state)
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

      airbrush_motion (paint_core, drawable, airbrush_options->pressure);

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
		   airbrush_options->pressure);
  gdisplays_flush ();

  /*  restart the timer  */
  if (airbrush_options->rate != 0.0)
    return TRUE;
  else
    return FALSE;
}


static void
airbrush_motion (PaintCore *paint_core,
		 GimpDrawable *drawable,
		 double     pressure)
{
  gint opacity;
  GImage *gimage;
  TempBuf * area;
  unsigned char col[MAX_CHANNELS];

  if (!drawable) 
    return;

  if (! (gimage = drawable_gimage (drawable)))
    return;

  gimage_get_foreground (gimage, drawable, col);

  if (! (area = paint_core_get_paint_area (paint_core, drawable)))
    return;

  /*  color the pixels  */
  col[area->bytes - 1] = OPAQUE_OPACITY;

  /*  color the pixels  */
  color_pixels (temp_buf_data (area), col,
		area->width * area->height, area->bytes);

  opacity = pressure * (paint_core->curpressure / 0.5);
  if (opacity > 255)
    opacity = 255;

  /*  paste the newly painted area to the image  */
  paint_core_paste_canvas (paint_core, drawable,
			   opacity,
			   (int) (gimp_brush_get_opacity () * 255),
			   gimp_brush_get_paint_mode (),
			   SOFT, CONSTANT);
}


static void *
airbrush_non_gui_paint_func (PaintCore *paint_core,
			     GimpDrawable *drawable,
			     int        state)
{
  airbrush_motion (paint_core, drawable, non_gui_pressure);

  return NULL;
}


/*  The airbrush procedure definition  */
ProcArg airbrush_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_FLOAT,
    "pressure",
    "The pressure of the airbrush strokes: 0 <= pressure <= 100"
  },
  { PDB_INT32,
    "num_strokes",
    "number of stroke control points (count each coordinate as 2 points)"
  },
  { PDB_FLOATARRAY,
    "strokes",
    "array of stroke coordinates: {s1.x, s1.y, s2.x, s2.y, ..., sn.x, sn.y}"
  }
};


ProcRecord airbrush_proc =
{
  "gimp_airbrush",
  "Paint in the current brush with varying pressure.  Paint application is time-dependent",
  "This tool simulates the use of an airbrush.  Paint pressure represents the relative intensity of the paint application.  High pressure results in a thicker layer of paint while low pressure results in a thinner layer.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  4,
  airbrush_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { airbrush_invoker } },
};


static Argument *
airbrush_invoker (Argument *args)
{
  int success = TRUE;
  GImage *gimage;
  GimpDrawable *drawable;
  int num_strokes;
  double *stroke_array;
  int int_value;
  double fp_value;
  int i;

  drawable = NULL;
  num_strokes = 0;

  /*  the drawable  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL)
	success = FALSE;
      else
        gimage = drawable_gimage (drawable);
    }
  /*  pressure  */
  if (success)
    {
      fp_value = args[1].value.pdb_float;
      if (fp_value >= 0.0 && fp_value <= 100.0)
	non_gui_pressure = fp_value;
      else
	success = FALSE;
    }
  /*  num strokes  */
  if (success)
    {
      int_value = args[2].value.pdb_int;
      if (int_value > 0)
	num_strokes = int_value / 2;
      else
	success = FALSE;
    }

      /*  point array  */
  if (success)
    stroke_array = (double *) args[3].value.pdb_pointer;

  if (success)
    /*  init the paint core  */
    success = paint_core_init (&non_gui_paint_core, drawable,
			       stroke_array[0], stroke_array[1]);

  if (success)
    {
      /*  set the paint core's paint func  */
      non_gui_paint_core.paint_func = airbrush_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      if (num_strokes == 1)
	airbrush_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

      for (i = 1; i < num_strokes; i++)
	{
	  non_gui_paint_core.curx = stroke_array[i * 2 + 0];
	  non_gui_paint_core.cury = stroke_array[i * 2 + 1];

	  paint_core_interpolate (&non_gui_paint_core, drawable);

	  non_gui_paint_core.lastx = non_gui_paint_core.curx;
	  non_gui_paint_core.lasty = non_gui_paint_core.cury;
	}

      /*  finish the painting  */
      paint_core_finish (&non_gui_paint_core, drawable, -1);

      /*  cleanup  */
      paint_core_cleanup ();
    }

  return procedural_db_return_args (&airbrush_proc, success);
}
