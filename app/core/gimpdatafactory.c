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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <stdlib.h>

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


enum
{
  PROP_0,
  PROP_GIMP,
  PROP_DATA_TYPE,
  PROP_PATH_PROPERTY_NAME,
  PROP_WRITABLE_PROPERTY_NAME,
  PROP_EXT_PROPERTY_NAME,
  PROP_NEW_FUNC,
  PROP_GET_STANDARD_FUNC,
  PROP_UNIQUE_NAMES
};


struct _GimpDataFactoryPrivate
{
  Gimp                    *gimp;

  GType                    data_type;
  GimpContainer           *container;
  GimpContainer           *container_obsolete;
  gboolean                 unique_names;

  gchar                   *path_property_name;
  gchar                   *writable_property_name;
  gchar                   *ext_property_name;

  GimpDataNewFunc          data_new_func;
  GimpDataGetStandardFunc  data_get_standard_func;

  GimpAsyncSet            *async_set;
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

static void       gimp_data_factory_real_data_save      (GimpDataFactory     *factory);
static void       gimp_data_factory_real_data_cancel    (GimpDataFactory     *factory);
static GimpData * gimp_data_factory_real_data_duplicate (GimpDataFactory     *factory,
                                                         GimpData            *data);
static gboolean   gimp_data_factory_real_data_delete    (GimpDataFactory     *factory,
                                                         GimpData            *data,
                                                         gboolean             delete_from_disk,
                                                         GError             **error);

static void       gimp_data_factory_path_notify         (GObject             *object,
                                                         const GParamSpec    *pspec,
                                                         GimpDataFactory     *factory);
static GFile    * gimp_data_factory_get_save_dir        (GimpDataFactory     *factory,
                                                         GError             **error);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (GimpDataFactory, gimp_data_factory,
                                     GIMP_TYPE_OBJECT)

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

  klass->data_init               = NULL;
  klass->data_refresh            = NULL;
  klass->data_save               = gimp_data_factory_real_data_save;
  klass->data_cancel             = gimp_data_factory_real_data_cancel;
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

  g_object_class_install_property (object_class, PROP_EXT_PROPERTY_NAME,
                                   g_param_spec_string ("ext-property-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_NEW_FUNC,
                                   g_param_spec_pointer ("new-func",
                                                         NULL, NULL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_GET_STANDARD_FUNC,
                                   g_param_spec_pointer ("get-standard-func",
                                                         NULL, NULL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_UNIQUE_NAMES,
                                   g_param_spec_boolean ("unique-names",
                                                         NULL, NULL,
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

}

static void
gimp_data_factory_init (GimpDataFactory *factory)
{
  factory->priv = gimp_data_factory_get_instance_private (factory);

  factory->priv->async_set = gimp_async_set_new ();
}

static void
gimp_data_factory_constructed (GObject *object)
{
  GimpDataFactoryPrivate *priv = GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_GIMP (priv->gimp));
  gimp_assert (g_type_is_a (priv->data_type, GIMP_TYPE_DATA));
  gimp_assert (GIMP_DATA_FACTORY_GET_CLASS (object)->data_init != NULL);
  gimp_assert (GIMP_DATA_FACTORY_GET_CLASS (object)->data_refresh != NULL);

  /* Passing along the "unique names" property to the data container. */
  priv->container = gimp_list_new (priv->data_type, priv->unique_names);
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

    case PROP_EXT_PROPERTY_NAME:
      priv->ext_property_name = g_value_dup_string (value);
      break;

    case PROP_NEW_FUNC:
      priv->data_new_func = g_value_get_pointer (value);
      break;

    case PROP_GET_STANDARD_FUNC:
      priv->data_get_standard_func = g_value_get_pointer (value);
      break;

    case PROP_UNIQUE_NAMES:
      priv->unique_names = g_value_get_boolean (value);
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

    case PROP_EXT_PROPERTY_NAME:
      g_value_set_string (value, priv->ext_property_name);
      break;

    case PROP_NEW_FUNC:
      g_value_set_pointer (value, priv->data_new_func);
      break;

    case PROP_GET_STANDARD_FUNC:
      g_value_set_pointer (value, priv->data_get_standard_func);
      break;

    case PROP_UNIQUE_NAMES:
      g_value_set_boolean (value, priv->unique_names);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_data_factory_finalize (GObject *object)
{
  GimpDataFactory        *factory = GIMP_DATA_FACTORY (object);
  GimpDataFactoryPrivate *priv    = GET_PRIVATE (object);

  if (priv->async_set)
    {
      gimp_data_factory_data_cancel (factory);

      g_clear_object (&priv->async_set);
    }

  g_clear_object (&priv->container);
  g_clear_object (&priv->container_obsolete);

  g_clear_pointer (&priv->path_property_name,     g_free);
  g_clear_pointer (&priv->writable_property_name, g_free);
  g_clear_pointer (&priv->ext_property_name,      g_free);

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

      if (gimp_data_get_image (data))
        continue;

      if (! gimp_data_get_file (data))
        gimp_data_create_filename (data, writable_dir);

      if (factory->priv->gimp->be_verbose)
        {
          GFile *file = gimp_data_get_file (data);

          if (file)
            g_print ("Writing dirty data '%s'\n",
                     gimp_file_get_utf8_name (file));
        }

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
gimp_data_factory_real_data_cancel (GimpDataFactory *factory)
{
  GimpDataFactoryPrivate *priv = GET_PRIVATE (factory);

  gimp_cancelable_cancel (GIMP_CANCELABLE (priv->async_set));
  gimp_waitable_wait     (GIMP_WAITABLE   (priv->async_set));
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
      GError      *error = NULL;

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

      if (! gimp_data_factory_data_save_single (factory, new_data, &error))
        g_critical ("%s: data saving failed: %s", G_STRFUNC, error->message);

      gimp_container_add (priv->container, GIMP_OBJECT (new_data));
      g_object_unref (new_data);
      g_clear_error (&error);
    }

  return new_data;
}

static gboolean
gimp_data_factory_real_data_delete (GimpDataFactory  *factory,
                                    GimpData         *data,
                                    gboolean          delete_from_disk,
                                    GError          **error)
{
  if (delete_from_disk && gimp_data_get_file (data))
    return gimp_data_delete_from_disk (data, error);

  return TRUE;
}


/*  public functions  */

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

  signal_name = g_strdup_printf ("notify::%s", priv->ext_property_name);
  g_signal_connect_object (priv->gimp->extension_manager, signal_name,
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

static void
gimp_data_factory_data_free_foreach (GimpDataFactory *factory,
                                     GimpData        *data,
                                     gpointer         user_data)
{
  gimp_container_remove (factory->priv->container, GIMP_OBJECT (data));
}

void
gimp_data_factory_data_free (GimpDataFactory *factory)
{
  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  gimp_data_factory_data_cancel (factory);

  if (! gimp_container_is_empty (factory->priv->container))
    {
      gimp_container_freeze (factory->priv->container);

      gimp_data_factory_data_foreach (factory, TRUE,
                                      gimp_data_factory_data_free_foreach,
                                      NULL);

      gimp_container_thaw (factory->priv->container);
    }
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

void
gimp_data_factory_data_cancel (GimpDataFactory *factory)
{
  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));

  GIMP_DATA_FACTORY_GET_CLASS (factory)->data_cancel (factory);
}

gboolean
gimp_data_factory_has_data_new_func (GimpDataFactory *factory)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), FALSE);

  return factory->priv->data_new_func != NULL;
}

GimpData *
gimp_data_factory_data_new (GimpDataFactory *factory,
                            GimpContext     *context,
                            const gchar     *name)
{
  GimpDataFactoryPrivate *priv;

  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (*name != '\0', NULL);

  priv = GET_PRIVATE (factory);

  if (priv->data_new_func)
    {
      GimpData *data = priv->data_new_func (context, name);

      if (data)
        {
          GError *error = NULL;

          if (! gimp_data_factory_data_save_single (factory, data, &error))
            g_critical ("%s: data saving failed: %s", G_STRFUNC, error->message);

          gimp_container_add (priv->container, GIMP_OBJECT (data));
          g_object_unref (data);
          g_clear_error (&error);

          return data;
        }

      g_warning ("%s: GimpDataFactory::data_new_func() returned NULL",
                 G_STRFUNC);
    }

  return NULL;
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
  GimpDataFactoryPrivate *priv;
  gboolean                retval = TRUE;

  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), FALSE);
  g_return_val_if_fail (GIMP_IS_DATA (data), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  priv = GET_PRIVATE (factory);

  if (gimp_container_have (priv->container, GIMP_OBJECT (data)))
    {
      g_object_ref (data);

      gimp_container_remove (priv->container, GIMP_OBJECT (data));

      retval = GIMP_DATA_FACTORY_GET_CLASS (factory)->data_delete (factory, data,
                                                                   delete_from_disk,
                                                                   error);

      g_object_unref (data);
    }

  return retval;
}

gboolean
gimp_data_factory_data_save_single (GimpDataFactory  *factory,
                                    GimpData         *data,
                                    GError          **error)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), FALSE);
  g_return_val_if_fail (GIMP_IS_DATA (data), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! gimp_data_is_dirty (data) || gimp_data_get_image (data))
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

  if (factory->priv->gimp->be_verbose)
    {
      GFile *file = gimp_data_get_file (data);

      if (file)
        g_print ("Writing dirty data '%s'\n",
                 gimp_file_get_utf8_name (file));
    }

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

void
gimp_data_factory_data_foreach (GimpDataFactory     *factory,
                                gboolean             skip_internal,
                                GimpDataForeachFunc  callback,
                                gpointer             user_data)
{
  GList *list;

  g_return_if_fail (GIMP_IS_DATA_FACTORY (factory));
  g_return_if_fail (callback != NULL);

  list = GIMP_LIST (factory->priv->container)->queue->head;

  while (list)
    {
      GList *next = g_list_next (list);

      if (! (skip_internal && gimp_data_is_internal (list->data)))
        callback (factory, list->data, user_data);

      list = next;
    }
}

Gimp *
gimp_data_factory_get_gimp (GimpDataFactory *factory)
{
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);

  return factory->priv->gimp;
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

const GList *
gimp_data_factory_get_data_path_ext (GimpDataFactory *factory)
{
  GimpDataFactoryPrivate *priv = GET_PRIVATE (factory);
  GList                  *list = NULL;

  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (factory), NULL);

  g_object_get (priv->gimp->extension_manager,
                priv->ext_property_name, &list,
                NULL);

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
              GFile  *dir         = found->data;
              GError *mkdir_error = NULL;

              found_any = TRUE;

              /* We always try and create but do not check the error code
               * because some errors might be non-fatale. E.g.
               * G_IO_ERROR_EXISTS may mean either that the folder exists
               * (good) or that a file of another type exists (bad).
               * I am also unsure if G_IO_ERROR_NOT_SUPPORTED might happen in
               * cases where creating directories is forbidden but one might
               * already exist.
               * So in the end, we create then check the file type.
               */
              if (! g_file_make_directory_with_parents (dir, NULL, &mkdir_error) &&
                  g_file_query_file_type (dir, G_FILE_QUERY_INFO_NONE,
                                          NULL) != G_FILE_TYPE_DIRECTORY)
                {
                  /*  error out only if this is the last chance  */
                  if (! list->next)
                    {
                      if (mkdir_error)
                        g_set_error (error, GIMP_DATA_ERROR, 0,
                                     _("You have a writable data folder "
                                       "configured (%s), but this folder could "
                                       "not be created: \"%s\"\n\n"
                                       "Please check your configuration in the "
                                       "Preferences dialog's 'Folders' section."),
                                     gimp_file_get_utf8_name (dir),
                                     mkdir_error->message);
                      else
                        g_set_error (error, GIMP_DATA_ERROR, 0,
                                     _("You have a writable data folder "
                                       "configured (%s), but this folder does "
                                       "not exist. Please create the folder or "
                                       "fix your configuration in the "
                                       "Preferences dialog's 'Folders' section."),
                                     gimp_file_get_utf8_name (dir));
                    }

                  g_clear_error (&mkdir_error);
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
