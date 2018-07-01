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
  PROP_GIMP
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
          g_hash_table_insert (manager->p->active_extensions,
                               (gpointer) gimp_object_get_name (list->data),
                               list->data);
          /* TODO: do whatever is needed to make extension "active". */

        }
    }
  for (list = manager->p->sys_extensions; list; list = g_list_next (list))
    {
      if (! g_hash_table_contains (manager->p->active_extensions,
                                   gimp_object_get_name (list->data)))
        {
          g_hash_table_insert (manager->p->active_extensions,
                               (gpointer) gimp_object_get_name (list->data),
                               list->data);
          /* TODO: do whatever is needed to make extension "active". */
        }
    }
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
