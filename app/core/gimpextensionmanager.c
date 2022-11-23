/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * ligmaextensionmanager.c
 * Copyright (C) 2018 Jehan <jehan@ligma.org>
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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "config/ligmacoreconfig.h"

#include "ligma.h"
#include "ligmaextension.h"
#include "ligmaextension-error.h"
#include "ligmaobject.h"
#include "ligmamarshal.h"
#include "ligma-utils.h"

#include "ligmaextensionmanager.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_LIGMA,
  PROP_BRUSH_PATHS,
  PROP_DYNAMICS_PATHS,
  PROP_MYPAINT_BRUSH_PATHS,
  PROP_PATTERN_PATHS,
  PROP_GRADIENT_PATHS,
  PROP_PALETTE_PATHS,
  PROP_TOOL_PRESET_PATHS,
  PROP_SPLASH_PATHS,
  PROP_THEME_PATHS,
  PROP_PLUG_IN_PATHS,
};

enum
{
  EXTENSION_INSTALLED,
  EXTENSION_REMOVED,
  LAST_SIGNAL
};

struct _LigmaExtensionManagerPrivate
{
  Ligma       *ligma;

  /* Installed system (read-only) extensions. */
  GList      *sys_extensions;
  /* Self-installed (read-write) extensions. */
  GList      *extensions;
  /* Uninstalled extensions (cached to allow undo). */
  GList      *uninstalled_extensions;

  /* Running extensions */
  GHashTable *running_extensions;

  /* Metadata properties */
  GList      *brush_paths;
  GList      *dynamics_paths;
  GList      *mypaint_brush_paths;
  GList      *pattern_paths;
  GList      *gradient_paths;
  GList      *palette_paths;
  GList      *tool_preset_paths;
  GList      *splash_paths;
  GList      *theme_paths;
  GList      *plug_in_paths;
};

static void     ligma_extension_manager_config_iface_init   (LigmaConfigInterface  *iface);
static gboolean ligma_extension_manager_serialize           (LigmaConfig           *config,
                                                            LigmaConfigWriter     *writer,
                                                            gpointer              data);
static gboolean ligma_extension_manager_deserialize         (LigmaConfig           *config,
                                                            GScanner             *scanner,
                                                            gint                  nest_level,
                                                            gpointer              data);

static void     ligma_extension_manager_finalize            (GObject              *object);
static void     ligma_extension_manager_set_property        (GObject              *object,
                                                            guint                 property_id,
                                                            const GValue         *value,
                                                            GParamSpec           *pspec);
static void     ligma_extension_manager_get_property        (GObject              *object,
                                                            guint                 property_id,
                                                            GValue               *value,
                                                            GParamSpec           *pspec);

static void     ligma_extension_manager_serialize_extension (LigmaExtensionManager *manager,
                                                            LigmaExtension        *extension,
                                                            LigmaConfigWriter     *writer);

static void     ligma_extension_manager_refresh             (LigmaExtensionManager *manager);
static void     ligma_extension_manager_search_directory    (LigmaExtensionManager *manager,
                                                            GFile                *directory,
                                                            gboolean              system_dir);

static void     ligma_extension_manager_extension_running   (LigmaExtension        *extension,
                                                            GParamSpec           *pspec,
                                                            LigmaExtensionManager *manager);


G_DEFINE_TYPE_WITH_CODE (LigmaExtensionManager, ligma_extension_manager,
                         LIGMA_TYPE_OBJECT,
                         G_ADD_PRIVATE (LigmaExtensionManager)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                ligma_extension_manager_config_iface_init))

#define parent_class ligma_extension_manager_parent_class

static guint signals[LAST_SIGNAL] = { 0, };

static void
ligma_extension_manager_class_init (LigmaExtensionManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = ligma_extension_manager_finalize;
  object_class->set_property = ligma_extension_manager_set_property;
  object_class->get_property = ligma_extension_manager_get_property;

  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma", NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_BRUSH_PATHS,
                                   g_param_spec_pointer ("brush-paths",
                                                         NULL, NULL,
                                                         LIGMA_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_DYNAMICS_PATHS,
                                   g_param_spec_pointer ("dynamics-paths",
                                                         NULL, NULL,
                                                         LIGMA_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_MYPAINT_BRUSH_PATHS,
                                   g_param_spec_pointer ("mypaint-brush-paths",
                                                         NULL, NULL,
                                                         LIGMA_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PATTERN_PATHS,
                                   g_param_spec_pointer ("pattern-paths",
                                                         NULL, NULL,
                                                         LIGMA_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_GRADIENT_PATHS,
                                   g_param_spec_pointer ("gradient-paths",
                                                         NULL, NULL,
                                                         LIGMA_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PALETTE_PATHS,
                                   g_param_spec_pointer ("palette-paths",
                                                         NULL, NULL,
                                                         LIGMA_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_TOOL_PRESET_PATHS,
                                   g_param_spec_pointer ("tool-preset-paths",
                                                         NULL, NULL,
                                                         LIGMA_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_SPLASH_PATHS,
                                   g_param_spec_pointer ("splash-paths",
                                                         NULL, NULL,
                                                         LIGMA_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_THEME_PATHS,
                                   g_param_spec_pointer ("theme-paths",
                                                         NULL, NULL,
                                                         LIGMA_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PLUG_IN_PATHS,
                                   g_param_spec_pointer ("plug-in-paths",
                                                         NULL, NULL,
                                                         LIGMA_PARAM_READWRITE));

  signals[EXTENSION_INSTALLED] =
    g_signal_new ("extension-installed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaExtensionManagerClass, extension_installed),
                  NULL, NULL,
                  ligma_marshal_VOID__OBJECT_BOOLEAN,
                  G_TYPE_NONE, 2,
                  LIGMA_TYPE_EXTENSION,
                  G_TYPE_BOOLEAN);
  signals[EXTENSION_REMOVED] =
    g_signal_new ("extension-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaExtensionManagerClass, extension_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);
}

static void
ligma_extension_manager_init (LigmaExtensionManager *manager)
{
  manager->p = ligma_extension_manager_get_instance_private (manager);
  manager->p->extensions     = NULL;
  manager->p->sys_extensions = NULL;
}

static void
ligma_extension_manager_config_iface_init (LigmaConfigInterface *iface)
{
  iface->serialize   = ligma_extension_manager_serialize;
  iface->deserialize = ligma_extension_manager_deserialize;
}

static gboolean
ligma_extension_manager_serialize (LigmaConfig       *config,
                                  LigmaConfigWriter *writer,
                                  gpointer          data G_GNUC_UNUSED)
{
  LigmaExtensionManager *manager = LIGMA_EXTENSION_MANAGER (config);
  GList                *iter;

  /* TODO: another information we will want to add will be the last
   * extension list update to allow ourselves to regularly check for
   * updates while not doing it too often.
   */
  for (iter = manager->p->extensions; iter; iter = iter->next)
    ligma_extension_manager_serialize_extension (manager, iter->data, writer);
  for (iter = manager->p->sys_extensions; iter; iter = iter->next)
    {
      if (g_list_find_custom (manager->p->extensions, iter->data,
                              (GCompareFunc) ligma_extension_cmp))
        continue;
      ligma_extension_manager_serialize_extension (manager, iter->data, writer);
    }

  return TRUE;
}

static gboolean
ligma_extension_manager_deserialize (LigmaConfig *config,
                                    GScanner   *scanner,
                                    gint        nest_level,
                                    gpointer    data)
{
  LigmaExtensionManager  *manager  = LIGMA_EXTENSION_MANAGER (config);
  GList                **processed = (GList**) data;
  GTokenType             token;

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_IDENTIFIER;
          break;

        case G_TOKEN_IDENTIFIER:
          {
            gchar    *name      = NULL;
            gboolean  is_active = FALSE;
            GType     type;

            type = g_type_from_name (scanner->value.v_identifier);

            if (! type)
              {
                g_scanner_error (scanner,
                                 "unable to determine type of '%s'",
                                 scanner->value.v_identifier);
                return FALSE;
              }

            if (! g_type_is_a (type, LIGMA_TYPE_EXTENSION))
              {
                g_scanner_error (scanner,
                                 "'%s' is not a subclass of '%s'",
                                 scanner->value.v_identifier,
                                 g_type_name (LIGMA_TYPE_EXTENSION));
                return FALSE;
              }

            if (! ligma_scanner_parse_string (scanner, &name))
              {
                g_scanner_error (scanner,
                                 "Expected extension id not found after LigmaExtension.");
                return FALSE;
              }

            if (! name || strlen (name) == 0)
              {
                g_scanner_error (scanner,
                                 "NULL or empty strings are not valid extension IDs.");
                if (name)
                  g_free (name);
                return FALSE;
              }

            if (! ligma_scanner_parse_token (scanner, G_TOKEN_LEFT_PAREN))
              {
                g_scanner_error (scanner,
                                 "Left paren expected after extension ID.");
                g_free (name);
                return FALSE;
              }
            if (! ligma_scanner_parse_identifier (scanner, "active"))
              {
                g_scanner_error (scanner,
                                 "Expected identifier \"active\" after extension ID.");
                g_free (name);
                return FALSE;
              }
            if (ligma_scanner_parse_boolean (scanner, &is_active))
              {
                if (is_active)
                  {
                    GList *list;

                    list = g_list_find_custom (manager->p->extensions, name,
                                               (GCompareFunc) ligma_extension_id_cmp);
                    if (! list)
                      list = g_list_find_custom (manager->p->sys_extensions, name,
                                                 (GCompareFunc) ligma_extension_id_cmp);
                    if (list)
                      {
                        GError *error = NULL;

                        if (ligma_extension_run (list->data, &error))
                          {
                            g_hash_table_insert (manager->p->running_extensions,
                                                 (gpointer) ligma_object_get_name (list->data),
                                                 list->data);
                          }
                        else
                          {
                            g_printerr ("Extension '%s' failed to run: %s\n",
                                        ligma_object_get_name (list->data),
                                        error->message);
                            g_error_free (error);
                          }
                      }
                  }

                /* Save the list of processed extension IDs, whether
                 * active or not.
                 */
                *processed = g_list_prepend (*processed, name);
              }
            else
              {
                g_scanner_error (scanner,
                                 "Expected boolean after \"active\" identifier.");
                g_free (name);
                return FALSE;
              }

            if (! ligma_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
              {
                g_scanner_error (scanner,
                                 "Right paren expected after \"active\" identifier.");
                return FALSE;
              }
          }
          token = G_TOKEN_RIGHT_PAREN;
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default: /* do nothing */
          break;
        }
    }

  return ligma_config_deserialize_return (scanner, token, nest_level);
}

static void
ligma_extension_manager_finalize (GObject *object)
{
  GList *iter;

  LigmaExtensionManager *manager = LIGMA_EXTENSION_MANAGER (object);

  g_list_free_full (manager->p->sys_extensions, g_object_unref);
  g_list_free_full (manager->p->extensions, g_object_unref);
  g_hash_table_unref (manager->p->running_extensions);

  for (iter = manager->p->uninstalled_extensions; iter; iter = iter->next)
    {
      /* Recursively delete folders of uninstalled extensions. */
      GError *error = NULL;
      GFile  *file;

      file = g_file_new_for_path (ligma_extension_get_path (iter->data));
      if (! ligma_file_delete_recursive (file, &error))
        g_warning ("%s: %s\n", G_STRFUNC, error->message);
      g_object_unref (file);
    }
  g_list_free_full (manager->p->uninstalled_extensions, g_object_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_extension_manager_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  LigmaExtensionManager *manager = LIGMA_EXTENSION_MANAGER (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      manager->p->ligma = g_value_get_object (value);
      break;
    case PROP_BRUSH_PATHS:
      manager->p->brush_paths = g_value_get_pointer (value);
      break;
    case PROP_DYNAMICS_PATHS:
      manager->p->dynamics_paths = g_value_get_pointer (value);
      break;
    case PROP_MYPAINT_BRUSH_PATHS:
      manager->p->mypaint_brush_paths = g_value_get_pointer (value);
      break;
    case PROP_PATTERN_PATHS:
      manager->p->pattern_paths = g_value_get_pointer (value);
      break;
    case PROP_GRADIENT_PATHS:
      manager->p->gradient_paths = g_value_get_pointer (value);
      break;
    case PROP_PALETTE_PATHS:
      manager->p->palette_paths = g_value_get_pointer (value);
      break;
    case PROP_TOOL_PRESET_PATHS:
      manager->p->tool_preset_paths = g_value_get_pointer (value);
      break;
    case PROP_SPLASH_PATHS:
      manager->p->splash_paths = g_value_get_pointer (value);
      break;
    case PROP_THEME_PATHS:
      manager->p->theme_paths = g_value_get_pointer (value);
      break;
    case PROP_PLUG_IN_PATHS:
      manager->p->plug_in_paths = g_value_get_pointer (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_extension_manager_get_property (GObject      *object,
                                     guint         property_id,
                                     GValue       *value,
                                     GParamSpec   *pspec)
{
  LigmaExtensionManager *manager = LIGMA_EXTENSION_MANAGER (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      g_value_set_object (value, manager->p->ligma);
      break;
    case PROP_BRUSH_PATHS:
      g_value_set_pointer (value, manager->p->brush_paths);
      break;
    case PROP_DYNAMICS_PATHS:
      g_value_set_pointer (value, manager->p->dynamics_paths);
      break;
    case PROP_MYPAINT_BRUSH_PATHS:
      g_value_set_pointer (value, manager->p->mypaint_brush_paths);
      break;
    case PROP_PATTERN_PATHS:
      g_value_set_pointer (value, manager->p->pattern_paths);
      break;
    case PROP_GRADIENT_PATHS:
      g_value_set_pointer (value, manager->p->gradient_paths);
      break;
    case PROP_PALETTE_PATHS:
      g_value_set_pointer (value, manager->p->palette_paths);
      break;
    case PROP_TOOL_PRESET_PATHS:
      g_value_set_pointer (value, manager->p->tool_preset_paths);
      break;
    case PROP_SPLASH_PATHS:
      g_value_set_pointer (value, manager->p->splash_paths);
      break;
    case PROP_THEME_PATHS:
      g_value_set_pointer (value, manager->p->theme_paths);
      break;
    case PROP_PLUG_IN_PATHS:
      g_value_set_pointer (value, manager->p->plug_in_paths);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/* Public functions. */

LigmaExtensionManager *
ligma_extension_manager_new (Ligma *ligma)
{
  LigmaExtensionManager *manager;

  manager = g_object_new (LIGMA_TYPE_EXTENSION_MANAGER,
                          "ligma", ligma,
                          NULL);

  return manager;
}

void
ligma_extension_manager_initialize (LigmaExtensionManager *manager)
{
  GFile  *file;
  GError *error = NULL;
  gchar  *path_str;
  GList  *path;
  GList  *list;
  GList  *processed_ids;

  g_return_if_fail (LIGMA_IS_EXTENSION_MANAGER (manager));

  /* List user-installed extensions. */
  path_str = ligma_config_build_writable_path ("extensions");
  path = ligma_config_path_expand_to_files (path_str, NULL);
  g_free (path_str);
  for (list = path; list; list = g_list_next (list))
    ligma_extension_manager_search_directory (manager, list->data, FALSE);
  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  /* List system extensions. */
  path_str = ligma_config_build_system_path ("extensions");
  path = ligma_config_path_expand_to_files (path_str, NULL);
  g_free (path_str);
  for (list = path; list; list = g_list_next (list))
    ligma_extension_manager_search_directory (manager, list->data, TRUE);
  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  /* Actually load the extensions. */
  if (manager->p->running_extensions)
    g_hash_table_unref (manager->p->running_extensions);
  manager->p->running_extensions = g_hash_table_new (g_str_hash, g_str_equal);

  file = ligma_directory_file ("extensionrc", NULL);

  processed_ids = NULL;
  if (g_file_query_exists (file, NULL))
    {
      if (manager->p->ligma->be_verbose)
        g_print ("Parsing '%s'\n", ligma_file_get_utf8_name (file));

      ligma_config_deserialize_file (LIGMA_CONFIG (manager),
                                    file, &processed_ids, &error);
      if (error)
        {
          g_printerr ("Failed to parse '%s': %s\n",
                      ligma_file_get_utf8_name (file),
                      error->message);
          g_error_free (error);
        }
    }
  g_object_unref (file);

  /* System extensions are on by default and user ones are off by
   * default */
  for (list = manager->p->extensions; list; list = list->next)
    {
      /* Directly flag user-installed extensions as processed. We do not
       * load them unless they were explicitly loaded (i.e. they must be
       * set in extensionrc.
       */
      if (! g_list_find_custom (processed_ids,
                                ligma_object_get_name (list->data),
                                (GCompareFunc) g_strcmp0))
        processed_ids = g_list_prepend (processed_ids,
                                        g_strdup (ligma_object_get_name (list->data)));
      g_signal_connect (list->data, "notify::running",
                        G_CALLBACK (ligma_extension_manager_extension_running),
                        manager);
    }
  for (list = manager->p->sys_extensions; list; list = g_list_next (list))
    {
      /* Unlike user-installed extensions, system extensions are loaded
       * by default if they were not set in the extensionrc (so that new
       * extensions installed with LIGMA updates get loaded) and if they
       * were not overridden by a user-installed extension (same ID).
       */
      if (! g_list_find_custom (processed_ids,
                                ligma_object_get_name (list->data),
                                (GCompareFunc) g_strcmp0))

        {
          error = NULL;
          if (ligma_extension_run (list->data, &error))
            {
              g_hash_table_insert (manager->p->running_extensions,
                                   (gpointer) ligma_object_get_name (list->data),
                                   list->data);
            }
          else
            {
              g_printerr ("Extension '%s' failed to run: %s\n",
                          ligma_object_get_name (list->data),
                          error->message);
              g_error_free (error);
            }
        }
      g_signal_connect (list->data, "notify::running",
                        G_CALLBACK (ligma_extension_manager_extension_running),
                        manager);
    }

  ligma_extension_manager_refresh (manager);
  g_list_free_full (processed_ids, g_free);
}

void
ligma_extension_manager_exit (LigmaExtensionManager *manager)
{
  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (LIGMA_IS_EXTENSION_MANAGER (manager));

  file = ligma_directory_file ("extensionrc", NULL);

  if (manager->p->ligma->be_verbose)
    g_print ("Writing '%s'\n", ligma_file_get_utf8_name (file));

  if (! ligma_config_serialize_to_file (LIGMA_CONFIG (manager),
                                       file,
                                       "LIGMA extensionrc",
                                       "end of extensionrc",
                                       NULL,
                                       &error))
    {
      ligma_message_literal (manager->p->ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }

  g_object_unref (file);
}

const GList *
ligma_extension_manager_get_system_extensions (LigmaExtensionManager *manager)
{
  return manager->p->sys_extensions;
}

const GList *
ligma_extension_manager_get_user_extensions (LigmaExtensionManager *manager)
{
  return manager->p->extensions;
}

/**
 * ligma_extension_manager_is_running:
 * @extension:
 *
 * Returns: %TRUE if @extension is ON.
 */
gboolean
ligma_extension_manager_is_running (LigmaExtensionManager *manager,
                                   LigmaExtension        *extension)
{
  LigmaExtension *ext;

  ext = g_hash_table_lookup (manager->p->running_extensions,
                             ligma_object_get_name (extension));

  return (ext && ext == extension);
}

/**
 * ligma_extension_manager_can_run:
 * @extension:
 *
 * Returns: %TRUE is @extension can be run.
 */
gboolean
ligma_extension_manager_can_run (LigmaExtensionManager *manager,
                                LigmaExtension        *extension)
{
  /* System extension overridden by another extension. */
  if (g_list_find (manager->p->sys_extensions, extension) &&
      g_list_find_custom (manager->p->extensions, extension,
                          (GCompareFunc) ligma_extension_cmp))
    return FALSE;

  /* TODO: should return FALSE if required LIGMA version or other
   * requirements are not filled as well.
   */

  return TRUE;
}

/**
 * ligma_extension_manager_is_removed:
 * @manager:
 * @extension:
 *
 * Returns: %TRUE is @extension was installed and has been removed
 * (hence ligma_extension_manager_undo_remove() can be used on it).
 */
gboolean
ligma_extension_manager_is_removed (LigmaExtensionManager *manager,
                                   LigmaExtension        *extension)
{
  GList *iter;

  g_return_val_if_fail (LIGMA_IS_EXTENSION_MANAGER (manager), FALSE);
  g_return_val_if_fail (LIGMA_IS_EXTENSION (extension), FALSE);

  iter = manager->p->uninstalled_extensions;
  for (; iter; iter = iter->next)
    if (ligma_extension_cmp (iter->data, extension) == 0)
      break;

  return (iter != NULL);
}

/**
 * ligma_extension_manager_install:
 * @manager:
 * @extension:
 * @error:
 *
 * Install @extension. This merely adds the extension in the known
 * extension list to make the manager aware of it at runtime, and to
 * emit a signal for GUI update.
 */
gboolean
ligma_extension_manager_install (LigmaExtensionManager *manager,
                                LigmaExtension        *extension,
                                GError              **error)
{
  gboolean success = FALSE;

  if ((success = ligma_extension_load (extension, error)))
    {
      manager->p->extensions = g_list_prepend (manager->p->extensions,
                                               extension);
      g_signal_connect (extension, "notify::running",
                        G_CALLBACK (ligma_extension_manager_extension_running),
                        manager);
      g_signal_emit (manager, signals[EXTENSION_INSTALLED], 0, extension, FALSE);
    }

  return success;
}

/**
 * ligma_extension_manager_remove:
 * @manager:
 * @extension:
 * @error:
 *
 * Uninstall @extension. Technically this only move the object to a
 * temporary list. The extension folder will be really deleted when LIGMA
 * will stop.
 * This allows to undo a deletion for as long as the session runs.
 */
gboolean
ligma_extension_manager_remove (LigmaExtensionManager  *manager,
                               LigmaExtension         *extension,
                               GError               **error)
{
  GList *iter;

  g_return_val_if_fail (LIGMA_IS_EXTENSION_MANAGER (manager), FALSE);
  g_return_val_if_fail (LIGMA_IS_EXTENSION (extension), FALSE);

  iter = (GList *) ligma_extension_manager_get_system_extensions (manager);
  for (; iter; iter = iter->next)
    if (iter->data == extension)
      {
        /* System extensions cannot be uninstalled. */
        if (error)
          *error = g_error_new (LIGMA_EXTENSION_ERROR,
                                LIGMA_EXTENSION_FAILED,
                                _("System extensions cannot be uninstalled."));
        return FALSE;
      }

  iter = (GList *) ligma_extension_manager_get_user_extensions (manager);
  for (; iter; iter = iter->next)
    if (ligma_extension_cmp (iter->data, extension) == 0)
      break;

  /* The extension has to be in the extension list. */
  g_return_val_if_fail (iter != NULL, FALSE);

  ligma_extension_stop (extension);

  manager->p->extensions = g_list_remove_link (manager->p->extensions,
                                               iter);
  manager->p->uninstalled_extensions = g_list_concat (manager->p->uninstalled_extensions,
                                                      iter);
  g_signal_emit (manager, signals[EXTENSION_REMOVED], 0,
                 ligma_object_get_name (extension));

  return TRUE;
}

gboolean
ligma_extension_manager_undo_remove (LigmaExtensionManager *manager,
                                    LigmaExtension        *extension,
                                    GError              **error)
{
  GList *iter;

  g_return_val_if_fail (LIGMA_IS_EXTENSION_MANAGER (manager), FALSE);
  g_return_val_if_fail (LIGMA_IS_EXTENSION (extension), FALSE);

  iter = manager->p->uninstalled_extensions;
  for (; iter; iter = iter->next)
    if (ligma_extension_cmp (iter->data, extension) == 0)
      break;

  /* The extension has to be in the uninstalled extension list. */
  g_return_val_if_fail (iter != NULL, FALSE);

  manager->p->uninstalled_extensions = g_list_remove (manager->p->uninstalled_extensions,
                                                      extension);
  ligma_extension_manager_install (manager, extension, error);

  return TRUE;
}

/* Private functions. */

static void
ligma_extension_manager_serialize_extension (LigmaExtensionManager *manager,
                                            LigmaExtension        *extension,
                                            LigmaConfigWriter     *writer)
{
  const gchar *name = ligma_object_get_name (extension);

  g_return_if_fail (name != NULL);

  ligma_config_writer_open (writer, g_type_name (G_TYPE_FROM_INSTANCE (extension)));
  ligma_config_writer_string (writer, name);

  /* The extensionrc does not need to save any information about an
   * extension. This is all saved in the metadata. The only thing we
   * care to store is the state of extensions, i.e. which one is loaded
   * or not (since you can install an extension but leave it unloaded),
   * named by its unique ID.
   * As a consequence, LigmaExtension does not need to implement
   * LigmaConfigInterface.
   */
  ligma_config_writer_open (writer, "active");
  if (g_hash_table_contains (manager->p->running_extensions, name))
    ligma_config_writer_identifier (writer, "yes");
  else
    ligma_config_writer_identifier (writer, "no");
  ligma_config_writer_close (writer);

  ligma_config_writer_close (writer);
}

static void
ligma_extension_manager_refresh (LigmaExtensionManager *manager)
{
  GHashTableIter iter;
  gpointer       key;
  gpointer       value;
  GList         *brush_paths         = NULL;
  GList         *dynamics_paths      = NULL;
  GList         *mypaint_brush_paths = NULL;
  GList         *pattern_paths       = NULL;
  GList         *gradient_paths      = NULL;
  GList         *palette_paths       = NULL;
  GList         *tool_preset_paths   = NULL;
  GList         *splash_paths        = NULL;
  GList         *theme_paths         = NULL;
  GList         *plug_in_paths       = NULL;

  g_hash_table_iter_init (&iter, manager->p->running_extensions);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      LigmaExtension *extension = value;
      GList         *new_paths;

      new_paths = g_list_copy_deep (ligma_extension_get_brush_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      brush_paths = g_list_concat (brush_paths, new_paths);

      new_paths = g_list_copy_deep (ligma_extension_get_dynamics_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      dynamics_paths = g_list_concat (dynamics_paths, new_paths);

      new_paths = g_list_copy_deep (ligma_extension_get_mypaint_brush_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      mypaint_brush_paths = g_list_concat (mypaint_brush_paths, new_paths);

      new_paths = g_list_copy_deep (ligma_extension_get_pattern_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      pattern_paths = g_list_concat (pattern_paths, new_paths);

      new_paths = g_list_copy_deep (ligma_extension_get_gradient_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      gradient_paths = g_list_concat (gradient_paths, new_paths);

      new_paths = g_list_copy_deep (ligma_extension_get_palette_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      palette_paths = g_list_concat (palette_paths, new_paths);

      new_paths = g_list_copy_deep (ligma_extension_get_tool_preset_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      tool_preset_paths = g_list_concat (tool_preset_paths, new_paths);

      new_paths = g_list_copy_deep (ligma_extension_get_splash_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      splash_paths = g_list_concat (splash_paths, new_paths);

      new_paths = g_list_copy_deep (ligma_extension_get_theme_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      theme_paths = g_list_concat (theme_paths, new_paths);

      new_paths = g_list_copy_deep (ligma_extension_get_plug_in_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      plug_in_paths = g_list_concat (plug_in_paths, new_paths);
    }

  g_object_set (manager,
                "brush-paths",         brush_paths,
                "dynamics-paths",      dynamics_paths,
                "mypaint-brush-paths", brush_paths,
                "pattern-paths",       pattern_paths,
                "gradient-paths",      gradient_paths,
                "palette-paths",       palette_paths,
                "tool-preset-paths",   tool_preset_paths,
                "plug-in-paths",       plug_in_paths,
                "splash-paths",        splash_paths,
                "theme-paths",         theme_paths,
                NULL);
}

static void
ligma_extension_manager_search_directory (LigmaExtensionManager *manager,
                                         GFile                *directory,
                                         gboolean              system_dir)
{
  GFileEnumerator *enumerator;

  enumerator = g_file_enumerate_children (directory,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                          G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                          G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                          G_FILE_QUERY_INFO_NONE,
                                          NULL, NULL);
  if (enumerator)
    {
      GFileInfo *info;

      while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)))
        {
          GFile *subdir;

          if (g_file_info_get_is_hidden (info))
            {
              g_object_unref (info);
              continue;
            }

          subdir = g_file_enumerator_get_child (enumerator, info);

          if (g_file_query_file_type (subdir,
                                      G_FILE_QUERY_INFO_NONE,
                                      NULL) == G_FILE_TYPE_DIRECTORY)
            {
              LigmaExtension *extension;
              GError        *error = NULL;

              extension = ligma_extension_new (g_file_peek_path (subdir),
                                              ! system_dir);

              if (ligma_extension_load (extension, &error))
                {
                  if (system_dir)
                    manager->p->sys_extensions = g_list_prepend (manager->p->sys_extensions,
                                                                 extension);
                  else
                    manager->p->extensions = g_list_prepend (manager->p->extensions,
                                                             extension);
                }
              else
                {
                  g_object_unref (extension);
                  if (error)
                    {
                      g_printerr (_("Skipping extension '%s': %s\n"),
                                  g_file_peek_path (subdir), error->message);
                      g_error_free (error);
                    }
                }
            }
          else
            {
              g_printerr (_("Skipping unknown file '%s' in extension directory.\n"),
                          g_file_peek_path (subdir));
            }

          g_object_unref (subdir);
          g_object_unref (info);
        }

      g_object_unref (enumerator);
    }
}

static void
ligma_extension_manager_extension_running (LigmaExtension        *extension,
                                          GParamSpec           *pspec,
                                          LigmaExtensionManager *manager)
{
  gboolean running;

  g_object_get (extension,
                "running", &running,
                NULL);
  if (running)
    g_hash_table_insert (manager->p->running_extensions,
                         (gpointer) ligma_object_get_name (extension),
                         extension);
  else
    g_hash_table_remove (manager->p->running_extensions,
                         (gpointer) ligma_object_get_name (extension));

  ligma_extension_manager_refresh (manager);
}
