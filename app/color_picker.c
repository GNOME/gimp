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
#include "appenv.h"
#include "actionarea.h"
#include "color_picker.h"
#include "drawable.h"
#include "gdisplay.h"
#include "info_dialog.h"
#include "palette.h"
#include "tools.h"

#include "libgimp/gimpintl.h"

/* maximum information buffer size */

#define MAX_INFO_BUF    8


/*  local function prototypes  */

static void  color_picker_button_press     (Tool *, GdkEventButton *, gpointer);
static void  color_picker_button_release   (Tool *, GdkEventButton *, gpointer);
static void  color_picker_motion           (Tool *, GdkEventMotion *, gpointer);
static void  color_picker_cursor_update    (Tool *, GdkEventMotion *, gpointer);
static void  color_picker_control          (Tool *, int, void *);
static void  color_picker_info_window_close_callback  (GtkWidget *, gpointer);

static int   get_color                     (GImage *, GimpDrawable *, int, int, int, int);
static void  color_picker_info_update      (Tool *, int);

static Argument *color_picker_invoker (Argument *);

/*  local variables  */

static int            col_value [5] = { 0, 0, 0, 0, 0 };
static GimpDrawable * active_drawable;
static int            update_type;
static int            sample_type;
static InfoDialog *   color_picker_info = NULL;
static char           red_buf   [MAX_INFO_BUF];
static char           green_buf [MAX_INFO_BUF];
static char           blue_buf  [MAX_INFO_BUF];
static char           alpha_buf [MAX_INFO_BUF];
static char           index_buf [MAX_INFO_BUF];
static char           gray_buf  [MAX_INFO_BUF];
static char           hex_buf   [MAX_INFO_BUF];

typedef struct _ColorPickerOptions ColorPickerOptions;
struct _ColorPickerOptions
{
  int sample_merged;
};

static ColorPickerOptions * color_picker_options = NULL;


static void
color_picker_toggle_update (GtkWidget *w,
			    gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static ColorPickerOptions *
create_color_picker_options (void)
{
  ColorPickerOptions *options;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *sample_merged_toggle;

  /*  the new options structure  */
  options = (ColorPickerOptions *) g_malloc (sizeof (ColorPickerOptions));
  options->sample_merged = 0;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);

  /*  the main label  */
  label = gtk_label_new (_("Color Picker Options"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  the sample merged toggle button  */
  sample_merged_toggle = gtk_check_button_new_with_label (_("Sample Merged"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sample_merged_toggle), options->sample_merged);
  gtk_box_pack_start (GTK_BOX (vbox), sample_merged_toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (sample_merged_toggle), "toggled",
		      (GtkSignalFunc) color_picker_toggle_update,
		      &options->sample_merged);
  gtk_widget_show (sample_merged_toggle);


  /*  Register this selection options widget with the main tools options dialog  */
  tools_register_options (COLOR_PICKER, vbox);

  return options;
}

static ActionAreaItem action_items[] =
{
  { N_("Close"), color_picker_info_window_close_callback, NULL, NULL },
};

static void
color_picker_button_press (Tool           *tool,
			   GdkEventButton *bevent,
			   gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;

  /*  If this is the first invocation of the tool, or a different gdisplay,
   *  create (or recreate) the info dialog...
   */
  if (tool->state == INACTIVE || gdisp_ptr != tool->gdisp_ptr ||
      active_drawable != gimage_active_drawable (gdisp->gimage))
    {
      /*  if the dialog exists, free it  */
      if (color_picker_info)
	info_dialog_free (color_picker_info);

      color_picker_info = info_dialog_new (_("Color Picker"));
      active_drawable = gimage_active_drawable (gdisp->gimage);

      /*  if the gdisplay is for a color image, the dialog must have RGB  */
      switch (drawable_type (active_drawable))
	{
	case RGB_GIMAGE: case RGBA_GIMAGE:
	  info_dialog_add_field (color_picker_info, _("Red"), red_buf, NULL, NULL);
	  info_dialog_add_field (color_picker_info, _("Green"), green_buf, NULL, NULL);
	  info_dialog_add_field (color_picker_info, _("Blue"), blue_buf, NULL, NULL);
	  info_dialog_add_field (color_picker_info, _("Alpha"), alpha_buf, NULL, NULL);
	  info_dialog_add_field (color_picker_info, _("Hex Triplet"), hex_buf, NULL, NULL);
	  break;

	case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
	  info_dialog_add_field (color_picker_info, _("Index"), index_buf, NULL, NULL);
	  info_dialog_add_field (color_picker_info, _("Alpha"), alpha_buf, NULL, NULL);
	  info_dialog_add_field (color_picker_info, _("Red"), red_buf, NULL, NULL);
	  info_dialog_add_field (color_picker_info, _("Green"), green_buf, NULL, NULL);
	  info_dialog_add_field (color_picker_info, _("Blue"), blue_buf, NULL, NULL);
	  info_dialog_add_field (color_picker_info, _("Hex Triplet"), hex_buf, NULL, NULL);
	  break;

	case GRAY_GIMAGE: case GRAYA_GIMAGE:
	  info_dialog_add_field (color_picker_info, _("Intensity"), gray_buf, NULL, NULL);
	  info_dialog_add_field (color_picker_info, _("Alpha"), alpha_buf, NULL, NULL);
	  info_dialog_add_field (color_picker_info, _("Hex Triplet"), hex_buf, NULL, NULL);
	  break;

	default :
	  break;
	}
  /* Create the action area  */
  action_items[0].user_data = color_picker_info;
  build_action_area (GTK_DIALOG (color_picker_info->shell), action_items, 1, 0);
    }

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    (GDK_POINTER_MOTION_HINT_MASK |
		     GDK_BUTTON1_MOTION_MASK |
		     GDK_BUTTON_RELEASE_MASK),
		    NULL, NULL, bevent->time);

  /*  Make the tool active and set the gdisplay which owns it  */
  tool->gdisp_ptr = gdisp_ptr;
  tool->state = ACTIVE;

  /*  First, transform the coordinates to gimp image space  */
  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y, FALSE, FALSE);

  /*  if the shift key is down, create a new color.
   *  otherwise, modify the current color.
   */
  if (bevent->state & GDK_SHIFT_MASK)
    {
      color_picker_info_update (tool, get_color (gdisp->gimage, active_drawable, x, y,
						 color_picker_options->sample_merged,
						 COLOR_NEW));
      update_type = COLOR_UPDATE_NEW;
    }
  else
    {
      color_picker_info_update (tool, get_color (gdisp->gimage, active_drawable, x, y,
						 color_picker_options->sample_merged,
						 COLOR_UPDATE));
      update_type = COLOR_UPDATE;
    }


}

static void
color_picker_button_release (Tool           *tool,
			     GdkEventButton *bevent,
			     gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  int x, y;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();
  gdisp = (GDisplay *) gdisp_ptr;

  /*  First, transform the coordinates to gimp image space  */
  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y, FALSE, FALSE);

  color_picker_info_update (tool, get_color (gdisp->gimage, active_drawable, x, y,
					     color_picker_options->sample_merged,
					     update_type));
}

static void
color_picker_motion (Tool           *tool,
		     GdkEventMotion *mevent,
		     gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;

  /*  First, transform the coordinates to gimp image space  */
  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, FALSE, FALSE);

  color_picker_info_update (tool, get_color (gdisp->gimage, active_drawable, x, y,
					     color_picker_options->sample_merged,
					     update_type));
}

static void
color_picker_cursor_update (Tool           *tool,
			    GdkEventMotion *mevent,
			    gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, FALSE, FALSE);
  if (gimage_pick_correlate_layer (gdisp->gimage, x, y))
    gdisplay_install_tool_cursor (gdisp, GDK_TCROSS);
  else
    gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
color_picker_control (Tool     *tool,
		      int       action,
		      gpointer  gdisp_ptr)
{
}

static int
get_color (GImage *gimage,
	   GimpDrawable *drawable,
	   int     x,
	   int     y,
	   int     sample_merged,
	   int     final)
{
  unsigned char *color;
  int offx, offy;
  int has_alpha;
  int is_indexed;
  if (!drawable && !sample_merged) 
    return FALSE;

  if (! sample_merged)
    {
      drawable_offsets (drawable, &offx, &offy);
      x -= offx;
      y -= offy;
      if (!(color = gimp_drawable_get_color_at(drawable, x, y)))
	return FALSE;
      sample_type = gimp_drawable_type(drawable);
      is_indexed = gimp_drawable_indexed (drawable);
    }
  else
    {
      if (!(color = gimp_image_get_color_at(gimage, x, y)))
	return FALSE;
      sample_type = gimp_image_composite_type(gimage);
      is_indexed = FALSE;
    }

  has_alpha = TYPE_HAS_ALPHA(sample_type);

  col_value[RED_PIX] = color[RED_PIX];
  col_value[GREEN_PIX] = color[GREEN_PIX];
  col_value[BLUE_PIX] = color[BLUE_PIX];
  if (has_alpha)
    {
      col_value [ALPHA_PIX] = color[3];
    }
  if (is_indexed)
    col_value [4] = color[4];
  palette_set_active_color (col_value [RED_PIX], col_value [GREEN_PIX],
			    col_value [BLUE_PIX], final);
  g_free(color);
  return TRUE;
}


static void
color_picker_info_update (Tool *tool,
			  int   valid)
{
  if (!valid)
    {
      g_snprintf (red_buf, MAX_INFO_BUF, _("N/A"));
      g_snprintf (green_buf, MAX_INFO_BUF, _("N/A"));
      g_snprintf (blue_buf, MAX_INFO_BUF, _("N/A"));
      g_snprintf (alpha_buf, MAX_INFO_BUF, _("N/A"));
      g_snprintf (index_buf, MAX_INFO_BUF, _("N/A"));
      g_snprintf (gray_buf, MAX_INFO_BUF, _("N/A"));
      g_snprintf (hex_buf, MAX_INFO_BUF, _("N/A"));
    }
  else
    {
      switch (sample_type)
	{
	case RGB_GIMAGE: case RGBA_GIMAGE:
	  g_snprintf (red_buf, MAX_INFO_BUF, "%d", col_value [RED_PIX]);
	  g_snprintf (green_buf, MAX_INFO_BUF, "%d", col_value [GREEN_PIX]);
	  g_snprintf (blue_buf, MAX_INFO_BUF, "%d", col_value [BLUE_PIX]);
	  if (sample_type == RGBA_GIMAGE)
	    g_snprintf (alpha_buf, MAX_INFO_BUF, "%d", col_value [ALPHA_PIX]);
	  else
	    g_snprintf (alpha_buf, MAX_INFO_BUF, _("N/A"));
	  g_snprintf (hex_buf, MAX_INFO_BUF, "#%.2x%.2x%.2x",
		      col_value [RED_PIX],
		      col_value [GREEN_PIX],
		      col_value [BLUE_PIX]);
	  break;

	case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
	  g_snprintf (index_buf, MAX_INFO_BUF, "%d", col_value [4]);
	  if (sample_type == INDEXEDA_GIMAGE)
	    g_snprintf (alpha_buf, MAX_INFO_BUF, "%d", col_value [ALPHA_PIX]);
	  else
	    g_snprintf (alpha_buf, MAX_INFO_BUF, _("N/A"));
	  g_snprintf (red_buf, MAX_INFO_BUF, "%d", col_value [RED_PIX]);
	  g_snprintf (green_buf, MAX_INFO_BUF, "%d", col_value [GREEN_PIX]);
	  g_snprintf (blue_buf, MAX_INFO_BUF, "%d", col_value [BLUE_PIX]);
	  g_snprintf (hex_buf, MAX_INFO_BUF, "#%.2x%.2x%.2x",
		      col_value [RED_PIX],
		      col_value [GREEN_PIX],
		      col_value [BLUE_PIX]);
	  break;

	case GRAY_GIMAGE: case GRAYA_GIMAGE:
	  g_snprintf (gray_buf, MAX_INFO_BUF, "%d", col_value [GRAY_PIX]);
	  if (sample_type == GRAYA_GIMAGE)
	    g_snprintf (alpha_buf, MAX_INFO_BUF, "%d", col_value [ALPHA_PIX]);
	  else
	    g_snprintf (alpha_buf, MAX_INFO_BUF, _("N/A"));
	  g_snprintf (hex_buf, MAX_INFO_BUF, "#%.2x%.2x%.2x",
		      col_value [GRAY_PIX],
		      col_value [GRAY_PIX],
		      col_value [GRAY_PIX]);
	  break;
	}
    }

  info_dialog_update (color_picker_info);
  info_dialog_popup (color_picker_info);
}

Tool *
tools_new_color_picker ()
{
  Tool * tool;

  if (! color_picker_options)
    color_picker_options = create_color_picker_options ();

  tool = (Tool *) g_malloc (sizeof (Tool));

  tool->type = COLOR_PICKER;
  tool->state = INACTIVE;
  tool->scroll_lock = 0;  /*  Allow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = NULL;
  tool->button_press_func = color_picker_button_press;
  tool->button_release_func = color_picker_button_release;
  tool->motion_func = color_picker_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = color_picker_cursor_update;
  tool->control_func = color_picker_control;
  tool->preserve = TRUE;

  return tool;
}

void
tools_free_color_picker (Tool *tool)
{
  if (color_picker_info)
    {
      info_dialog_free (color_picker_info);
      color_picker_info = NULL;
    }
}

/*  The color_picker procedure definition  */
ProcArg color_picker_args[] =
{
  { PDB_DRAWABLE,
    "drawable",
    "the drawable"
  },
  { PDB_FLOAT,
    "x",
    "x coordinate of upper-left corner of rectangle"
  },
  { PDB_FLOAT,
    "y",
    "y coordinate of upper-left corner of rectangle"
  },
  { PDB_INT32,
    "sample_merged",
    "use the composite image, not the drawable"
  },
  { PDB_INT32,
    "save_color",
    "save the color to the active palette"
  }
};

ProcArg color_picker_out_args[] =
{
  { PDB_COLOR,
    "color",
    "the return color"
  }
};

ProcRecord color_picker_proc =
{
  "gimp_color_picker",
  "Determine the color at the given drawable coordinates",
  "This tool determines the color at the specified coordinates.  The returned color is an RGB triplet even for grayscale and indexed drawables.  If the coordinates lie outside of the extents of the specified drawable, then an error is returned.  If the drawable has an alpha channel, the algorithm examines the alpha value of the drawable at the coordinates.  If the alpha value is completely transparent (0), then an error is returned.  If the sample_merged parameter is non-zero, the data of the composite image will be used instead of that for the specified drawable.  This is equivalent to sampling for colors after merging all visible layers.  In the case of a merged sampling, the supplied drawable is ignored.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  5,
  color_picker_args,

  /*  Output arguments  */
  1,
  color_picker_out_args,

  /*  Exec method  */
  { { color_picker_invoker } },
};


static Argument *
color_picker_invoker (Argument *args)
{
  GImage *gimage;
  int success = TRUE;
  GimpDrawable *drawable;
  double x, y;
  int sample_merged;
  int save_color;
  int int_value;
  Argument *return_args;
  unsigned char *color;

  drawable = NULL;
  x             = 0;
  y             = 0;
  sample_merged = FALSE;
  save_color    = COLOR_UPDATE;

  /*  the drawable  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      drawable = drawable_get_ID (int_value);
      if (drawable == NULL)                                        
        success = FALSE;
      else
        gimage = drawable_gimage (drawable);
    }
  /*  x, y  */
  if (success)
    {
      x = args[1].value.pdb_float;
      y = args[2].value.pdb_float;
    }
  /*  sample_merged  */
  if (success)
    {
      int_value = args[3].value.pdb_int;
      sample_merged = (int_value) ? TRUE : FALSE;
    }
  /*  save_color  */
  if (success)
    {
      int_value = args[4].value.pdb_int;
      save_color = (int_value) ? COLOR_NEW : COLOR_UPDATE;
    }

  /*  Make sure that if we're not using the composite, the specified drawable is valid  */
  if (success && !sample_merged)
    if (!drawable || (drawable_gimage (drawable)) != gimage)
      success = FALSE;

  /*  call the color_picker procedure  */
  if (success)
    success = get_color (gimage, drawable, (int) x, (int) y, sample_merged, save_color);

  return_args = procedural_db_return_args (&color_picker_proc, success);

  if (success)
    {
      color = (unsigned char *) g_malloc (3);
      color[RED_PIX] = col_value[RED_PIX];
      color[GREEN_PIX] = col_value[GREEN_PIX];
      color[BLUE_PIX] = col_value[BLUE_PIX];
      return_args[1].value.pdb_pointer = color;
    }

  return return_args;
}

static void
color_picker_info_window_close_callback (GtkWidget *w,
			    gpointer   client_data)
{
  info_dialog_popdown ((InfoDialog *) client_data);
}
