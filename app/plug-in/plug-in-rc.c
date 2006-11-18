/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in-rc.c
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
#include "libgimpconfig/gimpconfig.h"

#include "plug-in-types.h"

#include "core/gimp.h"

#include "pdb/gimp-pdb-compat.h"

#include "gimppluginprocedure.h"
#include "plug-in-def.h"
#include "plug-in-rc.h"

#include "gimp-intl.h"


/*
 *  All deserialize functions return G_TOKEN_LEFT_PAREN on success,
 *  or the GTokenType they would have expected but didn't get.
 */

static GTokenType plug_in_def_deserialize        (Gimp                 *gimp,
                                                  GScanner             *scanner,
                                                  GSList              **plug_in_defs);
static GTokenType plug_in_procedure_deserialize  (GScanner             *scanner,
                                                  Gimp                 *gimp,
                                                  const gchar          *prog,
                                                  GimpPlugInProcedure **proc);
static GTokenType plug_in_menu_path_deserialize  (GScanner             *scanner,
                                                  GimpPlugInProcedure  *proc);
static GTokenType plug_in_icon_deserialize       (GScanner             *scanner,
                                                  GimpPlugInProcedure  *proc);
static GTokenType plug_in_file_proc_deserialize  (GScanner             *scanner,
                                                  GimpPlugInProcedure  *proc);
static GTokenType plug_in_proc_arg_deserialize   (GScanner             *scanner,
                                                  Gimp                 *gimp,
                                                  GimpProcedure        *procedure,
                                                  gboolean              return_value);
static GTokenType plug_in_locale_def_deserialize (GScanner             *scanner,
                                                  PlugInDef            *plug_in_def);
static GTokenType plug_in_help_def_deserialize   (GScanner             *scanner,
                                                  PlugInDef            *plug_in_def);
static GTokenType plug_in_has_init_deserialize   (GScanner             *scanner,
                                                  PlugInDef            *plug_in_def);


enum
{
  PROTOCOL_VERSION = 1,
  PLUG_IN_DEF,
  PROC_DEF,
  LOCALE_DEF,
  HELP_DEF,
  HAS_INIT,
  PROC_ARG,
  MENU_PATH,
  ICON,
  LOAD_PROC,
  SAVE_PROC,
  EXTENSION,
  PREFIX,
  MAGIC,
  MIME_TYPE,
  THUMB_LOADER
};


GSList *
plug_in_rc_parse (Gimp         *gimp,
                  const gchar  *filename,
                  GError      **error)
{
  GScanner   *scanner;
  GEnumClass *enum_class;
  GSList     *plug_in_defs = NULL;
  gint        version      = GIMP_PROTOCOL_VERSION;
  GTokenType  token;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  scanner = gimp_scanner_new_file (filename, error);

  if (! scanner)
    return NULL;

  enum_class = g_type_class_ref (GIMP_TYPE_ICON_TYPE);

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
  g_scanner_scope_add_symbol (scanner, PLUG_IN_DEF,
                              "menu-path", GINT_TO_POINTER (MENU_PATH));
  g_scanner_scope_add_symbol (scanner, PLUG_IN_DEF,
                              "icon", GINT_TO_POINTER (ICON));
  g_scanner_scope_add_symbol (scanner, PLUG_IN_DEF,
                              "load-proc", GINT_TO_POINTER (LOAD_PROC));
  g_scanner_scope_add_symbol (scanner, PLUG_IN_DEF,
                              "save-proc", GINT_TO_POINTER (SAVE_PROC));

  g_scanner_scope_add_symbol (scanner, LOAD_PROC,
                              "extension", GINT_TO_POINTER (EXTENSION));
  g_scanner_scope_add_symbol (scanner, LOAD_PROC,
                              "prefix", GINT_TO_POINTER (PREFIX));
  g_scanner_scope_add_symbol (scanner, LOAD_PROC,
                              "magic", GINT_TO_POINTER (MAGIC));
  g_scanner_scope_add_symbol (scanner, LOAD_PROC,
                              "mime-type", GINT_TO_POINTER (MIME_TYPE));
  g_scanner_scope_add_symbol (scanner, LOAD_PROC,
                              "thumb-loader", GINT_TO_POINTER (THUMB_LOADER));

  g_scanner_scope_add_symbol (scanner, SAVE_PROC,
                              "extension", GINT_TO_POINTER (EXTENSION));
  g_scanner_scope_add_symbol (scanner, SAVE_PROC,
                              "prefix", GINT_TO_POINTER (PREFIX));
  g_scanner_scope_add_symbol (scanner, SAVE_PROC,
                              "mime-type", GINT_TO_POINTER (MIME_TYPE));

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
              token = plug_in_def_deserialize (gimp, scanner, &plug_in_defs);
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

  if (version != GIMP_PROTOCOL_VERSION ||
      token   != G_TOKEN_LEFT_PAREN)
    {
      if (version != GIMP_PROTOCOL_VERSION)
        {
          g_set_error (error,
                       GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_VERSION,
                       _("Skipping '%s': wrong GIMP protocol version."),
                       gimp_filename_to_utf8 (filename));
        }
      else
        {
          g_scanner_get_next_token (scanner);
          g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                                 _("fatal parse error"), TRUE);
        }

      g_slist_foreach (plug_in_defs, (GFunc) plug_in_def_free, NULL);
      g_slist_free (plug_in_defs);
      plug_in_defs = NULL;
    }

  g_type_class_unref (enum_class);

  gimp_scanner_destroy (scanner);

  return g_slist_reverse (plug_in_defs);
}

static GTokenType
plug_in_def_deserialize (Gimp      *gimp,
                         GScanner  *scanner,
                         GSList   **plug_in_defs)
{
  PlugInDef           *plug_in_def;
  GimpPlugInProcedure *proc = NULL;
  gchar               *name;
  gchar               *path;
  gint                 mtime;
  GTokenType           token;

  if (! gimp_scanner_parse_string (scanner, &name))
    return G_TOKEN_STRING;

  path = gimp_config_path_expand (name, TRUE, NULL);
  g_free (name);

  plug_in_def = plug_in_def_new (path);
  g_free (path);

  if (! gimp_scanner_parse_int (scanner, &mtime))
    {
      plug_in_def_free (plug_in_def);
      return G_TOKEN_INT;
    }

  plug_in_def->mtime = mtime;

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
              token = plug_in_procedure_deserialize (scanner, gimp,
                                                     plug_in_def->prog,
                                                     &proc);

              if (token == G_TOKEN_LEFT_PAREN)
                plug_in_def_add_procedure (plug_in_def, proc);

              if (proc)
                g_object_unref (proc);
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
          *plug_in_defs = g_slist_prepend (*plug_in_defs, plug_in_def);
          return G_TOKEN_LEFT_PAREN;
        }
    }

  plug_in_def_free (plug_in_def);

  return token;
}

static GTokenType
plug_in_procedure_deserialize (GScanner             *scanner,
                               Gimp                 *gimp,
                               const gchar          *prog,
                               GimpPlugInProcedure **proc)
{
  GimpProcedure   *procedure;
  GTokenType       token;
  gchar           *str;
  gint             proc_type;
  gint             n_args;
  gint             n_return_vals;
  gint             n_menu_paths;
  gint             i;

  if (! gimp_scanner_parse_string (scanner, &str))
    return G_TOKEN_STRING;

  if (! gimp_scanner_parse_int (scanner, &proc_type))
    {
      g_free (str);
      return G_TOKEN_INT;
    }

  procedure = gimp_plug_in_procedure_new (proc_type, prog);

  *proc = GIMP_PLUG_IN_PROCEDURE (procedure);

  gimp_object_take_name (GIMP_OBJECT (procedure),
                         gimp_canonicalize_identifier (str));

  procedure->original_name = str;

  if (! gimp_scanner_parse_string (scanner, &procedure->blurb))
    return G_TOKEN_STRING;
  if (! gimp_scanner_parse_string (scanner, &procedure->help))
    return G_TOKEN_STRING;
  if (! gimp_scanner_parse_string (scanner, &procedure->author))
    return G_TOKEN_STRING;
  if (! gimp_scanner_parse_string (scanner, &procedure->copyright))
    return G_TOKEN_STRING;
  if (! gimp_scanner_parse_string (scanner, &procedure->date))
    return G_TOKEN_STRING;
  if (! gimp_scanner_parse_string (scanner, &(*proc)->menu_label))
    return G_TOKEN_STRING;

  if (! gimp_scanner_parse_int (scanner, &n_menu_paths))
    return G_TOKEN_INT;

  for (i = 0; i < n_menu_paths; i++)
    {
      token = plug_in_menu_path_deserialize (scanner, *proc);
      if (token != G_TOKEN_LEFT_PAREN)
        return token;
    }

  token = plug_in_icon_deserialize (scanner, *proc);
  if (token != G_TOKEN_LEFT_PAREN)
    return token;

  token = plug_in_file_proc_deserialize (scanner, *proc);
  if (token != G_TOKEN_LEFT_PAREN)
    return token;

  if (! gimp_scanner_parse_string (scanner, &str))
    return G_TOKEN_STRING;

  gimp_plug_in_procedure_set_image_types (*proc, str);
  g_free (str);

  if (! gimp_scanner_parse_int (scanner, (gint *) &n_args))
    return G_TOKEN_INT;
  if (! gimp_scanner_parse_int (scanner, (gint *) &n_return_vals))
    return G_TOKEN_INT;

  for (i = 0; i < n_args; i++)
    {
      token = plug_in_proc_arg_deserialize (scanner, gimp, procedure, FALSE);
      if (token != G_TOKEN_LEFT_PAREN)
        return token;
    }

  for (i = 0; i < n_return_vals; i++)
    {
      token = plug_in_proc_arg_deserialize (scanner, gimp, procedure, TRUE);
      if (token != G_TOKEN_LEFT_PAREN)
        return token;
    }

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType
plug_in_menu_path_deserialize (GScanner            *scanner,
                               GimpPlugInProcedure *proc)
{
  gchar *menu_path;

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_LEFT_PAREN))
    return G_TOKEN_LEFT_PAREN;

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_SYMBOL) ||
      GPOINTER_TO_INT (scanner->value.v_symbol) != MENU_PATH)
    return G_TOKEN_SYMBOL;

  if (! gimp_scanner_parse_string (scanner, &menu_path))
    return G_TOKEN_STRING;

  proc->menu_paths = g_list_append (proc->menu_paths, menu_path);

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType
plug_in_icon_deserialize (GScanner            *scanner,
                          GimpPlugInProcedure *proc)
{
  GEnumClass   *enum_class;
  GEnumValue   *enum_value;
  GimpIconType  icon_type;
  gint          icon_data_length;
  gchar        *icon_name;
  guint8       *icon_data;

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_LEFT_PAREN))
    return G_TOKEN_LEFT_PAREN;

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_SYMBOL) ||
      GPOINTER_TO_INT (scanner->value.v_symbol) != ICON)
    return G_TOKEN_SYMBOL;

  enum_class = g_type_class_peek (GIMP_TYPE_ICON_TYPE);

  switch (g_scanner_peek_next_token (scanner))
    {
    case G_TOKEN_IDENTIFIER:
      g_scanner_get_next_token (scanner);

      enum_value = g_enum_get_value_by_nick (G_ENUM_CLASS (enum_class),
                                             scanner->value.v_identifier);
      if (!enum_value)
        enum_value = g_enum_get_value_by_name (G_ENUM_CLASS (enum_class),
                                               scanner->value.v_identifier);

      if (!enum_value)
        {
          g_scanner_error (scanner,
                           _("invalid value '%s' for icon type"),
                           scanner->value.v_identifier);
          return G_TOKEN_NONE;
        }
      break;

    case G_TOKEN_INT:
      g_scanner_get_next_token (scanner);

      enum_value = g_enum_get_value (enum_class,
                                     (gint) scanner->value.v_int64);

      if (!enum_value)
        {
          g_scanner_error (scanner,
                           _("invalid value '%ld' for icon type"),
                           (glong) scanner->value.v_int64);
          return G_TOKEN_NONE;
        }
      break;

    default:
      return G_TOKEN_IDENTIFIER;
    }

  icon_type = enum_value->value;

  if (! gimp_scanner_parse_int (scanner, &icon_data_length))
    return G_TOKEN_INT;

  switch (icon_type)
    {
    case GIMP_ICON_TYPE_STOCK_ID:
    case GIMP_ICON_TYPE_IMAGE_FILE:
      icon_data_length = -1;

      if (! gimp_scanner_parse_string_no_validate (scanner, &icon_name))
        return G_TOKEN_STRING;

      icon_data = (guint8 *) icon_name;
      break;

    case GIMP_ICON_TYPE_INLINE_PIXBUF:
      if (icon_data_length < 0)
        return G_TOKEN_STRING;

      if (! gimp_scanner_parse_data (scanner, icon_data_length, &icon_data))
        return G_TOKEN_STRING;
      break;
    }

  proc->icon_type        = icon_type;
  proc->icon_data_length = icon_data_length;
  proc->icon_data        = icon_data;

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType
plug_in_file_proc_deserialize (GScanner            *scanner,
                               GimpPlugInProcedure *proc)
{
  GTokenType  token;
  gint        symbol;
  gchar      *value;

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_LEFT_PAREN))
    return G_TOKEN_LEFT_PAREN;

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_SYMBOL))
    return G_TOKEN_SYMBOL;

  symbol = GPOINTER_TO_INT (scanner->value.v_symbol);
  if (symbol != LOAD_PROC && symbol != SAVE_PROC)
    return G_TOKEN_SYMBOL;

  proc->file_proc = TRUE;

  g_scanner_set_scope (scanner, symbol);

  while (g_scanner_peek_next_token (scanner) == G_TOKEN_LEFT_PAREN)
    {
      token = g_scanner_get_next_token (scanner);

      if (token != G_TOKEN_LEFT_PAREN)
        return token;

      if (! gimp_scanner_parse_token (scanner, G_TOKEN_SYMBOL))
        return G_TOKEN_SYMBOL;

      symbol = GPOINTER_TO_INT (scanner->value.v_symbol);

      if (symbol == MAGIC)
        {
          if (! gimp_scanner_parse_string_no_validate (scanner, &value))
            return G_TOKEN_STRING;
        }
      else
        {
          if (! gimp_scanner_parse_string (scanner, &value))
            return G_TOKEN_STRING;
        }

      switch (symbol)
        {
        case EXTENSION:
          g_free (proc->extensions);
          proc->extensions = value;
          break;

        case PREFIX:
          g_free (proc->prefixes);
          proc->prefixes = value;
          break;

        case MAGIC:
          g_free (proc->magics);
          proc->magics = value;
          break;

        case MIME_TYPE:
          gimp_plug_in_procedure_set_mime_type (proc, value);
          g_free (value);
          break;

        case THUMB_LOADER:
          gimp_plug_in_procedure_set_thumb_loader (proc, value);
          g_free (value);
          break;

        default:
           return G_TOKEN_SYMBOL;
        }
      if (! gimp_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
        return G_TOKEN_RIGHT_PAREN;
    }

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  g_scanner_set_scope (scanner, PLUG_IN_DEF);

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType
plug_in_proc_arg_deserialize (GScanner      *scanner,
                              Gimp          *gimp,
                              GimpProcedure *procedure,
                              gboolean       return_value)
{
  GTokenType  token;
  gint        arg_type;
  gchar      *name = NULL;
  gchar      *desc = NULL;
  GParamSpec *pspec;

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_LEFT_PAREN))
    {
      token = G_TOKEN_LEFT_PAREN;
      goto error;
    }

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_SYMBOL) ||
      GPOINTER_TO_INT (scanner->value.v_symbol) != PROC_ARG)
    {
      token = G_TOKEN_SYMBOL;
      goto error;
    }

  if (! gimp_scanner_parse_int (scanner, (gint *) &arg_type))
    {
      token = G_TOKEN_INT;
      goto error;
    }
  if (! gimp_scanner_parse_string (scanner, &name))
    {
      token = G_TOKEN_STRING;
      goto error;
    }
  if (! gimp_scanner_parse_string (scanner, &desc))
    {
      token = G_TOKEN_STRING;
      goto error;
    }

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    {
      token = G_TOKEN_RIGHT_PAREN;
      goto error;
    }

  token = G_TOKEN_LEFT_PAREN;

  pspec = gimp_pdb_compat_param_spec (gimp, arg_type, name, desc);

  if (return_value)
    gimp_procedure_add_return_value (procedure, pspec);
  else
    gimp_procedure_add_argument (procedure, pspec);

 error:

  g_free (name);
  g_free (desc);

  return token;
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
  GEnumClass       *enum_class;
  GSList           *list;

  writer = gimp_config_writer_new_file (filename,
                                        FALSE,
                                        "GIMP pluginrc\n\n"
                                        "This file can safely be removed and "
                                        "will be automatically regenerated by "
                                        "querying the installed plugins.",
                                        error);
  if (!writer)
    return FALSE;

  enum_class = g_type_class_ref (GIMP_TYPE_ICON_TYPE);

  gimp_config_writer_open (writer, "protocol-version");
  gimp_config_writer_printf (writer, "%d", GIMP_PROTOCOL_VERSION);
  gimp_config_writer_close (writer);
  gimp_config_writer_linefeed (writer);

  for (list = plug_in_defs; list; list = list->next)
    {
      PlugInDef *plug_in_def = list->data;

      if (plug_in_def->procedures)
        {
          GSList *list2;
          gchar  *utf8;

          utf8 = g_filename_to_utf8 (plug_in_def->prog, -1, NULL, NULL, NULL);

          if (! utf8)
            continue;

          gimp_config_writer_open (writer, "plug-in-def");
          gimp_config_writer_string (writer, utf8);
          gimp_config_writer_printf (writer, "%ld", plug_in_def->mtime);

          g_free (utf8);

          for (list2 = plug_in_def->procedures; list2; list2 = list2->next)
            {
              GimpPlugInProcedure *proc      = list2->data;
              GimpProcedure       *procedure = GIMP_PROCEDURE (proc);
              GEnumValue          *enum_value;
              GList               *list3;
              gint                 i;

              if (proc->installed_during_init)
                continue;

              gimp_config_writer_open (writer, "proc-def");
              gimp_config_writer_printf (writer, "\"%s\" %d",
                                         procedure->original_name,
                                         procedure->proc_type);
              gimp_config_writer_linefeed (writer);
              gimp_config_writer_string (writer, procedure->blurb);
              gimp_config_writer_linefeed (writer);
              gimp_config_writer_string (writer, procedure->help);
              gimp_config_writer_linefeed (writer);
              gimp_config_writer_string (writer, procedure->author);
              gimp_config_writer_linefeed (writer);
              gimp_config_writer_string (writer, procedure->copyright);
              gimp_config_writer_linefeed (writer);
              gimp_config_writer_string (writer, procedure->date);
              gimp_config_writer_linefeed (writer);
              gimp_config_writer_string (writer, proc->menu_label);
              gimp_config_writer_linefeed (writer);

              gimp_config_writer_printf (writer, "%d",
                                         g_list_length (proc->menu_paths));
              for (list3 = proc->menu_paths; list3; list3 = list3->next)
                {
                  gimp_config_writer_open (writer, "menu-path");
                  gimp_config_writer_string (writer, list3->data);
                  gimp_config_writer_close (writer);
                }

              gimp_config_writer_open (writer, "icon");
              enum_value = g_enum_get_value (enum_class, proc->icon_type);
              gimp_config_writer_identifier (writer, enum_value->value_nick);
              gimp_config_writer_printf (writer, "%d",
                                         proc->icon_data_length);

              switch (proc->icon_type)
                {
                case GIMP_ICON_TYPE_STOCK_ID:
                case GIMP_ICON_TYPE_IMAGE_FILE:
                  gimp_config_writer_string (writer, (gchar *) proc->icon_data);
                  break;

                case GIMP_ICON_TYPE_INLINE_PIXBUF:
                  gimp_config_writer_data (writer, proc->icon_data_length,
                                           proc->icon_data);
                  break;
                }

              gimp_config_writer_close (writer);

              if (proc->file_proc)
                {
                  gimp_config_writer_open (writer,
                                           proc->image_types ?
                                           "save-proc" : "load-proc");

                  if (proc->extensions && *proc->extensions)
                    {
                      gimp_config_writer_open (writer, "extension");
                      gimp_config_writer_string (writer, proc->extensions);
                      gimp_config_writer_close (writer);
                    }

                  if (proc->prefixes && *proc->prefixes)
                    {
                      gimp_config_writer_open (writer, "prefix");
                      gimp_config_writer_string (writer, proc->prefixes);
                      gimp_config_writer_close (writer);
                    }

                  if (proc->magics && *proc->magics)
                    {
                      gimp_config_writer_open (writer, "magic");
                      gimp_config_writer_string (writer, proc->magics);
                      gimp_config_writer_close (writer);
                    }

                  if (proc->mime_type)
                    {
                      gimp_config_writer_open (writer, "mime-type");
                      gimp_config_writer_string (writer, proc->mime_type);
                      gimp_config_writer_close (writer);
                    }

                  if (proc->thumb_loader)
                    {
                      gimp_config_writer_open (writer, "thumb-loader");
                      gimp_config_writer_string (writer, proc->thumb_loader);
                      gimp_config_writer_close (writer);
                    }

                  gimp_config_writer_close (writer);
                }

              gimp_config_writer_linefeed (writer);

              gimp_config_writer_string (writer, proc->image_types);
              gimp_config_writer_linefeed (writer);

              gimp_config_writer_printf (writer, "%d %d",
                                         procedure->num_args,
                                         procedure->num_values);

              for (i = 0; i < procedure->num_args; i++)
                {
                  GParamSpec *pspec = procedure->args[i];

                  gimp_config_writer_open (writer, "proc-arg");
                  gimp_config_writer_printf (writer, "%d",
                                             gimp_pdb_compat_arg_type_from_gtype (G_PARAM_SPEC_VALUE_TYPE (pspec)));

                  gimp_config_writer_string (writer,
                                             g_param_spec_get_name (pspec));
                  gimp_config_writer_string (writer,
                                             g_param_spec_get_blurb (pspec));

                  gimp_config_writer_close (writer);
                }

              for (i = 0; i < procedure->num_values; i++)
                {
                  GParamSpec *pspec = procedure->values[i];

                  gimp_config_writer_open (writer, "proc-arg");
                  gimp_config_writer_printf (writer, "%d",
                                             gimp_pdb_compat_arg_type_from_gtype (G_PARAM_SPEC_VALUE_TYPE (pspec)));

                  gimp_config_writer_string (writer,
                                             g_param_spec_get_name (pspec));
                  gimp_config_writer_string (writer,
                                             g_param_spec_get_blurb (pspec));

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

  g_type_class_unref (enum_class);

  return gimp_config_writer_finish (writer, "end of pluginrc", error);
}
