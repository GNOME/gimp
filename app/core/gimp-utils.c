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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib.h>

#ifdef G_OS_WIN32
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <process.h>
#endif

#if defined(G_OS_UNIX) && defined(HAVE_EXECINFO_H)
/* For get_backtrace() */
#include <stdlib.h>
#include <string.h>
#include <execinfo.h>
#endif

#include <cairo.h>
#include <gegl.h>
#include <gobject/gvaluecollector.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimp.h"
#include "gimp-utils.h"
#include "gimpcontext.h"
#include "gimperror.h"

#include "gimp-intl.h"


#define MAX_FUNC 100


gint
gimp_get_pid (void)
{
  return (gint) getpid ();
}

guint64
gimp_get_physical_memory_size (void)
{
#ifdef G_OS_UNIX
#if defined(HAVE_UNISTD_H) && defined(_SC_PHYS_PAGES) && defined (_SC_PAGE_SIZE)
  return (guint64) sysconf (_SC_PHYS_PAGES) * sysconf (_SC_PAGE_SIZE);
#endif
#endif

#ifdef G_OS_WIN32
# if defined(_MSC_VER) && (_MSC_VER <= 1200)
  MEMORYSTATUS memory_status;
  memory_status.dwLength = sizeof (memory_status);

  GlobalMemoryStatus (&memory_status);
  return memory_status.dwTotalPhys;
# else
  /* requires w2k and newer SDK than provided with msvc6 */
  MEMORYSTATUSEX memory_status;

  memory_status.dwLength = sizeof (memory_status);

  if (GlobalMemoryStatusEx (&memory_status))
    return memory_status.ullTotalPhys;
# endif
#endif

  return 0;
}

/**
 * gimp_get_backtrace:
 *
 * Returns: The current stack trace. Free with g_free(). Mainly meant
 * for debugging, for example storing the allocation stack traces for
 * objects to hunt down leaks.
 **/
gchar *
gimp_get_backtrace (void)
{
#if defined(G_OS_UNIX) && defined(HAVE_EXECINFO_H)
  void     *functions[MAX_FUNC];
  char    **function_names;
  int       n_functions;
  int       i;
  GString  *result;

  /* Get symbols */
  n_functions    = backtrace (functions, MAX_FUNC);
  function_names = backtrace_symbols (functions, n_functions);

  /* Construct stack trace */
  result = g_string_new ("");
  for (i = 0; i < n_functions; i++)
    g_string_append_printf (result, "%s\n", function_names[i]);

  /* We must not free the function names themselves, we only need to
   * free the array that points to them
   */
  free (function_names);

  return g_string_free (result, FALSE/*free_segment*/);
#else
  return g_strdup ("backtrace() only available with GNU libc\n");
#endif
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

gboolean
gimp_get_fill_params (GimpContext   *context,
                      GimpFillType   fill_type,
                      GimpRGB       *color,
                      GimpPattern  **pattern,
                      GError       **error)

{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (color != NULL, FALSE);
  g_return_val_if_fail (pattern != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  *pattern = NULL;

  switch (fill_type)
    {
    case GIMP_FILL_FOREGROUND:
      gimp_context_get_foreground (context, color);
      break;

    case GIMP_FILL_BACKGROUND:
      gimp_context_get_background (context, color);
      break;

    case GIMP_FILL_WHITE:
      gimp_rgba_set (color, 1.0, 1.0, 1.0, GIMP_OPACITY_OPAQUE);
      break;

    case GIMP_FILL_TRANSPARENT:
      gimp_rgba_set (color, 0.0, 0.0, 0.0, GIMP_OPACITY_TRANSPARENT);
      break;

    case GIMP_FILL_PATTERN:
      *pattern = gimp_context_get_pattern (context);

      if (! *pattern)
        {
          g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                               _("No patterns available for this operation."));

          /*  fall back to BG fill  */
          gimp_context_get_background (context, color);

          return FALSE;
        }
      break;

    default:
      g_warning ("%s: invalid fill_type %d", G_STRFUNC, fill_type);
      return FALSE;
    }

  return TRUE;
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

gint
gimp_file_compare (GFile *file1,
                   GFile *file2)
{
  if (g_file_equal (file1, file2))
    {
      return 0;
    }
  else
    {
      gchar *uri1   = g_file_get_uri (file1);
      gchar *uri2   = g_file_get_uri (file2);
      gint   result = strcmp (uri1, uri2);

      g_free (uri1);
      g_free (uri2);

      return result;
    }
}

static inline gboolean
is_script (const gchar *filename)
{
#ifdef G_OS_WIN32
  /* On Windows there is no concept like the Unix executable flag.
   * There is a weak emulation provided by the MS C Runtime using file
   * extensions (com, exe, cmd, bat). This needs to be extended to
   * treat scripts (Python, Perl, ...) as executables, too. We use the
   * PATHEXT variable, which is also used by cmd.exe.
   */
  static gchar **exts = NULL;

  const gchar   *ext = strrchr (filename, '.');
  gchar         *pathext;
  gint           i;

  if (exts == NULL)
    {
      pathext = g_getenv ("PATHEXT");
      if (pathext != NULL)
        {
          exts = g_strsplit (pathext, G_SEARCHPATH_SEPARATOR_S, 100);
        }
      else
        {
          exts = g_new (gchar *, 1);
          exts[0] = NULL;
        }
    }

  for (i = 0; exts[i]; i++)
    {
      if (g_ascii_strcasecmp (ext, exts[i]) == 0)
        return TRUE;
    }
#endif /* G_OS_WIN32 */

  return FALSE;
}

gboolean
gimp_file_is_executable (GFile *file)
{
  GFileInfo *info;
  gboolean   executable = FALSE;

  g_return_val_if_fail (G_IS_FILE (file), FALSE);

  info = g_file_query_info (file,
                            G_FILE_ATTRIBUTE_STANDARD_NAME ","
                            G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                            G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE ",",
                            G_FILE_QUERY_INFO_NONE,
                            NULL, NULL);

  if (info)
    {
      GFileType    file_type = g_file_info_get_file_type (info);
      const gchar *filename  = g_file_info_get_name (info);

      if (file_type == G_FILE_TYPE_REGULAR &&
          (g_file_info_get_attribute_boolean (info,
                                              G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE) ||
           is_script (filename)))
        {
          executable = TRUE;
        }

      g_object_unref (info);
    }

  return executable;
}

/**
 * gimp_file_get_extension:
 * @file: A #GFile
 *
 * Returns @file's extension (including the .), or NULL if there is no
 * extension. Note that this function handles compressed files too,
 * e.g. for "file.png.gz" it will return ".png.gz".
 *
 * Returns: The @file's extension. Free with g_free() when no longer needed.
 **/
gchar *
gimp_file_get_extension (GFile *file)
{
  gchar *uri;
  gint   uri_len;
  gchar *ext = NULL;
  gint   search_len;

  g_return_val_if_fail (G_IS_FILE (file), NULL);

  uri     = g_file_get_uri (file);
  uri_len = strlen (uri);

  if (g_str_has_suffix (uri, ".gz"))
    search_len = uri_len - 3;
  else if (g_str_has_suffix (uri, ".bz2"))
    search_len = uri_len - 4;
  else if (g_str_has_suffix (uri, ".xz"))
    search_len = uri_len - 3;
  else
    search_len = uri_len;

  ext = g_strrstr_len (uri, search_len, ".");

  if (ext)
    ext = g_strdup (ext);

  g_free (uri);

  return ext;
}

GFile *
gimp_file_with_new_extension (GFile *file,
                              GFile *ext_file)
{
  gchar *uri;
  gchar *file_ext;
  gint   file_ext_len = 0;
  gchar *ext_file_ext = NULL;
  gchar *uri_without_ext;
  gchar *new_uri;
  GFile *ret;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (ext_file == NULL || G_IS_FILE (ext_file), NULL);

  uri      = g_file_get_uri (file);
  file_ext = gimp_file_get_extension (file);

  if (file_ext)
    {
      file_ext_len = strlen (file_ext);
      g_free (file_ext);
    }

  if (ext_file)
    ext_file_ext = gimp_file_get_extension (ext_file);

  uri_without_ext = g_strndup (uri, strlen (uri) - file_ext_len);

  g_free (uri);

  new_uri = g_strconcat (uri_without_ext, ext_file_ext, NULL);

  ret = g_file_new_for_uri (new_uri);

  g_free (ext_file_ext);
  g_free (uri_without_ext);
  g_free (new_uri);

  return ret;
}


/*  debug stuff  */

#include "gegl/gimp-babl.h"
#include "gimpimage.h"
#include "gimplayer.h"
#include "gimplayer-new.h"

GimpImage *
gimp_create_image_from_buffer (Gimp        *gimp,
                               GeglBuffer  *buffer,
                               const gchar *image_name)
{
  GimpImage  *image;
  GimpLayer  *layer;
  const Babl *format;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  if (! image_name)
    image_name = "Debug Image";

  format = gegl_buffer_get_format (buffer);

  image = gimp_create_image (gimp,
                             gegl_buffer_get_width  (buffer),
                             gegl_buffer_get_height (buffer),
                             gimp_babl_format_get_base_type (format),
                             gimp_babl_format_get_precision (format),
                             FALSE);

  layer = gimp_layer_new_from_gegl_buffer (buffer, image, format,
                                           image_name,
                                           GIMP_OPACITY_OPAQUE,
                                           GIMP_LAYER_MODE_NORMAL,
                                           NULL /* same image */);
  gimp_image_add_layer (image, layer, NULL, -1, FALSE);

  gimp_create_display (gimp, image, GIMP_UNIT_PIXEL, 1.0, NULL, 0);

  /* unref the image unconditionally, even when no display was created */
  g_object_add_weak_pointer (G_OBJECT (image), (gpointer) &image);
  g_object_unref (image);

  return image;
}
