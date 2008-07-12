/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptagcache.c
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
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

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpdata.h"
#include "gimptagcache.h"
#include "gimplist.h"

#include "gimp-intl.h"

#define GIMP_TAG_CACHE_FILE     ".tag-cache.xml"


typedef struct _ParseData ParseData;

struct _ParseData
{
  GArray               *records;
  GimpTagCacheRecord    current_record;
};


static void    gimp_tag_cache_finalize          (GObject              *object);

static gint64  gimp_tag_cache_get_memsize       (GimpObject           *object,
                                                 gint64               *gui_size);
static void    gimp_tag_cache_object_initialize (GimpTagged           *tagged,
                                                 GimpTagCache         *cache);
static void    gimp_tag_cache_object_add        (GimpContainer        *container,
                                                 GimpTagged           *tagged,
                                                 GimpTagCache         *cache);

static void    gimp_tag_cache_load_start_element(GMarkupParseContext *context,
                                                  const gchar         *element_name,
                                                  const gchar        **attribute_names,
                                                  const gchar        **attribute_values,
                                                  gpointer             user_data,
                                                  GError             **error);
static void    gimp_tag_cache_load_end_element  (GMarkupParseContext *context,
                                                 const gchar         *element_name,
                                                 gpointer             user_data,
                                                 GError             **error);
static void    gimp_tag_cache_load_text         (GMarkupParseContext *context,
                                                 const gchar         *text,
                                                 gsize                text_len,
                                                 gpointer             user_data,
                                                 GError             **error);
static  void   gimp_tag_cache_load_error        (GMarkupParseContext *context,
                                                 GError              *error,
                                                 gpointer             user_data);

static const gchar*  attribute_name_to_value    (const gchar  **attribute_names,
                                                 const gchar  **attribute_values,
                                                 const gchar   *name);

static GQuark  gimp_tag_cache_get_error_domain  (void);


G_DEFINE_TYPE (GimpTagCache, gimp_tag_cache, GIMP_TYPE_OBJECT)

#define parent_class gimp_tag_cache_parent_class


static void
gimp_tag_cache_class_init (GimpTagCacheClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  object_class->finalize         = gimp_tag_cache_finalize;

  gimp_object_class->get_memsize = gimp_tag_cache_get_memsize;
}

static void
gimp_tag_cache_init (GimpTagCache *cache)
{
  cache->gimp                   = NULL;

  cache->records                = NULL;
  cache->containers             = NULL;
}

static void
gimp_tag_cache_finalize (GObject *object)
{
  GimpTagCache *cache = GIMP_TAG_CACHE (object);
  gint          i;

  if (cache->records)
    {
      for (i = 0; i < cache->records->len; i++)
        {
          GimpTagCacheRecord *rec =
              &g_array_index (cache->records, GimpTagCacheRecord, i);
          g_list_free (rec->tags);
        }
      g_array_free (cache->records, TRUE);
      cache->records = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_tag_cache_get_memsize (GimpObject *object,
                            gint64     *gui_size)
{
  /*GimpTagCache *cache = GIMP_TAG_CACHE (object);*/
  gint64           memsize = 0;

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

GimpTagCache *
gimp_tag_cache_new (Gimp       *gimp)
{
  GimpTagCache *cache;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  cache = g_object_new (GIMP_TYPE_TAG_CACHE, NULL);

  cache->gimp                   = gimp;
  cache->records                = g_array_new (FALSE, FALSE, sizeof(GimpTagCacheRecord));

  return cache;
}

void
gimp_tag_cache_add_container (GimpTagCache     *cache,
                              GimpContainer    *container)
{
  cache->containers = g_list_append (cache->containers, container);
  gimp_container_foreach (container, (GFunc) gimp_tag_cache_object_initialize, cache);

  g_signal_connect (container, "add",
                    G_CALLBACK (gimp_tag_cache_object_add), cache);
}

static void
gimp_tag_cache_object_add (GimpContainer       *container,
                           GimpTagged          *tagged,
                           GimpTagCache        *cache)
{
  const char           *identifier;
  GQuark                identifier_quark = 0;
  gchar                *checksum_string;
  GQuark                checksum_quark;
  GList                *tag_iterator;
  gint                  i;

  identifier = gimp_tagged_get_identifier (tagged);

  if (identifier
      && !gimp_tagged_get_tags (tagged))
    {
      identifier_quark = g_quark_try_string (identifier);
      if (identifier_quark)
        {
          for (i = 0; i < cache->records->len; i++)
            {
              GimpTagCacheRecord *rec = &g_array_index (cache->records,
                                                        GimpTagCacheRecord, i);

              if (rec->identifier == identifier_quark)
                {
                  tag_iterator = rec->tags;
                  while (tag_iterator)
                    {
                      printf ("assigning cached tag: %s to %s\n",
                              g_quark_to_string (GPOINTER_TO_UINT (tag_iterator->data)),
                              identifier);
                      gimp_tagged_add_tag (tagged, GPOINTER_TO_UINT (tag_iterator->data));
                      tag_iterator = g_list_next (tag_iterator);
                    }
                  rec->referenced = TRUE;
                  return;
                }
            }
        }

      checksum_string = gimp_tagged_get_checksum (tagged);
      checksum_quark = g_quark_try_string (checksum_string);
      g_free (checksum_string);

      if (checksum_quark)
        {
          for (i = 0; i < cache->records->len; i++)
            {
              GimpTagCacheRecord *rec = &g_array_index (cache->records,
                                                        GimpTagCacheRecord, i);

              if (rec->checksum == checksum_quark)
                {
                  printf ("remapping identifier: %s ==> %s\n",
                          g_quark_to_string (rec->identifier), identifier);
                  rec->identifier = g_quark_from_string (identifier);

                  tag_iterator = rec->tags;
                  while (tag_iterator)
                    {
                      printf ("assigning cached tag: %s to %s\n",
                              g_quark_to_string (GPOINTER_TO_UINT (tag_iterator->data)),
                              identifier);
                      gimp_tagged_add_tag (tagged, GPOINTER_TO_UINT (tag_iterator->data));
                      tag_iterator = g_list_next (tag_iterator);
                    }
                  rec->referenced = TRUE;
                  return;
                }
            }
        }
    }
}

static void
gimp_tag_cache_object_initialize (GimpTagged          *tagged,
                                  GimpTagCache        *cache)
{
  gimp_tag_cache_object_add (NULL, tagged, cache);
}

static void
tagged_to_cache_record_foreach (GimpTagged     *tagged,
                                GList         **cache_records)
{
  const char           *identifier;
  gchar                *checksum;
  GimpTagCacheRecord   *cache_rec;

  identifier = gimp_tagged_get_identifier (tagged);
  if (identifier)
    {
      cache_rec = (GimpTagCacheRecord*) g_malloc (sizeof (GimpTagCacheRecord));
      cache_rec->identifier = g_quark_from_string (identifier);
      checksum = gimp_tagged_get_checksum (tagged);
      cache_rec->checksum = g_quark_from_string (checksum);
      g_free (checksum);
      cache_rec->tags = g_list_copy (gimp_tagged_get_tags (tagged));
      *cache_records = g_list_append (*cache_records, cache_rec);
    }
}

void
gimp_tag_cache_save (GimpTagCache      *cache)
{
  GString      *buf;
  GList        *saved_records;
  GList        *iterator;
  gchar        *filename;
  GError       *error;
  gint          i;

  printf ("saving cache to disk ...\n");

  saved_records = NULL;
  for (i = 0; i < cache->records->len; i++)
    {
      GimpTagCacheRecord *current_record = &g_array_index(cache->records, GimpTagCacheRecord, i);
      if (! current_record->referenced
          && current_record->tags)
        {
          /* keep tagged objects which have tags assigned
           * but were not loaded. */
          GimpTagCacheRecord *record_copy = (GimpTagCacheRecord*) g_malloc (sizeof (GimpTagCacheRecord));
          record_copy->identifier = current_record->identifier;
          record_copy->checksum = current_record->checksum;
          record_copy->tags = g_list_copy (current_record->tags);
          saved_records = g_list_append (saved_records, record_copy);
        }
    }

  iterator = cache->containers;
  while (iterator)
    {
      gimp_container_foreach (GIMP_CONTAINER (iterator->data),
                              (GFunc) tagged_to_cache_record_foreach, &saved_records);
      iterator = g_list_next (iterator);
    }

  buf = g_string_new ("");
  g_string_append (buf, "<?xml version='1.0' encoding='UTF-8'?>\n");
  g_string_append (buf, "<tag_cache>\n");
  iterator = saved_records;
  while (iterator)
    {
      GimpTagCacheRecord *cache_rec = (GimpTagCacheRecord*) iterator->data;
      GList              *tag_iterator;

      g_string_append_printf (buf, "\t<resource identifier=\"%s\" checksum=\"%s\">\n",
                              g_quark_to_string (cache_rec->identifier),
                              g_quark_to_string (cache_rec->checksum));
      tag_iterator = cache_rec->tags;
      while (tag_iterator)
        {
          g_string_append_printf (buf, "\t\t<tag>%s</tag>\n",
                                  g_quark_to_string (GPOINTER_TO_UINT (tag_iterator->data)));
          tag_iterator = g_list_next (tag_iterator);
        }
      g_string_append (buf, "\t</resource>\n");
      iterator = g_list_next (iterator);
    }
  g_string_append (buf, "</tag_cache>\n");

  filename = g_build_filename (gimp_directory (), GIMP_TAG_CACHE_FILE, NULL);
  printf ("writing tag cache to %s\n", filename);
  error = NULL;
  if (!g_file_set_contents (filename, buf->str, buf->len, &error))
    {
      printf ("Error while saving tag cache: %s\n", error->message);
      g_error_free (error);
    }

  g_free (filename);
  g_string_free (buf, TRUE);

  iterator = saved_records;
  while (iterator)
    {
      GimpTagCacheRecord *cache_rec = (GimpTagCacheRecord*) iterator->data;
      g_list_free (cache_rec->tags);
      g_free (cache_rec);
      iterator = g_list_next (iterator);
    }
  g_list_free (saved_records);
}

void
gimp_tag_cache_load (GimpTagCache      *cache)
{
  gchar                *filename;
  gchar                *buffer;
  gsize                 length;
  GError               *error;
  GMarkupParser         markup_parser;
  GMarkupParseContext  *parse_context;
  ParseData             parse_data;

  /* clear any previous records */
  cache->records = g_array_set_size (cache->records, 0);

  filename = g_build_filename (gimp_directory (), GIMP_TAG_CACHE_FILE, NULL);
  printf ("reading tag cache to %s\n", filename);
  error = NULL;
  if (!g_file_get_contents (filename, &buffer, &length, &error))
    {
      printf ("Error while reading tag cache: %s\n", error->message);
      g_error_free (error);
      g_free (filename);
      return;
    }
  g_free (filename);


  parse_data.records = g_array_new (FALSE, FALSE, sizeof (GimpTagCacheRecord));
  memset (&parse_data.current_record, 0, sizeof (GimpTagCacheRecord));

  markup_parser.start_element = gimp_tag_cache_load_start_element;
  markup_parser.end_element = gimp_tag_cache_load_end_element;
  markup_parser.text = gimp_tag_cache_load_text;
  markup_parser.passthrough = NULL;
  markup_parser.error = gimp_tag_cache_load_error;
  parse_context = g_markup_parse_context_new (&markup_parser, 0,
                                              &parse_data, NULL);
  if (! g_markup_parse_context_parse (parse_context, buffer, length, &error))
    {
      printf ("Failed to parse tag cache.\n");
    }
  else
    {
      cache->records = g_array_append_vals (cache->records,
                                            parse_data.records->data,
                                            parse_data.records->len);
    }

  g_array_free (parse_data.records, TRUE);
  g_free (buffer);
}

static  void
gimp_tag_cache_load_start_element  (GMarkupParseContext *context,
                                    const gchar         *element_name,
                                    const gchar        **attribute_names,
                                    const gchar        **attribute_values,
                                    gpointer             user_data,
                                    GError             **error)
{
  ParseData            *parse_data = (ParseData*) user_data;

  if (! strcmp (element_name, "resource"))
    {
      const gchar      *identifier;
      const gchar      *checksum;

      identifier = attribute_name_to_value (attribute_names, attribute_values,
                                            "identifier");
      checksum   = attribute_name_to_value (attribute_names, attribute_values,
                                            "checksum");

      if (! identifier)
        {
          GQuark domain;
          g_set_error (error, domain, 1001, "Resource tag does not contain required attribute identifier.");
          return;
        }

      memset (&parse_data->current_record, 0, sizeof (GimpTagCacheRecord));

      parse_data->current_record.identifier = g_quark_from_string (identifier);
      parse_data->current_record.checksum   = g_quark_from_string (checksum);
    }
}

static void
gimp_tag_cache_load_end_element (GMarkupParseContext *context,
                                 const gchar         *element_name,
                                 gpointer             user_data,
                                 GError             **error)
{
  ParseData            *parse_data = (ParseData*) user_data;

  if (! strcmp (element_name, "resource"))
    {
      parse_data->records = g_array_append_val (parse_data->records,
                                                parse_data->current_record);
      memset (&parse_data->current_record, 0, sizeof (GimpTagCacheRecord));
    }
}

static void
gimp_tag_cache_load_text (GMarkupParseContext *context,
                          const gchar         *text,
                          gsize                text_len,
                          gpointer             user_data,
                          GError             **error)
{
  ParseData            *parse_data = (ParseData*) user_data;
  const gchar          *current_element;
  gchar                 buffer[2048];
  GQuark                tag_quark;

  current_element = g_markup_parse_context_get_element (context);
  if (current_element
      && ! strcmp (current_element, "tag"))
    {
      if (text_len >= sizeof (buffer))
        {
          g_set_error (error, gimp_tag_cache_get_error_domain (), 1002,
                       "Tag value is too long.");
          return;
        }

      memcpy (buffer, text, text_len);
      buffer[text_len] = '\0';

      printf ("assigning tag %s to %s\n", buffer,
              g_quark_to_string (parse_data->current_record.identifier));

      tag_quark = g_quark_from_string (buffer);
      parse_data->current_record.tags = g_list_append (parse_data->current_record.tags,
                                                        GUINT_TO_POINTER (tag_quark));
    }
}

static  void
gimp_tag_cache_load_error (GMarkupParseContext *context,
                           GError              *error,
                           gpointer             user_data)
{
  printf ("Tag cache parse error: %s\n", error->message);
}

static const gchar*
attribute_name_to_value (const gchar  **attribute_names,
                         const gchar  **attribute_values,
                         const gchar   *name)
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

static GQuark
gimp_tag_cache_get_error_domain (void)
{
  return g_quark_from_static_string ("gimp-tag-cache-error-quark");
}


