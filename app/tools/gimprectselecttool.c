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

#define NO  0
#define YES 1

#define FIXED_ENTRY_SIZE 50
#define FIXED_ENTRY_MAX_CHARS 10

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
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (w)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static void
selection_spinbutton_update (GtkWidget *widget,
			     gpointer   data)
{
  int *val;

  val = data;
  *val = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));
}

static void
selection_scale_update (GtkAdjustment *adjustment,
			double        *scale_val)
{
  *scale_val = adjustment->value;
}

SelectionOptions *
create_selection_options (ToolType tool_type)
{
  SelectionOptions *options;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label = NULL;
  GtkWidget *antialias_toggle;
  GtkWidget *feather_toggle;
  GtkWidget *feather_scale;
  GtkWidget *sample_merged_toggle;
  GtkWidget *bezier_toggle;
  GtkObject *feather_scale_data;
  GtkWidget *fixed_size_toggle;
  GtkAdjustment *adj;
  GtkWidget *spinbutton;

  /*  the new options structure  */
  options = (SelectionOptions *) g_malloc (sizeof (SelectionOptions));
  options->antialias = TRUE;
  options->feather = FALSE;
  options->feather_radius = 10.0;
  options->sample_merged = FALSE;
  options->fixed_size = FALSE;
  options->fixed_height = 1;
  options->fixed_width = 1;
  options->sample_merged = TRUE;

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);

  /*  the main label  */
  switch (tool_type)
    {
    case RECT_SELECT:
      label = gtk_label_new ("Rectangular Select Options");
      break;
    case ELLIPSE_SELECT:
      label = gtk_label_new ("Elliptical Selection Options");
      break;
    case FREE_SELECT:
      label = gtk_label_new ("Free-hand Selection Options");
      break;
    case FUZZY_SELECT:
      label = gtk_label_new ("Fuzzy Selection Options");
      break;
    case BEZIER_SELECT:
      label = gtk_label_new ("Bezier Selection Options");
      break;
    case ISCISSORS:
      label = gtk_label_new ("Intelligent Scissors Options");
      break;
    case BY_COLOR_SELECT:
      label = gtk_label_new ("By-Color Select Options");
      break;
    default:
      break;
    }

  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

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
      sample_merged_toggle = gtk_check_button_new_with_label ("Sample Merged");
      gtk_box_pack_start (GTK_BOX (vbox), sample_merged_toggle, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (sample_merged_toggle), "toggled",
			  (GtkSignalFunc) selection_toggle_update,
			  &options->sample_merged);
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (sample_merged_toggle), options->sample_merged);
      gtk_widget_show (sample_merged_toggle);
      break;
    default:
      break;
    }

  if (tool_type == BEZIER_SELECT)
    {
      bezier_toggle = gtk_check_button_new_with_label ("Bezier Extends");
      gtk_box_pack_start (GTK_BOX (vbox), bezier_toggle, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (bezier_toggle), "toggled",
			  (GtkSignalFunc) selection_toggle_update,
			  &options->extend);
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (bezier_toggle), options->extend);
      gtk_widget_show (bezier_toggle);

    }

  /*  the antialias toggle button  */
  if (tool_type != RECT_SELECT)
    {
      antialias_toggle = gtk_check_button_new_with_label ("Antialiasing");
      gtk_box_pack_start (GTK_BOX (vbox), antialias_toggle, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (antialias_toggle), "toggled",
			  (GtkSignalFunc) selection_toggle_update,
			  &options->antialias);
      gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (antialias_toggle), options->antialias);
      gtk_widget_show (antialias_toggle);
    }

  /* Widgets for fixed size select */
  if (tool_type == RECT_SELECT || tool_type == ELLIPSE_SELECT) 
    {
      fixed_size_toggle = gtk_check_button_new_with_label ("Fixed size / aspect ratio");
      gtk_box_pack_start (GTK_BOX(vbox), fixed_size_toggle, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT(fixed_size_toggle), "toggled",
			  (GtkSignalFunc)selection_toggle_update,
			  &options->fixed_size);
      gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(fixed_size_toggle),
				  options->fixed_size);
      gtk_widget_show(fixed_size_toggle);
      
      hbox = gtk_hbox_new (TRUE, 5);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      label = gtk_label_new (" Width: ");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      adj = (GtkAdjustment *) gtk_adjustment_new (options->fixed_width, 1.0,
                                                  32767.0, 1.0, 50.0, 0.0);
      spinbutton = gtk_spin_button_new (adj, 1.0, 0.0);
      gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(spinbutton), GTK_SHADOW_NONE);
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spinbutton), TRUE);
      gtk_widget_set_usize (spinbutton, 75, 0);
      gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (spinbutton), "changed",
                          (GtkSignalFunc) selection_spinbutton_update,
                          &options->fixed_width);
      gtk_widget_show (label);
      gtk_widget_show (spinbutton);
      gtk_widget_show (hbox);
      
      hbox = gtk_hbox_new (TRUE, 5);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      label = gtk_label_new (" Height: ");
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      adj = (GtkAdjustment *) gtk_adjustment_new (options->fixed_height, 1.0,
                                                  32767.0, 1.0, 50.0, 0.0);
      spinbutton = gtk_spin_button_new (adj, 1.0, 0.0);
      gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(spinbutton), GTK_SHADOW_NONE);
      gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spinbutton), TRUE);
      gtk_widget_set_usize (spinbutton, 75, 0);
      gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (spinbutton), "changed",
                          (GtkSignalFunc) selection_spinbutton_update,
                          &options->fixed_height);
      gtk_widget_show (label);
      gtk_widget_show (spinbutton);
      gtk_widget_show (hbox);
    }

  /*  the feather toggle button  */
  feather_toggle = gtk_check_button_new_with_label ("Feather");
  gtk_box_pack_start (GTK_BOX (vbox), feather_toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (feather_toggle), "toggled",
		      (GtkSignalFunc) selection_toggle_update,
		      &options->feather);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (feather_toggle), options->feather);
  gtk_widget_show (feather_toggle);

  /*  the feather radius scale  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new ("Feather Radius");
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  feather_scale_data = gtk_adjustment_new (options->feather_radius, 0.0, 100.0, 1.0, 1.0, 0.0);
  feather_scale = gtk_hscale_new (GTK_ADJUSTMENT (feather_scale_data));
  gtk_box_pack_start (GTK_BOX (hbox), feather_scale, TRUE, TRUE, 0);
  gtk_scale_set_value_pos (GTK_SCALE (feather_scale), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (feather_scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (feather_scale_data), "value_changed",
		      (GtkSignalFunc) selection_scale_update,
		      &options->feather_radius);
  gtk_widget_show (feather_scale);
  gtk_widget_show (hbox);

  /*  Register this selection options widget with the main tools options dialog  */
  tools_register_options (tool_type, vbox);

  return options;
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
      break;
    case ELLIPSE_SELECT:
      rect_sel->fixed_size = ellipse_options->fixed_size;
      rect_sel->fixed_width = ellipse_options->fixed_width;
      rect_sel->fixed_height = ellipse_options->fixed_height;
      break;
    default:
      break;
    }
  if (rect_sel->fixed_size) {
    rect_sel->w = rect_sel->fixed_width;
    rect_sel->h = rect_sel->fixed_height;
  } else {	
    rect_sel->w = 0;
    rect_sel->h = 0;
  }

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
      g_snprintf (select_mode, STATUSBAR_SIZE, "Selection: ADD");
      break;
    case SUB:
      g_snprintf (select_mode, STATUSBAR_SIZE, "Selection: SUBTRACT");
      break;
    case INTERSECT:
      g_snprintf (select_mode, STATUSBAR_SIZE, "Selection: INTERSECT");
      break;
    case REPLACE:
      g_snprintf (select_mode, STATUSBAR_SIZE, "Selection: REPLACE");
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

      if ((!w || !h) && !rect_sel->fixed_width)
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
	  th = (int)((x - ox) * ratio);
	  tw = (int)((y - oy) / ratio);

	  /*******************************************************
	   I'm currently not satisfied with the way that this algorithm
	   decides which measurement (mouse position) to snap the selection
	   box to.  If you have a better idea either tell me
	   (email: chap@cc.gatech.edu) or patch this sucker and send it.
	  *******************************************************/
	  if (abs(tw - (x - oy)) < abs(th - (y - oy))) {
		w = tw;
		h = (int)(w * ratio);
	  } else {
		h = th;
		w = (int)(h / ratio);
	  }

 	} else {
	  w = rect_sel->fixed_width;
	  h = rect_sel->fixed_height;
	  ox = x;
	  oy = y;
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
	  rect_sel->x = ox - w / 2;
	  rect_sel->y = oy - h / 2;
	  rect_sel->w = w;
	  rect_sel->h = h;
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
  g_snprintf (size, STATUSBAR_SIZE, "Selection: %d x %d", abs(rect_sel->w), abs(rect_sel->h));
  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar), rect_sel->context_id, size);

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

Tool *
tools_new_rect_select ()
{
  Tool * tool;
  RectSelect * private;

  /*  The tool options  */
  if (!rect_options)
    rect_options = create_selection_options (RECT_SELECT);

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
    "The image"
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
