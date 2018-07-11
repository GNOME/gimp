/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewrendererimagefile.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpthumb/gimpthumb.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpimagefile.h"

#include "gimpviewrendererimagefile.h"
#include "gimpviewrenderer-frame.h"
#include "gimpwidgets-utils.h"


static void        gimp_view_renderer_imagefile_render   (GimpViewRenderer *renderer,
                                                          GtkWidget        *widget);

static GdkPixbuf * gimp_view_renderer_imagefile_get_icon (GimpImagefile    *imagefile,
                                                          GtkWidget        *widget,
                                                          gint              size);


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
}

static void
gimp_view_renderer_imagefile_render (GimpViewRenderer *renderer,
                                     GtkWidget        *widget)
{
  GdkPixbuf *pixbuf = gimp_view_renderer_get_frame_pixbuf (renderer, widget,
                                                           renderer->width,
                                                           renderer->height);

  if (! pixbuf)
    {
      GimpImagefile *imagefile = GIMP_IMAGEFILE (renderer->viewable);

      pixbuf = gimp_view_renderer_imagefile_get_icon (imagefile,
                                                      widget,
                                                      MIN (renderer->width,
                                                           renderer->height));
    }

  if (pixbuf)
    {
      gimp_view_renderer_render_pixbuf (renderer, widget, pixbuf);
      g_object_unref (pixbuf);
    }
  else
    {
      const gchar *icon_name = gimp_viewable_get_icon_name (renderer->viewable);

      gimp_view_renderer_render_icon (renderer, widget, icon_name);
    }
}


/* The code to get an icon for a mime-type is lifted from GtkRecentManager. */

static GdkPixbuf *
get_icon_for_mime_type (const gchar *mime_type,
                        gint         pixel_size)
{
  GtkIconTheme *icon_theme;
  const gchar  *separator;
  GString      *icon_name;
  GdkPixbuf    *pixbuf;

  separator = strchr (mime_type, '/');
  if (! separator)
    return NULL;

  icon_theme = gtk_icon_theme_get_default ();

  /* try with the three icon name variants for MIME types */

  /* canonicalize MIME type: foo/x-bar -> foo-x-bar */
  icon_name = g_string_new (NULL);
  g_string_append_len (icon_name, mime_type, separator - mime_type);
  g_string_append_c (icon_name, '-');
  g_string_append (icon_name, separator + 1);
  pixbuf = gtk_icon_theme_load_icon (icon_theme, icon_name->str,
                                     pixel_size, 0, NULL);
  g_string_free (icon_name, TRUE);
  if (pixbuf)
    return pixbuf;

  /* canonicalize MIME type, and prepend "gnome-mime-" */
  icon_name = g_string_new ("gnome-mime-");
  g_string_append_len (icon_name, mime_type, separator - mime_type);
  g_string_append_c (icon_name, '-');
  g_string_append (icon_name, separator + 1);
  pixbuf = gtk_icon_theme_load_icon (icon_theme, icon_name->str,
                                     pixel_size, 0, NULL);
  g_string_free (icon_name, TRUE);
  if (pixbuf)
    return pixbuf;

  /* try the MIME family icon */
  icon_name = g_string_new ("gnome-mime-");
  g_string_append_len (icon_name, mime_type, separator - mime_type);
  pixbuf = gtk_icon_theme_load_icon (icon_theme, icon_name->str,
                                     pixel_size, 0, NULL);
  g_string_free (icon_name, TRUE);

  return pixbuf;
}

static GdkPixbuf *
gimp_view_renderer_imagefile_get_icon (GimpImagefile *imagefile,
                                       GtkWidget     *widget,
                                       gint           size)
{
  GdkScreen     *screen     = gtk_widget_get_screen (widget);
  GtkIconTheme  *icon_theme = gtk_icon_theme_get_for_screen (screen);
  GimpThumbnail *thumbnail  = gimp_imagefile_get_thumbnail (imagefile);
  GdkPixbuf     *pixbuf     = NULL;

  if (! gimp_object_get_name (imagefile))
    return NULL;

  if (! pixbuf)
    {
      GIcon *icon = gimp_imagefile_get_gicon (imagefile);

      if (icon)
        {
          GtkIconInfo *info;

          info = gtk_icon_theme_lookup_by_gicon (icon_theme, icon, size, 0);

          if (info)
            {
              pixbuf = gtk_icon_info_load_icon (info, NULL);

              gtk_icon_info_free (info);
            }
        }
    }

  if (! pixbuf && thumbnail->image_mimetype)
    {
      pixbuf = get_icon_for_mime_type (thumbnail->image_mimetype, size);
    }

  if (! pixbuf)
    {
      const gchar *icon_name = "text-x-generic";

      if (thumbnail->image_state == GIMP_THUMB_STATE_FOLDER)
        icon_name = "folder";

      pixbuf = gtk_icon_theme_load_icon (icon_theme,
                                         icon_name, size,
                                         GTK_ICON_LOOKUP_USE_BUILTIN,
                                         NULL);
    }

  return pixbuf;
}
