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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdlib.h>

#include "appenv.h"
#include "airbrush.h"
#include "brushes.h"
#include "canvas.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h" /* for airbrush_time_out() */
#include "gimage.h"
#include "paint.h"
#include "paint_core_16.h"
#include "paint_funcs_area.h"
#include "palette.h"
#include "pixelarea.h"
#include "tools.h"


typedef struct _AirbrushTimeout AirbrushTimeout;

struct _AirbrushTimeout
{
  PaintCore16 *paint_core;
  GimpDrawable *drawable;
};

/*  forward function declarations  */
static void         airbrush_motion   (PaintCore16 *, GimpDrawable *, double);
static gint         airbrush_time_out (gpointer);
static Argument *   airbrush_invoker  (Argument *);

static double non_gui_pressure;

/*  The maximum amount of pressure that can be exerted  */
#define             MAX_PRESSURE  0.075

#define             OFF           0
#define             ON            1

typedef struct _AirbrushOptions AirbrushOptions;
struct _AirbrushOptions
{
  double rate;
  double pressure;
};


/*  local variables  */
static gint         timer;                 /*  timer for successive paint applications  */
static int          timer_state = OFF;     /*  state of airbrush tool  */
static AirbrushTimeout  airbrush_timeout;
static AirbrushOptions *airbrush_options = NULL;

static void
airbrush_scale_update (GtkAdjustment *adjustment,
		       double        *scale_val)
{
  *scale_val = adjustment->value;
}

static AirbrushOptions *
create_airbrush_options (void)
{
  AirbrushOptions *options;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *rate_scale;
  GtkWidget *pressure_scale;
  GtkObject *rate_scale_data;
  GtkObject *pressure_scale_data;

  /*  the new options structure  */
  options = (AirbrushOptions *) g_malloc (sizeof (AirbrushOptions));
  options->rate = 25.0;
  options->pressure = 10.0;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);

  /*  the main label  */
  label = gtk_label_new ("Airbrush Options");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  the rate scale  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Rate");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  rate_scale_data = gtk_adjustment_new (25.0, 0.0, 100.0, 1.0, 1.0, 0.0);
  rate_scale = gtk_hscale_new (GTK_ADJUSTMENT (rate_scale_data));
  gtk_box_pack_start (GTK_BOX (hbox), rate_scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (rate_scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (rate_scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (rate_scale_data), "value_changed",
		      (GtkSignalFunc) airbrush_scale_update, &options->rate);
  gtk_widget_show (rate_scale);

  /*  the pressure scale  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new ("Pressure");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  pressure_scale_data = gtk_adjustment_new (10.0, 0.0, 100.0, 1.0, 1.0, 0.0);
  pressure_scale = gtk_hscale_new (GTK_ADJUSTMENT (pressure_scale_data));
  gtk_box_pack_start (GTK_BOX (hbox), pressure_scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (pressure_scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (pressure_scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (pressure_scale_data), "value_changed",
		      (GtkSignalFunc) airbrush_scale_update, &options->pressure);
  gtk_widget_show (pressure_scale);

  /*  Register this selection options widget with the main tools options dialog  */
  tools_register_options (AIRBRUSH, vbox);

  return options;
}


void * 
airbrush_paint_func  (
                      PaintCore16 * paint_core,
                      GimpDrawable * drawable,
                      int state
                      )
{
  GBrushP brush;

  if (!drawable) 
    return NULL;

  brush = get_active_brush ();
  switch (state)
    {
    case INIT_PAINT :
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


Tool * 
tools_new_airbrush  (
                     void
                     )
{
  Tool * tool;
  PaintCore16 * private;

  if (! airbrush_options)
    airbrush_options = create_airbrush_options ();

  tool = paint_core_16_new (AIRBRUSH);

  private = (PaintCore16 *) tool->private;
  private->paint_func = airbrush_paint_func;

  return tool;
}


void 
tools_free_airbrush  (
                      Tool * tool
                      )
{
  paint_core_16_free (tool);

  if (timer_state == ON)
    gtk_timeout_remove (timer);
  timer_state = OFF;
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
airbrush_motion  (
                  PaintCore16 * paint_core,
                  GimpDrawable * drawable,
                  double pressure
                  )
{
  Canvas * painthit;
  PixelArea a;
      
  /* Get the working canvas */
  painthit = paint_core_16_area (paint_core, drawable);
  pixelarea_init (&a, painthit, NULL, 0, 0, 0, 0, TRUE);

  /* construct the paint hit */
  {
    Paint * paint = paint_new (canvas_tag (painthit), drawable);
    gimp16_palette_get_foreground (paint);
    color_area (&a, paint);
    paint_delete (paint);
  }
  
  /* apply it to the image */
  paint_core_16_area_paste (paint_core, drawable,
                            (gfloat) pressure / 100.0,
                            (gfloat) get_brush_opacity (),
                            SOFT,
                            CONSTANT,
                            get_brush_paint_mode ());
}


static void * 
airbrush_non_gui_paint_func  (
                              PaintCore16 * paint_core,
                              GimpDrawable * drawable,
                              int state
                              )
{
  airbrush_motion (paint_core, drawable, non_gui_pressure);

  return NULL;
}


/*  The airbrush procedure definition  */
ProcArg airbrush_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
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
  5,
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

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  the drawable  */
  if (success)
    {
      int_value = args[1].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL || gimage != drawable_gimage (drawable))
	success = FALSE;
    }
  /*  pressure  */
  if (success)
    {
      fp_value = args[2].value.pdb_float;
      if (fp_value >= 0.0 && fp_value <= 100.0)
	non_gui_pressure = fp_value;
      else
	success = FALSE;
    }
  /*  num strokes  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      if (int_value > 0)
	num_strokes = int_value / 2;
      else
	success = FALSE;
    }

      /*  point array  */
  if (success)
    stroke_array = (double *) args[4].value.pdb_pointer;

  if (success)
    /*  init the paint core  */
    success = paint_core_16_init (&non_gui_paint_core_16, drawable,
			       stroke_array[0], stroke_array[1]);

  if (success)
    {
      /*  set the paint core's paint func  */
      non_gui_paint_core_16.paint_func = airbrush_non_gui_paint_func;

      non_gui_paint_core_16.startx = non_gui_paint_core_16.lastx = stroke_array[0];
      non_gui_paint_core_16.starty = non_gui_paint_core_16.lasty = stroke_array[1];

      if (num_strokes == 1)
	airbrush_non_gui_paint_func (&non_gui_paint_core_16, drawable, 0);

      for (i = 1; i < num_strokes; i++)
	{
	  non_gui_paint_core_16.curx = stroke_array[i * 2 + 0];
	  non_gui_paint_core_16.cury = stroke_array[i * 2 + 1];

	  paint_core_16_interpolate (&non_gui_paint_core_16, drawable);

	  non_gui_paint_core_16.lastx = non_gui_paint_core_16.curx;
	  non_gui_paint_core_16.lasty = non_gui_paint_core_16.cury;
	}

      /*  finish the painting  */
      paint_core_16_finish (&non_gui_paint_core_16, drawable, -1);

      /*  cleanup  */
      paint_core_16_cleanup ();
    }

  return procedural_db_return_args (&airbrush_proc, success);
}
