/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in-rc.c
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpprotocol.h"
#include "libgimpconfig/gimpconfig.h"

#include "plug-in-types.h"

#include "core/gimp.h"

#include "pdb/gimp-pdb-compat.h"

#include "gimpplugindef.h"
#include "gimppluginprocedure.h"
#include "plug-in-rc.h"

#include "gimp-intl.h"


#define PLUG_IN_RC_FILE_VERSION 5


/*
 *  All deserialize functions return G_TOKEN_LEFT_PAREN on success,
 *  or the GTokenType they would have expected but didn't get,
 *  or G_TOKEN_ERROR if the function already set an error itself.
 */

static GTokenType plug_in_def_deserialize        (Gimp                 *gimp,
                                                  GScanner             *scanner,
                                                  GSList              **plug_in_defs);
static GTokenType plug_in_procedure_deserialize  (GScanner             *scanner,
                                                  Gimp                 *gimp,
                                                  GFile                *file,
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
                                                  GimpPlugInDef        *plug_in_def);
static GTokenType plug_in_help_def_deserialize   (GScanner             *scanner,
                                                  GimpPlugInDef        *plug_in_def);
static GTokenType plug_in_has_init_deserialize   (GScanner             *scanner,
                                                  GimpPlugInDef        *plug_in_def);


enum
{
  PROTOCOL_VERSION = 1,
  FILE_VERSION,
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
  EXTENSIONS,
  PREFIXES,
  MAGICS,
  PRIORITY,
  MIME_TYPES,
  HANDLES_URI,
  HANDLES_RAW,
  THUMB_LOADER
};


GSList *
plug_in_rc_parse (Gimp    *gimp,
                  GFile   *file,
                  GError **error)
{
  GScanner   *scanner;
  GEnumClass *enum_class;
  GSList     *plug_in_defs     = NULL;
  gint        protocol_version = GIMP_PROTOCOL_VERSION;
  gint        file_version     = PLUG_IN_RC_FILE_VERSION;
  GTokenType  token;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  scanner = gimp_scanner_new_gfile (file, error);

  if (! scanner)
    return NULL;

  enum_class = g_type_class_ref (GIMP_TYPE_ICON_TYPE);

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
                              "handles-uri", GINT_TO_POINTER (HANDLES_URI));
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
                              "handles-uri", GINT_TO_POINTER (HANDLES_URI));

  token = G_TOKEN_LEFT_PAREN;

  while (protocol_version == GIMP_PROTOCOL_VERSION   &&
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
              if (gimp_scanner_parse_int (scanner, &protocol_version))
                token = G_TOKEN_RIGHT_PAREN;
              break;

            case FILE_VERSION:
              token = G_TOKEN_INT;
              if (gimp_scanner_parse_int (scanner, &file_version))
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

  if (protocol_version != GIMP_PROTOCOL_VERSION   ||
      file_version     != PLUG_IN_RC_FILE_VERSION ||
      token            != G_TOKEN_LEFT_PAREN)
    {
      if (protocol_version != GIMP_PROTOCOL_VERSION)
        {
          g_set_error (error,
                       GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_VERSION,
                       _("Skipping '%s': wrong GIMP protocol version."),
                       gimp_file_get_utf8_name (file));
        }
      else if (file_version != PLUG_IN_RC_FILE_VERSION)
        {
          g_set_error (error,
                       GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_VERSION,
                       _("Skipping '%s': wrong pluginrc file format version."),
                       gimp_file_get_utf8_name (file));
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

  gimp_scanner_destroy (scanner);

  return g_slist_reverse (plug_in_defs);
}

static GTokenType
plug_in_def_deserialize (Gimp      *gimp,
                         GScanner  *scanner,
                         GSList   **plug_in_defs)
{
  GimpPlugInDef       *plug_in_def;
  GimpPlugInProcedure *proc = NULL;
  gchar               *path;
  GFile               *file;
  gint64               mtime;
  GTokenType           token;
  GError              *error = NULL;

  if (! gimp_scanner_parse_string (scanner, &path))
    return G_TOKEN_STRING;

  if (! (path && *path))
    {
      g_scanner_error (scanner, "plug-in filename is empty");
      return G_TOKEN_ERROR;
    }

  file = gimp_file_new_for_config_path (path, &error);
  g_free (path);

  if (! file)
    {
      g_scanner_error (scanner,
                       "unable to parse plug-in filename: %s",
                       error->message);
      g_clear_error (&error);
      return G_TOKEN_ERROR;
    }

  plug_in_def = gimp_plug_in_def_new (file);
  g_object_unref (file);

  if (! gimp_scanner_parse_int64 (scanner, &mtime))
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
              token = plug_in_procedure_deserialize (scanner, gimp,
                                                     plug_in_def->file,
                                                     &proc);

              if (token == G_TOKEN_LEFT_PAREN)
                gimp_plug_in_def_add_procedure (plug_in_def, proc);

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

  g_object_unref (plug_in_def);

  return token;
}

static GTokenType
plug_in_procedure_deserialize (GScanner             *scanner,
                               Gimp                 *gimp,
                               GFile                *file,
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

  if (! (str && *str))
    {
      g_scanner_error (scanner, "procedure name is empty");
      return G_TOKEN_ERROR;
    }

  if (! gimp_scanner_parse_int (scanner, &proc_type))
    {
      g_free (str);
      return G_TOKEN_INT;
    }

  if (proc_type != GIMP_PLUGIN &&
      proc_type != GIMP_EXTENSION)
    {
      g_free (str);
      g_scanner_error (scanner, "procedure type %d is out of range",
                       proc_type);
      return G_TOKEN_ERROR;
    }

  procedure = gimp_plug_in_procedure_new (proc_type, file);

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
    case GIMP_ICON_TYPE_ICON_NAME:
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

  gimp_plug_in_procedure_take_icon (proc, icon_type,
                                    icon_data, icon_data_length);

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType
plug_in_file_proc_deserialize (GScanner            *scanner,
                               GimpPlugInProcedure *proc)
{
  GTokenType token;
  gint       symbol;

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

      switch (symbol)
        {
        case EXTENSIONS:
          {
            gchar *extensions;

            if (! gimp_scanner_parse_string (scanner, &extensions))
              return G_TOKEN_STRING;

            g_free (proc->extensions);
            proc->extensions = extensions;
          }
          break;

        case PREFIXES:
          {
            gchar *prefixes;

            if (! gimp_scanner_parse_string (scanner, &prefixes))
              return G_TOKEN_STRING;

            g_free (proc->prefixes);
            proc->prefixes = prefixes;
          }
          break;

        case MAGICS:
          {
            gchar *magics;

            if (! gimp_scanner_parse_string_no_validate (scanner, &magics))
              return G_TOKEN_STRING;

            g_free (proc->magics);
            proc->magics = magics;
          }
          break;

        case PRIORITY:
          {
            gint priority;

            if (! gimp_scanner_parse_int (scanner, &priority))
              return G_TOKEN_INT;

            gimp_plug_in_procedure_set_priority (proc, priority);
          }
          break;

        case MIME_TYPES:
          {
            gchar *mime_types;

            if (! gimp_scanner_parse_string (scanner, &mime_types))
              return G_TOKEN_STRING;

            gimp_plug_in_procedure_set_mime_types (proc, mime_types);

            g_free (mime_types);
          }
          break;

        case HANDLES_URI:
          gimp_plug_in_procedure_set_handles_uri (proc);
          break;

        case HANDLES_RAW:
          gimp_plug_in_procedure_set_handles_raw (proc);
          break;

        case THUMB_LOADER:
          {
            gchar *thumb_loader;

            if (! gimp_scanner_parse_string (scanner, &thumb_loader))
              return G_TOKEN_STRING;

            gimp_plug_in_procedure_set_thumb_loader (proc, thumb_loader);

            g_free (thumb_loader);
          }
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
plug_in_locale_def_deserialize (GScanner      *scanner,
                                GimpPlugInDef *plug_in_def)
{
  gchar *domain_name;
  gchar *domain_path   = NULL;
  gchar *expanded_path = NULL;

  if (! gimp_scanner_parse_string (scanner, &domain_name))
    return G_TOKEN_STRING;

  if (gimp_scanner_parse_string (scanner, &domain_path))
    expanded_path = gimp_config_path_expand (domain_path, TRUE, NULL);

  gimp_plug_in_def_set_locale_domain (plug_in_def, domain_name, expanded_path);

  g_free (domain_name);
  g_free (domain_path);
  g_free (expanded_path);

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType
plug_in_help_def_deserialize (GScanner      *scanner,
                              GimpPlugInDef *plug_in_def)
{
  gchar *domain_name;
  gchar *domain_uri;

  if (! gimp_scanner_parse_string (scanner, &domain_name))
    return G_TOKEN_STRING;

  if (! gimp_scanner_parse_string (scanner, &domain_uri))
    domain_uri = NULL;

  gimp_plug_in_def_set_help_domain (plug_in_def, domain_name, domain_uri);

  g_free (domain_name);
  g_free (domain_uri);

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}

static GTokenType
plug_in_has_init_deserialize (GScanner      *scanner,
                              GimpPlugInDef *plug_in_def)
{
  gimp_plug_in_def_set_has_init (plug_in_def, TRUE);

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}


/* serialize functions */

gboolean
plug_in_rc_write (GSList  *plug_in_defs,
                  GFile   *file,
                  GError **error)
{
  GimpConfigWriter *writer;
  GEnumClass       *enum_class;
  GSList           *list;

  writer = gimp_config_writer_new_gfile (file,
                                         FALSE,
                                         "GIMP pluginrc\n\n"
                                         "This file can safely be removed and "
                                         "will be automatically regenerated by "
                                         "querying the installed plug-ins.",
                                         error);
  if (!writer)
    return FALSE;

  enum_class = g_type_class_ref (GIMP_TYPE_ICON_TYPE);

  gimp_config_writer_open (writer, "protocol-version");
  gimp_config_writer_printf (writer, "%d", GIMP_PROTOCOL_VERSION);
  gimp_config_writer_close (writer);

  gimp_config_writer_open (writer, "file-version");
  gimp_config_writer_printf (writer, "%d", PLUG_IN_RC_FILE_VERSION);
  gimp_config_writer_close (writer);

  gimp_config_writer_linefeed (writer);

  for (list = plug_in_defs; list; list = list->next)
    {
      GimpPlugInDef *plug_in_def = list->data;

      if (plug_in_def->procedures)
        {
          GSList *list2;
          gchar  *path;

          path = gimp_file_get_config_path (plug_in_def->file, NULL);
          if (! path)
            continue;

          gimp_config_writer_open (writer, "plug-in-def");
          gimp_config_writer_string (writer, path);
          gimp_config_writer_printf (writer, "%"G_GINT64_FORMAT,
                                     plug_in_def->mtime);

          g_free (path);

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
                case GIMP_ICON_TYPE_ICON_NAME:
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
                      gimp_config_writer_open (writer, "extensions");
                      gimp_config_writer_string (writer, proc->extensions);
                      gimp_config_writer_close (writer);
                    }

                  if (proc->prefixes && *proc->prefixes)
                    {
                      gimp_config_writer_open (writer, "prefixes");
                      gimp_config_writer_string (writer, proc->prefixes);
                      gimp_config_writer_close (writer);
                    }

                  if (proc->magics && *proc->magics)
                    {
                      gimp_config_writer_open (writer, "magics");
                      gimp_config_writer_string (writer, proc->magics);
                      gimp_config_writer_close (writer);
                    }

                  if (proc->priority)
                    {
                      gimp_config_writer_open (writer, "priority");
                      gimp_config_writer_printf (writer, "%d", proc->priority);
                      gimp_config_writer_close (writer);
                    }

                  if (proc->mime_types && *proc->mime_types)
                    {
                      gimp_config_writer_open (writer, "mime-types");
                      gimp_config_writer_string (writer, proc->mime_types);
                      gimp_config_writer_close (writer);
                    }

                  if (proc->priority)
                    {
                      gimp_config_writer_open (writer, "priority");
                      gimp_config_writer_printf (writer, "%d", proc->priority);
                      gimp_config_writer_close (writer);
                    }

                  if (proc->handles_uri)
                    {
                      gimp_config_writer_open (writer, "handles-uri");
                      gimp_config_writer_close (writer);
                    }

                  if (proc->handles_raw && ! proc->image_types)
                    {
                      gimp_config_writer_open (writer, "handles-raw");
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
                {
                  path = gimp_config_path_unexpand (plug_in_def->locale_domain_path,
                                                    TRUE, NULL);
                  if (path)
                    {
                      gimp_config_writer_string (writer, path);
                      g_free (path);
                    }
                }

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
