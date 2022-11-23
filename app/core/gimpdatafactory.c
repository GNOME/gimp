/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadatafactory.c
 * Copyright (C) 2001-2018 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "ligma.h"
#include "ligma-utils.h"
#include "ligmaasyncset.h"
#include "ligmacancelable.h"
#include "ligmacontext.h"
#include "ligmadata.h"
#include "ligmadatafactory.h"
#include "ligmalist.h"
#include "ligmauncancelablewaitable.h"
#include "ligmawaitable.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_LIGMA,
  PROP_DATA_TYPE,
  PROP_PATH_PROPERTY_NAME,
  PROP_WRITABLE_PROPERTY_NAME,
  PROP_EXT_PROPERTY_NAME,
  PROP_NEW_FUNC,
  PROP_GET_STANDARD_FUNC
};


struct _LigmaDataFactoryPrivate
{
  Ligma                    *ligma;

  GType                    data_type;
  LigmaContainer           *container;
  LigmaContainer           *container_obsolete;

  gchar                   *path_property_name;
  gchar                   *writable_property_name;
  gchar                   *ext_property_name;

  LigmaDataNewFunc          data_new_func;
  LigmaDataGetStandardFunc  data_get_standard_func;

  LigmaAsyncSet            *async_set;
};

#define GET_PRIVATE(obj) (((LigmaDataFactory *) (obj))->priv)


static void       ligma_data_factory_constructed         (GObject             *object);
static void       ligma_data_factory_set_property        (GObject             *object,
                                                         guint                property_id,
                                                         const GValue        *value,
                                                         GParamSpec          *pspec);
static void       ligma_data_factory_get_property        (GObject             *object,
                                                         guint                property_id,
                                                         GValue              *value,
                                                         GParamSpec          *pspec);
static void       ligma_data_factory_finalize            (GObject             *object);

static gint64     ligma_data_factory_get_memsize         (LigmaObject          *object,
                                                         gint64              *gui_size);

static void       ligma_data_factory_real_data_save      (LigmaDataFactory     *factory);
static void       ligma_data_factory_real_data_cancel    (LigmaDataFactory     *factory);
static LigmaData * ligma_data_factory_real_data_duplicate (LigmaDataFactory     *factory,
                                                         LigmaData            *data);
static gboolean   ligma_data_factory_real_data_delete    (LigmaDataFactory     *factory,
                                                         LigmaData            *data,
                                                         gboolean             delete_from_disk,
                                                         GError             **error);

static void       ligma_data_factory_path_notify         (GObject             *object,
                                                         const GParamSpec    *pspec,
                                                         LigmaDataFactory     *factory);
static GFile    * ligma_data_factory_get_save_dir        (LigmaDataFactory     *factory,
                                                         GError             **error);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (LigmaDataFactory, ligma_data_factory,
                                     LIGMA_TYPE_OBJECT)

#define parent_class ligma_data_factory_parent_class


static void
ligma_data_factory_class_init (LigmaDataFactoryClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);

  object_class->constructed      = ligma_data_factory_constructed;
  object_class->set_property     = ligma_data_factory_set_property;
  object_class->get_property     = ligma_data_factory_get_property;
  object_class->finalize         = ligma_data_factory_finalize;

  ligma_object_class->get_memsize = ligma_data_factory_get_memsize;

  klass->data_init               = NULL;
  klass->data_refresh            = NULL;
  klass->data_save               = ligma_data_factory_real_data_save;
  klass->data_cancel             = ligma_data_factory_real_data_cancel;
  klass->data_duplicate          = ligma_data_factory_real_data_duplicate;
  klass->data_delete             = ligma_data_factory_real_data_delete;

  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma", NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DATA_TYPE,
                                   g_param_spec_gtype ("data-type", NULL, NULL,
                                                       LIGMA_TYPE_DATA,
                                                       LIGMA_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PATH_PROPERTY_NAME,
                                   g_param_spec_string ("path-property-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_WRITABLE_PROPERTY_NAME,
                                   g_param_spec_string ("writable-property-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_EXT_PROPERTY_NAME,
                                   g_param_spec_string ("ext-property-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_NEW_FUNC,
                                   g_param_spec_pointer ("new-func",
                                                         NULL, NULL,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_GET_STANDARD_FUNC,
                                   g_param_spec_pointer ("get-standard-func",
                                                         NULL, NULL,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_data_factory_init (LigmaDataFactory *factory)
{
  factory->priv = ligma_data_factory_get_instance_private (factory);

  factory->priv->async_set = ligma_async_set_new ();
}

static void
ligma_data_factory_constructed (GObject *object)
{
  LigmaDataFactoryPrivate *priv = GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_LIGMA (priv->ligma));
  ligma_assert (g_type_is_a (priv->data_type, LIGMA_TYPE_DATA));
  ligma_assert (LIGMA_DATA_FACTORY_GET_CLASS (object)->data_init != NULL);
  ligma_assert (LIGMA_DATA_FACTORY_GET_CLASS (object)->data_refresh != NULL);

  priv->container = ligma_list_new (priv->data_type, TRUE);
  ligma_list_set_sort_func (LIGMA_LIST (priv->container),
                           (GCompareFunc) ligma_data_compare);

  priv->container_obsolete = ligma_list_new (priv->data_type, TRUE);
  ligma_list_set_sort_func (LIGMA_LIST (priv->container_obsolete),
                           (GCompareFunc) ligma_data_compare);
}

static void
ligma_data_factory_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaDataFactoryPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      priv->ligma = g_value_get_object (value); /* don't ref */
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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_data_factory_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaDataFactoryPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      g_value_set_object (value, priv->ligma);
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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_data_factory_finalize (GObject *object)
{
  LigmaDataFactory        *factory = LIGMA_DATA_FACTORY (object);
  LigmaDataFactoryPrivate *priv    = GET_PRIVATE (object);

  if (priv->async_set)
    {
      ligma_data_factory_data_cancel (factory);

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
ligma_data_factory_get_memsize (LigmaObject *object,
                               gint64     *gui_size)
{
  LigmaDataFactoryPrivate *priv    = GET_PRIVATE (object);
  gint64                  memsize = 0;

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (priv->container),
                                      gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (priv->container_obsolete),
                                      gui_size);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
ligma_data_factory_real_data_save (LigmaDataFactory *factory)
{
  LigmaDataFactoryPrivate *priv = GET_PRIVATE (factory);
  GList                  *dirty = NULL;
  GList                  *list;
  GFile                  *writable_dir;
  GError                 *error = NULL;

  for (list = LIGMA_LIST (priv->container)->queue->head;
       list;
       list = g_list_next (list))
    {
      LigmaData *data = list->data;

      if (ligma_data_is_dirty (data) &&
          ligma_data_is_writable (data))
        {
          dirty = g_list_prepend (dirty, data);
        }
    }

  if (! dirty)
    return;

  writable_dir = ligma_data_factory_get_save_dir (factory, &error);

  if (! writable_dir)
    {
      ligma_message (priv->ligma, NULL, LIGMA_MESSAGE_ERROR,
                    _("Failed to save data:\n\n%s"),
                    error->message);
      g_clear_error (&error);

      g_list_free (dirty);

      return;
    }

  for (list = dirty; list; list = g_list_next (list))
    {
      LigmaData *data  = list->data;
      GError   *error = NULL;

      if (! ligma_data_get_file (data))
        ligma_data_create_filename (data, writable_dir);

      if (factory->priv->ligma->be_verbose)
        {
          GFile *file = ligma_data_get_file (data);

          if (file)
            g_print ("Writing dirty data '%s'\n",
                     ligma_file_get_utf8_name (file));
        }

      if (! ligma_data_save (data, &error))
        {
          /*  check if there actually was an error (no error
           *  means the data class does not implement save)
           */
          if (error)
            {
              ligma_message (priv->ligma, NULL, LIGMA_MESSAGE_ERROR,
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
ligma_data_factory_real_data_cancel (LigmaDataFactory *factory)
{
  LigmaDataFactoryPrivate *priv = GET_PRIVATE (factory);

  ligma_cancelable_cancel (LIGMA_CANCELABLE (priv->async_set));
  ligma_waitable_wait     (LIGMA_WAITABLE   (priv->async_set));
}

static LigmaData *
ligma_data_factory_real_data_duplicate (LigmaDataFactory *factory,
                                       LigmaData        *data)
{
  LigmaDataFactoryPrivate *priv = GET_PRIVATE (factory);
  LigmaData               *new_data;

  new_data = ligma_data_duplicate (data);

  if (new_data)
    {
      const gchar *name = ligma_object_get_name (data);
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

      ligma_object_take_name (LIGMA_OBJECT (new_data), new_name);

      ligma_container_add (priv->container, LIGMA_OBJECT (new_data));
      g_object_unref (new_data);
    }

  return new_data;
}

static gboolean
ligma_data_factory_real_data_delete (LigmaDataFactory  *factory,
                                    LigmaData         *data,
                                    gboolean          delete_from_disk,
                                    GError          **error)
{
  if (delete_from_disk && ligma_data_get_file (data))
    return ligma_data_delete_from_disk (data, error);

  return TRUE;
}


/*  public functions  */

void
ligma_data_factory_data_init (LigmaDataFactory *factory,
                             LigmaContext     *context,
                             gboolean         no_data)
{
  LigmaDataFactoryPrivate *priv = GET_PRIVATE (factory);
  gchar                  *signal_name;

  g_return_if_fail (LIGMA_IS_DATA_FACTORY (factory));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  /*  Always freeze() and thaw() the container around initialization,
   *  even if no_data, the thaw() will implicitly make LigmaContext
   *  create the standard data that serves as fallback.
   */
  ligma_container_freeze (priv->container);

  if (! no_data)
    {
      if (priv->ligma->be_verbose)
        {
          const gchar *name = ligma_object_get_name (factory);

          g_print ("Loading '%s' data\n", name ? name : "???");
        }

      LIGMA_DATA_FACTORY_GET_CLASS (factory)->data_init (factory, context);
    }

  ligma_container_thaw (priv->container);

  signal_name = g_strdup_printf ("notify::%s", priv->path_property_name);
  g_signal_connect_object (priv->ligma->config, signal_name,
                           G_CALLBACK (ligma_data_factory_path_notify),
                           factory, 0);
  g_free (signal_name);

  signal_name = g_strdup_printf ("notify::%s", priv->ext_property_name);
  g_signal_connect_object (priv->ligma->extension_manager, signal_name,
                           G_CALLBACK (ligma_data_factory_path_notify),
                           factory, 0);
  g_free (signal_name);
}

static void
ligma_data_factory_clean_cb (LigmaDataFactory *factory,
                            LigmaData        *data,
                            gpointer         user_data)
{
  if (ligma_data_is_dirty (data))
    ligma_data_clean (data);
}

void
ligma_data_factory_data_clean (LigmaDataFactory *factory)
{
  g_return_if_fail (LIGMA_IS_DATA_FACTORY (factory));

  ligma_data_factory_data_foreach (factory, TRUE,
                                  ligma_data_factory_clean_cb, NULL);
}

void
ligma_data_factory_data_refresh (LigmaDataFactory *factory,
                                LigmaContext     *context)
{
  g_return_if_fail (LIGMA_IS_DATA_FACTORY (factory));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  LIGMA_DATA_FACTORY_GET_CLASS (factory)->data_refresh (factory, context);
}

void
ligma_data_factory_data_save (LigmaDataFactory *factory)
{
  g_return_if_fail (LIGMA_IS_DATA_FACTORY (factory));

  if (! ligma_container_is_empty (factory->priv->container))
    LIGMA_DATA_FACTORY_GET_CLASS (factory)->data_save (factory);
}

static void
ligma_data_factory_data_free_foreach (LigmaDataFactory *factory,
                                     LigmaData        *data,
                                     gpointer         user_data)
{
  ligma_container_remove (factory->priv->container, LIGMA_OBJECT (data));
}

void
ligma_data_factory_data_free (LigmaDataFactory *factory)
{
  g_return_if_fail (LIGMA_IS_DATA_FACTORY (factory));

  ligma_data_factory_data_cancel (factory);

  if (! ligma_container_is_empty (factory->priv->container))
    {
      ligma_container_freeze (factory->priv->container);

      ligma_data_factory_data_foreach (factory, TRUE,
                                      ligma_data_factory_data_free_foreach,
                                      NULL);

      ligma_container_thaw (factory->priv->container);
    }
}

LigmaAsyncSet *
ligma_data_factory_get_async_set (LigmaDataFactory *factory)
{
  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), NULL);

  return factory->priv->async_set;
}

gboolean
ligma_data_factory_data_wait (LigmaDataFactory *factory)
{
  LigmaDataFactoryPrivate *priv;
  LigmaWaitable           *waitable;

  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), FALSE);

  priv = GET_PRIVATE (factory);

  /* don't allow cancellation for now */
  waitable = ligma_uncancelable_waitable_new (LIGMA_WAITABLE (priv->async_set));

  ligma_wait (priv->ligma, waitable,
             _("Loading fonts (this may take a while...)"));

  g_object_unref (waitable);

  return TRUE;
}

void
ligma_data_factory_data_cancel (LigmaDataFactory *factory)
{
  g_return_if_fail (LIGMA_IS_DATA_FACTORY (factory));

  LIGMA_DATA_FACTORY_GET_CLASS (factory)->data_cancel (factory);
}

gboolean
ligma_data_factory_has_data_new_func (LigmaDataFactory *factory)
{
  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), FALSE);

  return factory->priv->data_new_func != NULL;
}

LigmaData *
ligma_data_factory_data_new (LigmaDataFactory *factory,
                            LigmaContext     *context,
                            const gchar     *name)
{
  LigmaDataFactoryPrivate *priv;

  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (*name != '\0', NULL);

  priv = GET_PRIVATE (factory);

  if (priv->data_new_func)
    {
      LigmaData *data = priv->data_new_func (context, name);

      if (data)
        {
          ligma_container_add (priv->container, LIGMA_OBJECT (data));
          g_object_unref (data);

          return data;
        }

      g_warning ("%s: LigmaDataFactory::data_new_func() returned NULL",
                 G_STRFUNC);
    }

  return NULL;
}

LigmaData *
ligma_data_factory_data_get_standard (LigmaDataFactory *factory,
                                     LigmaContext     *context)
{
  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  if (factory->priv->data_get_standard_func)
    return factory->priv->data_get_standard_func (context);

  return NULL;
}

LigmaData *
ligma_data_factory_data_duplicate (LigmaDataFactory *factory,
                                  LigmaData        *data)
{
  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (LIGMA_IS_DATA (data), NULL);

  return LIGMA_DATA_FACTORY_GET_CLASS (factory)->data_duplicate (factory, data);
}

gboolean
ligma_data_factory_data_delete (LigmaDataFactory  *factory,
                               LigmaData         *data,
                               gboolean          delete_from_disk,
                               GError          **error)
{
  LigmaDataFactoryPrivate *priv;
  gboolean                retval = TRUE;

  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), FALSE);
  g_return_val_if_fail (LIGMA_IS_DATA (data), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  priv = GET_PRIVATE (factory);

  if (ligma_container_have (priv->container, LIGMA_OBJECT (data)))
    {
      g_object_ref (data);

      ligma_container_remove (priv->container, LIGMA_OBJECT (data));

      retval = LIGMA_DATA_FACTORY_GET_CLASS (factory)->data_delete (factory, data,
                                                                   delete_from_disk,
                                                                   error);

      g_object_unref (data);
    }

  return retval;
}

gboolean
ligma_data_factory_data_save_single (LigmaDataFactory  *factory,
                                    LigmaData         *data,
                                    GError          **error)
{
  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), FALSE);
  g_return_val_if_fail (LIGMA_IS_DATA (data), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! ligma_data_is_dirty (data))
    return TRUE;

  if (! ligma_data_get_file (data))
    {
      GFile  *writable_dir;
      GError *my_error = NULL;

      writable_dir = ligma_data_factory_get_save_dir (factory, &my_error);

      if (! writable_dir)
        {
          g_set_error (error, LIGMA_DATA_ERROR, 0,
                       _("Failed to save data:\n\n%s"),
                       my_error->message);
          g_clear_error (&my_error);

          return FALSE;
        }

      ligma_data_create_filename (data, writable_dir);

      g_object_unref (writable_dir);
    }

  if (! ligma_data_is_writable (data))
    return FALSE;

  if (factory->priv->ligma->be_verbose)
    {
      GFile *file = ligma_data_get_file (data);

      if (file)
        g_print ("Writing dirty data '%s'\n",
                 ligma_file_get_utf8_name (file));
    }

  if (! ligma_data_save (data, error))
    {
      /*  check if there actually was an error (no error
       *  means the data class does not implement save)
       */
      if (! error)
        g_set_error (error, LIGMA_DATA_ERROR, 0,
                     _("Failed to save data:\n\n%s"),
                     "Data class does not implement saving");

      return FALSE;
    }

  return TRUE;
}

void
ligma_data_factory_data_foreach (LigmaDataFactory     *factory,
                                gboolean             skip_internal,
                                LigmaDataForeachFunc  callback,
                                gpointer             user_data)
{
  GList *list;

  g_return_if_fail (LIGMA_IS_DATA_FACTORY (factory));
  g_return_if_fail (callback != NULL);

  list = LIGMA_LIST (factory->priv->container)->queue->head;

  while (list)
    {
      GList *next = g_list_next (list);

      if (! (skip_internal && ligma_data_is_internal (list->data)))
        callback (factory, list->data, user_data);

      list = next;
    }
}

Ligma *
ligma_data_factory_get_ligma (LigmaDataFactory *factory)
{
  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), NULL);

  return factory->priv->ligma;
}

GType
ligma_data_factory_get_data_type (LigmaDataFactory *factory)
{
  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), G_TYPE_NONE);

  return ligma_container_get_children_type (factory->priv->container);
}

LigmaContainer *
ligma_data_factory_get_container (LigmaDataFactory *factory)
{
  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), NULL);

  return factory->priv->container;
}

LigmaContainer *
ligma_data_factory_get_container_obsolete (LigmaDataFactory *factory)
{
  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), NULL);

  return factory->priv->container_obsolete;
}

GList *
ligma_data_factory_get_data_path (LigmaDataFactory *factory)
{
  LigmaDataFactoryPrivate *priv = GET_PRIVATE (factory);
  gchar                  *path = NULL;
  GList                  *list = NULL;

  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), NULL);

  g_object_get (priv->ligma->config,
                priv->path_property_name, &path,
                NULL);

  if (path)
    {
      list = ligma_config_path_expand_to_files (path, NULL);
      g_free (path);
    }

  return list;
}

GList *
ligma_data_factory_get_data_path_writable (LigmaDataFactory *factory)
{
  LigmaDataFactoryPrivate *priv = GET_PRIVATE (factory);
  gchar                  *path = NULL;
  GList                  *list = NULL;

  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), NULL);

  g_object_get (priv->ligma->config,
                priv->writable_property_name, &path,
                NULL);

  if (path)
    {
      list = ligma_config_path_expand_to_files (path, NULL);
      g_free (path);
    }

  return list;
}

const GList *
ligma_data_factory_get_data_path_ext (LigmaDataFactory *factory)
{
  LigmaDataFactoryPrivate *priv = GET_PRIVATE (factory);
  GList                  *list = NULL;

  g_return_val_if_fail (LIGMA_IS_DATA_FACTORY (factory), NULL);

  g_object_get (priv->ligma->extension_manager,
                priv->ext_property_name, &list,
                NULL);

  return list;
}


/*  private functions  */

static void
ligma_data_factory_path_notify (GObject          *object,
                               const GParamSpec *pspec,
                               LigmaDataFactory  *factory)
{
  LigmaDataFactoryPrivate *priv = GET_PRIVATE (factory);

  ligma_set_busy (priv->ligma);

  ligma_data_factory_data_refresh (factory, ligma_get_user_context (priv->ligma));

  ligma_unset_busy (priv->ligma);
}

static GFile *
ligma_data_factory_get_save_dir (LigmaDataFactory  *factory,
                                GError          **error)
{
  GList *path;
  GList *writable_path;
  GFile *writable_dir = NULL;

  path          = ligma_data_factory_get_data_path          (factory);
  writable_path = ligma_data_factory_get_data_path_writable (factory);

  if (writable_path)
    {
      GList    *list;
      gboolean  found_any = FALSE;

      for (list = writable_path; list; list = g_list_next (list))
        {
          GList *found = g_list_find_custom (path, list->data,
                                             (GCompareFunc) ligma_file_compare);
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
                      g_set_error (error, LIGMA_DATA_ERROR, 0,
                                   _("You have a writable data folder "
                                     "configured (%s), but this folder does "
                                     "not exist. Please create the folder or "
                                     "fix your configuration in the "
                                     "Preferences dialog's 'Folders' section."),
                                   ligma_file_get_utf8_name (dir));
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
          g_set_error (error, LIGMA_DATA_ERROR, 0,
                       _("You have a writable data folder configured, but this "
                         "folder is not part of your data search path. You "
                         "probably edited the ligmarc file manually, "
                         "please fix it in the Preferences dialog's 'Folders' "
                         "section."));
        }
    }
  else
    {
      g_set_error (error, LIGMA_DATA_ERROR, 0,
                   _("You don't have any writable data folder configured."));
    }

  g_list_free_full (path,          (GDestroyNotify) g_object_unref);
  g_list_free_full (writable_path, (GDestroyNotify) g_object_unref);

  return writable_dir;
}
