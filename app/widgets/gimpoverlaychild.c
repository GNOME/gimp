/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoverlaychild.c
 * Copyright (C) 2009 Michael Natterer <mitch@gimp.org>
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

#include <libgimpmath/gimpmath.h>

#include "widgets-types.h"

#include "core/gimp-utils.h"

#include "gimpoverlaybox.h"
#include "gimpoverlaychild.h"
#include "gimpwidgets-utils.h"


/*  local function prototypes  */

static void   gimp_overlay_child_transform_bounds (GimpOverlayChild *child,
                                                   GdkRectangle     *bounds_child,
                                                   GdkRectangle     *bounds_box);
static void   gimp_overlay_child_from_embedder    (GdkWindow        *child_window,
                                                   gdouble           box_x,
                                                   gdouble           box_y,
                                                   gdouble          *child_x,
                                                   gdouble          *child_y,
                                                   GimpOverlayChild *child);
static void   gimp_overlay_child_to_embedder      (GdkWindow        *child_window,
                                                   gdouble           child_x,
                                                   gdouble           child_y,
                                                   gdouble          *box_x,
                                                   gdouble          *box_y,
                                                   GimpOverlayChild *child);


/*  public functions  */

GimpOverlayChild *
gimp_overlay_child_new (GimpOverlayBox *box,
                        GtkWidget      *widget,
                        gdouble         xalign,
                        gdouble         yalign,
                        gdouble         angle,
                        gdouble         opacity)
{
  GimpOverlayChild *child;

  g_return_val_if_fail (GIMP_IS_OVERLAY_BOX (box), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  child = g_slice_new0 (GimpOverlayChild);

  child->widget       = widget;
  child->xalign       = CLAMP (xalign, 0.0, 1.0);
  child->yalign       = CLAMP (yalign, 0.0, 1.0);
  child->x            = 0.0;
  child->y            = 0.0;
  child->has_position = FALSE;
  child->angle        = angle;
  child->opacity      = CLAMP (opacity, 0.0, 1.0);

  cairo_matrix_init_identity (&child->matrix);

  if (gtk_widget_get_realized (GTK_WIDGET (box)))
    gimp_overlay_child_realize (box, child);

  gtk_widget_set_parent (widget, GTK_WIDGET (box));

  return child;
}

void
gimp_overlay_child_free (GimpOverlayBox   *box,
                         GimpOverlayChild *child)
{
  g_return_if_fail (GIMP_IS_OVERLAY_BOX (box));
  g_return_if_fail (child != NULL);

  gtk_widget_unparent (child->widget);

  if (gtk_widget_get_realized (GTK_WIDGET (box)))
    gimp_overlay_child_unrealize (box, child);

  g_slice_free (GimpOverlayChild, child);
}

GimpOverlayChild *
gimp_overlay_child_find (GimpOverlayBox *box,
                         GtkWidget      *widget)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_OVERLAY_BOX (box), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (gtk_widget_get_parent (widget) == GTK_WIDGET (box),
                        NULL);

  for (list = box->children; list; list = g_list_next (list))
    {
      GimpOverlayChild *child = list->data;

      if (child->widget == widget)
        return child;
    }

  return NULL;
}

void
gimp_overlay_child_realize (GimpOverlayBox   *box,
                            GimpOverlayChild *child)
{
  GtkWidget     *widget;
  GdkDisplay    *display;
  GdkScreen     *screen;
  GdkVisual     *visual;
  GtkAllocation  child_allocation;
  GdkWindowAttr  attributes;
  gint           attributes_mask;

  g_return_if_fail (GIMP_IS_OVERLAY_BOX (box));
  g_return_if_fail (child != NULL);
  g_return_if_fail (child->window == NULL);

  widget = GTK_WIDGET (box);

  display = gtk_widget_get_display (widget);
  screen  = gtk_widget_get_screen (widget);

  visual = gdk_screen_get_rgba_visual (screen);
  if (visual)
    gtk_widget_set_visual (child->widget, visual);

  gtk_widget_get_allocation (child->widget, &child_allocation);

  if (gtk_widget_get_visible (child->widget))
    {
      attributes.width  = child_allocation.width;
      attributes.height = child_allocation.height;
    }
  else
    {
      attributes.width  = 1;
      attributes.height = 1;
    }

  attributes.x           = child_allocation.x;
  attributes.y           = child_allocation.y;
  attributes.window_type = GDK_WINDOW_OFFSCREEN;
  attributes.wclass      = GDK_INPUT_OUTPUT;
  attributes.visual      = gtk_widget_get_visual (child->widget);
  attributes.event_mask  = (gtk_widget_get_events (widget) |
                            GDK_EXPOSURE_MASK);
  attributes.cursor      = gdk_cursor_new_for_display (display, GDK_LEFT_PTR);

  attributes_mask = (GDK_WA_X        |
                     GDK_WA_Y        |
                     GDK_WA_VISUAL   |
                     GDK_WA_CURSOR);

  child->window = gdk_window_new (gtk_widget_get_root_window (widget),
                                  &attributes, attributes_mask);
  gdk_window_set_user_data (child->window, widget);
  gtk_widget_set_parent_window (child->widget, child->window);
  gdk_offscreen_window_set_embedder (child->window,
                                     gtk_widget_get_window (widget));

  g_object_unref (attributes.cursor);

  g_signal_connect (child->window, "from-embedder",
                    G_CALLBACK (gimp_overlay_child_from_embedder),
                    child);
  g_signal_connect (child->window, "to-embedder",
                    G_CALLBACK (gimp_overlay_child_to_embedder),
                    child);

  gtk_style_set_background (gtk_widget_get_style (widget),
                            child->window, GTK_STATE_NORMAL);
  gdk_window_show (child->window);
}

void
gimp_overlay_child_unrealize (GimpOverlayBox   *box,
                              GimpOverlayChild *child)
{
  g_return_if_fail (GIMP_IS_OVERLAY_BOX (box));
  g_return_if_fail (child != NULL);
  g_return_if_fail (child->window != NULL);

  gdk_window_set_user_data (child->window, NULL);
  gdk_window_destroy (child->window);
  child->window = NULL;
}

void
gimp_overlay_child_get_preferred_width (GimpOverlayBox   *box,
                                        GimpOverlayChild *child)
{
  gint minimum;
  gint natural;

  g_return_if_fail (GIMP_IS_OVERLAY_BOX (box));
  g_return_if_fail (child != NULL);

  gtk_widget_get_preferred_width (child->widget, &minimum, &natural);
}

void
gimp_overlay_child_get_preferred_height (GimpOverlayBox   *box,
                                         GimpOverlayChild *child)
{
  gint minimum;
  gint natural;

  g_return_if_fail (GIMP_IS_OVERLAY_BOX (box));
  g_return_if_fail (child != NULL);

  gtk_widget_get_preferred_height (child->widget, &minimum, &natural);
}

void
gimp_overlay_child_size_allocate (GimpOverlayBox   *box,
                                  GimpOverlayChild *child)
{
  GtkWidget      *widget;
  GtkRequisition  child_requisition;
  GtkAllocation   child_allocation;
  gint            x;
  gint            y;

  g_return_if_fail (GIMP_IS_OVERLAY_BOX (box));
  g_return_if_fail (child != NULL);

  widget = GTK_WIDGET (box);

  gimp_overlay_child_invalidate (box, child);

  gtk_widget_get_preferred_size (child->widget, &child_requisition, NULL);

  child_allocation.x      = 0;
  child_allocation.y      = 0;
  child_allocation.width  = child_requisition.width;
  child_allocation.height = child_requisition.height;

  gtk_widget_size_allocate (child->widget, &child_allocation);

  if (gtk_widget_get_realized (GTK_WIDGET (widget)))
    gdk_window_move_resize (child->window,
                            child_allocation.x,
                            child_allocation.y,
                            child_allocation.width,
                            child_allocation.height);

  cairo_matrix_init_identity (&child->matrix);

  /* local transform */
  cairo_matrix_rotate (&child->matrix, child->angle);

  if (child->has_position)
    {
      x = child->x;
      y = child->y;
    }
  else
    {
      GtkAllocation allocation;
      GdkRectangle  bounds;
      gint          border;
      gint          available_width;
      gint          available_height;

      gtk_widget_get_allocation (widget, &allocation);

      gimp_overlay_child_transform_bounds (child, &child_allocation, &bounds);

      border = gtk_container_get_border_width (GTK_CONTAINER (box));

      available_width  = allocation.width  - 2 * border;
      available_height = allocation.height - 2 * border;

      x = border;
      y = border;

      if (available_width > bounds.width)
        x += child->xalign * (available_width - bounds.width) - bounds.x;

      if (available_height > bounds.height)
        y += child->yalign * (available_height - bounds.height) - bounds.y;
    }

  cairo_matrix_init_translate (&child->matrix, x, y);

  /* local transform */
  cairo_matrix_rotate (&child->matrix, child->angle);

  gimp_overlay_child_invalidate (box, child);
}

static void
gimp_overlay_child_clip_fully_opaque (GimpOverlayChild *child,
                                      GtkContainer     *container,
                                      cairo_t          *cr)
{
  GList *children;
  GList *list;

  children = gtk_container_get_children (container);

  for (list = children; list; list = g_list_next (list))
    {
      GtkWidget *widget = list->data;

      if (gimp_widget_get_fully_opaque (widget))
        {
          GtkAllocation allocation;
          gint          x, y;

          gtk_widget_get_allocation (widget, &allocation);
          gtk_widget_translate_coordinates (widget, child->widget,
                                            0, 0, &x, &y);

          cairo_rectangle (cr, x, y, allocation.width, allocation.height);
        }
      else if (GTK_IS_CONTAINER (widget))
        {
          gimp_overlay_child_clip_fully_opaque (child,
                                                GTK_CONTAINER (widget),
                                                cr);
        }
    }

  g_list_free (children);
}

gboolean
gimp_overlay_child_draw (GimpOverlayBox   *box,
                         GimpOverlayChild *child,
                         cairo_t          *cr)
{
  GtkWidget *widget;

  g_return_val_if_fail (GIMP_IS_OVERLAY_BOX (box), FALSE);
  g_return_val_if_fail (child != NULL, FALSE);
  g_return_val_if_fail (cr != NULL, FALSE);

  widget = GTK_WIDGET (box);

  if (gtk_cairo_should_draw_window (cr, gtk_widget_get_window (widget)))
    {
      GtkAllocation child_allocation;
      GdkRectangle  bounds;

      gtk_widget_get_allocation (child->widget, &child_allocation);

      gimp_overlay_child_transform_bounds (child, &child_allocation, &bounds);

      if (gtk_widget_get_visible (child->widget))
        {
          cairo_surface_t *surface;

          gdk_window_process_updates (child->window, FALSE);

          surface = gdk_offscreen_window_get_surface (child->window);

          cairo_save (cr);

          cairo_transform (cr, &child->matrix);
          cairo_set_source_surface (cr, surface, 0, 0);
          cairo_paint_with_alpha (cr, child->opacity);

          gimp_overlay_child_clip_fully_opaque (child,
                                                GTK_CONTAINER (child->widget),
                                                cr);
          cairo_clip (cr);
          cairo_paint (cr);

          cairo_restore (cr);
        }
    }

  if (gtk_cairo_should_draw_window (cr, child->window))
    {
      if (! gtk_widget_get_app_paintable (child->widget))
        gtk_paint_flat_box (gtk_widget_get_style (child->widget),
                            cr,
                            GTK_STATE_NORMAL, GTK_SHADOW_NONE,
                            widget, NULL,
                            0, 0, -1, -1);

      gtk_container_propagate_draw (GTK_CONTAINER (widget),
                                    child->widget,
                                    cr);

      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_overlay_child_damage (GimpOverlayBox   *box,
                           GimpOverlayChild *child,
                           GdkEventExpose   *event)
{
  GtkWidget *widget;

  g_return_val_if_fail (GIMP_IS_OVERLAY_BOX (box), FALSE);
  g_return_val_if_fail (child != NULL, FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  widget = GTK_WIDGET (box);

  if (event->window == child->window)
    {
      gint n_rects;
      gint i;

      n_rects = cairo_region_num_rectangles (event->region);

      for (i = 0; i < n_rects; i++)
        {
          GdkRectangle rect;
          GdkRectangle bounds;

          cairo_region_get_rectangle (event->region, i, &rect);

          gimp_overlay_child_transform_bounds (child, &rect, &bounds);

          gdk_window_invalidate_rect (gtk_widget_get_window (widget),
                                      &bounds, FALSE);
        }

      return TRUE;
    }

  return FALSE;
}

void
gimp_overlay_child_invalidate (GimpOverlayBox   *box,
                               GimpOverlayChild *child)
{
  GdkWindow *window;

  g_return_if_fail (GIMP_IS_OVERLAY_BOX (box));
  g_return_if_fail (child != NULL);

  window = gtk_widget_get_window (GTK_WIDGET (box));

  if (window && gtk_widget_get_visible (child->widget))
    {
      GtkAllocation child_allocation;
      GdkRectangle  bounds;

      gtk_widget_get_allocation (child->widget, &child_allocation);

      gimp_overlay_child_transform_bounds (child, &child_allocation,
                                           &bounds);

      gdk_window_invalidate_rect (window, &bounds, FALSE);
    }
}

gboolean
gimp_overlay_child_pick (GimpOverlayBox   *box,
                         GimpOverlayChild *child,
                         gdouble           box_x,
                         gdouble           box_y)
{
  GtkAllocation child_allocation;
  gdouble       child_x;
  gdouble       child_y;

  g_return_val_if_fail (GIMP_IS_OVERLAY_BOX (box), FALSE);
  g_return_val_if_fail (child != NULL, FALSE);

  gimp_overlay_child_from_embedder (child->window,
                                    box_x, box_y,
                                    &child_x, &child_y,
                                    child);

  gtk_widget_get_allocation (child->widget, &child_allocation);

  if (child_x >= 0                      &&
      child_x <  child_allocation.width &&
      child_y >= 0                      &&
      child_y <  child_allocation.height)
    {
      return TRUE;
    }

  return FALSE;
}


/*  private functions  */

static void
gimp_overlay_child_transform_bounds (GimpOverlayChild *child,
                                     GdkRectangle     *bounds_child,
                                     GdkRectangle     *bounds_box)
{
  gdouble x1, x2, x3, x4;
  gdouble y1, y2, y3, y4;

  x1 = bounds_child->x;
  y1 = bounds_child->y;

  x2 = bounds_child->x + bounds_child->width;
  y2 = bounds_child->y;

  x3 = bounds_child->x;
  y3 = bounds_child->y + bounds_child->height;

  x4 = bounds_child->x + bounds_child->width;
  y4 = bounds_child->y + bounds_child->height;

  cairo_matrix_transform_point (&child->matrix, &x1, &y1);
  cairo_matrix_transform_point (&child->matrix, &x2, &y2);
  cairo_matrix_transform_point (&child->matrix, &x3, &y3);
  cairo_matrix_transform_point (&child->matrix, &x4, &y4);

  bounds_box->x      = (gint) floor (MIN4 (x1, x2, x3, x4));
  bounds_box->y      = (gint) floor (MIN4 (y1, y2, y3, y4));
  bounds_box->width  = (gint) ceil (MAX4 (x1, x2, x3, x4)) - bounds_box->x;
  bounds_box->height = (gint) ceil (MAX4 (y1, y2, y3, y4)) - bounds_box->y;
}

static void
gimp_overlay_child_from_embedder (GdkWindow        *child_window,
                                  gdouble           box_x,
                                  gdouble           box_y,
                                  gdouble          *child_x,
                                  gdouble          *child_y,
                                  GimpOverlayChild *child)
{
  cairo_matrix_t inverse = child->matrix;

  *child_x = box_x;
  *child_y = box_y;

  cairo_matrix_invert (&inverse);
  cairo_matrix_transform_point (&inverse, child_x, child_y);
}

static void
gimp_overlay_child_to_embedder (GdkWindow        *child_window,
                                gdouble           child_x,
                                gdouble           child_y,
                                gdouble          *box_x,
                                gdouble          *box_y,
                                GimpOverlayChild *child)
{
  *box_x = child_x;
  *box_y = child_y;

  cairo_matrix_transform_point (&child->matrix, box_x, box_y);
}
