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
#include "appenv.h"
#include "color_panel.h"
#include "color_picker.h"
#include "draw_core.h"
#include "drawable.h"
#include "gdisplay.h"
#include "gimpui.h"
#include "cursorutil.h"
#include "info_dialog.h"
#include "palette.h"
#include "tools.h"
#include "gimprc.h"

#include "config.h"
#include "libgimp/gimpintl.h"

/*  maximum information buffer size  */
#define MAX_INFO_BUF 8

/*  the color picker structures  */

typedef struct _ColorPickerOptions ColorPickerOptions;

struct _ColorPickerOptions
{
  ToolOptions  tool_options;

  gboolean     sample_merged;
  gboolean     sample_merged_d;
  GtkWidget   *sample_merged_w;
  
  gboolean     sample_average;
  gboolean     sample_average_d;
  GtkWidget   *sample_average_w;
  
  gdouble      average_radius;
  gdouble      average_radius_d;
  GtkObject   *average_radius_w;

  gboolean     update_active;
  gboolean     update_active_d;
  GtkWidget   *update_active_w;
};

typedef struct _ColorPickerTool ColorPickerTool;

struct _ColorPickerTool
{
  DrawCore *core;       /*  Core select object  */

  gint      centerx;    /*  starting x coord    */
  gint      centery;    /*  starting y coord    */
};

/*  the color picker tool options  */
static ColorPickerOptions * color_picker_options = NULL;

/*  the color value  */
gint col_value[5] = { 0, 0, 0, 0, 0 };

/*  the color picker dialog  */
static gint           update_type;
static GimpImageType  sample_type;
static InfoDialog    *color_picker_info = NULL;
static ColorPanel    *color_panel = NULL;
static gchar          red_buf   [MAX_INFO_BUF];
static gchar          green_buf [MAX_INFO_BUF];
static gchar          blue_buf  [MAX_INFO_BUF];
static gchar          alpha_buf [MAX_INFO_BUF];
static gchar          index_buf [MAX_INFO_BUF];
static gchar          gray_buf  [MAX_INFO_BUF];
static gchar          hex_buf   [MAX_INFO_BUF];


/*  local function prototypes  */

static void  color_picker_button_press   (Tool *, GdkEventButton *, gpointer);
static void  color_picker_button_release (Tool *, GdkEventButton *, gpointer);
static void  color_picker_motion         (Tool *, GdkEventMotion *, gpointer);
static void  color_picker_cursor_update  (Tool *, GdkEventMotion *, gpointer);
static void  color_picker_control        (Tool *, ToolAction,       gpointer);

static void  color_picker_info_window_close_callback (GtkWidget *, gpointer);
static void  color_picker_info_update                (Tool *, gboolean);

static gboolean  pick_color_do (GimpImage    *gimage,
				GimpDrawable *drawable,
				gint          x,
				gint          y,
				gboolean      sample_merged,
				gboolean      sample_average,
				gdouble       average_radius,
				gboolean      update_active,
				gint          final);

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
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->update_active_w),
				options->update_active_d);
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
  options = g_new (ColorPickerOptions, 1);
  tool_options_init ((ToolOptions *) options,
		     _("Color Picker"),
		     color_picker_options_reset);
  options->sample_merged  = options->sample_merged_d  = FALSE;
  options->sample_average = options->sample_average_d = FALSE;
  options->average_radius = options->average_radius_d = 1.0;
  options->update_active  = options->update_active_d  = TRUE;

  /*  the main vbox  */
  vbox = options->tool_options.main_vbox;

  /*  the sample merged toggle button  */
  options->sample_merged_w =
    gtk_check_button_new_with_label (_("Sample Merged"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				options->sample_merged_d);
  gtk_box_pack_start (GTK_BOX (vbox), options->sample_merged_w, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (options->sample_merged_w), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
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
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
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
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &options->average_radius);
  gtk_widget_show (scale);
  gtk_widget_show (table);

  /*  the update active color toggle button  */
  options->update_active_w =
    gtk_check_button_new_with_label (_("Update Active Color"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->update_active_w),
				options->update_active_d);
  gtk_box_pack_start (GTK_BOX (vbox), options->update_active_w, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (options->update_active_w), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
		      &options->update_active);
  gtk_widget_show (options->update_active_w);

  return options;
}

static void
color_picker_button_press (Tool           *tool,
			   GdkEventButton *bevent,
			   gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  ColorPickerTool *cp_tool;
  gint x, y;

  gdisp = (GDisplay *) gdisp_ptr;
  cp_tool = (ColorPickerTool *) tool->private;

  /*  Make the tool active and set it's gdisplay & drawable  */
  tool->gdisp_ptr = gdisp;
  tool->drawable = gimage_active_drawable (gdisp->gimage);
  tool->state = ACTIVE;

  /*  create the info dialog if it doesn't exist  */
  if (! color_picker_info)
    {
      GtkWidget *hbox;

      color_picker_info = info_dialog_new (_("Color Picker"),
					   tools_help_func, NULL);

      /*  if the gdisplay is for a color image, the dialog must have RGB  */
      switch (drawable_type (tool->drawable))
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

      hbox = gtk_hbox_new (FALSE, 4);
      gtk_box_pack_start (GTK_BOX (color_picker_info->vbox), hbox,
			  FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      gtk_widget_reparent (color_picker_info->info_table, hbox);

      color_panel = color_panel_new (NULL, 48, 64);
      gtk_box_pack_start (GTK_BOX (hbox), color_panel->color_panel_widget,
			  FALSE, FALSE, 0);
      gtk_widget_show (color_panel->color_panel_widget);

      /*  create the action area  */
      gimp_dialog_create_action_area
	(GTK_DIALOG (color_picker_info->shell),

	 _("Close"), color_picker_info_window_close_callback,
	 color_picker_info, NULL, NULL, TRUE, FALSE,

	 NULL);
    }

  /*  Keep the coordinates of the target  */
  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y,
			       &cp_tool->centerx, &cp_tool->centery, FALSE, 1);

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    (GDK_POINTER_MOTION_HINT_MASK |
		     GDK_BUTTON1_MOTION_MASK |
		     GDK_BUTTON_RELEASE_MASK),
		    NULL, NULL, bevent->time);

  /*  First, transform the coordinates to gimp image space  */
  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y,
			       FALSE, FALSE);

  /*  if the shift key is down, create a new color.
   *  otherwise, modify the current color.
   */
  if (bevent->state & GDK_SHIFT_MASK)
    {
      color_picker_info_update
	(tool, pick_color_do (gdisp->gimage, tool->drawable, x, y,
			      color_picker_options->sample_merged,
			      color_picker_options->sample_average,
			      color_picker_options->average_radius,
			      color_picker_options->update_active,
			      COLOR_NEW));
      update_type = COLOR_UPDATE_NEW;
    }
  else
    {
      color_picker_info_update
	(tool, pick_color_do (gdisp->gimage, tool->drawable, x, y,
			      color_picker_options->sample_merged,
			      color_picker_options->sample_average,
			      color_picker_options->average_radius,
			      color_picker_options->update_active,
			      COLOR_UPDATE));
      update_type = COLOR_UPDATE;
    }

  /*  Start drawing the colorpicker tool  */
  draw_core_start (cp_tool->core, gdisp->canvas->window, tool);
}

static void
color_picker_button_release (Tool           *tool,
			     GdkEventButton *bevent,
			     gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  ColorPickerTool *cp_tool;
  gint x, y;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();
  gdisp = (GDisplay *) gdisp_ptr;
  cp_tool = (ColorPickerTool *) tool->private;

  /*  First, transform the coordinates to gimp image space  */
  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y,
			       FALSE, FALSE);

  color_picker_info_update
    (tool, pick_color_do (gdisp->gimage, tool->drawable, x, y,
			  color_picker_options->sample_merged,
			  color_picker_options->sample_average,
			  color_picker_options->average_radius,
			  color_picker_options->update_active,
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
  ColorPickerTool *cp_tool;
  gint x, y;

  gdisp = (GDisplay *) gdisp_ptr;
  cp_tool = (ColorPickerTool *) tool->private;

  /*  undraw the current tool  */
  draw_core_pause (cp_tool->core, tool);

  /*  First, transform the coordinates to gimp image space  */
  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y,
			       FALSE, FALSE);

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y,
			       &cp_tool->centerx, &cp_tool->centery,
			       FALSE, TRUE);

  color_picker_info_update
    (tool, pick_color_do (gdisp->gimage, tool->drawable, x, y,
			  color_picker_options->sample_merged,
			  color_picker_options->sample_average,
			  color_picker_options->average_radius,
			  color_picker_options->update_active,
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
  gint x, y;

  gdisp = (GDisplay *) gdisp_ptr;

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y,
			       FALSE, FALSE);

  if (gimage_pick_correlate_layer (gdisp->gimage, x, y))
    gdisplay_install_tool_cursor (gdisp, GIMP_COLOR_PICKER_CURSOR);
  else
    gdisplay_install_tool_cursor (gdisp, GDK_TOP_LEFT_ARROW);
}

static void
color_picker_control (Tool       *tool,
		      ToolAction  action,
		      gpointer    gdisp_ptr)
{
  ColorPickerTool * cp_tool;

  cp_tool = (ColorPickerTool *) tool->private;

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
      info_dialog_popdown (color_picker_info);
      break;

    default:
      break;
    }
}

typedef guchar * (*GetColorFunc) (GtkObject *, int, int);

static gboolean
pick_color_do (GimpImage    *gimage,
	       GimpDrawable *drawable,
	       gint          x,
	       gint          y,
	       gboolean      sample_merged,
	       gboolean      sample_average,
	       gdouble       average_radius,
	       gboolean      update_active,
	       gint          final)
{
  guchar *color;
  gint offx, offy;
  gint has_alpha;
  gint is_indexed;
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
      is_indexed = gimp_drawable_is_indexed (drawable);
      
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
      gint i, j;
      gint count = 0;
      gint color_avg[4] = { 0, 0, 0, 0 };
      guchar *tmp_color;
      gint radius = (gint) average_radius;

      for (i = x - radius; i <= x + radius; i++)
	for (j = y - radius; j <= y + radius; j++)
	  if ((tmp_color = (*get_color_func) (get_color_obj, i, j)))
	    {
	      count++;
	      
	      color_avg[RED_PIX]   += tmp_color[RED_PIX];
	      color_avg[GREEN_PIX] += tmp_color[GREEN_PIX];
	      color_avg[BLUE_PIX]  += tmp_color[BLUE_PIX];
	      if (has_alpha)
		color_avg[ALPHA_PIX] += tmp_color[3];
	      
	      g_free (tmp_color);
	    }
      
      color[RED_PIX]   = (guchar) (color_avg[RED_PIX] / count);
      color[GREEN_PIX] = (guchar) (color_avg[GREEN_PIX] / count);
      color[BLUE_PIX]  = (guchar) (color_avg[BLUE_PIX] / count);
      if (has_alpha)
	color[ALPHA_PIX] = (guchar) (color_avg[3] / count);
      
      is_indexed = FALSE;
    }

  col_value[RED_PIX]   = color[RED_PIX];
  col_value[GREEN_PIX] = color[GREEN_PIX];
  col_value[BLUE_PIX]  = color[BLUE_PIX];
  if (has_alpha)
    col_value[ALPHA_PIX] = color[3];
  if (is_indexed)
    col_value[4] = color[4];

  if (update_active)
    palette_set_active_color (col_value [RED_PIX],
			      col_value [GREEN_PIX],
			      col_value [BLUE_PIX],
			      final);

  g_free (color);
  return TRUE;
}

gboolean
pick_color (GimpImage    *gimage,
	    GimpDrawable *drawable,
	    gint          x,
	    gint          y,
	    gboolean      sample_merged,
	    gboolean      sample_average,
	    gdouble       average_radius,
	    gint          final)
{
  return pick_color_do (gimage, drawable,
			x, y,
			sample_merged,
			sample_average,
			average_radius,
			TRUE,
			final);
}

static void
colorpicker_draw (Tool *tool)
{
  GDisplay * gdisp;
  ColorPickerTool * cp_tool;
  gint tx, ty;
  gint radiusx, radiusy;
  gint cx, cy;

  if (! color_picker_options->sample_average)
    return;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  cp_tool = (ColorPickerTool *) tool->private;

  gdisplay_transform_coords (gdisp, cp_tool->centerx, cp_tool->centery,
			     &tx, &ty, TRUE);

  radiusx = SCALEX (gdisp, color_picker_options->average_radius);
  radiusy = SCALEY (gdisp, color_picker_options->average_radius);
  cx = SCALEX (gdisp, 1);
  cy = SCALEY (gdisp, 1);

  /*  Draw the circle around the collecting area */
  gdk_draw_rectangle (cp_tool->core->win, cp_tool->core->gc, 0,
		      tx - radiusx, 
		      ty - radiusy,
		      2 * radiusx + cx, 2 * radiusy + cy);

  if(radiusx > 1 && radiusy > 1)
    {
      gdk_draw_rectangle (cp_tool->core->win, cp_tool->core->gc, 0,
			  tx - radiusx + 2, 
			  ty - radiusy + 2,
			  2 * radiusx + cx - 4, 2 * radiusy + cy - 4);
    }
}

static void
color_picker_info_update (Tool     *tool,
			  gboolean  valid)
{
  if (!valid)
    {
      if (GTK_WIDGET_IS_SENSITIVE (color_panel->color_panel_widget))
	gtk_widget_set_sensitive (color_panel->color_panel_widget, FALSE);

      g_snprintf (red_buf,   MAX_INFO_BUF, _("N/A"));
      g_snprintf (green_buf, MAX_INFO_BUF, _("N/A"));
      g_snprintf (blue_buf,  MAX_INFO_BUF, _("N/A"));
      g_snprintf (alpha_buf, MAX_INFO_BUF, _("N/A"));
      g_snprintf (index_buf, MAX_INFO_BUF, _("N/A"));
      g_snprintf (gray_buf,  MAX_INFO_BUF, _("N/A"));
      g_snprintf (hex_buf,   MAX_INFO_BUF, _("N/A"));
    }
  else
    {
      guchar col[3];

      if (! GTK_WIDGET_IS_SENSITIVE (color_panel->color_panel_widget))
	gtk_widget_set_sensitive (color_panel->color_panel_widget, TRUE);

      switch (sample_type)
	{
	case RGB_GIMAGE: case RGBA_GIMAGE:
	  g_snprintf (red_buf,   MAX_INFO_BUF, "%d", col_value [RED_PIX]);
	  g_snprintf (green_buf, MAX_INFO_BUF, "%d", col_value [GREEN_PIX]);
	  g_snprintf (blue_buf,  MAX_INFO_BUF, "%d", col_value [BLUE_PIX]);
	  if (sample_type == RGBA_GIMAGE)
	    g_snprintf (alpha_buf, MAX_INFO_BUF, "%d", col_value [ALPHA_PIX]);
	  else
	    g_snprintf (alpha_buf, MAX_INFO_BUF, _("N/A"));
	  g_snprintf (hex_buf, MAX_INFO_BUF, "#%.2x%.2x%.2x",
		      col_value [RED_PIX],
		      col_value [GREEN_PIX],
		      col_value [BLUE_PIX]);
	  col[0] = col_value [RED_PIX];
	  col[1] = col_value [GREEN_PIX];
	  col[2] = col_value [BLUE_PIX];
	  break;

	case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
	  g_snprintf (index_buf, MAX_INFO_BUF, "%d", col_value [4]);
	  if (sample_type == INDEXEDA_GIMAGE)
	    g_snprintf (alpha_buf, MAX_INFO_BUF, "%d", col_value [ALPHA_PIX]);
	  else
	    g_snprintf (alpha_buf, MAX_INFO_BUF, _("N/A"));
	  g_snprintf (red_buf,   MAX_INFO_BUF, "%d", col_value [RED_PIX]);
	  g_snprintf (green_buf, MAX_INFO_BUF, "%d", col_value [GREEN_PIX]);
	  g_snprintf (blue_buf,  MAX_INFO_BUF, "%d", col_value [BLUE_PIX]);
	  g_snprintf (hex_buf,   MAX_INFO_BUF, "#%.2x%.2x%.2x",
		      col_value [RED_PIX],
		      col_value [GREEN_PIX],
		      col_value [BLUE_PIX]);
	  col[0] = col_value [RED_PIX];
	  col[1] = col_value [GREEN_PIX];
	  col[2] = col_value [BLUE_PIX];
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
	  col[0] = col_value [GRAY_PIX];
	  col[1] = col_value [GRAY_PIX];
	  col[2] = col_value [GRAY_PIX];
	  break;
	}

      color_panel_set_color (color_panel, col);
    }

  info_dialog_update (color_picker_info);
  info_dialog_popup (color_picker_info);
}

static void
color_picker_info_window_close_callback (GtkWidget *widget,
					 gpointer   client_data)
{
  info_dialog_popdown ((InfoDialog *) client_data);
}

Tool *
tools_new_color_picker ()
{
  Tool * tool;
  ColorPickerTool * private;

  /*  The tool options  */
  if (! color_picker_options)
    {
      color_picker_options = color_picker_options_new ();
      tools_register (COLOR_PICKER, (ToolOptions *) color_picker_options);
    }

  tool = tools_new_tool (COLOR_PICKER);
  private = g_new (ColorPickerTool, 1);

  private->core = draw_core_new (colorpicker_draw);

  tool->preserve = FALSE;  /*  Don't preserve on drawable change  */

  tool->private = (void *) private;

  tool->button_press_func   = color_picker_button_press;
  tool->button_release_func = color_picker_button_release;
  tool->motion_func         = color_picker_motion;
  tool->cursor_update_func  = color_picker_cursor_update;
  tool->control_func        = color_picker_control;

  return tool;
}

void
tools_free_color_picker (Tool *tool)
{
  ColorPickerTool * cp_tool;

  cp_tool = (ColorPickerTool *) tool->private;

  if (tool->state == ACTIVE)
    draw_core_stop (cp_tool->core, tool);

  draw_core_free (cp_tool->core);

  if (color_picker_info)
    {
      info_dialog_free (color_picker_info);
      color_picker_info = NULL;

      color_panel_free (color_panel);
      color_panel = NULL;
    }

  g_free (cp_tool);
}
