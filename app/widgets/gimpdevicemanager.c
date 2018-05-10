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



struct _GimpDeviceManagerPrivate
{
  Gimp           *gimp;
  GHashTable     *displays;
  GimpDeviceInfo *current_device;
};

#define GET_PRIVATE(obj) (((GimpDeviceManager *) (obj))->priv)


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

static void   gimp_device_manager_device_added   (GdkDisplay        *gdk_display,
                                                  GdkDevice         *device,
                                                  GimpDeviceManager *manager);
static void   gimp_device_manager_device_removed (GdkDisplay        *gdk_display,
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
  manager->priv = G_TYPE_INSTANCE_GET_PRIVATE (manager,
                                               GIMP_TYPE_DEVICE_MANAGER,
                                               GimpDeviceManagerPrivate);

  manager->priv->displays = g_hash_table_new (g_direct_hash,
                                              g_direct_equal);
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

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_GIMP (private->gimp));

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

  display = gdk_display_get_default ();

  private->current_device =
    gimp_device_info_get_by_device (gdk_display_get_core_pointer (display));

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
  GimpDeviceManagerPrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->displays, g_hash_table_unref);

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
  GimpDeviceManagerPrivate *private = GET_PRIVATE (manager);
  GList                    *list;
  gint                      count;

  count = GPOINTER_TO_INT (g_hash_table_lookup (private->displays,
                                                gdk_display));

  g_hash_table_insert (private->displays, gdk_display,
                       GINT_TO_POINTER (count + 1));

  /*  don't add the same display twice  */
  if (count > 0)
    return;

  /*  create device info structures for present devices */
  for (list = gdk_display_list_devices (gdk_display); list; list = list->next)
    {
      GdkDevice *device = list->data;

      gimp_device_manager_device_added (gdk_display, device, manager);
    }

  g_signal_connect (gdk_display, "closed",
                    G_CALLBACK (gimp_device_manager_display_closed),
                    manager);
}

static void
gimp_device_manager_display_closed (GdkDisplay        *gdk_display,
                                    gboolean           is_error,
                                    GimpDeviceManager *manager)
{
  GimpDeviceManagerPrivate *private = GET_PRIVATE (manager);
  GList                    *list;
  gint                      count;

  count = GPOINTER_TO_INT (g_hash_table_lookup (private->displays,
                                                gdk_display));

  /*  don't remove the same display twice  */
  if (count > 1)
    {
      g_hash_table_insert (private->displays, gdk_display,
                           GINT_TO_POINTER (count - 1));
      return;
    }

  g_hash_table_remove (private->displays, gdk_display);

  for (list = gdk_display_list_devices (gdk_display); list; list = list->next)
    {
      GdkDevice *device = list->data;

      gimp_device_manager_device_removed (gdk_display, device, manager);
    }
}

static void
gimp_device_manager_device_added (GdkDisplay        *gdk_display,
                                  GdkDevice         *device,
                                  GimpDeviceManager *manager)
{
  GimpDeviceManagerPrivate *private = GET_PRIVATE (manager);
  GimpDeviceInfo           *device_info;

  device_info =
    GIMP_DEVICE_INFO (gimp_container_get_child_by_name (GIMP_CONTAINER (manager),
                                                        device->name));

  if (device_info)
    {
      gimp_device_info_set_device (device_info, device, gdk_display);
    }
  else
    {
      device_info = gimp_device_info_new (private->gimp, device, gdk_display);

      gimp_device_info_set_default_tool (device_info);

      gimp_container_add (GIMP_CONTAINER (manager), GIMP_OBJECT (device_info));
      g_object_unref (device_info);
    }
}

static void
gimp_device_manager_device_removed (GdkDisplay        *gdk_display,
                                    GdkDevice         *device,
                                    GimpDeviceManager *manager)
{
  GimpDeviceManagerPrivate *private = GET_PRIVATE (manager);
  GimpDeviceInfo           *device_info;

  device_info =
    GIMP_DEVICE_INFO (gimp_container_get_child_by_name (GIMP_CONTAINER (manager),
                                                        device->name));

  if (device_info)
    {
      gimp_device_info_set_device (device_info, NULL, NULL);

      if (device_info == private->current_device)
        {
          device      = gdk_display_get_core_pointer (gdk_display);
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
