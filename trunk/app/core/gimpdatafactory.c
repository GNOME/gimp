/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdatafactory.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include <stdlib.h>
#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpdata.h"
#include "gimpdatafactory.h"
#include "gimplist.h"

#include "gimp-intl.h"


#define WRITABLE_PATH_KEY "gimp-data-factory-writable-path"


static void    gimp_data_factory_finalize     (GObject              *object);

static void    gimp_data_factory_data_load    (GimpDataFactory      *factory,
                                               GHashTable           *cache);

static gint64  gimp_data_factory_get_memsize  (GimpObject           *object,
                                               gint64               *gui_size);

static gchar * gimp_data_factory_get_save_dir (GimpDataFactory      *factory);

static void    gimp_data_factory_load_data  (const GimpDatafileData *file_data,
                                             gpointer                data);


G_DEFINE_TYPE (GimpDataFactory, gimp_data_factory, GIMP_TYPE_OBJECT)

#define parent_class gimp_data_factory_parent_class


static void
gimp_data_factory_class_init (GimpDataFactoryClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  object_class->finalize         = gimp_data_factory_finalize;

  gimp_object_class->get_memsize = gimp_data_factory_get_memsize;
}

static void
gimp_data_factory_init (GimpDataFactory *factory)
{
  factory->gimp                   = NULL;
  factory->container              = NULL;
  factory->path_property_name     = NULL;
  factory->writable_property_name = NULL;
  factory->loader_entries         = NULL;
  factory->n_loader_entries       = 0;
  factory->data_new_func          = NULL;
  factory->data_get_standard_func = NULL;
}

static void
gimp_data_factory_finalize (GObject *object)
{
  GimpDataFactory *factory = GIMP_DATA_FACTORY (object);

  if (factory->container)
    {
      g_object_unref (factory->container);
      factory->container = NULL;
    }

  if (factory->path_property_name)
    {
      g_free (factory->path_property_name);
      factory->path_property_name = NULL;
    }

  if (factory->writable_property_name)
    {
      g_free (factory->writable_property_name);
      factory->writable_property_name = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_data_factory_get_memsize (GimpObject *object,
                               gint64     *gui_size)
{
  GimpDataFactory *factory = GIMP_DATA_FACTORY (object);
  gint64           memsize = 0;

  memsize += gimp_object_get_memsize (GIMP_OBJECT (factory->container),
                                      gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

GimpDataFactory *
gimp_data_factory_new (Gimp                             *gimp,
                       GType                             data_type,
                       const gchar                      *path_property_name,
                       const gchar                      *writable_property_name,
                       const GimpDataFactoryLoaderEntry *loader_entries,
                       gint                              n_loader_entries,
                       GimpDataNewFunc                   new_func,
                       GimpDataGetStandardFunc           standard_func)
{
  GimpDataFactory *factory;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (g_type_is_a (data_type, GIMP_TYPE_DATA), NULL);
  g_return_val_if_fail (path_property_name != NULL, NULL);
  g_return_val_if_fail (writable_property_name != NULL, NULL);
  g_return_val_if_fail (loader_entries != NULL, NULL);
  g_return_val_if_fail (n_loader_entries > 0, NULL);

  factory = g_object_new (GIMP_TYPE_DATA_FACTORY, NULL);

  factory->gimp                   = gimp;
  factory->container              = gimp_list_new (data_type, TRUE);
  gimp_list_set_sort_func (GIMP_LIST (factory->container),
                           (GCompareFunc) gimp_data_name_compare);

  factory->path_property_name     = g_strdup (path_property_name);
  factory->writable_property_name = g_strdup (writable_property_name);

  factory->loader_entries         = loader_entries;
  factory->n_loader_entries       = n_loader_entries;

  factory->data_new_func          = new_func;
  factory->data_get_standard_func = standard_func;

  return factory;
}

void
gimp_data_factory_data_init (GimpDataFactory *factory,
                             gboolean         no_data)
{
  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  /*  Freeze and thaw the container even if no_data,
   *  this creates the standard data that serves as fallback.
   */
  gimp_container_freeze (factory->container);

  if (! no_data)
    {
      if (factory->gimp->be_verbose)
        {
          const gchar *name = gimp_object_get_name (GIMP_OBJECT (factory));

          g_print ("Loading '%s' data\n", name ? name : "???");
        }

      gimp_data_factory_data_load (factory, NULL);
    }

  gimp_container_thaw (factory->container);
}

typedef struct
{
  GimpDataFactory *factory;
  GHashTable      *cache;
} GimpDataLoadContext;

static gboolean
gimp_data_factory_refresh_cache_remove (gpointer key,
                                        gpointer value,
                                        gpointer user_data)
{
  GList *list;

  for (list = value; list; list = list->next)
    g_object_unref (list->data);

  g_list_free (value);

  return TRUE;
}

static void
gimp_data_factory_data_move_to_cache (GimpDataFactory *factory,
                                      gpointer         object,
                                      gpointer         data)
{
  GHashTable  *cache    = data;
  GList       *list;
  const gchar *filename = GIMP_DATA (object)->filename;

  g_object_ref (object);

  gimp_container_remove (factory->container, GIMP_OBJECT (object));

  list = g_hash_table_lookup (cache, filename);
  list = g_list_prepend (list, object);

  g_hash_table_insert (cache, (gpointer) filename, list);
}

static void
gimp_data_factory_data_foreach (GimpDataFactory *factory,
                                void (*callback) (GimpDataFactory *factory,
                                                  gpointer         expr,
                                                  gpointer         context),
                                gpointer         context)
{
  GimpList *list;

  if (gimp_container_is_empty (factory->container))
    return;

  list = GIMP_LIST (factory->container);

  if (list->list)
    {
      if (GIMP_DATA (list->list->data)->internal)
        {
          /*  if there are internal objects in the list, skip them  */
          GList *glist;

          for (glist = list->list; glist; glist = g_list_next (glist))
            {
              if (glist->next && ! GIMP_DATA (glist->next->data)->internal)
                {
                  while (glist->next)
                    callback (factory, glist->next->data, context);

                  break;
                }
            }
        }
      else
        {
          while (list->list)
            callback (factory, list->list->data, context);
        }
    }
}

static void
gimp_data_factory_data_load (GimpDataFactory *factory,
                             GHashTable      *cache)
{
  gchar *path;
  gchar *writable_path;

  g_object_get (factory->gimp->config,
                factory->path_property_name,     &path,
                factory->writable_property_name, &writable_path,
                NULL);

  if (path && strlen (path))
    {
      GList               *writable_list = NULL;
      gchar               *tmp;
      GimpDataLoadContext  context;

      context.factory = factory;
      context.cache   = cache;

      tmp = gimp_config_path_expand (path, TRUE, NULL);
      g_free (path);
      path = tmp;

      if (writable_path)
        {
          tmp = gimp_config_path_expand (writable_path, TRUE, NULL);
          g_free (writable_path);
          writable_path = tmp;

          writable_list = gimp_path_parse (writable_path, 16, TRUE, NULL);

          g_object_set_data (G_OBJECT (factory),
                             WRITABLE_PATH_KEY, writable_list);
        }

      gimp_datafiles_read_directories (path, G_FILE_TEST_EXISTS,
                                       gimp_data_factory_load_data, &context);

      if (writable_path)
        {
          gimp_path_free (writable_list);
          g_object_set_data (G_OBJECT (factory), WRITABLE_PATH_KEY, NULL);
        }
    }

  g_free (path);
  g_free (writable_path);
}

static void
gimp_data_factory_data_reload (GimpDataFactory *factory)
{

  GHashTable *cache;

  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  gimp_container_freeze (factory->container);

  cache = g_hash_table_new (g_str_hash, g_str_equal);

  gimp_data_factory_data_foreach (factory,
                                  gimp_data_factory_data_move_to_cache, cache);

  /* Now the cache contains a filename => list-of-objects mapping of
   * the old objects. So we should now traverse the directory and for
   * each file load it only if it's mtime is newer.
   *
   * Once a file was added, it is removed from the cache, so the only
   * objects remaining there will be those that are not present on the
   * disk (that have to be destroyed)
   */
  gimp_data_factory_data_load (factory, cache);

  /* Now all the data is loaded. Free what remains in the cache. */
  g_hash_table_foreach_remove (cache,
                               gimp_data_factory_refresh_cache_remove, NULL);
  g_hash_table_destroy (cache);

  gimp_container_thaw (factory->container);
}

void
gimp_data_factory_data_refresh (GimpDataFactory *factory)
{
  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  gimp_container_freeze (factory->container);

  gimp_data_factory_data_save (factory);

  gimp_data_factory_data_reload (factory);

  gimp_container_thaw (factory->container);
}

void
gimp_data_factory_data_save (GimpDataFactory *factory)
{
  GList *list;
  gchar *writable_dir;

  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  if (gimp_container_is_empty (factory->container))
    return;

  writable_dir = gimp_data_factory_get_save_dir (factory);

  if (! writable_dir)
    return;

  for (list = GIMP_LIST (factory->container)->list;
       list;
       list = g_list_next (list))
    {
      GimpData *data = list->data;

      if (! data->filename)
        gimp_data_create_filename (data, writable_dir);

      if (data->dirty && data->writable)
        {
          GError *error = NULL;

          if (! gimp_data_save (data, &error))
            {
              /*  check if there actually was an error (no error
               *  means the data class does not implement save)
               */
              if (error)
                {
                  gimp_message (factory->gimp, NULL, GIMP_MESSAGE_ERROR,
                                _("Failed to save data:\n\n%s"),
                                error->message);
                  g_clear_error (&error);
                }
            }
        }
    }

  g_free (writable_dir);
}

static void
gimp_data_factory_remove_cb (GimpDataFactory *factory,
                             gpointer         data,
                             gpointer         context)
{
  gimp_container_remove (factory->container, GIMP_OBJECT (data));
}

void
gimp_data_factory_data_free (GimpDataFactory *factory)
{
  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  gimp_container_freeze (factory->container);

  gimp_data_factory_data_foreach (factory, gimp_data_factory_remove_cb, NULL);

  gimp_container_thaw (factory->container);
}

GimpData *
gimp_data_factory_data_new (GimpDataFactory *factory,
                            const gchar     *name)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (*name != '\0', NULL);

  if (factory->data_new_func)
    {
      GimpData *data = factory->data_new_func (name);

      if (data)
        {
          gimp_container_add (factory->container, GIMP_OBJECT (data));
          g_object_unref (data);

          return data;
        }

      g_warning ("%s: factory->data_new_func() returned NULL", G_STRFUNC);
    }

  return NULL;
}

GimpData *
gimp_data_factory_data_duplicate (GimpDataFactory *factory,
                                  GimpData        *data)
{
  GimpData *new_data;

  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (GIMP_IS_DATA (data), NULL);

  new_data = gimp_data_duplicate (data);

  if (new_data)
    {
      const gchar *name;
      gchar       *ext;
      gint         copy_len;
      gint         number;
      gchar       *new_name;

      name = gimp_object_get_name (GIMP_OBJECT (data));

      ext      = strrchr (name, '#');
      copy_len = strlen (_("copy"));

      if ((strlen (name) >= copy_len                                 &&
           strcmp (&name[strlen (name) - copy_len], _("copy")) == 0) ||
          (ext && (number = atoi (ext + 1)) > 0                      &&
           ((gint) (log10 (number) + 1)) == strlen (ext + 1)))
        {
          /* don't have redundant "copy"s */
          new_name = g_strdup (name);
        }
      else
        {
          new_name = g_strdup_printf (_("%s copy"), name);
        }

      gimp_object_take_name (GIMP_OBJECT (new_data), new_name);

      gimp_container_add (factory->container, GIMP_OBJECT (new_data));
      g_object_unref (new_data);
    }

  return new_data;
}

gboolean
gimp_data_factory_data_delete (GimpDataFactory  *factory,
                               GimpData         *data,
                               gboolean          delete_from_disk,
                               GError          **error)
{
  gboolean retval = TRUE;

  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), FALSE);
  g_return_val_if_fail (GIMP_IS_DATA (data), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (gimp_container_have (factory->container, GIMP_OBJECT (data)))
    {
      g_object_ref (data);

      gimp_container_remove (factory->container, GIMP_OBJECT (data));

      if (delete_from_disk && data->filename)
        retval = gimp_data_delete_from_disk (data, error);

      g_object_unref (data);
    }

  return retval;
}

GimpData *
gimp_data_factory_data_get_standard (GimpDataFactory *factory)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);

  if (factory->data_get_standard_func)
    return factory->data_get_standard_func ();

  return NULL;
}

gboolean
gimp_data_factory_data_save_single (GimpDataFactory  *factory,
                                    GimpData         *data,
                                    GError          **error)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), FALSE);
  g_return_val_if_fail (GIMP_IS_DATA (data), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! data->dirty)
    return TRUE;

  if (! data->filename)
    {
      gchar *writable_dir = gimp_data_factory_get_save_dir (factory);

      if (! writable_dir)
        {
          g_set_error (error, GIMP_DATA_ERROR, 0,
                       _("Failed to save data:\n\n%s"),
                       _("You don't have a writable data folder configured."));
          return FALSE;
        }

      gimp_data_create_filename (data, writable_dir);

      g_free (writable_dir);
    }

  if (! data->writable)
    return FALSE;

  if (! gimp_data_save (data, error))
    {
      /*  check if there actually was an error (no error
       *  means the data class does not implement save)
       */
      if (! error)
        g_set_error (error, GIMP_DATA_ERROR, 0,
                     _("Failed to save data:\n\n%s"),
                     "Data class does not implement saving");

      return FALSE;
    }

  return TRUE;
}


/*  private functions  */

static gchar *
gimp_data_factory_get_save_dir (GimpDataFactory *factory)
{
  gchar *path;
  gchar *writable_path;
  gchar *tmp;
  GList *path_list;
  GList *writable_list;
  GList *list;
  gchar *writable_dir = NULL;

  g_object_get (factory->gimp->config,
                factory->path_property_name,     &path,
                factory->writable_property_name, &writable_path,
                NULL);

  tmp = gimp_config_path_expand (path, TRUE, NULL);
  g_free (path);
  path = tmp;

  tmp = gimp_config_path_expand (writable_path, TRUE, NULL);
  g_free (writable_path);
  writable_path = tmp;

  path_list     = gimp_path_parse (path,          16, TRUE, NULL);
  writable_list = gimp_path_parse (writable_path, 16, TRUE, NULL);

  g_free (path);
  g_free (writable_path);

  for (list = writable_list; list; list = g_list_next (list))
    {
      GList *found = g_list_find_custom (path_list,
                                         list->data, (GCompareFunc) strcmp);
      if (found)
        {
          writable_dir = g_strdup (found->data);
          break;
        }
    }

  gimp_path_free (path_list);
  gimp_path_free (writable_list);

  return writable_dir;
}

static void
gimp_data_factory_load_data (const GimpDatafileData *file_data,
                             gpointer                data)
{
  GimpDataLoadContext *context = data;
  GimpDataFactory     *factory = context->factory;
  GHashTable          *cache   = context->cache;
  gint                 i;

  for (i = 0; i < factory->n_loader_entries; i++)
    {
      if (! factory->loader_entries[i].extension ||
          gimp_datafiles_check_extension (file_data->filename,
                                          factory->loader_entries[i].extension))
        goto insert;
    }

  return;

 insert:
  {
    GList    *cached_data;
    gboolean  load_from_disk = TRUE;

    if (cache &&
        (cached_data = g_hash_table_lookup (cache, file_data->filename)))
      {
        GimpData *data = cached_data->data;

        load_from_disk = (data->mtime == 0 || data->mtime != file_data->mtime);

        if (! load_from_disk)
          {
            GList *list;

            for (list = cached_data; list; list = g_list_next (list))
              gimp_container_add (factory->container, GIMP_OBJECT (list->data));
          }
      }

    if (load_from_disk)
      {
        GList  *data_list;
        GError *error = NULL;

        data_list = factory->loader_entries[i].load_func (file_data->filename,
                                                          &error);

        if (G_LIKELY (data_list))
          {
            GList    *writable_list;
            GList    *list;
            gboolean  writable;
            gboolean  deletable;

            writable_list = g_object_get_data (G_OBJECT (factory),
                                               WRITABLE_PATH_KEY);

            deletable = (g_list_length (data_list) == 1 &&
                         g_list_find_custom (writable_list, file_data->dirname,
                                             (GCompareFunc) strcmp) != NULL);

            writable = (deletable && factory->loader_entries[i].writable);

            for (list = data_list; list; list = g_list_next (list))
              {
                GimpData *data = list->data;

                gimp_data_set_filename (data, file_data->filename,
                                        writable, deletable);
                data->mtime = file_data->mtime;
                data->dirty = FALSE;

                gimp_container_add (factory->container, GIMP_OBJECT (data));
                g_object_unref (data);
              }

            g_list_free (data_list);
          }

        if (G_UNLIKELY (error))
          {
            gimp_message (factory->gimp, NULL, GIMP_MESSAGE_ERROR,
                          _("Failed to load data:\n\n%s"), error->message);
            g_clear_error (&error);
          }
      }
  }
}
