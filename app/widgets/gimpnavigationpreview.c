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
#include "gimppreviewrenderer.h"


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

static void   gimp_navigation_preview_destroy    (GtkObject                  *object);
static void   gimp_navigation_preview_realize    (GtkWidget                  *widget);
static gboolean   gimp_navigation_preview_expose (GtkWidget                  *widget,
						  GdkEventExpose             *eevent);
static gboolean   gimp_navigation_preview_button_press   (GtkWidget          *widget,
							  GdkEventButton     *bevent);
static gboolean   gimp_navigation_preview_button_release (GtkWidget          *widget, 
							  GdkEventButton     *bevent);
static gboolean   gimp_navigation_preview_scroll         (GtkWidget          *widget, 
							  GdkEventScroll     *sevent);
static gboolean   gimp_navigation_preview_motion_notify  (GtkWidget          *widget, 
							  GdkEventMotion     *mevent);
static gboolean   gimp_navigation_preview_key_press      (GtkWidget          *widget, 
							  GdkEventKey        *kevent);
static void  gimp_navigation_preview_draw_marker (GimpNavigationPreview      *nav_preview,
						  GdkRectangle               *area);


static guint preview_signals[LAST_SIGNAL] = { 0 };

static GimpPreviewClass *parent_class = NULL;


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

      preview_type = g_type_register_static (GIMP_TYPE_PREVIEW,
                                             "GimpNavigationPreview",
                                             &preview_info, 0);
    }

  return preview_type;
}

static void
gimp_navigation_preview_class_init (GimpNavigationPreviewClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = GTK_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  preview_signals[MARKER_CHANGED] = 
    g_signal_new ("marker_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpNavigationPreviewClass, marker_changed),
		  NULL, NULL,
		  gimp_marshal_VOID__INT_INT,
		  G_TYPE_NONE, 2,
		  G_TYPE_INT,
		  G_TYPE_INT);

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

  object_class->destroy              = gimp_navigation_preview_destroy;

  widget_class->realize              = gimp_navigation_preview_realize;
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

  preview->x               = 0;
  preview->y               = 0;
  preview->width           = 0;
  preview->height          = 0;

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
gimp_navigation_preview_destroy (GtkObject *object)
{
  GimpNavigationPreview *nav_preview;

  nav_preview = GIMP_NAVIGATION_PREVIEW (object);

  if (nav_preview->gc)
    {
      g_object_unref (nav_preview->gc);
      nav_preview->gc = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_navigation_preview_realize (GtkWidget *widget)
{
  GimpNavigationPreview *nav_preview;

  nav_preview = GIMP_NAVIGATION_PREVIEW (widget);

  if (GTK_WIDGET_CLASS (parent_class)->realize)
    GTK_WIDGET_CLASS (parent_class)->realize (widget);

  nav_preview->gc = gdk_gc_new (widget->window);

  gdk_gc_set_function (nav_preview->gc, GDK_INVERT);
  gdk_gc_set_line_attributes (nav_preview->gc,
			      BORDER_PEN_WIDTH, 
			      GDK_LINE_SOLID, GDK_CAP_BUTT, GDK_JOIN_ROUND);
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
  GimpPreview *preview;
  GimpImage   *gimage;
  gdouble      ratiox, ratioy;
  gint         x, y;

  preview = GIMP_PREVIEW (nav_preview);

  if (! preview->viewable)
    return;

  tx = CLAMP (tx, 0, preview->renderer->width);
  ty = CLAMP (ty, 0, preview->renderer->height);
  
  if ((tx + nav_preview->p_width) >= preview->renderer->width)
    {
      tx = preview->renderer->width - nav_preview->p_width;
    }
  
  if ((ty + nav_preview->p_height) >= preview->renderer->height)
    {
      ty = preview->renderer->height - nav_preview->p_height;
    }

  if (nav_preview->p_x == tx && nav_preview->p_y == ty)
    return;

  gimage = GIMP_IMAGE (preview->viewable);

  /*  transform to image coordinates  */
  ratiox = ((gdouble) preview->renderer->width  / (gdouble) gimage->width);
  ratioy = ((gdouble) preview->renderer->height / (gdouble) gimage->height);

  x = RINT (tx / ratiox);
  y = RINT (ty / ratioy);

  g_signal_emit (preview, preview_signals[MARKER_CHANGED], 0, x, y);
}

void
gimp_navigation_preview_grab_pointer (GimpNavigationPreview *nav_preview)
{
  GtkWidget *widget;
  GdkCursor *cursor;

  widget = GTK_WIDGET (nav_preview);

  nav_preview->has_grab = TRUE;

  gtk_grab_add (widget);

  cursor = gdk_cursor_new (GDK_FLEUR);

  gdk_pointer_grab (widget->window, TRUE,
		    GDK_BUTTON_RELEASE_MASK |
		    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON_MOTION_MASK |
		    GDK_EXTENSION_EVENTS_ALL,
		    widget->window, cursor, 0);

  gdk_cursor_unref (cursor);
}

static gboolean
gimp_navigation_preview_button_press (GtkWidget      *widget,
				      GdkEventButton *bevent)
{
  GimpNavigationPreview *nav_preview;
  gint                   tx, ty;

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

          cursor = gdk_cursor_new (GDK_FLEUR);
          gdk_window_set_cursor (widget->window, cursor);
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
      gdk_pointer_ungrab (0);
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
  gint                   tx, ty;
  GdkModifierType        mask;

  nav_preview = GIMP_NAVIGATION_PREVIEW (widget);

  if (! nav_preview->has_grab)
    {
      GdkCursor *cursor;

      if (nav_preview->p_x == 0 &&
          nav_preview->p_y == 0 &&
          nav_preview->p_width  == GIMP_PREVIEW (widget)->renderer->width &&
          nav_preview->p_height == GIMP_PREVIEW (widget)->renderer->height)
        {
          gdk_window_set_cursor (widget->window, NULL);
          return FALSE;
        }
      else if (mevent->x >= nav_preview->p_x &&
          mevent->y >= nav_preview->p_y &&
          mevent->x <  nav_preview->p_x + nav_preview->p_width &&
          mevent->y <  nav_preview->p_y + nav_preview->p_height)
        {
          cursor = gdk_cursor_new (GDK_FLEUR);
        }
      else
        {
          cursor = gdk_cursor_new (GDK_HAND2);
        }

      gdk_window_set_cursor (widget->window, cursor);
      gdk_cursor_unref (cursor);

      return FALSE;
    }

  gdk_window_get_pointer (widget->window, &tx, &ty, &mask);

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
gimp_navigation_preview_draw_marker (GimpNavigationPreview *nav_preview,
				     GdkRectangle          *area)
{
  GimpPreview *preview;

  preview = GIMP_PREVIEW (nav_preview);

  if (preview->viewable  &&
      nav_preview->width &&
      nav_preview->height)
    {
      GimpImage *gimage;

      gimage = GIMP_IMAGE (preview->viewable);

      if (nav_preview->x      > 0             ||
	  nav_preview->y      > 0             ||
	  nav_preview->width  < gimage->width ||
	  nav_preview->height < gimage->height)
	{
	  if (area)
	    gdk_gc_set_clip_rectangle (nav_preview->gc, area);

	  gdk_draw_rectangle (GTK_WIDGET (preview)->window, nav_preview->gc,
			      FALSE,
			      nav_preview->p_x,
			      nav_preview->p_y,
			      nav_preview->p_width  - BORDER_PEN_WIDTH + 1,
			      nav_preview->p_height - BORDER_PEN_WIDTH + 1);

	  if (area)
	    gdk_gc_set_clip_rectangle (nav_preview->gc, NULL);
	}
    }
}

GtkWidget *
gimp_navigation_preview_new (GimpImage *gimage,
			     gint       size)
{
  GtkWidget *preview;

  g_return_val_if_fail (! gimage || GIMP_IS_IMAGE (gimage), NULL);
  g_return_val_if_fail (size > 0 && size <= GIMP_PREVIEW_MAX_SIZE, NULL);

  preview = gimp_preview_new_by_types (GIMP_TYPE_NAVIGATION_PREVIEW,
                                       GIMP_TYPE_IMAGE,
                                       size, 0, TRUE);

  if (gimage)
    gimp_preview_set_viewable (GIMP_PREVIEW (preview), GIMP_VIEWABLE (gimage));

  return preview;
}

void
gimp_navigation_preview_set_marker (GimpNavigationPreview *nav_preview,
				    gint                   x,
				    gint                   y,
				    gint                   width,
				    gint                   height)
{
  GimpPreview *preview;
  GimpImage   *gimage;
  gdouble      ratiox, ratioy;

  g_return_if_fail (GIMP_IS_NAVIGATION_PREVIEW (nav_preview));

  preview = GIMP_PREVIEW (nav_preview);

  g_return_if_fail (preview->viewable);

  gimage = GIMP_IMAGE (preview->viewable);

  /*  remove old marker  */
  if (GTK_WIDGET_DRAWABLE (preview))
    gimp_navigation_preview_draw_marker (nav_preview, NULL);

  nav_preview->x = CLAMP (x, 0, gimage->width  - 1);
  nav_preview->y = CLAMP (y, 0, gimage->height - 1);

  if (width == -1)
    width = gimage->width;

  if (height == -1)
    height = gimage->height;

  nav_preview->width  = CLAMP (width,  1, gimage->width  - nav_preview->x);
  nav_preview->height = CLAMP (height, 1, gimage->height - nav_preview->y);

  /*  transform to preview coordinates  */
  ratiox = ((gdouble) preview->renderer->width  / (gdouble) gimage->width);
  ratioy = ((gdouble) preview->renderer->height / (gdouble) gimage->height);

  nav_preview->p_x = RINT (nav_preview->x * ratiox);
  nav_preview->p_y = RINT (nav_preview->y * ratioy);

  nav_preview->p_width  = RINT (nav_preview->width  * ratiox);
  nav_preview->p_height = RINT (nav_preview->height * ratioy);

  /*  draw new marker  */
  if (GTK_WIDGET_DRAWABLE (preview))
    gimp_navigation_preview_draw_marker (nav_preview, NULL);
}
