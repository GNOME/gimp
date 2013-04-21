/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#ifdef HAVE__NL_MEASUREMENT_MEASUREMENT
#include <langinfo.h>
#endif

#include <glib.h>

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#include <cairo.h>
#include <glib-object.h>
#include <gobject/gvaluecollector.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "base/base-utils.h"

#include "config/gimpbaseconfig.h"

#include "gimp.h"
#include "gimp-utils.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimpparamspecs.h"


gint64
gimp_g_type_instance_get_memsize (GTypeInstance *instance)
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
gimp_g_object_get_memsize (GObject *object)
{
  if (object)
    return gimp_g_type_instance_get_memsize ((GTypeInstance *) object);

  return 0;
}

gint64
gimp_g_hash_table_get_memsize (GHashTable *hash,
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

  g_return_val_if_fail (func != NULL, 0);

  if (! hash)
    return 0;

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
  gint64 memsize = 0;

  if (! value)
    return 0;

  if (G_VALUE_HOLDS_STRING (value))
    {
      memsize += gimp_string_get_memsize (g_value_get_string (value));
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
          memsize += gimp_parasite_get_memsize (g_value_get_boxed (value),
                                                NULL);
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
                    memsize += gimp_string_get_memsize (tmp[i]);
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

  return memsize + sizeof (GValue);
}

gint64
gimp_g_param_spec_get_memsize (GParamSpec *pspec)
{
  gint64 memsize = 0;

  if (! pspec)
    return 0;

  if (! (pspec->flags & G_PARAM_STATIC_NAME))
    memsize += gimp_string_get_memsize (g_param_spec_get_name (pspec));

  if (! (pspec->flags & G_PARAM_STATIC_NICK))
    memsize += gimp_string_get_memsize (g_param_spec_get_nick (pspec));

  if (! (pspec->flags & G_PARAM_STATIC_BLURB))
    memsize += gimp_string_get_memsize (g_param_spec_get_blurb (pspec));

  return memsize + gimp_g_type_instance_get_memsize ((GTypeInstance *) pspec);
}

gint64
gimp_string_get_memsize (const gchar *string)
{
  if (string)
    return strlen (string) + 1;

  return 0;
}

gint64
gimp_parasite_get_memsize (GimpParasite *parasite,
                           gint64       *gui_size)
{
  if (parasite)
    return (sizeof (GimpParasite) +
            gimp_string_get_memsize (parasite->name) +
            parasite->size);

  return 0;
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
#if defined (HAVE__NL_MEASUREMENT_MEASUREMENT)
  const gchar *measurement = nl_langinfo (_NL_MEASUREMENT_MEASUREMENT);

  switch (*((guchar *) measurement))
    {
    case 1: /* metric   */
      return GIMP_UNIT_MM;

    case 2: /* imperial */
      return GIMP_UNIT_INCH;
    }

#elif defined (G_OS_WIN32)
  DWORD measurement;
  int   ret;

  ret = GetLocaleInfo(LOCALE_USER_DEFAULT,
                      LOCALE_IMEASURE | LOCALE_RETURN_NUMBER,
                      (LPTSTR)&measurement,
                      sizeof(measurement) / sizeof(TCHAR) );

  if (ret != 0) /* GetLocaleInfo succeeded */
    {
    switch ((guint) measurement)
      {
      case 0: /* metric */
        return GIMP_UNIT_MM;

      case 1: /* imperial */
        return GIMP_UNIT_INCH;
      }
    }
#endif

  return GIMP_UNIT_MM;
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

  if (id == 0)
    pid = get_pid ();

  if (extension)
    basename = g_strdup_printf ("gimp-temp-%d%d.%s", pid, id++, extension);
  else
    basename = g_strdup_printf ("gimp-temp-%d%d", pid, id++);

  path = gimp_config_path_expand (GIMP_BASE_CONFIG (gimp->config)->temp_path,
                                  TRUE, NULL);

  filename = g_build_filename (path, basename, NULL);

  g_free (path);
  g_free (basename);

  return filename;
}

/*  markup unescape code stolen and adapted from gmarkup.c
 */
static gchar *
char_str (gunichar c,
          gchar   *buf)
{
  memset (buf, 0, 8);
  g_unichar_to_utf8 (c, buf);
  return buf;
}

static gboolean
unescape_gstring (GString *string)
{
  const gchar *from;
  gchar       *to;

  /*
   * Meeks' theorum: unescaping can only shrink text.
   * for &lt; etc. this is obvious, for &#xffff; more
   * thought is required, but this is patently so.
   */
  for (from = to = string->str; *from != '\0'; from++, to++)
    {
      *to = *from;

      if (*to == '\r')
        {
          *to = '\n';
          if (from[1] == '\n')
            from++;
        }
      if (*from == '&')
        {
          from++;
          if (*from == '#')
            {
              gboolean is_hex = FALSE;
              gulong   l;
              gchar   *end = NULL;

              from++;

              if (*from == 'x')
                {
                  is_hex = TRUE;
                  from++;
                }

              /* digit is between start and p */
              errno = 0;
              if (is_hex)
                l = strtoul (from, &end, 16);
              else
                l = strtoul (from, &end, 10);

              if (end == from || errno != 0)
                {
                  return FALSE;
                }
              else if (*end != ';')
                {
                  return FALSE;
                }
              else
                {
                  /* characters XML 1.1 permits */
                  if ((0 < l && l <= 0xD7FF) ||
                      (0xE000 <= l && l <= 0xFFFD) ||
                      (0x10000 <= l && l <= 0x10FFFF))
                    {
                      gchar buf[8];
                      char_str (l, buf);
                      strcpy (to, buf);
                      to += strlen (buf) - 1;
                      from = end;
                    }
                  else
                    {
                      return FALSE;
                    }
                }
            }

          else if (strncmp (from, "lt;", 3) == 0)
            {
              *to = '<';
              from += 2;
            }
          else if (strncmp (from, "gt;", 3) == 0)
            {
              *to = '>';
              from += 2;
            }
          else if (strncmp (from, "amp;", 4) == 0)
            {
              *to = '&';
              from += 3;
            }
          else if (strncmp (from, "quot;", 5) == 0)
            {
              *to = '"';
              from += 4;
            }
          else if (strncmp (from, "apos;", 5) == 0)
            {
              *to = '\'';
              from += 4;
            }
          else
            {
              return FALSE;
            }
        }
    }

  g_assert (to - string->str <= string->len);
  if (to - string->str != string->len)
    g_string_truncate (string, to - string->str);

  return TRUE;
}

gchar *
gimp_markup_extract_text (const gchar *markup)
{
  GString     *string;
  const gchar *p;
  gboolean     in_tag = FALSE;

  if (! markup)
    return NULL;

  string = g_string_new (NULL);

  for (p = markup; *p; p++)
    {
      if (in_tag)
        {
          if (*p == '>')
            in_tag = FALSE;
        }
      else
        {
          if (*p == '<')
            in_tag = TRUE;
          else
            g_string_append_c (string, *p);
        }
    }

  unescape_gstring (string);

  return g_string_free (string, FALSE);
}

/**
 * gimp_enum_get_value_name:
 * @enum_type: Enum type
 * @value:     Enum value
 *
 * Returns the value name for a given value of a given enum
 * type. Useful to have inline in GIMP_LOG() messages for example.
 *
 * Returns: The value name.
 **/
const gchar *
gimp_enum_get_value_name (GType enum_type,
                          gint  value)
{
  const gchar *value_name = NULL;
  
  gimp_enum_get_value (enum_type,
                       value,
                       &value_name,
                       NULL /*value_nick*/,
                       NULL /*value_desc*/,
                       NULL /*value_help*/);

  return value_name;
}

/**
 * gimp_utils_point_to_line_distance:
 * @point:              The point to calculate the distance for.
 * @point_on_line:      A point on the line.
 * @line_direction:     Normalized line direction vector.
 * @closest_line_point: Gets set to the point on the line that is
 *                      closest to @point.
 *
 * Returns: The shortest distance from @point to the line defined by
 *          @point_on_line and @normalized_line_direction.
 **/
static gdouble
gimp_utils_point_to_line_distance (const GimpVector2 *point,
                                   const GimpVector2 *point_on_line,
                                   const GimpVector2 *line_direction,
                                   GimpVector2       *closest_line_point)
{
  GimpVector2 distance_vector;
  GimpVector2 tmp_a;
  GimpVector2 tmp_b;
  gdouble     d;

  gimp_vector2_sub (&tmp_a, point, point_on_line);

  d = gimp_vector2_inner_product (&tmp_a, line_direction);

  tmp_b = gimp_vector2_mul_val (*line_direction, d);

  *closest_line_point = gimp_vector2_add_val (*point_on_line,
                                              tmp_b);

  gimp_vector2_sub (&distance_vector, closest_line_point, point);

  return gimp_vector2_length (&distance_vector);
}

/**
 * gimp_constrain_line:
 * @start_x:
 * @start_y:
 * @end_x:
 * @end_y:
 * @n_snap_lines: Number evenly disributed lines to snap to.
 *
 * Projects a line onto the specified subset of evenly radially
 * distributed lines. @n_lines of 2 makes the line snap horizontally
 * or vertically. @n_lines of 4 snaps on 45 degree steps. @n_lines of
 * 12 on 15 degree steps. etc.
 **/
void
gimp_constrain_line (gdouble  start_x,
                     gdouble  start_y,
                     gdouble *end_x,
                     gdouble *end_y,
                     gint     n_snap_lines)
{
  GimpVector2 line_point          = {  start_x,  start_y };
  GimpVector2 point               = { *end_x,   *end_y   };
  GimpVector2 constrained_point;
  GimpVector2 line_dir;
  gdouble     shortest_dist_moved = G_MAXDOUBLE;
  gdouble     dist_moved;
  gdouble     angle;
  gint        i;

  for (i = 0; i < n_snap_lines; i++)
    {
      angle = i * G_PI / n_snap_lines;

      gimp_vector2_set (&line_dir,
                        cos (angle),
                        sin (angle));

      dist_moved = gimp_utils_point_to_line_distance (&point,
                                                      &line_point,
                                                      &line_dir,
                                                      &constrained_point);
      if (dist_moved < shortest_dist_moved)
        {
          shortest_dist_moved = dist_moved;

          *end_x = constrained_point.x;
          *end_y = constrained_point.y;
        }
    }
}
