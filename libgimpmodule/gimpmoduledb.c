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


struct _GimpModuleDB
{
  GObject    parent_instance;

  GPtrArray *modules;

  gchar     *load_inhibit;
  gboolean   verbose;
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

static void         gimp_module_db_list_model_iface_init (GListModelInterface *iface);


G_DEFINE_TYPE_WITH_CODE (GimpModuleDB, gimp_module_db, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL,
                                                gimp_module_db_list_model_iface_init))

#define parent_class gimp_module_db_parent_class


static void
gimp_module_db_class_init (GimpModuleDBClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_module_db_finalize;
}

static void
gimp_module_db_init (GimpModuleDB *db)
{
  db->modules      = g_ptr_array_new ();
  db->load_inhibit = NULL;
  db->verbose      = FALSE;
}

static void
gimp_module_db_finalize (GObject *object)
{
  GimpModuleDB *db = GIMP_MODULE_DB (object);

  g_clear_pointer (&db->modules,      g_ptr_array_unref);
  g_clear_pointer (&db->load_inhibit, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GType
gimp_module_db_get_item_type (GListModel *list)
{
  return GIMP_TYPE_MODULE;
}

static guint
gimp_module_db_get_n_items (GListModel *list)
{
  GimpModuleDB *self = GIMP_MODULE_DB (list);
  return self->modules->len;
}

static void *
gimp_module_db_get_item (GListModel *list,
                         guint       index)
{
  GimpModuleDB *self = GIMP_MODULE_DB (list);

  if (index >= self->modules->len)
    return NULL;
  return g_object_ref (g_ptr_array_index (self->modules, index));
}

static void
gimp_module_db_list_model_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = gimp_module_db_get_item_type;
  iface->get_n_items = gimp_module_db_get_n_items;
  iface->get_item = gimp_module_db_get_item;
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

  db->verbose = verbose ? TRUE : FALSE;

  return db;
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

  db->verbose = verbose ? TRUE : FALSE;
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

  return db->verbose;
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
  guint i;

  g_return_if_fail (GIMP_IS_MODULE_DB (db));

  g_free (db->load_inhibit);

  db->load_inhibit = g_strdup (load_inhibit);

  for (i = 0; i < db->modules->len; i++)
    {
      GimpModule *module = g_ptr_array_index (db->modules, i);
      gboolean    inhibit;

      inhibit = is_in_inhibit_list (gimp_module_get_file (module),
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

  return db->load_inhibit;
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
    g_ptr_array_foreach (db->modules, gimp_module_db_module_dump_func, NULL);
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
  guint i;

  g_return_if_fail (GIMP_IS_MODULE_DB (db));
  g_return_if_fail (module_path != NULL);

  for (i = 0; i < db->modules->len; i++)
    {
      GimpModule *module = g_ptr_array_index (db->modules, i);

      if (! gimp_module_is_on_disk (module) &&
          ! gimp_module_is_loaded (module))
        {
          g_ptr_array_remove_index (db->modules, i);
          g_list_model_items_changed (G_LIST_MODEL (db), i, 1, 0);
          i--;
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
          GFileType file_type = g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_STANDARD_TYPE);

         if (file_type == G_FILE_TYPE_REGULAR &&
             ! g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN))
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

  load_inhibit = is_in_inhibit_list (file, db->load_inhibit);

  module = gimp_module_new (file,
                            ! load_inhibit,
                            db->verbose);

  g_ptr_array_add (db->modules, module);
  g_list_model_items_changed (G_LIST_MODEL (db), db->modules->len - 1, 0, 1);
}

static GimpModule *
gimp_module_db_module_find_by_file (GimpModuleDB *db,
                                    GFile        *file)
{
  guint i;

  for (i = 0; i < db->modules->len; i++)
    {
      GimpModule *module = g_ptr_array_index (db->modules, i);

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
