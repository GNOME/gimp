/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>

#include <glib-object.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "config/gimpbaseconfig.h"

#include "base/temp-buf.h"

#include "gimpimage.h"
#include "gimppattern.h"
#include "gimppattern-header.h"

#include "gimp-intl.h"


static void       gimp_pattern_class_init      (GimpPatternClass *klass);
static void       gimp_pattern_init            (GimpPattern      *pattern);

static void       gimp_pattern_finalize        (GObject          *object);

static gint64     gimp_pattern_get_memsize     (GimpObject       *object,
                                                gint64           *gui_size);

static gboolean   gimp_pattern_get_popup_size  (GimpViewable     *viewable,
                                                gint              width,
                                                gint              height,
                                                gboolean          dot_for_dot,
                                                gint             *popup_width,
                                                gint             *popup_heigh);
static TempBuf  * gimp_pattern_get_new_preview (GimpViewable     *viewable,
                                                gint              width,
                                                gint              height);
static gchar    * gimp_pattern_get_description (GimpViewable     *viewable,
                                                gchar           **tooltip);
static gchar    * gimp_pattern_get_extension   (GimpData         *data);
static GimpData * gimp_pattern_duplicate       (GimpData         *data,
                                                gboolean          stingy_memory_use);


static GimpDataClass *parent_class = NULL;


GType
gimp_pattern_get_type (void)
{
  static GType pattern_type = 0;

  if (! pattern_type)
    {
      static const GTypeInfo pattern_info =
      {
        sizeof (GimpPatternClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_pattern_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpPattern),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_pattern_init,
      };

      pattern_type = g_type_register_static (GIMP_TYPE_DATA,
					     "GimpPattern",
					     &pattern_info, 0);
  }

  return pattern_type;
}

static void
gimp_pattern_class_init (GimpPatternClass *klass)
{
  GObjectClass      *object_class;
  GimpObjectClass   *gimp_object_class;
  GimpViewableClass *viewable_class;
  GimpDataClass     *data_class;

  object_class      = G_OBJECT_CLASS (klass);
  gimp_object_class = GIMP_OBJECT_CLASS (klass);
  viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  data_class        = GIMP_DATA_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize           = gimp_pattern_finalize;

  gimp_object_class->get_memsize   = gimp_pattern_get_memsize;

  viewable_class->default_stock_id = "gimp-tool-bucket-fill";
  viewable_class->get_popup_size   = gimp_pattern_get_popup_size;
  viewable_class->get_new_preview  = gimp_pattern_get_new_preview;
  viewable_class->get_description  = gimp_pattern_get_description;

  data_class->get_extension        = gimp_pattern_get_extension;
  data_class->duplicate            = gimp_pattern_duplicate;
}

static void
gimp_pattern_init (GimpPattern *pattern)
{
  pattern->mask = NULL;
}

static void
gimp_pattern_finalize (GObject *object)
{
  GimpPattern *pattern = GIMP_PATTERN (object);

  if (pattern->mask)
    {
      temp_buf_free (pattern->mask);
      pattern->mask = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_pattern_get_memsize (GimpObject *object,
                          gint64     *gui_size)
{
  GimpPattern *pattern = GIMP_PATTERN (object);
  gint64       memsize = 0;

  if (pattern->mask)
    memsize += temp_buf_get_memsize (pattern->mask);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_pattern_get_popup_size (GimpViewable *viewable,
                             gint          width,
                             gint          height,
                             gboolean      dot_for_dot,
                             gint         *popup_width,
                             gint         *popup_height)
{
  GimpPattern *pattern = GIMP_PATTERN (viewable);

  if (pattern->mask->width > width || pattern->mask->height > height)
    {
      *popup_width  = pattern->mask->width;
      *popup_height = pattern->mask->height;

      return TRUE;
    }

  return FALSE;
}

static TempBuf *
gimp_pattern_get_new_preview (GimpViewable *viewable,
			      gint          width,
			      gint          height)
{
  GimpPattern *pattern = GIMP_PATTERN (viewable);
  TempBuf     *temp_buf;
  gint         copy_width;
  gint         copy_height;

  copy_width  = MIN (width,  pattern->mask->width);
  copy_height = MIN (height, pattern->mask->height);

  temp_buf = temp_buf_new (copy_width, copy_height,
			   pattern->mask->bytes,
			   0, 0, NULL);

  temp_buf_copy_area (pattern->mask, temp_buf,
		      0, 0, copy_width, copy_height, 0, 0);

  return temp_buf;
}

static gchar *
gimp_pattern_get_description (GimpViewable  *viewable,
                              gchar        **tooltip)
{
  GimpPattern *pattern = GIMP_PATTERN (viewable);

  if (tooltip)
    *tooltip = NULL;

  return g_strdup_printf ("%s (%d x %d)",
                          GIMP_OBJECT (pattern)->name,
                          pattern->mask->width,
                          pattern->mask->height);
}

static gchar *
gimp_pattern_get_extension (GimpData *data)
{
  return GIMP_PATTERN_FILE_EXTENSION;
}

static GimpData *
gimp_pattern_duplicate (GimpData *data,
                        gboolean  stingy_memory_use)
{
  GimpPattern *pattern;

  pattern = g_object_new (GIMP_TYPE_PATTERN, NULL);

  pattern->mask = temp_buf_copy (GIMP_PATTERN (data)->mask, NULL);

  /*  Swap the pattern to disk (if we're being stingy with memory) */
  if (stingy_memory_use)
    temp_buf_swap (pattern->mask);

  return GIMP_DATA (pattern);
}

GimpData *
gimp_pattern_new (const gchar *name,
                  gboolean     stingy_memory_use)
{
  GimpPattern *pattern;
  guchar      *data;
  gint         row, col;

  g_return_val_if_fail (name != NULL, NULL);

  pattern = g_object_new (GIMP_TYPE_PATTERN,
                          "name", name,
                          NULL);

  pattern->mask = temp_buf_new (32, 32, 3, 0, 0, NULL);

  data = temp_buf_data (pattern->mask);

  for (row = 0; row < pattern->mask->height; row++)
    for (col = 0; col < pattern->mask->width; col++)
      {
	memset (data, (col % 2) && (row % 2) ? 255 : 0, 3);
	data += 3;
      }

  /*  Swap the pattern to disk (if we're being stingy with memory) */
  if (stingy_memory_use)
    temp_buf_swap (pattern->mask);

  return GIMP_DATA (pattern);
}

GimpData *
gimp_pattern_get_standard (void)
{
  static GimpData *standard_pattern = NULL;

  if (! standard_pattern)
    {
      standard_pattern = gimp_pattern_new ("Standard", FALSE);

      standard_pattern->dirty = FALSE;
      gimp_data_make_internal (standard_pattern);

      /*  set ref_count to 2 --> never swap the standard pattern  */
      g_object_ref (standard_pattern);
    }

  return standard_pattern;
}

GList *
gimp_pattern_load (const gchar  *filename,
                   gboolean      stingy_memory_use,
                   GError      **error)
{
  GimpPattern   *pattern = NULL;
  gint           fd;
  PatternHeader  header;
  gint           bn_size;
  gchar         *name    = NULL;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (filename), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  fd = open (filename, O_RDONLY | _O_BINARY);
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
                     "Could not read %d bytes: %s"),
                   gimp_filename_to_utf8 (filename),
		   (gint) sizeof (header), g_strerror (errno));
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
                         "Could not read %d bytes: %s"),
                       gimp_filename_to_utf8 (filename), bn_size,
		       g_strerror (errno));
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
                          "name", name,
                          NULL);

  g_free (name);

  pattern->mask = temp_buf_new (header.width, header.height, header.bytes,
                                0, 0, NULL);
  if (read (fd, temp_buf_data (pattern->mask),
            header.width * header.height * header.bytes) <
      header.width * header.height * header.bytes)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in pattern file '%s': "
                     "Could not read %d bytes: %s"),
                   gimp_filename_to_utf8 (filename),
		   header.width * header.height * header.bytes,
                   g_strerror (errno));
      goto error;
    }

  close (fd);

  /*  Swap the pattern to disk (if we're being stingy with memory) */
  if (stingy_memory_use)
    temp_buf_swap (pattern->mask);

  return g_list_prepend (NULL, pattern);

 error:
  if (pattern)
    g_object_unref (pattern);

  close (fd);

  return NULL;
}

GList *
gimp_pattern_load_pixbuf (const gchar  *filename,
                          gboolean      stingy_memory_use,
                          GError      **error)
{
  GimpPattern *pattern;
  GdkPixbuf   *pixbuf;
  guchar      *pat_data;
  guchar      *buf_data;
  gchar       *name;
  gint         width;
  gint         height;
  gint         bytes;
  gint         rowstride;
  gint         i;

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
    {
      gchar *basename = g_path_get_basename (filename);

      name = g_filename_to_utf8 (basename, -1, NULL, NULL, NULL);

      g_free (basename);
    }

  pattern = g_object_new (GIMP_TYPE_PATTERN,
                          "name", name,
                          NULL);
  g_free (name);

  width     = gdk_pixbuf_get_width (pixbuf);
  height    = gdk_pixbuf_get_height (pixbuf);
  bytes     = gdk_pixbuf_get_n_channels (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  pattern->mask = temp_buf_new (width, height, bytes, 0, 0, NULL);

  pat_data = gdk_pixbuf_get_pixels (pixbuf);
  buf_data = temp_buf_data (pattern->mask);

  for (i = 0; i < height; i++)
    {
      memcpy (buf_data + i * width * bytes, pat_data, width * bytes);
      pat_data += rowstride;
    }

  g_object_unref (pixbuf);

  /*  Swap the pattern to disk (if we're being stingy with memory) */
  if (stingy_memory_use)
    temp_buf_swap (pattern->mask);

  return g_list_prepend (NULL, pattern);
}

TempBuf *
gimp_pattern_get_mask (const GimpPattern *pattern)
{
  g_return_val_if_fail (GIMP_IS_PATTERN (pattern), NULL);

  return pattern->mask;
}
