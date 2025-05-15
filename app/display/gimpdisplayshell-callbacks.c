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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimage-quick-mask.h"

#include "menus/menus.h"

#include "widgets/gimpcairo-wilber.h"
#include "widgets/gimpmenufactory.h"
#include "widgets/gimpuimanager.h"

#include "gimpcanvasitem.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-callbacks.h"
#include "gimpdisplayshell-draw.h"
#include "gimpdisplayshell-render.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-scrollbars.h"
#include "gimpdisplayshell-selection.h"
#include "gimpdisplayshell-title.h"
#include "gimpdisplayshell-transform.h"
#include "gimpimagewindow.h"
#include "gimpnavigationeditor.h"

#include "git-version.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void       gimp_display_shell_vadjustment_changed      (GtkAdjustment    *adjustment,
                                                               GimpDisplayShell *shell);
static void       gimp_display_shell_hadjustment_changed      (GtkAdjustment    *adjustment,
                                                               GimpDisplayShell *shell);
static gboolean   gimp_display_shell_vscrollbar_change_value  (GtkRange         *range,
                                                               GtkScrollType     scroll,
                                                               gdouble           value,
                                                               GimpDisplayShell *shell);

static gboolean   gimp_display_shell_hscrollbar_change_value  (GtkRange         *range,
                                                               GtkScrollType     scroll,
                                                               gdouble           value,
                                                               GimpDisplayShell *shell);

static void       gimp_display_shell_canvas_draw_image        (GimpDisplayShell *shell,
                                                               cairo_t          *cr);
static void       gimp_display_shell_canvas_draw_drop_zone    (GimpDisplayShell *shell,
                                                               cairo_t          *cr);


/*  public functions  */

void
gimp_display_shell_canvas_realize (GtkWidget        *canvas,
                                   GimpDisplayShell *shell)
{
  GimpCanvasPaddingMode  padding_mode;
  GeglColor             *padding_color;
  GtkAllocation          allocation;

  gtk_widget_grab_focus (canvas);

  gimp_display_shell_get_padding (shell, &padding_mode, &padding_color);
  gimp_display_shell_set_padding (shell, padding_mode, padding_color);
  g_clear_object (&padding_color);

  gtk_widget_get_allocation (canvas, &allocation);

  gimp_display_shell_title_update (shell);

  shell->disp_width  = allocation.width;
  shell->disp_height = allocation.height;

  gimp_display_shell_render_set_scale (
    shell,
    gdk_window_get_scale_factor (
      gtk_widget_get_window (gtk_widget_get_toplevel (canvas))));

  /*  set up the scrollbar observers  */
  g_signal_connect (shell->hsbdata, "value-changed",
                    G_CALLBACK (gimp_display_shell_hadjustment_changed),
                    shell);
  g_signal_connect (shell->vsbdata, "value-changed",
                    G_CALLBACK (gimp_display_shell_vadjustment_changed),
                    shell);

  g_signal_connect (shell->hsb, "change-value",
                    G_CALLBACK (gimp_display_shell_hscrollbar_change_value),
                    shell);

  g_signal_connect (shell->vsb, "change-value",
                    G_CALLBACK (gimp_display_shell_vscrollbar_change_value),
                    shell);

  /*  allow shrinking  */
  gtk_widget_set_size_request (GTK_WIDGET (shell), 0, 0);
}

typedef struct
{
  GimpDisplayShell *shell;
  gint              prev_width;
  gint              prev_height;
} TickClosure;

static gboolean
gimp_display_shell_canvas_tick (GtkWidget     *widget,
                                GdkFrameClock *frame_clock,
                                TickClosure   *tick)
{
  GimpDisplayShell *shell = tick->shell;
  GtkAllocation     allocation;

  gtk_widget_get_allocation (widget, &allocation);

  if ((tick->prev_width  != allocation.width) ||
      (tick->prev_height != allocation.height))
    {
      if (shell->zoom_on_resize  &&
          tick->prev_width  > 64 &&
          tick->prev_height > 64 &&
          allocation.width  > 64 &&
          allocation.height > 64)
        {
          gdouble scale = gimp_zoom_model_get_factor (shell->zoom);
          gint    offset_x;
          gint    offset_y;

          /* FIXME: The code is a bit of a mess */

          /*  multiply the zoom_factor with the ratio of the new and
           *  old canvas diagonals
           */
          scale *= (sqrt (SQR (allocation.width) +
                          SQR (allocation.height)) /
                    sqrt (SQR (tick->prev_width) +
                          SQR (tick->prev_height)));

          offset_x = UNSCALEX (shell, shell->offset_x);
          offset_y = UNSCALEY (shell, shell->offset_y);

          gimp_zoom_model_zoom (shell->zoom, GIMP_ZOOM_TO, scale);

          shell->offset_x = SCALEX (shell, offset_x);
          shell->offset_y = SCALEY (shell, offset_y);
        }

      /* When we size-allocate due to resize of the top level window,
       * we want some additional logic. Don't apply it on
       * zoom_on_resize though.
       */
      if (shell->size_allocate_from_configure_event &&
          ! shell->zoom_on_resize)
        {
          gboolean center_horizontally;
          gboolean center_vertically;
          gint     target_offset_x;
          gint     target_offset_y;
          gint     sw;
          gint     sh;

          gimp_display_shell_scale_get_image_size (shell, &sw, &sh);

          center_horizontally = sw <= shell->disp_width;
          center_vertically   = sh <= shell->disp_height;

          if (! gimp_display_shell_get_infinite_canvas (shell))
            {
              gimp_display_shell_scroll_center_image (shell,
                                                      center_horizontally,
                                                      center_vertically);
            }
          else
            {
              gimp_display_shell_scroll_center_content (shell,
                                                        center_horizontally,
                                                        center_vertically);
            }

          /* This is basically the best we can do before we get an
           * API for storing the image offset at the start of an
           * image window resize using the mouse
           */
          target_offset_x = shell->offset_x;
          target_offset_y = shell->offset_y;

          if (! center_horizontally)
            {
              target_offset_x = MAX (shell->offset_x, 0);
            }

          if (! center_vertically)
            {
              target_offset_y = MAX (shell->offset_y, 0);
            }

          gimp_display_shell_scroll_set_offset (shell,
                                                target_offset_x,
                                                target_offset_y);
        }

      gimp_display_shell_scroll_clamp_and_update (shell);
      gimp_display_shell_scaled (shell);

      shell->size_allocate_from_configure_event = FALSE;
    }

  if (shell->size_allocate_center_image)
    {
      gimp_display_shell_scroll_center_image (shell, TRUE, TRUE);

      shell->size_allocate_center_image = FALSE;
    }

  /* undo size request from gimp_display_shell_constructed() */
  gtk_widget_set_size_request (widget, -1, -1);

  return G_SOURCE_REMOVE;
}

void
gimp_display_shell_canvas_size_allocate (GtkWidget        *widget,
                                         GtkAllocation    *allocation,
                                         GimpDisplayShell *shell)
{
  TickClosure *tick;

  /*  are we in destruction?  */
  if (! shell->display || ! gimp_display_get_shell (shell->display) || ! gimp_display_get_image (shell->display))
    return;

  tick = g_new0 (TickClosure, 1);

  tick->shell       = shell;
  tick->prev_width  = shell->disp_width;
  tick->prev_height = shell->disp_height;

  if (shell->disp_width  != allocation->width ||
      shell->disp_height != allocation->height)
    {
      g_clear_pointer (&shell->render_cache, cairo_surface_destroy);
      gimp_display_shell_render_invalidate_full (shell);

      shell->disp_width  = allocation->width;
      shell->disp_height = allocation->height;
    }

  gtk_widget_add_tick_callback (widget,
                                (GtkTickCallback) gimp_display_shell_canvas_tick,
                                tick, (GDestroyNotify) g_free);
}

gboolean
gimp_display_shell_canvas_draw (GtkWidget        *widget,
                                cairo_t          *cr,
                                GimpDisplayShell *shell)
{
  /*  are we in destruction?  */
  if (! shell->display || ! gimp_display_get_shell (shell->display))
    return TRUE;

  /*  we will scroll around in the next tick anyway, so we just can as
   *  well skip the drawing of this frame and wait for the next
   */
  if (shell->size_allocate_center_image)
    return TRUE;

  /*  ignore events on overlays  */
  if (gtk_cairo_should_draw_window (cr, gtk_widget_get_window (widget)))
    {
      GimpImage *image = gimp_display_get_image (shell->display);

      /* If we are currently converting the image, it might be in inconsistent
       * state and should not be redrawn.
       */
      if (image != NULL && ! gimp_image_get_converting (image))
        {
          gimp_display_shell_canvas_draw_image (shell, cr);
        }
      else if (image == NULL)
        {
          gimp_display_shell_canvas_draw_drop_zone (shell, cr);
        }
    }

  return FALSE;
}

gboolean
gimp_display_shell_origin_button_press (GtkWidget        *widget,
                                        GdkEventButton   *event,
                                        GimpDisplayShell *shell)
{
  if (! shell->display->gimp->busy)
    {
      if (event->type == GDK_BUTTON_PRESS && event->button == 1)
        {
          gboolean unused;

          g_signal_emit_by_name (shell, "popup-menu", &unused);
        }
    }

  /* Return TRUE to stop signal emission so the button doesn't grab the
   * pointer away from us.
   */
  return TRUE;
}

gboolean
gimp_display_shell_quick_mask_button_press (GtkWidget        *widget,
                                            GdkEventButton   *bevent,
                                            GimpDisplayShell *shell)
{
  if (! gimp_display_get_image (shell->display))
    return TRUE;

  if (gdk_event_triggers_context_menu ((GdkEvent *) bevent))
    {
      GimpImageWindow *window = gimp_display_shell_get_window (shell);

      if (window)
        {
          Gimp          *gimp;
          GimpUIManager *manager;

          gimp    = shell->display->gimp;
          manager = gimp_menu_factory_get_manager (menus_get_global_menu_factory (gimp), "<QuickMask>", gimp);
          gimp_ui_manager_ui_popup_at_widget (manager,
                                              "/quick-mask-popup",
                                              NULL, NULL,
                                              widget,
                                              GDK_GRAVITY_EAST,
                                              GDK_GRAVITY_SOUTH_WEST,
                                              (GdkEvent *) bevent,
                                              NULL, NULL);
        }

      return TRUE;
    }

  return FALSE;
}

void
gimp_display_shell_quick_mask_toggled (GtkWidget        *widget,
                                       GimpDisplayShell *shell)
{
  GimpImage *image  = gimp_display_get_image (shell->display);
  gboolean   active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  if (active != gimp_image_get_quick_mask_state (image))
    {
      GimpImageWindow *window = gimp_display_shell_get_window (shell);

      if (window)
        {
          GimpUIManager *manager = menus_get_image_manager_singleton (shell->display->gimp);

          gimp_ui_manager_toggle_action (manager,
                                         "quick-mask", "quick-mask-toggle",
                                         active);
        }
    }
}

gboolean
gimp_display_shell_navigation_button_press (GtkWidget        *widget,
                                            GdkEventButton   *bevent,
                                            GimpDisplayShell *shell)
{
  if (! gimp_display_get_image (shell->display))
    return TRUE;

  if (bevent->type == GDK_BUTTON_PRESS && bevent->button == 1)
    {
      gimp_navigation_editor_popup (shell, widget,
                                    (GdkEvent *) bevent,
                                    bevent->x, bevent->y);
    }

  return TRUE;
}


/*  private functions  */

static void
gimp_display_shell_vadjustment_changed (GtkAdjustment    *adjustment,
                                        GimpDisplayShell *shell)
{
  /*  If we are panning with mouse, scrollbars are to be ignored or
   *  they will cause jitter in motion
   */
  if (shell->mod_action == GIMP_MODIFIER_ACTION_NONE)
    gimp_display_shell_scroll (shell,
                               0,
                               gtk_adjustment_get_value (adjustment) -
                               shell->offset_y);
}

static void
gimp_display_shell_hadjustment_changed (GtkAdjustment    *adjustment,
                                        GimpDisplayShell *shell)
{
  /* If we are panning with mouse, scrollbars are to be ignored or
   * they will cause jitter in motion
   */
  if (shell->mod_action == GIMP_MODIFIER_ACTION_NONE)
    gimp_display_shell_scroll (shell,
                               gtk_adjustment_get_value (adjustment) -
                               shell->offset_x,
                               0);
}

static gboolean
gimp_display_shell_hscrollbar_change_value (GtkRange         *range,
                                            GtkScrollType     scroll,
                                            gdouble           value,
                                            GimpDisplayShell *shell)
{
  if (! shell->display)
    return TRUE;

  if ((scroll == GTK_SCROLL_JUMP)          ||
      (scroll == GTK_SCROLL_PAGE_BACKWARD) ||
      (scroll == GTK_SCROLL_PAGE_FORWARD))
    return FALSE;

  g_object_freeze_notify (G_OBJECT (shell->hsbdata));

  gimp_display_shell_scrollbars_setup_horizontal (shell, value);

  g_object_thaw_notify (G_OBJECT (shell->hsbdata)); /* emits "changed" */

  return FALSE;
}

static gboolean
gimp_display_shell_vscrollbar_change_value (GtkRange         *range,
                                            GtkScrollType     scroll,
                                            gdouble           value,
                                            GimpDisplayShell *shell)
{
  if (! shell->display)
    return TRUE;

  if ((scroll == GTK_SCROLL_JUMP)          ||
      (scroll == GTK_SCROLL_PAGE_BACKWARD) ||
      (scroll == GTK_SCROLL_PAGE_FORWARD))
    return FALSE;

  g_object_freeze_notify (G_OBJECT (shell->vsbdata));

  gimp_display_shell_scrollbars_setup_vertical (shell, value);

  g_object_thaw_notify (G_OBJECT (shell->vsbdata)); /* emits "changed" */

  return FALSE;
}

static void
gimp_display_shell_canvas_draw_image (GimpDisplayShell *shell,
                                      cairo_t          *cr)
{
  cairo_rectangle_list_t *clip_rectangles;
  GeglRectangle           image_rect;
  GeglRectangle           rotated_image_rect;
  GeglRectangle           canvas_rect;
  cairo_matrix_t          matrix;
  gdouble                 x1, y1;
  gdouble                 x2, y2;

  gimp_display_shell_scale_get_image_unrotated_bounding_box (
    shell,
    &image_rect.x,
    &image_rect.y,
    &image_rect.width,
    &image_rect.height);

  gimp_display_shell_scale_get_image_unrotated_bounds (
    shell,
    &canvas_rect.x,
    &canvas_rect.y,
    &canvas_rect.width,
    &canvas_rect.height);

  /*  first, draw the background
   */

  gimp_display_shell_draw_background (shell, cr);


  /*  then, draw the exposed part of the region that is inside the
   *  image
   */

  cairo_save (cr);
  clip_rectangles = cairo_copy_clip_rectangle_list (cr);
  cairo_get_matrix (cr, &matrix);

  if (shell->rotate_transform)
    cairo_transform (cr, shell->rotate_transform);

  if (shell->show_all)
    {
      cairo_save (cr);

      if (gimp_display_shell_get_padding_in_show_all (shell))
        {
          cairo_rectangle (cr,
                           canvas_rect.x,
                           canvas_rect.y,
                           canvas_rect.width,
                           canvas_rect.height);
          cairo_clip (cr);
        }

      gimp_display_shell_draw_checkerboard (shell, cr);

      cairo_restore (cr);
    }

  cairo_rectangle (cr,
                   image_rect.x,
                   image_rect.y,
                   image_rect.width,
                   image_rect.height);
  cairo_clip (cr);

  gimp_display_shell_rotate_bounds (shell,
                                    image_rect.x,
                                    image_rect.y,
                                    image_rect.x + image_rect.width,
                                    image_rect.y + image_rect.height,
                                    &x1, &y1, &x2, &y2);

  rotated_image_rect.x      = floor (x1);
  rotated_image_rect.y      = floor (y1);
  rotated_image_rect.width  = ceil  (x2) - rotated_image_rect.x;
  rotated_image_rect.height = ceil  (y2) - rotated_image_rect.y;

  if (gdk_cairo_get_clip_rectangle (cr, NULL))
    {
      gint i;

      if (! shell->show_all)
        {
          cairo_save (cr);
          gimp_display_shell_draw_checkerboard (shell, cr);
          cairo_restore (cr);
        }

      if (shell->show_image)
        {
          cairo_set_matrix (cr, &matrix);

          for (i = 0; i < clip_rectangles->num_rectangles; i++)
            {
              cairo_rectangle_t clip_rect = clip_rectangles->rectangles[i];
              GeglRectangle     rect;

              rect.x      = floor (clip_rect.x);
              rect.y      = floor (clip_rect.y);
              rect.width  = ceil  (clip_rect.x + clip_rect.width)  - rect.x;
              rect.height = ceil  (clip_rect.y + clip_rect.height) - rect.y;

              if (gegl_rectangle_intersect (&rect, &rect, &rotated_image_rect))
                {
                  gimp_display_shell_draw_image (shell, cr,
                                                 rect.x,     rect.y,
                                                 rect.width, rect.height);
                }
            }
        }
    }

  cairo_rectangle_list_destroy (clip_rectangles);
  cairo_restore (cr);


  /*  finally, draw all the remaining image window stuff on top
   */

  /* draw canvas items */
  cairo_save (cr);

  if (shell->rotate_transform)
    cairo_transform (cr, shell->rotate_transform);

  gimp_canvas_item_draw (shell->canvas_item, cr);

  cairo_restore (cr);

  gimp_canvas_item_draw (shell->unrotated_item, cr);

  /* restart (and recalculate) the selection boundaries */
  gimp_display_shell_selection_draw (shell, cr);
  gimp_display_shell_selection_restart (shell);
}

static void
gimp_display_shell_canvas_draw_drop_zone (GimpDisplayShell *shell,
                                          cairo_t          *cr)
{
  cairo_save (cr);

  gimp_display_shell_draw_background (shell, cr);

  gimp_cairo_draw_drop_wilber (shell->canvas, cr, shell->blink);

  cairo_restore (cr);

#ifdef GIMP_UNSTABLE
  {
    GtkWidget       *widget  = GTK_WIDGET (shell);
    GtkStyleContext *context = gtk_widget_get_style_context (widget);
    GtkStateFlags    state   = gtk_widget_get_state_flags (widget);
    PangoLayout     *layout;
    gchar           *msg;
    GtkAllocation    allocation;
    gint             width;
    gint             height;
    gdouble          scale;
    GdkRGBA          color;

    layout = gtk_widget_create_pango_layout (shell->canvas, NULL);

    msg = g_strdup_printf (_("<big>Unstable Development Version</big>\n\n"
                             "<small>commit <tt>%s</tt></small>\n\n"
                             "<small>Please test bugs against "
                             "latest git master branch\n"
                             "before reporting them.</small>"),
                             GIMP_GIT_VERSION_ABBREV);
    pango_layout_set_markup (layout, msg, -1);
    g_free (msg);
    pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);

    pango_layout_get_pixel_size (layout, &width, &height);
    gtk_widget_get_allocation (shell->canvas, &allocation);

    scale = MIN (((gdouble) allocation.width  / 2.0) / (gdouble) width,
                 ((gdouble) allocation.height / 2.0) / (gdouble) height);

    gtk_style_context_get_color (context, state, &color);
    gdk_cairo_set_source_rgba (cr, &color);

    cairo_move_to (cr,
                   (allocation.width  - (width  * scale)) / 2,
                   (allocation.height - (height * scale)) / 2);

    cairo_scale (cr, scale, scale);

    pango_cairo_show_layout (cr, layout);

    g_object_unref (layout);
  }
#endif /* GIMP_UNSTABLE */
}
