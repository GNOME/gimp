/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmanavigationeditor.c
 * Copyright (C) 2001 Michael Natterer <mitch@ligma.org>
 *
 * partly based on app/nav_window
 * Copyright (C) 1999 Andy Thomas <alt@ligma.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "display-types.h"

#include "config/ligmadisplayconfig.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaimageproxy.h"

#include "widgets/ligmadocked.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmamenufactory.h"
#include "widgets/ligmanavigationview.h"
#include "widgets/ligmauimanager.h"
#include "widgets/ligmaviewrenderer.h"

#include "ligmadisplay.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-appearance.h"
#include "ligmadisplayshell-scale.h"
#include "ligmadisplayshell-scroll.h"
#include "ligmadisplayshell-transform.h"
#include "ligmanavigationeditor.h"

#include "ligma-intl.h"


#define UPDATE_DELAY 300 /* From GtkRange in GTK+ 2.22 */


static void        ligma_navigation_editor_docked_iface_init            (LigmaDockedInterface  *iface);

static void        ligma_navigation_editor_dispose                      (GObject              *object);

static void        ligma_navigation_editor_set_context                  (LigmaDocked           *docked,
                                                                        LigmaContext          *context);

static GtkWidget * ligma_navigation_editor_new_private                  (LigmaMenuFactory      *menu_factory,
                                                                        LigmaDisplayShell     *shell);

static void        ligma_navigation_editor_set_shell                    (LigmaNavigationEditor *editor,
                                                                        LigmaDisplayShell     *shell);
static gboolean    ligma_navigation_editor_button_release               (GtkWidget            *widget,
                                                                        GdkEventButton       *bevent,
                                                                        LigmaDisplayShell     *shell);
static void        ligma_navigation_editor_marker_changed               (LigmaNavigationView   *view,
                                                                        gdouble               center_x,
                                                                        gdouble               center_y,
                                                                        gdouble               width,
                                                                        gdouble               height,
                                                                        LigmaNavigationEditor *editor);
static void        ligma_navigation_editor_zoom                         (LigmaNavigationView   *view,
                                                                        LigmaZoomType          direction,
                                                                        gdouble               delta,
                                                                        LigmaNavigationEditor *editor);
static void        ligma_navigation_editor_scroll                       (LigmaNavigationView   *view,
                                                                        GdkEventScroll       *sevent,
                                                                        LigmaNavigationEditor *editor);

static void        ligma_navigation_editor_zoom_adj_changed             (GtkAdjustment        *adj,
                                                                        LigmaNavigationEditor *editor);

static void        ligma_navigation_editor_shell_infinite_canvas_notify (LigmaDisplayShell     *shell,
                                                                        const GParamSpec     *pspec,
                                                                        LigmaNavigationEditor *editor);
static void        ligma_navigation_editor_shell_scaled                 (LigmaDisplayShell     *shell,
                                                                        LigmaNavigationEditor *editor);
static void        ligma_navigation_editor_shell_scrolled               (LigmaDisplayShell     *shell,
                                                                        LigmaNavigationEditor *editor);
static void        ligma_navigation_editor_shell_rotated                (LigmaDisplayShell     *shell,
                                                                        LigmaNavigationEditor *editor);
static void        ligma_navigation_editor_shell_reconnect              (LigmaDisplayShell     *shell,
                                                                        LigmaNavigationEditor *editor);

static void        ligma_navigation_editor_viewable_size_changed        (LigmaViewable         *viewable,
                                                                        LigmaNavigationEditor *editor);

static void        ligma_navigation_editor_options_show_canvas_notify   (LigmaDisplayOptions   *options,
                                                                        const GParamSpec     *pspec,
                                                                        LigmaNavigationEditor *editor);

static void        ligma_navigation_editor_update_marker                (LigmaNavigationEditor *editor);


G_DEFINE_TYPE_WITH_CODE (LigmaNavigationEditor, ligma_navigation_editor,
                         LIGMA_TYPE_EDITOR,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED,
                                                ligma_navigation_editor_docked_iface_init))

#define parent_class ligma_navigation_editor_parent_class


static void
ligma_navigation_editor_class_init (LigmaNavigationEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = ligma_navigation_editor_dispose;
}

static void
ligma_navigation_editor_docked_iface_init (LigmaDockedInterface *iface)
{
  iface->set_context = ligma_navigation_editor_set_context;
}

static void
ligma_navigation_editor_init (LigmaNavigationEditor *editor)
{
  GtkWidget *frame;

  editor->context       = NULL;
  editor->shell         = NULL;
  editor->scale_timeout = 0;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  editor->view = ligma_view_new_by_types (NULL,
                                         LIGMA_TYPE_NAVIGATION_VIEW,
                                         LIGMA_TYPE_IMAGE_PROXY,
                                         LIGMA_VIEW_SIZE_MEDIUM, 0, TRUE);
  gtk_container_add (GTK_CONTAINER (frame), editor->view);
  gtk_widget_show (editor->view);

  g_signal_connect (editor->view, "marker-changed",
                    G_CALLBACK (ligma_navigation_editor_marker_changed),
                    editor);
  g_signal_connect (editor->view, "zoom",
                    G_CALLBACK (ligma_navigation_editor_zoom),
                    editor);
  g_signal_connect (editor->view, "scroll",
                    G_CALLBACK (ligma_navigation_editor_scroll),
                    editor);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
}

static void
ligma_navigation_editor_dispose (GObject *object)
{
  LigmaNavigationEditor *editor = LIGMA_NAVIGATION_EDITOR (object);

  if (editor->shell)
    ligma_navigation_editor_set_shell (editor, NULL);

  if (editor->scale_timeout)
    {
      g_source_remove (editor->scale_timeout);
      editor->scale_timeout = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_navigation_editor_display_changed (LigmaContext          *context,
                                        LigmaDisplay          *display,
                                        LigmaNavigationEditor *editor)
{
  LigmaDisplayShell *shell = NULL;

  if (display && ligma_display_get_image (display))
    shell = ligma_display_get_shell (display);

  ligma_navigation_editor_set_shell (editor, shell);
}

static void
ligma_navigation_editor_image_chaged (LigmaContext          *context,
                                     LigmaImage            *image,
                                     LigmaNavigationEditor *editor)
{
  LigmaDisplay      *display = ligma_context_get_display (context);
  LigmaDisplayShell *shell   = NULL;

  if (display && image)
    shell = ligma_display_get_shell (display);

  ligma_navigation_editor_set_shell (editor, shell);
}

static void
ligma_navigation_editor_set_context (LigmaDocked  *docked,
                                    LigmaContext *context)
{
  LigmaNavigationEditor *editor  = LIGMA_NAVIGATION_EDITOR (docked);
  LigmaDisplay          *display = NULL;

  if (editor->context)
    {
      g_signal_handlers_disconnect_by_func (editor->context,
                                            ligma_navigation_editor_display_changed,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->context,
                                            ligma_navigation_editor_image_chaged,
                                            editor);
    }

  editor->context = context;

  if (editor->context)
    {
      g_signal_connect (context, "display-changed",
                        G_CALLBACK (ligma_navigation_editor_display_changed),
                        editor);
      /* make sure to also call ligma_navigation_editor_set_shell() when the
       * last image is closed, even though the display isn't changed, so that
       * the editor is properly cleared.
       */
      g_signal_connect (context, "image-changed",
                        G_CALLBACK (ligma_navigation_editor_image_chaged),
                        editor);

      display = ligma_context_get_display (context);
    }

  ligma_view_renderer_set_context (LIGMA_VIEW (editor->view)->renderer,
                                  context);

  ligma_navigation_editor_display_changed (editor->context,
                                          display,
                                          editor);
}


/*  public functions  */

GtkWidget *
ligma_navigation_editor_new (LigmaMenuFactory *menu_factory)
{
  return ligma_navigation_editor_new_private (menu_factory, NULL);
}

void
ligma_navigation_editor_popup (LigmaDisplayShell *shell,
                              GtkWidget        *widget,
                              GdkEvent         *event,
                              gint              click_x,
                              gint              click_y)
{
  GtkStyleContext      *style = gtk_widget_get_style_context (widget);
  LigmaNavigationEditor *editor;
  LigmaNavigationView   *view;
  gint                  x, y;
  gint                  view_marker_center_x, view_marker_center_y;
  gint                  view_marker_width, view_marker_height;

  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  if (! shell->nav_popup)
    {
      GtkWidget *frame;

      shell->nav_popup = gtk_window_new (GTK_WINDOW_POPUP);

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (shell->nav_popup), frame);
      gtk_widget_show (frame);

      editor =
        LIGMA_NAVIGATION_EDITOR (ligma_navigation_editor_new_private (NULL,
                                                                    shell));
      gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (editor));
      gtk_widget_show (GTK_WIDGET (editor));

      g_signal_connect (editor->view, "button-release-event",
                        G_CALLBACK (ligma_navigation_editor_button_release),
                        shell);
    }
  else
    {
      GtkWidget *bin = gtk_bin_get_child (GTK_BIN (shell->nav_popup));

      editor = LIGMA_NAVIGATION_EDITOR (gtk_bin_get_child (GTK_BIN (bin)));
    }

  view = LIGMA_NAVIGATION_VIEW (editor->view);

  /* Set poup screen */
  gtk_window_set_screen (GTK_WINDOW (shell->nav_popup),
                         gtk_widget_get_screen (widget));

  ligma_navigation_view_get_local_marker (view,
                                         &view_marker_center_x,
                                         &view_marker_center_y,
                                         &view_marker_width,
                                         &view_marker_height);
  /* Position the popup */
  {
    GdkMonitor   *monitor;
    GdkRectangle  workarea;
    GtkBorder     border;
    gint          x_origin, y_origin;
    gint          popup_width, popup_height;
    gint          border_width, border_height;
    gint          screen_click_x, screen_click_y;

    monitor = ligma_widget_get_monitor (widget);
    gdk_monitor_get_workarea (monitor, &workarea);

    gdk_window_get_origin (gtk_widget_get_window (widget),
                           &x_origin, &y_origin);

    gtk_style_context_get_border (style, gtk_widget_get_state_flags (widget),
                                  &border);

    screen_click_x = x_origin + click_x;
    screen_click_y = y_origin + click_y;
    border_width   = 2 * border.left;
    border_height  = 2 * border.top;
    popup_width    = LIGMA_VIEW (view)->renderer->width  - 2 * border_width;
    popup_height   = LIGMA_VIEW (view)->renderer->height - 2 * border_height;

    x = screen_click_x -
        border_width -
        view_marker_center_x;

    y = screen_click_y -
        border_height -
        view_marker_center_y;

    /* When the image is zoomed out and overscrolled, the above
     * calculation risks positioning the popup far far away from the
     * click coordinate. We don't want that, so perform some clamping.
     */
    x = CLAMP (x, screen_click_x - popup_width,  screen_click_x);
    y = CLAMP (y, screen_click_y - popup_height, screen_click_y);

    /* If the popup doesn't fit into the screen, we have a problem.
     * We move the popup onscreen and risk that the pointer is not
     * in the square representing the viewable area anymore. Moving
     * the pointer will make the image scroll by a large amount,
     * but then it works as usual. Probably better than a popup that
     * is completely unusable in the lower right of the screen.
     *
     * Warping the pointer would be another solution ...
     */
    x = CLAMP (x, workarea.x, workarea.x + workarea.width  - popup_width);
    y = CLAMP (y, workarea.y, workarea.y + workarea.height - popup_height);

    gtk_window_move (GTK_WINDOW (shell->nav_popup), x, y);
  }

  gtk_widget_show (shell->nav_popup);
  gdk_display_flush (gtk_widget_get_display (shell->nav_popup));

  /* fill in then grab pointer */
  ligma_navigation_view_set_motion_offset (view, 0, 0);
  ligma_navigation_view_grab_pointer (view, event);
}


/*  private functions  */

static GtkWidget *
ligma_navigation_editor_new_private (LigmaMenuFactory  *menu_factory,
                                    LigmaDisplayShell *shell)
{
  LigmaNavigationEditor *editor;

  g_return_val_if_fail (menu_factory == NULL ||
                        LIGMA_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (shell == NULL || LIGMA_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (menu_factory || shell, NULL);

  if (shell)
    {
      Ligma              *ligma   = shell->display->ligma;
      LigmaDisplayConfig *config = shell->display->config;
      LigmaView          *view;

      editor = g_object_new (LIGMA_TYPE_NAVIGATION_EDITOR, NULL);

      view = LIGMA_VIEW (editor->view);

      ligma_view_renderer_set_size (view->renderer,
                                   config->nav_preview_size * 3,
                                   view->renderer->border_width);
      ligma_view_renderer_set_context (view->renderer,
                                      ligma_get_user_context (ligma));
      ligma_view_renderer_set_color_config (view->renderer,
                                           ligma_display_shell_get_color_config (shell));

      ligma_navigation_editor_set_shell (editor, shell);

    }
  else
    {
      GtkWidget *hscale;
      GtkWidget *hbox;

      editor = g_object_new (LIGMA_TYPE_NAVIGATION_EDITOR,
                             "menu-factory",    menu_factory,
                             "menu-identifier", "<NavigationEditor>",
                             NULL);

      gtk_widget_set_size_request (editor->view,
                                   LIGMA_VIEW_SIZE_HUGE,
                                   LIGMA_VIEW_SIZE_HUGE);
      ligma_view_set_expand (LIGMA_VIEW (editor->view), TRUE);

      /* the editor buttons */

      editor->zoom_out_button =
        ligma_editor_add_action_button (LIGMA_EDITOR (editor), "view",
                                       "view-zoom-out", NULL);

      editor->zoom_in_button =
        ligma_editor_add_action_button (LIGMA_EDITOR (editor), "view",
                                       "view-zoom-in", NULL);

      editor->zoom_100_button =
        ligma_editor_add_action_button (LIGMA_EDITOR (editor), "view",
                                       "view-zoom-1-1", NULL);

      editor->zoom_fit_in_button =
        ligma_editor_add_action_button (LIGMA_EDITOR (editor), "view",
                                       "view-zoom-fit-in", NULL);

      editor->zoom_fill_button =
        ligma_editor_add_action_button (LIGMA_EDITOR (editor), "view",
                                       "view-zoom-fill", NULL);

      editor->shrink_wrap_button =
        ligma_editor_add_action_button (LIGMA_EDITOR (editor), "view",
                                       "view-shrink-wrap", NULL);

      /* the zoom scale */

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_box_pack_end (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      editor->zoom_adjustment = gtk_adjustment_new (0.0, -8.0, 8.0,
                                                    0.5, 1.0, 0.0);

      g_signal_connect (editor->zoom_adjustment, "value-changed",
                        G_CALLBACK (ligma_navigation_editor_zoom_adj_changed),
                        editor);

      hscale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL,
                              editor->zoom_adjustment);
      gtk_scale_set_draw_value (GTK_SCALE (hscale), FALSE);
      gtk_box_pack_start (GTK_BOX (hbox), hscale, TRUE, TRUE, 0);
      gtk_widget_show (hscale);

      /* the zoom label */

      editor->zoom_label = gtk_label_new ("100%");
      gtk_label_set_width_chars (GTK_LABEL (editor->zoom_label), 7);
      gtk_box_pack_start (GTK_BOX (hbox), editor->zoom_label, FALSE, FALSE, 0);
      gtk_widget_show (editor->zoom_label);
    }

  ligma_view_renderer_set_background (LIGMA_VIEW (editor->view)->renderer,
                                     LIGMA_ICON_TEXTURE);

  return GTK_WIDGET (editor);
}

static void
ligma_navigation_editor_set_shell (LigmaNavigationEditor *editor,
                                  LigmaDisplayShell     *shell)
{
  g_return_if_fail (LIGMA_IS_NAVIGATION_EDITOR (editor));
  g_return_if_fail (! shell || LIGMA_IS_DISPLAY_SHELL (shell));

  if (shell == editor->shell)
    return;

  if (editor->shell)
    {
      g_signal_handlers_disconnect_by_func (editor->shell,
                                            ligma_navigation_editor_shell_infinite_canvas_notify,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->shell,
                                            ligma_navigation_editor_shell_scaled,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->shell,
                                            ligma_navigation_editor_shell_scrolled,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->shell,
                                            ligma_navigation_editor_shell_rotated,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->shell,
                                            ligma_navigation_editor_shell_reconnect,
                                            editor);

      g_signal_handlers_disconnect_by_func (editor->shell->options,
                                            ligma_navigation_editor_options_show_canvas_notify,
                                            editor);
      g_signal_handlers_disconnect_by_func (editor->shell->fullscreen_options,
                                            ligma_navigation_editor_options_show_canvas_notify,
                                            editor);
    }
  else if (shell)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
    }

  editor->shell = shell;

  if (editor->shell)
    {
      LigmaImage *image = ligma_display_get_image (shell->display);

      g_clear_object (&editor->image_proxy);

      if (image)
        {
          editor->image_proxy = ligma_image_proxy_new (image);

          g_signal_connect (
            editor->image_proxy, "size-changed",
            G_CALLBACK (ligma_navigation_editor_viewable_size_changed),
            editor);
        }

      ligma_view_set_viewable (LIGMA_VIEW (editor->view),
                              LIGMA_VIEWABLE (editor->image_proxy));

      g_signal_connect (editor->shell, "notify::infinite-canvas",
                        G_CALLBACK (ligma_navigation_editor_shell_infinite_canvas_notify),
                        editor);
      g_signal_connect (editor->shell, "scaled",
                        G_CALLBACK (ligma_navigation_editor_shell_scaled),
                        editor);
      g_signal_connect (editor->shell, "scrolled",
                        G_CALLBACK (ligma_navigation_editor_shell_scrolled),
                        editor);
      g_signal_connect (editor->shell, "rotated",
                        G_CALLBACK (ligma_navigation_editor_shell_rotated),
                        editor);
      g_signal_connect (editor->shell, "reconnect",
                        G_CALLBACK (ligma_navigation_editor_shell_reconnect),
                        editor);

      g_signal_connect (editor->shell->options, "notify::show-canvas-boundary",
                        G_CALLBACK (ligma_navigation_editor_options_show_canvas_notify),
                        editor);
      g_signal_connect (editor->shell->fullscreen_options, "notify::show-canvas-boundary",
                        G_CALLBACK (ligma_navigation_editor_options_show_canvas_notify),
                        editor);

      ligma_navigation_editor_shell_scaled (editor->shell, editor);
    }
  else
    {
      ligma_view_set_viewable (LIGMA_VIEW (editor->view), NULL);
      gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);

      g_clear_object (&editor->image_proxy);
    }

  if (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)))
    ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)),
                            ligma_editor_get_popup_data (LIGMA_EDITOR (editor)));
}

static gboolean
ligma_navigation_editor_button_release (GtkWidget        *widget,
                                       GdkEventButton   *bevent,
                                       LigmaDisplayShell *shell)
{
  if (bevent->button == 1)
    {
      gtk_widget_hide (shell->nav_popup);
    }

  return FALSE;
}

static void
ligma_navigation_editor_marker_changed (LigmaNavigationView   *view,
                                       gdouble               center_x,
                                       gdouble               center_y,
                                       gdouble               width,
                                       gdouble               height,
                                       LigmaNavigationEditor *editor)
{
  LigmaViewRenderer *renderer = LIGMA_VIEW (editor->view)->renderer;

  if (editor->shell)
    {
      if (ligma_display_get_image (editor->shell->display))
        {
          GeglRectangle bounding_box;

          bounding_box = ligma_image_proxy_get_bounding_box (
            LIGMA_IMAGE_PROXY (renderer->viewable));

          center_x += bounding_box.x;
          center_y += bounding_box.y;

          ligma_display_shell_scroll_center_image_xy (editor->shell,
                                                     center_x, center_y);
        }
    }
}

static void
ligma_navigation_editor_zoom (LigmaNavigationView   *view,
                             LigmaZoomType          direction,
                             gdouble               delta,
                             LigmaNavigationEditor *editor)
{
  g_return_if_fail (direction != LIGMA_ZOOM_TO);

  if (editor->shell)
    {
      if (ligma_display_get_image (editor->shell->display))
        ligma_display_shell_scale (editor->shell,
                                  direction,
                                  delta,
                                  LIGMA_ZOOM_FOCUS_BEST_GUESS);
    }
}

static void
ligma_navigation_editor_scroll (LigmaNavigationView   *view,
                               GdkEventScroll       *sevent,
                               LigmaNavigationEditor *editor)
{
  if (editor->shell)
    {
      gdouble value_x;
      gdouble value_y;

      ligma_scroll_adjustment_values (sevent,
                                     editor->shell->hsbdata,
                                     editor->shell->vsbdata,
                                     &value_x, &value_y);

      gtk_adjustment_set_value (editor->shell->hsbdata, value_x);
      gtk_adjustment_set_value (editor->shell->vsbdata, value_y);
    }
}

static gboolean
ligma_navigation_editor_zoom_adj_changed_timeout (gpointer data)
{
  LigmaNavigationEditor *editor = LIGMA_NAVIGATION_EDITOR (data);
  GtkAdjustment        *adj    = editor->zoom_adjustment;

  if (ligma_display_get_image (editor->shell->display))
    ligma_display_shell_scale (editor->shell,
                              LIGMA_ZOOM_TO,
                              pow (2.0, gtk_adjustment_get_value (adj)),
                              LIGMA_ZOOM_FOCUS_BEST_GUESS);

  editor->scale_timeout = 0;

  return FALSE;
}

static void
ligma_navigation_editor_zoom_adj_changed (GtkAdjustment        *adj,
                                         LigmaNavigationEditor *editor)
{
  if (editor->scale_timeout)
    g_source_remove (editor->scale_timeout);

  editor->scale_timeout =
    g_timeout_add (UPDATE_DELAY,
                   ligma_navigation_editor_zoom_adj_changed_timeout,
                   editor);
}

static void
ligma_navigation_editor_shell_infinite_canvas_notify (LigmaDisplayShell     *shell,
                                                     const GParamSpec     *pspec,
                                                     LigmaNavigationEditor *editor)
{
  ligma_navigation_editor_update_marker (editor);

  if (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)))
    ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)),
                            ligma_editor_get_popup_data (LIGMA_EDITOR (editor)));
}

static void
ligma_navigation_editor_shell_scaled (LigmaDisplayShell     *shell,
                                     LigmaNavigationEditor *editor)
{
  if (editor->zoom_label)
    {
      gchar *str;

      g_object_get (shell->zoom,
                    "percentage", &str,
                    NULL);
      gtk_label_set_text (GTK_LABEL (editor->zoom_label), str);
      g_free (str);
    }

  if (editor->zoom_adjustment)
    {
      gdouble val;

      val = log (ligma_zoom_model_get_factor (shell->zoom)) / G_LN2;

      g_signal_handlers_block_by_func (editor->zoom_adjustment,
                                       ligma_navigation_editor_zoom_adj_changed,
                                       editor);

      gtk_adjustment_set_value (editor->zoom_adjustment, val);

      g_signal_handlers_unblock_by_func (editor->zoom_adjustment,
                                         ligma_navigation_editor_zoom_adj_changed,
                                         editor);
    }

  ligma_navigation_editor_update_marker (editor);

  if (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)))
    ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)),
                            ligma_editor_get_popup_data (LIGMA_EDITOR (editor)));
}

static void
ligma_navigation_editor_shell_scrolled (LigmaDisplayShell     *shell,
                                       LigmaNavigationEditor *editor)
{
  ligma_navigation_editor_update_marker (editor);

  if (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)))
    ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)),
                            ligma_editor_get_popup_data (LIGMA_EDITOR (editor)));
}

static void
ligma_navigation_editor_shell_rotated (LigmaDisplayShell     *shell,
                                      LigmaNavigationEditor *editor)
{
  ligma_navigation_editor_update_marker (editor);

  if (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)))
    ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)),
                            ligma_editor_get_popup_data (LIGMA_EDITOR (editor)));
}

static void
ligma_navigation_editor_viewable_size_changed (LigmaViewable         *viewable,
                                              LigmaNavigationEditor *editor)
{
  ligma_navigation_editor_update_marker (editor);

  if (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)))
    ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)),
                            ligma_editor_get_popup_data (LIGMA_EDITOR (editor)));
}

static void
ligma_navigation_editor_options_show_canvas_notify (LigmaDisplayOptions   *options,
                                                   const GParamSpec     *pspec,
                                                   LigmaNavigationEditor *editor)
{
  ligma_navigation_editor_update_marker (editor);
}

static void
ligma_navigation_editor_shell_reconnect (LigmaDisplayShell     *shell,
                                        LigmaNavigationEditor *editor)
{
  LigmaImage *image = ligma_display_get_image (shell->display);

  g_clear_object (&editor->image_proxy);

  if (image)
    {
      editor->image_proxy = ligma_image_proxy_new (image);

      g_signal_connect (
        editor->image_proxy, "size-changed",
        G_CALLBACK (ligma_navigation_editor_viewable_size_changed),
        editor);
    }

  ligma_view_set_viewable (LIGMA_VIEW (editor->view),
                          LIGMA_VIEWABLE (editor->image_proxy));

  if (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)))
    ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)),
                            ligma_editor_get_popup_data (LIGMA_EDITOR (editor)));
}

static void
ligma_navigation_editor_update_marker (LigmaNavigationEditor *editor)
{
  LigmaViewRenderer *renderer = LIGMA_VIEW (editor->view)->renderer;
  LigmaDisplayShell *shell    = editor->shell;

  if (renderer->dot_for_dot != shell->dot_for_dot)
    ligma_view_renderer_set_dot_for_dot (renderer, shell->dot_for_dot);

  if (renderer->viewable)
    {
      LigmaNavigationView *view = LIGMA_NAVIGATION_VIEW (editor->view);
      LigmaImage          *image;
      GeglRectangle       bounding_box;
      gdouble             x, y;
      gdouble             w, h;

      image = ligma_image_proxy_get_image (
        LIGMA_IMAGE_PROXY (renderer->viewable));

      ligma_image_proxy_set_show_all (
        LIGMA_IMAGE_PROXY (renderer->viewable),
        ligma_display_shell_get_infinite_canvas (shell));

      bounding_box = ligma_image_proxy_get_bounding_box (
        LIGMA_IMAGE_PROXY (renderer->viewable));

      ligma_display_shell_scroll_get_viewport (shell, &x, &y, &w, &h);
      ligma_display_shell_untransform_xy_f (shell,
                                           shell->disp_width  / 2,
                                           shell->disp_height / 2,
                                           &x, &y);

      x -= bounding_box.x;
      y -= bounding_box.y;

      ligma_navigation_view_set_marker (view,
                                       x, y, w, h,
                                       shell->flip_horizontally,
                                       shell->flip_vertically,
                                       shell->rotate_angle);

      ligma_navigation_view_set_canvas (
        view,
        ligma_display_shell_get_infinite_canvas (shell) &&
        ligma_display_shell_get_show_canvas (shell),
        -bounding_box.x,              -bounding_box.y,
        ligma_image_get_width (image), ligma_image_get_height (image));
    }
}
