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

#include "disp_callbacks.h"
#include "gdisplay_ops.h"
#include "gimpdnd.h"
#include "interface.h"
#include "gui/menus.h"
#include "nav_window.h"

#include "gimphelp.h"
#include "gimpimage.h"
#include "gimppattern.h"
#include "gimprc.h"
#include "qmask.h"

#include "pixmaps/qmasksel.xpm"
#include "pixmaps/qmasknosel.xpm"
#include "pixmaps/navbutton.xpm"

#include "libgimp/gimpintl.h"


/*  local functions  */
static void   gdisplay_destroy (GtkWidget *widget,
				GDisplay  *display);
static gint   gdisplay_delete  (GtkWidget *widget,
				GdkEvent  *event,
				GDisplay  *display);

static GtkTargetEntry display_target_table[] =
{
  GIMP_TARGET_LAYER,
  GIMP_TARGET_CHANNEL,
  GIMP_TARGET_LAYER_MASK,
  GIMP_TARGET_COLOR,
  GIMP_TARGET_PATTERN
};
static guint display_n_targets = (sizeof (display_target_table) /
				  sizeof (display_target_table[0]));

static void
gdisplay_destroy (GtkWidget *widget,
		  GDisplay  *gdisp)
{
  gdisplay_remove_and_delete (gdisp);
}

static gboolean
gdisplay_delete (GtkWidget *widget,
		 GdkEvent  *event,
		 GDisplay  *gdisp)
{
  gdisplay_close_window (gdisp, FALSE);

  return TRUE;
}

static gboolean
gdisplay_get_accel_context (gpointer data)
{
  GDisplay *gdisp;

  gdisp = (GDisplay *) data;

  if (gdisp)
    return gdisp->gimage;

  return NULL;
}

void
create_display_shell (GDisplay *gdisp,
		      gint      width,
		      gint      height,
		      gchar    *title,
		      gint      type)
{
  static GdkPixmap *qmasksel_pixmap   = NULL;
  static GdkBitmap *qmasksel_mask     = NULL;
  static GdkPixmap *qmasknosel_pixmap = NULL;
  static GdkBitmap *qmasknosel_mask   = NULL;
  static GdkPixmap *navbutton_pixmap  = NULL;
  static GdkBitmap *navbutton_mask    = NULL;

  GtkWidget *main_vbox;
  GtkWidget *disp_vbox;
  GtkWidget *upper_hbox;
  GtkWidget *lower_hbox;
  GtkWidget *inner_table;
  GtkWidget *status_hbox;
  GtkWidget *arrow;
  GtkWidget *pixmap;
  GtkWidget *label_frame;
  GtkWidget *nav_ebox;

  GSList *group = NULL;

  gint n_width, n_height;
  gint s_width, s_height;
  gint scalesrc, scaledest;
  gint contextid;

  /*  adjust the initial scale -- so that window fits on screen
   *  the 75% value is the same as in gdisplay_shrink_wrap. It
   *  probably should be a user-configurable option.
   */
  s_width  = gdk_screen_width () * 0.75;
  s_height = gdk_screen_height () * 0.75;

  scalesrc  = SCALESRC (gdisp);
  scaledest = SCALEDEST (gdisp);

  n_width  = SCALEX (gdisp, width);
  n_height = SCALEX (gdisp, height);

  /*  Limit to the size of the screen...  */
  while (n_width > s_width || n_height > s_height)
    {
      if (scaledest > 1)
	scaledest--;
      else
	if (scalesrc < 0xff)
	  scalesrc++;

      n_width  = width * 
	(scaledest * SCREEN_XRES (gdisp)) / (scalesrc * gdisp->gimage->xresolution);
      n_height = height *
	(scaledest * SCREEN_XRES (gdisp)) / (scalesrc * gdisp->gimage->xresolution);
    }

  gdisp->scale = (scaledest << 8) + scalesrc;

  /*  the toplevel shell */
  gdisp->shell = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_ref  (gdisp->shell);
  gtk_window_set_title (GTK_WINDOW (gdisp->shell), title);
  gtk_window_set_wmclass (GTK_WINDOW (gdisp->shell), "image_window", "Gimp");
  gtk_window_set_policy (GTK_WINDOW (gdisp->shell), TRUE, TRUE, TRUE);
  gtk_object_set_user_data (GTK_OBJECT (gdisp->shell), (gpointer) gdisp);
  gtk_widget_set_events (gdisp->shell, (GDK_POINTER_MOTION_MASK      |
					GDK_POINTER_MOTION_HINT_MASK |
					GDK_BUTTON_PRESS_MASK        |
					GDK_KEY_PRESS_MASK           |
					GDK_KEY_RELEASE_MASK));
  gtk_signal_connect (GTK_OBJECT (gdisp->shell), "delete_event",
		      GTK_SIGNAL_FUNC (gdisplay_delete),
		      gdisp);
  gtk_signal_connect (GTK_OBJECT (gdisp->shell), "destroy",
		      GTK_SIGNAL_FUNC (gdisplay_destroy),
		      gdisp);

  /*  active display callback  */
  gtk_signal_connect (GTK_OBJECT (gdisp->shell), "button_press_event",
		      GTK_SIGNAL_FUNC (gdisplay_shell_events),
		      gdisp);
  gtk_signal_connect (GTK_OBJECT (gdisp->shell), "key_press_event",
		      GTK_SIGNAL_FUNC (gdisplay_shell_events),
		      gdisp);

  /*  dnd stuff  */
  gtk_drag_dest_set (gdisp->shell,
		     GTK_DEST_DEFAULT_ALL,
		     display_target_table, display_n_targets,
		     GDK_ACTION_COPY);
  gtk_signal_connect (GTK_OBJECT (gdisp->shell), "drag_drop",
		      GTK_SIGNAL_FUNC (gdisplay_drag_drop),
		      gdisp);
  gimp_dnd_color_dest_set (gdisp->shell, gdisplay_drop_color, gdisp);
  gimp_dnd_viewable_dest_set (gdisp->shell,
			      GIMP_TYPE_PATTERN,
			      gdisplay_drop_viewable, gdisp);

  /*  the popup menu  */
  gdisp->ifactory = menus_get_image_factory ();

  /*  The accelerator table for images  */
  gimp_window_add_accel_group (GTK_WINDOW (gdisp->shell),
			       gdisp->ifactory,
			       gdisplay_get_accel_context,
			       gdisp);

  /*  connect the "F1" help key  */
  gimp_help_connect_help_accel (gdisp->shell,
				gimp_standard_help_func,
				"image/image_window.html");

  /*  GtkTable widgets are not able to shrink a row/column correctly if
   *  widgets are attached with GTK_EXPAND even if those widgets have
   *  other rows/columns in their rowspan/colspan where they could
   *  nicely expand without disturbing the row/column which is supposed
   *  to shrink. --Mitch
   *
   *  Changed the packing to use hboxes and vboxes which behave nicer:
   *
   *  main_vbox
   *     |
   *     +-- disp_vbox
   *     |      |
   *     |      +-- upper_hbox
   *     |      |      |
   *     |      |      +-- inner_table
   *     |      |      |      |
   *     |      |      |      +-- origin
   *     |      |      |      +-- hruler
   *     |      |      |      +-- vruler
   *     |      |      |      +-- canvas
   *     |      |      |     
   *     |      |      +-- vscrollbar
   *     |      |    
   *     |      +-- lower_hbox
   *     |             |
   *     |             +-- qmaskoff
   *     |             +-- qmaskon
   *     |             +-- hscrollbar
   *     |             +-- navbutton
   *     |
   *     +-- statusarea
   *            |
   *            +-- cursorlabel
   *            +-- statusbar
   *            +-- progressbar
   *            +-- cancelbutton
   */

  /*  first, set up the container hierarchy  *********************************/

  /*  the vbox containing all widgets  */
  main_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 2);
  gtk_container_add (GTK_CONTAINER (gdisp->shell), main_vbox);

  /*  another vbox for everything except the statusbar  */
  disp_vbox = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (main_vbox), disp_vbox, TRUE, TRUE, 0);
  gtk_widget_show (disp_vbox);

  /*  a hbox for the inner_table and the vertical scrollbar  */
  upper_hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (disp_vbox), upper_hbox, TRUE, TRUE, 0);
  gtk_widget_show (upper_hbox);

  /*  the table containing origin, rulers and the canvas  */
  inner_table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (inner_table), 0, 1);
  gtk_table_set_row_spacing (GTK_TABLE (inner_table), 0, 1);
  gtk_box_pack_start (GTK_BOX (upper_hbox), inner_table, TRUE, TRUE, 0);
  gtk_widget_show (inner_table);

  /*  the hbox containing qmask buttons, vertical scrollbar and nav button  */
  lower_hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (disp_vbox), lower_hbox, FALSE, FALSE, 0);
  gtk_widget_show (lower_hbox);

  /*  eventbox and hbox for status area  */
  gdisp->statusarea = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), gdisp->statusarea, FALSE, FALSE, 0);

  gimp_help_set_help_data (gdisp->statusarea, NULL, "#status_area");

  status_hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (gdisp->statusarea), status_hbox);
  gtk_widget_show (status_hbox);

  gtk_container_set_resize_mode (GTK_CONTAINER (status_hbox),
				 GTK_RESIZE_QUEUE);

  /*  create the scrollbars  *************************************************/

  /*  the horizontal scrollbar  */
  gdisp->hsbdata =
    GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, width, 1, 1, width));
  gdisp->hsb = gtk_hscrollbar_new (gdisp->hsbdata);
  GTK_WIDGET_UNSET_FLAGS (gdisp->hsb, GTK_CAN_FOCUS);

  /*  the vertical scrollbar  */
  gdisp->vsbdata =
    GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, height, 1, 1, height));
  gdisp->vsb = gtk_vscrollbar_new (gdisp->vsbdata);
  GTK_WIDGET_UNSET_FLAGS (gdisp->vsb, GTK_CAN_FOCUS);

  /*  create the contents of the inner_table  ********************************/

  /*  the menu popup button  */
  gdisp->origin = gtk_button_new ();
  GTK_WIDGET_UNSET_FLAGS (gdisp->origin, GTK_CAN_FOCUS);
  gtk_widget_set_events (GTK_WIDGET (gdisp->origin),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_signal_connect (GTK_OBJECT (gdisp->origin), "button_press_event",
		      GTK_SIGNAL_FUNC (gdisplay_origin_button_press),
		      gdisp);

  gimp_help_set_help_data (gdisp->origin, NULL, "#origin_button");

  arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
  gtk_container_set_border_width (GTK_CONTAINER (gdisp->origin), 0);
  gtk_container_add (GTK_CONTAINER (gdisp->origin), arrow);
  gtk_widget_show (arrow);

  /*  the horizontal ruler  */
  gdisp->hrule = gtk_hruler_new ();
  gtk_widget_set_events (GTK_WIDGET (gdisp->hrule),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_signal_connect_object (GTK_OBJECT (gdisp->shell), "motion_notify_event",
			     GTK_SIGNAL_FUNC (GTK_WIDGET_CLASS (GTK_OBJECT (gdisp->hrule)->klass)->motion_notify_event),
			     GTK_OBJECT (gdisp->hrule));
  gtk_signal_connect (GTK_OBJECT (gdisp->hrule), "button_press_event",
		      GTK_SIGNAL_FUNC (gdisplay_hruler_button_press),
		      gdisp);

  gimp_help_set_help_data (gdisp->hrule, NULL, "#ruler");

  /*  the vertical ruler  */
  gdisp->vrule = gtk_vruler_new ();
  gtk_widget_set_events (GTK_WIDGET (gdisp->vrule),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_signal_connect_object (GTK_OBJECT (gdisp->shell), "motion_notify_event",
			     GTK_SIGNAL_FUNC (GTK_WIDGET_CLASS (GTK_OBJECT (gdisp->vrule)->klass)->motion_notify_event),
			     GTK_OBJECT (gdisp->vrule));
  gtk_signal_connect (GTK_OBJECT (gdisp->vrule), "button_press_event",
		      GTK_SIGNAL_FUNC (gdisplay_vruler_button_press),
		      gdisp);

  gimp_help_set_help_data (gdisp->vrule, NULL, "#ruler");

  /*  the canvas  */
  gdisp->canvas = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (gdisp->canvas), n_width, n_height);
  gtk_widget_set_events (gdisp->canvas, CANVAS_EVENT_MASK);
  gtk_widget_set_extension_events (gdisp->canvas, GDK_EXTENSION_EVENTS_ALL);
  GTK_WIDGET_SET_FLAGS (gdisp->canvas, GTK_CAN_FOCUS);
  gtk_object_set_user_data (GTK_OBJECT (gdisp->canvas), (gpointer) gdisp);

  /*  set the active display before doing any other canvas event processing  */
  gtk_signal_connect (GTK_OBJECT (gdisp->canvas), "event",
		      GTK_SIGNAL_FUNC (gdisplay_shell_events),
		      gdisp);
  gtk_signal_connect (GTK_OBJECT (gdisp->canvas), "event",
		      GTK_SIGNAL_FUNC (gdisplay_canvas_events),
		      gdisp);

  /*  create the contents of the lower_hbox  *********************************/

  /*  the qmask buttons  */
  gdisp->qmaskoff = gtk_radio_button_new (group);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (gdisp->qmaskoff));
  gtk_widget_set_usize (GTK_WIDGET (gdisp->qmaskoff), 15, 15);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (gdisp->qmaskoff), FALSE);
  GTK_WIDGET_UNSET_FLAGS (gdisp->qmaskoff, GTK_CAN_FOCUS);
  gtk_signal_connect (GTK_OBJECT (gdisp->qmaskoff), "toggled",
		      GTK_SIGNAL_FUNC (qmask_deactivate),
		      gdisp);
  gtk_signal_connect (GTK_OBJECT (gdisp->qmaskoff), "button_press_event",
		      GTK_SIGNAL_FUNC (qmask_click_handler),
		      gdisp);

  gimp_help_set_help_data (gdisp->qmaskoff, NULL, "#qmask_off_button");

  gdisp->qmaskon = gtk_radio_button_new (group);
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (gdisp->qmaskon));
  gtk_widget_set_usize (GTK_WIDGET (gdisp->qmaskon), 15, 15);
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (gdisp->qmaskon), FALSE);
  GTK_WIDGET_UNSET_FLAGS (gdisp->qmaskon, GTK_CAN_FOCUS);
  gtk_signal_connect (GTK_OBJECT (gdisp->qmaskon), "toggled",
		      GTK_SIGNAL_FUNC (qmask_activate),
		      gdisp);
  gtk_signal_connect (GTK_OBJECT (gdisp->qmaskon), "button_press_event",
		      GTK_SIGNAL_FUNC (qmask_click_handler),
		      gdisp);

  gimp_help_set_help_data (gdisp->qmaskon, NULL, "#qmask_on_button");

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gdisp->qmaskoff), TRUE);

  /*  the navigation window button  */
  nav_ebox = gtk_event_box_new ();
  gtk_signal_connect (GTK_OBJECT (nav_ebox), "button_press_event",
		      GTK_SIGNAL_FUNC (nav_popup_click_handler),
		      gdisp);

  gimp_help_set_help_data (nav_ebox, NULL, "#nav_window_button");

  /* We need to realize the shell so that we have a GdkWindow for
   * the pixmap creation.  */

  gtk_widget_realize (gdisp->shell);
  
  /*  create the pixmaps  ****************************************************/
  if (!qmasksel_pixmap)
    {
      GtkStyle *style;

      style = gtk_widget_get_style (gdisp->shell);

      qmasksel_pixmap =
	gdk_pixmap_create_from_xpm_d (gdisp->shell->window,
				      &qmasksel_mask,
				      &style->bg[GTK_STATE_NORMAL],
				      qmasksel_xpm);   
      qmasknosel_pixmap =
	gdk_pixmap_create_from_xpm_d (gdisp->shell->window,
				      &qmasknosel_mask,
				      &style->bg[GTK_STATE_NORMAL],
				      qmasknosel_xpm);   
      navbutton_pixmap =
	gdk_pixmap_create_from_xpm_d (gdisp->shell->window,
				      &navbutton_mask,
				      &style->bg[GTK_STATE_NORMAL],
				      navbutton_xpm);   
    }

  /*  Icon stuff  */
  gdisp->iconsize = 32;
  gdisp->icon = NULL;
  gdisp->iconmask = NULL;
  gdisp->icon_needs_update = 0;
  gdisp->icon_timeout_id = 0;
  gdisp->icon_idle_id = 0;
  gtk_signal_connect (GTK_OBJECT (gdisp->gimage), "invalidate_preview",
                      GTK_SIGNAL_FUNC (gdisplay_update_icon_scheduler),
		      gdisp);
  gdisplay_update_icon_scheduler (gdisp->gimage, gdisp);


  /*  create the GtkPixmaps  */
  pixmap = gtk_pixmap_new (qmasksel_pixmap, qmasksel_mask);
  gtk_container_add (GTK_CONTAINER (gdisp->qmaskon), pixmap);
  gtk_widget_show (pixmap);

  pixmap = gtk_pixmap_new (qmasknosel_pixmap, qmasknosel_mask);
  gtk_container_add (GTK_CONTAINER (gdisp->qmaskoff), pixmap);
  gtk_widget_show (pixmap);

  pixmap = gtk_pixmap_new (navbutton_pixmap, navbutton_mask);
  gtk_container_add (GTK_CONTAINER (nav_ebox), pixmap); 
  gtk_widget_show (pixmap);

  /*  create the contents of the status area *********************************/

  /*  the cursor label  */
  label_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (label_frame), GTK_SHADOW_IN);

  gdisp->cursor_label = gtk_label_new (" ");
  gtk_container_add (GTK_CONTAINER (label_frame), gdisp->cursor_label);
  gtk_widget_show (gdisp->cursor_label);

  /*  the statusbar  */
  gdisp->statusbar = gtk_statusbar_new ();
  gtk_widget_set_usize (gdisp->statusbar, 1, -1);
  gtk_container_set_resize_mode (GTK_CONTAINER (gdisp->statusbar),
				 GTK_RESIZE_QUEUE);
  contextid = gtk_statusbar_get_context_id (GTK_STATUSBAR (gdisp->statusbar),
					    "title");
  gtk_statusbar_push (GTK_STATUSBAR (gdisp->statusbar),
		      contextid,
		      title);

  /*  the progress bar  */
  gdisp->progressbar = gtk_progress_bar_new ();
  gtk_widget_set_usize (gdisp->progressbar, 80, -1);

  /*  the cancel button  */
  gdisp->cancelbutton = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_set_sensitive (gdisp->cancelbutton, FALSE);

  /*  pack all the widgets  **************************************************/

  /*  fill the upper_hbox  */
  gtk_box_pack_start (GTK_BOX (upper_hbox), gdisp->vsb, FALSE, FALSE, 0);

  /*  fill the inner_table  */
  gtk_table_attach (GTK_TABLE (inner_table), gdisp->origin, 0, 1, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (inner_table), gdisp->hrule, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (inner_table), gdisp->vrule, 0, 1, 1, 2,
		    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (inner_table), gdisp->canvas, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  /*  fill the lower_hbox  */
  gtk_box_pack_start (GTK_BOX (lower_hbox), gdisp->qmaskoff, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (lower_hbox), gdisp->qmaskon, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (lower_hbox), gdisp->hsb, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (lower_hbox), nav_ebox, FALSE, FALSE, 0);

  /*  fill the status area  */
  gtk_box_pack_start (GTK_BOX (status_hbox), label_frame, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (status_hbox), gdisp->statusbar, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (status_hbox), gdisp->progressbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (status_hbox), gdisp->cancelbutton, FALSE, FALSE, 0);

  /*  show everything  *******************************************************/

  if (show_rulers)
    {
      gtk_widget_show (gdisp->origin);
      gtk_widget_show (gdisp->hrule);
      gtk_widget_show (gdisp->vrule);
    }
  gtk_widget_show (gdisp->canvas);

  gtk_widget_show (gdisp->vsb);
  gtk_widget_show (gdisp->hsb);

  gtk_widget_show (gdisp->qmaskoff);
  gtk_widget_show (gdisp->qmaskon);
  gtk_widget_show (nav_ebox);

  gtk_widget_show (label_frame);
  gtk_widget_show (gdisp->statusbar);
  gtk_widget_show (gdisp->progressbar);
  gtk_widget_show (gdisp->cancelbutton);
  if (show_statusbar)
    {
      gtk_widget_show (gdisp->statusarea);
    }

  gtk_widget_realize (gdisp->canvas);
  gdk_window_set_back_pixmap (gdisp->canvas->window, NULL, FALSE);

  /*  we need to realize the cursor_label widget here, so the size gets
   *  computed correctly
   */
  gtk_widget_realize (gdisp->cursor_label);
  gdisplay_resize_cursor_label (gdisp);

  /*
  {
    GtkWidget *hbox;
    GtkWidget *preview;

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    preview = gimp_preview_new (GIMP_VIEWABLE (gdisp->gimage->layers->data));
    gtk_widget_set_usize (preview, 64, 64);
    gtk_box_pack_start (GTK_BOX (hbox), preview, FALSE, FALSE, 0);
    gtk_widget_show (preview);
  }
  */

  gtk_widget_show (main_vbox);
  gtk_widget_show (gdisp->shell);

  /*  set the focus to the canvas area  */
  gtk_widget_grab_focus (gdisp->canvas);
}
