/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppreviewrendererimagefile.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include "libgimpthumb/gimpthumb.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimpimagefile.h"

#include "gimppreviewrendererimagefile.h"

#ifdef ENABLE_FILE_SYSTEM_ICONS
#define GTK_FILE_SYSTEM_ENABLE_UNSUPPORTED
#include <gtk/gtkfilesystem.h>
#endif


static void   gimp_preview_renderer_imagefile_class_init (GimpPreviewRendererImagefileClass *klass);
static void   gimp_preview_renderer_imagefile_init       (GimpPreviewRendererImagefile      *renderer);

static void   gimp_preview_renderer_imagefile_render     (GimpPreviewRenderer *renderer,
                                                          GtkWidget           *widget);


static GimpPreviewRendererClass *parent_class = NULL;


GType
gimp_preview_renderer_imagefile_get_type (void)
{
  static GType renderer_type = 0;

  if (! renderer_type)
    {
      static const GTypeInfo renderer_info =
      {
        sizeof (GimpPreviewRendererImagefileClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_preview_renderer_imagefile_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpPreviewRendererImagefile),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_preview_renderer_imagefile_init,
      };

      renderer_type = g_type_register_static (GIMP_TYPE_PREVIEW_RENDERER,
                                              "GimpPreviewRendererImagefile",
                                              &renderer_info, 0);
    }

  return renderer_type;
}

static void
gimp_preview_renderer_imagefile_class_init (GimpPreviewRendererImagefileClass *klass)
{
  GimpPreviewRendererClass *renderer_class;

  renderer_class = GIMP_PREVIEW_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  renderer_class->render = gimp_preview_renderer_imagefile_render;
}

static void
gimp_preview_renderer_imagefile_init (GimpPreviewRendererImagefile *renderer)
{
#ifdef ENABLE_FILE_SYSTEM_ICONS
  renderer->file_system = NULL;
#endif
}

static void
gimp_preview_renderer_imagefile_render (GimpPreviewRenderer *renderer,
                                        GtkWidget           *widget)
{
  TempBuf *temp_buf = gimp_viewable_get_preview (renderer->viewable,
                                                 renderer->width,
                                                 renderer->height);

  if (temp_buf)
    {
      gimp_preview_renderer_default_render_buffer (renderer, widget, temp_buf);
    }
  else
    {
      GdkPixbuf *pixbuf = NULL;

#ifdef ENABLE_FILE_SYSTEM_ICONS
      if (GIMP_PREVIEW_RENDERER_IMAGEFILE (renderer)->file_system)
        {
          GtkFileSystem *file_system;
          const gchar   *uri;

          file_system = GIMP_PREVIEW_RENDERER_IMAGEFILE (renderer)->file_system;

          uri = gimp_object_get_name (GIMP_OBJECT (renderer->viewable));

          if (uri)
            {
              GtkFilePath *path;

              path = gtk_file_system_uri_to_path (file_system, uri);
              pixbuf = gtk_file_system_render_icon (file_system, path, widget,
                                                    MIN (renderer->width,
                                                         renderer->height),
                                                    NULL);
              gtk_file_path_free (path);
            }
        }
#endif /* ENABLE_FILE_SYSTEM_ICONS */

      if (pixbuf)
        {
          gint    width;
          gint    height;
          gint    bytes;
          guchar *src;
          guchar *dest;
          gint    y;

          width  = gdk_pixbuf_get_width (pixbuf);
          height = gdk_pixbuf_get_height (pixbuf);
          bytes  = gdk_pixbuf_get_n_channels (pixbuf);

          temp_buf = temp_buf_new (width, height, bytes, 0, 0, NULL);

          dest = temp_buf_data (temp_buf);
          src  = gdk_pixbuf_get_pixels (pixbuf);

          for (y = 0; y < height; y++)
            {
              memcpy (dest, src, width * bytes);
              dest += width * bytes;
              src += gdk_pixbuf_get_rowstride (pixbuf);
            }

          if (temp_buf->width < renderer->width)
            temp_buf->x = (renderer->width - temp_buf->width)  / 2;

          if (temp_buf->height < renderer->height)
            temp_buf->y = (renderer->height - temp_buf->height) / 2;

          gimp_preview_renderer_render_buffer (renderer, temp_buf, -1,
                                               GIMP_PREVIEW_BG_WHITE,
                                               GIMP_PREVIEW_BG_WHITE);

          temp_buf_free (temp_buf);
          g_object_unref (pixbuf);
        }
      else
        {
          const gchar  *stock_id;

          stock_id = gimp_viewable_get_stock_id (renderer->viewable);

          gimp_preview_renderer_default_render_stock (renderer, widget,
                                                      stock_id);
        }
    }
}
