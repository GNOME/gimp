/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpImagefilePreview Widget
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

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimpimagefile.h"

#include "gimpimagefilepreview.h"


static void     gimp_imagefile_preview_class_init  (GimpImagefilePreviewClass *klass);

static void     gimp_imagefile_finalize            (GObject        *object);
static gboolean gimp_imagefile_expose_event        (GtkWidget      *widget,
                                                    GdkEventExpose *event);
static void     gimp_imagefile_preview_render      (GimpPreview    *preview);
static gboolean gimp_imagefile_preview_needs_popup (GimpPreview    *preview);


static GimpPreviewClass *parent_class = NULL;


GType
gimp_imagefile_preview_get_type (void)
{
  static GType preview_type = 0;

  if (! preview_type)
    {
      static const GTypeInfo preview_info =
      {
        sizeof (GimpImagefilePreviewClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_imagefile_preview_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpImagefilePreview),
        0,              /* n_preallocs */
        NULL            /* instance_init */
      };

      preview_type = g_type_register_static (GIMP_TYPE_PREVIEW,
                                             "GimpImagefilePreview",
                                             &preview_info, 0);
    }
  
  return preview_type;
}

static void
gimp_imagefile_preview_class_init (GimpImagefilePreviewClass *klass)
{
  GObjectClass     *object_class;
  GtkWidgetClass   *widget_class;
  GimpPreviewClass *preview_class;

  parent_class = g_type_class_peek_parent (klass);

  object_class  = G_OBJECT_CLASS (klass);
  widget_class  = GTK_WIDGET_CLASS (klass);
  preview_class = GIMP_PREVIEW_CLASS (klass);

  object_class->finalize     = gimp_imagefile_finalize;

  widget_class->expose_event = gimp_imagefile_expose_event;

  preview_class->needs_popup = gimp_imagefile_preview_needs_popup;
  preview_class->render      = gimp_imagefile_preview_render;
}

static void
gimp_imagefile_finalize (GObject *object)
{
  GimpImagefilePreview *preview = GIMP_IMAGEFILE_PREVIEW (object);

  if (preview->no_preview_pixbuf)
    g_object_unref (preview->no_preview_pixbuf);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_imagefile_expose_event (GtkWidget      *widget,
                             GdkEventExpose *event)
{
  GimpImagefilePreview *preview = GIMP_IMAGEFILE_PREVIEW (widget);
  GdkRectangle          draw_rect;
  GdkRectangle          rect; 

  if (!GTK_WIDGET_DRAWABLE (widget))
    return FALSE;

  if (!preview->no_preview_pixbuf)
    return GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);

  rect.width  = gdk_pixbuf_get_width  (preview->no_preview_pixbuf);
  rect.height = gdk_pixbuf_get_height (preview->no_preview_pixbuf);
  rect.x      = (widget->allocation.width  - rect.width)  / 2;
  rect.y      = (widget->allocation.height - rect.height) / 2;

  if (gdk_rectangle_intersect (&rect, &event->area, &draw_rect))
    gdk_pixbuf_render_to_drawable (preview->no_preview_pixbuf,
                                   GDK_DRAWABLE (widget->window),
                                   widget->style->bg_gc[widget->state],
                                   draw_rect.x - rect.x,
                                   draw_rect.y - rect.y,
                                   draw_rect.x, draw_rect.y, 
                                   draw_rect.width, draw_rect.height,
                                   GDK_RGB_DITHER_NORMAL,
                                   event->area.x, event->area.y);

  return FALSE;
}

static void
gimp_imagefile_preview_render (GimpPreview *preview)
{
  GimpImagefilePreview *file_preview = GIMP_IMAGEFILE_PREVIEW (preview);
  TempBuf              *render_buf;

  g_return_if_fail (preview->width > 0 && preview->height > 0);

  render_buf = gimp_viewable_get_new_preview (preview->viewable,
                                              preview->width, 
                                              preview->height);

  if (render_buf)
    {
      TempBuf *temp_buf;
      guchar   white[4] = { 255, 255, 255, 255 };

      if (file_preview->no_preview_pixbuf)
        {
          g_object_unref (file_preview->no_preview_pixbuf);
          file_preview->no_preview_pixbuf = NULL;
        }

      if (render_buf->width > preview->width || 
          render_buf->height > preview->height)
        {
          gdouble  ratio;
          gint     width;
          gint     height;
          
          if (render_buf->width >= render_buf->height)
            ratio = (gdouble) preview->width / (gdouble) render_buf->width;
          else
            ratio = (gdouble) preview->height / (gdouble) render_buf->height;
          
          width  = RINT (ratio * render_buf->width); 
          height = RINT (ratio * render_buf->height); 
          
          temp_buf = render_buf;
          render_buf = temp_buf_scale (temp_buf, width, height);
          temp_buf_free (temp_buf);
        }
      
      if (render_buf->width != preview->width || 
          render_buf->height != preview->height)
        {
          gint   x, y;
          
          x = (preview->width  - render_buf->width)  / 2;
          y = (preview->height - render_buf->height) / 2;
          
          temp_buf = temp_buf_new (preview->width, preview->height,
                                   render_buf->bytes,
                                   0, 0,
                                   white);          
          temp_buf_copy_area (render_buf, temp_buf,
                              0, 0,
                              render_buf->width,
                              render_buf->height,
                              x, y);
          temp_buf_free (render_buf);
     
          render_buf = temp_buf;
        }

      gimp_preview_render_and_flush (preview, render_buf, -1);
      
      temp_buf_free (render_buf);
    }
  else  /* no preview available */
    {
      GdkPixbuf *pixbuf;
      gint       width, height;

      if (preview->buffer)
        {
          g_free (preview->buffer);
          preview->buffer = NULL;
        }

      if (file_preview->no_preview_pixbuf)
        {
          g_object_unref (file_preview->no_preview_pixbuf);
          file_preview->no_preview_pixbuf = NULL;
        }

      pixbuf = gtk_widget_render_icon (GTK_WIDGET (preview),
                                       GTK_STOCK_DIALOG_QUESTION,
                                       GTK_ICON_SIZE_DIALOG,
                                       NULL);
      if (pixbuf)
        {
          width  = gdk_pixbuf_get_width (pixbuf);
          height = gdk_pixbuf_get_height (pixbuf);
          
          if (width > preview->width || height > preview->height)
            {
              gdouble ratio = 
                MIN ((gdouble) preview->width  / (gdouble) width,
                     (gdouble) preview->height / (gdouble) height);

              width  = ratio * (gdouble) width;
              height = ratio * (gdouble) height;
              
              file_preview->no_preview_pixbuf = 
                gdk_pixbuf_scale_simple (pixbuf, width, height,
                                         GDK_INTERP_BILINEAR);
              g_object_unref (pixbuf);
            }
          else
            file_preview->no_preview_pixbuf = pixbuf;
        }

      gtk_widget_queue_draw (GTK_WIDGET (preview));
    }
}

static gboolean
gimp_imagefile_preview_needs_popup (GimpPreview *preview)
{
  return FALSE;
}
