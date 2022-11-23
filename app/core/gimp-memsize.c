/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmacolor/ligmacolor.h"

#include "core-types.h"

#include "ligma-memsize.h"
#include "ligmaparamspecs.h"


gint64
ligma_g_type_instance_get_memsize (GTypeInstance *instance)
{
  if (instance)
    {
      GTypeQuery type_query;

      g_type_query (G_TYPE_FROM_INSTANCE (instance), &type_query);

      return type_query.instance_size;
    }

  return 0;
}

gint64
ligma_g_object_get_memsize (GObject *object)
{
  if (object)
    return ligma_g_type_instance_get_memsize ((GTypeInstance *) object);

  return 0;
}

gint64
ligma_g_hash_table_get_memsize (GHashTable *hash,
                               gint64      data_size)
{
  if (hash)
    return (2 * sizeof (gint) +
            5 * sizeof (gpointer) +
            g_hash_table_size (hash) * (3 * sizeof (gpointer) + data_size));

  return 0;
}

typedef struct
{
  LigmaMemsizeFunc func;
  gint64          memsize;
  gint64          gui_size;
} HashMemsize;

static void
hash_memsize_foreach (gpointer     key,
                      gpointer     value,
                      HashMemsize *memsize)
{
  gint64 gui_size = 0;

  memsize->memsize  += memsize->func (value, &gui_size);
  memsize->gui_size += gui_size;
}

gint64
ligma_g_hash_table_get_memsize_foreach (GHashTable      *hash,
                                       LigmaMemsizeFunc  func,
                                       gint64          *gui_size)
{
  HashMemsize memsize;

  g_return_val_if_fail (func != NULL, 0);

  if (! hash)
    return 0;

  memsize.func     = func;
  memsize.memsize  = 0;
  memsize.gui_size = 0;

  g_hash_table_foreach (hash, (GHFunc) hash_memsize_foreach, &memsize);

  if (gui_size)
    *gui_size = memsize.gui_size;

  return memsize.memsize + ligma_g_hash_table_get_memsize (hash, 0);
}

gint64
ligma_g_slist_get_memsize (GSList  *slist,
                          gint64   data_size)
{
  return g_slist_length (slist) * (data_size + sizeof (GSList));
}

gint64
ligma_g_slist_get_memsize_foreach (GSList          *slist,
                                  LigmaMemsizeFunc  func,
                                  gint64          *gui_size)
{
  GSList *l;
  gint64  memsize = 0;

  g_return_val_if_fail (func != NULL, 0);

  for (l = slist; l; l = g_slist_next (l))
    memsize += sizeof (GSList) + func (l->data, gui_size);

  return memsize;
}

gint64
ligma_g_list_get_memsize (GList  *list,
                         gint64  data_size)
{
  return g_list_length (list) * (data_size + sizeof (GList));
}

gint64
ligma_g_list_get_memsize_foreach (GList           *list,
                                 LigmaMemsizeFunc  func,
                                 gint64          *gui_size)
{
  GList  *l;
  gint64  memsize = 0;

  g_return_val_if_fail (func != NULL, 0);

  for (l = list; l; l = g_list_next (l))
    memsize += sizeof (GList) + func (l->data, gui_size);

  return memsize;
}

gint64
ligma_g_queue_get_memsize (GQueue *queue,
                          gint64  data_size)
{
  if (queue)
    {
      return sizeof (GQueue) +
             g_queue_get_length (queue) * (data_size + sizeof (GList));
    }

  return 0;
}

gint64
ligma_g_queue_get_memsize_foreach (GQueue          *queue,
                                  LigmaMemsizeFunc  func,
                                  gint64          *gui_size)
{
  gint64 memsize = 0;

  g_return_val_if_fail (func != NULL, 0);

  if (queue)
    {
      GList *l;

      memsize = sizeof (GQueue);

      for (l = queue->head; l; l = g_list_next (l))
        memsize += sizeof (GList) + func (l->data, gui_size);
    }

  return memsize;
}

gint64
ligma_g_value_get_memsize (GValue *value)
{
  gint64 memsize = 0;

  if (! value)
    return 0;

  if (G_VALUE_HOLDS_STRING (value))
    {
      memsize += ligma_string_get_memsize (g_value_get_string (value));
    }
  else if (G_VALUE_HOLDS_BOXED (value))
    {
      if (LIGMA_VALUE_HOLDS_RGB (value))
        {
          memsize += sizeof (LigmaRGB);
        }
      else if (LIGMA_VALUE_HOLDS_MATRIX2 (value))
        {
          memsize += sizeof (LigmaMatrix2);
        }
      else if (LIGMA_VALUE_HOLDS_PARASITE (value))
        {
          memsize += ligma_parasite_get_memsize (g_value_get_boxed (value),
                                                NULL);
        }
      else if (LIGMA_VALUE_HOLDS_ARRAY (value)       ||
               LIGMA_VALUE_HOLDS_UINT8_ARRAY (value) ||
               LIGMA_VALUE_HOLDS_INT32_ARRAY (value) ||
               LIGMA_VALUE_HOLDS_FLOAT_ARRAY (value))
        {
          LigmaArray *array = g_value_get_boxed (value);

          if (array)
            memsize += sizeof (LigmaArray) +
                       (array->static_data ? 0 : array->length);
        }
      else if (G_VALUE_HOLDS (value, G_TYPE_STRV))
        {
          gchar **array = g_value_get_boxed (value);

          if (array)
            {
              guint length = g_strv_length (array);

              memsize += (length + 1) * sizeof (gchar *);
              for (gint i = 0; i < length; i++)
                memsize += ligma_string_get_memsize (array[i]);
            }
        }
      else if (strcmp ("LigmaValueArray", G_VALUE_TYPE_NAME (value)) == 0)
        {
          LigmaValueArray *array = g_value_get_boxed (value);

          if (array)
            {
              gint n_values = ligma_value_array_length (array), i;

              memsize += /* sizeof (LigmaValueArray) */ sizeof (GValue *) + 3 * sizeof (gint);

              for (i = 0; i < n_values; i++)
                memsize += ligma_g_value_get_memsize (ligma_value_array_index (array, i));
            }
        }
      else
        {
          g_printerr ("%s: unhandled boxed value type: %s\n",
                      G_STRFUNC, G_VALUE_TYPE_NAME (value));
        }
    }
  else if (G_VALUE_HOLDS_OBJECT (value))
    {
      if (strcmp ("LigmaPattern", G_VALUE_TYPE_NAME (value)) == 0)
        memsize += ligma_g_object_get_memsize (g_value_get_object (value));
      else
        g_printerr ("%s: unhandled object value type: %s\n",
                    G_STRFUNC, G_VALUE_TYPE_NAME (value));
    }

  return memsize + sizeof (GValue);
}

gint64
ligma_g_param_spec_get_memsize (GParamSpec *pspec)
{
  gint64 memsize = 0;

  if (! pspec)
    return 0;

  if (! (pspec->flags & G_PARAM_STATIC_NAME))
    memsize += ligma_string_get_memsize (g_param_spec_get_name (pspec));

  if (! (pspec->flags & G_PARAM_STATIC_NICK))
    memsize += ligma_string_get_memsize (g_param_spec_get_nick (pspec));

  if (! (pspec->flags & G_PARAM_STATIC_BLURB))
    memsize += ligma_string_get_memsize (g_param_spec_get_blurb (pspec));

  return memsize + ligma_g_type_instance_get_memsize ((GTypeInstance *) pspec);
}

gint64
ligma_gegl_buffer_get_memsize (GeglBuffer *buffer)
{
  if (buffer)
    {
      const Babl *format = gegl_buffer_get_format (buffer);

      return ((gint64) babl_format_get_bytes_per_pixel (format) *
              (gint64) gegl_buffer_get_width (buffer) *
              (gint64) gegl_buffer_get_height (buffer) +
              ligma_g_object_get_memsize (G_OBJECT (buffer)));
    }

  return 0;
}

gint64
ligma_gegl_pyramid_get_memsize (GeglBuffer *buffer)
{
  if (buffer)
    {
      const Babl *format = gegl_buffer_get_format (buffer);

      /* The pyramid levels constitute a geometric sum with a ratio of 1/4. */
      return ((gint64) babl_format_get_bytes_per_pixel (format) *
              (gint64) gegl_buffer_get_width (buffer) *
              (gint64) gegl_buffer_get_height (buffer) * 1.33 +
              ligma_g_object_get_memsize (G_OBJECT (buffer)));
    }

  return 0;
}

gint64
ligma_string_get_memsize (const gchar *string)
{
  if (string)
    return strlen (string) + 1;

  return 0;
}

gint64
ligma_parasite_get_memsize (LigmaParasite *parasite,
                           gint64       *gui_size)
{
  if (parasite)
    return (sizeof (LigmaParasite) +
            ligma_string_get_memsize (parasite->name) +
            parasite->size);

  return 0;
}
