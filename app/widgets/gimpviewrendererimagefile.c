/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrendererimagefile.c
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

#include "core/gimpimagefile.h"

#include "gimpviewrendererimagefile.h"
#include "gimpviewrenderer-frame.h"

#ifdef ENABLE_FILE_SYSTEM_ICONS
#define GTK_FILE_SYSTEM_ENABLE_UNSUPPORTED
#include <gtk/gtkfilesystem.h>
#endif


static void   gimp_view_renderer_imagefile_render (GimpViewRenderer *renderer,
                                                   GtkWidget        *widget);


G_DEFINE_TYPE (GimpViewRendererImagefile, gimp_view_renderer_imagefile,
               GIMP_TYPE_VIEW_RENDERER)

#define parent_class gimp_view_renderer_imagefile_parent_class


static void
gimp_view_renderer_imagefile_class_init (GimpViewRendererImagefileClass *klass)
{
  GimpViewRendererClass *renderer_class = GIMP_VIEW_RENDERER_CLASS (klass);

  renderer_class->render = gimp_view_renderer_imagefile_render;
}

static void
gimp_view_renderer_imagefile_init (GimpViewRendererImagefile *renderer)
{
#ifdef ENABLE_FILE_SYSTEM_ICONS
  renderer->file_system = NULL;
#endif
}

static void
gimp_view_renderer_imagefile_render (GimpViewRenderer *renderer,
                                     GtkWidget        *widget)
{
  GdkPixbuf *pixbuf = gimp_view_renderer_get_frame_pixbuf (renderer, widget,
                                                           renderer->width,
                                                           renderer->height);

#ifdef ENABLE_FILE_SYSTEM_ICONS
  if (! pixbuf &&
      GIMP_VIEW_RENDERER_IMAGEFILE (renderer)->file_system)
    {
      const gchar *uri;

      uri = gimp_object_get_name (GIMP_OBJECT (renderer->viewable));
      if (uri)
        {
          GtkFileSystem *file_system;
          GtkFilePath   *path;

          file_system = GIMP_VIEW_RENDERER_IMAGEFILE (renderer)->file_system;

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
      gimp_view_renderer_render_pixbuf (renderer, pixbuf);
      g_object_unref (pixbuf);
    }
  else
    {
      const gchar *stock_id = gimp_viewable_get_stock_id (renderer->viewable);

      gimp_view_renderer_default_render_stock (renderer, widget, stock_id);
    }
}
