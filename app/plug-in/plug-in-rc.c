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

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpprotocol.h"

#include "plug-in-types.h"

#include "config/gimpconfig-error.h"
#include "config/gimpconfigwriter.h"
#include "config/gimpscanner.h"

#include "core/gimp.h"

#include "file/file-utils.h"

#include "plug-ins.h"
#include "plug-in-def.h"
#include "plug-in-proc.h"
#include "plug-in-rc.h"

#include "gimp-intl.h"


/*
 *  All deserialize functions return G_TOKEN_LEFT_PAREN on success,
 *  or the GTokenType they would have expected but didn't get.
 */

static GTokenType plug_in_def_deserialize        (Gimp          *gimp,
                                                  GScanner      *scanner);
static GTokenType plug_in_proc_def_deserialize   (GScanner      *scanner,
                                                  PlugInProcDef *proc_def);
static GTokenType plug_in_proc_arg_deserialize   (GScanner      *scanner,
                                                  ProcArg       *arg);
static GTokenType plug_in_locale_def_deserialize (GScanner      *scanner,
                                                  PlugInDef     *plug_in_def);
static GTokenType plug_in_help_def_deserialize   (GScanner      *scanner,
                                                  PlugInDef     *plug_in_def);
static GTokenType plug_in_has_init_deserialize   (GScanner      *scanner,
                                                  PlugInDef     *plug_in_def);


enum
{
  PROTOCOL_VERSION = 1,
  PLUG_IN_DEF,
  PROC_DEF,
  LOCALE_DEF,
  HELP_DEF,
  HAS_INIT,
  PROC_ARG
};


gboolean
plug_in_rc_parse (Gimp         *gimp,
                  const gchar  *filename,
                  GError      **error)
{
  GScanner   *scanner;
  GTokenType  token;
  gboolean    retval  = FALSE;
  gint        version = GIMP_PROTOCOL_VERSION;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  scanner = gimp_scanner_new_file (filename, error);

  if (! scanner)
    return TRUE;

  g_scanner_scope_add_symbol (scanner, 0,
                              "protocol-version",
                              GINT_TO_POINTER (PROTOCOL_VERSION));
  g_scanner_scope_add_symbol (scanner, 0,
                              "plug-in-def", GINT_TO_POINTER (PLUG_IN_DEF));
  g_scanner_scope_add_symbol (scanner, PLUG_IN_DEF,
                              "proc-def", GINT_TO_POINTER (PROC_DEF));
  g_scanner_scope_add_symbol (scanner, PLUG_IN_DEF,
                              "locale-def", GINT_TO_POINTER (LOCALE_DEF));
  g_scanner_scope_add_symbol (scanner, PLUG_IN_DEF,
                              "help-def", GINT_TO_POINTER (HELP_DEF));
  g_scanner_scope_add_symbol (scanner, PLUG_IN_DEF,
                              "has-init", GINT_TO_POINTER (HAS_INIT));
  g_scanner_scope_add_symbol (scanner, PLUG_IN_DEF,
                              "proc-arg", GINT_TO_POINTER (PROC_ARG));

  token = G_TOKEN_LEFT_PAREN;

  while (version == GIMP_PROTOCOL_VERSION &&
         g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          switch (GPOINTER_TO_INT (scanner->value.v_symbol))
            {
            case PROTOCOL_VERSION:
              token = G_TOKEN_INT;
              if (gimp_scanner_parse_int (scanner, &version))
                token = G_TOKEN_RIGHT_PAREN;
              break;
            case PLUG_IN_DEF:
              g_scanner_set_scope (scanner, PLUG_IN_DEF);
              token = plug_in_def_deserialize (gimp, scanner);
              g_scanner_set_scope (scanner, 0);
              break;
            default:
              break;
            }
              break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default: /* do nothing */
          break;
        }
    }

  if (version != GIMP_PROTOCOL_VERSION)
    {
      g_set_error (error,
                   GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_VERSION,
                   _("Skipping '%s': wrong GIMP protocol version."),
		   gimp_filename_to_utf8 (filename));
    }
  else if (token != G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                             _("fatal parse error"), TRUE);
    }
  else
    {
      retval = TRUE;
    }

  gimp_scanner_destroy (scanner);

  return retval;
}

static GTokenType
plug_in_def_deserialize (Gimp     *gimp,
                         GScanner *scanner)
{
  gchar         *name;
  PlugInDef     *plug_in_def;
  PlugInProcDef *proc_def;
  GTokenType     token;

  if (! gimp_scanner_parse_string (scanner, &name))
    return G_TOKEN_STRING;

  plug_in_def = plug_in_def_new (name);
  g_free (name);

  if (! gimp_scanner_parse_int (scanner, (gint *) &plug_in_def->mtime))
    {
      plug_in_def_free (plug_in_def, TRUE);
      return G_TOKEN_INT;
    }

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          switch (GPOINTER_TO_INT (scanner->value.v_symbol))
            {
            case PROC_DEF:
              proc_def = plug_in_proc_def_new ();
              token = plug_in_proc_def_deserialize (scanner, proc_def);

              if (token == G_TOKEN_LEFT_PAREN)
                plug_in_def_add_proc_def (plug_in_def, proc_def);
              else
                plug_in_proc_def_free (proc_def);
              break;

            case LOCALE_DEF:
              token = plug_in_locale_def_deserialize (scanner, plug_in_def);
              break;

            case HELP_DEF:
              token = plug_in_help_def_deserialize (scanner, plug_in_def);
              break;

            case HAS_INIT:
              token = plug_in_has_init_deserialize (scanner, plug_in_def);
              break;

            default:
              break;
            }
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default:
          break;
        }
    }

  if (token == G_TOKEN_LEFT_PAREN)
    {
      token = G_TOKEN_RIGHT_PAREN;

      if (gimp_scanner_parse_token (scanner, token))
        {
          plug_ins_def_add_from_rc (gimp, plug_in_def);
          return G_TOKEN_LEFT_PAREN;
        }
    }

  plug_in_def_free (plug_in_def, TRUE);

  return token;
}

static GTokenType
plug_in_proc_def_deserialize (GScanner      *scanner,
                              PlugInProcDef *proc_def)
{
  GTokenType token;
  gint       i;

  if (! gimp_scanner_parse_string (scanner, &proc_def->db_info.name))
    return G_TOKEN_STRING;
  if (! gimp_scanner_parse_int (scanner, (gint *) &proc_def->db_info.proc_type))
    return G_TOKEN_INT;
  if (! gimp_scanner_parse_string (scanner, &proc_def->db_info.blurb))
    return G_TOKEN_STRING;
  if (! gimp_scanner_parse_string (scanner, &proc_def->db_info.help))
    return G_TOKEN_STRING;
  if (! gimp_scanner_parse_string (scanner, &proc_def->db_info.author))
    return G_TOKEN_STRING;
  if (! gimp_scanner_parse_string (scanner, &proc_def->db_info.copyright))
    return G_TOKEN_STRING;
  if (! gimp_scanner_parse_string (scanner, &proc_def->db_info.date))
    return G_TOKEN_STRING;
  if (! gimp_scanner_parse_string (scanner, &proc_def->menu_path))
    return G_TOKEN_STRING;
  if (! gimp_scanner_parse_string (scanner, &proc_def->extensions))
    return G_TOKEN_STRING;
  if (! gimp_scanner_parse_string (scanner, &proc_def->prefixes))
    return G_TOKEN_STRING;
  if (! gimp_scanner_parse_string_no_validate (scanner, &proc_def->magics))
    return G_TOKEN_STRING;
  if (! gimp_scanner_parse_string (scanner, &proc_def->image_types))
    return G_TOKEN_STRING;

  proc_def->image_types_val =
    plug_ins_image_types_parse (proc_def->image_types);

  if (! gimp_scanner_parse_int (scanner, (gint *) &proc_def->db_info.num_args))
    return G_TOKEN_INT;
  if (! gimp_scanner_parse_int (scanner, (gint *) &proc_def->db_info.num_values))
    return G_TOKEN_INT;

  if (proc_def->db_info.num_args > 0)
    proc_def->db_info.args = g_new0 (ProcArg, proc_def->db_info.num_args);

  for (i = 0; i < proc_def->db_info.num_args; i++)
    {
      token = plug_in_proc_arg_deserialize (scanner,
                                            &proc_def->db_info.args[i]);
      if (token != G_TOKEN_LEFT_PAREN)
        return token;
    }

  if (proc_def->db_info.num_values > 0)
    proc_def->db_info.values = g_new0 (ProcArg, proc_def->db_info.num_values);

  for (i = 0; i < proc_def->db_info.num_values; i++)
    {
      token = plug_in_proc_arg_deserialize (scanner,
                                            &proc_def->db_info.values[i]);
      if (token != G_TOKEN_LEFT_PAREN)
        return token;
    }

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType
plug_in_proc_arg_deserialize (GScanner *scanner,
                              ProcArg  *arg)
{
  if (! gimp_scanner_parse_token (scanner, G_TOKEN_LEFT_PAREN))
    return G_TOKEN_LEFT_PAREN;

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_SYMBOL) ||
      GPOINTER_TO_INT (scanner->value.v_symbol) != PROC_ARG)
    return G_TOKEN_SYMBOL;

  if (! gimp_scanner_parse_int (scanner, (gint *) &arg->arg_type))
    return G_TOKEN_INT;
  if (! gimp_scanner_parse_string (scanner, &arg->name))
    return G_TOKEN_STRING;
  if (! gimp_scanner_parse_string (scanner, &arg->description))
    return G_TOKEN_STRING;

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType
plug_in_locale_def_deserialize (GScanner  *scanner,
                                PlugInDef *plug_in_def)
{
  gchar *string;

  if (! gimp_scanner_parse_string (scanner, &string))
    return G_TOKEN_STRING;

  plug_in_def_set_locale_domain_name (plug_in_def, string);
  g_free (string);

  if (gimp_scanner_parse_string (scanner, &string))
    {
      plug_in_def_set_locale_domain_path (plug_in_def, string);
      g_free (string);
    }

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType
plug_in_help_def_deserialize (GScanner  *scanner,
                              PlugInDef *plug_in_def)
{
  gchar *string;

  if (! gimp_scanner_parse_string (scanner, &string))
    return G_TOKEN_STRING;

  plug_in_def_set_help_domain_name (plug_in_def, string);
  g_free (string);

  if (gimp_scanner_parse_string (scanner, &string))
    {
      plug_in_def_set_help_domain_uri (plug_in_def, string);
      g_free (string);
    }

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType
plug_in_has_init_deserialize (GScanner  *scanner,
                              PlugInDef *plug_in_def)
{
  plug_in_def_set_has_init (plug_in_def, TRUE);

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}


/* serialize functions */

gboolean
plug_in_rc_write (GSList       *plug_in_defs,
                  const gchar  *filename,
                  GError      **error)
{
  GimpConfigWriter *writer;
  PlugInDef        *plug_in_def;
  PlugInProcDef    *proc_def;
  GSList           *list;
  GSList           *list2;
  gint              i;

  writer = gimp_config_writer_new_file (filename,
					FALSE,
					"GIMP plug-ins\n\n"
					"This file can safely be removed and "
					"will be automatically regenerated by "
					"querying the installed plugins.",
					error);
  if (!writer)
    return FALSE;

  gimp_config_writer_open (writer, "protocol-version");
  gimp_config_writer_printf (writer, "%d", GIMP_PROTOCOL_VERSION);
  gimp_config_writer_close (writer);
  gimp_config_writer_linefeed (writer);

  for (list = plug_in_defs; list; list = list->next)
    {
      plug_in_def = list->data;

      if (plug_in_def->proc_defs)
	{
          gimp_config_writer_open (writer, "plug-in-def");
          gimp_config_writer_string (writer, plug_in_def->prog);
          gimp_config_writer_printf (writer, "%ld", plug_in_def->mtime);

	  for (list2 = plug_in_def->proc_defs; list2; list2 = list2->next)
	    {
	      proc_def = list2->data;

              gimp_config_writer_open (writer, "proc-def");
              gimp_config_writer_printf (writer, "\"%s\" %d",
                                         proc_def->db_info.name,
                                         proc_def->db_info.proc_type);
              gimp_config_writer_linefeed (writer);
              gimp_config_writer_string (writer, proc_def->db_info.blurb);
              gimp_config_writer_linefeed (writer);
	      gimp_config_writer_string (writer, proc_def->db_info.help);
	      gimp_config_writer_linefeed (writer);
	      gimp_config_writer_string (writer, proc_def->db_info.author);
	      gimp_config_writer_linefeed (writer);
	      gimp_config_writer_string (writer, proc_def->db_info.copyright);
	      gimp_config_writer_linefeed (writer);
	      gimp_config_writer_string (writer, proc_def->db_info.date);
	      gimp_config_writer_linefeed (writer);
	      gimp_config_writer_string (writer, proc_def->menu_path);
	      gimp_config_writer_linefeed (writer);
	      gimp_config_writer_string (writer, proc_def->extensions);
	      gimp_config_writer_linefeed (writer);
	      gimp_config_writer_string (writer, proc_def->prefixes);
	      gimp_config_writer_linefeed (writer);
	      gimp_config_writer_string (writer, proc_def->magics);
	      gimp_config_writer_linefeed (writer);
	      gimp_config_writer_string (writer, proc_def->image_types);
	      gimp_config_writer_linefeed (writer);

	      gimp_config_writer_printf (writer, "%d %d",
                                         proc_def->db_info.num_args,
                                         proc_def->db_info.num_values);

	      for (i = 0; i < proc_def->db_info.num_args; i++)
		{
                  gimp_config_writer_open (writer, "proc-arg");
                  gimp_config_writer_printf (writer, "%d",
                                             proc_def->db_info.args[i].arg_type);

		  gimp_config_writer_string (writer,
                                             proc_def->db_info.args[i].name);
		  gimp_config_writer_string (writer,
                                             proc_def->db_info.args[i].description);

                  gimp_config_writer_close (writer);
		}

	      for (i = 0; i < proc_def->db_info.num_values; i++)
		{
		  gimp_config_writer_open (writer, "proc-arg");
                  gimp_config_writer_printf (writer, "%d",
                                             proc_def->db_info.values[i].arg_type);

		  gimp_config_writer_string (writer,
                                             proc_def->db_info.values[i].name);
		  gimp_config_writer_string (writer,
                                             proc_def->db_info.values[i].description);

                  gimp_config_writer_close (writer);
		}

	      gimp_config_writer_close (writer);
	    }

	  if (plug_in_def->locale_domain_name)
	    {
              gimp_config_writer_open (writer, "locale-def");
              gimp_config_writer_string (writer,
                                         plug_in_def->locale_domain_name);

	      if (plug_in_def->locale_domain_path)
		gimp_config_writer_string (writer,
                                           plug_in_def->locale_domain_path);

              gimp_config_writer_close (writer);
	    }

	  if (plug_in_def->help_domain_name)
	    {
	      gimp_config_writer_open (writer, "help-def");
              gimp_config_writer_string (writer,
                                         plug_in_def->help_domain_name);

	      if (plug_in_def->help_domain_uri)
		gimp_config_writer_string (writer,
                                           plug_in_def->help_domain_uri);

             gimp_config_writer_close (writer);
	    }

	  if (plug_in_def->has_init)
	    {
	      gimp_config_writer_open (writer, "has-init");
              gimp_config_writer_close (writer);
	    }

	  gimp_config_writer_close (writer);
	}
    }

  return gimp_config_writer_finish (writer, "end of plug-ins", error);
}
