/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdataloaderfactory.c
 * Copyright (C) 2001-2018 Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp.h"
#include "gimp-utils.h"
#include "gimpcontainer.h"
#include "gimpdata.h"
#include "gimpdataloaderfactory.h"

#include "gimp-intl.h"


/* Data files that have this string in their path are considered
 * obsolete and are only kept around for backwards compatibility
 */
#define GIMP_OBSOLETE_DATA_DIR_NAME "gimp-obsolete-files"


typedef struct _GimpDataLoader GimpDataLoader;

struct _GimpDataLoader
{
  gchar            *name;
  GimpDataLoadFunc  load_func;
  gchar            *extension;
  gboolean          writable;
};


struct _GimpDataLoaderFactoryPrivate
{
  GList          *loaders;
  GimpDataLoader *fallback;
};

#define GET_PRIVATE(obj) (((GimpDataLoaderFactory *) (obj))->priv)


static void   gimp_data_loader_factory_finalize       (GObject         *object);

static void   gimp_data_loader_factory_data_init      (GimpDataFactory *factory,
                                                       GimpContext     *context);
static void   gimp_data_loader_factory_data_refresh   (GimpDataFactory *factory,
                                                       GimpContext     *context);

static GimpDataLoader *
              gimp_data_loader_factory_get_loader     (GimpDataFactory *factory,
                                                       GFile           *file);

static void   gimp_data_loader_factory_load           (GimpDataFactory *factory,
                                                       GimpContext     *context,
                                                       GHashTable      *cache);
static void   gimp_data_loader_factory_load_directory (GimpDataFactory *factory,
                                                       GimpContext     *context,
                                                       GHashTable      *cache,
                                                       gboolean         dir_writable,
                                                       GFile           *directory,
                                                       GFile           *top_directory);
static void   gimp_data_loader_factory_load_data      (GimpDataFactory *factory,
                                                       GimpContext     *context,
                                                       GHashTable      *cache,
                                                       gboolean         dir_writable,
                                                       GFile           *file,
                                                       GFileInfo       *info,
                                                       GFile           *top_directory);

static GimpDataLoader * gimp_data_loader_new          (const gchar     *name,
                                                       GimpDataLoadFunc load_func,
                                                       const gchar     *extension,
                                                       gboolean         writable);
static void            gimp_data_loader_free          (GimpDataLoader  *loader);


G_DEFINE_TYPE_WITH_PRIVATE (GimpDataLoaderFactory, gimp_data_loader_factory,
                            GIMP_TYPE_DATA_FACTORY)

#define parent_class gimp_data_loader_factory_parent_class


static void
gimp_data_loader_factory_class_init (GimpDataLoaderFactoryClass *klass)
{
  GObjectClass         *object_class  = G_OBJECT_CLASS (klass);
  GimpDataFactoryClass *factory_class = GIMP_DATA_FACTORY_CLASS (klass);

  object_class->finalize      = gimp_data_loader_factory_finalize;

  factory_class->data_init    = gimp_data_loader_factory_data_init;
  factory_class->data_refresh = gimp_data_loader_factory_data_refresh;
}

static void
gimp_data_loader_factory_init (GimpDataLoaderFactory *factory)
{
  factory->priv = gimp_data_loader_factory_get_instance_private (factory);
}

static void
gimp_data_loader_factory_finalize (GObject *object)
{
  GimpDataLoaderFactoryPrivate *priv = GET_PRIVATE (object);

  g_list_free_full (priv->loaders, (GDestroyNotify) gimp_data_loader_free);
  priv->loaders = NULL;

  g_clear_pointer (&priv->fallback, gimp_data_loader_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_data_loader_factory_data_init (GimpDataFactory *factory,
                                    GimpContext     *context)
{
  gimp_data_loader_factory_load (factory, context, NULL);
}

static void
gimp_data_loader_factory_refresh_cache_add (GimpDataFactory *factory,
                                            GimpData        *data,
                                            gpointer         user_data)
{
  GFile *file = gimp_data_get_file (data);

  if (file)
    {
      GimpContainer *container = gimp_data_factory_get_container (factory);
      GHashTable    *cache     = user_data;
      GList         *list;

      g_object_ref (data);

      gimp_container_remove (container, GIMP_OBJECT (data));

      list = g_hash_table_lookup (cache, file);
      list = g_list_prepend (list, data);

      g_hash_table_insert (cache, file, list);
    }
}

static gboolean
gimp_data_loader_factory_refresh_cache_remove (gpointer key,
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
gimp_data_loader_factory_data_refresh (GimpDataFactory *factory,
                                       GimpContext     *context)
{
  GimpContainer *container = gimp_data_factory_get_container (factory);
  GHashTable    *cache;

  gimp_container_freeze (container);

  /*  First, save all dirty data objects  */
  gimp_data_factory_data_save (factory);

  cache = g_hash_table_new (g_file_hash, (GEqualFunc) g_file_equal);

  gimp_data_factory_data_foreach (factory, TRUE,
                                  gimp_data_loader_factory_refresh_cache_add,
                                  cache);

  /*  Now the cache contains a GFile => list-of-objects mapping of
   *  the old objects. So we should now traverse the directory and for
   *  each file load it only if its mtime is newer.
   *
   *  Once a file was added, it is removed from the cache, so the only
   *  objects remaining there will be those that are not present on
   *  the disk (that have to be destroyed)
   */
  gimp_data_loader_factory_load (factory, context, cache);

  /*  Now all the data is loaded. Free what remains in the cache  */
  g_hash_table_foreach_remove (cache,
                               gimp_data_loader_factory_refresh_cache_remove,
                               NULL);

  g_hash_table_destroy (cache);

  gimp_container_thaw (container);
}


/*  public functions  */

GimpDataFactory *
gimp_data_loader_factory_new (Gimp                    *gimp,
                              GType                    data_type,
                              const gchar             *path_property_name,
                              const gchar             *writable_property_name,
                              GimpDataNewFunc          new_func,
                              GimpDataGetStandardFunc  get_standard_func)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (g_type_is_a (data_type, GIMP_TYPE_DATA), NULL);
  g_return_val_if_fail (path_property_name != NULL, NULL);
  g_return_val_if_fail (writable_property_name != NULL, NULL);

  return g_object_new (GIMP_TYPE_DATA_LOADER_FACTORY,
                       "gimp",                   gimp,
                       "data-type",              data_type,
                       "path-property-name",     path_property_name,
                       "writable-property-name", writable_property_name,
                       "new-func",               new_func,
                       "get-standard-func",      get_standard_func,
                       NULL);
}

void
gimp_data_loader_factory_add_loader (GimpDataFactory  *factory,
                                     const gchar      *name,
                                     GimpDataLoadFunc  load_func,
                                     const gchar      *extension,
                                     gboolean          writable)
{
  GimpDataLoaderFactoryPrivate *priv;
  GimpDataLoader               *loader;

  g_return_if_fail (GIMP_IS_DATA_LOADER_FACTORY (factory));
  g_return_if_fail (name != NULL);
  g_return_if_fail (load_func != NULL);
  g_return_if_fail (extension != NULL);

  priv = GET_PRIVATE (factory);

  loader = gimp_data_loader_new (name, load_func, extension, writable);

  priv->loaders = g_list_append (priv->loaders, loader);
}

void
gimp_data_loader_factory_add_fallback (GimpDataFactory  *factory,
                                       const gchar      *name,
                                       GimpDataLoadFunc  load_func)
{
  GimpDataLoaderFactoryPrivate *priv;

  g_return_if_fail (GIMP_IS_DATA_LOADER_FACTORY (factory));
  g_return_if_fail (name != NULL);
  g_return_if_fail (load_func != NULL);

  priv = GET_PRIVATE (factory);

  g_clear_pointer (&priv->fallback, gimp_data_loader_free);

  priv->fallback = gimp_data_loader_new (name, load_func, NULL, FALSE);
}


/*  private functions  */

static GimpDataLoader *
gimp_data_loader_factory_get_loader (GimpDataFactory *factory,
                                     GFile           *file)
{
  GimpDataLoaderFactoryPrivate *priv = GET_PRIVATE (factory);
  GList                        *list;

  for (list = priv->loaders; list; list = g_list_next (list))
    {
      GimpDataLoader *loader = list->data;

      if (gimp_file_has_extension (file, loader->extension))
        return loader;
    }

  return priv->fallback;
}

static void
gimp_data_loader_factory_load (GimpDataFactory *factory,
                               GimpContext     *context,
                               GHashTable      *cache)
{
  GList *path;
  GList *writable_path;
  GList *list;

  path          = gimp_data_factory_get_data_path          (factory);
  writable_path = gimp_data_factory_get_data_path_writable (factory);

  for (list = path; list; list = g_list_next (list))
    {
      gboolean dir_writable = FALSE;

      if (g_list_find_custom (writable_path, list->data,
                              (GCompareFunc) gimp_file_compare))
        dir_writable = TRUE;

      gimp_data_loader_factory_load_directory (factory, context, cache,
                                               dir_writable,
                                               list->data,
                                               list->data);
    }

  g_list_free_full (path,          (GDestroyNotify) g_object_unref);
  g_list_free_full (writable_path, (GDestroyNotify) g_object_unref);
}

static void
gimp_data_loader_factory_load_directory (GimpDataFactory *factory,
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
              gimp_data_loader_factory_load_directory (factory, context, cache,
                                                       dir_writable,
                                                       child,
                                                       top_directory);
            }
          else if (file_type == G_FILE_TYPE_REGULAR)
            {
              gimp_data_loader_factory_load_data (factory, context, cache,
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
gimp_data_loader_factory_load_data (GimpDataFactory *factory,
                                    GimpContext     *context,
                                    GHashTable      *cache,
                                    gboolean         dir_writable,
                                    GFile           *file,
                                    GFileInfo       *info,
                                    GFile           *top_directory)
{
  GimpDataLoader *loader;
  GimpContainer  *container;
  GimpContainer  *container_obsolete;
  GList          *data_list = NULL;
  GInputStream   *input;
  guint64         mtime;
  GError         *error = NULL;

  loader = gimp_data_loader_factory_get_loader (factory, file);

  if (! loader)
    return;

  container          = gimp_data_factory_get_container          (factory);
  container_obsolete = gimp_data_factory_get_container_obsolete (factory);

  if (gimp_data_factory_get_gimp (factory)->be_verbose)
    g_print ("  Loading %s\n", gimp_file_get_utf8_name (file));

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
            gimp_container_add (container, list->data);

          return;
        }
    }

  input = G_INPUT_STREAM (g_file_read (file, NULL, &error));

  if (input)
    {
      GInputStream *buffered = g_buffered_input_stream_new (input);

      data_list = loader->load_func (context, file, buffered, &error);

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

      g_object_unref (buffered);
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
              gimp_container_add (container_obsolete,
                                  GIMP_OBJECT (data));
            }
          else
            {
              gimp_data_set_folder_tags (data, top_directory);

              gimp_container_add (container,
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
      gimp_message (gimp_data_factory_get_gimp (factory), NULL,
                    GIMP_MESSAGE_ERROR,
                    _("Failed to load data:\n\n%s"), error->message);
      g_clear_error (&error);
    }
}

static GimpDataLoader *
gimp_data_loader_new (const gchar      *name,
                      GimpDataLoadFunc  load_func,
                      const gchar      *extension,
                      gboolean          writable)
{
  GimpDataLoader *loader = g_slice_new (GimpDataLoader);

  loader->name      = g_strdup (name);
  loader->load_func = load_func;
  loader->extension = g_strdup (extension);
  loader->writable  = writable ? TRUE : FALSE;

  return loader;
}

static void
gimp_data_loader_free (GimpDataLoader *loader)
{
  g_free (loader->name);
  g_free (loader->extension);

  g_slice_free (GimpDataLoader, loader);
}
