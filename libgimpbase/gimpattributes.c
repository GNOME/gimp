/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpattributes.c
 * Copyright (C) 2014  Hartmut Kuhse <hatti@gimp.org>
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

#include <glib-object.h>
#include <gexiv2/gexiv2.h>

#ifdef G_OS_WIN32
#endif

#include "gimpbasetypes.h"
#include "gimpmetadata.h"
#include "gimpattribute.h"
#include "gimpattributes.h"

/**
 * SECTION: gimpattributes
 * @title: gimpattributes
 * @short_description: Class to store #GimpAttribute objects.
 * @see_also: GimpAttribute,
 *
 * Class to store #GimpAttribute objects.
 **/

#define _g_free0(var) (var = (g_free (var), NULL))

typedef struct _GimpAttributesClass GimpAttributesClass;
typedef struct _GimpAttributesPrivate GimpAttributesPrivate;

struct _GimpAttributes {
  GObject               parent_instance;
  GimpAttributesPrivate *priv;
};

struct _GimpAttributesClass {
  GObjectClass parent_class;
};

struct _GimpAttributesPrivate {
  GHashTable *attribute_table;
  GHashTable *sorted_to_attribute;
  GList      *sorted_key_list;
};

typedef struct
{
  gchar                     name[1024];
  gboolean                  base64;
  GimpAttributeValueType    type;
  GimpAttributes           *attributes;
} GimpAttributesParseData;

struct Namespaces{
  const gchar         *namespace_name;
  const gchar         *namespace_URI;
};

struct Namespaces namespaces_table[] = {
  {"gimp",     "http://www.gimp.org/ns/2.10/"                        },
  {"dwc",      "http://rs.tdwg.org/dwc/terms/"                       },
  {"lr",       "http://ns.adobe.com/lr/1.0/"                         },
  {"gpano",    "http://ns.google.com/photos/1.0/panorama/"           },
  {"panorama", "http://ns.adobe.com/photoshop/1.0/panorama-profile/" }

};

static gpointer                  gimp_attributes_parent_class = NULL;
static GimpAttribute            *current_attribute            = NULL;
static gboolean                  iter_initialized             = FALSE;


static GimpAttributes*         gimp_attributes_construct                   (GType                object_type);
static void                    gimp_attributes_finalize                    (GObject             *obj);
static void                    gimp_attributes_get_property                (GObject             *object,
                                                                            guint                property_id,
                                                                            GValue              *value,
                                                                            GParamSpec          *pspec);
static void                    gimp_attributes_set_property                (GObject             *object,
                                                                            guint                property_id,
                                                                            const GValue        *value,
                                                                            GParamSpec          *pspec);
static void                    gimp_attributes_deserialize_error           (GMarkupParseContext *context,
                                                                            GError              *error,
                                                                            gpointer             user_data);
static GQuark                  gimp_attributes_error_quark                 (void);
static void                    gimp_attributes_deserialize_start_element   (GMarkupParseContext *context,
                                                                            const gchar         *element_name,
                                                                            const gchar        **attribute_names,
                                                                            const gchar        **attribute_values,
                                                                            gpointer             user_data,
                                                                            GError             **error);
static void                    gimp_attributes_deserialize_text            (GMarkupParseContext  *context,
                                                                            const gchar          *text,
                                                                            gsize                 text_len,
                                                                            gpointer              user_data,
                                                                            GError              **error);
static void                    gimp_attributes_deserialize_end_element     (GMarkupParseContext *context,
                                                                            const gchar         *element_name,
                                                                            gpointer             user_data,
                                                                            GError             **error);
static const gchar*            gimp_attributes_name_to_value               (const gchar        **attribute_names,
                                                                            const gchar        **attribute_values,
                                                                            const gchar         *name);


static gboolean                has_xmp_structure                           (GSList              *xmp_list,
                                                                            const gchar         *entry);
static GimpAttribute *         gimp_attributes_get_attribute_sorted        (GimpAttributes      *attributes,
                                                                            const gchar         *sorted_name);

/**
 * gimp_attributes_new:
 *
 * returns a new #GimpAttributes object
 *
 * Return value:  a new @GimpAttributes object
 *
 * Since : 2.10
 */
GimpAttributes *
gimp_attributes_new (void)
{
  return gimp_attributes_construct (GIMP_TYPE_ATTRIBUTES);
}

/**
 * gimp_attributes_construct:
 *
 * @object_type: a #GType
 *
 * constructs a new #GimpAttributes object
 *
 * Return value:  a new @GimpAttributes
 *
 * Since : 2.10
 */
static GimpAttributes*
gimp_attributes_construct (GType object_type)
{
  GimpAttributes * attributes = NULL;

  attributes = (GimpAttributes*) g_object_new (object_type, NULL);
  return attributes;
}

/**
 * gimp_attributes_class_init:
 *
 * class initializer
 */
static void
gimp_attributes_class_init (GimpAttributesClass * klass)
{
  gimp_attributes_parent_class = g_type_class_peek_parent (klass);
  g_type_class_add_private (klass, sizeof(GimpAttributesPrivate));

  G_OBJECT_CLASS (klass)->get_property = gimp_attributes_get_property;
  G_OBJECT_CLASS (klass)->set_property = gimp_attributes_set_property;
  G_OBJECT_CLASS (klass)->finalize = gimp_attributes_finalize;
}

/**
 * gimp_attributes_instance_init:
 *
 * instance initializer
 */
static void
gimp_attributes_instance_init (GimpAttributes * attributes)
{
  attributes->priv = GIMP_ATTRIBUTES_GET_PRIVATE (attributes);
  attributes->priv->attribute_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
  attributes->priv->sorted_to_attribute = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  attributes->priv->sorted_key_list = NULL;
}

/**
 * gimp_attributes_finalize:
 *
 * instance finalizer
 */
static void
gimp_attributes_finalize (GObject* obj)
{
  GimpAttributes * attributes;

  attributes = G_TYPE_CHECK_INSTANCE_CAST (obj, GIMP_TYPE_ATTRIBUTES, GimpAttributes);

  g_hash_table_unref (attributes->priv->attribute_table);
  g_hash_table_unref (attributes->priv->sorted_to_attribute);
  g_list_free_full (attributes->priv->sorted_key_list, (GDestroyNotify) g_free);

  G_OBJECT_CLASS (gimp_attributes_parent_class)->finalize (obj);
}

/**
 * gimp_attributes_get_property
 *
 * instance get property
 */
static void
gimp_attributes_get_property (GObject * object,
                             guint property_id,
                             GValue * value,
                             GParamSpec * pspec)
{
}

/**
 * gimp_attributes_set_property
 *
 * instance set property
 */
static void
gimp_attributes_set_property (GObject * object,
                             guint property_id,
                             const GValue * value,
                             GParamSpec * pspec)
{
}

/**
 * gimp_attributes_get_type
 *
 * Return value: #GimpAttributes type
 */
GType
gimp_attributes_get_type (void)
{
  static volatile gsize gimp_attributes_type_id__volatile = 0;
  if (g_once_init_enter(&gimp_attributes_type_id__volatile))
    {
      static const GTypeInfo g_define_type_info =
            { sizeof(GimpAttributesClass),
                (GBaseInitFunc) NULL,
                (GBaseFinalizeFunc) NULL,
                (GClassInitFunc) gimp_attributes_class_init,
                (GClassFinalizeFunc) NULL,
                NULL,
                sizeof(GimpAttributes),
                0,
                (GInstanceInitFunc) gimp_attributes_instance_init,
                NULL
            };

      GType gimp_attributes_type_id;
      gimp_attributes_type_id = g_type_register_static (G_TYPE_OBJECT,
                                                       "GimpAttributes",
                                                       &g_define_type_info,
                                                       0);

      g_once_init_leave(&gimp_attributes_type_id__volatile,
                        gimp_attributes_type_id);
    }
  return gimp_attributes_type_id__volatile;
}

/**
 * gimp_attributes_size:
 *
 * @attributes: a #GimpAttributes
 *
 * Return value:  a #gint: amount of #GimpAttribute objects in
 * the #GimpAttributes container
 *
 * Since : 2.10
 */
gint
gimp_attributes_size (GimpAttributes *attributes)
{
  GimpAttributesPrivate *private = GIMP_ATTRIBUTES_GET_PRIVATE (attributes);

  return g_hash_table_size (private->attribute_table);
}

/**
 * gimp_attributes_duplicate:
 *
 * @attributes: a #GimpAttributes
 *
 * Duplicates the #GimpAttributes object with all the #GimpAttribute objects
 * Return value:  a copy of the @attributes object
 *
 * Since : 2.10
 */
GimpAttributes *
gimp_attributes_duplicate (GimpAttributes *attributes)
{
  GimpAttributes * new_attributes = NULL;

  g_return_val_if_fail (GIMP_IS_ATTRIBUTES (attributes), NULL);

  if (attributes)
    {
      gchar *xml;

      xml = gimp_attributes_serialize (attributes);
      new_attributes = gimp_attributes_deserialize (xml);
    }

  return new_attributes;
}

/**
 * gimp_attributes_get_table:
 *
 * @attributes: a #GimpAttributes
 *
 * Return value:  the #GHashTable
 *
 * Since : 2.10
 */
GHashTable *
gimp_attributes_get_table (GimpAttributes *attributes)
{
  return attributes->priv->attribute_table;
}

/**
 * gimp_attributes_add_attribute:
 *
 * @attributes: a #GimpAttributes
 * @attribute : a #GimpAttribute
 *
 * stores the @attribute in the @attributes container
 *
 * Since : 2.10
 */
void
gimp_attributes_add_attribute (GimpAttributes      *attributes,
                               GimpAttribute       *attribute)
{
  const gchar *name;
  gchar       *lowchar;

  g_return_if_fail (GIMP_IS_ATTRIBUTES (attributes));
  g_return_if_fail (GIMP_IS_ATTRIBUTE (attribute));

  name = gimp_attribute_get_name (attribute);

  if (name)
    {
        lowchar = g_ascii_strdown (name, -1);

      /* FIXME: really simply add? That means, that an older value is overwritten */

      if (g_hash_table_insert (attributes->priv->attribute_table, (gpointer) g_strdup (lowchar), (gpointer) attribute))
        {
          gchar *sortable_tag;

          sortable_tag = g_ascii_strdown (gimp_attribute_get_sortable_name (attribute), -1);

          if (g_hash_table_insert (attributes->priv->sorted_to_attribute, (gpointer) g_strdup (sortable_tag), (gpointer) g_strdup (lowchar)))
            {
              attributes->priv->sorted_key_list = g_list_insert_sorted (attributes->priv->sorted_key_list,
                                                                        (gpointer) g_strdup (sortable_tag),
                                                                        (GCompareFunc) g_strcmp0);
            }
          else
            {
              g_hash_table_remove (attributes->priv->attribute_table, (gpointer) g_strdup (lowchar));
            }

          g_free (sortable_tag);
        }
      g_free (lowchar);
    }
}

/**
 * gimp_attributes_get_attribute:
 *
 * @attributes: a #GimpAttributes
 * @name : a #gchar array
 *
 * gets the #GimpAttribute object with @name
 *
 * Return value: the #GimpAttribute object if found, NULL otherwise.
 *
 * Since : 2.10
 */
GimpAttribute *
gimp_attributes_get_attribute (GimpAttributes      *attributes,
                               const gchar         *name)
{
  gchar          *lowchar;
  GimpAttribute  *attribute_data = NULL;
  gpointer       *data;

  lowchar = g_ascii_strdown (name, -1);

  data = g_hash_table_lookup (attributes->priv->attribute_table, (gpointer) lowchar);
  if (data)
    {
      attribute_data = (GimpAttribute *) data;
    }

  g_free (lowchar);

  return attribute_data;
}

/**
 * gimp_attributes_get_attribute_sorted:
 *
 * @attributes: a #GimpAttributes
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
gimp_attributes_get_attribute_sorted (GimpAttributes      *attributes,
                                      const gchar         *sorted_name)
{
  gchar          *lowchar;
  GimpAttribute  *attribute_data = NULL;
  gpointer       *data;
  gchar          *name_of_tag;

  g_return_val_if_fail (GIMP_IS_ATTRIBUTES (attributes), NULL);
  g_return_val_if_fail (sorted_name != NULL, NULL);

  lowchar = g_ascii_strdown (sorted_name, -1);

  data = g_hash_table_lookup (attributes->priv->sorted_to_attribute, (gpointer) lowchar);

  if (data)
    {
      name_of_tag = (gchar *) data;

      data = g_hash_table_lookup (attributes->priv->attribute_table, (gpointer) name_of_tag);
      if (data)
        {
          attribute_data = (GimpAttribute *) data;
        }
    }

  g_free (lowchar);

  return attribute_data;
}

/**
 * gimp_attributes_remove_attribute:
 *
 * @attributes: a #GimpAttributes
 * @name : a #gchar array
 *
 * removes the #GimpAttribute object with @name from the @attributes container
 *
 * Return value: TRUE, if removing was successful, FALSE otherwise.
 *
 * Since : 2.10
 */
gboolean
gimp_attributes_remove_attribute (GimpAttributes      *attributes,
                                  const gchar         *name)
{
  gchar            *lowchar;
  gboolean          success       = FALSE;
  GHashTableIter    iter_remove;
  gpointer          key, value;
  gchar            *tag_to_remove = NULL;
  gchar            *name_of_tag   = NULL;

  lowchar = g_ascii_strdown (name, -1);

  if (g_hash_table_remove (attributes->priv->attribute_table, (gpointer) lowchar))
    {
      gchar *tag_list_remove = NULL;

      g_hash_table_iter_init (&iter_remove, attributes->priv->sorted_to_attribute);
      while (g_hash_table_iter_next (&iter_remove, &key, &value))
        {
          tag_to_remove  = (gchar *) key;
          name_of_tag    = (gchar *) value;

          if (! g_strcmp0 (lowchar, name_of_tag))
            break;
        }

      tag_list_remove = g_strdup (tag_to_remove); /* because removing from hashtable frees tag_to_remove */

      if (g_hash_table_remove (attributes->priv->sorted_to_attribute, (gpointer) tag_to_remove))
        {
          GList *list = NULL;

          for (list = attributes->priv->sorted_key_list; list; list = list->next)
            {
              gchar *s_tag = (gchar *) list->data;

              if (! g_strcmp0 (s_tag, tag_list_remove))
                {
                  attributes->priv->sorted_key_list = g_list_remove (attributes->priv->sorted_key_list,
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

  return success;

}

/**
 * gimp_attributes_has_attribute:
 *
 * @attributes: a #GimpAttributes
 * @name : a #gchar array
 *
 * tests, if a #GimpAttribute object with @name is in the @attributes container
 *
 * Return value: TRUE if yes, FALSE otherwise.
 *
 * Since : 2.10
 */
gboolean
gimp_attributes_has_attribute (GimpAttributes      *attributes,
                               const gchar         *name)
{
  gchar *lowchar;
  gboolean success;

  lowchar = g_ascii_strdown (name, -1);

  success = g_hash_table_contains (attributes->priv->attribute_table, (gpointer) lowchar);

  g_free (lowchar);

  return success;
}

/**
 * gimp_attributes_new_attribute:
 *
 * @attributes: a #GimpAttributes
 * @name : a #gchar array
 * @value: a #gchar array
 * @type : a #GimpAttributeValueType
 *
 * adds a #GimpAttribute object to @attributes container.
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
gimp_attributes_new_attribute (GimpAttributes *attributes,
                               const gchar    *name,
                               gchar          *value,
                               GimpAttributeValueType type)
{
  GimpAttribute *attribute;

  attribute = gimp_attribute_new_string (name, value, type);
  if (attribute)
    {
      gimp_attributes_add_attribute (attributes, attribute);
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_attributes_serialize:
 *
 * @attributes: a #GimpAttributes
 *
 * creates a xml representation of all #GimpAttribute objects in the #GimpAttributes container.
 * see #GimpAttribute:gimp_attribute_get_xml
 *
 * Return value: a new #gchar array, the xml representation of the #GimpAttributes object.
 *
 * Since : 2.10
 */
gchar *
gimp_attributes_serialize (GimpAttributes *attributes)
{
  GString        *string;
  GList          *key_list;

  g_return_val_if_fail (GIMP_IS_ATTRIBUTES (attributes), NULL);

  string = g_string_new (NULL);

  g_string_append (string, "<?xml version='1.0' encoding='UTF-8'?>\n");
  g_string_append (string, "<attributes>\n");

  for (key_list = attributes->priv->sorted_key_list; key_list; key_list = key_list->next)
    {
      gchar          *xml       = NULL;
      GimpAttribute  *attribute = NULL;
      gchar          *p_key = (gchar *) key_list->data;

      attribute      = gimp_attributes_get_attribute_sorted (attributes, p_key);

      xml = gimp_attribute_get_xml (attribute);

      g_string_append_printf (string, "%s\n", xml);

      g_free (xml);
    }

  g_string_append (string, "</attributes>\n");

  return g_string_free (string, FALSE);
}

/**
 * gimp_attributes_deserialize:
 *
 * @xml: a #gchar array
 *
 * parses a xml representation of a #GimpAttributes container.
 * see
 * #GimpAttributes:gimp_attributes_deserialize_start_element
 * #GimpAttributes:gimp_attributes_deserialize_end_element
 * #GimpAttributes:gimp_attributes_deserialize_text
 * #GimpAttributes:gimp_attributes_deserialize_error
 *
 * Return value: a new #GimpAttributes object.
 *
 * Since : 2.10
 */
GimpAttributes *
gimp_attributes_deserialize (const gchar *xml)
{
  GMarkupParser            *markup_parser = g_slice_new (GMarkupParser);
  GimpAttributesParseData  *parse_data = g_slice_new (GimpAttributesParseData);
  GMarkupParseContext      *context;
  GimpAttributes           *attributes;

  g_return_val_if_fail (xml != NULL, NULL);

  attributes = gimp_attributes_new ();

  parse_data->attributes = attributes;

  markup_parser->start_element = gimp_attributes_deserialize_start_element;
  markup_parser->end_element   = gimp_attributes_deserialize_end_element;
  markup_parser->text          = gimp_attributes_deserialize_text;
  markup_parser->passthrough   = NULL;
  markup_parser->error         = gimp_attributes_deserialize_error;

  context = g_markup_parse_context_new (markup_parser, 0, parse_data, NULL);

  g_markup_parse_context_parse (context,
                                xml, strlen (xml),
                                NULL);

  g_markup_parse_context_unref (context);

  g_slice_free (GMarkupParser, markup_parser);
  g_slice_free (GimpAttributesParseData, parse_data);

 return attributes;
}


/**
 * gimp_attributes_from_metadata:
 * @attributes:  The attributes the metadata will be added to, may be %NULL
 * @metadata:  The metadata in gexiv2 format
 *
 * Converts the @metadata retrieved from a file into
 * a #GimpAttributes object
 *
 * Return value: The #GimpAttributes object
 *
 * Since: GIMP 2.10
 */
GimpAttributes *
gimp_attributes_from_metadata (GimpAttributes *attributes, GimpMetadata *metadata)
{
  const gchar               *tag_type;
  GimpAttribute             *attribute;
  GimpAttributeValueType     attrib_type;
  gint                       i;
  GimpAttributes            *new_attributes    = NULL;

  g_return_val_if_fail (GEXIV2_IS_METADATA (metadata), NULL);

  if (!attributes)
    {
      new_attributes = gimp_attributes_new ();
    }
  else
    {
      new_attributes = gimp_attributes_duplicate (attributes);
      g_object_unref (attributes);
    }

  if (new_attributes)
    {
      gchar    **exif_data;
      gchar    **xmp_data;
      gchar    **iptc_data;
      gboolean   no_interpreted    = TRUE; /*FIXME: No interpreted String possible */

      exif_data = gexiv2_metadata_get_exif_tags (metadata);

      for (i = 0; exif_data[i] != NULL; i++)
        {
          gchar    *interpreted_value = NULL;
          gchar    *value             = NULL;
          gboolean  interpreted       = FALSE;

          value = gexiv2_metadata_get_tag_string (metadata, exif_data[i]);
          interpreted_value = gexiv2_metadata_get_tag_interpreted_string (metadata, exif_data[i]);
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
              gimp_attributes_add_attribute (new_attributes, attribute);
            }
          else
            {
              g_object_unref (attribute);
            }

          g_free (interpreted_value);
          g_free (value);
        }

      g_strfreev (exif_data);

      xmp_data = gexiv2_metadata_get_xmp_tags (metadata);

      for (i = 0; xmp_data[i] != NULL; i++)
        {
          gchar    *interpreted_value = NULL;
          gchar    *value             = NULL;
          gboolean  interpreted       = FALSE;

          value = gexiv2_metadata_get_tag_string (metadata, xmp_data[i]);
          interpreted_value = gexiv2_metadata_get_tag_interpreted_string (metadata, xmp_data[i]);
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
              gimp_attributes_add_attribute (new_attributes, attribute);
            }
          else
            {
              g_object_unref (attribute);
            }

          g_free (value);
        }

      g_strfreev (xmp_data);

      iptc_data = gexiv2_metadata_get_iptc_tags (metadata);

      for (i = 0; iptc_data[i] != NULL; i++)
        {
          gchar    *interpreted_value = NULL;
          gchar    *value             = NULL;
          gboolean  interpreted       = FALSE;

          value = gexiv2_metadata_get_tag_string (metadata, iptc_data[i]);
          interpreted_value = gexiv2_metadata_get_tag_interpreted_string (metadata, iptc_data[i]);
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
              gimp_attributes_add_attribute (new_attributes, attribute);
            }
          else
            {
              g_object_unref (attribute);
            }

          g_free (value);
        }

      g_strfreev (iptc_data);
    }
  return new_attributes;
}

/**
 * gimp_attributes_print:
 * @attributes:  The #GimpAttributes
 *
 * prints out information of attributes
 *
 * Since: GIMP 2.10
 */
void
gimp_attributes_print (GimpAttributes *attributes)
{
  gint            i;
  GList          *key_list = NULL;


  g_return_if_fail (GIMP_IS_ATTRIBUTES (attributes));

  i = 0;

  for (key_list = attributes->priv->sorted_key_list; key_list; key_list = key_list->next)
    {
      const gchar    *tag;
      const gchar    *interpreted;
      gchar          *value;
      GimpAttribute  *attribute;
      gchar          *p_key = (gchar *) key_list->data;

      attribute      = gimp_attributes_get_attribute_sorted (attributes, p_key);

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
 * gimp_attributes_to_metadata:
 * @attributes:  The #GimpAttributes
 * @metadata:  The #GimpMetadata
 * @mime_type: the mime type of the image
 *
 * Converts @attributes to @metadata in gexiv2 format
 *
 * Since: GIMP 2.10
 */
void
gimp_attributes_to_metadata (GimpAttributes *attributes,
                             GimpMetadata *metadata,
                             const gchar *mime_type)
{
  gchar          *o_packet            = NULL;
  gboolean        write_tag           = FALSE;
  gboolean        namespace           = FALSE;
  gboolean        check_mime          = TRUE;
  gboolean        support_exif;
  gboolean        support_xmp;
  gboolean        support_iptc;
  GSList         *xmp_structure_list  = NULL;
  GList          *key_list            = NULL;
  gint            i;


  g_return_if_fail (GIMP_IS_ATTRIBUTES (attributes));
  g_return_if_fail (GEXIV2_IS_METADATA (metadata));

  for (i = 0; i < G_N_ELEMENTS (namespaces_table); i++)
    {
      struct Namespaces n_space = namespaces_table[i];
      gexiv2_metadata_register_xmp_namespace (n_space.namespace_URI, n_space.namespace_name);
    }

  support_exif = gexiv2_metadata_get_supports_exif (metadata);
  support_xmp  = gexiv2_metadata_get_supports_xmp  (metadata);
  support_iptc = gexiv2_metadata_get_supports_iptc (metadata);

  for (key_list = attributes->priv->sorted_key_list; key_list; key_list = key_list->next)
    {
      const gchar              *tag;
      const gchar              *ns_name;
      gchar                    *p_key = (gchar *) key_list->data;
      gchar                    *value         = NULL;
      gchar                    *category      = NULL;
      gboolean                  is_xmp        = FALSE;
      gboolean                  has_structure = FALSE;
      GimpAttribute            *attribute;
      GimpAttributeValueType    tag_value_type;

      write_tag      = FALSE;
      namespace      = FALSE;

      attribute      = gimp_attributes_get_attribute_sorted (attributes, p_key);

      if (! attribute)
        continue;

      tag            = gimp_attribute_get_name (attribute);
      has_structure  = gimp_attribute_has_structure (attribute);

      if (mime_type)
        check_mime = gimp_metadata_is_tag_supported (tag, mime_type);

      if (check_mime)
        {
          gchar *t_packet       = NULL;

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
                          gboolean       has_tag = gexiv2_metadata_has_tag (metadata, structure_element);

                          if (!has_tag && ! has_xmp_structure (xmp_structure_list, structure_element))
                            {
                              switch (structure_type)
                              {
                                case STRUCTURE_TYPE_ALT:
                                  success = gexiv2_metadata_set_xmp_tag_struct (metadata, structure_element, GEXIV2_STRUCTURE_XA_ALT); /*start block*/
                                  break;
                                case STRUCTURE_TYPE_BAG:
                                  success = gexiv2_metadata_set_xmp_tag_struct (metadata, structure_element, GEXIV2_STRUCTURE_XA_BAG); /*start block*/
                                  break;
                                case STRUCTURE_TYPE_SEQ:
                                  success = gexiv2_metadata_set_xmp_tag_struct (metadata, structure_element, GEXIV2_STRUCTURE_XA_SEQ); /*start block*/
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
                        gexiv2_metadata_set_tag_string (metadata, tag, value);
                      }
                      break;
                    case TYPE_MULTIPLE:
                      {
                        GValue h;
                        gchar **values;

                        h = gimp_attribute_get_value (attribute);
                        values = (gchar **) g_value_get_boxed (&h);
                        gexiv2_metadata_set_tag_multiple (metadata, tag, (const gchar **) values);
                        g_strfreev  (values);
                      }
                      break;
                    case TYPE_UNKNOWN:
                    default:
                      break;

                  }

                  if (is_xmp)
                    {
                      t_packet = gexiv2_metadata_generate_xmp_packet (metadata, GEXIV2_USE_COMPACT_FORMAT | GEXIV2_OMIT_ALL_FORMATTING, 0);

                      if (! g_strcmp0 (t_packet, o_packet))
                        {
                          gexiv2_metadata_clear_tag (metadata, tag);
                          g_print ("cleared to metadata:\n%s, %s\n", tag, value);
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
    }

  if (o_packet)
    g_free (o_packet);
  if (xmp_structure_list)
    g_slist_free (xmp_structure_list);
}


#if 0
/**
 * gimp_attributes_to_xmp_packet:
 * @attributes:  The #GimpAttributes
 * @mime_type :  a mime_type
 *
 * placeholder, until gexiv2 can generate xmp packet.
 * always returns NULL
 *
 * Since: GIMP 2.10
 */
const gchar *
gimp_attributes_to_xmp_packet (GimpAttributes *attributes,
                               const gchar *mime_type)

{
  return NULL;
}
#endif

/**
 * gimp_attributes_to_xmp_packet:
 * @attributes:  The #GimpAttributes
 * @mime_type :  a mime_type
 *
 * Converts @attributes to a xmp packet
 * It looks like an ugly hack, but let
 * gexiv2/exiv2 do all the hard work.
 *
 * Return value: a #gchar*, representing a xml packet.
 *
 * Since: GIMP 2.10
 */
const gchar *
gimp_attributes_to_xmp_packet (GimpAttributes *attributes,
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
  GimpMetadata   *metadata;
  GSList         *xmp_structure_list = NULL;
  GList          *key_list           = NULL;

  g_return_val_if_fail (GIMP_IS_ATTRIBUTES (attributes), NULL);

  metadata = gimp_metadata_new ();

  for (i = 0; i < G_N_ELEMENTS (namespaces_table); i++)
    {
      struct Namespaces n_space = namespaces_table[i];
      gexiv2_metadata_register_xmp_namespace (n_space.namespace_URI, n_space.namespace_name);
    }

  support_exif = gexiv2_metadata_get_supports_exif (metadata);
  support_xmp  = gexiv2_metadata_get_supports_xmp  (metadata);
  support_iptc = gexiv2_metadata_get_supports_iptc (metadata);

  for (key_list = attributes->priv->sorted_key_list; key_list; key_list = key_list->next)
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

      attribute = gimp_attributes_get_attribute_sorted (attributes, p_key);

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
                          gboolean       has_tag = gexiv2_metadata_has_tag (metadata, structure_element);

                          if (!has_tag && ! has_xmp_structure (xmp_structure_list, structure_element))
                            {
                              switch (structure_type)
                              {
                                case STRUCTURE_TYPE_ALT:
                                  success = gexiv2_metadata_set_xmp_tag_struct (metadata, structure_element, GEXIV2_STRUCTURE_XA_ALT); /*start block*/
                                  break;
                                case STRUCTURE_TYPE_BAG:
                                  success = gexiv2_metadata_set_xmp_tag_struct (metadata, structure_element, GEXIV2_STRUCTURE_XA_BAG); /*start block*/
                                  break;
                                case STRUCTURE_TYPE_SEQ:
                                  success = gexiv2_metadata_set_xmp_tag_struct (metadata, structure_element, GEXIV2_STRUCTURE_XA_SEQ); /*start block*/
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
                        gexiv2_metadata_set_tag_string (metadata, new_tag, value);
                        if (sec_tag)
                          {
                            gexiv2_metadata_set_tag_string (metadata, sec_tag, value);
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
                        gexiv2_metadata_set_tag_multiple (metadata, new_tag, (const gchar **) values);
                        if (sec_tag)
                          {
                            gexiv2_metadata_set_tag_multiple (metadata, sec_tag, (const gchar **) values);
                            g_free (sec_tag);
                          }
                        g_strfreev  (values);
                      }
                      break;
                    case TYPE_UNKNOWN:
                    default:
                      break;

                  }

                  t_packet = gexiv2_metadata_generate_xmp_packet (metadata, GEXIV2_USE_COMPACT_FORMAT | GEXIV2_OMIT_ALL_FORMATTING, 0);

                  if (! t_packet || ! g_strcmp0 (t_packet, o_packet))
                    {
                      gexiv2_metadata_clear_tag (metadata, new_tag);
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

  packet_string = gexiv2_metadata_generate_xmp_packet (metadata, GEXIV2_USE_COMPACT_FORMAT | GEXIV2_WRITE_ALIAS_COMMENTS, 0);
  return packet_string;
}

/**
 * gimp_attributes_has_tag_type:
 * @attributes:  The attributes
 * @tag_type:  The #GimpAttributeTagType to test
 *
 * tests, if @attributes contains at least one tag of @tag_type
 *
 * Return value: TRUE if found, FALSE otherwise
 *
 * Since: GIMP 2.10
 */
gboolean
gimp_attributes_has_tag_type (GimpAttributes       *attributes,
                              GimpAttributeTagType  tag_type)
{
  GHashTableIter  iter;
  gpointer        p_key, p_value;

  g_return_val_if_fail (GIMP_IS_ATTRIBUTES (attributes), FALSE);

  g_hash_table_iter_init (&iter, attributes->priv->attribute_table);

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
gimp_attributes_iter_init (GimpAttributes *attributes, GList **iter)
{
  g_return_val_if_fail (GIMP_IS_ATTRIBUTES (attributes), NULL);

  *iter = attributes->priv->sorted_key_list;
  iter_initialized = TRUE;

  return attributes->priv->sorted_key_list;
}

gboolean
gimp_attributes_iter_next (GimpAttributes *attributes, GimpAttribute **attribute, GList **prev)
{
  gchar         *sorted;
  GList         *tmp;

  g_return_val_if_fail (GIMP_IS_ATTRIBUTES (attributes), FALSE);

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

      *attribute = gimp_attributes_get_attribute_sorted (attributes, sorted);
    }

  iter_initialized = FALSE;

  if (*attribute)
    return TRUE;
  else
    return FALSE;

}

/**
 * gimp_attributes_error_quark:
 *
 * Return value: #GQuark
 *
 * Since: GIMP 2.10
 */
static GQuark
gimp_attributes_error_quark (void)
{
  static GQuark quark = 0;

  if (G_UNLIKELY (quark == 0))
    quark = g_quark_from_static_string ("gimp-attributes-error-quark");

  return quark;
}

/**
 * gimp_attributes_deserialize_error:
 *
 * Error while parsing
 *
 * Since: GIMP 2.10
 */
static void
gimp_attributes_deserialize_error (GMarkupParseContext *context,
                                   GError              *error,
                                   gpointer             user_data)
{
  g_printerr ("XML parse error: %s\n", error->message);
}

/**
 * gimp_attributes_name_to_value:
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
gimp_attributes_name_to_value (const gchar **attribute_names,
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
 * gimp_attributes_deserialize_start_element:
 *
 * @context           : #GMarkupParseContext
 * @element_name      : #gchar *
 * @attribute_names   : #gchar **
 * @attribute_values  : #gchar **
 * @user_data         : #gpointer to #GimpAttributesParseData struct
 * @error             : #GError **
 *
 * start of a tag (parsing xml)
 *
 * Since: GIMP 2.10
 */
static void
gimp_attributes_deserialize_start_element (GMarkupParseContext *context,
                                           const gchar         *element_name,
                                           const gchar        **attribute_names,
                                           const gchar        **attribute_values,
                                           gpointer             user_data,
                                           GError             **error)
{
  GimpAttributesParseData *parse_data = user_data;

  if (! strcmp (element_name, "tag"))
    {
      const gchar *name = NULL;
      const gchar *type = NULL;

      name     = gimp_attributes_name_to_value (attribute_names,
                                                attribute_values,
                                                "name");
      type     = gimp_attributes_name_to_value (attribute_names,
                                                attribute_values,
                                                "type");

      if (! name)
        {
          g_set_error (error, gimp_attributes_error_quark (), 1001,
                       "Element 'tag' does not contain required attribute 'name'.");
          return;
        }

      if (! type)
        {
          g_set_error (error, gimp_attributes_error_quark (), 1001,
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

      encoding = gimp_attributes_name_to_value (attribute_names,
                                                attribute_values,
                                                "encoding");

      parse_data->base64 = (encoding && ! strcmp (encoding, "base64"));
    }
  else if (! strcmp (element_name, "interpreted"))
    {
      const gchar *encoding = NULL;

      encoding = gimp_attributes_name_to_value (attribute_names,
                                                attribute_values,
                                                "encoding");

      parse_data->base64 = (encoding && ! strcmp (encoding, "base64"));
    }

}

/**
 * gimp_attributes_deserialize_text:
 *
 * @context           : #GMarkupParseContext
 * @text              : const #gchar *
 * @text_len          : #gsize
 * @user_data         : #gpointer to #GimpAttributesParseData struct
 * @error             : #GError **
 *
 * text of a tag (parsing xml)
 *
 * Since: GIMP 2.10
 */
static void
gimp_attributes_deserialize_text (GMarkupParseContext  *context,
                                  const gchar          *text,
                                  gsize                 text_len,
                                  gpointer              user_data,
                                  GError              **error)
{
  GimpAttributesParseData *parse_data       = user_data;
  GimpAttribute           *attribute        = NULL;
  const gchar             *current_element;

  current_element = g_markup_parse_context_get_element (context);

  if (! g_strcmp0 (current_element, "value"))
    {
      gchar *value = g_strndup (text, text_len);

//      if (g_str_has_prefix (parse_data->name, "Exif.GPSInfo"))
//      if (! g_strcmp0 (private->name, "Exif.Image.ResolutionUnit"))
//        {
//          g_print ("found: %s\n", parse_data->name);
//        }

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
          gimp_attributes_add_attribute (parse_data->attributes, attribute);
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
 * gimp_attributes_deserialize_end_element:
 *
 * @context           : #GMarkupParseContext
 * @element_name      : #gchar *
 * @user_data         : #gpointer to #GimpAttributesParseData struct
 * @error             : #GError **
 *
 * end of a tag (parsing xml)
 *
 * Since: GIMP 2.10
 */
static void
gimp_attributes_deserialize_end_element (GMarkupParseContext *context,
                                         const gchar         *element_name,
                                         gpointer             user_data,
                                         GError             **error)
{
  GimpAttributesParseData *parse_data = user_data;

  if (! strcmp (element_name, "tag"))
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
