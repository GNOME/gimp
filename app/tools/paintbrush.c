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
#include "buildmenu.h"
#include "drawable.h"
#include "errors.h"
#include "gdisplay.h"
#include "gimpbrushlist.h"
#include "gimpbrushpipe.h"
#include "gradient.h"
#include "paint_funcs.h"
#include "paint_core.h"
#include "paint_options.h"
#include "paintbrush.h"
#include "selection.h"
#include "tool_options_ui.h"
#include "tools.h"

#include "libgimp/gimpunitmenu.h"
#include "libgimp/gimpintl.h"

/*  defines  */
#define  PAINT_LEFT_THRESHOLD  0.05

/* defaults for the tool options */
#define PAINTBRUSH_DEFAULT_INCREMENTAL       FALSE
#define PAINTBRUSH_DEFAULT_USE_FADE          FALSE
#define PAINTBRUSH_DEFAULT_FADE_OUT          100.0
#define PAINTBRUSH_DEFAULT_FADE_UNIT         UNIT_PIXEL
#define PAINTBRUSH_DEFAULT_USE_GRADIENT      FALSE
#define PAINTBRUSH_DEFAULT_GRADIENT_LENGTH   100.0
#define PAINTBRUSH_DEFAULT_GRADIENT_UNIT     UNIT_PIXEL
#define PAINTBRUSH_DEFAULT_GRADIENT_TYPE     LOOP_TRIANGLE

/*  the paintbrush structures  */

typedef struct _PaintbrushOptions PaintbrushOptions;
struct _PaintbrushOptions
{
  PaintOptions  paint_options;

  gboolean      use_fade;
  gboolean      use_fade_d;
  GtkWidget    *use_fade_w;

  gdouble       fade_out;
  gdouble       fade_out_d;
  GtkObject    *fade_out_w;

  GUnit         fade_unit;
  GUnit         fade_unit_d;
  GtkWidget    *fade_unit_w;

  gboolean      use_gradient;
  gboolean      use_gradient_d;
  GtkWidget    *use_gradient_w;

  gdouble       gradient_length;
  gdouble       gradient_length_d;
  GtkObject    *gradient_length_w;

  GUnit         gradient_unit;
  GUnit         gradient_unit_d;
  GtkWidget    *gradient_unit_w;

  gint          gradient_type;
  gint          gradient_type_d;
  GtkWidget    *gradient_type_w;
};

/*  the paint brush tool options  */
static PaintbrushOptions * paintbrush_options = NULL;

/*  local variables  */
static double  non_gui_fade_out;
static double  non_gui_gradient_length;
static int     non_gui_gradient_type;
static double  non_gui_incremental;
static GUnit   non_gui_fade_unit;
static GUnit   non_gui_gradient_unit;


/*  forward function declarations  */
static void paintbrush_motion (PaintCore *, GimpDrawable *, PaintPressureOptions *,
			       double, double, PaintApplicationMode, GradientPaintMode);


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
      incremental_save = options->paint_options.incremental;
      gtk_toggle_button_set_active
	(GTK_TOGGLE_BUTTON (options->paint_options.incremental_w), TRUE);
    }
  else
    {
      gtk_toggle_button_set_active
	(GTK_TOGGLE_BUTTON (options->paint_options.incremental_w),
	 incremental_save);
    }
}

static void
paintbrush_gradient_type_callback (GtkWidget *widget,
				   gpointer   data)
{
  if (paintbrush_options)
    paintbrush_options->gradient_type = (GradientPaintMode) data;
}


static void
paintbrush_options_reset (void)
{
  PaintbrushOptions *options = paintbrush_options;
  GtkWidget *spinbutton;
  int        digits;

  paint_options_reset ((PaintOptions *) options);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->use_fade_w),
				options->use_fade_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->fade_out_w),
			    options->fade_out_d);
  gimp_unit_menu_set_unit (GIMP_UNIT_MENU (options->fade_unit_w),
			   options->fade_unit_d);
  digits = ((options->fade_unit_d == UNIT_PIXEL) ? 0 :
	    ((options->fade_unit_d == UNIT_PERCENT) ? 2 :
	     (MIN (6, MAX (3, gimp_unit_get_digits (options->fade_unit_d))))));
  spinbutton = gtk_object_get_data (GTK_OBJECT (options->fade_unit_w), "set_digits");
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spinbutton), digits);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->use_gradient_w),
				options->use_gradient_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->gradient_length_w),
			    options->gradient_length_d);
  gimp_unit_menu_set_unit (GIMP_UNIT_MENU (options->gradient_unit_w),
			   options->gradient_unit_d);
  digits = ((options->gradient_unit_d == UNIT_PIXEL) ? 0 :
	    ((options->gradient_unit_d == UNIT_PERCENT) ? 2 :
	     (MIN (6, MAX (3, gimp_unit_get_digits (options->gradient_unit_d))))));
  spinbutton = gtk_object_get_data (GTK_OBJECT (options->gradient_unit_w), "set_digits");
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spinbutton), digits);
  gtk_option_menu_set_history (GTK_OPTION_MENU (options->gradient_type_w), 
			       options->gradient_type_d);
}

static PaintbrushOptions *
paintbrush_options_new (void)
{
  PaintbrushOptions *options;

  GtkWidget *vbox;
  GtkWidget *abox;
  GtkWidget *table;
  GtkWidget *type_label;
  GtkWidget *spinbutton;
  GtkWidget *menu;

  static MenuItem gradient_type_items[] =
  {
    { N_("Once Forward"),  0, 0, paintbrush_gradient_type_callback, 
      (gpointer) ONCE_FORWARD, NULL, NULL },
    { N_("Once Backward"), 0, 0, paintbrush_gradient_type_callback, 
      (gpointer) ONCE_BACKWARDS, NULL, NULL },
    { N_("Loop Sawtooth"), 0, 0, paintbrush_gradient_type_callback, 
      (gpointer) LOOP_SAWTOOTH, NULL, NULL },
    { N_("Loop Triangle"), 0, 0, paintbrush_gradient_type_callback, 
      (gpointer) LOOP_TRIANGLE, NULL, NULL },
    { NULL, 0, 0, NULL, NULL, NULL, NULL }
  };

  /*  the new paint tool options structure  */
  options = g_new (PaintbrushOptions, 1);
  paint_options_init ((PaintOptions *) options,
		      PAINTBRUSH,
		      paintbrush_options_reset);

  options->use_fade        = 
                  options->use_fade_d        = PAINTBRUSH_DEFAULT_USE_FADE;
  options->fade_out        = 
                  options->fade_out_d        = PAINTBRUSH_DEFAULT_FADE_OUT;
  options->fade_unit        = 
                  options->fade_unit_d       = PAINTBRUSH_DEFAULT_FADE_UNIT;
  options->use_gradient    = 
                  options->use_gradient_d    = PAINTBRUSH_DEFAULT_USE_GRADIENT;
  options->gradient_length = 
                  options->gradient_length_d = PAINTBRUSH_DEFAULT_GRADIENT_LENGTH;
  options->gradient_unit        = 
                  options->gradient_unit_d   = PAINTBRUSH_DEFAULT_GRADIENT_UNIT;
  options->gradient_type   = 
                  options->gradient_type_d   = PAINTBRUSH_DEFAULT_GRADIENT_TYPE;
  
  /*  the main vbox  */
  vbox = ((ToolOptions *) options)->main_vbox;

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 3);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  the use fade toggle  */
  abox = gtk_alignment_new (0.5, 1.0, 1.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), abox, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (abox);

  options->use_fade_w =
    gtk_check_button_new_with_label (_("Fade Out"));
  gtk_container_add (GTK_CONTAINER (abox), options->use_fade_w);
  gtk_signal_connect (GTK_OBJECT (options->use_fade_w), "toggled",
		      GTK_SIGNAL_FUNC (tool_options_toggle_update),
		      &options->use_fade);
  gtk_widget_show (options->use_fade_w);

  /*  the fade-out sizeentry  */
  options->fade_out_w =  
    gtk_adjustment_new (options->fade_out_d,  1e-5, 32767.0, 1.0, 50.0, 0.0);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (options->fade_out_w), 1.0, 0.0);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton), GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  gtk_signal_connect (GTK_OBJECT (options->fade_out_w), "value_changed",
		      (GtkSignalFunc) tool_options_double_adjustment_update,
		      &options->fade_out);
  gtk_table_attach_defaults (GTK_TABLE (table), spinbutton, 1, 2, 0, 1);
  gtk_widget_show (spinbutton);

  /*  the fade-out unitmenu  */
  options->fade_unit_w = 
    gimp_unit_menu_new ("%a", options->fade_unit_d, TRUE, TRUE, TRUE);
  gtk_signal_connect (GTK_OBJECT (options->fade_unit_w), "unit_changed",
		      (GtkSignalFunc) tool_options_unitmenu_update,
		      &options->fade_unit);
  gtk_object_set_data (GTK_OBJECT (options->fade_unit_w), "set_digits", spinbutton);
  gtk_table_attach (GTK_TABLE (table), options->fade_unit_w, 2, 3, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (options->fade_unit_w);

  /*  automatically set the sensitive state of the fadeout stuff  */
  gtk_widget_set_sensitive (spinbutton, options->use_fade_d);
  gtk_widget_set_sensitive (options->fade_unit_w, options->use_fade_d);
  gtk_object_set_data (GTK_OBJECT (options->use_fade_w),
		       "set_sensitive", spinbutton);
  gtk_object_set_data (GTK_OBJECT (spinbutton),
		       "set_sensitive", options->fade_unit_w);

  /*  the use gradient toggle  */
  abox = gtk_alignment_new (0.5, 1.0, 1.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), abox, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (abox);

  options->use_gradient_w =
    gtk_check_button_new_with_label (_("Gradient"));
  gtk_container_add (GTK_CONTAINER (abox), options->use_gradient_w);
  gtk_signal_connect (GTK_OBJECT (options->use_gradient_w), "toggled",
		      GTK_SIGNAL_FUNC (paintbrush_gradient_toggle_callback),
		      &options->use_gradient);
  gtk_widget_show (options->use_gradient_w);

  /*  the gradient length scale  */
  options->gradient_length_w =  
    gtk_adjustment_new (options->gradient_length_d,  1e-5, 32767.0, 1.0, 50.0, 0.0);
  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (options->gradient_length_w), 1.0, 0.0);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton), GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  gtk_signal_connect (GTK_OBJECT (options->gradient_length_w), "value_changed",
		      (GtkSignalFunc) tool_options_double_adjustment_update,
		      &options->gradient_length);
  gtk_table_attach_defaults (GTK_TABLE (table), spinbutton, 1, 2, 1, 2);
  gtk_widget_show (spinbutton);

  /*  the gradient unitmenu  */
  options->gradient_unit_w = 
    gimp_unit_menu_new ("%a", options->gradient_unit_d, TRUE, TRUE, TRUE);
  gtk_signal_connect (GTK_OBJECT (options->gradient_unit_w), "unit_changed",
		      (GtkSignalFunc) tool_options_unitmenu_update,
		      &options->gradient_unit);
  gtk_object_set_data (GTK_OBJECT (options->gradient_unit_w), "set_digits", spinbutton);
  gtk_table_attach (GTK_TABLE (table), options->gradient_unit_w, 2, 3, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (options->gradient_unit_w);

  /*  the gradient type  */
  type_label = gtk_label_new (_("Type:"));
  gtk_misc_set_alignment (GTK_MISC (type_label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), type_label, 0, 1, 2, 3,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (type_label);

  abox = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_table_attach_defaults (GTK_TABLE (table), abox, 1, 3, 2, 3);
  gtk_widget_show (abox);

  options->gradient_type_w = gtk_option_menu_new ();
  gtk_container_add (GTK_CONTAINER (abox), options->gradient_type_w);
  gtk_widget_show (options->gradient_type_w);

  menu = build_menu (gradient_type_items, NULL);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (options->gradient_type_w), menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (options->gradient_type_w), 
			       options->gradient_type_d);

  gtk_widget_show (table);

  /*  automatically set the sensitive state of the gradient stuff  */
  gtk_widget_set_sensitive (spinbutton, options->use_gradient_d);
  gtk_widget_set_sensitive (spinbutton, options->use_gradient_d);
  gtk_widget_set_sensitive (options->gradient_unit_w, options->use_gradient_d);
  gtk_widget_set_sensitive (options->gradient_type_w, options->use_gradient_d);
  gtk_widget_set_sensitive (type_label, options->use_gradient_d);
  gtk_widget_set_sensitive (options->paint_options.incremental_w,
			    ! options->use_gradient_d);
  gtk_object_set_data (GTK_OBJECT (options->use_gradient_w), "set_sensitive",
		       spinbutton);
  gtk_object_set_data (GTK_OBJECT (spinbutton), "set_sensitive",
		       options->gradient_unit_w);
  gtk_object_set_data (GTK_OBJECT (options->gradient_unit_w), "set_sensitive",
		       options->gradient_type_w);
  gtk_object_set_data (GTK_OBJECT (options->gradient_type_w), "set_sensitive",
		       type_label);
  gtk_object_set_data (GTK_OBJECT (options->use_gradient_w), "inverse_sensitive",
		       options->paint_options.incremental_w);

  return options;
}

#define TIMED_BRUSH 0

void *
paintbrush_paint_func (PaintCore    *paint_core,
		       GimpDrawable *drawable,
		       int           state)
{  
  GDisplay *gdisp = gdisplay_active ();
  double fade_out;
  double gradient_length;	
  double unit_factor;
  
  g_return_val_if_fail (gdisp != NULL, NULL);

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
      switch (paintbrush_options->fade_unit)
	{
	case UNIT_PIXEL:
	  fade_out = paintbrush_options->fade_out;
	  break;
	case UNIT_PERCENT:
	  fade_out = MAX (gdisp->gimage->width, gdisp->gimage->height) * 
	    paintbrush_options->fade_out / 100;
	  break;
	default:
	  unit_factor = gimp_unit_get_factor (paintbrush_options->fade_unit);
	  fade_out = paintbrush_options->fade_out * 
	    MAX (gdisp->gimage->xresolution, gdisp->gimage->yresolution) / unit_factor;
	  break;
	}
      
      switch (paintbrush_options->gradient_unit)
	{
	case UNIT_PIXEL:
	  gradient_length = paintbrush_options->gradient_length;
	  break;
	case UNIT_PERCENT:
	  gradient_length = MAX (gdisp->gimage->width, gdisp->gimage->height) * 
	    paintbrush_options->gradient_length / 100;
	  break;
	default:
	  unit_factor = gimp_unit_get_factor (paintbrush_options->gradient_unit);
	  gradient_length = paintbrush_options->gradient_length * 
	    MAX (gdisp->gimage->xresolution, gdisp->gimage->yresolution) / unit_factor;
	  break;
	}
      
      paintbrush_motion (paint_core, drawable, 
			 paintbrush_options->paint_options.pressure_options,
			 paintbrush_options->use_fade ? fade_out : 0, 
			 paintbrush_options->use_gradient ? gradient_length : 0,
			 paintbrush_options->paint_options.incremental,
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
  private->flags |= TOOL_CAN_HANDLE_CHANGING_BRUSH;

  return tool;
}


void
tools_free_paintbrush (Tool *tool)
{
  paint_core_free (tool);
}


static void
paintbrush_motion (PaintCore            *paint_core,
		   GimpDrawable         *drawable,
		   PaintPressureOptions *pressure_options,
		   double                fade_out,
		   double                gradient_length,
		   PaintApplicationMode  incremental,
		   GradientPaintMode     gradient_type)
{
  GImage *gimage;
  TempBuf * area;
  gdouble x, paint_left;
  gdouble position;
  guchar local_blend = OPAQUE_OPACITY;
  guchar temp_blend = OPAQUE_OPACITY;
  guchar col[MAX_CHANNELS];
  gdouble r,g,b,a;
  gint mode;
  gint opacity;
  gdouble scale;
  PaintApplicationMode paint_appl_mode = incremental ? INCREMENTAL : CONSTANT;

  position = 0.0;
  if (! (gimage = drawable_gimage (drawable)))
    return;

  if (pressure_options->size)
    scale = paint_core->curpressure;
  else
    scale = 1.0;

  if (pressure_options->color)
    gradient_length = 1.0; /* not really used, only for if cases */

  /*  Get a region which can be used to paint to  */
  if (! (area = paint_core_get_paint_area (paint_core, drawable, scale)))
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
	  if (pressure_options->color)
	    gradient_get_color_at (gimp_context_get_gradient (NULL),
				   paint_core->curpressure, &r, &g, &b, &a);
	  else
	    paint_core_get_color_from_gradient (paint_core, gradient_length, 
						&r, &g, &b, &a, mode);
	  r = r * 255.0;
	  g = g * 255.0;
	  b = b * 255.0;
 	  a = a * 255.0;
	  temp_blend =  (gint)((a * local_blend) / 255);
	  col[0] = (gint)r;
	  col[1] = (gint)g;
	  col[2] = (gint)b;
	  col[3] = OPAQUE_OPACITY;
	  /* always use incremental mode with gradients */
	  /* make the gui cool later */
	  paint_appl_mode = INCREMENTAL;
	  color_pixels (temp_buf_data (area), col,
			area->width * area->height, area->bytes);
	}
      /* we check to see if this is a pixmap, if so composite the
	 pixmap image into the are instead of the color */
      else if (GIMP_IS_BRUSH_PIXMAP (paint_core->brush))
	{
	  paint_core_color_area_with_pixmap (paint_core, gimage, drawable, area, 
					     scale, SOFT);
	  paint_appl_mode = INCREMENTAL;
	}
      else 
	{
	  gimage_get_foreground (gimage, drawable, col);
	  col[area->bytes - 1] = OPAQUE_OPACITY;
	  color_pixels (temp_buf_data (area), col,
			area->width * area->height, area->bytes);
	}

      opacity = (gdouble)temp_blend;
      if (pressure_options->opacity)
	opacity = opacity * 2.0 * paint_core->curpressure;

      paint_core_paste_canvas (paint_core, drawable, 
			       MIN (opacity, 255), 
			       gimp_context_get_opacity (NULL) * 255,
			       gimp_context_get_paint_mode (NULL),
			       pressure_options->pressure ? PRESSURE : SOFT,
			       scale, paint_appl_mode);
    }
}


static void *
paintbrush_non_gui_paint_func (PaintCore    *paint_core,
			       GimpDrawable *drawable,
			       int           state)
{
  GImage *gimage;
  double fade_out;
  double gradient_length;	
  double unit_factor;

  if (! (gimage = drawable_gimage (drawable)))
    return NULL;

  switch (non_gui_fade_unit)
    {
    case UNIT_PIXEL:
      fade_out = non_gui_fade_out;
      break;
    case UNIT_PERCENT:
      fade_out = MAX (gimage->width, gimage->height) * 
	non_gui_fade_out / 100;
      break;
    default:
      unit_factor = gimp_unit_get_factor (non_gui_fade_unit);
      fade_out = non_gui_fade_out * 
	MAX (gimage->xresolution, gimage->yresolution) / unit_factor;
      break;
    }
  
  switch (non_gui_gradient_unit)
    {
    case UNIT_PIXEL:
      gradient_length = non_gui_gradient_length;
      break;
    case UNIT_PERCENT:
      gradient_length = MAX (gimage->width, gimage->height) * 
	non_gui_gradient_length / 100;
      break;
    default:
      unit_factor = gimp_unit_get_factor (non_gui_gradient_unit);
      gradient_length = non_gui_gradient_length * 
	MAX (gimage->xresolution, gimage->yresolution) / unit_factor;
      break;
    }

  paintbrush_motion (paint_core,drawable, 
		     &non_gui_pressure_options,
		     fade_out,
		     gradient_length,
		     non_gui_incremental, 
		     non_gui_gradient_type);

  return NULL;
}

gboolean
paintbrush_non_gui_default (GimpDrawable *drawable,
			    int            num_strokes,
			    double        *stroke_array)
{
  PaintbrushOptions *options = paintbrush_options;
  double             fade_out        = PAINTBRUSH_DEFAULT_FADE_OUT;
  gboolean           incremental     = PAINTBRUSH_DEFAULT_INCREMENTAL;
  gboolean           use_gradient    = PAINTBRUSH_DEFAULT_USE_GRADIENT;
  gboolean           use_fade        = PAINTBRUSH_DEFAULT_USE_FADE;
  double             gradient_length = PAINTBRUSH_DEFAULT_GRADIENT_LENGTH;
  int                gradient_type   = PAINTBRUSH_DEFAULT_GRADIENT_TYPE;
  GUnit              fade_unit       = PAINTBRUSH_DEFAULT_FADE_UNIT;
  GUnit              gradient_unit   = PAINTBRUSH_DEFAULT_GRADIENT_UNIT;
  int                i;

  if (options)
    {
      fade_out        = options->fade_out;
      incremental     = options->paint_options.incremental;
      use_gradient    = options->use_gradient;
      gradient_length = options->gradient_length;
      gradient_type   = options->gradient_type;
      use_fade        = options->use_fade;
      fade_unit       = options->fade_unit;
      gradient_unit   = options->gradient_unit;
    }

  if (use_gradient == FALSE)
    gradient_length = 0.0;

  if (use_fade == FALSE)
    fade_out = 0.0;

  /* Hmmm... PDB paintbrush should have gradient type added to it!*/
  /* thats why the code below is duplicated.
   */ 
  if (paint_core_init (&non_gui_paint_core, drawable,
		       stroke_array[0], stroke_array[1]))
    {
      non_gui_fade_out        = fade_out;
      non_gui_gradient_length = gradient_length;
      non_gui_gradient_type   = gradient_type;
      non_gui_incremental     = incremental;
      non_gui_fade_unit       = fade_unit;
      non_gui_gradient_unit   = gradient_unit;

      /* Set the paint core's paint func */
      non_gui_paint_core.paint_func = paintbrush_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      non_gui_paint_core.flags |= TOOL_CAN_HANDLE_CHANGING_BRUSH;

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

gboolean
paintbrush_non_gui (GimpDrawable *drawable,
                   int            num_strokes,
                   double        *stroke_array,
                   double         fade_out,
                   int            method,
                   double         gradient_length)
{
  int i;

  /* Code duplicated above */
  if (paint_core_init (&non_gui_paint_core, drawable,
                      stroke_array[0], stroke_array[1]))
    {
      non_gui_fade_out        = fade_out;
      non_gui_gradient_length = gradient_length;
      non_gui_gradient_type   = LOOP_TRIANGLE;
      non_gui_incremental     = method;

      /* Set the paint core's paint func */
      non_gui_paint_core.paint_func = paintbrush_non_gui_paint_func;

      non_gui_paint_core.startx = non_gui_paint_core.lastx = stroke_array[0];
      non_gui_paint_core.starty = non_gui_paint_core.lasty = stroke_array[1];

      non_gui_paint_core.flags |= TOOL_CAN_HANDLE_CHANGING_BRUSH;

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
