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

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "base/temp-buf.h"

#include "core/gimp.h"
#include "core/gimpbuffer.h"
#include "core/gimpcontext.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-mask.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimppattern.h"

#include "plug-in/plug-in.h"

#include "widgets/gimpcolorpanel.h"
#include "widgets/gimpcursor.h"
#include "widgets/gimpdnd.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimpwidgets-utils.h"

#include "gui/gui-types.h" /* FIXME */

#include "gui/info-window.h"

#include "tools/tools-types.h"

#include "tools/tool_manager.h"

#include "gimpdisplay.h"
#include "gimpdisplay-area.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-callbacks.h"
#include "gimpdisplayshell-dnd.h"
#include "gimpdisplayshell-handlers.h"
#include "gimpdisplayshell-render.h"
#include "gimpdisplayshell-selection.h"

#include "gimprc.h"
#include "nav_window.h"
#include "undo.h"

#ifdef DISPLAY_FILTERS
#include "gdisplay_color.h"
#endif /* DISPLAY_FILTERS */

#include "libgimp/gimpintl.h"


#define MAX_TITLE_BUF 256


/*  local function prototypes  */

static void       gimp_display_shell_class_init   (GimpDisplayShellClass *klass);
static void       gimp_display_shell_init         (GimpDisplayShell      *shell);

static void       gimp_display_shell_destroy           (GtkObject        *object);
static gboolean   gimp_display_shell_delete_event      (GtkWidget        *widget,
                                                        GdkEventAny      *aevent);

static gpointer   gimp_display_shell_get_accel_context (gpointer          data);

static void       gimp_display_shell_display_area      (GimpDisplayShell *shell,
                                                        gint              x,
                                                        gint              y,
                                                        gint              w,
                                                        gint              h);
static void	  gimp_display_shell_draw_cursor       (GimpDisplayShell *shell);

static void       gimp_display_shell_format_title      (GimpDisplayShell *gdisp,
                                                        gchar            *title,
                                                        gint              title_len);

static void       gimp_display_shell_update_icon       (GimpDisplayShell *gdisp);
static gboolean gimp_display_shell_update_icon_invoker (gpointer          data);
static void   gimp_display_shell_update_icon_scheduler (GimpImage        *gimage,
                                                        gpointer          data);

static void    gimp_display_shell_close_warning_dialog (GimpDisplayShell *shell,
                                                        const gchar      *image_name);
static void  gimp_display_shell_close_warning_callback (GtkWidget        *widget,
                                                        gboolean          close,
                                                        gpointer          data);


static GtkWindowClass *parent_class = NULL;


static GtkTargetEntry display_target_table[] =
{
  GIMP_TARGET_LAYER,
  GIMP_TARGET_CHANNEL,
  GIMP_TARGET_LAYER_MASK,
  GIMP_TARGET_COLOR,
  GIMP_TARGET_PATTERN,
  GIMP_TARGET_BUFFER
};


GType
gimp_display_shell_get_type (void)
{
  static GType shell_type = 0;

  if (! shell_type)
    {
      static const GTypeInfo shell_info =
      {
        sizeof (GimpDisplayShellClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_display_shell_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpDisplayShell),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_display_shell_init,
      };

      shell_type = g_type_register_static (GTK_TYPE_WINDOW,
                                           "GimpDisplayShell",
                                           &shell_info, 0);
    }

  return shell_type;
}

static void
gimp_display_shell_class_init (GimpDisplayShellClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = GTK_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->destroy      = gimp_display_shell_destroy;

  widget_class->delete_event = gimp_display_shell_delete_event;
}

static void
gimp_display_shell_init (GimpDisplayShell *shell)
{
  shell->gdisp                 = NULL;
  shell->ifactory              = NULL;

  shell->offset_x              = 0;
  shell->offset_y              = 0;

  shell->disp_width            = 0;
  shell->disp_height           = 0;
  shell->disp_xoffset          = 0;
  shell->disp_yoffset          = 0;

  shell->proximity             = FALSE;

  shell->select                = NULL;

  shell->display_areas         = NULL;

  shell->hsbdata               = NULL;
  shell->vsbdata               = NULL;

  shell->canvas                = NULL;

  shell->hsb                   = NULL;
  shell->vsb                   = NULL;
  shell->qmask                 = NULL;
  shell->hrule                 = NULL;
  shell->vrule                 = NULL;
  shell->origin                = NULL;
  shell->statusarea            = NULL;
  shell->progressbar           = NULL;
  shell->progressid            = FALSE;
  shell->cursor_label          = NULL;
  shell->cursor_format_str[0]  = '\0';
  shell->cancelbutton          = NULL;

  shell->render_buf            = g_malloc (GIMP_DISPLAY_SHELL_RENDER_BUF_WIDTH  *
                                           GIMP_DISPLAY_SHELL_RENDER_BUF_HEIGHT *
                                           3);
  shell->render_gc             = NULL;

  shell->icon_size             = 32;
  shell->icon_idle_id          = 0;

  shell->current_cursor        = (GdkCursorType) -1;
  shell->tool_cursor           = GIMP_TOOL_CURSOR_NONE;
  shell->cursor_modifier       = GIMP_CURSOR_MODIFIER_NONE;

  shell->override_cursor       = (GdkCursorType) -1;
  shell->using_override_cursor = FALSE;

  shell->draw_cursor           = FALSE;
  shell->have_cursor           = FALSE;
  shell->cursor_x              = 0;
  shell->cursor_y              = 0;

  shell->padding_button        = NULL;
  gimp_rgb_set (&shell->padding_color, 1.0, 1.0, 1.0);
  shell->padding_gc            = NULL;

  shell->warning_dialog        = NULL;
  shell->info_dialog           = NULL;
  shell->nav_dialog            = NULL;
  shell->nav_popup             = NULL;

#ifdef DISPLAY_FILTERS
  shell->cd_list               = NULL;
  shell->cd_ui                 = NULL;
#endif /* DISPLAY_FILTERS */
}

static void
gimp_display_shell_destroy (GtkObject *object)
{
  GimpDisplayShell *shell;

  shell = GIMP_DISPLAY_SHELL (object);

  if (shell->gdisp)
    {
      gimp_display_shell_disconnect (shell);
    }

  if (shell->select)
    {
      gimp_display_shell_selection_free (shell->select);
      shell->select = NULL;
    }

  shell->display_areas = gimp_display_area_list_free (shell->display_areas);

#ifdef DISPLAY_FILTERS
  /* detach any color displays */
  gdisplay_color_detach_all (gdisp);
#endif /* DISPLAY_FILTERS */

  if (shell->render_buf)
    {
      g_free (shell->render_buf);
      shell->render_buf = NULL;
    }

  if (shell->render_gc)
    {
      g_object_unref (shell->render_gc);
      shell->render_gc = NULL;
    }

  if (shell->icon_idle_id)
    {
      g_source_remove (shell->icon_idle_id);
      shell->icon_idle_id = 0;
    }

  if (shell->padding_gc)
    {
      g_object_unref (G_OBJECT (shell->padding_gc));
      shell->padding_gc = NULL;
    }

  if (shell->info_dialog)
    {
      info_window_free (shell->info_dialog);
      shell->info_dialog = NULL;
    }

  if (shell->nav_dialog)
    {
      nav_dialog_free (shell->gdisp, shell->nav_dialog);
      shell->nav_dialog = NULL;
    }

  if (shell->nav_popup)
    {
      nav_dialog_free (shell->gdisp, shell->nav_popup);
      shell->nav_popup = NULL;
    }

  shell->gdisp = NULL;

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static gboolean
gimp_display_shell_delete_event (GtkWidget   *widget,
                                 GdkEventAny *aevent)
{
  GimpDisplayShell *shell;

  shell = GIMP_DISPLAY_SHELL (widget);

  gimp_display_shell_close (shell, FALSE);

  return TRUE;
}

GtkWidget *
gimp_display_shell_new (GimpDisplay *gdisp)
{
  GimpDisplayShell *shell;
  GtkWidget        *main_vbox;
  GtkWidget        *disp_vbox;
  GtkWidget        *upper_hbox;
  GtkWidget        *right_vbox;
  GtkWidget        *lower_hbox;
  GtkWidget        *inner_table;
  GtkWidget        *status_hbox;
  GtkWidget        *arrow;
  GtkWidget        *image;
  GtkWidget        *label_frame;
  GtkWidget        *nav_ebox;
  gint              image_width, image_height;
  gint              n_width, n_height;
  gint              s_width, s_height;
  gint              scalesrc, scaledest;
  gint              contextid;

  g_return_val_if_fail (GIMP_IS_DISPLAY (gdisp), NULL);

  image_width  = gdisp->gimage->width;
  image_height = gdisp->gimage->height;

  /*  adjust the initial scale -- so that window fits on screen
   *  the 75% value is the same as in gdisplay_shrink_wrap. It
   *  probably should be a user-configurable option.
   */
  s_width  = gdk_screen_width () * 0.75;
  s_height = gdk_screen_height () * 0.75;

  scalesrc  = SCALESRC (gdisp);
  scaledest = SCALEDEST (gdisp);

  n_width  = SCALEX (gdisp, image_width);
  n_height = SCALEX (gdisp, image_height);

  /*  Limit to the size of the screen...  */
  while (n_width > s_width || n_height > s_height)
    {
      if (scaledest > 1)
	scaledest--;
      else
	if (scalesrc < 0xff)
	  scalesrc++;

      n_width  = image_width * 
	(scaledest * SCREEN_XRES (gdisp)) / (scalesrc * gdisp->gimage->xresolution);
      n_height = image_height *
	(scaledest * SCREEN_XRES (gdisp)) / (scalesrc * gdisp->gimage->xresolution);
    }

  gdisp->scale = (scaledest << 8) + scalesrc;

  /*  the toplevel shell */
  shell = g_object_new (GIMP_TYPE_DISPLAY_SHELL, NULL);

  shell->gdisp = gdisp;

  gtk_window_set_wmclass (GTK_WINDOW (shell), "image_window", "Gimp");
  gtk_window_set_policy (GTK_WINDOW (shell), TRUE, TRUE, TRUE);
  gtk_widget_set_events (GTK_WIDGET (shell), (GDK_POINTER_MOTION_MASK      |
                                              GDK_POINTER_MOTION_HINT_MASK |
                                              GDK_BUTTON_PRESS_MASK        |
                                              GDK_KEY_PRESS_MASK           |
                                              GDK_KEY_RELEASE_MASK));

  /*  the popup menu  */
  shell->ifactory = gtk_item_factory_from_path ("<Image>");

  /*  The accelerator table for images  */
  gimp_window_add_accel_group (GTK_WINDOW (shell),
			       shell->ifactory,
			       gimp_display_shell_get_accel_context,
			       shell);

  /*  active display callback  */
  g_signal_connect (G_OBJECT (shell), "button_press_event",
		    G_CALLBACK (gimp_display_shell_events),
		    shell);
  g_signal_connect (G_OBJECT (shell), "key_press_event",
		    G_CALLBACK (gimp_display_shell_events),
		    shell);

  /*  dnd stuff  */
  gtk_drag_dest_set (GTK_WIDGET (shell),
		     GTK_DEST_DEFAULT_ALL,
		     display_target_table,
                     G_N_ELEMENTS (display_target_table),
		     GDK_ACTION_COPY);

  gimp_dnd_viewable_dest_set (GTK_WIDGET (shell), GIMP_TYPE_LAYER,
			      gimp_display_shell_drop_drawable,
                              shell);
  gimp_dnd_viewable_dest_set (GTK_WIDGET (shell), GIMP_TYPE_LAYER_MASK,
			      gimp_display_shell_drop_drawable,
                              shell);
  gimp_dnd_viewable_dest_set (GTK_WIDGET (shell), GIMP_TYPE_CHANNEL,
			      gimp_display_shell_drop_drawable,
                              shell);
  gimp_dnd_viewable_dest_set (GTK_WIDGET (shell), GIMP_TYPE_PATTERN,
			      gimp_display_shell_drop_pattern,
                              shell);
  gimp_dnd_viewable_dest_set (GTK_WIDGET (shell), GIMP_TYPE_BUFFER,
			      gimp_display_shell_drop_buffer,
                              shell);
  gimp_dnd_color_dest_set    (GTK_WIDGET (shell),
			      gimp_display_shell_drop_color,
                              shell);

  /*  connect the "F1" help key  */
  gimp_help_connect (GTK_WIDGET (shell),
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
   *     |      |      +-- right_vbox
   *     |      |             |
   *     |      |             +-- padding_button
   *     |      |             +-- vscrollbar
   *     |      |    
   *     |      +-- lower_hbox
   *     |             |
   *     |             +-- qmask
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
  gtk_container_add (GTK_CONTAINER (shell), main_vbox);

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

  /*  the vbox containing the color button and the vertical scrollbar  */
  right_vbox = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (upper_hbox), right_vbox, FALSE, FALSE, 0);
  gtk_widget_show (right_vbox);

  /*  the hbox containing qmask button, vertical scrollbar and nav button  */
  lower_hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (disp_vbox), lower_hbox, FALSE, FALSE, 0);
  gtk_widget_show (lower_hbox);

  /*  eventbox and hbox for status area  */
  shell->statusarea = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), shell->statusarea, FALSE, FALSE, 0);

  gimp_help_set_help_data (shell->statusarea, NULL, "#status_area");

  status_hbox = gtk_hbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (shell->statusarea), status_hbox);
  gtk_widget_show (status_hbox);

  gtk_container_set_resize_mode (GTK_CONTAINER (status_hbox),
				 GTK_RESIZE_QUEUE);

  /*  create the scrollbars  *************************************************/

  /*  the horizontal scrollbar  */
  shell->hsbdata =
    GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, image_width, 1, 1, image_width));
  shell->hsb = gtk_hscrollbar_new (shell->hsbdata);
  GTK_WIDGET_UNSET_FLAGS (shell->hsb, GTK_CAN_FOCUS);

  /*  the vertical scrollbar  */
  shell->vsbdata =
    GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, image_height, 1, 1, image_height));
  shell->vsb = gtk_vscrollbar_new (shell->vsbdata);
  GTK_WIDGET_UNSET_FLAGS (shell->vsb, GTK_CAN_FOCUS);

  /*  create the contents of the inner_table  ********************************/

  /*  the menu popup button  */
  shell->origin = gtk_button_new ();
  GTK_WIDGET_UNSET_FLAGS (shell->origin, GTK_CAN_FOCUS);
  gtk_widget_set_events (GTK_WIDGET (shell->origin),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  g_signal_connect (G_OBJECT (shell->origin), "button_press_event",
		    G_CALLBACK (gimp_display_shell_origin_button_press),
		    shell);

  gimp_help_set_help_data (shell->origin, NULL, "#origin_button");

  arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
  gtk_container_set_border_width (GTK_CONTAINER (shell->origin), 0);
  gtk_container_add (GTK_CONTAINER (shell->origin), arrow);
  gtk_widget_show (arrow);

  /*  the horizontal ruler  */
  shell->hrule = gtk_hruler_new ();
  gtk_widget_set_events (GTK_WIDGET (shell->hrule),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  g_signal_connect_swapped (G_OBJECT (shell), "motion_notify_event",
			    G_CALLBACK (GTK_WIDGET_GET_CLASS (shell->hrule)->motion_notify_event),
			    shell->hrule);
  g_signal_connect (G_OBJECT (shell->hrule), "button_press_event",
		    G_CALLBACK (gimp_display_shell_hruler_button_press),
		    shell);

  gimp_help_set_help_data (shell->hrule, NULL, "#ruler");

  /*  the vertical ruler  */
  shell->vrule = gtk_vruler_new ();
  gtk_widget_set_events (GTK_WIDGET (shell->vrule),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  g_signal_connect_swapped (G_OBJECT (shell), "motion_notify_event",
			    G_CALLBACK (GTK_WIDGET_GET_CLASS (shell->vrule)->motion_notify_event),
			    shell->vrule);
  g_signal_connect (G_OBJECT (shell->vrule), "button_press_event",
		    G_CALLBACK (gimp_display_shell_vruler_button_press),
		    shell);

  gimp_help_set_help_data (shell->vrule, NULL, "#ruler");

  /*  the canvas  */
  shell->canvas = gtk_drawing_area_new ();
  gtk_widget_set_name (shell->canvas, "gimp-canvas");
  gtk_drawing_area_size (GTK_DRAWING_AREA (shell->canvas), n_width, n_height);
  gtk_widget_set_events (shell->canvas, GIMP_DISPLAY_SHELL_CANVAS_EVENT_MASK);
  gtk_widget_set_extension_events (shell->canvas, GDK_EXTENSION_EVENTS_ALL);
  GTK_WIDGET_SET_FLAGS (shell->canvas, GTK_CAN_FOCUS);

  g_signal_connect (G_OBJECT (shell->canvas), "realize",
                    G_CALLBACK (gimp_display_shell_canvas_realize),
                    shell);

  /*  set the active display before doing any other canvas event processing  */
  g_signal_connect (G_OBJECT (shell->canvas), "event",
		    G_CALLBACK (gimp_display_shell_events),
		    shell);

  g_signal_connect (G_OBJECT (shell->canvas), "expose_event",
		    G_CALLBACK (gimp_display_shell_canvas_expose),
		    shell);
  g_signal_connect (G_OBJECT (shell->canvas), "configure_event",
		    G_CALLBACK (gimp_display_shell_canvas_configure),
		    shell);
  g_signal_connect (G_OBJECT (shell->canvas), "focus_in_event",
		    G_CALLBACK (gimp_display_shell_canvas_focus_in),
		    shell);
  g_signal_connect (G_OBJECT (shell->canvas), "focus_out_event",
		    G_CALLBACK (gimp_display_shell_canvas_focus_in),
		    shell);
  g_signal_connect (G_OBJECT (shell->canvas), "event",
		    G_CALLBACK (gimp_display_shell_canvas_tool_events),
		    shell);

  /*  create the contents of the right_vbox  *********************************/
  shell->padding_button = gimp_color_panel_new (_("Set Canvas Padding Color"),
                                                &shell->padding_color,
                                                GIMP_COLOR_AREA_FLAT,
                                                15, 15);
  GTK_WIDGET_UNSET_FLAGS (shell->padding_button, GTK_CAN_FOCUS);

  gimp_help_set_help_data (shell->padding_button,
                           _("Set canvas padding color"), "#padding_button");

  g_signal_connect (G_OBJECT (shell->padding_button), "color_changed",
                    G_CALLBACK (gimp_display_shell_color_changed),
                    shell);

  /*  create the contents of the lower_hbox  *********************************/

  /*  the qmask button  */
  shell->qmask = gtk_check_button_new ();
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (shell->qmask), FALSE);
  gtk_widget_set_usize (GTK_WIDGET (shell->qmask), 16, 16);
  GTK_WIDGET_UNSET_FLAGS (shell->qmask, GTK_CAN_FOCUS);

  if (gdisp->gimage->qmask_state)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (shell->qmask), TRUE);
      image = gtk_image_new_from_stock (GIMP_STOCK_QMASK_ON,
                                        GTK_ICON_SIZE_MENU);
    }
  else
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (shell->qmask), FALSE);
      image = gtk_image_new_from_stock (GIMP_STOCK_QMASK_OFF,
                                        GTK_ICON_SIZE_MENU);
    }

  gtk_container_add (GTK_CONTAINER (shell->qmask), image);
  gtk_widget_show (image);

  gimp_help_set_help_data (shell->qmask,
                           _("Toggle QuickMask"), "#qmask_button");

  g_signal_connect (G_OBJECT (shell->qmask), "toggled",
		    G_CALLBACK (gimp_display_shell_qmask_toggled),
		    shell);
  g_signal_connect (G_OBJECT (shell->qmask), "button_press_event",
		    G_CALLBACK (gimp_display_shell_qmask_button_press),
		    shell);

  /*  the navigation window button  */
  nav_ebox = gtk_event_box_new ();

  image = gtk_image_new_from_stock (GIMP_STOCK_NAVIGATION, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (nav_ebox), image); 
  gtk_widget_show (image);

  g_signal_connect (G_OBJECT (nav_ebox), "button_press_event",
		    G_CALLBACK (nav_popup_click_handler),
		    gdisp);

  gimp_help_set_help_data (nav_ebox, NULL, "#nav_window_button");

  /*  Icon stuff  */
  g_signal_connect (G_OBJECT (gdisp->gimage), "invalidate_preview",
		    G_CALLBACK (gimp_display_shell_update_icon_scheduler),
		    shell);

  gimp_display_shell_update_icon_scheduler (gdisp->gimage, shell);

  /*  create the contents of the status area *********************************/

  /*  the cursor label  */
  label_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (label_frame), GTK_SHADOW_IN);

  shell->cursor_label = gtk_label_new (" ");
  gtk_container_add (GTK_CONTAINER (label_frame), shell->cursor_label);
  gtk_widget_show (shell->cursor_label);

  /*  the statusbar  */
  shell->statusbar = gtk_statusbar_new ();
  gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (shell->statusbar), FALSE);
  gtk_widget_set_usize (shell->statusbar, 1, -1);
  gtk_container_set_resize_mode (GTK_CONTAINER (shell->statusbar),
				 GTK_RESIZE_QUEUE);

  contextid = gtk_statusbar_get_context_id (GTK_STATUSBAR (shell->statusbar),
					    "title");
  gtk_statusbar_push (GTK_STATUSBAR (shell->statusbar),
		      contextid,
		      "FooBar");

  /*  the progress bar  */
  shell->progressbar = gtk_progress_bar_new ();
  gtk_widget_set_usize (shell->progressbar, 80, -1);

  /*  the cancel button  */
  shell->cancelbutton = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_set_sensitive (shell->cancelbutton, FALSE);

  /*  pack all the widgets  **************************************************/

  /*  fill the inner_table  */
  gtk_table_attach (GTK_TABLE (inner_table), shell->origin, 0, 1, 0, 1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (inner_table), shell->hrule, 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (inner_table), shell->vrule, 0, 1, 1, 2,
		    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (inner_table), shell->canvas, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  /*  fill the right_vbox  */
  gtk_box_pack_start (GTK_BOX (right_vbox), shell->padding_button,
                      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (right_vbox), shell->vsb, TRUE, TRUE, 0);

  /*  fill the lower_hbox  */
  gtk_box_pack_start (GTK_BOX (lower_hbox), shell->qmask, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (lower_hbox), shell->hsb, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (lower_hbox), nav_ebox, FALSE, FALSE, 0);

  /*  fill the status area  */
  gtk_box_pack_start (GTK_BOX (status_hbox), label_frame, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (status_hbox), shell->statusbar, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (status_hbox), shell->progressbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (status_hbox), shell->cancelbutton, FALSE, FALSE, 0);

  /*  show everything  *******************************************************/

  if (gimprc.show_rulers)
    {
      gtk_widget_show (shell->origin);
      gtk_widget_show (shell->hrule);
      gtk_widget_show (shell->vrule);
    }
  gtk_widget_show (shell->canvas);

  gtk_widget_show (shell->vsb);
  gtk_widget_show (shell->hsb);

  gtk_widget_show (shell->padding_button);

  gtk_widget_show (shell->qmask);
  gtk_widget_show (nav_ebox);

  gtk_widget_show (label_frame);
  gtk_widget_show (shell->statusbar);
  gtk_widget_show (shell->progressbar);
  gtk_widget_show (shell->cancelbutton);
  if (gimprc.show_statusbar)
    {
      gtk_widget_show (shell->statusarea);
    }

  gtk_widget_show (main_vbox);

  gimp_display_shell_connect (shell);

  return GTK_WIDGET (shell);
}

void
gimp_display_shell_close (GimpDisplayShell *shell,
                          gboolean          kill_it)
{
  GimpImage *gimage;

  gimage = shell->gdisp->gimage;

  /*  FIXME: gimp_busy HACK not really appropriate here because we only
   *  want to prevent the busy image and display to be closed.  --Mitch
   */
  if (gimage->gimp->busy)
    return;

  /*  If the image has been modified, give the user a chance to save
   *  it before nuking it--this only applies if its the last view
   *  to an image canvas.  (a gimage with disp_count = 1)
   */
  if (! kill_it &&
      (gimage->disp_count == 1) &&
      gimage->dirty &&
      gimprc.confirm_on_close)
    {
      gchar *basename;

      basename = g_path_get_basename (gimp_image_get_filename (gimage));

      gimp_display_shell_close_warning_dialog (shell, basename);

      g_free (basename);
    }
  else
    {
      gimp_display_delete (shell->gdisp);
    }
}

void
gimp_display_shell_reconnect (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_DISPLAY (shell->gdisp));
  g_return_if_fail (GIMP_IS_IMAGE (shell->gdisp->gimage));

  gimp_display_shell_connect (shell);

  gimp_display_shell_resize_cursor_label (shell);
  gimp_display_shell_shrink_wrap (shell);
}

void
gimp_display_shell_transform_coords (GimpDisplayShell *shell,
                                     GimpCoords       *image_coords,
                                     GimpCoords       *display_coords)
{
  gdouble scalex;
  gdouble scaley;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (image_coords != NULL);
  g_return_if_fail (display_coords != NULL);

  scalex = SCALEFACTOR_X (shell->gdisp);
  scaley = SCALEFACTOR_Y (shell->gdisp);

  display_coords->x = scalex * image_coords->x;
  display_coords->y = scaley * image_coords->y;

  display_coords->x += - shell->offset_x + shell->disp_xoffset;
  display_coords->y += - shell->offset_y + shell->disp_yoffset;
}

void
gimp_display_shell_untransform_coords (GimpDisplayShell *shell,
                                       GimpCoords       *display_coords,
                                       GimpCoords       *image_coords)
{
  gdouble scalex;
  gdouble scaley;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (display_coords != NULL);
  g_return_if_fail (image_coords != NULL);

  scalex = SCALEFACTOR_X (shell->gdisp);
  scaley = SCALEFACTOR_Y (shell->gdisp);

  image_coords->x = display_coords->x - shell->disp_xoffset + shell->offset_x;
  image_coords->y = display_coords->y - shell->disp_yoffset + shell->offset_y;

  image_coords->x /= scalex;
  image_coords->y /= scaley;
}

void
gimp_display_shell_set_menu_sensitivity (GimpDisplayShell *shell)
{
  GimpDisplay       *gdisp     = NULL;
  GimpImage         *gimage    = NULL;
  GimpImageBaseType  base_type = 0;
  GimpImageType      type      = -1;
  GimpDrawable      *drawable  = NULL;
  GimpLayer         *layer     = NULL;
  GimpRGB            fg;
  GimpRGB            bg;
  gboolean           fs        = FALSE;
  gboolean           aux       = FALSE;
  gboolean           lm        = FALSE;
  gboolean           lp        = FALSE;
  gboolean           alpha     = FALSE;
  gint               lind      = -1;
  gint               lnum      = -1;

  g_return_if_fail (! shell || GIMP_IS_DISPLAY_SHELL (shell));

  if (shell)
    gdisp = shell->gdisp;

  if (gdisp)
    {
      gimage = gdisp->gimage;

      base_type = gimp_image_base_type (gimage);

      fs  = (gimp_image_floating_sel (gimage) != NULL);
      aux = (gimp_image_get_active_channel (gimage) != NULL);
      lp  = ! gimp_image_is_empty (gimage);

      drawable = gimp_image_active_drawable (gimage);
      if (drawable)
	type = gimp_drawable_type (drawable);

      if (lp)
	{
	  layer = gimp_image_get_active_layer (gimage);

	  if (layer)
	    {
	      lm    = gimp_layer_get_mask (layer) ? TRUE : FALSE;
	      alpha = gimp_layer_has_alpha (layer);
	      lind  = gimp_image_get_layer_index (gimage, layer);
	    }

	  lnum = gimp_container_num_children (gimage->layers);
	}

      gimp_context_get_foreground (gimp_get_user_context (gdisp->gimage->gimp),
                                   &fg);
      gimp_context_get_background (gimp_get_user_context (gdisp->gimage->gimp),
                                   &bg);

    }

#define SET_ACTIVE(menu,condition) \
        gimp_menu_item_set_active ("<Image>/" menu, (condition) != 0)
#define SET_LABEL(menu,label) \
        gimp_menu_item_set_label ("<Image>/" menu, (label))
#define SET_SENSITIVE(menu,condition) \
        gimp_menu_item_set_sensitive ("<Image>/" menu, (condition) != 0)

  SET_SENSITIVE ("File/Save", gdisp && drawable);
  SET_SENSITIVE ("File/Save as...", gdisp && drawable);
  SET_SENSITIVE ("File/Save a Copy as...", gdisp && drawable);
  SET_SENSITIVE ("File/Revert...", gdisp && GIMP_OBJECT (gimage)->name);
  SET_SENSITIVE ("File/Close", gdisp);

  SET_SENSITIVE ("Edit", gdisp);
  SET_SENSITIVE ("Edit/Buffer", gdisp);
  if (gdisp)
    {
      gchar *undo_name = NULL;
      gchar *redo_name = NULL;

      if (gimp_image_undo_is_enabled (gimage))
        {
          undo_name = (gchar *) undo_get_undo_name (gimage);
          redo_name = (gchar *) undo_get_redo_name (gimage);
        }

      if (undo_name)
        undo_name = g_strdup_printf (_("Undo %s"), gettext (undo_name));

      if (redo_name)
        redo_name = g_strdup_printf (_("Redo %s"), gettext (redo_name));

      SET_LABEL ("Edit/Undo", undo_name ? undo_name : _("Undo"));
      SET_LABEL ("Edit/Redo", redo_name ? redo_name : _("Redo"));

      SET_SENSITIVE ("Edit/Undo", undo_name);
      SET_SENSITIVE ("Edit/Redo", redo_name);

      g_free (undo_name);
      g_free (redo_name);

      SET_SENSITIVE ("Edit/Cut", lp);
      SET_SENSITIVE ("Edit/Copy", lp);
      SET_SENSITIVE ("Edit/Buffer/Cut Named...", lp);
      SET_SENSITIVE ("Edit/Buffer/Copy Named...", lp);
      SET_SENSITIVE ("Edit/Clear", lp);
      SET_SENSITIVE ("Edit/Fill with FG Color", lp);
      SET_SENSITIVE ("Edit/Fill with BG Color", lp);
      SET_SENSITIVE ("Edit/Stroke", lp);
    }

  SET_SENSITIVE ("Select", gdisp && lp);
  SET_SENSITIVE ("Select/Save to Channel", !fs);

  SET_SENSITIVE ("View", gdisp);
  SET_SENSITIVE ("View/Zoom", gdisp);
  if (gdisp)
    {
      SET_ACTIVE ("View/Toggle Selection", ! shell->select->hidden);
      SET_ACTIVE ("View/Toggle Layer Boundary", ! shell->select->layer_hidden);
      SET_ACTIVE ("View/Toggle Rulers",
                  GTK_WIDGET_VISIBLE (shell->origin) ? 1 : 0);
      SET_ACTIVE ("View/Toggle Guides", gdisp->draw_guides);
      SET_ACTIVE ("View/Snap to Guides", gdisp->snap_to_guides);
      SET_ACTIVE ("View/Toggle Statusbar",
                  GTK_WIDGET_VISIBLE (shell->statusarea) ? 1 : 0);
      SET_ACTIVE ("View/Dot for Dot", gdisp->dot_for_dot);
    }

  SET_SENSITIVE ("Image", gdisp);

  SET_SENSITIVE ("Image/Mode", gdisp);
  if (gdisp)
    {
      SET_SENSITIVE ("Image/Mode/RGB", (base_type != RGB));
      SET_SENSITIVE ("Image/Mode/Grayscale", (base_type != GRAY));
      SET_SENSITIVE ("Image/Mode/Indexed...", (base_type != INDEXED));
    }

  SET_SENSITIVE ("Layer/Stack", gdisp);
  if (gdisp)
    {
      SET_SENSITIVE ("Layer/Stack/Previous Layer",
		     !fs && !aux && lp && lind > 0);
      SET_SENSITIVE ("Layer/Stack/Next Layer",
		     !fs && !aux && lp && lind < (lnum - 1));
      SET_SENSITIVE ("Layer/Stack/Raise Layer",
		     !fs && !aux && lp && alpha && lind > 0);
      SET_SENSITIVE ("Layer/Stack/Lower Layer",
		     !fs && !aux && lp && alpha && lind < (lnum - 1));
      SET_SENSITIVE ("Layer/Stack/Layer to Top",
		     !fs && !aux && lp && alpha && lind > 0);
      SET_SENSITIVE ("Layer/Stack/Layer to Bottom",
		     !fs && !aux && lp && alpha && lind < (lnum - 1));
    }

  SET_SENSITIVE ("Layer/New Layer...", gdisp);
  SET_SENSITIVE ("Layer/Duplicate Layer", gdisp && !fs && !aux && lp);
  SET_SENSITIVE ("Layer/Anchor Layer", gdisp && fs && !aux && lp);
  SET_SENSITIVE ("Layer/Delete Layer", gdisp && !aux && lp);

  SET_SENSITIVE ("Layer/Layer Boundary Size...", gdisp && !aux && lp);
  SET_SENSITIVE ("Layer/Layer to Imagesize", gdisp && !aux && lp);
  SET_SENSITIVE ("Layer/Scale Layer...", gdisp && !aux && lp);

  SET_SENSITIVE ("Layer/Transform/Offset...", lp);

  SET_SENSITIVE ("Layer/Merge Visible Layers...", gdisp && !fs && !aux && lp);
  SET_SENSITIVE ("Layer/Merge Down", gdisp && !fs && !aux && lp);
  SET_SENSITIVE ("Layer/Flatten Image", gdisp && !fs && !aux && lp);

  SET_SENSITIVE ("Layer/Colors", gdisp);
  SET_SENSITIVE ("Layer/Colors/Auto", gdisp);

  if (gdisp)
    {
      SET_SENSITIVE ("Layer/Colors", lp);
      SET_SENSITIVE ("Layer/Colors/Color Balance...", (base_type == RGB));
      SET_SENSITIVE ("Layer/Colors/Hue-Saturation...", (base_type == RGB));
      SET_SENSITIVE ("Layer/Colors/Brightness-Contrast...", (base_type != INDEXED));
      SET_SENSITIVE ("Layer/Colors/Threshold...", (base_type != INDEXED));
      SET_SENSITIVE ("Layer/Colors/Levels...", (base_type != INDEXED));
      SET_SENSITIVE ("Layer/Colors/Curves...", (base_type != INDEXED));
      SET_SENSITIVE ("Layer/Colors/Desaturate", (base_type == RGB));
      SET_SENSITIVE ("Layer/Colors/Posterize...", (base_type != INDEXED));
      SET_SENSITIVE ("Layer/Colors/Invert", (base_type != INDEXED));
      SET_SENSITIVE ("Layer/Colors/Auto/Equalize", (base_type != INDEXED));

      SET_SENSITIVE ("Layer/Colors/Histogram...", lp);
    }

  SET_SENSITIVE ("Layer/Mask/Add Layer Mask...", 
		 gdisp && !aux && !lm && lp && alpha && (base_type != INDEXED));
  SET_SENSITIVE ("Layer/Mask/Apply Layer Mask",
                 gdisp && !aux && lm && lp);
  SET_SENSITIVE ("Layer/Mask/Delete Layer Mask",
                 gdisp && !aux && lm && lp);
  SET_SENSITIVE ("Layer/Mask/Mask to Selection",
                 gdisp && !aux && lm && lp);

  SET_SENSITIVE ("Layer/Alpha/Alpha to Selection",
                 gdisp && !aux && lp && alpha);
  SET_SENSITIVE ("Layer/Alpha/Add Alpha Channel",
		 gdisp && !fs && !aux && lp && !lm && !alpha);

  SET_SENSITIVE ("Filters", gdisp && lp);

  SET_SENSITIVE ("Script-Fu", gdisp && lp);

#undef SET_ACTIVE
#undef SET_LABEL
#undef SET_SENSITIVE

  plug_in_set_menu_sensitivity (type);
}

GimpGuide *
gimp_display_shell_find_guide (GimpDisplayShell *shell,
                               gdouble           x,
                               gdouble           y)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  if (shell->gdisp->draw_guides)
    {
      gdouble image_x, image_y;

      gdisplay_untransform_coords_f (shell->gdisp,
                                     x, y,
                                     &image_x, &image_y,
                                     TRUE);

      return gimp_image_find_guide (shell->gdisp->gimage,
                                    (gint) image_x,
                                    (gint) image_y);
    }

  return NULL;
}

gboolean
gimp_display_shell_snap_point (GimpDisplayShell *shell,
                               gdouble           x,
                               gdouble           y,
                               gdouble          *tx,
                               gdouble          *ty)
{
  gboolean snapped = FALSE;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);
  g_return_val_if_fail (tx != NULL, FALSE);
  g_return_val_if_fail (ty != NULL, FALSE);

  *tx = x;
  *ty = y;

  if (shell->gdisp->draw_guides &&
      shell->gdisp->snap_to_guides &&
      shell->gdisp->gimage->guides)
    {
      gdouble  image_x, image_y;
      gint     image_tx, image_ty;

      gdisplay_untransform_coords_f (shell->gdisp,
                                     x, y,
                                     &image_x, &image_y,
                                     TRUE);

      snapped = gimp_image_snap_point (shell->gdisp->gimage,
                                       (gint) image_x,
                                       (gint) image_y,
                                       &image_tx,
                                       &image_ty);

      if (snapped)
        {
          gdisplay_transform_coords_f (shell->gdisp,
                                       (gdouble) image_tx, (gdouble) image_ty,
                                       tx, ty,
                                       FALSE);
        }
    }

  return snapped;
}

gboolean
gimp_display_shell_snap_rectangle (GimpDisplayShell *shell,
                                   gdouble           x1,
                                   gdouble           y1,
                                   gdouble           x2,
                                   gdouble           y2,
                                   gdouble          *tx1,
                                   gdouble          *ty1)
{
  gdouble  nx1, ny1;
  gdouble  nx2, ny2;
  gboolean snap1, snap2;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);
  g_return_val_if_fail (tx1 != NULL, FALSE);
  g_return_val_if_fail (ty1 != NULL, FALSE);

  *tx1 = x1;
  *ty1 = y1;

  snap1 = gimp_display_shell_snap_point (shell, x1, y1, &nx1, &ny1);
  snap2 = gimp_display_shell_snap_point (shell, x2, y2, &nx2, &ny2);
  
  if (snap1 || snap2)
    {
      if (x1 != nx1)
	*tx1 = nx1;
      else if (x2 != nx2)
	*tx1 = x1 + (nx2 - x2);
  
      if (y1 != ny1)
	*ty1 = ny1;
      else if (y2 != ny2)
	*ty1 = y1 + (ny2 - y2);
    }

  return snap1 || snap2;
}

gint
gimp_display_shell_mask_value (GimpDisplayShell *shell,
                               gint              x,
                               gint              y)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), 0);

  /*  move the coordinates from screen space to image space  */
  gdisplay_untransform_coords (shell->gdisp, x, y, &x, &y, FALSE, FALSE);

  return gimp_image_mask_value (shell->gdisp->gimage, x, y);
}

gboolean
gimp_display_shell_mask_bounds (GimpDisplayShell *shell,
                                gint             *x1,
                                gint             *y1,
                                gint             *x2,
                                gint             *y2)
{
  GimpLayer *layer;
  gint       off_x;
  gint       off_y;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);
  g_return_val_if_fail (x1 != NULL, FALSE);
  g_return_val_if_fail (y1 != NULL, FALSE);
  g_return_val_if_fail (x2 != NULL, FALSE);
  g_return_val_if_fail (y2 != NULL, FALSE);

  /*  If there is a floating selection, handle things differently  */
  if ((layer = gimp_image_floating_sel (shell->gdisp->gimage)))
    {
      gimp_drawable_offsets (GIMP_DRAWABLE (layer), &off_x, &off_y);

      if (! gimp_channel_bounds (gimp_image_get_mask (shell->gdisp->gimage),
				 x1, y1, x2, y2))
	{
	  *x1 = off_x;
	  *y1 = off_y;
	  *x2 = off_x + gimp_drawable_width (GIMP_DRAWABLE (layer));
	  *y2 = off_y + gimp_drawable_height (GIMP_DRAWABLE (layer));
	}
      else
	{
	  *x1 = MIN (off_x, *x1);
	  *y1 = MIN (off_y, *y1);
	  *x2 = MAX (off_x + gimp_drawable_width (GIMP_DRAWABLE (layer)), *x2);
	  *y2 = MAX (off_y + gimp_drawable_height (GIMP_DRAWABLE (layer)), *y2);
	}
    }
  else if (! gimp_channel_bounds (gimp_image_get_mask (shell->gdisp->gimage),
				  x1, y1, x2, y2))
    {
      return FALSE;
    }

  gdisplay_transform_coords (shell->gdisp, *x1, *y1, x1, y1, 0);
  gdisplay_transform_coords (shell->gdisp, *x2, *y2, x2, y2, 0);

  /*  Make sure the extents are within bounds  */
  *x1 = CLAMP (*x1, 0, shell->disp_width);
  *y1 = CLAMP (*y1, 0, shell->disp_height);
  *x2 = CLAMP (*x2, 0, shell->disp_width);
  *y2 = CLAMP (*y2, 0, shell->disp_height);

  return TRUE;
}

void
gimp_display_shell_add_expose_area (GimpDisplayShell *shell,
                                    gint              x,
                                    gint              y,
                                    gint              w,
                                    gint              h)
{
  GimpArea *area;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  area = gimp_area_new (CLAMP (x,     0, shell->disp_width),
                        CLAMP (y,     0, shell->disp_height),
                        CLAMP (x + w, 0, shell->disp_width),
                        CLAMP (y + h, 0, shell->disp_height));

  shell->display_areas = gimp_display_area_list_process (shell->display_areas,
                                                         area);
}

void
gimp_display_shell_expose_guide (GimpDisplayShell *shell,
                                 GimpGuide        *guide)
{
  gint x;
  gint y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (guide != NULL);

  if (guide->position < 0)
    return;

  gdisplay_transform_coords (shell->gdisp,
                             guide->position,
			     guide->position,
                             &x, &y,
                             FALSE);

  switch (guide->orientation)
    {
    case ORIENTATION_HORIZONTAL:
      gimp_display_shell_add_expose_area (shell, 0, y, shell->disp_width, 1);
      break;

    case ORIENTATION_VERTICAL:
      gimp_display_shell_add_expose_area (shell, x, 0, 1, shell->disp_height);
      break;

    default:
      break;
    }
}

void
gimp_display_shell_expose_full (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_add_expose_area (shell,
                                      0, 0,
                                      shell->disp_width,
                                      shell->disp_height);
}

void
gimp_display_shell_flush (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->display_areas)
    {
      GSList   *list;
      GimpArea *area;

      /*  stop the currently active tool  */
      tool_manager_control_active (shell->gdisp->gimage->gimp, PAUSE,
                                   shell->gdisp);

      for (list = shell->display_areas; list; list = g_slist_next (list))
	{
	  /*  Paint the area specified by the GimpArea  */

	  area = (GimpArea *) list->data;

	  gimp_display_shell_display_area (shell,
                                           area->x1,
                                           area->y1,
                                           (area->x2 - area->x1),
                                           (area->y2 - area->y1));
	}

      /*  Free the update lists  */
      shell->display_areas = gimp_display_area_list_free (shell->display_areas);

      /* draw the guides */
      gimp_display_shell_draw_guides (shell);

      /* and the cursor (if we have a software cursor */
      if (shell->have_cursor)
	gimp_display_shell_draw_cursor (shell);

      /* restart (and recalculate) the selection boundaries */
      gimp_display_shell_selection_start (shell->select, TRUE);

      /* start the currently active tool */
      tool_manager_control_active (shell->gdisp->gimage->gimp, RESUME,
                                   shell->gdisp);
    }  
}

void
gimp_display_shell_real_install_tool_cursor (GimpDisplayShell   *shell,
                                             GdkCursorType       cursor_type,
                                             GimpToolCursorType  tool_cursor,
                                             GimpCursorModifier  modifier,
                                             gboolean            always_install)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (cursor_type != GIMP_BAD_CURSOR)
    {
      switch (gimprc.cursor_mode)
	{
	case CURSOR_MODE_TOOL_ICON:
	  break;

	case CURSOR_MODE_TOOL_CROSSHAIR:
	  cursor_type = GIMP_CROSSHAIR_SMALL_CURSOR;
	  break;

	case CURSOR_MODE_CROSSHAIR:
	  cursor_type = GIMP_CROSSHAIR_CURSOR;
	  tool_cursor = GIMP_TOOL_CURSOR_NONE;
	  modifier    = GIMP_CURSOR_MODIFIER_NONE;
	  break;
	}
    }

  if (shell->current_cursor  != cursor_type ||
      shell->tool_cursor     != tool_cursor ||
      shell->cursor_modifier != modifier    ||
      always_install)
    {
      GdkCursor *cursor;

      shell->current_cursor  = cursor_type;
      shell->tool_cursor     = tool_cursor;
      shell->cursor_modifier = modifier;

      cursor = gimp_cursor_new (cursor_type,
				tool_cursor,
				modifier);
      gdk_window_set_cursor (shell->canvas->window, cursor);
      gdk_cursor_unref (cursor);
    }
}

void
gimp_display_shell_install_tool_cursor (GimpDisplayShell   *shell,
                                        GdkCursorType       cursor_type,
                                        GimpToolCursorType  tool_cursor,
                                        GimpCursorModifier  modifier)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (! shell->using_override_cursor)
    {
      gimp_display_shell_real_install_tool_cursor (shell,
                                                   cursor_type,
                                                   tool_cursor,
                                                   modifier,
                                                   FALSE);
    }
}

void
gimp_display_shell_remove_tool_cursor (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gdk_window_set_cursor (shell->canvas->window, NULL);
}

void
gimp_display_shell_install_override_cursor (GimpDisplayShell *shell,
                                            GdkCursorType     cursor_type)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (! shell->using_override_cursor ||
      (shell->using_override_cursor &&
       shell->override_cursor != cursor_type))
    {
      GdkCursor *cursor;

      shell->override_cursor       = cursor_type;
      shell->using_override_cursor = TRUE;

      cursor = gimp_cursor_new (cursor_type,
				GIMP_TOOL_CURSOR_NONE,
				GIMP_CURSOR_MODIFIER_NONE);
      gdk_window_set_cursor (shell->canvas->window, cursor);
      gdk_cursor_unref (cursor);
    }
}

void
gimp_display_shell_remove_override_cursor (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->using_override_cursor)
    {
      shell->using_override_cursor = FALSE;

      gimp_display_shell_real_install_tool_cursor (shell,
                                                   shell->current_cursor,
                                                   shell->tool_cursor,
                                                   shell->cursor_modifier,
                                                   TRUE);
    }
}

void
gimp_display_shell_update_cursor (GimpDisplayShell *shell,
                                  gint              x, 
                                  gint              y)
{
  GimpImage *gimage;
  gboolean   new_cursor;
  gboolean   flush = FALSE;
  gchar      buffer[CURSOR_STR_LENGTH];
  gint       t_x = -1;
  gint       t_y = -1;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimage = shell->gdisp->gimage;

  new_cursor = (shell->draw_cursor &&
                shell->proximity   &&
                x > 0              &&
                y > 0);

  /* Erase old cursor, if necessary */

  if (shell->have_cursor && (! new_cursor         ||
                             x != shell->cursor_x ||
                             y != shell->cursor_y))
    {
      gimp_display_shell_add_expose_area (shell,
                                          shell->cursor_x - 7,
                                          shell->cursor_y - 7,
                                          15, 15);
      if (! new_cursor)
	{
	  shell->have_cursor = FALSE;
	  flush = TRUE;
	}
    }

  shell->have_cursor = new_cursor;
  shell->cursor_x    = x;
  shell->cursor_y    = y;
  
  if (new_cursor || flush)
    {
      gimp_display_shell_flush (shell);
    }

  if (x > 0 && y > 0)
    {
      gdisplay_untransform_coords (shell->gdisp, x, y, &t_x, &t_y, FALSE, FALSE);
    }

  if (t_x < 0              ||
      t_y < 0              ||
      t_x >= gimage->width ||
      t_y >= gimage->height)
    {
      gtk_label_set_text (GTK_LABEL (shell->cursor_label), "");
      info_window_update_extended (shell->gdisp, -1, -1);
    } 
  else 
    {
      if (shell->gdisp->dot_for_dot)
	{
	  g_snprintf (buffer, sizeof (buffer), shell->cursor_format_str,
                      "", t_x, ", ", t_y);
	}
      else /* show real world units */
	{
	  gdouble unit_factor = gimp_unit_get_factor (gimage->unit);
	  
	  g_snprintf (buffer, sizeof (buffer), shell->cursor_format_str,
                      "",
                      (gdouble) t_x * unit_factor / gimage->xresolution,
                      ", ",
                      (gdouble) t_y * unit_factor / gimage->yresolution);
	}

      gtk_label_set_text (GTK_LABEL (shell->cursor_label), buffer);
      info_window_update_extended (shell->gdisp, t_x, t_y);
    }
}

void
gimp_display_shell_resize_cursor_label (GimpDisplayShell *shell)
{
  static PangoLayout *layout = NULL;

  GimpImage *gimage;
  gchar      buffer[CURSOR_STR_LENGTH];
  gint       cursor_label_width;
  gint       label_frame_size_difference;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimage = shell->gdisp->gimage;

  if (shell->gdisp->dot_for_dot)
    {
      g_snprintf (shell->cursor_format_str, sizeof (shell->cursor_format_str),
		  "%%s%%d%%s%%d");
      g_snprintf (buffer, sizeof (buffer), shell->cursor_format_str,
		  "", gimage->width, ", ", gimage->height);
    }
  else /* show real world units */
    {
      gdouble unit_factor = gimp_unit_get_factor (gimage->unit);

      g_snprintf (shell->cursor_format_str, sizeof (shell->cursor_format_str),
		  "%%s%%.%df%%s%%.%df %s",
		  gimp_unit_get_digits (gimage->unit),
		  gimp_unit_get_digits (gimage->unit),
		  gimp_unit_get_symbol (gimage->unit));

      g_snprintf (buffer, sizeof (buffer), shell->cursor_format_str,
		  "",
		  (gdouble) gimage->width * unit_factor /
		  gimage->xresolution,
		  ", ",
		  (gdouble) gimage->height * unit_factor /
		  gimage->yresolution);
    }

  /* one static layout for all displays should be fine */
  if (! layout)
    layout = gtk_widget_create_pango_layout (shell->cursor_label, buffer);
  else
    pango_layout_set_text (layout, buffer, -1);

  pango_layout_get_pixel_size (layout, &cursor_label_width, NULL);
  
  /*  find out how many pixels the label's parent frame is bigger than
   *  the label itself
   */
  label_frame_size_difference =
    shell->cursor_label->parent->allocation.width -
    shell->cursor_label->allocation.width;

  gtk_widget_set_usize (shell->cursor_label, cursor_label_width, -1);
      
  /* don't resize if this is a new display */
  if (label_frame_size_difference)
    gtk_widget_set_usize (shell->cursor_label->parent,
                          cursor_label_width + label_frame_size_difference, -1);

  gimp_display_shell_update_cursor (shell,
                                    shell->cursor_x,
                                    shell->cursor_y);
}

void
gimp_display_shell_update_title (GimpDisplayShell *shell)
{
  gchar title[MAX_TITLE_BUF];
  guint context_id;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  /* format the title */
  gimp_display_shell_format_title (shell, title, MAX_TITLE_BUF);
  gdk_window_set_title (GTK_WIDGET (shell)->window, title);

  /* update the statusbar */
  context_id =
    gtk_statusbar_get_context_id (GTK_STATUSBAR (shell->statusbar), "title");
  gtk_statusbar_pop (GTK_STATUSBAR (shell->statusbar), context_id);
  gtk_statusbar_push (GTK_STATUSBAR (shell->statusbar), context_id, title);
}

void
gimp_display_shell_draw_guide (GimpDisplayShell *shell,
                               GimpGuide        *guide,
                               gboolean          active)
{
  static GdkGC    *normal_hgc  = NULL;
  static GdkGC    *active_hgc  = NULL;
  static GdkGC    *normal_vgc  = NULL;
  static GdkGC    *active_vgc  = NULL;
  static gboolean  initialized = FALSE;
  gint x1, x2;
  gint y1, y2;
  gint w, h;
  gint x, y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (guide != NULL);

  if (guide->position < 0)
    return;

  if (! initialized)
    {
      GdkGCValues values;
      GdkColor    fg;
      GdkColor    bg;

      const gchar stipple[] =
      {
	0xF0,    /*  ####----  */
	0xE1,    /*  ###----#  */
	0xC3,    /*  ##----##  */
	0x87,    /*  #----###  */
	0x0F,    /*  ----####  */
	0x1E,    /*  ---####-  */
	0x3C,    /*  --####--  */
	0x78,    /*  -####---  */
      };

      initialized = TRUE;

      {
        fg.red   = 0x0;
        fg.green = 0x0;
        fg.blue  = 0x0;

        bg.red   = 0x0;
        bg.green = 0x7f7f;
        bg.blue  = 0xffff;

        values.fill = GDK_OPAQUE_STIPPLED;
        values.stipple = gdk_bitmap_create_from_data (shell->canvas->window,
                                                      (gchar *) stipple, 8, 1);
        normal_hgc = gdk_gc_new_with_values (shell->canvas->window, &values,
                                             GDK_GC_FILL       |
                                             GDK_GC_STIPPLE);

        gdk_gc_set_rgb_fg_color (normal_hgc, &fg);
        gdk_gc_set_rgb_bg_color (normal_hgc, &bg);

        values.fill = GDK_OPAQUE_STIPPLED;
        values.stipple = gdk_bitmap_create_from_data (shell->canvas->window,
                                                      (gchar *) stipple, 1, 8);
        normal_vgc = gdk_gc_new_with_values (shell->canvas->window, &values,
                                             GDK_GC_FILL       |
                                             GDK_GC_STIPPLE);

        gdk_gc_set_rgb_fg_color (normal_vgc, &fg);
        gdk_gc_set_rgb_bg_color (normal_vgc, &bg);
      }

      {
        fg.red   = 0x0;
        fg.green = 0x0;
        fg.blue  = 0x0;

        bg.red   = 0xffff;
        bg.green = 0x0;
        bg.blue  = 0x0;

        values.fill = GDK_OPAQUE_STIPPLED;
        values.stipple = gdk_bitmap_create_from_data (shell->canvas->window,
                                                      (gchar *) stipple, 8, 1);
        active_hgc = gdk_gc_new_with_values (shell->canvas->window, &values,
                                             GDK_GC_FILL       |
                                             GDK_GC_STIPPLE);

        gdk_gc_set_rgb_fg_color (active_hgc, &fg);
        gdk_gc_set_rgb_bg_color (active_hgc, &bg);

        values.fill = GDK_OPAQUE_STIPPLED;
        values.stipple = gdk_bitmap_create_from_data (shell->canvas->window,
                                                      (gchar *) stipple, 1, 8);
        active_vgc = gdk_gc_new_with_values (shell->canvas->window, &values,
                                             GDK_GC_FILL       |
                                             GDK_GC_STIPPLE);

        gdk_gc_set_rgb_fg_color (active_vgc, &fg);
        gdk_gc_set_rgb_bg_color (active_vgc, &bg);
      }
    }

  gdisplay_transform_coords (shell->gdisp, 0, 0, &x1, &y1, FALSE);
  gdisplay_transform_coords (shell->gdisp,
			     shell->gdisp->gimage->width,
                             shell->gdisp->gimage->height,
			     &x2, &y2, FALSE);
  gdk_drawable_get_size (shell->canvas->window, &w, &h);

  if (x1 < 0) x1 = 0;
  if (y1 < 0) y1 = 0;
  if (x2 > w) x2 = w;
  if (y2 > h) y2 = h;

  if (guide->orientation == ORIENTATION_HORIZONTAL)
    {
      gdisplay_transform_coords (shell->gdisp,
                                 0, guide->position, &x, &y, FALSE);

      if (active)
	gdk_draw_line (shell->canvas->window, active_hgc, x1, y, x2, y);
      else
	gdk_draw_line (shell->canvas->window, normal_hgc, x1, y, x2, y);
    }
  else if (guide->orientation == ORIENTATION_VERTICAL)
    {
      gdisplay_transform_coords (shell->gdisp,
                                 guide->position, 0, &x, &y, FALSE);

      if (active)
	gdk_draw_line (shell->canvas->window, active_vgc, x, y1, x, y2);
      else
	gdk_draw_line (shell->canvas->window, normal_vgc, x, y1, x, y2);
    }
}

void
gimp_display_shell_draw_guides (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->gdisp->draw_guides)
    {
      GList     *list;
      GimpGuide *guide;

      for (list = shell->gdisp->gimage->guides; list; list = g_list_next (list))
	{
	  guide = (GimpGuide *) list->data;

	  gimp_display_shell_draw_guide (shell, guide, FALSE);
	}
    }
}

void
gimp_display_shell_shrink_wrap (GimpDisplayShell *shell)
{
  /* 
   * I'm pretty sure this assumes that the current size is < display size
   * Is this a valid assumption? 
   */
  GtkAllocation allocation;
  gint          disp_width, disp_height;
  gint          width, height;
  gint          shell_x, shell_y;
  gint          shell_width, shell_height;
  gint          max_auto_width, max_auto_height;
  gint          border_x, border_y;
  gint          s_width, s_height;
  gboolean      resize = FALSE;

  s_width  = gdk_screen_width ();
  s_height = gdk_screen_height ();

  width  = SCALEX (shell->gdisp, shell->gdisp->gimage->width);
  height = SCALEY (shell->gdisp, shell->gdisp->gimage->height);

  disp_width  = shell->disp_width;
  disp_height = shell->disp_height;

  shell_width  = GTK_WIDGET (shell)->allocation.width;
  shell_height = GTK_WIDGET (shell)->allocation.height;

  border_x = shell_width  - disp_width;
  border_y = shell_height - disp_height;

  max_auto_width  = (s_width  - border_x) * 0.75;
  max_auto_height = (s_height - border_y) * 0.75;

  allocation.x = 0;
  allocation.y = 0;

  /* If one of the display dimensions has changed and one of the
   * dimensions fits inside the screen
   */
  if (((width + border_x) < s_width || (height + border_y) < s_height) &&
      (width != disp_width || height != disp_height))
    {
      width  = ((width  + border_x) < s_width)  ? width  : max_auto_width;
      height = ((height + border_y) < s_height) ? height : max_auto_height;

      resize = TRUE;
    }
  /*  If the projected dimension is greater than current, but less than
   *  3/4 of the screen size, expand automagically
   */
  else if ((width > disp_width || height > disp_height) &&
	   (disp_width < max_auto_width || disp_height < max_auto_height))
    {
      width  = MIN (max_auto_width, width);
      height = MIN (max_auto_height, height);

      resize = TRUE;
    }

  if (resize)
    {
      if (width < shell->statusarea->requisition.width) 
        { 
          width = shell->statusarea->requisition.width; 
        }

#undef RESIZE_DEBUG
#ifdef RESIZE_DEBUG
      g_print ("1w:%d/%d d:%d/%d s:%d/%d b:%d/%d\n",
	       width, height,
	       disp_width, disp_height,
	       shell_width, shell_height,
	       border_x, border_y);
#endif /* RESIZE_DEBUG */

      shell->disp_width  = width;
      shell->disp_height = height;

      allocation.width  = width  + border_x;
      allocation.height = height + border_y;

      /*  block the resulting expose event on any of the following
       *  changes because our caller has to do a full display update anyway
       */
      g_signal_handlers_block_by_func (G_OBJECT (shell->canvas),
                                       gimp_display_shell_canvas_expose,
                                       shell);

      gtk_widget_size_allocate (GTK_WIDGET (shell), &allocation);

      gdk_window_resize (GTK_WIDGET (shell)->window,
			 allocation.width,
			 allocation.height);

      /*  let Gtk/X/WM position the window  */
      while (gtk_events_pending ())
	gtk_main_iteration ();

      gdk_window_get_origin (GTK_WIDGET (shell)->window, &shell_x, &shell_y);

      /*  if the window is offscreen, center it...  */
      if (shell_x > s_width || shell_y > s_height ||
	  (shell_x + width +  border_x) < 0 || (shell_y + height + border_y) < 0)
	{
	  shell_x = (s_width  - width  - border_x) >> 1;
	  shell_y = (s_height - height - border_y) >> 1;

	  gdk_window_move (GTK_WIDGET (shell)->window, shell_x, shell_y);
	}

      g_signal_handlers_unblock_by_func (G_OBJECT (shell->canvas),
                                         gimp_display_shell_canvas_expose,
                                         shell);
    }

  /*  If the width or height of the display has changed, recalculate
   *  the display offsets...
   */
  if (disp_width  != shell->disp_width ||
      disp_height != shell->disp_height)
    {
      shell->offset_x += (disp_width  - shell->disp_width) / 2;
      shell->offset_y += (disp_height - shell->disp_height) / 2;
    }
}

void
gimp_display_shell_selection_visibility (GimpDisplayShell     *shell,
                                         GimpSelectionControl  control)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->select)
    {
      switch (control)
	{
	case GIMP_SELECTION_OFF:
	  gimp_display_shell_selection_invis (shell->select);
	  break;
	case GIMP_SELECTION_LAYER_OFF:
	  gimp_display_shell_selection_layer_invis (shell->select);
	  break;
	case GIMP_SELECTION_ON:
	  gimp_display_shell_selection_start (shell->select, TRUE);
	  break;
	case GIMP_SELECTION_PAUSE:
	  gimp_display_shell_selection_pause (shell->select);
	  break;
	case GIMP_SELECTION_RESUME:
	  gimp_display_shell_selection_resume (shell->select);
	  break;
	}
    }
}


/*  private functions  */

static gpointer
gimp_display_shell_get_accel_context (gpointer data)
{
  GimpDisplayShell *shell;

  shell = (GimpDisplayShell *) data;

  if (shell)
    return shell->gdisp->gimage;

  return NULL;
}

static void
gimp_display_shell_display_area (GimpDisplayShell *shell,
                                 gint              x,
                                 gint              y,
                                 gint              w,
                                 gint              h)
{
  gint    sx, sy;
  gint    x1, y1;
  gint    x2, y2;
  gint    dx, dy;
  gint    i, j;

  sx = SCALEX (shell->gdisp, shell->gdisp->gimage->width);
  sy = SCALEY (shell->gdisp, shell->gdisp->gimage->height);

  /*  Bounds check  */
  x1 = CLAMP (x,     0, shell->disp_width);
  y1 = CLAMP (y,     0, shell->disp_height);
  x2 = CLAMP (x + w, 0, shell->disp_width);
  y2 = CLAMP (y + h, 0, shell->disp_height);

  if (y1 < shell->disp_yoffset)
    {
      gdk_draw_rectangle (shell->canvas->window,
			  shell->padding_gc,
                          TRUE,
			  x, y,
                          w, shell->disp_yoffset - y);
      /* X X X
         . # .
         . . . */

      y1 = shell->disp_yoffset;
    }

  if (x1 < shell->disp_xoffset)
    {
      gdk_draw_rectangle (shell->canvas->window,
			  shell->padding_gc,
                          TRUE,
			  x, y1,
                          shell->disp_xoffset - x, h);
      /* . . .
         X # .
         X . . */

      x1 = shell->disp_xoffset;
    }

  if (x2 > (shell->disp_xoffset + sx))
    {
      gdk_draw_rectangle (shell->canvas->window,
			  shell->padding_gc,
                          TRUE,
			  shell->disp_xoffset + sx, y1,
			  x2 - (shell->disp_xoffset + sx), h - (y1 - y));
      /* . . .
         . # X
         . . X */

      x2 = shell->disp_xoffset + sx;
    }

  if (y2 > (shell->disp_yoffset + sy))
    {
      gdk_draw_rectangle (shell->canvas->window,
			  shell->padding_gc,
                          TRUE,
			  x1, shell->disp_yoffset + sy,
			  x2 - x1, y2 - (shell->disp_yoffset + sy));
      /* . . .
         . # .
         . X . */

      y2 = shell->disp_yoffset + sy;
    }

  /*  display the image in RENDER_BUF_WIDTH x RENDER_BUF_HEIGHT sized chunks */
  for (i = y1; i < y2; i += GIMP_DISPLAY_SHELL_RENDER_BUF_HEIGHT)
    {
      for (j = x1; j < x2; j += GIMP_DISPLAY_SHELL_RENDER_BUF_WIDTH)
        {
          dx = MIN (x2 - j, GIMP_DISPLAY_SHELL_RENDER_BUF_WIDTH);
          dy = MIN (y2 - i, GIMP_DISPLAY_SHELL_RENDER_BUF_HEIGHT);

          gimp_display_shell_render (shell,
                                     j - shell->disp_xoffset,
                                     i - shell->disp_yoffset,
                                     dx, dy);

#if 0
          /* Invalidate the projection just after we render it! */
          gimp_image_invalidate_without_render (shell->gdisp->gimage,
                                                j - shell->disp_xoffset,
                                                i - shell->disp_yoffset,
                                                dx, dy,
                                                0, 0, 0, 0);
#endif
        }
    }
}

static void
gimp_display_shell_draw_cursor (GimpDisplayShell *shell)
{
  gint x, y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  x = shell->cursor_x;
  y = shell->cursor_y;

  gdk_draw_line (shell->canvas->window,
		 shell->canvas->style->white_gc,
		 x - 7, y-1, x + 7, y-1);
  gdk_draw_line (shell->canvas->window,
		 shell->canvas->style->black_gc,
		 x - 7, y, x + 7, y);
  gdk_draw_line (shell->canvas->window,
		 shell->canvas->style->white_gc,
		 x - 7, y+1, x + 7, y+1);
  gdk_draw_line (shell->canvas->window,
		 shell->canvas->style->white_gc,
		 x-1, y - 7, x-1, y + 7);
  gdk_draw_line (shell->canvas->window,
		 shell->canvas->style->black_gc,
		 x, y - 7, x, y + 7);
  gdk_draw_line (shell->canvas->window,
		 shell->canvas->style->white_gc,
		 x+1, y - 7, x+1, y + 7);
}

static void
gimp_display_shell_update_icon (GimpDisplayShell *shell)
{
  GdkPixbuf *pixbuf;
  gint       width, height;
  gdouble    factor;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  factor = ((gdouble) gimp_image_get_height (shell->gdisp->gimage) /
            (gdouble) gimp_image_get_width (shell->gdisp->gimage));

  if (factor >= 1)
    {
      height = MAX (shell->icon_size, 1);
      width  = MAX (((gdouble) shell->icon_size) / factor, 1);
    }
  else
    {
      height = MAX (((gdouble) shell->icon_size) * factor, 1);
      width  = MAX (shell->icon_size, 1);
    }

  pixbuf = gimp_viewable_get_new_preview_pixbuf (GIMP_VIEWABLE (shell->gdisp->gimage),
                                                 width, height);

  gtk_window_set_icon (GTK_WINDOW (shell), pixbuf);

  g_object_unref (G_OBJECT (pixbuf));
}

/* Just a dumb invoker for gdisplay_update_icon ()
 */
static gboolean
gimp_display_shell_update_icon_invoker (gpointer data)
{
  GimpDisplayShell *shell;

  shell = GIMP_DISPLAY_SHELL (data);

  shell->icon_idle_id = 0;

  gimp_display_shell_update_icon (shell);

  return FALSE;
}

/* This function marks the icon as invalid and sets up the infrastructure
 * to check every 8 seconds if an update is necessary.
 */
static void
gimp_display_shell_update_icon_scheduler (GimpImage *gimage,
                                          gpointer   data)
{
  GimpDisplayShell *shell;

  shell = GIMP_DISPLAY_SHELL (data);

  if (shell->icon_idle_id)
    {
      g_source_remove (shell->icon_idle_id);
    }

  shell->icon_idle_id = g_idle_add_full (G_PRIORITY_LOW,
                                         gimp_display_shell_update_icon_invoker,
                                         shell,
                                         NULL);
}

static int print (char *, int, int, const char *, ...) G_GNUC_PRINTF (4, 5);

static int
print (char *buf, int len, int start, const char *fmt, ...)
{
  va_list args;
  int printed;

  va_start (args, fmt);

  printed = g_vsnprintf (buf + start, len - start, fmt, args);
  if (printed < 0)
    printed = len - start;

  va_end (args);

  return printed;
}

static void
gimp_display_shell_format_title (GimpDisplayShell *shell,
                                 gchar            *title,
                                 gint              title_len)
{
  GimpImage *gimage;
  gchar     *image_type_str;
  gboolean   empty;
  gint       i;
  gchar     *format;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimage = shell->gdisp->gimage;

  empty = gimp_image_is_empty (gimage);

  switch (gimp_image_base_type (gimage))
    {
    case RGB:
      image_type_str = empty ? _("RGB-empty") : _("RGB");
      break;
    case GRAY:
      image_type_str = empty ? _("grayscale-empty") : _("grayscale");
      break;
    case INDEXED:
      image_type_str = empty ? _("indexed-empty") : _("indexed");
      break;
    default:
      image_type_str = NULL;
      break;
    }

  i = 0;
  format = gimprc.image_title_format;

  while (i < title_len && *format)
    {
      switch (*format)
	{
	case '%':
	  format++;
	  switch (*format)
	    {
	    case 0:
	      g_warning ("image-title-format string ended within %%-sequence");
	      break;

	    case '%':
	      title[i++] = '%';
	      break;

	    case 'f': /* pruned filename */
	      {
		gchar *basename;

		basename =
                  g_path_get_basename (gimp_image_get_filename (gimage));

		i += print (title, title_len, i, "%s", basename);

		g_free (basename);
	      }
	      break;

	    case 'F': /* full filename */
	      i += print (title, title_len, i, "%s", gimp_image_get_filename (gimage));
	      break;

	    case 'p': /* PDB id */
	      i += print (title, title_len, i, "%d", gimp_image_get_ID (gimage));
	      break;

	    case 'i': /* instance */
	      i += print (title, title_len, i, "%d", shell->gdisp->instance);
	      break;

	    case 't': /* type */
	      i += print (title, title_len, i, "%s", image_type_str);
	      break;

	    case 's': /* user source zoom factor */
	      i += print (title, title_len, i, "%d", SCALESRC (shell->gdisp));
	      break;

	    case 'd': /* user destination zoom factor */
	      i += print (title, title_len, i, "%d", SCALEDEST (shell->gdisp));
	      break;

	    case 'z': /* user zoom factor (percentage) */
	      i += print (title, title_len, i, "%d",
                          100 * SCALEDEST (shell->gdisp) / SCALESRC (shell->gdisp));
	      break;

	    case 'D': /* dirty flag */
	      if (format[1] == 0)
		{
		  g_warning("image-title-format string ended within %%D-sequence");
		  break;
		}
	      if (gimage->dirty)
		title[i++] = format[1];
	      format++;
	      break;

	      /* Other cool things to be added:
	       * %m = memory used by picture
	       * some kind of resolution / image size thing
	       * people seem to want to know the active layer name
	       */

	    default:
	      g_warning ("image-title-format contains unknown format sequence '%%%c'", *format);
	      break;
	    }
	  break;

	default:
	  title[i++] = *format;
	  break;
	}

      format++;
    }

  title[MIN(i, title_len-1)] = 0;
}

static void
gimp_display_shell_close_warning_dialog (GimpDisplayShell *shell,
                                         const gchar      *image_name)
{
  GtkWidget *mbox;
  gchar     *title_buf;
  gchar     *warning_buf;

  if (shell->warning_dialog)
    {
      gdk_window_raise (shell->warning_dialog->window);
      return;
    }

  warning_buf = g_strdup_printf (_("Changes were made to %s.\nClose anyway?"), 
                                 image_name);
  title_buf = g_strdup_printf (_("Close %s?"), image_name);

  shell->warning_dialog = mbox =
    gimp_query_boolean_box (title_buf,
			    gimp_standard_help_func,
			    "dialogs/really_close.html",
			    FALSE,
			    warning_buf,
			    GTK_STOCK_CLOSE, GTK_STOCK_CANCEL,
			    NULL, NULL,
			    gimp_display_shell_close_warning_callback,
			    shell);

  g_free (title_buf);
  g_free (warning_buf);

  gtk_widget_show (mbox);
}

static void
gimp_display_shell_close_warning_callback (GtkWidget *widget,
                                           gboolean   close,
                                           gpointer   data)
{
  GimpDisplayShell *shell;

  shell = GIMP_DISPLAY_SHELL (data);

  shell->warning_dialog = NULL;

  if (close)
    {
      gimp_display_delete (shell->gdisp);
    }
}
