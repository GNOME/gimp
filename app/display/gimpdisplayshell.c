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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"
#include "tools/tools-types.h"

#ifdef __GNUC__
#warning FIXME #include "gui/gui-types.h"
#endif
#include "gui/gui-types.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-utils.h"
#include "config/gimpdisplayconfig.h"
#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpbuffer.h"
#include "core/gimpcontext.h"
#include "core/gimpgrid.h"
#include "core/gimpimage.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-snap.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimplist.h"
#include "core/gimpmarshal.h"
#include "core/gimppattern.h"

#include "file/file-utils.h"

#include "vectors/gimpvectors.h"
#include "vectors/gimpstroke.h"

#include "widgets/gimpcolorpanel.h"
#include "widgets/gimpdnd.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimpmenufactory.h"
#include "widgets/gimpwidgets-utils.h"

#include "gui/info-window.h"

#include "tools/tool_manager.h"

#include "gimpcanvas.h"
#include "gimpdisplay.h"
#include "gimpdisplayoptions.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-callbacks.h"
#include "gimpdisplayshell-cursor.h"
#include "gimpdisplayshell-dnd.h"
#include "gimpdisplayshell-draw.h"
#include "gimpdisplayshell-filter.h"
#include "gimpdisplayshell-handlers.h"
#include "gimpdisplayshell-render.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-selection.h"
#include "gimpdisplayshell-title.h"
#include "gimpdisplayshell-transform.h"
#include "gimpstatusbar.h"

#include "gimp-intl.h"


enum
{
  SCALED,
  SCROLLED,
  RECONNECT,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void      gimp_display_shell_class_init    (GimpDisplayShellClass *klass);
static void      gimp_display_shell_init          (GimpDisplayShell      *shell);

static void      gimp_display_shell_finalize           (GObject          *object);
static void      gimp_display_shell_destroy            (GtkObject        *object);
static void      gimp_display_shell_screen_changed     (GtkWidget        *widget,
                                                        GdkScreen        *previous);
static gboolean  gimp_display_shell_delete_event       (GtkWidget        *widget,
							GdkEventAny      *aevent);

static void      gimp_display_shell_real_scaled        (GimpDisplayShell *shell);

static void  gimp_display_shell_close_warning_dialog   (GimpDisplayShell *shell,
                                                        GimpImage        *gimage);
static void  gimp_display_shell_close_warning_response (GtkWidget        *widget,
                                                        gboolean          close,
                                                        GimpDisplayShell *shell);


static guint  display_shell_signals[LAST_SIGNAL] = { 0 };

static GtkWindowClass *parent_class = NULL;


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
  GObjectClass   *gobject_class;
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  object_class  = GTK_OBJECT_CLASS (klass);
  widget_class  = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  display_shell_signals[SCALED] =
    g_signal_new ("scaled",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDisplayShellClass, scaled),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  display_shell_signals[SCROLLED] =
    g_signal_new ("scrolled",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDisplayShellClass, scrolled),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  display_shell_signals[RECONNECT] =
    g_signal_new ("reconnect",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDisplayShellClass, reconnect),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gobject_class->finalize      = gimp_display_shell_finalize;

  object_class->destroy        = gimp_display_shell_destroy;

  widget_class->screen_changed = gimp_display_shell_screen_changed;
  widget_class->delete_event   = gimp_display_shell_delete_event;
  widget_class->popup_menu     = gimp_display_shell_popup_menu;

  klass->scaled                = gimp_display_shell_real_scaled;
  klass->scrolled              = NULL;
  klass->reconnect             = NULL;
}

static void
gimp_display_shell_init (GimpDisplayShell *shell)
{
  shell->gdisp                 = NULL;
  shell->menubar_factory       = NULL;
  shell->popup_factory         = NULL;
  shell->qmask_factory         = NULL;

  shell->scale                 = 1.0;
  shell->other_scale           = 0.0;
  shell->dot_for_dot           = TRUE;

  shell->offset_x              = 0;
  shell->offset_y              = 0;

  shell->disp_width            = 0;
  shell->disp_height           = 0;
  shell->disp_xoffset          = 0;
  shell->disp_yoffset          = 0;

  shell->proximity             = FALSE;
  shell->snap_to_guides        = TRUE;
  shell->snap_to_grid          = FALSE;

  shell->select                = NULL;

  shell->hsbdata               = NULL;
  shell->vsbdata               = NULL;

  shell->canvas                = NULL;
  shell->grid_gc               = NULL;

  shell->hsb                   = NULL;
  shell->vsb                   = NULL;
  shell->qmask                 = NULL;
  shell->hrule                 = NULL;
  shell->vrule                 = NULL;
  shell->origin                = NULL;

  shell->statusbar             = NULL;

  shell->render_buf            = g_malloc (GIMP_DISPLAY_SHELL_RENDER_BUF_WIDTH  *
                                           GIMP_DISPLAY_SHELL_RENDER_BUF_HEIGHT *
                                           3);

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
  shell->nav_ebox              = NULL;

  shell->warning_dialog        = NULL;
  shell->info_dialog           = NULL;
  shell->scale_dialog          = NULL;
  shell->nav_popup             = NULL;
  shell->grid_dialog           = NULL;

  shell->filter_stack          = NULL;
  shell->filter_idle_id        = 0;
  shell->filters_dialog        = NULL;

  shell->window_state          = 0;

  shell->paused_count          = 0;

  shell->options               =
    g_object_new (GIMP_TYPE_DISPLAY_OPTIONS, NULL);
  shell->fullscreen_options    =
    g_object_new (GIMP_TYPE_DISPLAY_OPTIONS_FULLSCREEN, NULL);

  shell->space_pressed         = FALSE;
  shell->space_release_pending = FALSE;
  shell->scrolling             = FALSE;
  shell->scroll_start_x        = 0;
  shell->scroll_start_y        = 0;
  shell->button_press_before_focus = FALSE;

  gtk_window_set_role (GTK_WINDOW (shell), "gimp-image-window");
  gtk_window_set_resizable (GTK_WINDOW (shell), TRUE);

  gtk_widget_set_events (GTK_WIDGET (shell), (GDK_POINTER_MOTION_MASK      |
                                              GDK_POINTER_MOTION_HINT_MASK |
                                              GDK_BUTTON_PRESS_MASK        |
                                              GDK_KEY_PRESS_MASK           |
                                              GDK_KEY_RELEASE_MASK         |
                                              GDK_FOCUS_CHANGE_MASK        |
                                              GDK_SCROLL_MASK));

  /*  active display callback  */
  g_signal_connect (shell, "button_press_event",
		    G_CALLBACK (gimp_display_shell_events),
		    shell);
  g_signal_connect (shell, "button_release_event",
		    G_CALLBACK (gimp_display_shell_events),
		    shell);
  g_signal_connect (shell, "key_press_event",
		    G_CALLBACK (gimp_display_shell_events),
		    shell);
  g_signal_connect (shell, "window_state_event",
		    G_CALLBACK (gimp_display_shell_events),
		    shell);

  /*  dnd stuff  */
  gimp_dnd_viewable_dest_add (GTK_WIDGET (shell), GIMP_TYPE_LAYER,
			      gimp_display_shell_drop_drawable,
                              shell);
  gimp_dnd_viewable_dest_add (GTK_WIDGET (shell), GIMP_TYPE_LAYER_MASK,
			      gimp_display_shell_drop_drawable,
                              shell);
  gimp_dnd_viewable_dest_add (GTK_WIDGET (shell), GIMP_TYPE_CHANNEL,
			      gimp_display_shell_drop_drawable,
                              shell);
  gimp_dnd_svg_dest_add      (GTK_WIDGET (shell),
			      gimp_display_shell_drop_svg,
                              shell);
  gimp_dnd_viewable_dest_add (GTK_WIDGET (shell), GIMP_TYPE_VECTORS,
			      gimp_display_shell_drop_vectors,
                              shell);
  gimp_dnd_viewable_dest_add (GTK_WIDGET (shell), GIMP_TYPE_PATTERN,
			      gimp_display_shell_drop_pattern,
                              shell);
  gimp_dnd_viewable_dest_add (GTK_WIDGET (shell), GIMP_TYPE_BUFFER,
			      gimp_display_shell_drop_buffer,
                              shell);
  gimp_dnd_color_dest_add    (GTK_WIDGET (shell),
			      gimp_display_shell_drop_color,
                              shell);

  gimp_help_connect (GTK_WIDGET (shell), gimp_standard_help_func,
		     GIMP_HELP_IMAGE_WINDOW, NULL);
}

static void
gimp_display_shell_finalize (GObject *object)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (object);

  if (shell->options)
    g_object_unref (shell->options);

  if (shell->fullscreen_options)
    g_object_unref (shell->fullscreen_options);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_display_shell_destroy (GtkObject *object)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (object);

  if (shell->gdisp)
    gimp_display_shell_disconnect (shell);

  if (shell->menubar_factory)
    {
      g_object_unref (shell->menubar_factory);
      shell->menubar_factory = NULL;
    }

  shell->popup_factory = NULL;

  if (shell->qmask_factory)
    {
      g_object_unref (shell->qmask_factory);
      shell->qmask_factory = NULL;
    }

  if (shell->select)
    {
      gimp_display_shell_selection_free (shell->select);
      shell->select = NULL;
    }

  if (shell->filter_stack)
    gimp_display_shell_filter_set (shell, NULL);

  if (shell->filter_idle_id)
    {
      g_source_remove (shell->filter_idle_id);
      shell->filter_idle_id = 0;
    }

  if (shell->render_buf)
    {
      g_free (shell->render_buf);
      shell->render_buf = NULL;
    }

  if (shell->title_idle_id)
    {
      g_source_remove (shell->title_idle_id);
      shell->title_idle_id = 0;
    }

  if (shell->info_dialog)
    {
      info_window_free (shell->info_dialog);
      shell->info_dialog = NULL;
    }

  if (shell->nav_popup)
    {
      gtk_widget_destroy (shell->nav_popup);
      shell->nav_popup = NULL;
    }

  if (shell->grid_dialog)
    {
      gtk_widget_destroy (shell->grid_dialog);
      shell->grid_dialog = NULL;
    }

  shell->gdisp = NULL;

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_display_shell_screen_changed (GtkWidget *widget,
                                   GdkScreen *previous)
{
  GimpDisplayShell  *shell;
  GimpDisplayConfig *config;

  if (GTK_WIDGET_CLASS (parent_class)->screen_changed)
    GTK_WIDGET_CLASS (parent_class)->screen_changed (widget, previous);

  shell = GIMP_DISPLAY_SHELL (widget);
  config = GIMP_DISPLAY_CONFIG (shell->gdisp->gimage->gimp->config);

  if (GIMP_DISPLAY_CONFIG (config)->monitor_res_from_gdk)
    {
      gimp_get_screen_resolution (gtk_widget_get_screen (widget),
                                  &shell->monitor_xres,
                                  &shell->monitor_yres);
    }
  else
    {
      shell->monitor_xres = GIMP_DISPLAY_CONFIG (config)->monitor_xres;
      shell->monitor_yres = GIMP_DISPLAY_CONFIG (config)->monitor_yres;
    }
}

static gboolean
gimp_display_shell_delete_event (GtkWidget   *widget,
                                 GdkEventAny *aevent)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (widget);

  gimp_display_shell_close (shell, FALSE);

  return TRUE;
}

static void
gimp_display_shell_real_scaled (GimpDisplayShell *shell)
{
  GimpContext *user_context;

  gimp_display_shell_update_title (shell);

  /* update the <Image>/View/Zoom menu */
  gimp_item_factory_update (shell->menubar_factory, shell);

  user_context = gimp_get_user_context (shell->gdisp->gimage->gimp);

  if (shell->gdisp == gimp_context_get_display (user_context))
    gimp_item_factory_update (shell->popup_factory, shell);
}

GtkWidget *
gimp_display_shell_new (GimpDisplay     *gdisp,
                        gdouble          scale,
                        GimpMenuFactory *menu_factory,
                        GimpItemFactory *popup_factory)
{
  GimpDisplayShell  *shell;
  GimpDisplayConfig *config;
  GtkWidget         *main_vbox;
  GtkWidget         *disp_vbox;
  GtkWidget         *upper_hbox;
  GtkWidget         *right_vbox;
  GtkWidget         *lower_hbox;
  GtkWidget         *inner_table;
  GtkWidget         *image;
  GtkWidget         *menubar;
  GdkScreen         *screen;
  gint               image_width, image_height;
  gint               n_width, n_height;
  gint               s_width, s_height;
  gdouble            new_scale;

  g_return_val_if_fail (GIMP_IS_DISPLAY (gdisp), NULL);
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (GIMP_IS_ITEM_FACTORY (popup_factory), NULL);

  /*  the toplevel shell */
  shell = g_object_new (GIMP_TYPE_DISPLAY_SHELL, NULL);

  shell->gdisp = gdisp;
  shell->scale = scale;

  image_width  = gdisp->gimage->width;
  image_height = gdisp->gimage->height;

  config = GIMP_DISPLAY_CONFIG (gdisp->gimage->gimp->config);

  shell->dot_for_dot = config->default_dot_for_dot;

  gimp_config_sync (GIMP_CONFIG (config->default_view),
                    GIMP_CONFIG (shell->options), 0);
  gimp_config_sync (GIMP_CONFIG (config->default_fullscreen_view),
                    GIMP_CONFIG (shell->fullscreen_options), 0);

  /* adjust the initial scale -- so that window fits on screen the 75%
   * value is the same as in gimp_display_shell_shrink_wrap. It
   * probably should be a user-configurable option.
   */
  screen = gtk_widget_get_screen (GTK_WIDGET (shell));

  if (GIMP_DISPLAY_CONFIG (config)->monitor_res_from_gdk)
    {
      gimp_get_screen_resolution (screen,
                                  &shell->monitor_xres, &shell->monitor_yres);
    }
  else
    {
      shell->monitor_xres = GIMP_DISPLAY_CONFIG (config)->monitor_xres;
      shell->monitor_yres = GIMP_DISPLAY_CONFIG (config)->monitor_yres;
    }

  s_width  = gdk_screen_get_width (screen)  * 0.75;
  s_height = gdk_screen_get_height (screen) * 0.75;

  n_width  = SCALEX (shell, image_width);
  n_height = SCALEY (shell, image_height);

  if (config->initial_zoom_to_fit)
    {
      /*  Limit to the size of the screen...  */
      if (n_width > s_width || n_height > s_height)
        {
          new_scale = shell->scale * MIN (((gdouble) s_height) / n_height,
                                          ((gdouble) s_width) / n_width);

          new_scale = gimp_display_shell_scale_zoom_step (GIMP_ZOOM_OUT,
                                                          new_scale);

          /* since zooming out might skip a zoom step we zoom in again
           * and test if we are small enough. */
          shell->scale = gimp_display_shell_scale_zoom_step (GIMP_ZOOM_IN,
                                                             new_scale);

          if (SCALEX (shell, image_width) > s_width ||
              SCALEY (shell, image_height) > s_height)
            shell->scale = new_scale;

          n_width  = SCALEX (shell, image_width);
          n_height = SCALEY (shell, image_height);
        }
    }
  else
    {
      /* Set up size like above, but do not zoom to fit.
	 Useful when working on large images. */

      if (n_width > s_width)
	n_width = s_width;

      if (n_height > s_height)
	n_height = s_height;
    }


  shell->menubar_factory = gimp_menu_factory_menu_new (menu_factory,
                                                       "<Image>",
                                                       GTK_TYPE_MENU_BAR,
                                                       gdisp,
                                                       FALSE);

  shell->popup_factory = popup_factory;

  shell->qmask_factory = gimp_menu_factory_menu_new (menu_factory,
                                                     "<QMask>",
                                                     GTK_TYPE_MENU,
                                                     shell,
                                                     FALSE);

  /*  The accelerator table for images  */
  gtk_window_add_accel_group (GTK_WINDOW (shell),
                              GTK_ITEM_FACTORY (shell->menubar_factory)->accel_group);

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
   *     +-- menubar
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
   *     +-- statusbar
   */

  /*  first, set up the container hierarchy  *********************************/

  /*  the vbox containing all widgets  */

  main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (shell), main_vbox);

  menubar = GTK_ITEM_FACTORY (shell->menubar_factory)->widget;

  gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, FALSE, 0);

  if (shell->options->show_menubar)
    gtk_widget_show (menubar);

  /*  make sure we can activate accels even if the menubar is invisible
   *  (see http://bugzilla.gnome.org/show_bug.cgi?id=137151)
   */
  if (! gtk_check_version (2, 4, 0))
    g_signal_connect (menubar, "can-activate-accel",
                      G_CALLBACK (gtk_true),
                      NULL);

  /*  active display callback  */
  g_signal_connect (menubar, "button_press_event",
                    G_CALLBACK (gimp_display_shell_events),
                    shell);
  g_signal_connect (menubar, "button_release_event",
                    G_CALLBACK (gimp_display_shell_events),
                    shell);
  g_signal_connect (menubar, "key_press_event",
                    G_CALLBACK (gimp_display_shell_events),
                    shell);

  /*  another vbox for everything except the statusbar  */
  disp_vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (disp_vbox), 2);
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

  image = gtk_image_new_from_stock (GIMP_STOCK_MENU_RIGHT, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (shell->origin), image);
  gtk_widget_show (image);

  g_signal_connect (shell->origin, "button_press_event",
                    G_CALLBACK (gimp_display_shell_origin_button_press),
                    shell);

  gimp_help_set_help_data (shell->origin, NULL,
                           GIMP_HELP_IMAGE_WINDOW_ORIGIN_BUTTON);

  shell->canvas = gimp_canvas_new ();

  shell->select = gimp_display_shell_selection_new (shell);

  /*  the horizontal ruler  */
  shell->hrule = gtk_hruler_new ();
  gtk_widget_set_events (GTK_WIDGET (shell->hrule),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  g_signal_connect_swapped (shell->canvas, "motion_notify_event",
			    G_CALLBACK (GTK_WIDGET_GET_CLASS (shell->hrule)->motion_notify_event),
			    shell->hrule);
  g_signal_connect (shell->hrule, "button_press_event",
		    G_CALLBACK (gimp_display_shell_hruler_button_press),
		    shell);

  gimp_help_set_help_data (shell->hrule, NULL, GIMP_HELP_IMAGE_WINDOW_RULER);

  /*  the vertical ruler  */
  shell->vrule = gtk_vruler_new ();
  gtk_widget_set_events (GTK_WIDGET (shell->vrule),
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  g_signal_connect_swapped (shell->canvas, "motion_notify_event",
			    G_CALLBACK (GTK_WIDGET_GET_CLASS (shell->vrule)->motion_notify_event),
			    shell->vrule);
  g_signal_connect (shell->vrule, "button_press_event",
		    G_CALLBACK (gimp_display_shell_vruler_button_press),
		    shell);

  gimp_help_set_help_data (shell->vrule, NULL, GIMP_HELP_IMAGE_WINDOW_RULER);

  /*  the canvas  */
  gtk_widget_set_size_request (shell->canvas, n_width, n_height);
  gtk_widget_set_events (shell->canvas, GIMP_DISPLAY_SHELL_CANVAS_EVENT_MASK);
  gtk_widget_set_extension_events (shell->canvas, GDK_EXTENSION_EVENTS_ALL);
  GTK_WIDGET_SET_FLAGS (shell->canvas, GTK_CAN_FOCUS);

  g_signal_connect (shell->canvas, "realize",
                    G_CALLBACK (gimp_display_shell_canvas_realize),
                    shell);

  /*  set the active display before doing any other canvas event processing  */
  g_signal_connect (shell->canvas, "event",
		    G_CALLBACK (gimp_display_shell_events),
		    shell);

  g_signal_connect (shell->canvas, "expose_event",
		    G_CALLBACK (gimp_display_shell_canvas_expose),
		    shell);
  g_signal_connect (shell->canvas, "configure_event",
		    G_CALLBACK (gimp_display_shell_canvas_configure),
		    shell);
  g_signal_connect (shell->canvas, "event",
		    G_CALLBACK (gimp_display_shell_canvas_tool_events),
		    shell);

  /*  create the contents of the right_vbox  *********************************/
  shell->padding_button = gimp_color_panel_new (_("Set Canvas Padding Color"),
                                                &shell->options->padding_color,
                                                GIMP_COLOR_AREA_FLAT,
                                                15, 15);
  GTK_WIDGET_UNSET_FLAGS (shell->padding_button, GTK_CAN_FOCUS);
  gimp_color_panel_set_context (GIMP_COLOR_PANEL (shell->padding_button),
                                gimp_get_user_context (gdisp->gimage->gimp));

  gimp_help_set_help_data (shell->padding_button,
                           _("Set canvas padding color"),
                           GIMP_HELP_IMAGE_WINDOW_PADDING_BUTTON);

  g_signal_connect (shell->padding_button, "button_press_event",
                    G_CALLBACK (gimp_display_shell_color_button_press),
                    shell);
  g_signal_connect (shell->padding_button, "color_changed",
                    G_CALLBACK (gimp_display_shell_color_button_changed),
                    shell);

  {
    static GtkItemFactoryEntry menu_items[] =
    {
      { N_("/From _Theme"), NULL,
        gimp_display_shell_color_button_menu_callback,
        GIMP_CANVAS_PADDING_MODE_DEFAULT, NULL },
      { N_("/_Light Check Color"), NULL,
        gimp_display_shell_color_button_menu_callback,
        GIMP_CANVAS_PADDING_MODE_LIGHT_CHECK, NULL },
      { N_("/_Dark Check Color"), NULL,
        gimp_display_shell_color_button_menu_callback,
        GIMP_CANVAS_PADDING_MODE_DARK_CHECK, NULL },

      { "/---", NULL, NULL, 0, "<Separator>"},

      { N_("/Select _Custom Color..."), NULL,
        gimp_display_shell_color_button_menu_callback,
        GIMP_CANVAS_PADDING_MODE_CUSTOM, "<StockItem>",
        GTK_STOCK_SELECT_COLOR },
      { N_("/As in _Preferences"), NULL,
        gimp_display_shell_color_button_menu_callback,
        0xffff, "<StockItem>",
        GIMP_STOCK_RESET }
    };

    GtkItemFactory *item_factory;

    item_factory =
      GTK_ITEM_FACTORY (GIMP_COLOR_BUTTON (shell->padding_button)->popup_menu);

    gtk_item_factory_create_items (item_factory,
                                   G_N_ELEMENTS (menu_items), menu_items,
                                   shell);
  }

  /*  create the contents of the lower_hbox  *********************************/

  /*  the qmask button  */
  shell->qmask = gtk_check_button_new ();
  gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (shell->qmask), FALSE);
  gtk_widget_set_size_request (GTK_WIDGET (shell->qmask), 16, 16);
  GTK_WIDGET_UNSET_FLAGS (shell->qmask, GTK_CAN_FOCUS);

  image = gtk_image_new_from_stock (GIMP_STOCK_QMASK_OFF, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (shell->qmask), image);
  gtk_widget_show (image);

  gimp_help_set_help_data (shell->qmask,
                           _("Toggle QuickMask"),
                           GIMP_HELP_IMAGE_WINDOW_QMASK_BUTTON);

  g_signal_connect (shell->qmask, "toggled",
		    G_CALLBACK (gimp_display_shell_qmask_toggled),
		    shell);
  g_signal_connect (shell->qmask, "button_press_event",
		    G_CALLBACK (gimp_display_shell_qmask_button_press),
		    shell);

  /*  the navigation window button  */
  shell->nav_ebox = gtk_event_box_new ();

  image = gtk_image_new_from_stock (GIMP_STOCK_NAVIGATION, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (shell->nav_ebox), image);
  gtk_widget_show (image);

  g_signal_connect (shell->nav_ebox, "button_press_event",
		    G_CALLBACK (gimp_display_shell_nav_button_press),
		    shell);

  gimp_help_set_help_data (shell->nav_ebox, NULL,
                           GIMP_HELP_IMAGE_WINDOW_NAV_BUTTON);

  /*  create the contents of the status area *********************************/

  /*  the statusbar  */
  shell->statusbar = gimp_statusbar_new (shell);
  gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (shell->statusbar), FALSE);
  gimp_help_set_help_data (shell->statusbar, NULL,
                           GIMP_HELP_IMAGE_WINDOW_STATUS_BAR);

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
  gtk_box_pack_start (GTK_BOX (lower_hbox), shell->nav_ebox, FALSE, FALSE, 0);

  gtk_box_pack_end (GTK_BOX (main_vbox), shell->statusbar, FALSE, FALSE, 0);

  /*  show everything  *******************************************************/

  if (shell->options->show_rulers)
    {
      gtk_widget_show (shell->origin);
      gtk_widget_show (shell->hrule);
      gtk_widget_show (shell->vrule);
    }

  gtk_widget_show (GTK_WIDGET (shell->canvas));

  if (shell->options->show_scrollbars)
    {
      gtk_widget_show (shell->vsb);
      gtk_widget_show (shell->hsb);
      gtk_widget_show (shell->padding_button);
      gtk_widget_show (shell->qmask);
      gtk_widget_show (shell->nav_ebox);
    }

  if (shell->options->show_statusbar)
    gtk_widget_show (shell->statusbar);

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
  if (! kill_it               &&
      gimage->disp_count == 1 &&
      gimage->dirty           &&
      GIMP_DISPLAY_CONFIG (gimage->gimp->config)->confirm_on_close)
    {
      gimp_display_shell_close_warning_dialog (shell, gimage);
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

  gimp_statusbar_resize_cursor (GIMP_STATUSBAR (shell->statusbar));
  gimp_display_shell_scale_setup (shell);
  gimp_display_shell_expose_full (shell);

  g_signal_emit (shell, display_shell_signals[RECONNECT], 0);
}

void
gimp_display_shell_scaled (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  g_signal_emit (shell, display_shell_signals[SCALED], 0);
}

void
gimp_display_shell_scrolled (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  g_signal_emit (shell, display_shell_signals[SCROLLED], 0);
}

gboolean
gimp_display_shell_snap_coords (GimpDisplayShell *shell,
                                GimpCoords       *coords,
                                GimpCoords       *snapped_coords,
                                gint              snap_offset_x,
                                gint              snap_offset_y,
                                gint              snap_width,
                                gint              snap_height)
{
  gboolean snap_to_guides = FALSE;
  gboolean snap_to_grid   = FALSE;
  gboolean snapped        = FALSE;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);
  g_return_val_if_fail (snapped_coords != NULL, FALSE);

  *snapped_coords = *coords;

  if (shell->snap_to_guides &&
      shell->gdisp->gimage->guides)
    {
      snap_to_guides = TRUE;
    }

  if (gimp_display_shell_get_snap_to_grid (shell) &&
      shell->gdisp->gimage->grid)
    {
      snap_to_grid = TRUE;
    }

  if (snap_to_guides || snap_to_grid)
    {
      gdouble tx, ty;
      gint    snap_distance;

      snap_distance =
        GIMP_GUI_CONFIG (shell->gdisp->gimage->gimp->config)->snap_distance;

      if (snap_width > 0 && snap_height > 0)
        {
          snapped = gimp_image_snap_rectangle (shell->gdisp->gimage,
                                               coords->x + snap_offset_x,
                                               coords->y + snap_offset_y,
                                               coords->x + snap_offset_x +
                                               snap_width,
                                               coords->y + snap_offset_y +
                                               snap_height,
                                               &tx,
                                               &ty,
                                               FUNSCALEX (shell, snap_distance),
                                               FUNSCALEY (shell, snap_distance),
                                               snap_to_guides,
                                               snap_to_grid);
        }
      else
        {
          snapped = gimp_image_snap_point (shell->gdisp->gimage,
                                           coords->x + snap_offset_x,
                                           coords->y + snap_offset_y,
                                           &tx,
                                           &ty,
                                           FUNSCALEX (shell, snap_distance),
                                           FUNSCALEY (shell, snap_distance),
                                           snap_to_guides,
                                           snap_to_grid);
        }

      if (snapped)
        {
          snapped_coords->x = tx - snap_offset_x;
          snapped_coords->y = ty - snap_offset_y;
        }
    }

  return snapped;
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
      gimp_item_offsets (GIMP_ITEM (layer), &off_x, &off_y);

      if (! gimp_channel_bounds (gimp_image_get_mask (shell->gdisp->gimage),
				 x1, y1, x2, y2))
	{
	  *x1 = off_x;
	  *y1 = off_y;
	  *x2 = off_x + gimp_item_width  (GIMP_ITEM (layer));
	  *y2 = off_y + gimp_item_height (GIMP_ITEM (layer));
	}
      else
	{
	  *x1 = MIN (off_x, *x1);
	  *y1 = MIN (off_y, *y1);
	  *x2 = MAX (off_x + gimp_item_width  (GIMP_ITEM (layer)), *x2);
	  *y2 = MAX (off_y + gimp_item_height (GIMP_ITEM (layer)), *y2);
	}
    }
  else if (! gimp_channel_bounds (gimp_image_get_mask (shell->gdisp->gimage),
				  x1, y1, x2, y2))
    {
      return FALSE;
    }

  gimp_display_shell_transform_xy (shell, *x1, *y1, x1, y1, FALSE);
  gimp_display_shell_transform_xy (shell, *x2, *y2, x2, y2, FALSE);

  /*  Make sure the extents are within bounds  */
  *x1 = CLAMP (*x1, 0, shell->disp_width);
  *y1 = CLAMP (*y1, 0, shell->disp_height);
  *x2 = CLAMP (*x2, 0, shell->disp_width);
  *y2 = CLAMP (*y2, 0, shell->disp_height);

  return TRUE;
}

void
gimp_display_shell_expose_area (GimpDisplayShell *shell,
                                gint              x,
                                gint              y,
                                gint              w,
                                gint              h)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gtk_widget_queue_draw_area (shell->canvas, x, y, w, h);
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

  gimp_display_shell_transform_xy (shell,
                                   guide->position,
                                   guide->position,
                                   &x, &y,
                                   FALSE);

  switch (guide->orientation)
    {
    case GIMP_ORIENTATION_HORIZONTAL:
      gimp_display_shell_expose_area (shell, 0, y, shell->disp_width, 1);
      break;

    case GIMP_ORIENTATION_VERTICAL:
      gimp_display_shell_expose_area (shell, x, 0, 1, shell->disp_height);
      break;

    default:
      break;
    }
}

void
gimp_display_shell_expose_full (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gtk_widget_queue_draw (shell->canvas);
}

void
gimp_display_shell_flush (GimpDisplayShell *shell,
                          gboolean          now)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_update_title (shell);

  if (now)
    gdk_window_process_updates (shell->canvas->window, FALSE);
}

void
gimp_display_shell_pause (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  shell->paused_count++;

  if (shell->paused_count == 1)
    {
      /*  pause the currently active tool  */
      tool_manager_control_active (shell->gdisp->gimage->gimp, PAUSE,
                                   shell->gdisp);

      gimp_display_shell_draw_vectors (shell);
    }
}

void
gimp_display_shell_resume (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (shell->paused_count > 0);

  shell->paused_count--;

  if (shell->paused_count == 0)
    {
      gimp_display_shell_draw_vectors (shell);

      /* start the currently active tool */
      tool_manager_control_active (shell->gdisp->gimage->gimp, RESUME,
                                   shell->gdisp);
    }
}

void
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

  pixbuf =
    gimp_viewable_get_new_preview_pixbuf (GIMP_VIEWABLE (shell->gdisp->gimage),
                                          width, height);

  gtk_window_set_icon (GTK_WINDOW (shell), pixbuf);

  if (pixbuf)
    g_object_unref (pixbuf);
}

void
gimp_display_shell_shrink_wrap (GimpDisplayShell *shell)
{
  GtkWidget    *widget;
  GdkScreen    *screen;
  GdkRectangle  rect;
  gint          monitor;
  gint          disp_width, disp_height;
  gint          width, height;
  gint          max_auto_width, max_auto_height;
  gint          border_x, border_y;
  gboolean      resize = FALSE;

  g_return_if_fail (GTK_WIDGET_REALIZED (shell));

  widget = GTK_WIDGET (shell);
  screen = gtk_widget_get_screen (widget);

  monitor = gdk_screen_get_monitor_at_window (screen, widget->window);
  gdk_screen_get_monitor_geometry (screen, monitor, &rect);

  width  = SCALEX (shell, shell->gdisp->gimage->width);
  height = SCALEY (shell, shell->gdisp->gimage->height);

  disp_width  = shell->disp_width;
  disp_height = shell->disp_height;

  border_x = widget->allocation.width  - disp_width;
  border_y = widget->allocation.height - disp_height;

  max_auto_width  = (rect.width  - border_x) * 0.75;
  max_auto_height = (rect.height - border_y) * 0.75;

  /* If one of the display dimensions has changed and one of the
   * dimensions fits inside the screen
   */
  if (((width + border_x) < rect.width || (height + border_y) < rect.height) &&
      (width != disp_width || height != disp_height))
    {
      width  = ((width  + border_x) < rect.width)  ? width  : max_auto_width;
      height = ((height + border_y) < rect.height) ? height : max_auto_height;

      resize = TRUE;
    }

  /*  If the projected dimension is greater than current, but less than
   *  3/4 of the screen size, expand automagically
   */
  else if ((width > disp_width || height > disp_height) &&
	   (disp_width < max_auto_width || disp_height < max_auto_height))
    {
      width  = MIN (max_auto_width,  width);
      height = MIN (max_auto_height, height);

      resize = TRUE;
    }

  if (resize)
    {
      if (width < shell->statusbar->requisition.width)
        width = shell->statusbar->requisition.width;

      gtk_window_resize (GTK_WINDOW (shell),
                         width  + border_x,
                         height + border_y);
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

static void
gimp_display_shell_close_warning_dialog (GimpDisplayShell *shell,
                                         GimpImage        *gimage)
{
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *image;
  GtkWidget *label;
  PangoAttrList  *attrs;
  PangoAttribute *attr;
  gchar     *name;
  gchar     *title;
  gchar     *message;

  if (shell->warning_dialog)
    {
      gtk_window_present (GTK_WINDOW (shell->warning_dialog));
      return;
    }

  name = file_utils_uri_to_utf8_basename (gimp_image_get_uri (gimage));

  title = g_strdup_printf (_("Close %s?"), name);

  shell->warning_dialog =
    dialog = gimp_dialog_new (title,
                              "gimp-display-shell-close",
                              GTK_WIDGET (shell), 0,
                              gimp_standard_help_func,
                              GIMP_HELP_FILE_CLOSE_CONFIRM,

                              GTK_STOCK_CANCEL,      GTK_RESPONSE_CANCEL,
                              _("_Discard changes"), GTK_RESPONSE_OK,

                              NULL);

  g_free (title);

  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &shell->warning_dialog);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gimp_display_shell_close_warning_response),
                    shell);

  hbox = gtk_hbox_new (FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_stock (GIMP_STOCK_WARNING, GTK_ICON_SIZE_DIALOG);
  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  attrs = pango_attr_list_new ();

  attr = pango_attr_scale_new (PANGO_SCALE_LARGE);
  attr->start_index = 0;
  attr->end_index   = -1;
  pango_attr_list_insert (attrs, attr);

  attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
  attr->start_index = 0;
  attr->end_index   = -1;
  pango_attr_list_insert (attrs, attr);

  message = g_strdup_printf (_("Changes were made to '%s'."), name);

  label = gtk_label_new (message);

  g_free (message);
  g_free (name);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_attributes (GTK_LABEL (label), attrs);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  pango_attr_list_unref (attrs);

  label = gtk_label_new (_("Unsaved changes will be lost."));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (dialog);
}

static void
gimp_display_shell_close_warning_response (GtkWidget        *widget,
                                           gint              response_id,
                                           GimpDisplayShell *shell)
{
  gtk_widget_destroy (GTK_WIDGET (shell->warning_dialog));

  if (response_id == GTK_RESPONSE_OK)
    gimp_display_delete (shell->gdisp);
}

