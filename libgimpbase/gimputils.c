/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimputils.c
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <glib.h>

#include "gimputils.h"

#include "libgimp/libgimp-intl.h"


/**
 * gimp_utf8_strtrim:
 * @str: an UTF-8 encoded string (or %NULL)
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
  static const gchar *ellipsis = "...";
  static const gint   e_len    = 3;

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
 * gimp_any_to_utf8:
 * @str:            The string to be converted to UTF-8.
 * @len:            The length of the string, or -1 if the string
 *                  is nul-terminated.
 * @warning_format: The message format for the warning message if conversion
 *                  to UTF-8 fails. See the <function>printf()</function>
 *                  documentation.
 * @Varargs:        The parameters to insert into the format string.
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
 * Return value: The UTF-8 string as described above.
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
    utf8 = g_strdup (str);
  else
    utf8 = g_locale_to_utf8 (str, len, NULL, NULL, NULL);

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
          utf8 = g_strconcat (tmp, _("(invalid UTF-8 string)"), NULL);
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
 * Return value: A temporarily valid UTF-8 representation of @filename.
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
      filename_utf8 = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
      g_hash_table_insert (ht, g_strdup (filename), filename_utf8);
    }

  return filename_utf8;
}

/**
 * gimp_memsize_to_string:
 * @memsize: A memory size in bytes.
 *
 * This function returns a human readable, translated representation
 * of the passed @memsize. Large values are displayed using a
 * reasonable memsize unit, e.g.: "345" becomes "345 Bytes", "4500"
 * becomes "4.4 KB" and so on.
 *
 * Return value: A newly allocated human-readable, translated string.
 **/
gchar *
gimp_memsize_to_string (guint64 memsize)
{
#if defined _MSC_VER && (_MSC_VER < 1200)
/* sorry, error C2520: conversion from unsigned __int64 to double not
 *                     implemented, use signed __int64
 */
#  define CAST_DOUBLE (gdouble)(gint64)
#else
#  define CAST_DOUBLE (gdouble)
#endif

  if (memsize < 1024)
    {
      return g_strdup_printf (_("%d Bytes"), (gint) memsize);
    }

  if (memsize < 1024 * 10)
    {
      return g_strdup_printf (_("%.2f KB"), CAST_DOUBLE memsize / 1024.0);
    }
  else if (memsize < 1024 * 100)
    {
      return g_strdup_printf (_("%.1f KB"), CAST_DOUBLE memsize / 1024.0);
    }
  else if (memsize < 1024 * 1024)
    {
      return g_strdup_printf (_("%d KB"), (gint) memsize / 1024);
    }

  memsize /= 1024;

  if (memsize < 1024 * 10)
    {
      return g_strdup_printf (_("%.2f MB"), CAST_DOUBLE memsize / 1024.0);
    }
  else if (memsize < 1024 * 100)
    {
      return g_strdup_printf (_("%.1f MB"), CAST_DOUBLE memsize / 1024.0);
    }
  else if (memsize < 1024 * 1024)
    {
      return g_strdup_printf (_("%d MB"), (gint) memsize / 1024);
    }

  memsize /= 1024;

  if (memsize < 1024 * 10)
    {
      return g_strdup_printf (_("%.2f GB"), CAST_DOUBLE memsize / 1024.0);
    }
  else if (memsize < 1024 * 100)
    {
      return g_strdup_printf (_("%.1f GB"), CAST_DOUBLE memsize / 1024.0);
    }
  else
    {
      return g_strdup_printf (_("%d GB"), (gint) memsize / 1024);
    }
#undef CAST_DOUBLE
}

/**
 * gimp_strip_uline:
 * @str: Underline infested string (or %NULL)
 *
 * This function returns a copy of @str stripped of underline
 * characters. This comes in handy when needing to strip mnemonics
 * from menu paths etc.
 *
 * Return value: A (possibly stripped) copy of @str which should be
 * freed using g_free() when it is not needed any longer.
 **/
gchar *
gimp_strip_uline (const gchar *str)
{
  gchar *escaped;
  gchar *p;

  if (! str)
    return NULL;

  p = escaped = g_strdup (str);

  while (*str)
    {
      if (*str == '_')
        {
          /*  "__" means a literal "_" in the menu path  */
          if (str[1] == '_')
            *p++ = *str++;

          str++;
        }
      else
        {
          *p++ = *str++;
        }
    }

  *p = '\0';

  return escaped;
}
