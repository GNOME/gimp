/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplanguagestore-parser.c
 * Copyright (C) 2008  Sven Neumann <sven@gimp.org>
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

#include <string.h>

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "config/gimpxmlparser.h"

#include "gimplanguagestore.h"
#include "gimplanguagestore-parser.h"


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
  GString             *value;
  GimpLanguageStore   *store;
} IsoCodesParser;


static void  iso_codes_parser_start_element (GMarkupParseContext  *context,
                                             const gchar          *element_name,
                                             const gchar         **attribute_names,
                                             const gchar         **attribute_values,
                                             gpointer              user_data,
                                             GError              **error);
static void  iso_codes_parser_end_element   (GMarkupParseContext  *context,
                                             const gchar          *element_name,
                                             gpointer              user_data,
                                             GError              **error);
static void  iso_codes_parser_characters    (GMarkupParseContext  *context,
                                             const gchar          *text,
                                             gsize                 text_len,
                                             gpointer              user_data,
                                             GError              **error);

static void  iso_codes_parser_start_unknown (IsoCodesParser       *parser);
static void  iso_codes_parser_end_unknown   (IsoCodesParser       *parser);


static const GMarkupParser markup_parser =
{
  iso_codes_parser_start_element,
  iso_codes_parser_end_element,
  iso_codes_parser_characters,
  NULL,  /*  passthrough  */
  NULL   /*  error        */
};


gboolean
gimp_language_store_populate (GimpLanguageStore  *store,
                              GError            **error)
{
#ifdef HAVE_ISO_CODES
  GimpXmlParser  *xml_parser;
  gchar          *filename;
  IsoCodesParser  parser = { 0, };

  g_return_val_if_fail (GIMP_IS_LANGUAGE_STORE (store), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  parser.value = g_string_new (NULL);

  xml_parser = gimp_xml_parser_new (&markup_parser, &parser);

  filename = g_build_filename (ISO_CODES_LOCATION, "iso_639.xml", NULL);

  gimp_xml_parser_parse_file (xml_parser, filename, error);

  gimp_xml_parser_free (xml_parser);
  g_free (filename);
  g_string_free (parser.value, TRUE);
#endif

  return TRUE;
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
    default:
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
    default:
      iso_codes_parser_end_unknown (parser);
      break;
    }
}

static void
iso_codes_parser_characters (GMarkupParseContext *context,
                             const gchar         *text,
                             gsize                text_len,
                             gpointer             user_data,
                             GError             **error)
{
  IsoCodesParser *parser = user_data;

  switch (parser->state)
    {
    default:
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
  g_assert (parser->unknown_depth > 0 && parser->state == ISO_CODES_IN_UNKNOWN);

  parser->unknown_depth--;

  if (parser->unknown_depth == 0)
    parser->state = parser->last_known_state;
}
