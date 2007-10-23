/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help plug-in
 * Copyright (C) 1999-2004 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *                         Henrik Brix Andersen <brix@gimp.org>
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

/*  This code is written so that it can also be compiled standalone.
 *  It shouldn't depend on libgimp.
 */

#include "config.h"

#include <string.h>

#include <glib.h>

#include "gimphelp.h"

#ifdef DISABLE_NLS
#define _(String)  (String)
#else
#include "libgimp/stdplugins-intl.h"
#endif


/*  local function prototypes  */

static void   locale_set_error (GError      **error,
                                const gchar  *format,
                                const gchar  *filename);


/*  public functions  */

GimpHelpLocale *
gimp_help_locale_new (const gchar *locale_id)
{
  GimpHelpLocale *locale = g_slice_new0 (GimpHelpLocale);

  locale->locale_id = g_strdup (locale_id);

  return locale;
}

void
gimp_help_locale_free (GimpHelpLocale *locale)
{
  if (locale->help_id_mapping)
    g_hash_table_destroy (locale->help_id_mapping);

  g_free (locale->locale_id);
  g_free (locale->help_missing);

  g_list_free (locale->toplevel_items);

  g_slice_free (GimpHelpLocale, locale);
}

const gchar *
gimp_help_locale_map (GimpHelpLocale *locale,
                      const gchar    *help_id)
{
  if (locale->help_id_mapping)
    {
      GimpHelpItem *item = g_hash_table_lookup (locale->help_id_mapping,
                                                help_id);

      if (item)
        return item->ref;
    }

  return NULL;
}


/*  the locale mapping parser  */

typedef enum
{
  LOCALE_START,
  LOCALE_IN_HELP,
  LOCALE_IN_ITEM,
  LOCALE_IN_MISSING,
  LOCALE_IN_UNKNOWN
} LocaleParserState;

typedef struct
{
  const gchar       *filename;
  LocaleParserState  state;
  LocaleParserState  last_known_state;
  gint               markup_depth;
  gint               unknown_depth;
  GString           *value;

  GimpHelpLocale    *locale;
  const gchar       *help_domain;
  gchar             *id_attr_name;
} LocaleParser;

static gboolean  locale_parser_parse       (GMarkupParseContext  *context,
                                            GIOChannel           *io,
                                            GError              **error);
static void  locale_parser_start_element   (GMarkupParseContext  *context,
                                            const gchar          *element_name,
                                            const gchar         **attribute_names,
                                            const gchar         **attribute_values,
                                            gpointer              user_data,
                                            GError              **error);
static void  locale_parser_end_element     (GMarkupParseContext  *context,
                                            const gchar          *element_name,
                                            gpointer              user_data,
                                            GError              **error);
static void  locale_parser_error           (GMarkupParseContext  *context,
                                            GError               *error,
                                            gpointer              user_data);
static void  locale_parser_start_unknown   (LocaleParser         *parser);
static void  locale_parser_end_unknown     (LocaleParser         *parser);
static void  locale_parser_parse_namespace (LocaleParser         *parser,
                                            const gchar         **names,
                                            const gchar         **values);
static void  locale_parser_parse_item      (LocaleParser         *parser,
                                            const gchar         **names,
                                            const gchar         **values);
static void  locale_parser_parse_missing   (LocaleParser         *parser,
                                            const gchar         **names,
                                            const gchar         **values);

static const GMarkupParser markup_parser =
{
  locale_parser_start_element,
  locale_parser_end_element,
  NULL,  /*  characters   */
  NULL,  /*  passthrough  */
  locale_parser_error
};

gboolean
gimp_help_locale_parse (GimpHelpLocale  *locale,
                        const gchar     *filename,
                        const gchar     *help_domain,
                        GError         **error)
{
  GMarkupParseContext *context;
  GIOChannel          *io;
  LocaleParser         parser = { NULL, };
  gboolean             success;

  g_return_val_if_fail (locale != NULL, FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (locale->help_id_mapping)
    {
      g_hash_table_destroy (locale->help_id_mapping);
      locale->help_id_mapping = NULL;
    }

  if (locale->help_missing)
    {
      g_free (locale->help_missing);
      locale->help_missing = NULL;
    }

#ifdef GIMP_HELP_DEBUG
  g_printerr ("help (%s): parsing '%s' for locale \"%s\"\n",
              locale->locale_id,
              filename,
              help_domain);
#endif

  io = g_io_channel_new_file (filename, "r", error);
  if (! io)
    {
      locale_set_error (error,
                        _("Could not open '%s' for reading: %s"),
                        filename);
      return FALSE;
    }

  parser.filename     = filename;
  parser.value        = g_string_new (NULL);
  parser.locale       = locale;
  parser.help_domain  = help_domain;
  parser.id_attr_name = g_strdup ("id");

  context = g_markup_parse_context_new (&markup_parser, 0, &parser, NULL);

  success = locale_parser_parse (context, io, error);

  g_markup_parse_context_free (context);
  g_io_channel_unref (io);

  g_string_free (parser.value, TRUE);
  g_free (parser.id_attr_name);

  if (! success)
    locale_set_error (error, _("Parse error in '%s':\n%s"), filename);

  return success;
}

static gboolean
locale_parser_parse (GMarkupParseContext  *context,
                     GIOChannel           *io,
                     GError              **error)
{
  GIOStatus  status;
  gsize      len;
  gchar      buffer[4096];

  while (TRUE)
    {
      status = g_io_channel_read_chars (io,
                                        buffer, sizeof (buffer), &len, error);

      switch (status)
        {
        case G_IO_STATUS_ERROR:
          return FALSE;
        case G_IO_STATUS_EOF:
          return g_markup_parse_context_end_parse (context, error);
        case G_IO_STATUS_NORMAL:
          if (! g_markup_parse_context_parse (context, buffer, len, error))
            return FALSE;
          break;
        case G_IO_STATUS_AGAIN:
          break;
        }
    }

  return TRUE;
}

static void
locale_parser_start_element (GMarkupParseContext *context,
                             const gchar         *element_name,
                             const gchar        **attribute_names,
                             const gchar        **attribute_values,
                             gpointer             user_data,
                             GError             **error)
{
  LocaleParser *parser = (LocaleParser *) user_data;

  switch (parser->state)
    {
    case LOCALE_START:
      if (strcmp (element_name, "gimp-help") == 0)
        {
          parser->state = LOCALE_IN_HELP;
          locale_parser_parse_namespace (parser,
                                         attribute_names, attribute_values);
        }
      else
        locale_parser_start_unknown (parser);
      break;

    case LOCALE_IN_HELP:
      if (strcmp (element_name, "help-item") == 0)
        {
          parser->state = LOCALE_IN_ITEM;
          locale_parser_parse_item (parser,
                                    attribute_names, attribute_values);
        }
      else if (strcmp (element_name, "help-missing") == 0)
        {
          parser->state = LOCALE_IN_MISSING;
          locale_parser_parse_missing (parser,
                                       attribute_names, attribute_values);
        }
      else
        locale_parser_start_unknown (parser);
      break;

    case LOCALE_IN_ITEM:
    case LOCALE_IN_MISSING:
    case LOCALE_IN_UNKNOWN:
      locale_parser_start_unknown (parser);
      break;
    }
}

static void
locale_parser_end_element (GMarkupParseContext *context,
                           const gchar         *element_name,
                           gpointer             user_data,
                           GError             **error)
{
  LocaleParser *parser = (LocaleParser *) user_data;

  switch (parser->state)
    {
    case LOCALE_START:
      g_warning ("locale_parser: This shouldn't happen.");
      break;

    case LOCALE_IN_HELP:
      parser->state = LOCALE_START;
      break;

    case LOCALE_IN_ITEM:
    case LOCALE_IN_MISSING:
      parser->state = LOCALE_IN_HELP;
      break;

    case LOCALE_IN_UNKNOWN:
      locale_parser_end_unknown (parser);
      break;
    }
}

static void
locale_parser_error (GMarkupParseContext *context,
                     GError              *error,
                     gpointer             user_data)
{
  LocaleParser *parser = (LocaleParser *) user_data;

  g_printerr ("help (parsing %s): %s", parser->filename, error->message);
}

static void
locale_parser_start_unknown (LocaleParser *parser)
{
  if (parser->unknown_depth == 0)
    parser->last_known_state = parser->state;

  parser->state = LOCALE_IN_UNKNOWN;
  parser->unknown_depth++;
}

static void
locale_parser_end_unknown (LocaleParser *parser)
{
  g_assert (parser->unknown_depth > 0 && parser->state == LOCALE_IN_UNKNOWN);

  parser->unknown_depth--;

  if (parser->unknown_depth == 0)
    parser->state = parser->last_known_state;
}

static void
locale_parser_parse_namespace (LocaleParser  *parser,
                               const gchar  **names,
                               const gchar  **values)
{
  for (; *names && *values; names++, values++)
    {
      if (! strncmp (*names, "xmlns:", 6) &&
          ! strcmp (*values, parser->help_domain))
        {
          g_free (parser->id_attr_name);
          parser->id_attr_name = g_strdup_printf ("%s:id", *names + 6);

#ifdef GIMP_HELP_DEBUG
          g_printerr ("help (%s): id attribute name for \"%s\" is \"%s\"\n",
                      parser->locale->locale_id,
                      parser->help_domain,
                      parser->id_attr_name);
#endif
        }
    }
}

static void
locale_parser_parse_item (LocaleParser  *parser,
                          const gchar  **names,
                          const gchar  **values)
{
  const gchar *id     = NULL;
  const gchar *ref    = NULL;
  const gchar *title  = NULL;
  const gchar *parent = NULL;

  for (; *names && *values; names++, values++)
    {
      if (! strcmp (*names, parser->id_attr_name))
        id = *values;

      if (! strcmp (*names, "ref"))
        ref = *values;

      if (! strcmp (*names, "title"))
        title = *values;

      if (! strcmp (*names, "parent"))
        parent = *values;
    }

  if (id && ref)
    {
      if (! parser->locale->help_id_mapping)
        parser->locale->help_id_mapping =
          g_hash_table_new_full (g_str_hash,
                                 g_str_equal,
                                 g_free,
                                 (GDestroyNotify) gimp_help_item_free);

      g_hash_table_insert (parser->locale->help_id_mapping,
                           g_strdup (id),
                           gimp_help_item_new (ref, title, parent));

#ifdef GIMP_HELP_DEBUG
      g_printerr ("help (%s): added mapping \"%s\" -> \"%s\"\n",
                  parser->locale->locale_id, id, ref);
#endif
    }
}

static void
locale_parser_parse_missing (LocaleParser  *parser,
                             const gchar  **names,
                             const gchar  **values)
{
  const gchar *ref = NULL;

  for (; *names && *values; names++, values++)
    {
      if (! strcmp (*names, "ref"))
        ref = *values;
    }

  if (ref &&
      parser->locale->help_missing == NULL)
    {
      parser->locale->help_missing = g_strdup (ref);

#ifdef GIMP_HELP_DEBUG
      g_printerr ("help (%s): added fallback for missing help -> \"%s\"\n",
                  parser->locale->locale_id, ref);
#endif
    }
}

static void
locale_set_error (GError      **error,
                  const gchar  *format,
                  const gchar  *filename)
{
  if (error && *error)
    {
      gchar *name = g_filename_display_name (filename);
      gchar *msg  = g_strdup_printf (format, name, (*error)->message);

      g_free (name);

      g_free ((*error)->message);
      (*error)->message = msg;
    }
}
