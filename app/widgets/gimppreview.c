/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppreview.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimpbrush.h"
#include "core/gimpbuffer.h"
#include "core/gimpdrawable.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimpimagefile.h"
#include "core/gimpmarshal.h"
#include "core/gimppalette.h"
#include "core/gimppattern.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplayshell-render.h"

#include "gimpbrushpreview.h"
#include "gimpbufferpreview.h"
#include "gimpdnd.h"
#include "gimpdrawablepreview.h"
#include "gimpgradientpreview.h"
#include "gimpimagepreview.h"
#include "gimpimagefilepreview.h"
#include "gimppalettepreview.h"
#include "gimppatternpreview.h"
#include "gimppreview.h"
#include "gimptoolinfopreview.h"


#define PREVIEW_POPUP_DELAY 150

#define PREVIEW_EVENT_MASK  (GDK_BUTTON_PRESS_MASK   | \
                             GDK_BUTTON_RELEASE_MASK | \
                             GDK_ENTER_NOTIFY_MASK   | \
                             GDK_LEAVE_NOTIFY_MASK)

enum
{
  CLICKED,
  DOUBLE_CLICKED,
  EXTENDED_CLICKED,
  CONTEXT,
  RENDER,
  LAST_SIGNAL
};


static void        gimp_preview_class_init           (GimpPreviewClass *klass);
static void        gimp_preview_init                 (GimpPreview      *preview);
static void        gimp_preview_destroy              (GtkObject        *object);
static void        gimp_preview_size_allocate        (GtkWidget        *widget,
						      GtkAllocation    *allocation);
static gint        gimp_preview_button_press_event   (GtkWidget        *widget,
						      GdkEventButton   *bevent);
static gint        gimp_preview_button_release_event (GtkWidget        *widget, 
						      GdkEventButton   *bevent);
static gint        gimp_preview_enter_notify_event   (GtkWidget        *widget,
						      GdkEventCrossing *event);
static gint        gimp_preview_leave_notify_event   (GtkWidget        *widget,
						      GdkEventCrossing *event);

static void        gimp_preview_real_render          (GimpPreview      *preview);
static void        gimp_preview_get_size             (GimpPreview      *preview,
						      gint              size,
						      gint             *width,
						      gint             *height);
static void        gimp_preview_real_get_size        (GimpPreview      *preview,
						      gint              size,
						      gint             *width,
						      gint             *height);
static gboolean    gimp_preview_needs_popup          (GimpPreview      *preview);
static gboolean    gimp_preview_real_needs_popup     (GimpPreview      *preview);
static GtkWidget * gimp_preview_create_popup         (GimpPreview      *preview);
static GtkWidget * gimp_preview_real_create_popup    (GimpPreview      *preview);

static void        gimp_preview_popup_show           (GimpPreview      *preview, 
						      gint              x,
						      gint              y);
static void        gimp_preview_popup_hide           (GimpPreview      *preview);
static void        gimp_preview_size_changed         (GimpPreview      *preview,
						      GimpViewable     *viewable);
static void        gimp_preview_paint                (GimpPreview      *preview);
static gboolean    gimp_preview_idle_paint           (GimpPreview      *preview);
static GimpViewable * gimp_preview_drag_viewable     (GtkWidget        *widget,
						      gpointer          data);


static guint preview_signals[LAST_SIGNAL] = { 0 };

static GtkPreviewClass *parent_class = NULL;


GType
gimp_preview_get_type (void)
{
  static GType preview_type = 0;

  if (! preview_type)
    {
      static const GTypeInfo preview_info =
      {
        sizeof (GimpPreviewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_preview_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpPreview),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_preview_init,
      };

      preview_type = g_type_register_static (GTK_TYPE_PREVIEW,
                                             "GimpPreview",
                                             &preview_info, 0);
    }
  
  return preview_type;
}

static void
gimp_preview_class_init (GimpPreviewClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = GTK_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  preview_signals[CLICKED] = 
    g_signal_new ("clicked",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpPreviewClass, clicked),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  preview_signals[DOUBLE_CLICKED] = 
    g_signal_new ("double_clicked",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpPreviewClass, double_clicked),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  preview_signals[EXTENDED_CLICKED] = 
    g_signal_new ("extended_clicked",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpPreviewClass, extended_clicked),
		  NULL, NULL,
		  gimp_marshal_VOID__UINT,
		  G_TYPE_NONE, 1,
		  G_TYPE_UINT);

  preview_signals[CONTEXT] = 
    g_signal_new ("context",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpPreviewClass, context),
		  NULL, NULL,
		  gimp_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->destroy              = gimp_preview_destroy;

  widget_class->size_allocate        = gimp_preview_size_allocate;
  widget_class->button_press_event   = gimp_preview_button_press_event;
  widget_class->button_release_event = gimp_preview_button_release_event;
  widget_class->enter_notify_event   = gimp_preview_enter_notify_event;
  widget_class->leave_notify_event   = gimp_preview_leave_notify_event;

  klass->clicked                     = NULL;
  klass->double_clicked              = NULL;
  klass->extended_clicked            = NULL;
  klass->context                     = NULL;
  klass->render                      = gimp_preview_real_render;
  klass->get_size                    = gimp_preview_real_get_size;
  klass->needs_popup                 = gimp_preview_real_needs_popup;
  klass->create_popup                = gimp_preview_real_create_popup;
}

static void
gimp_preview_init (GimpPreview *preview)
{
  preview->viewable     = NULL;

  preview->width        = 8;
  preview->height       = 8;
  preview->border_width = 0;
  preview->dot_for_dot  = TRUE;

  gimp_rgba_set (&preview->border_color, 0.0, 0.0, 0.0, 1.0);

  preview->is_popup     = FALSE;
  preview->clickable    = FALSE;
  preview->show_popup   = FALSE;

  preview->size         = -1;
  preview->in_button    = FALSE;
  preview->idle_id      = 0;
  preview->popup_id     = 0;
  preview->popup_x      = 0;
  preview->popup_y      = 0;

  GTK_PREVIEW (preview)->type   = GTK_PREVIEW_COLOR;
  GTK_PREVIEW (preview)->bpp    = 3;
  GTK_PREVIEW (preview)->dither = GDK_RGB_DITHER_NORMAL;

  gtk_widget_set_events (GTK_WIDGET (preview), PREVIEW_EVENT_MASK);
}

static void
gimp_preview_destroy (GtkObject *object)
{
  GimpPreview *preview;

  preview = GIMP_PREVIEW (object);

  if (preview->idle_id)
    {
      g_source_remove (preview->idle_id);
      preview->idle_id = 0;
    }

  gimp_preview_popup_hide (preview);

  if (preview->viewable)
    gimp_preview_set_viewable (preview, NULL);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static GimpPreview *
gimp_preview_new_by_type (GimpViewable *viewable)
{
  GimpPreview *preview;

  if (GIMP_IS_BRUSH (viewable))
    {
      preview = g_object_new (GIMP_TYPE_BRUSH_PREVIEW, NULL);
    }
  else if (GIMP_IS_DRAWABLE (viewable))
    {
      preview = g_object_new (GIMP_TYPE_DRAWABLE_PREVIEW, NULL);
    }
  else if (GIMP_IS_IMAGE (viewable))
    {
      preview = g_object_new (GIMP_TYPE_IMAGE_PREVIEW, NULL);
    }
  else if (GIMP_IS_PATTERN (viewable))
    {
      preview = g_object_new (GIMP_TYPE_PATTERN_PREVIEW, NULL);
    }
  else if (GIMP_IS_GRADIENT (viewable))
    {
      preview = g_object_new (GIMP_TYPE_GRADIENT_PREVIEW, NULL);
    }
  else if (GIMP_IS_PALETTE (viewable))
    {
      preview = g_object_new (GIMP_TYPE_PALETTE_PREVIEW, NULL);
    }
  else if (GIMP_IS_BUFFER (viewable))
    {
      preview = g_object_new (GIMP_TYPE_BUFFER_PREVIEW, NULL);
    }
  else if (GIMP_IS_TOOL_INFO (viewable))
    {
      preview = g_object_new (GIMP_TYPE_TOOL_INFO_PREVIEW, NULL);
    }
  else if (GIMP_IS_IMAGEFILE (viewable))
    {
      preview = g_object_new (GIMP_TYPE_IMAGEFILE_PREVIEW, NULL);
    }
  else
    {
      preview = g_object_new (GIMP_TYPE_PREVIEW, NULL);
    }

  return preview;
}

GtkWidget *
gimp_preview_new (GimpViewable *viewable,
		  gint          size,
		  gint          border_width,
		  gboolean      is_popup)
{
  GimpPreview *preview;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (size > 0 && size <= 256, NULL);
  g_return_val_if_fail (border_width >= 0 && border_width <= 16, NULL);

  preview = gimp_preview_new_by_type (viewable);

  preview->is_popup = is_popup;

  gimp_preview_set_viewable (preview, viewable);

  gimp_preview_set_size (preview, size, border_width);

  return GTK_WIDGET (preview);
}

GtkWidget *
gimp_preview_new_full (GimpViewable *viewable,
		       gint          width,
		       gint          height,
		       gint          border_width,
		       gboolean      is_popup,
		       gboolean      clickable,
		       gboolean      show_popup)
{
  GimpPreview *preview;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (width  > 0 && width  <= 256, NULL);
  g_return_val_if_fail (height > 0 && height <= 256, NULL);
  g_return_val_if_fail (border_width >= 0 && border_width <= 16, NULL);

  preview = gimp_preview_new_by_type (viewable);

  preview->is_popup   = is_popup;
  preview->clickable  = clickable;
  preview->show_popup = show_popup;

  gimp_preview_set_viewable (preview, viewable);

  gimp_preview_set_size_full (preview, width, height, border_width);

  return GTK_WIDGET (preview);
}

void
gimp_preview_set_viewable (GimpPreview  *preview,
			   GimpViewable *viewable)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));
  g_return_if_fail (! viewable || GIMP_IS_VIEWABLE (viewable));

  if (preview->viewable)
    {
      if (! preview->is_popup)
	{
	  gtk_drag_source_unset (GTK_WIDGET (preview));
	  gimp_dnd_viewable_source_unset (GTK_WIDGET (preview),
					  G_TYPE_FROM_INSTANCE (preview->viewable));
	}

      g_object_remove_weak_pointer (G_OBJECT (preview->viewable),
				    (gpointer *) &preview->viewable);

      g_signal_handlers_disconnect_by_func (G_OBJECT (preview->viewable),
                                            G_CALLBACK (gimp_preview_paint),
                                            preview);

      g_signal_handlers_disconnect_by_func (G_OBJECT (preview->viewable),
                                            G_CALLBACK (gimp_preview_size_changed),
                                            preview);

    }

  preview->viewable = viewable;

  if (preview->viewable)
    {
      if (! preview->is_popup)
	{
	  gimp_gtk_drag_source_set_by_type (GTK_WIDGET (preview),
					    GDK_BUTTON1_MASK | GDK_BUTTON2_MASK,
					    G_TYPE_FROM_INSTANCE (preview->viewable),
					    GDK_ACTION_COPY);
	  gimp_dnd_viewable_source_set (GTK_WIDGET (preview),
					G_TYPE_FROM_INSTANCE (preview->viewable),
					gimp_preview_drag_viewable,
					NULL);
	}

      g_object_add_weak_pointer (G_OBJECT (preview->viewable),
				 (gpointer *) &preview->viewable);

      g_signal_connect_swapped (G_OBJECT (preview->viewable),
                                "invalidate_preview",
                                G_CALLBACK (gimp_preview_paint),
                                preview);

      g_signal_connect_swapped (G_OBJECT (preview->viewable),
                                "size_changed",
                                G_CALLBACK (gimp_preview_size_changed),
                                preview);

      if (preview->size != -1)
	{
	  gimp_preview_set_size (preview,
				 preview->size,
				 preview->border_width);
	}

      gimp_preview_paint (preview);
    }
}

void
gimp_preview_set_size (GimpPreview *preview,
		       gint         preview_size,
		       gint         border_width)
{
  gint width, height;

  g_return_if_fail (GIMP_IS_PREVIEW (preview));
  g_return_if_fail (preview_size > 0 && preview_size <= 256);
  g_return_if_fail (border_width >= 0 && border_width <= 16);

  preview->size = preview_size;

  gimp_preview_get_size (preview, preview_size, &width, &height);

  gimp_preview_set_size_full (preview,
			      width,
			      height,
			      border_width);
}

void
gimp_preview_set_size_full (GimpPreview *preview,
			    gint         width,
			    gint         height,
			    gint         border_width)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));
  g_return_if_fail (width  > 0 && width  <= 256);
  g_return_if_fail (height > 0 && height <= 256);
  g_return_if_fail (border_width >= 0 && border_width <= 16);

  preview->width        = width;
  preview->height       = height;
  preview->border_width = border_width;

  gtk_preview_size (GTK_PREVIEW (preview),
		    width  + 2 * preview->border_width,
		    height + 2 * preview->border_width);

  gtk_widget_hide (GTK_WIDGET (preview));
  gtk_widget_show (GTK_WIDGET (preview));
}

void
gimp_preview_set_dot_for_dot (GimpPreview *preview,
			      gboolean     dot_for_dot)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  if (dot_for_dot != preview->dot_for_dot)
    {
      preview->dot_for_dot = dot_for_dot ? TRUE: FALSE;

      if (preview->size != -1)
	{
	  gimp_preview_set_size (preview,
				 preview->size,
				 preview->border_width);
	}

      gimp_preview_paint (preview);
    }
}

void
gimp_preview_set_border_color (GimpPreview   *preview,
			       const GimpRGB *color)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));
  g_return_if_fail (color != NULL);

  if (gimp_rgb_distance (&preview->border_color, color))
    {
      preview->border_color = *color;

      gimp_preview_paint (preview);
    }
}

void
gimp_preview_render (GimpPreview *preview)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  GIMP_PREVIEW_GET_CLASS (preview)->render (preview);
}

static gint
gimp_preview_button_press_event (GtkWidget      *widget,
				 GdkEventButton *bevent)
{
  GimpPreview *preview;

  preview = GIMP_PREVIEW (widget);

  if (! preview->clickable &&
      ! preview->show_popup)
    return FALSE;

  if (bevent->type == GDK_BUTTON_PRESS)
    {
      if (bevent->button == 1)
	{
	  gtk_grab_add (widget);

	  preview->press_state = bevent->state;

	  if (preview->show_popup && gimp_preview_needs_popup (preview))
	    {
	      gimp_preview_popup_show (preview,
				       bevent->x,
				       bevent->y);
	    }
	}
      else
	{
	  preview->press_state = 0;

	  if (bevent->button == 3)
	    {
	      g_signal_emit (G_OBJECT (widget), preview_signals[CONTEXT], 0);
	    }
	  else
	    {
	      return FALSE;
	    }
	}
    }
  else if (bevent->type == GDK_2BUTTON_PRESS)
    {
      if (bevent->button == 1)
	{
	  g_signal_emit (G_OBJECT (widget), preview_signals[DOUBLE_CLICKED], 0);
	}
    }

  return TRUE;
}
  
static gint
gimp_preview_button_release_event (GtkWidget      *widget,
				   GdkEventButton *bevent)
{
  GimpPreview *preview;
  gboolean     click = TRUE;

  preview = GIMP_PREVIEW (widget);

  if (! preview->clickable &&
      ! preview->show_popup)
    return FALSE;

  if (bevent->button == 1)
    {
      if (preview->show_popup && gimp_preview_needs_popup (preview))
	{
	  click = (preview->popup_id != 0);
        }

      gimp_preview_popup_hide (preview);

      /*  remove the grab _after_ hiding the popup  */
      gtk_grab_remove (widget);

      if (preview->clickable && click && preview->in_button)
	{
	  if (preview->press_state &
	      (GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK))
	    {
	      g_signal_emit (G_OBJECT (widget),
                             preview_signals[EXTENDED_CLICKED], 0,
                             preview->press_state);
	    }
	  else
	    {
	      g_signal_emit (G_OBJECT (widget), preview_signals[CLICKED], 0);
	    }
	}
    }
  else
    {
      return FALSE;
    }

  return TRUE;
}

static gint
gimp_preview_enter_notify_event (GtkWidget        *widget,
				 GdkEventCrossing *event)
{
  GimpPreview *preview;
  GtkWidget   *event_widget;

  preview = GIMP_PREVIEW (widget);
  event_widget = gtk_get_event_widget ((GdkEvent *) event);

  if ((event_widget == widget) &&
      (event->detail != GDK_NOTIFY_INFERIOR))
    {
      preview->in_button = TRUE;
    }

  return FALSE;
}

static gint
gimp_preview_leave_notify_event (GtkWidget        *widget,
				 GdkEventCrossing *event)
{
  GimpPreview *preview;
  GtkWidget   *event_widget;

  preview = GIMP_PREVIEW (widget);
  event_widget = gtk_get_event_widget ((GdkEvent *) event);

  if ((event_widget == widget) &&
      (event->detail != GDK_NOTIFY_INFERIOR))
    {
      preview->in_button = FALSE;
    }

  return FALSE;
}

static void
gimp_preview_real_render (GimpPreview *preview)
{
  TempBuf *temp_buf;

  temp_buf = gimp_viewable_get_preview (preview->viewable,
					preview->width,
					preview->height);

  if (temp_buf)
    {
      gimp_preview_render_and_flush (preview,
				     temp_buf,
				     -1);
    }
}

static void
gimp_preview_get_size (GimpPreview *preview,
		       gint         size,
		       gint        *width,
		       gint        *height)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));
  g_return_if_fail (width != NULL);
  g_return_if_fail (height != NULL);

  GIMP_PREVIEW_GET_CLASS (preview)->get_size (preview, size, width, height);
}

static void
gimp_preview_real_get_size (GimpPreview *preview,
			    gint         size,
			    gint        *width,
			    gint        *height)
{
  *width  = size;
  *height = size;
}

static gboolean
gimp_preview_needs_popup (GimpPreview *preview)
{
  g_return_val_if_fail (GIMP_IS_PREVIEW (preview), FALSE);

  return GIMP_PREVIEW_GET_CLASS (preview)->needs_popup (preview);
}

static gboolean
gimp_preview_real_needs_popup (GimpPreview *preview)
{
  return TRUE;
}

static GtkWidget *
gimp_preview_create_popup (GimpPreview *preview)
{
  g_return_val_if_fail (GIMP_IS_PREVIEW (preview), NULL);

  return GIMP_PREVIEW_GET_CLASS (preview)->create_popup (preview);
}

static GtkWidget *
gimp_preview_real_create_popup (GimpPreview *preview)
{
  gint popup_width;
  gint popup_height;

  popup_width  = MIN (preview->width  * 2, 256);
  popup_height = MIN (preview->height * 2, 256);

  return gimp_preview_new_full (preview->viewable,
				popup_width,
				popup_height,
				0,
				TRUE, FALSE, FALSE);
}

static gboolean
gimp_preview_popup_timeout (GimpPreview *preview)
{
  GtkWidget *widget;
  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *popup_preview;
  gint       orig_x;
  gint       orig_y;
  gint       scr_width;
  gint       scr_height;
  gint       x;
  gint       y;
  gint       popup_width;
  gint       popup_height;

  widget = GTK_WIDGET (preview);

  x = preview->popup_x;
  y = preview->popup_y;

  preview->popup_id = 0;
  preview->popup_x  = 0;
  preview->popup_y  = 0;

  window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (window), frame);
  gtk_widget_show (frame);

  popup_preview = gimp_preview_create_popup (preview);

  popup_width  = popup_preview->requisition.width;
  popup_height = popup_preview->requisition.height;

  gtk_container_add (GTK_CONTAINER (frame), popup_preview);
  gtk_widget_show (popup_preview);

  gdk_window_get_origin (widget->window, &orig_x, &orig_y);
  scr_width  = gdk_screen_width ();
  scr_height = gdk_screen_height ();
  x = orig_x + x - (popup_width  >> 1);
  y = orig_y + y - (popup_height >> 1);
  x = (x < 0) ? 0 : x;
  y = (y < 0) ? 0 : y;
  x = (x + popup_width  > scr_width)  ? scr_width  - popup_width  : x;
  y = (y + popup_height > scr_height) ? scr_height - popup_height : y;

  gtk_window_move (GTK_WINDOW (window), x, y);
  gtk_widget_show (window);

  g_object_set_data_full (G_OBJECT (preview), "preview_popup_window", window,
                          (GDestroyNotify) gtk_widget_destroy);

  return FALSE;
}

static void
gimp_preview_popup_show (GimpPreview *preview,
			 gint         x,
			 gint         y)
{
  preview->popup_x = x;
  preview->popup_y = y;

  preview->popup_id = g_timeout_add (PREVIEW_POPUP_DELAY,
				     (GSourceFunc) gimp_preview_popup_timeout,
				     preview);
}

static void
gimp_preview_popup_hide (GimpPreview *preview)
{
  if (preview->popup_id)
    {
      g_source_remove (preview->popup_id);

      preview->popup_id = 0;
      preview->popup_x  = 0;
      preview->popup_y  = 0;
    }

  g_object_set_data (G_OBJECT (preview), "preview_popup_window", NULL);
}

static void
gimp_preview_size_allocate (GtkWidget     *widget,
			    GtkAllocation *allocation)
{
  if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  gimp_preview_paint (GIMP_PREVIEW (widget));
}

static void
gimp_preview_size_changed (GimpPreview  *preview,
			   GimpViewable *viewable)
{
  if (preview->size != -1)
    {
      g_print ("size_changed (%d)\n", preview->size);

      gimp_preview_set_size (preview,
			     preview->size,
			     preview->border_width);
    }
}

static void
gimp_preview_paint (GimpPreview *preview)
{
  if (preview->idle_id)
    {
      g_source_remove (preview->idle_id);
    }

  preview->idle_id =
    g_idle_add_full (G_PRIORITY_LOW,
		     (GSourceFunc) gimp_preview_idle_paint, preview,
		     NULL);
}

static gboolean
gimp_preview_idle_paint (GimpPreview *preview)
{
  preview->idle_id = 0;

  if (! preview->viewable)
    return FALSE;

  gimp_preview_render (preview);

  return FALSE;
}

void
gimp_preview_calc_size (GimpPreview *preview,
			gint         aspect_width,
			gint         aspect_height,
			gint         width,
			gint         height,
			gdouble      xresolution,
			gdouble      yresolution,
			gint        *return_width,
			gint        *return_height,
			gboolean    *scaling_up)
{
  gdouble xratio;
  gdouble yratio;

  if (aspect_width > aspect_height)
    {
      xratio = yratio = (gdouble) width / (gdouble) aspect_width;
    }
  else
    {
      xratio = yratio = (gdouble) height / (gdouble) aspect_height;
    }

  if (! preview->dot_for_dot && xresolution != yresolution)
    {
      yratio *= xresolution / yresolution;
    }

  width  = RINT (xratio * (gdouble) aspect_width);
  height = RINT (yratio * (gdouble) aspect_height);

  if (width  < 1) width  = 1;
  if (height < 1) height = 1;

  *return_width  = width;
  *return_height = height;

  if (scaling_up)
    *scaling_up = (xratio > 1.0) || (yratio > 1.0);
}

void
gimp_preview_render_and_flush (GimpPreview *preview,
			       TempBuf     *temp_buf,
			       gint         channel)
{
  gint      width;
  gint      height;
  guchar   *src, *s;
  guchar   *cb;
  guchar   *buf;
  gint      a;
  gint      i, j, b;
  gint      x1, y1, x2, y2;
  gint      rowstride;
  gboolean  color_buf;
  gboolean  color;
  gint      alpha;
  gboolean  has_alpha;
  gboolean  render_composite;
  gint      image_bytes;
  gint      offset;
  gint      border;
  guchar    border_color[3];

  width  = preview->width;
  height = preview->height;
  border = preview->border_width;

  gimp_rgb_get_uchar (&preview->border_color,
		      &border_color[0],
		      &border_color[1],
		      &border_color[2]);

  alpha = ALPHA_PIX;

  /*  Here are the different cases this functions handles correctly:
   *  1)  Offset temp_buf which does not necessarily cover full image area
   *  2)  Color conversion of temp_buf if it is gray and image is color
   *  3)  Background check buffer for transparent temp_bufs
   *  4)  Using the optional "channel" argument, one channel can be extracted
   *      from a multi-channel temp_buf and composited as a grayscale
   *  Prereqs:
   *  1)  Grayscale temp_bufs have bytes == {1, 2}
   *  2)  Color temp_bufs have bytes == {3, 4}
   *  3)  If image is gray, then temp_buf should have bytes == {1, 2}
   */
  color_buf   = (GTK_PREVIEW (preview)->type == GTK_PREVIEW_COLOR);
  image_bytes = (color_buf) ? 3 : 1;
  has_alpha   = (temp_buf->bytes == 2 || temp_buf->bytes == 4);
  rowstride   = temp_buf->width * temp_buf->bytes;

  /*  Determine if the preview buf supplied is color
   *   Generally, if the bytes == {3, 4}, this is true.
   *   However, if the channel argument supplied is not -1, then
   *   the preview buf is assumed to be gray despite the number of
   *   channels it contains
   */
  color = ((channel == -1) &&
	   (temp_buf->bytes == 3 || temp_buf->bytes == 4));

  /*  render the checkerboard only if the temp_buf has alpha *and*
   *  we render a composite preview (channel == -1)
   */
  if (has_alpha && (channel == -1))
    {
      buf              = render_check_buf;
      render_composite = TRUE;
      alpha            = ((color) ? ALPHA_PIX :
			  ((channel != -1) ? (temp_buf->bytes - 1) :
			   ALPHA_G_PIX));
    }
  else
    {
      buf              = render_empty_buf;
      render_composite = FALSE;
    }

  x1 = CLAMP (temp_buf->x, 0, width);
  y1 = CLAMP (temp_buf->y, 0, height);
  x2 = CLAMP (temp_buf->x + temp_buf->width,  0, width);
  y2 = CLAMP (temp_buf->y + temp_buf->height, 0, height);

  src = temp_buf_data (temp_buf) + ((y1 - temp_buf->y) * rowstride +
				    (x1 - temp_buf->x) * temp_buf->bytes);

  /*  One last thing for efficiency's sake:  */
  if (channel == -1)
    channel = 0;

  /*  Set the border color once before rendering  */
  for (j = 0; j < width + border * 2; j++)
    for (b = 0; b < image_bytes; b++)
      render_temp_buf[j * image_bytes + b] = border_color[b];

  for (i = 0; i < border; i++)
    gtk_preview_draw_row (GTK_PREVIEW (preview),
			  render_temp_buf,
			  0, i,
			  width + 2 * border);

  for (i = border + height; i < 2 * border + height; i++)
    gtk_preview_draw_row (GTK_PREVIEW (preview),
			  render_temp_buf,
			  0, i,
			  width + 2 * border);

  for (i = 0; i < height; i++)
    {
      if (i & 0x4)
	{
	  offset = 4;
	  cb = buf + offset * 3;
	}
      else
	{
	  offset = 0;
	  cb = buf;
	}

      /*  The interesting stuff between leading & trailing 
       *  vertical transparency
       */
      if (i >= y1 && i < y2)
	{
	  /*  Handle the leading transparency  */
	  for (j = 0; j < x1; j++)
	    for (b = 0; b < image_bytes; b++)
	      render_temp_buf[(border + j) * image_bytes + b] = cb[j * 3 + b];

	  /*  The stuff in the middle  */
	  s = src;
	  for (j = x1; j < x2; j++)
	    {
	      if (color)
		{
		  if (has_alpha && render_composite)
		    {
		      a = s[alpha] << 8;

		      if ((j + offset) & 0x4)
			{
			  render_temp_buf[(border + j) * 3 + 0] = 
			    render_blend_dark_check [(a | s[RED_PIX])];
			  render_temp_buf[(border + j) * 3 + 1] = 
			    render_blend_dark_check [(a | s[GREEN_PIX])];
			  render_temp_buf[(border + j) * 3 + 2] = 
			    render_blend_dark_check [(a | s[BLUE_PIX])];
			}
		      else
			{
			  render_temp_buf[(border + j) * 3 + 0] = 
			    render_blend_light_check [(a | s[RED_PIX])];
			  render_temp_buf[(border + j) * 3 + 1] = 
			    render_blend_light_check [(a | s[GREEN_PIX])];
			  render_temp_buf[(border + j) * 3 + 2] = 
			    render_blend_light_check [(a | s[BLUE_PIX])];
			}
		    }
		  else
		    {
		      render_temp_buf[(border + j) * 3 + 0] = s[RED_PIX];
		      render_temp_buf[(border + j) * 3 + 1] = s[GREEN_PIX];
		      render_temp_buf[(border + j) * 3 + 2] = s[BLUE_PIX];
		    }
		}
	      else
		{
		  if (has_alpha && render_composite)
		    {
		      a = s[alpha] << 8;

		      if ((j + offset) & 0x4)
			{
			  if (color_buf)
			    {
			      render_temp_buf[(border + j) * 3 + 0] = 
				render_blend_dark_check [(a | s[GRAY_PIX])];
			      render_temp_buf[(border + j) * 3 + 1] = 
				render_blend_dark_check [(a | s[GRAY_PIX])];
			      render_temp_buf[(border + j) * 3 + 2] = 
				render_blend_dark_check [(a | s[GRAY_PIX])];
			    }
			  else
			    {
			      render_temp_buf[(border + j)] = 
				render_blend_dark_check [(a | s[GRAY_PIX + channel])];
			    }
			}
		      else
			{
			  if (color_buf)
			    {
			      render_temp_buf[(border + j) * 3 + 0] = 
				render_blend_light_check [(a | s[GRAY_PIX])];
			      render_temp_buf[(border + j) * 3 + 1] = 
				render_blend_light_check [(a | s[GRAY_PIX])];
			      render_temp_buf[(border + j) * 3 + 2] = 
				render_blend_light_check [(a | s[GRAY_PIX])];
			    }
			  else
			    {
			      render_temp_buf[(border + j)] = 
				render_blend_light_check [(a | s[GRAY_PIX + channel])];
			    }
			}
		    }
		  else
		    {
		      if (color_buf)
			{
			  render_temp_buf[(border + j) * 3 + 0] = s[GRAY_PIX];
			  render_temp_buf[(border + j) * 3 + 1] = s[GRAY_PIX];
			  render_temp_buf[(border + j) * 3 + 2] = s[GRAY_PIX];
			}
		      else
			{
			  render_temp_buf[(border + j)] = s[GRAY_PIX + channel];
			}
		    }
		}

	      s += temp_buf->bytes;
	    }

	  /*  Handle the trailing transparency  */
	  for (j = x2; j < width; j++)
	    for (b = 0; b < image_bytes; b++)
	      render_temp_buf[(border + j) * image_bytes + b] = cb[j * 3 + b];

	  src += rowstride;
	}
      else
	{
	  for (j = 0; j < width; j++)
	    for (b = 0; b < image_bytes; b++)
	      render_temp_buf[(border + j) * image_bytes + b] = cb[j * 3 + b];
	}

      gtk_preview_draw_row (GTK_PREVIEW (preview),
			    render_temp_buf,
			    0,
			    i + border,
			    width + 2 * border);
    }

  gtk_widget_queue_draw (GTK_WIDGET (preview));
}

static GimpViewable *
gimp_preview_drag_viewable (GtkWidget *widget,
			    gpointer   data)
{
  return GIMP_PREVIEW (widget)->viewable;
}
