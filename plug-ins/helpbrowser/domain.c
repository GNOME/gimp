/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help Browser
 * Copyright (C) 1999-2003 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *
 * domain.c
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

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <glib-object.h>

#include "domain.h"


struct _HelpDomain
{
  gchar      *help_domain;
  gchar      *help_uri;
  GHashTable *help_id_mapping;
};


/*  local function prototypes  */

HelpDomain * domain_new   (const gchar  *domain_name,
                           const gchar  *domain_uri);
void         domain_free  (HelpDomain   *domain);
gboolean     domain_parse (HelpDomain   *domain,
                           GError      **error);


/*  private variables  */

static GHashTable  *domain_hash = NULL;


/*  public functions  */

void
domain_register (const gchar *domain_name,
                 const gchar *domain_uri)
{
  g_return_if_fail (domain_name != NULL);
  g_return_if_fail (domain_uri != NULL);

  g_print ("helpbrowser: registering help domain \"%s\" with base uri \"%s\"\n",
           domain_name, domain_uri);

  if (! domain_hash)
    domain_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
                                         g_free, (GDestroyNotify ) domain_free);

  g_hash_table_insert (domain_hash,
                       g_strdup (domain_name),
                       domain_new (domain_name, domain_uri));
}

HelpDomain *
domain_lookup (const gchar *domain_name)
{
  g_return_val_if_fail (domain_name != NULL, NULL);

  if (domain_hash)
    return g_hash_table_lookup (domain_hash, domain_name);

  return NULL;
}

gchar *
domain_map (HelpDomain  *domain,
            const gchar *help_locale,
            const gchar *help_id)
{
  const gchar *ref;
  gchar       *full_uri;

  g_return_val_if_fail (domain != NULL, NULL);
  g_return_val_if_fail (help_locale != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);

  if (! domain->help_id_mapping)
    {
      GError *error = NULL;

      if (! domain_parse (domain, &error) || error)
        {
          if (! domain->help_id_mapping)
            g_message ("Failed to open help domain:\n\n%s", error->message);
          else
            g_message ("Parse error in help domain:\n\n%s\n\n"
                       "(Added entires before error anyway)", error->message);

          if (error)
            g_clear_error (&error);
        }

      if (! domain->help_id_mapping)
        return NULL;
    }

  ref = g_hash_table_lookup (domain->help_id_mapping, help_id);

  if (! ref)
    {
      g_message ("Help ID \"%s\" unknown", help_id);
      return NULL;
    }

  full_uri = g_strconcat (domain->help_uri, "/",
                          help_locale, "/",
                          ref, NULL);

  return full_uri;
}


/*  private functions  */

HelpDomain *
domain_new (const gchar *domain_name,
            const gchar *domain_uri)
{
  HelpDomain *domain;

  domain = g_new0 (HelpDomain, 1);

  domain->help_domain = g_strdup (domain_name);
  domain->help_uri    = g_strdup (domain_uri);

  return domain;
}

void
domain_free (HelpDomain *domain)
{
  g_return_if_fail (domain != NULL);

  if (domain->help_id_mapping)
    g_hash_table_destroy (domain->help_id_mapping);

  g_free (domain->help_domain);
  g_free (domain->help_uri);
  g_free (domain);
}


/*  the domain mapping parser  */

typedef enum
{
  DOMAIN_START,
  DOMAIN_IN_HELP,
  DOMAIN_IN_ITEM,
  DOMAIN_IN_UNKNOWN
} DomainParserState;

typedef struct
{
  const gchar       *filename;
  DomainParserState  state;
  DomainParserState  last_known_state;
  gint               markup_depth;
  gint               unknown_depth;
  GString           *value;

  HelpDomain        *domain;
  gchar             *id_attr_name;
} DomainParser;

static void  domain_parser_start_element   (GMarkupParseContext  *context,
                                            const gchar          *element_name,
                                            const gchar         **attribute_names,
                                            const gchar         **attribute_values,
                                            gpointer              user_data,
                                            GError              **error);
static void  domain_parser_end_element     (GMarkupParseContext  *context,
                                            const gchar          *element_name,
                                            gpointer              user_data,
                                            GError              **error);
static void  domain_parser_error           (GMarkupParseContext  *context,
                                            GError               *error,
                                            gpointer              user_data);
static void  domain_parser_start_unknown   (DomainParser         *parser);
static void  domain_parser_end_unknown     (DomainParser         *parser);
static void  domain_parser_parse_namespace (DomainParser  *parser,
                                            const gchar  **names,
                                            const gchar  **values);
static void  domain_parser_parse_item      (DomainParser         *parser,
                                            const gchar         **names,
                                            const gchar         **values);

static const GMarkupParser markup_parser =
{
  domain_parser_start_element,
  domain_parser_end_element,
  NULL,  /*  characters   */
  NULL,  /*  passthrough  */
  domain_parser_error
};

gboolean
domain_parse (HelpDomain  *domain,
              GError     **error)
{
  GMarkupParseContext *context;
  DomainParser        *parser;
  gchar               *base_dir;
  gchar               *filename;
  FILE                *fp;
  gsize                bytes;
  gchar                buf[4096];

  g_return_val_if_fail (domain != NULL, FALSE);
  g_return_val_if_fail (domain->help_id_mapping == NULL, FALSE);

  base_dir = g_filename_from_uri (domain->help_uri, NULL, NULL);
  filename = g_build_filename (base_dir, "gimp-help.xml", NULL);
  g_free (base_dir);

  fp = fopen (filename, "r");
  if (! fp)
    {
      g_set_error (error, 0, 0,
                   "Could not open gimp-help.xml mapping file from '%s': %s",
                   domain->help_uri, g_strerror (errno));
      g_free (filename);
      return FALSE;
    }

  parser = g_new0 (DomainParser, 1);
  parser->filename     = filename;
  parser->value        = g_string_new (NULL);
  parser->id_attr_name = g_strdup ("id");
  parser->domain       = domain;

  context = g_markup_parse_context_new (&markup_parser, 0, parser, NULL);

  while ((bytes = fread (buf, sizeof (gchar), sizeof (buf), fp)) > 0 &&
         g_markup_parse_context_parse (context, buf, bytes, error))
    ;

  if (error == NULL || *error == NULL)
    g_markup_parse_context_end_parse (context, error);

  fclose (fp);
  g_markup_parse_context_free (context);

  g_string_free (parser->value, TRUE);
  g_free (parser->id_attr_name);
  g_free (parser);

  g_free (filename);

  return (domain->help_id_mapping != NULL);
}

static void
domain_parser_start_element (GMarkupParseContext *context,
                             const gchar         *element_name,
                             const gchar        **attribute_names,
                             const gchar        **attribute_values,
                             gpointer             user_data,
                             GError             **error)
{
  DomainParser *parser = (DomainParser *) user_data;

  switch (parser->state)
    {
    case DOMAIN_START:
      if (strcmp (element_name, "gimp-help") == 0)
        {
          parser->state = DOMAIN_IN_HELP;
          domain_parser_parse_namespace (parser, attribute_names,
                                         attribute_values);
        }
      else
        domain_parser_start_unknown (parser);
      break;

    case DOMAIN_IN_HELP:
      if (strcmp (element_name, "help-item") == 0)
        {
          parser->state = DOMAIN_IN_ITEM;
          domain_parser_parse_item (parser, attribute_names,
                                    attribute_values);
        }
      else
        domain_parser_start_unknown (parser);
      break;

    case DOMAIN_IN_ITEM:
    case DOMAIN_IN_UNKNOWN:
      domain_parser_start_unknown (parser);
      break;
    }
}

static void
domain_parser_end_element (GMarkupParseContext *context,
                           const gchar         *element_name,
                           gpointer             user_data,
                           GError             **error)
{
  DomainParser *parser = (DomainParser *) user_data;

  switch (parser->state)
    {
    case DOMAIN_START:
      g_warning ("tips_parser: This shouldn't happen.");
      break;

    case DOMAIN_IN_HELP:
      parser->state = DOMAIN_START;
      break;

    case DOMAIN_IN_ITEM:
      parser->state = DOMAIN_IN_HELP;
      break;

    case DOMAIN_IN_UNKNOWN:
      domain_parser_end_unknown (parser);
      break;
    }
}

static void
domain_parser_error (GMarkupParseContext *context,
                     GError              *error,
                     gpointer             user_data)
{
  DomainParser *parser = (DomainParser *) user_data;

  g_warning ("%s: %s", parser->filename, error->message);
}

static void
domain_parser_start_unknown (DomainParser *parser)
{
  if (parser->unknown_depth == 0)
    parser->last_known_state = parser->state;

  parser->state = DOMAIN_IN_UNKNOWN;
  parser->unknown_depth++;
}

static void
domain_parser_end_unknown (DomainParser *parser)
{
  g_assert (parser->unknown_depth > 0 && parser->state == DOMAIN_IN_UNKNOWN);

  parser->unknown_depth--;

  if (parser->unknown_depth == 0)
    parser->state = parser->last_known_state;
}

static void
domain_parser_parse_namespace (DomainParser  *parser,
                               const gchar  **names,
                               const gchar  **values)
{
  while (*names && *values)
    {
      if (! strncmp (*names, "xmlns:", 6) &&
          ! strcmp (*values, parser->domain->help_domain))
        {
          g_free (parser->id_attr_name);
          parser->id_attr_name = g_strdup_printf ("%s:id", *names + 6);

          g_print ("domain parser: id attribute name for \"%s\" is \"%s\"\n",
                   parser->domain->help_domain, parser->id_attr_name);
        }

      names++;
      values++;
    }
}

static void
domain_parser_parse_item (DomainParser  *parser,
                          const gchar  **names,
                          const gchar  **values)
{
  const gchar *id  = NULL;
  const gchar *ref = NULL;

  while (*names && *values)
    {
      if (! strcmp (*names, parser->id_attr_name))
        id = *values;

      if (! strcmp (*names, "ref"))
        ref = *values;

      names++;
      values++;
    }

  if (id && ref)
    {
      if (! parser->domain->help_id_mapping)
        parser->domain->help_id_mapping = g_hash_table_new_full (g_str_hash,
                                                                 g_str_equal,
                                                                 g_free,
                                                                 g_free);

      g_hash_table_insert (parser->domain->help_id_mapping,
                           g_strdup (id), g_strdup (ref));

      g_print ("domain parser: added mapping \"%s\" -> \"%s\"\n", id, ref);
    }
}
