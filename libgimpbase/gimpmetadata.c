/* LIBGIMPBASE - The GIMP Basic Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmetadata.c
 * Copyright (C) 2013 Hartmut Kuhse <hartmutkuhse@src.gnome.org>
 *                    Michael Natterer <mitch@gimp.org>
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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gio/gio.h>

#include "libgimpmath/gimpmath.h"

#include "gimpbasetypes.h"

#include "gimplimits.h"
#include "gimpmetadata.h"
#include "gimpunit.h"

#include "libgimp/libgimp-intl.h"

/**
 * SECTION: gimpmetadata
 * @title: GimpMetadata
 * @short_description: Basic functions for handling #GimpMetadata objects.
 * @see_also: gimp_image_metadata_load_prepare(),
 *            gimp_image_metadata_load_finish(),
 *            gimp_image_metadata_save_prepare(),
 *            gimp_image_metadata_save_finish().
 *
 * Basic functions for handling #GimpMetadata objects.
 **/

struct _GimpMetadata
{
  GExiv2Metadata parent_instance;
};

#define GIMP_METADATA_ERROR gimp_metadata_error_quark ()

static GQuark   gimp_metadata_error_quark     (void);
static void     gimp_metadata_copy_tag        (GExiv2Metadata  *src,
                                               GExiv2Metadata  *dest,
                                               const gchar     *tag);
static void     gimp_metadata_copy_tags       (GExiv2Metadata  *src,
                                               GExiv2Metadata  *dest,
                                               const gchar    **tags);
static void     gimp_metadata_add             (GimpMetadata    *src,
                                               GimpMetadata    *dest);
static void     gimp_metadata_add_namespace   (GHashTable      *namespaces,
                                               GString         *xml,
                                               gchar           *prefix);
static void gimp_metadata_add_xmp_namespaces  (GHashTable      *namespaces,
                                               GString         *xml,
                                               const gchar     *tag);


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
  "Exif.Image.XPTitle",
  "Exif.Image.XPComment",
  "Exif.Image.XPAuthor",
  "Exif.Image.XPKeywords",
  "Exif.Image.XPSubject",
  "Exif.Image.DNGVersion",
  "Exif.Image.DNGBackwardVersion",
  "Exif.Iop",
  /* FIXME Even though adding the tags below fixes the issue it's not very flexible.
     It might be better in the long run if there was a way for a user to configure which
     tags to block or a way for us to detect problems with tags before writing them. */
  /* Issues #1367, #2253. Offending tag is PreviewOffset but the other Preview tags
     (PreviewResolution, PreviewLength, PreviewImageBorders) also make no sense because
     we are not including a Pentax specific preview image. */
  "Exif.Pentax.Preview",
  "Exif.PentaxDng.Preview",
  /* Never save the complete brand specific MakerNote data. We load and
   * should only save the specific brand tags inside the MakerNote.
   * Sometimes the MakerNote is invalid or exiv2 doesn't know how to parse
   * it. In that case we still get the (invalid) MakerNote, but not the
   * individual tags or just a subset of them.
   * If there are recognized brand specific tags, exiv2 will create the
   * required MakerNote itself (which in can still be invalid but that's an
   * exiv2 issue not ours). */
  "Exif.Photo.MakerNote",
  "Exif.MakerNote.ByteOrder",
  "Exif.MakerNote.Offset",
  /* Photoshop resources can contain sensitive data. We should not save the
   * unedited original state. */
  "Exif.Image.ImageResources",
  "Exif.Image.0x935c",
};

static const guint8 minimal_exif[] =
{
  0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46, 0x00, 0x01,
  0x01, 0x01, 0x00, 0x5a, 0x00, 0x5a, 0x00, 0x00, 0xff, 0xe1
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

G_DEFINE_TYPE (GimpMetadata, gimp_metadata, GEXIV2_TYPE_METADATA)


static void
gimp_metadata_class_init (GimpMetadataClass *klass)
{
  GError *error = NULL;

  if (! gexiv2_metadata_try_register_xmp_namespace ("http://ns.adobe.com/DICOM/",
                                                    "DICOM", &error))
    {
      g_printerr ("Failed to register XMP namespace 'DICOM': %s\n", error->message);
      g_clear_error (&error);
    }

  if (! gexiv2_metadata_try_register_xmp_namespace ("http://darktable.sf.net/",
                                                    "darktable", &error))
    {
      g_printerr ("Failed to register XMP namespace 'darktable': %s\n", error->message);
      g_clear_error (&error);
    }

  /* Usage example Xmp.GIMP.tagname */
  if (! gexiv2_metadata_try_register_xmp_namespace ("http://www.gimp.org/xmp/",
                                                    "GIMP", &error))
    {
      g_printerr ("Failed to register XMP namespace 'GIMP': %s\n", error->message);
      g_clear_error (&error);
    }
}

static void
gimp_metadata_init (GimpMetadata *metadata)
{
}

/**
 * gimp_metadata_get_guid:
 *
 * Generate Version 4 UUID/GUID.
 *
 * Returns: The new GUID/UUID string.
 *
 * Since: 2.10
 */
gchar *
gimp_metadata_get_guid (void)
{
  GRand       *rand;
  gint         bake;
  gchar       *GUID;
  const gchar *szHex = "0123456789abcdef-";

  rand = g_rand_new ();

#define DALLOC 36

  GUID = g_malloc0 (DALLOC + 1);

  for (bake = 0; bake < DALLOC; bake++)
    {
      gint  r = g_rand_int (rand) % 16;
      gchar c = ' ';

      switch (bake)
        {
        default:
          c = szHex [r];
          break;

        case 19 :
          c = szHex [(r & 0x03) | 0x08];
          break;

        case 8:
        case 13:
        case 18:
        case 23:
          c = '-';
          break;

        case 14:
          c = '4';
          break;
        }

      GUID[bake] = (bake < DALLOC) ? c : 0x00;
    }

  g_rand_free (rand);

  return GUID;
}

/**
 * gimp_metadata_add_history:
 *
 * Add XMP mm History data to file metadata.
 *
 * Since: 2.10
 */
void
gimp_metadata_add_xmp_history (GimpMetadata *metadata,
                               gchar        *state_status)
{
  time_t     now;
  struct tm *now_tm;
  gchar     *tmp;
  char       timestr[256];
  char       tzstr[7];
  gchar      strdata[1024];
  gchar      tagstr[1024];
  gchar     *uuid;
  gchar     *str;
  gchar     *did;
  gchar     *odid;
  GError    *error = NULL;
  gint       id_count;
  gint       found;
  gint       lastfound;
  gint       count;
  int        ii;

  static const gchar *tags[] =
  {
    "Xmp.xmpMM.InstanceID",
    "Xmp.xmpMM.DocumentID",
    "Xmp.xmpMM.OriginalDocumentID",
    "Xmp.xmpMM.History"
  };

  static const gchar *history_tags[] =
  {
    "/stEvt:action",
    "/stEvt:instanceID",
    "/stEvt:when",
    "/stEvt:softwareAgent",
    "/stEvt:changed"
  };

  g_return_if_fail (GIMP_IS_METADATA (metadata));

  /* Update new Instance ID */
  uuid = gimp_metadata_get_guid ();

  str = g_strconcat ("xmp.iid:", uuid, NULL);

  gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                      tags[0], str, NULL);
  g_free (uuid);
  g_free (str);

  /* Update new Document ID if none found */
  did = gexiv2_metadata_try_get_tag_interpreted_string (GEXIV2_METADATA (metadata),
                                                        tags[1], NULL);
  if (! did || ! strlen (did))
    {
      gchar *did_data;
      gchar *uuid = gimp_metadata_get_guid ();

      did_data = g_strconcat ("gimp:docid:gimp:", uuid, NULL);

      gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                          tags[1], did_data, NULL);
      g_free (uuid);
      g_free (did_data);
    }

  /* Update new Original Document ID if none found */
  odid = gexiv2_metadata_try_get_tag_interpreted_string (GEXIV2_METADATA (metadata),
                                                         tags[2], NULL);
  if (! odid || ! strlen (odid))
    {
      gchar *did_data;
      gchar *uuid = gimp_metadata_get_guid ();

      did_data = g_strconcat ("xmp.did:", uuid, NULL);

      gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                          tags[2], did_data, NULL);
      g_free (uuid);
      g_free (did_data);
    }

  /* Handle Xmp.xmpMM.History */

  gexiv2_metadata_try_set_xmp_tag_struct (GEXIV2_METADATA (metadata),
                                          tags[3],
                                          GEXIV2_STRUCTURE_XA_SEQ,
                                          NULL);

  /* Find current number of entries for Xmp.xmpMM.History */
  found = 0;
  for (count = 1; count < 65536; count++)
    {
      lastfound = 0;
      for (ii = 0; ii < 5; ii++)
        {
          g_snprintf (tagstr, sizeof (tagstr), "%s[%d]%s",
                      tags[3], count, history_tags[ii]);

          if (gexiv2_metadata_try_has_tag (GEXIV2_METADATA (metadata),
                                           tagstr, NULL))
            {
              lastfound = 1;
            }
        }

      if (lastfound == 0)
        break;

      found++;
    }

  id_count = found + 1;

  memset (tagstr, 0, sizeof (tagstr));

  g_snprintf (tagstr, sizeof (tagstr), "%s[%d]%s",
              tags[3], id_count, history_tags[0]);

  gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                      tagstr, "saved", &error);
  if (error)
    {
      g_printerr ("%s: failed to set metadata '%s': %s\n",
                  G_STRFUNC, tagstr, error->message);
      g_clear_error (&error);
    }

  memset (tagstr, 0, sizeof (tagstr));
  memset (strdata, 0, sizeof (strdata));

  uuid = gimp_metadata_get_guid ();

  g_snprintf (tagstr, sizeof (tagstr), "%s[%d]%s",
              tags[3], id_count, history_tags[1]);
  g_snprintf (strdata, sizeof (strdata), "xmp.iid:%s",
              uuid);

  gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                      tagstr, strdata, &error);
  if (error)
    {
      g_printerr ("%s: failed to set metadata '%s': %s\n",
                  G_STRFUNC, tagstr, error->message);
      g_clear_error (&error);
    }
  g_free(uuid);

  memset (tagstr, 0, sizeof (tagstr));

  g_snprintf (tagstr, sizeof (tagstr), "%s[%d]%s",
              tags[3], id_count, history_tags[2]);

  /* get local time */
  time (&now);
  now_tm = localtime (&now);

  /* get timezone and fix format */
  strftime (tzstr, 7, "%z", now_tm);
  tzstr[6] = '\0';
  tzstr[5] = tzstr[4];
  tzstr[4] = tzstr[3];
  tzstr[3] = ':';

  /* get current time and timezone string */
  strftime (timestr, 256, "%Y-%m-%dT%H:%M:%S", now_tm);
  tmp = g_strdup_printf ("%s%s", timestr, tzstr);
  gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                      tagstr, tmp, &error);
  if (error)
    {
      g_printerr ("%s: failed to set metadata '%s': %s\n",
                  G_STRFUNC, tagstr, error->message);
      g_clear_error (&error);
    }
  g_free (tmp);

  memset (tagstr, 0, sizeof (tagstr));

  g_snprintf (tagstr, sizeof (tagstr), "%s[%d]%s",
              tags[3], id_count, history_tags[3]);

  gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                      tagstr,
                                      PACKAGE_STRING " "
#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
                                      "(Windows)",
#elif defined(__linux__)
                                      "(Linux)",
#elif defined(__APPLE__) && defined(__MACH__)
                                      "(Mac OS)",
#elif defined(unix) || defined(__unix__) || defined(__unix)
                                      "(Unix)",
#else
                                      "(Unknown)",
#endif
                                      &error);
  if (error)
    {
      g_printerr ("%s: failed to set metadata '%s': %s\n",
                  G_STRFUNC, tagstr, error->message);
      g_clear_error (&error);
    }

  memset (tagstr, 0, sizeof (tagstr));

  g_snprintf (tagstr, sizeof (tagstr), "%s[%d]%s",
              tags[3], id_count, history_tags[4]);

  str = g_strconcat ("/", state_status, NULL);

  gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                      tagstr, str, &error);
  g_free (str);
  if (error)
    {
      g_printerr ("%s: failed to set metadata '%s': %s\n",
                  G_STRFUNC, tagstr, error->message);
      g_clear_error (&error);
    }
}

/**
 * gimp_metadata_new:
 *
 * Creates a new #GimpMetadata instance.
 *
 * Returns: (transfer full): The new #GimpMetadata.
 *
 * Since: 2.10
 */
GimpMetadata *
gimp_metadata_new (void)
{
  GimpMetadata *metadata = NULL;

  if (gexiv2_initialize ())
    {
      metadata = g_object_new (GIMP_TYPE_METADATA, NULL);

      if (! gexiv2_metadata_open_buf (GEXIV2_METADATA (metadata),
                                      wilber_jpg, wilber_jpg_len,
                                      NULL))
        {
          g_object_unref (metadata);

          return NULL;
        }
    }

  return metadata;
}

/**
 * gimp_metadata_duplicate:
 * @metadata: The object to duplicate, or %NULL.
 *
 * Duplicates a #GimpMetadata instance.
 *
 * Returns: (transfer full):
 *               The new #GimpMetadata, or %NULL if @metadata is %NULL.
 *
 * Since: 2.10
 */
GimpMetadata *
gimp_metadata_duplicate (GimpMetadata *metadata)
{
  GimpMetadata *new_metadata = NULL;

  g_return_val_if_fail (metadata == NULL || GIMP_IS_METADATA (metadata), NULL);

  if (metadata)
    {
      gchar *xml;

      xml = gimp_metadata_serialize (metadata);
      new_metadata = gimp_metadata_deserialize (xml);
      g_free (xml);
    }

  return new_metadata;
}

typedef struct
{
  gchar         name[1024];
  gchar         prefix[256];
  gboolean      base64;
  gboolean      excessive_message_shown;
  GimpMetadata *metadata;
} GimpMetadataParseData;

static const gchar*
gimp_metadata_attribute_name_to_value (const gchar **attribute_names,
                                       const gchar **attribute_values,
                                       const gchar  *name)
{
  while (*attribute_names)
    {
      if (! strcmp (*attribute_names, name))
        {
          return *attribute_values;
        }

      attribute_names++;
      attribute_values++;
    }

  return NULL;
}

static void
gimp_metadata_deserialize_start_element (GMarkupParseContext *context,
                                         const gchar         *element_name,
                                         const gchar        **attribute_names,
                                         const gchar        **attribute_values,
                                         gpointer             user_data,
                                         GError             **error)
{
  GimpMetadataParseData *parse_data = user_data;

  if (! strcmp (element_name, "tag"))
    {
      const gchar *name;
      const gchar *encoding;

      name = gimp_metadata_attribute_name_to_value (attribute_names,
                                                    attribute_values,
                                                    "name");
      encoding = gimp_metadata_attribute_name_to_value (attribute_names,
                                                        attribute_values,
                                                        "encoding");

      if (! name)
        {
          g_set_error (error, GIMP_METADATA_ERROR, 1001,
                       "Element 'tag' does not contain required attribute 'name'.");
          return;
        }

      g_strlcpy (parse_data->name, name, sizeof (parse_data->name));

      parse_data->base64 = (encoding && ! strcmp (encoding, "base64"));
    }
  else if (! strcmp (element_name, "namespace"))
    {
      const gchar *url;
      const gchar *prefix;

      prefix = gimp_metadata_attribute_name_to_value (attribute_names,
                                                      attribute_values,
                                                      "prefix");
      url = gimp_metadata_attribute_name_to_value (attribute_names,
                                                   attribute_values,
                                                   "url");
      if (! prefix)
        {
          g_set_error (error, GIMP_METADATA_ERROR, 1002,
                       "Element 'namespace' does not contain required attribute 'prefix'.");
          return;
        }
      if (! url)
        {
          g_set_error (error, GIMP_METADATA_ERROR, 1003,
                       "Element 'namespace' does not contain required attribute 'url'.");
          return;
        }

      g_strlcpy (parse_data->prefix, prefix, sizeof (parse_data->prefix));
      g_strlcpy (parse_data->name, url, sizeof (parse_data->name));
    }
}

static void
gimp_metadata_deserialize_end_element (GMarkupParseContext *context,
                                       const gchar         *element_name,
                                       gpointer             user_data,
                                       GError             **error)
{
}

static void
gimp_metadata_deserialize_text (GMarkupParseContext  *context,
                                const gchar          *text,
                                gsize                 text_len,
                                gpointer              user_data,
                                GError              **error)
{
  GimpMetadataParseData *parse_data = user_data;
  const gchar           *current_element;

  current_element = g_markup_parse_context_get_element (context);

  if (! g_strcmp0 (current_element, "tag"))
    {
      gchar *value = g_strndup (text, text_len);

      if (parse_data->base64)
        {
          guchar *decoded;
          gsize   len;

          decoded = g_base64_decode (value, &len);

          if (decoded[len - 1] == '\0')
            {
              g_free (value);
              value = (gchar *) decoded;
            }
          else
            {
              g_clear_pointer (&value,   g_free);
              g_clear_pointer (&decoded, g_free);
            }
        }

      if (value)
        {
          GExiv2Metadata  *g2_metadata = GEXIV2_METADATA (parse_data->metadata);
          GError          *error       = NULL;
          gchar          **values;

          values = gexiv2_metadata_try_get_tag_multiple (g2_metadata,
                                                         parse_data->name,
                                                         &error);

          if (error)
            {
              g_printerr ("%s: %s\n", G_STRFUNC, error->message);
              g_clear_error (&error);
              g_strfreev (values);
            }
          else if (values)
            {
              guint length = g_strv_length (values);

              if (length > 1000 &&
                  ! g_strcmp0 (parse_data->name, "Xmp.photoshop.DocumentAncestors"))
                {
                  /* Issue #8025, see also #7464 Some XCF images can have huge
                   * amounts of this tag, apparently due to a bug in PhotoShop.
                   * This makes deserializing it in the way we currently do
                   * too slow. Until we can change this let's ignore everything
                   * but the first 1000 values when serializing. */

                  if (! parse_data->excessive_message_shown)
                    {
                      g_message ("Excessive number of Xmp.photoshop.DocumentAncestors tags found. "
                                 "Only keeping the first 1000 values.");
                      parse_data->excessive_message_shown = TRUE;
                    }
                }
              else
                {
                  values = g_renew (gchar *, values, length + 2);
                  values[length]     = g_strdup (value);
                  values[length + 1] = NULL;

                  gexiv2_metadata_try_set_tag_multiple (g2_metadata,
                                                        parse_data->name,
                                                        (const gchar **) values,
                                                        &error);
                  if (error)
                    {
                      g_warning ("%s: failed to set multiple metadata '%s': %s\n",
                                 G_STRFUNC, parse_data->name, error->message);
                      g_clear_error (&error);
                    }
                }
              g_strfreev (values);
            }
          else
            {
              gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (g2_metadata),
                                                  parse_data->name,
                                                  value, &error);
              if (error)
                {
                  g_warning ("%s: failed to set metadata '%s': %s\n",
                             G_STRFUNC, parse_data->name, error->message);
                  g_clear_error (&error);
                }
            }
          g_free (value);
        }
    }
  else if (! g_strcmp0 (current_element, "namespace"))
    {
      GError *error = NULL;

      gexiv2_metadata_try_register_xmp_namespace (parse_data->name,
                                                  parse_data->prefix,
                                                  &error);
      if (error) {
          g_warning ("%s: failed to register namespace %s (url: '%s'): %s\n",
                     G_STRFUNC, parse_data->prefix, parse_data->name,
                     error->message);
          g_clear_error(&error);
      }
    }
}

static  void
gimp_metadata_deserialize_error (GMarkupParseContext *context,
                                 GError              *error,
                                 gpointer             user_data)
{
  g_printerr ("Metadata parse error: %s\n", error->message);
}

/**
 * gimp_metadata_deserialize:
 * @metadata_xml: A string of serialized metadata XML.
 *
 * Deserializes a string of XML that has been created by
 * gimp_metadata_serialize().
 *
 * Returns: (transfer full): The new #GimpMetadata.
 *
 * Since: 2.10
 */
GimpMetadata *
gimp_metadata_deserialize (const gchar *metadata_xml)
{
  GimpMetadata          *metadata;
  GMarkupParser          markup_parser;
  GimpMetadataParseData  parse_data;
  GMarkupParseContext   *context;

  g_return_val_if_fail (metadata_xml != NULL, NULL);

  metadata = gimp_metadata_new ();

  parse_data.metadata = metadata;
  parse_data.excessive_message_shown = FALSE;

  markup_parser.start_element = gimp_metadata_deserialize_start_element;
  markup_parser.end_element   = gimp_metadata_deserialize_end_element;
  markup_parser.text          = gimp_metadata_deserialize_text;
  markup_parser.passthrough   = NULL;
  markup_parser.error         = gimp_metadata_deserialize_error;

  context = g_markup_parse_context_new (&markup_parser, 0, &parse_data, NULL);

  g_markup_parse_context_parse (context,
                                metadata_xml, strlen (metadata_xml),
                                NULL);

  g_markup_parse_context_unref (context);

  return metadata;
}

static gchar *
gimp_metadata_escape (const gchar *name,
                      const gchar *value,
                      gboolean    *base64)
{
  if (! g_utf8_validate (value, -1, NULL))
    {
      gchar *encoded;

      encoded = g_base64_encode ((const guchar *) value, strlen (value) + 1);

      g_printerr ("Invalid UTF-8 in metadata value %s, encoding as base64: %s\n",
                  name, encoded);

      *base64 = TRUE;

      return encoded;
    }

  *base64 = FALSE;

  return g_markup_escape_text (value, -1);
}

static void
gimp_metadata_append_tag (GString     *string,
                          const gchar *name,
                          gchar       *value,
                          gboolean     base64)
{
  if (value)
    {
      if (base64)
        {
          g_string_append_printf (string, "  <tag name=\"%s\" encoding=\"base64\">%s</tag>\n",
                                  name, value);
        }
      else
        {
          g_string_append_printf (string, "  <tag name=\"%s\">%s</tag>\n",
                                  name, value);
        }

      g_free (value);
    }
}

static void
gimp_metadata_add_namespace (GHashTable  *namespaces,
                             GString     *xml,
                             gchar       *prefix)
{
  if (! g_hash_table_lookup (namespaces, prefix))
    {
      gchar  *namespace_url;
      GError *error = NULL;

      namespace_url = gexiv2_metadata_try_get_xmp_namespace_for_tag (prefix, &error);

      if (! namespace_url)
        {
          /* Weird, we didn't find the namespace url.
             Let's add a dummy url, that way we can keep the tags. */
          if (error)
            {
              g_warning ("XMP namespace url not found! %s", error->message);
              g_clear_error (&error);
            }

          /* Fix the one namespace url we know of, and add a generic fix for
             any others. */
          if (g_strcmp0 (prefix, "Item") == 0)
            /* FIXME Remove this specific check for Item after this is fixed
               in our dependencies (exiv2?), see issue #10557. */
            namespace_url = g_strdup ("http://ns.google.com/photos/1.0/container/item/");
          else
            namespace_url = g_strdup_printf ("http://missing-url.org/%s/", prefix);

          if (! gexiv2_metadata_try_register_xmp_namespace (namespace_url,
                                                            prefix, &error))
            {
              g_warning ("Registering XMP namespace failed! %s\n", error->message);
              g_clear_error (&error);
            }
        }

      if (namespace_url)
        {
          g_debug ("Adding namespace %s, url: %s", prefix, namespace_url);

          if (! g_hash_table_insert (namespaces, prefix, namespace_url))
            g_warning ("Namespace already present: %s!", prefix);

          g_string_append_printf (xml,
                                  "  <namespace prefix=\"%s\" url=\"%s\"></namespace>\n",
                                  prefix, namespace_url);

          /* namespace_url and prefix are added to hashtable, so we don't free here */
        }
      else
        {
          g_free (prefix);
        }
    }
  else
    {
      g_free (prefix);
    }
}

/* Register a namespace in our xml metadata for each XMP namespace.
 * We use the following XML format:
 * <namespace prefix="namespace-prefix" url="namespace-url"></namespace>
 *
 * There are two types of namespace prefixes:
 * - Xmp.prefix.whatever, and
 * - /prefix:something, which is prefixed by the above
 *
 * We use a hashtable to keep track of which namespaces we have already
 * seen in the current run.
 */

static void
gimp_metadata_add_xmp_namespaces (GHashTable  *namespaces,
                                  GString     *xml,
                                  const gchar *tag)
{
  gchar  *tag_ptr = (gchar *) tag;
  gchar  *prefix;
  gchar **substrings;

  /* Find word between the first and second '.' */
  substrings = g_strsplit ((gchar *) tag_ptr, ".", 3);
  if (substrings && substrings[1])
    {
      prefix = g_strdup (substrings[1]);

      gimp_metadata_add_namespace (namespaces, xml, prefix);
    }
  g_strfreev (substrings);

  /* Multiple namespaces in the form /prefix:value are possible in one tag. */
  while (tag_ptr)
    {
      gchar *tag_next = NULL;

      tag_ptr = strstr (tag_ptr, "/");
      if (! tag_ptr || strlen (tag_ptr) <= 1)
        break;
      tag_ptr++;
      tag_next = strstr (tag_ptr, ":");

      if (tag_next)
        {
          gsize prefix_len = (gsize) tag_next - (gsize) tag_ptr + 1;

          prefix = g_new (gchar, prefix_len);
          g_strlcpy (prefix, tag_ptr, prefix_len);

          gimp_metadata_add_namespace (namespaces, xml, prefix);

          tag_ptr = tag_next;
        }
    }
}

/**
 * gimp_metadata_serialize:
 * @metadata: A #GimpMetadata instance.
 *
 * Serializes @metadata into an XML string that can later be deserialized
 * using gimp_metadata_deserialize().
 *
 * Returns: The serialized XML string.
 *
 * Since: 2.10
 */
gchar *
gimp_metadata_serialize (GimpMetadata *metadata)
{
  GString  *string;
  gchar   **exif_data = NULL;
  gchar   **iptc_data = NULL;
  gchar   **xmp_data  = NULL;
  gchar    *value;
  gchar    *escaped;
  GError   *error     = NULL;
  gboolean  base64;
  gint      i;

  g_return_val_if_fail (GIMP_IS_METADATA (metadata), NULL);

  string = g_string_new (NULL);

  g_string_append (string, "<?xml version='1.0' encoding='UTF-8'?>\n");
  g_string_append (string, "<metadata>\n");

  exif_data = gexiv2_metadata_get_exif_tags (GEXIV2_METADATA (metadata));

  if (exif_data)
    {
      for (i = 0; exif_data[i] != NULL; i++)
        {
          value = gexiv2_metadata_try_get_tag_string (GEXIV2_METADATA (metadata),
                                                      exif_data[i], &error);
          if (value)
            {
              escaped = gimp_metadata_escape (exif_data[i], value, &base64);
              g_free (value);

              gimp_metadata_append_tag (string, exif_data[i], escaped, base64);
            }
          else if (error)
            {
              g_printerr ("%s: failed to get Exif metadata '%s': %s\n",
                          G_STRFUNC, exif_data[i], error->message);
              g_clear_error (&error);
            }
        }

      g_strfreev (exif_data);
    }

  xmp_data = gexiv2_metadata_get_xmp_tags (GEXIV2_METADATA (metadata));

  if (xmp_data)
    {
      GHashTable *namespaces = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

      for (i = 0; xmp_data[i] != NULL; i++)
        {
          gimp_metadata_add_xmp_namespaces (namespaces, string, xmp_data[i]);

          /* XmpText is always a single value, but structures like
           * XmpBag and XmpSeq can have multiple values that need to be
           * treated separately or else saving will do things wrong. */
          if (! g_strcmp0 (gexiv2_metadata_try_get_tag_type (xmp_data[i], NULL), "XmpText"))
            {
              value = gexiv2_metadata_try_get_tag_string (GEXIV2_METADATA (metadata),
                                                          xmp_data[i], &error);
              if (value)
                {
                  escaped = gimp_metadata_escape (xmp_data[i], value, &base64);
                  g_free (value);

                  gimp_metadata_append_tag (string, xmp_data[i], escaped, base64);
                }
              else if (error)
                {
                  g_printerr ("%s: failed to get XMP metadata '%s': %s\n",
                              G_STRFUNC, xmp_data[i], error->message);
                  g_clear_error (&error);
                }
            }
          else
            {
              gchar **values;

              values = gexiv2_metadata_try_get_tag_multiple (GEXIV2_METADATA (metadata),
                                                             xmp_data[i], &error);

              if (values)
                {
                  gint  vi;
                  gint  cnt = 0;

                  if (! g_strcmp0 (xmp_data[i], "Xmp.photoshop.DocumentAncestors"))
                    {
                      /* Issue #7464 Some images can have huge amounts of this
                       * tag (more than 100000 in certain cases), apparently
                       * due to a bug in PhotoShop. This makes deserializing it
                       * in the way we currently do too slow. Until we can
                       * change this let's remove everything but the first 1000
                       * values when serializing. */
                      cnt = g_strv_length (values);

                      if (cnt > 1000)
                        {
                          g_message ("Excessive number of Xmp.photoshop.DocumentAncestors tags found: %d. "
                                     "Only keeping the first 1000 values.", cnt);
                        }
                    }

                  for (vi = 0; values[vi] != NULL && (cnt <= 1000 || vi < 1000); vi++)
                    {
                      escaped = gimp_metadata_escape (xmp_data[i], values[vi], &base64);
                      gimp_metadata_append_tag (string, xmp_data[i], escaped, base64);
                    }

                  g_strfreev (values);
                }
              else if (error)
                {
                  g_printerr ("%s: failed to get multiple XMP metadata '%s': %s\n",
                              G_STRFUNC, xmp_data[i], error->message);
                  g_clear_error (&error);
                }
            }
        }
      g_strfreev (xmp_data);
      g_hash_table_destroy (namespaces);
    }

  iptc_data = gexiv2_metadata_get_iptc_tags (GEXIV2_METADATA (metadata));

  if (iptc_data)
    {
      gchar **iptc_tags = iptc_data;
      gchar  *last_tag  = NULL;

      while (*iptc_tags)
        {
          gchar **values;

          if (last_tag && ! strcmp (*iptc_tags, last_tag))
            {
              iptc_tags++;
              continue;
            }
          last_tag = *iptc_tags;

          values = gexiv2_metadata_try_get_tag_multiple (GEXIV2_METADATA (metadata),
                                                         *iptc_tags, &error);

          if (values)
            {
              for (i = 0; values[i] != NULL; i++)
                {
                  escaped = gimp_metadata_escape (*iptc_tags, values[i], &base64);
                  gimp_metadata_append_tag (string, *iptc_tags, escaped, base64);
                }

              g_strfreev (values);
            }
          else if (error)
            {
              g_printerr ("%s: failed to get multiple IPTC metadata '%s': %s\n",
                          G_STRFUNC, *iptc_tags, error->message);
              g_clear_error (&error);
            }

          iptc_tags++;
        }

      g_strfreev (iptc_data);
    }

  g_string_append (string, "</metadata>\n");

  return g_string_free (string, FALSE);
}

/**
 * gimp_metadata_load_from_file:
 * @file:  The #GFile to load the metadata from
 * @error: Return location for error message
 *
 * Loads #GimpMetadata from @file.
 *
 * Returns: (transfer full): The loaded #GimpMetadata.
 *
 * Since: 2.10
 */
GimpMetadata  *
gimp_metadata_load_from_file (GFile   *file,
                              GError **error)
{
  GimpMetadata *meta = NULL;
  gchar        *path;
  gchar        *filename;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  path = g_file_get_path (file);

  if (! path)
    {
      g_set_error (error, GIMP_METADATA_ERROR, 0,
                   _("Can load metadata only from local files"));
      return NULL;
    }

  filename = g_strdup (path);

  g_free (path);

  if (gexiv2_initialize ())
    {
      meta = g_object_new (GIMP_TYPE_METADATA, NULL);

      if (! gexiv2_metadata_open_path (GEXIV2_METADATA (meta), filename, error))
        {
          g_object_unref (meta);
          g_free (filename);

          return NULL;
        }
    }

  g_free (filename);

  return meta;
}

/**
 * gimp_metadata_save_to_file:
 * @metadata: A #GimpMetadata instance.
 * @file:     The file to save the metadata to
 * @error:    Return location for error message
 *
 * Saves @metadata to @file.
 *
 * Returns: %TRUE on success, %FALSE otherwise.
 *
 * Since: 2.10
 */
gboolean
gimp_metadata_save_to_file (GimpMetadata  *metadata,
                            GFile         *file,
                            GError       **error)
{
  gchar    *path;
  gchar    *filename;
  gboolean  success;

  g_return_val_if_fail (GIMP_IS_METADATA (metadata), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  path = g_file_get_path (file);

  if (! path)
    {
      g_set_error (error, GIMP_METADATA_ERROR, 0,
                   _("Can save metadata only to local files"));
      return FALSE;
    }

  filename = g_strdup (path);

  g_free (path);

  success = gexiv2_metadata_save_file (GEXIV2_METADATA (metadata),
                                       filename, error);

  g_free (filename);

  return success;
}

/**
 * gimp_metadata_set_from_exif:
 * @metadata:         A #GimpMetadata instance.
 * @exif_data: (array length=exif_data_length): The blob of Exif data to set
 * @exif_data_length: Length of @exif_data, in bytes
 * @error:            Return location for error message
 *
 * Sets the tags from a piece of Exif data on @metadata.
 *
 * Returns: %TRUE on success, %FALSE otherwise.
 *
 * Since: 2.10
 */
gboolean
gimp_metadata_set_from_exif (GimpMetadata  *metadata,
                             const guchar  *exif_data,
                             gint           exif_data_length,
                             GError       **error)
{

  GByteArray   *exif_bytes     = NULL;
  GimpMetadata *exif_metadata;
  const guchar  exif_marker[6] = "Exif\0\0";
  const guchar *data;
  gint          data_length;

  g_return_val_if_fail (GIMP_IS_METADATA (metadata), FALSE);
  g_return_val_if_fail (exif_data != NULL || exif_data_length == 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (exif_data_length < 0 || exif_data_length + 2 >= 65536)
    {
      g_set_error (error, GIMP_METADATA_ERROR, 0,
                   _("Invalid Exif data size."));
      return FALSE;
    }

  /* Old GIMP exif parasite marker "Exif\0\0" needs special handling. */
  if (exif_data_length >= 6 &&
      ! memcmp (exif_marker, exif_data, sizeof(exif_marker)))
    {
      guint8        data_size[2]  = { 0, };
      const guint8  eoi[2]        = { 0xff, 0xd9 };

      data_size[0] = ((exif_data_length + 2) & 0xFF00) >> 8;
      data_size[1] = ((exif_data_length + 2) & 0x00FF);

      exif_bytes = g_byte_array_new ();
      exif_bytes = g_byte_array_append (exif_bytes,
                                        minimal_exif, G_N_ELEMENTS (minimal_exif));
      exif_bytes = g_byte_array_append (exif_bytes,
                                        data_size, 2);
      exif_bytes = g_byte_array_append (exif_bytes,
                                        (guint8 *) exif_data, exif_data_length);
      exif_bytes = g_byte_array_append (exif_bytes, eoi, 2);
      data = exif_bytes->data;
      data_length = exif_bytes->len;
    }
  else
    {
      data = exif_data;
      data_length = exif_data_length;
    }

  exif_metadata = gimp_metadata_new ();

  if (! gexiv2_metadata_open_buf (GEXIV2_METADATA (exif_metadata),
                                  data, data_length, error))
    {
      g_object_unref (exif_metadata);
      if (exif_bytes)
        g_byte_array_free (exif_bytes, TRUE);
      return FALSE;
    }

  if (! gexiv2_metadata_has_exif (GEXIV2_METADATA (exif_metadata)))
    {
      g_set_error (error, GIMP_METADATA_ERROR, 0,
                   _("Parsing Exif data failed."));
      g_object_unref (exif_metadata);
      if (exif_bytes)
        g_byte_array_free (exif_bytes, TRUE);
      return FALSE;
    }

  gimp_metadata_add (exif_metadata, metadata);
  g_object_unref (exif_metadata);
  if (exif_bytes)
    g_byte_array_free (exif_bytes, TRUE);

  return TRUE;
}

/**
 * gimp_metadata_set_from_iptc:
 * @metadata:        A #GimpMetadata instance.
 * @iptc_data: (array length=iptc_data_length): The blob of Iptc data to set
 * @iptc_data_length:Length of @iptc_data, in bytes
 * @error:           Return location for error message
 *
 * Sets the tags from a piece of IPTC data on @metadata.
 *
 * Returns: %TRUE on success, %FALSE otherwise.
 *
 * Since: 2.10
 */
gboolean
gimp_metadata_set_from_iptc (GimpMetadata  *metadata,
                             const guchar  *iptc_data,
                             gint           iptc_data_length,
                             GError       **error)
{
  GimpMetadata *iptc_metadata;

  g_return_val_if_fail (GIMP_IS_METADATA (metadata), FALSE);
  g_return_val_if_fail (iptc_data != NULL || iptc_data_length == 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  iptc_metadata = gimp_metadata_new ();

  if (! gexiv2_metadata_open_buf (GEXIV2_METADATA (iptc_metadata),
                                  iptc_data, iptc_data_length, error))
    {
      g_object_unref (iptc_metadata);
      return FALSE;
    }

  if (! gexiv2_metadata_has_iptc (GEXIV2_METADATA (iptc_metadata)))
    {
      g_set_error (error, GIMP_METADATA_ERROR, 0,
                   _("Parsing IPTC data failed."));
      g_object_unref (iptc_metadata);
      return FALSE;
    }

  gimp_metadata_add (iptc_metadata, metadata);
  g_object_unref (iptc_metadata);

  return TRUE;
}

/**
 * gimp_metadata_set_from_xmp:
 * @metadata:        A #GimpMetadata instance.
 * @xmp_data: (array length=xmp_data_length): The blob of XMP data to set
 * @xmp_data_length: Length of @xmp_data, in bytes
 * @error:           Return location for error message
 *
 * Sets the tags from a piece of XMP data on @metadata.
 *
 * Returns: %TRUE on success, %FALSE otherwise.
 *
 * Since: 2.10
 */
gboolean
gimp_metadata_set_from_xmp (GimpMetadata  *metadata,
                            const guchar  *xmp_data,
                            gint           xmp_data_length,
                            GError       **error)
{
  GimpMetadata *xmp_metadata;

  g_return_val_if_fail (GIMP_IS_METADATA (metadata), FALSE);
  g_return_val_if_fail (xmp_data != NULL || xmp_data_length == 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  xmp_metadata = gimp_metadata_new ();

  if (! gexiv2_metadata_open_buf (GEXIV2_METADATA (xmp_metadata),
                                  xmp_data, xmp_data_length, error))
    {
      g_object_unref (xmp_metadata);
      return FALSE;
    }

  if (! gexiv2_metadata_has_xmp (GEXIV2_METADATA (xmp_metadata)))
    {
      g_set_error (error, GIMP_METADATA_ERROR, 0,
                   _("Parsing XMP data failed."));
      g_object_unref (xmp_metadata);
      return FALSE;
    }

  gimp_metadata_add (xmp_metadata, metadata);
  g_object_unref (xmp_metadata);

  return TRUE;
}

/**
 * gimp_metadata_set_pixel_size:
 * @metadata: A #GimpMetadata instance.
 * @width:    Width in pixels
 * @height:   Height in pixels
 *
 * Sets Exif.Image.ImageWidth and Exif.Image.ImageLength on @metadata.
 * If already present, also sets Exif.Photo.PixelXDimension and
 * Exif.Photo.PixelYDimension.
 *
 * Since: 2.10
 */
void
gimp_metadata_set_pixel_size (GimpMetadata *metadata,
                              gint          width,
                              gint          height)
{
  gchar buffer[32];

  g_return_if_fail (GIMP_IS_METADATA (metadata));

  g_snprintf (buffer, sizeof (buffer), "%d", width);
  gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                      "Exif.Image.ImageWidth", buffer, NULL);
  if (gexiv2_metadata_try_has_tag (GEXIV2_METADATA (metadata),
                                   "Exif.Photo.PixelXDimension", NULL))
    {
      gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                          "Exif.Photo.PixelXDimension",
                                          buffer, NULL);
    }

  g_snprintf (buffer, sizeof (buffer), "%d", height);
  gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                      "Exif.Image.ImageLength", buffer, NULL);
  if (gexiv2_metadata_try_has_tag (GEXIV2_METADATA (metadata),
                                   "Exif.Photo.PixelYDimension", NULL))
    {
      gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                          "Exif.Photo.PixelYDimension",
                                          buffer, NULL);
    }
}

/**
 * gimp_metadata_set_bits_per_sample:
 * @metadata:        A #GimpMetadata instance.
 * @bits_per_sample: Bits per pixel, per component
 *
 * Sets Exif.Image.BitsPerSample on @metadata.
 *
 * Since: 2.10
 */
void
gimp_metadata_set_bits_per_sample (GimpMetadata *metadata,
                                   gint          bits_per_sample)
{
  gchar buffer[32];

  g_return_if_fail (GIMP_IS_METADATA (metadata));

  g_snprintf (buffer, sizeof (buffer), "%d %d %d",
              bits_per_sample, bits_per_sample, bits_per_sample);
  gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                      "Exif.Image.BitsPerSample", buffer, NULL);
}

/**
 * gimp_metadata_get_resolution:
 * @metadata: A #GimpMetadata instance.
 * @xres: (out) (optional): Return location for the X Resolution, in ppi
 * @yres: (out) (optional): Return location for the Y Resolution, in ppi
 * @unit: (out) (optional): Return location for the unit unit
 *
 * Returns values based on Exif.Image.XResolution,
 * Exif.Image.YResolution and Exif.Image.ResolutionUnit of @metadata.
 *
 * Returns: %TRUE on success, %FALSE otherwise.
 *
 * Since: 2.10
 */
gboolean
gimp_metadata_get_resolution (GimpMetadata  *metadata,
                              gdouble       *xres,
                              gdouble       *yres,
                              GimpUnit     **unit)
{
  gint xnom, xdenom;
  gint ynom, ydenom;

  g_return_val_if_fail (GIMP_IS_METADATA (metadata), FALSE);

  if (gexiv2_metadata_try_get_exif_tag_rational (GEXIV2_METADATA (metadata),
                                                 "Exif.Image.XResolution",
                                                 &xnom, &xdenom, NULL) &&
      gexiv2_metadata_try_get_exif_tag_rational (GEXIV2_METADATA (metadata),
                                                 "Exif.Image.YResolution",
                                                 &ynom, &ydenom, NULL))
    {
      gchar *un;
      gint   exif_unit = 2;

      un = gexiv2_metadata_try_get_tag_string (GEXIV2_METADATA (metadata),
                                               "Exif.Image.ResolutionUnit", NULL);
      if (un)
        {
          exif_unit = atoi (un);
          g_free (un);
        }

      if (xnom != 0 && xdenom != 0 &&
          ynom != 0 && ydenom != 0)
        {
          gdouble xresolution = (gdouble) xnom / (gdouble) xdenom;
          gdouble yresolution = (gdouble) ynom / (gdouble) ydenom;

          if (exif_unit == 3)
            {
              xresolution *= 2.54;
              yresolution *= 2.54;
            }

         if (xresolution >= GIMP_MIN_RESOLUTION &&
             xresolution <= GIMP_MAX_RESOLUTION &&
             yresolution >= GIMP_MIN_RESOLUTION &&
             yresolution <= GIMP_MAX_RESOLUTION)
           {
             if (xres)
               *xres = xresolution;

             if (yres)
               *yres = yresolution;

             if (unit)
               {
                 if (exif_unit == 3)
                   *unit = gimp_unit_mm ();
                 else
                   *unit = gimp_unit_inch ();
               }

             return TRUE;
           }
        }
    }

  return FALSE;
}

/**
 * gimp_metadata_set_resolution:
 * @metadata: A #GimpMetadata instance.
 * @xres:     The image's X Resolution, in ppi
 * @yres:     The image's Y Resolution, in ppi
 * @unit:     The image's unit
 *
 * Sets Exif.Image.XResolution, Exif.Image.YResolution and
 * Exif.Image.ResolutionUnit of @metadata.
 *
 * Since: 2.10
 */
void
gimp_metadata_set_resolution (GimpMetadata *metadata,
                              gdouble       xres,
                              gdouble       yres,
                              GimpUnit     *unit)
{
  gchar buffer[32];
  gint  exif_unit;
  gint  factor;

  g_return_if_fail (GIMP_IS_METADATA (metadata));

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

  for (factor = 1; factor <= 100 /* arbitrary */; factor++)
    {
      if (fabs (xres * factor - ROUND (xres * factor)) < 0.01 &&
          fabs (yres * factor - ROUND (yres * factor)) < 0.01)
        break;
    }

  gexiv2_metadata_try_set_exif_tag_rational (GEXIV2_METADATA (metadata),
                                             "Exif.Image.XResolution",
                                             ROUND (xres * factor), factor, NULL);

  gexiv2_metadata_try_set_exif_tag_rational (GEXIV2_METADATA (metadata),
                                             "Exif.Image.YResolution",
                                             ROUND (yres * factor), factor, NULL);

  g_snprintf (buffer, sizeof (buffer), "%d", exif_unit);
  gexiv2_metadata_try_set_tag_string (GEXIV2_METADATA (metadata),
                                      "Exif.Image.ResolutionUnit", buffer, NULL);
}

/**
 * gimp_metadata_get_colorspace:
 * @metadata: A #GimpMetadata instance.
 *
 * Returns values based on Exif.Photo.ColorSpace, Xmp.exif.ColorSpace,
 * Exif.Iop.InteroperabilityIndex, Exif.Nikon3.ColorSpace,
 * Exif.Canon.ColorSpace of @metadata.
 *
 * Returns: The colorspace specified by above tags.
 *
 * Since: 2.10
 */
GimpMetadataColorspace
gimp_metadata_get_colorspace (GimpMetadata *metadata)
{
  glong exif_cs = -1;

  g_return_val_if_fail (GIMP_IS_METADATA (metadata),
                        GIMP_METADATA_COLORSPACE_UNSPECIFIED);

  /*  the logic here was mostly taken from darktable and libkexiv2  */

  if (gexiv2_metadata_try_has_tag (GEXIV2_METADATA (metadata),
                                   "Exif.Photo.ColorSpace", NULL))
    {
      exif_cs = gexiv2_metadata_try_get_tag_long (GEXIV2_METADATA (metadata),
                                                  "Exif.Photo.ColorSpace", NULL);
    }
  else if (gexiv2_metadata_try_has_tag (GEXIV2_METADATA (metadata),
                                        "Xmp.exif.ColorSpace", NULL))
    {
      exif_cs = gexiv2_metadata_try_get_tag_long (GEXIV2_METADATA (metadata),
                                                  "Xmp.exif.ColorSpace", NULL);
    }

  if (exif_cs == 0x01)
    {
      return GIMP_METADATA_COLORSPACE_SRGB;
    }
  else if (exif_cs == 0x02)
    {
      return GIMP_METADATA_COLORSPACE_ADOBERGB;
    }
  else
    {
      if (exif_cs == 0xffff)
        {
          gchar *iop_index;

          iop_index = gexiv2_metadata_try_get_tag_string (GEXIV2_METADATA (metadata),
                                                          "Exif.Iop.InteroperabilityIndex", NULL);

          if (! g_strcmp0 (iop_index, "R03"))
            {
              g_free (iop_index);

              return GIMP_METADATA_COLORSPACE_ADOBERGB;
            }
          else if (! g_strcmp0 (iop_index, "R98"))
            {
              g_free (iop_index);

              return GIMP_METADATA_COLORSPACE_SRGB;
            }

          g_free (iop_index);
        }

      if (gexiv2_metadata_try_has_tag (GEXIV2_METADATA (metadata),
                                       "Exif.Nikon3.ColorSpace", NULL))
        {
          glong nikon_cs;

          nikon_cs = gexiv2_metadata_try_get_tag_long (GEXIV2_METADATA (metadata),
                                                       "Exif.Nikon3.ColorSpace", NULL);

          if (nikon_cs == 0x01)
            {
              return GIMP_METADATA_COLORSPACE_SRGB;
            }
          else if (nikon_cs == 0x02)
            {
              return GIMP_METADATA_COLORSPACE_ADOBERGB;
            }
        }

      if (gexiv2_metadata_try_has_tag (GEXIV2_METADATA (metadata),
                                       "Exif.Canon.ColorSpace", NULL))
        {
          glong canon_cs;

          canon_cs = gexiv2_metadata_try_get_tag_long (GEXIV2_METADATA (metadata),
                                                       "Exif.Canon.ColorSpace", NULL);

          if (canon_cs == 0x01)
            {
              return GIMP_METADATA_COLORSPACE_SRGB;
            }
          else if (canon_cs == 0x02)
            {
              return GIMP_METADATA_COLORSPACE_ADOBERGB;
            }
        }

      if (exif_cs == 0xffff)
        return GIMP_METADATA_COLORSPACE_UNCALIBRATED;
    }

  return GIMP_METADATA_COLORSPACE_UNSPECIFIED;
}

/**
 * gimp_metadata_set_colorspace:
 * @metadata:   A #GimpMetadata instance.
 * @colorspace: The color space.
 *
 * Sets Exif.Photo.ColorSpace, Xmp.exif.ColorSpace,
 * Exif.Iop.InteroperabilityIndex, Exif.Nikon3.ColorSpace,
 * Exif.Canon.ColorSpace of @metadata.
 *
 * Since: 2.10
 */
void
gimp_metadata_set_colorspace (GimpMetadata           *metadata,
                              GimpMetadataColorspace  colorspace)
{
  GExiv2Metadata *g2metadata = GEXIV2_METADATA (metadata);

  switch (colorspace)
    {
    case GIMP_METADATA_COLORSPACE_UNSPECIFIED:
      gexiv2_metadata_try_clear_tag (g2metadata, "Exif.Photo.ColorSpace", NULL);
      gexiv2_metadata_try_clear_tag (g2metadata, "Xmp.exif.ColorSpace", NULL);
      gexiv2_metadata_try_clear_tag (g2metadata, "Exif.Iop.InteroperabilityIndex", NULL);
      gexiv2_metadata_try_clear_tag (g2metadata, "Exif.Nikon3.ColorSpace", NULL);
      gexiv2_metadata_try_clear_tag (g2metadata, "Exif.Canon.ColorSpace", NULL);
      break;

    case GIMP_METADATA_COLORSPACE_UNCALIBRATED:
      gexiv2_metadata_try_set_tag_long (g2metadata, "Exif.Photo.ColorSpace", 0xffff, NULL);
      if (gexiv2_metadata_try_has_tag (g2metadata, "Xmp.exif.ColorSpace", NULL))
        gexiv2_metadata_try_set_tag_long (g2metadata, "Xmp.exif.ColorSpace", 0xffff, NULL);
      gexiv2_metadata_try_clear_tag (g2metadata, "Exif.Iop.InteroperabilityIndex", NULL);
      gexiv2_metadata_try_clear_tag (g2metadata, "Exif.Nikon3.ColorSpace", NULL);
      gexiv2_metadata_try_clear_tag (g2metadata, "Exif.Canon.ColorSpace", NULL);
      break;

    case GIMP_METADATA_COLORSPACE_SRGB:
      gexiv2_metadata_try_set_tag_long (g2metadata, "Exif.Photo.ColorSpace", 0x01, NULL);

      if (gexiv2_metadata_try_has_tag (g2metadata, "Xmp.exif.ColorSpace", NULL))
        gexiv2_metadata_try_set_tag_long (g2metadata, "Xmp.exif.ColorSpace", 0x01, NULL);

      if (gexiv2_metadata_try_has_tag (g2metadata, "Exif.Iop.InteroperabilityIndex", NULL))
        gexiv2_metadata_try_set_tag_string (g2metadata,
                                            "Exif.Iop.InteroperabilityIndex",
                                            "R98", NULL);

      if (gexiv2_metadata_try_has_tag (g2metadata, "Exif.Nikon3.ColorSpace", NULL))
        gexiv2_metadata_try_set_tag_long (g2metadata, "Exif.Nikon3.ColorSpace", 0x01, NULL);

      if (gexiv2_metadata_try_has_tag (g2metadata, "Exif.Canon.ColorSpace", NULL))
        gexiv2_metadata_try_set_tag_long (g2metadata, "Exif.Canon.ColorSpace", 0x01, NULL);
      break;

    case GIMP_METADATA_COLORSPACE_ADOBERGB:
      gexiv2_metadata_try_set_tag_long (g2metadata, "Exif.Photo.ColorSpace", 0x02, NULL);

      if (gexiv2_metadata_try_has_tag (g2metadata, "Xmp.exif.ColorSpace", NULL))
        gexiv2_metadata_try_set_tag_long (g2metadata, "Xmp.exif.ColorSpace", 0x02, NULL);

      if (gexiv2_metadata_try_has_tag (g2metadata, "Exif.Iop.InteroperabilityIndex", NULL))
        gexiv2_metadata_try_set_tag_string (g2metadata,
                                            "Exif.Iop.InteroperabilityIndex",
                                            "R03", NULL);

      if (gexiv2_metadata_try_has_tag (g2metadata, "Exif.Nikon3.ColorSpace", NULL))
        gexiv2_metadata_try_set_tag_long (g2metadata, "Exif.Nikon3.ColorSpace", 0x02, NULL);

      if (gexiv2_metadata_try_has_tag (g2metadata, "Exif.Canon.ColorSpace", NULL))
        gexiv2_metadata_try_set_tag_long (g2metadata, "Exif.Canon.ColorSpace", 0x02, NULL);
      break;
    }
}

/**
 * gimp_metadata_is_tag_supported:
 * @tag:       A metadata tag name
 * @mime_type: A mime type
 *
 * Returns whether @tag is supported in a file of type @mime_type.
 *
 * Returns: %TRUE if the @tag supported with @mime_type, %FALSE otherwise.
 *
 * Since: 2.10
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

static GQuark
gimp_metadata_error_quark (void)
{
  static GQuark quark = 0;

  if (G_UNLIKELY (quark == 0))
    quark = g_quark_from_static_string ("gimp-metadata-error-quark");

  return quark;
}

static void
gimp_metadata_copy_tag (GExiv2Metadata *src,
                        GExiv2Metadata *dest,
                        const gchar    *tag)
{
  gchar  **values;
  GError  *error = NULL;

  values = gexiv2_metadata_try_get_tag_multiple (src, tag, &error);

  if (error)
    {
      g_printerr ("%s: %s\n", G_STRFUNC, error->message);
      g_clear_error (&error);
      g_strfreev (values);
    }
  else if (values)
    {
      gexiv2_metadata_try_set_tag_multiple (dest, tag, (const gchar **) values, &error);
      if (error)
        {
          g_warning ("%s: failed to set multiple metadata '%s': %s\n",
                     G_STRFUNC, tag, error->message);
          g_clear_error (&error);
        }

      g_strfreev (values);
    }
  else
    {
      gchar *value = gexiv2_metadata_try_get_tag_string (src, tag, &error);

      if (value)
        {
          gexiv2_metadata_try_set_tag_string (dest, tag, value, &error);
          if (error)
            {
              g_warning ("%s: failed to set metadata '%s': %s\n",
                         G_STRFUNC, tag, error->message);
              g_clear_error (&error);
            }
          g_free (value);
        }
      else if (error)
        {
          g_warning ("%s: failed to get metadata '%s': %s\n",
                     G_STRFUNC, tag, error->message);
          g_clear_error (&error);
        }
    }
}

static void
gimp_metadata_copy_tags (GExiv2Metadata  *src,
                         GExiv2Metadata  *dest,
                         const gchar    **tags)
{
  gint i;

  for (i = 0; tags[i] != NULL; i++)
    {
      /* don't copy the same tag multiple times */
      if (i > 0 && ! strcmp (tags[i], tags[i - 1]))
        continue;

      gimp_metadata_copy_tag (src, dest, tags[i]);
    }
 }

static void
gimp_metadata_add (GimpMetadata *src,
                   GimpMetadata *dest)
{
  GExiv2Metadata *g2src  = GEXIV2_METADATA (src);
  GExiv2Metadata *g2dest = GEXIV2_METADATA (dest);

  if (gexiv2_metadata_get_supports_exif (g2src) &&
      gexiv2_metadata_get_supports_exif (g2dest))
    {
      gchar **exif_tags = gexiv2_metadata_get_exif_tags (g2src);

      if (exif_tags)
        {
          gimp_metadata_copy_tags (g2src, g2dest,
                                   (const gchar **) exif_tags);
          g_strfreev (exif_tags);
        }
    }

  if (gexiv2_metadata_get_supports_xmp (g2src) &&
      gexiv2_metadata_get_supports_xmp (g2dest))
    {
      gchar **xmp_tags = gexiv2_metadata_get_xmp_tags (g2src);

      if (xmp_tags)
        {
          gimp_metadata_copy_tags (g2src, g2dest,
                                   (const gchar **) xmp_tags);
          g_strfreev (xmp_tags);
        }
    }

  if (gexiv2_metadata_get_supports_iptc (g2src) &&
      gexiv2_metadata_get_supports_iptc (g2dest))
    {
      gchar **iptc_tags = gexiv2_metadata_get_iptc_tags (g2src);

      if (iptc_tags)
        {
          gimp_metadata_copy_tags (g2src, g2dest,
                                   (const gchar **) iptc_tags);
          g_strfreev (iptc_tags);
        }
    }
}
