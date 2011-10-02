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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimage-quick-mask.h"

#include "widgets/gimpcairo.h"
#include "widgets/gimpuimanager.h"

#include "gimpcanvasitem.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-callbacks.h"
#include "gimpdisplayshell-draw.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-selection.h"
#include "gimpdisplayshell-title.h"
#include "gimpimagewindow.h"
#include "gimpnavigationeditor.h"


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

static void       gimp_display_shell_canvas_expose_image      (GimpDisplayShell *shell,
                                                               GdkEventExpose   *eevent,
                                                               cairo_t          *cr);
static void       gimp_display_shell_canvas_expose_drop_zone  (GimpDisplayShell *shell,
                                                               GdkEventExpose   *eevent,
                                                               cairo_t          *cr);


/*  public functions  */

void
gimp_display_shell_canvas_realize (GtkWidget        *canvas,
                                   GimpDisplayShell *shell)
{
  GimpCanvasPaddingMode padding_mode;
  GimpRGB               padding_color;
  GtkAllocation         allocation;

  gtk_widget_grab_focus (canvas);

  gimp_display_shell_get_padding (shell, &padding_mode, &padding_color);
  gimp_display_shell_set_padding (shell, padding_mode, &padding_color);

  gtk_widget_get_allocation (canvas, &allocation);

  gimp_display_shell_title_update (shell);

  shell->disp_width  = allocation.width;
  shell->disp_height = allocation.height;

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

void
gimp_display_shell_canvas_size_allocate (GtkWidget        *widget,
                                         GtkAllocation    *allocation,
                                         GimpDisplayShell *shell)
{
  /*  are we in destruction?  */
  if (! shell->display || ! gimp_display_get_shell (shell->display))
    return;

  if ((shell->disp_width  != allocation->width) ||
      (shell->disp_height != allocation->height))
    {
      if (shell->zoom_on_resize   &&
          shell->disp_width  > 64 &&
          shell->disp_height > 64 &&
          allocation->width  > 64 &&
          allocation->height > 64)
        {
          gdouble scale = gimp_zoom_model_get_factor (shell->zoom);
          gint    offset_x;
          gint    offset_y;

          /* FIXME: The code is a bit of a mess */

          /*  multiply the zoom_factor with the ratio of the new and
           *  old canvas diagonals
           */
          scale *= (sqrt (SQR (allocation->width) +
                          SQR (allocation->height)) /
                    sqrt (SQR (shell->disp_width) +
                          SQR (shell->disp_height)));

          offset_x = UNSCALEX (shell, shell->offset_x);
          offset_y = UNSCALEX (shell, shell->offset_y);

          gimp_zoom_model_zoom (shell->zoom, GIMP_ZOOM_TO, scale);

          shell->offset_x = SCALEX (shell, offset_x);
          shell->offset_y = SCALEY (shell, offset_y);
        }

      shell->disp_width  = allocation->width;
      shell->disp_height = allocation->height;

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

          gimp_display_shell_draw_get_scaled_image_size (shell, &sw, &sh);

          center_horizontally = sw <= shell->disp_width;
          center_vertically   = sh <= shell->disp_height;

          gimp_display_shell_scroll_center_image (shell,
                                                  center_horizontally,
                                                  center_vertically);

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

      /* Reset */
      shell->size_allocate_from_configure_event = FALSE;
    }
}

static gboolean
gimp_display_shell_is_double_buffered (GimpDisplayShell *shell)
{
  return TRUE; /* FIXME: repair this after cairo tool drawing is done */

  /*  always double-buffer if there are overlay children or a
   *  transform preview, or they will flicker badly. Also double
   *  buffer when we are editing paths.
   */
#if 0
  if (GIMP_OVERLAY_BOX (shell->canvas)->children    ||
      gimp_display_shell_get_show_transform (shell) ||
      GIMP_IS_VECTOR_TOOL (tool_manager_get_active (shell->display->gimp)))
    return TRUE;
#endif

  return FALSE;
}

gboolean
gimp_display_shell_canvas_expose (GtkWidget        *widget,
                                  GdkEventExpose   *eevent,
                                  GimpDisplayShell *shell)
{
  /*  are we in destruction?  */
  if (! shell->display || ! gimp_display_get_shell (shell->display))
    return TRUE;

  /*  ignore events on overlays  */
  if (eevent->window == gtk_widget_get_window (widget))
    {
      cairo_t *cr;

      if (gimp_display_get_image (shell->display))
        {
          if (gimp_display_shell_is_double_buffered (shell))
            gdk_window_begin_paint_region (eevent->window, eevent->region);
        }

      /*  create the cairo_t after enabling double buffering, or we
       *  will get the wrong window destination surface
       */
      cr = gdk_cairo_create (gtk_widget_get_window (shell->canvas));
      gdk_cairo_region (cr, eevent->region);
      cairo_clip (cr);

      if (gimp_display_get_image (shell->display))
        {
          gimp_display_shell_canvas_expose_image (shell, eevent, cr);
        }
      else
        {
          gimp_display_shell_canvas_expose_drop_zone (shell, eevent, cr);
        }

      cairo_destroy (cr);
    }

  return FALSE;
}

gboolean
gimp_display_shell_canvas_expose_after (GtkWidget        *widget,
                                        GdkEventExpose   *eevent,
                                        GimpDisplayShell *shell)
{
  /*  are we in destruction?  */
  if (! shell->display || ! gimp_display_get_shell (shell->display))
    return TRUE;

  /*  ignore events on overlays  */
  if (eevent->window == gtk_widget_get_window (widget))
    {
      if (gimp_display_get_image (shell->display))
        {
          if (gimp_display_shell_is_double_buffered (shell))
            gdk_window_end_paint (eevent->window);
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
          GimpUIManager *manager = gimp_image_window_get_ui_manager (window);

          gimp_ui_manager_ui_popup (manager,
                                    "/quick-mask-popup",
                                    GTK_WIDGET (shell),
                                    NULL, NULL, NULL, NULL);
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
      gimp_image_set_quick_mask_state (image, active);

      gimp_image_flush (image);
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
      gimp_navigation_editor_popup (shell, widget, bevent->x, bevent->y);
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
  if (! shell->scrolling)
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
  if (! shell->scrolling)
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

  gimp_display_shell_scroll_setup_hscrollbar (shell, value);

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

  gimp_display_shell_scroll_setup_vscrollbar (shell, value);

  g_object_thaw_notify (G_OBJECT (shell->vsbdata)); /* emits "changed" */

  return FALSE;
}

static void
gimp_display_shell_canvas_expose_image (GimpDisplayShell *shell,
                                        GdkEventExpose   *eevent,
                                        cairo_t          *cr)
{
  GdkRegion    *clear_region;
  GdkRegion    *image_region;
  GdkRectangle  image_rect;
  GdkRectangle *rects;
  gint          n_rects;
  gint          i;

  /*  first, clear the exposed part of the region that is outside the
   *  image, which is the exposed region minus the image rectangle
   */

  clear_region = gdk_region_copy (eevent->region);

  image_rect.x = - shell->offset_x;
  image_rect.y = - shell->offset_y;
  gimp_display_shell_draw_get_scaled_image_size (shell,
                                                 &image_rect.width,
                                                 &image_rect.height);
  image_region = gdk_region_rectangle (&image_rect);

  gdk_region_subtract (clear_region, image_region);
  gdk_region_destroy (image_region);

  if (! gdk_region_empty (clear_region))
    {
      gdk_region_get_rectangles (clear_region, &rects, &n_rects);

      for (i = 0; i < n_rects; i++)
        gdk_window_clear_area (gtk_widget_get_window (shell->canvas),
                               rects[i].x,
                               rects[i].y,
                               rects[i].width,
                               rects[i].height);

      g_free (rects);
    }

  /*  then, draw the exposed part of the region that is inside the
   *  image, which is the exposed region minus the region used for
   *  clearing above
   */

  image_region = gdk_region_copy (eevent->region);

  gdk_region_subtract (image_region, clear_region);
  gdk_region_destroy (clear_region);

  if (! gdk_region_empty (image_region))
    {
      cairo_save (cr);
      gimp_display_shell_draw_checkerboard (shell, cr,
                                            image_rect.x,
                                            image_rect.y,
                                            image_rect.width,
                                            image_rect.height);
      cairo_restore (cr);


      cairo_save (cr);
      gdk_region_get_rectangles (image_region, &rects, &n_rects);

      for (i = 0; i < n_rects; i++)
        gimp_display_shell_draw_image (shell, cr,
                                       rects[i].x,
                                       rects[i].y,
                                       rects[i].width,
                                       rects[i].height);

      g_free (rects);
      cairo_restore (cr);
    }

  gdk_region_destroy (image_region);

  /*  finally, draw all the remaining image window stuff on top
   */

  /* draw canvas items */
  gimp_canvas_item_draw (shell->canvas_item, cr);

  /* restart (and recalculate) the selection boundaries */
  gimp_display_shell_selection_restart (shell);
}

static void
gimp_display_shell_canvas_expose_drop_zone (GimpDisplayShell *shell,
                                            GdkEventExpose   *eevent,
                                            cairo_t          *cr)
{
  GdkRectangle *rects;
  gint          n_rects;
  gint          i;

  gdk_region_get_rectangles (eevent->region, &rects, &n_rects);

  for (i = 0; i < n_rects; i++)
    {
      gdk_window_clear_area (gtk_widget_get_window (shell->canvas),
                             rects[i].x,
                             rects[i].y,
                             rects[i].width,
                             rects[i].height);
    }

  g_free (rects);

  gimp_cairo_draw_drop_wilber (shell->canvas, cr);
}
