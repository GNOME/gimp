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
#include "draw_core.h"
#include "drawable.h"
#include "gdisplay.h"
#include "cursorutil.h"
#include "info_dialog.h"
#include "palette.h"
#include "tool_options_ui.h"
#include "tools.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"

/*  maximum information buffer size  */
#define MAX_INFO_BUF 8

/*  the color picker structures  */

typedef struct _ColorPickerOptions ColorPickerOptions;
struct _ColorPickerOptions
{
  ToolOptions  tool_options;

  int          sample_merged;
  int          sample_merged_d;
  GtkWidget   *sample_merged_w;
  
  int          sample_average;
  int          sample_average_d;
  GtkWidget   *sample_average_w;
  
  double       average_radius;
  double       average_radius_d;
  GtkObject   *average_radius_w;
};

typedef struct _ColourPickerTool ColourPickerTool;
struct _ColourPickerTool
{
  DrawCore *   core;       /*  Core select object          */

  int          centerx;    /*  starting x coord            */
  int          centery;    /*  starting y coord            */
};

/*  the color picker tool options  */
static ColorPickerOptions * color_picker_options = NULL;

/*  the color value  */
int            col_value[5] = { 0, 0, 0, 0, 0 };

/*  the color picker dialog  */
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


/*  local function prototypes  */
static void  color_picker_button_press     (Tool *, GdkEventButton *, gpointer);
static void  color_picker_button_release   (Tool *, GdkEventButton *, gpointer);
static void  color_picker_motion           (Tool *, GdkEventMotion *, gpointer);
static void  color_picker_cursor_update    (Tool *, GdkEventMotion *, gpointer);
static void  color_picker_control          (Tool *, int, void *);
static void  color_picker_info_window_close_callback  (GtkWidget *, gpointer);

static void  color_picker_info_update      (Tool *, int);


/*  functions  */

static void
color_picker_options_reset (void)
{
  ColorPickerOptions *options = color_picker_options;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				options->sample_merged_d);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_average_w),
				options->sample_average_d);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (options->average_radius_w),
			    options->average_radius_d);
}

static ColorPickerOptions *
color_picker_options_new (void)
{
  ColorPickerOptions *options;

  GtkWidget *vbox;
  GtkWidget *abox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *scale;

  /*  the new color picker tool options structure  */
  options = (ColorPickerOptions *) g_malloc (sizeof (ColorPickerOptions));
  tool_options_init ((ToolOptions *) options,
		     N_("Color Picker Options"),
		     color_picker_options_reset);
  options->sample_merged  = options->sample_merged_d  = FALSE;
  options->sample_average = options->sample_average_d = FALSE;
  options->average_radius = options->average_radius_d = 1.0;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  the sample merged toggle button  */
  options->sample_merged_w =
    gtk_check_button_new_with_label (_("Sample Merged"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				options->sample_merged_d);
  gtk_box_pack_start (GTK_BOX (vbox), options->sample_merged_w, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (options->sample_merged_w), "toggled",
		      (GtkSignalFunc) tool_options_toggle_update,
		      &options->sample_merged);
  gtk_widget_show (options->sample_merged_w);

  /*  the sample average options  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  options->sample_average_w =
    gtk_check_button_new_with_label (_("Sample Average"));
  gtk_table_attach (GTK_TABLE (table), options->sample_average_w, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_average_w),
				options->sample_average_d);
  gtk_signal_connect (GTK_OBJECT (options->sample_average_w), "toggled",
		      (GtkSignalFunc) tool_options_toggle_update,
		      &options->sample_average);
  gtk_widget_show (options->sample_average_w);

  label = gtk_label_new (_("Radius:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /*  the feather radius scale  */
  abox = gtk_alignment_new (0.5, 1.0, 1.0, 0.0);
  gtk_table_attach (GTK_TABLE (table), abox, 1, 2, 0, 2,
		    GTK_EXPAND | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (abox);

  options->average_radius_w =
    gtk_adjustment_new (options->average_radius_d, 1.0, 15.0, 2.0, 2.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->average_radius_w));
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_container_add (GTK_CONTAINER (abox), scale);
  gtk_widget_set_sensitive (scale, options->sample_average_d);
  gtk_object_set_data (GTK_OBJECT (options->sample_average_w), "set_sensitive",
		       scale);
  gtk_widget_set_sensitive (label, options->sample_average_d);
  gtk_object_set_data (GTK_OBJECT (scale), "set_sensitive",
		       label);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->average_radius_w), "value_changed",
		      (GtkSignalFunc) tool_options_double_adjustment_update,
		      &options->average_radius);
  gtk_widget_show (scale);
  gtk_widget_show (table);

  return options;
}

static void
color_picker_button_press (Tool           *tool,
			   GdkEventButton *bevent,
			   gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  ColourPickerTool *cp_tool;
  int x, y;

  static ActionAreaItem action_items[] =
  {
    { N_("Close"), color_picker_info_window_close_callback, NULL, NULL },
  };

  gdisp = (GDisplay *) gdisp_ptr;
  cp_tool = (ColourPickerTool *) tool->private;

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
	  info_dialog_add_label (color_picker_info, _("Red:"), red_buf);
	  info_dialog_add_label (color_picker_info, _("Green:"), green_buf);
	  info_dialog_add_label (color_picker_info, _("Blue:"), blue_buf);
	  info_dialog_add_label (color_picker_info, _("Alpha:"), alpha_buf);
	  info_dialog_add_label (color_picker_info, _("Hex Triplet:"), hex_buf);
	  break;

	case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
	  info_dialog_add_label (color_picker_info, _("Index:"), index_buf);
	  info_dialog_add_label (color_picker_info, _("Alpha:"), alpha_buf);
	  info_dialog_add_label (color_picker_info, _("Red:"), red_buf);
	  info_dialog_add_label (color_picker_info, _("Green:"), green_buf);
	  info_dialog_add_label (color_picker_info, _("Blue:"), blue_buf);
	  info_dialog_add_label (color_picker_info, _("Hex Triplet"), hex_buf);
	  break;

	case GRAY_GIMAGE: case GRAYA_GIMAGE:
	  info_dialog_add_label (color_picker_info, _("Intensity:"), gray_buf);
	  info_dialog_add_label (color_picker_info, _("Alpha:"), alpha_buf);
	  info_dialog_add_label (color_picker_info, _("Hex Triplet:"), hex_buf);
	  break;

	default :
	  break;
	}

      /*  create the action area  */
      action_items[0].user_data = color_picker_info;
      build_action_area (GTK_DIALOG (color_picker_info->shell),
			 action_items, 1, 0);
    }

  /*  Keep the coordinates of the target  */
  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y,
			       &cp_tool->centerx, &cp_tool->centery, FALSE, 1);

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
      color_picker_info_update (tool, pick_color (gdisp->gimage, active_drawable, x, y,
						  color_picker_options->sample_merged,
						  color_picker_options->sample_average,
						  color_picker_options->average_radius,
						  COLOR_NEW));
      update_type = COLOR_UPDATE_NEW;
    }
  else
    {
      color_picker_info_update (tool, pick_color (gdisp->gimage, active_drawable, x, y,
						  color_picker_options->sample_merged,
						  color_picker_options->sample_average,
						  color_picker_options->average_radius,
						  COLOR_UPDATE));
      update_type = COLOR_UPDATE;
    }

  /*  Start drawing the colourpicker tool  */
  draw_core_start (cp_tool->core, gdisp->canvas->window, tool);

}

static void
color_picker_button_release (Tool           *tool,
			     GdkEventButton *bevent,
			     gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  ColourPickerTool *cp_tool;
  int x, y;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();
  gdisp = (GDisplay *) gdisp_ptr;
  cp_tool = (ColourPickerTool *) tool->private;

  /*  First, transform the coordinates to gimp image space  */
  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y, FALSE, FALSE);

  color_picker_info_update (tool, pick_color (gdisp->gimage, active_drawable, x, y,
					      color_picker_options->sample_merged,
					      color_picker_options->sample_average,
					      color_picker_options->average_radius,
					      update_type));

  draw_core_stop (cp_tool->core, tool);
  tool->state = INACTIVE;
}

static void
color_picker_motion (Tool           *tool,
		     GdkEventMotion *mevent,
		     gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  ColourPickerTool *cp_tool;
  int x, y;

  gdisp = (GDisplay *) gdisp_ptr;
  cp_tool = (ColourPickerTool *) tool->private;

  /*  undraw the current tool  */
  draw_core_pause (cp_tool->core, tool);

  /*  First, transform the coordinates to gimp image space  */
  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, FALSE, FALSE);

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &cp_tool->centerx, &cp_tool->centery, FALSE, TRUE);

  color_picker_info_update (tool, pick_color (gdisp->gimage, active_drawable, x, y,
					      color_picker_options->sample_merged,
					      color_picker_options->sample_average,
					      color_picker_options->average_radius,
					      update_type));
  /*  redraw the current tool  */
  draw_core_resume (cp_tool->core, tool);
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
    gdisplay_install_tool_cursor (gdisp, GIMP_COLOR_PICKER_CURSOR);
  else
    gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
color_picker_control (Tool     *tool,
		      int       action,
		      gpointer  gdisp_ptr)
{
  ColourPickerTool * cp_tool;

  cp_tool = (ColourPickerTool *) tool->private;

  switch (action)
    {
    case PAUSE :
      draw_core_pause (cp_tool->core, tool);
      break;
    case RESUME :
      draw_core_resume (cp_tool->core, tool);
      break;
    case HALT :
      draw_core_stop (cp_tool->core, tool);
      break;
    }
}

typedef guchar * (*GetColorFunc) (GtkObject *, int, int);

int
pick_color (GimpImage *gimage,
	    GimpDrawable *drawable,
	    int      x,
	    int      y,
	    gboolean sample_merged,
	    gboolean sample_average,
	    double   average_radius,
	    int      final)
{
  guchar *color;
  int offx, offy;
  int has_alpha;
  int is_indexed;
  GetColorFunc get_color_func;
  GtkObject *get_color_obj;

  if (!drawable && !sample_merged) 
    return FALSE;

  if (!sample_merged)
    {
      drawable_offsets (drawable, &offx, &offy);
      x -= offx;
      y -= offy;
      
      sample_type = gimp_drawable_type (drawable);
      is_indexed = gimp_drawable_indexed (drawable);
      
      get_color_func = (GetColorFunc) gimp_drawable_get_color_at;
      get_color_obj = GTK_OBJECT (drawable);
    }
  else
    {
      sample_type = gimp_image_composite_type (gimage);
      is_indexed = FALSE;

      get_color_func = (GetColorFunc) gimp_image_get_color_at;
      get_color_obj = GTK_OBJECT (gimage);
    }

  has_alpha = TYPE_HAS_ALPHA (sample_type);

  if (!(color = (*get_color_func) (get_color_obj, x, y)))
    return FALSE;

  if (sample_average)
    {
      int i, j;
      int count = 0;
      int color_avg[4] = { 0, 0, 0, 0 };
      guchar *tmp_color;
      int radius = (int) average_radius;

      for (i = x - radius; i <= x + radius; i++)
	for (j = y - radius; j <= y + radius; j++)
	  if ((tmp_color = (*get_color_func) (get_color_obj, i, j)))
	    {
	      count++;

	      color_avg[RED_PIX] += tmp_color[RED_PIX];
	      color_avg[GREEN_PIX] += tmp_color[GREEN_PIX];
	      color_avg[BLUE_PIX] += tmp_color[BLUE_PIX];
	      if (has_alpha)
		color_avg[ALPHA_PIX] += tmp_color[3];

	      g_free(tmp_color);
	    }

      color[RED_PIX] = (guchar) (color_avg[RED_PIX] / count);
      color[GREEN_PIX] = (guchar) (color_avg[GREEN_PIX] / count);
      color[BLUE_PIX] = (guchar) (color_avg[BLUE_PIX] / count);
      if (has_alpha)
	color[ALPHA_PIX] = (guchar) (color_avg[3] / count);
      
      is_indexed = FALSE;
    }

  col_value[RED_PIX] = color[RED_PIX];
  col_value[GREEN_PIX] = color[GREEN_PIX];
  col_value[BLUE_PIX] = color[BLUE_PIX];
  if (has_alpha)
    col_value[ALPHA_PIX] = color[3];
  if (is_indexed)
    col_value[4] = color[4];

  palette_set_active_color (col_value [RED_PIX], col_value [GREEN_PIX],
			    col_value [BLUE_PIX], final);
  g_free(color);
  return TRUE;
}

static void
colourpicker_draw (Tool *tool)
{
  GDisplay * gdisp;
  ColourPickerTool * cp_tool;
  int tx, ty;
  int radiusx,radiusy;
  int cx,cy;

  if(!color_picker_options->sample_average)
    return;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  cp_tool = (ColourPickerTool *) tool->private;

  gdisplay_transform_coords (gdisp, cp_tool->centerx, cp_tool->centery,
			     &tx, &ty, 1);

  radiusx = SCALEX(gdisp,color_picker_options->average_radius);
  radiusy = SCALEY(gdisp,color_picker_options->average_radius);
  cx = SCALEX(gdisp,1);
  cy = SCALEY(gdisp,1);

  /*  Draw the circle around the collecting area */
  gdk_draw_rectangle(cp_tool->core->win, cp_tool->core->gc, 0,
		     tx - radiusx, 
		     ty - radiusy,
		     2*radiusx+cx, 2*radiusy+cy);

  if(radiusx > 1 && radiusy > 1)
    {
      gdk_draw_rectangle(cp_tool->core->win, cp_tool->core->gc, 0,
			 tx - radiusx+2, 
			 ty - radiusy+2,
			 2*radiusx+cx-4, 2*radiusy+cy-4);
    }
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
  ColourPickerTool * private;

  /*  The tool options  */
  if (! color_picker_options)
    {
      color_picker_options = color_picker_options_new ();
      tools_register (COLOR_PICKER, (ToolOptions *) color_picker_options);
    }

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (ColourPickerTool *) g_malloc(sizeof(ColourPickerTool));

  private->core = draw_core_new (colourpicker_draw);

  tool->type = COLOR_PICKER;
  tool->state = INACTIVE;
  tool->scroll_lock = 0;  /*  Allow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = private;
  tool->button_press_func = color_picker_button_press;
  tool->button_release_func = color_picker_button_release;
  tool->motion_func = color_picker_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;  tool->modifier_key_func = standard_modifier_key_func;
  tool->cursor_update_func = color_picker_cursor_update;
  tool->control_func = color_picker_control;
  tool->preserve = TRUE;

  return tool;
}

void
tools_free_color_picker (Tool *tool)
{
  ColourPickerTool * cp_tool;

  cp_tool = (ColourPickerTool *) tool->private;

  if (tool->state == ACTIVE)
    draw_core_stop (cp_tool->core, tool);

  draw_core_free (cp_tool->core);

  if (color_picker_info)
    {
      info_dialog_free (color_picker_info);
      color_picker_info = NULL;
    }

  g_free(cp_tool);
}

static void
color_picker_info_window_close_callback (GtkWidget *w,
			    gpointer   client_data)
{
  info_dialog_popdown ((InfoDialog *) client_data);
}
