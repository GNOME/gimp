/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdatafactory.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include <stdlib.h>
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp.h"
#include "gimp-utils.h"
#include "gimpcontext.h"
#include "gimpdata.h"
#include "gimpdatafactory.h"
#include "gimplist.h"

#include "gimp-intl.h"


/* Data files that have this string in their path are considered
 * obsolete and are only kept around for backwards compatibility
 */
#define GIMP_OBSOLETE_DATA_DIR_NAME "gimp-obsolete-files"


typedef void (* GimpDataForeachFunc) (GimpDataFactory *factory,
                                      GimpData        *data,
                                      gpointer         user_data);


struct _GimpDataFactoryPriv
{
  Gimp                             *gimp;
  GimpContainer                    *container;

  GimpContainer                    *container_obsolete;

  gchar                            *path_property_name;
  gchar                            *writable_property_name;

  const GimpDataFactoryLoaderEntry *loader_entries;
  gint                              n_loader_entries;

  GimpDataNewFunc                   data_new_func;
  GimpDataGetStandardFunc           data_get_standard_func;
};


static void    gimp_data_factory_finalize       (GObject             *object);

static gint64  gimp_data_factory_get_memsize    (GimpObject          *object,
                                                 gint64              *gui_size);

static void    gimp_data_factory_data_foreach   (GimpDataFactory     *factory,
                                                 gboolean             skip_internal,
                                                 GimpDataForeachFunc  callback,
                                                 gpointer             user_data);

static void    gimp_data_factory_data_load      (GimpDataFactory     *factory,
                                                 GimpContext         *context,
                                                 GHashTable          *cache);

static GFile * gimp_data_factory_get_save_dir   (GimpDataFactory     *factory,
                                                 GError             **error);

static void    gimp_data_factory_load_directory (GimpDataFactory     *factory,
                                                 GimpContext         *context,
                                                 GHashTable          *cache,
                                                 gboolean             dir_writable,
                                                 GFile               *directory,
                                                 GFile               *top_directory);
static void    gimp_data_factory_load_data      (GimpDataFactory     *factory,
                                                 GimpContext         *context,
                                                 GHashTable          *cache,
                                                 gboolean             dir_writable,
                                                 GFile               *file,
                                                 GFileInfo           *info,
                                                 GFile               *top_directory);


G_DEFINE_TYPE (GimpDataFactory, gimp_data_factory, GIMP_TYPE_OBJECT)

#define parent_class gimp_data_factory_parent_class


static void
gimp_data_factory_class_init (GimpDataFactoryClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  object_class->finalize         = gimp_data_factory_finalize;

  gimp_object_class->get_memsize = gimp_data_factory_get_memsize;

  g_type_class_add_private (klass, sizeof (GimpDataFactoryPriv));
}

static void
gimp_data_factory_init (GimpDataFactory *factory)
{
  factory->priv = G_TYPE_INSTANCE_GET_PRIVATE (factory,
                                               GIMP_TYPE_DATA_FACTORY,
                                               GimpDataFactoryPriv);

  factory->priv->gimp                   = NULL;
  factory->priv->container              = NULL;
  factory->priv->container_obsolete     = NULL;
  factory->priv->path_property_name     = NULL;
  factory->priv->writable_property_name = NULL;
  factory->priv->loader_entries         = NULL;
  factory->priv->n_loader_entries       = 0;
  factory->priv->data_new_func          = NULL;
  factory->priv->data_get_standard_func = NULL;
}

static void
gimp_data_factory_finalize (GObject *object)
{
  GimpDataFactory *factory = GIMP_DATA_FACTORY (object);

  g_clear_object (&factory->priv->container);
  g_clear_object (&factory->priv->container_obsolete);

  g_clear_pointer (&factory->priv->path_property_name,     g_free);
  g_clear_pointer (&factory->priv->writable_property_name, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_data_factory_get_memsize (GimpObject *object,
                               gint64     *gui_size)
{
  GimpDataFactory *factory = GIMP_DATA_FACTORY (object);
  gint64           memsize = 0;

  memsize += gimp_object_get_memsize (GIMP_OBJECT (factory->priv->container),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (factory->priv->container_obsolete),
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
                       GimpDataGetStandardFunc           get_standard_func)
{
  GimpDataFactory *factory;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (g_type_is_a (data_type, GIMP_TYPE_DATA), NULL);
  g_return_val_if_fail (path_property_name != NULL, NULL);
  g_return_val_if_fail (writable_property_name != NULL, NULL);
  g_return_val_if_fail (loader_entries != NULL, NULL);
  g_return_val_if_fail (n_loader_entries > 0, NULL);

  factory = g_object_new (GIMP_TYPE_DATA_FACTORY, NULL);

  factory->priv->gimp                   = gimp;
  factory->priv->container              = gimp_list_new (data_type, TRUE);
  gimp_list_set_sort_func (GIMP_LIST (factory->priv->container),
                           (GCompareFunc) gimp_data_compare);
  factory->priv->container_obsolete     = gimp_list_new (data_type, TRUE);
  gimp_list_set_sort_func (GIMP_LIST (factory->priv->container_obsolete),
                           (GCompareFunc) gimp_data_compare);

  factory->priv->path_property_name     = g_strdup (path_property_name);
  factory->priv->writable_property_name = g_strdup (writable_property_name);

  factory->priv->loader_entries         = loader_entries;
  factory->priv->n_loader_entries       = n_loader_entries;

  factory->priv->data_new_func          = new_func;
  factory->priv->data_get_standard_func = get_standard_func;

  return factory;
}

void
gimp_data_factory_data_init (GimpDataFactory *factory,
                             GimpContext     *context,
                             gboolean         no_data)
{
  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  /*  Freeze and thaw the container even if no_data,
   *  this creates the standard data that serves as fallback.
   */
  gimp_container_freeze (factory->priv->container);

  if (! no_data)
    {
      if (factory->priv->gimp->be_verbose)
        {
          const gchar *name = gimp_object_get_name (factory);

          g_print ("Loading '%s' data\n", name ? name : "???");
        }

      gimp_data_factory_data_load (factory, context, NULL);
    }

  gimp_container_thaw (factory->priv->container);
}

static void
gimp_data_factory_clean_cb (GimpDataFactory *factory,
                            GimpData        *data,
                            gpointer         user_data)
{
  if (gimp_data_is_dirty (data))
    gimp_data_clean (data);
}

void
gimp_data_factory_data_clean (GimpDataFactory *factory)
{
  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  gimp_data_factory_data_foreach (factory, TRUE,
                                  gimp_data_factory_clean_cb, NULL);
}

static void
gimp_data_factory_refresh_cache_add (GimpDataFactory *factory,
                                     GimpData        *data,
                                     gpointer         user_data)
{
  GFile *file = gimp_data_get_file (data);

  if (file)
    {
      GHashTable *cache = user_data;
      GList      *list;

      g_object_ref (data);

      gimp_container_remove (factory->priv->container, GIMP_OBJECT (data));

      list = g_hash_table_lookup (cache, file);
      list = g_list_prepend (list, data);

      g_hash_table_insert (cache, file, list);
    }
}

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
gimp_data_factory_data_foreach (GimpDataFactory     *factory,
                                gboolean             skip_internal,
                                GimpDataForeachFunc  callback,
                                gpointer             user_data)
{
  GList *list = GIMP_LIST (factory->priv->container)->queue->head;

  if (skip_internal)
    {
      while (list && gimp_data_is_internal (GIMP_DATA (list->data)))
        list = g_list_next (list);
    }

  while (list)
    {
      GList *next = g_list_next (list);

      callback (factory, list->data, user_data);

      list = next;
    }
}

static void
gimp_data_factory_data_load (GimpDataFactory *factory,
                             GimpContext     *context,
                             GHashTable      *cache)
{
  gchar *p;
  gchar *wp;
  GList *path;
  GList *writable_path;
  GList *list;

  g_object_get (factory->priv->gimp->config,
                factory->priv->path_property_name,     &p,
                factory->priv->writable_property_name, &wp,
                NULL);

  path          = gimp_config_path_expand_to_files (p, NULL);
  writable_path = gimp_config_path_expand_to_files (wp, NULL);

  g_free (p);
  g_free (wp);

  for (list = path; list; list = g_list_next (list))
    {
      gboolean dir_writable = FALSE;

      if (g_list_find_custom (writable_path, list->data,
                              (GCompareFunc) gimp_file_compare))
        dir_writable = TRUE;

      gimp_data_factory_load_directory (factory, context, cache,
                                        dir_writable,
                                        list->data,
                                        list->data);
    }

  g_list_free_full (path,          (GDestroyNotify) g_object_unref);
  g_list_free_full (writable_path, (GDestroyNotify) g_object_unref);
}

void
gimp_data_factory_data_refresh (GimpDataFactory *factory,
                                GimpContext     *context)
{
  GHashTable *cache;

  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  gimp_container_freeze (factory->priv->container);

  /*  First, save all dirty data objects  */
  gimp_data_factory_data_save (factory);

  cache = g_hash_table_new (g_file_hash, (GEqualFunc) g_file_equal);

  gimp_data_factory_data_foreach (factory, TRUE,
                                  gimp_data_factory_refresh_cache_add, cache);

  /*  Now the cache contains a GFile => list-of-objects mapping of
   *  the old objects. So we should now traverse the directory and for
   *  each file load it only if its mtime is newer.
   *
   *  Once a file was added, it is removed from the cache, so the only
   *  objects remaining there will be those that are not present on
   *  the disk (that have to be destroyed)
   */
  gimp_data_factory_data_load (factory, context, cache);

  /*  Now all the data is loaded. Free what remains in the cache  */
  g_hash_table_foreach_remove (cache,
                               gimp_data_factory_refresh_cache_remove, NULL);

  g_hash_table_destroy (cache);

  gimp_container_thaw (factory->priv->container);
}

void
gimp_data_factory_data_save (GimpDataFactory *factory)
{
  GList  *dirty = NULL;
  GList  *list;
  GFile  *writable_dir;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  if (gimp_container_is_empty (factory->priv->container))
    return;

  for (list = GIMP_LIST (factory->priv->container)->queue->head;
       list;
       list = g_list_next (list))
    {
      GimpData *data = list->data;

      if (gimp_data_is_dirty (data) &&
          gimp_data_is_writable (data))
        {
          dirty = g_list_prepend (dirty, data);
        }
    }

  if (! dirty)
    return;

  writable_dir = gimp_data_factory_get_save_dir (factory, &error);

  if (! writable_dir)
    {
      gimp_message (factory->priv->gimp, NULL, GIMP_MESSAGE_ERROR,
                    _("Failed to save data:\n\n%s"),
                    error->message);
      g_clear_error (&error);

      g_list_free (dirty);

      return;
    }

  for (list = dirty; list; list = g_list_next (list))
    {
      GimpData *data  = list->data;
      GError   *error = NULL;

      if (! gimp_data_get_file (data))
        gimp_data_create_filename (data, writable_dir);

      if (! gimp_data_save (data, &error))
        {
          /*  check if there actually was an error (no error
           *  means the data class does not implement save)
           */
          if (error)
            {
              gimp_message (factory->priv->gimp, NULL, GIMP_MESSAGE_ERROR,
                            _("Failed to save data:\n\n%s"),
                            error->message);
              g_clear_error (&error);
            }
        }
    }

  g_object_unref (writable_dir);

  g_list_free (dirty);
}

static void
gimp_data_factory_remove_cb (GimpDataFactory *factory,
                             GimpData        *data,
                             gpointer         user_data)
{
  gimp_container_remove (factory->priv->container, GIMP_OBJECT (data));
}

void
gimp_data_factory_data_free (GimpDataFactory *factory)
{
  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  gimp_container_freeze (factory->priv->container);

  gimp_data_factory_data_foreach (factory, TRUE,
                                  gimp_data_factory_remove_cb, NULL);

  gimp_container_thaw (factory->priv->container);
}

GimpData *
gimp_data_factory_data_new (GimpDataFactory *factory,
                            GimpContext     *context,
                            const gchar     *name)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (*name != '\0', NULL);

  if (factory->priv->data_new_func)
    {
      GimpData *data = factory->priv->data_new_func (context, name);

      if (data)
        {
          gimp_container_add (factory->priv->container, GIMP_OBJECT (data));
          g_object_unref (data);

          return data;
        }

      g_warning ("%s: factory->priv->data_new_func() returned NULL", G_STRFUNC);
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
      const gchar *name = gimp_object_get_name (data);
      gchar       *ext;
      gint         copy_len;
      gint         number;
      gchar       *new_name;

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

      gimp_container_add (factory->priv->container, GIMP_OBJECT (new_data));
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

  if (gimp_container_have (factory->priv->container, GIMP_OBJECT (data)))
    {
      g_object_ref (data);

      gimp_container_remove (factory->priv->container, GIMP_OBJECT (data));

      if (delete_from_disk && gimp_data_get_file (data))
        retval = gimp_data_delete_from_disk (data, error);

      g_object_unref (data);
    }

  return retval;
}

GimpData *
gimp_data_factory_data_get_standard (GimpDataFactory *factory,
                                     GimpContext     *context)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  if (factory->priv->data_get_standard_func)
    return factory->priv->data_get_standard_func (context);

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

  if (! gimp_data_is_dirty (data))
    return TRUE;

  if (! gimp_data_get_file (data))
    {
      GFile  *writable_dir;
      GError *my_error = NULL;

      writable_dir = gimp_data_factory_get_save_dir (factory, &my_error);

      if (! writable_dir)
        {
          g_set_error (error, GIMP_DATA_ERROR, 0,
                       _("Failed to save data:\n\n%s"),
                       my_error->message);
          g_clear_error (&my_error);

          return FALSE;
        }

      gimp_data_create_filename (data, writable_dir);

      g_object_unref (writable_dir);
    }

  if (! gimp_data_is_writable (data))
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

GType
gimp_data_factory_get_data_type (GimpDataFactory *factory)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), G_TYPE_NONE);

  return gimp_container_get_children_type (factory->priv->container);
}

GimpContainer *
gimp_data_factory_get_container (GimpDataFactory *factory)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);

  return factory->priv->container;
}

GimpContainer *
gimp_data_factory_get_container_obsolete (GimpDataFactory *factory)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);

  return factory->priv->container_obsolete;
}

Gimp *
gimp_data_factory_get_gimp (GimpDataFactory *factory)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);

  return factory->priv->gimp;
}

gboolean
gimp_data_factory_has_data_new_func (GimpDataFactory *factory)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), FALSE);

  return factory->priv->data_new_func != NULL;
}


/*  private functions  */

static GFile *
gimp_data_factory_get_save_dir (GimpDataFactory  *factory,
                                GError          **error)
{
  gchar *p;
  gchar *wp;
  GList *path;
  GList *writable_path;
  GFile *writable_dir = NULL;

  g_object_get (factory->priv->gimp->config,
                factory->priv->path_property_name,     &p,
                factory->priv->writable_property_name, &wp,
                NULL);

  path          = gimp_config_path_expand_to_files (p, NULL);
  writable_path = gimp_config_path_expand_to_files (wp, NULL);

  g_free (p);
  g_free (wp);

  if (writable_path)
    {
      GList    *list;
      gboolean  found_any = FALSE;

      for (list = writable_path; list; list = g_list_next (list))
        {
          GList *found = g_list_find_custom (path, list->data,
                                             (GCompareFunc) gimp_file_compare);
          if (found)
            {
              GFile *dir = found->data;

              found_any = TRUE;

              if (g_file_query_file_type (dir, G_FILE_QUERY_INFO_NONE,
                                          NULL) != G_FILE_TYPE_DIRECTORY)
                {
                  /*  error out only if this is the last chance  */
                  if (! list->next)
                    {
                      g_set_error (error, GIMP_DATA_ERROR, 0,
                                   _("You have a writable data folder "
                                     "configured (%s), but this folder does "
                                     "not exist. Please create the folder or "
                                     "fix your configuration in the "
                                     "Preferences dialog's 'Folders' section."),
                                   gimp_file_get_utf8_name (dir));
                    }
                }
              else
                {
                  writable_dir = g_object_ref (dir);
                  break;
                }
            }
        }

      if (! writable_dir && ! found_any)
        {
          g_set_error (error, GIMP_DATA_ERROR, 0,
                       _("You have a writable data folder configured, but this "
                         "folder is not part of your data search path. You "
                         "probably edited the gimprc file manually, "
                         "please fix it in the Preferences dialog's 'Folders' "
                         "section."));
        }
    }
  else
    {
      g_set_error (error, GIMP_DATA_ERROR, 0,
                   _("You don't have any writable data folder configured."));
    }

  g_list_free_full (path,          (GDestroyNotify) g_object_unref);
  g_list_free_full (writable_path, (GDestroyNotify) g_object_unref);

  return writable_dir;
}

static void
gimp_data_factory_load_directory (GimpDataFactory *factory,
                                  GimpContext     *context,
                                  GHashTable      *cache,
                                  gboolean         dir_writable,
                                  GFile           *directory,
                                  GFile           *top_directory)
{
  GFileEnumerator *enumerator;

  enumerator = g_file_enumerate_children (directory,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                          G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                          G_FILE_ATTRIBUTE_STANDARD_TYPE ","
                                          G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                          G_FILE_QUERY_INFO_NONE,
                                          NULL, NULL);

  if (enumerator)
    {
      GFileInfo *info;

      while ((info = g_file_enumerator_next_file (enumerator, NULL, NULL)))
        {
          GFileType  file_type;
          GFile     *child;

          if (g_file_info_get_is_hidden (info))
            {
              g_object_unref (info);
              continue;
            }

          file_type = g_file_info_get_file_type (info);
          child     = g_file_enumerator_get_child (enumerator, info);

          if (file_type == G_FILE_TYPE_DIRECTORY)
            {
              gimp_data_factory_load_directory (factory, context, cache,
                                                dir_writable,
                                                child,
                                                top_directory);
            }
          else if (file_type == G_FILE_TYPE_REGULAR)
            {
              gimp_data_factory_load_data (factory, context, cache,
                                           dir_writable,
                                           child, info,
                                           top_directory);
            }

          g_object_unref (child);
          g_object_unref (info);
        }

      g_object_unref (enumerator);
    }
}

static void
gimp_data_factory_load_data (GimpDataFactory *factory,
                             GimpContext     *context,
                             GHashTable      *cache,
                             gboolean         dir_writable,
                             GFile           *file,
                             GFileInfo       *info,
                             GFile           *top_directory)
{
  const GimpDataFactoryLoaderEntry *loader    = NULL;
  GList                            *data_list = NULL;
  GInputStream                     *input;
  guint64                           mtime;
  gint                              i;
  GError                           *error = NULL;

  for (i = 0; i < factory->priv->n_loader_entries; i++)
    {
      loader = &factory->priv->loader_entries[i];

      /* a loder matches if its extension matches, or if it doesn't
       * have an extension, which is the case for the fallback loader,
       * which must be last in the loader array
       */
      if (! loader->extension ||
          gimp_file_has_extension (file, loader->extension))
        {
          goto insert;
        }
    }

  return;

 insert:
  mtime = g_file_info_get_attribute_uint64 (info,
                                            G_FILE_ATTRIBUTE_TIME_MODIFIED);

  if (cache)
    {
      GList *cached_data = g_hash_table_lookup (cache, file);

      if (cached_data &&
          gimp_data_get_mtime (cached_data->data) != 0 &&
          gimp_data_get_mtime (cached_data->data) == mtime)
        {
          GList *list;

          for (list = cached_data; list; list = g_list_next (list))
            gimp_container_add (factory->priv->container, list->data);

          return;
        }
    }

  input = G_INPUT_STREAM (g_file_read (file, NULL, &error));

  if (input)
    {
      data_list = loader->load_func (context, file, input, &error);

      if (error)
        {
          g_prefix_error (&error,
                          _("Error loading '%s': "),
                          gimp_file_get_utf8_name (file));
        }
      else if (! data_list)
        {
          g_set_error (&error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Error loading '%s'"),
                       gimp_file_get_utf8_name (file));
        }

      g_object_unref (input);
    }
  else
    {
      g_prefix_error (&error,
                      _("Could not open '%s' for reading: "),
                      gimp_file_get_utf8_name (file));
    }

  if (G_LIKELY (data_list))
    {
      GList    *list;
      gchar    *uri;
      gboolean  obsolete;
      gboolean  writable  = FALSE;
      gboolean  deletable = FALSE;

      uri = g_file_get_uri (file);

      obsolete = (strstr (uri, GIMP_OBSOLETE_DATA_DIR_NAME) != 0);

      g_free (uri);

      /* obsolete files are immutable, don't check their writability */
      if (! obsolete)
        {
          deletable = (g_list_length (data_list) == 1 && dir_writable);
          writable  = (deletable && loader->writable);
        }

      for (list = data_list; list; list = g_list_next (list))
        {
          GimpData *data = list->data;

          gimp_data_set_file (data, file, writable, deletable);
          gimp_data_set_mtime (data, mtime);
          gimp_data_clean (data);

          if (obsolete)
            {
              gimp_container_add (factory->priv->container_obsolete,
                                  GIMP_OBJECT (data));
            }
          else
            {
              gimp_data_set_folder_tags (data, top_directory);

              gimp_container_add (factory->priv->container,
                                  GIMP_OBJECT (data));
            }

          g_object_unref (data);
        }

      g_list_free (data_list);
    }

  /*  not else { ... } because loader->load_func() can return a list
   *  of data objects *and* an error message if loading failed after
   *  something was already loaded
   */
  if (G_UNLIKELY (error))
    {
      gimp_message (factory->priv->gimp, NULL, GIMP_MESSAGE_ERROR,
                    _("Failed to load data:\n\n%s"), error->message);
      g_clear_error (&error);
    }
}
