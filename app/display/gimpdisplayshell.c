/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "display-types.h"
#include "tools/tools-types.h"

#include "config/ligmacoreconfig.h"
#include "config/ligmadisplayconfig.h"
#include "config/ligmadisplayoptions.h"

#include "gegl/ligma-babl.h"

#include "core/ligma.h"
#include "core/ligma-utils.h"
#include "core/ligmachannel.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-grid.h"
#include "core/ligmaimage-guides.h"
#include "core/ligmaimage-snap.h"
#include "core/ligmapickable.h"
#include "core/ligmaprojectable.h"
#include "core/ligmaprojection.h"
#include "core/ligmatemplate.h"

#include "widgets/ligmadevices.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmauimanager.h"
#include "widgets/ligmawidgets-utils.h"

#include "tools/tool_manager.h"

#include "ligmacanvas.h"
#include "ligmacanvascanvasboundary.h"
#include "ligmacanvaslayerboundary.h"
#include "ligmadisplay.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-appearance.h"
#include "ligmadisplayshell-callbacks.h"
#include "ligmadisplayshell-cursor.h"
#include "ligmadisplayshell-dnd.h"
#include "ligmadisplayshell-expose.h"
#include "ligmadisplayshell-filter.h"
#include "ligmadisplayshell-handlers.h"
#include "ligmadisplayshell-items.h"
#include "ligmadisplayshell-profile.h"
#include "ligmadisplayshell-progress.h"
#include "ligmadisplayshell-render.h"
#include "ligmadisplayshell-rotate.h"
#include "ligmadisplayshell-rulers.h"
#include "ligmadisplayshell-scale.h"
#include "ligmadisplayshell-scroll.h"
#include "ligmadisplayshell-scrollbars.h"
#include "ligmadisplayshell-selection.h"
#include "ligmadisplayshell-title.h"
#include "ligmadisplayshell-tool-events.h"
#include "ligmadisplayshell-transform.h"
#include "ligmaimagewindow.h"
#include "ligmamotionbuffer.h"
#include "ligmastatusbar.h"

#include "about.h"
#include "ligma-log.h"
#include "ligma-priorities.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_POPUP_MANAGER,
  PROP_INITIAL_MONITOR,
  PROP_DISPLAY,
  PROP_UNIT,
  PROP_TITLE,
  PROP_STATUS,
  PROP_SHOW_ALL,
  PROP_INFINITE_CANVAS
};

enum
{
  SCALED,
  SCROLLED,
  ROTATED,
  RECONNECT,
  LAST_SIGNAL
};


typedef struct _LigmaDisplayShellOverlay LigmaDisplayShellOverlay;

struct _LigmaDisplayShellOverlay
{
  LigmaDisplayShell *shell;
  gdouble           image_x;
  gdouble           image_y;
  LigmaHandleAnchor  anchor;
  gint              spacing_x;
  gint              spacing_y;
};


/*  local function prototypes  */

static void      ligma_color_managed_iface_init     (LigmaColorManagedInterface *iface);

static void      ligma_display_shell_constructed    (GObject          *object);
static void      ligma_display_shell_dispose        (GObject          *object);
static void      ligma_display_shell_finalize       (GObject          *object);
static void      ligma_display_shell_set_property   (GObject          *object,
                                                    guint             property_id,
                                                    const GValue     *value,
                                                    GParamSpec       *pspec);
static void      ligma_display_shell_get_property   (GObject          *object,
                                                    guint             property_id,
                                                    GValue           *value,
                                                    GParamSpec       *pspec);

static void      ligma_display_shell_unrealize      (GtkWidget        *widget);
static void      ligma_display_shell_unmap          (GtkWidget        *widget);
static void      ligma_display_shell_screen_changed (GtkWidget        *widget,
                                                    GdkScreen        *previous);
static gboolean  ligma_display_shell_popup_menu     (GtkWidget        *widget);

static void      ligma_display_shell_real_scaled    (LigmaDisplayShell *shell);
static void      ligma_display_shell_real_scrolled  (LigmaDisplayShell *shell);
static void      ligma_display_shell_real_rotated   (LigmaDisplayShell *shell);

static const guint8 *
                 ligma_display_shell_get_icc_profile(LigmaColorManaged *managed,
                                                    gsize            *len);
static LigmaColorProfile *
               ligma_display_shell_get_color_profile(LigmaColorManaged *managed);
static void      ligma_display_shell_profile_changed(LigmaColorManaged *managed);
static void    ligma_display_shell_simulation_profile_changed
                                                   (LigmaColorManaged *managed);
static void    ligma_display_shell_simulation_intent_changed
                                                   (LigmaColorManaged *managed);
static void    ligma_display_shell_simulation_bpc_changed
                                                   (LigmaColorManaged *managed);
static void      ligma_display_shell_zoom_button_callback
                                                   (LigmaDisplayShell *shell,
                                                    GtkWidget        *zoom_button);
static void      ligma_display_shell_sync_config    (LigmaDisplayShell  *shell,
                                                    LigmaDisplayConfig *config);

static void    ligma_display_shell_overlay_allocate (GtkWidget        *child,
                                                    GtkAllocation    *allocation,
                                                    LigmaDisplayShellOverlay *overlay);
static void      ligma_display_shell_remove_overlay (GtkWidget        *canvas,
                                                    GtkWidget        *child,
                                                    LigmaDisplayShell *shell);
static void   ligma_display_shell_transform_overlay (LigmaDisplayShell *shell,
                                                    GtkWidget        *child,
                                                    gdouble          *x,
                                                    gdouble          *y);


G_DEFINE_TYPE_WITH_CODE (LigmaDisplayShell, ligma_display_shell,
                         GTK_TYPE_EVENT_BOX,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_PROGRESS,
                                                ligma_display_shell_progress_iface_init)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_COLOR_MANAGED,
                                                ligma_color_managed_iface_init))


#define parent_class ligma_display_shell_parent_class

static guint display_shell_signals[LAST_SIGNAL] = { 0 };


static void
ligma_display_shell_class_init (LigmaDisplayShellClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  display_shell_signals[SCALED] =
    g_signal_new ("scaled",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDisplayShellClass, scaled),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  display_shell_signals[SCROLLED] =
    g_signal_new ("scrolled",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDisplayShellClass, scrolled),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  display_shell_signals[ROTATED] =
    g_signal_new ("rotated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDisplayShellClass, rotated),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  display_shell_signals[RECONNECT] =
    g_signal_new ("reconnect",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDisplayShellClass, reconnect),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed        = ligma_display_shell_constructed;
  object_class->dispose            = ligma_display_shell_dispose;
  object_class->finalize           = ligma_display_shell_finalize;
  object_class->set_property       = ligma_display_shell_set_property;
  object_class->get_property       = ligma_display_shell_get_property;

  widget_class->unrealize          = ligma_display_shell_unrealize;
  widget_class->unmap              = ligma_display_shell_unmap;
  widget_class->screen_changed     = ligma_display_shell_screen_changed;
  widget_class->popup_menu         = ligma_display_shell_popup_menu;

  klass->scaled                    = ligma_display_shell_real_scaled;
  klass->scrolled                  = ligma_display_shell_real_scrolled;
  klass->rotated                   = ligma_display_shell_real_rotated;
  klass->reconnect                 = NULL;

  g_object_class_install_property (object_class, PROP_POPUP_MANAGER,
                                   g_param_spec_object ("popup-manager",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_UI_MANAGER,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_INITIAL_MONITOR,
                                   g_param_spec_object ("initial-monitor",
                                                        NULL, NULL,
                                                        GDK_TYPE_MONITOR,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DISPLAY,
                                   g_param_spec_object ("display", NULL, NULL,
                                                        LIGMA_TYPE_DISPLAY,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_UNIT,
                                   ligma_param_spec_unit ("unit", NULL, NULL,
                                                         TRUE, FALSE,
                                                         LIGMA_UNIT_PIXEL,
                                                         LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_TITLE,
                                   g_param_spec_string ("title", NULL, NULL,
                                                        LIGMA_NAME,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_STATUS,
                                   g_param_spec_string ("status", NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SHOW_ALL,
                                   g_param_spec_boolean ("show-all",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_INFINITE_CANVAS,
                                   g_param_spec_boolean ("infinite-canvas",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READABLE));

  gtk_widget_class_set_css_name (widget_class, "LigmaDisplayShell");
}

static void
ligma_color_managed_iface_init (LigmaColorManagedInterface *iface)
{
  iface->get_icc_profile            = ligma_display_shell_get_icc_profile;
  iface->get_color_profile          = ligma_display_shell_get_color_profile;
  iface->profile_changed            = ligma_display_shell_profile_changed;
  iface->simulation_profile_changed = ligma_display_shell_simulation_profile_changed;
  iface->simulation_intent_changed  = ligma_display_shell_simulation_intent_changed;
  iface->simulation_bpc_changed     = ligma_display_shell_simulation_bpc_changed;
}

static void
ligma_display_shell_init (LigmaDisplayShell *shell)
{
  const gchar *env;

  shell->options            = g_object_new (LIGMA_TYPE_DISPLAY_OPTIONS, NULL);
  shell->fullscreen_options = g_object_new (LIGMA_TYPE_DISPLAY_OPTIONS_FULLSCREEN, NULL);
  shell->no_image_options   = g_object_new (LIGMA_TYPE_DISPLAY_OPTIONS_NO_IMAGE, NULL);

  shell->zoom        = ligma_zoom_model_new ();
  shell->dot_for_dot = TRUE;
  shell->scale_x     = 1.0;
  shell->scale_y     = 1.0;

  shell->show_image  = TRUE;

  shell->show_all    = FALSE;

  ligma_display_shell_items_init (shell);

  shell->cursor_handedness = LIGMA_HANDEDNESS_RIGHT;
  shell->current_cursor    = (LigmaCursorType) -1;
  shell->tool_cursor       = LIGMA_TOOL_CURSOR_NONE;
  shell->cursor_modifier   = LIGMA_CURSOR_MODIFIER_NONE;
  shell->override_cursor   = (LigmaCursorType) -1;

  shell->filter_format     = babl_format ("R'G'B'A float");
  shell->filter_profile    = ligma_babl_get_builtin_color_profile (LIGMA_RGB,
                                                                  LIGMA_TRC_NON_LINEAR);

  shell->render_scale      = 1;

  shell->render_buf_width  = 256;
  shell->render_buf_height = 256;

  env = g_getenv ("LIGMA_DISPLAY_RENDER_BUF_SIZE");

  if (env)
    {
      gint width  = atoi (env);
      gint height = width;

      env = strchr (env, 'x');
      if (env)
        height = atoi (env + 1);

      if (width  > 0 && width  <= 8192 &&
          height > 0 && height <= 8192)
        {
          shell->render_buf_width  = width;
          shell->render_buf_height = height;
        }
    }

  shell->motion_buffer   = ligma_motion_buffer_new ();

  g_signal_connect (shell->motion_buffer, "stroke",
                    G_CALLBACK (ligma_display_shell_buffer_stroke),
                    shell);
  g_signal_connect (shell->motion_buffer, "hover",
                    G_CALLBACK (ligma_display_shell_buffer_hover),
                    shell);

  shell->zoom_focus_point = NULL;

  gtk_widget_set_events (GTK_WIDGET (shell), (GDK_POINTER_MOTION_MASK    |
                                              GDK_BUTTON_PRESS_MASK      |
                                              GDK_KEY_PRESS_MASK         |
                                              GDK_KEY_RELEASE_MASK       |
                                              GDK_FOCUS_CHANGE_MASK      |
                                              GDK_VISIBILITY_NOTIFY_MASK |
                                              GDK_SCROLL_MASK            |
                                              GDK_SMOOTH_SCROLL_MASK));

  /*  zoom model callback  */
  g_signal_connect_swapped (shell->zoom, "zoomed",
                            G_CALLBACK (ligma_display_shell_scale_update),
                            shell);

  /*  active display callback  */
  g_signal_connect (shell, "button-press-event",
                    G_CALLBACK (ligma_display_shell_events),
                    shell);
  g_signal_connect (shell, "button-release-event",
                    G_CALLBACK (ligma_display_shell_events),
                    shell);
  g_signal_connect (shell, "key-press-event",
                    G_CALLBACK (ligma_display_shell_events),
                    shell);

  ligma_help_connect (GTK_WIDGET (shell), ligma_standard_help_func,
                     LIGMA_HELP_IMAGE_WINDOW, NULL, NULL);
}

static void
ligma_display_shell_constructed (GObject *object)
{
  LigmaDisplayShell  *shell = LIGMA_DISPLAY_SHELL (object);
  LigmaDisplayConfig *config;
  LigmaImage         *image;
  GtkWidget         *grid;
  GtkWidget         *gtk_image;
  LigmaAction        *action;
  gint               image_width;
  gint               image_height;
  gint               shell_width;
  gint               shell_height;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_UI_MANAGER (shell->popup_manager));
  ligma_assert (LIGMA_IS_DISPLAY (shell->display));

  config = shell->display->config;
  image  = ligma_display_get_image (shell->display);

  ligma_display_shell_profile_init (shell);

  if (image)
    {
      image_width  = ligma_image_get_width  (image);
      image_height = ligma_image_get_height (image);
    }
  else
    {
      /* These values are arbitrary. The width is determined by the
       * menubar and the height is chosen to give a window aspect
       * ratio of roughly 3:1 (as requested by the UI team).
       */
      image_width  = LIGMA_DEFAULT_IMAGE_WIDTH;
      image_height = LIGMA_DEFAULT_IMAGE_HEIGHT / 3;
    }

  shell->dot_for_dot = config->default_dot_for_dot;

  if (config->monitor_res_from_gdk)
    {
      ligma_get_monitor_resolution (shell->initial_monitor,
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
      ligma_display_shell_set_initial_scale (shell, 1.0, //scale,
                                            &shell_width, &shell_height);
    }
  else
    {
      shell_width  = -1;
      shell_height = image_height;
    }

  ligma_display_shell_sync_config (shell, config);

  /*  the grid containing everything  */
  grid = gtk_grid_new ();
  gtk_container_add (GTK_CONTAINER (shell), grid);
  gtk_widget_show (grid);

  /*  the horizontal scrollbar  */
  shell->hsbdata = gtk_adjustment_new (0, 0, image_width,
                                       1, 1, image_width);
  shell->hsb = gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL, shell->hsbdata);
  gtk_widget_set_can_focus (shell->hsb, FALSE);

  /*  the vertical scrollbar  */
  shell->vsbdata = gtk_adjustment_new (0, 0, image_height,
                                       1, 1, image_height);
  shell->vsb = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, shell->vsbdata);
  gtk_widget_set_can_focus (shell->vsb, FALSE);

  /*  the menu popup button  */
  shell->origin = gtk_event_box_new ();
  gtk_image = gtk_image_new_from_icon_name (LIGMA_ICON_MENU_RIGHT,
                                            GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (shell->origin), gtk_image);
  gtk_widget_show (gtk_image);

  g_signal_connect (shell->origin, "button-press-event",
                    G_CALLBACK (ligma_display_shell_origin_button_press),
                    shell);

  ligma_help_set_help_data (shell->origin,
                           _("Access the image menu"),
                           LIGMA_HELP_IMAGE_WINDOW_ORIGIN);

  /*  the canvas  */
  shell->canvas = ligma_canvas_new (config);
  gtk_widget_set_size_request (shell->canvas, shell_width, shell_height);
  gtk_container_set_border_width (GTK_CONTAINER (shell->canvas), 10);

  g_signal_connect (shell->canvas, "remove",
                    G_CALLBACK (ligma_display_shell_remove_overlay),
                    shell);

  ligma_display_shell_dnd_init (shell);
  ligma_display_shell_selection_init (shell);

  shell->zoom_gesture = gtk_gesture_zoom_new (GTK_WIDGET (shell->canvas));
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (shell->zoom_gesture),
                                              GTK_PHASE_CAPTURE);
  shell->zoom_gesture_active = FALSE;

  shell->rotate_gesture = gtk_gesture_rotate_new (GTK_WIDGET (shell->canvas));
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (shell->rotate_gesture),
                                              GTK_PHASE_CAPTURE);
  shell->rotate_gesture_active = FALSE;

  /*  the horizontal ruler  */
  shell->hrule = ligma_ruler_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_events (GTK_WIDGET (shell->hrule),
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK);
  ligma_ruler_add_track_widget (LIGMA_RULER (shell->hrule), shell->canvas);

  g_signal_connect (shell->hrule, "button-press-event",
                    G_CALLBACK (ligma_display_shell_hruler_button_press),
                    shell);

  ligma_help_set_help_data (shell->hrule, NULL, LIGMA_HELP_IMAGE_WINDOW_RULER);

  /*  the vertical ruler  */
  shell->vrule = ligma_ruler_new (GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_events (GTK_WIDGET (shell->vrule),
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK);
  ligma_ruler_add_track_widget (LIGMA_RULER (shell->vrule), shell->canvas);

  g_signal_connect (shell->vrule, "button-press-event",
                    G_CALLBACK (ligma_display_shell_vruler_button_press),
                    shell);

  ligma_help_set_help_data (shell->vrule, NULL, LIGMA_HELP_IMAGE_WINDOW_RULER);

  /*  set the rulers as track widgets for each other, so we don't end up
   *  with one ruler wrongly being stuck a few pixels off while we are
   *  hovering the other
   */
  ligma_ruler_add_track_widget (LIGMA_RULER (shell->hrule), shell->vrule);
  ligma_ruler_add_track_widget (LIGMA_RULER (shell->vrule), shell->hrule);

  ligma_devices_add_widget (shell->display->ligma, shell->hrule);
  ligma_devices_add_widget (shell->display->ligma, shell->vrule);

  g_signal_connect (shell->canvas, "grab-notify",
                    G_CALLBACK (ligma_display_shell_canvas_grab_notify),
                    shell);

  g_signal_connect (shell->canvas, "realize",
                    G_CALLBACK (ligma_display_shell_canvas_realize),
                    shell);
  g_signal_connect (shell->canvas, "size-allocate",
                    G_CALLBACK (ligma_display_shell_canvas_size_allocate),
                    shell);
  g_signal_connect (shell->canvas, "draw",
                    G_CALLBACK (ligma_display_shell_canvas_draw),
                    shell);

  g_signal_connect (shell->canvas, "enter-notify-event",
                    G_CALLBACK (ligma_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "leave-notify-event",
                    G_CALLBACK (ligma_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "proximity-in-event",
                    G_CALLBACK (ligma_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "proximity-out-event",
                    G_CALLBACK (ligma_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "focus-in-event",
                    G_CALLBACK (ligma_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "focus-out-event",
                    G_CALLBACK (ligma_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "button-press-event",
                    G_CALLBACK (ligma_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "button-release-event",
                    G_CALLBACK (ligma_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "scroll-event",
                    G_CALLBACK (ligma_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "motion-notify-event",
                    G_CALLBACK (ligma_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "key-press-event",
                    G_CALLBACK (ligma_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->canvas, "key-release-event",
                    G_CALLBACK (ligma_display_shell_canvas_tool_events),
                    shell);
  g_signal_connect (shell->zoom_gesture, "begin",
                    G_CALLBACK (ligma_display_shell_zoom_gesture_begin),
                    shell);
  g_signal_connect (shell->zoom_gesture, "update",
                    G_CALLBACK (ligma_display_shell_zoom_gesture_update),
                    shell);
  g_signal_connect (shell->zoom_gesture, "end",
                    G_CALLBACK (ligma_display_shell_zoom_gesture_end),
                    shell);
  g_signal_connect (shell->rotate_gesture, "begin",
                    G_CALLBACK (ligma_display_shell_rotate_gesture_begin),
                    shell);
  g_signal_connect (shell->rotate_gesture, "update",
                    G_CALLBACK (ligma_display_shell_rotate_gesture_update),
                    shell);
  g_signal_connect (shell->rotate_gesture, "end",
                    G_CALLBACK (ligma_display_shell_rotate_gesture_end),
                    shell);

  /*  the zoom button  */
  shell->zoom_button = g_object_new (GTK_TYPE_CHECK_BUTTON,
                                     "draw-indicator", FALSE,
                                     "relief",         GTK_RELIEF_NONE,
                                     "width-request",  18,
                                     "height-request", 18,
                                     NULL);
  gtk_widget_set_can_focus (shell->zoom_button, FALSE);
  gtk_image = gtk_image_new_from_icon_name (LIGMA_ICON_ZOOM_FOLLOW_WINDOW,
                                            GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (shell->zoom_button), gtk_image);
  gtk_widget_show (gtk_image);

  g_signal_connect_swapped (shell->zoom_button, "toggled",
                            G_CALLBACK (ligma_display_shell_zoom_button_callback),
                            shell);

  ligma_help_set_help_data (shell->zoom_button,
                           _("Zoom image when window size changes"),
                           LIGMA_HELP_IMAGE_WINDOW_ZOOM_FOLLOW_BUTTON);

  /*  the quick mask button  */
  shell->quick_mask_button = g_object_new (GTK_TYPE_CHECK_BUTTON,
                                           "draw-indicator", FALSE,
                                           "relief",         GTK_RELIEF_NONE,
                                           "width-request",  18,
                                           "height-request", 18,
                                           NULL);
  gtk_widget_set_can_focus (shell->quick_mask_button, FALSE);
  gtk_image = gtk_image_new_from_icon_name (LIGMA_ICON_QUICK_MASK_OFF,
                                            GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (shell->quick_mask_button), gtk_image);
  gtk_widget_show (gtk_image);

  g_signal_connect (shell->quick_mask_button, "toggled",
                    G_CALLBACK (ligma_display_shell_quick_mask_toggled),
                    shell);
  g_signal_connect (shell->quick_mask_button, "button-press-event",
                    G_CALLBACK (ligma_display_shell_quick_mask_button_press),
                    shell);

  action = ligma_ui_manager_find_action (shell->popup_manager,
                                        "quick-mask", "quick-mask-toggle");
  if (action)
    ligma_widget_set_accel_help (shell->quick_mask_button, action);
  else
    ligma_help_set_help_data (shell->quick_mask_button,
                             _("Toggle Quick Mask"),
                             LIGMA_HELP_IMAGE_WINDOW_QUICK_MASK_BUTTON);

  /*  the navigation window button  */
  shell->nav_ebox = gtk_event_box_new ();
  gtk_image = gtk_image_new_from_icon_name (LIGMA_ICON_DIALOG_NAVIGATION,
                                            GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (shell->nav_ebox), gtk_image);
  gtk_widget_show (gtk_image);

  g_signal_connect (shell->nav_ebox, "button-press-event",
                    G_CALLBACK (ligma_display_shell_navigation_button_press),
                    shell);

  ligma_help_set_help_data (shell->nav_ebox,
                           _("Navigate the image display"),
                           LIGMA_HELP_IMAGE_WINDOW_NAV_BUTTON);

  /*  the statusbar  */
  shell->statusbar = ligma_statusbar_new ();
  ligma_statusbar_set_shell (LIGMA_STATUSBAR (shell->statusbar), shell);
  ligma_help_set_help_data (shell->statusbar, NULL,
                           LIGMA_HELP_IMAGE_WINDOW_STATUS_BAR);

  /*  pack all the widgets  */
  gtk_grid_attach (GTK_GRID (grid), shell->origin, 0, 0, 1, 1);

  gtk_widget_set_hexpand (shell->hrule, TRUE);
  gtk_grid_attach (GTK_GRID (grid), shell->hrule, 1, 0, 1, 1);

  gtk_widget_set_vexpand (shell->vrule, TRUE);
  gtk_grid_attach (GTK_GRID (grid), shell->vrule, 0, 1, 1, 1);

  gtk_widget_set_hexpand (shell->canvas, TRUE);
  gtk_widget_set_vexpand (shell->canvas, TRUE);
  gtk_grid_attach (GTK_GRID (grid), shell->canvas, 1, 1, 1, 1);

  gtk_grid_attach (GTK_GRID (grid), shell->zoom_button, 2, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), shell->quick_mask_button, 0, 2, 1, 1);

  gtk_grid_attach (GTK_GRID (grid), shell->vsb, 2, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), shell->hsb, 1, 2, 1, 1);

  gtk_grid_attach (GTK_GRID (grid), shell->nav_ebox, 2, 2, 1, 1);

  gtk_widget_set_hexpand (shell->statusbar, TRUE);
  gtk_grid_attach (GTK_GRID (grid), shell->statusbar, 0, 3, 3, 1);

  /*  show everything that is always shown */
  gtk_widget_show (GTK_WIDGET (shell->canvas));

  if (image)
    {
      ligma_display_shell_connect (shell);

      /* After connecting to the image we want to center it. Since we
       * not even finished creating the display shell, we can safely
       * assume we will get a size-allocate later.
       */
      shell->size_allocate_center_image = TRUE;
    }
  else
    {
#if 0
      /* Disabled because it sets GDK_POINTER_MOTION_HINT on
       * shell->canvas. For info see Bug 677375
       */
      ligma_help_set_help_data (shell->canvas,
                               _("Drop image files here to open them"),
                               NULL);
#endif

      ligma_statusbar_empty (LIGMA_STATUSBAR (shell->statusbar));
    }

  /* make sure the information is up-to-date */
  ligma_display_shell_scale_update (shell);

  ligma_display_shell_set_show_all (shell, config->default_show_all);
}

static void
ligma_display_shell_dispose (GObject *object)
{
  LigmaDisplayShell *shell = LIGMA_DISPLAY_SHELL (object);

  if (shell->display && ligma_display_get_shell (shell->display))
    ligma_display_shell_disconnect (shell);

  shell->popup_manager = NULL;

  if (shell->selection)
    ligma_display_shell_selection_free (shell);

  ligma_display_shell_filter_set (shell, NULL);

  if (shell->filter_idle_id)
    {
      g_source_remove (shell->filter_idle_id);
      shell->filter_idle_id = 0;
    }

  g_clear_object (&shell->zoom_gesture);
  g_clear_object (&shell->rotate_gesture);

  g_clear_pointer (&shell->render_cache,       cairo_surface_destroy);
  g_clear_pointer (&shell->render_cache_valid, cairo_region_destroy);

  g_clear_pointer (&shell->render_surface, cairo_surface_destroy);
  g_clear_pointer (&shell->mask_surface,   cairo_surface_destroy);
  g_clear_pointer (&shell->checkerboard,   cairo_pattern_destroy);

  ligma_display_shell_profile_finalize (shell);

  g_clear_object (&shell->filter_buffer);
  shell->filter_data   = NULL;
  shell->filter_stride = 0;

  g_clear_object (&shell->mask);

  ligma_display_shell_items_free (shell);

  g_clear_object (&shell->motion_buffer);

  if (shell->zoom_focus_point)
    {
      g_slice_free (GdkPoint, shell->zoom_focus_point);
      shell->zoom_focus_point = NULL;
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
ligma_display_shell_finalize (GObject *object)
{
  LigmaDisplayShell *shell = LIGMA_DISPLAY_SHELL (object);

  g_clear_object (&shell->zoom);
  g_clear_pointer (&shell->rotate_transform,   g_free);
  g_clear_pointer (&shell->rotate_untransform, g_free);
  g_clear_object (&shell->options);
  g_clear_object (&shell->fullscreen_options);
  g_clear_object (&shell->no_image_options);
  g_clear_pointer (&shell->title,  g_free);
  g_clear_pointer (&shell->status, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_display_shell_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  LigmaDisplayShell *shell = LIGMA_DISPLAY_SHELL (object);

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
      ligma_display_shell_set_unit (shell, g_value_get_int (value));
      break;
    case PROP_TITLE:
      g_free (shell->title);
      shell->title = g_value_dup_string (value);
      break;
    case PROP_STATUS:
      g_free (shell->status);
      shell->status = g_value_dup_string (value);
      break;
    case PROP_SHOW_ALL:
      ligma_display_shell_set_show_all (shell, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_display_shell_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  LigmaDisplayShell *shell = LIGMA_DISPLAY_SHELL (object);

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
    case PROP_SHOW_ALL:
      g_value_set_boolean (value, shell->show_all);
      break;
    case PROP_INFINITE_CANVAS:
      g_value_set_boolean (value,
                           ligma_display_shell_get_infinite_canvas (shell));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_display_shell_unrealize (GtkWidget *widget)
{
  LigmaDisplayShell *shell = LIGMA_DISPLAY_SHELL (widget);

  if (shell->nav_popup)
    gtk_widget_unrealize (shell->nav_popup);

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
ligma_display_shell_unmap (GtkWidget *widget)
{
  LigmaDisplayShell *shell = LIGMA_DISPLAY_SHELL (widget);

  ligma_display_shell_selection_undraw (shell);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
ligma_display_shell_screen_changed (GtkWidget *widget,
                                   GdkScreen *previous)
{
  LigmaDisplayShell *shell = LIGMA_DISPLAY_SHELL (widget);

  if (GTK_WIDGET_CLASS (parent_class)->screen_changed)
    GTK_WIDGET_CLASS (parent_class)->screen_changed (widget, previous);

  if (shell->display->config->monitor_res_from_gdk)
    {
      ligma_get_monitor_resolution (ligma_widget_get_monitor (widget),
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
ligma_display_shell_popup_menu (GtkWidget *widget)
{
  LigmaDisplayShell *shell = LIGMA_DISPLAY_SHELL (widget);

  ligma_context_set_display (ligma_get_user_context (shell->display->ligma),
                            shell->display);

  ligma_ui_manager_ui_popup_at_widget (shell->popup_manager,
                                      "/dummy-menubar/image-popup",
                                      shell->origin,
                                      GDK_GRAVITY_EAST,
                                      GDK_GRAVITY_NORTH_WEST,
                                      NULL,
                                      NULL, NULL);

  return TRUE;
}

static void
ligma_display_shell_real_scaled (LigmaDisplayShell *shell)
{
  LigmaContext *user_context;

  if (! shell->display)
    return;

  ligma_display_shell_title_update (shell);

  user_context = ligma_get_user_context (shell->display->ligma);

  if (shell->display == ligma_context_get_display (user_context))
    {
      ligma_display_shell_update_priority_rect (shell);

      ligma_ui_manager_update (shell->popup_manager, shell->display);
    }
}

static void
ligma_display_shell_real_scrolled (LigmaDisplayShell *shell)
{
  LigmaContext *user_context;

  if (! shell->display)
    return;

  ligma_display_shell_title_update (shell);

  user_context = ligma_get_user_context (shell->display->ligma);

  if (shell->display == ligma_context_get_display (user_context))
    {
      ligma_display_shell_update_priority_rect (shell);

    }
}

static void
ligma_display_shell_real_rotated (LigmaDisplayShell *shell)
{
  LigmaContext *user_context;

  if (! shell->display)
    return;

  ligma_display_shell_title_update (shell);

  user_context = ligma_get_user_context (shell->display->ligma);

  if (shell->display == ligma_context_get_display (user_context))
    {
      ligma_display_shell_update_priority_rect (shell);

      ligma_ui_manager_update (shell->popup_manager, shell->display);
    }
}

static const guint8 *
ligma_display_shell_get_icc_profile (LigmaColorManaged *managed,
                                    gsize            *len)
{
  LigmaDisplayShell *shell = LIGMA_DISPLAY_SHELL (managed);
  LigmaImage        *image = ligma_display_get_image (shell->display);

  if (image)
    return ligma_color_managed_get_icc_profile (LIGMA_COLOR_MANAGED (image), len);

  return NULL;
}

static LigmaColorProfile *
ligma_display_shell_get_color_profile (LigmaColorManaged *managed)
{
  LigmaDisplayShell *shell = LIGMA_DISPLAY_SHELL (managed);
  LigmaImage        *image = ligma_display_get_image (shell->display);

  if (image)
    return ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (image));

  return NULL;
}

static void
ligma_display_shell_profile_changed (LigmaColorManaged *managed)
{
  LigmaDisplayShell *shell = LIGMA_DISPLAY_SHELL (managed);

  ligma_display_shell_profile_update (shell);
  ligma_display_shell_expose_full (shell);
  ligma_display_shell_render_invalidate_full (shell);
}

static void
ligma_display_shell_simulation_profile_changed (LigmaColorManaged *managed)
{
  LigmaDisplayShell *shell = LIGMA_DISPLAY_SHELL (managed);

  ligma_display_shell_expose_full (shell);
  ligma_display_shell_render_invalidate_full (shell);
}

static void
ligma_display_shell_simulation_intent_changed (LigmaColorManaged *managed)
{
  LigmaDisplayShell *shell = LIGMA_DISPLAY_SHELL (managed);

  ligma_display_shell_expose_full (shell);
  ligma_display_shell_render_invalidate_full (shell);
}

static void
ligma_display_shell_simulation_bpc_changed (LigmaColorManaged *managed)
{
  LigmaDisplayShell *shell = LIGMA_DISPLAY_SHELL (managed);

  ligma_display_shell_expose_full (shell);
  ligma_display_shell_render_invalidate_full (shell);
}

static void
ligma_display_shell_zoom_button_callback (LigmaDisplayShell *shell,
                                         GtkWidget        *zoom_button)
{
  shell->zoom_on_resize =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (zoom_button));

  if (shell->zoom_on_resize &&
      ligma_display_shell_scale_image_is_within_viewport (shell, NULL, NULL))
    {
      /* Implicitly make a View -> Fit Image in Window */
      ligma_display_shell_scale_fit_in (shell);
    }
}

static void
ligma_display_shell_sync_config (LigmaDisplayShell  *shell,
                                LigmaDisplayConfig *config)
{
  ligma_config_sync (G_OBJECT (config->default_view),
                    G_OBJECT (shell->options), 0);
  ligma_config_sync (G_OBJECT (config->default_fullscreen_view),
                    G_OBJECT (shell->fullscreen_options), 0);
}

static void
ligma_display_shell_overlay_allocate (GtkWidget               *child,
                                     GtkAllocation           *allocation,
                                     LigmaDisplayShellOverlay *overlay)
{
  gdouble x, y;

  ligma_display_shell_transform_overlay (overlay->shell, child, &x, &y);

  ligma_overlay_box_set_child_position (LIGMA_OVERLAY_BOX (overlay->shell->canvas),
                                       child, x, y);
}

static void
ligma_display_shell_remove_overlay (GtkWidget        *canvas,
                                   GtkWidget        *child,
                                   LigmaDisplayShell *shell)
{
  LigmaDisplayShellOverlay *overlay;

  overlay = g_object_get_data (G_OBJECT (child), "image-coords-overlay");

  g_signal_handlers_disconnect_by_func (child,
                                        ligma_display_shell_overlay_allocate,
                                        overlay);

  shell->children = g_list_remove (shell->children, child);
}

static void
ligma_display_shell_transform_overlay (LigmaDisplayShell *shell,
                                      GtkWidget        *child,
                                      gdouble          *x,
                                      gdouble          *y)
{
  LigmaDisplayShellOverlay *overlay;
  GtkRequisition           requisition;

  overlay = g_object_get_data (G_OBJECT (child), "image-coords-overlay");

  ligma_display_shell_transform_xy_f (shell,
                                     overlay->image_x,
                                     overlay->image_y,
                                     x, y);

  gtk_widget_get_preferred_size (child, &requisition, NULL);

  switch (overlay->anchor)
    {
    case LIGMA_HANDLE_ANCHOR_CENTER:
      *x -= requisition.width  / 2;
      *y -= requisition.height / 2;
      break;

    case LIGMA_HANDLE_ANCHOR_NORTH:
      *x -= requisition.width / 2;
      *y += overlay->spacing_y;
      break;

    case LIGMA_HANDLE_ANCHOR_NORTH_WEST:
      *x += overlay->spacing_x;
      *y += overlay->spacing_y;
      break;

    case LIGMA_HANDLE_ANCHOR_NORTH_EAST:
      *x -= requisition.width + overlay->spacing_x;
      *y += overlay->spacing_y;
      break;

    case LIGMA_HANDLE_ANCHOR_SOUTH:
      *x -= requisition.width / 2;
      *y -= requisition.height + overlay->spacing_y;
      break;

    case LIGMA_HANDLE_ANCHOR_SOUTH_WEST:
      *x += overlay->spacing_x;
      *y -= requisition.height + overlay->spacing_y;
      break;

    case LIGMA_HANDLE_ANCHOR_SOUTH_EAST:
      *x -= requisition.width + overlay->spacing_x;
      *y -= requisition.height + overlay->spacing_y;
      break;

    case LIGMA_HANDLE_ANCHOR_WEST:
      *x += overlay->spacing_x;
      *y -= requisition.height / 2;
      break;

    case LIGMA_HANDLE_ANCHOR_EAST:
      *x -= requisition.width + overlay->spacing_x;
      *y -= requisition.height / 2;
      break;
    }
}


/*  public functions  */

GtkWidget *
ligma_display_shell_new (LigmaDisplay   *display,
                        LigmaUnit       unit,
                        gdouble        scale,
                        LigmaUIManager *popup_manager,
                        GdkMonitor    *monitor)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (LIGMA_IS_UI_MANAGER (popup_manager), NULL);
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);

  return g_object_new (LIGMA_TYPE_DISPLAY_SHELL,
                       "popup-manager",   popup_manager,
                       "initial-monitor", monitor,
                       "display",         display,
                       "unit",            unit,
                       NULL);
}

void
ligma_display_shell_add_overlay (LigmaDisplayShell *shell,
                                GtkWidget        *child,
                                gdouble           image_x,
                                gdouble           image_y,
                                LigmaHandleAnchor  anchor,
                                gint              spacing_x,
                                gint              spacing_y)
{
  LigmaDisplayShellOverlay *overlay;
  gdouble                  x, y;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GTK_IS_WIDGET (shell));

  overlay = g_new0 (LigmaDisplayShellOverlay, 1);

  overlay->shell     = shell;
  overlay->image_x   = image_x;
  overlay->image_y   = image_y;
  overlay->anchor    = anchor;
  overlay->spacing_x = spacing_x;
  overlay->spacing_y = spacing_y;

  g_object_set_data_full (G_OBJECT (child), "image-coords-overlay", overlay,
                          (GDestroyNotify) g_free);

  shell->children = g_list_prepend (shell->children, child);

  g_signal_connect (child, "size-allocate",
                    G_CALLBACK (ligma_display_shell_overlay_allocate),
                    overlay);

  ligma_display_shell_transform_overlay (shell, child, &x, &y);

  ligma_overlay_box_add_child (LIGMA_OVERLAY_BOX (shell->canvas), child, 0.0, 0.0);
  ligma_overlay_box_set_child_position (LIGMA_OVERLAY_BOX (shell->canvas),
                                       child, x, y);
}

void
ligma_display_shell_move_overlay (LigmaDisplayShell *shell,
                                 GtkWidget        *child,
                                 gdouble           image_x,
                                 gdouble           image_y,
                                 LigmaHandleAnchor  anchor,
                                 gint              spacing_x,
                                 gint              spacing_y)
{
  LigmaDisplayShellOverlay *overlay;
  gdouble                  x, y;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GTK_IS_WIDGET (shell));

  overlay = g_object_get_data (G_OBJECT (child), "image-coords-overlay");

  g_return_if_fail (overlay != NULL);

  overlay->image_x   = image_x;
  overlay->image_y   = image_y;
  overlay->anchor    = anchor;
  overlay->spacing_x = spacing_x;
  overlay->spacing_y = spacing_y;

  ligma_display_shell_transform_overlay (shell, child, &x, &y);

  ligma_overlay_box_set_child_position (LIGMA_OVERLAY_BOX (shell->canvas),
                                       child, x, y);
}

LigmaImageWindow *
ligma_display_shell_get_window (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return LIGMA_IMAGE_WINDOW (gtk_widget_get_ancestor (GTK_WIDGET (shell),
                                                     LIGMA_TYPE_IMAGE_WINDOW));
}

LigmaStatusbar *
ligma_display_shell_get_statusbar (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return LIGMA_STATUSBAR (shell->statusbar);
}

LigmaColorConfig *
ligma_display_shell_get_color_config (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return shell->color_config;
}

void
ligma_display_shell_present (LigmaDisplayShell *shell)
{
  LigmaImageWindow *window;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  window = ligma_display_shell_get_window (shell);

  if (window)
    {
      ligma_image_window_set_active_shell (window, shell);

      gtk_window_present (GTK_WINDOW (window));
    }
}

void
ligma_display_shell_reconnect (LigmaDisplayShell *shell)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (LIGMA_IS_DISPLAY (shell->display));
  g_return_if_fail (ligma_display_get_image (shell->display) != NULL);

  if (shell->fill_idle_id)
    {
      g_source_remove (shell->fill_idle_id);
      shell->fill_idle_id = 0;
    }

  g_signal_emit (shell, display_shell_signals[RECONNECT], 0);

  ligma_color_managed_profile_changed (LIGMA_COLOR_MANAGED (shell));
  ligma_display_shell_simulation_profile_changed (LIGMA_COLOR_MANAGED (shell));
  ligma_display_shell_simulation_intent_changed (LIGMA_COLOR_MANAGED (shell));
  ligma_display_shell_simulation_bpc_changed (LIGMA_COLOR_MANAGED (shell));

  ligma_display_shell_scroll_clamp_and_update (shell);

  ligma_display_shell_scaled (shell);

  ligma_display_shell_expose_full (shell);
  ligma_display_shell_render_invalidate_full (shell);
}

static gboolean
ligma_display_shell_blink (LigmaDisplayShell *shell)
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
        g_timeout_add (100, (GSourceFunc) ligma_display_shell_blink, shell);
    }

  ligma_display_shell_expose_full (shell);

  return FALSE;
}

void
ligma_display_shell_empty (LigmaDisplayShell *shell)
{
  LigmaContext     *user_context;
  LigmaImageWindow *window;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (LIGMA_IS_DISPLAY (shell->display));
  g_return_if_fail (ligma_display_get_image (shell->display) == NULL);

  window = ligma_display_shell_get_window (shell);

  if (shell->fill_idle_id)
    {
      g_source_remove (shell->fill_idle_id);
      shell->fill_idle_id = 0;
    }

  ligma_display_shell_selection_undraw (shell);

  ligma_display_shell_unset_cursor (shell);

  ligma_display_shell_filter_set (shell, NULL);

  ligma_display_shell_sync_config (shell, shell->display->config);

  ligma_display_shell_appearance_update (shell);
  ligma_image_window_update_tabs (window);
#if 0
  ligma_help_set_help_data (shell->canvas,
                           _("Drop image files here to open them"), NULL);
#endif

  ligma_statusbar_empty (LIGMA_STATUSBAR (shell->statusbar));

  shell->flip_horizontally = FALSE;
  shell->flip_vertically   = FALSE;
  shell->rotate_angle      = 0.0;
  ligma_display_shell_rotate_update_transform (shell);

  ligma_display_shell_expose_full (shell);
  ligma_display_shell_render_invalidate_full (shell);

  user_context = ligma_get_user_context (shell->display->ligma);

  if (shell->display == ligma_context_get_display (user_context))
    ligma_ui_manager_update (shell->popup_manager, shell->display);

  shell->blink_timeout_id =
    g_timeout_add (1403230, (GSourceFunc) ligma_display_shell_blink, shell);
}

static gboolean
ligma_display_shell_fill_idle (LigmaDisplayShell *shell)
{
  GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (shell));

  shell->fill_idle_id = 0;

  if (GTK_IS_WINDOW (toplevel))
    {
      ligma_display_shell_scale_shrink_wrap (shell, TRUE);

      gtk_window_present (GTK_WINDOW (toplevel));
    }

  return FALSE;
}

void
ligma_display_shell_fill (LigmaDisplayShell *shell,
                         LigmaImage        *image,
                         LigmaUnit          unit,
                         gdouble           scale)
{
  LigmaDisplayConfig *config;
  LigmaImageWindow   *window;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (LIGMA_IS_DISPLAY (shell->display));
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  config = shell->display->config;
  window = ligma_display_shell_get_window (shell);

  shell->show_image  = TRUE;

  shell->dot_for_dot = config->default_dot_for_dot;

  ligma_display_shell_set_unit (shell, unit);
  ligma_display_shell_set_initial_scale (shell, scale, NULL, NULL);
  ligma_display_shell_scale_update (shell);

  ligma_display_shell_sync_config (shell, config);

  ligma_image_window_suspend_keep_pos (window);
  ligma_display_shell_appearance_update (shell);
  ligma_image_window_resume_keep_pos (window);

  ligma_image_window_update_tabs (window);
#if 0
  ligma_help_set_help_data (shell->canvas, NULL, NULL);
#endif

  ligma_statusbar_fill (LIGMA_STATUSBAR (shell->statusbar));

  /* make sure a size-allocate always occurs, even when the rulers and
   * scrollbars are hidden.  see issue #4968.
   */
  shell->size_allocate_center_image = TRUE;
  gtk_widget_queue_resize (GTK_WIDGET (shell->canvas));

  if (shell->blink_timeout_id)
    {
      g_source_remove (shell->blink_timeout_id);
      shell->blink_timeout_id = 0;
    }

  shell->fill_idle_id =
    g_idle_add_full (LIGMA_PRIORITY_DISPLAY_SHELL_FILL_IDLE,
                     (GSourceFunc) ligma_display_shell_fill_idle, shell,
                     NULL);

  ligma_display_shell_set_show_all (shell, config->default_show_all);
}

void
ligma_display_shell_scaled (LigmaDisplayShell *shell)
{
  GList *list;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  ligma_display_shell_rotate_update_transform (shell);

  for (list = shell->children; list; list = g_list_next (list))
    {
      GtkWidget *child = list->data;
      gdouble    x, y;

      ligma_display_shell_transform_overlay (shell, child, &x, &y);

      ligma_overlay_box_set_child_position (LIGMA_OVERLAY_BOX (shell->canvas),
                                           child, x, y);
    }

  g_signal_emit (shell, display_shell_signals[SCALED], 0);
}

void
ligma_display_shell_scrolled (LigmaDisplayShell *shell)
{
  GList *list;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  ligma_display_shell_rotate_update_transform (shell);

  for (list = shell->children; list; list = g_list_next (list))
    {
      GtkWidget *child = list->data;
      gdouble    x, y;

      ligma_display_shell_transform_overlay (shell, child, &x, &y);

      ligma_overlay_box_set_child_position (LIGMA_OVERLAY_BOX (shell->canvas),
                                           child, x, y);
    }

  g_signal_emit (shell, display_shell_signals[SCROLLED], 0);
}

void
ligma_display_shell_rotated (LigmaDisplayShell *shell)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  ligma_display_shell_rotate_update_transform (shell);

  g_signal_emit (shell, display_shell_signals[ROTATED], 0);
}

void
ligma_display_shell_set_unit (LigmaDisplayShell *shell,
                             LigmaUnit          unit)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (shell->unit != unit)
    {
      shell->unit = unit;

      ligma_display_shell_rulers_update (shell);

      ligma_display_shell_scaled (shell);

      g_object_notify (G_OBJECT (shell), "unit");
    }
}

LigmaUnit
ligma_display_shell_get_unit (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), LIGMA_UNIT_PIXEL);

  return shell->unit;
}

gboolean
ligma_display_shell_snap_coords (LigmaDisplayShell *shell,
                                LigmaCoords       *coords,
                                gint              snap_offset_x,
                                gint              snap_offset_y,
                                gint              snap_width,
                                gint              snap_height)
{
  LigmaImage *image;
  gboolean   snap_to_guides  = FALSE;
  gboolean   snap_to_grid    = FALSE;
  gboolean   snap_to_canvas  = FALSE;
  gboolean   snap_to_vectors = FALSE;
  gboolean   snapped         = FALSE;

  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);

  image = ligma_display_get_image (shell->display);

  if (ligma_display_shell_get_snap_to_guides (shell) &&
      ligma_image_get_guides (image))
    {
      snap_to_guides = TRUE;
    }

  if (ligma_display_shell_get_snap_to_grid (shell) &&
      ligma_image_get_grid (image))
    {
      snap_to_grid = TRUE;
    }

  snap_to_canvas = ligma_display_shell_get_snap_to_canvas (shell);

  if (ligma_display_shell_get_snap_to_vectors (shell) &&
      ligma_image_get_selected_vectors (image))
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
          snapped = ligma_image_snap_rectangle (image,
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
          snapped = ligma_image_snap_point (image,
                                           coords->x + snap_offset_x,
                                           coords->y + snap_offset_y,
                                           &tx,
                                           &ty,
                                           FUNSCALEX (shell, snap_distance),
                                           FUNSCALEY (shell, snap_distance),
                                           snap_to_guides,
                                           snap_to_grid,
                                           snap_to_canvas,
                                           snap_to_vectors,
                                           shell->show_all);
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
ligma_display_shell_mask_bounds (LigmaDisplayShell *shell,
                                gint             *x,
                                gint             *y,
                                gint             *width,
                                gint             *height)
{
  LigmaImage *image;
  LigmaLayer *layer;
  gint       x1, y1;
  gint       x2, y2;
  gdouble    x1_f, y1_f;
  gdouble    x2_f, y2_f;

  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), FALSE);
  g_return_val_if_fail (x != NULL, FALSE);
  g_return_val_if_fail (y != NULL, FALSE);
  g_return_val_if_fail (width != NULL, FALSE);
  g_return_val_if_fail (height != NULL, FALSE);

  image = ligma_display_get_image (shell->display);

  /*  If there is a floating selection, handle things differently  */
  if ((layer = ligma_image_get_floating_selection (image)))
    {
      gint fs_x;
      gint fs_y;
      gint fs_width;
      gint fs_height;

      ligma_item_get_offset (LIGMA_ITEM (layer), &fs_x, &fs_y);
      fs_width  = ligma_item_get_width  (LIGMA_ITEM (layer));
      fs_height = ligma_item_get_height (LIGMA_ITEM (layer));

      if (! ligma_item_bounds (LIGMA_ITEM (ligma_image_get_mask (image)),
                              x, y, width, height))
        {
          *x      = fs_x;
          *y      = fs_y;
          *width  = fs_width;
          *height = fs_height;
        }
      else
        {
          ligma_rectangle_union (*x, *y, *width, *height,
                                fs_x, fs_y, fs_width, fs_height,
                                x, y, width, height);
        }
    }
  else if (! ligma_item_bounds (LIGMA_ITEM (ligma_image_get_mask (image)),
                               x, y, width, height))
    {
      return FALSE;
    }

  x1 = *x;
  y1 = *y;
  x2 = *x + *width;
  y2 = *y + *height;

  ligma_display_shell_transform_bounds (shell,
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
ligma_display_shell_set_show_image (LigmaDisplayShell *shell,
                                   gboolean          show_image)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (show_image != shell->show_image)
    {
      shell->show_image = show_image;

      ligma_display_shell_expose_full (shell);
    }
}

void
ligma_display_shell_set_show_all (LigmaDisplayShell *shell,
                                 gboolean          show_all)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (show_all != shell->show_all)
    {
      shell->show_all = show_all;

      if (shell->display && ligma_display_get_image (shell->display))
        {
          LigmaImage   *image = ligma_display_get_image (shell->display);
          LigmaContext *user_context;

          if (show_all)
            ligma_image_inc_show_all_count (image);
          else
            ligma_image_dec_show_all_count (image);

          ligma_image_flush (image);

          ligma_display_update_bounding_box (shell->display);

          ligma_display_shell_update_show_canvas (shell);

          ligma_display_shell_scroll_clamp_and_update (shell);
          ligma_display_shell_scrollbars_update (shell);

          ligma_display_shell_expose_full (shell);

          user_context = ligma_get_user_context (shell->display->ligma);

          if (shell->display == ligma_context_get_display (user_context))
            {
              ligma_display_shell_update_priority_rect (shell);

              ligma_ui_manager_update (shell->popup_manager, shell->display);
            }
        }

      g_object_notify (G_OBJECT (shell), "show-all");
      g_object_notify (G_OBJECT (shell), "infinite-canvas");
    }
}


LigmaPickable *
ligma_display_shell_get_pickable (LigmaDisplayShell *shell)
{
  LigmaImage *image;

  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  image = ligma_display_get_image (shell->display);

  if (image)
    {
      if (! shell->show_all)
        return LIGMA_PICKABLE (image);
      else
        return LIGMA_PICKABLE (ligma_image_get_projection (image));
    }

  return NULL;
}

LigmaPickable *
ligma_display_shell_get_canvas_pickable (LigmaDisplayShell *shell)
{
  LigmaImage *image;

  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  image = ligma_display_get_image (shell->display);

  if (image)
    {
      if (! ligma_display_shell_get_infinite_canvas (shell))
        return LIGMA_PICKABLE (image);
      else
        return LIGMA_PICKABLE (ligma_image_get_projection (image));
    }

  return NULL;
}

GeglRectangle
ligma_display_shell_get_bounding_box (LigmaDisplayShell *shell)
{
  GeglRectangle  bounding_box = {};
  LigmaImage     *image;

  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), bounding_box);

  image = ligma_display_get_image (shell->display);

  if (image)
    {
      if (! shell->show_all)
        {
          bounding_box.width  = ligma_image_get_width  (image);
          bounding_box.height = ligma_image_get_height (image);
        }
      else
        {
          bounding_box = ligma_projectable_get_bounding_box (
            LIGMA_PROJECTABLE (image));
        }
    }

  return bounding_box;
}

gboolean
ligma_display_shell_get_infinite_canvas (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), FALSE);

  return shell->show_all &&
         ! ligma_display_shell_get_padding_in_show_all (shell);
}

void
ligma_display_shell_update_priority_rect (LigmaDisplayShell *shell)
{
  LigmaImage *image;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  image = ligma_display_get_image (shell->display);

  if (image)
    {
      LigmaProjection *projection = ligma_image_get_projection (image);
      gint            x, y;
      gint            width, height;

      ligma_display_shell_untransform_viewport (shell, ! shell->show_all,
                                               &x, &y, &width, &height);
      ligma_projection_set_priority_rect (projection, x, y, width, height);
    }
}

void
ligma_display_shell_flush (LigmaDisplayShell *shell)
{
  LigmaImageWindow *window;
  LigmaContext     *context;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  window = ligma_display_shell_get_window (shell);

  ligma_display_shell_title_update (shell);

  ligma_canvas_layer_boundary_set_layers (LIGMA_CANVAS_LAYER_BOUNDARY (shell->layer_boundary),
                                         ligma_image_get_selected_layers (ligma_display_get_image (shell->display)));

  ligma_canvas_canvas_boundary_set_image (LIGMA_CANVAS_CANVAS_BOUNDARY (shell->canvas_boundary),
                                         ligma_display_get_image (shell->display));

  if (window && ligma_image_window_get_active_shell (window) == shell)
    {
      LigmaUIManager *manager = ligma_image_window_get_ui_manager (window);

      ligma_ui_manager_update (manager, shell->display);
    }

  context = ligma_get_user_context (shell->display->ligma);

  if (shell->display == ligma_context_get_display (context))
    {
      ligma_ui_manager_update (shell->popup_manager, shell->display);
    }
}

/**
 * ligma_display_shell_pause:
 * @shell: a display shell
 *
 * This function increments the pause count or the display shell.
 * If it was zero coming in, then the function pauses the active tool,
 * so that operations on the display can take place without corrupting
 * anything that the tool has drawn.  It "undraws" the current tool
 * drawing, and must be followed by ligma_display_shell_resume() after
 * the operation in question is completed.
 **/
void
ligma_display_shell_pause (LigmaDisplayShell *shell)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  shell->paused_count++;

  if (shell->paused_count == 1)
    {
      /*  pause the currently active tool  */
      tool_manager_control_active (shell->display->ligma,
                                   LIGMA_TOOL_ACTION_PAUSE,
                                   shell->display);
    }
}

/**
 * ligma_display_shell_resume:
 * @shell: a display shell
 *
 * This function decrements the pause count for the display shell.
 * If this brings it to zero, then the current tool is resumed.
 * It is an error to call this function without having previously
 * called ligma_display_shell_pause().
 **/
void
ligma_display_shell_resume (LigmaDisplayShell *shell)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (shell->paused_count > 0);

  shell->paused_count--;

  if (shell->paused_count == 0)
    {
      /* start the currently active tool */
      tool_manager_control_active (shell->display->ligma,
                                   LIGMA_TOOL_ACTION_RESUME,
                                   shell->display);
    }
}

/**
 * ligma_display_shell_set_highlight:
 * @shell:     a #LigmaDisplayShell
 * @highlight: a rectangle in image coordinates that should be brought out
 * @opacity:   how much to hide the unselected area
 *
 * This function sets an area of the image that should be
 * accentuated. The actual implementation is to dim all pixels outside
 * this rectangle. Passing %NULL for @highlight unsets the rectangle.
 **/
void
ligma_display_shell_set_highlight (LigmaDisplayShell   *shell,
                                  const GdkRectangle *highlight,
                                  gdouble             opacity)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));

  if (highlight)
    {
      ligma_canvas_item_begin_change (shell->passe_partout);

      ligma_canvas_rectangle_set (shell->passe_partout,
                                 highlight->x,
                                 highlight->y,
                                 highlight->width,
                                 highlight->height);
      g_object_set (shell->passe_partout, "opacity", opacity, NULL);

      ligma_canvas_item_set_visible (shell->passe_partout, TRUE);

      ligma_canvas_item_end_change (shell->passe_partout);
    }
  else
    {
      ligma_canvas_item_set_visible (shell->passe_partout, FALSE);
    }
}

/**
 * ligma_display_shell_set_mask:
 * @shell: a #LigmaDisplayShell
 * @mask:  a #LigmaDrawable (1 byte per pixel)
 * @color: the color to use for drawing the mask
 * @inverted: %TRUE if the mask should be drawn inverted
 *
 * Previews a mask originating at offset_x, offset_x. Depending on
 * @inverted, pixels that are selected or not selected are tinted with
 * the given color.
 **/
void
ligma_display_shell_set_mask (LigmaDisplayShell *shell,
                             GeglBuffer       *mask,
                             gint              offset_x,
                             gint              offset_y,
                             const LigmaRGB    *color,
                             gboolean          inverted)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));
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

  ligma_display_shell_expose_full (shell);
  ligma_display_shell_render_invalidate_full (shell);
}
