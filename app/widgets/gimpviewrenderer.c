/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpPreview Widget
 * Copyright (C) 2001 Michael Natterer
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

#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "gimpimage.h"
#include "gimppreview.h"
#include "gimpviewable.h"
#include "image_render.h"
#include "temp_buf.h"

#include "libgimp/gimplimits.h"


#define PREVIEW_POPUP_DELAY 150

#define PREVIEW_EVENT_MASK  (GDK_BUTTON_PRESS_MASK   | \
                             GDK_BUTTON_RELEASE_MASK | \
                             GDK_ENTER_NOTIFY_MASK   | \
                             GDK_LEAVE_NOTIFY_MASK)

enum
{
  CLICKED,
  LAST_SIGNAL
};


static void       gimp_preview_class_init           (GimpPreviewClass *klass);
static void       gimp_preview_init                 (GimpPreview      *preview);
static void       gimp_preview_destroy              (GtkObject        *object);
static void       gimp_preview_size_allocate        (GtkWidget        *widget,
						     GtkAllocation    *allocation);
static gint       gimp_preview_button_press_event   (GtkWidget        *widget,
						     GdkEventButton   *bevent);
static gint       gimp_preview_button_release_event (GtkWidget        *widget, 
						     GdkEventButton   *bevent);
static gint       gimp_preview_enter_notify_event   (GtkWidget        *widget,
						     GdkEventCrossing *event);
static gint       gimp_preview_leave_notify_event   (GtkWidget        *widget,
						     GdkEventCrossing *event);
static void       gimp_preview_popup_show           (GimpPreview      *preview, 
						     gint              x,
						     gint              y);
static void       gimp_preview_popup_hide           (GimpPreview      *preview);
static void       gimp_preview_paint                (GimpPreview      *preview);
static gboolean   gimp_preview_idle_paint           (GimpPreview      *preview);


static guint preview_signals[LAST_SIGNAL] = { 0 };

static GtkPreviewClass *parent_class = NULL;


GtkType
gimp_preview_get_type (void)
{
  static GtkType preview_type = 0;

  if (! preview_type)
    {
      GtkTypeInfo preview_info =
      {
	"GimpPreview",
	sizeof (GimpPreview),
	sizeof (GimpPreviewClass),
	(GtkClassInitFunc) gimp_preview_class_init,
	(GtkObjectInitFunc) gimp_preview_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      preview_type = gtk_type_unique (GTK_TYPE_PREVIEW, &preview_info);
    }
  
  return preview_type;
}

static void
gimp_preview_class_init (GimpPreviewClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  parent_class = gtk_type_class (GTK_TYPE_PREVIEW);

  preview_signals[CLICKED] = 
    gtk_signal_new ("clicked",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpPreviewClass,
				       clicked),
                    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, preview_signals, LAST_SIGNAL);

  object_class->destroy = gimp_preview_destroy;

  widget_class->size_allocate        = gimp_preview_size_allocate;
  widget_class->button_press_event   = gimp_preview_button_press_event;
  widget_class->button_release_event = gimp_preview_button_release_event;
  widget_class->enter_notify_event   = gimp_preview_enter_notify_event;
  widget_class->leave_notify_event   = gimp_preview_leave_notify_event;

  klass->clicked = NULL;
}

static void
gimp_preview_init (GimpPreview *preview)
{
  preview->viewable   = NULL;
  preview->clickable  = FALSE;
  preview->show_popup = FALSE;

  preview->in_button  = FALSE;
  preview->idle_id    = 0;
  preview->popup_id   = 0;
  preview->popup_x    = 0;
  preview->popup_y    = 0;

  GTK_PREVIEW (preview)->type   = GTK_PREVIEW_COLOR;
  GTK_PREVIEW (preview)->bpp    = 3;
  GTK_PREVIEW (preview)->dither = GDK_RGB_DITHER_NORMAL;

  gtk_widget_set_events (GTK_WIDGET (preview), PREVIEW_EVENT_MASK);
}

static void
gimp_preview_destroy (GtkObject *object)
{
  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

GtkWidget *
gimp_preview_new (GimpViewable *viewable,
		  gint          width,
		  gint          height,
		  gboolean      clickable,
		  gboolean      show_popup)
{
  GimpPreview *preview;

  g_return_val_if_fail (viewable != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (width  > 0 && width  <= 256, NULL);
  g_return_val_if_fail (height > 0 && height <= 256, NULL);

  preview = gtk_type_new (GIMP_TYPE_PREVIEW);

  preview->viewable   = viewable;
  preview->clickable  = clickable;
  preview->show_popup = show_popup;

  gtk_signal_connect_object_while_alive (GTK_OBJECT (viewable),
					 "invalidate_preview",
					 GTK_SIGNAL_FUNC (gimp_preview_paint),
					 GTK_OBJECT (preview));

  gtk_signal_connect_while_alive (GTK_OBJECT (viewable), "destroy",
				  GTK_SIGNAL_FUNC (gtk_widget_destroyed),
				  &preview->viewable,
				  GTK_OBJECT (preview));

  gtk_preview_size (GTK_PREVIEW (preview), width, height);

  return GTK_WIDGET (preview);
}

static gint
gimp_preview_button_press_event (GtkWidget      *widget,
				 GdkEventButton *bevent)
{
  GimpPreview *preview;

  preview = GIMP_PREVIEW (widget);

  if (bevent->button == 1)
    {
      gtk_grab_add (widget);

      if (preview->show_popup)
        {
          gimp_preview_popup_show (preview,
				   bevent->x,
				   bevent->y);
        }
    }

  return TRUE;
}
  
static gint
gimp_preview_button_release_event (GtkWidget      *widget,
				   GdkEventButton *bevent)
{
  GimpPreview *preview;
  gboolean     fast_click = TRUE;

  preview = GIMP_PREVIEW (widget);

  if (bevent->button == 1)
    {
      gtk_grab_remove (widget);

      if (preview->show_popup)
        {
	  fast_click = (preview->popup_id != 0);

          gimp_preview_popup_hide (preview);
        }

      if (preview->clickable && fast_click && preview->in_button)
	{
	  gtk_signal_emit (GTK_OBJECT (widget), preview_signals[CLICKED]);

	  g_print ("clicked\n");
	}
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

  popup_width  = MIN (widget->allocation.width  * 2, 256);
  popup_height = MIN (widget->allocation.height * 2, 256);

  window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_policy (GTK_WINDOW (window), FALSE, FALSE, TRUE);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (window), frame);
  gtk_widget_show (frame);

  popup_preview = gimp_preview_new (preview->viewable,
				    popup_width, popup_height,
				    FALSE, FALSE);
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

  gtk_widget_popup (window, x, y);

  gtk_object_set_data_full (GTK_OBJECT (preview), "preview_popup_window", window,
			    (GtkDestroyNotify) gtk_widget_destroy);

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

  gtk_object_set_data (GTK_OBJECT (preview), "preview_popup_window", NULL);
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
gimp_preview_paint (GimpPreview *preview)
{
  if (preview->idle_id)
    return;

  preview->idle_id = g_idle_add_full (G_PRIORITY_LOW,
				      (GSourceFunc) gimp_preview_idle_paint, preview,
				      NULL);
}

static gboolean
gimp_preview_idle_paint (GimpPreview *preview)
{
  TempBuf *temp_buf;
  gint     width;
  gint     height;
  gint     channel;

  preview->idle_id = 0;

  if (! preview->viewable)
    return FALSE;

  temp_buf = gimp_viewable_preview_new (preview->viewable,
					GTK_WIDGET (preview)->allocation.width,
					GTK_WIDGET (preview)->allocation.height);

  width   = temp_buf->width;
  height  = temp_buf->height;
  channel = -1;

  /*  from layers_dialog.c  */
  {
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
    gint      image_bytes;
    gint      offset;

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

    if (has_alpha)
      {
	buf = render_check_buf;
	alpha = ((color) ? ALPHA_PIX :
		 ((channel != -1) ? (temp_buf->bytes - 1) :
		  ALPHA_G_PIX));
      }
    else
      {
	buf = render_empty_buf;
      }

    x1 = CLAMP (temp_buf->x, 0, width);
    y1 = CLAMP (temp_buf->y, 0, height);
    x2 = CLAMP (temp_buf->x + temp_buf->width, 0, width);
    y2 = CLAMP (temp_buf->y + temp_buf->height, 0, height);

    src = temp_buf_data (temp_buf) + ((y1 - temp_buf->y) * rowstride +
				      (x1 - temp_buf->x) * temp_buf->bytes);

    /*  One last thing for efficiency's sake:  */
    if (channel == -1)
      channel = 0;

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
		render_temp_buf[j * image_bytes + b] = cb[j * 3 + b];

	    /*  The stuff in the middle  */
	    s = src;
	    for (j = x1; j < x2; j++)
	      {
		if (color)
		  {
		    if (has_alpha)
		      {
			a = s[alpha] << 8;

			if ((j + offset) & 0x4)
			  {
			    render_temp_buf[j * 3 + 0] = 
			      render_blend_dark_check [(a | s[RED_PIX])];
			    render_temp_buf[j * 3 + 1] = 
			      render_blend_dark_check [(a | s[GREEN_PIX])];
			    render_temp_buf[j * 3 + 2] = 
			      render_blend_dark_check [(a | s[BLUE_PIX])];
			  }
			else
			  {
			    render_temp_buf[j * 3 + 0] = 
			      render_blend_light_check [(a | s[RED_PIX])];
			    render_temp_buf[j * 3 + 1] = 
			      render_blend_light_check [(a | s[GREEN_PIX])];
			    render_temp_buf[j * 3 + 2] = 
			      render_blend_light_check [(a | s[BLUE_PIX])];
			  }
		      }
		    else
		      {
			render_temp_buf[j * 3 + 0] = s[RED_PIX];
			render_temp_buf[j * 3 + 1] = s[GREEN_PIX];
			render_temp_buf[j * 3 + 2] = s[BLUE_PIX];
		      }
		  }
		else
		  {
		    if (has_alpha)
		      {
			a = s[alpha] << 8;

			if ((j + offset) & 0x4)
			  {
			    if (color_buf)
			      {
				render_temp_buf[j * 3 + 0] = 
				  render_blend_dark_check [(a | s[GRAY_PIX])];
				render_temp_buf[j * 3 + 1] = 
				  render_blend_dark_check [(a | s[GRAY_PIX])];
				render_temp_buf[j * 3 + 2] = 
				  render_blend_dark_check [(a | s[GRAY_PIX])];
			      }
			    else
			      {
				render_temp_buf[j] = 
				  render_blend_dark_check [(a | s[GRAY_PIX + channel])];
			      }
			  }
			else
			  {
			    if (color_buf)
			      {
				render_temp_buf[j * 3 + 0] = 
				  render_blend_light_check [(a | s[GRAY_PIX])];
				render_temp_buf[j * 3 + 1] = 
				  render_blend_light_check [(a | s[GRAY_PIX])];
				render_temp_buf[j * 3 + 2] = 
				  render_blend_light_check [(a | s[GRAY_PIX])];
			      }
			    else
			      {
				render_temp_buf[j] = 
				  render_blend_light_check [(a | s[GRAY_PIX + channel])];
			      }
			  }
		      }
		    else
		      {
			if (color_buf)
			  {
			    render_temp_buf[j * 3 + 0] = s[GRAY_PIX];
			    render_temp_buf[j * 3 + 1] = s[GRAY_PIX];
			    render_temp_buf[j * 3 + 2] = s[GRAY_PIX];
			  }
			else
			  {
			    render_temp_buf[j] = s[GRAY_PIX + channel];
			  }
		      }
		  }

		s += temp_buf->bytes;
	      }

	    /*  Handle the trailing transparency  */
	    for (j = x2; j < width; j++)
	      for (b = 0; b < image_bytes; b++)
		render_temp_buf[j * image_bytes + b] = cb[j * 3 + b];

	    src += rowstride;
	  }
	else
	  {
	    for (j = 0; j < width; j++)
	      for (b = 0; b < image_bytes; b++)
		render_temp_buf[j * image_bytes + b] = cb[j * 3 + b];
	  }

	gtk_preview_draw_row (GTK_PREVIEW (preview),
			      render_temp_buf, 0, i, width);
      }
  }

  temp_buf_free (temp_buf);

  gtk_widget_queue_draw (GTK_WIDGET (preview));

  return FALSE;
}
