/* GIMP - The GNU Image Manipulation Program
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
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include <glib-object.h>
#include <glib/gstdio.h>

#include "libgimpbase/gimpbase.h"

#ifdef G_OS_WIN32
#include "libgimpbase/gimpwin32-io.h"
#endif

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimpbrush.h"
#include "gimpbrush-header.h"
#include "gimpbrush-load.h"

#include "gimp-intl.h"


/* stuff from abr2gbr Copyright (C) 2001 Marco Lamberto <lm@sunnyspot.org>  */
/* the above is GPL  see http://the.sunnyspot.org/gimp/  */

typedef struct _AbrHeader               AbrHeader;
typedef struct _AbrBrushHeader          AbrBrushHeader;
typedef struct _AbrSampledBrushHeader   AbrSampledBrushHeader;

struct _AbrHeader
{
  gint16 version;
  gint16 count;
};

struct _AbrBrushHeader
{
  gint16 type;
  gint32 size;
};

struct _AbrSampledBrushHeader
{
  gint32   misc;
  gint16   spacing;
  gchar    antialiasing;
  gint16   bounds[4];
  gint32   bounds_long[4];
  gint16   depth;
  gboolean wide;
};


/*  local function prototypes  */

static GimpBrush * gimp_brush_load_abr_brush (FILE         *file,
                                              AbrHeader    *abr_hdr,
                                              gint          index,
                                              const gchar  *filename,
                                              GError      **error);
static gchar       abr_read_char             (FILE         *file);
static gint16      abr_read_short            (FILE         *file);
static gint32      abr_read_long             (FILE         *file);
static gchar     * abr_v2_read_brush_name    (FILE         *file);


/*  public functions  */

GList *
gimp_brush_load (const gchar  *filename,
                 GError      **error)
{
  GimpBrush *brush;
  gint       fd;

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

  brush = gimp_brush_load_brush (fd, filename, error);

  close (fd);

  if (! brush)
    return NULL;

  return g_list_prepend (NULL, brush);
}

GimpBrush *
gimp_brush_load_brush (gint          fd,
                       const gchar  *filename,
                       GError      **error)
{
  GimpBrush   *brush;
  gint         bn_size;
  BrushHeader  header;
  gchar       *name = NULL;
  guchar      *pixmap;
  guchar      *mask;
  gssize       i, size;
  gboolean     success = TRUE;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (fd != -1, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  /*  Read in the header size  */
  if (read (fd, &header, sizeof (header)) != sizeof (header))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Could not read %d bytes from '%s': %s"),
                   (gint) sizeof (header),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  /*  rearrange the bytes in each unsigned int  */
  header.header_size  = g_ntohl (header.header_size);
  header.version      = g_ntohl (header.version);
  header.width        = g_ntohl (header.width);
  header.height       = g_ntohl (header.height);
  header.bytes        = g_ntohl (header.bytes);
  header.magic_number = g_ntohl (header.magic_number);
  header.spacing      = g_ntohl (header.spacing);

  /*  Check for correct file format */

  if (header.width == 0)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "Width = 0."),
                   gimp_filename_to_utf8 (filename));
      return NULL;
    }

  if (header.height == 0)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "Height = 0."),
                   gimp_filename_to_utf8 (filename));
      return NULL;
    }

  if (header.bytes == 0)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "Bytes = 0."),
                   gimp_filename_to_utf8 (filename));
      return NULL;
    }

  switch (header.version)
    {
    case 1:
      /*  If this is a version 1 brush, set the fp back 8 bytes  */
      lseek (fd, -8, SEEK_CUR);
      header.header_size += 8;
      /*  spacing is not defined in version 1  */
      header.spacing = 25;
      break;

    case 3:  /*  cinepaint brush  */
      if (header.bytes == 18  /* FLOAT16_GRAY_GIMAGE */)
        {
          header.bytes = 2;
        }
      else
        {
          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Fatal parse error in brush file '%s': "
                         "Unknown depth %d."),
                       gimp_filename_to_utf8 (filename), header.bytes);
          return NULL;
        }
      /*  fallthrough  */

    case 2:
      if (header.magic_number == GBRUSH_MAGIC)
        break;

    default:
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "Unknown version %d."),
                   gimp_filename_to_utf8 (filename), header.version);
      return NULL;
    }

  /*  Read in the brush name  */
  if ((bn_size = (header.header_size - sizeof (header))))
    {
      gchar *utf8;

      name = g_new (gchar, bn_size);

      if ((read (fd, name, bn_size)) < bn_size)
        {
          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Fatal parse error in brush file '%s': "
                         "File appears truncated."),
                       gimp_filename_to_utf8 (filename));
          g_free (name);
          return NULL;
        }

      utf8 = gimp_any_to_utf8 (name, -1,
                               _("Invalid UTF-8 string in brush file '%s'."),
                               gimp_filename_to_utf8 (filename));
      g_free (name);
      name = utf8;
    }

  if (!name)
    name = g_strdup (_("Unnamed"));

  brush = g_object_new (GIMP_TYPE_BRUSH,
                        "name",      name,
                        "mime-type", "image/x-gimp-gbr",
                        NULL);
  g_free (name);

  brush->mask = temp_buf_new (header.width, header.height, 1, 0, 0, NULL);

  mask = temp_buf_data (brush->mask);
  size = header.width * header.height * header.bytes;

  switch (header.bytes)
    {
    case 1:
      success = (read (fd, mask, size) == size);
      break;

    case 2:  /*  cinepaint brush, 16 bit floats  */
      {
        guchar buf[8 * 1024];

        for (i = 0; success && i < size;)
          {
            gssize  bytes = MIN (size - i, sizeof (buf));

            success = (read (fd, buf, bytes) == bytes);

            if (success)
              {
                guint16 *b = (guint16 *) buf;

                i += bytes;

                for (; bytes > 0; bytes -= 2, mask++, b++)
                  {
                    union
                    {
                      guint16 u[2];
                      gfloat  f;
                    } short_float;

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
                    short_float.u[0] = 0;
                    short_float.u[1] = GUINT16_FROM_BE (*b);
#else
                    short_float.u[0] = GUINT16_FROM_BE (*b);
                    short_float.u[1] = 0;
#endif

                    *mask = (guchar) (short_float.f * 255.0 + 0.5);
                  }
              }
          }
      }
      break;

    case 4:
      {
        guchar buf[8 * 1024];

        brush->pixmap = temp_buf_new (header.width, header.height,
                                      3, 0, 0, NULL);
        pixmap = temp_buf_data (brush->pixmap);

        for (i = 0; success && i < size;)
          {
            gssize  bytes = MIN (size - i, sizeof (buf));

            success = (read (fd, buf, bytes) == bytes);

            if (success)
              {
                guchar *b = buf;

                i += bytes;

                for (; bytes > 0; bytes -= 4, pixmap += 3, mask++, b += 4)
                  {
                    pixmap[0] = b[0];
                    pixmap[1] = b[1];
                    pixmap[2] = b[2];

                    mask[0]   = b[3];
                  }
              }
          }
      }
      break;

    default:
      g_object_unref (brush);
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "Unsupported brush depth %d\n"
                     "GIMP brushes must be GRAY or RGBA."),
                   gimp_filename_to_utf8 (filename), header.bytes);
      return NULL;
    }

  if (! success)
    {
      g_object_unref (brush);
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "File appears truncated."),
                   gimp_filename_to_utf8 (filename));
      return NULL;
    }

  brush->spacing  = header.spacing;
  brush->x_axis.x = header.width  / 2.0;
  brush->x_axis.y = 0.0;
  brush->y_axis.x = 0.0;
  brush->y_axis.y = header.height / 2.0;

  return brush;
}

GList *
gimp_brush_load_abr (const gchar  *filename,
                     GError      **error)
{
  FILE      *file;
  AbrHeader  abr_hdr;
  GList     *brush_list = NULL;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (filename), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  file = g_fopen (filename, "rb");

  if (! file)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  abr_hdr.version = abr_read_short (file);
  abr_hdr.count   = abr_read_short (file);

  /* g_print("version: %d  count: %d\n", abr_hdr.version, abr_hdr.count); */

  if (abr_hdr.version > 2)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "unable to decode abr format version %d."),
                   gimp_filename_to_utf8 (filename), abr_hdr.version);
    }
  else
    {
      gint i;

      for (i = 0; i < abr_hdr.count; i++)
        {
          GimpBrush *brush;

          brush = gimp_brush_load_abr_brush (file, &abr_hdr, i,
                                             filename, error);

          if (! brush)
            {
              g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                           _("Fatal parse error in brush file '%s'"),
                           gimp_filename_to_utf8 (filename));
              break;
            }

          brush_list = g_list_prepend (brush_list, brush);
        }
    }

  fclose (file);

  return g_list_reverse (brush_list);
}


/*  private functions  */

static GimpBrush *
gimp_brush_load_abr_brush (FILE         *file,
                           AbrHeader    *abr_hdr,
                           gint          index,
                           const gchar  *filename,
                           GError      **error)
{
  GimpBrush      *brush = NULL;
  AbrBrushHeader  abr_brush_hdr;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  abr_brush_hdr.type = abr_read_short (file);
  abr_brush_hdr.size = abr_read_long (file);

  /*  g_print(" + BRUSH\n | << type: %i  block size: %i bytes\n",
   *          abr_brush_hdr.type, abr_brush_hdr.size);
   */

  switch (abr_brush_hdr.type)
    {
    case 1: /* computed brush */
      /* FIXME: support it!
       *
       * We can probabaly feed the info into the generated brush code
       * and get a useable brush back. It seems to support the same
       * types -akl
       */
      g_printerr ("WARNING: computed brush unsupported, skipping.\n");
      fseek (file, abr_brush_hdr.size, SEEK_CUR);
      break;

    case 2: /* sampled brush */
      {
        AbrSampledBrushHeader  abr_sampled_brush_hdr;
        gint                   width, height;
        gint                   bytes;
        gint                   size;
        guchar                *mask;
        gint                   i;
        gchar                 *name = NULL;
        gchar                 *tmp;
        gshort                 compress;

        abr_sampled_brush_hdr.misc    = abr_read_long (file);
        abr_sampled_brush_hdr.spacing = abr_read_short (file);

        if (abr_hdr->version == 2)
          name = abr_v2_read_brush_name (file);

        abr_sampled_brush_hdr.antialiasing = abr_read_char (file);

        for (i = 0; i < 4; i++)
          abr_sampled_brush_hdr.bounds[i] = abr_read_short (file);

        for (i = 0; i < 4; i++)
          abr_sampled_brush_hdr.bounds_long[i] = abr_read_long (file);

        abr_sampled_brush_hdr.depth = abr_read_short (file);

        height = (abr_sampled_brush_hdr.bounds_long[2] -
                  abr_sampled_brush_hdr.bounds_long[0]); /* bottom - top */
        width  = (abr_sampled_brush_hdr.bounds_long[3] -
                  abr_sampled_brush_hdr.bounds_long[1]); /* right - left */
        bytes  = abr_sampled_brush_hdr.depth >> 3;

        /* g_print("width %i  height %i\n", width, height); */

        abr_sampled_brush_hdr.wide = height > 16384;

        if (abr_sampled_brush_hdr.wide)
          {
            /* FIXME: support wide brushes */

            g_printerr ("WARING: wide brushes not supported\n");
            return NULL;
          }

        if (! name)
          {
            tmp = g_filename_display_basename (filename);
            name = g_strdup_printf ("%s-%03d", tmp, index);
            g_free (tmp);
          }

        brush = g_object_new (GIMP_TYPE_BRUSH,
                              "name",      name,
                              /*  FIXME: MIME type!!  */
                              "mime-type", "application/x-photoshop-abr",
                              NULL);

        g_free (name);

        brush->spacing  = abr_sampled_brush_hdr.spacing;
        brush->x_axis.x = width / 2.0;
        brush->x_axis.y = 0.0;
        brush->y_axis.x = 0.0;
        brush->y_axis.y = height / 2.0;
        brush->mask     = temp_buf_new (width, height, 1, 0, 0, NULL);

        mask = temp_buf_data (brush->mask);
        size = width * height * bytes;

        compress = abr_read_char (file);

        /* g_print(" | << size: %dx%d %d bit (%d bytes) %s\n",
         *         width, height, abr_sampled_brush_hdr.depth, size,
         *         comppres ? "compressed" : "raw");
         */

        if (! compress)
          {
            fread (mask, size, 1, file);
          }
        else
          {
            gint16 *cscanline_len;
            gint32  n;
            gchar   ch;
            gint    i, j, c;

            /* read compressed size foreach scanline */
            cscanline_len = g_new0 (gshort, height);

            for (i = 0; i < height; i++)
              cscanline_len[i] = abr_read_short (file);

            /* unpack each scanline data */
            for (i = 0; i < height; i++)
              {
                for (j = 0; j < cscanline_len[i];)
                  {
                    n = abr_read_char (file);
                    j++;

                    if (n >= 128)       /* force sign */
                      n -= 256;

                    if (n < 0)
                      {
                        /* copy the following char -n + 1 times */

                        if (n == -128)    /* it's a nop */
                          continue;

                        n = -n + 1;
                        ch = abr_read_char (file);
                        j++;

                        for (c = 0; c < n; c++, mask++)
                          *mask = ch;
                      }
                    else
                      {
                        /* read the following n + 1 chars (no compr) */

                        for (c = 0; c < n + 1; c++, j++, mask++)
                          *mask = (guchar) abr_read_char (file);
                      }
                  }
              }

            g_free (cscanline_len);
          }
      }
      break;

    default:
      g_printerr ("WARNING: unknown brush type, skipping.\n");
      fseek (file, abr_brush_hdr.size, SEEK_CUR);
      break;
    }

  return brush;
}

static gchar
abr_read_char (FILE *file)
{
  return fgetc (file);
}

static gint16
abr_read_short (FILE *file)
{
  gint16 val;

  fread (&val, sizeof (val), 1, file);

  return GINT16_FROM_BE (val);
}

static gint32
abr_read_long (FILE *file)
{
  gint32 val;

  fread (&val, sizeof (val), 1, file);

  return GINT32_FROM_BE (val);
}

static gchar *
abr_v2_read_brush_name (FILE *file)
{
  gchar *name_ucs2;
  gchar *name_utf8;
  gint   len;
  gint   i;

  /* two-bytes characters encoded (UCS-2)
   *  format:
   *   long : number of characters in string
   *   data : zero terminated UCS-2 string
   */

  len = 2 * abr_read_long (file);
  if (len <= 0)
    return NULL;

  name_ucs2 = g_new (gchar, len);

  for (i = 0; i < len; i++)
    name_ucs2[i] =  abr_read_char (file);

  name_utf8 = g_convert (name_ucs2, len,
                         "UTF-8", "UCS-2BE",
                         NULL, NULL, NULL);

  g_free(name_ucs2);

  return name_utf8;
}
