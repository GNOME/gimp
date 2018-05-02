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
#include "core/gimp-utils.h"
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
#include "gimpdisplayshell-profile.h"
#include "gimpdisplayshell-progress.h"
#include "gimpdisplayshell-render.h"
#include "gimpdisplayshell-rotate.h"
#include "gimpdisplayshell-rulers.h"
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
#include "gimp-priorities.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_POPUP_MANAGER,
  PROP_INITIAL_MONITOR,
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
  ROTATED,
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
static void      gimp_display_shell_real_scrolled  (GimpDisplayShell *shell);
static void      gimp_display_shell_real_rotated   (GimpDisplayShell *shell);

static const guint8 *
                 gimp_display_shell_get_icc_profile(GimpColorManaged *managed,
                                                    gsize            *len);
static GimpColorProfile *
               gimp_display_shell_get_color_profile(GimpColorManaged *managed);
static void      gimp_display_shell_profile_changed(GimpColorManaged *managed);

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
                         GTK_TYPE_EVENT_BOX,
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

  display_shell_signals[ROTATED] =
    g_signal_new ("rotated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDisplayShellClass, rotated),
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
  klass->scrolled                  = gimp_display_shell_real_scrolled;
  klass->rotated                   = gimp_display_shell_real_rotated;
  klass->reconnect                 = NULL;

  g_object_class_install_property (object_class, PROP_POPUP_MANAGER,
                                   g_param_spec_object ("popup-manager",
                                                        NULL, NULL,
                                                        GIMP_TYPE_UI_MANAGER,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_INITIAL_MONITOR,
                                   g_param_spec_object ("initial-monitor",
                                                        NULL, NULL,
                                                        GDK_TYPE_MONITOR,
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
  iface->get_icc_profile   = gimp_display_shell_get_icc_profile;
  iface->get_color_profile = gimp_display_shell_get_color_profile;
  iface->profile_changed   = gimp_display_shell_profile_changed;
}

static void
gimp_display_shell_init (GimpDisplayShell *shell)
{
  shell->options            = g_object_new (GIMP_TYPE_DISPLAY_OPTIONS, NULL);
  shell->fullscreen_options = g_object_new (GIMP_TYPE_DISPLAY_OPTIONS_FULLSCREEN, NULL);
  shell->no_image_options   = g_object_new (GIMP_TYPE_DISPLAY_OPTIONS_NO_IMAGE, NULL);

  shell->zoom        = gimp_zoom_model_new ();
  shell->dot_for_dot = TRUE;
  shell->scale_x     = 1.0;
  shell->scale_y     = 1.0;

  gimp_display_shell_items_init (shell);

  shell->icon_size       = 128;
  shell->icon_size_small = 96;

  shell->cursor_handedness = GIMP_HANDEDNESS_RIGHT;
  shell->current_cursor    = (GimpCursorType) -1;
  shell->tool_cursor       = GIMP_TOOL_CURSOR_NONE;
  shell->cursor_modifier   = GIMP_CURSOR_MODIFIER_NONE;
  shell->override_cursor   = (GimpCursorType) -1;

  shell->filter_format     = babl_format ("R'G'B'A float");

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
                            G_CALLBACK (gimp_display_shell_scale_update),
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
  GtkWidget             *main_vbox;
  GtkWidget             *upper_hbox;
  GtkWidget             *right_vbox;
  GtkWidget             *lower_hbox;
  GtkWidget             *inner_grid;
  GtkWidget             *gtk_image;
  GtkAction             *action;
  gint                   image_width;
  gint                   image_height;
  gint                   shell_width;
  gint                   shell_height;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_UI_MANAGER (shell->popup_manager));
  gimp_assert (GIMP_IS_DISPLAY (shell->display));

  config = shell->display->config;
  image  = gimp_display_get_image (shell->display);

  gimp_display_shell_profile_init (shell);

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

  if (config->monitor_res_from_gdk)
    {
      gimp_get_monitor_resolution (shell->initial_monitor,
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
   *     +-- main_vbox
   *            |
   *            +-- upper_hbox
   *            |      |
   *            |      +-- inner_grid
   *            |      |      |
   *            |      |      +-- origin
   *            |      |      +-- hruler
   *            |      |      +-- vruler
   *            |      |      +-- canvas
   *            |      |
   *            |      +-- right_vbox
   *            |             |
   *            |             +-- zoom_on_resize_button
   *            |             +-- vscrollbar
   *            |
   *            +-- lower_hbox
   *            |      |
   *            |      +-- quick_mask
   *            |      +-- hscrollbar
   *            |      +-- navbutton
   *            |
   *            +-- statusbar
   *
   *  Note that we separate "shell" and "main_vbox", so that we can make
   *  "shell" a GtkEventBox, giving it its own window.  This isolates our
   *  events from those of our ancestors, avoiding some potential slowdowns,
   *  and making things generally smoother.  See bug #778966.
   */

  /*  first, set up the container hierarchy  *********************************/

  /*  the root vbox  */

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (shell), main_vbox);
  gtk_widget_show (main_vbox);

  /*  a hbox for the inner_grid and the vertical scrollbar  */
  upper_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), upper_hbox, TRUE, TRUE, 0);
  gtk_widget_show (upper_hbox);

  /*  the table containing origin, rulers and the canvas  */
  inner_grid = gtk_grid_new ();
  gtk_box_pack_start (GTK_BOX (upper_hbox), inner_grid, TRUE, TRUE, 0);
  gtk_widget_show (inner_grid);

  /*  the vbox containing the color button and the vertical scrollbar  */
  right_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
  gtk_box_pack_start (GTK_BOX (upper_hbox), right_vbox, FALSE, FALSE, 0);
  gtk_widget_show (right_vbox);

  /*  the hbox containing the quickmask button, vertical scrollbar and
   *  the navigation button
   */
  lower_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_box_pack_start (GTK_BOX (main_vbox), lower_hbox, FALSE, FALSE, 0);
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

  /*  create the contents of the inner_grif * ********************************/

  /*  the menu popup button  */
  shell->origin = gtk_event_box_new ();

  gtk_image = gtk_image_new_from_icon_name (GIMP_ICON_MENU_RIGHT,
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

  gimp_help_set_help_data (shell->vrule, NULL, GIMP_HELP_IMAGE_WINDOW_RULER);

  /*  set the rulers as track widgets for each other, so we don't end up
   *  with one ruler wrongly being stuck a few pixels off while we are
   *  hovering the other
   */
  gimp_ruler_add_track_widget (GIMP_RULER (shell->hrule), shell->vrule);
  gimp_ruler_add_track_widget (GIMP_RULER (shell->vrule), shell->hrule);

  gimp_devices_add_widget (shell->display->gimp, shell->hrule);
  gimp_devices_add_widget (shell->display->gimp, shell->vrule);

  g_signal_connect (shell->canvas, "grab-notify",
                    G_CALLBACK (gimp_display_shell_canvas_grab_notify),
                    shell);

  g_signal_connect (shell->canvas, "realize",
                    G_CALLBACK (gimp_display_shell_canvas_realize),
                    shell);
  g_signal_connect (shell->canvas, "size-allocate",
                    G_CALLBACK (gimp_display_shell_canvas_size_allocate),
                    shell);
  g_signal_connect (shell->canvas, "draw",
                    G_CALLBACK (gimp_display_shell_canvas_draw),
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

  gtk_image = gtk_image_new_from_icon_name (GIMP_ICON_ZOOM_FOLLOW_WINDOW,
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

  gtk_image = gtk_image_new_from_icon_name (GIMP_ICON_QUICK_MASK_OFF,
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

  gtk_image = gtk_image_new_from_icon_name (GIMP_ICON_DIALOG_NAVIGATION,
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
  gtk_box_pack_end (GTK_BOX (main_vbox), shell->statusbar, FALSE, FALSE, 0);

  /*  pack all the widgets  **************************************************/

  /*  fill the inner_grid  */
  gtk_grid_attach (GTK_GRID (inner_grid), shell->origin, 0, 0, 1, 1);

  gtk_widget_set_hexpand (shell->hrule, TRUE);
  gtk_grid_attach (GTK_GRID (inner_grid), shell->hrule, 1, 0, 1, 1);

  gtk_widget_set_vexpand (shell->vrule, TRUE);
  gtk_grid_attach (GTK_GRID (inner_grid), shell->vrule, 0, 1, 1, 1);

  gtk_widget_set_hexpand (shell->canvas, TRUE);
  gtk_widget_set_vexpand (shell->canvas, TRUE);
  gtk_grid_attach (GTK_GRID (inner_grid), shell->canvas, 1, 1, 1, 1);

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

  /*  add display filters  */

  filter = gimp_display_shell_filter_new (shell);

  if (filter)
    {
      gimp_display_shell_filter_set (shell, filter);
      g_object_unref (filter);
    }

  if (image)
    {
      gimp_display_shell_connect (shell);

      /* After connecting to the image we want to center it. Since we
       * not even finished creating the display shell, we can safely
       * assume we will get a size-allocate later.
       */
      gimp_display_shell_scroll_center_image_on_size_allocate (shell,
                                                               TRUE, TRUE);
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
  gimp_display_shell_scale_update (shell);
}

static void
gimp_display_shell_dispose (GObject *object)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (object);

  if (shell->display && gimp_display_get_shell (shell->display))
    gimp_display_shell_disconnect (shell);

  shell->popup_manager = NULL;

  if (shell->selection)
    gimp_display_shell_selection_free (shell);

  if (shell->filter_stack)
    gimp_display_shell_filter_set (shell, NULL);

  if (shell->filter_idle_id)
    {
      g_source_remove (shell->filter_idle_id);
      shell->filter_idle_id = 0;
    }

  g_clear_pointer (&shell->mask_surface, cairo_surface_destroy);
  g_clear_pointer (&shell->checkerboard, cairo_pattern_destroy);

  gimp_display_shell_profile_finalize (shell);

  g_clear_object (&shell->filter_buffer);
  shell->filter_data   = NULL;
  shell->filter_stride = 0;

  g_clear_object (&shell->mask);

  gimp_display_shell_items_free (shell);

  g_clear_object (&shell->motion_buffer);

  g_clear_pointer (&shell->zoom_focus_pointer_queue, g_queue_free);

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

  g_clear_pointer (&shell->nav_popup, gtk_widget_destroy);

  if (shell->blink_timeout_id)
    {
      g_source_remove (shell->blink_timeout_id);
      shell->blink_timeout_id = 0;
    }

  shell->display = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_display_shell_finalize (GObject *object)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (object);

  g_clear_object (&shell->zoom);
  g_clear_pointer (&shell->rotate_transform,   g_free);
  g_clear_pointer (&shell->rotate_untransform, g_free);
  g_clear_object (&shell->options);
  g_clear_object (&shell->fullscreen_options);
  g_clear_object (&shell->no_image_options);
  g_clear_pointer (&shell->title,  g_free);
  g_clear_pointer (&shell->status, g_free);
  g_clear_object (&shell->icon);

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
    case PROP_INITIAL_MONITOR:
      shell->initial_monitor = g_value_get_object (value);
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
    case PROP_INITIAL_MONITOR:
      g_value_set_object (value, shell->initial_monitor);
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
      gimp_get_monitor_resolution (gimp_widget_get_monitor (widget),
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

  gimp_ui_manager_ui_popup_at_widget (shell->popup_manager,
                                      "/dummy-menubar/image-popup",
                                      shell->origin,
                                      GDK_GRAVITY_EAST,
                                      GDK_GRAVITY_NORTH_WEST,
                                      NULL,
                                      NULL, NULL);

  return TRUE;
}

static void
gimp_display_shell_set_priority_viewport (GimpDisplayShell *shell)
{
  GimpImage *image = gimp_display_get_image (shell->display);

  if (image)
    {
      GimpProjection *projection = gimp_image_get_projection (image);
      gint            x, y;
      gint            width, height;

      gimp_display_shell_untransform_viewport (shell, &x, &y, &width, &height);
      gimp_projection_set_priority_rect (projection, x, y, width, height);
    }
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
    {
      gimp_display_shell_set_priority_viewport (shell);

      gimp_ui_manager_update (shell->popup_manager, shell->display);
    }
}

static void
gimp_display_shell_real_scrolled (GimpDisplayShell *shell)
{
  GimpContext *user_context;

  if (! shell->display)
    return;

  gimp_display_shell_title_update (shell);

  user_context = gimp_get_user_context (shell->display->gimp);

  if (shell->display == gimp_context_get_display (user_context))
    {
      gimp_display_shell_set_priority_viewport (shell);

    }
}

static void
gimp_display_shell_real_rotated (GimpDisplayShell *shell)
{
  GimpContext *user_context;

  if (! shell->display)
    return;

  gimp_display_shell_title_update (shell);

  user_context = gimp_get_user_context (shell->display->gimp);

  if (shell->display == gimp_context_get_display (user_context))
    {
      gimp_display_shell_set_priority_viewport (shell);

      gimp_ui_manager_update (shell->popup_manager, shell->display);
    }
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

static GimpColorProfile *
gimp_display_shell_get_color_profile (GimpColorManaged *managed)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (managed);
  GimpImage        *image = gimp_display_get_image (shell->display);

  if (image)
    return gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (image));

  return NULL;
}

static void
gimp_display_shell_profile_changed (GimpColorManaged *managed)
{
  GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (managed);

  gimp_display_shell_profile_update (shell);
  gimp_display_shell_expose_full (shell);
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

  gtk_widget_get_preferred_size (child, &requisition, NULL);

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
gimp_display_shell_new (GimpDisplay   *display,
                        GimpUnit       unit,
                        gdouble        scale,
                        GimpUIManager *popup_manager,
                        GdkMonitor    *monitor)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (GIMP_IS_UI_MANAGER (popup_manager), NULL);
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);

  return g_object_new (GIMP_TYPE_DISPLAY_SHELL,
                       "popup-manager",   popup_manager,
                       "initial-monitor", monitor,
                       "display",         display,
                       "unit",            unit,
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

GimpColorConfig *
gimp_display_shell_get_color_config (GimpDisplayShell *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return shell->color_config;
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

static gboolean
gimp_display_shell_blink (GimpDisplayShell *shell)
{
  shell->blink_timeout_id = 0;

  if (shell->blink)
    {
      shell->blink = FALSE;
    }
  else
    {
      shell->blink = TRUE;

      shell->blink_timeout_id =
        g_timeout_add (100, (GSourceFunc) gimp_display_shell_blink, shell);
    }

  gimp_display_shell_expose_full (shell);

  return FALSE;
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

  gimp_display_shell_filter_set (shell, NULL);

  gimp_display_shell_sync_config (shell, shell->display->config);

  gimp_display_shell_appearance_update (shell);
  gimp_image_window_update_tabs (window);
#if 0
  gimp_help_set_help_data (shell->canvas,
                           _("Drop image files here to open them"), NULL);
#endif

  gimp_statusbar_empty (GIMP_STATUSBAR (shell->statusbar));

  shell->flip_horizontally = FALSE;
  shell->flip_vertically   = FALSE;
  shell->rotate_angle      = 0.0;
  gimp_display_shell_rotate_update_transform (shell);

  gimp_display_shell_expose_full (shell);

  user_context = gimp_get_user_context (shell->display->gimp);

  if (shell->display == gimp_context_get_display (user_context))
    gimp_ui_manager_update (shell->popup_manager, shell->display);

  shell->blink_timeout_id =
    g_timeout_add (1403230, (GSourceFunc) gimp_display_shell_blink, shell);
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
  GimpDisplayConfig *config;
  GimpImageWindow   *window;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GIMP_IS_DISPLAY (shell->display));
  g_return_if_fail (GIMP_IS_IMAGE (image));

  config = shell->display->config;
  window = gimp_display_shell_get_window (shell);

  shell->dot_for_dot = config->default_dot_for_dot;

  gimp_display_shell_set_unit (shell, unit);
  gimp_display_shell_set_initial_scale (shell, scale, NULL, NULL);
  gimp_display_shell_scale_update (shell);

  /* center the image so subsequent stuff only moves it a little in
   * the center
   */
  gimp_display_shell_scroll_center_image (shell, TRUE, TRUE);

  gimp_display_shell_sync_config (shell, config);

  gimp_image_window_suspend_keep_pos (window);
  gimp_display_shell_appearance_update (shell);
  gimp_image_window_resume_keep_pos (window);

  gimp_image_window_update_tabs (window);
#if 0
  gimp_help_set_help_data (shell->canvas, NULL, NULL);
#endif

  gimp_statusbar_fill (GIMP_STATUSBAR (shell->statusbar));

  /* A size-allocate will always occur because the scrollbars will
   * become visible forcing the canvas to become smaller
   */
  gimp_display_shell_scroll_center_image_on_size_allocate (shell, TRUE, TRUE);

  if (shell->blink_timeout_id)
    {
      g_source_remove (shell->blink_timeout_id);
      shell->blink_timeout_id = 0;
    }

  shell->fill_idle_id =
    g_idle_add_full (GIMP_PRIORITY_DISPLAY_SHELL_FILL_IDLE,
                     (GSourceFunc) gimp_display_shell_fill_idle, shell,
                     NULL);
}

void
gimp_display_shell_scaled (GimpDisplayShell *shell)
{
  GList *list;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_rotate_update_transform (shell);

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

  gimp_display_shell_rotate_update_transform (shell);

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
gimp_display_shell_rotated (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  gimp_display_shell_rotate_update_transform (shell);

  g_signal_emit (shell, display_shell_signals[ROTATED], 0);
}

void
gimp_display_shell_set_unit (GimpDisplayShell *shell,
                             GimpUnit          unit)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->unit != unit)
    {
      shell->unit = unit;

      gimp_display_shell_rulers_update (shell);

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
                                gint             *x,
                                gint             *y,
                                gint             *width,
                                gint             *height)
{
  GimpImage *image;
  GimpLayer *layer;
  gint       x1, y1;
  gint       x2, y2;
  gdouble    x1_f, y1_f;
  gdouble    x2_f, y2_f;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), FALSE);
  g_return_val_if_fail (x != NULL, FALSE);
  g_return_val_if_fail (y != NULL, FALSE);
  g_return_val_if_fail (width != NULL, FALSE);
  g_return_val_if_fail (height != NULL, FALSE);

  image = gimp_display_get_image (shell->display);

  /*  If there is a floating selection, handle things differently  */
  if ((layer = gimp_image_get_floating_selection (image)))
    {
      gint fs_x;
      gint fs_y;
      gint fs_width;
      gint fs_height;

      gimp_item_get_offset (GIMP_ITEM (layer), &fs_x, &fs_y);
      fs_width  = gimp_item_get_width  (GIMP_ITEM (layer));
      fs_height = gimp_item_get_height (GIMP_ITEM (layer));

      if (! gimp_item_bounds (GIMP_ITEM (gimp_image_get_mask (image)),
                              x, y, width, height))
        {
          *x      = fs_x;
          *y      = fs_y;
          *width  = fs_width;
          *height = fs_height;
        }
      else
        {
          gimp_rectangle_union (*x, *y, *width, *height,
                                fs_x, fs_y, fs_width, fs_height,
                                x, y, width, height);
        }
    }
  else if (! gimp_item_bounds (GIMP_ITEM (gimp_image_get_mask (image)),
                               x, y, width, height))
    {
      return FALSE;
    }

  x1 = *x;
  y1 = *y;
  x2 = *x + *width;
  y2 = *y + *height;

  gimp_display_shell_transform_bounds (shell,
                                       x1, y1, x2, y2,
                                       &x1_f, &y1_f, &x2_f, &y2_f);

  /*  Make sure the extents are within bounds  */
  x1 = CLAMP (floor (x1_f), 0, shell->disp_width);
  y1 = CLAMP (floor (y1_f), 0, shell->disp_height);
  x2 = CLAMP (ceil (x2_f),  0, shell->disp_width);
  y2 = CLAMP (ceil (y2_f),  0, shell->disp_height);

  *x      = x1;
  *y      = y1;
  *width  = x2 - x1;
  *height = y2 - y1;

  return (*width > 0) && (*height > 0);
}

void
gimp_display_shell_flush (GimpDisplayShell *shell,
                          gboolean          now)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (now)
    {
      gdk_window_process_updates (gtk_widget_get_window (shell->canvas),
                                  FALSE);
    }
  else
    {
      GimpImageWindow *window = gimp_display_shell_get_window (shell);
      GimpContext     *context;

      gimp_display_shell_title_update (shell);

      gimp_canvas_layer_boundary_set_layer (GIMP_CANVAS_LAYER_BOUNDARY (shell->layer_boundary),
                                            gimp_image_get_active_layer (gimp_display_get_image (shell->display)));

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
 * @opacity:   how much to hide the unselected area
 *
 * This function sets an area of the image that should be
 * accentuated. The actual implementation is to dim all pixels outside
 * this rectangle. Passing %NULL for @highlight unsets the rectangle.
 **/
void
gimp_display_shell_set_highlight (GimpDisplayShell   *shell,
                                  const GdkRectangle *highlight,
                                  gdouble             opacity)
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
      g_object_set (shell->passe_partout, "opacity", opacity, NULL);

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
 * @inverted: #TRUE if the mask should be drawn inverted
 *
 * Previews a mask originating at offset_x, offset_x. Depending on
 * @inverted, pixels that are selected or not selected are tinted with
 * the given color.
 **/
void
gimp_display_shell_set_mask (GimpDisplayShell *shell,
                             GeglBuffer       *mask,
                             gint              offset_x,
                             gint              offset_y,
                             const GimpRGB    *color,
                             gboolean          inverted)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (mask == NULL || GEGL_IS_BUFFER (mask));
  g_return_if_fail (mask == NULL || color != NULL);

  if (mask)
    g_object_ref (mask);

  if (shell->mask)
    g_object_unref (shell->mask);

  shell->mask = mask;

  shell->mask_offset_x = offset_x;
  shell->mask_offset_y = offset_y;

  if (mask)
    shell->mask_color = *color;

  shell->mask_inverted = inverted;

  gimp_display_shell_expose_full (shell);
}
