/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * tips-parser.c -- Parse the gimp-tips.xml file.
 * Copyright (C) 2002  Sven Neumann <sven@gimp.org>
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

#include <glib.h>

#include "tips-parser.h"

#include "libgimp/gimpintl.h"


typedef enum
{
  TIPS_START,
  TIPS_IN_TIPS,
  TIPS_IN_TIP,
  TIPS_IN_WELCOME,
  TIPS_IN_THETIP,
  TIPS_UNKNOWN
} TipsParserState;

typedef enum
{
  TIPS_LOCALE_NONE,
  TIPS_LOCALE_MATCH,
  TIPS_LOCALE_MISMATCH,
} TipsParserLocaleState;

typedef struct _TipsParser TipsParser;
struct _TipsParser
{
  TipsParserState        state;
  TipsParserState        last_known_state;
  const gchar           *locale;
  TipsParserLocaleState  locale_state;
  gint                   unknown_depth;
  GString               *value;

  GimpTip               *current_tip;
  GList                 *tips;
};


static void  tips_parser_start_element (GMarkupParseContext  *context,
                                        const gchar          *element_name,
                                        const gchar         **attribute_names,
                                        const gchar         **attribute_values,
                                        gpointer              user_data,
                                        GError              **error);
static void  tips_parser_end_element   (GMarkupParseContext  *context,
                                        const gchar          *element_name,
                                        gpointer              user_data,
                                        GError              **error);
static void  tips_parser_characters    (GMarkupParseContext  *context,
                                        const gchar          *text,
                                        gsize                 text_len,
                                        gpointer              user_data,
                                        GError              **error);
static void  tips_parser_error         (GMarkupParseContext  *context,
                                        GError               *error,
                                        gpointer              user_data);

static void tips_parser_start_unknown  (TipsParser   *parser);
static void tips_parser_end_unknown    (TipsParser   *parser);
static void tips_parser_parse_locale   (TipsParser   *parser,
                                        const gchar **names,
                                        const gchar **values);
static void  tips_parser_set_by_locale (TipsParser   *parser,
                                        gchar       **dest);


static const GMarkupParser markup_parser =
{
  tips_parser_start_element,
  tips_parser_end_element,
  tips_parser_characters,
  NULL,  /*  passthrough  */
  tips_parser_error
};


GimpTip *
gimp_tip_new  (const gchar *welcome,
               const gchar *thetip)
{
  GimpTip *tip = g_new (GimpTip, 1);

  tip->welcome = welcome ? g_strdup (welcome) : NULL;
  tip->thetip  = thetip  ? g_strdup (thetip)  : NULL;

  return tip;
}

void
gimp_tip_free (GimpTip *tip)
{
  if (!tip)
    return;

  g_free (tip->welcome);
  g_free (tip->thetip);
  g_free (tip);
}

GList *
gimp_tips_from_file (const gchar  *filename,
                     const gchar  *locale,
                     GError      **error)
{
  GMarkupParseContext *context;
  TipsParser          *parser;
  FILE   *fp;
  gsize   bytes;
  gchar   buf[4096];
  GList  *tips        = NULL;
  GError *parse_error = NULL;

  fp = fopen (filename, "r");
  if (!fp)
    {
      g_set_error (error,
                   0, /* error domain */
                   0, /* error code   */
                   _("Your GIMP tips file appears to be missing!\n"
                     "There should be a file called gimp-tips.xml in "
                     "the tips subfolder of the GIMP data folder.\n"
                     "Please check your installation."));
      return NULL;
    }

  parser = g_new0 (TipsParser, 1);
  parser->locale = locale;
  parser->value  = g_string_new (NULL);

  context = g_markup_parse_context_new (&markup_parser, 0, parser, NULL);

  while ((bytes = fread (buf, sizeof (gchar), sizeof (buf), fp)) > 0 &&
         g_markup_parse_context_parse (context, buf, bytes, &parse_error))
    ;

  /* FIXME */
  if (parse_error)
    g_clear_error (&parse_error);

  g_markup_parse_context_end_parse (context, &parse_error);

  /* FIXME */
  if (parse_error)
    g_clear_error (&parse_error);

  fclose (fp);
  g_markup_parse_context_free (context);

  if (parser->state != TIPS_START)
    {
      g_set_error (error,
                   0, /* error domain */
                   0, /* error code   */
                     _("Your GIMP tips file could not be parsed correctly!\n"
                       "Please check your installation."));
    }

  tips = g_list_reverse (parser->tips);

  gimp_tip_free (parser->current_tip);
  g_string_free (parser->value, TRUE);
  g_free (parser);

  return tips;
}

void
gimp_tips_free (GList *tips)
{
  GList *list;

  for (list = tips; list; list = list->next)
    gimp_tip_free (list->data);

  g_list_free (tips);
}

static void
tips_parser_start_element (GMarkupParseContext *context,
                           const gchar         *element_name,
                           const gchar        **attribute_names,
                           const gchar        **attribute_values,
                           gpointer             user_data,
                           GError             **error)
{
  TipsParser *parser = (TipsParser *) user_data;

  switch (parser->state)
    {
    case TIPS_START:
      if (strcmp (element_name, "gimp-tips") == 0)
        parser->state = TIPS_IN_TIPS;
      else
        tips_parser_start_unknown (parser);
      break;

    case TIPS_IN_TIPS:
      if (strcmp (element_name, "tip") == 0)
        {
          parser->state = TIPS_IN_TIP;
          parser->current_tip = g_new0 (GimpTip, 1);
        }
      else
        tips_parser_start_unknown (parser);
      break;
      
    case TIPS_IN_TIP:
      if (strcmp (element_name, "welcome") == 0)
        {
          parser->state = TIPS_IN_WELCOME;
          tips_parser_parse_locale (parser, attribute_names, attribute_values);
        }
      else if (strcmp (element_name, "thetip") == 0)
        {
          parser->state = TIPS_IN_THETIP;
          tips_parser_parse_locale (parser, attribute_names, attribute_values);
        }
      else
        tips_parser_start_unknown (parser);
      break;
    
    case TIPS_IN_WELCOME:
    case TIPS_IN_THETIP:
    case TIPS_UNKNOWN:
      tips_parser_start_unknown (parser);
      break;
    }
}

static void
tips_parser_end_element (GMarkupParseContext *context,
                         const gchar         *element_name,
                         gpointer             user_data,
                         GError             **error)
{
  TipsParser *parser = (TipsParser *) user_data;

  switch (parser->state)
    {
    case TIPS_START:
      g_warning ("tips_parser: This shouldn't happen.\n");
      break;

    case TIPS_IN_TIPS:
      parser->state = TIPS_START;
      break;

    case TIPS_IN_TIP:
      parser->state = TIPS_IN_TIPS;
      parser->tips = g_list_prepend (parser->tips, parser->current_tip);
      parser->current_tip = NULL;
      break;
      
    case TIPS_IN_WELCOME:
      tips_parser_set_by_locale (parser, &parser->current_tip->welcome);
      parser->state = TIPS_IN_TIP;
      break;

    case TIPS_IN_THETIP:
      tips_parser_set_by_locale (parser, &parser->current_tip->thetip);
      parser->state = TIPS_IN_TIP;
      break;

      break;

    case TIPS_UNKNOWN:
      tips_parser_end_unknown (parser);
    }

  g_string_assign (parser->value, "");
}

static void
tips_parser_characters (GMarkupParseContext *context,
                        const gchar         *text,
                        gsize                text_len,
                        gpointer             user_data,
                        GError             **error)
{
  TipsParser *parser = (TipsParser *) user_data;

  switch (parser->state)
    {
    case TIPS_IN_WELCOME:
    case TIPS_IN_THETIP:
      if (parser->locale_state != TIPS_LOCALE_MISMATCH)
        {
          gint i;

          /* strip tabs, newlines and adjacent whitespace */ 
          for (i = 0; i < text_len; i++)
            {
              if (text[i] != ' ' && text[i] != '\t' && text[i] != '\n')
                g_string_append_c (parser->value, text[i]);
              else if (parser->value->len > 0 &&
                       parser->value->str[parser->value->len - 1] != ' ')
                g_string_append_c (parser->value, ' ');
            }
        }
      break;
    default:
      break;
    }
}

static void
tips_parser_error (GMarkupParseContext *context,
                   GError              *error,
                   gpointer             user_data)
{
  g_warning (error->message);
}

static void
tips_parser_start_unknown (TipsParser *parser)
{
  if (parser->unknown_depth == 0)
    parser->last_known_state = parser->state;

  parser->state = TIPS_UNKNOWN;
  parser->unknown_depth++;
}

static void
tips_parser_end_unknown (TipsParser *parser)
{
  g_assert (parser->unknown_depth > 0 && parser->state == TIPS_UNKNOWN);

  parser->unknown_depth--;
  
  if (parser->unknown_depth == 0)
    parser->state = parser->last_known_state;
}

static void
tips_parser_parse_locale (TipsParser   *parser,
                          const gchar **names,
                          const gchar **values)
{
  parser->locale_state = TIPS_LOCALE_NONE;

  while (*names && *values)
    {
      if (strcmp (*names, "xml:lang") == 0 && **values)
        {
          parser->locale_state = ((parser->locale &&
                                   (strcmp (*values, parser->locale) == 0 ||
                                    strncmp (*values, parser->locale, 2) == 0))
                                  ? TIPS_LOCALE_MATCH 
                                  : TIPS_LOCALE_MISMATCH);
        }

      names++;
      values++;
    }
}

static void
tips_parser_set_by_locale (TipsParser  *parser,
                           gchar      **dest) 
{
  switch (parser->locale_state)
    {
    case TIPS_LOCALE_NONE:
      if (!parser->locale)
        {
          g_free (*dest);
          *dest = g_strdup (parser->value->str);
        }
      else if (*dest == NULL)
        {
          *dest = g_strdup (parser->value->str);
        }
      break;

    case TIPS_LOCALE_MATCH:
      g_free (*dest);
      *dest = g_strdup (parser->value->str);
      break;
        
    case TIPS_LOCALE_MISMATCH:
      break;
    }
}
