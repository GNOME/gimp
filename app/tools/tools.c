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
#include "eraser.h"
#include "gdisplay.h"
#include "hue_saturation.h"
#include "ellipse_select.h"
#include "flip_tool.h"
#include "free_select.h"
#include "fuzzy_select.h"
#include "gimprc.h"
#include "histogram_tool.h"
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

/* Local Data */

static GtkWidget *options_shell = NULL;
static GtkWidget *options_vbox = NULL;
static ToolType active_tool_type = -1;

static int global_tool_ID = 0;

ToolInfo tool_info[] =
{
  { NULL, "Rect Select", 0 },
  { NULL, "Ellipse Select", 1 },
  { NULL, "Free Select", 2 },
  { NULL, "Fuzzy Select", 3 },
  { NULL, "Bezier Select", 4 },
  { NULL, "Intelligent Scissors", 5 },
  { NULL, "Move", 6 },
  { NULL, "Magnify", 7 },
  { NULL, "Crop", 8 },
  { NULL, "Transform", 9 }, /* rotate */
  { NULL, "Transform", 9 }, /* scale */
  { NULL, "Transform", 9 }, /* shear */
  { NULL, "Transform", 9 }, /* perspective */
  { NULL, "Flip", 10 }, /* horizontal */
  { NULL, "Flip", 10 }, /* vertical */
  { NULL, "Text", 11 },
  { NULL, "Color Picker", 12 },
  { NULL, "Bucket Fill", 13 },
  { NULL, "Blend", 14 },
  { NULL, "Pencil", 15 },
  { NULL, "Paintbrush", 16 },
  { NULL, "Eraser", 17 },
  { NULL, "Airbrush", 18 },
  { NULL, "Clone", 19 },
  { NULL, "Convolve", 20 },

  /*  Non-toolbox tools  */
  { NULL, "By Color Select", 21 },
  { NULL, "Color Balance", 22 },
  { NULL, "Brightness-Contrast", 23 },
  { NULL, "Hue-Saturation", 24 },
  { NULL, "Posterize", 25 },
  { NULL, "Threshold", 26 },
  { NULL, "Curves", 27 },
  { NULL, "Levels", 28 },
  { NULL, "Histogram", 29 }
};


/*  Local function declarations  */

static void tools_options_dialog_callback (GtkWidget *, gpointer);
static gint tools_options_delete_callback (GtkWidget *, GdkEvent *, gpointer);


/* Function definitions */

static void
active_tool_free (void)
{
  gtk_container_disable_resize (GTK_CONTAINER (options_shell));
  
  if (tool_info[(int) active_tool->type].tool_options)
    gtk_widget_hide (tool_info[(int) active_tool->type].tool_options);
  
  switch (active_tool->type)
    {
    case RECT_SELECT:
      tools_free_rect_select (active_tool);
      break;
    case ELLIPSE_SELECT:
      tools_free_ellipse_select (active_tool);
      break;
    case FREE_SELECT:
      tools_free_free_select (active_tool);
      break;
    case FUZZY_SELECT:
      tools_free_fuzzy_select (active_tool);
      break;
    case BEZIER_SELECT:
      tools_free_bezier_select (active_tool);
      break;
    case ISCISSORS:
      tools_free_iscissors (active_tool);
      break;
    case CROP:
      tools_free_crop (active_tool);
      break;
    case MOVE:
      tools_free_move_tool (active_tool);
      break;
    case MAGNIFY:
      tools_free_magnify (active_tool);
      break;
    case ROTATE:
      tools_free_transform_tool (active_tool);
      break;
    case SCALE:
      tools_free_transform_tool (active_tool);
      break;
    case SHEAR:
      tools_free_transform_tool (active_tool);
      break;
    case PERSPECTIVE:
      tools_free_transform_tool (active_tool);
      break;
    case FLIP_HORZ:
      tools_free_flip_tool (active_tool);
      break;
    case FLIP_VERT:
      tools_free_flip_tool (active_tool);
      break;
    case TEXT:
      tools_free_text (active_tool);
      break;
    case COLOR_PICKER:
      tools_free_color_picker (active_tool);
      break;
    case BUCKET_FILL:
      tools_free_bucket_fill (active_tool);
      break;
    case BLEND:
      tools_free_blend (active_tool);
      break;
    case PENCIL:
      tools_free_pencil (active_tool);
      break;
    case PAINTBRUSH:
      tools_free_paintbrush (active_tool);
      break;
    case ERASER:
      tools_free_eraser (active_tool);
      break;
    case AIRBRUSH:
      tools_free_airbrush (active_tool);
      break;
    case CLONE:
      tools_free_clone (active_tool);
      break;
    case CONVOLVE:
      tools_free_convolve (active_tool);
      break;
    case BY_COLOR_SELECT:
      tools_free_by_color_select (active_tool);
      break;
    case COLOR_BALANCE:
      tools_free_color_balance (active_tool);
      break;
    case BRIGHTNESS_CONTRAST:
      tools_free_brightness_contrast (active_tool);
      break;
    case HUE_SATURATION:
      tools_free_hue_saturation (active_tool);
      break;
    case POSTERIZE:
      tools_free_posterize (active_tool);
      break;
    case THRESHOLD:
      tools_free_threshold (active_tool);
      break;
    case CURVES:
      tools_free_curves (active_tool);
      break;
    case LEVELS:
      tools_free_levels (active_tool);
      break;
    case HISTOGRAM:
      tools_free_histogram_tool (active_tool);
      break;
    default:
      return;
    }

  g_free (active_tool);
  active_tool = NULL;
  active_tool_layer = NULL;
}


void
tools_select (ToolType type)
{
  if (active_tool)
    active_tool_free ();

  switch (type)
    {
    case RECT_SELECT:
      active_tool = tools_new_rect_select ();
      break;
    case ELLIPSE_SELECT:
      active_tool = tools_new_ellipse_select ();
      break;
    case FREE_SELECT:
      active_tool = tools_new_free_select ();
      break;
    case FUZZY_SELECT:
      active_tool = tools_new_fuzzy_select ();
      break;
    case BEZIER_SELECT:
      active_tool = tools_new_bezier_select ();
      break;
    case ISCISSORS:
      active_tool = tools_new_iscissors ();
      break;
    case MOVE:
      active_tool = tools_new_move_tool ();
      break;
    case MAGNIFY:
      active_tool = tools_new_magnify ();
      break;
    case CROP:
      active_tool = tools_new_crop ();
      break;
    case ROTATE:
      active_tool = tools_new_transform_tool ();
      break;
    case SCALE:
      active_tool = tools_new_transform_tool ();
      break;
    case SHEAR:
      active_tool = tools_new_transform_tool ();
      break;
    case PERSPECTIVE:
      active_tool = tools_new_transform_tool ();
      break;
    case FLIP_HORZ:
      active_tool = tools_new_flip ();
      break;
    case FLIP_VERT:
      active_tool = tools_new_flip ();
      break;
    case TEXT:
      active_tool = tools_new_text ();
      break;
    case COLOR_PICKER:
      active_tool = tools_new_color_picker ();
      break;
    case BUCKET_FILL:
      active_tool = tools_new_bucket_fill ();
      break;
    case BLEND:
      active_tool = tools_new_blend ();
      break;
    case PENCIL:
      active_tool = tools_new_pencil ();
      break;
    case PAINTBRUSH:
      active_tool = tools_new_paintbrush ();
      break;
    case ERASER:
      active_tool = tools_new_eraser ();
      break;
    case AIRBRUSH:
      active_tool = tools_new_airbrush ();
      break;
    case CLONE:
      active_tool = tools_new_clone ();
      break;
    case CONVOLVE:
      active_tool = tools_new_convolve ();
      break;
    case BY_COLOR_SELECT:
      active_tool = tools_new_by_color_select ();
      break;
    case COLOR_BALANCE:
      active_tool = tools_new_color_balance ();
      break;
    case BRIGHTNESS_CONTRAST:
      active_tool = tools_new_brightness_contrast ();
      break;
    case HUE_SATURATION:
      active_tool = tools_new_hue_saturation ();
      break;
    case POSTERIZE:
      active_tool = tools_new_posterize ();
      break;
    case THRESHOLD:
      active_tool = tools_new_threshold ();
      break;
    case CURVES:
      active_tool = tools_new_curves ();
      break;
    case LEVELS:
      active_tool = tools_new_levels ();
      break;
    case HISTOGRAM:
      active_tool = tools_new_histogram_tool ();
      break;
    default:
      return;
    }

  /*  Show the options for the active tool
   */
  if (tool_info[(int) active_tool->type].tool_options)
    gtk_widget_show (tool_info[(int) active_tool->type].tool_options);

  gtk_container_enable_resize (GTK_CONTAINER (options_shell));

  /*  Set the paused count variable to 0
   */
  active_tool->paused_count = 0;
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
  if (!active_tool)
    active_tool_control (RECREATE, gdisplay_active());

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
  if (action == RECREATE && active_tool_type >= 0)
    tools_select (active_tool_type);
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

  return FALSE;
}

static void
tools_options_dialog_callback (GtkWidget *w,
			       gpointer   client_data)
{
  GtkWidget *shell;

  shell = (GtkWidget *) client_data;
  gtk_widget_hide (shell);
}


