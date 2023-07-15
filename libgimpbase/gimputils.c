/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimputils.c
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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
#include <stdio.h>

#ifdef PLATFORM_OSX
#include <AppKit/AppKit.h>
#include <libunwind.h>
#endif

#ifdef HAVE_EXECINFO_H
/* Allowing backtrace() API. */
#include <execinfo.h>
#endif

#include <gio/gio.h>
#include <glib/gprintf.h>

#if defined(G_OS_WIN32)
# include <windows.h>
# include <shlobj.h>

#else /* G_OS_WIN32 */

/* For waitpid() */
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

/* For thread IDs. */
#include <sys/types.h>
#include <sys/syscall.h>

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#ifdef HAVE_SYS_THR_H
#include <sys/thr.h>
#endif

#endif /* G_OS_WIN32 */

#include "gimpbasetypes.h"
#include "gimputils.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimputils
 * @title: gimputils
 * @short_description: Utilities of general interest
 *
 * Utilities of general interest
 **/

static gboolean gimp_utils_generic_available (const gchar *program,
                                              gint         major,
                                              gint         minor);
static gboolean gimp_utils_gdb_available     (gint         major,
                                              gint         minor);

/**
 * gimp_utf8_strtrim:
 * @str: (nullable): an UTF-8 encoded string (or %NULL)
 * @max_chars: the maximum number of characters before the string get
 * trimmed
 *
 * Creates a (possibly trimmed) copy of @str. The string is cut if it
 * exceeds @max_chars characters or on the first newline. The fact
 * that the string was trimmed is indicated by appending an ellipsis.
 *
 * Returns: A (possibly trimmed) copy of @str which should be freed
 * using g_free() when it is not needed any longer.
 **/
gchar *
gimp_utf8_strtrim (const gchar *str,
                   gint         max_chars)
{
  /* FIXME: should we make this translatable? */
  const gchar ellipsis[] = "...";
  const gint  e_len      = strlen (ellipsis);

  if (str)
    {
      const gchar *p;
      const gchar *newline = NULL;
      gint         chars   = 0;
      gunichar     unichar;

      for (p = str; *p; p = g_utf8_next_char (p))
        {
          if (++chars > max_chars)
            break;

          unichar = g_utf8_get_char (p);

          switch (g_unichar_break_type (unichar))
            {
            case G_UNICODE_BREAK_MANDATORY:
            case G_UNICODE_BREAK_LINE_FEED:
              newline = p;
              break;
            default:
              continue;
            }

          break;
        }

      if (*p)
        {
          gsize  len     = p - str;
          gchar *trimmed = g_new (gchar, len + e_len + 2);

          memcpy (trimmed, str, len);
          if (newline)
            trimmed[len++] = ' ';

          g_strlcpy (trimmed + len, ellipsis, e_len + 1);

          return trimmed;
        }

      return g_strdup (str);
    }

  return NULL;
}

/**
 * gimp_any_to_utf8: (skip)
 * @str: (array length=len): The string to be converted to UTF-8.
 * @len:            The length of the string, or -1 if the string
 *                  is nul-terminated.
 * @warning_format: The message format for the warning message if conversion
 *                  to UTF-8 fails. See the <function>printf()</function>
 *                  documentation.
 * @...:            The parameters to insert into the format string.
 *
 * This function takes any string (UTF-8 or not) and always returns a valid
 * UTF-8 string.
 *
 * If @str is valid UTF-8, a copy of the string is returned.
 *
 * If UTF-8 validation fails, g_locale_to_utf8() is tried and if it
 * succeeds the resulting string is returned.
 *
 * Otherwise, the portion of @str that is UTF-8, concatenated
 * with "(invalid UTF-8 string)" is returned. If not even the start
 * of @str is valid UTF-8, only "(invalid UTF-8 string)" is returned.
 *
 * Returns: The UTF-8 string as described above.
 **/
gchar *
gimp_any_to_utf8 (const gchar  *str,
                  gssize        len,
                  const gchar  *warning_format,
                  ...)
{
  const gchar *start_invalid;
  gchar       *utf8;

  g_return_val_if_fail (str != NULL, NULL);

  if (g_utf8_validate (str, len, &start_invalid))
    {
      if (len < 0)
        utf8 = g_strdup (str);
      else
        utf8 = g_strndup (str, len);
    }
  else
    {
      utf8 = g_locale_to_utf8 (str, len, NULL, NULL, NULL);
    }

  if (! utf8)
    {
      if (warning_format)
        {
          va_list warning_args;

          va_start (warning_args, warning_format);

          g_logv (G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE,
                  warning_format, warning_args);

          va_end (warning_args);
        }

      if (start_invalid > str)
        {
          gchar *tmp;

          tmp = g_strndup (str, start_invalid - str);
          utf8 = g_strconcat (tmp, " ", _("(invalid UTF-8 string)"), NULL);
          g_free (tmp);
        }
      else
        {
          utf8 = g_strdup (_("(invalid UTF-8 string)"));
        }
    }

  return utf8;
}

/**
 * gimp_filename_to_utf8:
 * @filename: The filename to be converted to UTF-8.
 *
 * Convert a filename in the filesystem's encoding to UTF-8
 * temporarily.  The return value is a pointer to a string that is
 * guaranteed to be valid only during the current iteration of the
 * main loop or until the next call to gimp_filename_to_utf8().
 *
 * The only purpose of this function is to provide an easy way to pass
 * a filename in the filesystem encoding to a function that expects an
 * UTF-8 encoded filename.
 *
 * Returns: A temporarily valid UTF-8 representation of @filename.
 *               This string must not be changed or freed.
 **/
const gchar *
gimp_filename_to_utf8 (const gchar *filename)
{
  /* Simpleminded implementation, but at least allocates just one copy
   * of each translation. Could check if already UTF-8, and if so
   * return filename as is. Could perhaps (re)use a suitably large
   * cyclic buffer, but then would have to verify that all calls
   * really need the return value just for a "short" time.
   */

  static GHashTable *ht = NULL;
  gchar             *filename_utf8;

  if (! filename)
    return NULL;

  if (! ht)
    ht = g_hash_table_new (g_str_hash, g_str_equal);

  filename_utf8 = g_hash_table_lookup (ht, filename);

  if (! filename_utf8)
    {
      filename_utf8 = g_filename_display_name (filename);
      g_hash_table_insert (ht, g_strdup (filename), filename_utf8);
    }

  return filename_utf8;
}

/**
 * gimp_file_get_utf8_name:
 * @file: a #GFile
 *
 * This function works like gimp_filename_to_utf8() and returns
 * a UTF-8 encoded string that does not need to be freed.
 *
 * It converts a #GFile's path or uri to UTF-8 temporarily.  The
 * return value is a pointer to a string that is guaranteed to be
 * valid only during the current iteration of the main loop or until
 * the next call to gimp_file_get_utf8_name().
 *
 * The only purpose of this function is to provide an easy way to pass
 * a #GFile's name to a function that expects an UTF-8 encoded string.
 *
 * See g_file_get_parse_name().
 *
 * Since: 2.10
 *
 * Returns: A temporarily valid UTF-8 representation of @file's name.
 *               This string must not be changed or freed.
 **/
const gchar *
gimp_file_get_utf8_name (GFile *file)
{
  gchar *name;

  g_return_val_if_fail (G_IS_FILE (file), NULL);

  name = g_file_get_parse_name (file);

  g_object_set_data_full (G_OBJECT (file), "gimp-parse-name", name,
                          (GDestroyNotify) g_free);

  return name;
}

/**
 * gimp_file_has_extension:
 * @file:      a #GFile
 * @extension: an ASCII extension
 *
 * This function checks if @file's URI ends with @extension. It behaves
 * like g_str_has_suffix() on g_file_get_uri(), except that the string
 * comparison is done case-insensitively using g_ascii_strcasecmp().
 *
 * Since: 2.10
 *
 * Returns: %TRUE if @file's URI ends with @extension,
 *               %FALSE otherwise.
 **/
gboolean
gimp_file_has_extension (GFile       *file,
                         const gchar *extension)
{
  gchar    *uri;
  gint      uri_len;
  gint      ext_len;
  gboolean  result = FALSE;

  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (extension != NULL, FALSE);

  uri = g_file_get_uri (file);

  uri_len = strlen (uri);
  ext_len = strlen (extension);

  if (uri_len && ext_len && (uri_len > ext_len))
    {
      if (g_ascii_strcasecmp (uri + uri_len - ext_len, extension) == 0)
        result = TRUE;
    }

  g_free (uri);

  return result;
}

/**
 * gimp_file_show_in_file_manager:
 * @file:  a #GFile
 * @error: return location for a #GError
 *
 * Shows @file in the system file manager.
 *
 * Since: 2.10
 *
 * Returns: %TRUE on success, %FALSE otherwise. On %FALSE, @error
 *               is set.
 **/
gboolean
gimp_file_show_in_file_manager (GFile   *file,
                                GError **error)
{
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

#if defined(G_OS_WIN32)

  {
    gboolean ret;
    char *filename;
    LPWSTR w_filename = NULL;
    ITEMIDLIST *pidl = NULL;

    ret = FALSE;

    /* Calling this function multiple times should do no harm, but it is
       easier to put this here as it needs linking against ole32. */
    if (FAILED (CoInitialize (NULL)))
      return ret;

    filename = g_file_get_path (file);
    if (!filename)
      {
        g_set_error_literal (error, G_FILE_ERROR, 0,
                             _("File path is NULL"));
        goto out;
      }

    w_filename = g_utf8_to_utf16 (filename, -1, NULL, NULL, NULL);
    if (!w_filename)
      {
        g_set_error_literal (error, G_FILE_ERROR, 0,
                             _("Error converting UTF-8 filename to wide char"));
        goto out;
      }

    pidl = ILCreateFromPathW (w_filename);
    if (!pidl)
      {
        g_set_error_literal (error, G_FILE_ERROR, 0,
                             _("ILCreateFromPath() failed"));
        goto out;
      }

    SHOpenFolderAndSelectItems (pidl, 0, NULL, 0);
    ret = TRUE;

  out:
    if (pidl)
      ILFree (pidl);
    g_free (w_filename);
    g_free (filename);

    CoUninitialize ();

    return ret;
  }

#elif defined(PLATFORM_OSX)

  {
    gchar    *uri;
    NSString *filename;
    NSURL    *url;
    gboolean  retval = TRUE;

    uri = g_file_get_uri (file);
    filename = [NSString stringWithUTF8String:uri];

    url = [NSURL URLWithString:filename];
    if (url)
      {
        NSArray *url_array = [NSArray arrayWithObject:url];

        [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:url_array];
      }
    else
      {
        g_set_error (error, G_FILE_ERROR, 0,
                     _("Cannot convert '%s' into a valid NSURL."), uri);
        retval = FALSE;
      }

    g_free (uri);

    return retval;
  }

#else /* UNIX */

  {
    GDBusProxy      *proxy;
    GVariant        *retval;
    GVariantBuilder *builder;
    gchar           *uri;

    proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                           G_DBUS_PROXY_FLAGS_NONE,
                                           NULL,
                                           "org.freedesktop.FileManager1",
                                           "/org/freedesktop/FileManager1",
                                           "org.freedesktop.FileManager1",
                                           NULL, error);

    if (! proxy)
      {
        g_prefix_error (error,
                        _("Connecting to org.freedesktop.FileManager1 failed: "));
        return FALSE;
      }

    uri = g_file_get_uri (file);

    builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
    g_variant_builder_add (builder, "s", uri);

    g_free (uri);

    retval = g_dbus_proxy_call_sync (proxy,
                                     "ShowItems",
                                     g_variant_new ("(ass)",
                                                    builder,
                                                    ""),
                                     G_DBUS_CALL_FLAGS_NONE,
                                     -1, NULL, error);

    g_variant_builder_unref (builder);
    g_object_unref (proxy);

    if (! retval)
      {
        g_prefix_error (error, _("Calling ShowItems failed: "));
        return FALSE;
      }

    g_variant_unref (retval);

    return TRUE;
  }

#endif
}

/**
 * gimp_strip_uline:
 * @str: (nullable): underline infested string (or %NULL)
 *
 * This function returns a copy of @str stripped of underline
 * characters. This comes in handy when needing to strip mnemonics
 * from menu paths etc.
 *
 * In some languages, mnemonics are handled by adding the mnemonic
 * character in brackets (like "File (_F)"). This function recognizes
 * this construct and removes the whole bracket construction to get
 * rid of the mnemonic (see bug 157561).
 *
 * Returns: A (possibly stripped) copy of @str which should be
 *               freed using g_free() when it is not needed any longer.
 **/
gchar *
gimp_strip_uline (const gchar *str)
{
  gchar    *escaped;
  gchar    *p;
  gboolean  past_bracket = FALSE;

  if (! str)
    return NULL;

  p = escaped = g_strdup (str);

  while (*str)
    {
      if (*str == '_')
        {
          /*  "__" means a literal "_" in the menu path  */
          if (str[1] == '_')
            {
             *p++ = *str++;
             str++;
             continue;
            }

          /*  find the "(_X)" construct and remove it entirely  */
          if (past_bracket && str[1] && *(g_utf8_next_char (str + 1)) == ')')
            {
              str = g_utf8_next_char (str + 1) + 1;
              p--;
            }
          else
            {
              str++;
            }
        }
      else
        {
          past_bracket = (*str == '(');

          *p++ = *str++;
        }
    }

  *p = '\0';

  return escaped;
}

/**
 * gimp_escape_uline:
 * @str: (nullable): Underline infested string (or %NULL)
 *
 * This function returns a copy of @str with all underline converted
 * to two adjacent underlines. This comes in handy when needing to display
 * strings with underlines (like filenames) in a place that would convert
 * them to mnemonics.
 *
 * Returns: A (possibly escaped) copy of @str which should be
 * freed using g_free() when it is not needed any longer.
 *
 * Since: 2.2
 **/
gchar *
gimp_escape_uline (const gchar *str)
{
  gchar *escaped;
  gchar *p;
  gint   n_ulines = 0;

  if (! str)
    return NULL;

  for (p = (gchar *) str; *p; p++)
    if (*p == '_')
      n_ulines++;

  p = escaped = g_malloc (strlen (str) + n_ulines + 1);

  while (*str)
    {
      if (*str == '_')
        *p++ = '_';

      *p++ = *str++;
    }

  *p = '\0';

  return escaped;
}

/**
 * gimp_is_canonical_identifier:
 * @identifier: The identifier string to check.
 *
 * Checks if @identifier is canonical and non-%NULL.
 *
 * Canonical identifiers are e.g. expected by the PDB for procedure
 * and parameter names. Every character of the input string must be
 * either '-', 'a-z', 'A-Z' or '0-9'.
 *
 * Returns: %TRUE if @identifier is canonical, %FALSE otherwise.
 *
 * Since: 3.0
 **/
gboolean
gimp_is_canonical_identifier (const gchar *identifier)
{
  if (identifier)
    {
      const gchar *p;

      for (p = identifier; *p != 0; p++)
        {
          const gchar c = *p;

          if (! (c == '-' ||
                 (c >= '0' && c <= '9') ||
                 (c >= 'A' && c <= 'Z') ||
                 (c >= 'a' && c <= 'z')))
            {
              return FALSE;
            }
        }

      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_canonicalize_identifier:
 * @identifier: The identifier string to canonicalize.
 *
 * Turns any input string into a canonicalized string.
 *
 * Canonical identifiers are e.g. expected by the PDB for procedure
 * and parameter names. Every character of the input string that is
 * not either '-', 'a-z', 'A-Z' or '0-9' will be replaced by a '-'.
 *
 * Returns: The canonicalized identifier. This is a newly allocated
 *          string that should be freed with g_free() when no longer
 *          needed.
 *
 * Since: 2.4
 **/
gchar *
gimp_canonicalize_identifier (const gchar *identifier)
{
  gchar *canonicalized = NULL;

  if (identifier)
    {
      gchar *p;

      canonicalized = g_strdup (identifier);

      for (p = canonicalized; *p != 0; p++)
        {
          gchar c = *p;

          if (c != '-' &&
              (c < '0' || c > '9') &&
              (c < 'A' || c > 'Z') &&
              (c < 'a' || c > 'z'))
            {
              *p = '-';
            }
        }
    }

  return canonicalized;
}

/**
 * gimp_enum_get_desc:
 * @enum_class: a #GEnumClass
 * @value:      a value from @enum_class
 *
 * Retrieves #GimpEnumDesc associated with the given value, or %NULL.
 *
 * Returns: (nullable): the value's #GimpEnumDesc.
 *
 * Since: 2.2
 **/
const GimpEnumDesc *
gimp_enum_get_desc (GEnumClass *enum_class,
                    gint        value)
{
  const GimpEnumDesc *value_desc;

  g_return_val_if_fail (G_IS_ENUM_CLASS (enum_class), NULL);

  value_desc =
    gimp_enum_get_value_descriptions (G_TYPE_FROM_CLASS (enum_class));

  if (value_desc)
    {
      while (value_desc->value_desc)
        {
          if (value_desc->value == value)
            return value_desc;

          value_desc++;
        }
    }

  return NULL;
}

/**
 * gimp_enum_get_value:
 * @enum_type:  the #GType of a registered enum
 * @value:      an integer value
 * @value_name: (out) (optional): return location for the value's name, or %NULL
 * @value_nick: (out) (optional): return location for the value's nick, or %NULL
 * @value_desc: (out) (optional): return location for the value's translated
 *                                description, or %NULL
 * @value_help: (out) (optional): return location for the value's translated
 *                                help, or %NULL
 *
 * Checks if @value is valid for the enum registered as @enum_type.
 * If the value exists in that enum, its name, nick and its translated
 * description and help are returned (if @value_name, @value_nick,
 * @value_desc and @value_help are not %NULL).
 *
 * Returns: %TRUE if @value is valid for the @enum_type, %FALSE otherwise
 *
 * Since: 2.2
 **/
gboolean
gimp_enum_get_value (GType         enum_type,
                     gint          value,
                     const gchar **value_name,
                     const gchar **value_nick,
                     const gchar **value_desc,
                     const gchar **value_help)
{
  GEnumClass       *enum_class;
  const GEnumValue *enum_value;
  gboolean          success = FALSE;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), FALSE);

  enum_class = g_type_class_ref (enum_type);
  enum_value = g_enum_get_value (enum_class, value);

  if (enum_value)
    {
      if (value_name)
        *value_name = enum_value->value_name;

      if (value_nick)
        *value_nick = enum_value->value_nick;

      if (value_desc || value_help)
        {
          const GimpEnumDesc *enum_desc;

          enum_desc = gimp_enum_get_desc (enum_class, value);

          if (value_desc)
            {
              if (enum_desc && enum_desc->value_desc)
                {
                  const gchar *context;

                  context = gimp_type_get_translation_context (enum_type);

                  if (context)  /*  the new way, using NC_()    */
                    *value_desc = g_dpgettext2 (gimp_type_get_translation_domain (enum_type),
                                                context,
                                                enum_desc->value_desc);
                  else          /*  for backward compatibility  */
                    *value_desc = g_strip_context (enum_desc->value_desc,
                                                   dgettext (gimp_type_get_translation_domain (enum_type),
                                                             enum_desc->value_desc));
                }
              else
                {
                  *value_desc = NULL;
                }
            }

          if (value_help)
            {
              *value_help = ((enum_desc && enum_desc->value_help) ?
                             dgettext (gimp_type_get_translation_domain (enum_type),
                                       enum_desc->value_help) :
                             NULL);
            }
        }

      success = TRUE;
    }

  g_type_class_unref (enum_class);

  return success;
}

/**
 * gimp_enum_value_get_desc:
 * @enum_class: a #GEnumClass
 * @enum_value: a #GEnumValue from @enum_class
 *
 * Retrieves the translated description for a given @enum_value.
 *
 * Returns: the translated description of the enum value
 *
 * Since: 2.2
 **/
const gchar *
gimp_enum_value_get_desc (GEnumClass       *enum_class,
                          const GEnumValue *enum_value)
{
  GType               type = G_TYPE_FROM_CLASS (enum_class);
  const GimpEnumDesc *enum_desc;

  enum_desc = gimp_enum_get_desc (enum_class, enum_value->value);

  if (enum_desc && enum_desc->value_desc)
    {
      const gchar *context;

      context = gimp_type_get_translation_context (type);

      if (context)  /*  the new way, using NC_()    */
        return g_dpgettext2 (gimp_type_get_translation_domain (type),
                             context,
                             enum_desc->value_desc);
      else          /*  for backward compatibility  */
        return g_strip_context (enum_desc->value_desc,
                                dgettext (gimp_type_get_translation_domain (type),
                                          enum_desc->value_desc));
    }

  return enum_value->value_name;
}

/**
 * gimp_enum_value_get_help:
 * @enum_class: a #GEnumClass
 * @enum_value: a #GEnumValue from @enum_class
 *
 * Retrieves the translated help for a given @enum_value.
 *
 * Returns: the translated help of the enum value
 *
 * Since: 2.2
 **/
const gchar *
gimp_enum_value_get_help (GEnumClass       *enum_class,
                          const GEnumValue *enum_value)
{
  GType               type = G_TYPE_FROM_CLASS (enum_class);
  const GimpEnumDesc *enum_desc;

  enum_desc = gimp_enum_get_desc (enum_class, enum_value->value);

  if (enum_desc && enum_desc->value_help)
    return dgettext (gimp_type_get_translation_domain (type),
                     enum_desc->value_help);

  return NULL;
}

/**
 * gimp_enum_value_get_abbrev:
 * @enum_class: a #GEnumClass
 * @enum_value: a #GEnumValue from @enum_class
 *
 * Retrieves the translated abbreviation for a given @enum_value.
 *
 * Returns: the translated abbreviation of the enum value
 *
 * Since: 2.10
 **/
const gchar *
gimp_enum_value_get_abbrev (GEnumClass       *enum_class,
                            const GEnumValue *enum_value)
{
  GType               type = G_TYPE_FROM_CLASS (enum_class);
  const GimpEnumDesc *enum_desc;

  enum_desc = gimp_enum_get_desc (enum_class, enum_value->value);

  if (enum_desc                              &&
      enum_desc[1].value == enum_desc->value &&
      enum_desc[1].value_desc)
    {
      return g_dpgettext2 (gimp_type_get_translation_domain (type),
                           gimp_type_get_translation_context (type),
                           enum_desc[1].value_desc);
    }

  return NULL;
}

/**
 * gimp_flags_get_first_desc:
 * @flags_class: a #GFlagsClass
 * @value:       a value from @flags_class
 *
 * Retrieves the first #GimpFlagsDesc that matches the given value, or %NULL.
 *
 * Returns: (nullable): the value's #GimpFlagsDesc.
 *
 * Since: 2.2
 **/
const GimpFlagsDesc *
gimp_flags_get_first_desc (GFlagsClass *flags_class,
                           guint        value)
{
  const GimpFlagsDesc *value_desc;

  g_return_val_if_fail (G_IS_FLAGS_CLASS (flags_class), NULL);

  value_desc =
    gimp_flags_get_value_descriptions (G_TYPE_FROM_CLASS (flags_class));

  if (value_desc)
    {
      while (value_desc->value_desc)
        {
          if ((value_desc->value & value) == value_desc->value)
            return value_desc;

          value_desc++;
        }
    }

  return NULL;
}

/**
 * gimp_flags_get_first_value:
 * @flags_type: the #GType of registered flags
 * @value:      an integer value
 * @value_name: (out) (optional): return location for the value's name, or %NULL
 * @value_nick: (out) (optional): return location for the value's nick, or %NULL
 * @value_desc: (out) (optional): return location for the value's translated
 *                                description, or %NULL
 * @value_help: (out) (optional): return location for the value's translated
 *                                help, or %NULL
 *
 * Checks if @value is valid for the flags registered as @flags_type.
 * If the value exists in that flags, its name, nick and its
 * translated description and help are returned (if @value_name,
 * @value_nick, @value_desc and @value_help are not %NULL).
 *
 * Returns: %TRUE if @value is valid for the @flags_type, %FALSE otherwise
 *
 * Since: 2.2
 **/
gboolean
gimp_flags_get_first_value (GType         flags_type,
                            guint         value,
                            const gchar **value_name,
                            const gchar **value_nick,
                            const gchar **value_desc,
                            const gchar **value_help)
{
  GFlagsClass       *flags_class;
  const GFlagsValue *flags_value;

  g_return_val_if_fail (G_TYPE_IS_FLAGS (flags_type), FALSE);

  flags_class = g_type_class_peek (flags_type);
  flags_value = g_flags_get_first_value (flags_class, value);

  if (flags_value)
    {
      if (value_name)
        *value_name = flags_value->value_name;

      if (value_nick)
        *value_nick = flags_value->value_nick;

      if (value_desc || value_help)
        {
          const GimpFlagsDesc *flags_desc;

          flags_desc = gimp_flags_get_first_desc (flags_class, value);

          if (value_desc)
            *value_desc = ((flags_desc && flags_desc->value_desc) ?
                           dgettext (gimp_type_get_translation_domain (flags_type),
                                     flags_desc->value_desc) :
                           NULL);

          if (value_help)
            *value_help = ((flags_desc && flags_desc->value_desc) ?
                           dgettext (gimp_type_get_translation_domain (flags_type),
                                     flags_desc->value_help) :
                           NULL);
        }

      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_flags_value_get_desc:
 * @flags_class: a #GFlagsClass
 * @flags_value: a #GFlagsValue from @flags_class
 *
 * Retrieves the translated description for a given @flags_value.
 *
 * Returns: the translated description of the flags value
 *
 * Since: 2.2
 **/
const gchar *
gimp_flags_value_get_desc (GFlagsClass       *flags_class,
                           const GFlagsValue *flags_value)
{
  GType                type = G_TYPE_FROM_CLASS (flags_class);
  const GimpFlagsDesc *flags_desc;

  flags_desc = gimp_flags_get_first_desc (flags_class, flags_value->value);

  if (flags_desc->value_desc)
    {
      const gchar *context;

      context = gimp_type_get_translation_context (type);

      if (context)  /*  the new way, using NC_()    */
        return g_dpgettext2 (gimp_type_get_translation_domain (type),
                             context,
                             flags_desc->value_desc);
      else          /*  for backward compatibility  */
        return g_strip_context (flags_desc->value_desc,
                                dgettext (gimp_type_get_translation_domain (type),
                                          flags_desc->value_desc));
    }

  return flags_value->value_name;
}

/**
 * gimp_flags_value_get_help:
 * @flags_class: a #GFlagsClass
 * @flags_value: a #GFlagsValue from @flags_class
 *
 * Retrieves the translated help for a given @flags_value.
 *
 * Returns: the translated help of the flags value
 *
 * Since: 2.2
 **/
const gchar *
gimp_flags_value_get_help (GFlagsClass       *flags_class,
                           const GFlagsValue *flags_value)
{
  GType                type = G_TYPE_FROM_CLASS (flags_class);
  const GimpFlagsDesc *flags_desc;

  flags_desc = gimp_flags_get_first_desc (flags_class, flags_value->value);

  if (flags_desc->value_help)
    return dgettext (gimp_type_get_translation_domain (type),
                     flags_desc->value_help);

  return NULL;
}

/**
 * gimp_flags_value_get_abbrev:
 * @flags_class: a #GFlagsClass
 * @flags_value: a #GFlagsValue from @flags_class
 *
 * Retrieves the translated abbreviation for a given @flags_value.
 *
 * Returns: the translated abbreviation of the flags value
 *
 * Since: 2.10
 **/
const gchar *
gimp_flags_value_get_abbrev (GFlagsClass       *flags_class,
                             const GFlagsValue *flags_value)
{
  GType                type = G_TYPE_FROM_CLASS (flags_class);
  const GimpFlagsDesc *flags_desc;

  flags_desc = gimp_flags_get_first_desc (flags_class, flags_value->value);

  if (flags_desc                               &&
      flags_desc[1].value == flags_desc->value &&
      flags_desc[1].value_desc)
    {
      return g_dpgettext2 (gimp_type_get_translation_domain (type),
                           gimp_type_get_translation_context (type),
                           flags_desc[1].value_desc);
    }

  return NULL;
}

/**
 * gimp_stack_trace_available:
 * @optimal: whether we get optimal traces.
 *
 * Returns %TRUE if we have dependencies to generate backtraces. If
 * @optimal is %TRUE, the function will return %TRUE only when we
 * are able to generate optimal traces (i.e. with GDB or LLDB);
 * otherwise we return %TRUE even if only backtrace() API is available.
 *
 * On Win32, we return TRUE if Dr. Mingw is built-in, FALSE otherwise.
 *
 * Note: this function is not crash-safe, i.e. you should not try to use
 * it in a callback when the program is already crashing. In such a
 * case, call gimp_stack_trace_print() or gimp_stack_trace_query()
 * directly.
 *
 * Since: 2.10
 **/
gboolean
gimp_stack_trace_available (gboolean optimal)
{
#ifndef G_OS_WIN32
  gchar    *lld_path = NULL;
  gboolean  has_lldb = FALSE;

  /* Similarly to gdb, we could check for lldb by calling:
   * gimp_utils_generic_available ("lldb", major, minor).
   * We don't do so on purpose because on macOS, when lldb is absent, it
   * triggers a popup asking to install Xcode. So instead, we just
   * search for the executable in path.
   * This is the reason why this function is not crash-safe, since
   * g_find_program_in_path() allocates memory.
   * See issue #1999.
   */
  lld_path = g_find_program_in_path ("lldb");
  if (lld_path)
    {
      has_lldb = TRUE;
      g_free (lld_path);
    }

  if (gimp_utils_gdb_available (7, 0) || has_lldb)
    return TRUE;
#ifdef HAVE_EXECINFO_H
  if (! optimal)
    return TRUE;
#endif
#else /* G_OS_WIN32 */
#ifdef HAVE_EXCHNDL
  return TRUE;
#endif
#endif /* G_OS_WIN32 */
  return FALSE;
}

/**
 * gimp_stack_trace_print:
 * @prog_name: the program to attach to.
 * @stream: a FILE* stream.
 * @trace: (out) (optional): location to store a newly allocated string of
 *                           the trace.
 *
 * Attempts to generate a stack trace at current code position in
 * @prog_name. @prog_name is mostly a helper and can be set to NULL.
 * Nevertheless if set, it has to be the current program name (argv[0]).
 * This function is not meant to generate stack trace for third-party
 * programs, and will attach the current process id only.
 * Internally, this function uses `gdb` or `lldb` if they are available,
 * or the stacktrace() API on platforms where it is available. It always
 * fails on Win32.
 *
 * The stack trace, once generated, will either be printed to @stream or
 * returned as a newly allocated string in @trace, if not %NULL.
 *
 * In some error cases (e.g. segmentation fault), trying to allocate
 * more memory will trigger more segmentation faults and therefore loop
 * our error handling (which is just wrong). Therefore printing to a
 * file description is an implementation without any memory allocation.

 * Returns: %TRUE if a stack trace could be generated, %FALSE
 * otherwise.
 *
 * Since: 2.10
 **/
gboolean
gimp_stack_trace_print (const gchar   *prog_name,
                        gpointer      stream,
                        gchar       **trace)
{
  gboolean stack_printed = FALSE;

#ifdef PLATFORM_OSX
  pid_t    pid = getpid();
  uint64   tid64;
  long     tid;
  GString *gtrace = NULL;

  /* On macOS, we can't use gdb or lldb to attach to a process, so we
   * have to use the stacktrace() API.
   */

  unw_cursor_t cursor;
  unw_context_t context;

  unw_getcontext (&context);
  unw_init_local (&cursor, &context);


  pthread_threadid_np (NULL, &tid64);
  tid = (long) tid64;

  if (stream)
      g_fprintf (stream,
                  "\n# Stack traces obtained from PID %d - Thread 0x%lx #\n\n",
                  pid, tid);
  if (trace)
    {
      gtrace = g_string_new (NULL);
      g_string_printf (gtrace,
                        "\n# Stack traces obtained from PID %d - Thread 0x%lx #\n\n",
                        pid, tid);
    }

  while (unw_step (&cursor) > 0)
    {
      unw_word_t offset, pc;
      char fname[64];

      unw_get_reg (&cursor, UNW_REG_IP, &pc);
      fname[0] = '\0';
      unw_get_proc_name (&cursor, fname, sizeof(fname), &offset);

      stack_printed = TRUE;
      if (stream)
        g_fprintf (stream, "%p : (%s+0x%lx)\n", (void *)pc, fname, (unsigned long)offset);
      if (trace)
        g_string_append_printf (gtrace, "%p : (%s+0x%lx)\n", (void *) pc, fname, (unsigned long) offset);
    }

  if (trace)
    *trace = g_string_free (gtrace, FALSE);

  /* Stack printing conflicts with the OS stack printing */
  return stack_printed;

#else /* PLATFORM_OSX */
  /* This works only on UNIX systems. */
#ifndef G_OS_WIN32
  GString *gtrace = NULL;
  gchar    gimp_pid[16];
  gchar    buffer[256];
  ssize_t  read_n;
  int      sync_fd[2];
  int      out_fd[2];
  pid_t    fork_pid;
  pid_t    pid = getpid();
  gint     eintr_count = 0;
#if defined(G_OS_WIN32)
  DWORD    tid = GetCurrentThreadId ();
#elif defined(SYS_gettid)
  long     tid = syscall (SYS_gettid);
#elif defined(HAVE_THR_SELF)
  long     tid = 0;
  thr_self (&tid);
#endif

  g_snprintf (gimp_pid, 16, "%u", (guint) pid);

  if (pipe (sync_fd) == -1)
    {
      return FALSE;
    }

  if (pipe (out_fd) == -1)
    {
      close (sync_fd[0]);
      close (sync_fd[1]);

      return FALSE;
    }

  fork_pid = fork ();
  if (fork_pid == 0)
    {
      /* Child process. */
      gchar *args[9] = { "gdb", "-batch",
                         "-ex", "info threads",
                         /* We used to be able to ask for the full
                          * backtrace of all threads, but a bug,
                          * possibly in gdb, could lock the whole GIMP
                          * process. So for now, let's just have a
                          * backtrace of the main process (most bugs
                          * happen here anyway).
                          * See issue #7539.
                          */
                         /*"-ex", "thread apply all backtrace full",*/
                         "-ex", "backtrace full",
                         (gchar *) prog_name, NULL, NULL };

      if (prog_name == NULL)
        args[6] = "-p";

      args[7] = gimp_pid;

      /* Wait until the parent enabled us to ptrace it. */
      {
        gchar dummy;

        close (sync_fd[1]);
        while (read (sync_fd[0], &dummy, 1) < 0 && errno == EINTR);
        close (sync_fd[0]);
      }

      /* Redirect the debugger output. */
      dup2 (out_fd[1], STDOUT_FILENO);
      close (out_fd[0]);
      close (out_fd[1]);

      /* Run GDB if version 7.0 or over. Why I do such a check is that
       * it turns out older versions may not only fail, but also have
       * very undesirable side effects like terminating the debugged
       * program, at least on FreeBSD where GDB 6.1 is apparently
       * installed by default on the stable release at day of writing.
       * See bug 793514. */
      if (! gimp_utils_gdb_available (7, 0) ||
          execvp (args[0], args) == -1)
        {
          /* LLDB as alternative if the GDB call failed or if it was in
           * a too-old version. */
          gchar *args_lldb[15] = { "lldb", "--attach-pid", NULL, "--batch",
                                   "--one-line", "thread list",
                                   "--one-line", "thread backtrace all",
                                   "--one-line", "bt all",
                                   "--one-line-on-crash", "bt",
                                   "--one-line-on-crash", "quit", NULL };

          args_lldb[2] = gimp_pid;

          execvp (args_lldb[0], args_lldb);
        }

      _exit (0);
    }
  else if (fork_pid > 0)
    {
      /* Main process */
      int status;

      /* Allow the child to ptrace us, and signal it to start. */
      close (sync_fd[0]);
#ifdef PR_SET_PTRACER
      prctl (PR_SET_PTRACER, fork_pid, 0, 0, 0);
#endif
      close (sync_fd[1]);

      /* It is important to close the writing side of the pipe, otherwise
       * the read() will wait forever without getting the information that
       * writing is finished.
       */
      close (out_fd[1]);

      while ((read_n = read (out_fd[0], buffer, 255)) != 0)
        {
          if (read_n < 0)
            {
              /* LLDB on macOS seems to trigger a few EINTR error (see
               * !13), though read() finally ends up working later. So
               * let's not make this error fatal, and instead try again.
               * Yet to avoid infinite loop (in case the error really
               * happens at every call), we abandon after a few
               * consecutive errors.
               * Note: macOS no longer runs through this code path
               */
              if (errno == EINTR && eintr_count <= 5)
                {
                  eintr_count++;
                  continue;
                }
              break;
            }
          eintr_count = 0;
          if (! stack_printed)
            {
#if defined(G_OS_WIN32) || defined(SYS_gettid) || defined(HAVE_THR_SELF)
              if (stream)
                g_fprintf (stream,
                           "\n# Stack traces obtained from PID %d - Thread %lu #\n\n",
                           pid, tid);
#endif
              if (trace)
                {
                  gtrace = g_string_new (NULL);
#if defined(G_OS_WIN32) || defined(SYS_gettid) || defined(HAVE_THR_SELF)
                  g_string_printf (gtrace,
                                   "\n# Stack traces obtained from PID %d - Thread %lu #\n\n",
                                   pid, tid);
#endif
                }
            }
          /* It's hard to know if the debugger was found since it
           * happened in the child. Let's just assume that any output
           * means it succeeded.
           */
          stack_printed = TRUE;

          buffer[read_n] = '\0';
          if (stream)
            g_fprintf (stream, "%s", buffer);
          if (trace)
            g_string_append (gtrace, (const gchar *) buffer);
        }
      close (out_fd[0]);

#ifdef PR_SET_PTRACER
      /* Clear ptrace permission set above */
      prctl (PR_SET_PTRACER, 0, 0, 0, 0);
#endif

      waitpid (fork_pid, &status, 0);
    }
  /* else if (fork_pid == (pid_t) -1)
   * Fork failed!
   * Just continue, maybe the backtrace() API will succeed.
   */

#ifdef HAVE_EXECINFO_H
  if (! stack_printed)
    {
      /* As a last resort, try using the backtrace() Linux API. It is a bit
       * less fancy than gdb or lldb, which is why it is not given priority.
       */
      void *bt_buf[100];
      int   n_symbols;

      n_symbols = backtrace (bt_buf, 100);
      if (trace && n_symbols)
        {
          char **symbols;
          int    i;

          symbols = backtrace_symbols (bt_buf, n_symbols);
          if (symbols)
            {
              for (i = 0; i < n_symbols; i++)
                {
                  if (stream)
                    g_fprintf (stream, "%s\n", (const gchar *) symbols[i]);
                  if (trace)
                    {
                      if (! gtrace)
                        gtrace = g_string_new (NULL);
                      g_string_append (gtrace,
                                       (const gchar *) symbols[i]);
                      g_string_append_c (gtrace, '\n');
                    }
                }
              free (symbols);
            }
        }
      else if (n_symbols)
        {
          /* This allows to generate traces without memory allocation.
           * In some cases, this is necessary, especially during
           * segfault-type crashes.
           */
          backtrace_symbols_fd (bt_buf, n_symbols, fileno ((FILE *) stream));
        }
      stack_printed = (n_symbols > 0);
    }
#endif /* HAVE_EXECINFO_H */

  if (trace)
    {
      if (gtrace)
        *trace = g_string_free (gtrace, FALSE);
      else
        *trace = NULL;
    }
#endif /* G_OS_WIN32 */

  return stack_printed;
#endif /* PLATFORM_OSX */
}

/**
 * gimp_stack_trace_query:
 * @prog_name: the program to attach to.
 *
 * This is mostly the same as g_on_error_query() except that we use our
 * own backtrace function, much more complete.
 * @prog_name must be the current program name (argv[0]).
 * It does nothing on Win32.
 *
 * Since: 2.10
 **/
void
gimp_stack_trace_query (const gchar *prog_name)
{
#ifndef G_OS_WIN32
  gchar buf[16];

 retry:

  g_fprintf (stdout,
             "%s (pid:%u): %s: ",
             prog_name,
             (guint) getpid (),
             "[E]xit, show [S]tack trace or [P]roceed");
  fflush (stdout);

  if (isatty(0) && isatty(1))
    fgets (buf, 8, stdin);
  else
    strcpy (buf, "E\n");

  if ((buf[0] == 'E' || buf[0] == 'e')
      && buf[1] == '\n')
    _exit (0);
  else if ((buf[0] == 'P' || buf[0] == 'p')
           && buf[1] == '\n')
    return;
  else if ((buf[0] == 'S' || buf[0] == 's')
           && buf[1] == '\n')
    {
      if (! gimp_stack_trace_print (prog_name, stdout, NULL))
        g_fprintf (stderr, "%s\n", "Stack trace not available on your system.");
      goto retry;
    }
  else
    goto retry;
#endif
}

/**
 * gimp_range_estimate_settings:
 * @lower: the lower value.
 * @upper: the higher value.
 * @step: (out) (optional): the proposed step increment.
 * @page: (out) (optional): the proposed page increment.
 * @digits: (out) (optional): the proposed decimal places precision.
 *
 * This function proposes reasonable settings for increments and display
 * digits. These can be used for instance on #GtkRange or other widgets
 * using a #GtkAdjustment typically.
 * Note that it will never return @digits with value 0. If you know that
 * your input needs to display integer values, there is no need to set
 * @digits.
 *
 * There is no universal answer to the best increments and number of
 * decimal places. It often depends on context of what the value is
 * meant to represent. This function only tries to provide sensible
 * generic values which can be used when it doesn't matter too much or
 * for generated GUI for instance. If you know exactly how you want to
 * show and interact with a given range, you don't have to use this
 * function.
 */
void
gimp_range_estimate_settings (gdouble  lower,
                              gdouble  upper,
                              gdouble *step,
                              gdouble *page,
                              gint    *digits)
{
  gdouble range;

  g_return_if_fail (upper >= lower);
  g_return_if_fail (step || page || digits);

  range = upper - lower;

  if (range > 0 && range <= 1.0)
    {
      gdouble places = 10.0;

      if (digits)
        *digits = 3;

      /* Compute some acceptable step and page increments always in the
       * format `10**-X` where X is the rounded precision.
       * So for instance:
       *  0.8 will have increments 0.01 and 0.1.
       *  0.3 will have increments 0.001 and 0.01.
       *  0.06 will also have increments 0.001 and 0.01.
       */
      while (range * places < 5.0)
        {
          places *= 10.0;
          if (digits)
            (*digits)++;
        }


      if (step)
        *step = 0.1 / places;
      if (page)
        *page = 1.0 / places;
    }
  else if (range <= 2.0)
    {
      if (step)
        *step = 0.01;
      if (page)
        *page = 0.1;

      if (digits)
        *digits = 3;
    }
  else if (range <= 5.0)
    {
      if (step)
        *step = 0.1;
      if (page)
        *page = 1.0;
      if (digits)
        *digits = 2;
    }
  else if (range <= 40.0)
    {
      if (step)
        *step = 1.0;
      if (page)
        *page = 2.0;
      if (digits)
        *digits = 2;
    }
  else
    {
      if (step)
        *step = 1.0;
      if (page)
        *page = 10.0;
      if (digits)
        *digits = 1;
    }
}

/**
 * gimp_bind_text_domain:
 * @domain_name: a gettext domain name
 * @dir_name:    path of the catalog directory
 *
 * This function wraps bindtextdomain on UNIX and wbintextdomain on Windows.
 * @dir_name is expected to be in the encoding used by the C library on UNIX
 * and UTF-8 on Windows.
 *
 * Since: 3.0
 **/
void
gimp_bind_text_domain (const gchar *domain_name,
                       const gchar *dir_name)
{
#if defined (_WIN32) && !defined (__CYGWIN__)
  wchar_t *dir_name_utf16 = g_utf8_to_utf16 (dir_name, -1, NULL, NULL, NULL);

  if G_UNLIKELY (!dir_name_utf16)
    {
      g_printerr ("[%s] Cannot translate the catalog directory to UTF-16\n", __func__);
    }
  else
    {
      wbindtextdomain (domain_name, dir_name_utf16);
      g_free (dir_name_utf16);
    }
#else
  bindtextdomain (domain_name, dir_name);
#endif
}


/* Private functions. */

static gboolean
gimp_utils_generic_available (const gchar *program,
                              gint         major,
                              gint         minor)
{
#ifndef G_OS_WIN32
  pid_t pid;
  int   out_fd[2];

  if (pipe (out_fd) == -1)
    {
      return FALSE;
    }

  /* XXX: I don't use g_spawn_sync() or similar glib functions because
   * to read the contents of the stdout, these functions would allocate
   * memory dynamically. As we know, when debugging crashes, this is a
   * definite blocker. So instead I simply use a buffer on the stack
   * with a lower level fork() call.
   */
  pid = fork ();
  if (pid == 0)
    {
      /* Child process. */
      gchar *args[3] = { (gchar *) program, "--version", NULL };

      /* Redirect the debugger output. */
      dup2 (out_fd[1], STDOUT_FILENO);
      close (out_fd[0]);
      close (out_fd[1]);

      /* Run version check. */
      execvp (args[0], args);
      _exit (-1);
    }
  else if (pid > 0)
    {
      /* Main process */
      gchar    buffer[256];
      ssize_t  read_n;
      int      status;
      gint     installed_major = 0;
      gint     installed_minor = 0;
      gboolean major_reading = FALSE;
      gboolean minor_reading = FALSE;
      gint     i;
      gchar    c;

      waitpid (pid, &status, 0);

      if (! WIFEXITED (status) || WEXITSTATUS (status) != 0)
        return FALSE;

      /* It is important to close the writing side of the pipe, otherwise
       * the read() will wait forever without getting the information that
       * writing is finished.
       */
      close (out_fd[1]);

      /* I could loop forever until EOL, but I am pretty sure the
       * version information is stored on the first line and one call to
       * read() with 256 characters should be more than enough.
       */
      read_n = read (out_fd[0], buffer, 256);

      /* This is quite a very stupid parser. I only look for the first
       * numbers and consider them as version information. This works
       * fine for both GDB and LLDB as far as I can see for the output
       * of `${program} --version` but this should obviously not be
       * considered as a *really* generic version test.
       */
      for (i = 0; i < read_n; i++)
        {
          c = buffer[i];
          if (c >= '0' && c <= '9')
            {
              if (minor_reading)
                {
                  installed_minor = 10 * installed_minor + (c - '0');
                }
              else
                {
                  major_reading = TRUE;
                  installed_major = 10 * installed_major + (c - '0');
                }
            }
          else if (c == '.')
            {
              if (major_reading)
                {
                  minor_reading = TRUE;
                  major_reading = FALSE;
                }
              else if (minor_reading)
                {
                  break;
                }
            }
          else if (c == '\n')
            {
              /* Version information should be in the first line. */
              break;
            }
        }
      close (out_fd[0]);

      return (installed_major > 0 &&
              (installed_major > major ||
               (installed_major == major && installed_minor >= minor)));
    }
#endif

  /* Fork failed, or Win32. */
  return FALSE;
}

static gboolean
gimp_utils_gdb_available (gint major,
                          gint minor)
{
  return gimp_utils_generic_available ("gdb", major, minor);
}
