/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrush-load.c
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

static GList     * gimp_brush_load_abr_v12       (FILE         *file,
                                                  AbrHeader    *abr_hdr,
                                                  const gchar  *filename,
                                                  GError      **error);
static GList     * gimp_brush_load_abr_v6        (FILE         *file,
                                                  AbrHeader    *abr_hdr,
                                                  const gchar  *filename,
                                                  GError      **error);
static GimpBrush * gimp_brush_load_abr_brush_v12 (FILE         *file,
                                                  AbrHeader    *abr_hdr,
                                                  gint          index,
                                                  const gchar  *filename,
                                                  GError      **error);
static GimpBrush * gimp_brush_load_abr_brush_v6  (FILE         *file,
                                                  AbrHeader    *abr_hdr,
                                                  gint32        max_offset,
                                                  gint          index,
                                                  const gchar  *filename,
                                                  GError      **error);

static gchar       abr_read_char                 (FILE         *file);
static gint16      abr_read_short                (FILE         *file);
static gint32      abr_read_long                 (FILE         *file);
static gchar     * abr_read_ucs2_text            (FILE         *file);
static gboolean    abr_supported                 (AbrHeader    *abr_hdr,
                                                  const gchar  *filename,
                                                  GError      **error);
static gboolean    abr_reach_8bim_section        (FILE         *abr,
                                                  const gchar  *name);
static gint32      abr_rle_decode                (FILE         *file,
                                                  gchar        *buffer,
                                                  gint32        height);


/*  public functions  */

GList *
gimp_brush_load (GimpContext  *context,
                 const gchar  *filename,
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

  brush = gimp_brush_load_brush (context, fd, filename, error);

  close (fd);

  if (! brush)
    return NULL;

  return g_list_prepend (NULL, brush);
}

GimpBrush *
gimp_brush_load_brush (GimpContext  *context,
                       gint          fd,
                       const gchar  *filename,
                       GError      **error)
{
  GimpBrush   *brush;
  gsize        bn_size;
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
                   ngettext ("Could not read %d byte from '%s': %s",
                             "Could not read %d bytes from '%s': %s",
                             (gint) sizeof (header)),
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

  if (header.header_size < sizeof (BrushHeader))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Unsupported brush format"));
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

      utf8 = gimp_any_to_utf8 (name, bn_size - 1,
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

  mask = temp_buf_get_data (brush->mask);
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

    case 3:
      /* The obsolete .gbp format had a 3-byte pattern following a
       * 1-byte brush, when embedded in a brush pipe, the current code
       * tries to load that pattern as a brush, and encounters the '3'
       * in the header.
       */
      g_object_unref (brush);
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "Unsupported brush depth %d\n"
                     "GIMP brushes must be GRAY or RGBA.\n"
                     "This might be an obsolete GIMP brush file, try "
                     "loading it as image and save it again."),
                   gimp_filename_to_utf8 (filename), header.bytes);
      return NULL;
      break;

    case 4:
      {
        guchar buf[8 * 1024];

        brush->pixmap = temp_buf_new (header.width, header.height,
                                      3, 0, 0, NULL);
        pixmap = temp_buf_get_data (brush->pixmap);

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
gimp_brush_load_abr (GimpContext  *context,
                     const gchar  *filename,
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
  abr_hdr.count   = abr_read_short (file); /* sub-version for ABR v6 */

  if (abr_supported (&abr_hdr, filename, error))
    {
      switch (abr_hdr.version)
        {
        case 1:
        case 2:
          brush_list = gimp_brush_load_abr_v12 (file, &abr_hdr,
                                                filename, error);
          break;

        case 6:
          brush_list = gimp_brush_load_abr_v6 (file, &abr_hdr,
                                               filename, error);
        }
    }

  fclose (file);

  if (! brush_list && (error && ! *error))
    g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                 _("Fatal parse error in brush file '%s': "
                   "unable to decode abr format version %d."),
                 gimp_filename_to_utf8 (filename), abr_hdr.version);

  return g_list_reverse (brush_list);
}


/*  private functions  */

static GList *
gimp_brush_load_abr_v12 (FILE         *file,
                         AbrHeader    *abr_hdr,
                         const gchar  *filename,
                         GError      **error)
{
  GList *brush_list = NULL;
  gint   i;

  for (i = 0; i < abr_hdr->count; i++)
    {
      GimpBrush *brush;
      GError    *my_error = NULL;

      brush = gimp_brush_load_abr_brush_v12 (file, abr_hdr, i,
                                             filename, &my_error);

      /*  a NULL brush without an error means an unsupported brush
       *  type was encountered, silently skip it and try the next one
       */

      if (brush)
        {
          brush_list = g_list_prepend (brush_list, brush);
        }
      else if (my_error)
        {
          g_propagate_error (error, my_error);
          break;
        }
    }

  return brush_list;
}

static GList *
gimp_brush_load_abr_v6 (FILE         *file,
                        AbrHeader    *abr_hdr,
                        const gchar  *filename,
                        GError      **error)
{
  GList  *brush_list = NULL;
  gint32  sample_section_size;
  gint32  sample_section_end;
  gint    i = 1;

  if (! abr_reach_8bim_section (file, "samp"))
    return brush_list;

  sample_section_size = abr_read_long (file);
  sample_section_end  = sample_section_size + ftell (file);

  while (ftell (file) < sample_section_end)
    {
      GimpBrush *brush;
      GError    *my_error = NULL;

      brush = gimp_brush_load_abr_brush_v6 (file, abr_hdr, sample_section_end,
                                            i, filename, &my_error);

      /*  a NULL brush without an error means an unsupported brush
       *  type was encountered, silently skip it and try the next one
       */

      if (brush)
        {
          brush_list = g_list_prepend (brush_list, brush);
        }
      else if (my_error)
        {
          g_propagate_error (error, my_error);
          break;
        }

      i++;
    }

  return brush_list;
}

static GimpBrush *
gimp_brush_load_abr_brush_v12 (FILE         *file,
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
        gchar                 *name;
        gchar                 *sample_name = NULL;
        gchar                 *tmp;
        gshort                 compress;

        abr_sampled_brush_hdr.misc    = abr_read_long (file);
        abr_sampled_brush_hdr.spacing = abr_read_short (file);

        if (abr_hdr->version == 2)
          sample_name = abr_read_ucs2_text (file);

        abr_sampled_brush_hdr.antialiasing = abr_read_char (file);

        for (i = 0; i < 4; i++)
          abr_sampled_brush_hdr.bounds[i] = abr_read_short (file);

        for (i = 0; i < 4; i++)
          abr_sampled_brush_hdr.bounds_long[i] = abr_read_long (file);

        abr_sampled_brush_hdr.depth = abr_read_short (file);

        if (feof (file))
          {
            g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                         _("Fatal parse error in brush file '%s': "
                           "File appears truncated."),
                         gimp_filename_to_utf8 (filename));
            return NULL;
          }

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

            g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                         _("Fatal parse error in brush file '%s': "
                           "Wide brushes are not supported."),
                         gimp_filename_to_utf8 (filename));
            return NULL;
          }

        tmp = g_filename_display_basename (filename);
        if (! sample_name)
          {
            /* build name from filename and index */
            name = g_strdup_printf ("%s-%03d", tmp, index);
          }
        else
          {
            /* build name from filename and sample name */
            name = g_strdup_printf ("%s-%s", tmp, sample_name);
            g_free (sample_name);
          }
        g_free (tmp);

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

        mask = temp_buf_get_data (brush->mask);
        size = width * height * bytes;

        compress = abr_read_char (file);

        /* g_print(" | << size: %dx%d %d bit (%d bytes) %s\n",
         *         width, height, abr_sampled_brush_hdr.depth, size,
         *         comppres ? "compressed" : "raw");
         */

        if (! compress)
          fread (mask, size, 1, file);
        else
          abr_rle_decode (file, (gchar *) mask, height);

        if (feof (file))
          {
            g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                         _("Fatal parse error in brush file '%s': "
                           "File appears truncated."),
                         gimp_filename_to_utf8 (filename));
            g_object_unref (brush);
            return NULL;
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

static GimpBrush *
gimp_brush_load_abr_brush_v6 (FILE         *file,
                              AbrHeader    *abr_hdr,
                              gint32        max_offset,
                              gint          index,
                              const gchar  *filename,
                              GError      **error)
{
  GimpBrush *brush = NULL;
  guchar    *mask;

  gint32     brush_size;
  gint32     brush_end;
  gint32     next_brush;

  gint32     top, left, bottom, right;
  gint16     depth;
  gchar      compress;

  gint32     width, height;
  gint32     size;

  gchar     *tmp;
  gchar     *name;
  gint       r;

  brush_size = abr_read_long (file);
  brush_end = brush_size;

  /* complement to 4 */
  while (brush_end % 4 != 0)
    brush_end++;

  next_brush = ftell (file) + brush_end;

  if (abr_hdr->count == 1)
    /* discard key and short coordinates and unknown short */
    r = fseek (file, 47, SEEK_CUR);
  else
    /* discard key and unknown bytes */
    r = fseek (file, 301, SEEK_CUR);

  if (r == -1)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "File appears truncated."),
                   gimp_filename_to_utf8 (filename));
      return NULL;
    }

  top      = abr_read_long (file);
  left     = abr_read_long (file);
  bottom   = abr_read_long (file);
  right    = abr_read_long (file);
  depth    = abr_read_short (file);
  compress = abr_read_char (file);

  if (feof (file))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "File appears truncated."),
                   gimp_filename_to_utf8 (filename));
      return NULL;
    }

  width  = right - left;
  height = bottom - top;
  size   = width * (depth >> 3) * height;

  tmp = g_filename_display_basename (filename);
  name = g_strdup_printf ("%s-%03d", tmp, index);
  g_free (tmp);

  brush = g_object_new (GIMP_TYPE_BRUSH,
                        "name",      name,
                        /*  FIXME: MIME type!!  */
                        "mime-type", "application/x-photoshop-abr",
                        NULL);

  g_free (name);

  brush->spacing  = 25; /* real value needs 8BIMdesc section parser */
  brush->x_axis.x = width / 2.0;
  brush->x_axis.y = 0.0;
  brush->y_axis.x = 0.0;
  brush->y_axis.y = height / 2.0;
  brush->mask     = temp_buf_new (width, height, 1, 0, 0, NULL);

  mask = temp_buf_get_data (brush->mask);

  /* data decoding */
  if (! compress)
    /* not compressed - read raw bytes as brush data */
    fread (mask, size, 1, file);
  else
    abr_rle_decode (file, (gchar *) mask, height);

  fseek (file, next_brush, SEEK_SET);

  if (feof (file))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "File appears truncated."),
                   gimp_filename_to_utf8 (filename));
      g_object_unref (brush);
      return NULL;
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
abr_read_ucs2_text (FILE *file)
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
    name_ucs2[i] = abr_read_char (file);

  name_utf8 = g_convert (name_ucs2, len,
                         "UTF-8", "UCS-2BE",
                         NULL, NULL, NULL);

  g_free (name_ucs2);

  return name_utf8;
}

static gboolean
abr_supported (AbrHeader    *abr_hdr,
               const gchar  *filename,
               GError      **error)
{
  switch (abr_hdr->version)
    {
    case 1:
    case 2:
      return TRUE;
      break;

    case 6:
      /* in this case, count contains format sub-version */
      if (abr_hdr->count == 1 || abr_hdr->count == 2)
        return TRUE;

      if (error && ! (*error))
        g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                     _("Fatal parse error in brush file '%s': "
                       "unable to decode abr format version %d."),
                     gimp_filename_to_utf8 (filename),

                     /* horrid subversion display, but better than
                      * having yet another translatable string for
                      * this
                      */
                     abr_hdr->version * 10 + abr_hdr->count);
      break;
    }

  return FALSE;
}

static gboolean
abr_reach_8bim_section (FILE        *abr,
                        const gchar *name)
{
  gchar  tag[4];
  gchar  tagname[5];
  gint32 section_size;
  gint   r;

  while (! feof (abr))
    {
      r = fread (&tag, 1, 4, abr);
      if (r != 4)
        return FALSE;

      if (strncmp (tag, "8BIM", 4))
        return FALSE;

      r = fread (&tagname, 1, 4, abr);
      if (r != 4)
        return FALSE;

      tagname[4] = '\0';

      if (! strncmp (tagname, name, 4))
        return TRUE;

      section_size = abr_read_long (abr);
      r = fseek (abr, section_size, SEEK_CUR);
      if (r == -1)
        return FALSE;
    }

  return FALSE;
}

static gint32
abr_rle_decode (FILE   *file,
                gchar  *buffer,
                gint32  height)
{
  gchar   ch;
  gint    i, j, c;
  gshort *cscanline_len;
  gchar  *data = buffer;

  /* read compressed size foreach scanline */
  cscanline_len = g_new0 (gshort, height);
  for (i = 0; i < height; i++)
    cscanline_len[i] = abr_read_short (file);

  /* unpack each scanline data */
  for (i = 0; i < height; i++)
    {
      for (j = 0; j < cscanline_len[i];)
        {
          gint32 n = abr_read_char (file);

          j++;

          if (n >= 128)     /* force sign */
            n -= 256;

          if (n < 0)
            {
              /* copy the following char -n + 1 times */

              if (n == -128)  /* it's a nop */
                continue;

              n = -n + 1;
              ch = abr_read_char (file);
              j++;

              for (c = 0; c < n; c++, data++)
                *data = ch;
            }
          else
            {
              /* read the following n + 1 chars (no compr) */

              for (c = 0; c < n + 1; c++, j++, data++)
                *data = abr_read_char (file);
            }
        }
    }

  g_free (cscanline_len);

  return 0;
}
