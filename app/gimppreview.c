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


#define PREVIEW_EVENT_MASK  (GDK_BUTTON_PRESS_MASK   | \
                             GDK_BUTTON_RELEASE_MASK | \
                             GDK_ENTER_NOTIFY_MASK   | \
                             GDK_LEAVE_NOTIFY_MASK)


static void       gimp_preview_class_init           (GimpPreviewClass *klass);
static void       gimp_preview_init                 (GimpPreview      *preview);
static void       gimp_preview_destroy              (GtkObject        *object);
static void       gimp_preview_size_allocate        (GtkWidget        *widget,
						     GtkAllocation    *allocation);
static gint       gimp_preview_button_press_event   (GtkWidget        *widget,
						     GdkEventButton   *bevent);
static gint       gimp_preview_button_release_event (GtkWidget        *widget, 
						     GdkEventButton   *bevent);
static void       gimp_preview_paint                (GimpPreview      *preview);
static gboolean   gimp_preview_idle_paint           (GimpPreview      *preview);


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

  object_class->destroy = gimp_preview_destroy;

  widget_class->size_allocate        = gimp_preview_size_allocate;
  widget_class->button_press_event   = gimp_preview_button_press_event;
  widget_class->button_release_event = gimp_preview_button_release_event;
}

static void
gimp_preview_init (GimpPreview *preview)
{
  preview->viewable = NULL;
  preview->idle_id  = 0;

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
		  gint          height)
{
  GimpPreview *preview;

  g_return_val_if_fail (viewable != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (width  > 0 && width  <= 64, NULL);
  g_return_val_if_fail (height > 0 && height <= 64, NULL);

  preview = gtk_type_new (GIMP_TYPE_PREVIEW);

  preview->viewable = viewable;

  gtk_signal_connect_object_while_alive (GTK_OBJECT (viewable),
					 "invalidate_preview",
					 GTK_SIGNAL_FUNC (gimp_preview_paint),
					 GTK_OBJECT (preview));

  gtk_preview_size (GTK_PREVIEW (preview), width, height);

  return GTK_WIDGET (preview);
}

static gint
gimp_preview_button_press_event (GtkWidget      *widget,
				 GdkEventButton *bevent)
{
  return FALSE;
}
  
static gint
gimp_preview_button_release_event (GtkWidget      *widget,
				   GdkEventButton *bevent)
{
  return FALSE;
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
