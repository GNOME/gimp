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
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include <glib-object.h>

#include "plug-in-types.h"

#include "plug-in.h"
#include "plug-in-proc.h"
#include "plug-in-rc.h"

#include "libgimp/gimpintl.h"


/*  
 *  All functions return G_TOKEN_LEFT_PAREN on success,
 *  or the GTokenType they would have expected but didn't get.
 */

static GTokenType plug_in_def_deserialize        (GScanner      *scanner);
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

static inline gboolean parse_token               (GScanner      *scanner,
                                                  GTokenType     token);
static inline gboolean parse_string              (GScanner      *scanner,
                                                  gchar        **dest);
static inline gboolean parse_string_no_validate  (GScanner      *scanner,
                                                  gchar        **dest);
static inline gboolean parse_int                 (GScanner      *scanner,
                                                  gint          *dest);


enum
{
  PLUG_IN_DEF,
  PROC_DEF,
  LOCALE_DEF,
  HELP_DEF,
  HAS_INIT,
  PROC_ARG
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
                              "plug-in-def", GINT_TO_POINTER (PLUG_IN_DEF));
  g_scanner_scope_add_symbol (scanner, 0, 
                              "proc-def", GINT_TO_POINTER (PROC_DEF));
  g_scanner_scope_add_symbol (scanner, 0, 
                              "locale-def", GINT_TO_POINTER (LOCALE_DEF));
  g_scanner_scope_add_symbol (scanner, 0, 
                              "help-def", GINT_TO_POINTER (HELP_DEF));
  g_scanner_scope_add_symbol (scanner, 0, 
                              "has-init", GINT_TO_POINTER (HAS_INIT));
  g_scanner_scope_add_symbol (scanner, 0, 
                              "proc-arg", GINT_TO_POINTER (PROC_ARG));

  token = G_TOKEN_LEFT_PAREN;
  
  do
    {
      if (g_scanner_get_next_token (scanner) != token)
        break;

      token = g_scanner_get_next_token (scanner);
      
      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          if (scanner->value.v_symbol == PLUG_IN_DEF)
            token = plug_in_def_deserialize (scanner);
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;
          
        default: /* do nothing */
          break;
        }
    }
  while (token != G_TOKEN_EOF);
  
  if (token != G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                             _("fatal parse error"), TRUE);
    }

  g_scanner_destroy (scanner);
  close (fd);

  return (token != G_TOKEN_EOF);
}

static GTokenType
plug_in_def_deserialize (GScanner *scanner)
{
  gchar         *name;
  PlugInDef     *plug_in_def;
  PlugInProcDef *proc_def;
  GTokenType     token;

  if (!parse_string (scanner, &name))
    return G_TOKEN_STRING;

  plug_in_def = plug_in_def_new (name);

  if (!parse_int (scanner, (gint *) &plug_in_def->mtime))
    {
      plug_in_def_free (plug_in_def, TRUE);
      return G_TOKEN_INT;
    }

  token = G_TOKEN_LEFT_PAREN;

  do
    {
      if (g_scanner_peek_next_token (scanner) != token)
        break;
      
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
              proc_def = g_new0 (PlugInProcDef, 1);
              token = plug_in_proc_def_deserialize (scanner, proc_def);
              
              if (token == G_TOKEN_LEFT_PAREN)
                {
                  plug_in_def_add_proc_def (plug_in_def, proc_def);
                }
              else
                {
                  plug_in_proc_def_destroy (proc_def, FALSE);
                }
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
  while (token != G_TOKEN_EOF);

  if (token == G_TOKEN_LEFT_PAREN)
    {
      token = G_TOKEN_RIGHT_PAREN;

      if (parse_token (scanner, token))
        {
          plug_in_def_add (plug_in_def);
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

  if (!parse_string (scanner, &proc_def->db_info.name))
    return G_TOKEN_STRING;
  if (!parse_int (scanner, (gint *) &proc_def->db_info.proc_type)) 
    return G_TOKEN_INT;
  if (!parse_string (scanner, &proc_def->db_info.blurb))
    return G_TOKEN_STRING;
  if (!parse_string (scanner, &proc_def->db_info.help))
    return G_TOKEN_STRING;
  if (!parse_string (scanner, &proc_def->db_info.author)) 
    return G_TOKEN_STRING;
  if (!parse_string (scanner, &proc_def->db_info.copyright)) 
    return G_TOKEN_STRING;
  if (!parse_string (scanner, &proc_def->db_info.date)) 
    return G_TOKEN_STRING;
  if (!parse_string (scanner, &proc_def->menu_path)) 
    return G_TOKEN_STRING;
  if (!parse_string (scanner, &proc_def->extensions)) 
    return G_TOKEN_STRING;
  if (!parse_string (scanner, &proc_def->prefixes)) 
    return G_TOKEN_STRING;
  if (!parse_string_no_validate (scanner, &proc_def->magics)) 
    return G_TOKEN_STRING;
  if (!parse_string (scanner, &proc_def->image_types)) 
    return G_TOKEN_STRING;

  proc_def->image_types_val = 
    plug_in_image_types_parse (proc_def->image_types);

  if (!parse_int (scanner, (gint *) &proc_def->db_info.num_args)) 
    return G_TOKEN_INT;
  if (!parse_int (scanner, (gint *) &proc_def->db_info.num_values)) 
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
  
  if (!parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType
plug_in_proc_arg_deserialize (GScanner *scanner,
                              ProcArg  *arg)
{
  if (!parse_token (scanner, G_TOKEN_LEFT_PAREN))
    return G_TOKEN_LEFT_PAREN;

  if (!parse_token (scanner, G_TOKEN_SYMBOL) ||
      GPOINTER_TO_INT (scanner->value.v_symbol) != PROC_ARG)
    return G_TOKEN_SYMBOL;

  if (!parse_int (scanner, (gint *) &arg->arg_type))
    return G_TOKEN_INT;
  if (!parse_string (scanner, &arg->name))
    return G_TOKEN_STRING;
  if (!parse_string (scanner, &arg->description))
    return G_TOKEN_STRING;
  
  if (!parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType
plug_in_locale_def_deserialize (GScanner  *scanner,
                                PlugInDef *plug_in_def)
{
  if (!parse_string (scanner, &plug_in_def->locale_domain))
    return G_TOKEN_STRING;

  parse_string (scanner, &plug_in_def->locale_path);

  if (!parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType 
plug_in_help_def_deserialize (GScanner  *scanner,
                              PlugInDef *plug_in_def)
{
  if (!parse_string (scanner, &plug_in_def->help_path))
    return G_TOKEN_STRING;

  if (!parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType 
plug_in_has_init_deserialize (GScanner  *scanner,
                              PlugInDef *plug_in_def)
{
  plug_in_def->has_init = TRUE;

  if (!parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}


/* helper functions */


static inline gboolean
parse_token (GScanner   *scanner,
             GTokenType  token)
{
  if (g_scanner_peek_next_token (scanner) != token)
    return FALSE;

  g_scanner_get_next_token (scanner);

  return TRUE;
}

static inline gboolean
parse_string (GScanner  *scanner,
              gchar    **dest)
{
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_STRING)
    return FALSE;

  g_scanner_get_next_token (scanner);

  if (*scanner->value.v_string)
    {
      if (!g_utf8_validate (scanner->value.v_string, -1, NULL))
        {
          g_scanner_warn (scanner, _("invalid UTF-8 string"));
          return FALSE;
        }
      
      *dest = g_strdup (scanner->value.v_string);
    }

  return TRUE;
}

static inline gboolean
parse_string_no_validate (GScanner  *scanner,
                          gchar    **dest)
{
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_STRING)
    return FALSE;

  g_scanner_get_next_token (scanner);

  if (*scanner->value.v_string)
    *dest = g_strdup (scanner->value.v_string);

  return TRUE;
}

static inline gboolean
parse_int (GScanner  *scanner,
           gint      *dest)
{
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_INT)
    return FALSE;

  g_scanner_get_next_token (scanner);

  *dest = scanner->value.v_int;

  return TRUE;
}


/* serialize functions */

static void
plug_in_write_rc_string (FILE  *fp,
			 gchar *str)
{
  fputc ('"', fp);

  if (str)
    while (*str)
      {
	if (*str == '\n')
	  {
	    fputc ('\\', fp);
	    fputc ('n', fp);
	  }
	else if (*str == '\r')
	  {
	    fputc ('\\', fp);
	    fputc ('r', fp);
	  }
	else if (*str == '\032') /* ^Z is problematic on Windows */
	  {
	    fputc ('\\', fp);
	    fputc ('z', fp);
	  }
	else
	  {
	    if ((*str == '"') || (*str == '\\'))
	      fputc ('\\', fp);
	    fputc (*str, fp);
	  }
	str += 1;
      }

  fputc ('"', fp);
}

gboolean
plug_in_rc_write (GSList      *plug_in_defs,
                  const gchar *filename)
{
  FILE          *fp;
  PlugInDef     *plug_in_def;
  PlugInProcDef *proc_def;
  GSList        *list;
  GSList        *tmp2;
  gint i;

  g_return_val_if_fail (filename != NULL, FALSE);

  fp = fopen (filename, "w");
  if (!fp)
    return FALSE;

  for (list = plug_in_defs; list; list = list->next)
    {
      plug_in_def = list->data;

      if (plug_in_def->proc_defs)
	{
	  fprintf (fp, "(plug-in-def ");
	  plug_in_write_rc_string (fp, plug_in_def->prog);
	  fprintf (fp, " %ld", (long) plug_in_def->mtime);

	  tmp2 = plug_in_def->proc_defs;
	  if (tmp2)
	    fprintf (fp, "\n");

	  while (tmp2)
	    {
	      proc_def = tmp2->data;
	      tmp2 = tmp2->next;

	      fprintf (fp, "\t(proc-def \"%s\" %d\n",
		       proc_def->db_info.name, proc_def->db_info.proc_type);
	      fprintf (fp, "\t\t");
	      plug_in_write_rc_string (fp, proc_def->db_info.blurb);
	      fprintf (fp, "\n\t\t");
	      plug_in_write_rc_string (fp, proc_def->db_info.help);
	      fprintf (fp, "\n\t\t");
	      plug_in_write_rc_string (fp, proc_def->db_info.author);
	      fprintf (fp, "\n\t\t");
	      plug_in_write_rc_string (fp, proc_def->db_info.copyright);
	      fprintf (fp, "\n\t\t");
	      plug_in_write_rc_string (fp, proc_def->db_info.date);
	      fprintf (fp, "\n\t\t");
	      plug_in_write_rc_string (fp, proc_def->menu_path);
	      fprintf (fp, "\n\t\t");
	      plug_in_write_rc_string (fp, proc_def->extensions);
	      fprintf (fp, "\n\t\t");
	      plug_in_write_rc_string (fp, proc_def->prefixes);
	      fprintf (fp, "\n\t\t");
	      plug_in_write_rc_string (fp, proc_def->magics);
	      fprintf (fp, "\n\t\t");
	      plug_in_write_rc_string (fp, proc_def->image_types);
	      fprintf (fp, "\n\t\t%d %d\n",
		       proc_def->db_info.num_args, proc_def->db_info.num_values);

	      for (i = 0; i < proc_def->db_info.num_args; i++)
		{
		  fprintf (fp, "\t\t(proc-arg %d ",
			   proc_def->db_info.args[i].arg_type);

		  plug_in_write_rc_string (fp, proc_def->db_info.args[i].name);
		  plug_in_write_rc_string (fp, proc_def->db_info.args[i].description);

		  fprintf (fp, ")%s",
			   (proc_def->db_info.num_values ||
			    (i < (proc_def->db_info.num_args - 1))) ? "\n" : "");
		}

	      for (i = 0; i < proc_def->db_info.num_values; i++)
		{
		  fprintf (fp, "\t\t(proc-arg %d ",
			   proc_def->db_info.values[i].arg_type);

		  plug_in_write_rc_string (fp, proc_def->db_info.values[i].name);
		  plug_in_write_rc_string (fp, proc_def->db_info.values[i].description);

		  fprintf (fp, ")%s", (i < (proc_def->db_info.num_values - 1)) ? "\n" : "");
		}

	      fprintf (fp, ")");

	      if (tmp2)
		fprintf (fp, "\n");
	    }
	  
	  if (plug_in_def->locale_domain)
	    {
	      fprintf (fp, "\n\t(locale-def \"%s\"", plug_in_def->locale_domain);
	      if (plug_in_def->locale_path)
		fprintf (fp, " \"%s\")", plug_in_def->locale_path);
	      else
		fprintf (fp, ")");
	    }

	  if (plug_in_def->help_path)
	    {
	      fprintf (fp, "\n\t(help-def \"%s\")", plug_in_def->help_path);
	    }
	    
	  if (plug_in_def->has_init)
	    {
	      fprintf (fp, "\n\t(has-init)");
	    }

	  fprintf (fp, ")\n");

	  if (list->next)
	    fprintf (fp, "\n");
	}
      
    }

  fclose (fp);

  return TRUE;
}

