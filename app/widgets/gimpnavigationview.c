/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpNavigationView Widget
 * Copyright (C) 2001-2002 Michael Natterer <mitch@gimp.org>
 *
 * partly based on app/nav_window
 * Copyright (C) 1999 Andy Thomas <alt@gimp.org>
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

#include <math.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"

#include "widgets-types.h"

#include "core/gimpimage.h"
#include "core/gimpmarshal.h"

#include "gimpnavigationview.h"
#include "gimpviewrenderer.h"
#include "gimpwidgets-utils.h"


#define BORDER_WIDTH 2


enum
{
  MARKER_CHANGED,
  ZOOM,
  SCROLL,
  LAST_SIGNAL
};


struct _GimpNavigationView
{
  GimpView     parent_instance;

  /*  values in image coordinates  */
  gdouble      center_x;
  gdouble      center_y;
  gdouble      width;
  gdouble      height;
  gboolean     flip_horizontally;
  gboolean     flip_vertically;
  gdouble      rotate_angle;

  /*  values in view coordinates  */
  gint         p_center_x;
  gint         p_center_y;
  gint         p_width;
  gint         p_height;

  gint         motion_offset_x;
  gint         motion_offset_y;
  gboolean     has_grab;
};


static void     gimp_navigation_view_size_allocate   (GtkWidget      *widget,
                                                      GtkAllocation  *allocation);
static gboolean gimp_navigation_view_draw            (GtkWidget      *widget,
                                                      cairo_t        *cr);
static gboolean gimp_navigation_view_button_press    (GtkWidget      *widget,
                                                      GdkEventButton *bevent);
static gboolean gimp_navigation_view_button_release  (GtkWidget      *widget,
                                                      GdkEventButton *bevent);
static gboolean gimp_navigation_view_scroll          (GtkWidget      *widget,
                                                      GdkEventScroll *sevent);
static gboolean gimp_navigation_view_motion_notify   (GtkWidget      *widget,
                                                      GdkEventMotion *mevent);
static gboolean gimp_navigation_view_key_press       (GtkWidget      *widget,
                                                      GdkEventKey    *kevent);

static void     gimp_navigation_view_transform       (GimpNavigationView *nav_view);
static void     gimp_navigation_view_draw_marker     (GimpNavigationView *nav_view,
                                                      cairo_t            *cr);
static void     gimp_navigation_view_move_to         (GimpNavigationView *nav_view,
                                                      gint                tx,
                                                      gint                ty);
static void     gimp_navigation_view_get_ratio       (GimpNavigationView *nav_view,
                                                      gdouble            *ratiox,
                                                      gdouble            *ratioy);
static gboolean gimp_navigation_view_point_in_marker (GimpNavigationView *nav_view,
                                                      gint                x,
                                                      gint                y);


G_DEFINE_TYPE (GimpNavigationView, gimp_navigation_view, GIMP_TYPE_VIEW)

#define parent_class gimp_navigation_view_parent_class

static guint view_signals[LAST_SIGNAL] = { 0 };


static void
gimp_navigation_view_class_init (GimpNavigationViewClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  view_signals[MARKER_CHANGED] =
    g_signal_new ("marker-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpNavigationViewClass, marker_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__DOUBLE_DOUBLE_DOUBLE_DOUBLE,
                  G_TYPE_NONE, 4,
                  G_TYPE_DOUBLE,
                  G_TYPE_DOUBLE,
                  G_TYPE_DOUBLE,
                  G_TYPE_DOUBLE);

  view_signals[ZOOM] =
    g_signal_new ("zoom",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpNavigationViewClass, zoom),
                  NULL, NULL,
                  gimp_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_ZOOM_TYPE);

  view_signals[SCROLL] =
    g_signal_new ("scroll",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpNavigationViewClass, scroll),
                  NULL, NULL,
                  gimp_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  GDK_TYPE_SCROLL_DIRECTION);

  widget_class->size_allocate        = gimp_navigation_view_size_allocate;
  widget_class->draw                 = gimp_navigation_view_draw;
  widget_class->button_press_event   = gimp_navigation_view_button_press;
  widget_class->button_release_event = gimp_navigation_view_button_release;
  widget_class->scroll_event         = gimp_navigation_view_scroll;
  widget_class->motion_notify_event  = gimp_navigation_view_motion_notify;
  widget_class->key_press_event      = gimp_navigation_view_key_press;
}

static void
gimp_navigation_view_init (GimpNavigationView *view)
{
  gtk_widget_set_can_focus (GTK_WIDGET (view), TRUE);
  gtk_widget_add_events (GTK_WIDGET (view),
                         GDK_POINTER_MOTION_MASK |
                         GDK_KEY_PRESS_MASK);

  view->center_x          = 0.0;
  view->center_y          = 0.0;
  view->width             = 0.0;
  view->height            = 0.0;
  view->flip_horizontally = FALSE;
  view->flip_vertically   = FALSE;
  view->rotate_angle      = 0.0;

  view->p_center_x      = 0;
  view->p_center_y      = 0;
  view->p_width         = 0;
  view->p_height        = 0;

  view->motion_offset_x = 0;
  view->motion_offset_y = 0;
  view->has_grab        = FALSE;
}

static void
gimp_navigation_view_size_allocate (GtkWidget     *widget,
                                    GtkAllocation *allocation)
{
  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  if (GIMP_VIEW (widget)->renderer->viewable)
    gimp_navigation_view_transform (GIMP_NAVIGATION_VIEW (widget));
}

static gboolean
gimp_navigation_view_draw (GtkWidget *widget,
                           cairo_t   *cr)
{
  GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);

  gimp_navigation_view_draw_marker (GIMP_NAVIGATION_VIEW (widget), cr);

  return TRUE;
}

void
gimp_navigation_view_grab_pointer (GimpNavigationView *nav_view)
{
  GtkWidget  *widget = GTK_WIDGET (nav_view);
  GdkDisplay *display;
  GdkCursor  *cursor;
  GdkWindow  *window;

  nav_view->has_grab = TRUE;

  gtk_grab_add (widget);

  display = gtk_widget_get_display (widget);
  cursor = gdk_cursor_new_for_display (display, GDK_FLEUR);

  window = GIMP_VIEW (nav_view)->event_window;

  gdk_pointer_grab (window, FALSE,
                    GDK_BUTTON_RELEASE_MASK      |
                    GDK_POINTER_MOTION_HINT_MASK |
                    GDK_BUTTON_MOTION_MASK,
                    NULL, cursor, GDK_CURRENT_TIME);

  g_object_unref (cursor);
}

static gboolean
gimp_navigation_view_button_press (GtkWidget      *widget,
                                   GdkEventButton *bevent)
{
  GimpNavigationView *nav_view = GIMP_NAVIGATION_VIEW (widget);
  gint                tx, ty;
  GdkDisplay         *display;

  tx = bevent->x;
  ty = bevent->y;

  if (bevent->type == GDK_BUTTON_PRESS && bevent->button == 1)
    {
      if (! gimp_navigation_view_point_in_marker (nav_view, tx, ty))
        {
          GdkCursor *cursor;

          nav_view->motion_offset_x = 0;
          nav_view->motion_offset_y = 0;

          gimp_navigation_view_move_to (nav_view, tx, ty);

          display = gtk_widget_get_display (widget);
          cursor = gdk_cursor_new_for_display (display, GDK_FLEUR);
          gdk_window_set_cursor (GIMP_VIEW (widget)->event_window, cursor);
          g_object_unref (cursor);
        }
      else
        {
          nav_view->motion_offset_x = tx - nav_view->p_center_x;
          nav_view->motion_offset_y = ty - nav_view->p_center_y;
        }

      gimp_navigation_view_grab_pointer (nav_view);
    }

  return TRUE;
}

static gboolean
gimp_navigation_view_button_release (GtkWidget      *widget,
                                     GdkEventButton *bevent)
{
  GimpNavigationView *nav_view = GIMP_NAVIGATION_VIEW (widget);

  if (bevent->button == 1 && nav_view->has_grab)
    {
      nav_view->has_grab = FALSE;

      gtk_grab_remove (widget);
      gdk_display_pointer_ungrab (gtk_widget_get_display (widget),
                                  GDK_CURRENT_TIME);
    }

  return TRUE;
}

static gboolean
gimp_navigation_view_scroll (GtkWidget      *widget,
                             GdkEventScroll *sevent)
{
  if (sevent->state & gimp_get_toggle_behavior_mask ())
    {
      switch (sevent->direction)
        {
        case GDK_SCROLL_UP:
          g_signal_emit (widget, view_signals[ZOOM], 0, GIMP_ZOOM_IN);
          break;

        case GDK_SCROLL_DOWN:
          g_signal_emit (widget, view_signals[ZOOM], 0, GIMP_ZOOM_OUT);
          break;

        default:
          break;
        }
    }
  else
    {
      GdkScrollDirection direction = sevent->direction;

      if (sevent->state & GDK_SHIFT_MASK)
        switch (direction)
          {
          case GDK_SCROLL_UP:    direction = GDK_SCROLL_LEFT;  break;
          case GDK_SCROLL_DOWN:  direction = GDK_SCROLL_RIGHT; break;
          case GDK_SCROLL_LEFT:  direction = GDK_SCROLL_UP;    break;
          case GDK_SCROLL_RIGHT: direction = GDK_SCROLL_DOWN;  break;
          }

      g_signal_emit (widget, view_signals[SCROLL], 0, direction);
    }

  return TRUE;
}

static gboolean
gimp_navigation_view_motion_notify (GtkWidget      *widget,
                                    GdkEventMotion *mevent)
{
  GimpNavigationView *nav_view = GIMP_NAVIGATION_VIEW (widget);
  GimpView           *view     = GIMP_VIEW (widget);

  if (! nav_view->has_grab)
    {
      GdkDisplay *display = gtk_widget_get_display (widget);
      GdkCursor  *cursor;

      if (nav_view->p_center_x == view->renderer->width  / 2 &&
          nav_view->p_center_y == view->renderer->height / 2 &&
          nav_view->p_width    == view->renderer->width      &&
          nav_view->p_height   == view->renderer->height)
        {
          gdk_window_set_cursor (view->event_window, NULL);
          return FALSE;
        }
      else if (gimp_navigation_view_point_in_marker (nav_view,
                                                     mevent->x, mevent->y))
        {
          cursor = gdk_cursor_new_for_display (display, GDK_FLEUR);
        }
      else
        {
          cursor = gdk_cursor_new_for_display (display, GDK_HAND2);
        }

      gdk_window_set_cursor (view->event_window, cursor);
      g_object_unref (cursor);

      return FALSE;
    }

  gimp_navigation_view_move_to (nav_view,
                                mevent->x - nav_view->motion_offset_x,
                                mevent->y - nav_view->motion_offset_y);

  gdk_event_request_motions (mevent);

  return TRUE;
}

static gboolean
gimp_navigation_view_key_press (GtkWidget   *widget,
                                GdkEventKey *kevent)
{
  GimpNavigationView *nav_view = GIMP_NAVIGATION_VIEW (widget);
  gint                scroll_x = 0;
  gint                scroll_y = 0;

  switch (kevent->keyval)
    {
    case GDK_KEY_Up:
      scroll_y = -1;
      break;

    case GDK_KEY_Left:
      scroll_x = -1;
      break;

    case GDK_KEY_Right:
      scroll_x = 1;
      break;

    case GDK_KEY_Down:
      scroll_y = 1;
      break;

    default:
      break;
    }

  if (scroll_x || scroll_y)
    {
      gimp_navigation_view_move_to (nav_view,
                                    nav_view->p_center_x + scroll_x,
                                    nav_view->p_center_y + scroll_y);
      return TRUE;
    }

  return FALSE;
}


/*  public functions  */

void
gimp_navigation_view_set_marker (GimpNavigationView *nav_view,
                                 gdouble             center_x,
                                 gdouble             center_y,
                                 gdouble             width,
                                 gdouble             height,
                                 gboolean            flip_horizontally,
                                 gboolean            flip_vertically,
                                 gdouble             rotate_angle)
{
  GimpView *view;

  g_return_if_fail (GIMP_IS_NAVIGATION_VIEW (nav_view));

  view = GIMP_VIEW (nav_view);

  g_return_if_fail (view->renderer->viewable);

  nav_view->center_x          = center_x;
  nav_view->center_y          = center_y;
  nav_view->width             = MAX (1.0, width);
  nav_view->height            = MAX (1.0, height);
  nav_view->flip_horizontally = flip_horizontally ? TRUE : FALSE;
  nav_view->flip_vertically   = flip_vertically   ? TRUE : FALSE;
  nav_view->rotate_angle      = rotate_angle;

  gimp_navigation_view_transform (nav_view);

  /* Marker changed, redraw */
  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
gimp_navigation_view_set_motion_offset (GimpNavigationView *view,
                                        gint                motion_offset_x,
                                        gint                motion_offset_y)
{
  g_return_if_fail (GIMP_IS_NAVIGATION_VIEW (view));

  view->motion_offset_x = motion_offset_x;
  view->motion_offset_y = motion_offset_y;
}

void
gimp_navigation_view_get_local_marker (GimpNavigationView *view,
                                       gint               *center_x,
                                       gint               *center_y,
                                       gint               *width,
                                       gint               *height)
{
  g_return_if_fail (GIMP_IS_NAVIGATION_VIEW (view));

  if (center_x) *center_x = view->p_center_x;
  if (center_y) *center_y = view->p_center_y;
  if (width)    *width    = view->p_width;
  if (height)   *height   = view->p_height;
}


/*  private functions  */

static void
gimp_navigation_view_transform (GimpNavigationView *nav_view)
{
  gdouble ratiox, ratioy;

  gimp_navigation_view_get_ratio (nav_view, &ratiox, &ratioy);

  nav_view->p_center_x = RINT (nav_view->center_x * ratiox);
  nav_view->p_center_y = RINT (nav_view->center_y * ratioy);

  nav_view->p_width  = ceil (nav_view->width  * ratiox);
  nav_view->p_height = ceil (nav_view->height * ratioy);
}

static void
gimp_navigation_view_draw_marker (GimpNavigationView *nav_view,
                                  cairo_t            *cr)
{
  GimpView *view = GIMP_VIEW (nav_view);

  if (view->renderer->viewable && nav_view->width && nav_view->height)
    {
      GtkWidget     *widget = GTK_WIDGET (view);
      GtkAllocation  allocation;
      gint           p_width_2;
      gint           p_height_2;
      gdouble        angle;

      p_width_2  = nav_view->p_width  / 2;
      p_height_2 = nav_view->p_height / 2;

      angle = G_PI * nav_view->rotate_angle / 180.0;
      if (nav_view->flip_horizontally != nav_view->flip_vertically)
        angle = -angle;

      gtk_widget_get_allocation (widget, &allocation);

      cairo_rectangle (cr,
                       0, 0,
                       allocation.width, allocation.height);
      cairo_translate (cr, nav_view->p_center_x, nav_view->p_center_y);
      cairo_rotate (cr, -angle);
      cairo_rectangle (cr,
                       -p_width_2, -p_height_2,
                       nav_view->p_width, nav_view->p_height);

      cairo_set_source_rgba (cr, 0, 0, 0, 0.5);
      cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
      cairo_fill (cr);

      cairo_rectangle (cr,
                       -p_width_2, -p_height_2,
                       nav_view->p_width, nav_view->p_height);

      cairo_set_source_rgb (cr, 1, 1, 1);
      cairo_set_line_width (cr, BORDER_WIDTH);
      cairo_stroke (cr);
    }
}

static void
gimp_navigation_view_move_to (GimpNavigationView *nav_view,
                              gint                tx,
                              gint                ty)
{
  GimpView  *view = GIMP_VIEW (nav_view);
  gdouble    ratiox, ratioy;
  gdouble    x, y;

  if (! view->renderer->viewable)
    return;

  gimp_navigation_view_get_ratio (nav_view, &ratiox, &ratioy);

  x = tx / ratiox;
  y = ty / ratioy;

  g_signal_emit (view, view_signals[MARKER_CHANGED], 0,
                 x, y, nav_view->width, nav_view->height);
}

static void
gimp_navigation_view_get_ratio (GimpNavigationView *nav_view,
                                gdouble            *ratiox,
                                gdouble            *ratioy)
{
  GimpView  *view = GIMP_VIEW (nav_view);
  GimpImage *image;

  image = GIMP_IMAGE (view->renderer->viewable);

  *ratiox = (gdouble) view->renderer->width  /
            (gdouble) gimp_image_get_width  (image);
  *ratioy = (gdouble) view->renderer->height /
            (gdouble) gimp_image_get_height (image);
}

static gboolean
gimp_navigation_view_point_in_marker (GimpNavigationView *nav_view,
                                      gint                x,
                                      gint                y)
{
  gint    p_width_2, p_height_2;
  gdouble angle;
  gdouble tx, ty;

  p_width_2  = nav_view->p_width  / 2;
  p_height_2 = nav_view->p_height / 2;

  angle = G_PI * nav_view->rotate_angle / 180.0;
  if (nav_view->flip_horizontally != nav_view->flip_vertically)
    angle = -angle;

  x -= nav_view->p_center_x;
  y -= nav_view->p_center_y;

  tx = cos (angle) * x - sin (angle) * y;
  ty = sin (angle) * x + cos (angle) * y;

  return tx >= -p_width_2  && tx < p_width_2 &&
         ty >= -p_height_2 && ty < p_height_2;
}
