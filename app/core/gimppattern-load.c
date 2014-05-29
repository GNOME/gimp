/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <fcntl.h>

#include <glib.h>
#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimppattern.h"
#include "gimppattern-header.h"
#include "gimppattern-load.h"
#include "gimptempbuf.h"

#include "gimp-intl.h"


GList *
gimp_pattern_load (GimpContext  *context,
                   const gchar  *filename,
                   GError      **error)
{
  GimpPattern   *pattern = NULL;
  const Babl    *format  = NULL;
  gint           fd;
  PatternHeader  header;
  gint           bn_size;
  gchar         *name    = NULL;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (filename), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  fd = g_open (filename, O_RDONLY | _O_BINARY, 0);
  if (fd == -1)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  /*  Read in the header size  */
  if (read (fd, &header, sizeof (header)) != sizeof (header))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in pattern file '%s': "
                     "File appears truncated."),
                   gimp_filename_to_utf8 (filename));
      goto error;
    }

  /*  rearrange the bytes in each unsigned int  */
  header.header_size  = g_ntohl (header.header_size);
  header.version      = g_ntohl (header.version);
  header.width        = g_ntohl (header.width);
  header.height       = g_ntohl (header.height);
  header.bytes        = g_ntohl (header.bytes);
  header.magic_number = g_ntohl (header.magic_number);

  /*  Check for correct file format */
  if (header.magic_number != GPATTERN_MAGIC || header.version != 1 ||
      header.header_size <= sizeof (header))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in pattern file '%s': "
                     "Unknown pattern format version %d."),
                   gimp_filename_to_utf8 (filename), header.version);
      goto error;
    }

  /*  Check for supported bit depths  */
  if (header.bytes < 1 || header.bytes > 4)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in pattern file '%s: "
                     "Unsupported pattern depth %d.\n"
                     "GIMP Patterns must be GRAY or RGB."),
                   gimp_filename_to_utf8 (filename), header.bytes);
      goto error;
    }

  /*  Read in the pattern name  */
  if ((bn_size = (header.header_size - sizeof (header))))
    {
      gchar *utf8;

      name = g_new (gchar, bn_size);

      if ((read (fd, name, bn_size)) < bn_size)
        {
          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Fatal parse error in pattern file '%s': "
                         "File appears truncated."),
                       gimp_filename_to_utf8 (filename));
          g_free (name);
          goto error;
        }

      utf8 = gimp_any_to_utf8 (name, -1,
                               _("Invalid UTF-8 string in pattern file '%s'."),
                               gimp_filename_to_utf8 (filename));
      g_free (name);
      name = utf8;
    }

  if (! name)
    name = g_strdup (_("Unnamed"));

  pattern = g_object_new (GIMP_TYPE_PATTERN,
                          "name",      name,
                          "mime-type", "image/x-gimp-pat",
                          NULL);

  g_free (name);

  switch (header.bytes)
    {
    case 1: format = babl_format ("Y' u8");      break;
    case 2: format = babl_format ("Y'A u8");     break;
    case 3: format = babl_format ("R'G'B' u8");  break;
    case 4: format = babl_format ("R'G'B'A u8"); break;
    }

  pattern->mask = gimp_temp_buf_new (header.width, header.height, format);

  if (read (fd, gimp_temp_buf_get_data (pattern->mask),
            header.width * header.height * header.bytes) <
      header.width * header.height * header.bytes)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in pattern file '%s': "
                     "File appears truncated."),
                   gimp_filename_to_utf8 (filename));
      goto error;
    }

  close (fd);

  return g_list_prepend (NULL, pattern);

 error:
  if (pattern)
    g_object_unref (pattern);

  close (fd);

  return NULL;
}

GList *
gimp_pattern_load_pixbuf (GimpContext  *context,
                          const gchar  *filename,
                          GError      **error)
{
  GimpPattern *pattern;
  GdkPixbuf   *pixbuf;
  gchar       *name;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (filename), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  pixbuf = gdk_pixbuf_new_from_file (filename, error);

  if (! pixbuf)
    return NULL;

  name = g_strdup (gdk_pixbuf_get_option (pixbuf, "tEXt::Title"));

  if (! name)
    name = g_strdup (gdk_pixbuf_get_option (pixbuf, "tEXt::Comment"));

  if (! name)
    name = g_filename_display_basename (filename);

  pattern = g_object_new (GIMP_TYPE_PATTERN,
                          "name",      name,
                          "mime-type", NULL, /* FIXME!! */
                          NULL);
  g_free (name);

  pattern->mask = gimp_temp_buf_new_from_pixbuf (pixbuf, NULL);

  g_object_unref (pixbuf);

  return g_list_prepend (NULL, pattern);
}
