/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <glib-object.h>

#include "vectors-types.h"

#include "core/gimpimage.h"

#include "gimpstroke.h"
#include "gimpvectors.h"
#include "gimpvectors-import.h"

#include "gimp-intl.h"


typedef enum
{
  PARSER_START,
  PARSER_IN_SVG,
  PARSER_IN_PATH,
  PARSER_IN_UNKNOWN
} ParserState;


typedef struct
{
  ParserState   state;
  ParserState   last_known_state;
  gint          unknown_depth;
  GimpVectors  *vectors;
} VectorsParser;


static void  parser_start_element (GMarkupParseContext  *context,
                                   const gchar          *element_name,
                                   const gchar         **attribute_names,
                                   const gchar         **attribute_values,
                                   gpointer              user_data,
                                   GError              **error);
static void  parser_end_element   (GMarkupParseContext  *context,
                                   const gchar          *element_name,
                                   gpointer              user_data,
                                   GError              **error);

static void  parser_start_unknown (VectorsParser        *parser);
static void  parser_end_unknown   (VectorsParser        *parser);

static void  parser_path_data     (VectorsParser        *parser,
                                   const gchar          *data);


static const GMarkupParser markup_parser =
{
  parser_start_element,
  parser_end_element,
  NULL,  /*  characters   */
  NULL,  /*  passthrough  */
  NULL   /*  error        */
};


GimpVectors *
gimp_vectors_import (GimpImage    *image,
                     const gchar  *filename,
                     GError      **error)
{
  GMarkupParseContext *context;
  FILE                *file;
  VectorsParser        parser;
  gsize                bytes;
  gchar                buf[4096];

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  file = fopen (filename, "r");
  if (!file)
    {
      g_set_error (error, 0, 0,
                   _("Failed to open file: '%s': %s"),
                   filename, g_strerror (errno));
      return NULL;
    }

  parser.state   = PARSER_START;
  parser.vectors = gimp_vectors_new (image, _("Imported Path"));

  context = g_markup_parse_context_new (&markup_parser, 0, &parser, NULL);

  while ((bytes = fread (buf, sizeof (gchar), sizeof (buf), file)) > 0 &&
         g_markup_parse_context_parse (context, buf, bytes, error))
    ;

  if (error == NULL || *error == NULL)
    g_markup_parse_context_end_parse (context, error);

  fclose (file);
  g_markup_parse_context_free (context);

  return parser.vectors;
}

static void
parser_start_element (GMarkupParseContext *context,
                      const gchar         *element_name,
                      const gchar        **attribute_names,
                      const gchar        **attribute_values,
                      gpointer             user_data,
                      GError             **error)
{
  VectorsParser *parser = (VectorsParser *) user_data;

  switch (parser->state)
    {
    case PARSER_START:
      if (strcmp (element_name, "svg") == 0)
        parser->state = PARSER_IN_SVG;
      else
        parser_start_unknown (parser);
      break;

    case PARSER_IN_SVG:
      if (strcmp (element_name, "path") == 0)
        parser->state = PARSER_IN_PATH;
      else
        parser_start_unknown (parser);
      break;

    case PARSER_IN_PATH:
    case PARSER_IN_UNKNOWN:
      parser_start_unknown (parser);
      break;
    }

  if (parser->state == PARSER_IN_PATH)
    {
      while (*attribute_names)
        {
          if (strcmp (*attribute_names, "d") == 0)
            parser_path_data (parser, *attribute_values);

          attribute_names++;
          attribute_values++;
        }
    }
}

static void
parser_end_element (GMarkupParseContext *context,
                    const gchar         *element_name,
                    gpointer             user_data,
                    GError             **error)
{
  VectorsParser *parser = (VectorsParser *) user_data;

  switch (parser->state)
    {
    case PARSER_START:
      g_return_if_reached ();
      break;

    case PARSER_IN_SVG:
      parser->state = PARSER_START;
      break;

    case PARSER_IN_PATH:
      parser->state = PARSER_IN_SVG;
      break;

    case PARSER_IN_UNKNOWN:
      parser_end_unknown (parser);
      break;
    }
}

static void
parser_start_unknown (VectorsParser *parser)
{
  if (parser->unknown_depth == 0)
    parser->last_known_state = parser->state;

  parser->state = PARSER_IN_UNKNOWN;
  parser->unknown_depth++;
}

static void
parser_end_unknown (VectorsParser *parser)
{
  g_return_if_fail (parser->unknown_depth > 0 &&
                    parser->state == PARSER_IN_UNKNOWN);

  parser->unknown_depth--;

  if (parser->unknown_depth == 0)
    parser->state = parser->last_known_state;
}

static void
parser_path_data (VectorsParser *parser,
                  const gchar   *data)
{
  /* FIXME */
}
