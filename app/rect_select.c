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
#include "gdisplay.h"
#include "gimage_mask.h"
#include "edit_selection.h"
#include "floating_sel.h"
#include "rect_select.h"
#include "rect_selectP.h"

#include "config.h"
#include "libgimp/gimpintl.h"
#include "libgimp/gimpunitmenu.h"

#define STATUSBAR_SIZE 128

extern SelectionOptions *ellipse_options;
static SelectionOptions *rect_options = NULL;

static void rect_select (GImage *, int, int, int, int, int, int, double);
extern void ellipse_select (GImage *, int, int, int, int, int, int, int, double);

static Argument *rect_select_invoker (Argument *);

/*  Selection options dialog--for all selection tools  */

static void
selection_toggle_update (GtkWidget *w,
			 gpointer   data)
{
  GtkWidget *set_sensitive;
  int       *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;

  set_sensitive =
    (GtkWidget*) gtk_object_get_data (GTK_OBJECT (w), "set_sensitive");

  if (set_sensitive)
    {
      gtk_widget_set_sensitive (set_sensitive, *toggle_val);

      if (GTK_IS_SCALE (set_sensitive))
	{
	  set_sensitive =
	    gtk_object_get_data (GTK_OBJECT (set_sensitive), "scale_label");
	  if (set_sensitive)
	    gtk_widget_set_sensitive (set_sensitive, *toggle_val);
	}
    }
}

static void
selection_adjustment_update (GtkWidget *widget,
			     gpointer   data)
{
  int *val;

  val = (int *) data;
  *val = GTK_ADJUSTMENT (widget)->value;
}

static void
selection_unitmenu_update (GtkWidget *widget,
			   gpointer   data)
{
  GUnit         *val;
  GtkSpinButton *spinbutton;
  int            digits;

  val = (GUnit *) data;
  *val = gimp_unit_menu_get_unit (GIMP_UNIT_MENU (widget));

  digits = ((*val == UNIT_PIXEL) ? 0 :
	    ((*val == UNIT_PERCENT) ? 2 :
	     (MIN (6, MAX (3, gimp_unit_get_digits (*val))))));

  spinbutton = GTK_SPIN_BUTTON (gtk_object_get_data (GTK_OBJECT (widget),
						     "fixed_width_spinbutton"));
  gtk_spin_button_set_digits (spinbutton, digits);
  spinbutton = GTK_SPIN_BUTTON (gtk_object_get_data (GTK_OBJECT (widget),
						     "fixed_height_spinbutton"));
  gtk_spin_button_set_digits (spinbutton, digits);
}

static void
selection_scale_update (GtkAdjustment *adjustment,
			double        *scale_val)
{
  *scale_val = adjustment->value;
}

SelectionOptions *
create_selection_options (ToolType             tool_type,
			  ToolOptionsResetFunc reset_func)
{
  SelectionOptions *options;
  GtkWidget        *vbox;
  GtkWidget        *abox;
  GtkWidget        *table;
  GtkWidget        *label;
  GtkWidget        *scale;

  /*  the new options structure  */
  options = (SelectionOptions *) g_malloc (sizeof (SelectionOptions));
  options->antialias      = options->antialias_d      = TRUE;
  options->feather        = options->feather_d        = FALSE;
  options->feather_radius = options->feather_radius_d = 10.0;
  options->sample_merged  = options->sample_merged_d  = FALSE;
  options->fixed_size     = options->fixed_size_d     = FALSE;
  options->fixed_height   = options->fixed_height_d   = 1;
  options->fixed_width    = options->fixed_width_d    = 1;
  options->fixed_unit     = options->fixed_unit_d     = UNIT_PIXEL;
  options->sample_merged  = options->sample_merged_d  = TRUE;

  options->antialias_w      = NULL;
  options->feather_w        = NULL;
  options->feather_radius_w = NULL;
  options->sample_merged_w  = NULL;
  options->fixed_size_w     = NULL;
  options->fixed_height_w   = NULL;
  options->fixed_width_w    = NULL;
  options->fixed_unit_w     = NULL;
  options->sample_merged_w  = NULL;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 2);

  /*  the sample merged option  */
  switch (tool_type)
    {
    case RECT_SELECT:
    case ELLIPSE_SELECT:
    case FREE_SELECT:
    case BEZIER_SELECT:
      break;
    case FUZZY_SELECT:
    case ISCISSORS:
    case BY_COLOR_SELECT:
      options->sample_merged_w =
	gtk_check_button_new_with_label (_("Sample Merged"));
      gtk_box_pack_start (GTK_BOX (vbox), options->sample_merged_w,
			  FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (options->sample_merged_w), "toggled",
			  (GtkSignalFunc) selection_toggle_update,
			  &options->sample_merged);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				    options->sample_merged_d);
      gtk_widget_show (options->sample_merged_w);
      break;
    default:
      break;
    }

  /*  the antialias toggle button  */
  if (tool_type != RECT_SELECT)
    {
      options->antialias_w = gtk_check_button_new_with_label (_("Antialiasing"));
      gtk_box_pack_start (GTK_BOX (vbox), options->antialias_w, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (options->antialias_w), "toggled",
			  (GtkSignalFunc) selection_toggle_update,
			  &options->antialias);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->antialias_w),
				    options->antialias_d);
      gtk_widget_show (options->antialias_w);
    }

  /*  the feather options  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  options->feather_w = gtk_check_button_new_with_label (_("Feather"));
  gtk_table_attach (GTK_TABLE (table), options->feather_w, 0, 1, 0, 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->feather_w),
				options->feather_d);
  gtk_signal_connect (GTK_OBJECT (options->feather_w), "toggled",
		      (GtkSignalFunc) selection_toggle_update,
		      &options->feather);
  gtk_widget_show (options->feather_w);

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

  options->feather_radius_w =
    gtk_adjustment_new (options->feather_radius_d, 0.0, 100.0, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (options->feather_radius_w));
  gtk_container_add (GTK_CONTAINER (abox), scale);
  gtk_widget_set_sensitive (scale, options->feather_d);
  gtk_object_set_data (GTK_OBJECT (options->feather_w), "set_sensitive",
		       scale);
  gtk_widget_set_sensitive (label, options->feather_d);
  gtk_object_set_data (GTK_OBJECT (scale), "scale_label",
		       label);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (options->feather_radius_w), "value_changed",
		      (GtkSignalFunc) selection_scale_update,
		      &options->feather_radius);
  gtk_widget_show (scale);
  gtk_widget_show (table);

  /* Widgets for fixed size select */
  if (tool_type == RECT_SELECT || tool_type == ELLIPSE_SELECT) 
    {
      GtkWidget *alignment;
      GtkWidget *table;
      GtkWidget *width_spinbutton;
      GtkWidget *height_spinbutton;

      options->fixed_size_w =
	gtk_check_button_new_with_label (_("Fixed size / aspect ratio"));
      gtk_box_pack_start (GTK_BOX (vbox), options->fixed_size_w,
			  FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (options->fixed_size_w), "toggled",
			  (GtkSignalFunc)selection_toggle_update,
			  &options->fixed_size);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options->fixed_size_w),
				   options->fixed_size_d);
      gtk_widget_show(options->fixed_size_w);

      alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, FALSE, 0);
      gtk_widget_show (alignment);

      table = gtk_table_new (3, 2, FALSE);
      gtk_table_set_col_spacing (GTK_TABLE (table), 0, 6);
      gtk_table_set_row_spacings (GTK_TABLE (table), 1);
      gtk_container_add (GTK_CONTAINER (alignment), table);

      gtk_widget_set_sensitive (table, options->fixed_size_d);
      gtk_object_set_data (GTK_OBJECT (options->fixed_size_w), "set_sensitive",
			   table);

      label = gtk_label_new (_("Width:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label,
			0, 1, 0, 1,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      options->fixed_width_w =
	gtk_adjustment_new (options->fixed_width_d, 1e-5, 32767.0,
			    1.0, 50.0, 0.0);
      width_spinbutton =
	gtk_spin_button_new (GTK_ADJUSTMENT (options->fixed_width_w), 1.0, 0.0);
      gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(width_spinbutton),
				       GTK_SHADOW_NONE);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (width_spinbutton), TRUE);
      gtk_widget_set_usize (width_spinbutton, 75, 0);
      gtk_signal_connect (GTK_OBJECT (options->fixed_width_w), "value_changed",
                          (GtkSignalFunc) selection_adjustment_update,
                          &options->fixed_width);
      gtk_table_attach (GTK_TABLE (table), width_spinbutton,
			1, 2, 0, 1,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);
      gtk_widget_show (width_spinbutton);
      
      label = gtk_label_new (_("Height:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label,
			0, 1, 1, 2,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      options->fixed_height_w =
	gtk_adjustment_new (options->fixed_height_d, 1e-5, 32767.0,
			    1.0, 50.0, 0.0);
      height_spinbutton =
	gtk_spin_button_new (GTK_ADJUSTMENT (options->fixed_height_w), 1.0, 0.0);
      gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(height_spinbutton),
				       GTK_SHADOW_NONE);
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(height_spinbutton), TRUE);
      gtk_widget_set_usize (height_spinbutton, 75, 0);
      gtk_signal_connect (GTK_OBJECT (options->fixed_height_w), "value_changed",
                          (GtkSignalFunc) selection_adjustment_update,
                          &options->fixed_height);
      gtk_table_attach (GTK_TABLE (table), height_spinbutton,
			1, 2, 1, 2,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);
      gtk_widget_show (height_spinbutton);

      label = gtk_label_new (_("Unit:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label,
			0, 1, 2, 3,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      options->fixed_unit_w =
	gimp_unit_menu_new ("%a", options->fixed_unit_d, TRUE, TRUE, TRUE);
      gtk_signal_connect (GTK_OBJECT (options->fixed_unit_w), "unit_changed",
                          (GtkSignalFunc) selection_unitmenu_update,
                          &options->fixed_unit);
      gtk_object_set_data (GTK_OBJECT (options->fixed_unit_w),
			   "fixed_width_spinbutton", width_spinbutton);
      gtk_object_set_data (GTK_OBJECT (options->fixed_unit_w),
			   "fixed_height_spinbutton", height_spinbutton);
      gtk_table_attach (GTK_TABLE (table), options->fixed_unit_w,
			1, 2, 2, 3,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);
      gtk_widget_show (options->fixed_unit_w);
      
      gtk_widget_show (table);
    }

  /*  Register this selection options widget with the main tools options dialog
   */
  tools_register (tool_type,
		  vbox,
		  ((tool_type == RECT_SELECT) ?
		   _("Rectangular Select Options") :
		   ((tool_type == ELLIPSE_SELECT) ?
		    _("Elliptical Selection Options") :
		    ((tool_type == FREE_SELECT) ?
		     _("Free-hand Selection Options") :
		     ((tool_type == FUZZY_SELECT) ?
		      _("Fuzzy Selection Options") :
		      ((tool_type == BEZIER_SELECT) ?
		       _("Bezier Selection Options") :
		       ((tool_type == ISCISSORS) ?
			_("Intelligent Scissors Options") :
			((tool_type == BY_COLOR_SELECT) ?
			 _("By-Color Select Options") :
			 _("Unknown Selection Type ???")))))))),
		  reset_func);

  return options;
}

void
reset_selection_options (SelectionOptions *options)
{
  if (options->sample_merged_w)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->sample_merged_w),
				  options->sample_merged_d);
  if (options->antialias_w)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->antialias_w),
				  options->antialias_d);
  if (options->feather_w)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->feather_w),
				    options->feather_d);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (options->feather_radius_w),
				options->feather_radius_d);
    }
  if (options->fixed_size_w)
    {
      GtkSpinButton *spinbutton;
      int            digits;

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(options->fixed_size_w),
				    options->fixed_size_d);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (options->fixed_width_w),
				options->fixed_width_d);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (options->fixed_height_w),
				options->fixed_height_d);

      options->fixed_unit = options->fixed_unit_d;
      gimp_unit_menu_set_unit (GIMP_UNIT_MENU (options->fixed_unit_w),
			       options->fixed_unit_d);

      digits =
	((options->fixed_unit_d == UNIT_PIXEL) ? 0 :
	 ((options->fixed_unit_d == UNIT_PERCENT) ? 2 :
	  (MIN (6, MAX (3, gimp_unit_get_digits (options->fixed_unit_d))))));

      spinbutton =
	GTK_SPIN_BUTTON (gtk_object_get_data (GTK_OBJECT (options->fixed_unit_w),
					      "fixed_width_spinbutton"));
      gtk_spin_button_set_digits (spinbutton, digits);
      spinbutton =
	GTK_SPIN_BUTTON (gtk_object_get_data (GTK_OBJECT (options->fixed_unit_w),
					      "fixed_height_spinbutton"));
      gtk_spin_button_set_digits (spinbutton, digits);
    }
}

/*************************************/
/*  Rectangular selection apparatus  */

static void
rect_select (GImage *gimage,
	     int     x,
	     int     y,
	     int     w,
	     int     h,
	     int     op,
	     int     feather,
	     double  feather_radius)
{
  Channel * new_mask;

  /*  if applicable, replace the current selection  */
  if (op == REPLACE)
    gimage_mask_clear (gimage);
  else
    gimage_mask_undo (gimage);

  /*  if feathering for rect, make a new mask with the
   *  rectangle and feather that with the old mask
   */
  if (feather)
    {
      new_mask = channel_new_mask (gimage, gimage->width, gimage->height);
      channel_combine_rect (new_mask, ADD, x, y, w, h);
      channel_feather (new_mask, gimage_get_mask (gimage),
		       feather_radius, op, 0, 0);
      channel_delete (new_mask);
    }
  else if (op == INTERSECT)
    {
      new_mask = channel_new_mask (gimage, gimage->width, gimage->height);
      channel_combine_rect (new_mask, ADD, x, y, w, h);
      channel_combine_mask (gimage_get_mask (gimage), new_mask, op, 0, 0);
      channel_delete (new_mask);
    }
  else
    channel_combine_rect (gimage_get_mask (gimage), op, x, y, w, h);
}

void
rect_select_button_press (Tool           *tool,
			  GdkEventButton *bevent,
			  gpointer        gdisp_ptr)
{
  GDisplay * gdisp;
  RectSelect * rect_sel;
  gchar select_mode[STATUSBAR_SIZE];
  int x, y;
  GUnit unit = UNIT_PIXEL;
  float unit_factor;

  gdisp = (GDisplay *) gdisp_ptr;
  rect_sel = (RectSelect *) tool->private;

  gdisplay_untransform_coords (gdisp, bevent->x, bevent->y, &x, &y, TRUE, 0);

  rect_sel->x = x;
  rect_sel->y = y;
  switch (tool->type)
    {
    case RECT_SELECT:
      rect_sel->fixed_size = rect_options->fixed_size;
      rect_sel->fixed_width = rect_options->fixed_width;
      rect_sel->fixed_height = rect_options->fixed_height;
      unit = rect_options->fixed_unit;
      break;
    case ELLIPSE_SELECT:
      rect_sel->fixed_size = ellipse_options->fixed_size;
      rect_sel->fixed_width = ellipse_options->fixed_width;
      rect_sel->fixed_height = ellipse_options->fixed_height;
      unit = ellipse_options->fixed_unit;
      break;
    default:
      break;
    }

  switch (unit)
    {
    case UNIT_PIXEL:
      break;
    case UNIT_PERCENT:
      rect_sel->fixed_width =
	gdisp->gimage->width * rect_sel->fixed_width / 100;
      rect_sel->fixed_height =
	gdisp->gimage->height * rect_sel->fixed_height / 100;
      break;
    default:
      unit_factor = gimp_unit_get_factor (unit);
      rect_sel->fixed_width =
	 rect_sel->fixed_width * gdisp->gimage->xresolution / unit_factor;
      rect_sel->fixed_height =
	rect_sel->fixed_height * gdisp->gimage->yresolution / unit_factor;
      break;
    }

  rect_sel->fixed_width = MAX (1, rect_sel->fixed_width);
  rect_sel->fixed_height = MAX (1, rect_sel->fixed_height);

  rect_sel->w = 0;
  rect_sel->h = 0;

  rect_sel->center = FALSE;

  gdk_pointer_grab (gdisp->canvas->window, FALSE,
		    GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, bevent->time);

  tool->state = ACTIVE;
  tool->gdisp_ptr = gdisp_ptr;

  if (bevent->state & GDK_MOD1_MASK)
    {
      init_edit_selection (tool, gdisp_ptr, bevent, MaskTranslate);
      return;
    }
  else if ((bevent->state & GDK_SHIFT_MASK) && !(bevent->state & GDK_CONTROL_MASK))
    rect_sel->op = ADD;
  else if ((bevent->state & GDK_CONTROL_MASK) && !(bevent->state & GDK_SHIFT_MASK))
    rect_sel->op = SUB;
  else if ((bevent->state & GDK_CONTROL_MASK) && (bevent->state & GDK_SHIFT_MASK))
    rect_sel->op = INTERSECT;
  else
    {
      if (! (layer_is_floating_sel (gimage_get_active_layer (gdisp->gimage))) &&
	  gdisplay_mask_value (gdisp, bevent->x, bevent->y) > HALF_WAY)
	{
	  init_edit_selection (tool, gdisp_ptr, bevent, MaskToLayerTranslate);
	  return;
	}
      rect_sel->op = REPLACE;
    }

  /* initialize the statusbar display */
  rect_sel->context_id = gtk_statusbar_get_context_id (GTK_STATUSBAR (gdisp->statusbar), "selection");
  switch (rect_sel->op)
    {
    case ADD:
      g_snprintf (select_mode, STATUSBAR_SIZE, _("Selection: ADD"));
      break;
    case SUB:
      g_snprintf (select_mode, STATUSBAR_SIZE, _("Selection: SUBTRACT"));
      break;
    case INTERSECT:
      g_snprintf (select_mode, STATUSBAR_SIZE, _("Selection: INTERSECT"));
      break;
    case REPLACE:
      g_snprintf (select_mode, STATUSBAR_SIZE, _("Selection: REPLACE"));
      break;
    default:
      break;
    }
  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar), rect_sel->context_id, select_mode);

  draw_core_start (rect_sel->core, gdisp->canvas->window, tool);
}

void
rect_select_button_release (Tool           *tool,
			    GdkEventButton *bevent,
			    gpointer        gdisp_ptr)
{
  RectSelect * rect_sel;
  GDisplay * gdisp;
  int x1, y1, x2, y2, w, h;

  gdisp = (GDisplay *) gdisp_ptr;
  rect_sel = (RectSelect *) tool->private;

  gdk_pointer_ungrab (bevent->time);
  gdk_flush ();

  gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), rect_sel->context_id);

  draw_core_stop (rect_sel->core, tool);
  tool->state = INACTIVE;

  /*  First take care of the case where the user "cancels" the action  */
  if (! (bevent->state & GDK_BUTTON3_MASK))
    {
      x1 = (rect_sel->w < 0) ? rect_sel->x + rect_sel->w : rect_sel->x;
      y1 = (rect_sel->h < 0) ? rect_sel->y + rect_sel->h : rect_sel->y;
      w = (rect_sel->w < 0) ? -rect_sel->w : rect_sel->w;
      h = (rect_sel->h < 0) ? -rect_sel->h : rect_sel->h;

     if ((!w || !h) && !rect_sel->fixed_size)
	{
	  /*  If there is a floating selection, anchor it  */
	  if (gimage_floating_sel (gdisp->gimage))
	    floating_sel_anchor (gimage_floating_sel (gdisp->gimage));
	  /*  Otherwise, clear the selection mask  */
	  else
	    gimage_mask_clear (gdisp->gimage);
	  
	  gdisplays_flush ();
	  return;
	}
      
      x2 = x1 + w;
      y2 = y1 + h;

      switch (tool->type)
	{
	case RECT_SELECT:
	  rect_select (gdisp->gimage,
		       x1, y1, (x2 - x1), (y2 - y1),
		       rect_sel->op,
		       rect_options->feather,
		       rect_options->feather_radius);
	  break;
	  
	case ELLIPSE_SELECT:
	  ellipse_select (gdisp->gimage,
			  x1, y1, (x2 - x1), (y2 - y1),
			  rect_sel->op,
			  ellipse_options->antialias,
			  ellipse_options->feather,
			  ellipse_options->feather_radius);
	  break;
	default:
	  break;
	}
	  
      /*  show selection on all views  */
      gdisplays_flush ();
   }
}

void
rect_select_motion (Tool           *tool,
		    GdkEventMotion *mevent,
		    gpointer        gdisp_ptr)
{
  RectSelect * rect_sel;
  GDisplay * gdisp;
  gchar size[STATUSBAR_SIZE];
  int ox, oy;
  int x, y;
  int w, h, s;
  int tw, th;
  double ratio;

  if (tool->state != ACTIVE)
    return;

  gdisp = (GDisplay *) gdisp_ptr;
  rect_sel = (RectSelect *) tool->private;

  draw_core_pause (rect_sel->core, tool);

  /* Calculate starting point */

  if (rect_sel->center)
    {
      ox = rect_sel->x + rect_sel->w / 2;
      oy = rect_sel->y + rect_sel->h / 2;
    }
  else
    {
      ox = rect_sel->x;
      oy = rect_sel->y;
    }

  gdisplay_untransform_coords (gdisp, mevent->x, mevent->y, &x, &y, TRUE, 0);
  if (rect_sel->fixed_size) {
    if (mevent->state & GDK_SHIFT_MASK) {
      ratio = (double)(rect_sel->fixed_height /
		       (double)rect_sel->fixed_width);
      tw = x - ox;
      th = y - oy;
       /* 
        * This is probably an inefficient way to do it, but it gives
        * nicer, more predictable results than the original agorithm
        */
 
       if ((abs(th) < (ratio * abs(tw))) && (abs(tw) > (abs(th) / ratio)))
         {
           w = tw;
           h = (int)(tw * ratio);
           /* h should have the sign of th */
           if ((th < 0 && h > 0) || (th > 0 && h < 0))
             h = -h;
         } 
       else 
         {
           h = th;
           w = (int)(th / ratio);
           /* w should have the sign of tw */
           if ((tw < 0 && w > 0) || (tw > 0 && w < 0))
             w = -w;
         }
     } else {
       w = (x - ox > 0 ? rect_sel->fixed_width  : -rect_sel->fixed_width);
       h = (y - oy > 0 ? rect_sel->fixed_height : -rect_sel->fixed_height);
     }
  } else {
    w = (x - ox);
    h = (y - oy);
  }

  /*  If the shift key is down, then make the rectangle square (or ellipse circular) */
  if ((mevent->state & GDK_SHIFT_MASK) && !rect_sel->fixed_size)
    {
      s = MAXIMUM(abs(w), abs(h));
	  
      if (w < 0)
		w = -s;
      else
		w = s;

      if (h < 0)
		h = -s;
      else
		h = s;
    }

  /*  If the control key is down, create the selection from the center out */
  if (mevent->state & GDK_CONTROL_MASK)
    {
      if (rect_sel->fixed_size) 
	{
          if (mevent->state & GDK_SHIFT_MASK) 
            {
              rect_sel->x = ox - w;
              rect_sel->y = oy - h;
              rect_sel->w = w * 2;
              rect_sel->h = h * 2;
            }
          else
            {
              rect_sel->x = ox - w / 2;
              rect_sel->y = oy - h / 2;
              rect_sel->w = w;
              rect_sel->h = h;
            }
	} 
      else 
	{
	  w = abs(w);
	  h = abs(h);
	  
	  rect_sel->x = ox - w;
	  rect_sel->y = oy - h;
	  rect_sel->w = 2 * w + 1;
	  rect_sel->h = 2 * h + 1;
	}
      rect_sel->center = TRUE;
    }
  else
    {
      rect_sel->x = ox;
      rect_sel->y = oy;
      rect_sel->w = w;
      rect_sel->h = h;

      rect_sel->center = FALSE;
    }

  gtk_statusbar_pop (GTK_STATUSBAR (gdisp->statusbar), rect_sel->context_id);
  if (gdisp->dot_for_dot)
    {
      g_snprintf (size, STATUSBAR_SIZE, gdisp->cursor_format_str,
		  _("Selection: "), abs(rect_sel->w), " x ", abs(rect_sel->h));
    }
  else /* show real world units */
    {
      float unit_factor = gimp_unit_get_factor (gdisp->gimage->unit);

      g_snprintf (size, STATUSBAR_SIZE, gdisp->cursor_format_str,
		  _("Selection: "),
		  (float)abs(rect_sel->w) * unit_factor /
		  gdisp->gimage->xresolution,
		  " x ",
		  (float)abs(rect_sel->h) * unit_factor /
		  gdisp->gimage->yresolution);
    }
  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar), rect_sel->context_id,
		      size);

  draw_core_resume (rect_sel->core, tool);
}


void
rect_select_draw (Tool *tool)
{
  GDisplay * gdisp;
  RectSelect * rect_sel;
  int x1, y1, x2, y2;

  gdisp = (GDisplay *) tool->gdisp_ptr;
  rect_sel = (RectSelect *) tool->private;

  x1 = MINIMUM (rect_sel->x, rect_sel->x + rect_sel->w);
  y1 = MINIMUM (rect_sel->y, rect_sel->y + rect_sel->h);
  x2 = MAXIMUM (rect_sel->x, rect_sel->x + rect_sel->w);
  y2 = MAXIMUM (rect_sel->y, rect_sel->y + rect_sel->h);

  gdisplay_transform_coords (gdisp, x1, y1, &x1, &y1, 0);
  gdisplay_transform_coords (gdisp, x2, y2, &x2, &y2, 0);

  gdk_draw_rectangle (rect_sel->core->win,
		      rect_sel->core->gc, 0,
		      x1, y1, (x2 - x1), (y2 - y1));
}


void
rect_select_cursor_update (Tool           *tool,
			   GdkEventMotion *mevent,
			   gpointer        gdisp_ptr)
{
  GDisplay *gdisp;
  int active;

  gdisp = (GDisplay *) gdisp_ptr;
  active = (active_tool->state == ACTIVE);

  /*  if alt key is depressed, use the diamond cursor  */
  if (mevent->state & GDK_MOD1_MASK && !active)
    gdisplay_install_tool_cursor (gdisp, GDK_DIAMOND_CROSS);
  /*  if the cursor is over the selected region, but no modifiers
   *  are depressed, use a fleur cursor--for cutting and moving the selection
   */
  else if (gdisplay_mask_value (gdisp, mevent->x, mevent->y) &&
	   ! (layer_is_floating_sel (gimage_get_active_layer (gdisp->gimage))) &&
	   ! (mevent->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) &&
	   ! active)
    gdisplay_install_tool_cursor (gdisp, GDK_FLEUR);
  else
    gdisplay_install_tool_cursor (gdisp, GDK_TCROSS);
}


void
rect_select_control (Tool     *tool,
		     int       action,
		     gpointer  gdisp_ptr)
{
  RectSelect * rect_sel;

  rect_sel = (RectSelect *) tool->private;

  switch (action)
    {
    case PAUSE :
      draw_core_pause (rect_sel->core, tool);
      break;
    case RESUME :
      draw_core_resume (rect_sel->core, tool);
      break;
    case HALT :
      draw_core_stop (rect_sel->core, tool);
      break;
    }
}

static void
rect_select_reset_options ()
{
  reset_selection_options (rect_options);
}

Tool *
tools_new_rect_select ()
{
  Tool * tool;
  RectSelect * private;

  /*  The tool options  */
  if (!rect_options)
    rect_options = create_selection_options (RECT_SELECT,
					     rect_select_reset_options);

  tool = (Tool *) g_malloc (sizeof (Tool));
  private = (RectSelect *) g_malloc (sizeof (RectSelect));

  private->core = draw_core_new (rect_select_draw);
  private->x = private->y = 0;
  private->w = private->h = 0;

  tool->type = RECT_SELECT;
  tool->state = INACTIVE;
  tool->scroll_lock = 0;  /*  Allow scrolling  */
  tool->auto_snap_to = TRUE;
  tool->private = (void *) private;
  tool->button_press_func = rect_select_button_press;
  tool->button_release_func = rect_select_button_release;
  tool->motion_func = rect_select_motion;
  tool->arrow_keys_func = standard_arrow_keys_func;
  tool->cursor_update_func = rect_select_cursor_update;
  tool->control_func = rect_select_control;
  tool->preserve = TRUE;

  return tool;
}

void
tools_free_rect_select (Tool *tool)
{
  RectSelect * rect_sel;

  rect_sel = (RectSelect *) tool->private;

  draw_core_free (rect_sel->core);
  g_free (rect_sel);
}

/*  The rect_select procedure definition  */
ProcArg rect_select_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image"
  },
  { PDB_FLOAT,
    "x",
    "x coordinate of upper-left corner of rectangle"
  },
  { PDB_FLOAT,
    "y",
    "y coordinate of upper-left corner of rectangle"
  },
  { PDB_FLOAT,
    "width",
    "the width of the rectangle: width > 0"
  },
  { PDB_FLOAT,
    "height",
    "the height of the rectangle: height > 0"
  },
  { PDB_INT32,
    "operation",
    "the selection operation: { ADD (0), SUB (1), REPLACE (2), INTERSECT (3) }"
  },
  { PDB_INT32,
    "feather",
    "feather option for selections"
  },
  { PDB_FLOAT,
    "feather_radius",
    "radius for feather operation"
  }
};

ProcRecord rect_select_proc =
{
  "gimp_rect_select",
  "Create a rectangular selection over the specified image",
  "This tool creates a rectangular selection over the specified image.  The rectangular region can be either added to, subtracted from, or replace the contents of the previous selection mask.  If the feather option is enabled, the resulting selection is blurred before combining.  The blur is a gaussian blur with the specified feather radius.",
  "Spencer Kimball & Peter Mattis",
  "Spencer Kimball & Peter Mattis",
  "1995-1996",
  PDB_INTERNAL,

  /*  Input arguments  */
  8,
  rect_select_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { rect_select_invoker } },
};


static Argument *
rect_select_invoker (Argument *args)
{
  int success = TRUE;
  GImage *gimage;
  int op;
  int feather;
  double x, y;
  double w, h;
  double feather_radius;
  int int_value;

  op = REPLACE;

  /*  the gimage  */
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if (! (gimage = gimage_get_ID (int_value)))
	success = FALSE;
    }
  /*  x, y, w, h  */
  if (success)
    {
      x = args[1].value.pdb_float;
      y = args[2].value.pdb_float;
      w = args[3].value.pdb_float;
      h = args[4].value.pdb_float;
    }
  /*  operation  */
  if (success)
    {
      int_value = args[5].value.pdb_int;
      switch (int_value)
	{
	case 0: op = ADD; break;
	case 1: op = SUB; break;
	case 2: op = REPLACE; break;
	case 3: op = INTERSECT; break;
	default: success = FALSE;
	}
    }
  /*  feathering  */
  if (success)
    {
      int_value = args[6].value.pdb_int;
      feather = (int_value) ? TRUE : FALSE;
    }
  /*  feather radius  */
  if (success)
    {
      feather_radius = args[7].value.pdb_float;
    }

  /*  call the rect_select procedure  */
  if (success)
    rect_select (gimage, (int) x, (int) y, (int) w, (int) h,
		 op, feather, feather_radius);

  return procedural_db_return_args (&rect_select_proc, success);
}
