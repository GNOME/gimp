/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpNavigationView Widget
 * Copyright (C) 2001-2002 Michael Natterer <mitch@gimp.org>
 *
 * partly based on app/nav_window
 * Copyright (C) 1999 Andy Thomas <alt@gimp.org>
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
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"

#include "widgets-types.h"

#include "core/gimpimage.h"
#include "core/gimpmarshal.h"

#include "gimpnavigationview.h"
#include "gimpviewrenderer.h"


#define BORDER_PEN_WIDTH 3


enum
{
  MARKER_CHANGED,
  ZOOM,
  SCROLL,
  LAST_SIGNAL
};


static void     gimp_navigation_view_realize        (GtkWidget      *widget);
static void     gimp_navigation_view_unrealize      (GtkWidget      *widget);
static void     gimp_navigation_view_size_allocate  (GtkWidget      *widget,
                                                     GtkAllocation  *allocation);
static gboolean gimp_navigation_view_expose         (GtkWidget      *widget,
                                                     GdkEventExpose *eevent);
static gboolean gimp_navigation_view_button_press   (GtkWidget      *widget,
                                                     GdkEventButton *bevent);
static gboolean gimp_navigation_view_button_release (GtkWidget      *widget,
                                                     GdkEventButton *bevent);
static gboolean gimp_navigation_view_scroll         (GtkWidget      *widget,
                                                     GdkEventScroll *sevent);
static gboolean gimp_navigation_view_motion_notify  (GtkWidget      *widget,
                                                     GdkEventMotion *mevent);
static gboolean gimp_navigation_view_key_press      (GtkWidget      *widget,
                                                     GdkEventKey    *kevent);

static void     gimp_navigation_view_transform      (GimpNavigationView *nav_view);
static void     gimp_navigation_view_draw_marker    (GimpNavigationView *nav_view,
                                                     GdkRectangle       *area);


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
                  gimp_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2,
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

  widget_class->realize              = gimp_navigation_view_realize;
  widget_class->unrealize            = gimp_navigation_view_unrealize;
  widget_class->size_allocate        = gimp_navigation_view_size_allocate;
  widget_class->expose_event         = gimp_navigation_view_expose;
  widget_class->button_press_event   = gimp_navigation_view_button_press;
  widget_class->button_release_event = gimp_navigation_view_button_release;
  widget_class->scroll_event         = gimp_navigation_view_scroll;
  widget_class->motion_notify_event  = gimp_navigation_view_motion_notify;
  widget_class->key_press_event      = gimp_navigation_view_key_press;
}

static void
gimp_navigation_view_init (GimpNavigationView *view)
{
  GTK_WIDGET_SET_FLAGS (view, GTK_CAN_FOCUS);

  gtk_widget_add_events (GTK_WIDGET (view), (GDK_POINTER_MOTION_MASK |
                                             GDK_KEY_PRESS_MASK));

  view->x               = 0.0;
  view->y               = 0.0;
  view->width           = 0.0;
  view->height          = 0.0;

  view->p_x             = 0;
  view->p_y             = 0;
  view->p_width         = 0;
  view->p_height        = 0;

  view->motion_offset_x = 0;
  view->motion_offset_y = 0;
  view->has_grab        = FALSE;

  view->gc              = NULL;
}

static void
gimp_navigation_view_realize (GtkWidget *widget)
{
  GimpNavigationView *nav_view = GIMP_NAVIGATION_VIEW (widget);

  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  nav_view->gc = gdk_gc_new (widget->window);

  gdk_gc_set_function (nav_view->gc, GDK_INVERT);
  gdk_gc_set_line_attributes (nav_view->gc,
                              BORDER_PEN_WIDTH,
                              GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
}

static void
gimp_navigation_view_unrealize (GtkWidget *widget)
{
  GimpNavigationView *nav_view = GIMP_NAVIGATION_VIEW (widget);

  if (nav_view->gc)
    {
      g_object_unref (nav_view->gc);
      nav_view->gc = NULL;
    }

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
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
gimp_navigation_view_expose (GtkWidget      *widget,
                             GdkEventExpose *eevent)
{
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      GTK_WIDGET_CLASS (parent_class)->expose_event (widget, eevent);

      gimp_navigation_view_draw_marker (GIMP_NAVIGATION_VIEW (widget),
                                        &eevent->area);
    }

  return TRUE;
}

static void
gimp_navigation_view_move_to (GimpNavigationView *nav_view,
                              gint                tx,
                              gint                ty)
{
  GimpView  *view = GIMP_VIEW (nav_view);
  GimpImage *image;
  gdouble    ratiox, ratioy;
  gdouble    x, y;

  if (! view->renderer->viewable)
    return;

  tx = CLAMP (tx, 0, view->renderer->width  - nav_view->p_width);
  ty = CLAMP (ty, 0, view->renderer->height - nav_view->p_height);

  image = GIMP_IMAGE (view->renderer->viewable);

  /*  transform to image coordinates  */
  if (view->renderer->width != nav_view->p_width)
    ratiox = ((image->width - nav_view->width + 1.0) /
              (view->renderer->width - nav_view->p_width));
  else
    ratiox = 1.0;

  if (view->renderer->height != nav_view->p_height)
    ratioy = ((image->height - nav_view->height + 1.0) /
              (view->renderer->height - nav_view->p_height));
  else
    ratioy = 1.0;

  x = tx * ratiox;
  y = ty * ratioy;

  g_signal_emit (view, view_signals[MARKER_CHANGED], 0, x, y);
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

  gdk_pointer_grab (window, TRUE,
                    GDK_BUTTON_RELEASE_MASK      |
                    GDK_POINTER_MOTION_HINT_MASK |
                    GDK_BUTTON_MOTION_MASK       |
                    GDK_EXTENSION_EVENTS_ALL,
                    window, cursor, GDK_CURRENT_TIME);

  gdk_cursor_unref (cursor);
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

  switch (bevent->button)
    {
    case 1:
      if (! (tx >  nav_view->p_x &&
             tx < (nav_view->p_x + nav_view->p_width) &&
             ty >  nav_view->p_y &&
             ty < (nav_view->p_y + nav_view->p_height)))
        {
          GdkCursor *cursor;

          nav_view->motion_offset_x = nav_view->p_width  / 2;
          nav_view->motion_offset_y = nav_view->p_height / 2;

          tx -= nav_view->motion_offset_x;
          ty -= nav_view->motion_offset_y;

          gimp_navigation_view_move_to (nav_view, tx, ty);

          display = gtk_widget_get_display (widget);
          cursor = gdk_cursor_new_for_display (display, GDK_FLEUR);
          gdk_window_set_cursor (GIMP_VIEW (widget)->event_window, cursor);
          gdk_cursor_unref (cursor);
        }
      else
        {
          nav_view->motion_offset_x = tx - nav_view->p_x;
          nav_view->motion_offset_y = ty - nav_view->p_y;
        }

      gimp_navigation_view_grab_pointer (nav_view);
      break;

    default:
      break;
    }

  return TRUE;
}

static gboolean
gimp_navigation_view_button_release (GtkWidget      *widget,
                                     GdkEventButton *bevent)
{
  GimpNavigationView *nav_view = GIMP_NAVIGATION_VIEW (widget);

  switch (bevent->button)
    {
    case 1:
      nav_view->has_grab = FALSE;

      gtk_grab_remove (widget);
      gdk_display_pointer_ungrab (gtk_widget_get_display (widget),
                                  GDK_CURRENT_TIME);
      break;

    default:
      break;
    }

  return TRUE;
}

static gboolean
gimp_navigation_view_scroll (GtkWidget      *widget,
                             GdkEventScroll *sevent)
{
  if (sevent->state & GDK_CONTROL_MASK)
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
  gint                tx, ty;
  GdkModifierType     mask;

  if (! nav_view->has_grab)
    {
      GdkCursor  *cursor;
      GdkDisplay *display;

      display = gtk_widget_get_display (widget);

      if (nav_view->p_x == 0 &&
          nav_view->p_y == 0 &&
          nav_view->p_width  == view->renderer->width &&
          nav_view->p_height == view->renderer->height)
        {
          gdk_window_set_cursor (view->event_window, NULL);
          return FALSE;
        }
      else if (mevent->x >= nav_view->p_x &&
               mevent->y >= nav_view->p_y &&
               mevent->x <  nav_view->p_x + nav_view->p_width &&
               mevent->y <  nav_view->p_y + nav_view->p_height)
        {
          cursor = gdk_cursor_new_for_display (display, GDK_FLEUR);
        }
      else
        {
          cursor = gdk_cursor_new_for_display (display, GDK_HAND2);
        }

      gdk_window_set_cursor (view->event_window, cursor);
      gdk_cursor_unref (cursor);

      return FALSE;
    }

  gdk_window_get_pointer (view->event_window, &tx, &ty, &mask);

  tx -= nav_view->motion_offset_x;
  ty -= nav_view->motion_offset_y;

  gimp_navigation_view_move_to (nav_view, tx, ty);

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
    case GDK_Up:
      scroll_y = -1;
      break;

    case GDK_Left:
      scroll_x = -1;
      break;

    case GDK_Right:
      scroll_x = 1;
      break;

    case GDK_Down:
      scroll_y = 1;
      break;

    default:
      break;
    }

  if (scroll_x || scroll_y)
    {
      gimp_navigation_view_move_to (nav_view,
                                    nav_view->p_x + scroll_x,
                                    nav_view->p_y + scroll_y);
      return TRUE;
    }

  return FALSE;
}

static void
gimp_navigation_view_transform (GimpNavigationView *nav_view)
{
  GimpView  *view = GIMP_VIEW (nav_view);
  GimpImage *image;
  gdouble    ratiox, ratioy;

  image = GIMP_IMAGE (view->renderer->viewable);

  ratiox = ((gdouble) view->renderer->width  / (gdouble) image->width);
  ratioy = ((gdouble) view->renderer->height / (gdouble) image->height);

  nav_view->p_x = RINT (nav_view->x * ratiox);
  nav_view->p_y = RINT (nav_view->y * ratioy);

  nav_view->p_width  = RINT (nav_view->width  * ratiox);
  nav_view->p_height = RINT (nav_view->height * ratioy);
}

static void
gimp_navigation_view_draw_marker (GimpNavigationView *nav_view,
                                  GdkRectangle       *area)
{
  GimpView *view = GIMP_VIEW (nav_view);

  if (view->renderer->viewable  &&
      nav_view->width           &&
      nav_view->height)
    {
      GimpImage *image;

      image = GIMP_IMAGE (view->renderer->viewable);

      if (nav_view->x      > 0             ||
          nav_view->y      > 0             ||
          nav_view->width  < image->width ||
          nav_view->height < image->height)
        {
          GtkWidget *widget = GTK_WIDGET (view);

          if (area)
            gdk_gc_set_clip_rectangle (nav_view->gc, area);

          gdk_draw_rectangle (widget->window, nav_view->gc,
                              FALSE,
                              widget->allocation.x + nav_view->p_x + 1,
                              widget->allocation.y + nav_view->p_y + 1,
                              MAX (1, nav_view->p_width  - BORDER_PEN_WIDTH),
                              MAX (1, nav_view->p_height - BORDER_PEN_WIDTH));

          if (area)
            gdk_gc_set_clip_rectangle (nav_view->gc, NULL);
        }
    }
}

void
gimp_navigation_view_set_marker (GimpNavigationView *nav_view,
                                 gdouble             x,
                                 gdouble             y,
                                 gdouble             width,
                                 gdouble             height)
{
  GimpView  *view;
  GimpImage *image;

  g_return_if_fail (GIMP_IS_NAVIGATION_VIEW (nav_view));

  view = GIMP_VIEW (nav_view);

  g_return_if_fail (view->renderer->viewable);

  image = GIMP_IMAGE (view->renderer->viewable);

  /*  remove old marker  */
  if (GTK_WIDGET_DRAWABLE (view))
    gimp_navigation_view_draw_marker (nav_view, NULL);

  nav_view->x = CLAMP (x, 0.0, image->width  - 1.0);
  nav_view->y = CLAMP (y, 0.0, image->height - 1.0);

  if (width < 0.0)
    width = image->width;

  if (height < 0.0)
    height = image->height;

  nav_view->width  = CLAMP (width,  1.0, image->width  - nav_view->x);
  nav_view->height = CLAMP (height, 1.0, image->height - nav_view->y);

  gimp_navigation_view_transform (nav_view);

  /*  draw new marker  */
  if (GTK_WIDGET_DRAWABLE (view))
    gimp_navigation_view_draw_marker (nav_view, NULL);
}
