/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in-rc.c
 * Copyright (C) 2001  Sven Neumann <sven@ligma.org>
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmabase/ligmaprotocol.h"
#include "libligmaconfig/ligmaconfig.h"

#include "libligma/ligmagpparams.h"

#include "plug-in-types.h"

#include "core/ligma.h"

#include "ligmaplugindef.h"
#include "ligmapluginprocedure.h"
#include "plug-in-rc.h"

#include "ligma-intl.h"


#define PLUG_IN_RC_FILE_VERSION 13


/*
 *  All deserialize functions return G_TOKEN_LEFT_PAREN on success,
 *  or the GTokenType they would have expected but didn't get,
 *  or G_TOKEN_ERROR if the function already set an error itself.
 */

static GTokenType plug_in_def_deserialize        (Ligma                 *ligma,
                                                  GScanner             *scanner,
                                                  GSList              **plug_in_defs);
static GTokenType plug_in_procedure_deserialize  (GScanner             *scanner,
                                                  Ligma                 *ligma,
                                                  GFile                *file,
                                                  LigmaPlugInProcedure **proc);
static GTokenType plug_in_menu_path_deserialize  (GScanner             *scanner,
                                                  LigmaPlugInProcedure  *proc);
static GTokenType plug_in_icon_deserialize       (GScanner             *scanner,
                                                  LigmaPlugInProcedure  *proc);
static GTokenType plug_in_file_or_batch_proc_deserialize  (GScanner             *scanner,
                                                           LigmaPlugInProcedure  *proc);
static GTokenType plug_in_proc_arg_deserialize   (GScanner             *scanner,
                                                  Ligma                 *ligma,
                                                  LigmaProcedure        *procedure,
                                                  gboolean              return_value);
static GTokenType plug_in_help_def_deserialize   (GScanner             *scanner,
                                                  LigmaPlugInDef        *plug_in_def);
static GTokenType plug_in_has_init_deserialize   (GScanner             *scanner,
                                                  LigmaPlugInDef        *plug_in_def);


enum
{
  PROTOCOL_VERSION = 1,
  FILE_VERSION,
  PLUG_IN_DEF,
  PROC_DEF,
  HELP_DEF,
  HAS_INIT,
  PROC_ARG,
  MENU_PATH,
  ICON,
  LOAD_PROC,
  SAVE_PROC,
  EXTENSIONS,
  PREFIXES,
  MAGICS,
  PRIORITY,
  MIME_TYPES,
  HANDLES_REMOTE,
  HANDLES_RAW,
  THUMB_LOADER,
  BATCH_INTERPRETER,
};


GSList *
plug_in_rc_parse (Ligma    *ligma,
                  GFile   *file,
                  GError **error)
{
  GScanner   *scanner;
  GEnumClass *enum_class;
  GSList     *plug_in_defs     = NULL;
  gint        protocol_version = LIGMA_PROTOCOL_VERSION;
  gint        file_version     = PLUG_IN_RC_FILE_VERSION;
  GTokenType  token;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  scanner = ligma_scanner_new_file (file, error);

  if (! scanner)
    return NULL;

  enum_class = g_type_class_ref (LIGMA_TYPE_ICON_TYPE);

  g_scanner_scope_add_symbol (scanner, 0,
                              "protocol-version",
                              GINT_TO_POINTER (PROTOCOL_VERSION));
  g_scanner_scope_add_symbol (scanner, 0,
                              "file-version",
                              GINT_TO_POINTER (FILE_VERSION));
  g_scanner_scope_add_symbol (scanner, 0,
                              "plug-in-def", GINT_TO_POINTER (PLUG_IN_DEF));

  g_scanner_scope_add_symbol (scanner, PLUG_IN_DEF,
                              "proc-def", GINT_TO_POINTER (PROC_DEF));
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
  g_scanner_scope_add_symbol (scanner, PLUG_IN_DEF,
                              "batch-interpreter", GINT_TO_POINTER (BATCH_INTERPRETER));

  g_scanner_scope_add_symbol (scanner, LOAD_PROC,
                              "extensions", GINT_TO_POINTER (EXTENSIONS));
  g_scanner_scope_add_symbol (scanner, LOAD_PROC,
                              "prefixes", GINT_TO_POINTER (PREFIXES));
  g_scanner_scope_add_symbol (scanner, LOAD_PROC,
                              "magics", GINT_TO_POINTER (MAGICS));
  g_scanner_scope_add_symbol (scanner, LOAD_PROC,
                              "priority", GINT_TO_POINTER (PRIORITY));
  g_scanner_scope_add_symbol (scanner, LOAD_PROC,
                              "mime-types", GINT_TO_POINTER (MIME_TYPES));
  g_scanner_scope_add_symbol (scanner, LOAD_PROC,
                              "handles-remote", GINT_TO_POINTER (HANDLES_REMOTE));
  g_scanner_scope_add_symbol (scanner, LOAD_PROC,
                              "handles-raw", GINT_TO_POINTER (HANDLES_RAW));
  g_scanner_scope_add_symbol (scanner, LOAD_PROC,
                              "thumb-loader", GINT_TO_POINTER (THUMB_LOADER));

  g_scanner_scope_add_symbol (scanner, SAVE_PROC,
                              "extensions", GINT_TO_POINTER (EXTENSIONS));
  g_scanner_scope_add_symbol (scanner, SAVE_PROC,
                              "prefixes", GINT_TO_POINTER (PREFIXES));
  g_scanner_scope_add_symbol (scanner, SAVE_PROC,
                              "priority", GINT_TO_POINTER (PRIORITY));
  g_scanner_scope_add_symbol (scanner, SAVE_PROC,
                              "mime-types", GINT_TO_POINTER (MIME_TYPES));
  g_scanner_scope_add_symbol (scanner, SAVE_PROC,
                              "handles-remote", GINT_TO_POINTER (HANDLES_REMOTE));

  token = G_TOKEN_LEFT_PAREN;

  while (protocol_version == LIGMA_PROTOCOL_VERSION   &&
         file_version     == PLUG_IN_RC_FILE_VERSION &&
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
              if (ligma_scanner_parse_int (scanner, &protocol_version))
                token = G_TOKEN_RIGHT_PAREN;
              break;

            case FILE_VERSION:
              token = G_TOKEN_INT;
              if (ligma_scanner_parse_int (scanner, &file_version))
                token = G_TOKEN_RIGHT_PAREN;
              break;

            case PLUG_IN_DEF:
              g_scanner_set_scope (scanner, PLUG_IN_DEF);
              token = plug_in_def_deserialize (ligma, scanner, &plug_in_defs);
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

  if (protocol_version != LIGMA_PROTOCOL_VERSION   ||
      file_version     != PLUG_IN_RC_FILE_VERSION ||
      token            != G_TOKEN_LEFT_PAREN)
    {
      if (protocol_version != LIGMA_PROTOCOL_VERSION)
        {
          g_set_error (error,
                       LIGMA_CONFIG_ERROR, LIGMA_CONFIG_ERROR_VERSION,
                       _("Skipping '%s': wrong LIGMA protocol version."),
                       ligma_file_get_utf8_name (file));
        }
      else if (file_version != PLUG_IN_RC_FILE_VERSION)
        {
          g_set_error (error,
                       LIGMA_CONFIG_ERROR, LIGMA_CONFIG_ERROR_VERSION,
                       _("Skipping '%s': wrong pluginrc file format version."),
                       ligma_file_get_utf8_name (file));
        }
      else if (token != G_TOKEN_ERROR)
        {
          g_scanner_get_next_token (scanner);
          g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                                 _("fatal parse error"), TRUE);
        }

      g_slist_free_full (plug_in_defs, (GDestroyNotify) g_object_unref);
      plug_in_defs = NULL;
    }

  g_type_class_unref (enum_class);

  ligma_scanner_unref (scanner);

  return g_slist_reverse (plug_in_defs);
}

static GTokenType
plug_in_def_deserialize (Ligma      *ligma,
                         GScanner  *scanner,
                         GSList   **plug_in_defs)
{
  LigmaPlugInDef       *plug_in_def;
  LigmaPlugInProcedure *proc = NULL;
  gchar               *path;
  GFile               *file;
  gint64               mtime;
  GTokenType           token;
  GError              *error = NULL;

  if (! ligma_scanner_parse_string (scanner, &path))
    return G_TOKEN_STRING;

  if (! (path && *path))
    {
      g_scanner_error (scanner, "plug-in filename is empty");
      return G_TOKEN_ERROR;
    }

  file = ligma_file_new_for_config_path (path, &error);
  g_free (path);

  if (! file)
    {
      g_scanner_error (scanner,
                       "unable to parse plug-in filename: %s",
                       error->message);
      g_clear_error (&error);
      return G_TOKEN_ERROR;
    }

  plug_in_def = ligma_plug_in_def_new (file);
  g_object_unref (file);

  if (! ligma_scanner_parse_int64 (scanner, &mtime))
    {
      g_object_unref (plug_in_def);
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
              token = plug_in_procedure_deserialize (scanner, ligma,
                                                     plug_in_def->file,
                                                     &proc);

              if (token == G_TOKEN_LEFT_PAREN)
                ligma_plug_in_def_add_procedure (plug_in_def, proc);

              if (proc)
                g_object_unref (proc);
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

      if (ligma_scanner_parse_token (scanner, token))
        {
          *plug_in_defs = g_slist_prepend (*plug_in_defs, plug_in_def);
          return G_TOKEN_LEFT_PAREN;
        }
    }

  g_object_unref (plug_in_def);

  return token;
}

static GTokenType
plug_in_procedure_deserialize (GScanner             *scanner,
                               Ligma                 *ligma,
                               GFile                *file,
                               LigmaPlugInProcedure **proc)
{
  LigmaProcedure   *procedure;
  GTokenType       token;
  gchar           *str;
  gint             proc_type;
  gint             n_args;
  gint             n_return_vals;
  gint             n_menu_paths;
  gint             sensitivity_mask;
  gint             i;

  if (! ligma_scanner_parse_string (scanner, &str))
    return G_TOKEN_STRING;

  if (! (str && *str))
    {
      g_scanner_error (scanner, "procedure name is empty");
      return G_TOKEN_ERROR;
    }

  if (! ligma_scanner_parse_int (scanner, &proc_type))
    {
      g_free (str);
      return G_TOKEN_INT;
    }

  if (proc_type != LIGMA_PDB_PROC_TYPE_PLUGIN &&
      proc_type != LIGMA_PDB_PROC_TYPE_EXTENSION)
    {
      g_free (str);
      g_scanner_error (scanner, "procedure type %d is out of range",
                       proc_type);
      return G_TOKEN_ERROR;
    }

  procedure = ligma_plug_in_procedure_new (proc_type, file);

  *proc = LIGMA_PLUG_IN_PROCEDURE (procedure);

  ligma_object_take_name (LIGMA_OBJECT (procedure), str);

  if (! ligma_scanner_parse_string (scanner, &procedure->blurb))
    return G_TOKEN_STRING;
  if (! ligma_scanner_parse_string (scanner, &procedure->help))
    return G_TOKEN_STRING;
  if (! ligma_scanner_parse_string (scanner, &procedure->authors))
    return G_TOKEN_STRING;
  if (! ligma_scanner_parse_string (scanner, &procedure->copyright))
    return G_TOKEN_STRING;
  if (! ligma_scanner_parse_string (scanner, &procedure->date))
    return G_TOKEN_STRING;
  if (! ligma_scanner_parse_string (scanner, &(*proc)->menu_label))
    return G_TOKEN_STRING;

  if (! ligma_scanner_parse_int (scanner, &n_menu_paths))
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

  token = plug_in_file_or_batch_proc_deserialize (scanner, *proc);
  if (token != G_TOKEN_LEFT_PAREN)
    return token;

  if (! ligma_scanner_parse_string (scanner, &str))
    return G_TOKEN_STRING;

  ligma_plug_in_procedure_set_image_types (*proc, str);
  g_free (str);

  if (! ligma_scanner_parse_int (scanner, &sensitivity_mask))
    return G_TOKEN_INT;

  ligma_plug_in_procedure_set_sensitivity_mask (*proc, sensitivity_mask);

  if (! ligma_scanner_parse_int (scanner, (gint *) &n_args))
    return G_TOKEN_INT;
  if (! ligma_scanner_parse_int (scanner, (gint *) &n_return_vals))
    return G_TOKEN_INT;

  for (i = 0; i < n_args; i++)
    {
      token = plug_in_proc_arg_deserialize (scanner, ligma, procedure, FALSE);
      if (token != G_TOKEN_LEFT_PAREN)
        return token;
    }

  for (i = 0; i < n_return_vals; i++)
    {
      token = plug_in_proc_arg_deserialize (scanner, ligma, procedure, TRUE);
      if (token != G_TOKEN_LEFT_PAREN)
        return token;
    }

  if (! ligma_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType
plug_in_menu_path_deserialize (GScanner            *scanner,
                               LigmaPlugInProcedure *proc)
{
  gchar *menu_path;

  if (! ligma_scanner_parse_token (scanner, G_TOKEN_LEFT_PAREN))
    return G_TOKEN_LEFT_PAREN;

  if (! ligma_scanner_parse_token (scanner, G_TOKEN_SYMBOL) ||
      GPOINTER_TO_INT (scanner->value.v_symbol) != MENU_PATH)
    return G_TOKEN_SYMBOL;

  if (! ligma_scanner_parse_string (scanner, &menu_path))
    return G_TOKEN_STRING;

  proc->menu_paths = g_list_append (proc->menu_paths, menu_path);

  if (! ligma_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType
plug_in_icon_deserialize (GScanner            *scanner,
                          LigmaPlugInProcedure *proc)
{
  GEnumClass   *enum_class;
  GEnumValue   *enum_value;
  LigmaIconType  icon_type;
  gint          icon_data_length;
  gchar        *icon_name;
  guint8       *icon_data;

  if (! ligma_scanner_parse_token (scanner, G_TOKEN_LEFT_PAREN))
    return G_TOKEN_LEFT_PAREN;

  if (! ligma_scanner_parse_token (scanner, G_TOKEN_SYMBOL) ||
      GPOINTER_TO_INT (scanner->value.v_symbol) != ICON)
    return G_TOKEN_SYMBOL;

  enum_class = g_type_class_peek (LIGMA_TYPE_ICON_TYPE);

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

  if (! ligma_scanner_parse_int (scanner, &icon_data_length))
    return G_TOKEN_INT;

  switch (icon_type)
    {
    case LIGMA_ICON_TYPE_ICON_NAME:
    case LIGMA_ICON_TYPE_IMAGE_FILE:
      icon_data_length = -1;

      if (! ligma_scanner_parse_string_no_validate (scanner, &icon_name))
        return G_TOKEN_STRING;

      icon_data = (guint8 *) icon_name;
      break;

    case LIGMA_ICON_TYPE_PIXBUF:
      if (icon_data_length < 0)
        return G_TOKEN_STRING;

      if (! ligma_scanner_parse_data (scanner, icon_data_length, &icon_data))
        return G_TOKEN_STRING;
      break;
    }

  ligma_plug_in_procedure_take_icon (proc, icon_type,
                                    icon_data, icon_data_length,
                                    NULL);

  if (! ligma_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType
plug_in_file_or_batch_proc_deserialize (GScanner            *scanner,
                                        LigmaPlugInProcedure *proc)
{
  GTokenType token;
  gint       symbol;

  if (! ligma_scanner_parse_token (scanner, G_TOKEN_LEFT_PAREN))
    return G_TOKEN_LEFT_PAREN;

  if (! ligma_scanner_parse_token (scanner, G_TOKEN_SYMBOL))
    return G_TOKEN_SYMBOL;

  symbol = GPOINTER_TO_INT (scanner->value.v_symbol);
  if (symbol != LOAD_PROC && symbol != SAVE_PROC && symbol != BATCH_INTERPRETER)
    return G_TOKEN_SYMBOL;

  if (symbol == BATCH_INTERPRETER)
    {
      gchar *interpreter_name;

      if (! ligma_scanner_parse_string (scanner, &interpreter_name))
        return G_TOKEN_STRING;

      ligma_plug_in_procedure_set_batch_interpreter (proc, interpreter_name);
      g_free (interpreter_name);
    }
  else
    {
      proc->file_proc = TRUE;

      g_scanner_set_scope (scanner, symbol);

      while (g_scanner_peek_next_token (scanner) == G_TOKEN_LEFT_PAREN)
        {
          token = g_scanner_get_next_token (scanner);

          if (token != G_TOKEN_LEFT_PAREN)
            return token;

          if (! ligma_scanner_parse_token (scanner, G_TOKEN_SYMBOL))
            return G_TOKEN_SYMBOL;

          symbol = GPOINTER_TO_INT (scanner->value.v_symbol);

          switch (symbol)
            {
            case EXTENSIONS:
                {
                  gchar *extensions;

                  if (! ligma_scanner_parse_string (scanner, &extensions))
                    return G_TOKEN_STRING;

                  g_free (proc->extensions);
                  proc->extensions = extensions;
                }
              break;

            case PREFIXES:
                {
                  gchar *prefixes;

                  if (! ligma_scanner_parse_string (scanner, &prefixes))
                    return G_TOKEN_STRING;

                  g_free (proc->prefixes);
                  proc->prefixes = prefixes;
                }
              break;

            case MAGICS:
                {
                  gchar *magics;

                  if (! ligma_scanner_parse_string_no_validate (scanner, &magics))
                    return G_TOKEN_STRING;

                  g_free (proc->magics);
                  proc->magics = magics;
                }
              break;

            case PRIORITY:
                {
                  gint priority;

                  if (! ligma_scanner_parse_int (scanner, &priority))
                    return G_TOKEN_INT;

                  ligma_plug_in_procedure_set_priority (proc, priority);
                }
              break;

            case MIME_TYPES:
                {
                  gchar *mime_types;

                  if (! ligma_scanner_parse_string (scanner, &mime_types))
                    return G_TOKEN_STRING;

                  ligma_plug_in_procedure_set_mime_types (proc, mime_types);

                  g_free (mime_types);
                }
              break;

            case HANDLES_REMOTE:
              ligma_plug_in_procedure_set_handles_remote (proc);
              break;

            case HANDLES_RAW:
              ligma_plug_in_procedure_set_handles_raw (proc);
              break;

            case THUMB_LOADER:
                {
                  gchar *thumb_loader;

                  if (! ligma_scanner_parse_string (scanner, &thumb_loader))
                    return G_TOKEN_STRING;

                  ligma_plug_in_procedure_set_thumb_loader (proc, thumb_loader);

                  g_free (thumb_loader);
                }
              break;

            default:
              return G_TOKEN_SYMBOL;
            }
          if (! ligma_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
            return G_TOKEN_RIGHT_PAREN;
        }
    }

  if (! ligma_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  g_scanner_set_scope (scanner, PLUG_IN_DEF);

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType
plug_in_proc_arg_deserialize (GScanner      *scanner,
                              Ligma          *ligma,
                              LigmaProcedure *procedure,
                              gboolean       return_value)
{
  GTokenType  token;
  GPParamDef  param_def = { 0, };
  GParamSpec *pspec;

  if (! ligma_scanner_parse_token (scanner, G_TOKEN_LEFT_PAREN))
    {
      token = G_TOKEN_LEFT_PAREN;
      goto error;
    }

  if (! ligma_scanner_parse_token (scanner, G_TOKEN_SYMBOL) ||
      GPOINTER_TO_INT (scanner->value.v_symbol) != PROC_ARG)
    {
      token = G_TOKEN_SYMBOL;
      goto error;
    }

  if (! ligma_scanner_parse_int (scanner, (gint *) &param_def.param_def_type))
    {
      token = G_TOKEN_INT;
      goto error;
    }

  if (! ligma_scanner_parse_string (scanner, &param_def.type_name)       ||
      ! ligma_scanner_parse_string (scanner, &param_def.value_type_name) ||
      ! ligma_scanner_parse_string (scanner, &param_def.name)            ||
      ! ligma_scanner_parse_string (scanner, &param_def.nick)            ||
      ! ligma_scanner_parse_string (scanner, &param_def.blurb)           ||
      ! ligma_scanner_parse_int    (scanner, (gint *) &param_def.flags))
    {
      token = G_TOKEN_STRING;
      goto error;
    }

  switch (param_def.param_def_type)
    {
    case GP_PARAM_DEF_TYPE_DEFAULT:
      break;

    case GP_PARAM_DEF_TYPE_INT:
      if (! ligma_scanner_parse_int64 (scanner,
                                      &param_def.meta.m_int.min_val) ||
          ! ligma_scanner_parse_int64 (scanner,
                                      &param_def.meta.m_int.max_val) ||
          ! ligma_scanner_parse_int64 (scanner,
                                      &param_def.meta.m_int.default_val))
        {
          token = G_TOKEN_INT;
          goto error;
        }
      break;

    case GP_PARAM_DEF_TYPE_UNIT:
      if (! ligma_scanner_parse_int (scanner,
                                    &param_def.meta.m_unit.allow_pixels) ||
          ! ligma_scanner_parse_int (scanner,
                                    &param_def.meta.m_unit.allow_percent) ||
          ! ligma_scanner_parse_int (scanner,
                                    &param_def.meta.m_unit.default_val))
        {
          token = G_TOKEN_INT;
          goto error;
        }
      break;

    case GP_PARAM_DEF_TYPE_ENUM:
      if (! ligma_scanner_parse_int (scanner,
                                    &param_def.meta.m_enum.default_val))
        {
          token = G_TOKEN_STRING;
          goto error;
        }
      break;

    case GP_PARAM_DEF_TYPE_BOOLEAN:
      if (! ligma_scanner_parse_int (scanner,
                                    &param_def.meta.m_boolean.default_val))
        {
          token = G_TOKEN_INT;
          goto error;
        }
      break;

    case GP_PARAM_DEF_TYPE_FLOAT:
      if (! ligma_scanner_parse_float (scanner,
                                      &param_def.meta.m_float.min_val) ||
          ! ligma_scanner_parse_float (scanner,
                                      &param_def.meta.m_float.max_val) ||
          ! ligma_scanner_parse_float (scanner,
                                      &param_def.meta.m_float.default_val))
        {
          token = G_TOKEN_FLOAT;
          goto error;
        }
      break;

    case GP_PARAM_DEF_TYPE_STRING:
      if (! ligma_scanner_parse_string (scanner,
                                       &param_def.meta.m_string.default_val))
        {
          token = G_TOKEN_STRING;
          goto error;
        }
      break;

    case GP_PARAM_DEF_TYPE_COLOR:
      if (! ligma_scanner_parse_int (scanner,
                                    &param_def.meta.m_color.has_alpha))
        {
          token = G_TOKEN_INT;
          goto error;
        }
      if (! ligma_scanner_parse_float (scanner,
                                      &param_def.meta.m_color.default_val.r) ||
          ! ligma_scanner_parse_float (scanner,
                                      &param_def.meta.m_color.default_val.g) ||
          ! ligma_scanner_parse_float (scanner,
                                      &param_def.meta.m_color.default_val.b) ||
          ! ligma_scanner_parse_float (scanner,
                                      &param_def.meta.m_color.default_val.a))
        {
          token = G_TOKEN_FLOAT;
          goto error;
        }
      break;

    case GP_PARAM_DEF_TYPE_ID:
      if (! ligma_scanner_parse_int (scanner,
                                    &param_def.meta.m_id.none_ok))
        {
          token = G_TOKEN_INT;
          goto error;
        }
      break;

    case GP_PARAM_DEF_TYPE_ID_ARRAY:
      if (! ligma_scanner_parse_string (scanner,
                                       &param_def.meta.m_id_array.type_name))
        {
          token = G_TOKEN_STRING;
          goto error;
        }
      break;
    }

  if (! ligma_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    {
      token = G_TOKEN_RIGHT_PAREN;
      goto error;
    }

  token = G_TOKEN_LEFT_PAREN;

  pspec = _ligma_gp_param_def_to_param_spec (&param_def);

  if (return_value)
    ligma_procedure_add_return_value (procedure, pspec);
  else
    ligma_procedure_add_argument (procedure, pspec);

 error:

  g_free (param_def.type_name);
  g_free (param_def.value_type_name);
  g_free (param_def.name);
  g_free (param_def.nick);
  g_free (param_def.blurb);

  switch (param_def.param_def_type)
    {
    case GP_PARAM_DEF_TYPE_DEFAULT:
    case GP_PARAM_DEF_TYPE_INT:
    case GP_PARAM_DEF_TYPE_UNIT:
    case GP_PARAM_DEF_TYPE_ENUM:
      break;

    case GP_PARAM_DEF_TYPE_BOOLEAN:
    case GP_PARAM_DEF_TYPE_FLOAT:
      break;

    case GP_PARAM_DEF_TYPE_STRING:
      g_free (param_def.meta.m_string.default_val);
      break;

    case GP_PARAM_DEF_TYPE_COLOR:
    case GP_PARAM_DEF_TYPE_ID:
      break;

    case GP_PARAM_DEF_TYPE_ID_ARRAY:
      g_free (param_def.meta.m_id_array.type_name);
      break;
    }

  return token;
}

static GTokenType
plug_in_help_def_deserialize (GScanner      *scanner,
                              LigmaPlugInDef *plug_in_def)
{
  gchar *domain_name;
  gchar *domain_uri;

  if (! ligma_scanner_parse_string (scanner, &domain_name))
    return G_TOKEN_STRING;

  if (! ligma_scanner_parse_string (scanner, &domain_uri))
    domain_uri = NULL;

  ligma_plug_in_def_set_help_domain (plug_in_def, domain_name, domain_uri);

  g_free (domain_name);
  g_free (domain_uri);

  if (! ligma_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType
plug_in_has_init_deserialize (GScanner      *scanner,
                              LigmaPlugInDef *plug_in_def)
{
  ligma_plug_in_def_set_has_init (plug_in_def, TRUE);

  if (! ligma_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}


/* serialize functions */

static void
plug_in_rc_write_proc_arg (LigmaConfigWriter *writer,
                           GParamSpec       *pspec)
{
  GPParamDef param_def = { 0, };

  _ligma_param_spec_to_gp_param_def (pspec, &param_def);

  ligma_config_writer_open (writer, "proc-arg");
  ligma_config_writer_printf (writer, "%d", param_def.param_def_type);

  ligma_config_writer_string (writer, param_def.type_name);
  ligma_config_writer_string (writer, param_def.value_type_name);
  ligma_config_writer_string (writer, g_param_spec_get_name (pspec));
  ligma_config_writer_string (writer, g_param_spec_get_nick (pspec));
  ligma_config_writer_string (writer, g_param_spec_get_blurb (pspec));
  ligma_config_writer_printf (writer, "%d", pspec->flags);

  switch (param_def.param_def_type)
    {
      gchar buf[4][G_ASCII_DTOSTR_BUF_SIZE];

    case GP_PARAM_DEF_TYPE_DEFAULT:
      break;

    case GP_PARAM_DEF_TYPE_INT:
      ligma_config_writer_printf (writer,
                                 "%" G_GINT64_FORMAT
                                 " %" G_GINT64_FORMAT
                                 " %" G_GINT64_FORMAT,
                                 param_def.meta.m_int.min_val,
                                 param_def.meta.m_int.max_val,
                                 param_def.meta.m_int.default_val);
      break;

    case GP_PARAM_DEF_TYPE_UNIT:
      ligma_config_writer_printf (writer, "%d %d %d",
                                 param_def.meta.m_unit.allow_pixels,
                                 param_def.meta.m_unit.allow_percent,
                                 param_def.meta.m_unit.default_val);
      break;

    case GP_PARAM_DEF_TYPE_ENUM:
      ligma_config_writer_printf (writer, "%d",
                                 param_def.meta.m_enum.default_val);
      break;

    case GP_PARAM_DEF_TYPE_BOOLEAN:
      ligma_config_writer_printf (writer, "%d",
                                 param_def.meta.m_boolean.default_val);
      break;

    case GP_PARAM_DEF_TYPE_FLOAT:
      g_ascii_dtostr (buf[0], sizeof (buf[0]),
                      param_def.meta.m_float.min_val);
      g_ascii_dtostr (buf[1], sizeof (buf[1]),
                      param_def.meta.m_float.max_val),
      g_ascii_dtostr (buf[2], sizeof (buf[2]),
                      param_def.meta.m_float.default_val);
      ligma_config_writer_printf (writer, "%s %s %s",
                                 buf[0], buf[1], buf[2]);
      break;

    case GP_PARAM_DEF_TYPE_STRING:
      ligma_config_writer_string (writer,
                                 param_def.meta.m_string.default_val);
      break;

    case GP_PARAM_DEF_TYPE_COLOR:
      g_ascii_dtostr (buf[0], sizeof (buf[0]),
                      param_def.meta.m_color.default_val.r);
      g_ascii_dtostr (buf[1], sizeof (buf[1]),
                      param_def.meta.m_color.default_val.g),
      g_ascii_dtostr (buf[2], sizeof (buf[2]),
                      param_def.meta.m_color.default_val.b);
      g_ascii_dtostr (buf[3], sizeof (buf[3]),
                      param_def.meta.m_color.default_val.a);
      ligma_config_writer_printf (writer, "%d %s %s %s %s",
                                 param_def.meta.m_color.has_alpha,
                                 buf[0], buf[1], buf[2], buf[3]);
      break;

    case GP_PARAM_DEF_TYPE_ID:
      ligma_config_writer_printf (writer, "%d",
                                 param_def.meta.m_id.none_ok);
      break;

    case GP_PARAM_DEF_TYPE_ID_ARRAY:
      ligma_config_writer_string (writer,
                                 param_def.meta.m_id_array.type_name);
      break;
    }

  ligma_config_writer_close (writer);
}

gboolean
plug_in_rc_write (GSList  *plug_in_defs,
                  GFile   *file,
                  GError **error)
{
  LigmaConfigWriter *writer;
  GEnumClass       *enum_class;
  GSList           *list;

  writer = ligma_config_writer_new_from_file (file,
                                             FALSE,
                                             "LIGMA pluginrc\n\n"
                                             "This file can safely be removed and "
                                             "will be automatically regenerated by "
                                             "querying the installed plug-ins.",
                                             error);
  if (! writer)
    return FALSE;

  enum_class = g_type_class_ref (LIGMA_TYPE_ICON_TYPE);

  ligma_config_writer_open (writer, "protocol-version");
  ligma_config_writer_printf (writer, "%d", LIGMA_PROTOCOL_VERSION);
  ligma_config_writer_close (writer);

  ligma_config_writer_open (writer, "file-version");
  ligma_config_writer_printf (writer, "%d", PLUG_IN_RC_FILE_VERSION);
  ligma_config_writer_close (writer);

  ligma_config_writer_linefeed (writer);

  for (list = plug_in_defs; list; list = list->next)
    {
      LigmaPlugInDef *plug_in_def = list->data;

      if (plug_in_def->procedures)
        {
          GSList *list2;
          gchar  *path;

          path = ligma_file_get_config_path (plug_in_def->file, NULL);
          if (! path)
            continue;

          ligma_config_writer_open (writer, "plug-in-def");
          ligma_config_writer_string (writer, path);
          ligma_config_writer_printf (writer, "%"G_GINT64_FORMAT,
                                     plug_in_def->mtime);

          g_free (path);

          for (list2 = plug_in_def->procedures; list2; list2 = list2->next)
            {
              LigmaPlugInProcedure *proc      = list2->data;
              LigmaProcedure       *procedure = LIGMA_PROCEDURE (proc);
              GEnumValue          *enum_value;
              GList               *list3;
              gint                 i;

              if (proc->installed_during_init)
                continue;

              ligma_config_writer_open (writer, "proc-def");
              ligma_config_writer_printf (writer, "\"%s\" %d",
                                         ligma_object_get_name (procedure),
                                         procedure->proc_type);
              ligma_config_writer_linefeed (writer);
              ligma_config_writer_string (writer, procedure->blurb);
              ligma_config_writer_linefeed (writer);
              ligma_config_writer_string (writer, procedure->help);
              ligma_config_writer_linefeed (writer);
              ligma_config_writer_string (writer, procedure->authors);
              ligma_config_writer_linefeed (writer);
              ligma_config_writer_string (writer, procedure->copyright);
              ligma_config_writer_linefeed (writer);
              ligma_config_writer_string (writer, procedure->date);
              ligma_config_writer_linefeed (writer);
              ligma_config_writer_string (writer, proc->menu_label);
              ligma_config_writer_linefeed (writer);

              ligma_config_writer_printf (writer, "%d",
                                         g_list_length (proc->menu_paths));
              for (list3 = proc->menu_paths; list3; list3 = list3->next)
                {
                  ligma_config_writer_open (writer, "menu-path");
                  ligma_config_writer_string (writer, list3->data);
                  ligma_config_writer_close (writer);
                }

              ligma_config_writer_open (writer, "icon");
              enum_value = g_enum_get_value (enum_class, proc->icon_type);
              ligma_config_writer_identifier (writer, enum_value->value_nick);
              ligma_config_writer_printf (writer, "%d",
                                         proc->icon_data_length);

              switch (proc->icon_type)
                {
                case LIGMA_ICON_TYPE_ICON_NAME:
                case LIGMA_ICON_TYPE_IMAGE_FILE:
                  ligma_config_writer_string (writer, (gchar *) proc->icon_data);
                  break;

                case LIGMA_ICON_TYPE_PIXBUF:
                  ligma_config_writer_data (writer, proc->icon_data_length,
                                           proc->icon_data);
                  break;
                }

              ligma_config_writer_close (writer);

              if (proc->file_proc)
                {
                  ligma_config_writer_open (writer,
                                           proc->image_types ?
                                           "save-proc" : "load-proc");

                  if (proc->extensions && *proc->extensions)
                    {
                      ligma_config_writer_open (writer, "extensions");
                      ligma_config_writer_string (writer, proc->extensions);
                      ligma_config_writer_close (writer);
                    }

                  if (proc->prefixes && *proc->prefixes)
                    {
                      ligma_config_writer_open (writer, "prefixes");
                      ligma_config_writer_string (writer, proc->prefixes);
                      ligma_config_writer_close (writer);
                    }

                  if (proc->magics && *proc->magics)
                    {
                      ligma_config_writer_open (writer, "magics");
                      ligma_config_writer_string (writer, proc->magics);
                      ligma_config_writer_close (writer);
                    }

                  if (proc->priority)
                    {
                      ligma_config_writer_open (writer, "priority");
                      ligma_config_writer_printf (writer, "%d", proc->priority);
                      ligma_config_writer_close (writer);
                    }

                  if (proc->mime_types && *proc->mime_types)
                    {
                      ligma_config_writer_open (writer, "mime-types");
                      ligma_config_writer_string (writer, proc->mime_types);
                      ligma_config_writer_close (writer);
                    }

                  if (proc->priority)
                    {
                      ligma_config_writer_open (writer, "priority");
                      ligma_config_writer_printf (writer, "%d", proc->priority);
                      ligma_config_writer_close (writer);
                    }

                  if (proc->handles_remote)
                    {
                      ligma_config_writer_open (writer, "handles-remote");
                      ligma_config_writer_close (writer);
                    }

                  if (proc->handles_raw && ! proc->image_types)
                    {
                      ligma_config_writer_open (writer, "handles-raw");
                      ligma_config_writer_close (writer);
                    }

                  if (proc->thumb_loader)
                    {
                      ligma_config_writer_open (writer, "thumb-loader");
                      ligma_config_writer_string (writer, proc->thumb_loader);
                      ligma_config_writer_close (writer);
                    }

                  ligma_config_writer_close (writer);
                }
              else if (proc->batch_interpreter)
                {
                  ligma_config_writer_open (writer, "batch-interpreter");
                  ligma_config_writer_string (writer, proc->batch_interpreter_name);
                  ligma_config_writer_close (writer);
                }


              ligma_config_writer_linefeed (writer);

              ligma_config_writer_string (writer, proc->image_types);
              ligma_config_writer_linefeed (writer);

              ligma_config_writer_printf (writer, "%d",
                                         proc->sensitivity_mask);
              ligma_config_writer_linefeed (writer);

              ligma_config_writer_printf (writer, "%d %d",
                                         procedure->num_args,
                                         procedure->num_values);

              for (i = 0; i < procedure->num_args; i++)
                {
                  GParamSpec *pspec = procedure->args[i];

                  plug_in_rc_write_proc_arg (writer, pspec);
                }

              for (i = 0; i < procedure->num_values; i++)
                {
                  GParamSpec *pspec = procedure->values[i];

                  plug_in_rc_write_proc_arg (writer, pspec);
                }

              ligma_config_writer_close (writer);
            }

          if (plug_in_def->help_domain_name)
            {
              ligma_config_writer_open (writer, "help-def");
              ligma_config_writer_string (writer,
                                         plug_in_def->help_domain_name);

              if (plug_in_def->help_domain_uri)
                ligma_config_writer_string (writer,
                                           plug_in_def->help_domain_uri);

             ligma_config_writer_close (writer);
            }

          if (plug_in_def->has_init)
            {
              ligma_config_writer_open (writer, "has-init");
              ligma_config_writer_close (writer);
            }

          ligma_config_writer_close (writer);
        }
    }

  g_type_class_unref (enum_class);

  return ligma_config_writer_finish (writer, "end of pluginrc", error);
}
