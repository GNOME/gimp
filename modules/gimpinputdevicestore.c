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

#ifdef HAVE_DX_DINPUT
#define _WIN32_WINNT 0x0501 
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <rpc.h>

#include <gdk/gdkwin32.h>
#endif

#include "gimpinputdevicestore.h"


#ifdef HAVE_LIBHAL

enum
{
  COLUMN_UDI,
  COLUMN_LABEL,
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

typedef struct _GimpInputDeviceStoreClass GimpInputDeviceStoreClass;

struct _GimpInputDeviceStore
{
  GtkListStore    parent_instance;

  LibHalContext  *context;
  GError         *error;
};


struct _GimpInputDeviceStoreClass
{
  GtkListStoreClass   parent_class;

  void  (*device_added)   (GimpInputDeviceStore *store,
                           const gchar          *udi);
  void  (*device_removed) (GimpInputDeviceStore *store,
                           const gchar          *udi);
};


static void      gimp_input_device_store_class_init (GimpInputDeviceStoreClass *klass);
static void      gimp_input_device_store_init       (GimpInputDeviceStore *store);
static void      gimp_input_device_store_finalize   (GObject              *object);
static gboolean  gimp_input_device_store_add        (GimpInputDeviceStore *store,
                                                     const gchar          *udi);
static gboolean  gimp_input_device_store_remove     (GimpInputDeviceStore *store,
                                                     const gchar          *udi);

static void      gimp_input_device_store_device_added   (LibHalContext *ctx,
                                                         const char    *udi);
static void      gimp_input_device_store_device_removed (LibHalContext *ctx,
                                                         const char    *udi);


GType                     gimp_input_device_store_type = 0;
static GtkListStoreClass *parent_class                 = NULL;
static guint              store_signals[LAST_SIGNAL]   = { 0 };


GType
gimp_input_device_store_get_type (GTypeModule *module)
{
  if (! gimp_input_device_store_type)
    {
      const GTypeInfo info =
      {
        sizeof (GimpInputDeviceStoreClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_input_device_store_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpInputDeviceStore),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_input_device_store_init
      };

      gimp_input_device_store_type =
        g_type_module_register_type (module, GTK_TYPE_LIST_STORE,
                                     "GimpInputDeviceStore",
                                     &info, 0);
    }

  return gimp_input_device_store_type;
}


static void
gimp_input_device_store_class_init (GimpInputDeviceStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  store_signals[DEVICE_ADDED] =
    g_signal_new ("device-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpInputDeviceStoreClass, device_added),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  store_signals[DEVICE_REMOVED] =
    g_signal_new ("device-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpInputDeviceStoreClass, device_removed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  object_class->finalize = gimp_input_device_store_finalize;

  klass->device_added    = NULL;
  klass->device_removed  = NULL;
}

static void
gimp_input_device_store_init (GimpInputDeviceStore *store)
{
  GType            types[] = { G_TYPE_STRING, G_TYPE_STRING };
  DBusGConnection *connection;
  DBusError        dbus_error;

  gtk_list_store_set_column_types (GTK_LIST_STORE (store),
                                   G_N_ELEMENTS (types), types);

  connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &store->error);

  if (! connection)
    return;

  store->context = libhal_ctx_new ();

  libhal_ctx_set_dbus_connection (store->context,
                                  dbus_g_connection_get_connection (connection));
  dbus_g_connection_unref (connection);

  dbus_error_init (&dbus_error);

  if (libhal_ctx_init (store->context, &dbus_error))
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
  else
    {
      if (dbus_error_is_set (&dbus_error))
        {
          dbus_set_g_error (&store->error, &dbus_error);
          dbus_error_free (&dbus_error);
        }
      else
        {
          g_set_error (&store->error, 0, 0, "Unable to connect to hald");
        }

      libhal_ctx_free (store->context);
      store->context = NULL;
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

  if (store->error)
    {
      g_error_free (store->error);
      store->error = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
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

/*  insert in alphabetic order  */
static void
gimp_input_device_store_insert (GimpInputDeviceStore *store,
                                const gchar          *udi,
                                const gchar          *label)
{
  GtkTreeModel *model = GTK_TREE_MODEL (store);
  GtkTreeIter   iter;
  GValue        value = { 0, };
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
                                     COLUMN_UDI,   udi,
                                     COLUMN_LABEL, label,
                                     -1);
}

static gboolean
gimp_input_device_store_add (GimpInputDeviceStore *store,
                             const gchar          *udi)
{
  gboolean   added = FALSE;
  char     **caps;
  gint       i;

  caps = libhal_device_get_property_strlist (store->context,
                                             udi, "info.capabilities",
                                             NULL);

  for (i = 0; caps && caps[i] && !added; i++)
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
          gimp_input_device_store_insert (store, udi, str);

          libhal_free_string (str);

          added = TRUE;
        }
    }

  libhal_free_string_array (caps);

  return added;
}

static gboolean
gimp_input_device_store_remove (GimpInputDeviceStore *store,
                                const gchar          *udi)
{
  GtkTreeIter  iter;

  if (gimp_input_device_store_lookup (store, udi, &iter))
    {
      gtk_list_store_remove (GTK_LIST_STORE (store), &iter);
      return TRUE;
    }

  return FALSE;
}

static void
gimp_input_device_store_device_added (LibHalContext *ctx,
                                      const char    *udi)
{
  GimpInputDeviceStore *store = libhal_ctx_get_user_data (ctx);

  if (gimp_input_device_store_add (store, udi))
    {
      g_signal_emit (store, store_signals[DEVICE_ADDED], 0, udi);
    }
}

static void
gimp_input_device_store_device_removed (LibHalContext *ctx,
                                        const char    *udi)
{
  GimpInputDeviceStore *store = libhal_ctx_get_user_data (ctx);

  if (gimp_input_device_store_remove (store, udi))
    {
      g_signal_emit (store, store_signals[DEVICE_REMOVED], 0, udi);
    }
}

GimpInputDeviceStore *
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

  if (! store->context)
    return NULL;

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

GError *
gimp_input_device_store_get_error (GimpInputDeviceStore  *store)
{
  g_return_val_if_fail (GIMP_IS_INPUT_DEVICE_STORE (store), NULL);

  return store->error ? g_error_copy (store->error) : NULL;
}

#elif defined (HAVE_DX_DINPUT)

enum
{
  COLUMN_GUID,
  COLUMN_LABEL,
  COLUMN_IDEVICE,
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

typedef struct _GimpInputDeviceStoreClass GimpInputDeviceStoreClass;

struct _GimpInputDeviceStore
{
  GtkListStore    parent_instance;

  GdkWindow      *window;

  LPDIRECTINPUT8W directinput8;

  GError         *error;
};


struct _GimpInputDeviceStoreClass
{
  GtkListStoreClass   parent_class;

  void  (*device_added)   (GimpInputDeviceStore *store,
                           const gchar          *udi);
  void  (*device_removed) (GimpInputDeviceStore *store,
                           const gchar          *udi);
};


static void      gimp_input_device_store_class_init (GimpInputDeviceStoreClass *klass);
static void      gimp_input_device_store_init       (GimpInputDeviceStore *store);
static void      gimp_input_device_store_finalize   (GObject              *object);
static gboolean  gimp_input_device_store_add        (GimpInputDeviceStore *store,
                                                     const GUID           *guid);
static gboolean  gimp_input_device_store_remove     (GimpInputDeviceStore *store,
                                                     const gchar          *udi);

#if 0
static void      gimp_input_device_store_device_added   (LibHalContext *ctx,
                                                         const char    *udi);
static void      gimp_input_device_store_device_removed (LibHalContext *ctx,
                                                         const char    *udi);
#endif

GType                     gimp_input_device_store_type = 0;
static GtkListStoreClass *parent_class                 = NULL;
static guint              store_signals[LAST_SIGNAL]   = { 0 };


GType
gimp_input_device_store_get_type (GTypeModule *module)
{
  if (! gimp_input_device_store_type)
    {
      const GTypeInfo info =
      {
        sizeof (GimpInputDeviceStoreClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_input_device_store_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpInputDeviceStore),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_input_device_store_init
      };

      gimp_input_device_store_type =
        g_type_module_register_type (module, GTK_TYPE_LIST_STORE,
                                     "GimpInputDeviceStore",
                                     &info, 0);
    }

  return gimp_input_device_store_type;
}


static void
gimp_input_device_store_class_init (GimpInputDeviceStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  store_signals[DEVICE_ADDED] =
    g_signal_new ("device-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpInputDeviceStoreClass, device_added),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  store_signals[DEVICE_REMOVED] =
    g_signal_new ("device-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpInputDeviceStoreClass, device_removed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  object_class->finalize = gimp_input_device_store_finalize;

  klass->device_added    = NULL;
  klass->device_removed  = NULL;
}

static GdkFilterReturn
aux_window_filter (GdkXEvent *xevent,
		   GdkEvent  *event,
		   gpointer   data)
{
  GimpInputDeviceStore *store = (GimpInputDeviceStore *) data;
  const MSG *msg = (MSG *) xevent;

  /* Look for deviced being added or removed */
  switch (msg->message)
    {
    }

  return GDK_FILTER_REMOVE;
}

static GdkWindow *
create_aux_window (GimpInputDeviceStore *store)
{
  GdkWindowAttr wa;
  GdkWindow *retval;

  /* Create a dummy window to be associated with DirectInput devices */
  wa.wclass = GDK_INPUT_OUTPUT;
  wa.event_mask = GDK_ALL_EVENTS_MASK;
  wa.width = 2;
  wa.height = 2;
  wa.x = -100;
  wa.y = -100;
  wa.window_type = GDK_WINDOW_TOPLEVEL;
  if ((retval = gdk_window_new (NULL, &wa, GDK_WA_X|GDK_WA_Y)) == NULL)
    return NULL;
  g_object_ref (retval);

  gdk_window_add_filter (retval, aux_window_filter, store);

  return retval;
}

static BOOL CALLBACK
enum_devices (const DIDEVICEINSTANCEW *di,
	      void                    *user_data)
{
  GimpInputDeviceStore *store = (GimpInputDeviceStore *) user_data;

  gimp_input_device_store_add (store, &di->guidInstance);

  return DIENUM_CONTINUE;
}

static void
gimp_input_device_store_init (GimpInputDeviceStore *store)
{
  GType   types[] = { G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER };
  HRESULT hresult;
  HMODULE thismodule;
  HMODULE dinput8;

  typedef HRESULT (WINAPI *t_DirectInput8Create) (HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN);
  t_DirectInput8Create p_DirectInput8Create;

  g_assert (G_N_ELEMENTS (types) == NUM_COLUMNS);

  gtk_list_store_set_column_types (GTK_LIST_STORE (store),
                                   G_N_ELEMENTS (types), types);

  if (!GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
			  (LPCTSTR) &gimp_input_device_store_init,
			  &thismodule))
    return;

  if ((store->window = create_aux_window (store)) == NULL)
    {
      g_set_error (&store->error, 0, 0, "Could not create aux window");
      return;
    }

  if ((dinput8 = LoadLibrary ("dinput8.dll")) == NULL)
    {
      g_set_error (&store->error, 0, 0, "Could not load dinput8.dll");
      return;
    }

  if ((p_DirectInput8Create = (t_DirectInput8Create) GetProcAddress (dinput8, "DirectInput8Create")) == NULL)
    {
      g_set_error (&store->error, 0, 0, "Could not find DirectInput8Create in dinput8.dll");
      return;
    }
  
  if (FAILED ((hresult = (*p_DirectInput8Create) (thismodule,
						  DIRECTINPUT_VERSION,
						  &IID_IDirectInput8W,
						  (LPVOID *) &store->directinput8,
						  NULL))))
    {
      g_set_error (&store->error, 0, 0, "DirectInput8Create failed: %s", g_win32_error_message (hresult));
      return;
    }

  if (FAILED ((hresult = IDirectInput8_EnumDevices (store->directinput8,
						    DI8DEVCLASS_GAMECTRL,
						    enum_devices,
						    store,
						    DIEDFL_ATTACHEDONLY))))
    {
      g_set_error (&store->error, 0, 0, "IDirectInput8::EnumDevices failed: %s", g_win32_error_message (hresult));
      return;
    }
}

static void
gimp_input_device_store_finalize (GObject *object)
{
  GimpInputDeviceStore *store = GIMP_INPUT_DEVICE_STORE (object);

  if (store->directinput8)
    {
      IDirectInput8_Release (store->directinput8);
      store->directinput8 = NULL;
    }

  if (store->error)
    {
      g_error_free (store->error);
      store->error = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_input_device_store_lookup (GimpInputDeviceStore *store,
                                const gchar          *guid,
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

      gtk_tree_model_get_value (model, iter, COLUMN_GUID, &value);

      str = g_value_get_string (&value);

      if (strcmp (str, guid) == 0)
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
gimp_input_device_store_insert (GimpInputDeviceStore  *store,
                                const gchar           *guid,
                                const gchar           *label,
				LPDIRECTINPUTDEVICE8W  didevice8)
{
  GtkTreeModel *model = GTK_TREE_MODEL (store);
  GtkTreeIter   iter;
  GValue        value = { 0, };
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
                                     COLUMN_GUID,    guid,
                                     COLUMN_LABEL,   label,
				     COLUMN_IDEVICE, didevice8,
                                     -1);
}

static gboolean
gimp_input_device_store_add (GimpInputDeviceStore *store,
                             const GUID           *guid)
{
  HRESULT    	        hresult;
  LPDIRECTINPUTDEVICE8W didevice8;
  DIDEVICEINSTANCEW     di;
  gboolean              added = FALSE;
  unsigned char        *s;
  gchar                *guidstring;
  gchar                *name;

  if (UuidToString (guid, &s) != S_OK)
    return FALSE;
  guidstring = g_strdup (s);
  RpcStringFree (&s);

  if (FAILED ((hresult = IDirectInput8_CreateDevice (store->directinput8,
						     guid,
						     &didevice8,
						     NULL))))
    return FALSE;

  if (FAILED ((hresult = IDirectInputDevice8_SetCooperativeLevel (didevice8,
								  (HWND) gdk_win32_drawable_get_handle (store->window),
								  DISCL_NONEXCLUSIVE | DISCL_BACKGROUND))))
    {
      g_warning ("IDirectInputDevice8::SetCooperativeLevel failed: %s",
		 g_win32_error_message (hresult));
      return FALSE;
    }

  di.dwSize = sizeof (DIDEVICEINSTANCEW);
  if (FAILED ((hresult = IDirectInputDevice8_GetDeviceInfo (didevice8,
							    &di))))
    {
      g_warning ("IDirectInputDevice8::GetDeviceInfo failed: %s",
		 g_win32_error_message (hresult));
      return FALSE;
    }

  name = g_utf16_to_utf8 (di.tszInstanceName, -1, NULL, NULL, NULL);
  gimp_input_device_store_insert (store, guidstring, name, didevice8);
  
  return added;
}

static gboolean
gimp_input_device_store_remove (GimpInputDeviceStore *store,
                                const gchar          *guid)
{
  GtkTreeIter  iter;

  if (gimp_input_device_store_lookup (store, guid, &iter))
    {
      gtk_list_store_remove (GTK_LIST_STORE (store), &iter);
      return TRUE;
    }

  return FALSE;
}

#if 0

static void
gimp_input_device_store_device_added (LibHalContext *ctx,
                                      const char    *guid)
{
  GimpInputDeviceStore *store = libhal_ctx_get_user_data (ctx);

  if (gimp_input_device_store_add (store, udi))
    {
      g_signal_emit (store, store_signals[DEVICE_ADDED], 0, udi);
    }
}

static void
gimp_input_device_store_device_removed (LibHalContext *ctx,
                                        const char    *udi)
{
  GimpInputDeviceStore *store = libhal_ctx_get_user_data (ctx);

  if (gimp_input_device_store_remove (store, udi))
    {
      g_signal_emit (store, store_signals[DEVICE_REMOVED], 0, udi);
    }
}

#endif

GimpInputDeviceStore *
gimp_input_device_store_new (void)
{
  return g_object_new (GIMP_TYPE_INPUT_DEVICE_STORE, NULL);
}

gchar *
gimp_input_device_store_get_device_file (GimpInputDeviceStore *store,
                                         const gchar          *udi)
{
  GtkTreeIter iter;
  GValue      value = { 0, };

  g_return_val_if_fail (GIMP_IS_INPUT_DEVICE_STORE (store), NULL);
  g_return_val_if_fail (udi != NULL, NULL);

  if (! store->directinput8)
    return NULL;

  if (gimp_input_device_store_lookup (store, udi, &iter))
    {
      gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, COLUMN_IDEVICE, &value);
      return g_value_get_pointer (&value);
    }

  return NULL;
}

GError *
gimp_input_device_store_get_error (GimpInputDeviceStore  *store)
{
  g_return_val_if_fail (GIMP_IS_INPUT_DEVICE_STORE (store), NULL);

  return store->error ? g_error_copy (store->error) : NULL;
}

#else

GType gimp_input_device_store_type = G_TYPE_NONE;

GType
gimp_input_device_store_get_type (GTypeModule *module)
{
  return G_TYPE_NONE;
}

GimpInputDeviceStore *
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

GError *
gimp_input_device_store_get_error (GimpInputDeviceStore  *store)
{
  return NULL;
}

#endif
