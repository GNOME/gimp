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

#include <stdlib.h>
#include <string.h>
#include <locale.h>

#ifdef HAVE__NL_MEASUREMENT_MEASUREMENT
#include <langinfo.h>
#endif

#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib.h>

#ifdef G_OS_WIN32
#include <process.h>
#endif

#include <glib-object.h>
#include <gobject/gvaluecollector.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "config/gimpbaseconfig.h"

#include "gimp.h"
#include "gimp-utils.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimpparamspecs.h"


gint64
gimp_g_type_instance_get_memsize (GTypeInstance *instance)
{
  GTypeQuery type_query;
  gint64     memsize = 0;

  g_return_val_if_fail (instance != NULL, 0);

  g_type_query (G_TYPE_FROM_INSTANCE (instance), &type_query);

  memsize += type_query.instance_size;

  return memsize;
}

gint64
gimp_g_object_get_memsize (GObject *object)
{
  gint64 memsize = 0;

  g_return_val_if_fail (G_IS_OBJECT (object), 0);

  return memsize + gimp_g_type_instance_get_memsize ((GTypeInstance *) object);
}

gint64
gimp_g_hash_table_get_memsize (GHashTable *hash,
                               gint64      data_size)
{
  g_return_val_if_fail (hash != NULL, 0);

  return (2 * sizeof (gint) +
          5 * sizeof (gpointer) +
          g_hash_table_size (hash) * (3 * sizeof (gpointer) + data_size));
}

typedef struct
{
  GimpMemsizeFunc func;
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
gimp_g_hash_table_get_memsize_foreach (GHashTable      *hash,
                                       GimpMemsizeFunc  func,
                                       gint64          *gui_size)
{
  HashMemsize memsize;

  g_return_val_if_fail (hash != NULL, 0);
  g_return_val_if_fail (func != NULL, 0);

  memsize.func     = func;
  memsize.memsize  = 0;
  memsize.gui_size = 0;

  g_hash_table_foreach (hash, (GHFunc) hash_memsize_foreach, &memsize);

  if (gui_size)
    *gui_size = memsize.gui_size;

  return memsize.memsize + gimp_g_hash_table_get_memsize (hash, 0);
}

gint64
gimp_g_slist_get_memsize (GSList  *slist,
                          gint64   data_size)
{
  return g_slist_length (slist) * (data_size + sizeof (GSList));
}

gint64
gimp_g_slist_get_memsize_foreach (GSList          *slist,
                                  GimpMemsizeFunc  func,
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
gimp_g_list_get_memsize (GList  *list,
                         gint64  data_size)
{
  return g_list_length (list) * (data_size + sizeof (GList));
}

gint64
gimp_g_list_get_memsize_foreach (GList           *list,
                                 GimpMemsizeFunc  func,
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
gimp_g_value_get_memsize (GValue *value)
{
  gint64 memsize = sizeof (GValue);

  if (G_VALUE_HOLDS_STRING (value))
    {
      const gchar *str = g_value_get_string (value);

      if (str)
        memsize += strlen (str) + 1;
    }
  else if (G_VALUE_HOLDS_BOXED (value))
    {
      if (GIMP_VALUE_HOLDS_RGB (value))
        {
          memsize += sizeof (GimpRGB);
        }
      else if (GIMP_VALUE_HOLDS_MATRIX2 (value))
        {
          memsize += sizeof (GimpMatrix2);
        }
      else if (GIMP_VALUE_HOLDS_PARASITE (value))
        {
          GimpParasite *parasite = g_value_get_boxed (value);

          if (parasite)
            memsize += (sizeof (GimpParasite) + parasite->size +
                        parasite->name ? (strlen (parasite->name) + 1) : 0);
        }
      else if (GIMP_VALUE_HOLDS_ARRAY (value)       ||
               GIMP_VALUE_HOLDS_INT8_ARRAY (value)  ||
               GIMP_VALUE_HOLDS_INT16_ARRAY (value) ||
               GIMP_VALUE_HOLDS_INT32_ARRAY (value) ||
               GIMP_VALUE_HOLDS_FLOAT_ARRAY (value))
        {
          GimpArray *array = g_value_get_boxed (value);

          if (array)
            memsize += (sizeof (GimpArray) +
                        array->static_data ? 0 : array->length);
        }
      else if (GIMP_VALUE_HOLDS_STRING_ARRAY (value))
        {
          GimpArray *array = g_value_get_boxed (value);

          if (array)
            {
              memsize += sizeof (GimpArray);

              if (! array->static_data)
                {
                  gchar **tmp = (gchar **) array->data;
                  gint    i;

                  memsize += array->length * sizeof (gchar *);

                  for (i = 0; i < array->length; i++)
                    memsize += strlen (tmp[i]) + 1;
                }
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
      g_printerr ("%s: unhandled object value type: %s\n",
                  G_STRFUNC, G_VALUE_TYPE_NAME (value));
    }

  return memsize;
}

gint64
gimp_g_param_spec_get_memsize (GParamSpec *pspec)
{
  const gchar *str;
  gint64       memsize = 0;

  if (! (pspec->flags & G_PARAM_STATIC_NAME))
    {
      str = g_param_spec_get_name (pspec);
      if (str)
        memsize += strlen (str) + 1;
    }

  if (! (pspec->flags & G_PARAM_STATIC_NICK))
    {
      str = g_param_spec_get_nick (pspec);
      if (str)
        memsize += strlen (str) + 1;
    }

  if (! (pspec->flags & G_PARAM_STATIC_BLURB))
    {
      str = g_param_spec_get_blurb (pspec);
      if (str)
        memsize += strlen (str) + 1;
    }

  return memsize + gimp_g_type_instance_get_memsize ((GTypeInstance *) pspec);
}

gint64
gimp_parasite_get_memsize (GimpParasite *parasite,
                           gint64       *gui_size)
{
  gint64 memsize = 0;

  if (parasite)
    memsize += (sizeof (GimpParasite) +
                strlen (parasite->name) + 1 +
                parasite->size);

  return memsize;
}


/*
 *  basically copied from gtk_get_default_language()
 */
gchar *
gimp_get_default_language (const gchar *category)
{
  gchar *lang;
  gchar *p;
  gint   cat = LC_CTYPE;

  if (! category)
    category = "LC_CTYPE";

#ifdef G_OS_WIN32

  p = getenv ("LC_ALL");
  if (p != NULL)
    lang = g_strdup (p);
  else
    {
      p = getenv ("LANG");
      if (p != NULL)
        lang = g_strdup (p);
      else
        {
          p = getenv (category);
          if (p != NULL)
            lang = g_strdup (p);
          else
            lang = g_win32_getlocale ();
        }
    }

#else

  if (strcmp (category, "LC_CTYPE") == 0)
    cat = LC_CTYPE;
  else if (strcmp (category, "LC_MESSAGES") == 0)
    cat = LC_MESSAGES;
  else
    g_warning ("unsupported category used with gimp_get_default_language()");

  lang = g_strdup (setlocale (cat, NULL));

#endif

  p = strchr (lang, '.');
  if (p)
    *p = '\0';
  p = strchr (lang, '@');
  if (p)
    *p = '\0';

  return lang;
}

GimpUnit
gimp_get_default_unit (void)
{
#ifdef HAVE__NL_MEASUREMENT_MEASUREMENT
  const gchar *measurement = nl_langinfo (_NL_MEASUREMENT_MEASUREMENT);

  switch (*((guchar *) measurement))
    {
    case 1: /* metric   */
      return GIMP_UNIT_MM;

    case 2: /* imperial */
      return GIMP_UNIT_INCH;
    }
#endif

  return GIMP_UNIT_INCH;
}

GParameter *
gimp_parameters_append (GType       object_type,
                        GParameter *params,
                        gint       *n_params,
                        ...)
{
  va_list args;

  g_return_val_if_fail (g_type_is_a (object_type, G_TYPE_OBJECT), NULL);
  g_return_val_if_fail (n_params != NULL, NULL);
  g_return_val_if_fail (params != NULL || *n_params == 0, NULL);

  va_start (args, n_params);
  params = gimp_parameters_append_valist (object_type, params, n_params, args);
  va_end (args);

  return params;
}

GParameter *
gimp_parameters_append_valist (GType       object_type,
                               GParameter *params,
                               gint       *n_params,
                               va_list     args)
{
  GObjectClass *object_class;
  gchar        *param_name;

  g_return_val_if_fail (g_type_is_a (object_type, G_TYPE_OBJECT), NULL);
  g_return_val_if_fail (n_params != NULL, NULL);
  g_return_val_if_fail (params != NULL || *n_params == 0, NULL);

  object_class = g_type_class_ref (object_type);

  param_name = va_arg (args, gchar *);

  while (param_name)
    {
      gchar      *error = NULL;
      GParamSpec *pspec = g_object_class_find_property (object_class,
                                                        param_name);

      if (! pspec)
        {
          g_warning ("%s: object class `%s' has no property named `%s'",
                     G_STRFUNC, g_type_name (object_type), param_name);
          break;
        }

      params = g_renew (GParameter, params, *n_params + 1);

      params[*n_params].name         = param_name;
      params[*n_params].value.g_type = 0;

      g_value_init (&params[*n_params].value, G_PARAM_SPEC_VALUE_TYPE (pspec));

      G_VALUE_COLLECT (&params[*n_params].value, args, 0, &error);

      if (error)
        {
          g_warning ("%s: %s", G_STRFUNC, error);
          g_free (error);
          g_value_unset (&params[*n_params].value);
          break;
        }

      *n_params = *n_params + 1;

      param_name = va_arg (args, gchar *);
    }

  g_type_class_unref (object_class);

  return params;
}

void
gimp_parameters_free (GParameter *params,
                      gint        n_params)
{
  g_return_if_fail (params != NULL || n_params == 0);

  if (params)
    {
      gint i;

      for (i = 0; i < n_params; i++)
        g_value_unset (&params[i].value);

      g_free (params);
    }
}

void
gimp_value_array_truncate (GValueArray  *args,
                           gint          n_values)
{
  gint i;

  g_return_if_fail (args != NULL);
  g_return_if_fail (n_values > 0 && n_values <= args->n_values);

  for (i = args->n_values; i > n_values; i--)
    g_value_array_remove (args, i - 1);
}

gchar *
gimp_get_temp_filename (Gimp        *gimp,
                        const gchar *extension)
{
  static gint  id = 0;
  static gint  pid;
  gchar       *filename;
  gchar       *basename;
  gchar       *path;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (extension != NULL, NULL);

  if (id == 0)
    pid = getpid ();

  basename = g_strdup_printf ("gimp-temp-%d%d.%s", pid, id++, extension);

  path = gimp_config_path_expand (GIMP_BASE_CONFIG (gimp->config)->temp_path,
                                  TRUE, NULL);

  filename = g_build_filename (path, basename, NULL);

  g_free (path);
  g_free (basename);

  return filename;
}

GimpObject *
gimp_container_get_neighbor_of_active (GimpContainer *container,
                                       GimpContext   *context,
                                       GimpObject    *active)
{
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GIMP_IS_OBJECT (active), NULL);

  if (active == gimp_context_get_by_type (context, container->children_type))
    {
      gint index = gimp_container_get_child_index (container, active);

      if (index != -1)
        {
          GimpObject *new;

          new = gimp_container_get_child_by_index (container, index + 1);

          if (! new && index > 0)
            new = gimp_container_get_child_by_index (container, index - 1);

          return new;
        }
    }

  return NULL;
}
