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
#include <math.h>
#include "appenv.h"
#include "brushes.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h"
#include "paint_funcs.h"
#include "paint_core.h"
#include "palette.h"
#include "paintbrush.h"
#include "selection.h"
#include "tools.h"

/*  forward function declarations  */
static void         paintbrush_motion      (PaintCore *, GimpDrawable *, double, gboolean);
static Argument *   paintbrush_invoker     (Argument *);
static Argument *   paintbrush_extended_invoker     (Argument *);

static double non_gui_fade_out, non_gui_incremental;


/*  defines  */
#define  PAINT_LEFT_THRESHOLD  0.05

typedef struct _PaintOptions PaintOptions;
struct _PaintOptions
{
  double fade_out;
  gboolean incremental;
};

/*  local variables  */
static PaintOptions *paint_options = NULL;


static void
paintbrush_toggle_update (GtkWidget *w,
			  gpointer   data)
{
  gboolean *toggle_val;

  toggle_val = (gboolean *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static void
paintbrush_scale_update (GtkAdjustment *adjustment,
			 double        *scale_val)
{
  *scale_val = adjustment->value;
}

static PaintOptions *
create_paint_options (void)
{
  PaintOptions *options;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *fade_out_scale;
  GtkObject *fade_out_scale_data;
  GtkWidget *incremental_toggle;

  /*  the new options structure  */
  options = (PaintOptions *) g_malloc (sizeof (PaintOptions));
  options->fade_out = 0.0;
  options->incremental = FALSE;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);

  /*  the main label  */
  label = gtk_label_new ("Paintbrush Options");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  the fade-out scale  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new ("Fade Out");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  fade_out_scale_data = gtk_adjustment_new (0.0, 0.0, 1000.0, 1.0, 1.0, 0.0);
  fade_out_scale = gtk_hscale_new (GTK_ADJUSTMENT (fade_out_scale_data));
  gtk_box_pack_start (GTK_BOX (hbox), fade_out_scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (fade_out_scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (fade_out_scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (fade_out_scale_data), "value_changed",
		      (GtkSignalFunc) paintbrush_scale_update,
		      &options->fade_out);
  gtk_widget_show (fade_out_scale);
  gtk_widget_show (hbox);


  /* the incremental toggle */
  incremental_toggle = gtk_check_button_new_with_label ("Incremental");
  gtk_box_pack_start (GTK_BOX (vbox), incremental_toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (incremental_toggle), "toggled",
		      (GtkSignalFunc) paintbrush_toggle_update,
		      &options->incremental);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (incremental_toggle), options->incremental);
  gtk_widget_show (incremental_toggle);
  
  /*  Register this selection options widget with the main tools options dialog  */
  tools_register_options (PAINTBRUSH, vbox);

  return options;
}

void *
paintbrush_paint_func (PaintCore *paint_core,
		       GimpDrawable *drawable,
		       int        state)
{
  switch (state)
    {
    case INIT_PAINT :
      break;

    case MOTION_PAINT :
      paintbrush_motion (paint_core, drawable, paint_options->fade_out, paint_options->incremental);
      break;

    case FINISH_PAINT :
      break;

    default :
      break;
    }

  return NULL;
}


Tool *
tools_new_paintbrush ()
{
  Tool * tool;
  PaintCore * private;

  if (! paint_options)
    paint_options = create_paint_options ();

  tool = paint_core_new (PAINTBRUSH);

  private = (PaintCore *) tool->private;
  private->paint_func = paintbrush_paint_func;

  return tool;
}


void
tools_free_paintbrush (Tool *tool)
{
  paint_core_free (tool);
}


void
paintbrush_motion (PaintCore *paint_core,
		   GimpDrawable *drawable,
		   double     fade_out,
		   gboolean   incremental)
{
  GImage *gimage;
  TempBuf * area;
  double x, paint_left;
  unsigned char blend = OPAQUE_OPACITY;
  unsigned char col[MAX_CHANNELS];

  if (! (gimage = drawable_gimage (drawable)))
    return;

  gimage_get_foreground (gimage, drawable, col);

  /*  Get a region which can be used to paint to  */
  if (! (area = paint_core_get_paint_area (paint_core, drawable)))
    return;

  /*  factor in the fade out value  */
  if (fade_out)
    {
      /*  Model the amount of paint left as a gaussian curve  */
      x = ((double) paint_core->distance / fade_out);
      paint_left = exp (- x * x * 0.5);

      blend = (int) (255 * paint_left);
    }

  if (blend)
    {
      /*  set the alpha channel  */
      col[area->bytes - 1] = OPAQUE_OPACITY;

      /*  color the pixels  */
      color_pixels (temp_buf_data (area), col,
		    area->width * area->height, area->bytes);

      /*  paste the newly painted canvas to the gimage which is being worked on  */
      paint_core_paste_canvas (paint_core, drawable, blend,
			       (int) (get_brush_opacity () * 255),
			       get_brush_paint_mode (), SOFT,
			       incremental ? INCREMENTAL : CONSTANT);
    }
}


static void *
paintbrush_non_gui_paint_func (PaintCore *paint_core,
			       GimpDrawable *drawable,
			       int        state)
{	
  paintbrush_motion (paint_core, drawable, non_gui_fade_out, non_gui_incremental);

  return NULL;
}


/*  The paintbrush procedure definition  */
ProcArg paintbrush_args[] =
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
    "fade_out",
    "fade out parameter: fade_out > 0"
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

ProcArg paintbrush_extended_args[] =
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
    "fade_out",
    "fade out parameter: fade_out > 0"
  },
  { PDB_INT32,
    "num_strokes",
    "number of stroke control points (count each coordinate as 2 points)"
  },
  { PDB_FLOATARRAY,
    "strokes",
    "array of stroke coordinates: {s1.x, s1.y, s2.x, s2.y, ..., sn.x, sn.y}"
  },
  { PDB_INT32,
    "method",
    "CONTINUOUS(0) or INCREMENTAL(1)"
  }
};


ProcRecord paintbrush_proc =
{
  "gimp_paintbrush",
  "Paint in the current brush with optional fade out parameter",
  "This tool is the standard paintbrush.  It draws linearly interpolated lines through the specified stroke coordinates.  It operates on the specified drawable in the foreground color with the active brush.  The \"fade_out\" parameter is measured in pixels and allows the brush stroke to linearly fall off.  The pressure is set to the maximum at the beginning of the stroke.  As the distance of the stroke nears the fade_out value, the pressure will approach zero.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  5,
  paintbrush_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { paintbrush_invoker } },
};

ProcRecord paintbrush_extended_proc =
{
  "gimp_paintbrush_exteneded",
  "Paint in the current brush with optional fade out parameter",
  "This tool is the standard paintbrush.  It draws linearly interpolated lines through the specified stroke coordinates.  It operates on the specified drawable in the foreground color with the active brush.  The \"fade_out\" parameter is measured in pixels and allows the brush stroke to linearly fall off.  The pressure is set to the maximum at the beginning of the stroke.  As the distance of the stroke nears the fade_out value, the pressure will approach zero.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  6,
  paintbrush_extended_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { paintbrush_extended_invoker } },
};

static Argument *
paintbrush_invoker (Argument *args)
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
  /*  fade out  */
  if (success)
    {
      fp_value = args[2].value.pdb_float;
      if (fp_value >= 0.0)
	non_gui_fade_out = fp_value;
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
    success = paint_core_init (&non_gui_paint_core, drawable,
			       stroke_array[0], stroke_array[1]);

  if (success)
    {
      non_gui_incremental = 0;
      /*  set the paint core's paint func  */
      non_gui_paint_core.paint_func = paintbrush_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      if (num_strokes == 1)
	paintbrush_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

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

  return procedural_db_return_args (&paintbrush_proc, success);
}
static Argument *
paintbrush_extended_invoker (Argument *args)
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
  /*  fade out  */
  if (success)
    {
      fp_value = args[2].value.pdb_float;
      if (fp_value >= 0.0)
	non_gui_fade_out = fp_value;
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
    success = paint_core_init (&non_gui_paint_core, drawable,
			       stroke_array[0], stroke_array[1]);

  if (success)
    {
      non_gui_incremental = args[5].value.pdb_int;
      /*  set the paint core's paint func  */
      non_gui_paint_core.paint_func = paintbrush_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      if (num_strokes == 1)
	paintbrush_non_gui_paint_func (&non_gui_paint_core, drawable, 0);

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

  return procedural_db_return_args (&paintbrush_proc, success);
}
