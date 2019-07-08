/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppixbuf.c
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "gimppixbuf.h"


/*  local function prototypes  */

static gint   gimp_pixbuf_format_compare (GdkPixbufFormat *a,
                                          GdkPixbufFormat *b);


/*  public functions  */

GSList *
gimp_pixbuf_get_formats (void)
{
  return g_slist_sort (gdk_pixbuf_get_formats (),
                       (GCompareFunc) gimp_pixbuf_format_compare);
}

void
gimp_pixbuf_targets_add (GtkTargetList *target_list,
                         guint          info,
                         gboolean       writable)
{
  GSList *formats;
  GSList *list;

  g_return_if_fail (target_list != NULL);

  formats = gimp_pixbuf_get_formats ();

  for (list = formats; list; list = g_slist_next (list))
    {
      GdkPixbufFormat  *format = list->data;
      gchar           **mime_types;
      gchar           **type;

      if (writable && ! gdk_pixbuf_format_is_writable (format))
        continue;

      mime_types = gdk_pixbuf_format_get_mime_types (format);

      for (type = mime_types; *type; type++)
        {
          /*  skip Windows ICO as writable format  */
          if (writable && strcmp (*type, "image/x-icon") == 0)
            continue;

          gtk_target_list_add (target_list,
                               gdk_atom_intern (*type, FALSE), 0, info);

        }

      g_strfreev (mime_types);
    }

  g_slist_free (formats);
}

void
gimp_pixbuf_targets_remove (GtkTargetList *target_list)
{
  GSList *formats;
  GSList *list;

  g_return_if_fail (target_list != NULL);

  formats = gimp_pixbuf_get_formats ();

  for (list = formats; list; list = g_slist_next (list))
    {
      GdkPixbufFormat  *format = list->data;
      gchar           **mime_types;
      gchar           **type;

      mime_types = gdk_pixbuf_format_get_mime_types (format);

      for (type = mime_types; *type; type++)
        {
          gtk_target_list_remove (target_list,
                                  gdk_atom_intern (*type, FALSE));
        }

      g_strfreev (mime_types);
    }

  g_slist_free (formats);
}


/*  private functions  */

static gint
gimp_pixbuf_format_compare (GdkPixbufFormat *a,
                            GdkPixbufFormat *b)
{
  gchar *a_name = gdk_pixbuf_format_get_name (a);
  gchar *b_name = gdk_pixbuf_format_get_name (b);
  gint   retval = 0;

  /*  move PNG to the front of the list  */
  if (strcmp (a_name, "png") == 0)
    retval = -1;
  else if (strcmp (b_name, "png") == 0)
    retval = 1;

  /*  move JPEG to the end of the list  */
  else if (strcmp (a_name, "jpeg") == 0)
    retval = 1;
  else if (strcmp (b_name, "jpeg") == 0)
    retval = -1;

  /*  move GIF to the end of the list  */
  else if (strcmp (a_name, "gif") == 0)
    retval = 1;
  else if (strcmp (b_name, "gif") == 0)
    retval = -1;

  g_free (a_name);
  g_free (b_name);

  return retval;
}
