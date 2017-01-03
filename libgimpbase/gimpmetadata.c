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
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>

#include <gio/gio.h>
#include <gexiv2/gexiv2.h>

#include "libgimpmath/gimpmath.h"

#include "gimprational.h"
#include "gimpattribute.h"
#include "gimpbasetypes.h"

#include "gimplimits.h"
#include "gimpmetadata.h"
#include "gimpunit.h"

#include "libgimp/libgimp-intl.h"

typedef struct _GimpMetadataClass GimpMetadataClass;
typedef struct _GimpMetadataPrivate GimpMetadataPrivate;

struct _GimpMetadataPrivate {
  GHashTable      *attribute_table;
  GHashTable      *sorted_to_attribute;
  GList           *sorted_key_list;
  GSList          *xmp_structure_list;

};

struct _GimpMetadata {
  GExiv2Metadata       parent_instance;
  GimpMetadataPrivate *priv;
};

struct _GimpMetadataClass {
  GExiv2MetadataClass parent_class;
};

static gpointer              gimpmetadata_parent_class    = NULL;
static GimpAttribute        *current_attribute            = NULL;
static gboolean              iter_initialized             = FALSE;

static void                  gimp_metadata_finalize                    (GObject              *obj);
static void                  gimp_metadata_get_property                (GObject              *object,
                                                                        guint                 property_id,
                                                                        GValue               *value,
                                                                        GParamSpec           *pspec);
static void                  gimp_metadata_set_property                (GObject              *object,
                                                                        guint                 property_id,
                                                                        const GValue         *value,
                                                                        GParamSpec           *pspec);
static GimpAttribute *       gimp_metadata_get_attribute_sorted        (GimpMetadata         *metadata,
                                                                        const gchar          *sorted_name);
static void                  gimp_metadata_deserialize_error           (GMarkupParseContext  *context,
                                                                        GError               *error,
                                                                        gpointer              user_data);
static GQuark                gimp_metadata_error_quark                 (void);
static void                  gimp_metadata_deserialize_start_element   (GMarkupParseContext  *context,
                                                                        const gchar          *element_name,
                                                                        const gchar         **attribute_names,
                                                                        const gchar         **attribute_values,
                                                                        gpointer              user_data,
                                                                        GError              **error);
static void                  gimp_metadata_deserialize_text            (GMarkupParseContext  *context,
                                                                        const gchar          *text,
                                                                        gsize                 text_len,
                                                                        gpointer              user_data,
                                                                        GError              **error);
static void                  gimp_metadata_deserialize_end_element     (GMarkupParseContext  *context,
                                                                        const gchar          *element_name,
                                                                        gpointer              user_data,
                                                                        GError              **error);
static const gchar*          gimp_metadata_name_to_value               (const gchar         **attribute_names,
                                                                        const gchar         **attribute_values,
                                                                        const gchar          *name);
static gboolean              has_xmp_structure                         (GSList               *xmp_list,
                                                                        const gchar          *entry);
GExiv2Metadata *             gimp_metadata_new_gexiv2metadata          (void);
GimpAttribute *              gimp_metadata_from_parent                 (GimpMetadata         *metadata,
                                                                        const gchar          *name);
void                         gimp_metadata_to_parent                   (GimpMetadata         *metadata,
                                                                        GimpAttribute        *attribute);
void                         gimp_metadata_add_attribute_to_list       (GimpMetadata         *metadata,
                                                                        GimpAttribute        *attribute);
void                         gimp_metadata_from_gexiv2                 (GimpMetadata         *metadata);
void                         gimp_metadata_to_gexiv2                   (GimpMetadata         *metadata);

G_DEFINE_TYPE (GimpMetadata, gimp_metadata, GEXIV2_TYPE_METADATA)

#define parent_class gimp_metadata_parent_class

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
  "Exif.Iop"};

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

typedef struct
{
  gchar                     name[1024];
  gboolean                  base64;
  GimpAttributeValueType    type;
  GimpMetadata             *metadata;
} GimpMetadataParseData;

struct Namespaces{
  const gchar              *namespace_name;
  const gchar              *namespace_URI;
};

struct Namespaces namespaces_table[] = {
  {"gimp",     "http://www.gimp.org/ns/2.10/"                        },
  {"dwc",      "http://rs.tdwg.org/dwc/terms/"                       },
  {"lr",       "http://ns.adobe.com/lr/1.0/"                         },
  {"gpano",    "http://ns.google.com/photos/1.0/panorama/"           },
  {"panorama", "http://ns.adobe.com/photoshop/1.0/panorama-profile/" }

};


static void gimp_metadata_class_init (GimpMetadataClass * klass)
{
  gimpmetadata_parent_class = g_type_class_peek_parent (klass);
  g_type_class_add_private (klass, sizeof(GimpMetadataPrivate));

  G_OBJECT_CLASS (klass)->get_property = gimp_metadata_get_property;
  G_OBJECT_CLASS (klass)->set_property = gimp_metadata_set_property;
  G_OBJECT_CLASS (klass)->finalize = gimp_metadata_finalize;
}

static void gimp_metadata_init (GimpMetadata * self)
{
  self->priv = GIMP_METADATA_GET_PRIVATE (self);
  self->priv->attribute_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
  self->priv->sorted_to_attribute = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  self->priv->sorted_key_list = NULL;
  self->priv->xmp_structure_list = NULL;
}

/**
 * gimp_metadata_finalize:
 *
 * instance finalizer
 */
static void
gimp_metadata_finalize (GObject* obj)
{
  GimpMetadata * metadata;

  metadata = G_TYPE_CHECK_INSTANCE_CAST (obj, TYPE_GIMP_METADATA, GimpMetadata);

  g_hash_table_unref (metadata->priv->attribute_table);
  g_hash_table_unref (metadata->priv->sorted_to_attribute);
  g_list_free_full   (metadata->priv->sorted_key_list, (GDestroyNotify) g_free);
  g_slist_free (metadata->priv->xmp_structure_list);

  G_OBJECT_CLASS (gimpmetadata_parent_class)->finalize (obj);
}

/**
 * gimp_metadata_get_property
 *
 * instance get property
 */
static void
gimp_metadata_get_property (GObject * object,
                            guint property_id,
                            GValue * value,
                            GParamSpec * pspec)
{
}

/**
 * gimp_metadata_set_property
 *
 * instance set property
 */
static void
gimp_metadata_set_property (GObject * object,
                            guint property_id,
                            const GValue * value,
                            GParamSpec * pspec)
{
}

/**
 * gimp_metadata_new:
 *
 * returns a new #GimpMetadata object
 *
 * Return value:  a new @GimpMetadata object
 *
 * Since : 2.10
 */
GimpMetadata *
gimp_metadata_new (void)
{
  GimpMetadata *new_metadata = NULL;
  gint          i;

  if (gexiv2_initialize ())
    {
      new_metadata = g_object_new (TYPE_GIMP_METADATA, NULL);

      if (gexiv2_initialize ())
        {
          if (gexiv2_metadata_open_buf (GEXIV2_METADATA (new_metadata), wilber_jpg, wilber_jpg_len,
                                          NULL))
            {
              for (i = 0; i < G_N_ELEMENTS (namespaces_table); i++)
                {
                  struct Namespaces n_space = namespaces_table[i];
                  gexiv2_metadata_register_xmp_namespace (n_space.namespace_URI, n_space.namespace_name);
                }
            }

        }

      return new_metadata;

    }
  else
    {
      return NULL;
    }
}

/**
 * gimp_metadata_size: ToDo handle size
 *
 * @metadata: a #GimpMetadata
 *
 * Return value:  a #gint: amount of #GimpAttribute objects in
 * the #GimpMetadata container
 *
 * Since : 2.10
 */
gint
gimp_metadata_size (GimpMetadata *metadata)
{
  GimpMetadataPrivate *private = GIMP_METADATA_GET_PRIVATE (metadata);

  return g_hash_table_size (private->attribute_table);
}

/**
 * gimp_metadata_duplicate:
 *
 * @metadata: a #GimpMetadata
 *
 * Duplicates the #GimpMetadata object with all the #GimpAttribute objects
 * Return value:  a copy of the @metadata object
 *
 * Since : 2.10
 */
GimpMetadata *
gimp_metadata_duplicate (GimpMetadata *metadata)
{
  GimpMetadata * new_metadata = NULL;

  g_return_val_if_fail (IS_GIMP_METADATA (metadata), NULL);

  if (metadata)
    {
      gchar *xml;

      xml = gimp_metadata_serialize (metadata);
      new_metadata = gimp_metadata_deserialize (xml);
    }

  return new_metadata;
}

/**
 * gimp_metadata_get_table:
 *
 * @metadata: a #GimpMetadata
 *
 * Return value:  the #GHashTable
 *
 * Since : 2.10
 */
GHashTable *
gimp_metadata_get_table (GimpMetadata *metadata)
{
  return metadata->priv->attribute_table;
}

/**
 * gimp_metadata_add_attribute:
 *
 * @metadata  : a #GimpMetadata
 * @attribute : a #GimpAttribute
 *
 * stores the @attribute in the @metadata container
 *
 * Since : 2.10
 */
void
gimp_metadata_add_attribute (GimpMetadata        *metadata,
                             GimpAttribute       *attribute)
{
  const gchar *name;
  gchar       *lowchar;

  g_return_if_fail (IS_GIMP_METADATA (metadata));
  g_return_if_fail (GIMP_IS_ATTRIBUTE (attribute));

  name = gimp_attribute_get_name (attribute);

  if (name)
    {
        lowchar = g_ascii_strdown (name, -1);

      /* FIXME: really simply add? That means, that an older value is overwritten */

      if (g_hash_table_insert (metadata->priv->attribute_table, (gpointer) g_strdup (lowchar), (gpointer) attribute))
        {
          gchar *sortable_tag;

          sortable_tag = g_ascii_strdown (gimp_attribute_get_sortable_name (attribute), -1);

          if (g_hash_table_insert (metadata->priv->sorted_to_attribute, (gpointer) g_strdup (sortable_tag), (gpointer) g_strdup (lowchar)))
            {
              metadata->priv->sorted_key_list = g_list_insert_sorted (metadata->priv->sorted_key_list,
                                                                      (gpointer) g_strdup (sortable_tag),
                                                                      (GCompareFunc) g_strcmp0);
              gimp_metadata_to_parent (metadata, attribute);
            }
          else
            {
              g_hash_table_remove (metadata->priv->attribute_table, (gpointer) g_strdup (lowchar));
            }

          g_free (sortable_tag);
        }
      g_free (lowchar);
    }
}

/**
 * gimp_metadata_get_attribute:
 *
 * @metadata : a #GimpMetadata
 * @name     : a #gchar array
 *
 * gets the #GimpAttribute object with @name
 *
 * Return value: the #GimpAttribute object if found, NULL otherwise.
 *
 * Since : 2.10
 */
GimpAttribute *
gimp_metadata_get_attribute (GimpMetadata      *metadata,
                             const gchar       *name)
{
  GimpAttribute  *attribute_data = NULL;

  g_return_val_if_fail (IS_GIMP_METADATA (metadata), NULL);

  attribute_data = gimp_metadata_from_parent (metadata, name);

  return attribute_data;
}

/**
 * gimp_metadata_get_attribute_sorted:
 *
 * @metadata: a #GimpMetadata
 * @name : a #gchar array
 *
 * gets the #GimpAttribute object with @sorted_name as
 * sortable_name
 * see GimpAttribute::get_sortable_name
 *
 * Return value: the #GimpAttribute object if found, NULL otherwise.
 *
 * Since : 2.10
 */
static GimpAttribute *
gimp_metadata_get_attribute_sorted (GimpMetadata      *metadata,
                                    const gchar       *sorted_name)
{
  gchar          *lowchar;
  GimpAttribute  *attribute_data = NULL;
  gpointer       *data;
  gchar          *name_of_tag;

  g_return_val_if_fail (IS_GIMP_METADATA (metadata), NULL);
  g_return_val_if_fail (sorted_name != NULL, NULL);

  lowchar = g_ascii_strdown (sorted_name, -1);

  data = g_hash_table_lookup (metadata->priv->sorted_to_attribute, (gpointer) lowchar);

  if (data)
    {
      name_of_tag = (gchar *) data;

      data = g_hash_table_lookup (metadata->priv->attribute_table, (gpointer) name_of_tag);
      if (data)
        {
          attribute_data = (GimpAttribute *) data;
        }
    }

  g_free (lowchar);

  return attribute_data;
}

/**
 * gimp_metadata_remove_attribute: ToDo remove from gexiv2 ToDo rewrite
 *
 * @metadata : a #GimpMetadata
 * @name     : a #gchar array
 *
 * removes the #GimpAttribute object with @name from the @metadata container
 *
 * Return value: TRUE, if removing was successful, FALSE otherwise.
 *
 * Since : 2.10
 */
gboolean
gimp_metadata_remove_attribute (GimpMetadata      *metadata,
                                const gchar       *name)
{
  gchar            *lowchar;
  gboolean          success       = FALSE;
  GHashTableIter    iter_remove;
  gpointer          key, value;
  gchar            *tag_to_remove = NULL;
  gchar            *name_of_tag   = NULL;

  lowchar = g_ascii_strdown (name, -1);

  if (g_hash_table_remove (metadata->priv->attribute_table, (gpointer) lowchar))
    {
      gchar *tag_list_remove = NULL;

      g_hash_table_iter_init (&iter_remove, metadata->priv->sorted_to_attribute);
      while (g_hash_table_iter_next (&iter_remove, &key, &value))
        {
          tag_to_remove  = (gchar *) key;
          name_of_tag    = (gchar *) value;

          if (! g_strcmp0 (lowchar, name_of_tag))
            break;
        }

      tag_list_remove = g_strdup (tag_to_remove); /* because removing from hashtable frees tag_to_remove */

      if (g_hash_table_remove (metadata->priv->sorted_to_attribute, (gpointer) tag_to_remove))
        {
          GList *list = NULL;

          for (list = metadata->priv->sorted_key_list; list; list = list->next)
            {
              gchar *s_tag = (gchar *) list->data;

              if (! g_strcmp0 (s_tag, tag_list_remove))
                {
                  metadata->priv->sorted_key_list = g_list_remove (metadata->priv->sorted_key_list,
                                                                   (gconstpointer) s_tag);
                  g_free (s_tag);
                  break;
                }
            }
          g_free (tag_list_remove);
          success = TRUE;
        }
      else
        {
          if (name_of_tag)
            g_free (name_of_tag);
          if (tag_to_remove)
            g_free (tag_to_remove);
          success = FALSE;
        }
    }
  g_free (lowchar);

  gexiv2_metadata_clear_tag (GEXIV2_METADATA (metadata), name);

  return success;
}

/**
 * gimp_metadata_has_attribute:
 *
 * @metadata : a #GimpMetadata
 * @name     : a #gchar array
 *
 * tests, if a #GimpAttribute object with @name is in the @metadata container
 *
 * Return value: TRUE if yes, FALSE otherwise.
 *
 * Since : 2.10
 */
gboolean
gimp_metadata_has_attribute (GimpMetadata      *metadata,
                             const gchar         *name)
{
  gchar *lowchar;
  gboolean success;

  lowchar = g_ascii_strdown (name, -1);

  success = gexiv2_metadata_has_tag (GEXIV2_METADATA(metadata), name);

  g_free (lowchar);

  return success;
}

/**
 * gimp_metadata_new_attribute:
 *
 * @metadata: a #GimpMetadata
 * @name    : a #gchar array
 * @value   : a #gchar array
 * @type    : a #GimpAttributeValueType
 *
 * adds a #GimpAttribute object to @metadata container.
 * The #GimpAttribute object is created from the
 * @name,
 * @value and
 * @type parameters.
 *
 * Return value: TRUE if successful, FALSE otherwise.
 *
 * Since : 2.10
 */
gboolean
gimp_metadata_new_attribute (GimpMetadata *metadata,
                             const gchar    *name,
                             gchar          *value,
                             GimpAttributeValueType type)
{
  GimpAttribute *attribute;

  attribute = gimp_attribute_new_string (name, value, type);
  if (attribute)
    {
      gimp_metadata_add_attribute (metadata, attribute);
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_metadata_serialize: ToDo handle old metadata and get data from sorted list -> attributes.c
 *
 * @metadata: a #GimpMetadata
 *
 * creates a xml representation of all #GimpAttribute objects in the #GimpMetadata container.
 * see #GimpAttribute:gimp_attribute_get_xml
 *
 * Return value: a new #gchar array, the xml representation of the #GimpMetadata object.
 *
 * Since : 2.10
 */
gchar *
gimp_metadata_serialize (GimpMetadata *metadata)
{
  GString        *string;
  GList          *key_list;

  g_return_val_if_fail (IS_GIMP_METADATA (metadata), NULL);

  gimp_metadata_from_gexiv2 (metadata);

  string = g_string_new (NULL);

  g_string_append (string, "<?xml version='1.0' encoding='UTF-8'?>\n");
  g_string_append (string, "<metadata>\n");

  for (key_list = metadata->priv->sorted_key_list; key_list; key_list = key_list->next)
    {
      gchar          *xml       = NULL;
      GimpAttribute  *attribute = NULL;
      gchar          *p_key = (gchar *) key_list->data;

      attribute      = gimp_metadata_get_attribute_sorted (metadata, p_key);

      xml = gimp_attribute_get_xml (attribute);

      g_string_append_printf (string, "%s\n", xml);

      g_free (xml);
    }

  g_string_append (string, "</metadata>\n");

  return g_string_free (string, FALSE);
}

/**
 * gimp_metadata_deserialize:
 *
 * @xml: a #gchar array
 *
 * parses a xml representation of a #GimpMetadata container.
 * see
 * #GimpMetadata:gimp_metadata_deserialize_start_element
 * #GimpMetadata:gimp_metadata_deserialize_end_element
 * #GimpMetadata:gimp_metadata_deserialize_text
 * #GimpMetadata:gimp_metadata_deserialize_error
 *
 * Return value: a new #GimpMetadata object.
 *
 * Since : 2.10
 */
GimpMetadata *
gimp_metadata_deserialize (const gchar *xml)
{
  GMarkupParser            *markup_parser = g_slice_new (GMarkupParser);
  GimpMetadataParseData    *parse_data = g_slice_new (GimpMetadataParseData);
  GMarkupParseContext      *context;
  GimpMetadata             *metadata;

  g_return_val_if_fail (xml != NULL, NULL);

  metadata = gimp_metadata_new ();

  parse_data->metadata = metadata;

  markup_parser->start_element = gimp_metadata_deserialize_start_element;
  markup_parser->end_element   = gimp_metadata_deserialize_end_element;
  markup_parser->text          = gimp_metadata_deserialize_text;
  markup_parser->passthrough   = NULL;
  markup_parser->error         = gimp_metadata_deserialize_error;

  context = g_markup_parse_context_new (markup_parser, 0, parse_data, NULL);

  g_markup_parse_context_parse (context,
                                xml, strlen (xml),
                                NULL);

  g_markup_parse_context_unref (context);

  g_slice_free (GMarkupParser, markup_parser);
  g_slice_free (GimpMetadataParseData, parse_data);

 return metadata;
}

/**
 * gimp_metadata_from_gexiv2metadata:
 * @metadata:  The metadata the gexiv2metadata will be added to, may be %NULL
 * @gexiv2metadata:  The metadata in gexiv2 format
 *
 * Converts the @gexiv2metadata retrieved from a file into
 * a #GimpMetadata object
 *
 * Return value: The #GimpMetadata object
 *
 * Since: GIMP 2.10
 */
//GimpMetadata *
//gimp_metadata_from_gexiv2metadata (GimpMetadata   *metadata,
//                                   GimpMetadata   *gexivdata)
//{
//  const gchar               *tag_type;
//  GimpAttribute             *attribute;
//  GimpAttributeValueType     attrib_type;
//  gint                       i;
//  GimpMetadata              *new_metadata    = NULL;
//  GExiv2Metadata            *gexiv2metadata  = NULL;
//
//  gexiv2metadata = GEXIV2_METADATA(gexivdata);
//
//  if (!metadata)
//    {
//      new_metadata = gimp_metadata_new ();
//    }
//  else
//    {
//      new_metadata = gimp_metadata_duplicate (metadata);
//      g_object_unref (metadata);
//    }
//
//  if (new_metadata)
//    {
//      gchar    **exif_data;
//      gchar    **xmp_data;
//      gchar    **iptc_data;
//      gboolean   no_interpreted    = TRUE; /*FIXME: No interpreted String possible */
//
//      exif_data = gexiv2_metadata_get_exif_tags (gexiv2metadata);
//
//      for (i = 0; exif_data[i] != NULL; i++)
//        {
//          gchar    *interpreted_value = NULL;
//          gchar    *value             = NULL;
//          gboolean  interpreted       = FALSE;
//
//          value = gexiv2_metadata_get_tag_string (gexiv2metadata, exif_data[i]);
//          interpreted_value = gexiv2_metadata_get_tag_interpreted_string (gexiv2metadata, exif_data[i]);
//          tag_type = gexiv2_metadata_get_tag_type (exif_data[i]);
//          attrib_type = gimp_attribute_get_value_type_from_string (tag_type);
//
//          interpreted = g_strcmp0 (value, interpreted_value);
//
//          if (!interpreted)
//            {
//              gint length;
//
//              length = strlen (interpreted_value);
//              if (length > 2048)
//                {
//                  g_free (interpreted_value);
//                  interpreted_value = g_strdup_printf ("(Size of value: %d)", length);
//                  interpreted = TRUE;
//                }
//            }
//
//          attribute = gimp_attribute_new_string (exif_data[i], value, attrib_type);
//          if (gimp_attribute_is_valid (attribute))
//            {
//              if (no_interpreted)
//                {
//                  if (interpreted)
//                    {
//                      gimp_attribute_set_interpreted_string (attribute, interpreted_value);
//                    }
//                }
//              gimp_metadata_add_attribute (new_metadata, attribute);
//            }
//          else
//            {
//              g_object_unref (attribute);
//            }
//
//          g_free (interpreted_value);
//          g_free (value);
//        }
//
//      g_strfreev (exif_data);
//
//      xmp_data = gexiv2_metadata_get_xmp_tags (gexiv2metadata);
//
//      for (i = 0; xmp_data[i] != NULL; i++)
//        {
//          gchar    *interpreted_value = NULL;
//          gchar    *value             = NULL;
//          gboolean  interpreted       = FALSE;
//
//          value = gexiv2_metadata_get_tag_string (gexiv2metadata, xmp_data[i]);
//          interpreted_value = gexiv2_metadata_get_tag_interpreted_string (gexiv2metadata, xmp_data[i]);
//          tag_type = gexiv2_metadata_get_tag_type (xmp_data[i]);
//          attrib_type = gimp_attribute_get_value_type_from_string (tag_type);
//
//          interpreted = g_strcmp0 (value, interpreted_value);
//
//          attribute = gimp_attribute_new_string (xmp_data[i], value, attrib_type);
//
//          if (gimp_attribute_is_valid (attribute))
//            {
//              if (no_interpreted)
//                {
//                  if (interpreted)
//                    gimp_attribute_set_interpreted_string (attribute, interpreted_value);
//                }
//              gimp_metadata_add_attribute (new_metadata, attribute);
//            }
//          else
//            {
//              g_object_unref (attribute);
//            }
//
//          g_free (value);
//        }
//
//      g_strfreev (xmp_data);
//
//      iptc_data = gexiv2_metadata_get_iptc_tags (gexiv2metadata);
//
//      for (i = 0; iptc_data[i] != NULL; i++)
//        {
//          gchar    *interpreted_value = NULL;
//          gchar    *value             = NULL;
//          gboolean  interpreted       = FALSE;
//
//          value = gexiv2_metadata_get_tag_string (gexiv2metadata, iptc_data[i]);
//          interpreted_value = gexiv2_metadata_get_tag_interpreted_string (gexiv2metadata, iptc_data[i]);
//          tag_type = gexiv2_metadata_get_tag_type (iptc_data[i]);
//          attrib_type = gimp_attribute_get_value_type_from_string (tag_type);
//
//          interpreted = g_strcmp0 (value, interpreted_value);
//
//          attribute = gimp_attribute_new_string (iptc_data[i], value, attrib_type);
//          if (gimp_attribute_is_valid (attribute))
//            {
//              if (no_interpreted)
//                {
//                  if (interpreted)
//                    gimp_attribute_set_interpreted_string (attribute, interpreted_value);
//                }
//              gimp_metadata_add_attribute (new_metadata, attribute);
//            }
//          else
//            {
//              g_object_unref (attribute);
//            }
//
//          g_free (value);
//        }
//
//      g_strfreev (iptc_data);
//    }
//  return new_metadata;
//}

/**
 * gimp_metadata_from_gexiv2:
 * @metadata:  The metadata
 *
 * Constructs the @metadata retrieved from the gexiv2 package.
 *
 *
 * Since: GIMP 2.10
 */
void
gimp_metadata_from_gexiv2 (GimpMetadata   *metadata)
{
  const gchar               *tag_type;
  GimpAttribute             *attribute;
  GimpAttributeValueType     attrib_type;
  gint                       i;
  GExiv2Metadata            *gexiv2metadata  = NULL;
  GimpMetadataPrivate       *priv;

  g_return_if_fail (IS_GIMP_METADATA (metadata));

  priv = GIMP_METADATA_GET_PRIVATE (metadata);

  if (priv->attribute_table)
    g_hash_table_unref (priv->attribute_table);
  if (priv->sorted_to_attribute)
    g_hash_table_unref (priv->sorted_to_attribute);
  if (priv->sorted_key_list)
    g_list_free_full   (priv->sorted_key_list, (GDestroyNotify) g_free);
  if (priv->xmp_structure_list)
    g_slist_free (priv->xmp_structure_list);


  priv->attribute_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
  priv->sorted_to_attribute = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  priv->sorted_key_list = NULL;
  priv->xmp_structure_list = NULL;

  gexiv2metadata = GEXIV2_METADATA(metadata);

    {
      gchar    **exif_data;
      gchar    **xmp_data;
      gchar    **iptc_data;
      gboolean   no_interpreted    = TRUE; /*FIXME: No interpreted String possible */

      exif_data = gexiv2_metadata_get_exif_tags (gexiv2metadata);

      for (i = 0; exif_data[i] != NULL; i++)
        {
          gchar    *interpreted_value = NULL;
          gchar    *value             = NULL;
          gboolean  interpreted       = FALSE;

          value = gexiv2_metadata_get_tag_string (gexiv2metadata, exif_data[i]);
          interpreted_value = gexiv2_metadata_get_tag_interpreted_string (gexiv2metadata, exif_data[i]);
          tag_type = gexiv2_metadata_get_tag_type (exif_data[i]);
          attrib_type = gimp_attribute_get_value_type_from_string (tag_type);

          interpreted = g_strcmp0 (value, interpreted_value);

          if (!interpreted)
            {
              gint length;

              length = strlen (interpreted_value);
              if (length > 2048)
                {
                  g_free (interpreted_value);
                  interpreted_value = g_strdup_printf ("(Size of value: %d)", length);
                  interpreted = TRUE;
                }
            }

          attribute = gimp_attribute_new_string (exif_data[i], value, attrib_type);
          if (gimp_attribute_is_valid (attribute))
            {
              if (no_interpreted)
                {
                  if (interpreted)
                    {
                      gimp_attribute_set_interpreted_string (attribute, interpreted_value);
                    }
                }
              gimp_metadata_add_attribute_to_list (metadata, attribute);
            }
          else
            {
              g_object_unref (attribute);
            }

          g_free (interpreted_value);
          g_free (value);
        }

      g_strfreev (exif_data);

      xmp_data = gexiv2_metadata_get_xmp_tags (gexiv2metadata);

      for (i = 0; xmp_data[i] != NULL; i++)
        {
          gchar    *interpreted_value = NULL;
          gchar    *value             = NULL;
          gboolean  interpreted       = FALSE;

          value = gexiv2_metadata_get_tag_string (gexiv2metadata, xmp_data[i]);
          interpreted_value = gexiv2_metadata_get_tag_interpreted_string (gexiv2metadata, xmp_data[i]);
          tag_type = gexiv2_metadata_get_tag_type (xmp_data[i]);
          attrib_type = gimp_attribute_get_value_type_from_string (tag_type);

          interpreted = g_strcmp0 (value, interpreted_value);

          attribute = gimp_attribute_new_string (xmp_data[i], value, attrib_type);

          if (gimp_attribute_is_valid (attribute))
            {
              if (no_interpreted)
                {
                  if (interpreted)
                    gimp_attribute_set_interpreted_string (attribute, interpreted_value);
                }
              gimp_metadata_add_attribute_to_list (metadata, attribute);
            }
          else
            {
              g_object_unref (attribute);
            }

          g_free (value);
        }

      g_strfreev (xmp_data);

      iptc_data = gexiv2_metadata_get_iptc_tags (gexiv2metadata);

      for (i = 0; iptc_data[i] != NULL; i++)
        {
          gchar    *interpreted_value = NULL;
          gchar    *value             = NULL;
          gboolean  interpreted       = FALSE;

          value = gexiv2_metadata_get_tag_string (gexiv2metadata, iptc_data[i]);
          interpreted_value = gexiv2_metadata_get_tag_interpreted_string (gexiv2metadata, iptc_data[i]);
          tag_type = gexiv2_metadata_get_tag_type (iptc_data[i]);
          attrib_type = gimp_attribute_get_value_type_from_string (tag_type);

          interpreted = g_strcmp0 (value, interpreted_value);

          attribute = gimp_attribute_new_string (iptc_data[i], value, attrib_type);
          if (gimp_attribute_is_valid (attribute))
            {
              if (no_interpreted)
                {
                  if (interpreted)
                    gimp_attribute_set_interpreted_string (attribute, interpreted_value);
                }
              gimp_metadata_add_attribute (metadata, attribute);
            }
          else
            {
              g_object_unref (attribute);
            }

          g_free (value);
        }

      g_strfreev (iptc_data);
    }
}

/**
 * gimp_metadata_add_list:
 *
 * @metadata  : a #GimpMetadata
 * @attribute : a #GimpAttribute
 *
 * stores the @attribute in the @metadata container
 *
 * Since : 2.10
 */
void
gimp_metadata_add_attribute_to_list (GimpMetadata    *metadata,
                                     GimpAttribute   *attribute)
{
  const gchar         *name;
  gchar               *lowchar;
  GimpMetadataPrivate *priv;

  g_return_if_fail (IS_GIMP_METADATA (metadata));
  g_return_if_fail (GIMP_IS_ATTRIBUTE (attribute));

  priv = GIMP_METADATA_GET_PRIVATE (metadata);

  name = gimp_attribute_get_name (attribute);

  if (name)
    {
        lowchar = g_ascii_strdown (name, -1);

      /* FIXME: really simply add? That means, that an older value is overwritten */

      if (g_hash_table_insert (priv->attribute_table, (gpointer) g_strdup (lowchar), (gpointer) attribute))
        {
          gchar *sortable_tag;

          sortable_tag = g_ascii_strdown (gimp_attribute_get_sortable_name (attribute), -1);

          if (g_hash_table_insert (priv->sorted_to_attribute, (gpointer) g_strdup (sortable_tag), (gpointer) g_strdup (lowchar)))
            {
              priv->sorted_key_list = g_list_insert_sorted (priv->sorted_key_list,
                                                            (gpointer) g_strdup (sortable_tag),
                                                            (GCompareFunc) g_strcmp0);
            }
          else
            {
              g_hash_table_remove (priv->attribute_table, (gpointer) g_strdup (lowchar));
            }

          g_free (sortable_tag);
        }
      g_free (lowchar);
    }
}

/**
 * gimp_metadata_from_parent:
 * @metadata:  The metadata the gexiv2metadata will be added to, may be %NULL
 * @gexiv2metadata:  The metadata in gexiv2 format
 *
 * Converts the @gexiv2metadata retrieved from a file into
 * a #GimpMetadata object
 *
 * Return value: The #GimpMetadata object
 *
 * Since: GIMP 2.10
 */
GimpAttribute *
gimp_metadata_from_parent (GimpMetadata      *metadata,
                           const gchar       *name)
{
  const gchar               *tag_type;
  gboolean                   no_interpreted    = TRUE; /*FIXME: No interpreted String possible */
  gchar                     *interpreted_value = NULL;
  gchar                     *value             = NULL;
  gboolean                   interpreted       = FALSE;
  GimpAttribute             *attribute         = NULL;
  GimpAttributeValueType     attrib_type;
  GExiv2Metadata            *gexiv2metadata;

  g_return_val_if_fail (IS_GIMP_METADATA (metadata), NULL);

  gexiv2metadata = GEXIV2_METADATA (metadata);

  if (gexiv2_metadata_has_tag (gexiv2metadata, name))
    {
      value = gexiv2_metadata_get_tag_string (gexiv2metadata, name);
      interpreted_value = gexiv2_metadata_get_tag_interpreted_string (gexiv2metadata, name);
      tag_type = gexiv2_metadata_get_tag_type (name);
      attrib_type = gimp_attribute_get_value_type_from_string (tag_type);

      interpreted = g_strcmp0 (value, interpreted_value);

      if (!interpreted)
        {
          gint length;

          length = strlen (interpreted_value);
          if (length > 2048)
            {
              g_free (interpreted_value);
              interpreted_value = g_strdup_printf ("(Size of value: %d)", length);
              interpreted = TRUE;
            }
        }

      attribute = gimp_attribute_new_string (name, value, attrib_type);
      if (gimp_attribute_is_valid (attribute))
        {
          if (no_interpreted)
            {
              if (interpreted)
                {
                  gimp_attribute_set_interpreted_string (attribute, interpreted_value);
                }
            }
        }
      else
        {
          g_object_unref (attribute);
        }

      g_free (interpreted_value);
      g_free (value);
    }
  return attribute;
}

/**
 * gimp_metadata_print:
 * @metadata:  The #GimpMetadata
 *
 * prints out information of metadata
 *
 * Since: GIMP 2.10
 */
void
gimp_metadata_print (GimpMetadata *metadata)
{
  gint            i;
  GList          *key_list = NULL;

  g_return_if_fail (IS_GIMP_METADATA (metadata));

  gimp_metadata_from_gexiv2 (metadata);

  i = 0;

  for (key_list = metadata->priv->sorted_key_list; key_list; key_list = key_list->next)
    {
      const gchar    *tag;
      const gchar    *interpreted;
      gchar          *value;
      GimpAttribute  *attribute;
      gchar          *p_key = (gchar *) key_list->data;

      attribute      = gimp_metadata_get_attribute_sorted (metadata, p_key);

      if (! attribute)
        continue;

      i++;

      tag = gimp_attribute_get_name (attribute);
      value = gimp_attribute_get_string (attribute);
      interpreted = gimp_attribute_get_interpreted_string (attribute);

      g_print ("%p: %s\n%04d. Tag: %s\n\tValue:%s\n\tInterpreted value:%s\n", attribute, p_key, i, tag, value, interpreted);

      if (value)
        g_free (value);
    }
}

/**
 * gimp_metadata_to_gexiv2metadata:
 * @metadata:  The #GimpMetadata
 * @gexiv2metadata:  The #GimpMetadata
 * @mime_type: the mime type of the image
 *
 * Converts @metadata to @gexiv2metadata in gexiv2 format
 *
 * Since: GIMP 2.10
 */
//void
//gimp_metadata_to_gexiv2metadata (GimpMetadata   *metadata,
//                                 GimpMetadata   *gexivdata,
//                                 const gchar    *mime_type)
//{
//  gchar          *o_packet            = NULL;
//  gboolean        write_tag           = FALSE;
//  gboolean        namespace           = FALSE;
//  gboolean        check_mime          = TRUE;
//  gboolean        support_exif;
//  gboolean        support_xmp;
//  gboolean        support_iptc;
//  GSList         *xmp_structure_list  = NULL;
//  GList          *key_list            = NULL;
//  gint            i;
//  GExiv2Metadata *gexiv2metadata;
//
//  g_return_if_fail (IS_GIMP_METADATA (metadata));
//
//  gexiv2metadata = GEXIV2_METADATA(gexivdata);
//
//  for (i = 0; i < G_N_ELEMENTS (namespaces_table); i++)
//    {
//      struct Namespaces n_space = namespaces_table[i];
//      gexiv2_metadata_register_xmp_namespace (n_space.namespace_URI, n_space.namespace_name);
//    }
//
//  support_exif = gexiv2_metadata_get_supports_exif (gexiv2metadata);
//  support_xmp  = gexiv2_metadata_get_supports_xmp  (gexiv2metadata);
//  support_iptc = gexiv2_metadata_get_supports_iptc (gexiv2metadata);
//
//  for (key_list = metadata->priv->sorted_key_list; key_list; key_list = key_list->next)
//    {
//      const gchar              *tag;
//      const gchar              *ns_name;
//      gchar                    *p_key = (gchar *) key_list->data;
//      gchar                    *value         = NULL;
//      gchar                    *category      = NULL;
//      gboolean                  is_xmp        = FALSE;
//      gboolean                  has_structure = FALSE;
//      GimpAttribute            *attribute;
//      GimpAttributeValueType    tag_value_type;
//
//      write_tag      = FALSE;
//      namespace      = FALSE;
//
//      attribute      = gimp_metadata_get_attribute_sorted (metadata, p_key);
//
//      if (! attribute)
//        continue;
//
//      tag            = gimp_attribute_get_name (attribute);
//      has_structure  = gimp_attribute_has_structure (attribute);
//
//      if (mime_type)
//        check_mime = gimp_metadata_is_tag_supported (tag, mime_type);
//
//      if (check_mime)
//        {
//          gchar *t_packet       = NULL;
//
//          tag_value_type = gimp_attribute_get_value_type (attribute);
//          value = gimp_attribute_get_string (attribute);
//          category = g_ascii_strdown (gimp_attribute_get_attribute_type (attribute), -1);
//
//          if (tag && value && category)
//            {
//              if(! g_strcmp0 (category, "exif") && support_exif)
//                {
//                  write_tag = TRUE;
//                }
//              else if(! g_strcmp0 (category, "xmp") && support_xmp)
//                {
//                  write_tag = TRUE;
//                  is_xmp    = TRUE;
//
//                  namespace = gimp_attribute_is_new_namespace (attribute);
//
//                  if (namespace)
//                    {
//                      write_tag = FALSE;
//
//                      ns_name = gimp_attribute_get_attribute_ifd (attribute);
//
//                      for (i = 0; i < G_N_ELEMENTS (namespaces_table); i++)
//                        {
//                          struct Namespaces n_space = namespaces_table[i];
//                          if(! g_strcmp0 (ns_name, n_space.namespace_name))
//                            {
//                              write_tag = TRUE;
//                              break;
//                            }
//                        }
//                    }
//
//                  if (write_tag && has_structure)
//                    {
//                      gboolean                    success = TRUE;
//                      GSList                     *structure;
//                      GSList                     *list;
//                      GimpAttributeStructureType  structure_type;
//
//                      structure      = gimp_attribute_get_attribute_structure (attribute);
//                      structure_type = gimp_attribute_get_structure_type (attribute);
//
//                      for (list = structure; list; list = list->next)
//                        {
//                          const gchar   *structure_element = (const gchar*) list->data;
//                          gboolean       has_tag = gexiv2_metadata_has_tag (gexiv2metadata, structure_element);
//
//                          if (!has_tag && ! has_xmp_structure (xmp_structure_list, structure_element))
//                            {
//                              switch (structure_type)
//                              {
//                                case STRUCTURE_TYPE_ALT:
//                                  success = gexiv2_metadata_set_xmp_tag_struct (gexiv2metadata, structure_element, GEXIV2_STRUCTURE_XA_ALT); /*start block*/
//                                  break;
//                                case STRUCTURE_TYPE_BAG:
//                                  success = gexiv2_metadata_set_xmp_tag_struct (gexiv2metadata, structure_element, GEXIV2_STRUCTURE_XA_BAG); /*start block*/
//                                  break;
//                                case STRUCTURE_TYPE_SEQ:
//                                  success = gexiv2_metadata_set_xmp_tag_struct (gexiv2metadata, structure_element, GEXIV2_STRUCTURE_XA_SEQ); /*start block*/
//                                  break;
//                                default:
//                                  success = FALSE;
//                                  break;
//                              }
//
//                              if (success)
//                                xmp_structure_list = g_slist_prepend (xmp_structure_list, (gpointer)structure_element);
//                            }
//                        }
//                    }
//                }
//              else if(! g_strcmp0 (category, "iptc") && support_iptc)
//                {
//                  write_tag = TRUE;
//                }
//              else
//                {
//                  write_tag = FALSE;
//                }
//
//              if (write_tag)
//                {
//                  switch (tag_value_type)
//                  {
//                    case TYPE_INVALID:
//                      break;
//                    case TYPE_LONG:
//                    case TYPE_SLONG:
//                    case TYPE_FLOAT:
//                    case TYPE_DOUBLE:
//                    case TYPE_SHORT:
//                    case TYPE_SSHORT:
//                    case TYPE_DATE:
//                    case TYPE_TIME:
//                    case TYPE_ASCII:
//                    case TYPE_UNICODE:
//                    case TYPE_BYTE:
//                    case TYPE_RATIONAL:
//                    case TYPE_SRATIONAL:
//                      {
//                        gexiv2_metadata_set_tag_string (gexiv2metadata, tag, value);
//                      }
//                      break;
//                    case TYPE_MULTIPLE:
//                      {
//                        GValue h;
//                        gchar **values;
//
//                        h = gimp_attribute_get_value (attribute);
//                        values = (gchar **) g_value_get_boxed (&h);
//                        gexiv2_metadata_set_tag_multiple (gexiv2metadata, tag, (const gchar **) values);
//                        g_strfreev  (values);
//                      }
//                      break;
//                    case TYPE_UNKNOWN:
//                    default:
//                      break;
//
//                  }
//
//                  if (is_xmp)
//                    {
//                      t_packet = gexiv2_metadata_generate_xmp_packet (gexiv2metadata, GEXIV2_USE_COMPACT_FORMAT | GEXIV2_OMIT_ALL_FORMATTING, 0);
//
//                      if (! g_strcmp0 (t_packet, o_packet))
//                        {
//                          gexiv2_metadata_clear_tag (gexiv2metadata, tag);
//                          g_print ("cleared to gexiv2metadata:\n%s, %s\n", tag, value);
//                        }
//                      else
//                        {
//                          o_packet = g_strdup (t_packet);
//                        }
//                    }
//                }
//            }
//
//          if (t_packet)
//            g_free (t_packet);
//          if (value)
//            g_free (value);
//          if (category)
//            g_free (category);
//       }
//    }
//
//  if (o_packet)
//    g_free (o_packet);
//  if (xmp_structure_list)
//    g_slist_free (xmp_structure_list);
//}

/**
 * gimp_metadata_to_gexiv2:
 * @metadata:  The #GimpMetadata
 *
 * Converts @metadata to @gexiv2metadata packet
 *
 * Since: GIMP 2.10
 */
void
gimp_metadata_to_gexiv2 (GimpMetadata   *metadata)
{
  gchar          *o_packet            = NULL;
  gboolean        write_tag           = FALSE;
  gboolean        namespace           = FALSE;
  gboolean        support_exif;
  gboolean        support_xmp;
  gboolean        support_iptc;
  GSList         *xmp_structure_list  = NULL;
  GList          *key_list            = NULL;
  gint            i;
  GExiv2Metadata *gexiv2metadata;

  g_return_if_fail (IS_GIMP_METADATA (metadata));

  gexiv2metadata = GEXIV2_METADATA(metadata);

  for (i = 0; i < G_N_ELEMENTS (namespaces_table); i++)
    {
      struct Namespaces n_space = namespaces_table[i];
      gexiv2_metadata_register_xmp_namespace (n_space.namespace_URI, n_space.namespace_name);
    }

  support_exif = gexiv2_metadata_get_supports_exif (gexiv2metadata);
  support_xmp  = gexiv2_metadata_get_supports_xmp  (gexiv2metadata);
  support_iptc = gexiv2_metadata_get_supports_iptc (gexiv2metadata);

  for (key_list = metadata->priv->sorted_key_list; key_list; key_list = key_list->next)
    {
      const gchar              *tag;
      const gchar              *ns_name;
      gchar                    *p_key = (gchar *) key_list->data;
      gchar                    *value         = NULL;
      gchar                    *category      = NULL;
      gboolean                  is_xmp        = FALSE;
      gboolean                  has_structure = FALSE;
      gchar                    *t_packet      = NULL;
      GimpAttribute            *attribute;
      GimpAttributeValueType    tag_value_type;

      write_tag      = FALSE;
      namespace      = FALSE;

      attribute      = gimp_metadata_get_attribute_sorted (metadata, p_key);

      if (! attribute)
        continue;

      tag            = gimp_attribute_get_name (attribute);
      has_structure  = gimp_attribute_has_structure (attribute);

      tag_value_type = gimp_attribute_get_value_type (attribute);
      value = gimp_attribute_get_string (attribute);
      category = g_ascii_strdown (gimp_attribute_get_attribute_type (attribute), -1);

      if (tag && value && category)
        {
          if(! g_strcmp0 (category, "exif") && support_exif)
            {
              write_tag = TRUE;
            }
          else if(! g_strcmp0 (category, "xmp") && support_xmp)
            {
              write_tag = TRUE;
              is_xmp    = TRUE;

              namespace = gimp_attribute_is_new_namespace (attribute);

              if (namespace)
                {
                  write_tag = FALSE;

                  ns_name = gimp_attribute_get_attribute_ifd (attribute);

                  for (i = 0; i < G_N_ELEMENTS (namespaces_table); i++)
                    {
                      struct Namespaces n_space = namespaces_table[i];
                      if(! g_strcmp0 (ns_name, n_space.namespace_name))
                        {
                          write_tag = TRUE;
                          break;
                        }
                    }
                }

              if (write_tag && has_structure)
                {
                  gboolean                    success = TRUE;
                  GSList                     *structure;
                  GSList                     *list;
                  GimpAttributeStructureType  structure_type;

                  structure      = gimp_attribute_get_attribute_structure (attribute);
                  structure_type = gimp_attribute_get_structure_type (attribute);

                  for (list = structure; list; list = list->next)
                    {
                      const gchar   *structure_element = (const gchar*) list->data;
                      gboolean       has_tag = gexiv2_metadata_has_tag (gexiv2metadata, structure_element);

                      if (!has_tag && ! has_xmp_structure (xmp_structure_list, structure_element))
                        {
                          switch (structure_type)
                          {
                            case STRUCTURE_TYPE_ALT:
                              success = gexiv2_metadata_set_xmp_tag_struct (gexiv2metadata, structure_element, GEXIV2_STRUCTURE_XA_ALT); /*start block*/
                              break;
                            case STRUCTURE_TYPE_BAG:
                              success = gexiv2_metadata_set_xmp_tag_struct (gexiv2metadata, structure_element, GEXIV2_STRUCTURE_XA_BAG); /*start block*/
                              break;
                            case STRUCTURE_TYPE_SEQ:
                              success = gexiv2_metadata_set_xmp_tag_struct (gexiv2metadata, structure_element, GEXIV2_STRUCTURE_XA_SEQ); /*start block*/
                              break;
                            default:
                              success = FALSE;
                              break;
                          }

                          if (success)
                            xmp_structure_list = g_slist_prepend (xmp_structure_list, (gpointer)structure_element);
                        }
                    }
                }
            }
          else if(! g_strcmp0 (category, "iptc") && support_iptc)
            {
              write_tag = TRUE;
            }
          else
            {
              write_tag = FALSE;
            }

          if (write_tag)
            {
              switch (tag_value_type)
              {
                case TYPE_INVALID:
                  break;
                case TYPE_LONG:
                case TYPE_SLONG:
                case TYPE_FLOAT:
                case TYPE_DOUBLE:
                case TYPE_SHORT:
                case TYPE_SSHORT:
                case TYPE_DATE:
                case TYPE_TIME:
                case TYPE_ASCII:
                case TYPE_UNICODE:
                case TYPE_BYTE:
                case TYPE_RATIONAL:
                case TYPE_SRATIONAL:
                  {
                    gexiv2_metadata_set_tag_string (gexiv2metadata, tag, value);
                  }
                  break;
                case TYPE_MULTIPLE:
                  {
                    GValue h;
                    gchar **values;

                    h = gimp_attribute_get_value (attribute);
                    values = (gchar **) g_value_get_boxed (&h);
                    gexiv2_metadata_set_tag_multiple (gexiv2metadata, tag, (const gchar **) values);
                    g_strfreev  (values);
                  }
                  break;
                case TYPE_UNKNOWN:
                default:
                  break;

              }

              if (is_xmp)
                {
                  /* XMP packet for testing
                   * There is no way to check, if storing of tags were successful, because gexiv2 has no error handling
                   * The only way is to compare a xpm packet before and after the storing process. If they are equal, the storing was not successful
                   * so the stored tag must be deleted, otherwise the whole xmp data is corrupt. */
                  t_packet = gexiv2_metadata_generate_xmp_packet (gexiv2metadata, GEXIV2_USE_COMPACT_FORMAT | GEXIV2_OMIT_ALL_FORMATTING, 0);

                  if (! g_strcmp0 (t_packet, o_packet))
                    {
                      gexiv2_metadata_clear_tag (gexiv2metadata, tag);
                      g_print ("cleared to gexiv2metadata:\n%s, %s\n", tag, value);
                    }
                  else
                    {
                      o_packet = g_strdup (t_packet);
                    }
                }
            }
        }

      if (t_packet)
        g_free (t_packet);
      if (value)
        g_free (value);
      if (category)
        g_free (category);
    }

  if (o_packet)
    g_free (o_packet);
  if (xmp_structure_list)
    g_slist_free (xmp_structure_list);
}

/**
 * gimp_metadata_to_parent:
 * @metadata:  The #GimpMetadata
 * @gexiv2metadata:  The #GimpMetadata
 * @mime_type: the mime type of the image
 *
 * Converts @metadata to @gexiv2metadata in gexiv2 format
 *
 * Since: GIMP 2.10
 */
void
gimp_metadata_to_parent (GimpMetadata    *metadata,
                         GimpAttribute   *attribute)
{
  gchar                    *o_packet            = NULL;
  gboolean                  write_tag           = FALSE;
  gboolean                  namespace           = FALSE;
  gint                      i;
  GExiv2Metadata           *gexiv2metadata;
  const gchar              *tag;
  const gchar              *ns_name;
  gchar                    *value               = NULL;
  gchar                    *category            = NULL;
  gboolean                  is_xmp              = FALSE;
  gboolean                  has_structure       = FALSE;
  GimpAttributeValueType    tag_value_type;
  gchar                    *t_packet            = NULL;
  GimpMetadataPrivate      *priv;

  g_return_if_fail (IS_GIMP_METADATA (metadata));
  g_return_if_fail (GIMP_IS_ATTRIBUTE (attribute));

  gexiv2metadata = GEXIV2_METADATA (metadata);
  priv = GIMP_METADATA_GET_PRIVATE (metadata);
  write_tag      = FALSE;
  namespace      = FALSE;

  tag            = gimp_attribute_get_name (attribute);
  has_structure  = gimp_attribute_has_structure (attribute);


  tag_value_type = gimp_attribute_get_value_type (attribute);
  value = gimp_attribute_get_string (attribute);
  category = g_ascii_strdown (gimp_attribute_get_attribute_type (attribute), -1);

//  if (g_str_has_prefix (tag, "Xmp"))
//    {
//      g_print ("found xmp: %s\n", tag);
//    }
//
//  if (g_str_has_prefix (tag, "Xmp.xmpMM.History"))
//    {
//      g_print ("found struct: %s\n", tag);
//    }


  if (tag && value && category)
    {
      if(! g_strcmp0 (category, "exif"))
        {
          write_tag = TRUE;
        }
      else if(! g_strcmp0 (category, "xmp"))
        {
          write_tag = TRUE;
          is_xmp    = TRUE;

          /* XMP packet for testing before adding tag
           * There is no way to check, if storing of tags were successful, because gexiv2 has no error handling
           * The only way is to compare a xpm packet before and after the storing process. If they are equal, the storing was not successful
           * so the stored tag must be deleted, otherwise the whole xmp data is corrupt. */
          o_packet = gexiv2_metadata_generate_xmp_packet (gexiv2metadata, GEXIV2_USE_COMPACT_FORMAT | GEXIV2_OMIT_ALL_FORMATTING, 0);

          namespace = gimp_attribute_is_new_namespace (attribute);

          if (namespace)
            {
              write_tag = FALSE;

              ns_name = gimp_attribute_get_attribute_ifd (attribute);

              for (i = 0; i < G_N_ELEMENTS (namespaces_table); i++)
                {
                  struct Namespaces n_space = namespaces_table[i];
                  if(! g_strcmp0 (ns_name, n_space.namespace_name))
                    {
                      write_tag = TRUE;
                      break;
                    }
                }
            }

          if (write_tag && has_structure)
            {
              gboolean                    success = TRUE;
              GSList                     *structure;
              GSList                     *list;
              GimpAttributeStructureType  structure_type;

              structure      = gimp_attribute_get_attribute_structure (attribute);
              structure_type = gimp_attribute_get_structure_type (attribute);

              for (list = structure; list; list = list->next)
                {
                  const gchar   *structure_element = (const gchar*) list->data;
                  gboolean       has_tag = gexiv2_metadata_has_tag (gexiv2metadata, structure_element);

                  if (!has_tag && ! has_xmp_structure (priv->xmp_structure_list, structure_element))
                    {
                      switch (structure_type)
                      {
                        case STRUCTURE_TYPE_ALT:
                          success = gexiv2_metadata_set_xmp_tag_struct (gexiv2metadata, structure_element, GEXIV2_STRUCTURE_XA_ALT); /*start block*/
                          break;
                        case STRUCTURE_TYPE_BAG:
                          success = gexiv2_metadata_set_xmp_tag_struct (gexiv2metadata, structure_element, GEXIV2_STRUCTURE_XA_BAG); /*start block*/
                          break;
                        case STRUCTURE_TYPE_SEQ:
                          success = gexiv2_metadata_set_xmp_tag_struct (gexiv2metadata, structure_element, GEXIV2_STRUCTURE_XA_SEQ); /*start block*/
                          break;
                        default:
                          success = FALSE;
                          break;
                      }

                      if (success)
                        priv->xmp_structure_list = g_slist_prepend (priv->xmp_structure_list, (gpointer)structure_element);
                    }
                }
            }
        }
      else if(! g_strcmp0 (category, "iptc"))
        {
          write_tag = TRUE;
        }
      else
        {
          write_tag = FALSE;
        }

      if (write_tag)
        {
          switch (tag_value_type)
          {
            case TYPE_INVALID:
              break;
            case TYPE_LONG:
            case TYPE_SLONG:
            case TYPE_FLOAT:
            case TYPE_DOUBLE:
            case TYPE_SHORT:
            case TYPE_SSHORT:
            case TYPE_DATE:
            case TYPE_TIME:
            case TYPE_ASCII:
            case TYPE_UNICODE:
            case TYPE_BYTE:
            case TYPE_RATIONAL:
            case TYPE_SRATIONAL:
              {
                gexiv2_metadata_set_tag_string (gexiv2metadata, tag, value);
              }
              break;
            case TYPE_MULTIPLE:
              {
                GValue h;
                gchar **values;

                h = gimp_attribute_get_value (attribute);
                values = (gchar **) g_value_get_boxed (&h);
                gexiv2_metadata_set_tag_multiple (gexiv2metadata, tag, (const gchar **) values);
                g_strfreev  (values);
              }
              break;
            case TYPE_UNKNOWN:
            default:
              break;

          }

          if (is_xmp)
            {
              /* XMP packet for testing after adding tag */
              t_packet = gexiv2_metadata_generate_xmp_packet (gexiv2metadata, GEXIV2_USE_COMPACT_FORMAT | GEXIV2_OMIT_ALL_FORMATTING, 0);

              if (! g_strcmp0 (t_packet, o_packet))
                {
                  gexiv2_metadata_clear_tag (gexiv2metadata, tag);
                  g_print ("cleared to gexiv2metadata:\n%s, %s\n", tag, value);
                }
            }
        }
    }

  if (t_packet)
    g_free (t_packet);
  if (value)
    g_free (value);
  if (category)
    g_free (category);
  if (o_packet)
    g_free (o_packet);
}

/**
 * gimp_metadata_to_xmp_packet: ToDo handle xmp-packet
 * @metadata:  The #GimpMetadata
 * @mime_type :  a mime_type
 *
 * Converts @metadata to a xmp packet
 * It looks like an ugly hack, but let
 * gexiv2/exiv2 do all the hard work.
 *
 * Return value: a #gchar*, representing a xml packet.
 *
 * Since: GIMP 2.10
 */
const gchar *
gimp_metadata_to_xmp_packet (GimpMetadata *metadata,
                             const gchar *mime_type)

{
  gint            i;
  const gchar    *packet_string;
  gchar          *o_packet           = NULL;
  gboolean        check_mime         = TRUE;
  gboolean        write_tag          = FALSE;
  gboolean        namespace          = FALSE;
  gboolean        support_exif;
  gboolean        support_xmp;
  gboolean        support_iptc;
  GExiv2Metadata *gexiv2metadata;
  GSList         *xmp_structure_list = NULL;
  GList          *key_list           = NULL;

  g_return_val_if_fail (IS_GIMP_METADATA (metadata), NULL);

  gimp_metadata_from_gexiv2 (metadata);

  gexiv2metadata = gimp_metadata_new_gexiv2metadata ();

  for (i = 0; i < G_N_ELEMENTS (namespaces_table); i++)
    {
      struct Namespaces n_space = namespaces_table[i];
      gexiv2_metadata_register_xmp_namespace (n_space.namespace_URI, n_space.namespace_name);
    }

  support_exif = gexiv2_metadata_get_supports_exif (gexiv2metadata);
  support_xmp  = gexiv2_metadata_get_supports_xmp  (gexiv2metadata);
  support_iptc = gexiv2_metadata_get_supports_iptc (gexiv2metadata);

  for (key_list = metadata->priv->sorted_key_list; key_list; key_list = key_list->next)
    {
      gchar          *p_key = (gchar *) key_list->data;

      const gchar            *tag;
      const gchar            *attribute_tag;
      const gchar            *ns_name;
      gchar                  *new_tag         = NULL;
      gchar                  *value           = NULL;
      gchar                  *category        = NULL;
      gboolean                has_structure;
      gboolean                temp_attribute  = FALSE;
      GimpAttribute          *attribute;
      GimpAttributeValueType  tag_value_type;

      write_tag     = FALSE;
      namespace = FALSE;

      attribute = gimp_metadata_get_attribute_sorted (metadata, p_key);

      if (! attribute)
        continue;

      tag            = gimp_attribute_get_name (attribute);
      attribute_tag  = gimp_attribute_get_attribute_tag (attribute);
      has_structure  = gimp_attribute_has_structure (attribute);

      if (mime_type)
        check_mime = gimp_metadata_is_tag_supported (tag, mime_type);

      if (check_mime)
        {
          gchar *sec_tag        = NULL;
          gchar *t_packet       = NULL;

          tag_value_type = gimp_attribute_get_value_type (attribute);
          value = gimp_attribute_get_string (attribute);
          category = g_ascii_strdown (gimp_attribute_get_attribute_type (attribute), -1);

          if (tag && value && category)
            {
              if(! g_strcmp0 (category, "exif") && support_exif)
                {
                  new_tag = g_strdup_printf ("Xmp.exif.%s", attribute_tag);

                  write_tag = TRUE;

//                  /* Now for some specialities */
//                  if (! g_strcmp0 (new_tag, "Xmp.exif.ISOSpeedRatings"))
//                    {
//                      g_print ("ungültig\n");
//                       attribute = gimp_attribute_new_string ("Xmp.exif.ISOSpeedRatings", value, TYPE_ASCII);
//                      if (attribute)
//                        {
//                          temp_attribute = TRUE;
//                          tag_value_type = TYPE_ASCII;
//                        }
//                      else
//                        {
//                          write_tag = FALSE;
//                        }
//                    }
                }
              else if(! g_strcmp0 (category, "xmp") && support_xmp)
                {
                  new_tag = g_strdup_printf ("%s", tag);

                  write_tag = TRUE;

                  namespace = gimp_attribute_is_new_namespace (attribute);

                  if (namespace)
                    {
                      write_tag = FALSE;

                      ns_name = gimp_attribute_get_attribute_ifd (attribute);

                      for (i = 0; i < G_N_ELEMENTS (namespaces_table); i++)
                        {
                          struct Namespaces n_space = namespaces_table[i];
                          if(! g_strcmp0 (ns_name, n_space.namespace_name))
                            {
                              write_tag = TRUE;
                              break;
                            }
                        }
                    }

                  if (write_tag && has_structure)
                    {
                      gboolean                    success = TRUE;
                      GSList                     *structure;
                      GSList                     *list;
                      GimpAttributeStructureType  structure_type;

                      structure      = gimp_attribute_get_attribute_structure (attribute);
                      structure_type = gimp_attribute_get_structure_type (attribute);

                      for (list = structure; list; list = list->next)
                        {
                          const gchar   *structure_element = (const gchar*) list->data;
                          gboolean       has_tag = gexiv2_metadata_has_tag (gexiv2metadata, structure_element);

                          if (!has_tag && ! has_xmp_structure (xmp_structure_list, structure_element))
                            {
                              switch (structure_type)
                              {
                                case STRUCTURE_TYPE_ALT:
                                  success = gexiv2_metadata_set_xmp_tag_struct (gexiv2metadata, structure_element, GEXIV2_STRUCTURE_XA_ALT); /*start block*/
                                  break;
                                case STRUCTURE_TYPE_BAG:
                                  success = gexiv2_metadata_set_xmp_tag_struct (gexiv2metadata, structure_element, GEXIV2_STRUCTURE_XA_BAG); /*start block*/
                                  break;
                                case STRUCTURE_TYPE_SEQ:
                                  success = gexiv2_metadata_set_xmp_tag_struct (gexiv2metadata, structure_element, GEXIV2_STRUCTURE_XA_SEQ); /*start block*/
                                  break;
                                default:
                                  success = FALSE;
                                  break;
                              }

                              if (success)
                                xmp_structure_list = g_slist_prepend (xmp_structure_list, (gpointer)structure_element);
                            }
                        }
                    }
                }
              else if(! g_strcmp0 (category, "iptc") && support_iptc)
                {
                  new_tag = g_strdup_printf ("Xmp.iptc.%s", attribute_tag);
                  sec_tag = g_strdup_printf ("Xmp.iptcExt.%s", attribute_tag);
                  write_tag = TRUE;
                }
              else
                {
                  write_tag = FALSE;
                }

              if (write_tag)
                {
                  switch (tag_value_type)
                  {
                    case TYPE_INVALID:
                      break;
                    case TYPE_LONG:
                    case TYPE_SLONG:
                    case TYPE_FLOAT:
                    case TYPE_DOUBLE:
                    case TYPE_SHORT:
                    case TYPE_SSHORT:
                    case TYPE_DATE:
                    case TYPE_TIME:
                    case TYPE_ASCII:
                    case TYPE_UNICODE:
                    case TYPE_BYTE:
                    case TYPE_RATIONAL:
                    case TYPE_SRATIONAL:
                      {
                        gexiv2_metadata_set_tag_string (gexiv2metadata, new_tag, value);
                        if (sec_tag)
                          {
                            gexiv2_metadata_set_tag_string (gexiv2metadata, sec_tag, value);
                            g_free (sec_tag);
                          }

                      }
                      break;
                    case TYPE_MULTIPLE:
                      {
                        GValue h;
                        gchar **values;

                        h = gimp_attribute_get_value (attribute);
                        values = (gchar **) g_value_get_boxed (&h);
                        gexiv2_metadata_set_tag_multiple (gexiv2metadata, new_tag, (const gchar **) values);
                        if (sec_tag)
                          {
                            gexiv2_metadata_set_tag_multiple (gexiv2metadata, sec_tag, (const gchar **) values);
                            g_free (sec_tag);
                          }
                        g_strfreev  (values);
                      }
                      break;
                    case TYPE_UNKNOWN:
                    default:
                      break;

                  }

                  t_packet = gexiv2_metadata_generate_xmp_packet (gexiv2metadata, GEXIV2_USE_COMPACT_FORMAT | GEXIV2_OMIT_ALL_FORMATTING, 0);

                  if (! t_packet || ! g_strcmp0 (t_packet, o_packet))
                    {
                      gexiv2_metadata_clear_tag (gexiv2metadata, new_tag);
                    }
                  else
                    {
                      o_packet = g_strdup (t_packet);
                    }
                }
            }

          if (t_packet)
            g_free (t_packet);
          if (value)
            g_free (value);
          if (category)
            g_free (category);
          if (new_tag)
            g_free (new_tag);

          if (temp_attribute)
            g_object_unref (attribute);
       }
    }

  if (o_packet)
    g_free (o_packet);
  if (xmp_structure_list)
    g_slist_free (xmp_structure_list);

  packet_string = gexiv2_metadata_generate_xmp_packet (gexiv2metadata, GEXIV2_USE_COMPACT_FORMAT | GEXIV2_WRITE_ALIAS_COMMENTS, 0);
  return packet_string;
}

/**
 * gimp_metadata_has_tag_type:
 * @metadata:  The metadata
 * @tag_type:  The #GimpAttributeTagType to test
 *
 * tests, if @metadata contains at least one tag of @tag_type
 *
 * Return value: TRUE if found, FALSE otherwise
 *
 * Since: GIMP 2.10
 */
gboolean
gimp_metadata_has_tag_type (GimpMetadata         *metadata,
                            GimpAttributeTagType  tag_type)
{
  GHashTableIter  iter;
  gpointer        p_key, p_value;

  g_return_val_if_fail (IS_GIMP_METADATA (metadata), FALSE);

  gimp_metadata_from_gexiv2 (metadata);

  g_hash_table_iter_init (&iter, metadata->priv->attribute_table);

  while (g_hash_table_iter_next (&iter, &p_key, &p_value))
    {
      GimpAttribute         *attribute;

      attribute = (GimpAttribute *) p_value;

      if (gimp_attribute_get_tag_type (attribute) == tag_type)
        return TRUE;

    }
  return FALSE;
}

GList *
gimp_metadata_iter_init (GimpMetadata *metadata, GList **iter)
{
  g_return_val_if_fail (IS_GIMP_METADATA (metadata), NULL);

  gimp_metadata_from_gexiv2 (metadata);

  *iter = metadata->priv->sorted_key_list;
  iter_initialized = TRUE;

  return metadata->priv->sorted_key_list;
}

gboolean
gimp_metadata_iter_next (GimpMetadata *metadata, GimpAttribute **attribute, GList **prev)
{
  gchar         *sorted;
  GList         *tmp;

  g_return_val_if_fail (IS_GIMP_METADATA (metadata), FALSE);

  *attribute = NULL;

  if (iter_initialized)
    {
      tmp = g_list_first (*prev);
    }
  else
    {
      tmp = g_list_next (*prev);
    }

  if (tmp)
    {
      *prev = tmp;
      sorted = (gchar *) tmp->data;

      *attribute = gimp_metadata_get_attribute_sorted (metadata, sorted);
    }

  iter_initialized = FALSE;

  if (*attribute)
    return TRUE;
  else
    return FALSE;

}

/**
 * gimp_metadata_error_quark:
 *
 * Return value: #GQuark
 *
 * Since: GIMP 2.10
 */
static GQuark
gimp_metadata_error_quark (void)
{
  static GQuark quark = 0;

  if (G_UNLIKELY (quark == 0))
    quark = g_quark_from_static_string ("gimp-metadata-error-quark");

  return quark;
}

/**
 * gimp_metadata_deserialize_error:
 *
 * Error while parsing
 *
 * Since: GIMP 2.10
 */
static void
gimp_metadata_deserialize_error (GMarkupParseContext *context,
                                   GError              *error,
                                   gpointer             user_data)
{
  g_printerr ("XML parse error: %s\n", error->message);
}

/**
 * gimp_metadata_name_to_value:
 *
 * @attribute_names  : #gchar **
 * @attribute_values : #gchar **
 * @name             : #gchar *
 *
 * searches for values in @attribute_values by a given @name (parsing xml)
 *
 * Since: GIMP 2.10
 */
static const gchar*
gimp_metadata_name_to_value (const gchar **attribute_names,
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

/**
 * gimp_metadata_deserialize_start_element:
 *
 * @context           : #GMarkupParseContext
 * @element_name      : #gchar *
 * @attribute_names   : #gchar **
 * @attribute_values  : #gchar **
 * @user_data         : #gpointer to #GimpMetadataParseData struct
 * @error             : #GError **
 *
 * start of a tag (parsing xml)
 *
 * Since: GIMP 2.10
 */
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

      name = gimp_metadata_name_to_value (attribute_names,
                                                    attribute_values,
                                                    "name");
      encoding = gimp_metadata_name_to_value (attribute_names,
                                                        attribute_values,
                                                        "encoding");

      if (! name)
        {
          g_set_error (error, gimp_metadata_error_quark (), 1001,
                       "Element 'tag' does not contain required attribute 'name'.");
          return;
        }

      strncpy (parse_data->name, name, sizeof (parse_data->name));
      parse_data->name[sizeof (parse_data->name) - 1] = 0;

      parse_data->base64 = (encoding && ! strcmp (encoding, "base64"));
    }
  else if (! strcmp (element_name, "attribute"))
    {
      const gchar *name = NULL;
      const gchar *type = NULL;

      name     = gimp_metadata_name_to_value (attribute_names,
                                              attribute_values,
                                              "name");
      type     = gimp_metadata_name_to_value (attribute_names,
                                              attribute_values,
                                              "type");

      if (! name)
        {
          g_set_error (error, gimp_metadata_error_quark (), 1001,
                       "Element 'tag' does not contain required attribute 'name'.");
          return;
        }

      if (! type)
        {
          g_set_error (error, gimp_metadata_error_quark (), 1001,
                       "Element 'tag' does not contain required attribute 'type'.");
          return;
        }

      strncpy (parse_data->name, name, sizeof (parse_data->name));
      parse_data->name[sizeof (parse_data->name) - 1] = 0;

      parse_data->type = (GimpAttributeValueType) atoi (type);
    }
  else if (! strcmp (element_name, "value"))
    {
      const gchar *encoding = NULL;

      encoding = gimp_metadata_name_to_value (attribute_names,
                                              attribute_values,
                                              "encoding");

      parse_data->base64 = (encoding && ! strcmp (encoding, "base64"));
    }
  else if (! strcmp (element_name, "interpreted"))
    {
      const gchar *encoding = NULL;

      encoding = gimp_metadata_name_to_value (attribute_names,
                                              attribute_values,
                                              "encoding");

      parse_data->base64 = (encoding && ! strcmp (encoding, "base64"));
    }

}

/**
 * gimp_metadata_deserialize_text:
 *
 * @context           : #GMarkupParseContext
 * @text              : const #gchar *
 * @text_len          : #gsize
 * @user_data         : #gpointer to #GimpMetadataParseData struct
 * @error             : #GError **
 *
 * text of a tag (parsing xml)
 *
 * Since: GIMP 2.10
 */
static void
gimp_metadata_deserialize_text (GMarkupParseContext  *context,
                                const gchar          *text,
                                gsize                 text_len,
                                gpointer              user_data,
                                GError              **error)
{
  GimpMetadataParseData   *parse_data       = user_data;
  GimpAttribute           *attribute        = NULL;
  const gchar             *current_element;

  current_element = g_markup_parse_context_get_element (context);

  if (! g_strcmp0 (current_element, "tag")) /*old metadata*/
    {
      gchar *value = g_strndup (text, text_len);

      if (parse_data->base64)
        {
          guchar *decoded;
          gsize   len;

          decoded = g_base64_decode (value, &len);

          if (decoded[len - 1] == '\0')
            gexiv2_metadata_set_tag_string (GEXIV2_METADATA (parse_data->metadata),
                                            parse_data->name,
                                            (const gchar *) decoded);

          g_free (decoded);
        }
      else
        {
          gexiv2_metadata_set_tag_string (GEXIV2_METADATA (parse_data->metadata),
                                          parse_data->name,
                                          value);
        }

      g_free (value);
    }
  else if (! g_strcmp0 (current_element, "value"))
    {
      gchar *value = g_strndup (text, text_len);

      if (parse_data->base64)
        {
          guchar *decoded;
          gsize   len;

          decoded = g_base64_decode (value, &len);

          if (decoded[len - 1] == '\0')
            attribute = gimp_attribute_new_string (parse_data->name,
                                                   (gchar *)decoded,
                                                   parse_data->type);

          g_free (decoded);
        }
      else
        {
          attribute = gimp_attribute_new_string (parse_data->name,
                                                 value,
                                                 parse_data->type);
        }

      g_free (value);

      if (attribute && gimp_attribute_is_valid (attribute))
        {
          gimp_metadata_add_attribute (parse_data->metadata, attribute);
          current_attribute = attribute;
        }
      else
        g_object_unref (attribute);

    }
  else if (! g_strcmp0 (current_element, "structure"))
    {
      GimpAttribute *attribute = NULL;
      gchar         *value     = NULL;

      attribute = current_attribute;

      if (attribute)
        {
          gint                       v;
          GimpAttributeStructureType struct_type = STRUCTURE_TYPE_BAG;

          value = g_strndup (text, text_len);

          v = atoi (value);

          if (v > -1)
            struct_type = (GimpAttributeStructureType) v;

          gimp_attribute_set_structure_type (attribute, struct_type);

          g_free (value);
        }
    }
  else if (! g_strcmp0 (current_element, "interpreted"))
    {
      GimpAttribute *attribute = NULL;
      gchar         *value     = NULL;

      attribute = current_attribute;

      if (attribute)
        {
          value = g_strndup (text, text_len);

          if (parse_data->base64)
            {
              guchar *decoded = NULL;
              gsize   len;

              decoded = g_base64_decode (value, &len);

              if (decoded[len - 1] == '\0')
                gimp_attribute_set_interpreted_string (attribute, (const gchar *)decoded);

              g_free (decoded);
            }
          else
            {
              gimp_attribute_set_interpreted_string (attribute, (const gchar *)value);
            }
          g_free (value);
        }
    }
}

/**
 * gimp_metadata_deserialize_end_element:
 *
 * @context           : #GMarkupParseContext
 * @element_name      : #gchar *
 * @user_data         : #gpointer to #GimpMetadataParseData struct
 * @error             : #GError **
 *
 * end of a tag (parsing xml)
 *
 * Since: GIMP 2.10
 */
static void
gimp_metadata_deserialize_end_element (GMarkupParseContext *context,
                                         const gchar         *element_name,
                                         gpointer             user_data,
                                         GError             **error)
{
  GimpMetadataParseData *parse_data = user_data;

  if (! strcmp (element_name, "attribute"))
    {
      current_attribute = NULL;
    }
  else if (! strcmp (element_name, "value"))
    {
      parse_data->base64 = FALSE;
    }
  else if (! strcmp (element_name, "interpreted"))
    {
      parse_data->base64 = FALSE;
    }
}

static gboolean
has_xmp_structure (GSList *xmp_list, const gchar *entry)
{
  GSList *list;

  for (list = xmp_list; list; list = list->next)
    {
      const gchar *to_test = (const gchar*) list->data;

       if (! g_strcmp0 (to_test, entry))
        return TRUE;
    }

  return FALSE;
}

/**
 * gimp_metadata_new_gexiv2metadata:
 *
 * Creates a new #GExiv2Metadata instance.
 *
 * Return value: The new #GExiv2Metadata.
 *
 * Since: 2.10
 */
GExiv2Metadata *
gimp_metadata_new_gexiv2metadata (void)
{
  GExiv2Metadata *gexiv2metadata = NULL;
  gint            i;

  if (gexiv2_initialize ())
    {
      gexiv2metadata = gexiv2_metadata_new ();

      if (! gexiv2_metadata_open_buf (gexiv2metadata, wilber_jpg, wilber_jpg_len,
                                      NULL))
        {
          g_object_unref (gexiv2metadata);

          return NULL;
        }
      for (i = 0; i < G_N_ELEMENTS (namespaces_table); i++)
        {
          struct Namespaces n_space = namespaces_table[i];
          gexiv2_metadata_register_xmp_namespace (n_space.namespace_URI, n_space.namespace_name);
        }

    }
  return gexiv2metadata;
}


/**
 * gimp_metadata_save_to_file:
 * @metadata: A #GimpMetadata instance.
 * @file:     The file to save the metadata to
 * @error:    Return location for error message
 *
 * Saves @metadata to @file.
 *
 * Return value: %TRUE on success, %FALSE otherwise.
 *
 * Since: 2.10
 */
gboolean
gimp_metadata_save_to_file (GimpMetadata  *metadata,
                            GFile         *file,
                            GError       **error)
{
  gchar          *path;
  gchar          *filename;
  gboolean        success;

  g_return_val_if_fail (IS_GIMP_METADATA (metadata), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  path = g_file_get_path (file);

  if (! path)
    {
      g_set_error (error, gimp_metadata_error_quark (), 0,
                   _("Can save metadata only to local files"));
      return FALSE;
    }

#ifdef G_OS_WIN32
  filename = g_win32_locale_filename_from_utf8 (path);
#else
  filename = g_strdup (path);
#endif

  g_free (path);

  success = gexiv2_metadata_save_file (GEXIV2_METADATA (metadata), filename, error);

  g_free (filename);

  return success;
}

/**
 * gimp_metadata_load_from_file:
 * @file:  The #GFile to load the metadata from
 * @error: Return location for error message
 *
 * Loads #GimpMetadata from @file.
 *
 * Return value: The loaded #GimpMetadata.
 *
 * Since: 2.10
 */
GimpMetadata  *
gimp_metadata_load_from_file (GFile   *file,
                              GError **error)
{
  GimpMetadata   *metadata     = NULL;
  gchar          *path;
  gchar          *filename;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  path = g_file_get_path (file);

  if (! path)
    {
      g_set_error (error, gimp_metadata_error_quark (), 0,
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
      metadata = gimp_metadata_new ();

      if (! gexiv2_metadata_open_path (GEXIV2_METADATA (metadata), filename, error))
        {
          g_free (filename);
          g_object_unref (metadata);

          return NULL;
        }
    }

  g_free (filename);

  return metadata;
}

/**
 * gimp_metadata_set_from_exif:
 * @metadata:         A #GimpMetadata instance.
 * @exif_data:        The blob of Exif data to set
 * @exif_data_length: Length of @exif_data, in bytes
 * @error:            Return location for error message
 *
 * Sets the tags from a piece of Exif data on @metadata.
 *
 * Return value: %TRUE on success, %FALSE otherwise.
 *
 * Since: 2.10
 */
gboolean
gimp_metadata_set_from_exif (GimpMetadata  *metadata,
                             const guchar  *exif_data,
                             gint           exif_data_length,
                             GError       **error)
{

  GByteArray     *exif_bytes;
  GExiv2Metadata *exif_metadata;
  guint8          data_size[2] = { 0, };
  const guint8    eoi[2] = { 0xff, 0xd9 };

  g_return_val_if_fail (IS_GIMP_METADATA (metadata), FALSE);
  g_return_val_if_fail (exif_data != NULL, FALSE);
  g_return_val_if_fail (exif_data_length > 0, FALSE);
  g_return_val_if_fail (exif_data_length + 2 < 65536, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  exif_metadata = GEXIV2_METADATA (metadata);
  g_object_ref (exif_metadata);

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

  if (! gexiv2_metadata_open_buf (exif_metadata,
                                  exif_bytes->data, exif_bytes->len, error))
    {
      g_object_unref (exif_metadata);
      g_byte_array_free (exif_bytes, TRUE);
      return FALSE;
    }

  if (! gexiv2_metadata_has_exif (exif_metadata))
    {
      g_set_error (error, gimp_metadata_error_quark (), 0,
                   _("Parsing Exif data failed."));
      g_object_unref (exif_metadata);
      g_byte_array_free (exif_bytes, TRUE);
      return FALSE;
    }

  g_object_unref (exif_metadata);
  g_byte_array_free (exif_bytes, TRUE);

  return TRUE;
}

/**
 * gimp_metadata_set_from_xmp:
 * @metadata:        A #GimpMetadata instance.
 * @xmp_data:        The blob of Exif data to set
 * @xmp_data_length: Length of @exif_data, in bytes
 * @error:           Return location for error message
 *
 * Sets the tags from a piece of XMP data on @metadata.
 *
 * Return value: %TRUE on success, %FALSE otherwise.
 *
 * Since: 2.10
 */
gboolean
gimp_metadata_set_from_xmp (GimpMetadata  *metadata,
                            const guchar  *xmp_data,
                            gint           xmp_data_length,
                            GError       **error)
{
  GExiv2Metadata *xmp_metadata;

  g_return_val_if_fail (IS_GIMP_METADATA (metadata), FALSE);
  g_return_val_if_fail (xmp_data != NULL, FALSE);
  g_return_val_if_fail (xmp_data_length > 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  xmp_metadata = GEXIV2_METADATA (metadata);
  g_object_ref (xmp_metadata);

  if (! gexiv2_metadata_open_buf (xmp_metadata,
                                  xmp_data, xmp_data_length, error))
    {
      g_object_unref (xmp_metadata);
      return FALSE;
    }

  if (! gexiv2_metadata_has_xmp (xmp_metadata))
    {
      g_set_error (error, gimp_metadata_error_quark (), 0,
                   _("Parsing XMP data failed."));
      g_object_unref (xmp_metadata);
      return FALSE;
    }

  g_object_unref (xmp_metadata);
  return TRUE;
}

/**
 * gimp_metadata_is_tag_supported:
 * @tag:       A metadata tag name
 * @mime_type: A mime type
 *
 * Returns whether @tag is supported in a file of type @mime_type.
 *
 * Return value: %TRUE if the @tag supported with @mime_type, %FALSE otherwise.
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

/**
 * gimp_metadata_get_colorspace:
 * @metadata: Ab #GimpMetadata instance.
 *
 * Returns values based on Exif.Photo.ColorSpace, Xmp.exif.ColorSpace,
 * Exif.Iop.InteroperabilityIndex, Exif.Nikon3.ColorSpace,
 * Exif.Canon.ColorSpace of @metadata.
 *
 * Return value: The colorspace specified by above tags.
 *
 * Since: 2.10
 */
GimpMetadataColorspace
gimp_metadata_get_colorspace (GimpMetadata *metadata)
{
  glong          exif_cs   = -1;
  GimpAttribute *attribute = NULL;
  GValue         val;

  g_return_val_if_fail (IS_GIMP_METADATA (metadata),
                        GIMP_METADATA_COLORSPACE_UNSPECIFIED);

  /*  the logic here was mostly taken from darktable and libkexiv2  */

  attribute = gimp_metadata_get_attribute (metadata, "Exif.Photo.ColorSpace");
  if ( ! attribute)
    {
      attribute = gimp_metadata_get_attribute (metadata, "Xmp.exif.ColorSpace");
    }

  if (attribute)
    {
      if (gimp_attribute_get_value_type (attribute) == TYPE_LONG || gimp_attribute_get_value_type (attribute) == TYPE_SLONG)
        {
          val = gimp_attribute_get_value (attribute);
          exif_cs = g_value_get_long (&val);
        }
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

          attribute = gimp_metadata_get_attribute (metadata, "Exif.Iop.InteroperabilityIndex");
          if (attribute)
            iop_index = gimp_attribute_get_string (attribute);

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

      attribute = gimp_metadata_get_attribute (metadata, "Exif.Nikon3.ColorSpace");
      if (attribute)
        {
          if (gimp_attribute_get_value_type (attribute) == TYPE_LONG || gimp_attribute_get_value_type (attribute) == TYPE_SLONG)
            {
              glong nikon_cs;

              val = gimp_attribute_get_value (attribute);
              nikon_cs = g_value_get_long (&val);

              if (nikon_cs == 0x01)
                {
                  return GIMP_METADATA_COLORSPACE_SRGB;
                }
              else if (nikon_cs == 0x02)
                {
                  return GIMP_METADATA_COLORSPACE_ADOBERGB;
                }
            }
        }

      attribute = gimp_metadata_get_attribute (metadata, "Exif.Canon.ColorSpace");
      if (attribute)
        {
          if (gimp_attribute_get_value_type (attribute) == TYPE_LONG || gimp_attribute_get_value_type (attribute) == TYPE_SLONG)
            {
              glong canon_cs;

              val = gimp_attribute_get_value (attribute);
              canon_cs = g_value_get_long (&val);

              if (canon_cs == 0x01)
                {
                  return GIMP_METADATA_COLORSPACE_SRGB;
                }
              else if (canon_cs == 0x02)
                {
                  return GIMP_METADATA_COLORSPACE_ADOBERGB;
                }
            }
        }

      if (exif_cs == 0xffff)
        return GIMP_METADATA_COLORSPACE_UNCALIBRATED;
    }

  return GIMP_METADATA_COLORSPACE_UNSPECIFIED;
}

/**
 * gimp_metadata_get_resolution:
 * @metadata: A #GimpMetadata instance.
 * @xres:     Return location for the X Resolution, in ppi
 * @yres:     Return location for the Y Resolution, in ppi
 * @unit:     Return location for the unit unit
 *
 * Returns values based on Exif.Image.XResolution,
 * Exif.Image.YResolution and Exif.Image.ResolutionUnit of @metadata.
 *
 * Return value: %TRUE on success, %FALSE otherwise.
 *
 * Since: GIMP 2.10
 */
gboolean
gimp_metadata_get_resolution (GimpMetadata *metadata,
                              gdouble        *xres,
                              gdouble        *yres,
                              GimpUnit       *unit)
{
  GimpAttribute *x_attribute;
  GimpAttribute *y_attribute;
  GimpAttribute *res_attribute;

  gint   xnom, xdenom;
  gint   ynom, ydenom;
  gint   exif_unit = 2;

  g_return_val_if_fail (IS_GIMP_METADATA (metadata), FALSE);

  x_attribute = gimp_metadata_get_attribute (metadata, "Exif.Image.XResolution");
  y_attribute = gimp_metadata_get_attribute (metadata, "Exif.Image.YResolution");
  res_attribute = gimp_metadata_get_attribute (metadata, "Exif.Image.ResolutionUnit");

  if (x_attribute)
    {
      GValue    value;
      Rational *rats;
      gint     *_nom;
      gint     *_denom;
      gint      l;

      value = gimp_attribute_get_value (x_attribute);

      rats = g_value_get_boxed (&value);

      rational_to_int (rats, &_nom, &_denom, &l);

      if (l > 0)
        {
          xnom = _nom[0];
          xdenom = _denom[0];
        }
      rational_free (rats);
    }
  else
    return FALSE;

  if (y_attribute)
    {
      GValue    value;
      Rational *rats;
      gint     *_nom;
      gint     *_denom;
      gint      l;

      value = gimp_attribute_get_value (y_attribute);

      rats = g_value_get_boxed (&value);

      rational_to_int (rats, &_nom, &_denom, &l);

      if (l > 0)
        {
          ynom = _nom[0];
          ydenom = _denom[0];
        }
      rational_free (rats);
    }
  else
    return FALSE;

  if (res_attribute)
    {
      GValue value = gimp_attribute_get_value (res_attribute);

      exif_unit = (gint) g_value_get_uint (&value);
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
                *unit = GIMP_UNIT_MM;
              else
                *unit = GIMP_UNIT_INCH;
            }

          return TRUE;
        }
    }
  return FALSE;
}

/**
 * gimp_metadata_set_bits_per_sample:
 * @metadata: A #GimpMetadata instance.
 * @bps:      Bytes per pixel, per component
 *
 * Sets Exif.Image.BitsPerSample on @metadata.
 *
 * Since: GIMP 2.10
 */
void
gimp_metadata_set_bits_per_sample (GimpMetadata *metadata,
                                   gint            bps)
{
  gchar buffer[32];

  g_return_if_fail ( (metadata));

  g_snprintf (buffer, sizeof (buffer), "%d", bps);
  gimp_metadata_new_attribute (metadata, "Exif.Image.BitsPerSample", buffer, TYPE_SHORT);
}

/**
 * gimp_metadata_set_pixel_size:
 * @metadata: A #GimpMetadata instance.
 * @width:    Width in pixels
 * @height:   Height in pixels
 *
 * Sets Exif.Image.ImageWidth and Exif.Image.ImageLength on @metadata.
 *
 * Since: GIMP 2.10
 */
void
gimp_metadata_set_pixel_size (GimpMetadata *metadata,
                              gint            width,
                              gint            height)
{
  gchar buffer[32];

  g_return_if_fail (IS_GIMP_METADATA (metadata));

  g_snprintf (buffer, sizeof (buffer), "%d", width);
  gimp_metadata_new_attribute (metadata, "Exif.Image.ImageWidth", buffer, TYPE_LONG);

  g_snprintf (buffer, sizeof (buffer), "%d", height);
  gimp_metadata_new_attribute (metadata, "Exif.Image.ImageLength", buffer, TYPE_LONG);
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
  g_return_if_fail (IS_GIMP_METADATA (metadata));

  gimp_metadata_remove_attribute (metadata, "Exif.Photo.ColorSpace");
  gimp_metadata_remove_attribute (metadata, "Xmp.exif.ColorSpace");
  gimp_metadata_remove_attribute (metadata, "Exif.Iop.InteroperabilityIndex");

  switch (colorspace)
    {
    case GIMP_METADATA_COLORSPACE_UNSPECIFIED:
      gimp_metadata_remove_attribute (metadata, "Exif.Nikon3.ColorSpace");
      gimp_metadata_remove_attribute (metadata, "Exif.Canon.ColorSpace");
      break;

    case GIMP_METADATA_COLORSPACE_UNCALIBRATED:
      gimp_metadata_new_attribute (metadata, "Exif.Photo.ColorSpace", "0xffff", TYPE_LONG);
      gimp_metadata_new_attribute (metadata, "Xmp.exif.ColorSpace", "0xffff", TYPE_LONG);
      break;

    case GIMP_METADATA_COLORSPACE_SRGB:
      gimp_metadata_new_attribute (metadata, "Exif.Photo.ColorSpace", "0x01", TYPE_LONG);
      gimp_metadata_new_attribute (metadata, "Xmp.exif.ColorSpace", "0x01", TYPE_LONG);
      gimp_metadata_new_attribute (metadata, "Exif.Iop.InteroperabilityIndex", "R98", TYPE_ASCII);
      if (gimp_metadata_has_attribute (metadata, "Exif.Nikon3.ColorSpace"))
        {
          gimp_metadata_remove_attribute (metadata, "Exif.Nikon3.ColorSpace");
          gimp_metadata_new_attribute (metadata, "Exif.Nikon3.ColorSpace", "0x01", TYPE_LONG);
        }
      if (gimp_metadata_has_attribute (metadata, "Exif.Canon.ColorSpace"))
        {
          gimp_metadata_remove_attribute (metadata, "Exif.Canon.ColorSpace");
          gimp_metadata_new_attribute (metadata, "Exif.Canon.ColorSpace", "0x01", TYPE_LONG);
        }
      break;

    case GIMP_METADATA_COLORSPACE_ADOBERGB:
      gimp_metadata_new_attribute (metadata, "Exif.Photo.ColorSpace", "0x02", TYPE_LONG);
      gimp_metadata_new_attribute (metadata, "Xmp.exif.ColorSpace", "0x02", TYPE_LONG);
      gimp_metadata_new_attribute (metadata, "Exif.Iop.InteroperabilityIndex", "R03", TYPE_ASCII);
      if (gimp_metadata_has_attribute (metadata, "Exif.Nikon3.ColorSpace"))
        {
          gimp_metadata_remove_attribute (metadata, "Exif.Nikon3.ColorSpace");
          gimp_metadata_new_attribute (metadata, "Exif.Nikon3.ColorSpace", "0x02", TYPE_LONG);
        }
      if (gimp_metadata_has_attribute (metadata, "Exif.Canon.ColorSpace"))
        {
          gimp_metadata_remove_attribute (metadata, "Exif.Canon.ColorSpace");
          gimp_metadata_new_attribute (metadata, "Exif.Canon.ColorSpace", "0x02", TYPE_LONG);
        }
      break;
    }
}

/**
 * gimp_metadata_set_resolution:
 * @metadata: A #GimpMetadata instance.
 * @xres:     The image's X Resolution, in ppi
 * @yres:     The image's Y Resolution, in ppi
 * @unit:     The image's unit
 *
 * Sets Exif.Image.XResolution, Exif.Image.YResolution and
 * Exif.Image.ResolutionUnit @metadata.
 *
 * Since: GIMP 2.10
 */
void
gimp_metadata_set_resolution (GimpMetadata *metadata,
                              gdouble         xres,
                              gdouble         yres,
                              GimpUnit        unit)
{
  gchar buffer[64];
  gint  exif_unit;
  gint  factor;

  g_return_if_fail (IS_GIMP_METADATA (metadata));

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

  g_snprintf (buffer, sizeof (buffer), "%d/%d", ROUND (xres * factor), factor);
  gimp_metadata_new_attribute (metadata, "Exif.Image.XResolution", buffer, TYPE_RATIONAL);

  g_snprintf (buffer, sizeof (buffer), "%d/%d", ROUND (yres * factor), factor);
  gimp_metadata_new_attribute (metadata, "Exif.Image.YResolution", buffer, TYPE_RATIONAL);

  g_snprintf (buffer, sizeof (buffer), "%d", exif_unit);
  gimp_metadata_new_attribute (metadata, "Exif.Image.ResolutionUnit", buffer, TYPE_SHORT);
}

