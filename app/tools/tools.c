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
#include "context_manager.h"
#include "convolve.h"
#include "crop.h"
#include "curves.h"
#include "devices.h"
#include "dodgeburn.h"
#include "eraser.h"
#include "gdisplay.h"
#include "hue_saturation.h"
#include "ellipse_select.h"
#include "flip_tool.h"
#include "free_select.h"
#include "fuzzy_select.h"
#include "histogram_tool.h"
#include "ink.h"
#include "iscissors.h"
#include "levels.h"
#include "magnify.h"
#include "measure.h"
#include "move.h"
#include "paintbrush.h"
#include "pencil.h"
#include "pixmapbrush.h"
#include "posterize.h"
#include "rect_select.h"
#include "session.h"
#include "smudge.h"
#include "text_tool.h"
#include "threshold.h"
#include "tools.h"
#include "transform_tool.h"
#include "dialog_handler.h"

#include "config.h"
#include "libgimp/gimpintl.h"

#include "pixmaps2.h"

/*  Global Data  */
Tool * active_tool = NULL;

/*  Local  Data  */
static GtkWidget *options_shell = NULL;
static GtkWidget *options_vbox = NULL;
static GtkWidget *options_label = NULL;
static GtkWidget *options_reset_button = NULL;

static gint global_tool_ID = 0;

ToolInfo tool_info[] =
{
  {
    NULL,
    N_("Rect Select"),
    0,
    N_("/Tools/Rect Select"),
    "R",
    (char **) rect_bits,
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
    N_("Crop & Resize"),
    8,
    N_("/Tools/Crop & Resize"),
    "<shift>C",
    (char **) crop_bits,
    N_("Crop or resize the image"),
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
    (char **) flip_bits,
    N_("Flip the layer or selection"),
    "ContextHelp/flip",
    FLIP,
    tools_new_flip,
    tools_free_flip_tool,
    NULL
  },
    
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

  {
    NULL,
    N_("Dodge or Burn"),
    22,
    N_("/Tools/DodgeBurn"),
    "<shift>D",
    (char **) dodge_bits,
    N_("Dodge or Burn"),
    "ContextHelp/dodgeburn",
    DODGEBURN,
    tools_new_dodgeburn,
    tools_free_dodgeburn,
    NULL
  },

  {
    NULL,
    N_("Smudge"),
    23,
    N_("/Tools/Smudge"),
    "<shift>S",
    (char **) smudge_bits,
    N_("Smudge"),
    "ContextHelp/smudge",
    SMUDGE,
    tools_new_smudge,
    tools_free_smudge,
    NULL
  },

  {
    NULL,
    N_("Pixmap brush"),
    16,
    N_("/Tools/Pixmap Brush"),
    "",
    (char **) paint_bits,
    N_("Paint fuzzy brush strokes"),
    "ContextHelp/paintbrush",
    PIXMAPBRUSH,
    tools_new_pixmapbrush,
    tools_free_pixmapbrush, 
    NULL
  },

  {
    NULL,
    N_("Measure"),
    16,
    N_("/Tools/Measure"),
    "",
    (char **) measure_bits,
    N_("Measure distances and angles"),
    "ContextHelp/measure",
    MEASURE,
    tools_new_measure_tool,
    tools_free_measure_tool, 
    NULL
  },

  /*  Non-toolbox tools  */
  { 
    NULL,
    N_("By Color Select"),
    24,
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
    25,
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
    26,
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
    27,
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
    28,
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
    29,
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
    30,
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
    31,
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
    32,
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

gint num_tools = sizeof (tool_info) / sizeof (tool_info[0]);


/*  Local function declarations  */
static void tools_options_show             (ToolType);
static void tools_options_hide             (ToolType);
static void tools_options_reset_callback   (GtkWidget *, gpointer);
static void tools_options_close_callback   (GtkWidget *, gpointer);
static gint tools_options_delete_callback  (GtkWidget *, GdkEvent *, gpointer);


/*  Function definitions  */

static void
active_tool_free (void)
{
  if (!active_tool)
    return;

  tools_options_hide (active_tool->type);

  (* tool_info[(int) active_tool->type].free_func) (active_tool);

  g_free (active_tool);
  active_tool = NULL;
}

void
tools_select (ToolType type)
{
  /*  Care for switching to the tool's private context  */
  context_manager_set_tool (type);

  if (active_tool)
    active_tool_free ();

  active_tool = (* tool_info[(int) type].new_func) ();

  tools_options_show (active_tool->type);

  /*  Update the device-information dialog  */
  device_status_update (current_device);
}

void
tools_initialize (ToolType  type,
		  GDisplay *gdisp)
{
  /*  Tools which have an init function have dialogs and
   *  cannot be initialized without a display
   */
  if (tool_info[(int) type].init_func && !gdisp)
    type = RECT_SELECT;

  /*  Activate the appropriate widget.
   *  Implicitly calls tools_select()
   */
  if (active_tool->type == type)
    {
      tools_select (type);
    }
  else
    {
      gtk_widget_activate (tool_info[type].tool_widget);
    }

  if (tool_info[(int) type].init_func)
    {
      (* tool_info[(int) type].init_func) (gdisp);

      active_tool->drawable = gimage_active_drawable (gdisp->gimage);
    }

  /*  don't set gdisp_ptr here! (see commands.c)  */
}

void
tools_options_dialog_show ()
{
  if (!GTK_WIDGET_VISIBLE (options_shell)) 
    {
      gtk_widget_show (options_shell);
    } 
  else 
    {
      gdk_window_raise (options_shell->window);
    }
}

void
active_tool_control (ToolAction  action,
		     void       *gdisp_ptr)
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
              active_tool_free ();
              gtk_widget_hide (options_shell);
              break;

	    default:
	      break;
	    }
	}
      else if (action == HALT)
	{
	  active_tool->state = INACTIVE;
	}
    }
}

/*  standard member functions  */

static void
standard_button_press_func (Tool           *tool,
			    GdkEventButton *bevent,
			    gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = gdisp_ptr;

  tool->gdisp_ptr = gdisp;
  tool->drawable = gimage_active_drawable (gdisp->gimage);
}

static void
standard_button_release_func (Tool           *tool,
			      GdkEventButton *bevent,
			      gpointer        gdisp_ptr)
{
}

static void
standard_motion_func (Tool           *tool,
		      GdkEventMotion *mevent,
		      gpointer        gdisp_ptr)
{
}

static void
standard_arrow_keys_func (Tool        *tool,
			  GdkEventKey *kevent,
			  gpointer     gdisp_ptr)
{
}

static void
standard_modifier_key_func (Tool        *tool,
			    GdkEventKey *kevent,
			    gpointer     gdisp_ptr)
{
}

static void
standard_cursor_update_func (Tool           *tool,
			     GdkEventMotion *mevent,
			     gpointer        gdisp_ptr)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) gdisp_ptr;
  gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
standard_control_func (Tool       *tool,
		       ToolAction  action,
		       gpointer    gdisp_ptr)
{
}

/*  Create a default tool structure 
 *
 *  TODO: objectifying the tools will remove lots of code duplication
 */

Tool *
tools_new_tool (ToolType type)
{
  Tool *tool;

  tool = g_new (Tool, 1);

  tool->type = type;
  tool->ID   = global_tool_ID++;

  tool->state        = INACTIVE;
  tool->paused_count = 0;
  tool->scroll_lock  = FALSE;     /*  Allow scrolling  */
  tool->auto_snap_to = TRUE;      /*  Snap to guides   */

  tool->preserve  = TRUE;         /*  Preserve tool across drawable changes  */
  tool->gdisp_ptr = NULL;
  tool->drawable  = NULL;

  tool->private   = NULL;

  tool->button_press_func   = standard_button_press_func;
  tool->button_release_func = standard_button_release_func;
  tool->motion_func         = standard_motion_func;
  tool->arrow_keys_func     = standard_arrow_keys_func;
  tool->modifier_key_func   = standard_modifier_key_func;
  tool->cursor_update_func  = standard_cursor_update_func;
  tool->control_func        = standard_control_func;

  return tool;
}

/*  Tool options function  */

void
tools_options_dialog_new ()
{
  GtkWidget *frame;
  GtkWidget *vbox;

  ActionAreaItem action_items[] =
  {
    { N_("Reset"), tools_options_reset_callback, NULL, NULL },
    { N_("Close"), tools_options_close_callback, NULL, NULL }
  };

  /*  The shell and main vbox  */
  options_shell = gtk_dialog_new ();

  /*  Register dialog */
  dialog_register (options_shell);

  gtk_window_set_wmclass (GTK_WINDOW (options_shell), "tool_options", "Gimp");
  gtk_window_set_title (GTK_WINDOW (options_shell), _("Tool Options"));
  gtk_window_set_policy (GTK_WINDOW (options_shell), FALSE, TRUE, TRUE);
  session_set_window_geometry (options_shell, &tool_options_session_info,
			       FALSE );

  /*  The outer frame  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options_shell)->vbox), frame);
  gtk_widget_show (frame);

  /*  The vbox containing the title frame and the options vbox  */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /*  The title frame  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  options_label = gtk_label_new ("");
  gtk_misc_set_padding (GTK_MISC (options_label), 1, 0);
  gtk_container_add (GTK_CONTAINER (frame), options_label);
  gtk_widget_show (options_label);

  options_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (options_vbox), 2);
  gtk_box_pack_start (GTK_BOX (vbox), options_vbox, FALSE, FALSE, 0);

  /*  handle the window manager trying to close the window  */
  gtk_signal_connect (GTK_OBJECT (options_shell), "delete_event",
		      GTK_SIGNAL_FUNC (tools_options_delete_callback),
		      options_shell);

  action_items[0].user_data = options_shell;
  action_items[1].user_data = options_shell;
  build_action_area (GTK_DIALOG (options_shell), action_items, 2, 1);

  options_reset_button = action_items[0].widget;

  gtk_widget_show (options_vbox);

  /*  hide the separator between the dialog's vbox and the action area  */
  gtk_widget_hide (GTK_WIDGET (g_list_nth_data (gtk_container_children (GTK_CONTAINER (GTK_BIN (options_shell)->child)), 1)));
}

void
tools_options_dialog_free ()
{
  session_get_window_info (options_shell, &tool_options_session_info);
  gtk_widget_destroy (options_shell);
}

void
tools_register (ToolType     tool_type,
		ToolOptions *tool_options)
{
  g_return_if_fail (tool_options != NULL);

  tool_info [(int) tool_type].tool_options = tool_options;

  /*  need to check whether the widget is visible...this can happen
   *  because some tools share options such as the transformation tools
   */
  if (! GTK_WIDGET_VISIBLE (tool_options->main_vbox))
    {
      gtk_box_pack_start (GTK_BOX (options_vbox), tool_options->main_vbox,
			  TRUE, TRUE, 0);
      gtk_widget_show (tool_options->main_vbox);
    }

  gtk_label_set_text (GTK_LABEL (options_label), _(tool_options->title));
}

static void
tools_options_show (ToolType tooltype)
{
  if (tool_info[tooltype].tool_options->main_vbox)
    gtk_widget_show (tool_info[tooltype].tool_options->main_vbox);

  if (tool_info[tooltype].tool_options->title)
    gtk_label_set_text (GTK_LABEL (options_label),
			_(tool_info[tooltype].tool_options->title));

  if (tool_info[tooltype].tool_options->reset_func)
    gtk_widget_set_sensitive (options_reset_button, TRUE);
  else
    gtk_widget_set_sensitive (options_reset_button, FALSE);
}

static void
tools_options_hide (ToolType tooltype)
{
  if (tool_info[tooltype].tool_options)
    gtk_widget_hide (tool_info[tooltype].tool_options->main_vbox);
}

/*  Tool options callbacks  */

static gint
tools_options_delete_callback (GtkWidget *w,
			       GdkEvent  *e,
			       gpointer   client_data)
{
  tools_options_close_callback (w, client_data);

  return TRUE;
}

static void
tools_options_close_callback (GtkWidget *w,
			      gpointer   client_data)
{
  GtkWidget *shell;

  shell = (GtkWidget *) client_data;
  gtk_widget_hide (shell);
}

static void
tools_options_reset_callback (GtkWidget *w,
			      gpointer   client_data)
{
  GtkWidget *shell;

  shell = (GtkWidget *) client_data;

  if (! active_tool)
    return;

  if (tool_info[(int) active_tool->type].tool_options->reset_func)
    (* tool_info[(int) active_tool->type].tool_options->reset_func) ();
}
