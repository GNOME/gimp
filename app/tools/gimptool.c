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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "appenv.h"
#include "context_manager.h"
#include "gdisplay.h"
#include "gimpcontext.h"
#include "gimpdnd.h"
#include "gimpimage.h"
#include "gimpui.h"
#include "session.h"
#include "dialog_handler.h"

#include "gimptool.h"
#include "tool_options.h"
#include "tool_manager.h"

#include "libgimp/gimpintl.h"


enum
{
  INITIALIZE,
  CONTROL,
  BUTTON_PRESS,
  BUTTON_RELEASE,
  MOTION,
  ARROW_KEY,
  MODIFIER_KEY,
  CURSOR_UPDATE,
  OPER_UPDATE,
  LAST_SIGNAL
};


static void   gimp_tool_class_init          (GimpToolClass  *klass);
static void   gimp_tool_init                (GimpTool       *tool);

static void   gimp_tool_destroy             (GtkObject      *destroy);

static void   gimp_tool_real_initialize     (GimpTool       *tool,
					     GDisplay       *gdisp);
static void   gimp_tool_real_control        (GimpTool       *tool,
					     ToolAction      tool_action,
					     GDisplay       *gdisp);
static void   gimp_tool_real_button_press   (GimpTool       *tool,
					     GdkEventButton *bevent,
					     GDisplay       *gdisp);
static void   gimp_tool_real_button_release (GimpTool       *tool,
					     GdkEventButton *bevent,
					     GDisplay       *gdisp);
static void   gimp_tool_real_motion         (GimpTool       *tool,
					     GdkEventMotion *mevent,
					     GDisplay       *gdisp);
static void   gimp_tool_real_arrow_key      (GimpTool       *tool,
					     GdkEventKey    *kevent,
					     GDisplay       *gdisp);
static void   gimp_tool_real_modifier_key   (GimpTool       *tool,
					     GdkEventKey    *kevent,
					     GDisplay       *gdisp);
static void   gimp_tool_real_cursor_update  (GimpTool       *tool,
					     GdkEventMotion *mevent,
					     GDisplay       *gdisp);
static void   gimp_tool_real_oper_update    (GimpTool       *tool,
					     GdkEventMotion *mevent,
					     GDisplay       *gdisp);


static guint gimp_tool_signals[LAST_SIGNAL] = { 0 };

static GimpObjectClass *parent_class = NULL;

static gint global_tool_ID = 0;


GtkType
gimp_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      GtkTypeInfo tool_info =
      {
        "GimpTool",
        sizeof (GimpTool),
        sizeof (GimpToolClass),
        (GtkClassInitFunc) gimp_tool_class_init,
        (GtkObjectInitFunc) gimp_tool_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      tool_type = gtk_type_unique (GIMP_TYPE_OBJECT, &tool_info);
    }

  return tool_type;
}

static void
gimp_tool_class_init (GimpToolClass *klass)
{
  GtkObjectClass *object_class;
  
  object_class = (GtkObjectClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_OBJECT);

  gimp_tool_signals[INITIALIZE] =
    gtk_signal_new ("initialize",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       initialize),
		    gtk_marshal_NONE__POINTER, 
		    GTK_TYPE_NONE, 1,
		    GTK_TYPE_POINTER); 

  gimp_tool_signals[CONTROL] =
    gtk_signal_new ("control",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       control),
		    gtk_marshal_NONE__INT_POINTER, 
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_INT,
		    GTK_TYPE_POINTER); 

  gimp_tool_signals[BUTTON_PRESS] =
    gtk_signal_new ("button_press",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       button_press),
		    gtk_marshal_NONE__POINTER_POINTER,
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER);

  gimp_tool_signals[BUTTON_RELEASE] =
    gtk_signal_new ("button_release",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       button_release),
		    gtk_marshal_NONE__POINTER_POINTER, 
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER); 

  gimp_tool_signals[MOTION] =
    gtk_signal_new ("motion",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       motion),
		    gtk_marshal_NONE__POINTER_POINTER, 
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER); 

  gimp_tool_signals[ARROW_KEY] =
    gtk_signal_new ("arrow_key",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       arrow_key),
		    gtk_marshal_NONE__POINTER_POINTER, 
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER); 

  gimp_tool_signals[MODIFIER_KEY] =
    gtk_signal_new ("modifier_key",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       modifier_key),
		    gtk_marshal_NONE__POINTER_POINTER, 
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER); 

  gimp_tool_signals[CURSOR_UPDATE] =
    gtk_signal_new ("cursor_update",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       cursor_update),
		    gtk_marshal_NONE__POINTER_POINTER, 
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER);

  gimp_tool_signals[OPER_UPDATE] =
    gtk_signal_new ("oper_update",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       oper_update),
		    gtk_marshal_NONE__POINTER_POINTER, 
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER); 

  gtk_object_class_add_signals (object_class, gimp_tool_signals, LAST_SIGNAL);  

  object_class->destroy = gimp_tool_destroy;

  klass->initialize     = gimp_tool_real_initialize;
  klass->control        = gimp_tool_real_control;
  klass->button_press   = gimp_tool_real_button_press;
  klass->button_release = gimp_tool_real_button_release;
  klass->motion         = gimp_tool_real_motion;
  klass->arrow_key      = gimp_tool_real_arrow_key;
  klass->modifier_key   = gimp_tool_real_modifier_key;
  klass->cursor_update  = gimp_tool_real_cursor_update;
  klass->oper_update    = gimp_tool_real_oper_update;
}

static void
gimp_tool_init (GimpTool *tool)
{
  tool->ID            = global_tool_ID++;

  tool->state         = INACTIVE;
  tool->paused_count  = 0;
  tool->scroll_lock   = FALSE;    /*  Allow scrolling  */
  tool->auto_snap_to  = TRUE;     /*  Snap to guides   */

  tool->preserve      = TRUE;     /*  Preserve tool across drawable changes  */
  tool->gdisp         = NULL;
  tool->drawable      = NULL;

  tool->tool_cursor   = GIMP_TOOL_CURSOR_NONE;
  tool->toggle_cursor = GIMP_TOOL_CURSOR_NONE;
  tool->toggled       = FALSE;
}

static void
gimp_tool_destroy (GtkObject *object)
{
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

void
gimp_tool_initialize (GimpTool   *tool, 
		      GDisplay   *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[INITIALIZE],
		   gdisp);
}

void
gimp_tool_control (GimpTool   *tool, 
		   ToolAction  action, 
		   GDisplay   *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[CONTROL],
		   action, gdisp);
}

void
gimp_tool_button_press (GimpTool       *tool,
			GdkEventButton *bevent,
			GDisplay       *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[BUTTON_PRESS],
		   bevent, gdisp); 
}

void
gimp_tool_button_release (GimpTool       *tool,
			  GdkEventButton *bevent,
			  GDisplay       *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[BUTTON_RELEASE],
		   bevent, gdisp);
}

void
gimp_tool_motion (GimpTool       *tool,
		  GdkEventMotion *mevent,
		  GDisplay       *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[MOTION],
		   mevent, gdisp);
}

void
gimp_tool_arrow_key (GimpTool    *tool,
		     GdkEventKey *kevent,
		     GDisplay    *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[ARROW_KEY],
		   kevent, gdisp);
}

void
gimp_tool_modifier_key (GimpTool    *tool,
			GdkEventKey *kevent,
			GDisplay    *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[MODIFIER_KEY],
		   kevent, gdisp);  
}

void
gimp_tool_cursor_update (GimpTool       *tool,
			 GdkEventMotion *mevent,
			 GDisplay       *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[CURSOR_UPDATE],
		   mevent, gdisp);
}

void
gimp_tool_oper_update (GimpTool       *tool,
		       GdkEventMotion *mevent,
		       GDisplay       *gdisp)
{
  g_return_if_fail (tool);
  g_return_if_fail (GIMP_IS_TOOL (tool));

  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[OPER_UPDATE],
		   mevent, gdisp);
}


const gchar *
gimp_tool_get_PDB_string (GimpTool *tool)
{
  GtkObject     *object;
  GimpToolClass *klass;

  g_return_val_if_fail (tool, NULL);
  g_return_val_if_fail (GIMP_IS_TOOL (tool), NULL);
  
  object = GTK_OBJECT (tool);

  klass = GIMP_TOOL_CLASS (object->klass);

  return klass->pdb_string;
}


/*  standard member functions  */

static void
gimp_tool_real_initialize (GimpTool       *tool,
			   GDisplay       *gdisp)
{
}

static void
gimp_tool_real_control (GimpTool   *tool,
			ToolAction  tool_action,
			GDisplay   *gdisp)
{
}

static void
gimp_tool_real_button_press (GimpTool       *tool,
			     GdkEventButton *bevent,
			     GDisplay       *gdisp)
{
  tool->gdisp    = gdisp;
  tool->drawable = gimp_image_active_drawable (gdisp->gimage);

  tool->state = ACTIVE;
}

static void
gimp_tool_real_button_release (GimpTool       *tool,
			       GdkEventButton *bevent,
			       GDisplay       *gdisp)
{
  tool->state = INACTIVE;
}

static void
gimp_tool_real_motion (GimpTool       *tool,
		       GdkEventMotion *mevent,
		       GDisplay       *gdisp)
{
}

static void
gimp_tool_real_arrow_key (GimpTool    *tool,
			  GdkEventKey *kevent,
			  GDisplay    *gdisp)
{
}

static void
gimp_tool_real_modifier_key (GimpTool    *tool,
			     GdkEventKey *kevent,
			     GDisplay    *gdisp)
{
}

static void
gimp_tool_real_cursor_update (GimpTool       *tool,
			      GdkEventMotion *mevent,
			      GDisplay       *gdisp)
{
  gdisplay_install_tool_cursor (gdisp,
                                GDK_TOP_LEFT_ARROW,
				tool->tool_cursor,
				GIMP_CURSOR_MODIFIER_NONE);
}

static void
gimp_tool_real_oper_update (GimpTool       *tool,
			    GdkEventMotion *mevent,
			    GDisplay       *gdisp)
{
}





/*  Function definitions  */

void
gimp_tool_help_func (const gchar *help_data)
{
  gimp_standard_help_func (tool_manager_active_get_help_data());
}


#define STUB(x) void * x (void){g_message ("stub function %s called",#x); return NULL;}

STUB(airbrush_non_gui)
STUB(airbrush_non_gui_default)
STUB(by_color_select_initialize_by_image)
STUB(by_color_select)
STUB(perspective_find_transform)
STUB(perspective_tool_perspective)
STUB(rotate_tool_rotate)
STUB(shear_tool_shear)
STUB(smudge_non_gui)
STUB(smudge_non_gui_default)
STUB(clone_non_gui)
STUB(clone_non_gui_default)
STUB(convolve_non_gui)
STUB(convolve_non_gui_default)

STUB(curves_free)
STUB(hue_saturation_free)
STUB(levels_free)
STUB(curves_calculate_curve)
STUB(curves_lut_func)
STUB(color_balance_create_lookup_tables)
STUB(color_balance)
STUB(hue_saturation_calculate_transfers)
STUB(hue_saturation)
STUB(threshold_2)
STUB(color_balance_dialog_hide)
STUB(hue_saturation_dialog_hide)
STUB(brightness_contrast_dialog_hide)
STUB(threshold_dialog_hide)
STUB(levels_dialog_hide)
STUB(curves_dialog_hide)
STUB(posterize_dialog_hide)
STUB(histogram_tool_histogram_range)

#ifdef __GNUC__
#warning obsolete crap
#endif
#ifdef STONE_AGE
ToolInfo tool_info[] =
{
  {
    NULL,
    N_("Transform"),
    N_("/Tools/Transform Tools/Transform"),
    "<shift>T",
    (char **) scale_bits,
    NULL,
    NULL,
    N_("Rotation, scaling, shearing, perspective."),
    "tools/transform.html",
    ROTATE,
    tools_new_transform_tool,
    tools_free_transform_tool,
    NULL,
    NULL,
    NULL,
    {
      rotate_small_bits, rotate_small_mask_bits,
      rotate_small_width, rotate_small_height,
      0, 0, NULL, NULL, NULL
    },
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    }
  }, /* rotate */

  {
    NULL,
    N_("Transform"),
    NULL,
    NULL,
    (char **) scale_bits,
    NULL,
    NULL,
    N_("Rotation, scaling, shearing, perspective."),
    "tools/transform.html",
    SCALE,
    tools_new_transform_tool,
    tools_free_transform_tool,
    NULL,
    NULL,
    NULL,
    {
      resize_small_bits, resize_small_mask_bits,
      resize_small_width, resize_small_height,
      0, 0, NULL, NULL, NULL
    },
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    }
  }, /* scale */

  {
    NULL,
    N_("Transform"),
    NULL,
    NULL,
    (char **) scale_bits,
    NULL,
    NULL,
    N_("Rotation, scaling, shearing, perspective."),
    "tools/transform.html",
    SHEAR,
    tools_new_transform_tool,
    tools_free_transform_tool,
    NULL,
    NULL,
    NULL,
    {
      shear_small_bits, shear_small_mask_bits,
      shear_small_width, shear_small_height,
      0, 0, NULL, NULL, NULL
    },
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    }
  }, /* shear */

  {
    NULL,
    N_("Transform"),
    NULL,
    NULL,
    (char **) scale_bits,
    NULL,
    NULL,
    N_("Rotation, scaling, shearing, perspective."),
    "tools/transform.html",
    PERSPECTIVE,
    tools_new_transform_tool,
    tools_free_transform_tool,
    NULL,
    NULL,
    NULL,
    {
      perspective_small_bits, perspective_small_mask_bits,
      perspective_small_width, perspective_small_height,
      0, 0, NULL, NULL, NULL
    },
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    }
  }, /* perspective */

  {
    NULL,
    N_("Airbrush"),
    N_("/Tools/Paint Tools/Airbrush"),
    "A",
    (char **) airbrush_bits,
    NULL,
    NULL,
    N_("Airbrush with variable pressure"),
    "tools/airbrush.html",
    AIRBRUSH,
    tools_new_airbrush,
    tools_free_airbrush,
    NULL,
    NULL,
    NULL,
    {
      airbrush_small_bits, airbrush_small_mask_bits,
      airbrush_small_width, airbrush_small_height,
      0, 0, NULL, NULL, NULL
    },
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    }
  },

  {
    NULL,
    N_("Clone"),
    N_("/Tools/Paint Tools/Clone"),
    "C",
    (char **) clone_bits,
    NULL,
    NULL,
    N_("Paint using patterns or image regions"),
    "tools/clone.html",
    CLONE,
    tools_new_clone,
    tools_free_clone,
    NULL,
    NULL,
    NULL,
    {
      clone_small_bits, clone_small_mask_bits,
      clone_small_width, clone_small_height,
      0, 0, NULL, NULL, NULL
    },
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    }
  },

  {
    NULL,
    N_("Smudge"),
    N_("/Tools/Paint Tools/Smudge"),
    "<shift>S",
    (char **) smudge_bits,
    NULL,
    NULL,
    N_("Smudge"),
    "tools/smudge.html",
    SMUDGE,
    tools_new_smudge,
    tools_free_smudge,
    NULL,
    NULL,
    NULL,
    {
      smudge_small_bits, smudge_small_mask_bits,
      smudge_small_width, smudge_small_height,
      0, 0, NULL, NULL, NULL
    },
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    }
  },

  {
    NULL,
    N_("Convolve"),
    N_("/Tools/Paint Tools/Convolve"),
    "V",
    (char **) blur_bits,
    NULL,
    NULL,
    N_("Blur or sharpen"),
    "tools/convolve.html",
    CONVOLVE,
    tools_new_convolve,
    tools_free_convolve,
    NULL,
    NULL,
    NULL,
    {
      blur_small_bits, blur_small_mask_bits,
      blur_small_width, blur_small_height,
      0, 0, NULL, NULL, NULL
    },
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    }
  },

/*
  {
    NULL,
    N_("Xinput Airbrush"),
    N_("/Tools/Paint Tools/XinputAirbrush"),
    "<shift>A",
    (char **) xinput_airbrush_bits,
    NULL,
    NULL,
    N_("Natural Airbrush"),
    "tools/xinput_airbrush.html",
    XINPUT_AIRBRUSH,
    tools_new_xinput_airbrush,
    tools_free_xinput_airbrush,
    NULL,
    NULL,
    NULL,
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    },
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    }
  },
*/

/*
  {
    NULL,
    N_("Path"),
    N_("/Tools/Path"),
    "",
    (char **) path_tool_bits,
    NULL,
    NULL,
    N_("Manipulate paths"),
    "tools/path.html",
    PATH_TOOL,
    tools_new_path_tool,
    tools_free_path_tool,
    NULL,
    NULL,
    NULL,
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    },
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    }
  },
*/

  /*  Non-toolbox tools  */
  {
    NULL,
    N_("By Color Select"),
    N_("/Select/By Color..."),
    NULL,
    (char **) by_color_select_bits,
    NULL,
    NULL,
    N_("Select regions by color"),
    "tools/by_color_select.html",
    BY_COLOR_SELECT,
    tools_new_by_color_select,
    tools_free_by_color_select,
    by_color_select_initialize,
    NULL,
    NULL,
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    },
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    }
  },

  {
    NULL,
    N_("Color Balance"),
    N_("/Image/Colors/Color Balance..."),
    NULL,
    (char **) adjustment_bits,
    NULL,
    NULL,
    N_("Adjust color balance"),
    "tools/color_balance.html",
    COLOR_BALANCE,
    tools_new_color_balance,
    tools_free_color_balance,
    color_balance_initialize,
    NULL,
    NULL,
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    },
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    }
  },

  {
    NULL,
    N_("Brightness-Contrast"),
    N_("/Image/Colors/Brightness-Contrast..."),
    NULL,
    (char **) adjustment_bits,
    NULL,
    NULL,
    N_("Adjust brightness and contrast"),
    "tools/brightness_contrast.html",
    BRIGHTNESS_CONTRAST,
    tools_new_brightness_contrast,
    tools_free_brightness_contrast,
    brightness_contrast_initialize,
    NULL,
    NULL,
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    },
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    }
  },

  {
    NULL,
    N_("Hue-Saturation"),
    N_("/Image/Colors/Hue-Saturation..."),
    NULL,
    (char **) adjustment_bits,
    NULL,
    NULL,
    N_("Adjust hue and saturation"),
    "tools/hue_saturation.html",
    HUE_SATURATION,
    tools_new_hue_saturation,
    tools_free_hue_saturation,
    hue_saturation_initialize,
    NULL,
    NULL,
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    },
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    }
  },

  {
    NULL,
    N_("Posterize"),
    N_("/Image/Colors/Posterize..."),
    NULL,
    (char **) adjustment_bits,
    NULL,
    NULL,
    N_("Reduce image to a fixed numer of colors"),
    "tools/posterize.html",
    POSTERIZE,
    tools_new_posterize,
    tools_free_posterize,
    posterize_initialize,
    NULL,
    NULL,
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    },
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    }
  },

  {
    NULL,
    N_("Threshold"),
    N_("/Image/Colors/Threshold..."),
    NULL,
    (char **) levels_bits,
    NULL,
    NULL,
    N_("Reduce image to two colors using a threshold"),
    "tools/threshold.html",
    THRESHOLD,
    tools_new_threshold,
    tools_free_threshold,
    threshold_initialize,
    NULL,
    NULL,
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    },
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    }
  },

  {
    NULL,
    N_("Curves"),
    N_("/Image/Colors/Curves..."),
    NULL,
    (char **) curves_bits,
    NULL,
    NULL,
    N_("Adjust color curves"),
    "tools/curves.html",
    CURVES,
    tools_new_curves,
    tools_free_curves,
    curves_initialize,
    NULL,
    NULL,
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    },
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    }
  },

  {
    NULL,
    N_("Levels"),
    N_("/Image/Colors/Levels..."),
    NULL,
    (char **) levels_bits,
    NULL,
    NULL,
    N_("Adjust color levels"),
    "tools/levels.html",
    LEVELS,
    tools_new_levels,
    tools_free_levels,
    levels_initialize,
    NULL,
    NULL,
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    },
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    }
  },

  {
    NULL,
    N_("Histogram"),
    N_("/Image/Histogram..."),
    NULL,
    (char **) histogram_bits,
    NULL,
    NULL,
    N_("View image histogram"),
    "tools/histogram.html",
    HISTOGRAM,
    tools_new_histogram_tool,
    tools_free_histogram_tool,
    histogram_tool_initialize ,
    NULL,
    NULL,
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    },
    {
      NULL, NULL,
      0, 0,
      0, 0, NULL, NULL, NULL
    }
  }
};

gint num_tools = sizeof (tool_info) / sizeof (tool_info[0]);
#endif
