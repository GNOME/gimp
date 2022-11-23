/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmainputdevicestore-gudev.c
 * Input device store based on GUdev, the hardware abstraction layer.
 * Copyright (C) 2007  Sven Neumann <sven@ligma.org>
 *               2011  Michael Natterer <mitch@ligma.org>
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

#include <gtk/gtk.h>

#include "ligmainputdevicestore.h"

#include "libligmamodule/ligmamodule.h"


#ifdef HAVE_LIBGUDEV

#include <gudev/gudev.h>

enum
{
  COLUMN_IDENTIFIER,
  COLUMN_LABEL,
  COLUMN_DEVICE_FILE,
  NUM_COLUMNS
};

enum
{
  PROP_0,
  PROP_CONSTRUCT_ERROR
};

enum
{
  DEVICE_ADDED,
  DEVICE_REMOVED,
  LAST_SIGNAL
};

typedef struct _LigmaInputDeviceStoreClass LigmaInputDeviceStoreClass;

struct _LigmaInputDeviceStore
{
  GtkListStore  parent_instance;

  GUdevClient  *client;
  GError       *error;
};


struct _LigmaInputDeviceStoreClass
{
  GtkListStoreClass   parent_class;

  void  (* device_added)   (LigmaInputDeviceStore *store,
                            const gchar          *identifier);
  void  (* device_removed) (LigmaInputDeviceStore *store,
                            const gchar          *identifier);
};


static void      ligma_input_device_store_finalize   (GObject              *object);

static gboolean  ligma_input_device_store_add        (LigmaInputDeviceStore *store,
                                                     GUdevDevice          *device);
static gboolean  ligma_input_device_store_remove     (LigmaInputDeviceStore *store,
                                                     GUdevDevice          *device);

static void      ligma_input_device_store_uevent     (GUdevClient          *client,
                                                     const gchar          *action,
                                                     GUdevDevice          *device,
                                                     LigmaInputDeviceStore *store);


G_DEFINE_DYNAMIC_TYPE (LigmaInputDeviceStore, ligma_input_device_store,
                       GTK_TYPE_LIST_STORE)

static guint store_signals[LAST_SIGNAL] = { 0 };


void
ligma_input_device_store_register_types (GTypeModule *module)
{
  ligma_input_device_store_register_type (module);
}

static void
ligma_input_device_store_class_init (LigmaInputDeviceStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  store_signals[DEVICE_ADDED] =
    g_signal_new ("device-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaInputDeviceStoreClass, device_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  store_signals[DEVICE_REMOVED] =
    g_signal_new ("device-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaInputDeviceStoreClass, device_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  object_class->finalize = ligma_input_device_store_finalize;

  klass->device_added    = NULL;
  klass->device_removed  = NULL;
}

static void
ligma_input_device_store_class_finalize (LigmaInputDeviceStoreClass *klass)
{
}

static void
ligma_input_device_store_init (LigmaInputDeviceStore *store)
{
  GType        types[]      = { G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING };
  const gchar *subsystems[] = { "input", NULL };
  GList       *devices;
  GList       *list;

  gtk_list_store_set_column_types (GTK_LIST_STORE (store),
                                   G_N_ELEMENTS (types), types);

  store->client = g_udev_client_new (subsystems);

  devices = g_udev_client_query_by_subsystem (store->client, "input");

  for (list = devices; list; list = g_list_next (list))
    {
      GUdevDevice *device = list->data;

      ligma_input_device_store_add (store, device);
      g_object_unref (device);
    }

  g_list_free (devices);

  g_signal_connect (store->client, "uevent",
                    G_CALLBACK (ligma_input_device_store_uevent),
                    store);
}

static void
ligma_input_device_store_finalize (GObject *object)
{
  LigmaInputDeviceStore *store = LIGMA_INPUT_DEVICE_STORE (object);

  if (store->client)
    {
      g_object_unref (store->client);
      store->client = NULL;
    }

  if (store->error)
    {
      g_error_free (store->error);
      store->error = NULL;
    }

  G_OBJECT_CLASS (ligma_input_device_store_parent_class)->finalize (object);
}

static gboolean
ligma_input_device_store_lookup (LigmaInputDeviceStore *store,
                                const gchar          *identifier,
                                GtkTreeIter          *iter)
{
  GtkTreeModel *model = GTK_TREE_MODEL (store);
  GValue        value = G_VALUE_INIT;
  gboolean      iter_valid;

  for (iter_valid = gtk_tree_model_get_iter_first (model, iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, iter))
    {
      const gchar *str;

      gtk_tree_model_get_value (model, iter, COLUMN_IDENTIFIER, &value);

      str = g_value_get_string (&value);

      if (strcmp (str, identifier) == 0)
        {
          g_value_unset (&value);
          break;
        }

      g_value_unset (&value);
    }

  return iter_valid;
}

/*  insert in alphabetic order  */
static void
ligma_input_device_store_insert (LigmaInputDeviceStore *store,
                                const gchar          *identifier,
                                const gchar          *label,
                                const gchar          *device_file)
{
  GtkTreeModel *model = GTK_TREE_MODEL (store);
  GtkTreeIter   iter;
  GValue        value = G_VALUE_INIT;
  gint          pos   = 0;
  gboolean      iter_valid;

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter), pos++)
    {
      const gchar *str;

      gtk_tree_model_get_value (model, &iter, COLUMN_LABEL, &value);

      str = g_value_get_string (&value);

      if (g_utf8_collate (label, str) < 0)
        {
          g_value_unset (&value);
          break;
        }

      g_value_unset (&value);
    }

  gtk_list_store_insert_with_values (GTK_LIST_STORE (store), &iter, pos,
                                     COLUMN_IDENTIFIER,  identifier,
                                     COLUMN_LABEL,       label,
                                     COLUMN_DEVICE_FILE, device_file,
                                     -1);
}

static gboolean
ligma_input_device_store_add (LigmaInputDeviceStore *store,
                             GUdevDevice          *device)
{
  const gchar *device_file = g_udev_device_get_device_file (device);
#if 0
  const gchar *path        = g_udev_device_get_sysfs_path (device);
#endif
  const gchar *name        = g_udev_device_get_sysfs_attr (device, "name");

#if 0
  g_printerr ("\ndevice added: %s, %s, %s\n",
              name ? name : "NULL",
              device_file ? device_file : "NULL",
              path);
#endif

  if (device_file)
    {
      if (name)
        {
          GtkTreeIter unused;

          if (! ligma_input_device_store_lookup (store, name, &unused))
            {
              ligma_input_device_store_insert (store, name, name, device_file);

              g_signal_emit (store, store_signals[DEVICE_ADDED], 0,
                             name);

              return TRUE;
            }
        }
      else
        {
          GUdevDevice *parent = g_udev_device_get_parent (device);

          if (parent)
            {
              const gchar *parent_name;

              parent_name = g_udev_device_get_sysfs_attr (parent, "name");

              if (parent_name)
                {
                  GtkTreeIter unused;

                  if (! ligma_input_device_store_lookup (store, parent_name,
                                                        &unused))
                    {
                      ligma_input_device_store_insert (store,
                                                      parent_name, parent_name,
                                                      device_file);

                      g_signal_emit (store, store_signals[DEVICE_ADDED], 0,
                                     parent_name);

                      g_object_unref (parent);
                      return TRUE;
                    }
                }

              g_object_unref (parent);
            }
        }
    }

  return FALSE;
}

static gboolean
ligma_input_device_store_remove (LigmaInputDeviceStore *store,
                                GUdevDevice          *device)
{
  const gchar *name = g_udev_device_get_sysfs_attr (device, "name");
  GtkTreeIter  iter;

  if (name)
    {
      if (ligma_input_device_store_lookup (store, name, &iter))
        {
          gtk_list_store_remove (GTK_LIST_STORE (store), &iter);

          g_signal_emit (store, store_signals[DEVICE_REMOVED], 0, name);

          return TRUE;
        }
    }

  return FALSE;
}

static void
ligma_input_device_store_uevent (GUdevClient          *client,
                                const gchar          *action,
                                GUdevDevice          *device,
                                LigmaInputDeviceStore *store)
{
  if (! strcmp (action, "add"))
    {
      ligma_input_device_store_add (store, device);
    }
  else if (! strcmp (action, "remove"))
    {
      ligma_input_device_store_remove (store, device);
    }
}

LigmaInputDeviceStore *
ligma_input_device_store_new (void)
{
  return g_object_new (LIGMA_TYPE_INPUT_DEVICE_STORE, NULL);
}

gchar *
ligma_input_device_store_get_device_file (LigmaInputDeviceStore *store,
                                         const gchar          *identifier)
{
  GtkTreeIter iter;

  g_return_val_if_fail (LIGMA_IS_INPUT_DEVICE_STORE (store), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);

  if (! store->client)
    return NULL;

  if (ligma_input_device_store_lookup (store, identifier, &iter))
    {
      GtkTreeModel *model = GTK_TREE_MODEL (store);
      gchar        *device_file;

      gtk_tree_model_get (model, &iter,
                          COLUMN_DEVICE_FILE, &device_file,
                          -1);

      return device_file;
    }

  return NULL;
}

GError *
ligma_input_device_store_get_error (LigmaInputDeviceStore  *store)
{
  g_return_val_if_fail (LIGMA_IS_INPUT_DEVICE_STORE (store), NULL);

  return store->error ? g_error_copy (store->error) : NULL;
}

#else /* HAVE_LIBGUDEV */

void
ligma_input_device_store_register_types (GTypeModule *module)
{
}

GType
ligma_input_device_store_get_type (void)
{
  return G_TYPE_NONE;
}

LigmaInputDeviceStore *
ligma_input_device_store_new (void)
{
  return NULL;
}

gchar *
ligma_input_device_store_get_device_file (LigmaInputDeviceStore *store,
                                         const gchar          *identifier)
{
  return NULL;
}

GError *
ligma_input_device_store_get_error (LigmaInputDeviceStore  *store)
{
  return NULL;
}

#endif /* HAVE_LIBGUDEV */
