/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpcontainer.h"
#include "core/gimpdatafactory.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimpimagefile.h"
#include "core/gimpitem.h"
#include "core/gimppalette.h"
#include "core/gimppattern.h"
#include "core/gimptoolinfo.h"

#include "text/gimpfont.h"

#include "gimpselectiondata.h"

#include "gimp-intl.h"


/* #define DEBUG_DND */

#ifdef DEBUG_DND
#define D(stmnt) stmnt
#else
#define D(stmnt)
#endif


void
gimp_selection_data_set_uri_list (GtkSelectionData *selection,
                                  GdkAtom           atom,
                                  GList            *uri_list)
{
  GList *list;
  gchar *vals = NULL;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (atom != GDK_NONE);
  g_return_if_fail (uri_list != NULL);

  for (list = uri_list; list; list = g_list_next (list))
    {
      if (vals)
        {
          gchar *tmp = g_strconcat (vals,
                                    list->data,
                                    list->next ? "\n" : NULL,
                                    NULL);
          g_free (vals);
          vals = tmp;
        }
      else
        {
          vals = g_strconcat (list->data,
                              list->next ? "\n" : NULL,
                              NULL);
        }
    }

  gtk_selection_data_set (selection, atom,
                          8, (guchar *) vals, strlen (vals) + 1);

  g_free (vals);
}

/*  the next two functions are straight cut'n'paste from glib/glib/gconvert.c,
 *  except that gimp_unescape_uri_string() does not try to UTF-8 validate
 *  the unescaped result.
 */
static int
unescape_character (const char *scanner)
{
  int first_digit;
  int second_digit;

  first_digit = g_ascii_xdigit_value (scanner[0]);
  if (first_digit < 0)
    return -1;

  second_digit = g_ascii_xdigit_value (scanner[1]);
  if (second_digit < 0)
    return -1;

  return (first_digit << 4) | second_digit;
}

static gchar *
gimp_unescape_uri_string (const char *escaped,
                          int         len,
                          const char *illegal_escaped_characters,
                          gboolean    ascii_must_not_be_escaped)
{
  const gchar *in, *in_end;
  gchar *out, *result;
  int c;

  if (escaped == NULL)
    return NULL;

  if (len < 0)
    len = strlen (escaped);

  result = g_malloc (len + 1);

  out = result;
  for (in = escaped, in_end = escaped + len; in < in_end; in++)
    {
      c = *in;

      if (c == '%')
        {
          /* catch partial escape sequences past the end of the substring */
          if (in + 3 > in_end)
            break;

          c = unescape_character (in + 1);

          /* catch bad escape sequences and NUL characters */
          if (c <= 0)
            break;

          /* catch escaped ASCII */
          if (ascii_must_not_be_escaped && c <= 0x7F)
            break;

          /* catch other illegal escaped characters */
          if (strchr (illegal_escaped_characters, c) != NULL)
            break;

          in += 2;
        }

      *out++ = c;
    }

  g_assert (out - result <= len);
  *out = '\0';

  if (in != in_end)
    {
      g_free (result);
      return NULL;
    }

  return result;
}

GList *
gimp_selection_data_get_uri_list (GtkSelectionData *selection)
{
  GList    *crap_list = NULL;
  GList    *uri_list  = NULL;
  GList    *list;
  gchar    *buffer;

  g_return_val_if_fail (selection != NULL, NULL);

  if ((selection->format != 8) || (selection->length < 1))
    {
      g_warning ("Received invalid file data!");
      return NULL;
    }

  buffer = (gchar *) selection->data;

  D (g_print ("%s: raw buffer >>%s<<\n", G_STRFUNC, buffer));

  {
    gchar name_buffer[1024];

    while (*buffer && (buffer - (gchar *) selection->data < selection->length))
      {
	gchar *name = name_buffer;
	gint   len  = 0;

	while (len < sizeof (name_buffer) && *buffer && *buffer != '\n')
	  {
	    *name++ = *buffer++;
	    len++;
	  }
	if (len == 0)
	  break;

	if (*(name - 1) == 0xd)   /* gmc uses RETURN+NEWLINE as delimiter */
	  len--;

	if (len > 2)
	  crap_list = g_list_prepend (crap_list, g_strndup (name_buffer, len));

	if (*buffer)
	  buffer++;
      }
  }

  if (! crap_list)
    return NULL;

  /*  do various checks because file drag sources send all kinds of
   *  arbitrary crap...
   */
  for (list = crap_list; list; list = g_list_next (list))
    {
      const gchar *dnd_crap = list->data;
      gchar       *filename;
      gchar       *hostname;
      gchar       *uri   = NULL;
      GError      *error = NULL;

      D (g_print ("%s: trying to convert \"%s\" to an uri.\n",
                  G_STRFUNC, dnd_crap));

      filename = g_filename_from_uri (dnd_crap, &hostname, NULL);

      if (filename)
        {
          /*  if we got a correctly encoded "file:" uri...
           *
           *  (for GLib < 2.4.4, this is escaped UTF-8,
           *   for GLib > 2.4.4, this is escaped local filename encoding)
           */

          uri = g_filename_to_uri (filename, hostname, NULL);

          g_free (hostname);
          g_free (filename);
        }
      else if (g_file_test (dnd_crap, G_FILE_TEST_EXISTS))
        {
          /*  ...else if we got a valid local filename...  */

          uri = g_filename_to_uri (dnd_crap, NULL, NULL);
        }
      else
        {
          /*  ...otherwise do evil things...  */

          const gchar *start = dnd_crap;

          if (! strncmp (dnd_crap, "file://", strlen ("file://")))
            {
              start += strlen ("file://");
            }
          else if (! strncmp (dnd_crap, "file:", strlen ("file:")))
            {
              start += strlen ("file:");
            }

          if (start != dnd_crap)
            {
              /*  try if we got a "file:" uri in the wrong encoding...
               *
               *  (for GLib < 2.4.4, this is escaped local filename encoding,
               *   for GLib > 2.4.4, this is escaped UTF-8)
               */
              gchar *unescaped_filename;

              if (strstr (dnd_crap, "%"))
                {
                  gchar *local_filename;

                  unescaped_filename = gimp_unescape_uri_string (start, -1,
                                                                 "/", FALSE);

                  /*  check if we got a drop from an application that
                   *  encodes file: URIs as UTF-8 (apps linked against
                   *  GLib < 2.4.4)
                   */
                  local_filename = g_filename_from_utf8 (unescaped_filename,
                                                         -1, NULL, NULL,
                                                         NULL);

                  if (local_filename)
                    {
                      g_free (unescaped_filename);
                      unescaped_filename = local_filename;
                    }
                }
              else
                {
                  unescaped_filename = g_strdup (start);
                }

              uri = g_filename_to_uri (unescaped_filename, NULL, &error);

              if (! uri)
                {
                  gchar *escaped_filename = g_strescape (unescaped_filename,
                                                         NULL);

                  g_message (_("The filename '%s' couldn't be converted to a "
                               "valid URI:\n\n%s"),
                             escaped_filename,
                             error->message ?
                             error->message : _("Invalid UTF-8"));
                  g_free (escaped_filename);
                  g_clear_error (&error);

                  g_free (unescaped_filename);
                  continue;
                }

              g_free (unescaped_filename);
            }
          else
            {
              /*  otherwise try the crap passed anyway, in case it's
               *  a "http:" or whatever uri a plug-in might handle
               */
              uri = g_strdup (dnd_crap);
            }
        }

      uri_list = g_list_prepend (uri_list, uri);
    }

  g_list_foreach (crap_list, (GFunc) g_free, NULL);
  g_list_free (crap_list);

  return uri_list;
}

void
gimp_selection_data_set_color (GtkSelectionData *selection,
                               GdkAtom           atom,
                               const GimpRGB    *color)
{
  guint16 *vals;
  guchar   r, g, b, a;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (atom != GDK_NONE);
  g_return_if_fail (color != NULL);

  vals = g_new (guint16, 4);

  gimp_rgba_get_uchar (color, &r, &g, &b, &a);

  vals[0] = r + (r << 8);
  vals[1] = g + (g << 8);
  vals[2] = b + (b << 8);
  vals[3] = a + (a << 8);

  gtk_selection_data_set (selection, atom,
                          16, (guchar *) vals, 8);

  g_free (vals);
}

gboolean
gimp_selection_data_get_color (GtkSelectionData *selection,
                               GimpRGB          *color)
{
  guint16 *color_vals;

  g_return_val_if_fail (selection != NULL, FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  if ((selection->format != 16) || (selection->length != 8))
    {
      g_warning ("Received invalid color data!");
      return FALSE;
    }

  color_vals = (guint16 *) selection->data;

  gimp_rgba_set_uchar (color,
		       (guchar) (color_vals[0] >> 8),
		       (guchar) (color_vals[1] >> 8),
		       (guchar) (color_vals[2] >> 8),
		       (guchar) (color_vals[3] >> 8));

  return TRUE;
}

void
gimp_selection_data_set_stream (GtkSelectionData *selection,
                                GdkAtom           atom,
                                const guchar     *stream,
                                gsize             stream_length)
{
  g_return_if_fail (selection != NULL);
  g_return_if_fail (atom != GDK_NONE);
  g_return_if_fail (stream != NULL);
  g_return_if_fail (stream_length > 0);

  gtk_selection_data_set (selection, atom,
                          8, (guchar *) stream, stream_length);
}

const guchar *
gimp_selection_data_get_stream (GtkSelectionData *selection,
                                gsize            *stream_length)
{
  g_return_val_if_fail (selection != NULL, NULL);
  g_return_val_if_fail (stream_length != NULL, NULL);

  if ((selection->format != 8) || (selection->length < 1))
    {
      g_warning ("Received invalid data stream!");
      return NULL;
    }

  *stream_length = selection->length;

  return (const guchar *) selection->data;
}

void
gimp_selection_data_set_pixbuf (GtkSelectionData *selection,
                                GdkAtom           atom,
                                GdkPixbuf        *pixbuf,
                                const gchar      *format)
{
  gchar  *buffer;
  gsize   buffer_size;
  GError *error = NULL;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (atom != GDK_NONE);
  g_return_if_fail (GDK_IS_PIXBUF (pixbuf));
  g_return_if_fail (format != NULL);

  if (gdk_pixbuf_save_to_buffer (pixbuf,
                                 &buffer, &buffer_size, format,
                                 &error, NULL))
    {
      gtk_selection_data_set (selection, atom,
                              8, (guchar *) buffer, buffer_size);
      g_free (buffer);
    }
  else
    {
      g_warning ("%s: %s", G_STRFUNC, error->message);
      g_error_free (error);
    }
}

GdkPixbuf *
gimp_selection_data_get_pixbuf (GtkSelectionData *selection)
{
  GdkPixbufLoader *loader;
  GdkPixbuf       *pixbuf = NULL;
  GError          *error  = NULL;

  g_return_val_if_fail (selection != NULL, NULL);

  if ((selection->format != 8) || (selection->length < 1))
    {
      g_warning ("Received invalid image data!");
      return NULL;
    }

  loader = gdk_pixbuf_loader_new ();

  if (gdk_pixbuf_loader_write (loader,
                               selection->data, selection->length, &error) &&
      gdk_pixbuf_loader_close (loader, &error))
    {
      pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
      g_object_ref (pixbuf);
    }
  else
    {
      g_warning ("%s: %s", G_STRFUNC, error->message);
      g_error_free (error);
    }

  g_object_unref (loader);

  return pixbuf;
}

void
gimp_selection_data_set_image (GtkSelectionData *selection,
                               GdkAtom           atom,
                               GimpImage        *gimage)
{
  gchar *id;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (atom != GDK_NONE);
  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  id = g_strdup_printf ("%d", gimp_image_get_ID (gimage));

  gtk_selection_data_set (selection, atom,
                          8, (guchar *) id, strlen (id) + 1);

  g_free (id);
}

GimpImage *
gimp_selection_data_get_image (GtkSelectionData *selection,
                               Gimp             *gimp)
{
  gchar *id;
  gint   ID;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  if ((selection->format != 8) || (selection->length < 1))
    {
      g_warning ("Received invalid image ID data!");
      return NULL;
    }

  id = (gchar *) selection->data;
  ID = atoi (id);

  if (! ID)
    return NULL;

  return gimp_image_get_by_ID (gimp, ID);
}

void
gimp_selection_data_set_item (GtkSelectionData *selection,
                              GdkAtom           atom,
                              GimpItem         *item)
{
  gchar *id;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (atom != GDK_NONE);
  g_return_if_fail (GIMP_IS_ITEM (item));

  id = g_strdup_printf ("%d", gimp_item_get_ID (item));

  gtk_selection_data_set (selection, atom,
                          8, (guchar *) id, strlen (id) + 1);

  g_free (id);
}

GimpItem *
gimp_selection_data_get_item (GtkSelectionData *selection,
                              Gimp             *gimp)
{
  gchar *id;
  gint   ID;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  if ((selection->format != 8) || (selection->length < 1))
    {
      g_warning ("Received invalid item ID data!");
      return NULL;
    }

  id = (gchar *) selection->data;
  ID = atoi (id);

  if (! ID)
    return NULL;

  return gimp_item_get_by_ID (gimp, ID);
}

void
gimp_selection_data_set_viewable (GtkSelectionData *selection,
                                  GdkAtom           atom,
                                  GimpViewable     *viewable)
{
  const gchar *name;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (atom != GDK_NONE);
  g_return_if_fail (GIMP_IS_VIEWABLE (viewable));

  name = gimp_object_get_name (GIMP_OBJECT (viewable));

  if (name)
    gtk_selection_data_set (selection, atom,
                            8, (const guchar *) name, strlen (name) + 1);
}

GimpBrush *
gimp_selection_data_get_brush (GtkSelectionData *selection,
                               Gimp             *gimp)
{
  GimpBrush *brush;
  gchar     *name;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  if ((selection->format != 8) || (selection->length < 1))
    {
      g_warning ("Received invalid brush data!");
      return NULL;
    }

  name = (gchar *) selection->data;

  if (strcmp (name, "Standard") == 0)
    brush = GIMP_BRUSH (gimp_brush_get_standard ());
  else
    brush = (GimpBrush *)
      gimp_container_get_child_by_name (gimp->brush_factory->container, name);

  return brush;
}

GimpPattern *
gimp_selection_data_get_pattern (GtkSelectionData *selection,
                                 Gimp             *gimp)
{
  GimpPattern *pattern;
  gchar       *name;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  if ((selection->format != 8) || (selection->length < 1))
    {
      g_warning ("Received invalid pattern data!");
      return NULL;
    }

  name = (gchar *) selection->data;

  if (strcmp (name, "Standard") == 0)
    pattern = GIMP_PATTERN (gimp_pattern_get_standard ());
  else
    pattern = (GimpPattern *)
      gimp_container_get_child_by_name (gimp->pattern_factory->container, name);

  return pattern;
}

GimpGradient *
gimp_selection_data_get_gradient (GtkSelectionData *selection,
                                  Gimp             *gimp)
{
  GimpGradient *gradient;
  gchar        *name;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  if ((selection->format != 8) || (selection->length < 1))
    {
      g_warning ("Received invalid gradient data!");
      return NULL;
    }

  name = (gchar *) selection->data;

  if (strcmp (name, "Standard") == 0)
    gradient = GIMP_GRADIENT (gimp_gradient_get_standard ());
  else
    gradient = (GimpGradient *)
      gimp_container_get_child_by_name (gimp->gradient_factory->container, name);

  return gradient;
}

GimpPalette *
gimp_selection_data_get_palette (GtkSelectionData *selection,
                                 Gimp             *gimp)
{
  GimpPalette *palette;
  gchar       *name;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  if ((selection->format != 8) || (selection->length < 1))
    {
      g_warning ("Received invalid palette data!");
      return NULL;
    }

  name = (gchar *) selection->data;

  if (strcmp (name, "Standard") == 0)
    palette = GIMP_PALETTE (gimp_palette_get_standard ());
  else
    palette = (GimpPalette *)
      gimp_container_get_child_by_name (gimp->palette_factory->container, name);

  return palette;
}

GimpFont *
gimp_selection_data_get_font (GtkSelectionData *selection,
                              Gimp             *gimp)
{
  GimpFont *font;
  gchar    *name;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  if ((selection->format != 8) || (selection->length < 1))
    {
      g_warning ("Received invalid font data!");
      return NULL;
    }

  name = (gchar *) selection->data;

  if (strcmp (name, "Standard") == 0)
    font = gimp_font_get_standard ();
  else
    font = (GimpFont *)
      gimp_container_get_child_by_name (gimp->fonts, name);

  return font;
}

GimpBuffer *
gimp_selection_data_get_buffer (GtkSelectionData *selection,
                                Gimp             *gimp)
{
  GimpBuffer *buffer;
  gchar      *name;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  if ((selection->format != 8) || (selection->length < 1))
    {
      g_warning ("Received invalid buffer data!");
      return NULL;
    }

  name = (gchar *) selection->data;

  buffer = (GimpBuffer *)
    gimp_container_get_child_by_name (gimp->named_buffers, name);

  return buffer;
}

GimpImagefile *
gimp_selection_data_get_imagefile (GtkSelectionData *selection,
                                   Gimp             *gimp)
{
  GimpImagefile *imagefile;
  gchar         *name;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  if ((selection->format != 8) || (selection->length < 1))
    {
      g_warning ("Received invalid imagefile data!");
      return NULL;
    }

  name = (gchar *) selection->data;

  imagefile = (GimpImagefile *)
    gimp_container_get_child_by_name (gimp->documents, name);

  return imagefile;
}

GimpTemplate *
gimp_selection_data_get_template (GtkSelectionData *selection,
                                  Gimp             *gimp)
{
  GimpTemplate *template;
  gchar        *name;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  if ((selection->format != 8) || (selection->length < 1))
    {
      g_warning ("Received invalid template data!");
      return NULL;
    }

  name = (gchar *) selection->data;

  template = (GimpTemplate *)
    gimp_container_get_child_by_name (gimp->templates, name);

  return template;
}

GimpToolInfo *
gimp_selection_data_get_tool (GtkSelectionData *selection,
                              Gimp             *gimp)
{
  GimpToolInfo *tool_info;
  gchar        *name;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  if ((selection->format != 8) || (selection->length < 1))
    {
      g_warning ("Received invalid tool data!");
      return NULL;
    }

  name = (gchar *) selection->data;

  if (strcmp (name, "gimp-standard-tool") == 0)
    tool_info = gimp_tool_info_get_standard (gimp);
  else
    tool_info = (GimpToolInfo *)
      gimp_container_get_child_by_name (gimp->tool_info_list, name);

  return tool_info;
}
