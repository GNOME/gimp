/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpinputdevicestore-dx.c
 * Input device store based on DirectX.
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
 * Copyright (C) 2007  Tor Lillqvist <tml@novell.com>
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

#ifdef HAVE_DX_DINPUT
#define _WIN32_WINNT 0x0501
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <rpc.h>

#ifndef GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
#define GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS 0x00000004
#endif

#include <gdk/gdkwin32.h>

#include "libgimpmodule/gimpmodule.h"

#include "gimpinputdevicestore.h"


enum
{
  COLUMN_GUID,
  COLUMN_LABEL,
  COLUMN_IDEVICE,
  NUM_COLUMNS
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

  void  (* device_added)   (GimpInputDeviceStore *store,
                            const gchar          *udi);
  void  (* device_removed) (GimpInputDeviceStore *store,
                            const gchar          *udi);
};


static void      gimp_input_device_store_finalize (GObject              *object);

static gboolean  gimp_input_device_store_add      (GimpInputDeviceStore *store,
                                                   const GUID           *guid);
static gboolean  gimp_input_device_store_remove   (GimpInputDeviceStore *store,
                                                   const gchar          *udi);


G_DEFINE_DYNAMIC_TYPE (GimpInputDeviceStore, gimp_input_device_store,
                       GTK_TYPE_LIST_STORE)

static guint store_signals[LAST_SIGNAL] = { 0 };


void
gimp_input_device_store_register_types (GTypeModule *module)
{
  gimp_input_device_store_register_type (module);
}

static void
gimp_input_device_store_class_init (GimpInputDeviceStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

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
gimp_input_device_store_class_finalize (GimpInputDeviceStoreClass *klass)
{
}

static GdkFilterReturn
aux_window_filter (GdkXEvent *xevent,
                   GdkEvent  *event,
                   gpointer   data)
{
#if 0
  GimpInputDeviceStore *store = (GimpInputDeviceStore *) data;
  const MSG *msg = (MSG *) xevent;

  /* Look for deviced being added or removed */
  switch (msg->message)
    {
    }
#endif

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
      g_set_error_literal (&store->error, GIMP_MODULE_ERROR, GIMP_MODULE_FAILED,
			   "Could not create aux window");
      return;
    }

  if ((dinput8 = LoadLibrary ("dinput8.dll")) == NULL)
    {
      g_set_error_literal (&store->error, GIMP_MODULE_ERROR, GIMP_MODULE_FAILED,
			   "Could not load dinput8.dll");
      return;
    }

  if ((p_DirectInput8Create = (t_DirectInput8Create) GetProcAddress (dinput8, "DirectInput8Create")) == NULL)
    {
      g_set_error_literal (&store->error, GIMP_MODULE_ERROR, GIMP_MODULE_FAILED,
			   "Could not find DirectInput8Create in dinput8.dll");
      return;
    }

  if (FAILED ((hresult = (*p_DirectInput8Create) (thismodule,
                                                  DIRECTINPUT_VERSION,
                                                  &IID_IDirectInput8W,
                                                  (LPVOID *) &store->directinput8,
                                                  NULL))))
    {
      g_set_error (&store->error, GIMP_MODULE_ERROR, GIMP_MODULE_FAILED,
		   "DirectInput8Create failed: %s",
		   g_win32_error_message (hresult));
      return;
    }

  if (FAILED ((hresult = IDirectInput8_EnumDevices (store->directinput8,
                                                    DI8DEVCLASS_GAMECTRL,
                                                    enum_devices,
                                                    store,
                                                    DIEDFL_ATTACHEDONLY))))
    {
      g_set_error (&store->error, GIMP_MODULE_ERROR, GIMP_MODULE_FAILED,
		   "IDirectInput8::EnumDevices failed: %s",
		   g_win32_error_message (hresult));
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

  G_OBJECT_CLASS (gimp_input_device_store_parent_class)->finalize (object);
}

static gboolean
gimp_input_device_store_lookup (GimpInputDeviceStore *store,
                                const gchar          *guid,
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
                                     COLUMN_GUID,    guid,
                                     COLUMN_LABEL,   label,
                                     COLUMN_IDEVICE, didevice8,
                                     -1);
}

static gboolean
gimp_input_device_store_add (GimpInputDeviceStore *store,
                             const GUID           *guid)
{
  HRESULT               hresult;
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
    {
      g_free (guidstring);
      return FALSE;
    }

  if (FAILED ((hresult = IDirectInputDevice8_SetCooperativeLevel (didevice8,
                                                                  (HWND) gdk_win32_drawable_get_handle (store->window),
                                                                  DISCL_NONEXCLUSIVE | DISCL_BACKGROUND))))
    {
      g_warning ("IDirectInputDevice8::SetCooperativeLevel failed: %s",
                 g_win32_error_message (hresult));
      g_free (guidstring);
      return FALSE;
    }

  di.dwSize = sizeof (DIDEVICEINSTANCEW);
  if (FAILED ((hresult = IDirectInputDevice8_GetDeviceInfo (didevice8,
                                                            &di))))
    {
      g_warning ("IDirectInputDevice8::GetDeviceInfo failed: %s",
                 g_win32_error_message (hresult));
      g_free (guidstring);
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
  GValue      value = G_VALUE_INIT;

  g_return_val_if_fail (GIMP_IS_INPUT_DEVICE_STORE (store), NULL);
  g_return_val_if_fail (udi != NULL, NULL);

  if (! store->directinput8)
    return NULL;

  if (gimp_input_device_store_lookup (store, udi, &iter))
    {
      gtk_tree_model_get_value (GTK_TREE_MODEL (store),
                                &iter, COLUMN_IDEVICE, &value);
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

#endif /* HAVE_DX_DINPUT */
