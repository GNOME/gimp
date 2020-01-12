/* LIBGIMP - The GIMP Library
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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "gimpmoduletypes.h"

#include "gimpmodule.h"
#include "gimpmoduledb.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpmoduledb
 * @title: GimpModuleDB
 * @short_description: Keeps a list of #GimpModule's found in a given
 *                     searchpath.
 *
 * Keeps a list of #GimpModule's found in a given searchpath.
 **/


enum
{
  ADD,
  REMOVE,
  MODULE_MODIFIED,
  LAST_SIGNAL
};


struct _GimpModuleDBPrivate
{
  GList    *modules;

  gchar    *load_inhibit;
  gboolean  verbose;
};


static void         gimp_module_db_finalize            (GObject      *object);

static void         gimp_module_db_load_directory      (GimpModuleDB *db,
                                                        GFile        *directory);
static void         gimp_module_db_load_module         (GimpModuleDB *db,
                                                        GFile        *file);

static GimpModule * gimp_module_db_module_find_by_file (GimpModuleDB *db,
                                                        GFile        *file);

static void         gimp_module_db_module_dump_func    (gpointer      data,
                                                        gpointer      user_data);

static void         gimp_module_db_module_modified     (GimpModule   *module,
                                                        GimpModuleDB *db);


G_DEFINE_TYPE_WITH_PRIVATE (GimpModuleDB, gimp_module_db, G_TYPE_OBJECT)

#define parent_class gimp_module_db_parent_class

static guint db_signals[LAST_SIGNAL] = { 0 };


static void
gimp_module_db_class_init (GimpModuleDBClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  db_signals[ADD] =
    g_signal_new ("add",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpModuleDBClass, add),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_MODULE);

  db_signals[REMOVE] =
    g_signal_new ("remove",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpModuleDBClass, remove),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_MODULE);

  db_signals[MODULE_MODIFIED] =
    g_signal_new ("module-modified",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpModuleDBClass, module_modified),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_MODULE);

  object_class->finalize = gimp_module_db_finalize;

  klass->add             = NULL;
  klass->remove          = NULL;
}

static void
gimp_module_db_init (GimpModuleDB *db)
{
  db->priv = gimp_module_db_get_instance_private (db);

  db->priv->modules      = NULL;
  db->priv->load_inhibit = NULL;
  db->priv->verbose      = FALSE;
}

static void
gimp_module_db_finalize (GObject *object)
{
  GimpModuleDB *db = GIMP_MODULE_DB (object);
  GList        *list;

  for (list = db->priv->modules; list; list = g_list_next (list))
    {
      g_signal_handlers_disconnect_by_func (list->data,
                                            gimp_module_db_module_modified,
                                            object);
    }

  g_clear_pointer (&db->priv->modules,      g_list_free);
  g_clear_pointer (&db->priv->load_inhibit, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gimp_module_db_new:
 * @verbose: Pass %TRUE to enable debugging output.
 *
 * Creates a new #GimpModuleDB instance. The @verbose parameter will be
 * passed to the created #GimpModule instances using gimp_module_new().
 *
 * Returns: The new #GimpModuleDB instance.
 **/
GimpModuleDB *
gimp_module_db_new (gboolean verbose)
{
  GimpModuleDB *db;

  db = g_object_new (GIMP_TYPE_MODULE_DB, NULL);

  db->priv->verbose = verbose ? TRUE : FALSE;

  return db;
}

/**
 * gimp_module_db_get_modules:
 * @db: A #GimpModuleDB.
 *
 * Returns a #GList of the modules kept by @db. The list must not be
 * modified or freed.
 *
 * Returns: (element-type GimpModule) (transfer none): a #GList
 * of #GimpModule instances.
 *
 * Since: 3.0
 **/
GList *
gimp_module_db_get_modules (GimpModuleDB *db)
{
  g_return_val_if_fail (GIMP_IS_MODULE_DB (db), NULL);

  return db->priv->modules;
}

/**
 * gimp_module_db_set_verbose:
 * @db:      A #GimpModuleDB.
 * @verbose: the new 'verbose' setting
 *
 * Sets the 'verbose' setting of @db.
 *
 * Since: 3.0
 **/
void
gimp_module_db_set_verbose (GimpModuleDB *db,
                            gboolean      verbose)
{
  g_return_if_fail (GIMP_IS_MODULE_DB (db));

  db->priv->verbose = verbose ? TRUE : FALSE;
}

/**
 * gimp_module_db_get_verbose:
 * @db: A #GimpModuleDB.
 *
 * Returns the 'verbose' setting of @db.
 *
 * Returns: the 'verbose' setting.
 *
 * Since: 3.0
 **/
gboolean
gimp_module_db_get_verbose (GimpModuleDB *db)
{
  g_return_val_if_fail (GIMP_IS_MODULE_DB (db), FALSE);

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
 * gimp_module_db_set_load_inhibit:
 * @db:           A #GimpModuleDB.
 * @load_inhibit: A #G_SEARCHPATH_SEPARATOR delimited list of module
 *                filenames to exclude from auto-loading.
 *
 * Sets the @load_inhibit flag for all #GimpModule's which are kept
 * by @db (using gimp_module_set_load_inhibit()).
 **/
void
gimp_module_db_set_load_inhibit (GimpModuleDB *db,
                                 const gchar  *load_inhibit)
{
  GList *list;

  g_return_if_fail (GIMP_IS_MODULE_DB (db));

  if (db->priv->load_inhibit)
    g_free (db->priv->load_inhibit);

  db->priv->load_inhibit = g_strdup (load_inhibit);

  for (list = db->priv->modules; list; list = g_list_next (list))
    {
      GimpModule *module = list->data;
      gboolean    inhibit;

      inhibit =is_in_inhibit_list (gimp_module_get_file (module),
                                   load_inhibit);

      gimp_module_set_auto_load (module, ! inhibit);
    }
}

/**
 * gimp_module_db_get_load_inhibit:
 * @db: A #GimpModuleDB.
 *
 * Return the #G_SEARCHPATH_SEPARATOR delimited list of module filenames
 * which are excluded from auto-loading.
 *
 * Returns: the @db's @load_inhibit string.
 **/
const gchar *
gimp_module_db_get_load_inhibit (GimpModuleDB *db)
{
  g_return_val_if_fail (GIMP_IS_MODULE_DB (db), NULL);

  return db->priv->load_inhibit;
}

/**
 * gimp_module_db_load:
 * @db:          A #GimpModuleDB.
 * @module_path: A #G_SEARCHPATH_SEPARATOR delimited list of directories
 *               to load modules from.
 *
 * Scans the directories contained in @module_path and creates a
 * #GimpModule instance for every loadable module contained in the
 * directories.
 **/
void
gimp_module_db_load (GimpModuleDB *db,
                     const gchar  *module_path)
{
  g_return_if_fail (GIMP_IS_MODULE_DB (db));
  g_return_if_fail (module_path != NULL);

  if (g_module_supported ())
    {
      GList *path;
      GList *list;

      path = gimp_config_path_expand_to_files (module_path, NULL);

      for (list = path; list; list = g_list_next (list))
        {
          gimp_module_db_load_directory (db, list->data);
        }

      g_list_free_full (path, (GDestroyNotify) g_object_unref);
    }

  if (FALSE)
    g_list_foreach (db->priv->modules, gimp_module_db_module_dump_func, NULL);
}

/**
 * gimp_module_db_refresh:
 * @db:          A #GimpModuleDB.
 * @module_path: A #G_SEARCHPATH_SEPARATOR delimited list of directories
 *               to load modules from.
 *
 * Does the same as gimp_module_db_load(), plus removes all #GimpModule
 * instances whose modules have been deleted from disk.
 *
 * Note that the #GimpModule's will just be removed from the internal
 * list and not freed as this is not possible with #GTypeModule
 * instances which actually implement types.
 **/
void
gimp_module_db_refresh (GimpModuleDB *db,
                        const gchar  *module_path)
{
  GList *list;

  g_return_if_fail (GIMP_IS_MODULE_DB (db));
  g_return_if_fail (module_path != NULL);

  list = db->priv->modules;

  while (list)
    {
      GimpModule *module = list->data;

      list = g_list_next (list);

      if (! gimp_module_is_on_disk (module) &&
          ! gimp_module_is_loaded (module))
        {
          g_signal_handlers_disconnect_by_func (module,
                                                gimp_module_db_module_modified,
                                                db);

          db->priv->modules = g_list_remove (db->priv->modules, module);

          g_signal_emit (db, db_signals[REMOVE], 0, module);
        }
    }

  /* walk filesystem and add new things we find */
  gimp_module_db_load (db, module_path);
}

static void
gimp_module_db_load_directory (GimpModuleDB *db,
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

              gimp_module_db_load_module (db, child);

              g_object_unref (child);
            }

          g_object_unref (info);
        }

      g_object_unref (enumerator);
    }
}

static void
gimp_module_db_load_module (GimpModuleDB *db,
                            GFile        *file)
{
  GimpModule *module;
  gboolean   load_inhibit;

  if (! gimp_file_has_extension (file, "." G_MODULE_SUFFIX))
    return;

  /* don't load if we already know about it */
  if (gimp_module_db_module_find_by_file (db, file))
    return;

  load_inhibit = is_in_inhibit_list (file, db->priv->load_inhibit);

  module = gimp_module_new (file,
                            ! load_inhibit,
                            db->priv->verbose);

  g_signal_connect (module, "modified",
                    G_CALLBACK (gimp_module_db_module_modified),
                    db);

  db->priv->modules = g_list_append (db->priv->modules, module);

  g_signal_emit (db, db_signals[ADD], 0, module);
}

static GimpModule *
gimp_module_db_module_find_by_file (GimpModuleDB *db,
                                    GFile        *file)
{
  GList *list;

  for (list = db->priv->modules; list; list = g_list_next (list))
    {
      GimpModule *module = list->data;

      if (g_file_equal (file, gimp_module_get_file (module)))
        return module;
    }

  return NULL;
}

static void
gimp_module_db_module_dump_func (gpointer data,
                                 gpointer user_data)
{
  static const gchar * const statenames[] =
  {
    N_("Module error"),
    N_("Loaded"),
    N_("Load failed"),
    N_("Not loaded")
  };

  GimpModule           *module = data;
  const GimpModuleInfo *info   = gimp_module_get_info (module);

  g_print ("\n%s: %s\n",
           gimp_file_get_utf8_name (gimp_module_get_file (module)),
           gettext (statenames[gimp_module_get_state (module)]));

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
gimp_module_db_module_modified (GimpModule   *module,
                                GimpModuleDB *db)
{
  g_signal_emit (db, db_signals[MODULE_MODIFIED], 0, module);
}
