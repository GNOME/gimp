/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * PlugInRc
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
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
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#include <glib-object.h>

#include "plug-in-types.h"

#include "plug-in.h"
#include "plug-in-proc.h"
#include "plug-in-rc.h"

#include "libgimp/gimpintl.h"


static GTokenType plug_in_def_deserialize        (GScanner      *scanner);
static GTokenType plug_in_proc_def_deserialize   (GScanner      *scanner,
                                                  PlugInProcDef *proc_def);
static GTokenType plug_in_proc_arg_deserialize   (GScanner      *scanner,
                                                  ProcArg       *arg);
static GTokenType plug_in_locale_def_deserialize (GScanner      *scanner,
                                                  PlugInDef     *plug_in_def);
static GTokenType plug_in_help_def_deserialize   (GScanner      *scanner,
                                                  PlugInDef     *plug_in_def);

static inline GTokenType parse_string (GScanner   *scanner,
                                       gchar     **dest);
static inline GTokenType parse_int    (GScanner   *scanner,
                                       gint       *dest);

enum
{
  PROC_DEF,
  LOCALE_DEF,
  HELP_DEF
};


gboolean
plug_in_rc_parse (const gchar *filename)
{
  gint        fd;
  GScanner   *scanner;
  GTokenType  token;

  g_return_val_if_fail (filename != NULL, FALSE);

  fd = open (filename, O_RDONLY);

  if (fd == -1)
    return TRUE;

  scanner = g_scanner_new (NULL);

  scanner->config->cset_identifier_first = ( G_CSET_a_2_z );
  scanner->config->cset_identifier_nth   = ( G_CSET_a_2_z "-_" );

  g_scanner_input_file (scanner, fd);
  scanner->input_name = filename;

  g_scanner_scope_add_symbol (scanner, 0, 
                              "proc-def", GINT_TO_POINTER (PROC_DEF));
  g_scanner_scope_add_symbol (scanner, 0, 
                              "locale-def", GINT_TO_POINTER (LOCALE_DEF));
  g_scanner_scope_add_symbol (scanner, 0, 
                              "help-def", GINT_TO_POINTER (HELP_DEF));

  token = g_scanner_peek_next_token (scanner);

  while (token != G_TOKEN_EOF)
    {
      if ((token = g_scanner_get_next_token (scanner)) != G_TOKEN_LEFT_PAREN)
        break;

      if (g_scanner_get_next_token (scanner) != G_TOKEN_IDENTIFIER ||
          strcmp (scanner->value.v_identifier, "plug-in-def") != 0)
        {
          token = G_TOKEN_SYMBOL;
          break;
        }

      if ((token = plug_in_def_deserialize (scanner)) != G_TOKEN_NONE)
        break;

      token = g_scanner_peek_next_token (scanner);
    }

  if (token != G_TOKEN_EOF)
    g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                           _("fatal parse error"), TRUE);

  g_scanner_destroy (scanner);
  close (fd);

  return (token != G_TOKEN_EOF);
}

static GTokenType
plug_in_def_deserialize (GScanner *scanner)
{
  gchar      *name;
  PlugInDef  *plug_in_def;
  GTokenType  token;

  if ((token = parse_string (scanner, &name)) != G_TOKEN_NONE)
    return token;

  plug_in_def = plug_in_def_new (name);

  if ((token = parse_int (scanner, (gint *) &plug_in_def->mtime)) 
      != G_TOKEN_NONE)
    goto error;

  token = g_scanner_peek_next_token (scanner);
  while (token == G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);

      if ((token = g_scanner_get_next_token (scanner)) != G_TOKEN_SYMBOL) 
        goto error;

      switch (GPOINTER_TO_INT (scanner->value.v_symbol))
        {
        case PROC_DEF:
          {
            PlugInProcDef *proc_def = g_new0 (PlugInProcDef, 1);

            if ((token = plug_in_proc_def_deserialize (scanner, proc_def))
                != G_TOKEN_NONE)
              {
                plug_in_proc_def_destroy (proc_def, FALSE);
                goto error;
              }
            else
              {
                plug_in_def_add_proc_def (plug_in_def, proc_def);
              }
          }
          break;
          
        case LOCALE_DEF:
          if ((token = plug_in_locale_def_deserialize (scanner, plug_in_def))
              != G_TOKEN_NONE)
            goto error;
          break;
          
        case HELP_DEF:
          if ((token = plug_in_help_def_deserialize (scanner, plug_in_def))
              != G_TOKEN_NONE)
            goto error;
          break;
          
        default:
          g_assert_not_reached ();
        }

      if (token != G_TOKEN_NONE)
        goto error;
      
      token = g_scanner_peek_next_token (scanner);
    }

  if (g_scanner_get_next_token (scanner) == G_TOKEN_RIGHT_PAREN)
    {
      plug_in_def_add (plug_in_def);
      return G_TOKEN_NONE;
    }

  token = G_TOKEN_RIGHT_PAREN;

 error:
  plug_in_def_free (plug_in_def, TRUE);
  return token;
}

static GTokenType
plug_in_proc_def_deserialize (GScanner      *scanner,
                              PlugInProcDef *proc_def)
{
  GTokenType token;
  gint       i;

  if ((token = parse_string (scanner, &proc_def->db_info.name)) 
      != G_TOKEN_NONE)
    return token;
  if ((token = parse_int (scanner, (gint *) &proc_def->db_info.proc_type)) 
      != G_TOKEN_NONE)
    return token;
  if ((token = parse_string (scanner, &proc_def->db_info.blurb)) 
      != G_TOKEN_NONE)
    return token;
  if ((token = parse_string (scanner, &proc_def->db_info.help)) 
      != G_TOKEN_NONE)
    return token;
  if ((token = parse_string (scanner, &proc_def->db_info.author)) 
      != G_TOKEN_NONE)
    return token;
  if ((token = parse_string (scanner, &proc_def->db_info.copyright)) 
      != G_TOKEN_NONE)
    return token;
  if ((token = parse_string (scanner, &proc_def->db_info.date)) 
      != G_TOKEN_NONE)
    return token;

  if ((token = parse_string (scanner, &proc_def->menu_path)) != G_TOKEN_NONE)
    return token;
  if ((token = parse_string (scanner, &proc_def->extensions)) != G_TOKEN_NONE)
    return token;
  if ((token = parse_string (scanner, &proc_def->prefixes)) != G_TOKEN_NONE)
    return token;
  if ((token = parse_string (scanner, &proc_def->magics)) != G_TOKEN_NONE)
    return token;
  if ((token = parse_string (scanner, &proc_def->image_types)) != G_TOKEN_NONE)
    return token;

  proc_def->image_types_val = 
    plug_in_image_types_parse (proc_def->image_types);

  if ((token = parse_int (scanner, &proc_def->db_info.num_args))
      != G_TOKEN_NONE)
    return token;
  if ((token = parse_int (scanner, &proc_def->db_info.num_values)) 
      != G_TOKEN_NONE)
    return token;

  if (proc_def->db_info.num_args > 0)
    proc_def->db_info.args = g_new0 (ProcArg, proc_def->db_info.num_args);

  for (i = 0; i < proc_def->db_info.num_args; i++)
    {
      if ((token = plug_in_proc_arg_deserialize (scanner, 
                                                 &proc_def->db_info.args[i]))
          != G_TOKEN_NONE)
        return token;
    }

  if (proc_def->db_info.num_values > 0)
    proc_def->db_info.values = g_new0 (ProcArg, proc_def->db_info.num_values);

  for (i = 0; i < proc_def->db_info.num_values; i++)
    {
      if ((token = plug_in_proc_arg_deserialize (scanner, 
                                                 &proc_def->db_info.values[i]))
          != G_TOKEN_NONE)
        return token;
    }

  if (g_scanner_get_next_token (scanner) != G_TOKEN_RIGHT_PAREN) 
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_NONE;
}

static GTokenType
plug_in_proc_arg_deserialize (GScanner *scanner,
                              ProcArg  *arg)
{
  GTokenType token;

  if (g_scanner_get_next_token (scanner) != G_TOKEN_LEFT_PAREN) 
    return G_TOKEN_LEFT_PAREN;

  if (g_scanner_get_next_token (scanner) != G_TOKEN_IDENTIFIER) 
    return G_TOKEN_SYMBOL;

  if (strcmp (scanner->value.v_identifier, "proc-arg") != 0)
    return G_TOKEN_SYMBOL;

  if ((token = parse_int (scanner, (gint *) &arg->arg_type)) != G_TOKEN_NONE)
    return token;
  if ((token = parse_string (scanner, &arg->name)) != G_TOKEN_NONE)
    return token;
  if ((token = parse_string (scanner, &arg->description)) != G_TOKEN_NONE)
    return token;
  
  if (g_scanner_get_next_token (scanner) != G_TOKEN_RIGHT_PAREN) 
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_NONE;
}

static GTokenType
plug_in_locale_def_deserialize (GScanner  *scanner,
                                PlugInDef *plug_in_def)
{
  if (parse_string (scanner, &plug_in_def->locale_domain) != G_TOKEN_NONE)
    return G_TOKEN_STRING;

  if (parse_string (scanner, &plug_in_def->locale_path) != G_TOKEN_NONE)
    {
      if (scanner->token == G_TOKEN_RIGHT_PAREN)
        return G_TOKEN_NONE;
      else
        return G_TOKEN_RIGHT_PAREN;
    }

  if (g_scanner_get_next_token (scanner) != G_TOKEN_RIGHT_PAREN)
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_NONE;
}

static GTokenType 
plug_in_help_def_deserialize (GScanner  *scanner,
                              PlugInDef *plug_in_def)
{
  if (parse_string (scanner, &plug_in_def->locale_domain) != G_TOKEN_NONE)
    return G_TOKEN_STRING;

  if (g_scanner_get_next_token (scanner) != G_TOKEN_RIGHT_PAREN)
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_NONE;
}


/* helper functions */

static inline GTokenType
parse_string (GScanner  *scanner,
              gchar    **dest)
{
  if (g_scanner_get_next_token (scanner) != G_TOKEN_STRING)
    return G_TOKEN_STRING;

  if (*scanner->value.v_string)
    *dest = g_strdup (scanner->value.v_string);

  return G_TOKEN_NONE;
}

static inline GTokenType
parse_int (GScanner  *scanner,
           gint      *dest)
{
  if (g_scanner_get_next_token (scanner) != G_TOKEN_INT)
    return G_TOKEN_INT;

  *dest = scanner->value.v_int;
  return G_TOKEN_NONE;
}

