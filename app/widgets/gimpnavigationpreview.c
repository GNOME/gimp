/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpNavigationPreview Widget
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

#include "gimpnavigationpreview.h"
#include "gimpviewrenderer.h"


#define BORDER_PEN_WIDTH 3


enum
{
  MARKER_CHANGED,
  ZOOM,
  SCROLL,
  LAST_SIGNAL
};


static void   gimp_navigation_preview_class_init (GimpNavigationPreviewClass *klass);
static void   gimp_navigation_preview_init       (GimpNavigationPreview      *preview);

static void   gimp_navigation_preview_realize          (GtkWidget      *widget);
static void   gimp_navigation_preview_unrealize        (GtkWidget      *widget);
static void   gimp_navigation_preview_size_allocate    (GtkWidget      *widget,
                                                        GtkAllocation  *allocation);
static gboolean gimp_navigation_preview_expose         (GtkWidget      *widget,
                                                        GdkEventExpose *eevent);
static gboolean gimp_navigation_preview_button_press   (GtkWidget      *widget,
                                                        GdkEventButton *bevent);
static gboolean gimp_navigation_preview_button_release (GtkWidget      *widget,
                                                        GdkEventButton *bevent);
static gboolean gimp_navigation_preview_scroll         (GtkWidget      *widget,
                                                        GdkEventScroll *sevent);
static gboolean gimp_navigation_preview_motion_notify  (GtkWidget      *widget,
                                                        GdkEventMotion *mevent);
static gboolean gimp_navigation_preview_key_press      (GtkWidget      *widget,
                                                        GdkEventKey    *kevent);

static void gimp_navigation_preview_transform   (GimpNavigationPreview *nav_preview);
static void gimp_navigation_preview_draw_marker (GimpNavigationPreview *nav_preview,
                                                  GdkRectangle         *area);


static guint preview_signals[LAST_SIGNAL] = { 0 };

static GimpViewClass *parent_class = NULL;


GType
gimp_navigation_preview_get_type (void)
{
  static GType preview_type = 0;

  if (! preview_type)
    {
      static const GTypeInfo preview_info =
      {
        sizeof (GimpNavigationPreviewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_navigation_preview_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpNavigationPreview),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_navigation_preview_init,
      };

      preview_type = g_type_register_static (GIMP_TYPE_VIEW,
                                             "GimpNavigationPreview",
                                             &preview_info, 0);
    }

  return preview_type;
}

static void
gimp_navigation_preview_class_init (GimpNavigationPreviewClass *klass)
{
  GtkWidgetClass *widget_class;

  widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  preview_signals[MARKER_CHANGED] =
    g_signal_new ("marker_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpNavigationPreviewClass, marker_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2,
                  G_TYPE_DOUBLE,
                  G_TYPE_DOUBLE);

  preview_signals[ZOOM] =
    g_signal_new ("zoom",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpNavigationPreviewClass, zoom),
                  NULL, NULL,
                  gimp_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_ZOOM_TYPE);

  preview_signals[SCROLL] =
    g_signal_new ("scroll",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpNavigationPreviewClass, scroll),
                  NULL, NULL,
                  gimp_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  GDK_TYPE_SCROLL_DIRECTION);

  widget_class->realize              = gimp_navigation_preview_realize;
  widget_class->unrealize            = gimp_navigation_preview_unrealize;
  widget_class->size_allocate        = gimp_navigation_preview_size_allocate;
  widget_class->expose_event         = gimp_navigation_preview_expose;
  widget_class->button_press_event   = gimp_navigation_preview_button_press;
  widget_class->button_release_event = gimp_navigation_preview_button_release;
  widget_class->scroll_event         = gimp_navigation_preview_scroll;
  widget_class->motion_notify_event  = gimp_navigation_preview_motion_notify;
  widget_class->key_press_event      = gimp_navigation_preview_key_press;
}

static void
gimp_navigation_preview_init (GimpNavigationPreview *preview)
{
  GTK_WIDGET_SET_FLAGS (preview, GTK_CAN_FOCUS);

  gtk_widget_add_events (GTK_WIDGET (preview), (GDK_POINTER_MOTION_MASK |
                                                GDK_KEY_PRESS_MASK));

  preview->x               = 0.0;
  preview->y               = 0.0;
  preview->width           = 0.0;
  preview->height          = 0.0;

  preview->p_x             = 0;
  preview->p_y             = 0;
  preview->p_width         = 0;
  preview->p_height        = 0;

  preview->motion_offset_x = 0;
  preview->motion_offset_y = 0;
  preview->has_grab        = FALSE;

  preview->gc              = NULL;
}

static void
gimp_navigation_preview_realize (GtkWidget *widget)
{
  GimpNavigationPreview *nav_preview = GIMP_NAVIGATION_PREVIEW (widget);

  if (GTK_WIDGET_CLASS (parent_class)->realize)
    GTK_WIDGET_CLASS (parent_class)->realize (widget);

  nav_preview->gc = gdk_gc_new (widget->window);

  gdk_gc_set_function (nav_preview->gc, GDK_INVERT);
  gdk_gc_set_line_attributes (nav_preview->gc,
                              BORDER_PEN_WIDTH,
                              GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
}

static void
gimp_navigation_preview_unrealize (GtkWidget *widget)
{
  GimpNavigationPreview *nav_preview = GIMP_NAVIGATION_PREVIEW (widget);

  if (nav_preview->gc)
    {
      g_object_unref (nav_preview->gc);
      nav_preview->gc = NULL;
    }

  if (GTK_WIDGET_CLASS (parent_class)->unrealize)
    GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
gimp_navigation_preview_size_allocate (GtkWidget     *widget,
                                       GtkAllocation *allocation)
{
  if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  if (GIMP_VIEW (widget)->renderer->viewable)
    gimp_navigation_preview_transform (GIMP_NAVIGATION_PREVIEW (widget));
}

static gboolean
gimp_navigation_preview_expose (GtkWidget      *widget,
                                GdkEventExpose *eevent)
{
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      if (GTK_WIDGET_CLASS (parent_class)->expose_event)
        GTK_WIDGET_CLASS (parent_class)->expose_event (widget, eevent);

      gimp_navigation_preview_draw_marker (GIMP_NAVIGATION_PREVIEW (widget),
                                           &eevent->area);
    }

  return TRUE;
}

static void
gimp_navigation_preview_move_to (GimpNavigationPreview *nav_preview,
                                 gint                   tx,
                                 gint                   ty)
{
  GimpView    *view;
  GimpImage   *gimage;
  gdouble      ratiox, ratioy;
  gdouble      x, y;

  view = GIMP_VIEW (nav_preview);

  if (! view->renderer->viewable)
    return;

  tx = CLAMP (tx, 0, view->renderer->width  - nav_preview->p_width);
  ty = CLAMP (ty, 0, view->renderer->height - nav_preview->p_height);

  gimage = GIMP_IMAGE (view->renderer->viewable);

  /*  transform to image coordinates  */
  if (view->renderer->width != nav_preview->p_width)
    ratiox = ((gimage->width - nav_preview->width + 1.0) /
              (view->renderer->width - nav_preview->p_width));
  else
    ratiox = 1.0;

  if (view->renderer->height != nav_preview->p_height)
    ratioy = ((gimage->height - nav_preview->height + 1.0) /
              (view->renderer->height - nav_preview->p_height));
  else
    ratioy = 1.0;

  x = tx * ratiox;
  y = ty * ratioy;

  g_signal_emit (view, preview_signals[MARKER_CHANGED], 0, x, y);
}

void
gimp_navigation_preview_grab_pointer (GimpNavigationPreview *nav_preview)
{
  GtkWidget  *widget;
  GdkDisplay *display;
  GdkCursor  *cursor;
  GdkWindow  *window;

  widget = GTK_WIDGET (nav_preview);

  nav_preview->has_grab = TRUE;

  gtk_grab_add (widget);

  display = gtk_widget_get_display (widget);
  cursor = gdk_cursor_new_for_display (display, GDK_FLEUR);

  window = GIMP_VIEW (nav_preview)->event_window;

  gdk_pointer_grab (window, TRUE,
                    GDK_BUTTON_RELEASE_MASK      |
                    GDK_POINTER_MOTION_HINT_MASK |
                    GDK_BUTTON_MOTION_MASK       |
                    GDK_EXTENSION_EVENTS_ALL,
                    window, cursor, GDK_CURRENT_TIME);

  gdk_cursor_unref (cursor);
}

static gboolean
gimp_navigation_preview_button_press (GtkWidget      *widget,
                                      GdkEventButton *bevent)
{
  GimpNavigationPreview *nav_preview;
  gint                   tx, ty;
  GdkDisplay            *display;

  nav_preview = GIMP_NAVIGATION_PREVIEW (widget);

  tx = bevent->x;
  ty = bevent->y;

  switch (bevent->button)
    {
    case 1:
      if (! (tx >  nav_preview->p_x &&
             tx < (nav_preview->p_x + nav_preview->p_width) &&
             ty >  nav_preview->p_y &&
             ty < (nav_preview->p_y + nav_preview->p_height)))
        {
          GdkCursor *cursor;

          nav_preview->motion_offset_x = nav_preview->p_width  / 2;
          nav_preview->motion_offset_y = nav_preview->p_height / 2;

          tx -= nav_preview->motion_offset_x;
          ty -= nav_preview->motion_offset_y;

          gimp_navigation_preview_move_to (nav_preview, tx, ty);

          display = gtk_widget_get_display (widget);
          cursor = gdk_cursor_new_for_display (display, GDK_FLEUR);
          gdk_window_set_cursor (GIMP_VIEW (widget)->event_window, cursor);
          gdk_cursor_unref (cursor);
        }
      else
        {
          nav_preview->motion_offset_x = tx - nav_preview->p_x;
          nav_preview->motion_offset_y = ty - nav_preview->p_y;
        }

      gimp_navigation_preview_grab_pointer (nav_preview);
      break;

    default:
      break;
    }

  return TRUE;
}

static gboolean
gimp_navigation_preview_button_release (GtkWidget      *widget,
                                        GdkEventButton *bevent)
{
  GimpNavigationPreview *nav_preview;

  nav_preview = GIMP_NAVIGATION_PREVIEW (widget);

  switch (bevent->button)
    {
    case 1:
      nav_preview->has_grab = FALSE;

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
gimp_navigation_preview_scroll (GtkWidget      *widget,
                                GdkEventScroll *sevent)
{
  GimpNavigationPreview *nav_preview;

  nav_preview = GIMP_NAVIGATION_PREVIEW (widget);

  if (sevent->state & GDK_SHIFT_MASK)
    {
      if (sevent->direction == GDK_SCROLL_UP)
        {
          g_signal_emit (widget, preview_signals[ZOOM], 0, GIMP_ZOOM_IN);
        }
      else
        {
          g_signal_emit (widget, preview_signals[ZOOM], 0, GIMP_ZOOM_OUT);
        }
    }
  else
    {
      GdkScrollDirection direction;

      if (sevent->state & GDK_CONTROL_MASK)
        {
          if (sevent->direction == GDK_SCROLL_UP)
            direction = GDK_SCROLL_LEFT;
          else
            direction = GDK_SCROLL_RIGHT;
        }
      else
        {
          direction = sevent->direction;
        }

      g_signal_emit (widget, preview_signals[SCROLL], 0, direction);
    }

  return TRUE;
}

static gboolean
gimp_navigation_preview_motion_notify (GtkWidget      *widget,
                                       GdkEventMotion *mevent)
{
  GimpNavigationPreview *nav_preview;
  GimpView              *view;
  gint                   tx, ty;
  GdkModifierType        mask;

  nav_preview = GIMP_NAVIGATION_PREVIEW (widget);
  view        = GIMP_VIEW (widget);

  if (! nav_preview->has_grab)
    {
      GdkCursor  *cursor;
      GdkDisplay *display;

      display = gtk_widget_get_display (widget);

      if (nav_preview->p_x == 0 &&
          nav_preview->p_y == 0 &&
          nav_preview->p_width  == view->renderer->width &&
          nav_preview->p_height == view->renderer->height)
        {
          gdk_window_set_cursor (view->event_window, NULL);
          return FALSE;
        }
      else if (mevent->x >= nav_preview->p_x &&
               mevent->y >= nav_preview->p_y &&
               mevent->x <  nav_preview->p_x + nav_preview->p_width &&
               mevent->y <  nav_preview->p_y + nav_preview->p_height)
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

  tx -= nav_preview->motion_offset_x;
  ty -= nav_preview->motion_offset_y;

  gimp_navigation_preview_move_to (nav_preview, tx, ty);

  return TRUE;
}

static gboolean
gimp_navigation_preview_key_press (GtkWidget   *widget,
                                   GdkEventKey *kevent)
{
  GimpNavigationPreview *nav_preview;
  gint                   scroll_x = 0;
  gint                   scroll_y = 0;

  nav_preview = GIMP_NAVIGATION_PREVIEW (widget);

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
      gimp_navigation_preview_move_to (nav_preview,
                                       nav_preview->p_x + scroll_x,
                                       nav_preview->p_y + scroll_y);
      return TRUE;
    }

  return FALSE;
}

static void
gimp_navigation_preview_transform (GimpNavigationPreview *nav_preview)
{
  GimpView  *view;
  GimpImage *gimage;
  gdouble    ratiox, ratioy;

  view = GIMP_VIEW (nav_preview);

  gimage = GIMP_IMAGE (view->renderer->viewable);

  ratiox = ((gdouble) view->renderer->width  / (gdouble) gimage->width);
  ratioy = ((gdouble) view->renderer->height / (gdouble) gimage->height);

  nav_preview->p_x = RINT (nav_preview->x * ratiox);
  nav_preview->p_y = RINT (nav_preview->y * ratioy);

  nav_preview->p_width  = RINT (nav_preview->width  * ratiox);
  nav_preview->p_height = RINT (nav_preview->height * ratioy);
}

static void
gimp_navigation_preview_draw_marker (GimpNavigationPreview *nav_preview,
                                     GdkRectangle          *area)
{
  GimpView *view;

  view = GIMP_VIEW (nav_preview);

  if (view->renderer->viewable  &&
      nav_preview->width           &&
      nav_preview->height)
    {
      GimpImage *gimage;

      gimage = GIMP_IMAGE (view->renderer->viewable);

      if (nav_preview->x      > 0             ||
          nav_preview->y      > 0             ||
          nav_preview->width  < gimage->width ||
          nav_preview->height < gimage->height)
        {
          GtkWidget *widget = GTK_WIDGET (view);

          if (area)
            gdk_gc_set_clip_rectangle (nav_preview->gc, area);

          gdk_draw_rectangle (widget->window, nav_preview->gc,
                              FALSE,
                              widget->allocation.x + nav_preview->p_x + 1,
                              widget->allocation.y + nav_preview->p_y + 1,
                              MAX (1, nav_preview->p_width  - BORDER_PEN_WIDTH),
                              MAX (1, nav_preview->p_height - BORDER_PEN_WIDTH));

          if (area)
            gdk_gc_set_clip_rectangle (nav_preview->gc, NULL);
        }
    }
}

void
gimp_navigation_preview_set_marker (GimpNavigationPreview *nav_preview,
                                    gdouble                x,
                                    gdouble                y,
                                    gdouble                width,
                                    gdouble                height)
{
  GimpView  *view;
  GimpImage *gimage;

  g_return_if_fail (GIMP_IS_NAVIGATION_PREVIEW (nav_preview));

  view = GIMP_VIEW (nav_preview);

  g_return_if_fail (view->renderer->viewable);

  gimage = GIMP_IMAGE (view->renderer->viewable);

  /*  remove old marker  */
  if (GTK_WIDGET_DRAWABLE (view))
    gimp_navigation_preview_draw_marker (nav_preview, NULL);

  nav_preview->x = CLAMP (x, 0.0, gimage->width  - 1.0);
  nav_preview->y = CLAMP (y, 0.0, gimage->height - 1.0);

  if (width < 0.0)
    width = gimage->width;

  if (height < 0.0)
    height = gimage->height;

  nav_preview->width  = CLAMP (width,  1.0, gimage->width  - nav_preview->x);
  nav_preview->height = CLAMP (height, 1.0, gimage->height - nav_preview->y);

  gimp_navigation_preview_transform (nav_preview);

  /*  draw new marker  */
  if (GTK_WIDGET_DRAWABLE (view))
    gimp_navigation_preview_draw_marker (nav_preview, NULL);
}

