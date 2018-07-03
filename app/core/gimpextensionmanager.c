/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * gimpextensionmanager.c
 * Copyright (C) 2018 Jehan <jehan@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimpextension.h"
#include "gimpobject.h"
#include "gimpmarshal.h"

#include "gimpextensionmanager.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_GIMP,
  PROP_BRUSH_PATHS,
  PROP_DYNAMICS_PATHS,
  PROP_MYPAINT_BRUSH_PATHS,
  PROP_PATTERN_PATHS,
  PROP_GRADIENT_PATHS,
  PROP_PALETTE_PATHS,
  PROP_TOOL_PRESET_PATHS,
  PROP_PLUG_IN_PATHS,
};

struct _GimpExtensionManagerPrivate
{
  Gimp       *gimp;

  /* Installed system (read-only) extensions. */
  GList      *sys_extensions;
  /* Self-installed (read-write) extensions. */
  GList      *extensions;

  /* Running extensions */
  GHashTable *active_extensions;

  /* Metadata properties */
  GList      *brush_paths;
  GList      *dynamics_paths;
  GList      *mypaint_brush_paths;
  GList      *pattern_paths;
  GList      *gradient_paths;
  GList      *palette_paths;
  GList      *tool_preset_paths;
  GList      *plug_in_paths;
};

static void     gimp_extension_manager_finalize     (GObject    *object);
static void     gimp_extension_manager_set_property (GObject      *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void     gimp_extension_manager_get_property (GObject      *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);

static void     gimp_extension_manager_refresh          (GimpExtensionManager *manager);
static void     gimp_extension_manager_search_directory (GimpExtensionManager *manager,
                                                         GFile                *directory,
                                                         gboolean              system_dir);

G_DEFINE_TYPE (GimpExtensionManager, gimp_extension_manager, GIMP_TYPE_OBJECT)

#define parent_class gimp_extension_manager_parent_class

static void
gimp_extension_manager_class_init (GimpExtensionManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_extension_manager_finalize;
  object_class->set_property = gimp_extension_manager_set_property;
  object_class->get_property = gimp_extension_manager_get_property;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp", NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_BRUSH_PATHS,
                                   g_param_spec_pointer ("brush-paths",
                                                         NULL, NULL,
                                                         GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_DYNAMICS_PATHS,
                                   g_param_spec_pointer ("dynamics-paths",
                                                         NULL, NULL,
                                                         GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_MYPAINT_BRUSH_PATHS,
                                   g_param_spec_pointer ("mypaint-brush-paths",
                                                         NULL, NULL,
                                                         GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PATTERN_PATHS,
                                   g_param_spec_pointer ("pattern-paths",
                                                         NULL, NULL,
                                                         GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_GRADIENT_PATHS,
                                   g_param_spec_pointer ("gradient-paths",
                                                         NULL, NULL,
                                                         GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PALETTE_PATHS,
                                   g_param_spec_pointer ("palette-paths",
                                                         NULL, NULL,
                                                         GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_TOOL_PRESET_PATHS,
                                   g_param_spec_pointer ("tool-preset-paths",
                                                         NULL, NULL,
                                                         GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PLUG_IN_PATHS,
                                   g_param_spec_pointer ("plug-in-paths",
                                                         NULL, NULL,
                                                         GIMP_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (GimpExtensionManagerPrivate));
}

static void
gimp_extension_manager_init (GimpExtensionManager *manager)
{
  manager->p = G_TYPE_INSTANCE_GET_PRIVATE (manager,
                                            GIMP_TYPE_EXTENSION_MANAGER,
                                            GimpExtensionManagerPrivate);
}

static void
gimp_extension_manager_finalize (GObject *object)
{
  GimpExtensionManager *manager = GIMP_EXTENSION_MANAGER (object);

  g_list_free_full (manager->p->sys_extensions, g_object_unref);
  g_list_free_full (manager->p->extensions, g_object_unref);
  g_hash_table_unref (manager->p->active_extensions);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_extension_manager_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpExtensionManager *manager = GIMP_EXTENSION_MANAGER (object);

  switch (property_id)
    {
    case PROP_GIMP:
      manager->p->gimp = g_value_get_object (value);
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
    case PROP_PLUG_IN_PATHS:
      manager->p->plug_in_paths = g_value_get_pointer (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_extension_manager_get_property (GObject      *object,
                                     guint         property_id,
                                     GValue       *value,
                                     GParamSpec   *pspec)
{
  GimpExtensionManager *manager = GIMP_EXTENSION_MANAGER (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, manager->p->gimp);
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
    case PROP_PLUG_IN_PATHS:
      g_value_set_pointer (value, manager->p->plug_in_paths);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GimpExtensionManager *
gimp_extension_manager_new (Gimp *gimp)
{
  GimpExtensionManager *manager;

  manager = g_object_new (GIMP_TYPE_EXTENSION_MANAGER,
                          "gimp", gimp,
                          NULL);

  return manager;
}

void
gimp_extension_manager_initialize (GimpExtensionManager *manager)
{
  gchar *path_str;
  GList *path;
  GList *list;

  /* List user-installed extensions. */
  path_str = gimp_config_build_writable_path ("extensions");
  path = gimp_config_path_expand_to_files (path_str, NULL);
  for (list = path; list; list = g_list_next (list))
    gimp_extension_manager_search_directory (manager, list->data, FALSE);
  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  /* List system extensions. */
  path_str = gimp_config_build_system_path ("extensions");
  path = gimp_config_path_expand_to_files (path_str, NULL);
  for (list = path; list; list = g_list_next (list))
    gimp_extension_manager_search_directory (manager, list->data, TRUE);
  g_list_free_full (path, (GDestroyNotify) g_object_unref);

  /* Actually load the extensions. */
  if (manager->p->active_extensions)
    g_hash_table_unref (manager->p->active_extensions);
  manager->p->active_extensions = g_hash_table_new (g_str_hash,
                                                    g_str_equal);
  for (list = manager->p->extensions; list; list = g_list_next (list))
    {
      if (! g_hash_table_contains (manager->p->active_extensions,
                                   gimp_object_get_name (list->data)))
        {
          GError *error = NULL;

          g_hash_table_insert (manager->p->active_extensions,
                               (gpointer) gimp_object_get_name (list->data),
                               list->data);

          if (! gimp_extension_run (list->data, &error))
            {
              g_hash_table_remove (manager->p->active_extensions, list->data);
              if (error)
                g_printerr ("Extension '%s' failed to run: %s\n",
                            gimp_object_get_name (list->data),
                            error->message);
            }
        }
    }
  for (list = manager->p->sys_extensions; list; list = g_list_next (list))
    {
      if (! g_hash_table_contains (manager->p->active_extensions,
                                   gimp_object_get_name (list->data)))
        {
          GError *error = NULL;

          g_hash_table_insert (manager->p->active_extensions,
                               (gpointer) gimp_object_get_name (list->data),
                               list->data);

          if (! gimp_extension_run (list->data, &error))
            {
              g_hash_table_remove (manager->p->active_extensions, list->data);
              if (error)
                g_printerr ("Extension '%s' failed to run: %s\n",
                            gimp_object_get_name (list->data),
                            error->message);
            }
        }
    }

  gimp_extension_manager_refresh (manager);
}

static void
gimp_extension_manager_refresh (GimpExtensionManager *manager)
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
  GList         *plug_in_paths       = NULL;

  g_hash_table_iter_init (&iter, manager->p->active_extensions);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      GimpExtension *extension = value;
      GList         *new_paths;

      new_paths = g_list_copy_deep (gimp_extension_get_brush_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      brush_paths = g_list_concat (brush_paths, new_paths);

      new_paths = g_list_copy_deep (gimp_extension_get_dynamics_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      dynamics_paths = g_list_concat (dynamics_paths, new_paths);

      new_paths = g_list_copy_deep (gimp_extension_get_mypaint_brush_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      mypaint_brush_paths = g_list_concat (mypaint_brush_paths, new_paths);

      new_paths = g_list_copy_deep (gimp_extension_get_pattern_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      pattern_paths = g_list_concat (pattern_paths, new_paths);

      new_paths = g_list_copy_deep (gimp_extension_get_gradient_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      gradient_paths = g_list_concat (gradient_paths, new_paths);

      new_paths = g_list_copy_deep (gimp_extension_get_palette_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      palette_paths = g_list_concat (palette_paths, new_paths);

      new_paths = g_list_copy_deep (gimp_extension_get_tool_preset_paths (extension),
                                    (GCopyFunc) g_object_ref, NULL);
      tool_preset_paths = g_list_concat (tool_preset_paths, new_paths);

      new_paths = g_list_copy_deep (gimp_extension_get_plug_in_paths (extension),
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
                NULL);
}

static void
gimp_extension_manager_search_directory (GimpExtensionManager *manager,
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
              GimpExtension *extension;
              GError        *error = NULL;

              extension = gimp_extension_new (g_file_peek_path (subdir),
                                              ! system_dir);

              if (gimp_extension_load (extension, &error))
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
                    g_printerr (_("Skipping extension '%s': %s\n"),
                                g_file_peek_path (subdir), error->message);
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
