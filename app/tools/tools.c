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
#include "actionarea.h"
#include "airbrush.h"
#include "bezier_select.h"
#include "blend.h"
#include "brightness_contrast.h"
#include "bucket_fill.h"
#include "by_color_select.h"
#include "clone.h"
#include "color_balance.h"
#include "color_picker.h"
#include "convolve.h"
#include "crop.h"
#include "cursorutil.h"
#include "curves.h"
#include "devices.h"
#include "eraser.h"
#include "gdisplay.h"
#include "hue_saturation.h"
#include "ellipse_select.h"
#include "flip_tool.h"
#include "free_select.h"
#include "fuzzy_select.h"
#include "gimprc.h"
#include "histogram_tool.h"
#include "ink.h"
#include "interface.h"
#include "iscissors.h"
#include "levels.h"
#include "magnify.h"
#include "move.h"
#include "paintbrush.h"
#include "pencil.h"
#include "posterize.h"
#include "rect_select.h"
#include "text_tool.h"
#include "threshold.h"
#include "tools.h"
#include "transform_tool.h"

/* Global Data */

Tool * active_tool = NULL;
Layer * active_tool_layer = NULL;
ToolType active_tool_type = -1;

/* Local Data */

static GtkWidget *options_shell = NULL;
static GtkWidget *options_vbox = NULL;

static int global_tool_ID = 0;

ToolInfo tool_info[] =
{
  { NULL, "Rect Select", 0, tools_new_rect_select, tools_free_rect_select },
  { NULL, "Ellipse Select", 1, tools_new_ellipse_select, tools_free_ellipse_select },
  { NULL, "Free Select", 2, tools_new_free_select, tools_free_free_select },
  { NULL, "Fuzzy Select", 3, tools_new_fuzzy_select, tools_free_fuzzy_select },
  { NULL, "Bezier Select", 4, tools_new_bezier_select, tools_free_bezier_select },
  { NULL, "Intelligent Scissors", 5, tools_new_iscissors, tools_free_iscissors },
  { NULL, "Move", 6, tools_new_move_tool, tools_free_move_tool },
  { NULL, "Magnify", 7, tools_new_magnify, tools_free_magnify },
  { NULL, "Crop", 8, tools_new_crop, tools_free_crop },
  { NULL, "Transform", 9, tools_new_transform_tool, tools_free_transform_tool}, /* rotate */
  { NULL, "Transform", 9, tools_new_transform_tool, tools_free_transform_tool }, /* scale */
  { NULL, "Transform", 9, tools_new_transform_tool, tools_free_transform_tool }, /* shear */
  { NULL, "Transform", 9, tools_new_transform_tool, tools_free_transform_tool }, /* perspective */
  { NULL, "Flip", 10, tools_new_flip, tools_free_flip_tool }, /* horizontal */
  { NULL, "Flip", 10, tools_new_flip, tools_free_flip_tool }, /* vertical */
  { NULL, "Text", 11, tools_new_text, tools_free_text },
  { NULL, "Color Picker", 12, tools_new_color_picker, tools_free_color_picker },
  { NULL, "Bucket Fill", 13, tools_new_bucket_fill, tools_free_bucket_fill },
  { NULL, "Blend", 14, tools_new_blend, tools_free_blend },
  { NULL, "Pencil", 15, tools_new_pencil, tools_free_pencil },
  { NULL, "Paintbrush", 16, tools_new_paintbrush, tools_free_paintbrush },
  { NULL, "Eraser", 17, tools_new_eraser, tools_free_eraser },
  { NULL, "Airbrush", 18, tools_new_airbrush, tools_free_airbrush },
  { NULL, "Clone", 19, tools_new_clone, tools_free_clone },
  { NULL, "Convolve", 20, tools_new_convolve, tools_free_convolve },
  { NULL, "Ink", 21, tools_new_ink, tools_free_ink },

  /*  Non-toolbox tools  */
  { NULL, "By Color Select", 22, tools_new_by_color_select, tools_free_by_color_select },
  { NULL, "Color Balance", 23, tools_new_color_balance, tools_free_color_balance },
  { NULL, "Brightness-Contrast", 24, tools_new_brightness_contrast, tools_free_brightness_contrast },
  { NULL, "Hue-Saturation", 25, tools_new_hue_saturation, tools_free_hue_saturation },
  { NULL, "Posterize", 26, tools_new_posterize, tools_free_posterize },
  { NULL, "Threshold", 27, tools_new_threshold, tools_free_threshold },
  { NULL, "Curves", 28, tools_new_curves, tools_free_curves },
  { NULL, "Levels", 29, tools_new_levels, tools_free_levels },
  { NULL, "Histogram", 30, tools_new_histogram_tool, tools_free_histogram_tool }
};


/*  Local function declarations  */

static void tools_options_dialog_callback (GtkWidget *, gpointer);
static gint tools_options_delete_callback (GtkWidget *, GdkEvent *, gpointer);


/* Function definitions */

static void
active_tool_free (void)
{
  gtk_container_disable_resize (GTK_CONTAINER (options_shell));
  
  if (!active_tool)
    return;

  if (tool_info[(int) active_tool->type].tool_options)
    gtk_widget_hide (tool_info[(int) active_tool->type].tool_options);
  
  (* tool_info[(int) active_tool->type].free_func) (active_tool);

  g_free (active_tool);
  active_tool = NULL;
  active_tool_layer = NULL;
}


void
tools_select (ToolType type)
{
  if (active_tool)
    active_tool_free ();

  active_tool = (* tool_info[(int) type].new_func) ();

  /*  Show the options for the active tool
   */
  if (tool_info[(int) active_tool->type].tool_options)
    gtk_widget_show (tool_info[(int) active_tool->type].tool_options);

  gtk_container_enable_resize (GTK_CONTAINER (options_shell));

  /* Update the device-information dialog */
  
  device_status_update (current_device);

  /*  Set the paused count variable to 0
   */
  active_tool->paused_count = 0;
  active_tool->gdisp_ptr = NULL;
  active_tool->drawable = NULL;
  active_tool->ID = global_tool_ID++;
  active_tool_type = active_tool->type;
}

void
tools_initialize (ToolType type, GDisplay *gdisp_ptr)
{
  GDisplay *gdisp;

  /* If you're wondering... only these dialog type tools have init functions */
  gdisp = gdisp_ptr;

  if (active_tool)
    active_tool_free ();

  switch (type)
    {
    case BY_COLOR_SELECT:
      if (gdisp) {
	active_tool = tools_new_by_color_select ();
	by_color_select_initialize (gdisp->gimage);
      } else {
	active_tool = tools_new_rect_select ();
      }
      break;
    case COLOR_BALANCE:
      if (gdisp) {
	active_tool = tools_new_color_balance ();
	color_balance_initialize (gdisp);
      } else {
	active_tool = tools_new_rect_select ();
      }
      break;
    case BRIGHTNESS_CONTRAST:
      if (gdisp) {
	active_tool = tools_new_brightness_contrast ();
	brightness_contrast_initialize (gdisp);
      } else {
	active_tool = tools_new_rect_select ();
      }
      break;
    case HUE_SATURATION:
      if (gdisp) {
	active_tool = tools_new_hue_saturation ();
	hue_saturation_initialize (gdisp);
      } else {
	active_tool = tools_new_rect_select ();
      }
      break;
    case POSTERIZE:
      if (gdisp) {
	active_tool = tools_new_posterize ();
	posterize_initialize (gdisp);
      } else {
	active_tool = tools_new_rect_select ();
      }
      break;
    case THRESHOLD:
      if (gdisp) {
	active_tool = tools_new_threshold ();
	threshold_initialize (gdisp);
      } else {
	active_tool = tools_new_rect_select ();
      }
      break;
    case CURVES:
      if (gdisp) {
	active_tool = tools_new_curves ();
	curves_initialize (gdisp);
      } else {
	active_tool = tools_new_rect_select ();
      }
      break;
    case LEVELS:
      if (gdisp) {
	active_tool = tools_new_levels ();
	levels_initialize (gdisp);
      } else {
	active_tool = tools_new_rect_select ();
      }
      break;
    case HISTOGRAM:
      if (gdisp) {
	active_tool = tools_new_histogram_tool ();
	histogram_tool_initialize (gdisp);
      } else {
	active_tool = tools_new_rect_select ();
      }
      break;
    default:
      active_tool = (* tool_info[(int) type].new_func) ();
      break;
    }

  /*  Show the options for the active tool
   */
  if (tool_info[(int) active_tool->type].tool_options)
    gtk_widget_show (tool_info[(int) active_tool->type].tool_options);

  gtk_container_enable_resize (GTK_CONTAINER (options_shell));

  /*  Set the paused count variable to 0
   */
  if (gdisp)
    active_tool->drawable = gimage_active_drawable (gdisp->gimage);
  else
    active_tool->drawable = NULL;
  active_tool->gdisp_ptr = NULL;
  active_tool->ID = global_tool_ID++;
  active_tool_type = active_tool->type;
}

void
tools_options_dialog_new ()
{
  ActionAreaItem action_items[1] =
  {
    { "Close", tools_options_dialog_callback, NULL, NULL }
  };

  /*  The shell and main vbox  */
  options_shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (options_shell), "tool_options", "Gimp");
  gtk_window_set_title (GTK_WINDOW (options_shell), "Tool Options");
  gtk_window_set_policy (GTK_WINDOW (options_shell), FALSE, TRUE, TRUE);
  gtk_widget_set_uposition (options_shell, tool_options_x, tool_options_y);

  options_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_border_width (GTK_CONTAINER (options_vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (options_shell)->vbox), options_vbox, TRUE, TRUE, 0);

  /* handle the window manager trying to close the window */
  gtk_signal_connect (GTK_OBJECT (options_shell), "delete_event",
		      GTK_SIGNAL_FUNC (tools_options_delete_callback),
		      options_shell);

  action_items[0].user_data = options_shell;
  build_action_area (GTK_DIALOG (options_shell), action_items, 1, 0);

  gtk_widget_show (options_vbox);
}

void
tools_options_dialog_show ()
{
  /* menus_activate_callback() will destroy the active tool in many
     cases.  if the user tries to bring up the options before
     switching tools, the dialog will be empty.  recreate the active
     tool here if necessary to avoid this behavior */

  if (!GTK_WIDGET_VISIBLE(options_shell)) 
    {
      gtk_widget_show (options_shell);
    } 
  else 
    {
      gdk_window_raise (options_shell->window);
    }
}


void
tools_options_dialog_free ()
{
  gtk_widget_destroy (options_shell);
}


void
tools_register_options (ToolType   type,
			GtkWidget *options)
{
  /*  need to check whether the widget is visible...this can happen
   *  because some tools share options such as the transformation tools
   */
  if (! GTK_WIDGET_VISIBLE (options))
    {
      gtk_box_pack_start (GTK_BOX (options_vbox), options, TRUE, TRUE, 0);
      gtk_widget_show (options);
    }
  tool_info [(int) type].tool_options = options;
}

void *
tools_register_no_options (ToolType  tool_type,
			   char     *tool_title)
{
  GtkWidget *vbox;
  GtkWidget *label;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);

  /*  the main label  */
  label = gtk_label_new (tool_title);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  this tool has no special options  */
  label = gtk_label_new ("This tool has no options.");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  Register this selection options widget with the main tools options dialog  */
  tools_register_options (tool_type, vbox);

  return (void *) 1;
}

void
active_tool_control (int   action,
		     void *gdisp_ptr)
{
  if (active_tool)
    {
      if (active_tool->gdisp_ptr == gdisp_ptr)
	{
	  switch (action)
	    {
	    case PAUSE :
	      if (active_tool->state == ACTIVE)
		{
		  if (! active_tool->paused_count)
		    {
		      active_tool->state = PAUSED;
		      (* active_tool->control_func) (active_tool, action, gdisp_ptr);
		    }
		}
	      active_tool->paused_count++;

	      break;
	    case RESUME :
	      active_tool->paused_count--;
	      if (active_tool->state == PAUSED)
		{
		  if (! active_tool->paused_count)
		    {
		      active_tool->state = ACTIVE;
		      (* active_tool->control_func) (active_tool, action, gdisp_ptr);
		    }
		}
	      break;
	    case HALT :
	      active_tool->state = INACTIVE;
	      (* active_tool->control_func) (active_tool, action, gdisp_ptr);
	      break;
	    case DESTROY :
              active_tool_free();
              gtk_widget_hide (options_shell);
              break;
	    }
	}
      else if (action == HALT)
	active_tool->state = INACTIVE;
    }
}


void
standard_arrow_keys_func (Tool        *tool,
			  GdkEventKey *kevent,
			  gpointer     gdisp_ptr)
{
}

static gint
tools_options_delete_callback (GtkWidget *w,
			       GdkEvent *e,
			       gpointer   client_data)
{
  tools_options_dialog_callback (w, client_data);

  return TRUE;
}

static void
tools_options_dialog_callback (GtkWidget *w,
			       gpointer   client_data)
{
  GtkWidget *shell;

  shell = (GtkWidget *) client_data;
  gtk_widget_hide (shell);
}


