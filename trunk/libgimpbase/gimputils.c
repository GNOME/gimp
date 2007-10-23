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
#include <stdio.h>

#include <glib-object.h>

#include "gimpbasetypes.h"
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
      filename_utf8 = g_filename_display_name (filename);
      g_hash_table_insert (ht, g_strdup (filename), filename_utf8);
    }

  return filename_utf8;
}


/**
 * gimp_strip_uline:
 * @str: underline infested string (or %NULL)
 *
 * This function returns a copy of @str stripped of underline
 * characters. This comes in handy when needing to strip mnemonics
 * from menu paths etc.
 *
 * In some languages, mnemonics are handled by adding the mnemonic
 * character in brackets (like "File (_F)"). This function recognizes
 * this construct and removes the whole bracket construction to get
 * rid of the mnemonic (see bug #157561).
 *
 * Return value: A (possibly stripped) copy of @str which should be
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
 * @str: Underline infested string (or %NULL)
 *
 * This function returns a copy of @str with all underline converted
 * to two adjacent underlines. This comes in handy when needing to display
 * strings with underlines (like filenames) in a place that would convert
 * them to mnemonics.
 *
 * Return value: A (possibly escaped) copy of @str which should be
 * freed using g_free() when it is not needed any longer.
 *
 * Since: GIMP 2.2
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
 * gimp_canonicalize_identifier:
 * @identifier: The identifier string to canonicalize.
 *
 * Turns any input string into a canonicalized string.
 *
 * Canonical identifiers are e.g. expected by the PDB for procedure
 * and parameter names. Every character of the input string that is
 * not either '-', 'a-z', 'A-Z' or '0-9' will be replaced by a '-'.
 *
 * Return value: The canonicalized identifier. This is a newly
 *               allocated string that should be freed with g_free()
 *               when no longer needed.
 *
 * Since: GIMP 2.4
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
            *p = '-';
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
 * Return value: the value's #GimpEnumDesc.
 *
 * Since: GIMP 2.2
 **/
GimpEnumDesc *
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
            return (GimpEnumDesc *) value_desc;

          value_desc++;
        }
    }

  return NULL;
}

/**
 * gimp_enum_get_value:
 * @enum_type:  the #GType of a registered enum
 * @value:      an integer value
 * @value_name: return location for the value's name (or %NULL)
 * @value_nick: return location for the value's nick (or %NULL)
 * @value_desc: return location for the value's translated desc (or %NULL)
 * @value_help: return location for the value's translated help (or %NULL)
 *
 * Checks if @value is valid for the enum registered as @enum_type.
 * If the value exists in that enum, its name, nick and its translated
 * desc and help are returned (if @value_name, @value_nick, @value_desc
 * and @value_help are not %NULL).
 *
 * Return value: %TRUE if @value is valid for the @enum_type,
 *               %FALSE otherwise
 *
 * Since: GIMP 2.2
 **/
gboolean
gimp_enum_get_value (GType         enum_type,
                     gint          value,
                     const gchar **value_name,
                     const gchar **value_nick,
                     const gchar **value_desc,
                     const gchar **value_help)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  gboolean    success = FALSE;

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
          GimpEnumDesc *enum_desc;

          enum_desc = gimp_enum_get_desc (enum_class, value);

          if (value_desc) {
            *value_desc = ((enum_desc && enum_desc->value_desc) ?
                           g_strip_context (enum_desc->value_desc,
                                            dgettext (gimp_type_get_translation_domain (enum_type),
                                                      enum_desc->value_desc)) :
                           NULL);
          }

          if (value_help)
            *value_help = ((enum_desc && enum_desc->value_desc) ?
                           dgettext (gimp_type_get_translation_domain (enum_type),
                                     enum_desc->value_help) :
                           NULL);
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
 * Retrieves the translated desc for a given @enum_value.
 *
 * Return value: the translated desc of the enum value
 *
 * Since: GIMP 2.2
 **/
const gchar *
gimp_enum_value_get_desc (GEnumClass *enum_class,
                          GEnumValue *enum_value)
{
  GType         type = G_TYPE_FROM_CLASS (enum_class);
  GimpEnumDesc *enum_desc;

  enum_desc = gimp_enum_get_desc (enum_class, enum_value->value);

  if (enum_desc && enum_desc->value_desc)
    return g_strip_context (enum_desc->value_desc,
                            dgettext (gimp_type_get_translation_domain (type),
                                      enum_desc->value_desc));

  return enum_value->value_name;
}

/**
 * gimp_enum_value_get_help:
 * @enum_class: a #GEnumClass
 * @enum_value: a #GEnumValue from @enum_class
 *
 * Retrieves the translated help for a given @enum_value.
 *
 * Return value: the translated help of the enum value
 *
 * Since: GIMP 2.2
 **/
const gchar *
gimp_enum_value_get_help (GEnumClass *enum_class,
                          GEnumValue *enum_value)
{
  GType         type = G_TYPE_FROM_CLASS (enum_class);
  GimpEnumDesc *enum_desc;

  enum_desc = gimp_enum_get_desc (enum_class, enum_value->value);

  if (enum_desc && enum_desc->value_help)
    return dgettext (gimp_type_get_translation_domain (type),
                     enum_desc->value_help);

  return NULL;
}

/**
 * gimp_flags_get_first_desc:
 * @flags_class: a #GFlagsClass
 * @value:       a value from @flags_class
 *
 * Retrieves the first #GimpFlagsDesc that matches the given value, or %NULL.
 *
 * Return value: the value's #GimpFlagsDesc.
 *
 * Since: GIMP 2.2
 **/
GimpFlagsDesc *
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
            return (GimpFlagsDesc *) value_desc;

          value_desc++;
        }
    }

  return NULL;
}

/**
 * gimp_flags_get_first_value:
 * @flags_type: the #GType of registered flags
 * @value:      an integer value
 * @value_name: return location for the value's name (or %NULL)
 * @value_nick: return location for the value's nick (or %NULL)
 * @value_desc: return location for the value's translated desc (or %NULL)
 * @value_help: return location for the value's translated help (or %NULL)
 *
 * Checks if @value is valid for the flags registered as @flags_type.
 * If the value exists in that flags, its name, nick and its translated
 * desc and help are returned (if @value_name, @value_nick, @value_desc
 * and @value_help are not %NULL).
 *
 * Return value: %TRUE if @value is valid for the @flags_type,
 *               %FALSE otherwise
 *
 * Since: GIMP 2.2
 **/
gboolean
gimp_flags_get_first_value (GType         flags_type,
                            guint         value,
                            const gchar **value_name,
                            const gchar **value_nick,
                            const gchar **value_desc,
                            const gchar **value_help)
{
  GFlagsClass *flags_class;
  GFlagsValue *flags_value;

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
          GimpFlagsDesc *flags_desc;

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
 * Retrieves the translated desc for a given @flags_value.
 *
 * Return value: the translated desc of the flags value
 *
 * Since: GIMP 2.2
 **/
const gchar *
gimp_flags_value_get_desc (GFlagsClass *flags_class,
                           GFlagsValue *flags_value)
{
  GType         type = G_TYPE_FROM_CLASS (flags_class);
  GimpFlagsDesc *flags_desc;

  flags_desc = gimp_flags_get_first_desc (flags_class, flags_value->value);

  if (flags_desc->value_desc)
    return dgettext (gimp_type_get_translation_domain (type),
                     flags_desc->value_desc);

  return flags_value->value_name;
}

/**
 * gimp_flags_value_get_help:
 * @flags_class: a #GFlagsClass
 * @flags_value: a #GFlagsValue from @flags_class
 *
 * Retrieves the translated help for a given @flags_value.
 *
 * Return value: the translated help of the flags value
 *
 * Since: GIMP 2.2
 **/
const gchar *
gimp_flags_value_get_help (GFlagsClass *flags_class,
                           GFlagsValue *flags_value)
{
  GType         type = G_TYPE_FROM_CLASS (flags_class);
  GimpFlagsDesc *flags_desc;

  flags_desc = gimp_flags_get_first_desc (flags_class, flags_value->value);

  if (flags_desc->value_help)
    return dgettext (gimp_type_get_translation_domain (type),
                     flags_desc->value_help);

  return NULL;
}
