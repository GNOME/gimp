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
#include "session.h"
#include "text_tool.h"
#include "threshold.h"
#include "tools.h"
#include "transform_tool.h"

#include "libgimp/gimpintl.h"

#include "pixmaps2.h"

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
  {
    NULL,
    N_("Rect Select"),
    0,
    N_("/Tools/Rect Select"),
    "R",
    (char **)rect_bits,
    N_("Select rectangular regions"),
    "ContextHelp/rect-select",
    RECT_SELECT,
    tools_new_rect_select,
    tools_free_rect_select, 
    NULL
  },

  {
    NULL,
    N_("Ellipse Select"),
    1,
    N_("/Tools/Ellipse Select"),
    "E",
    (char **) circ_bits,
    N_("Select elliptical regions"),
    "ContextHelp/ellipse-select",
    ELLIPSE_SELECT,
    tools_new_ellipse_select,
    tools_free_ellipse_select,
    NULL
  },

  {
    NULL, 
    N_("Free Select"), 
    2, 
    N_("/Tools/Free Select"),
    "F",
    (char **) free_bits,
    N_("Select hand-drawn regions"),
    "ContextHelp/free-select",
    FREE_SELECT,
    tools_new_free_select,
    tools_free_free_select, 
    NULL
  },
  
  {
    NULL,
    N_("Fuzzy Select"),
    3,
    N_("/Tools/Fuzzy Select"),
    "Z",
    (char **) fuzzy_bits,
    N_("Select contiguous regions"),
    "ContextHelp/fuzzy-select",
    FUZZY_SELECT,
    tools_new_fuzzy_select,
    tools_free_fuzzy_select, 
    NULL
  },
  
  {
    NULL,
    N_("Bezier Select"),
    4,
    N_("/Tools/Bezier Select"),
    "B",
    (char **) bezier_bits,
    N_("Select regions using Bezier curves"),
    "ContextHelp/bezier-select",
    BEZIER_SELECT,
    tools_new_bezier_select,
    tools_free_bezier_select,
    NULL
  },
  
  {
    NULL,
    N_("Intelligent Scissors"),
    5,
    N_("/Tools/Intelligent Scissors"),
    "I",
    (char **) iscissors_bits,
    N_("Select shapes from image"),
    "ContextHelp/iscissors",
    ISCISSORS,
    tools_new_iscissors,
    tools_free_iscissors, 
    NULL
  },
  
  {
    NULL, 
    N_("Move"),
    6,
    N_("/Tools/Move"),
    "M",
    (char **) move_bits,
    N_("Move layers & selections"),
    "ContextHelp/move",
    MOVE,
    tools_new_move_tool,
    tools_free_move_tool, 
    NULL
  },

  {
    NULL,
    N_("Magnify"),
    7,
    N_("/Tools/Magnify"),
    "<shift>M",
    (char **) magnify_bits,
    N_("Zoom in & out"),
    "ContextHelp/magnify",
    MAGNIFY,
    tools_new_magnify,
    tools_free_magnify, 
    NULL
  },

  {
    NULL,
    N_("Crop"),
    8,
    N_("/Tools/Crop"),
    "<shift>C",
    (char **) crop_bits,
    N_("Crop the image"),
    "ContextHelp/crop",
    CROP,
    tools_new_crop,
    tools_free_crop,
    NULL
  },
  
  {
    NULL,
    N_("Transform"),
    9,
    N_("/Tools/Transform"),
    "<shift>T",
    (char **) scale_bits,
    N_("Transform the layer or selection"),
    "ContextHelp/rotate",
    ROTATE,
    tools_new_transform_tool,
    tools_free_transform_tool,
    NULL
  }, /* rotate */
  
  {
    NULL,
    N_("Transform"),
    9,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    SCALE,
    tools_new_transform_tool,
    tools_free_transform_tool, 
    NULL
  }, /* scale */
  
  {
    NULL, 
    N_("Transform"),
    9,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    SHEAR,
    tools_new_transform_tool,
    tools_free_transform_tool,
    NULL
  }, /* shear */
  
  {
    NULL, 
    N_("Transform"),
    9,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    PERSPECTIVE,
    tools_new_transform_tool,
    tools_free_transform_tool,
    NULL
  }, /* perspective */
  
  {
    NULL,
    N_("Flip"),
    10,
    N_("/Tools/Flip"),
    "<shift>F",
    (char **) horizflip_bits,
    N_("Flip the layer or selection"),
    "ContextHelp/flip",
    FLIP_HORZ,
    tools_new_flip,
    tools_free_flip_tool,
    NULL
  }, /* horizontal */
  
  {
    NULL,
    N_("Flip"),
    10,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    FLIP_VERT,
    tools_new_flip, 
    tools_free_flip_tool,
    NULL
  }, /* vertical */
  
  {
    NULL,
    N_("Text"),
    11,
    N_("/Tools/Text"),
    "T",
    (char **) text_bits,
    N_("Add text to the image"),
    "ContextHelp/text",
    TEXT,
    tools_new_text,
    tools_free_text,
    NULL,
  },
  
  {
    NULL,
    N_("Color Picker"),
    12,
    N_("/Tools/Color Picker"),
    "O",
    (char **) colorpicker_bits,
    N_("Pick colors from the image"),
    "ContextHelp/color-picker",
    COLOR_PICKER,
    tools_new_color_picker,
    tools_free_color_picker,
    NULL
  },
  
  { 
    NULL,
    N_("Bucket Fill"),
    13,
    N_("/Tools/Bucket Fill"),
    "<shift>B",
    (char **) fill_bits,
    N_("Fill with a color or pattern"),
    "ContextHelp/bucket-fill",
    BUCKET_FILL,
    tools_new_bucket_fill,
    tools_free_bucket_fill,
    NULL
  },

  { 
    NULL,
    N_("Blend"),
    14,
    N_("/Tools/Blend"),
    "L",
    (char **) gradient_bits,
    N_("Fill with a color gradient"),
    "ContextHelp/gradient",
    BLEND,
    tools_new_blend,
    tools_free_blend,
    NULL
  },
  
  {
    NULL,
    N_("Pencil"),
    15,
    N_("/Tools/Pencil"),
    "<shift>P",
    (char **) pencil_bits,
    N_("Draw sharp pencil strokes"),
    "ContextHelp/pencil",
    PENCIL,
    tools_new_pencil,
    tools_free_pencil,
    NULL
  },
  
  {
    NULL,
    N_("Paintbrush"),
    16,
    N_("/Tools/Paintbrush"),
    "P",
    (char **) paint_bits,
    N_("Paint fuzzy brush strokes"),
    "ContextHelp/paintbrush",
    PAINTBRUSH,
    tools_new_paintbrush,
    tools_free_paintbrush, 
    NULL
  },
  
  { 
    NULL,
    N_("Eraser"),
    17,
    N_("/Tools/Eraser"),
    "<shift>E",
    (char **) erase_bits,
    N_("Erase to background or transparency"),
    "ContextHelp/eraser",
    ERASER,
    tools_new_eraser,
    tools_free_eraser,
    NULL
  },
  
  { 
    NULL,
    N_("Airbrush"),
    18,
    N_("/Tools/Airbrush"),
    "A",
    (char **) airbrush_bits,
    N_("Airbrush with variable pressure"),
    "ContextHelp/airbrush",
    AIRBRUSH,
    tools_new_airbrush,
    tools_free_airbrush,
    NULL
  },
  
  { 
    NULL,
    N_("Clone"),
    19,
    N_("/Tools/Clone"),
    "C",
    (char **) clone_bits,
    N_("Paint using patterns or image regions"),
    "ContextHelp/clone",
    CLONE,
    tools_new_clone,
    tools_free_clone,
    NULL
  },
  
  { 
    NULL,
    N_("Convolve"),
    20,
    N_("/Tools/Convolve"),
    "V",
    (char **) blur_bits,
    N_("Blur or sharpen"),
    "ContextHelp/convolve",
    CONVOLVE,
    tools_new_convolve,
    tools_free_convolve,
    NULL
  },

  {
    NULL,
    N_("Ink"),
    21,
    N_("/Tools/Ink"),
    "K",
    (char **) ink_bits,
    N_("Draw in ink"),
    "ContextHelp/ink",
    INK,
    tools_new_ink,
    tools_free_ink,
    NULL
  },

  /*  Non-toolbox tools  */
  { 
    NULL,
    N_("By Color Select"),
    22,
    N_("/Select/By Color..."),
    NULL,
    NULL,
    NULL,
    NULL,
    BY_COLOR_SELECT,
    tools_new_by_color_select,
    tools_free_by_color_select,
    by_color_select_initialize 
  },
  
  { 
    NULL,
    N_("Color Balance"),
    23,
    N_("/Image/Colors/Color Balance"),
    NULL,
    NULL,
    NULL,
    NULL,
    COLOR_BALANCE,
    tools_new_color_balance,
    tools_free_color_balance,
    color_balance_initialize
  },
  
  { 
    NULL,
    N_("Brightness-Contrast"),
    24,
    N_("/Image/Colors/Brightness-Contrast"),
    NULL,
    NULL,
    NULL,
    NULL,
    BRIGHTNESS_CONTRAST,
    tools_new_brightness_contrast,
    tools_free_brightness_contrast,
    brightness_contrast_initialize
  },
  
  { 
    NULL,
    N_("Hue-Saturation"),
    25,
    N_("/Image/Colors/Hue-Saturation"),
    NULL,
    NULL,
    NULL,
    NULL,
    HUE_SATURATION,
    tools_new_hue_saturation,
    tools_free_hue_saturation, 
    hue_saturation_initialize
  },

  { 
    NULL,
    N_("Posterize"),
    26,
    N_("/Image/Colors/Posterize"),
    NULL,
    NULL,
    NULL,
    NULL,
    POSTERIZE,
    tools_new_posterize,
    tools_free_posterize,
    posterize_initialize
  },
  
  { 
    NULL,
    N_("Threshold"), 
    27,
    N_("/Image/Colors/Threshold"),
    NULL,
    NULL,
    NULL,
    NULL,
    THRESHOLD,
    tools_new_threshold,
    tools_free_threshold,
    threshold_initialize
  },
  
  { 
    NULL,
    N_("Curves"),
    28,
    N_("/Image/Colors/Curves"),
    NULL,
    NULL,
    NULL,
    NULL,
    CURVES,
    tools_new_curves, 
    tools_free_curves,
    curves_initialize
  },
  
  { 
    NULL,
    N_("Levels"),
    29,
    N_("/Image/Colors/Levels"),
    NULL,
    NULL,
    NULL,
    NULL,
    LEVELS,
    tools_new_levels, 
    tools_free_levels,
    levels_initialize
  },
  
  { 
    NULL,
    N_("Histogram"),
    30,
    N_("/Image/Histogram"),
    NULL,
    NULL,
    NULL,
    NULL,
    HISTOGRAM,
    tools_new_histogram_tool,
    tools_free_histogram_tool,
    histogram_tool_initialize 
  }
};

gint num_tools = sizeof (tool_info) / sizeof (ToolInfo);

/*  Local function declarations  */

static void tools_options_dialog_callback (GtkWidget *, gpointer);
static gint tools_options_delete_callback (GtkWidget *, GdkEvent *, gpointer);


/* Function definitions */

static void
active_tool_free (void)
{
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

  /*  Set the paused count variable to 0
   */
  active_tool->paused_count = 0;
  active_tool->gdisp_ptr = NULL;
  active_tool->drawable = NULL;
  active_tool->ID = global_tool_ID++;
  active_tool_type = active_tool->type;

  /* Update the device-information dialog */
  
  device_status_update (current_device);
}

void
tools_initialize (ToolType type, GDisplay *gdisp)
{
  /* If you're wondering... only these dialog type tools have init functions */
  if (active_tool)
    active_tool_free ();

  if (tool_info[(int) type].init_func)
    {
      if (gdisp) {
	active_tool = (* tool_info[(int) type].new_func) ();
	(* tool_info[(int) type].init_func) (gdisp);
      } else {
	active_tool = tools_new_rect_select ();
      }
    }
  else 
    active_tool = (* tool_info[(int) type].new_func) ();

  /*  Show the options for the active tool
   */
  if (tool_info[(int) active_tool->type].tool_options)
    gtk_widget_show (tool_info[(int) active_tool->type].tool_options);

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
    { N_("Close"), tools_options_dialog_callback, NULL, NULL }
  };

  /*  The shell and main vbox  */
  options_shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (options_shell), "tool_options", "Gimp");
  gtk_window_set_title (GTK_WINDOW (options_shell), _("Tool Options"));
  gtk_window_set_policy (GTK_WINDOW (options_shell), FALSE, TRUE, TRUE);
  session_set_window_geometry (options_shell, &tool_options_session_info, FALSE );

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
  session_get_window_info (options_shell, &tool_options_session_info);
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
  label = gtk_label_new (_("This tool has no options."));
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


