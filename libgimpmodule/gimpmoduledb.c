/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gio/gio.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "ligmamoduletypes.h"

#include "ligmamodule.h"
#include "ligmamoduledb.h"

#include "libligma/libligma-intl.h"


/**
 * SECTION: ligmamoduledb
 * @title: LigmaModuleDB
 * @short_description: Keeps a list of #LigmaModule's found in a given
 *                     searchpath.
 *
 * Keeps a list of #LigmaModule's found in a given searchpath.
 **/


enum
{
  ADD,
  REMOVE,
  MODULE_MODIFIED,
  LAST_SIGNAL
};


struct _LigmaModuleDBPrivate
{
  GList    *modules;

  gchar    *load_inhibit;
  gboolean  verbose;
};


static void         ligma_module_db_finalize            (GObject      *object);

static void         ligma_module_db_load_directory      (LigmaModuleDB *db,
                                                        GFile        *directory);
static void         ligma_module_db_load_module         (LigmaModuleDB *db,
                                                        GFile        *file);

static LigmaModule * ligma_module_db_module_find_by_file (LigmaModuleDB *db,
                                                        GFile        *file);

static void         ligma_module_db_module_dump_func    (gpointer      data,
                                                        gpointer      user_data);

static void         ligma_module_db_module_modified     (LigmaModule   *module,
                                                        LigmaModuleDB *db);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaModuleDB, ligma_module_db, G_TYPE_OBJECT)

#define parent_class ligma_module_db_parent_class

static guint db_signals[LAST_SIGNAL] = { 0 };


static void
ligma_module_db_class_init (LigmaModuleDBClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  db_signals[ADD] =
    g_signal_new ("add",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaModuleDBClass, add),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_MODULE);

  db_signals[REMOVE] =
    g_signal_new ("remove",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaModuleDBClass, remove),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_MODULE);

  db_signals[MODULE_MODIFIED] =
    g_signal_new ("module-modified",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaModuleDBClass, module_modified),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_MODULE);

  object_class->finalize = ligma_module_db_finalize;

  klass->add             = NULL;
  klass->remove          = NULL;
}

static void
ligma_module_db_init (LigmaModuleDB *db)
{
  db->priv = ligma_module_db_get_instance_private (db);

  db->priv->modules      = NULL;
  db->priv->load_inhibit = NULL;
  db->priv->verbose      = FALSE;
}

static void
ligma_module_db_finalize (GObject *object)
{
  LigmaModuleDB *db = LIGMA_MODULE_DB (object);
  GList        *list;

  for (list = db->priv->modules; list; list = g_list_next (list))
    {
      g_signal_handlers_disconnect_by_func (list->data,
                                            ligma_module_db_module_modified,
                                            object);
    }

  g_clear_pointer (&db->priv->modules,      g_list_free);
  g_clear_pointer (&db->priv->load_inhibit, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * ligma_module_db_new:
 * @verbose: Pass %TRUE to enable debugging output.
 *
 * Creates a new #LigmaModuleDB instance. The @verbose parameter will be
 * passed to the created #LigmaModule instances using ligma_module_new().
 *
 * Returns: The new #LigmaModuleDB instance.
 **/
LigmaModuleDB *
ligma_module_db_new (gboolean verbose)
{
  LigmaModuleDB *db;

  db = g_object_new (LIGMA_TYPE_MODULE_DB, NULL);

  db->priv->verbose = verbose ? TRUE : FALSE;

  return db;
}

/**
 * ligma_module_db_get_modules:
 * @db: A #LigmaModuleDB.
 *
 * Returns a #GList of the modules kept by @db. The list must not be
 * modified or freed.
 *
 * Returns: (element-type LigmaModule) (transfer none): a #GList
 * of #LigmaModule instances.
 *
 * Since: 3.0
 **/
GList *
ligma_module_db_get_modules (LigmaModuleDB *db)
{
  g_return_val_if_fail (LIGMA_IS_MODULE_DB (db), NULL);

  return db->priv->modules;
}

/**
 * ligma_module_db_set_verbose:
 * @db:      A #LigmaModuleDB.
 * @verbose: the new 'verbose' setting
 *
 * Sets the 'verbose' setting of @db.
 *
 * Since: 3.0
 **/
void
ligma_module_db_set_verbose (LigmaModuleDB *db,
                            gboolean      verbose)
{
  g_return_if_fail (LIGMA_IS_MODULE_DB (db));

  db->priv->verbose = verbose ? TRUE : FALSE;
}

/**
 * ligma_module_db_get_verbose:
 * @db: A #LigmaModuleDB.
 *
 * Returns the 'verbose' setting of @db.
 *
 * Returns: the 'verbose' setting.
 *
 * Since: 3.0
 **/
gboolean
ligma_module_db_get_verbose (LigmaModuleDB *db)
{
  g_return_val_if_fail (LIGMA_IS_MODULE_DB (db), FALSE);

  return db->priv->verbose;
}

static gboolean
is_in_inhibit_list (GFile       *file,
                    const gchar *inhibit_list)
{
  gchar       *filename;
  gchar       *p;
  gint         pathlen;
  const gchar *start;
  const gchar *end;

  if (! inhibit_list || ! strlen (inhibit_list))
    return FALSE;

  filename = g_file_get_path (file);

  p = strstr (inhibit_list, filename);
  if (! p)
    {
      g_free (filename);
      return FALSE;
    }

  /* we have a substring, but check for colons either side */
  start = p;
  while (start != inhibit_list && *start != G_SEARCHPATH_SEPARATOR)
    start--;

  if (*start == G_SEARCHPATH_SEPARATOR)
    start++;

  end = strchr (p, G_SEARCHPATH_SEPARATOR);
  if (! end)
    end = inhibit_list + strlen (inhibit_list);

  pathlen = strlen (filename);

  g_free (filename);

  if ((end - start) == pathlen)
    return TRUE;

  return FALSE;
}

/**
 * ligma_module_db_set_load_inhibit:
 * @db:           A #LigmaModuleDB.
 * @load_inhibit: A #G_SEARCHPATH_SEPARATOR delimited list of module
 *                filenames to exclude from auto-loading.
 *
 * Sets the @load_inhibit flag for all #LigmaModule's which are kept
 * by @db (using ligma_module_set_load_inhibit()).
 **/
void
ligma_module_db_set_load_inhibit (LigmaModuleDB *db,
                                 const gchar  *load_inhibit)
{
  GList *list;

  g_return_if_fail (LIGMA_IS_MODULE_DB (db));

  if (db->priv->load_inhibit)
    g_free (db->priv->load_inhibit);

  db->priv->load_inhibit = g_strdup (load_inhibit);

  for (list = db->priv->modules; list; list = g_list_next (list))
    {
      LigmaModule *module = list->data;
      gboolean    inhibit;

      inhibit =is_in_inhibit_list (ligma_module_get_file (module),
                                   load_inhibit);

      ligma_module_set_auto_load (module, ! inhibit);
    }
}

/**
 * ligma_module_db_get_load_inhibit:
 * @db: A #LigmaModuleDB.
 *
 * Return the #G_SEARCHPATH_SEPARATOR delimited list of module filenames
 * which are excluded from auto-loading.
 *
 * Returns: the @db's @load_inhibit string.
 **/
const gchar *
ligma_module_db_get_load_inhibit (LigmaModuleDB *db)
{
  g_return_val_if_fail (LIGMA_IS_MODULE_DB (db), NULL);

  return db->priv->load_inhibit;
}

/**
 * ligma_module_db_load:
 * @db:          A #LigmaModuleDB.
 * @module_path: A #G_SEARCHPATH_SEPARATOR delimited list of directories
 *               to load modules from.
 *
 * Scans the directories contained in @module_path and creates a
 * #LigmaModule instance for every loadable module contained in the
 * directories.
 **/
void
ligma_module_db_load (LigmaModuleDB *db,
                     const gchar  *module_path)
{
  g_return_if_fail (LIGMA_IS_MODULE_DB (db));
  g_return_if_fail (module_path != NULL);

  if (g_module_supported ())
    {
      GList *path;
      GList *list;

      path = ligma_config_path_expand_to_files (module_path, NULL);

      for (list = path; list; list = g_list_next (list))
        {
          ligma_module_db_load_directory (db, list->data);
        }

      g_list_free_full (path, (GDestroyNotify) g_object_unref);
    }

  if (FALSE)
    g_list_foreach (db->priv->modules, ligma_module_db_module_dump_func, NULL);
}

/**
 * ligma_module_db_refresh:
 * @db:          A #LigmaModuleDB.
 * @module_path: A #G_SEARCHPATH_SEPARATOR delimited list of directories
 *               to load modules from.
 *
 * Does the same as ligma_module_db_load(), plus removes all #LigmaModule
 * instances whose modules have been deleted from disk.
 *
 * Note that the #LigmaModule's will just be removed from the internal
 * list and not freed as this is not possible with #GTypeModule
 * instances which actually implement types.
 **/
void
ligma_module_db_refresh (LigmaModuleDB *db,
                        const gchar  *module_path)
{
  GList *list;

  g_return_if_fail (LIGMA_IS_MODULE_DB (db));
  g_return_if_fail (module_path != NULL);

  list = db->priv->modules;

  while (list)
    {
      LigmaModule *module = list->data;

      list = g_list_next (list);

      if (! ligma_module_is_on_disk (module) &&
          ! ligma_module_is_loaded (module))
        {
          g_signal_handlers_disconnect_by_func (module,
                                                ligma_module_db_module_modified,
                                                db);

          db->priv->modules = g_list_remove (db->priv->modules, module);

          g_signal_emit (db, db_signals[REMOVE], 0, module);
        }
    }

  /* walk filesystem and add new things we find */
  ligma_module_db_load (db, module_path);
}

static void
ligma_module_db_load_directory (LigmaModuleDB *db,
                               GFile        *directory)
{
  GFileEnumerator *enumerator;

  enumerator = g_file_enumerate_children (directory,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                          G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                          G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                          G_FILE_QUERY_INFO_NONE,
                                          NULL, NULL);

  if (enumerator)
    {
      GFileInfo *info;

      while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)))
        {
          GFileType file_type = g_file_info_get_file_type (info);

         if (file_type == G_FILE_TYPE_REGULAR &&
             ! g_file_info_get_is_hidden (info))
            {
              GFile *child = g_file_enumerator_get_child (enumerator, info);

              ligma_module_db_load_module (db, child);

              g_object_unref (child);
            }

          g_object_unref (info);
        }

      g_object_unref (enumerator);
    }
}

static void
ligma_module_db_load_module (LigmaModuleDB *db,
                            GFile        *file)
{
  LigmaModule *module;
  gboolean   load_inhibit;

  if (! ligma_file_has_extension (file, "." G_MODULE_SUFFIX))
    return;

  /* don't load if we already know about it */
  if (ligma_module_db_module_find_by_file (db, file))
    return;

  load_inhibit = is_in_inhibit_list (file, db->priv->load_inhibit);

  module = ligma_module_new (file,
                            ! load_inhibit,
                            db->priv->verbose);

  g_signal_connect (module, "modified",
                    G_CALLBACK (ligma_module_db_module_modified),
                    db);

  db->priv->modules = g_list_append (db->priv->modules, module);

  g_signal_emit (db, db_signals[ADD], 0, module);
}

static LigmaModule *
ligma_module_db_module_find_by_file (LigmaModuleDB *db,
                                    GFile        *file)
{
  GList *list;

  for (list = db->priv->modules; list; list = g_list_next (list))
    {
      LigmaModule *module = list->data;

      if (g_file_equal (file, ligma_module_get_file (module)))
        return module;
    }

  return NULL;
}

static void
ligma_module_db_module_dump_func (gpointer data,
                                 gpointer user_data)
{
  static const gchar * const statenames[] =
  {
    N_("Module error"),
    N_("Loaded"),
    N_("Load failed"),
    N_("Not loaded")
  };

  LigmaModule           *module = data;
  const LigmaModuleInfo *info   = ligma_module_get_info (module);

  g_print ("\n%s: %s\n",
           ligma_file_get_utf8_name (ligma_module_get_file (module)),
           gettext (statenames[ligma_module_get_state (module)]));

#if 0
  g_print ("  module: %p  lasterr: %s  query: %p register: %p\n",
           module->module,
           module->last_module_error ? module->last_module_error : "NONE",
           module->query_module,
           module->register_module);
#endif

  if (info)
    {
      g_print ("  purpose:   %s\n"
               "  author:    %s\n"
               "  version:   %s\n"
               "  copyright: %s\n"
               "  date:      %s\n",
               info->purpose   ? info->purpose   : "NONE",
               info->author    ? info->author    : "NONE",
               info->version   ? info->version   : "NONE",
               info->copyright ? info->copyright : "NONE",
               info->date      ? info->date      : "NONE");
    }
}

static void
ligma_module_db_module_modified (LigmaModule   *module,
                                LigmaModuleDB *db)
{
  g_signal_emit (db, db_signals[MODULE_MODIFIED], 0, module);
}
