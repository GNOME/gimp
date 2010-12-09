/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdevicemanager.c
 * Copyright (C) 2011 Michael Natterer
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

#undef GSEAL_ENABLE

#include <gegl.h>
#include <gtk/gtk.h>

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpmarshal.h"

#include "gimpdeviceinfo.h"
#include "gimpdevicemanager.h"


enum
{
  PROP_0,
  PROP_GIMP,
  PROP_CURRENT_DEVICE
};


typedef struct _GimpDeviceManagerPrivate GimpDeviceManagerPrivate;

struct _GimpDeviceManagerPrivate
{
  Gimp           *gimp;
  GimpDeviceInfo *current_device;
};

#define GET_PRIVATE(manager) \
        G_TYPE_INSTANCE_GET_PRIVATE (manager, \
                                     GIMP_TYPE_DEVICE_MANAGER, \
                                     GimpDeviceManagerPrivate)


static void   gimp_device_manager_constructed    (GObject           *object);
static void   gimp_device_manager_dispose        (GObject           *object);
static void   gimp_device_manager_finalize       (GObject           *object);
static void   gimp_device_manager_set_property   (GObject           *object,
                                                  guint              property_id,
                                                  const GValue      *value,
                                                  GParamSpec        *pspec);
static void   gimp_device_manager_get_property   (GObject           *object,
                                                  guint              property_id,
                                                  GValue            *value,
                                                  GParamSpec        *pspec);

static void   gimp_device_manager_display_opened (GdkDisplayManager *disp_manager,
                                                  GdkDisplay        *display,
                                                  GimpDeviceManager *manager);
static void   gimp_device_manager_display_closed (GdkDisplay        *display,
                                                  gboolean           is_error,
                                                  GimpDeviceManager *manager);

static void   gimp_device_manager_device_added   (GdkDeviceManager  *gdk_manager,
                                                  GdkDevice         *device,
                                                  GimpDeviceManager *manager);
static void   gimp_device_manager_device_removed (GdkDeviceManager  *gdk_manager,
                                                  GdkDevice         *device,
                                                  GimpDeviceManager *manager);

static void   gimp_device_manager_config_notify  (GimpGuiConfig     *config,
                                                  const GParamSpec  *pspec,
                                                  GimpDeviceManager *manager);


G_DEFINE_TYPE (GimpDeviceManager, gimp_device_manager, GIMP_TYPE_LIST)

#define parent_class gimp_device_manager_parent_class


static void
gimp_device_manager_class_init (GimpDeviceManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed        = gimp_device_manager_constructed;
  object_class->dispose            = gimp_device_manager_dispose;
  object_class->finalize           = gimp_device_manager_finalize;
  object_class->set_property       = gimp_device_manager_set_property;
  object_class->get_property       = gimp_device_manager_get_property;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CURRENT_DEVICE,
                                   g_param_spec_object ("current-device",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DEVICE_INFO,
                                                        GIMP_PARAM_STATIC_STRINGS |
                                                        G_PARAM_READABLE));

  g_type_class_add_private (object_class, sizeof (GimpDeviceManagerPrivate));
}

static void
gimp_device_manager_init (GimpDeviceManager *manager)
{
}

static void
gimp_device_manager_constructed (GObject *object)
{
  GimpDeviceManager        *manager = GIMP_DEVICE_MANAGER (object);
  GimpDeviceManagerPrivate *private = GET_PRIVATE (object);
  GdkDisplayManager        *disp_manager;
  GSList                   *displays;
  GSList                   *list;
  GdkDisplay               *display;
  GdkDeviceManager         *gdk_manager;
  GdkDevice                *client_pointer;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_GIMP (private->gimp));

  disp_manager = gdk_display_manager_get ();

  displays = gdk_display_manager_list_displays (disp_manager);

  /*  present displays in the order in which they were opened  */
  displays = g_slist_reverse (displays);

  for (list = displays; list; list = g_slist_next (list))
    {
      gimp_device_manager_display_opened (disp_manager, list->data, manager);
    }

  g_slist_free (displays);

  g_signal_connect (disp_manager, "display-opened",
                    G_CALLBACK (gimp_device_manager_display_opened),
                    manager);

  display     = gdk_display_get_default ();
  gdk_manager = gdk_display_get_device_manager (display);

  client_pointer = gdk_device_manager_get_client_pointer (gdk_manager);

  private->current_device = gimp_device_info_get_by_device (client_pointer);

  g_signal_connect_object (private->gimp->config, "notify::devices-share-tool",
                           G_CALLBACK (gimp_device_manager_config_notify),
                           manager, 0);
}

static void
gimp_device_manager_dispose (GObject *object)
{
  g_signal_handlers_disconnect_by_func (gdk_display_manager_get (),
                                        gimp_device_manager_display_opened,
                                        object);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_device_manager_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_device_manager_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpDeviceManagerPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      private->gimp = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_device_manager_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpDeviceManagerPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, private->gimp);
      break;

    case PROP_CURRENT_DEVICE:
      g_value_set_object (value, private->current_device);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

GimpDeviceManager *
gimp_device_manager_new (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_object_new (GIMP_TYPE_DEVICE_MANAGER,
                       "gimp",          gimp,
                       "children-type", GIMP_TYPE_DEVICE_INFO,
                       "policy",        GIMP_CONTAINER_POLICY_STRONG,
                       "unique-names",  FALSE,
                       "sort-func",     gimp_device_info_compare,
                       NULL);
}

GimpDeviceInfo *
gimp_device_manager_get_current_device (GimpDeviceManager *manager)
{
  g_return_val_if_fail (GIMP_IS_DEVICE_MANAGER (manager), NULL);

  return GET_PRIVATE (manager)->current_device;
}

void
gimp_device_manager_set_current_device (GimpDeviceManager *manager,
                                        GimpDeviceInfo    *info)
{
  GimpDeviceManagerPrivate *private;
  GimpGuiConfig            *config;

  g_return_if_fail (GIMP_IS_DEVICE_MANAGER (manager));
  g_return_if_fail (GIMP_IS_DEVICE_INFO (info));

  private = GET_PRIVATE (manager);

  config = GIMP_GUI_CONFIG (private->gimp->config);

  if (! config->devices_share_tool)
    {
      gimp_context_set_parent (GIMP_CONTEXT (private->current_device), NULL);
    }

  private->current_device = info;

  if (! config->devices_share_tool)
    {
      GimpContext *user_context = gimp_get_user_context (private->gimp);

      gimp_context_copy_properties (GIMP_CONTEXT (info), user_context,
                                    GIMP_DEVICE_INFO_CONTEXT_MASK);
      gimp_context_set_parent (GIMP_CONTEXT (info), user_context);
    }

  g_object_notify (G_OBJECT (manager), "current-device");
}


/*  private functions  */


static void
gimp_device_manager_display_opened (GdkDisplayManager *disp_manager,
                                    GdkDisplay        *gdk_display,
                                    GimpDeviceManager *manager)
{
  GdkDeviceManager *gdk_manager;
  GdkDevice        *device;
  GList            *devices;
  GList            *list;

  gdk_manager = gdk_display_get_device_manager (gdk_display);

  device = gdk_device_manager_get_client_pointer (gdk_manager);
  gimp_device_manager_device_added (gdk_manager, device, manager);

  devices = gdk_device_manager_list_devices (gdk_manager,
                                             GDK_DEVICE_TYPE_SLAVE);

  /*  create device info structures for present devices */
  for (list = devices; list; list = g_list_next (list))
    {
      device = list->data;

      gimp_device_manager_device_added (gdk_manager, device, manager);
    }

  g_list_free (devices);

  devices = gdk_device_manager_list_devices (gdk_manager,
                                             GDK_DEVICE_TYPE_FLOATING);

  /*  create device info structures for present devices */
  for (list = devices; list; list = g_list_next (list))
    {
      device = list->data;

      gimp_device_manager_device_added (gdk_manager, device, manager);
    }

  g_list_free (devices);

  g_signal_connect (gdk_manager, "device-added",
                    G_CALLBACK (gimp_device_manager_device_added),
                    manager);
  g_signal_connect (gdk_manager, "device-removed",
                    G_CALLBACK (gimp_device_manager_device_removed),
                    manager);

  g_signal_connect (gdk_display, "closed",
                    G_CALLBACK (gimp_device_manager_display_closed),
                    manager);
}

static void
gimp_device_manager_display_closed (GdkDisplay        *gdk_display,
                                    gboolean           is_error,
                                    GimpDeviceManager *manager)
{
  GdkDeviceManager *gdk_manager;
  GdkDevice        *device;
  GList            *devices;
  GList            *list;

  gdk_manager = gdk_display_get_device_manager (gdk_display);

  device = gdk_device_manager_get_client_pointer (gdk_manager);
  gimp_device_manager_device_removed (gdk_manager, device, manager);

  devices = gdk_device_manager_list_devices (gdk_manager,
                                             GDK_DEVICE_TYPE_SLAVE);

  for (list = devices; list; list = list->next)
    {
      device = list->data;

      gimp_device_manager_device_removed (gdk_manager, device, manager);
    }

  g_list_free (devices);

  devices = gdk_device_manager_list_devices (gdk_manager,
                                             GDK_DEVICE_TYPE_FLOATING);

  for (list = devices; list; list = list->next)
    {
      device = list->data;

      gimp_device_manager_device_removed (gdk_manager, device, manager);
    }

  g_list_free (devices);
}

static void
gimp_device_manager_device_added (GdkDeviceManager  *gdk_manager,
                                  GdkDevice         *device,
                                  GimpDeviceManager *manager)
{
  GimpDeviceManagerPrivate *private = GET_PRIVATE (manager);
  GdkDisplay               *display;
  GimpDeviceInfo           *device_info;

  if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
    return;

  if (device != gdk_device_manager_get_client_pointer (gdk_manager) &&
      gdk_device_get_device_type (device) == GDK_DEVICE_TYPE_MASTER)
    return;

  display = gdk_device_manager_get_display (gdk_manager);

  device_info =
    GIMP_DEVICE_INFO (gimp_container_get_child_by_name (GIMP_CONTAINER (manager),
                                                        gdk_device_get_name (device)));

  if (device_info)
    {
      gimp_device_info_set_device (device_info, device, display);
    }
  else
    {
      device_info = gimp_device_info_new (private->gimp, device, display);

      gimp_device_info_set_default_tool (device_info);

      gimp_container_add (GIMP_CONTAINER (manager), GIMP_OBJECT (device_info));
      g_object_unref (device_info);
    }
}

static void
gimp_device_manager_device_removed (GdkDeviceManager  *gdk_manager,
                                    GdkDevice         *device,
                                    GimpDeviceManager *manager)
{
  GimpDeviceManagerPrivate *private = GET_PRIVATE (manager);
  GimpDeviceInfo           *device_info;

  device_info =
    GIMP_DEVICE_INFO (gimp_container_get_child_by_name (GIMP_CONTAINER (manager),
                                                        gdk_device_get_name (device)));

  if (device_info)
    {
      gimp_device_info_set_device (device_info, NULL, NULL);

      if (device_info == private->current_device)
        {
          device      = gdk_device_manager_get_client_pointer (gdk_manager);
          device_info = gimp_device_info_get_by_device (device);

          gimp_device_manager_set_current_device (manager, device_info);
        }
    }
}

static void
gimp_device_manager_config_notify (GimpGuiConfig     *config,
                                   const GParamSpec  *pspec,
                                   GimpDeviceManager *manager)
{
  GimpDeviceManagerPrivate *private = GET_PRIVATE (manager);
  GimpDeviceInfo           *current_device;

  current_device = gimp_device_manager_get_current_device (manager);

  if (GIMP_GUI_CONFIG (private->gimp->config)->devices_share_tool)
    {
      gimp_context_set_parent (GIMP_CONTEXT (current_device), NULL);
    }
  else
    {
      GimpContext *user_context = gimp_get_user_context (private->gimp);

      gimp_context_copy_properties (GIMP_CONTEXT (current_device), user_context,
                                    GIMP_DEVICE_INFO_CONTEXT_MASK);
      gimp_context_set_parent (GIMP_CONTEXT (current_device), user_context);
    }
}
