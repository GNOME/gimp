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

#include "apptypes.h"

#include "appenv.h"
#include "context_manager.h"
#include "gdisplay.h"
#include "gimpcontext.h"
#include "gimpdnd.h"
#include "gimpimage.h"
#include "gimpui.h"
#include "session.h"
#include "tool.h"
#include "tool_options.h"
#include "tool_manager.h"
#include "dialog_handler.h"

#include "libgimp/gimpintl.h"

/*  Local  Data  */

enum {
	/* ??? SET_DISPLAY, ??? */
	BUTTON_PRESSED,
	BUTTON_RELEASED,
	MOTION,
	ARROW_KEYS,
	MODIFIER_KEY,
	CURSOR_UPDATED,
	OPER_UPDATE,
	TOOL_CONTROL,
	RESERVED1,
	RESERVED2,
	RESERVED3,
	LAST_SIGNAL
};

static guint gimp_tool_signals[LAST_SIGNAL] = { 0 };

static GimpObjectClass *parent_class = NULL;

static void gimp_tool_class_init (GimpToolClass *klass);
static void standard_control_func (GimpTool *,
					ToolAction,
					GDisplay *);

static gint global_tool_ID = 0;
#warning obsolete crap
#ifdef STONE_AGE
ToolInfo tool_info[] =
{
  {
    NULL,
    N_("Rect Select"),
    N_("/Tools/Select Tools/Rect Select"),
    "R",
    (char **) rect_bits,
    NULL,
    NULL,
    N_("Select rectangular regions"),
    "tools/rect_select.html",
    RECT_SELECT,
    tools_new_rect_select,
    tools_free_rect_select, 
    NULL,
    NULL,
    NULL,
    {
      rect_select_small_bits, rect_select_small_mask_bits,
      rect_select_small_width, rect_select_small_height,
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
    N_("Ellipse Select"),
    N_("/Tools/Select Tools/Ellipse Select"),
    "E",
    (char **) circ_bits,
    NULL,
    NULL,
    N_("Select elliptical regions"),
    "tools/ellipse_select.html",
    ELLIPSE_SELECT,
    tools_new_ellipse_select,
    tools_free_ellipse_select,
    NULL,
    NULL,
    NULL,
    {
      ellipse_select_small_bits, ellipse_select_small_mask_bits,
      ellipse_select_small_width, ellipse_select_small_height,
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
    N_("Free Select"), 
    N_("/Tools/Select Tools/Free Select"),
    "F",
    (char **) free_bits,
    NULL,
    NULL,
    N_("Select hand-drawn regions"),
    "tools/free_select.html",
    FREE_SELECT,
    tools_new_free_select,
    tools_free_free_select, 
    NULL,
    NULL,
    NULL,
    {
      free_select_small_bits, free_select_small_mask_bits,
      free_select_small_width, free_select_small_height,
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
    N_("Fuzzy Select"),
    N_("/Tools/Select Tools/Fuzzy Select"),
    "Z",
    (char **) fuzzy_bits,
    NULL,
    NULL,
    N_("Select contiguous regions"),
    "tools/fuzzy_select.html",
    FUZZY_SELECT,
    tools_new_fuzzy_select,
    tools_free_fuzzy_select, 
    NULL,
    NULL,
    NULL,
    {
      fuzzy_select_small_bits, fuzzy_select_small_mask_bits,
      fuzzy_select_small_width, fuzzy_select_small_height,
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
    N_("Bezier Select"),
    N_("/Tools/Select Tools/Bezier Select"),
    "B",
    (char **) bezier_bits,
    NULL,
    NULL,
    N_("Select regions using Bezier curves"),
    "tools/bezier_select.html",
    BEZIER_SELECT,
    tools_new_bezier_select,
    tools_free_bezier_select,
    NULL,
    NULL,
    NULL,
    {
      bezier_select_small_bits, bezier_select_small_mask_bits,
      bezier_select_small_width, bezier_select_small_height,
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
    N_("Intelligent Scissors"),
    N_("/Tools/Select Tools/Intelligent Scissors"),
    "I",
    (char **) iscissors_bits,
    NULL,
    NULL,
    N_("Select shapes from image"),
    "tools/intelligent_scissors.html",
    ISCISSORS,
    tools_new_iscissors,
    tools_free_iscissors, 
    NULL,
    NULL,
    NULL,
    {
      scissors_small_bits, scissors_small_mask_bits,
      scissors_small_width, scissors_small_height,
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
    N_("Move"),
    N_("/Tools/Transform Tools/Move"),
    "M",
    (char **) move_bits,
    NULL,
    NULL,
    N_("Move layers & selections"),
    "tools/move.html",
    MOVE,
    tools_new_move_tool,
    tools_free_move_tool, 
    NULL,
    NULL,
    NULL,
    {
      move_small_bits, move_small_mask_bits,
      move_small_width, move_small_height,
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
    N_("Magnify"),
    N_("/Tools/Transform Tools/Magnify"),
    "<shift>M",
    (char **) magnify_bits,
    NULL,
    NULL,
    N_("Zoom in & out"),
    "tools/magnify.html",
    MAGNIFY,
    tools_new_magnify,
    tools_free_magnify, 
    NULL,
    NULL,
    NULL,
    {
      zoom_small_bits, zoom_small_mask_bits,
      zoom_small_width, zoom_small_height,
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
    N_("Crop & Resize"),
    N_("/Tools/Transform Tools/Crop & Resize"),
    "<shift>C",
    (char **) crop_bits,
    NULL,
    NULL,
    N_("Crop or resize the image"),
    "tools/crop.html",
    CROP,
    tools_new_crop,
    tools_free_crop,
    NULL,
    NULL,
    NULL,
    {
      crop_small_bits, crop_small_mask_bits,
      crop_small_width, crop_small_height,
      0, 0, NULL, NULL, NULL
    },
    {
      resize_small_bits, resize_small_mask_bits,
      resize_small_width, resize_small_height,
      0, 0, NULL, NULL, NULL
    }
  },
  
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
    N_("Flip"),
    N_("/Tools/Transform Tools/Flip"),
    "<shift>F",
    (char **) flip_bits,
    NULL,
    NULL,
    N_("Flip the layer or selection"),
    "tools/flip.html",
    FLIP,
    tools_new_flip,
    tools_free_flip_tool,
    NULL,
    NULL,
    NULL,
    {
      flip_horizontal_small_bits, flip_horizontal_small_mask_bits,
      flip_horizontal_small_width, flip_horizontal_small_height,
      0, 0, NULL, NULL, NULL
    },
    {
      flip_vertical_small_bits, flip_vertical_small_mask_bits,
      flip_vertical_small_width, flip_vertical_small_height,
      0, 0, NULL, NULL, NULL
    }
  },
    
  {
    NULL,
    N_("Text"),
    N_("/Tools/Text"),
    "T",
    (char **) text_bits,
    NULL,
    NULL,
    N_("Add text to the image"),
    "tools/text.html",
    TEXT,
    tools_new_text,
    tools_free_text,
    NULL,
    NULL,
    NULL,
    {
      text_small_bits, text_small_mask_bits,
      text_small_width, text_small_height,
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
    N_("Bucket Fill"),
    N_("/Tools/Paint Tools/Bucket Fill"),
    "<shift>B",
    (char **) fill_bits,
    NULL,
    NULL,
    N_("Fill with a color or pattern"),
    "tools/bucket_fill.html",
    BUCKET_FILL,
    tools_new_bucket_fill,
    tools_free_bucket_fill,
    NULL,
    NULL,
    NULL,
    {
      bucket_fill_small_bits, bucket_fill_small_mask_bits,
      bucket_fill_small_width, bucket_fill_small_height,
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
    N_("Blend"),
    N_("/Tools/Paint Tools/Blend"),
    "L",
    (char **) gradient_bits,
    NULL,
    NULL,
    N_("Fill with a color gradient"),
    "tools/blend.html",
    BLEND,
    tools_new_blend,
    tools_free_blend,
    NULL,
    NULL,
    NULL,
    {
      blend_small_bits, blend_small_mask_bits,
      blend_small_width, blend_small_height,
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
    N_("Pencil"),
    N_("/Tools/Paint Tools/Pencil"),
    "<shift>P",
    (char **) pencil_bits,
    NULL,
    NULL,
    N_("Draw sharp pencil strokes"),
    "tools/pencil.html",
    PENCIL,
    tools_new_pencil,
    tools_free_pencil,
    NULL,
    NULL,
    NULL,
    {
      pencil_small_bits, pencil_small_mask_bits,
      pencil_small_width, pencil_small_height,
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
    N_("Paintbrush"),
    N_("/Tools/Paint Tools/Paintbrush"),
    "P",
    (char **) paint_bits,
    NULL,
    NULL,
    N_("Paint fuzzy brush strokes"),
    "tools/paintbrush.html",
    PAINTBRUSH,
    tools_new_paintbrush,
    tools_free_paintbrush, 
    NULL,
    NULL,
    NULL,
    {
      paintbrush_small_bits, paintbrush_small_mask_bits,
      paintbrush_small_width, paintbrush_small_height,
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
    N_("Ink"),
    N_("/Tools/Paint Tools/Ink"),
    "K",
    (char **) ink_bits,
    NULL,
    NULL,
    N_("Draw in ink"),
    "tools/ink.html",
    INK,
    tools_new_ink,
    tools_free_ink,
    NULL,
    NULL,
    NULL,
    {
      ink_small_bits, ink_small_mask_bits,
      ink_small_width, ink_small_height,
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
    N_("Eraser"),
    N_("/Tools/Paint Tools/Eraser"),
    "<shift>E",
    (char **) erase_bits,
    NULL,
    NULL,
    N_("Erase to background or transparency"),
    "tools/eraser.html",
    ERASER,
    tools_new_eraser,
    tools_free_eraser,
    NULL,
    NULL,
    NULL,
    {
      eraser_small_bits, eraser_small_mask_bits,
      eraser_small_width, eraser_small_height,
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

  {
    NULL,
    N_("Dodge or Burn"),
    N_("/Tools/Paint Tools/DodgeBurn"),
    "<shift>D",
    (char **) dodge_bits,
    NULL,
    NULL,
    N_("Dodge or Burn"),
    "tools/dodgeburn.html",
    DODGEBURN,
    tools_new_dodgeburn,
    tools_free_dodgeburn,
    NULL,
    NULL,
    NULL,
    {
      dodge_small_bits, dodge_small_mask_bits,
      dodge_small_width, dodge_small_height,
      0, 0, NULL, NULL, NULL
    },
    {
      burn_small_bits, burn_small_mask_bits,
      burn_small_width, burn_small_height,
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

  {
    NULL,
    N_("Measure"),
    N_("/Tools/Measure"),
    "",
     (char **) measure_bits,
    NULL,
    NULL,
    N_("Measure distances and angles"),
    "tools/measure.html",
    MEASURE,
    tools_new_measure_tool,
    tools_free_measure_tool, 
    NULL,
    NULL,
    NULL,
    {
      measure_small_bits, measure_small_mask_bits,
      measure_small_width, measure_small_height,
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
/*  dnd stuff  */
static GtkTargetEntry tool_target_table[] =
{
  GIMP_TARGET_TOOL
};
static guint n_tool_targets = (sizeof (tool_target_table) /
                               sizeof (tool_target_table[0]));


/*  Local function declarations  */
void      gimp_tool_show_options           (GimpTool *);


/*  Function definitions  */

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
        (GtkObjectInitFunc) gimp_tool_initialize,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      tool_type = gtk_type_unique (GIMP_TYPE_OBJECT, &tool_info);
    }

  return tool_type;
}



void
gimp_tool_old_initialize (GimpTool  *tool,
		  GDisplay *gdisp)
{
  /*  Tools which have an init function have dialogs and
   *  cannot be initialized without a display
   */
  /*if (tool_info[(gint) tool_type].init_func && !gdisp)
    tool_type = RECT_SELECT;*/

  /*  Force the emission of the "tool_changed" signal
   */
  /*if (active_tool->type == tool_type)
    {
      gimp_context_tool_changed (gimp_context_get_user ());
    }
  else*/
    {
      gimp_context_set_tool (gimp_context_get_user (), tool);
    }

  /*if (tool_info[(gint) tool_type].init_func)
    {
      (* tool_info[(gint) tool_type].init_func) (gdisp);*/

      active_tool->drawable = gimp_image_active_drawable (gdisp->gimage);
   /* } */
  /*  don't set tool->gdisp here! (see commands.c)  */
}


/*  standard member functions  */

static void
standard_button_press_func (GimpTool           *tool,
			    GdkEventButton *bevent,
			    GDisplay       *gdisp)
{
  tool->gdisp    = gdisp;
  tool->drawable = gimp_image_active_drawable (gdisp->gimage);
}

static void
standard_button_release_func (GimpTool           *tool,
			      GdkEventButton *bevent,
			      GDisplay       *gdisp)
{
}

static void
standard_motion_func (GimpTool           *tool,
		      GdkEventMotion *mevent,
		      GDisplay       *gdisp)
{
}

static void
standard_arrow_keys_func (GimpTool        *tool,
			  GdkEventKey *kevent,
			  GDisplay    *gdisp)
{
}

static void
standard_modifier_key_func (GimpTool        *tool,
			    GdkEventKey *kevent,
			    GDisplay    *gdisp)
{
}

static void
standard_cursor_update_func (GimpTool           *tool,
			     GdkEventMotion *mevent,
			     GDisplay       *gdisp)
{
  gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW,
				TOOL_TYPE_NONE,
				CURSOR_MODIFIER_NONE,
				FALSE);
}

static void
standard_operator_update_func (GimpTool       *tool,
			       GdkEventMotion *mevent,
			       GDisplay       *gdisp)
{
}


/*  Create a default tool object 
 */

GimpTool *
gimp_tool_new (void)
{
  GimpTool *tool;
  
  tool = gtk_type_new (GIMP_TYPE_TOOL);

  return tool;
}


/* set tool object defaults */
void gimp_tool_initialize (GimpTool *tool)
{
  tool->state        = INACTIVE;
  tool->paused_count = 0;
  tool->scroll_lock  = FALSE;     /*  Allow scrolling  */
  tool->auto_snap_to = TRUE;      /*  Snap to guides   */

  tool->preserve  = TRUE;         /*  Preserve tool across drawable changes  */
  tool->gdisp     = NULL;
  tool->drawable  = NULL;

  tool->toggled   = FALSE;

}

static void gimp_tool_class_init (GimpToolClass *klass)
{
  GtkObjectClass *object_class = (GtkObjectClass *) klass;
  
  parent_class = gtk_type_class (GIMP_TYPE_OBJECT);
  
  /* Welcome to signal registration syntax hell.
     Abandon hope, all ye developers that enter here. */ 
	   
  gimp_tool_signals[BUTTON_PRESSED]=
    gtk_signal_new ("button_pressed",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       button_press_func),
		    gtk_marshal_NONE__POINTER_POINTER, /* type that five times fast */
		    GTK_TYPE_NONE, /* now it's time for the exercise in redundancy exercise */
		    2, GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER);  /* now do I get a cookie? */

  gimp_tool_signals[BUTTON_RELEASED]=
    gtk_signal_new ("button_released",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       button_release_func),
		    gtk_marshal_NONE__POINTER_POINTER, 
		    GTK_TYPE_NONE, 
		    2, GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER); 


  gimp_tool_signals[MOTION]=
    gtk_signal_new ("motion",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       motion_func),
		    gtk_marshal_NONE__POINTER_POINTER, 
		    GTK_TYPE_NONE, 
		    2, GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER); 


  gimp_tool_signals[ARROW_KEYS]=
		
    gtk_signal_new ("arrow_keys",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       arrow_keys_func),
		    gtk_marshal_NONE__POINTER_POINTER, 
		    GTK_TYPE_NONE, 
		    2, GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER); 

  gimp_tool_signals[MODIFIER_KEY]=
    gtk_signal_new ("modifier_key",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       modifier_key_func),
		    gtk_marshal_NONE__POINTER_POINTER, 
		    GTK_TYPE_NONE, 
		    2, GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER); 

  gimp_tool_signals[CURSOR_UPDATED]=
    gtk_signal_new ("cursor_update",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       cursor_update_func),
		    gtk_marshal_NONE__POINTER_POINTER, 
		    GTK_TYPE_NONE, 
		    2, GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER); 



  gimp_tool_signals[OPER_UPDATE]=
    gtk_signal_new ("oper_update",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       oper_update_func),
		    gtk_marshal_NONE__POINTER_POINTER, 
		    GTK_TYPE_NONE, 
		    2, GTK_TYPE_POINTER,
		    GTK_TYPE_POINTER); 

  gimp_tool_signals[TOOL_CONTROL]=
    gtk_signal_new ("tool_control",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpToolClass,
				       control_func),
		    gtk_marshal_NONE__INT_POINTER, 
		    GTK_TYPE_NONE, 
		    2, GTK_TYPE_INT,
		    GTK_TYPE_POINTER); 

  gtk_object_class_add_signals (object_class, gimp_tool_signals, LAST_SIGNAL);  
	

  klass->button_press_func   = standard_button_press_func;
  klass->button_release_func = standard_button_release_func;
  klass->motion_func         = standard_motion_func;
  klass->arrow_keys_func     = standard_arrow_keys_func;
  klass->modifier_key_func   = standard_modifier_key_func;
  klass->cursor_update_func  = standard_cursor_update_func;
  klass->oper_update_func    = standard_operator_update_func;
  klass->control_func        = standard_control_func;
}


void
gimp_tool_help_func (const gchar *help_data)
{
  gimp_standard_help_func (tool_manager_active_get_help_data());
}

gchar *
gimp_tool_get_PDB_string (GimpTool *tool)
{
  GtkObject *object;
  GimpToolClass *klass;

  g_return_val_if_fail(tool, "gimp_core_nothing");
  
  object = GTK_OBJECT (tool);
  
  klass = GIMP_TOOL_CLASS (object->klass);
  
  return klass->pdb_string;
}

GdkPixmap *
gimp_tool_get_pixmap (GimpToolClass *type)
{
  g_return_val_if_fail(type, NULL);
  return (type->icon_pixmap);
}

GdkPixmap *
gimp_tool_get_mask (GimpToolClass *type)
{
  g_return_val_if_fail(type, NULL);
  return (type->icon_mask);
}

gchar *
gimp_tool_get_help_data (GimpTool *tool)
{
  GtkObject *object;
  GimpToolClass *klass;

  g_return_val_if_fail(tool, NULL);

  object = GTK_OBJECT (tool);
  
  klass = GIMP_TOOL_CLASS (object->klass);
	
  return klass->help_data;
}


void gimp_tool_control(GimpTool *tool, 
		       ToolAction action, 
		       GDisplay *gdisp)
{
  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[TOOL_CONTROL], action, gdisp);
}


static void
standard_control_func (GimpTool       *tool,
		       ToolAction  action,
		       GDisplay   *gdisp)
{
  if (tool)
    {
      if (tool->gdisp == gdisp)
        {
          switch (action)
            {
            case PAUSE :
              if (tool->state == ACTIVE)
                {
                  if (! tool->paused_count)
                    {
                      tool->state = PAUSED;
                    }
                }
              tool->paused_count++;
              break;

            case RESUME :
              tool->paused_count--;
              if (tool->state == PAUSED)
                {
                  if (! tool->paused_count)
                    {
                      tool->state = ACTIVE;
                    }
                }
              break;

            case HALT :
              tool->state = INACTIVE;
              break;

            case DESTROY :
              gtk_object_unref (GTK_OBJECT(tool));
              tool_options_hide_shell();
              break;

            default:
              break;
            }
        }
      else if (action == HALT)
        {
          tool->state = INACTIVE;
        }
    }
}

void gimp_tool_emit_button_press (GimpTool       *tool,
				  GdkEventButton *bevent,
				  GDisplay       *gdisp)
{
  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[BUTTON_PRESSED], bevent, gdisp); 

}

void gimp_tool_emit_button_release  (GimpTool       *tool,
				     GdkEventButton *bevent,
				     GDisplay       *gdisp)
{
  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[BUTTON_RELEASED], bevent, gdisp);
}  

void gimp_tool_emit_motion    (GimpTool       *tool,
			       GdkEventMotion *mevent,
			       GDisplay       *gdisp)
{
  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[MOTION], mevent, gdisp);
}

void gimp_tool_emit_arrow_keys (GimpTool       *tool,
				GdkEventKey    *kevent,
				GDisplay       *gdisp)
{
  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[ARROW_KEYS], kevent, gdisp);

}
void gimp_tool_emit_modifier_key (GimpTool       *tool,
				  GdkEventKey    *kevent,
				  GDisplay       *gdisp)
{
  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[MODIFIER_KEY], kevent, gdisp);  
}
void    gimp_tool_emit_cursor_update (GimpTool       *tool,
				      GdkEventMotion *mevent,
				      GDisplay       *gdisp)
{
  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[CURSOR_UPDATED], mevent, gdisp);
}

void  gimp_tool_emit_oper_update (GimpTool       *tool,
				  GdkEventMotion *mevent,
				  GDisplay       *gdisp)
{
  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[OPER_UPDATE], mevent, gdisp);
}

void gimp_tool_emit_control (GimpTool       *tool,
			     ToolAction     action,
			     GDisplay       *gdisp)
{
  gtk_signal_emit (GTK_OBJECT (tool), gimp_tool_signals[TOOL_CONTROL], action, gdisp);
}

#define STUB(x) void * x (void){g_message ("stub function %s called",#x); return NULL;}

#define QUIET_STUB(x) void * x (void){return NULL;}

STUB(curves_free)
STUB(hue_saturation_free)
STUB(levels_free)
STUB(tool_options_dialog_free)
STUB(paint_mode_menu_new)
STUB(curves_calculate_curve)
STUB(curves_lut_func)
STUB(color_balance_create_lookup_tables)
STUB(color_balance)
STUB(hue_saturation_calculate_transfers)
STUB(hue_saturation)
STUB(threshold_2)
STUB(tool_options_dialog_show)
STUB(paint_options_set_global)
STUB(color_balance_dialog_hide)
STUB(hue_saturation_dialog_hide)
STUB(brightness_contrast_dialog_hide)
STUB(threshold_dialog_hide)
STUB(levels_dialog_hide)
STUB(curves_dialog_hide)
STUB(posterize_dialog_hide)
STUB(move_tool_start_hguide)
STUB(move_tool_start_vguide)
STUB(bucket_fill_region)
STUB(pathpoints_copy)
STUB(pathpoints_free)
STUB(bezier_stroke)
STUB(bezier_distance_along)
STUB(bezier_select_free)
STUB(paths_dialog_destroy_cb)
STUB(bezier_select_reset)
STUB(bezier_add_point)
STUB(path_set_path)
STUB(path_set_path_points)
STUB(path_delete_path)
STUB(text_render)
STUB(text_get_extents)
STUB(tool_options_hide_shell)
STUB(gimp_tool_hide_options)
STUB(tool_options_show)
STUB(by_color_select_initialize_by_image)
STUB(path_transform_start_undo)
STUB(path_transform_do_undo)
STUB(path_transform_free_undo)
STUB(undo_pop_paint)
STUB(histogram_tool_histogram_range)
STUB(paths_dialog_create)
STUB(paths_dialog_flush)
STUB(paths_dialog_update)
STUB(paths_dialog_new_path_callback)
STUB(paths_dialog_dup_path_callback)
STUB(paths_dialog_path_to_sel_callback)
STUB(paths_dialog_sel_to_path_callback)
STUB(paths_dialog_stroke_path_callback)
STUB(paths_dialog_delete_path_callback)
STUB(paths_dialog_copy_path_callback)
STUB(paths_dialog_paste_path_callback)
STUB(paths_dialog_import_path_callback)
STUB(paths_dialog_export_path_callback)
STUB(paths_dialog_edit_path_attributes_callback)
STUB(tool_options_dialog_new)
STUB(tools_register)
QUIET_STUB(GIMP_IS_FUZZY_SELECT)
QUIET_STUB(GIMP_IS_MOVE_TOOL)
STUB(crop_image)
STUB(dodgeburn_non_gui)
STUB(dodgeburn_non_gui_default)
STUB(ellipse_select)
STUB(eraser_non_gui)
STUB(eraser_non_gui_default)
STUB(transform_core_cut)
STUB(flip_tool_flip)
STUB(transform_core_paste)
STUB(free_select)
STUB(find_contiguous_region)
STUB(fuzzy_mask)
STUB(fuzzy_select)
STUB(paintbrush_non_gui)
STUB(paintbrush_non_gui_default)
STUB(pencil_non_gui)
STUB(perspective_find_transform)
STUB(perspective_tool_perspective)
STUB(rect_select)
STUB(rotate_tool_rotate)
STUB(scale_tool_scale)
STUB(shear_tool_shear)
STUB(smudge_non_gui)
STUB(smudge_non_gui_default)
STUB(transform_core_do)
STUB(airbrush_non_gui)
STUB(airbrush_non_gui_default)
STUB(blend)
STUB(bucket_fill)
STUB(by_color_select)
STUB(clone_non_gui)
STUB(clone_non_gui_default)
STUB(convolve_non_gui)
STUB(convolve_non_gui_default)
