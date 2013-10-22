/* LIBGIMPBASE - The GIMP Basic Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmetadata.c
 * Copyright (C) 2013 Hartmut Kuhse <hartmutkuhse@src.gnome.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gio/gio.h>
#include <gexiv2/gexiv2.h>

#include "gimpbasetypes.h"

#include "gimpmetadata.h"
#include "gimpunit.h"

#include "libgimp/libgimp-intl.h"


#define TAG_LINE_DELIMITER "\v"
#define TAG_TAG_DELIMITER  "#"


static gint       gimp_metadata_length       (const gchar  *testline,
                                              const gchar  *delim);
static gboolean   gimp_metadata_get_rational (const gchar  *value,
                                              gint          sections,
                                              gchar      ***numerator,
                                              gchar      ***denominator);


static const gchar *tiff_tags[] =
{
  "Xmp.tiff",
  "Exif.Image.ImageWidth",
  "Exif.Image.ImageLength",
  "Exif.Image.BitsPerSample",
  "Exif.Image.Compression",
  "Exif.Image.PhotometricInterpretation",
  "Exif.Image.FillOrder",
  "Exif.Image.SamplesPerPixel",
  "Exif.Image.StripOffsets",
  "Exif.Image.RowsPerStrip",
  "Exif.Image.StripByteCounts",
  "Exif.Image.PlanarConfiguration"
};

static const gchar *jpeg_tags[] =
{
  "Exif.Image.JPEGProc",
  "Exif.Image.JPEGInterchangeFormat",
  "Exif.Image.JPEGInterchangeFormatLength",
  "Exif.Image.JPEGRestartInterval",
  "Exif.Image.JPEGLosslessPredictors",
  "Exif.Image.JPEGPointTransforms",
  "Exif.Image.JPEGQTables",
  "Exif.Image.JPEGDCTables",
  "Exif.Image.JPEGACTables"
};

static const gchar *unsupported_tags[] =
{
  "Exif.Image.SubIFDs",
  "Exif.Image.ClipPath",
  "Exif.Image.XClipPathUnits",
  "Exif.Image.YClipPathUnits",
  "Xmp.xmpMM.History",
  "Exif.Image.XPTitle",
  "Exif.Image.XPComment",
  "Exif.Image.XPAuthor",
  "Exif.Image.XPKeywords",
  "Exif.Image.XPSubject",
  "Exif.Image.DNGVersion",
  "Exif.Image.DNGBackwardVersion",
  "Exif.Iop"
};

static const guint8 wilber_jpg[] =
{
  0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46, 0x00, 0x01,
  0x01, 0x01, 0x00, 0x5a, 0x00, 0x5a, 0x00, 0x00, 0xff, 0xdb, 0x00, 0x43,
  0x00, 0x50, 0x37, 0x3c, 0x46, 0x3c, 0x32, 0x50, 0x46, 0x41, 0x46, 0x5a,
  0x55, 0x50, 0x5f, 0x78, 0xc8, 0x82, 0x78, 0x6e, 0x6e, 0x78, 0xf5, 0xaf,
  0xb9, 0x91, 0xc8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdb, 0x00, 0x43, 0x01, 0x55, 0x5a,
  0x5a, 0x78, 0x69, 0x78, 0xeb, 0x82, 0x82, 0xeb, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xc0, 0x00, 0x11, 0x08, 0x00, 0x10, 0x00, 0x10, 0x03,
  0x01, 0x22, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01, 0xff, 0xc4, 0x00,
  0x16, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x01, 0x02, 0xff, 0xc4, 0x00,
  0x1e, 0x10, 0x00, 0x01, 0x05, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x03, 0x11, 0x31,
  0x04, 0x12, 0x51, 0x61, 0x71, 0xff, 0xc4, 0x00, 0x14, 0x01, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xff, 0xc4, 0x00, 0x14, 0x11, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0xff, 0xda, 0x00, 0x0c, 0x03, 0x01, 0x00, 0x02, 0x11, 0x03, 0x11,
  0x00, 0x3f, 0x00, 0x18, 0xa0, 0x0e, 0x6d, 0xbc, 0xf5, 0xca, 0xf7, 0x78,
  0xb6, 0xfe, 0x3b, 0x23, 0xb2, 0x1d, 0x64, 0x68, 0xf0, 0x8a, 0x39, 0x4b,
  0x74, 0x9c, 0xa5, 0x5f, 0x35, 0x8a, 0xb2, 0x7e, 0xa0, 0xff, 0xd9, 0x00
};

static const guint wilber_jpg_len = G_N_ELEMENTS (wilber_jpg);


/**
 * Deserializes metadata from a string
 **/
GExiv2Metadata *
gimp_metadata_deserialize (const gchar *metadata_string)
{
  GExiv2Metadata  *metadata = NULL;
  GString         *buffer;
  gint             i;
  gint             j;
  gchar          **meta_info = NULL;
  gchar          **tag_info  = NULL;
  gint             num;
  gchar           *value;
  GError         **error = NULL;

  g_return_val_if_fail (metadata_string != NULL, NULL);

  if (gexiv2_initialize ())
    {
      metadata = gexiv2_metadata_new ();

      if (! gexiv2_metadata_open_buf (metadata, wilber_jpg, wilber_jpg_len,
                                      error))
        {
          return NULL;
        }
    }

  if (metadata)
    {
      meta_info = g_strsplit (metadata_string, TAG_LINE_DELIMITER, -1);

      if (meta_info)
        {
          for (i = 0; meta_info[i] != NULL; i++)
            {
              num = gimp_metadata_length (meta_info[i], TAG_TAG_DELIMITER);
              tag_info = g_strsplit (meta_info[i], TAG_TAG_DELIMITER, -1);
              if (num > 0)
                {
                  if (num > 1)
                    {
                      buffer = g_string_new (NULL);
                      for (j = 1; j < (num + 1); j++)
                        {
                          g_string_append_printf (buffer, "%s%s",
                                                  tag_info[j], TAG_TAG_DELIMITER); /* recreates value */
                        }
                      value = g_strndup (buffer->str, buffer->len - 1);  /* to avoid the trailing '#' */
                      g_string_free (buffer, TRUE);
                    }
                  else
                    {
                      value = tag_info[1];
                    }

                  gexiv2_metadata_set_tag_string (metadata, tag_info[0], value);
                }
            }

          g_strfreev (meta_info);
        }
    }

  if (tag_info)
    g_strfreev (tag_info);

  return metadata;
}

/**
 * Serializing metadata as a string
 */
gchar *
gimp_metadata_serialize (GExiv2Metadata *metadata)
{
  GString  *string;
  gchar   **exif_data = NULL;
  gchar   **iptc_data = NULL;
  gchar   **xmp_data  = NULL;
  gchar    *value;
  gint      i;
  gint      n;

  g_return_val_if_fail (GEXIV2_IS_METADATA (metadata), NULL);

  n = 0;
  string = g_string_new (NULL);

  exif_data = gexiv2_metadata_get_exif_tags (metadata);

  if (exif_data)
    {
      for (i = 0; exif_data[i] != NULL; i++)
        {
          value = gexiv2_metadata_get_tag_string (metadata, exif_data[i]);
          g_string_append_printf (string, "%s%s%s%s", exif_data[i],
                                  TAG_TAG_DELIMITER,
                                  value,
                                  TAG_LINE_DELIMITER);
          g_free (value);
          n++;
        }

      g_strfreev (exif_data);
    }

  xmp_data = gexiv2_metadata_get_xmp_tags (metadata);

  if (xmp_data)
    {
      for (i = 0; xmp_data[i] != NULL; i++)
        {
          value = gexiv2_metadata_get_tag_string (metadata, xmp_data[i]);
          g_string_append_printf (string, "%s%s%s%s", xmp_data[i],
                                  TAG_TAG_DELIMITER,
                                  value,
                                  TAG_LINE_DELIMITER);
          g_free (value);
          n++;
        }

      g_strfreev (xmp_data);
    }

  iptc_data = gexiv2_metadata_get_iptc_tags (metadata);

  if (iptc_data)
    {
      for (i = 0; iptc_data[i] != NULL; i++)
        {
          value = gexiv2_metadata_get_tag_string (metadata, iptc_data[i]);
          g_string_append_printf (string, "%s%s%s%s", iptc_data[i],
                                  TAG_TAG_DELIMITER,
                                  value,
                                  TAG_LINE_DELIMITER);
          g_free (value);
          n++;
        }

      g_strfreev (iptc_data);
    }

  return g_string_free (string, FALSE);
}

/**
 * Serializing metadata as a string
 */
gchar *
gimp_metadata_serialize_to_xml (GimpMetadata *metadata)
{
  GString  *string;
  gchar   **exif_data = NULL;
  gchar   **iptc_data = NULL;
  gchar   **xmp_data  = NULL;
  gchar    *value;
  gchar    *escaped;
  gint      i;

  g_return_val_if_fail (GEXIV2_IS_METADATA (metadata), NULL);

  string = g_string_new (NULL);

  g_string_append (string, "<?xml version='1.0' encoding='UTF-8'?>\n");
  g_string_append (string, "<metadata>\n");

  exif_data = gexiv2_metadata_get_exif_tags (metadata);

  if (exif_data)
    {
      for (i = 0; exif_data[i] != NULL; i++)
        {
          value   = gexiv2_metadata_get_tag_string (metadata, exif_data[i]);
          escaped = g_markup_escape_text (value, -1);

          g_string_append_printf (string, "  <tag name=\"%s\">%s</tag>\n",
                                  exif_data[i], escaped);

          g_free (escaped);
          g_free (value);
        }

      g_strfreev (exif_data);
    }

  xmp_data = gexiv2_metadata_get_xmp_tags (metadata);

  if (xmp_data)
    {
      for (i = 0; xmp_data[i] != NULL; i++)
        {
          value   = gexiv2_metadata_get_tag_string (metadata, xmp_data[i]);
          escaped = g_markup_escape_text (value, -1);

          g_string_append_printf (string, "  <tag name=\"%s\">%s</tag>\n",
                                  xmp_data[i], escaped);

          g_free (escaped);
          g_free (value);
        }

      g_strfreev (xmp_data);
    }

  iptc_data = gexiv2_metadata_get_iptc_tags (metadata);

  if (iptc_data)
    {
      for (i = 0; iptc_data[i] != NULL; i++)
        {
          value   = gexiv2_metadata_get_tag_string (metadata, iptc_data[i]);
          escaped = g_markup_escape_text (value, -1);

          g_string_append_printf (string, "  <tag name=\"%s\">%s</tag>\n",
                                  iptc_data[i], escaped);

          g_free (escaped);
          g_free (value);
        }

      g_strfreev (iptc_data);
    }

  g_string_append (string, "</metadata>\n");

  return g_string_free (string, FALSE);
}

/*
 * reads metadata from a physical file
 */
GimpMetadata  *
gimp_metadata_load_from_file (GFile   *file,
                              GError **error)
{
  GExiv2Metadata *meta = NULL;
  gchar          *path;
  gchar          *filename;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  path = g_file_get_path (file);

  if (! path)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Can load metadata only from local files"));
      return NULL;
    }

#ifdef G_OS_WIN32
  filename = g_win32_locale_filename_from_utf8 (path);
#else
  filename = g_strdup (path);
#endif

  g_free (path);

  if (gexiv2_initialize ())
    {
      meta = gexiv2_metadata_new ();

      if (! gexiv2_metadata_open_path (meta, filename, error))
        {
          g_object_unref (meta);
          g_free (filename);

          return NULL;
        }
    }

  g_free (filename);

  return meta;
}

/*
 * saves metadata to a physical file
 */
gboolean
gimp_metadata_save_to_file (GimpMetadata  *metadata,
                            GFile         *file,
                            GError       **error)
{
  gchar    *path;
  gchar    *filename;
  gboolean  success;

  g_return_val_if_fail (GEXIV2_IS_METADATA (metadata), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  path = g_file_get_path (file);

  if (! path)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   _("Can save metadata only from to files"));
      return FALSE;
    }

#ifdef G_OS_WIN32
  filename = g_win32_locale_filename_from_utf8 (path);
#else
  filename = g_strdup (path);
#endif

  g_free (path);

  success = gexiv2_metadata_save_file (metadata, filename, error);

  g_free (filename);

  return success;
}

void
gimp_metadata_set_pixel_size (GimpMetadata *metadata,
                              gint          width,
                              gint          height)
{
  gchar buffer[32];

  g_return_if_fail (GEXIV2_IS_METADATA (metadata));

  g_snprintf (buffer, sizeof (buffer), "%d", width);
  gexiv2_metadata_set_tag_string (metadata, "Exif.Image.ImageWidth", buffer);

  g_snprintf (buffer, sizeof (buffer), "%d", height);
  gexiv2_metadata_set_tag_string (metadata, "Exif.Image.ImageLength", buffer);
}

void
gimp_metadata_set_bits_per_sample (GimpMetadata *metadata,
                                   gint          bps)
{
  gchar buffer[32];

  g_return_if_fail (GEXIV2_IS_METADATA (metadata));

  g_snprintf (buffer, sizeof (buffer), "%d %d %d", bps, bps, bps);
  gexiv2_metadata_set_tag_string (metadata, "Exif.Image.BitsPerSample", buffer);
}

/**
 * gets exif resolution
 */
gboolean
gimp_metadata_get_resolution (GimpMetadata *metadata,
                              gdouble      *xres,
                              gdouble      *yres,
                              GimpUnit     *unit)
{
  gchar  *xr;
  gchar  *yr;
  gchar  *un;
  gint    exif_unit;
  gchar **xnom   = NULL;
  gchar **xdenom = NULL;
  gchar **ynom   = NULL;
  gchar **ydenom = NULL;

  g_return_val_if_fail (GEXIV2_IS_METADATA (metadata), FALSE);

  xr = gexiv2_metadata_get_tag_string (metadata, "Exif.Image.XResolution");
  yr = gexiv2_metadata_get_tag_string (metadata, "Exif.Image.YResolution");

  un = gexiv2_metadata_get_tag_string (metadata, "Exif.Image.ResolutionUnit");
  exif_unit = atoi (un);
  g_free (un);

  if (exif_unit == 3)
    *unit = GIMP_UNIT_MM;
  else
    *unit = GIMP_UNIT_INCH;

  if (gimp_metadata_get_rational (xr, 1, &xnom, &xdenom))
    {
      gdouble x1 = g_ascii_strtod (xnom[0], NULL);
      gdouble x2 = g_ascii_strtod (xdenom[0], NULL);
      gdouble xrd;

      if (x2 == 0.0)
        return FALSE;

      xrd = x1 / x2;

      if (exif_unit == 3)
        xrd *= 2.54;

      *xres = xrd;
    }

  if (gimp_metadata_get_rational (yr, 1, &ynom, &ydenom))
    {
      gdouble y1 = g_ascii_strtod (ynom[0], NULL);
      gdouble y2 = g_ascii_strtod (ydenom[0], NULL);
      gdouble yrd;

      if (y2 == 0.0)
        return FALSE;

      yrd = y1 / y2;

      if (exif_unit == 3)
        yrd *= 2.54;

      *yres = yrd;
    }

  g_free (xr);
  g_free (yr);

  g_strfreev (xnom);
  g_strfreev (xdenom);
  g_strfreev (ynom);
  g_strfreev (ydenom);

  return TRUE;
}

/**
 * sets exif resolution
 */
void
gimp_metadata_set_resolution (GimpMetadata *metadata,
                              gdouble       xres,
                              gdouble       yres,
                              GimpUnit      unit)
{
  gchar buffer[32];
  gint  exif_unit;

  g_return_if_fail (GEXIV2_IS_METADATA (metadata));

  if (gimp_unit_is_metric (unit))
    {
      xres /= 2.54;
      yres /= 2.54;

      exif_unit = 3;
    }
  else
    {
      exif_unit = 2;
    }

  g_ascii_formatd (buffer, sizeof (buffer), "%.0f/1", xres);
  gexiv2_metadata_set_tag_string (metadata, "Exif.Image.XResolution", buffer);

  g_ascii_formatd (buffer, sizeof (buffer), "%.0f/1", yres);
  gexiv2_metadata_set_tag_string (metadata, "Exif.Image.YResolution", buffer);

  g_snprintf (buffer, sizeof (buffer), "%d", exif_unit);
  gexiv2_metadata_set_tag_string (metadata, "Exif.Image.ResolutionUnit", buffer);
}

/**
 * checks for supported tags
 */
gboolean
gimp_metadata_is_tag_supported (const gchar *tag,
                                const gchar *mime_type)
{
  gint j;

  g_return_val_if_fail (tag != NULL, FALSE);
  g_return_val_if_fail (mime_type != NULL, FALSE);

  for (j = 0; j < G_N_ELEMENTS (unsupported_tags); j++)
    {
      if (g_str_has_prefix (tag, unsupported_tags[j]))
        {
          return FALSE;
        }
    }

  if (! strcmp (mime_type, "image/jpeg"))
    {
      for (j = 0; j < G_N_ELEMENTS (tiff_tags); j++)
        {
          if (g_str_has_prefix (tag, tiff_tags[j]))
            {
              return FALSE;
            }
        }
    }
  else if (! strcmp (mime_type, "image/tiff"))
    {
      for (j = 0; j < G_N_ELEMENTS (jpeg_tags); j++)
        {
          if (g_str_has_prefix (tag, jpeg_tags[j]))
            {
              return FALSE;
            }
        }
    }

  return TRUE;
}


/* private functions */

/**
 * determines the amount of delimiters in serialized
 * metadata string
 */
static gint
gimp_metadata_length (const gchar *testline,
                      const gchar *delim)
{
  gchar *delim_test;
  gint   i;
  gint   sum;

  delim_test = g_strdup (testline);

  sum =0;

  for (i=0; i < strlen (delim_test); i++)
    {
      if (delim_test[i] == delim[0])
        sum++;
    }

  g_free (delim_test);

  return sum;
}

/**
 * gets rational values from string
 */
static gboolean
gimp_metadata_get_rational (const gchar   *value,
                            gint           sections,
                            gchar       ***numerator,
                            gchar       ***denominator)
{
  GSList *nomlist = NULL;
  GSList *denomlist = NULL;

  GSList *nlist, *dlist;

  gchar   sect[] = " ";
  gchar   rdel[] = "/";
  gchar **sects;
  gchar **nom = NULL;
  gint    i;
  gint    n;

  gchar **num;
  gchar **den;

  if (! value)
    return FALSE;

  if (gimp_metadata_length (value, sect) == (sections -1))
    {
      i = 0;
      sects = g_strsplit (value, sect, -1);
      while (sects[i] != NULL)
        {
          if(gimp_metadata_length (sects[i], rdel) == 1)
            {
              nom = g_strsplit (sects[i], rdel, -1);
              nomlist = g_slist_prepend (nomlist, g_strdup (nom[0]));
              denomlist = g_slist_prepend (denomlist, g_strdup (nom[1]));
            }
          else
            {
              return FALSE;
            }
          i++;
        }
    }
  else
    {
      return FALSE;
    }

  n = i;

  num = g_new0 (gchar*, i + 1);
  den = g_new0 (gchar*, n + 1);

  for (nlist = nomlist; nlist; nlist = nlist->next)
    num[--i] = nlist->data;

  for (dlist = denomlist; dlist; dlist = dlist->next)
    den[--n] = dlist->data;

  *numerator = num;
  *denominator = den;

  g_slist_free (nomlist);
  g_slist_free (denomlist);

  g_strfreev (sects);
  g_strfreev (nom);

  return TRUE;
}
