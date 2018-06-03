/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdatafactory.c
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
#include "gimpasyncset.h"
#include "gimpcancelable.h"
#include "gimpcontext.h"
#include "gimpdata.h"
#include "gimpdatafactory.h"
#include "gimplist.h"
#include "gimpuncancelablewaitable.h"
#include "gimpwaitable.h"

#include "gimp-intl.h"


/* Data files that have this string in their path are considered
 * obsolete and are only kept around for backwards compatibility
 */
#define GIMP_OBSOLETE_DATA_DIR_NAME "gimp-obsolete-files"


enum
{
  PROP_0,
  PROP_GIMP,
  PROP_DATA_TYPE,
  PROP_PATH_PROPERTY_NAME,
  PROP_WRITABLE_PROPERTY_NAME
};


typedef void (* GimpDataForeachFunc) (GimpDataFactory *factory,
                                      GimpData        *data,
                                      gpointer         user_data);


struct _GimpDataFactoryPrivate
{
  Gimp                             *gimp;

  GType                             data_type;
  GimpContainer                    *container;
  GimpContainer                    *container_obsolete;

  gchar                            *path_property_name;
  gchar                            *writable_property_name;

  GimpAsyncSet                     *async_set;

  const GimpDataFactoryLoaderEntry *loader_entries;
  gint                              n_loader_entries;

  GimpDataNewFunc                   data_new_func;
  GimpDataGetStandardFunc           data_get_standard_func;
};

#define GET_PRIVATE(obj) (((GimpDataFactory *) (obj))->priv)


static void       gimp_data_factory_constructed         (GObject             *object);
static void       gimp_data_factory_set_property        (GObject             *object,
                                                         guint                property_id,
                                                         const GValue        *value,
                                                         GParamSpec          *pspec);
static void       gimp_data_factory_get_property        (GObject             *object,
                                                         guint                property_id,
                                                         GValue              *value,
                                                         GParamSpec          *pspec);
static void       gimp_data_factory_finalize            (GObject             *object);

static gint64     gimp_data_factory_get_memsize         (GimpObject          *object,
                                                         gint64              *gui_size);

static void       gimp_data_factory_real_data_init      (GimpDataFactory     *factory,
                                                         GimpContext         *context);
static void       gimp_data_factory_real_data_refresh   (GimpDataFactory     *factory,
                                                         GimpContext         *context);
static void       gimp_data_factory_real_data_save      (GimpDataFactory     *factory);
static void       gimp_data_factory_real_data_free      (GimpDataFactory     *factory);
static GimpData * gimp_data_factory_real_data_new       (GimpDataFactory     *factory,
                                                         GimpContext         *context,
                                                         const gchar         *name);
static GimpData * gimp_data_factory_real_data_duplicate (GimpDataFactory     *factory,
                                                         GimpData            *data);
static gboolean   gimp_data_factory_real_data_delete    (GimpDataFactory     *factory,
                                                         GimpData            *data,
                                                         gboolean             delete_from_disk,
                                                         GError             **error);

static void    gimp_data_factory_path_notify            (GObject             *object,
                                                         const GParamSpec    *pspec,
                                                         GimpDataFactory     *factory);
static void    gimp_data_factory_data_foreach           (GimpDataFactory     *factory,
                                                         gboolean             skip_internal,
                                                         GimpDataForeachFunc  callback,
                                                         gpointer             user_data);

static void    gimp_data_factory_data_load              (GimpDataFactory     *factory,
                                                         GimpContext         *context,
                                                         GHashTable          *cache);

static GFile * gimp_data_factory_get_save_dir           (GimpDataFactory     *factory,
                                                         GError             **error);

static void    gimp_data_factory_load_directory         (GimpDataFactory     *factory,
                                                         GimpContext         *context,
                                                         GHashTable          *cache,
                                                         gboolean             dir_writable,
                                                         GFile               *directory,
                                                         GFile               *top_directory);
static void    gimp_data_factory_load_data              (GimpDataFactory     *factory,
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

  object_class->constructed      = gimp_data_factory_constructed;
  object_class->set_property     = gimp_data_factory_set_property;
  object_class->get_property     = gimp_data_factory_get_property;
  object_class->finalize         = gimp_data_factory_finalize;

  gimp_object_class->get_memsize = gimp_data_factory_get_memsize;

  klass->data_init               = gimp_data_factory_real_data_init;
  klass->data_refresh            = gimp_data_factory_real_data_refresh;
  klass->data_save               = gimp_data_factory_real_data_save;
  klass->data_free               = gimp_data_factory_real_data_free;
  klass->data_new                = gimp_data_factory_real_data_new;
  klass->data_duplicate          = gimp_data_factory_real_data_duplicate;
  klass->data_delete             = gimp_data_factory_real_data_delete;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp", NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DATA_TYPE,
                                   g_param_spec_gtype ("data-type", NULL, NULL,
                                                       GIMP_TYPE_DATA,
                                                       GIMP_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PATH_PROPERTY_NAME,
                                   g_param_spec_string ("path-property-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_WRITABLE_PROPERTY_NAME,
                                   g_param_spec_string ("writable-property-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (klass, sizeof (GimpDataFactoryPrivate));
}

static void
gimp_data_factory_init (GimpDataFactory *factory)
{
  factory->priv = G_TYPE_INSTANCE_GET_PRIVATE (factory,
                                               GIMP_TYPE_DATA_FACTORY,
                                               GimpDataFactoryPrivate);

  factory->priv->async_set = gimp_async_set_new ();
}

static void
gimp_data_factory_constructed (GObject *object)
{
  GimpDataFactoryPrivate *priv = GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_GIMP (priv->gimp));
  gimp_assert (g_type_is_a (priv->data_type, GIMP_TYPE_DATA));

  priv->container = gimp_list_new (priv->data_type, TRUE);
  gimp_list_set_sort_func (GIMP_LIST (priv->container),
                           (GCompareFunc) gimp_data_compare);

  priv->container_obsolete = gimp_list_new (priv->data_type, TRUE);
  gimp_list_set_sort_func (GIMP_LIST (priv->container_obsolete),
                           (GCompareFunc) gimp_data_compare);
}

static void
gimp_data_factory_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpDataFactoryPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      priv->gimp = g_value_get_object (value); /* don't ref */
      break;

    case PROP_DATA_TYPE:
      priv->data_type = g_value_get_gtype (value);
      break;

    case PROP_PATH_PROPERTY_NAME:
      priv->path_property_name = g_value_dup_string (value);
      break;

    case PROP_WRITABLE_PROPERTY_NAME:
      priv->writable_property_name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_data_factory_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpDataFactoryPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, priv->gimp);
      break;

    case PROP_DATA_TYPE:
      g_value_set_gtype (value, priv->data_type);
      break;

    case PROP_PATH_PROPERTY_NAME:
      g_value_set_string (value, priv->path_property_name);
      break;

    case PROP_WRITABLE_PROPERTY_NAME:
      g_value_set_string (value, priv->writable_property_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_data_factory_finalize (GObject *object)
{
  GimpDataFactoryPrivate *priv = GET_PRIVATE (object);

  if (priv->async_set)
    {
      gimp_cancelable_cancel (GIMP_CANCELABLE (priv->async_set));

      g_clear_object (&priv->async_set);
    }

  g_clear_object (&priv->container);
  g_clear_object (&priv->container_obsolete);

  g_clear_pointer (&priv->path_property_name,     g_free);
  g_clear_pointer (&priv->writable_property_name, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_data_factory_get_memsize (GimpObject *object,
                               gint64     *gui_size)
{
  GimpDataFactoryPrivate *priv    = GET_PRIVATE (object);
  gint64                  memsize = 0;

  memsize += gimp_object_get_memsize (GIMP_OBJECT (priv->container),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (priv->container_obsolete),
                                      gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_data_factory_real_data_init (GimpDataFactory *factory,
                                  GimpContext     *context)
{
  gimp_data_factory_data_load (factory, context, NULL);
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
gimp_data_factory_real_data_refresh (GimpDataFactory *factory,
                                     GimpContext     *context)
{
  GimpDataFactoryPrivate *priv = GET_PRIVATE (factory);
  GHashTable             *cache;

  gimp_container_freeze (priv->container);

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

  gimp_container_thaw (priv->container);
}

static void
gimp_data_factory_real_data_save (GimpDataFactory *factory)
{
  GimpDataFactoryPrivate *priv = GET_PRIVATE (factory);
  GList                  *dirty = NULL;
  GList                  *list;
  GFile                  *writable_dir;
  GError                 *error = NULL;

  for (list = GIMP_LIST (priv->container)->queue->head;
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
      gimp_message (priv->gimp, NULL, GIMP_MESSAGE_ERROR,
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
              gimp_message (priv->gimp, NULL, GIMP_MESSAGE_ERROR,
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

static void
gimp_data_factory_real_data_free (GimpDataFactory *factory)
{
  gimp_container_freeze (factory->priv->container);

  gimp_data_factory_data_foreach (factory, TRUE,
                                  gimp_data_factory_remove_cb, NULL);

  gimp_container_thaw (factory->priv->container);
}

static GimpData *
gimp_data_factory_real_data_new (GimpDataFactory *factory,
                                 GimpContext     *context,
                                 const gchar     *name)
{
  GimpDataFactoryPrivate *priv = GET_PRIVATE (factory);

  if (priv->data_new_func)
    {
      GimpData *data = priv->data_new_func (context, name);

      if (data)
        {
          gimp_container_add (priv->container, GIMP_OBJECT (data));
          g_object_unref (data);

          return data;
        }

      g_warning ("%s: GimpDataFactory::data_new_func() returned NULL",
                 G_STRFUNC);
    }

  return NULL;
}

static GimpData *
gimp_data_factory_real_data_duplicate (GimpDataFactory *factory,
                                       GimpData        *data)
{
  GimpDataFactoryPrivate *priv = GET_PRIVATE (factory);
  GimpData               *new_data;

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

      gimp_container_add (priv->container, GIMP_OBJECT (new_data));
      g_object_unref (new_data);
    }

  return new_data;
}

static gboolean
gimp_data_factory_real_data_delete (GimpDataFactory  *factory,
                                    GimpData         *data,
                                    gboolean          delete_from_disk,
                                    GError          **error)
{
  GimpDataFactoryPrivate *priv   = GET_PRIVATE (factory);
  gboolean                retval = TRUE;

  if (gimp_container_have (priv->container, GIMP_OBJECT (data)))
    {
      g_object_ref (data);

      gimp_container_remove (priv->container, GIMP_OBJECT (data));

      if (delete_from_disk && gimp_data_get_file (data))
        retval = gimp_data_delete_from_disk (data, error);

      g_object_unref (data);
    }

  return retval;
}


/*  public functions  */

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

  factory = g_object_new (GIMP_TYPE_DATA_FACTORY,
                          "gimp",                   gimp,
                          "data-type",              data_type,
                          "path-property-name",     path_property_name,
                          "writable-property-name", writable_property_name,
                          NULL);

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
  GimpDataFactoryPrivate *priv = GET_PRIVATE (factory);
  gchar                  *signal_name;

  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  /*  Always freeze() and thaw() the container around initialization,
   *  even if no_data, the thaw() will implicitly make GimpContext
   *  create the standard data that serves as fallback.
   */
  gimp_container_freeze (priv->container);

  if (! no_data)
    {
      if (priv->gimp->be_verbose)
        {
          const gchar *name = gimp_object_get_name (factory);

          g_print ("Loading '%s' data\n", name ? name : "???");
        }

      GIMP_DATA_FACTORY_GET_CLASS (factory)->data_init (factory, context);
    }

  gimp_container_thaw (priv->container);

  signal_name = g_strdup_printf ("notify::%s", priv->path_property_name);
  g_signal_connect_object (priv->gimp->config, signal_name,
                           G_CALLBACK (gimp_data_factory_path_notify),
                           factory, 0);
  g_free (signal_name);
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

void
gimp_data_factory_data_refresh (GimpDataFactory *factory,
                                GimpContext     *context)
{
  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  GIMP_DATA_FACTORY_GET_CLASS (factory)->data_refresh (factory, context);
}

void
gimp_data_factory_data_save (GimpDataFactory *factory)
{
  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  if (! gimp_container_is_empty (factory->priv->container))
    GIMP_DATA_FACTORY_GET_CLASS (factory)->data_save (factory);
}

void
gimp_data_factory_data_free (GimpDataFactory *factory)
{
  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  if (! gimp_container_is_empty (factory->priv->container))
    GIMP_DATA_FACTORY_GET_CLASS (factory)->data_free (factory);
}

GimpAsyncSet *
gimp_data_factory_get_async_set (GimpDataFactory *factory)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);

  return factory->priv->async_set;
}

gboolean
gimp_data_factory_data_wait (GimpDataFactory *factory)
{
  GimpDataFactoryPrivate *priv;
  GimpWaitable           *waitable;

  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), FALSE);

  priv = GET_PRIVATE (factory);

  /* don't allow cancellation for now */
  waitable = gimp_uncancelable_waitable_new (GIMP_WAITABLE (priv->async_set));

  gimp_wait (priv->gimp, waitable,
             _("Loading fonts (this may take a while...)"));

  g_object_unref (waitable);

  return TRUE;
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

  return GIMP_DATA_FACTORY_GET_CLASS (factory)->data_new (factory, context,
                                                          name);
}

GimpData *
gimp_data_factory_data_duplicate (GimpDataFactory *factory,
                                  GimpData        *data)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (GIMP_IS_DATA (data), NULL);

  return GIMP_DATA_FACTORY_GET_CLASS (factory)->data_duplicate (factory, data);
}

gboolean
gimp_data_factory_data_delete (GimpDataFactory  *factory,
                               GimpData         *data,
                               gboolean          delete_from_disk,
                               GError          **error)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), FALSE);
  g_return_val_if_fail (GIMP_IS_DATA (data), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return GIMP_DATA_FACTORY_GET_CLASS (factory)->data_delete (factory, data,
                                                             delete_from_disk,
                                                             error);
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

GList *
gimp_data_factory_get_data_path (GimpDataFactory *factory)
{
  GimpDataFactoryPrivate *priv = GET_PRIVATE (factory);
  gchar                  *path = NULL;
  GList                  *list = NULL;

  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);

  g_object_get (priv->gimp->config,
                priv->path_property_name, &path,
                NULL);

  if (path)
    {
      list = gimp_config_path_expand_to_files (path, NULL);
      g_free (path);
    }

  return list;
}

GList *
gimp_data_factory_get_data_path_writable (GimpDataFactory *factory)
{
  GimpDataFactoryPrivate *priv = GET_PRIVATE (factory);
  gchar                  *path = NULL;
  GList                  *list = NULL;

  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);

  g_object_get (priv->gimp->config,
                priv->writable_property_name, &path,
                NULL);

  if (path)
    {
      list = gimp_config_path_expand_to_files (path, NULL);
      g_free (path);
    }

  return list;
}


/*  private functions  */

static void
gimp_data_factory_path_notify (GObject          *object,
                               const GParamSpec *pspec,
                               GimpDataFactory  *factory)
{
  GimpDataFactoryPrivate *priv = GET_PRIVATE (factory);

  gimp_set_busy (priv->gimp);

  gimp_data_factory_data_refresh (factory, gimp_get_user_context (priv->gimp));

  gimp_unset_busy (priv->gimp);
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

      gimp_data_factory_load_directory (factory, context, cache,
                                        dir_writable,
                                        list->data,
                                        list->data);
    }

  g_list_free_full (path,          (GDestroyNotify) g_object_unref);
  g_list_free_full (writable_path, (GDestroyNotify) g_object_unref);
}

static GFile *
gimp_data_factory_get_save_dir (GimpDataFactory  *factory,
                                GError          **error)
{
  GList *path;
  GList *writable_path;
  GFile *writable_dir = NULL;

  path          = gimp_data_factory_get_data_path          (factory);
  writable_path = gimp_data_factory_get_data_path_writable (factory);

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
