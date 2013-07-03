/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"
#include "tools/tools-types.h"

#include "config/gimpcoreconfig.h"
#include "config/gimpdisplayconfig.h"
#include "config/gimpdisplayoptions.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-grid.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-snap.h"
#include "core/gimpprojection.h"
#include "core/gimpmarshal.h"
#include "core/gimptemplate.h"

#include "widgets/gimpdevices.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"

#include "tools/tool_manager.h"

#include "gimpcanvas.h"
#include "gimpcanvaslayerboundary.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-callbacks.h"
#include "gimpdisplayshell-cursor.h"
#include "gimpdisplayshell-dnd.h"
#include "gimpdisplayshell-expose.h"
#include "gimpdisplayshell-filter.h"
#include "gimpdisplayshell-handlers.h"
#include "gimpdisplayshell-items.h"
#include "gimpdisplayshell-progress.h"
#include "gimpdisplayshell-render.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-selection.h"
#include "gimpdisplayshell-title.h"
#include "gimpdisplayshell-tool-events.h"
#include "gimpdisplayshell-transform.h"
#include "gimpimagewindow.h"
#include "gimpmotionbuffer.h"
#include "gimpstatusbar.h"

#include "about.h"
#include "gimp-log.h"

#include "gimp-intl.h"

enum
{
  PROP_0,
  PROP_POPUP_MANAGER,
  PROP_DISPLAY,
  PROP_UNIT,
  PROP_TITLE,
  PROP_STATUS,
  PROP_ICON
};

enum
{
  SCALED,
  SCROLLED,
  RECONNECT,
  LAST_SIGNAL
};


typedef struct _GimpDisplayShellOverlay GimpDisplayShellOverlay;

struct _GimpDisplayShellOverlay
{
  gdouble          image_x;
  gdouble          image_y;
  GimpHandleAnchor anchor;
  gint             spacing_x;
  gint             spacing_y;
};


/*  local function prototypes  */

static void      gimp_color_managed_iface_init     (GimpColorManagedInterface *iface);

static void      gimp_display_shell_constructed    (GObject          *object);
static void      gimp_display_shell_dispose        (GObject          *object);
static void      gimp_display_shell_finalize       (GObject          *object);
static void      gimp_display_shell_set_property   (GObject          *object,
                                                    guint             property_id,
                                                    const GValue     *value,
                                                    GParamSpec       *pspec);
static void      gimp_display_shell_get_property   (GObject          *object,
                                                    guint             property_id,
                                                    GValue           *value,
                                                    GParamSpec       *pspec);

static void      gimp_display_shell_unrealize      (GtkWidget        *widget);
static void      gimp_display_shell_screen_changed (GtkWidget        *widget,
                                                    GdkScreen        *previous);
static gboolean  gimp_display_shell_popup_menu     (GtkWidget        *widget);

static void      gimp_display_shell_real_scaled    (GimpDisplayShell *shell);

static const guint8 * gimp_display_shell_get_icc_profile
                                                   (GimpColorManaged *managed,
                                                    gsize            *len);

static void      gimp_display_shell_menu_position  (GtkMenu          *menu,
                                                    gint             *x,
                                                    gint             *y,
                                                    gpointer          data);
static void      gimp_display_shell_zoom_button_callback
                                                   (GimpDisplayShell *shell,
                                                    GtkWidget        *zoom_button);
static void      gimp_display_shell_sync_config    (GimpDisplayShell  *shell,
                                                    GimpDisplayConfig *config);

static void      gimp_display_shell_remove_overlay (GtkWidget        *canvas,
                                                    GtkWidget        *child,
                                                    GimpDisplayShell *shell);
static void   gimp_display_shell_transform_overlay (GimpDisplayShell *shell,
                                                    GtkWidget        *child,
                                                    gdouble          *x,
                                                    gdouble          *y);


G_DEFINE_TYPE_WITH_CODE (GimpDisplayShell, gimp_display_shell,
                         GTK_TYPE_BOX,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PROGRESS,
                                                gimp_display_shell_progress_iface_init)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_COLOR_MANAGED,
                                                gimp_color_managed_iface_init))


#define parent_class gimp_display_shell_parent_class

static guint display_shell_signals[LAST_SIGNAL] = { 0 };


static const gchar display_rc_style[] =
  "style \"check-button-style\"\n"
  "{\n"
  "  GtkToggleButton::child-displacement-x = 0\n"
  "  GtkToggleButton::child-displacement-y = 0\n"
  "}\n"
  "widget \"*\" style \"check-button-style\"";

static void
gimp_display_shell_class_init (GimpDisplayShellClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

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

  object_class->constructed        = gimp_display_shell_constructed;
  object_class->dispose            = gimp_display_shell_dispose;
  object_class->finalize           = gimp_display_shell_finalize;
  object_class->set_property       = gimp_display_shell_set_property;
  object_class->get_property       = gimp_display_shell_get_property;

  widget_class->unrealize          = gimp_display_shell_unrealize;
  widget_class->screen_changed     = gimp_display_shell_screen_changed;
  widget_class->popup_menu         = gimp_display_shell_popup_menu;

  klass->scaled                    = gimp_display_shell_real_scaled;
  klass->scrolled                  = NULL;
  klass->reconnect                 = NULL;

  g_object_class_install_property (object_class, PROP_POPUP_MANAGER,
                                   g_param_spec_object ("popup-manager",
                                                        NULL, NULL,
                                                        GIMP_TYPE_UI_MANAGER,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DISPLAY,
                                   g_param_spec_object ("display", NULL, NULL,
                                                        GIMP_TYPE_DISPLAY,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_UNIT,
                                   gimp_param_spec_unit ("unit", NULL, NULL,
                                                         TRUE, FALSE,
                                                         GIMP_UNIT_PIXEL,
                                                         GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_TITLE,
                                   g_param_spec_string ("title", NULL, NULL,
                                                        GIMP_NAME,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_STATUS,
                                   g_param_spec_string ("status", NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_ICON,
                                   g_param_spec_object ("icon", NULL, NULL,
                                                        GDK_TYPE_PIXBUF,
                                                        GIMP_PARAM_READWRITE));

  gtk_rc_parse_string (display_rc_style);
}

static void
gimp_color_managed_iface_init (GimpColorManagedInterface *iface)
{
  iface->get_icc_profile = gimp_display_shell_get_icc_profile;
}

static void
gimp_display_shell_init (GimpDisplayShell *shell)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (shell),
                                  GTK_ORIENTATION_VERTICAL);

  shell->options            = g_object_new (GIMP_TYPE_DISPLAY_OPTIONS, NULL);
  shell->fullscreen_options = g_object_new (GIMP_TYPE_DISPLAY_OPTIONS_FULLSCREEN, NULL);
  shell->no_image_options   = g_object_new (GIMP_TYPE_DISPLAY_OPTIONS_NO_IMAGE, NULL);

  shell->zoom        = gimp_zoom_model_new ();
  shell->dot_for_dot = TRUE;
  shell->scale_x     = 1.0;
  shell->scale_y     = 1.0;
  shell->x_dest_inc  = 1;
  shell->y_dest_inc  = 1;
  shell->x_src_dec   = 1;
  shell->y_src_dec   = 1;

  shell->render_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                      GIMP_DISPLAY_RENDER_BUF_WIDTH,
                                                      GIMP_DISPLAY_RENDER_BUF_HEIGHT);

  gimp_display_shell_items_init (shell);

  shell->icon_size  = 32;

  shell->cursor_format     = GIMP_CURSOR_FORMAT_BITMAP;
  shell->cursor_handedness = GIMP_HANDEDNESS_RIGHT;
  shell->current_cursor    = (GimpCursorType) -1;
  shell->tool_cursor       = GIMP_TOOL_CURSOR_NONE;
  shell->cursor_modifier   = GIMP_CURSOR_MODIFIER_NONE;
  shell->override_cursor   = (GimpCursorType) -1;

  shell->motion_buffer   = gimp_motion_buffer_new ();

  g_signal_connect (shell->motion_buffer, "stroke",
                    G_CALLBACK (gimp_display_shell_buffer_stroke),
                    shell);
  g_signal_connect (shell->motion_buffer, "hover",
                    G_CALLBACK (gimp_display_shell_buffer_hover),
                    shell);

  shell->zoom_focus_pointer_queue = g_queue_new ();

  gtk_widget_set_events (GTK_WIDGET (shell), (GDK_POINTER_MOTION_MASK    |
                                              GDK_BUTTON_PRESS_MASK      |
                                              GDK_KEY_PRESS_MASK         |
                                              GDK_KEY_RELEASE_MASK       |
                                              GDK_FOCUS_CHANGE_MASK      |
                                              GDK_VISIBILITY_NOTIFY_MASK |
                                              GDK_SCROLL_MASK));

  /*  zoom model callback  */
  g_signal_connect_swapped (shell->zoom, "zoomed",
                            G_CALLBACK (gimp_display_shell_scale_changed),
                            shell);

  /*  active display callback  */
  g_signal_connect (shell, "button-press-event",
                    G_CALLBACK (gimp_display_shell_events),
                    shell);
  g_signal_connect (shell, "button-release-event",
                    G_CALLBACK (gimp_display_shell_events),
                    shell);
  g_signal_connect (shell, "key-press-event",
                    G_CALLBACK (gimp_display_shell_events),
                    shell);

  gimp_help_connect (GTK_WIDGET (shell), gimp_standard_help_func,
                     GIMP_HELP_IMAGE_WINDOW, NULL);
}

static void
gimp_display_shell_constructed (GObject *object)
{
  GimpDisplayShell      *shell = GIMP_DISPLAY_SHELL (object);
  GimpDisplayConfig     *config;
  GimpImage             *image;
  GimpColorDisplayStack *filter;
  GtkWidget             *upper_hbox;
  GtkWidget             *right_vbox;
  GtkWidget             *lower_hbox;
  GtkWidget             *inner_table;
  GtkWidget             *gtk_image;
  GdkScreen             *screen;
  GtkAction             *action;
  gint                   image_width;
  gint                   image_height;
  gint                   shell_width;
  gint                   shell_height;

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_UI_MANAGER (shell->popup_manager));
  g_assert (GIMP_IS_DISPLAY (shell->display));

  config = shell->display->config;
  image  = gimp_display_get_image (shell->display);

  if (image)
    {
      image_width  = gimp_image_get_width  (image);
      image_height = gimp_image_get_height (image);
    }
  else
    {
      /* These values are arbitrary. The width is determined by the
       * menubar and the height is chosen to give a window aspect
       * ratio of roughly 3:1 (as requested by the UI team).
       */
      image_width  = GIMP_DEFAULT_IMAGE_WIDTH;
      image_height = GIMP_DEFAULT_IMAGE_HEIGHT / 3;
    }

  shell->dot_for_dot = config->default_dot_for_dot;

  screen = gtk_widget_get_screen (GTK_WIDGET (shell));

  if (config->monitor_res_from_gdk)
    {
      gimp_get_screen_resolution (screen,
                                  &shell->monitor_xres, &shell->monitor_yres);
    }
  else
    {
      shell->monitor_xres = config->monitor_xres;
      shell->monitor_yres = config->monitor_yres;
    }

  /* adjust the initial scale -- so that window fits on screen. */
  if (image)
    {
      gimp_display_shell_set_initial_scale (shell, 1.0, //scale,
                                            &shell_width, &shell_height);
    }
  else
    {
      shell_width  = -1;
      shell_height = image_height;
    }

  gimp_display_shell_sync_config (shell, config);

  /*  GtkTable widgets are not able to shrink a row/column correctly if
   *  widgets are attached with GTK_EXPAND even if those widgets have
   *  other rows/columns in their rowspan/colspan where they could
   *  nicely expand without disturbing the row/column which is supposed
   *  to shrink. --Mitch
   *
   *  Changed the packing to use hboxes and vboxes which behave nicer:
   *
   *  shell
   *     |
   *     +-- upper_hbox
   *     |      |
   *     |      +-- inner_table
   *     |      |      |
   *     |      |      +-- origin
   *     |      |      +-- hruler
   *     |      |      +-- vruler
   *     |      |      +-- canvas
   *     |      |
   *     |      +-- right_vbox
   *     |             |
   *     |             +-- zoom_on_resize_button
   *     |             +-- vscrollbar
   *     |
   *     +-- lower_hbox
   *     |      |
   *     |      +-- quick_mask
   *     |      +-- hscrollbar
   *     |      +-- navbutton
   *     |
   *     +-- statusbar
   */

  /*  first, set up the container hierarchy  *********************************/

  /*  a hbox for the inner_table and the vertical scrollbar  */
  upper_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (shell), upper_hbox, TRUE, TRUE, 0);
  gtk_widget_show (upper_hbox);

  /*  the table containing origin, rulers and the canvas  */
  inner_table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (inner_table), 0, 0);
  gtk_table_set_row_spacing (GTK_TABLE (inner_table), 0, 0);
  gtk_box_pack_start (GTK_BOX (upper_hbox), inner_table, TRUE, TRUE, 0);
  gtk_widget_show (inner_table);

  /*  the vbox containing the color button and the vertical scrollbar  */
  right_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
  gtk_box_pack_start (GTK_BOX (upper_hbox), right_vbox, FALSE, FALSE, 0);
  gtk_widget_show (right_vbox);

  /*  the hbox containing the quickmask button, vertical scrollbar and
   *  the navigation button
   */
  lower_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_box_pack_start (GTK_BOX (shell), lower_hbox, FALSE, FALSE, 0);
  gtk_widget_show (lower_hbox);

  /*  create the scrollbars  *************************************************/

  /*  the horizontal scrollbar  */
  shell->hsbdata = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, image_width,
                                                       1, 1, image_width));
  shell->hsb = gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL, shell->hsbdata);
  gtk_widget_set_can_focus (shell->hsb, FALSE);

  /*  the vertical scrollbar  */
  shell->vsbdata = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, image_height,
                                                       1, 1, image_height));
  shell->vsb = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, shell->vsbdata);
  gtk_widget_set_can_focus (shell->vsb, FALSE);

  /*  create the contents of the inner_table  ********************************/

  /*  the menu popup button  */
  shell->origin = gtk_event_box_new ();

  gtk_image = gtk_image_new_from_stock (GIMP_STOCK_MENU_RIGHT,
                                        GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (shell->origin), gtk_image);
  gtk_widget_show (gtk_image);

  g_signal_connect (shell->origin, "button-press-event",
                    G_CALLBACK (gimp_display_shell_origin_button_press),
                    shell);

  gimp_help_set_help_data (shell->origin,
                           _("Access the image menu"),
                           GIMP_HELP_IMAGE_WINDOW_ORIGIN);

  shell->canvas = gimp_canvas_new (config);
  gtk_widget_set_size_request (shell->canvas, shell_width, shell_height);
  gtk_container_set_border_width (GTK_CONTAINER (shell->canvas), 10);

  g_signal_connect (shell->canvas, "remove",
                    G_CALLBACK (gimp_display_shell_remove_overlay),
                    shell);

  gimp_display_shell_dnd_init (shell);
  gimp_display_shell_selection_init (shell);

  /*  the horizontal ruler  */
  shell->hrule = gimp_ruler_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_events (GTK_WIDGET (shell->hrule),
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  gimp_ruler_add_track_widget (GIMP_RULER (shell->hrule), shell->canvas);
  g_signal_connect (shell->hrule, "button-press-event",
                    G_CALLBACK (gimp_display_shell_hruler_button_press),
                    shell);

  gimp_help_set_help_data (shell->hrule, NULL, GIMP_HELP_IMAGE_WINDOW_RULER);

  /*  the vertical ruler  */
  shell->vrule = gimp_ruler_new (GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_events (GTK_WIDGET (shell->vrule),
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

  gimp_ruler_add_track_widget (GIMP_RULER (shell->vrule), shell->canvas);
  g_signal_connect (shell->vrule, "button-press-event",
                    G_CALLBACK (gimp_display_shell_vruler_button_press),
                    shell);

  /*  set the rulers as track widgets for each other, so we don't end up
   *  with one ruler wrongly being stuck a few pixels off while we are
   *  hovering the other
   */
  gimp_ruler_add_track_widget (GIMP_RULER (shell->hrule), shell->vrule);
  gimp_ruler_add_track_widget (GIMP_RULER (shell->vrule), shell->hrule);

  gimp_help_set_help_data (shell->vrule, NULL, GIMP_HELP_IMAGE_WINDOW_RULER);

  gimp_devices_add_widget (shell->display->gimp, shell->hrule);
  gimp_devices_add_widget (shell->display->gimp, shell->vrule);

  g_signal_connect (shell->canvas, "realize",
                    G_CALLBACK (gimp_display_shell_canvas_realize),
                    shell);
  g_signal_connect (shell->canvas, "size-allocate",
                    G_CALLBACK (gimp_display_shell_canvas_size_allocate),
                    shell);
  g_signal_connect (shell->canvas, "expose-event",
                    G_CALLBACK (gimp_display_shell_canvas_expose),
                    shell);
  g_signal_connect_after (shell->canvas, "expose-event",
                          G_CALLBACK (gimp_display_shell_canvas_expose_after),
                          shell);

  g_signal_connect (shell->canvas, "enter-notify-event",
                    G_CALLBACK (gimp_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "leave-notify-event",
                    G_CALLBACK (gimp_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "proximity-in-event",
                    G_CALLBACK (gimp_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "proximity-out-event",
                    G_CALLBACK (gimp_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "focus-in-event",
                    G_CALLBACK (gimp_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "focus-out-event",
                    G_CALLBACK (gimp_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "button-press-event",
                    G_CALLBACK (gimp_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "button-release-event",
                    G_CALLBACK (gimp_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "scroll-event",
                    G_CALLBACK (gimp_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "motion-notify-event",
                    G_CALLBACK (gimp_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "key-press-event",
                    G_CALLBACK (gimp_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "key-release-event",
                    G_CALLBACK (gimp_display_shell_canvas_tool_events),
                    shell);

  /*  create the contents of the right_vbox  *********************************/

  shell->zoom_button = g_object_new (GTK_TYPE_CHECK_BUTTON,
                                     "draw-indicator", FALSE,
                                     "relief",         GTK_RELIEF_NONE,
                                     "width-request",  18,
                                     "height-request", 18,
                                     NULL);
  gtk_widget_set_can_focus (shell->zoom_button, FALSE);

  gtk_image = gtk_image_new_from_stock (GIMP_STOCK_ZOOM_FOLLOW_WINDOW,
                                        GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (shell->zoom_button), gtk_image);
  gtk_widget_show (gtk_image);

  gimp_help_set_help_data (shell->zoom_button,
                           _("Zoom image when window size changes"),
                           GIMP_HELP_IMAGE_WINDOW_ZOOM_FOLLOW_BUTTON);

  g_signal_connect_swapped (shell->zoom_button, "toggled",
                            G_CALLBACK (gimp_display_shell_zoom_button_callback),
                            shell);

  /*  create the contents of the lower_hbox  *********************************/

  /*  the quick mask button  */
  shell->quick_mask_button = g_object_new (GTK_TYPE_CHECK_BUTTON,
                                           "draw-indicator", FALSE,
                                           "relief",         GTK_RELIEF_NONE,
                                           "width-request",  18,
                                           "height-request", 18,
                                           NULL);
  gtk_widget_set_can_focus (shell->quick_mask_button, FALSE);

  gtk_image = gtk_image_new_from_stock (GIMP_STOCK_QUICK_MASK_OFF,
                                        GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (shell->quick_mask_button), gtk_image);
  gtk_widget_show (gtk_image);

  action = gimp_ui_manager_find_action (shell->popup_manager,
                                        "quick-mask", "quick-mask-toggle");
  if (action)
    gimp_widget_set_accel_help (shell->quick_mask_button, action);
  else
    gimp_help_set_help_data (shell->quick_mask_button,
                             _("Toggle Quick Mask"),
                             GIMP_HELP_IMAGE_WINDOW_QUICK_MASK_BUTTON);

  g_signal_connect (shell->quick_mask_button, "toggled",
                    G_CALLBACK (gimp_display_shell_quick_mask_toggled),
                    shell);
  g_signal_connect (shell->quick_mask_button, "button-press-event",
                    G_CALLBACK (gimp_display_shell_quick_mask_button_press),
                    shell);

  /*  the navigation window button  */
  shell->nav_ebox = gtk_event_box_new ();

  gtk_image = gtk_image_new_from_stock (GIMP_STOCK_NAVIGATION,
                                        GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (shell->nav_ebox), gtk_image);
  gtk_widget_show (gtk_image);

  g_signal_connect (shell->nav_ebox, "button-press-event",
                    G_CALLBACK (gimp_display_shell_navigation_button_press),
                    shell);

  gimp_help_set_help_data (shell->nav_ebox,
                           _("Navigate the image display"),
                           GIMP_HELP_IMAGE_WINDOW_NAV_BUTTON);

  /*  the statusbar  ********************************************************/

  shell->statusbar = gimp_statusbar_new ();
  gimp_statusbar_set_shell (GIMP_STATUSBAR (shell->statusbar), shell);
  gimp_help_set_help_data (shell->statusbar, NULL,
                           GIMP_HELP_IMAGE_WINDOW_STATUS_BAR);
  gtk_box_pack_end (GTK_BOX (shell), shell->statusbar, FALSE, FALSE, 0);

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
  gtk_box_pack_start (GTK_BOX (right_vbox),
                      shell->zoom_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (right_vbox),
                      shell->vsb, TRUE, TRUE, 0);

  /*  fill the lower_hbox  */
  gtk_box_pack_start (GTK_BOX (lower_hbox),
                      shell->quick_mask_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (lower_hbox),
                      shell->hsb, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (lower_hbox),
                      shell->nav_ebox, FALSE, FALSE, 0);

  /*  show everything that is always shown ***********************************/

  gtk_widget_show (GTK_WIDGET (shell->canvas));

  /*  add display filter for color management  */

  filter = gimp_display_shell_filter_new (shell,
                                          GIMP_CORE_CONFIG (config)->color_management);

  if (filter)
    {
      gimp_display_shell_filter_set (shell, filter);
      g_object_unref (filter);
    }

  if (image)
    {
      gimp_display_shell_connect (shell);

      /* After connecting to the image we want to center it. Since we
       * not even finnished creating the display shell, we can safely
       * assume we will get a size-allocate later.
       */
      gimp_display_shell_scroll_center_image_on_next_size_allocate (shell,
                                                                    TRUE,
                                                                    TRUE);
    }
  else
    {
#if 0
      /* Disabled because it sets GDK_POINTER_MOTION_HINT on
       * shell->canvas. For info see Bug 677375
       */
      gimp_help_set_help_data (shell->canvas,
                               _("Drop image files here to open them"),
                               NULL);
#endif

      gimp_statusbar_empty (GIMP_STATUSBAR (shell->statusbar));
    }

  /* make sure the information is up-to-date */
  gimp_display_shell_scale_changed (shell);
}

static void
gimp_display_shell_dispose (GObject *object)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (object);

  if (shell->display && gimp_display_get_shell (shell->display))
    gimp_display_shell_disconnect (shell);

  shell->popup_manager = NULL;

  gimp_display_shell_selection_free (shell);

  if (shell->filter_stack)
    gimp_display_shell_filter_set (shell, NULL);

  if (shell->filter_idle_id)
    {
      g_source_remove (shell->filter_idle_id);
      shell->filter_idle_id = 0;
    }

  if (shell->render_surface)
    {
      cairo_surface_destroy (shell->render_surface);
      shell->render_surface = NULL;
    }

  if (shell->mask_surface)
    {
      cairo_surface_destroy (shell->mask_surface);
      shell->mask_surface = NULL;
    }

  if (shell->checkerboard)
    {
      cairo_pattern_destroy (shell->checkerboard);
      shell->checkerboard = NULL;
    }

  if (shell->mask)
    {
      g_object_unref (shell->mask);
      shell->mask = NULL;
    }

  gimp_display_shell_items_free (shell);

  if (shell->motion_buffer)
    {
      g_object_unref (shell->motion_buffer);
      shell->motion_buffer = NULL;
    }

  if (shell->zoom_focus_pointer_queue)
    {
      g_queue_free (shell->zoom_focus_pointer_queue);
      shell->zoom_focus_pointer_queue = NULL;
    }

  if (shell->title_idle_id)
    {
      g_source_remove (shell->title_idle_id);
      shell->title_idle_id = 0;
    }

  if (shell->fill_idle_id)
    {
      g_source_remove (shell->fill_idle_id);
      shell->fill_idle_id = 0;
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

  shell->display = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_display_shell_finalize (GObject *object)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (object);

  g_object_unref (shell->zoom);

  if (shell->options)
    g_object_unref (shell->options);

  if (shell->fullscreen_options)
    g_object_unref (shell->fullscreen_options);

  if (shell->no_image_options)
    g_object_unref (shell->no_image_options);

  if (shell->title)
    g_free (shell->title);

  if (shell->status)
    g_free (shell->status);

  if (shell->icon)
    g_object_unref (shell->icon);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_display_shell_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (object);

  switch (property_id)
    {
    case PROP_POPUP_MANAGER:
      shell->popup_manager = g_value_get_object (value);
      break;
    case PROP_DISPLAY:
      shell->display = g_value_get_object (value);
      break;
    case PROP_UNIT:
      gimp_display_shell_set_unit (shell, g_value_get_int (value));
      break;
    case PROP_TITLE:
      g_free (shell->title);
      shell->title = g_value_dup_string (value);
      break;
    case PROP_STATUS:
      g_free (shell->status);
      shell->status = g_value_dup_string (value);
      break;
    case PROP_ICON:
      if (shell->icon)
        g_object_unref (shell->icon);
      shell->icon = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_display_shell_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (object);

  switch (property_id)
    {
    case PROP_POPUP_MANAGER:
      g_value_set_object (value, shell->popup_manager);
      break;
    case PROP_DISPLAY:
      g_value_set_object (value, shell->display);
      break;
    case PROP_UNIT:
      g_value_set_int (value, shell->unit);
      break;
    case PROP_TITLE:
      g_value_set_string (value, shell->title);
      break;
    case PROP_STATUS:
      g_value_set_string (value, shell->status);
      break;
    case PROP_ICON:
      g_value_set_object (value, shell->icon);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_display_shell_unrealize (GtkWidget *widget)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (widget);

  if (shell->nav_popup)
    gtk_widget_unrealize (shell->nav_popup);

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
gimp_display_shell_screen_changed (GtkWidget *widget,
                                   GdkScreen *previous)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (widget);

  if (GTK_WIDGET_CLASS (parent_class)->screen_changed)
    GTK_WIDGET_CLASS (parent_class)->screen_changed (widget, previous);

  if (shell->display->config->monitor_res_from_gdk)
    {
      gimp_get_screen_resolution (gtk_widget_get_screen (widget),
                                  &shell->monitor_xres,
                                  &shell->monitor_yres);
    }
  else
    {
      shell->monitor_xres = shell->display->config->monitor_xres;
      shell->monitor_yres = shell->display->config->monitor_yres;
    }
}

static gboolean
gimp_display_shell_popup_menu (GtkWidget *widget)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (widget);

  gimp_context_set_display (gimp_get_user_context (shell->display->gimp),
                            shell->display);

  gimp_ui_manager_ui_popup (shell->popup_manager, "/dummy-menubar/image-popup",
                            GTK_WIDGET (shell),
                            gimp_display_shell_menu_position,
                            shell->origin,
                            NULL, NULL);

  return TRUE;
}

static void
gimp_display_shell_real_scaled (GimpDisplayShell *shell)
{
  GimpContext *user_context;

  if (! shell->display)
    return;

  gimp_display_shell_title_update (shell);

  user_context = gimp_get_user_context (shell->display->gimp);

  if (shell->display == gimp_context_get_display (user_context))
    gimp_ui_manager_update (shell->popup_manager, shell->display);
}

static const guint8 *
gimp_display_shell_get_icc_profile (GimpColorManaged *managed,
                                    gsize            *len)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (managed);
  GimpImage        *image = gimp_display_get_image (shell->display);

  if (image)
    return gimp_color_managed_get_icc_profile (GIMP_COLOR_MANAGED (image), len);

  return NULL;
}

static void
gimp_display_shell_menu_position (GtkMenu  *menu,
                                  gint     *x,
                                  gint     *y,
                                  gpointer  data)
{
  gimp_button_menu_position (GTK_WIDGET (data), menu, GTK_POS_RIGHT, x, y);
}

static void
gimp_display_shell_zoom_button_callback (GimpDisplayShell *shell,
                                         GtkWidget        *zoom_button)
{
  shell->zoom_on_resize =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (zoom_button));

  if (shell->zoom_on_resize &&
      gimp_display_shell_scale_image_is_within_viewport (shell, NULL, NULL))
    {
      /* Implicitly make a View -> Fit Image in Window */
      gimp_display_shell_scale_fit_in (shell);
    }
}

static void
gimp_display_shell_sync_config (GimpDisplayShell  *shell,
                                GimpDisplayConfig *config)
{
  gimp_config_sync (G_OBJECT (config->default_view),
                    G_OBJECT (shell->options), 0);
  gimp_config_sync (G_OBJECT (config->default_fullscreen_view),
                    G_OBJECT (shell->fullscreen_options), 0);

  if (shell->display && gimp_display_get_shell (shell->display))
    {
      /*  if the shell is already fully constructed, use proper API
       *  so the actions are updated accordingly.
       */
      gimp_display_shell_set_snap_to_guides  (shell,
                                              config->default_snap_to_guides);
      gimp_display_shell_set_snap_to_grid    (shell,
                                              config->default_snap_to_grid);
      gimp_display_shell_set_snap_to_canvas  (shell,
                                              config->default_snap_to_canvas);
      gimp_display_shell_set_snap_to_vectors (shell,
                                              config->default_snap_to_path);
    }
  else
    {
      /*  otherwise the shell is currently being constructed and
       *  display->shell is NULL.
       */
      shell->snap_to_guides  = config->default_snap_to_guides;
      shell->snap_to_grid    = config->default_snap_to_grid;
      shell->snap_to_canvas  = config->default_snap_to_canvas;
      shell->snap_to_vectors = config->default_snap_to_path;
    }
}

static void
gimp_display_shell_remove_overlay (GtkWidget        *canvas,
                                   GtkWidget        *child,
                                   GimpDisplayShell *shell)
{
  shell->children = g_list_remove (shell->children, child);
}

static void
gimp_display_shell_transform_overlay (GimpDisplayShell *shell,
                                      GtkWidget        *child,
                                      gdouble          *x,
                                      gdouble          *y)
{
  GimpDisplayShellOverlay *overlay;
  GtkRequisition           requisition;

  overlay = g_object_get_data (G_OBJECT (child), "image-coords-overlay");

  gimp_display_shell_transform_xy_f (shell,
                                     overlay->image_x,
                                     overlay->image_y,
                                     x, y);

  gtk_widget_size_request (child, &requisition);

  switch (overlay->anchor)
    {
    case GIMP_HANDLE_ANCHOR_CENTER:
      *x -= requisition.width  / 2;
      *y -= requisition.height / 2;
      break;

    case GIMP_HANDLE_ANCHOR_NORTH:
      *x -= requisition.width / 2;
      *y += overlay->spacing_y;
      break;

    case GIMP_HANDLE_ANCHOR_NORTH_WEST:
      *x += overlay->spacing_x;
      *y += overlay->spacing_y;
      break;

    case GIMP_HANDLE_ANCHOR_NORTH_EAST:
      *x -= requisition.width + overlay->spacing_x;
      *y += overlay->spacing_y;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH:
      *x -= requisition.width / 2;
      *y -= requisition.height + overlay->spacing_y;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH_WEST:
      *x += overlay->spacing_x;
      *y -= requisition.height + overlay->spacing_y;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH_EAST:
      *x -= requisition.width + overlay->spacing_x;
      *y -= requisition.height + overlay->spacing_y;
      break;

    case GIMP_HANDLE_ANCHOR_WEST:
      *x += overlay->spacing_x;
      *y -= requisition.height / 2;
      break;

    case GIMP_HANDLE_ANCHOR_EAST:
      *x -= requisition.width + overlay->spacing_x;
      *y -= requisition.height / 2;
      break;
    }
}


/*  public functions  */

GtkWidget *
gimp_display_shell_new (GimpDisplay       *display,
                        GimpUnit           unit,
                        gdouble            scale,
                        GimpUIManager     *popup_manager)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (GIMP_IS_UI_MANAGER (popup_manager), NULL);

  return g_object_new (GIMP_TYPE_DISPLAY_SHELL,
                       "popup-manager", popup_manager,
                       "display",       display,
                       "unit",          unit,
                       NULL);
}

void
gimp_display_shell_add_overlay (GimpDisplayShell *shell,
                                GtkWidget        *child,
                                gdouble           image_x,
                                gdouble           image_y,
                                GimpHandleAnchor  anchor,
                                gint              spacing_x,
                                gint              spacing_y)
{
  GimpDisplayShellOverlay *overlay;
  gdouble                  x, y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GTK_IS_WIDGET (shell));

  overlay = g_new0 (GimpDisplayShellOverlay, 1);

  overlay->image_x   = image_x;
  overlay->image_y   = image_y;
  overlay->anchor    = anchor;
  overlay->spacing_x = spacing_x;
  overlay->spacing_y = spacing_y;

  g_object_set_data_full (G_OBJECT (child), "image-coords-overlay", overlay,
                          (GDestroyNotify) g_free);

  shell->children = g_list_prepend (shell->children, child);

  gimp_display_shell_transform_overlay (shell, child, &x, &y);

  gimp_overlay_box_add_child (GIMP_OVERLAY_BOX (shell->canvas), child, 0.0, 0.0);
  gimp_overlay_box_set_child_position (GIMP_OVERLAY_BOX (shell->canvas),
                                       child, x, y);
}

void
gimp_display_shell_move_overlay (GimpDisplayShell *shell,
                                 GtkWidget        *child,
                                 gdouble           image_x,
                                 gdouble           image_y,
                                 GimpHandleAnchor  anchor,
                                 gint              spacing_x,
                                 gint              spacing_y)
{
  GimpDisplayShellOverlay *overlay;
  gdouble                  x, y;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GTK_IS_WIDGET (shell));

  overlay = g_object_get_data (G_OBJECT (child), "image-coords-overlay");

  g_return_if_fail (overlay != NULL);

  overlay->image_x   = image_x;
  overlay->image_y   = image_y;
  overlay->anchor    = anchor;
  overlay->spacing_x = spacing_x;
  overlay->spacing_y = spacing_y;

  gimp_display_shell_transform_overlay (shell, child, &x, &y);

  gimp_overlay_box_set_child_position (GIMP_OVERLAY_BOX (shell->canvas),
                                       child, x, y);
}

GimpImageWindow *
gimp_display_shell_get_window (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return GIMP_IMAGE_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (shell),
                                                     GIMP_TYPE_IMAGE_WINDOW));
}

GimpStatusbar *
gimp_display_shell_get_statusbar (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return GIMP_STATUSBAR (shell->statusbar);
}

void
gimp_display_shell_present (GimpDisplayShell *shell)
{
  GimpImageWindow *window;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  window = gimp_display_shell_get_window (shell);

  if (window)
    {
      gimp_image_window_set_active_shell (window, shell);

      gtk_window_present (GTK_WINDOW (window));
    }
}

void
gimp_display_shell_reconnect (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_DISPLAY (shell->display));
  g_return_if_fail (gimp_display_get_image (shell->display) != NULL);

  if (shell->fill_idle_id)
    {
      g_source_remove (shell->fill_idle_id);
      shell->fill_idle_id = 0;
    }

  g_signal_emit (shell, display_shell_signals[RECONNECT], 0);

  gimp_color_managed_profile_changed (GIMP_COLOR_MANAGED (shell));

  gimp_display_shell_scroll_clamp_and_update (shell);

  gimp_display_shell_scaled (shell);

  gimp_display_shell_expose_full (shell);
}

void
gimp_display_shell_empty (GimpDisplayShell *shell)
{
  GimpContext     *user_context;
  GimpImageWindow *window;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_DISPLAY (shell->display));
  g_return_if_fail (gimp_display_get_image (shell->display) == NULL);

  window = gimp_display_shell_get_window (shell);

  if (shell->fill_idle_id)
    {
      g_source_remove (shell->fill_idle_id);
      shell->fill_idle_id = 0;
    }

  gimp_display_shell_selection_undraw (shell);

  gimp_display_shell_unset_cursor (shell);

  gimp_display_shell_sync_config (shell, shell->display->config);

  gimp_display_shell_appearance_update (shell);
  gimp_image_window_update_tabs (window);
#if 0
  gimp_help_set_help_data (shell->canvas,
                           _("Drop image files here to open them"), NULL);
#endif

  gimp_statusbar_empty (GIMP_STATUSBAR (shell->statusbar));

  /*  so wilber doesn't flicker  */
  gtk_widget_set_double_buffered (shell->canvas, TRUE);

  gimp_display_shell_expose_full (shell);

  user_context = gimp_get_user_context (shell->display->gimp);

  if (shell->display == gimp_context_get_display (user_context))
    gimp_ui_manager_update (shell->popup_manager, shell->display);
}

static gboolean
gimp_display_shell_fill_idle (GimpDisplayShell *shell)
{
  GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (shell));

  shell->fill_idle_id = 0;

  if (GTK_IS_WINDOW (toplevel))
    {
      gimp_display_shell_scale_shrink_wrap (shell, TRUE);

      gtk_window_present (GTK_WINDOW (toplevel));
    }

  return FALSE;
}

void
gimp_display_shell_fill (GimpDisplayShell *shell,
                         GimpImage        *image,
                         GimpUnit          unit,
                         gdouble           scale)
{
  GimpImageWindow *window;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_DISPLAY (shell->display));
  g_return_if_fail (GIMP_IS_IMAGE (image));

  window = gimp_display_shell_get_window (shell);

  gimp_display_shell_set_unit (shell, unit);
  gimp_display_shell_set_initial_scale (shell, scale, NULL, NULL);
  gimp_display_shell_scale_changed (shell);

  gimp_display_shell_sync_config (shell, shell->display->config);

  gimp_display_shell_appearance_update (shell);
  gimp_image_window_update_tabs (window);
#if 0
  gimp_help_set_help_data (shell->canvas, NULL, NULL);
#endif

  gimp_statusbar_fill (GIMP_STATUSBAR (shell->statusbar));

  /* A size-allocate will always occur because the scrollbars will
   * become visible forcing the canvas to become smaller
   */
  gimp_display_shell_scroll_center_image_on_next_size_allocate (shell,
                                                                TRUE,
                                                                TRUE);

  /*  we double buffer image drawing manually  */
  gtk_widget_set_double_buffered (shell->canvas, FALSE);

  shell->fill_idle_id = g_idle_add_full (G_PRIORITY_LOW,
                                         (GSourceFunc) gimp_display_shell_fill_idle,
                                         shell, NULL);
}

/* We used to calculate the scale factor in the SCALEFACTOR_X() and
 * SCALEFACTOR_Y() macros. But since these are rather frequently
 * called and the values rarely change, we now store them in the
 * shell and call this function whenever they need to be recalculated.
 */
void
gimp_display_shell_scale_changed (GimpDisplayShell *shell)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  image = gimp_display_get_image (shell->display);

  if (image)
    {
      gimp_display_shell_calculate_scale_x_and_y (shell,
                                                  gimp_zoom_model_get_factor (shell->zoom),
                                                  &shell->scale_x,
                                                  &shell->scale_y);

      shell->x_dest_inc = gimp_image_get_width  (image);
      shell->y_dest_inc = gimp_image_get_height (image);
      shell->x_src_dec  = shell->scale_x * shell->x_dest_inc;
      shell->y_src_dec  = shell->scale_y * shell->y_dest_inc;

      if (shell->x_src_dec < 1)
        shell->x_src_dec = 1;

      if (shell->y_src_dec < 1)
        shell->y_src_dec = 1;
    }
  else
    {
      shell->scale_x = 1.0;
      shell->scale_y = 1.0;

      shell->x_dest_inc = 1;
      shell->y_dest_inc = 1;
      shell->x_src_dec  = 1;
      shell->y_src_dec  = 1;
    }
}

void
gimp_display_shell_scaled (GimpDisplayShell *shell)
{
  GList *list;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  for (list = shell->children; list; list = g_list_next (list))
    {
      GtkWidget *child = list->data;
      gdouble    x, y;

      gimp_display_shell_transform_overlay (shell, child, &x, &y);

      gimp_overlay_box_set_child_position (GIMP_OVERLAY_BOX (shell->canvas),
                                           child, x, y);
    }

  g_signal_emit (shell, display_shell_signals[SCALED], 0);
}

void
gimp_display_shell_scrolled (GimpDisplayShell *shell)
{
  GList *list;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  for (list = shell->children; list; list = g_list_next (list))
    {
      GtkWidget *child = list->data;
      gdouble    x, y;

      gimp_display_shell_transform_overlay (shell, child, &x, &y);

      gimp_overlay_box_set_child_position (GIMP_OVERLAY_BOX (shell->canvas),
                                           child, x, y);
    }

  g_signal_emit (shell, display_shell_signals[SCROLLED], 0);
}

void
gimp_display_shell_set_unit (GimpDisplayShell *shell,
                             GimpUnit          unit)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->unit != unit)
    {
      shell->unit = unit;

      gimp_display_shell_scale_update_rulers (shell);

      gimp_display_shell_scaled (shell);

      g_object_notify (G_OBJECT (shell), "unit");
    }
}

GimpUnit
gimp_display_shell_get_unit (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), GIMP_UNIT_PIXEL);

  return shell->unit;
}

gboolean
gimp_display_shell_snap_coords (GimpDisplayShell *shell,
                                GimpCoords       *coords,
                                gint              snap_offset_x,
                                gint              snap_offset_y,
                                gint              snap_width,
                                gint              snap_height)
{
  GimpImage *image;
  gboolean   snap_to_guides  = FALSE;
  gboolean   snap_to_grid    = FALSE;
  gboolean   snap_to_canvas  = FALSE;
  gboolean   snap_to_vectors = FALSE;
  gboolean   snapped         = FALSE;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);

  image = gimp_display_get_image (shell->display);

  if (gimp_display_shell_get_snap_to_guides (shell) &&
      gimp_image_get_guides (image))
    {
      snap_to_guides = TRUE;
    }

  if (gimp_display_shell_get_snap_to_grid (shell) &&
      gimp_image_get_grid (image))
    {
      snap_to_grid = TRUE;
    }

  snap_to_canvas = gimp_display_shell_get_snap_to_canvas (shell);

  if (gimp_display_shell_get_snap_to_vectors (shell) &&
      gimp_image_get_active_vectors (image))
    {
      snap_to_vectors = TRUE;
    }

  if (snap_to_guides || snap_to_grid || snap_to_canvas || snap_to_vectors)
    {
      gint    snap_distance;
      gdouble tx, ty;

      snap_distance = shell->display->config->snap_distance;

      if (snap_width > 0 && snap_height > 0)
        {
          snapped = gimp_image_snap_rectangle (image,
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
                                               snap_to_grid,
                                               snap_to_canvas,
                                               snap_to_vectors);
        }
      else
        {
          snapped = gimp_image_snap_point (image,
                                           coords->x + snap_offset_x,
                                           coords->y + snap_offset_y,
                                           &tx,
                                           &ty,
                                           FUNSCALEX (shell, snap_distance),
                                           FUNSCALEY (shell, snap_distance),
                                           snap_to_guides,
                                           snap_to_grid,
                                           snap_to_canvas,
                                           snap_to_vectors);
        }

      if (snapped)
        {
          coords->x = tx - snap_offset_x;
          coords->y = ty - snap_offset_y;
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
  GimpImage *image;
  GimpLayer *layer;
  gdouble    x1_f, y1_f;
  gdouble    x2_f, y2_f;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);
  g_return_val_if_fail (x1 != NULL, FALSE);
  g_return_val_if_fail (y1 != NULL, FALSE);
  g_return_val_if_fail (x2 != NULL, FALSE);
  g_return_val_if_fail (y2 != NULL, FALSE);

  image = gimp_display_get_image (shell->display);

  /*  If there is a floating selection, handle things differently  */
  if ((layer = gimp_image_get_floating_selection (image)))
    {
      gint off_x;
      gint off_y;

      gimp_item_get_offset (GIMP_ITEM (layer), &off_x, &off_y);

      if (! gimp_channel_bounds (gimp_image_get_mask (image),
                                 x1, y1, x2, y2))
        {
          *x1 = off_x;
          *y1 = off_y;
          *x2 = off_x + gimp_item_get_width  (GIMP_ITEM (layer));
          *y2 = off_y + gimp_item_get_height (GIMP_ITEM (layer));
        }
      else
        {
          *x1 = MIN (off_x, *x1);
          *y1 = MIN (off_y, *y1);
          *x2 = MAX (off_x + gimp_item_get_width  (GIMP_ITEM (layer)), *x2);
          *y2 = MAX (off_y + gimp_item_get_height (GIMP_ITEM (layer)), *y2);
        }
    }
  else if (! gimp_channel_bounds (gimp_image_get_mask (image),
                                  x1, y1, x2, y2))
    {
      return FALSE;
    }

  gimp_display_shell_transform_xy_f (shell, *x1, *y1, &x1_f, &y1_f);
  gimp_display_shell_transform_xy_f (shell, *x2, *y2, &x2_f, &y2_f);

  /*  Make sure the extents are within bounds  */
  *x1 = CLAMP (floor (x1_f), 0, shell->disp_width);
  *y1 = CLAMP (floor (y1_f), 0, shell->disp_height);
  *x2 = CLAMP (ceil (x2_f),  0, shell->disp_width);
  *y2 = CLAMP (ceil (y2_f),  0, shell->disp_height);

  return ((*x2 - *x1) > 0) && ((*y2 - *y1) > 0);
}

void
gimp_display_shell_flush (GimpDisplayShell *shell,
                          gboolean          now)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_title_update (shell);

  /* make sure the information is up-to-date */
  gimp_display_shell_scale_changed (shell);

  gimp_canvas_layer_boundary_set_layer (GIMP_CANVAS_LAYER_BOUNDARY (shell->layer_boundary),
                                        gimp_image_get_active_layer (gimp_display_get_image (shell->display)));

  if (now)
    {
      gdk_window_process_updates (gtk_widget_get_window (shell->canvas),
                                  FALSE);
    }
  else
    {
      GimpImageWindow *window = gimp_display_shell_get_window (shell);
      GimpContext     *context;

      if (window && gimp_image_window_get_active_shell (window) == shell)
        {
          GimpUIManager *manager = gimp_image_window_get_ui_manager (window);

          gimp_ui_manager_update (manager, shell->display);
        }

      context = gimp_get_user_context (shell->display->gimp);

      if (shell->display == gimp_context_get_display (context))
        {
          gimp_ui_manager_update (shell->popup_manager, shell->display);
        }
    }
}

/**
 * gimp_display_shell_pause:
 * @shell: a display shell
 *
 * This function increments the pause count or the display shell.
 * If it was zero coming in, then the function pauses the active tool,
 * so that operations on the display can take place without corrupting
 * anything that the tool has drawn.  It "undraws" the current tool
 * drawing, and must be followed by gimp_display_shell_resume() after
 * the operation in question is completed.
 **/
void
gimp_display_shell_pause (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  shell->paused_count++;

  if (shell->paused_count == 1)
    {
      /*  pause the currently active tool  */
      tool_manager_control_active (shell->display->gimp,
                                   GIMP_TOOL_ACTION_PAUSE,
                                   shell->display);
    }
}

/**
 * gimp_display_shell_resume:
 * @shell: a display shell
 *
 * This function decrements the pause count for the display shell.
 * If this brings it to zero, then the current tool is resumed.
 * It is an error to call this function without having previously
 * called gimp_display_shell_pause().
 **/
void
gimp_display_shell_resume (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (shell->paused_count > 0);

  shell->paused_count--;

  if (shell->paused_count == 0)
    {
      /* start the currently active tool */
      tool_manager_control_active (shell->display->gimp,
                                   GIMP_TOOL_ACTION_RESUME,
                                   shell->display);
    }
}

/**
 * gimp_display_shell_set_highlight:
 * @shell:     a #GimpDisplayShell
 * @highlight: a rectangle in image coordinates that should be brought out
 *
 * This function allows to set an area of the image that should be
 * accentuated. The actual implementation is to dim all pixels outside
 * this rectangle. Passing %NULL for @highlight unsets the rectangle.
 **/
void
gimp_display_shell_set_highlight (GimpDisplayShell   *shell,
                                  const GdkRectangle *highlight)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (highlight)
    {
      gimp_canvas_item_begin_change (shell->passe_partout);

      gimp_canvas_rectangle_set (shell->passe_partout,
                                 highlight->x,
                                 highlight->y,
                                 highlight->width,
                                 highlight->height);

      gimp_canvas_item_set_visible (shell->passe_partout, TRUE);

      gimp_canvas_item_end_change (shell->passe_partout);
    }
  else
    {
      gimp_canvas_item_set_visible (shell->passe_partout, FALSE);
    }
}

/**
 * gimp_display_shell_set_mask:
 * @shell: a #GimpDisplayShell
 * @mask:  a #GimpDrawable (1 byte per pixel)
 * @color: the color to use for drawing the mask
 *
 * Allows to preview a selection (used by the foreground selection
 * tool).  Pixels that are not selected (> 127) in the mask are tinted
 * with the given color.
 **/
void
gimp_display_shell_set_mask (GimpDisplayShell *shell,
                             GimpDrawable     *mask,
                             const GimpRGB    *color)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (mask == NULL ||
                    (GIMP_IS_DRAWABLE (mask) &&
                     gimp_drawable_bytes (mask) == 1));
  g_return_if_fail (mask == NULL || color != NULL);

  if (mask)
    g_object_ref (mask);

  if (shell->mask)
    g_object_unref (shell->mask);

  shell->mask = mask;

  if (mask)
    shell->mask_color = *color;

  gimp_display_shell_expose_full (shell);
}
