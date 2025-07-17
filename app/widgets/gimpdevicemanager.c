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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "widgets-types.h"

#include "libgimpbase/gimpbase.h"

#include "config/gimpconfig-utils.h"
#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpcurve.h"
#include "core/gimptoolinfo.h"

#include "gimpdeviceinfo.h"
#include "gimpdevicemanager.h"


enum
{
  PROP_0,
  PROP_GIMP,
  PROP_CURRENT_DEVICE
};

enum
{
  CONFIGURE_PAD,
  LAST_SIGNAL
};



struct _GimpDeviceManagerPrivate
{
  Gimp           *gimp;
  GHashTable     *displays;
  GimpDeviceInfo *current_device;
  GimpToolInfo   *active_tool;
};

#define GET_PRIVATE(obj) (((GimpDeviceManager *) (obj))->priv)


static void   gimp_device_manager_constructed     (GObject           *object);
static void   gimp_device_manager_dispose         (GObject           *object);
static void   gimp_device_manager_finalize        (GObject           *object);
static void   gimp_device_manager_set_property    (GObject           *object,
                                                   guint              property_id,
                                                   const GValue      *value,
                                                   GParamSpec        *pspec);
static void   gimp_device_manager_get_property    (GObject           *object,
                                                   guint              property_id,
                                                   GValue            *value,
                                                   GParamSpec        *pspec);

static void   gimp_device_manager_display_opened  (GdkDisplayManager *disp_manager,
                                                   GdkDisplay        *display,
                                                   GimpDeviceManager *manager);
static void   gimp_device_manager_display_closed  (GdkDisplay        *display,
                                                   gboolean           is_error,
                                                   GimpDeviceManager *manager);

static void   gimp_device_manager_device_added    (GdkSeat           *seat,
                                                   GdkDevice         *device,
                                                   GimpDeviceManager *manager);
static void   gimp_device_manager_device_removed  (GdkSeat           *seat,
                                                   GdkDevice         *device,
                                                   GimpDeviceManager *manager);

static void   gimp_device_manager_config_notify   (GimpGuiConfig     *config,
                                                   const GParamSpec  *pspec,
                                                   GimpDeviceManager *manager);

static void   gimp_device_manager_tool_changed    (GimpContext       *user_context,
                                                   GimpToolInfo      *tool_info,
                                                   GimpDeviceManager *manager);

static void   gimp_device_manager_connect_tool    (GimpDeviceManager *manager);
static void   gimp_device_manager_disconnect_tool (GimpDeviceManager *manager);

static void   gimp_device_manager_device_defaults (GdkSeat           *seat,
                                                   GdkDevice         *device,
                                                   GimpDeviceManager *manager);

static void   gimp_device_manager_reconfigure_pad_foreach (GimpDeviceInfo    *info,
                                                           GimpDeviceManager *manager);


G_DEFINE_TYPE_WITH_PRIVATE (GimpDeviceManager, gimp_device_manager,
                            GIMP_TYPE_LIST)

static guint device_manager_signals[LAST_SIGNAL];

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

  device_manager_signals[CONFIGURE_PAD] =
    g_signal_new ("configure-pad",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST, 0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GIMP_TYPE_DEVICE_INFO);

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
}

static void
gimp_device_manager_init (GimpDeviceManager *manager)
{
  manager->priv = gimp_device_manager_get_instance_private (manager);

  manager->priv->displays = g_hash_table_new_full (g_str_hash,
                                                   g_str_equal,
                                                   g_free, NULL);
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
  GdkSeat                  *seat;
  GdkDevice                *pointer;
  GimpDeviceInfo           *device_info;
  GimpContext              *user_context;

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
  seat    = gdk_display_get_default_seat (display);

  pointer = gdk_seat_get_pointer (seat);

  device_info = gimp_device_info_get_by_device (pointer);
  gimp_device_manager_set_current_device (manager, device_info);

  g_signal_connect_object (private->gimp->config, "notify::devices-share-tool",
                           G_CALLBACK (gimp_device_manager_config_notify),
                           manager, 0);

  user_context = gimp_get_user_context (private->gimp);

  g_signal_connect_object (user_context, "tool-changed",
                           G_CALLBACK (gimp_device_manager_tool_changed),
                           manager, 0);
}

static void
gimp_device_manager_dispose (GObject *object)
{
  GimpDeviceManager *manager = GIMP_DEVICE_MANAGER (object);

  gimp_device_manager_disconnect_tool (manager);

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
                       "gimp",         gimp,
                       "child-type",   GIMP_TYPE_DEVICE_INFO,
                       "policy",       GIMP_CONTAINER_POLICY_STRONG,
                       "unique-names", TRUE,
                       "sort-func",    gimp_device_info_compare,
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

  if (! config->devices_share_tool && private->current_device)
    {
      gimp_device_manager_disconnect_tool (manager);
    }

  private->current_device = info;

  if (! config->devices_share_tool && private->current_device)
    {
      GimpContext *user_context = gimp_get_user_context (private->gimp);

      g_signal_handlers_block_by_func (user_context,
                                       gimp_device_manager_tool_changed,
                                       manager);

      gimp_device_info_restore_tool (private->current_device);

      g_signal_handlers_unblock_by_func (user_context,
                                         gimp_device_manager_tool_changed,
                                         manager);

      private->active_tool = gimp_context_get_tool (user_context);
      gimp_device_manager_connect_tool (manager);
    }

  g_object_notify (G_OBJECT (manager), "current-device");
}

void
gimp_device_manager_reset (GimpDeviceManager *manager)
{
  GdkDisplayManager *disp_manager;
  GSList            *displays;
  GSList            *list;

  disp_manager = gdk_display_manager_get ();
  displays = gdk_display_manager_list_displays (disp_manager);

  for (list = displays; list; list = g_slist_next (list))
    {
      GdkDisplay *display = list->data;
      GdkSeat    *seat;
      GList      *devices;
      GList      *iter;

      seat    = gdk_display_get_default_seat (display);
      devices = gdk_seat_get_slaves (seat, GDK_SEAT_CAPABILITY_ALL);

      for (iter = devices; iter; iter = g_list_next (iter))
        {
          GdkDevice *device = iter->data;

          gimp_device_manager_device_defaults (seat, device, manager);
        }
    }

  g_slist_free (displays);

  gimp_device_manager_reconfigure_pads (manager);
}

void
gimp_device_manager_reconfigure_pads (GimpDeviceManager *manager)
{
  gimp_container_foreach (GIMP_CONTAINER (manager),
                          (GFunc) gimp_device_manager_reconfigure_pad_foreach,
                          manager);
}


/*  private functions  */

static void
gimp_device_manager_display_opened (GdkDisplayManager *disp_manager,
                                    GdkDisplay        *display,
                                    GimpDeviceManager *manager)
{
  GimpDeviceManagerPrivate *private = GET_PRIVATE (manager);
  GdkSeat                  *seat;
  GdkDevice                *device;
  GList                    *devices;
  GList                    *list;
  const gchar              *display_name;
  gint                      count;

  display_name = gdk_display_get_name (display);

  count = GPOINTER_TO_INT (g_hash_table_lookup (private->displays,
                                                display_name));

  g_hash_table_insert (private->displays, g_strdup (display_name),
                       GINT_TO_POINTER (count + 1));

  /*  don't add the same display twice  */
  if (count > 0)
    return;

  seat = gdk_display_get_default_seat (display);

  device = gdk_seat_get_pointer (seat);
  gimp_device_manager_device_added (seat, device, manager);

  devices = gdk_seat_get_slaves (seat, GDK_SEAT_CAPABILITY_ALL);

  /*  create device info structures for present devices */
  for (list = devices; list; list = g_list_next (list))
    {
      device = list->data;

      gimp_device_manager_device_added (seat, device, manager);
    }

  g_list_free (devices);

  g_signal_connect_object (seat, "device-added",
                           G_CALLBACK (gimp_device_manager_device_added),
                           manager, 0);
  g_signal_connect_object (seat, "device-removed",
                           G_CALLBACK (gimp_device_manager_device_removed),
                           manager, 0);

  g_signal_connect_object (display, "closed",
                           G_CALLBACK (gimp_device_manager_display_closed),
                           manager, 0);
}

static void
gimp_device_manager_display_closed (GdkDisplay        *display,
                                    gboolean           is_error,
                                    GimpDeviceManager *manager)
{
  GimpDeviceManagerPrivate *private = GET_PRIVATE (manager);
  GdkSeat                  *seat;
  GdkDevice                *device;
  GList                    *devices;
  GList                    *list;
  const gchar              *display_name;
  gint                      count;

  display_name = gdk_display_get_name (display);

  count = GPOINTER_TO_INT (g_hash_table_lookup (private->displays,
                                                display_name));

  /*  don't remove the same display twice  */
  if (count > 1)
    {
      g_hash_table_insert (private->displays, g_strdup (display_name),
                           GINT_TO_POINTER (count - 1));
      return;
    }

  g_hash_table_remove (private->displays, display_name);

  seat = gdk_display_get_default_seat (display);

  g_signal_handlers_disconnect_by_func (seat,
                                        gimp_device_manager_device_added,
                                        manager);
  g_signal_handlers_disconnect_by_func (seat,
                                        gimp_device_manager_device_removed,
                                        manager);

  g_signal_handlers_disconnect_by_func (display,
                                        gimp_device_manager_display_closed,
                                        manager);

  device = gdk_seat_get_pointer (seat);
  gimp_device_manager_device_removed (seat, device, manager);

  devices = gdk_seat_get_slaves (seat, GDK_SEAT_CAPABILITY_ALL_POINTING);

  for (list = devices; list; list = list->next)
    {
      device = list->data;

      gimp_device_manager_device_removed (seat, device, manager);
    }

  g_list_free (devices);
}

static void
gimp_device_manager_device_added (GdkSeat           *seat,
                                  GdkDevice         *device,
                                  GimpDeviceManager *manager)
{
  GimpDeviceManagerPrivate *private = GET_PRIVATE (manager);
  GdkDisplay               *display;
  GimpDeviceInfo           *device_info;

  if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
    return;

  gimp_device_manager_device_defaults (seat, device, manager);

  display = gdk_seat_get_display (seat);

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

  if (gdk_device_get_source (device) == GDK_SOURCE_TABLET_PAD)
    g_signal_emit (manager, device_manager_signals[CONFIGURE_PAD], 0, device_info);
}

static void
gimp_device_manager_device_removed (GdkSeat           *seat,
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
          device      = gdk_seat_get_pointer (seat);
          device_info = gimp_device_info_get_by_device (device);

          gimp_device_manager_set_current_device (manager, device_info);
        }

      if (gdk_device_get_source (device) == GDK_SOURCE_TABLET_PAD)
        g_signal_emit (manager, device_manager_signals[CONFIGURE_PAD], 0, device_info);
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

  if (config->devices_share_tool)
    {
      gimp_device_manager_disconnect_tool (manager);
      gimp_device_info_save_tool (current_device);
    }
  else
    {
      GimpContext *user_context = gimp_get_user_context (private->gimp);

      g_signal_handlers_block_by_func (user_context,
                                       gimp_device_manager_tool_changed,
                                       manager);

      gimp_device_info_restore_tool (private->current_device);

      g_signal_handlers_unblock_by_func (user_context,
                                         gimp_device_manager_tool_changed,
                                         manager);

      private->active_tool = gimp_context_get_tool (user_context);
      gimp_device_manager_connect_tool (manager);
    }
}

static void
gimp_device_manager_tool_changed (GimpContext       *user_context,
                                  GimpToolInfo      *tool_info,
                                  GimpDeviceManager *manager)
{
  GimpDeviceManagerPrivate *private = GET_PRIVATE (manager);
  GimpGuiConfig            *config;

  config = GIMP_GUI_CONFIG (private->gimp->config);

  if (! config->devices_share_tool)
    {
      gimp_device_manager_disconnect_tool (manager);
    }

  private->active_tool = tool_info;

  if (! config->devices_share_tool)
    {
      gimp_device_info_save_tool (private->current_device);
      gimp_device_manager_connect_tool (manager);
    }
}

static void
gimp_device_manager_connect_tool (GimpDeviceManager *manager)
{
  GimpDeviceManagerPrivate *private = GET_PRIVATE (manager);
  GimpGuiConfig            *config;

  config = GIMP_GUI_CONFIG (private->gimp->config);

  if (! config->devices_share_tool &&
      private->active_tool && private->current_device)
    {
      GimpToolPreset *preset = GIMP_TOOL_PRESET (private->current_device);

      gimp_config_connect (G_OBJECT (private->active_tool->tool_options),
                           G_OBJECT (preset->tool_options),
                           NULL);
    }
}

static void
gimp_device_manager_disconnect_tool (GimpDeviceManager *manager)
{
  GimpDeviceManagerPrivate *private = GET_PRIVATE (manager);
  GimpGuiConfig            *config;

  config = GIMP_GUI_CONFIG (private->gimp->config);

  if (! config->devices_share_tool &&
      private->active_tool && private->current_device)
    {
      GimpToolPreset *preset = GIMP_TOOL_PRESET (private->current_device);

      gimp_config_disconnect (G_OBJECT (private->active_tool->tool_options),
                              G_OBJECT (preset->tool_options));
    }
}

static void
gimp_device_manager_device_defaults (GdkSeat           *seat,
                                     GdkDevice         *device,
                                     GimpDeviceManager *manager)
{
  GimpDeviceInfo *info;
  gint            i;

  if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
    return;

  /* Set default mode for this device. */

  if (device == gdk_seat_get_pointer (seat))
    {
      gdk_device_set_mode (device, GDK_MODE_SCREEN);
    }
  else if (gdk_device_get_device_type (device) == GDK_DEVICE_TYPE_MASTER)
    {
      return;
    }
  else /* slave or floating device */
    {
      /* default to enabling all devices */
      GdkInputMode mode = GDK_MODE_SCREEN;

      switch (gdk_device_get_source (device))
        {
        case GDK_SOURCE_MOUSE:
          mode = GDK_MODE_DISABLED;
          break;

        case GDK_SOURCE_PEN:
        case GDK_SOURCE_ERASER:
        case GDK_SOURCE_CURSOR:
          break;

        case GDK_SOURCE_TOUCHSCREEN:
        case GDK_SOURCE_TOUCHPAD:
        case GDK_SOURCE_TRACKPOINT:
          mode = GDK_MODE_DISABLED;
          break;

        case GDK_SOURCE_TABLET_PAD:
          break;

        default:
          break;
        }

      if (gdk_device_set_mode (device, mode))
        {
          g_printerr ("set device '%s' to mode: %s\n",
                      gdk_device_get_name (device),
                      g_enum_get_value (g_type_class_peek (GDK_TYPE_INPUT_MODE),
                                        mode)->value_nick);
        }
      else
        {
          g_printerr ("failed to set device '%s' to mode: %s\n",
                      gdk_device_get_name (device),
                      g_enum_get_value (g_type_class_peek (GDK_TYPE_INPUT_MODE),
                                        mode)->value_nick);
         }
    }

  /* Reset curve for this device. */

  info =
    GIMP_DEVICE_INFO (gimp_container_get_child_by_name (GIMP_CONTAINER (manager),
                                                        gdk_device_get_name (device)));
  if (info)
    {
      for (i = 0; i < gimp_device_info_get_n_axes (info); i++)
        {
          GimpCurve  *curve;
          GdkAxisUse  use;

          use   = gimp_device_info_get_axis_use (info, i);
          curve = gimp_device_info_get_curve (info, use);

          if (curve)
            gimp_curve_reset (curve, TRUE);
        }
    }
}

static void
gimp_device_manager_reconfigure_pad_foreach (GimpDeviceInfo    *info,
                                             GimpDeviceManager *manager)
{
  if (gimp_device_info_get_device (info, NULL) == NULL)
    return;
  if (gimp_device_info_get_source (info) != GDK_SOURCE_TABLET_PAD)
    return;

  g_signal_emit (manager, device_manager_signals[CONFIGURE_PAD], 0, info);
}
