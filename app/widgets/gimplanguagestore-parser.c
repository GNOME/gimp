/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplanguagestore-parser.c
 * Copyright (C) 2008, 2009  Sven Neumann <sven@gimp.org>
 * Copyright (C) 2013  Jehan <jehan at girinstud.io>
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

#include <locale.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "widgets-types.h"

#include "config/gimpxmlparser.h"

#include "gimplanguagestore.h"
#include "gimplanguagestore-parser.h"

#include "gimp-intl.h"


typedef enum
{
  ISO_CODES_START,
  ISO_CODES_IN_ENTRIES,
  ISO_CODES_IN_ENTRY,
  ISO_CODES_IN_UNKNOWN
} IsoCodesParserState;

typedef struct
{
  IsoCodesParserState  state;
  IsoCodesParserState  last_known_state;
  gint                 unknown_depth;
  GHashTable          *base_lang_list;
} IsoCodesParser;


static gboolean parse_iso_codes                 (GHashTable  *base_lang_list,
                                                 GError     **error);

#ifdef HAVE_ISO_CODES
static void     iso_codes_parser_init           (void);
static void     iso_codes_parser_start_element  (GMarkupParseContext  *context,
                                                 const gchar          *element_name,
                                                 const gchar         **attribute_names,
                                                 const gchar         **attribute_values,
                                                 gpointer              user_data,
                                                 GError              **error);
static void     iso_codes_parser_end_element    (GMarkupParseContext  *context,
                                                 const gchar          *element_name,
                                                 gpointer              user_data,
                                                 GError              **error);

static void     iso_codes_parser_start_unknown  (IsoCodesParser       *parser);
static void     iso_codes_parser_end_unknown    (IsoCodesParser       *parser);
#endif /* HAVE_ISO_CODES */

/*
 * Language lists that we want to generate only once at program startup:
 * @l10n_lang_list: all available localizations self-localized;
 * @all_lang_list: all known languages, in the user-selected language.
 */
static GHashTable *l10n_lang_list = NULL;
static GHashTable *all_lang_list = NULL;

/********************\
 * Public Functions *
\********************/

/*
 * Initialize and run the language listing parser. This call must be
 * made only once, at program initialization, but after language_init().
 */
void
gimp_language_store_parser_init (void)
{
  GHashTable     *base_lang_list;
  gchar          *current_env;
  GDir           *locales_dir;
  GError         *error = NULL;
  GHashTableIter  lang_iter;
  gpointer        key;

  if (l10n_lang_list != NULL)
    {
      g_warning ("gimp_language_store_parser_init() must be run only once.");
      return;
    }

  current_env = g_strdup (g_getenv ("LANGUAGE"));

  l10n_lang_list = g_hash_table_new_full (g_str_hash, g_str_equal,
                                          (GDestroyNotify) g_free,
                                          (GDestroyNotify) g_free);
  all_lang_list = g_hash_table_new_full (g_str_hash, g_str_equal,
                                         (GDestroyNotify) g_free,
                                         (GDestroyNotify) g_free);
  base_lang_list = g_hash_table_new_full (g_str_hash, g_str_equal,
                                          (GDestroyNotify) g_free,
                                          (GDestroyNotify) g_free);

  /* Check all locales we have translations for. */
  locales_dir = g_dir_open (gimp_locale_directory (), 0, NULL);
  if (locales_dir)
    {
      const gchar *locale;

      while ((locale = g_dir_read_name (locales_dir)) != NULL)
        {
          gchar *filename = g_build_filename (gimp_locale_directory (),
                                              locale,
                                              "LC_MESSAGES",
                                              GETTEXT_PACKAGE ".mo",
                                              NULL);
          if (g_file_test (filename, G_FILE_TEST_EXISTS))
            {
              gchar *delimiter = NULL;
              gchar *base_code = NULL;

              delimiter = strchr (locale, '_');

              if (delimiter)
                base_code = g_strndup (locale, delimiter - locale);
              else
                base_code = g_strdup (locale);

              delimiter = strchr (base_code, '@');

              if (delimiter)
                {
                  gchar *temp = base_code;
                  base_code = g_strndup (base_code, delimiter - base_code);
                  g_free (temp);
                }

              /* Save the full language code. */
              g_hash_table_insert (l10n_lang_list, g_strdup (locale), NULL);
              /* Save the base language code. */
              g_hash_table_insert (base_lang_list, base_code, NULL);
            }

          g_free (filename);
        }

      g_dir_close (locales_dir);
    }

  /* Parse ISO-639 file to get full list of language and their names. */
  parse_iso_codes (base_lang_list, &error);

  /* Generate the localized language names. */
  g_hash_table_iter_init (&lang_iter, l10n_lang_list);
  while (g_hash_table_iter_next (&lang_iter, &key, NULL))
    {
      gchar *code           = (gchar*) key;
      gchar *localized_name = NULL;
      gchar *english_name   = NULL;
      gchar *delimiter      = NULL;
      gchar *base_code      = NULL;

      delimiter = strchr (code, '_');

      if (delimiter)
        base_code = g_strndup (code, delimiter - code);
      else
        base_code = g_strdup (code);

      delimiter = strchr (base_code, '@');

      if (delimiter)
        {
          gchar *temp = base_code;
          base_code = g_strndup (base_code, delimiter - base_code);
          g_free (temp);
        }

      english_name = (gchar*) (g_hash_table_lookup (base_lang_list, base_code));

      if (english_name)
        {
          gchar *semicolon;

          /* If possible, we want to localize a language in itself.
           * If it fails, gettext fallbacks to C (en_US) itself.
           */
          g_setenv ("LANGUAGE", code, TRUE);
          setlocale (LC_ALL, "");

          localized_name = g_strdup (dgettext ("iso_639", english_name));

          /* If original and localized names are the same for other than English,
           * maybe localization failed. Try now in the main dialect. */
          if (g_strcmp0 (english_name, localized_name) == 0 &&
              g_strcmp0 (base_code, "en") != 0 &&
              g_strcmp0 (code, base_code) != 0)
            {
              g_free (localized_name);

              g_setenv ("LANGUAGE", base_code, TRUE);
              setlocale (LC_ALL, "");

              localized_name = g_strdup (dgettext ("iso_639", english_name));
            }

          /*  there might be several language names; use the first one  */
          semicolon = strchr (localized_name, ';');

          if (semicolon)
            {
              gchar *temp = localized_name;
              localized_name = g_strndup (localized_name, semicolon - localized_name);
              g_free (temp);
            }
        }

      g_hash_table_replace (l10n_lang_list, g_strdup(code),
                            g_strdup_printf ("%s [%s]",
                                             localized_name ?
                                             localized_name : "???",
                                             code));
      g_free (localized_name);
      g_free (base_code);
    }

  /*  Add special entries for system locale.
   *  We want the system locale to be localized in itself. */
  g_setenv ("LANGUAGE", setlocale (LC_ALL, NULL), TRUE);
  setlocale (LC_ALL, "");

  /* g_str_hash() does not accept NULL. I give an empty code instead.
   * Other solution would to create a custom hash. */
  g_hash_table_insert (l10n_lang_list, g_strdup(""),
                       g_strdup (_("System Language")));

  /* Go back to original localization. */
  if (current_env)
    {
      g_setenv ("LANGUAGE", current_env, TRUE);
      g_free (current_env);
    }
  else
    g_unsetenv ("LANGUAGE");
  setlocale (LC_ALL, "");

  /* Add special entry for C (en_US). */
  g_hash_table_insert (l10n_lang_list, g_strdup ("en_US"),
                       g_strdup ("English [en_US]"));

  g_hash_table_destroy (base_lang_list);
}

void
gimp_language_store_parser_clean (void)
{
  g_hash_table_destroy (l10n_lang_list);
  g_hash_table_destroy (all_lang_list);
}

/*
 * Returns a Hash table of languages.
 * Keys and values are respectively language codes and names from the
 * ISO-639 standard code.
 *
 * If @localization_only is TRUE, it returns only the list of available
 * GIMP localizations, and language names are translated in their own
 * locale.
 * If @localization_only is FALSE, the full list of ISO-639 languages
 * is returned, and language names are in the user-set locale.
 *
 * Do not free the list or elements of the list.
 */
GHashTable *
gimp_language_store_parser_get_languages (gboolean localization_only)
{
  if (localization_only)
    return l10n_lang_list;
  else
    return all_lang_list;
}

/*****************************\
 * Private Parsing Functions *
\*****************************/

/*
 * Parse the ISO-639 code list if available on this system, and fill
 * @base_lang_list with English names of all needed base codes.
 *
 * It will also fill the static @all_lang_list.
 */
static gboolean
parse_iso_codes (GHashTable  *base_lang_list,
                 GError     **error)
{
  gboolean success = TRUE;

#ifdef HAVE_ISO_CODES
  static const GMarkupParser markup_parser =
    {
      iso_codes_parser_start_element,
      iso_codes_parser_end_element,
      NULL,  /*  characters   */
      NULL,  /*  passthrough  */
      NULL   /*  error        */
    };

  GimpXmlParser  *xml_parser;
  GFile          *file;
  IsoCodesParser  parser = { 0, };

  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  iso_codes_parser_init ();

  parser.base_lang_list = g_hash_table_ref (base_lang_list);

  xml_parser = gimp_xml_parser_new (&markup_parser, &parser);

#if ENABLE_RELOCATABLE_RESOURCES
  file = gimp_installation_directory_file ("share", "xml", "iso-codes",
                                           "iso_639.xml", NULL);
#else
  file = g_file_new_for_path (ISO_CODES_LOCATION G_DIR_SEPARATOR_S
                              "iso_639.xml");
#endif

  success = gimp_xml_parser_parse_gfile (xml_parser, file, error);
  if (error && *error)
    {
      g_warning ("%s: error parsing '%s': %s\n",
                 G_STRFUNC, g_file_get_path (file),
                 (*error)->message);
      g_clear_error (error);
    }

  g_object_unref (file);

  gimp_xml_parser_free (xml_parser);
  g_hash_table_unref (parser.base_lang_list);

#endif /* HAVE_ISO_CODES */

  return success;
}

#ifdef HAVE_ISO_CODES
static void
iso_codes_parser_init (void)
{
  static gboolean initialized = FALSE;

  if (initialized)
    return;

#ifdef G_OS_WIN32
  /*  on Win32, assume iso-codes is installed in the same location as GIMP  */
  bindtextdomain ("iso_639", gimp_locale_directory ());
#else
  bindtextdomain ("iso_639", ISO_CODES_LOCALEDIR);
#endif

  bind_textdomain_codeset ("iso_639", "UTF-8");

  initialized = TRUE;
}

static void
iso_codes_parser_entry (IsoCodesParser  *parser,
                        const gchar    **names,
                        const gchar    **values)
{
  const gchar *lang = NULL;
  const gchar *code = NULL;

  while (*names && *values)
    {
      if (strcmp (*names, "name") == 0)
        lang = *values;
      else if (strcmp (*names, "iso_639_2B_code") == 0 && code == NULL)
        /* 2-letter ISO 639-1 codes have priority.
         * But some languages have no 2-letter code. Ex: Asturian (ast).
         */
        code = *values;
      else if (strcmp (*names, "iso_639_2T_code") == 0 && code == NULL)
        code = *values;
      else if (strcmp (*names, "iso_639_1_code") == 0)
        code = *values;

      names++;
      values++;
    }

  if (lang && *lang && code && *code)
    {
      gchar *semicolon;
      gchar *localized_name = g_strdup (dgettext ("iso_639", lang));

      /* If the language is in our base table, we save its standard English name. */
      if (g_hash_table_contains (parser->base_lang_list, code))
        g_hash_table_replace (parser->base_lang_list, g_strdup (code), g_strdup (lang));

      /*  there might be several language names; use the first one  */
      semicolon = strchr (localized_name, ';');

      if (semicolon)
        {
          gchar *temp = localized_name;
          localized_name = g_strndup (localized_name, semicolon - localized_name);
          g_free (temp);
        }
      /* In any case, we save the name in user-set language for all lang. */
      g_hash_table_insert (all_lang_list, g_strdup (code), localized_name);
    }
}

static void
iso_codes_parser_start_element (GMarkupParseContext  *context,
                                const gchar          *element_name,
                                const gchar         **attribute_names,
                                const gchar         **attribute_values,
                                gpointer              user_data,
                                GError              **error)
{
  IsoCodesParser *parser = user_data;

  switch (parser->state)
    {
    case ISO_CODES_START:
      if (strcmp (element_name, "iso_639_entries") == 0)
        {
          parser->state = ISO_CODES_IN_ENTRIES;
          break;
        }

    case ISO_CODES_IN_ENTRIES:
      if (strcmp (element_name, "iso_639_entry") == 0)
        {
          parser->state = ISO_CODES_IN_ENTRY;
          iso_codes_parser_entry (parser, attribute_names, attribute_values);
          break;
        }

    case ISO_CODES_IN_ENTRY:
    case ISO_CODES_IN_UNKNOWN:
      iso_codes_parser_start_unknown (parser);
      break;
    }
}

static void
iso_codes_parser_end_element (GMarkupParseContext *context,
                              const gchar         *element_name,
                              gpointer             user_data,
                              GError             **error)
{
  IsoCodesParser *parser = user_data;

  switch (parser->state)
    {
    case ISO_CODES_START:
      g_warning ("%s: shouldn't get here", G_STRLOC);
      break;

    case ISO_CODES_IN_ENTRIES:
      parser->state = ISO_CODES_START;
      break;

    case ISO_CODES_IN_ENTRY:
      parser->state = ISO_CODES_IN_ENTRIES;
      break;

    case ISO_CODES_IN_UNKNOWN:
      iso_codes_parser_end_unknown (parser);
      break;
    }
}

static void
iso_codes_parser_start_unknown (IsoCodesParser *parser)
{
  if (parser->unknown_depth == 0)
    parser->last_known_state = parser->state;

  parser->state = ISO_CODES_IN_UNKNOWN;
  parser->unknown_depth++;
}

static void
iso_codes_parser_end_unknown (IsoCodesParser *parser)
{
  gimp_assert (parser->unknown_depth > 0 &&
               parser->state == ISO_CODES_IN_UNKNOWN);

  parser->unknown_depth--;

  if (parser->unknown_depth == 0)
    parser->state = parser->last_known_state;
}
#endif /* HAVE_ISO_CODES */
