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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpbrush.h"
#include "gimpbrush-header.h"
#include "gimpbrush-load.h"
#include "gimpbrush-private.h"
#include "gimppattern-header.h"
#include "gimptempbuf.h"

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

static GList     * gimp_brush_load_abr_v12       (GDataInputStream  *input,
                                                  AbrHeader         *abr_hdr,
                                                  GFile             *file,
                                                  GError           **error);
static GList     * gimp_brush_load_abr_v6        (GDataInputStream  *input,
                                                  AbrHeader         *abr_hdr,
                                                  GFile             *file,
                                                  GError           **error);
static GimpBrush * gimp_brush_load_abr_brush_v12 (GDataInputStream  *input,
                                                  AbrHeader         *abr_hdr,
                                                  gint               index,
                                                  GFile             *file,
                                                  GError           **error);
static GimpBrush * gimp_brush_load_abr_brush_v6  (GDataInputStream  *input,
                                                  AbrHeader         *abr_hdr,
                                                  gint32             max_offset,
                                                  gint               index,
                                                  GFile             *file,
                                                  GError           **error);

static gchar       abr_read_char                 (GDataInputStream  *input,
                                                  GError           **error);
static gint16      abr_read_short                (GDataInputStream  *input,
                                                  GError           **error);
static gint32      abr_read_long                 (GDataInputStream  *input,
                                                  GError           **error);
static gchar     * abr_read_ucs2_text            (GDataInputStream  *input,
                                                  GError           **error);
static gboolean    abr_supported                 (AbrHeader         *abr_hdr,
                                                  GError           **error);
static gboolean    abr_reach_8bim_section        (GDataInputStream  *input,
                                                  const gchar       *name,
                                                  GError           **error);
static gboolean    abr_rle_decode                (GDataInputStream  *input,
                                                  gchar             *buffer,
                                                  gsize              buffer_size,
                                                  gint32             height,
                                                  GError           **error);


/*  public functions  */

GList *
gimp_brush_load (GimpContext   *context,
                 GFile         *file,
                 GInputStream  *input,
                 GError       **error)
{
  GimpBrush *brush;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  brush = gimp_brush_load_brush (context, file, input, error);
  if (! brush)
    return NULL;

  return g_list_prepend (NULL, brush);
}

GimpBrush *
gimp_brush_load_brush (GimpContext   *context,
                       GFile         *file,
                       GInputStream  *input,
                       GError       **error)
{
  GimpBrush       *brush;
  gsize            bn_size;
  GimpBrushHeader  header;
  gchar           *name = NULL;
  guchar          *mask;
  gsize            bytes_read;
  gssize           i, size;
  gboolean         success = TRUE;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  /*  read the header  */
  if (! g_input_stream_read_all (input, &header, sizeof (header),
                                 &bytes_read, NULL, error) ||
      bytes_read != sizeof (header))
    {
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
                   _("Fatal parse error in brush file: Width = 0."));
      return NULL;
    }

  if (header.height == 0)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file: Height = 0."));
      return NULL;
    }

  if (header.bytes == 0)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file: Bytes = 0."));
      return NULL;
    }

  if (header.width  > GIMP_BRUSH_MAX_SIZE ||
      header.height > GIMP_BRUSH_MAX_SIZE ||
      G_MAXSIZE / header.width / header.height / MAX (4, header.bytes) < 1)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file: %dx%d over max size."),
                   header.width, header.height);
      return NULL;
    }

  switch (header.version)
    {
    case 1:
      /*  If this is a version 1 brush, set the fp back 8 bytes  */
      if (! g_seekable_seek (G_SEEKABLE (input), -8, G_SEEK_CUR,
                             NULL, error))
        return NULL;

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
                       _("Fatal parse error in brush file: Unknown depth %d."),
                       header.bytes);
          return NULL;
        }
      /*  fallthrough  */

    case 2:
      if (header.magic_number == GIMP_BRUSH_MAGIC)
        break;

    default:
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file: Unknown version %d."),
                   header.version);
      return NULL;
    }

  if (header.header_size < sizeof (GimpBrushHeader))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Unsupported brush format"));
      return NULL;
    }

  /*  Read in the brush name  */
  if ((bn_size = (header.header_size - sizeof (header))))
    {
      gchar *utf8;

      if (bn_size > GIMP_BRUSH_MAX_NAME)
        {
          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Invalid header data in '%s': "
                         "Brush name is too long: %lu"),
                       gimp_file_get_utf8_name (file),
                       (gulong) bn_size);
          return NULL;
        }

      name = g_new0 (gchar, bn_size + 1);

      if (! g_input_stream_read_all (input, name, bn_size,
                                     &bytes_read, NULL, error) ||
          bytes_read != bn_size)
        {
          g_free (name);
          return NULL;
        }

      utf8 = gimp_any_to_utf8 (name, bn_size - 1,
                               _("Invalid UTF-8 string in brush file '%s'."),
                               gimp_file_get_utf8_name (file));
      g_free (name);
      name = utf8;
    }

  if (! name)
    name = g_strdup (_("Unnamed"));

  brush = g_object_new (GIMP_TYPE_BRUSH,
                        "name",      name,
                        "mime-type", "image/x-gimp-gbr",
                        NULL);
  g_free (name);

  brush->priv->mask = gimp_temp_buf_new (header.width, header.height,
                                         babl_format ("Y u8"));

  mask = gimp_temp_buf_get_data (brush->priv->mask);
  size = header.width * header.height * header.bytes;

  switch (header.bytes)
    {
    case 1:
      success = (g_input_stream_read_all (input, mask, size,
                                          &bytes_read, NULL, error) &&
                 bytes_read == size);

      /* For backwards-compatibility, check if a pattern follows.
       * The obsolete .gpb format did it this way.
       */
      if (success)
        {
          GimpPatternHeader ph;
          goffset           rewind;

          rewind = g_seekable_tell (G_SEEKABLE (input));

          if (g_input_stream_read_all (input, &ph, sizeof (GimpPatternHeader),
                                       &bytes_read, NULL, NULL) &&
              bytes_read == sizeof (GimpPatternHeader))
            {
              /*  rearrange the bytes in each unsigned int  */
              ph.header_size  = g_ntohl (ph.header_size);
              ph.version      = g_ntohl (ph.version);
              ph.width        = g_ntohl (ph.width);
              ph.height       = g_ntohl (ph.height);
              ph.bytes        = g_ntohl (ph.bytes);
              ph.magic_number = g_ntohl (ph.magic_number);

              if (ph.magic_number == GIMP_PATTERN_MAGIC        &&
                  ph.version      == 1                         &&
                  ph.header_size  > sizeof (GimpPatternHeader) &&
                  ph.bytes        == 3                         &&
                  ph.width        == header.width              &&
                  ph.height       == header.height             &&
                  g_input_stream_skip (input,
                                       ph.header_size -
                                       sizeof (GimpPatternHeader),
                                       NULL, NULL) ==
                  ph.header_size - sizeof (GimpPatternHeader))
                {
                  guchar *pixmap;
                  gssize  pixmap_size;

                  brush->priv->pixmap =
                    gimp_temp_buf_new (header.width, header.height,
                                       babl_format ("R'G'B' u8"));

                  pixmap = gimp_temp_buf_get_data (brush->priv->pixmap);

                  pixmap_size = gimp_temp_buf_get_data_size (brush->priv->pixmap);

                  success = (g_input_stream_read_all (input, pixmap,
                                                      pixmap_size,
                                                      &bytes_read, NULL,
                                                      error) &&
                             bytes_read == pixmap_size);
                }
              else
                {
                  /*  seek back if pattern wasn't found  */
                  success = g_seekable_seek (G_SEEKABLE (input),
                                             rewind, G_SEEK_SET,
                                             NULL, error);
                }
            }
        }
      break;

    case 2:  /*  cinepaint brush, 16 bit floats  */
      {
        guchar buf[8 * 1024];

        for (i = 0; success && i < size;)
          {
            gssize bytes = MIN (size - i, sizeof (buf));

            success = (g_input_stream_read_all (input, buf, bytes,
                                                &bytes_read, NULL, error) &&
                       bytes_read == bytes);

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
        guchar *pixmap;
        guchar  buf[8 * 1024];

        brush->priv->pixmap = gimp_temp_buf_new (header.width, header.height,
                                                 babl_format ("R'G'B' u8"));
        pixmap = gimp_temp_buf_get_data (brush->priv->pixmap);

        for (i = 0; success && i < size;)
          {
            gssize bytes = MIN (size - i, sizeof (buf));

            success = (g_input_stream_read_all (input, buf, bytes,
                                                &bytes_read, NULL, error) &&
                       bytes_read == bytes);

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
                   _("Fatal parse error in brush file:\n"
                     "Unsupported brush depth %d\n"
                     "GIMP brushes must be GRAY or RGBA."),
                   header.bytes);
      return NULL;
    }

  if (! success)
    {
      g_object_unref (brush);
      return NULL;
    }

  brush->priv->spacing  = header.spacing;
  brush->priv->x_axis.x = header.width  / 2.0;
  brush->priv->x_axis.y = 0.0;
  brush->priv->y_axis.x = 0.0;
  brush->priv->y_axis.y = header.height / 2.0;

  return brush;
}

GList *
gimp_brush_load_abr (GimpContext   *context,
                     GFile         *file,
                     GInputStream  *input,
                     GError       **error)
{
  GDataInputStream *data_input;
  AbrHeader         abr_hdr;
  GList            *brush_list = NULL;
  GError           *my_error   = NULL;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  data_input = g_data_input_stream_new (input);

  g_data_input_stream_set_byte_order (data_input,
                                      G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN);

  abr_hdr.version = abr_read_short (data_input, &my_error);
  if (my_error)
    goto done;

  /* sub-version for ABR v6 */
  abr_hdr.count = abr_read_short (data_input, &my_error);
  if (my_error)
    goto done;

  if (abr_supported (&abr_hdr, &my_error))
    {
      switch (abr_hdr.version)
        {
        case 1:
        case 2:
          brush_list = gimp_brush_load_abr_v12 (data_input, &abr_hdr,
                                                file, &my_error);
          break;

        case 10:
        case 6:
          brush_list = gimp_brush_load_abr_v6 (data_input, &abr_hdr,
                                               file, &my_error);
          break;
        }
    }

 done:

  g_object_unref (data_input);

  if (! brush_list)
    {
      if (! my_error)
        g_set_error (&my_error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                     _("Unable to decode abr format version %d."),
                     abr_hdr.version);
    }

  if (my_error)
    g_propagate_error (error, my_error);

  return g_list_reverse (brush_list);
}


/*  private functions  */

static GList *
gimp_brush_load_abr_v12 (GDataInputStream  *input,
                         AbrHeader         *abr_hdr,
                         GFile             *file,
                         GError           **error)
{
  GList *brush_list = NULL;
  gint   i;

  for (i = 0; i < abr_hdr->count; i++)
    {
      GimpBrush *brush;
      GError    *my_error = NULL;

      brush = gimp_brush_load_abr_brush_v12 (input, abr_hdr, i,
                                             file, &my_error);

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
gimp_brush_load_abr_v6 (GDataInputStream  *input,
                        AbrHeader         *abr_hdr,
                        GFile             *file,
                        GError           **error)
{
  GList   *brush_list = NULL;
  gint32   sample_section_size;
  goffset  sample_section_end;
  gint     i = 1;

  if (! abr_reach_8bim_section (input, "samp", error))
    return brush_list;

  sample_section_size = abr_read_long (input, error);
  if (error && *error)
    return brush_list;

  sample_section_end = (sample_section_size +
                        g_seekable_tell (G_SEEKABLE (input)));

  while (g_seekable_tell (G_SEEKABLE (input)) < sample_section_end)
    {
      GimpBrush *brush;
      GError    *my_error = NULL;

      brush = gimp_brush_load_abr_brush_v6 (input, abr_hdr, sample_section_end,
                                            i, file, &my_error);

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
gimp_brush_load_abr_brush_v12 (GDataInputStream  *input,
                               AbrHeader         *abr_hdr,
                               gint               index,
                               GFile             *file,
                               GError           **error)
{
  GimpBrush      *brush = NULL;
  AbrBrushHeader  abr_brush_hdr;

  abr_brush_hdr.type = abr_read_short (input, error);
  if (error && *error)
    return NULL;

  abr_brush_hdr.size = abr_read_long (input, error);
  if (error && *error)
    return NULL;

  if (abr_brush_hdr.size < 0)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file: "
                     "Brush size value corrupt."));
      return NULL;
    }

  /*  g_print(" + BRUSH\n | << type: %i  block size: %i bytes\n",
   *          abr_brush_hdr.type, abr_brush_hdr.size);
   */

  switch (abr_brush_hdr.type)
    {
    case 1: /* computed brush */
      /* FIXME: support it!
       *
       * We can probably feed the info into the generated brush code
       * and get a usable brush back. It seems to support the same
       * types -akl
       */
      g_printerr ("WARNING: computed brush unsupported, skipping.\n");
      g_seekable_seek (G_SEEKABLE (input), abr_brush_hdr.size,
                       G_SEEK_CUR, NULL, NULL);
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

        abr_sampled_brush_hdr.misc = abr_read_long (input, error);
        if (error && *error)
          break;

        abr_sampled_brush_hdr.spacing = abr_read_short (input, error);
        if (error && *error)
          break;

        if (abr_hdr->version == 2)
          {
            sample_name = abr_read_ucs2_text (input, error);
            if (error && *error)
              break;
          }

        abr_sampled_brush_hdr.antialiasing = abr_read_char (input, error);
        if (error && *error)
          break;

        for (i = 0; i < 4; i++)
          {
            abr_sampled_brush_hdr.bounds[i] = abr_read_short (input, error);
            if (error && *error)
              break;
          }

        for (i = 0; i < 4; i++)
          {
            abr_sampled_brush_hdr.bounds_long[i] = abr_read_long (input, error);
            if (error && *error)
              break;
          }

        abr_sampled_brush_hdr.depth = abr_read_short (input, error);
        if (error && *error)
          break;

        height = (abr_sampled_brush_hdr.bounds_long[2] -
                  abr_sampled_brush_hdr.bounds_long[0]); /* bottom - top */
        width  = (abr_sampled_brush_hdr.bounds_long[3] -
                  abr_sampled_brush_hdr.bounds_long[1]); /* right - left */
        bytes  = abr_sampled_brush_hdr.depth >> 3;

        /* g_print ("width %i  height %i  bytes %i\n", width, height, bytes); */

        if (width  < 1 || width  > 10000 ||
            height < 1 || height > 10000 ||
            bytes  < 1 || bytes  > 1     ||
            G_MAXSIZE / width / height / bytes < 1)
          {
            g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                         _("Fatal parse error in brush file: "
                           "Brush dimensions out of range."));
            break;
          }

        abr_sampled_brush_hdr.wide = height > 16384;

        if (abr_sampled_brush_hdr.wide)
          {
            /* FIXME: support wide brushes */

            g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                         _("Fatal parse error in brush file: "
                           "Wide brushes are not supported."));
            break;
          }

        tmp = g_path_get_basename (gimp_file_get_utf8_name (file));
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

        brush->priv->spacing  = abr_sampled_brush_hdr.spacing;
        brush->priv->x_axis.x = width / 2.0;
        brush->priv->x_axis.y = 0.0;
        brush->priv->y_axis.x = 0.0;
        brush->priv->y_axis.y = height / 2.0;
        brush->priv->mask     = gimp_temp_buf_new (width, height,
                                                   babl_format ("Y u8"));

        mask = gimp_temp_buf_get_data (brush->priv->mask);
        size = width * height * bytes;

        compress = abr_read_char (input, error);
        if (error && *error)
          {
            g_object_unref (brush);
            brush = NULL;
            break;
          }

        /* g_print(" | << size: %dx%d %d bit (%d bytes) %s\n",
         *         width, height, abr_sampled_brush_hdr.depth, size,
         *         comppres ? "compressed" : "raw");
         */

        if (! compress)
          {
            gsize bytes_read;

            if (! g_input_stream_read_all (G_INPUT_STREAM (input),
                                           mask, size,
                                           &bytes_read, NULL, error) ||
                bytes_read != size)
              {
                g_object_unref (brush);
                brush = NULL;
                break;
              }
          }
        else
          {
            if (! abr_rle_decode (input, (gchar *) mask, size, height, error))
              {
                g_object_unref (brush);
                brush = NULL;
                break;
              }
          }
      }
      break;

    default:
      g_printerr ("WARNING: unknown brush type, skipping.\n");
      g_seekable_seek (G_SEEKABLE (input), abr_brush_hdr.size,
                       G_SEEK_CUR, NULL, NULL);
      break;
    }

  return brush;
}

static GimpBrush *
gimp_brush_load_abr_brush_v6 (GDataInputStream  *input,
                              AbrHeader         *abr_hdr,
                              gint32             max_offset,
                              gint               index,
                              GFile             *file,
                              GError           **error)
{
  GimpBrush *brush = NULL;
  guchar    *mask;

  gint32     brush_size;
  gint32     brush_end;
  goffset    next_brush;

  gint32     top, left, bottom, right;
  gint16     depth;
  gchar      compress;

  gint32     width, height;
  gint32     size;

  gchar     *tmp;
  gchar     *name;
  gboolean   r;

  brush_size = abr_read_long (input, error);
  if (error && *error)
    return NULL;

  if (brush_size < 0)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file: "
                     "Brush size value corrupt."));
      return NULL;
    }

  brush_end = brush_size;

  /* complement to 4 */
  while (brush_end % 4 != 0)
    brush_end++;

  next_brush = (brush_end + g_seekable_tell (G_SEEKABLE (input)));

  if (abr_hdr->count == 1)
    {
      /* discard key and short coordinates and unknown short */
      r = g_seekable_seek (G_SEEKABLE (input), 47, G_SEEK_CUR,
                           NULL, error);
    }
  else
    {
      /* discard key and unknown bytes */
      r = g_seekable_seek (G_SEEKABLE (input), 301, G_SEEK_CUR,
                           NULL, error);
    }

  if (! r)
    {
      g_prefix_error (error,
                      _("Fatal parse error in brush file: "
                        "File appears truncated: "));
      return NULL;
    }

  top      = abr_read_long  (input, error); if (error && *error) return NULL;
  left     = abr_read_long  (input, error); if (error && *error) return NULL;
  bottom   = abr_read_long  (input, error); if (error && *error) return NULL;
  right    = abr_read_long  (input, error); if (error && *error) return NULL;
  depth    = abr_read_short (input, error); if (error && *error) return NULL;
  compress = abr_read_char  (input, error); if (error && *error) return NULL;

  depth = depth >> 3;

  width  = right - left;
  height = bottom - top;
  size   = width * depth * height;

#if 0
  g_printerr ("width %i  height %i  depth %i  compress %i\n",
              width, height, depth, compress);
#endif

  if (width  < 1 || width  > 10000 ||
      height < 1 || height > 10000 ||
      depth  < 1 || depth  > 1     ||
      G_MAXSIZE / width / height / depth < 1)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file: "
                     "Brush dimensions out of range."));
      return NULL;
    }

  if (compress < 0 || compress > 1)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file: "
                     "Unknown compression method."));
      return NULL;
    }

  tmp = g_path_get_basename (gimp_file_get_utf8_name (file));
  name = g_strdup_printf ("%s-%03d", tmp, index);
  g_free (tmp);

  brush = g_object_new (GIMP_TYPE_BRUSH,
                        "name",      name,
                        /*  FIXME: MIME type!!  */
                        "mime-type", "application/x-photoshop-abr",
                        NULL);

  g_free (name);

  brush->priv->spacing  = 25; /* real value needs 8BIMdesc section parser */
  brush->priv->x_axis.x = width / 2.0;
  brush->priv->x_axis.y = 0.0;
  brush->priv->y_axis.x = 0.0;
  brush->priv->y_axis.y = height / 2.0;
  brush->priv->mask     = gimp_temp_buf_new (width, height,
                                             babl_format ("Y u8"));

  mask = gimp_temp_buf_get_data (brush->priv->mask);

  /* data decoding */
  if (! compress)
    {
      /* not compressed - read raw bytes as brush data */
      gsize bytes_read;

      if (! g_input_stream_read_all (G_INPUT_STREAM (input),
                                     mask, size,
                                     &bytes_read, NULL, error) ||
          bytes_read != size)
        {
          g_object_unref (brush);
          return NULL;
        }
    }
  else
    {
      if (! abr_rle_decode (input, (gchar *) mask, size, height, error))
        {
          g_object_unref (brush);
          return NULL;
        }
    }

  if (g_seekable_tell (G_SEEKABLE (input)) <= next_brush)
    g_seekable_seek (G_SEEKABLE (input), next_brush, G_SEEK_SET,
                     NULL, NULL);

  return brush;
}

static gchar
abr_read_char (GDataInputStream  *input,
               GError           **error)
{
  return g_data_input_stream_read_byte (input, NULL, error);
}

static gint16
abr_read_short (GDataInputStream  *input,
                GError           **error)
{
  return g_data_input_stream_read_int16 (input, NULL, error);
}

static gint32
abr_read_long (GDataInputStream  *input,
               GError           **error)
{
  return g_data_input_stream_read_int32 (input, NULL, error);
}

static gchar *
abr_read_ucs2_text (GDataInputStream  *input,
                    GError           **error)
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

  len = 2 * abr_read_long (input, error);
  if (len <= 0)
    return NULL;

  name_ucs2 = g_new (gchar, len);

  for (i = 0; i < len; i++)
    {
      name_ucs2[i] = abr_read_char (input, error);
      if (error && *error)
        {
          g_free (name_ucs2);
          return NULL;
        }
    }

  name_utf8 = g_convert (name_ucs2, len,
                         "UTF-8", "UCS-2BE",
                         NULL, NULL, NULL);

  g_free (name_ucs2);

  return name_utf8;
}

static gboolean
abr_supported (AbrHeader  *abr_hdr,
               GError    **error)
{
  switch (abr_hdr->version)
    {
    case 1:
    case 2:
      return TRUE;
      break;

    case 10:
    case 6:
      /* in this case, count contains format sub-version */
      if (abr_hdr->count == 1 || abr_hdr->count == 2)
        return TRUE;

      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file: "
                     "Unable to decode abr format version %d."),

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
abr_reach_8bim_section (GDataInputStream  *input,
                        const gchar       *name,
                        GError           **error)
{
  while (TRUE)
    {
      gchar   tag[4];
      gchar   tagname[5];
      guint32 section_size;
      gsize   bytes_read;

      if (! g_input_stream_read_all (G_INPUT_STREAM (input),
                                     tag, 4,
                                     &bytes_read, NULL, error) ||
          bytes_read != 4)
        return FALSE;

      if (strncmp (tag, "8BIM", 4))
        return FALSE;

      if (! g_input_stream_read_all (G_INPUT_STREAM (input),
                                     tagname, 4,
                                     &bytes_read, NULL, error) ||
          bytes_read != 4)
        return FALSE;

      tagname[4] = '\0';

      if (! strncmp (tagname, name, 4))
        return TRUE;

      section_size = abr_read_long (input, error);
      if (error && *error)
        return FALSE;

      if (! g_seekable_seek (G_SEEKABLE (input), section_size, G_SEEK_CUR,
                             NULL, error))
        return FALSE;
    }

  return FALSE;
}

static gboolean
abr_rle_decode (GDataInputStream  *input,
                gchar             *buffer,
                gsize              buffer_size,
                gint32             height,
                GError           **error)
{
  gint    i, j;
  gshort *cscanline_len = NULL;
  gchar  *cdata         = NULL;
  gchar  *data          = buffer;

  /* read compressed size foreach scanline */
  cscanline_len = gegl_scratch_new (gshort, height);
  for (i = 0; i < height; i++)
    {
      cscanline_len[i] = abr_read_short (input, error);
      if ((error && *error) || cscanline_len[i] <= 0)
        goto err;
    }

  /* unpack each scanline data */
  for (i = 0; i < height; i++)
    {
      gint  len;
      gsize bytes_read;

      len = cscanline_len[i];

      cdata = gegl_scratch_alloc (len);

      if (! g_input_stream_read_all (G_INPUT_STREAM (input),
                                     cdata, len,
                                     &bytes_read, NULL, error) ||
          bytes_read != len)
        {
          goto err;
        }

      for (j = 0; j < len;)
        {
          gint32 n = cdata[j++];

          if (n >= 128)     /* force sign */
            n -= 256;

          if (n < 0)
            {
              /* copy the following char -n + 1 times */

              if (n == -128)  /* it's a nop */
                continue;

              n = -n + 1;

              if (j + 1 > len || (data - buffer) + n > buffer_size)
                goto err;

              memset (data, cdata[j], n);

              j    += 1;
              data += n;
            }
          else
            {
              /* read the following n + 1 chars (no compr) */

              n = n + 1;

              if (j + n > len || (data - buffer) + n > buffer_size)
                goto err;

              memcpy (data, &cdata[j], n);

              j    += n;
              data += n;
            }
        }

      g_clear_pointer (&cdata, gegl_scratch_free);
    }

  g_clear_pointer (&cscanline_len, gegl_scratch_free);

  return TRUE;

err:
  g_clear_pointer (&cdata, gegl_scratch_free);
  g_clear_pointer (&cscanline_len, gegl_scratch_free);
  if (error && ! *error)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file: "
                     "RLE compressed brush data corrupt."));
    }
  return FALSE;
}
