/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpinputdevicestore.c
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
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

#include <string.h>

#include <gtk/gtk.h>

#ifdef HAVE_LIBHAL
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <hal/libhal.h>
#endif

#include "gimpinputdevicestore.h"


#ifdef HAVE_LIBHAL

enum
{
  COLUMN_UDI,
  COLUMN_LABEL,
  NUM_COLUMNS
};

typedef GtkListStoreClass  GimpInputDeviceStoreClass;

struct _GimpInputDeviceStore
{
  GtkListStore   parent_instance;

  LibHalContext *context;
};


static void  gimp_input_device_store_class_init (GimpInputDeviceStoreClass *klass);
static void  gimp_input_device_store_init       (GimpInputDeviceStore *store);
static void  gimp_input_device_store_finalize   (GObject              *object);

static void  gimp_input_device_store_add        (GimpInputDeviceStore *store,
                                                 const gchar          *udi);
static void  gimp_input_device_store_remove     (GimpInputDeviceStore *store,
                                                 const gchar          *udi);

static void  gimp_input_device_store_device_added   (LibHalContext *ctx,
                                                     const char    *udi);
static void  gimp_input_device_store_device_removed (LibHalContext *ctx,
                                                     const char    *udi);


G_DEFINE_TYPE (GimpInputDeviceStore,
               gimp_input_device_store, GTK_TYPE_LIST_STORE)


static void
gimp_input_device_store_class_init (GimpInputDeviceStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_input_device_store_finalize;
}

static void
gimp_input_device_store_init (GimpInputDeviceStore *store)
{
  DBusGConnection *connection;
  GType            types[] = { G_TYPE_STRING, G_TYPE_STRING };
  GError          *error = NULL;

  gtk_list_store_set_column_types (GTK_LIST_STORE (store),
                                   G_N_ELEMENTS (types), types);

  connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);

  if (! connection)
    {
      g_printerr (error->message);
      g_error_free (error);
      return;
    }

  store->context = libhal_ctx_new ();

  libhal_ctx_set_dbus_connection (store->context,
                                  dbus_g_connection_get_connection (connection));
  dbus_g_connection_unref (connection);

  if (libhal_ctx_init (store->context, NULL))
    {
      char **devices;
      int    i, num_devices;

      devices = libhal_find_device_by_capability (store->context, "input",
                                                  &num_devices, NULL);

      for (i = 0; i < num_devices; i++)
        gimp_input_device_store_add (store, devices[i]);

      libhal_free_string_array (devices);

      libhal_ctx_set_user_data (store->context, store);
      libhal_ctx_set_device_added (store->context,
                                   gimp_input_device_store_device_added);
      libhal_ctx_set_device_removed (store->context,
                                     gimp_input_device_store_device_removed);
    }
}

static void
gimp_input_device_store_finalize (GObject *object)
{
  GimpInputDeviceStore *store = GIMP_INPUT_DEVICE_STORE (object);

  if (store->context)
    {
      libhal_ctx_shutdown (store->context, NULL);
      libhal_ctx_free (store->context);
      store->context = NULL;
    }

  G_OBJECT_CLASS (gimp_input_device_store_parent_class)->finalize (object);
}


static gboolean
gimp_input_device_store_lookup (GimpInputDeviceStore *store,
                                const gchar          *udi,
                                GtkTreeIter          *iter)
{
  GtkTreeModel *model = GTK_TREE_MODEL (store);
  GValue        value = { 0, };
  gboolean      iter_valid;

  for (iter_valid = gtk_tree_model_get_iter_first (model, iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, iter))
    {
      const gchar *str;

      gtk_tree_model_get_value (model, iter, COLUMN_UDI, &value);

      str = g_value_get_string (&value);

      if (strcmp (str, udi) == 0)
        {
          g_value_unset (&value);
          break;
        }

      g_value_unset (&value);
    }

  return iter_valid;
}

static void
gimp_input_device_store_add (GimpInputDeviceStore *store,
                             const gchar          *udi)
{
  char **caps;
  gint   i;

  caps = libhal_device_get_property_strlist (store->context,
                                             udi, "info.capabilities",
                                             NULL);

  for (i = 0; caps && caps[i]; i++)
    {
      char *str;

      if (strcmp (caps[i], "input") != 0)
        continue;

      /*  skip "PC Speaker" (why is this an input device at all?)  */
      str = libhal_device_get_property_string (store->context,
                                               udi, "input.physical_device",
                                               NULL);
      if (str)
        {
          gboolean speaker =
            strcmp (str, "/org/freedesktop/Hal/devices/platform_pcspkr") == 0;

          libhal_free_string (str);

          if (speaker)
            continue;
        }

      str = libhal_device_get_property_string (store->context,
                                               udi, "input.product",
                                               NULL);
      if (str)
        {
          GtkTreeIter iter;

          gtk_list_store_append (GTK_LIST_STORE (store), &iter);

          g_printerr ("%s\n", str);

          gtk_list_store_set (GTK_LIST_STORE (store), &iter,
                              COLUMN_UDI,   udi,
                              COLUMN_LABEL, str,
                              -1);

          libhal_free_string (str);
        }
    }

  libhal_free_string_array (caps);
}

static void
gimp_input_device_store_remove (GimpInputDeviceStore *store,
                                const gchar          *udi)
{
  GtkTreeIter  iter;

  if (gimp_input_device_store_lookup (store, udi, &iter))
    gtk_list_store_remove (GTK_LIST_STORE (store), &iter);
}

static void
gimp_input_device_store_device_added (LibHalContext *ctx,
                                      const char    *udi)
{
  GimpInputDeviceStore *store = libhal_ctx_get_user_data (ctx);

  gimp_input_device_store_add (store, udi);
}

static void
gimp_input_device_store_device_removed (LibHalContext *ctx,
                                        const char    *udi)
{
  GimpInputDeviceStore *store = libhal_ctx_get_user_data (ctx);

  gimp_input_device_store_remove (store, udi);
}

GtkListStore *
gimp_input_device_store_new (void)
{
  return g_object_new (GIMP_TYPE_INPUT_DEVICE_STORE, NULL);
}

gchar *
gimp_input_device_store_get_device_file (GimpInputDeviceStore *store,
                                         const gchar          *udi)
{
  GtkTreeIter iter;

  g_return_val_if_fail (GIMP_IS_INPUT_DEVICE_STORE (store), NULL);
  g_return_val_if_fail (udi != NULL, NULL);

  if (gimp_input_device_store_lookup (store, udi, &iter))
    {
      char *str = libhal_device_get_property_string (store->context,
                                                     udi, "input.device",
                                                     NULL);

      if (str)
        {
          gchar *retval = g_strdup (str);

          libhal_free_string (str);

          return retval;
        }
    }

  return NULL;
}

#else

GType
gimp_input_device_store_get_type (void)
{
  return G_TYPE_NONE;
}

GtkListStore *
gimp_input_device_store_new (void)
{
  return NULL;
}

gchar *
gimp_input_device_store_get_device_file (GimpInputDeviceStore *store,
                                         const gchar          *udi)
{
  return NULL;
}

#endif
