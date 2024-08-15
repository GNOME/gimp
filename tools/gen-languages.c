/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gen-languages.c (formerly gimplanguagestore-parser.c)
 * Copyright (C) 2008, 2009  Sven Neumann <sven@gimp.org>
 * Copyright (C) 2013, 2024  Jehan <jehan at girinstud.io>
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

#include <gio/gio.h>
#include <glib.h>

#include "libgimpbase/gimpbase.h"

#include "config/config-types.h"
#include "config/gimpxmlparser.h"

#include "gimp-intl.h"

#define PRINT(...) if (! g_output_stream_printf (output, NULL, NULL, &error, __VA_ARGS__))\
                     {\
                       g_printerr ("ERROR: %s\n", error->message);\
                       g_object_unref (output);\
                       exit (EXIT_FAILURE);\
                     }


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


static gboolean gimp_language_store_parser_init  (GError    **error);
static void     gimp_language_store_parser_clean (void);

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

int
main (int    argc,
      char **argv)
{
  GFile          *outfile;
  GOutputStream  *output;
  GHashTableIter  lang_iter;
  GError         *error = NULL;
  gpointer        code;
  gpointer        name;
  const gchar    *license_header = "\
/* GIMP - The GNU Image Manipulation Program\n\
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis\n\
 *\n\
 * gimplanguagestore-data.h\n\
 * Copyright (C) 2024 Jehan\n\
 *\n\
 * This program is free software: you can redistribute it and/or modify\n\
 * it under the terms of the GNU General Public License as published by\n\
 * the Free Software Foundation; either version 3 of the License, or\n\
 * (at your option) any later version.\n\
 *\n\
 * This program is distributed in the hope that it will be useful,\n\
 * but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
 * GNU General Public License for more details.\n\
 *\n\
 * You should have received a copy of the GNU General Public License\n\
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.\n\
 */\n";

  if (! gimp_language_store_parser_init (&error))
    {
      g_printerr ("ERROR: %s\n", error->message);
      exit (EXIT_FAILURE);
    }

  outfile = g_file_new_build_filename (argv[0], "../../app/widgets/gimplanguagestore-data.h", NULL);
  output = G_OUTPUT_STREAM (g_file_replace (outfile,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, &error));
  g_object_unref (outfile);

  if (! output)
    {
      g_printerr ("ERROR: %s\n", error->message);
      g_object_unref (output);
      exit (EXIT_FAILURE);
    }

  PRINT ("%s", license_header);
  PRINT ("\n/* NOTE: this file is auto-generated by %s */\n\n", argv[0]);

  g_hash_table_iter_init (&lang_iter, l10n_lang_list);

  PRINT ("#define GIMP_L10N_LANGS_SIZE %d\n", g_hash_table_size (l10n_lang_list));
  PRINT ("#define GIMP_ALL_LANGS_SIZE %d\n", g_hash_table_size (all_lang_list));
  PRINT ("\ntypedef struct"
         "\n{"
         "\n  gchar *code;"
         "\n  gchar *name;"
         "\n} GimpLanguageDef;\n");

  PRINT ("\nstatic const GimpLanguageDef GimpL10nLanguages[GIMP_L10N_LANGS_SIZE] =\n{\n");
  while (g_hash_table_iter_next (&lang_iter, &code, &name))
    PRINT ("  { \"%s\", \"%s\" },\n", (gchar *) code, (gchar *) name);
  PRINT ("};\n");

  g_hash_table_iter_init (&lang_iter, all_lang_list);
  PRINT ("\n\nstatic const GimpLanguageDef GimpAllLanguages[GIMP_ALL_LANGS_SIZE] =\n{\n");
  while (g_hash_table_iter_next (&lang_iter, &code, &name))
    PRINT ("  { \"%s\", \"%s\" },\n", (gchar *) code, (gchar *) name);
  PRINT ("};\n");

  gimp_language_store_parser_clean ();

  if (! g_output_stream_close (output, NULL, &error))
    {
      g_printerr ("ERROR: %s\n", error->message);
      g_object_unref (output);
      exit (EXIT_FAILURE);
    }
  g_object_unref (output);
  exit (EXIT_SUCCESS);
}

/*
 * Initialize and run the language listing parser. This call must be
 * made only once, at program initialization, but after language_init().
 */
static gboolean
gimp_language_store_parser_init (GError **error)
{
  GTimer           *timer  = g_timer_new ();
  GInputStream     *input  = NULL;
  GDataInputStream *dinput = NULL;
  GHashTable       *base_lang_list;
  gchar            *current_env;
  GFile            *linguas;
  GHashTableIter    lang_iter;
  gpointer          key;
  gchar            *locale;
  gint              n_locales = 0;
  gint              n_mo      = 0;
  gboolean          success = TRUE;

  if (l10n_lang_list != NULL)
    {
      g_set_error_literal (error, G_IO_ERROR, 0,
                           "gimp_language_store_parser_init() must be run only once.");
      return FALSE;
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
  linguas = g_file_new_build_filename (SRCDIR, "po", "LINGUAS", NULL);
  input = G_INPUT_STREAM (g_file_read (linguas, NULL, error));
  if (! input)
    {
      success = FALSE;
      goto cleanup;
    }

  dinput = g_data_input_stream_new (input);

  while ((locale = g_data_input_stream_read_line (dinput, NULL, NULL, error)))
    {
      g_strstrip (locale);

      if (strlen (locale) > 0 && locale[0] != '#')
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

          n_mo++;
        }

      n_locales++;

      g_free (locale);
    }

  if (n_mo == 0)
    {
      g_set_error (error, G_IO_ERROR, 0,
                   "No \"%s\" files were found in %s in %d locales",
                   GETTEXT_PACKAGE ".mo", gimp_locale_directory (),
                   n_locales);
      success = FALSE;
      goto cleanup;
    }

  /* Parse ISO-639 file to get full list of language and their names. */
  if (! parse_iso_codes (base_lang_list, error))
    {
      success = FALSE;
      goto cleanup;
    }

  /* Generate the localized language names. */
  g_hash_table_iter_init (&lang_iter, l10n_lang_list);
  gimp_bind_text_domain ("iso_639_3", ISOCODES_LOCALEDIR);
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
          g_setenv ("LANG", code, TRUE);
          setlocale (LC_ALL, "");

          localized_name = g_strdup (dgettext ("iso_639_3", english_name));

          /* If original and localized names are the same for other than English,
           * maybe localization failed. Try now in the main dialect. */
          if (g_strcmp0 (english_name, localized_name) == 0 &&
              g_strcmp0 (base_code, "en") != 0 &&
              g_strcmp0 (code, base_code) != 0)
            {
              g_free (localized_name);

              g_setenv ("LANGUAGE", base_code, TRUE);
              g_setenv ("LANG", base_code, TRUE);
              setlocale (LC_ALL, "");

              localized_name = g_strdup (dgettext ("iso_639_3", english_name));
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

  /* Go back to original localization. */
  if (current_env)
    {
      g_setenv ("LANGUAGE", current_env, TRUE);
      g_free (current_env);
    }
  else
    {
      g_unsetenv ("LANGUAGE");
    }
  setlocale (LC_ALL, "");

  /* Add special entry for C (en_US). */
  g_hash_table_insert (l10n_lang_list, g_strdup ("en_US"),
                       g_strdup ("English [en_US]"));

cleanup:
  g_hash_table_destroy (base_lang_list);

  g_timer_stop (timer);
  g_print ("%s: %f seconds\n", G_STRFUNC, g_timer_elapsed (timer, NULL));
  g_timer_destroy (timer);
  g_object_unref (linguas);
  g_clear_object (&input);
  g_clear_object (&dinput);

  return success;
}

static void
gimp_language_store_parser_clean (void)
{
  g_hash_table_destroy (l10n_lang_list);
  g_hash_table_destroy (all_lang_list);
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

  file = g_file_new_for_path (ISO_CODES_LOCATION G_DIR_SEPARATOR_S
                              "iso_639_3.xml");

  success = gimp_xml_parser_parse_gfile (xml_parser, file, error);
  if (*error)
    {
      g_prefix_error (error, "%s: error parsing '%s': ",
                      G_STRFUNC, g_file_peek_path (file));
      success = FALSE;
    }
  else if (g_hash_table_size (all_lang_list) == 0)
    {
      g_set_error (error, G_IO_ERROR, 0,
                   "No language entries found in %s.",
                   g_file_peek_path (file));
      success = FALSE;
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
  gimp_bind_text_domain ("iso_639_3", gimp_locale_directory ());
#else
  gimp_bind_text_domain ("iso_639_3", ISO_CODES_LOCALEDIR);
#endif

  bind_textdomain_codeset ("iso_639_3", "UTF-8");

  initialized = TRUE;
}

static void
iso_codes_parser_entry (IsoCodesParser  *parser,
                        const gchar    **names,
                        const gchar    **values)
{
  const gchar *lang       = NULL;
  const gchar *code       = NULL;
  gboolean     has_part12 = FALSE;

  while (*names && *values)
    {
      if (strcmp (*names, "name") == 0)
        {
          lang = *values;
        }
      else if (strcmp (*names, "part1_code") == 0)
        /* 2-letter ISO 639-1 codes have priority.
         * But some languages have no 2-letter code. Ex: Asturian (ast).
         */
        {
          code = *values;
          has_part12 = TRUE;
        }
      else if (strcmp (*names, "part2_code") == 0 && code == NULL)
        {
          code = *values;
          has_part12 = TRUE;
        }
      else if (strcmp (*names, "id") == 0 && code == NULL)
        {
          code = *values;
        }

      names++;
      values++;
    }

  if (lang && *lang && code && *code)
    {
      gboolean save_anyway = FALSE;

      /* If the language is in our base table, we save its standard English name. */
      if (g_hash_table_contains (parser->base_lang_list, code))
        {
          g_hash_table_replace (parser->base_lang_list, g_strdup (code), g_strdup (lang));
          save_anyway = TRUE;
        }

      /* In any case, we save the name in US English for all lang with
       * part 1 or part 2 code, or for the few language with no part 1|2
       * code but for which we have a localization.
       */
      if (has_part12 || save_anyway)
        g_hash_table_insert (all_lang_list, g_strdup (code), g_strdup (lang));
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
      if (strcmp (element_name, "iso_639_3_entries") == 0)
        {
          parser->state = ISO_CODES_IN_ENTRIES;
          break;
        }

    case ISO_CODES_IN_ENTRIES:
      if (strcmp (element_name, "iso_639_3_entry") == 0)
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
