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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#include <windows.h>
#endif

#include <cairo.h>
#include <gegl.h>
#include <gobject/gvaluecollector.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "config/gimpxmlparser.h"

#include "gimp.h"
#include "gimp-utils.h"
#include "gimpasync.h"
#include "gimpcontext.h"
#include "gimpdata.h"
#include "gimperror.h"
#include "gimpimage.h"

#include "gimp-intl.h"


#define MAX_FUNC 100


typedef struct
{
  GString  *text;
  gint      level;

  gboolean  numbered_list;
  gint      list_num;
  gboolean  unnumbered_list;

  const gchar *lang;
  GString     *original;
  gint         foreign_level;

  gchar      **introduction;
  GList      **release_items;
} ParseState;


static gchar      * gimp_appstream_parse           (const gchar          *as_text,
                                                    gchar               **introduction,
                                                    GList               **release_items);
static void         appstream_text_start_element   (GMarkupParseContext  *context,
                                                    const gchar          *element_name,
                                                    const gchar         **attribute_names,
                                                    const gchar         **attribute_values,
                                                    gpointer              user_data,
                                                    GError              **error);
static void         appstream_text_end_element     (GMarkupParseContext  *context,
                                                    const gchar          *element_name,
                                                    gpointer              user_data,
                                                    GError              **error);
static void         appstream_text_characters      (GMarkupParseContext  *context,
                                                    const gchar          *text,
                                                    gsize                 text_len,
                                                    gpointer              user_data,
                                                    GError              **error);
static const gchar* gimp_extension_get_tag_lang    (const gchar         **attribute_names,
                                                    const gchar         **attribute_values);

static gboolean     gimp_version_break             (const gchar          *v,
                                                    gint                 *major,
                                                    gint                 *minor,
                                                    gint                 *micro,
                                                    gint                 *rc,
                                                    gboolean             *is_git);


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

/*
 *  basically copied from gtk_get_default_language()
 */
gchar *
gimp_get_default_language (const gchar *category)
{
  gchar *lang;
  gchar *p;
#ifndef G_OS_WIN32
  gint   cat = LC_CTYPE;
#endif

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

GimpUnit *
gimp_get_default_unit (void)
{
#if defined (HAVE__NL_MEASUREMENT_MEASUREMENT)
  const gchar *measurement = nl_langinfo (_NL_MEASUREMENT_MEASUREMENT);

  switch (*((guchar *) measurement))
    {
    case 1: /* metric   */
      return gimp_unit_mm ();

    case 2: /* imperial */
      return gimp_unit_inch ();
    }

#elif defined (G_OS_WIN32)
  DWORD measurement;
  int   ret;

  ret = GetLocaleInfoW (LOCALE_USER_DEFAULT,
                        LOCALE_IMEASURE | LOCALE_RETURN_NUMBER,
                        (LPWSTR) &measurement,
                        sizeof (measurement) / sizeof (wchar_t));

  if (ret != 0) /* GetLocaleInfo succeeded */
    {
    switch ((guint) measurement)
      {
      case 0: /* metric */
        return gimp_unit_mm ();

      case 1: /* imperial */
        return gimp_unit_inch ();
      }
    }
#endif

  return gimp_unit_mm ();
}

gchar **
gimp_properties_append (GType    object_type,
                        gint    *n_properties,
                        gchar  **names,
                        GValue **values,
                        ...)
{
  va_list args;

  g_return_val_if_fail (g_type_is_a (object_type, G_TYPE_OBJECT), NULL);
  g_return_val_if_fail (n_properties != NULL, NULL);
  g_return_val_if_fail (names != NULL || *n_properties == 0, NULL);
  g_return_val_if_fail (values != NULL || *n_properties == 0, NULL);

  va_start (args, values);
  names = gimp_properties_append_valist (object_type, n_properties,
                                         names, values, args);
  va_end (args);

  return names;
}

gchar **
gimp_properties_append_valist (GType     object_type,
                               gint      *n_properties,
                               gchar   **names,
                               GValue  **values,
                               va_list   args)
{
  GObjectClass *object_class;
  gchar        *param_name;

  g_return_val_if_fail (g_type_is_a (object_type, G_TYPE_OBJECT), NULL);
  g_return_val_if_fail (n_properties != NULL, NULL);
  g_return_val_if_fail (names != NULL || *n_properties == 0, NULL);
  g_return_val_if_fail (values != NULL || *n_properties == 0, NULL);

  object_class = g_type_class_ref (object_type);

  param_name = va_arg (args, gchar *);

  while (param_name)
    {
      GValue     *value;
      gchar      *error = NULL;
      GParamSpec *pspec = g_object_class_find_property (object_class,
                                                        param_name);

      if (! pspec)
        {
          g_warning ("%s: object class `%s' has no property named `%s'",
                     G_STRFUNC, g_type_name (object_type), param_name);
          break;
        }

      names   = g_renew (gchar *, names,   *n_properties + 1);
      *values = g_renew (GValue,  *values, *n_properties + 1);

      value = &((*values)[*n_properties]);

      names[*n_properties] = g_strdup (param_name);
      value->g_type = 0;

      g_value_init (value, G_PARAM_SPEC_VALUE_TYPE (pspec));

      G_VALUE_COLLECT (value, args, 0, &error);

      if (error)
        {
          g_warning ("%s: %s", G_STRFUNC, error);
          g_free (error);
          g_free (names[*n_properties]);
          g_value_unset (value);
          break;
        }

      *n_properties = *n_properties + 1;

      param_name = va_arg (args, gchar *);
    }

  g_type_class_unref (object_class);

  return names;
}

void
gimp_properties_free (gint     n_properties,
                      gchar  **names,
                      GValue  *values)
{
  g_return_if_fail (names  != NULL || n_properties == 0);
  g_return_if_fail (values != NULL || n_properties == 0);

  if (names && values)
    {
      gint i;

      for (i = 0; i < n_properties; i++)
        {
          g_free (names[i]);
          g_value_unset (&values[i]);
        }

      g_free (names);
      g_free (values);
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

  gimp_assert (to - string->str <= string->len);
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
                      GeglColor    **color,
                      GimpPattern  **pattern,
                      GError       **error)

{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (color != NULL, FALSE);
  g_return_val_if_fail (pattern != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  *color   = NULL;
  *pattern = NULL;

  switch (fill_type)
    {
    case GIMP_FILL_FOREGROUND:
      *color = gegl_color_duplicate (gimp_context_get_foreground (context));
      break;

    case GIMP_FILL_BACKGROUND:
      *color = gegl_color_duplicate (gimp_context_get_background (context));
      break;

    case GIMP_FILL_CIELAB_MIDDLE_GRAY:
      {
        const float cielab_pixel[3] = {50.f, 0.f, 0.f};

        *color = gegl_color_new (NULL);
        gegl_color_set_pixel (*color, babl_format ("CIE Lab float"), cielab_pixel);
      }
      break;

    case GIMP_FILL_WHITE:
      *color = gegl_color_new ("white");
      break;

    case GIMP_FILL_TRANSPARENT:
      *color = gegl_color_new ("transparent");
      break;

    case GIMP_FILL_PATTERN:
      *pattern = gimp_context_get_pattern (context);

      if (! *pattern)
        {
          g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                               _("No patterns available for this operation."));

          /*  fall back to BG fill  */
          *color = gegl_color_duplicate (gimp_context_get_background (context));

          return TRUE;
        }
      break;

    default:
      g_warning ("%s: invalid fill_type %d", G_STRFUNC, fill_type);
      return FALSE;
    }

  return TRUE;
}

/**
 * gimp_constrain_line:
 * @start_x:
 * @start_y:
 * @end_x:
 * @end_y:
 * @n_snap_lines: Number evenly distributed lines to snap to.
 * @offset_angle: The angle by which to offset the lines, in degrees.
 * @xres:         The horizontal resolution.
 * @yres:         The vertical resolution.
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
                     gint     n_snap_lines,
                     gdouble  offset_angle,
                     gdouble  xres,
                     gdouble  yres)
{
  GimpVector2 diff;
  GimpVector2 dir;
  gdouble     angle;

  offset_angle *= G_PI / 180.0;

  diff.x = (*end_x - start_x) / xres;
  diff.y = (*end_y - start_y) / yres;

  angle = (atan2 (diff.y, diff.x) - offset_angle) * n_snap_lines / G_PI;
  angle = RINT (angle) * G_PI / n_snap_lines + offset_angle;

  dir.x = cos (angle);
  dir.y = sin (angle);

  gimp_vector2_mul (&dir, gimp_vector2_inner_product (&dir, &diff));

  *end_x = start_x + dir.x * xres;
  *end_y = start_y + dir.y * yres;
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
  const gchar   *pathext;
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
      GFileType    file_type = g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_STANDARD_TYPE);
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
  GFileInfo *info;
  gchar     *basename;
  gint       basename_len;
  gchar     *ext = NULL;
  gint       search_len;

  g_return_val_if_fail (G_IS_FILE (file), NULL);

  /* Certain cloud providers return a blob name rather than the
   * actual file with g_file_get_uri (). Since we don't check
   * the magic numbers for remote files, we can't open it. The
   * actual name is stored as "display-name" in all cases, so we
   * use that instead. */
  info = g_file_query_info (file,
                            G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                            0, NULL, NULL);

  if (info != NULL)
    basename =
      g_file_info_get_attribute_as_string (info,
                                           G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME);
  else
    basename = g_file_get_basename (file);

  g_clear_object (&info);

  /* When making a temporary file for saving/exporting, we may not
   * have the display-name yet, so let's fallback to the URI */
  if (! basename)
    basename = g_file_get_uri (file);

  basename_len = strlen (basename);

  if (g_str_has_suffix (basename, ".gz"))
    search_len = basename_len - 3;
  else if (g_str_has_suffix (basename, ".bz2"))
    search_len = basename_len - 4;
  else if (g_str_has_suffix (basename, ".xz"))
    search_len = basename_len - 3;
  else
    search_len = basename_len;

  ext = g_strrstr_len (basename, search_len, ".");

  if (ext)
    ext = g_strdup (ext);

  g_free (basename);

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

/**
 * gimp_file_delete_recursive:
 * @file: #GFile to delete from file system.
 * @error:
 *
 * Delete @file. If file is a directory, it will delete its children as
 * well recursively. It will not follow symlinks so you won't end up in
 * infinite loops, not will you be at risk of deleting your whole file
 * system (unless you pass the root of course!).
 * Such function unfortunately does not exist in glib, which only allows
 * to delete single files or empty directories by default.
 *
 * Returns: %TRUE if @file was successfully deleted and all its
 * children, %FALSE otherwise with @error filled.
 */
gboolean
gimp_file_delete_recursive (GFile   *file,
                            GError **error)
{
  gboolean success = TRUE;

  if (g_file_query_exists (file, NULL))
    {
      if (g_file_query_file_type (file, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                  NULL) == G_FILE_TYPE_DIRECTORY)
        {
          GFileEnumerator *enumerator;

          enumerator = g_file_enumerate_children (file,
                                                  G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                                  G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                                  G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                                  G_FILE_QUERY_INFO_NONE,
                                                  NULL, NULL);
          if (enumerator)
            {
              GFileInfo *info;

              while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)))
                {
                  GFile *child;

                  child = g_file_enumerator_get_child (enumerator, info);
                  g_object_unref (info);

                  if (! gimp_file_delete_recursive (child, error))
                    success = FALSE;

                  g_object_unref (child);
                  if (! success)
                    break;
                }

              g_object_unref (enumerator);
            }
        }

      if (success)
        /* Non-directory or empty directory. */
        success = g_file_delete (file, NULL, error);
    }

  return success;
}

gchar *
gimp_data_input_stream_read_line_always (GDataInputStream  *stream,
                                         gsize             *length,
                                         GCancellable      *cancellable,
                                         GError           **error)
{
  GError *temp_error = NULL;
  gchar  *result;

  g_return_val_if_fail (G_IS_DATA_INPUT_STREAM (stream), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (! error)
    error = &temp_error;

  result = g_data_input_stream_read_line (stream, length, cancellable, error);

  if (! result && ! *error)
    {
      result = g_strdup ("");

      if (length) *length = 0;
    }

  g_clear_error (&temp_error);

  return result;
}

gboolean
gimp_data_input_stream_read_char (GDataInputStream  *input,
                                  gchar             *value,
                                  GError           **error)
{
  gchar result;

  result = g_data_input_stream_read_byte (input, NULL, error);
  if (error && *error)
    return FALSE;

  *value = result;
  return TRUE;
}

gboolean
gimp_data_input_stream_read_short (GDataInputStream  *input,
                                   gint16            *value,
                                   GError           **error)
{
  gint16 result;

  result = g_data_input_stream_read_int16 (input, NULL, error);
  if (error && *error)
    return FALSE;

  *value = result;
  return TRUE;
}

gboolean
gimp_data_input_stream_read_long (GDataInputStream  *input,
                                  gint32            *value,
                                  GError           **error)
{
  gint32 result;

  result = g_data_input_stream_read_int32 (input, NULL, error);
  if (error && *error)
    return FALSE;

  *value = result;
  return TRUE;
}

gboolean
gimp_data_input_stream_read_ucs2_text (GDataInputStream  *input,
                                       gchar            **value,
                                       GError           **error)
{
  gchar *name_ucs2;
  gint32 pslen = 0;
  gint   len;
  gint   i;

  /* two-bytes characters encoded (UCS-2)
   *  format:
   *   long : number of characters in string
   *   data : zero terminated UCS-2 string
   */

  if (! gimp_data_input_stream_read_long (input, &pslen, error) || pslen <= 0)
    return FALSE;

  len = 2 * pslen;

  name_ucs2 = g_new (gchar, len);

  for (i = 0; i < len; i++)
    {
      gchar mychar;

      if (! gimp_data_input_stream_read_char (input, &mychar, error))
        {
          g_free (name_ucs2);
          return FALSE;
        }
      name_ucs2[i] = mychar;
    }

  *value = g_convert (name_ucs2, len,
                      "UTF-8", "UCS-2BE",
                      NULL, NULL, NULL);

  g_free (name_ucs2);

  return (*value != NULL);
}

/* Photoshop resource (brush, pattern) rle decode */
gboolean
gimp_data_input_stream_rle_decode (GDataInputStream  *input,
                                   gchar             *buffer,
                                   gsize              buffer_size,
                                   gint32             height,
                                   GError           **error)
{
  gint      i, j;
  gshort   *cscanline_len = NULL;
  gchar    *cdata         = NULL;
  gchar    *data          = buffer;
  gboolean  result;

  /* read compressed size foreach scanline */
  cscanline_len = gegl_scratch_new (gshort, height);
  for (i = 0; i < height; i++)
    {
      result = gimp_data_input_stream_read_short (input, &cscanline_len[i], error);
      if (! result || cscanline_len[i] <= 0)
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
                   _("Fatal parse error in Photoshop resource file: "
                     "RLE compressed data is corrupt."));
    }
  return FALSE;
}

gboolean
gimp_ascii_strtoi (const gchar  *nptr,
                   gchar       **endptr,
                   gint          base,
                   gint         *result)
{
  gchar  *temp_endptr;
  gint64  temp_result;

  g_return_val_if_fail (nptr != NULL, FALSE);
  g_return_val_if_fail (base == 0 || (base >= 2 && base <= 36), FALSE);

  if (! endptr)
    endptr = &temp_endptr;

  temp_result = g_ascii_strtoll (nptr, endptr, base);

  if (*endptr == nptr || errno == ERANGE ||
      temp_result < G_MININT || temp_result > G_MAXINT)
    {
      errno = 0;

      return FALSE;
    }

  if (result) *result = temp_result;

  return TRUE;
}

gboolean
gimp_ascii_strtod (const gchar  *nptr,
                   gchar       **endptr,
                   gdouble      *result)
{
  gchar   *temp_endptr;
  gdouble  temp_result;

  g_return_val_if_fail (nptr != NULL, FALSE);

  if (! endptr)
    endptr = &temp_endptr;

  temp_result = g_ascii_strtod (nptr, endptr);

  if (*endptr == nptr || errno == ERANGE)
    {
      errno = 0;

      return FALSE;
    }

  if (result) *result = temp_result;

  return TRUE;
}

gchar *
gimp_appstream_to_pango_markup (const gchar *as_text)
{
  return gimp_appstream_parse (as_text, NULL, NULL);
}

void
gimp_appstream_to_pango_markups (const gchar  *as_text,
                                 gchar       **introduction,
                                 GList       **release_items)
{
  gchar * markup;

  markup = gimp_appstream_parse (as_text, introduction, release_items);
  g_free (markup);
}

gint
gimp_g_list_compare (GList *list1,
                     GList *list2)
{
  while (list1 && list2)
    {
      if (list1->data < list2->data)
        return -1;
      else if (list1->data > list2->data)
        return +1;

      list1 = g_list_next (list1);
      list2 = g_list_next (list2);
    }

  if (! list1)
    return -1;
  else if (! list2)
    return +1;

  return 0;
}

GimpTRCType
gimp_suggest_trc_for_component_type (GimpComponentType component_type,
                                     GimpTRCType       old_trc)
{
  GimpTRCType new_trc = old_trc;

  switch (component_type)
    {
    case GIMP_COMPONENT_TYPE_U8:
      /* default to non-linear when converting 8 bit */
      new_trc = GIMP_TRC_NON_LINEAR;
      break;

    case GIMP_COMPONENT_TYPE_U16:
    case GIMP_COMPONENT_TYPE_U32:
    default:
      /* leave TRC alone by default when converting to 16/32 bit int */
      break;

    case GIMP_COMPONENT_TYPE_HALF:
    case GIMP_COMPONENT_TYPE_FLOAT:
    case GIMP_COMPONENT_TYPE_DOUBLE:
      /* default to linear when converting to floating point */
      new_trc = GIMP_TRC_LINEAR;
      break;
    }

  return new_trc;
}

typedef struct
{
  gint              ref_count;

  GimpAsync        *async;
  gint              idle_id;

  GimpRunAsyncFunc  func;
  gpointer          user_data;
  GDestroyNotify    user_data_destroy_func;
} GimpIdleRunAsyncData;

static GimpIdleRunAsyncData *
gimp_idle_run_async_data_new (void)
{
  GimpIdleRunAsyncData *data;

  data = g_slice_new0 (GimpIdleRunAsyncData);

  data->ref_count = 1;

  return data;
}

static void
gimp_idle_run_async_data_inc_ref (GimpIdleRunAsyncData *data)
{
  data->ref_count++;
}

static void
gimp_idle_run_async_data_dec_ref (GimpIdleRunAsyncData *data)
{
  data->ref_count--;

  if (data->ref_count == 0)
    {
      g_signal_handlers_disconnect_by_data (data->async, data);

      if (! gimp_async_is_stopped (data->async))
        gimp_async_abort (data->async);

      g_object_unref (data->async);

      if (data->user_data && data->user_data_destroy_func)
        data->user_data_destroy_func (data->user_data);

      g_slice_free (GimpIdleRunAsyncData, data);
    }
}

static void
gimp_idle_run_async_cancel (GimpAsync            *async,
                            GimpIdleRunAsyncData *data)
{
  gimp_idle_run_async_data_inc_ref (data);

  if (data->idle_id)
    {
      g_source_remove (data->idle_id);

      data->idle_id = 0;
    }

  gimp_idle_run_async_data_dec_ref (data);
}

static void
gimp_idle_run_async_waiting (GimpAsync            *async,
                             GimpIdleRunAsyncData *data)
{
  gimp_idle_run_async_data_inc_ref (data);

  if (data->idle_id)
    {
      g_source_remove (data->idle_id);

      data->idle_id = 0;
    }

  g_signal_handlers_block_by_func (data->async,
                                   gimp_idle_run_async_cancel,
                                   data);

  while (! gimp_async_is_stopped (data->async))
    data->func (data->async, data->user_data);

  g_signal_handlers_unblock_by_func (data->async,
                                     gimp_idle_run_async_cancel,
                                     data);

  data->user_data = NULL;

  gimp_idle_run_async_data_dec_ref (data);
}

static gboolean
gimp_idle_run_async_idle (GimpIdleRunAsyncData *data)
{
  gimp_idle_run_async_data_inc_ref (data);

  g_signal_handlers_block_by_func (data->async,
                                   gimp_idle_run_async_cancel,
                                   data);

  data->func (data->async, data->user_data);

  g_signal_handlers_unblock_by_func (data->async,
                                     gimp_idle_run_async_cancel,
                                     data);

  if (gimp_async_is_stopped (data->async))
    {
      data->user_data = NULL;

      gimp_idle_run_async_data_dec_ref (data);

      return G_SOURCE_REMOVE;
    }

  gimp_idle_run_async_data_dec_ref (data);

  return G_SOURCE_CONTINUE;
}

GimpAsync *
gimp_idle_run_async (GimpRunAsyncFunc func,
                     gpointer         user_data)
{
  return gimp_idle_run_async_full (G_PRIORITY_DEFAULT_IDLE, func,
                                   user_data, NULL);
}

GimpAsync *
gimp_idle_run_async_full (gint             priority,
                          GimpRunAsyncFunc func,
                          gpointer         user_data,
                          GDestroyNotify   user_data_destroy_func)
{
  GimpIdleRunAsyncData *data;

  g_return_val_if_fail (func != NULL, NULL);

  data = gimp_idle_run_async_data_new ();

  data->func                   = func;
  data->user_data              = user_data;
  data->user_data_destroy_func = user_data_destroy_func;

  data->async = gimp_async_new ();

  g_signal_connect (data->async, "cancel",
                    G_CALLBACK (gimp_idle_run_async_cancel),
                    data);
  g_signal_connect (data->async, "waiting",
                    G_CALLBACK (gimp_idle_run_async_waiting),
                    data);

  data->idle_id = g_idle_add_full (
    priority,
    (GSourceFunc) gimp_idle_run_async_idle,
    data,
    (GDestroyNotify) gimp_idle_run_async_data_dec_ref);

  return g_object_ref (data->async);
}

#ifdef G_OS_WIN32

gboolean
gimp_win32_have_wintab (void)
{
  gunichar2 wchars_buffer[MAX_PATH + 1];
  UINT      wchars_count = 0;

  memset (wchars_buffer, 0, sizeof (wchars_buffer));
  wchars_count = GetSystemDirectoryW (wchars_buffer, MAX_PATH);
  if (wchars_count > 0 && wchars_count < MAX_PATH)
    {
      char *system32_directory = g_utf16_to_utf8 (wchars_buffer, -1, NULL, NULL, NULL);

      if (system32_directory)
        {
          GFile    *file   = g_file_new_build_filename (system32_directory, "Wintab32.dll", NULL);
          gboolean  exists = g_file_query_exists (file, NULL);

          g_object_unref (file);
          g_free (system32_directory);

          return exists;
        }
    }

  return FALSE;
}

gboolean
gimp_win32_have_windows_ink (void)
{
  /* Check for Windows 8 or later */
  return g_win32_check_windows_version (6, 2, 0, G_WIN32_OS_ANY);
}

#endif


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

  gimp_create_display (gimp, image, gimp_unit_pixel (), 1.0, NULL);

  /* unref the image unconditionally, even when no display was created */
  g_object_add_weak_pointer (G_OBJECT (image), (gpointer) &image);

  g_object_unref (image);

  if (image)
    g_object_remove_weak_pointer (G_OBJECT (image), (gpointer) &image);

  return image;
}

gint
gimp_view_size_get_larger (gint view_size)
{
  if (view_size < GIMP_VIEW_SIZE_TINY)
    return GIMP_VIEW_SIZE_TINY;
  if (view_size < GIMP_VIEW_SIZE_EXTRA_SMALL)
    return GIMP_VIEW_SIZE_EXTRA_SMALL;
  if (view_size < GIMP_VIEW_SIZE_SMALL)
    return GIMP_VIEW_SIZE_SMALL;
  if (view_size < GIMP_VIEW_SIZE_MEDIUM)
    return GIMP_VIEW_SIZE_MEDIUM;
  if (view_size < GIMP_VIEW_SIZE_LARGE)
    return GIMP_VIEW_SIZE_LARGE;
  if (view_size < GIMP_VIEW_SIZE_EXTRA_LARGE)
    return GIMP_VIEW_SIZE_EXTRA_LARGE;
  if (view_size < GIMP_VIEW_SIZE_HUGE)
    return GIMP_VIEW_SIZE_HUGE;
  if (view_size < GIMP_VIEW_SIZE_ENORMOUS)
    return GIMP_VIEW_SIZE_ENORMOUS;
  return GIMP_VIEW_SIZE_GIGANTIC;
}

gint
gimp_view_size_get_smaller (gint view_size)
{
  if (view_size > GIMP_VIEW_SIZE_GIGANTIC)
    return GIMP_VIEW_SIZE_GIGANTIC;
  if (view_size > GIMP_VIEW_SIZE_ENORMOUS)
    return GIMP_VIEW_SIZE_ENORMOUS;
  if (view_size > GIMP_VIEW_SIZE_HUGE)
    return GIMP_VIEW_SIZE_HUGE;
  if (view_size > GIMP_VIEW_SIZE_EXTRA_LARGE)
    return GIMP_VIEW_SIZE_EXTRA_LARGE;
  if (view_size > GIMP_VIEW_SIZE_LARGE)
    return GIMP_VIEW_SIZE_LARGE;
  if (view_size > GIMP_VIEW_SIZE_MEDIUM)
    return GIMP_VIEW_SIZE_MEDIUM;
  if (view_size > GIMP_VIEW_SIZE_SMALL)
    return GIMP_VIEW_SIZE_SMALL;
  if (view_size > GIMP_VIEW_SIZE_EXTRA_SMALL)
    return GIMP_VIEW_SIZE_EXTRA_SMALL;
  return GIMP_VIEW_SIZE_TINY;
}

/**
 * gimp_version_cmp:
 * @v1: a string representing a version, ex. "2.10.22".
 * @v2: a string representing another version, ex. "2.99.2".
 *
 * If @v2 is %NULL, @v1 is compared to the currently running version.
 *
 * Returns: an integer less than, equal to, or greater than zero if @v1
 *          is found to represent a version respectively, lower than,
 *          matching, or greater than @v2.
 */
gint
gimp_version_cmp (const gchar *v1,
                  const gchar *v2)
{
  gint     major1;
  gint     minor1;
  gint     micro1;
  gint     rc1;
  gboolean is_git1;
  gint     major2  = GIMP_MAJOR_VERSION;
  gint     minor2  = GIMP_MINOR_VERSION;
  gint     micro2  = GIMP_MICRO_VERSION;
  gint     rc2     = 0;
  gboolean is_git2 = FALSE;

#if defined(GIMP_RC_VERSION)
  rc2 = GIMP_RC_VERSION;
#if defined(GIMP_IS_RC_GIT)
  is_git2 = TRUE;
#endif
#endif

  g_return_val_if_fail (v1 != NULL, -1);

  if (! gimp_version_break (v1, &major1, &minor1, &micro1, &rc1, &is_git1))
    {
      /* If version is not properly parsed, something is wrong with
       * upstream version number or parsing. This should not happen.
       */
      g_printerr ("%s: version not properly formatted: %s\n",
                  G_STRFUNC, v1);

      return -1;
    }
  if (v2 && ! gimp_version_break (v2, &major2, &minor2, &micro2, &rc2, &is_git2))
    {
      g_printerr ("%s: version not properly formatted: %s\n",
                  G_STRFUNC, v2);

      return 1;
    }

  if (major1 == major2 && minor1 == minor2 && micro1 == micro2 &&
      rc1 == rc2 && is_git1 == is_git2)
    return 0;
  else if (major1 > major2                                                                    ||
           (major1 == major2 && minor1 > minor2)                                              ||
           (major1 == major2 && minor1 == minor2 && micro1 > micro2)                          ||
           /* RC 0 is the real release, so it's "higher" than any other. */
           (major1 == major2 && minor1 == minor2 && micro1 == micro2 && rc1 == 0 && rc2 > 0)  ||
           (major1 == major2 && minor1 == minor2 && micro1 == micro2 && rc1 > rc2 && rc2 > 0) ||
           (major1 == major2 && minor1 == minor2 && micro1 == micro2 && rc1 == rc2 && is_git1))
    return 1;
  else
    return -1;
}

/* Private functions */


static gchar *
gimp_appstream_parse (const gchar  *as_text,
                      gchar       **introduction,
                      GList       **release_items)
{
  static const GMarkupParser appstream_text_parser =
    {
      appstream_text_start_element,
      appstream_text_end_element,
      appstream_text_characters,
      NULL, /*  passthrough */
      NULL  /*  error       */
    };

  GimpXmlParser *xml_parser;
  gchar         *markup = NULL;
  GError        *error  = NULL;
  ParseState     state;

  state.level           = 0;
  state.foreign_level   = -1;
  state.text            = g_string_new (NULL);
  state.list_num        = 0;
  state.numbered_list   = FALSE;
  state.unnumbered_list = FALSE;
  state.lang            = g_getenv ("LANGUAGE");
  state.original        = NULL;
  state.introduction    = introduction;
  state.release_items   = release_items;

  xml_parser  = gimp_xml_parser_new (&appstream_text_parser, &state);
  if (as_text &&
      ! gimp_xml_parser_parse_buffer (xml_parser, as_text, -1, &error))
    {
      g_printerr ("%s: %s\n", G_STRFUNC, error->message);
      g_error_free (error);
    }

  /* Append possibly last original text without proper localization. */
  if (state.original)
    {
      g_string_append (state.text, state.original->str);
      g_string_free (state.original, TRUE);
    }

  if (release_items)
    *release_items = g_list_reverse (*release_items);

  markup = g_string_free (state.text, FALSE);
  gimp_xml_parser_free (xml_parser);

  return markup;
}

static void
appstream_text_start_element (GMarkupParseContext  *context,
                              const gchar          *element_name,
                              const gchar         **attribute_names,
                              const gchar         **attribute_values,
                              gpointer              user_data,
                              GError              **error)
{
  ParseState  *state  = user_data;
  GString     *output = state->text;
  const gchar *tag_lang;

  state->level++;

  if (state->foreign_level >= 0)
    return;

  tag_lang = gimp_extension_get_tag_lang (attribute_names, attribute_values);
  if ((state->lang == NULL && tag_lang == NULL) ||
      g_strcmp0 (tag_lang, state->lang) == 0)
    {
      /* Current tag is our current language. */
      if (state->original)
        g_string_free (state->original, TRUE);
      state->original = NULL;

      output = state->text;
    }
  else if (tag_lang == NULL)
    {
      /* Current tag is the original language (and we want a
       * localization).
       */
      if (state->original)
        {
          g_string_append (state->text, state->original->str);
          g_string_free (state->original, TRUE);
        }
      state->original = g_string_new (NULL);

      output = state->original;
    }
  else
    {
      /* Current tag is an unrelated language */
      state->foreign_level = state->level;
      return;
    }

  if ((state->numbered_list || state->unnumbered_list) &&
      (g_strcmp0 (element_name, "ul") == 0 ||
       g_strcmp0 (element_name, "ol") == 0))
    {
      g_set_error (error, GIMP_ERROR, GIMP_FAILED,
                   _("This parser does not support imbricated lists."));
    }
  else if (g_strcmp0 (element_name, "ul") == 0)
    {
      state->unnumbered_list = TRUE;
      state->list_num        = 0;
    }
  else if (g_strcmp0 (element_name, "ol") == 0)
    {
      state->numbered_list = TRUE;
      state->list_num      = 0;
    }
  else if (g_strcmp0 (element_name, "li") == 0)
    {
      state->list_num++;
      if (state->numbered_list)
        g_string_append_printf (output,
                                "\n<span weight='ultrabold' >\xe2\x9d\xa8%d\xe2\x9d\xa9</span> ",
                                state->list_num);
      else if (state->unnumbered_list)
        g_string_append (output, "\n<span weight='ultrabold' >\xe2\x80\xa2</span> ");
      else
        g_set_error (error, GIMP_ERROR, GIMP_FAILED,
                     _("<li> must be inside <ol> or <ul> tags."));
    }
  else if (g_strcmp0 (element_name, "p") != 0)
    {
      g_set_error (error, GIMP_ERROR, GIMP_FAILED,
                   _("Unknown tag <%s>."), element_name);
    }
}

static void
appstream_text_end_element (GMarkupParseContext  *context,
                            const gchar          *element_name,
                            gpointer              user_data,
                            GError              **error)
{
  ParseState *state = user_data;

  state->level--;

  if (g_strcmp0 (element_name, "p") == 0)
    {
      if (state->foreign_level < 0)
        {
          if (state->introduction &&
              *state->introduction == NULL)
            *state->introduction = g_strdup (state->original ? state->original->str : state->text->str);

          if (state->original)
            g_string_append (state->original, "\n\n");
          else
            g_string_append (state->text, "\n\n");
        }
    }
  else if (g_strcmp0 (element_name, "ul") == 0 ||
           g_strcmp0 (element_name, "ol") == 0)
    {
      state->numbered_list   = FALSE;
      state->unnumbered_list = FALSE;
    }
  else if (g_strcmp0 (element_name, "li") == 0)
    {
      if (state->original)
        g_string_append (state->original, "\n");
      else
        g_string_append (state->text, "\n");
    }

  if (state->foreign_level > state->level)
    state->foreign_level = -1;
}

static void
appstream_text_characters (GMarkupParseContext  *context,
                           const gchar          *text,
                           gsize                 text_len,
                           gpointer              user_data,
                           GError              **error)
{
  ParseState *state = user_data;

  if (state->foreign_level < 0 && text_len > 0)
    {
      if (state->list_num > 0 && state->release_items)
        {
          GList **items = state->release_items;

          if (state->list_num == g_list_length (*(state->release_items)))
            {
              gchar *tmp = (*items)->data;

              (*items)->data = g_strconcat (tmp, text, NULL);
              g_free (tmp);
            }
          else
            {
              *items = g_list_prepend (*items, g_strdup (text));
            }
        }
      if (state->original)
        g_string_append (state->original, text);
      else
        g_string_append (state->text, text);
    }
}

static const gchar *
gimp_extension_get_tag_lang (const gchar **attribute_names,
                             const gchar **attribute_values)
{
  while (*attribute_names)
    {
      if (! strcmp (*attribute_names, "xml:lang"))
        {
          return *attribute_values;
        }

      attribute_names++;
      attribute_values++;
    }

  return NULL;
}

static gboolean
gimp_version_break (const gchar *v,
                    gint        *major,
                    gint        *minor,
                    gint        *micro,
                    gint        *rc,
                    gboolean    *is_git)
{
  gchar **versions;

  *major  = 0;
  *minor  = 0;
  *micro  = 0;
  *rc     = 0;
  *is_git = FALSE;

  if (v == NULL)
    return FALSE;

  versions = g_strsplit_set (v, ".", 3);
  if (versions[0] != NULL)
    {
      *major = g_ascii_strtoll (versions[0], NULL, 10);
      if (versions[1] != NULL)
        {
          *minor = g_ascii_strtoll (versions[1], NULL, 10);
          if (versions[2] != NULL)
            {
              gchar **micro_rc_git;

              *micro = g_ascii_strtoll (versions[2], NULL, 10);

              micro_rc_git = g_strsplit_set (versions[2], "-", 2);

              if (g_strv_length (micro_rc_git) > 1 &&
                  strlen (micro_rc_git[1]) > 2     &&
                  micro_rc_git[1][0] == 'R'           &&
                  micro_rc_git[1][1] == 'C')
                {
                  gchar **rc_git;

                  *rc = g_ascii_strtoll (micro_rc_git[1] + 2, NULL, 10);

                  rc_git = g_strsplit_set (micro_rc_git[1], "+", 2);

                  if (g_strv_length (rc_git) > 1 &&
                      strlen (rc_git[1]) == 3    &&
                      rc_git[1][0] == 'g'        &&
                      rc_git[1][1] == 'i'        &&
                      rc_git[1][2] == 't')
                    {
                      *is_git = TRUE;
                    }

                  g_strfreev (rc_git);
                }

              g_strfreev (micro_rc_git);
            }
        }
    }

  g_strfreev (versions);

  return (*major > 0 || *minor > 0 || *micro > 0);
}
