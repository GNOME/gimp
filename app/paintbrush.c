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
#include <stdio.h>
#include <math.h>
#include "appenv.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h"
#include "gradient.h"
#include "paint_funcs.h"
#include "paint_core.h"
#include "palette.h"
#include "paint_options.h"
#include "paintbrush.h"
#include "selection.h"
#include "tool_options_ui.h"
#include "tools.h"

#include "libgimp/gimpintl.h"

/*  defines  */
#define  PAINT_LEFT_THRESHOLD  0.05

/*  the paintbrush structures  */

typedef struct _PaintbrushOptions PaintbrushOptions;
struct _PaintbrushOptions
{
  PaintOptions  paint_options;

  double        fade_out;
  double        fade_out_d;
  GtkObject    *fade_out_w;

  int           use_gradient;
  int           use_gradient_d;
  GtkWidget    *use_gradient_w;

  double        gradient_length;
  double        gradient_length_d;
  GtkObject    *gradient_length_w;

  int           gradient_type;
  int           gradient_type_d;
  GtkWidget    *gradient_type_w[4];  /* 4 radio buttons */

  gboolean      incremental;
  gboolean      incremental_d;
  GtkWidget    *incremental_w;
};

/*  the paint brush tool options  */
static PaintbrushOptions * paintbrush_options = NULL;

/*  local variables  */
static double  non_gui_fade_out;
static double  non_gui_gradient_length;
static int     non_gui_gradient_type;
static double  non_gui_incremental;


/*  forward function declarations  */
static void paintbrush_motion (PaintCore *, GimpDrawable *,
			       double, double, gboolean, int);


/*  functions  */

static void
paintbrush_gradient_toggle_callback (GtkWidget *widget,
				     gpointer   data)
{
  PaintbrushOptions *options = paintbrush_options;

  static int incremental_save = FALSE;

  tool_options_toggle_update (widget, data);

  if (paintbrush_options->use_gradient)
    {
      incremental_save = options->incremental;
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->incremental_w),
				    TRUE);
    }
  else
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->incremental_w),
				    incremental_save);
    }
}

static void
paintbrush_gradient_type_callback (GtkWidget *widget,
				   gpointer   data)
{
  if (paintbrush_options)
    paintbrush_options->gradient_type = (int) data;
}


static void
paintbrush_options_reset (void)
{
  PaintbrushOptions *options = paintbrush_options;

  paint_options_reset ((PaintOptions *) options);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->fade_out_w),
			    options->fade_out_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->use_gradient_w),
				options->use_gradient_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->gradient_length_w),
			    options->gradient_length_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->gradient_type_w[options->gradient_type_d]),
				TRUE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->incremental_w),
				options->incremental_d);
}

static PaintbrushOptions *
paintbrush_options_new (void)
{
  PaintbrushOptions *options;

  GtkWidget *vbox;
  GtkWidget *abox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *scale;
  GSList    *group = NULL;
  GtkWidget *radio_frame;
  GtkWidget *radio_box;
  GtkWidget *radio_button;

  int i;
  char *gradient_types[4] =
  {
    N_("Once Forward"),
    N_("Once Backward"),
    N_("Loop Sawtooth"),
    N_("Loop Triangle")
  };

  /*  the new paint tool options structure  */
  options = (PaintbrushOptions *) g_malloc (sizeof (PaintbrushOptions));
  paint_options_init ((PaintOptions *) options,
		      PAINTBRUSH,
		      paintbrush_options_reset);
  options->fade_out        = options->fade_out_d        = 0.0;
  options->incremental     = options->incremental_d     = FALSE;
  options->use_gradient    = options->use_gradient_d    = FALSE;
  options->gradient_length = options->gradient_length_d = 10.0;
  options->gradient_type   = options->gradient_type_d   = LOOP_TRIANGLE;
  
  /*  the main vbox  */
  vbox = ((ToolOptions *) options)->main_vbox;

  /*  the fade-out scale  */
  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 3);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  label = gtk_label_new (_("Fade Out:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  options->fade_out_w =
    gtk_adjustment_new (options->fade_out_d, 0.0, 1000.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->fade_out_w));
  gtk_table_attach_defaults (GTK_TABLE (table), scale, 1, 2, 0, 1);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->fade_out_w), "value_changed",
		      (GtkSignalFunc) tool_options_double_adjustment_update,
		      &options->fade_out);
  gtk_widget_show (scale);

  /*  the use gradient toggle  */
  options->use_gradient_w =
    gtk_check_button_new_with_label (_("Gradient"));
  gtk_table_attach (GTK_TABLE (table), options->use_gradient_w, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_signal_connect (GTK_OBJECT (options->use_gradient_w), "toggled",
		      (GtkSignalFunc) paintbrush_gradient_toggle_callback,
		      &options->use_gradient);
  gtk_widget_show (options->use_gradient_w);

  label = gtk_label_new (_("Length:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /*  the gradient length scale  */
  abox = gtk_alignment_new (0.5, 1.0, 1.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), abox, 1, 2, 1, 3,
		    GTK_EXPAND | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (abox);

  options->gradient_length_w =
    gtk_adjustment_new (options->gradient_length_d, 1.0, 50.0, 1.1, 0.1, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->gradient_length_w));
  gtk_container_add (GTK_CONTAINER (abox), scale);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->gradient_length_w), "value_changed",
		      (GtkSignalFunc) tool_options_double_adjustment_update,
		      &options->gradient_length);
  gtk_widget_show (scale);
  gtk_widget_show (table);

   /*  the radio frame and box  */
  radio_frame = gtk_frame_new (_("Gradient Type"));
  gtk_table_attach_defaults (GTK_TABLE (table), radio_frame, 0, 2, 3, 4);

  radio_box = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (radio_box), 2);
  gtk_container_add (GTK_CONTAINER (radio_frame), radio_box);

    /*  the radio buttons  */
  group = NULL;  
  for (i = 0; i < 4; i++)
    {
      radio_button = gtk_radio_button_new_with_label (group, gradient_types[i]);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (radio_button));
      gtk_signal_connect (GTK_OBJECT (radio_button), "toggled",
			  (GtkSignalFunc) paintbrush_gradient_type_callback,
			  (void *)((long) i));
      gtk_box_pack_start (GTK_BOX (radio_box), radio_button, FALSE, FALSE, 0);
      gtk_widget_show (radio_button);

      options->gradient_type_w[i] = radio_button;
    }

  gtk_widget_show (radio_box);
  gtk_widget_show (radio_frame);

  /* the incremental toggle */
  options->incremental_w = gtk_check_button_new_with_label (_("Incremental"));
  gtk_box_pack_start (GTK_BOX (vbox), options->incremental_w, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (options->incremental_w), "toggled",
		      (GtkSignalFunc) tool_options_toggle_update,
		      &options->incremental);
  gtk_widget_show (options->incremental_w);
  
  /*  automatically set the sensitive state of the gradient stuff  */
  gtk_widget_set_sensitive (scale, options->use_gradient_d);
  gtk_widget_set_sensitive (label, options->use_gradient_d);
  gtk_widget_set_sensitive (radio_frame, options->use_gradient_d);
  gtk_widget_set_sensitive (options->incremental_w, ! options->use_gradient_d);
  gtk_object_set_data (GTK_OBJECT (options->use_gradient_w), "set_sensitive",
		       scale);
  gtk_object_set_data (GTK_OBJECT (scale), "set_sensitive",
		       label);
  gtk_object_set_data (GTK_OBJECT (label), "set_sensitive",
		       radio_frame);
  gtk_object_set_data (GTK_OBJECT (options->use_gradient_w), "inverse_sensitive",
		       options->incremental_w);

  return options;
}

#define TIMED_BRUSH 0

void *
paintbrush_paint_func (PaintCore *paint_core,
		       GimpDrawable *drawable,
		       int        state)
{
#if TIMED_BRUSH
  static GTimer *timer = NULL;
#endif
  switch (state)
    {
    case INIT_PAINT :
#if TIMED_BRUSH
      timer = g_timer_new();
      g_timer_start(timer);
#endif /* TIMED_BRUSH */
      break;

    case MOTION_PAINT :
      paintbrush_motion (paint_core, drawable, 
			 paintbrush_options->fade_out, 
			 paintbrush_options->use_gradient ? 
			 exp(paintbrush_options->gradient_length/10) : 0,
			 paintbrush_options->incremental,
			 paintbrush_options->gradient_type);
      break;

    case FINISH_PAINT :
#if TIMED_BRUSH
      if (timer)
      {
	g_timer_stop(timer);
	printf("painting took %f:\n", g_timer_elapsed(timer, NULL));
	g_timer_destroy(timer);
	timer = NULL;
      }
#endif /* TIMED_BRUSH */
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

  /*  The tool options  */
  if (! paintbrush_options)
    {
      paintbrush_options = paintbrush_options_new ();
      tools_register (PAINTBRUSH, (ToolOptions *) paintbrush_options);

      /*  press all default buttons  */
      paintbrush_options_reset ();
    }

  tool = paint_core_new (PAINTBRUSH);

  private = (PaintCore *) tool->private;
  private->paint_func = paintbrush_paint_func;
  private->pick_colors = TRUE;

  return tool;
}


void
tools_free_paintbrush (Tool *tool)
{
  paint_core_free (tool);
}


static void
paintbrush_motion (PaintCore *paint_core,
		   GimpDrawable *drawable,
		   double     fade_out,
		   double     gradient_length,
		   gboolean   incremental,
		   int gradient_type)
{
  GImage *gimage;
  TempBuf * area;
  double x, paint_left;
  double position;
  unsigned char local_blend = OPAQUE_OPACITY;
  unsigned char temp_blend = OPAQUE_OPACITY;
  unsigned char col[MAX_CHANNELS];
  double r,g,b,a;
  int mode;

  position = 0.0;
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

      local_blend = (int) (255 * paint_left);
    }

  if (local_blend)
    {
      /*  set the alpha channel  */
      col[area->bytes - 1] = OPAQUE_OPACITY;
      temp_blend = local_blend;
      
      /* hard core to mode LOOP_TRIANGLE */
      /* need to make a gui to handle this */
      mode = gradient_type;

      if(gradient_length)
	{
	  paint_core_get_color_from_gradient (paint_core, gradient_length, &r,&g,&b,&a,mode);
	  r = r * 255.0;
	  g = g * 255.0;
	  b = b * 255.0;
	  a = a * 255.0;
	  temp_blend =  (gint)((a * local_blend)/255);
	  col[0] = (gint)r;
	  col[1] = (gint)g;
	  col[2] = (gint)b;
	  /* always use incremental mode with gradients */
	  /* make the gui cool later */
	  incremental = INCREMENTAL;
	}
  /* just leave this because I know as soon as i delete it i'll find a bug */
      /*          printf("temp_blend: %u grad_len: %f distance: %f \n",temp_blend, gradient_length, distance); */ 
	
      /*  color the pixels  */
      color_pixels (temp_buf_data (area), col,
		    area->width * area->height, area->bytes);

      /*  paste the newly painted canvas to the gimage which is being worked on  */
      paint_core_paste_canvas (paint_core, drawable, temp_blend,
			       (int) (PAINT_OPTIONS_GET_OPACITY (paintbrush_options) * 255),
			       PAINT_OPTIONS_GET_PAINT_MODE (paintbrush_options),
			       PRESSURE, 
			       incremental ? INCREMENTAL : CONSTANT);
    }
}


static void *
paintbrush_non_gui_paint_func (PaintCore *paint_core,
			       GimpDrawable *drawable,
			       int        state)
{	
  paintbrush_motion (paint_core, drawable, non_gui_fade_out,non_gui_gradient_length,  non_gui_incremental, non_gui_gradient_type);

  return NULL;
}

gboolean
paintbrush_non_gui (GimpDrawable *drawable,
                   int           num_strokes,
                   double       *stroke_array,
                   double        fade_out,
                   int           method,
                   double        gradient_length)
{
  int i;

  if (paint_core_init (&non_gui_paint_core, drawable,
                      stroke_array[0], stroke_array[1]))
    {
      non_gui_fade_out = fade_out;
      non_gui_gradient_length = gradient_length;
      non_gui_gradient_type = LOOP_TRIANGLE;
      non_gui_incremental = method;

      /* Set the paint core's paint func */
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

      /* Finish the painting */
      paint_core_finish (&non_gui_paint_core, drawable, -1);

      /* Cleanup */
      paint_core_cleanup ();
      return TRUE;
    }
  else
    return FALSE;
}
