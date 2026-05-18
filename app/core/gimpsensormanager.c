/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsensormanager.c
 * Copyright (C) 2026 Jehan <jehan@gimp.org>
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

#include <stdlib.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpsensormanager.h"

#include "gimp-intl.h"


struct _GimpSensorManager
{
  GimpObject                parent_instance;

  GimpSensorsAccelCallback  accel_callback;
  gpointer                  accel_callback_data;
  GDestroyNotify            accel_callback_data_free;

  gdouble                   accel_x;
  gdouble                   accel_y;
  gdouble                   accel_z;

#if defined(G_OS_UNIX) && ! defined(__APPLE__)
  guint                     watch_id;
  GDBusProxy               *sensor_portal;
  gboolean                  sensor_appeared;
#endif

  gboolean                  watch_accelerometer;
};


static void gimp_sensor_manager_constructed (GObject              *object);
static void gimp_sensor_manager_finalize    (GObject              *object);

static void sensor_start_portal             (GimpSensorManager    *manager,
                                             GError              **error);
static void sensor_stop_portal              (GimpSensorManager    *manager);

static void sensor_appeared_cb              (GDBusConnection      *connection,
                                             const gchar          *name,
                                             const gchar          *name_owner,
                                             gpointer              user_data);
static void sensor_vanished_cb              (GDBusConnection      *connection,
                                             const gchar          *name,
                                             gpointer              user_data);
static void sensor_properties_changed       (GDBusProxy           *proxy,
                                             GVariant             *changed_properties,
                                             GStrv                 invalidated_properties,
                                             gpointer              user_data);


G_DEFINE_TYPE (GimpSensorManager, gimp_sensor_manager, GIMP_TYPE_OBJECT)

#define parent_class gimp_sensor_manager_parent_class


static void
gimp_sensor_manager_class_init (GimpSensorManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_sensor_manager_constructed;
  object_class->finalize     = gimp_sensor_manager_finalize;
}

static void
gimp_sensor_manager_init (GimpSensorManager *manager)
{
#if defined(G_OS_UNIX) && ! defined(__APPLE__)
  manager->watch_id                 = 0;
  manager->sensor_portal            = NULL;
  manager->sensor_appeared          = FALSE;
#endif

  manager->watch_accelerometer      = FALSE;

  manager->accel_callback           = NULL;
  manager->accel_callback_data      = NULL;
  manager->accel_callback_data_free = NULL;

  manager->accel_x                  = 0.0;
  manager->accel_y                  = 0.0;
  manager->accel_z                  = 0.0;
}

static void
gimp_sensor_manager_constructed (GObject *object)
{
#if defined(G_OS_UNIX) && ! defined(__APPLE__)
  GimpSensorManager *manager = GIMP_SENSOR_MANAGER (object);

  manager->watch_id = g_bus_watch_name (G_BUS_TYPE_SYSTEM,
                                        "net.hadess.SensorProxy",
                                        G_BUS_NAME_WATCHER_FLAGS_NONE,
                                        sensor_appeared_cb,
                                        sensor_vanished_cb,
                                        manager, NULL);
#endif
}

static void
gimp_sensor_manager_finalize (GObject *object)
{
  GimpSensorManager *manager = GIMP_SENSOR_MANAGER (object);

  if (manager->accel_callback_data_free)
    g_clear_pointer (&manager->accel_callback_data,
                     manager->accel_callback_data_free);

#if defined(G_OS_UNIX) && ! defined(__APPLE__)
  sensor_stop_portal (manager);
  g_bus_unwatch_name (manager->watch_id);
#endif
}


/*  Public functions  */

GimpSensorManager *
gimp_sensor_manager_new (void)
{
  GimpSensorManager *manager;

  manager = g_object_new (GIMP_TYPE_SENSOR_MANAGER, NULL);

  return manager;
}

void
sensors_get_accel_coords (Gimp    *gimp,
                          gdouble *x,
                          gdouble *y,
                          gdouble *z)
{
  GimpSensorManager *manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  manager = gimp->sensor_manager;

  *x = manager->accel_x;
  *y = manager->accel_y;
  *z = manager->accel_z;
}

gboolean
sensors_watch_accelerometer (Gimp                     *gimp,
                             GimpSensorsAccelCallback  callback,
                             gpointer                  callback_data,
                             GDestroyNotify            callback_data_free,
                             GError                   **error)
{
  GimpSensorManager *manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  manager = gimp->sensor_manager;

  if (! manager->watch_accelerometer)
    {
      if (manager->accel_callback_data_free)
        g_clear_pointer (&manager->accel_callback_data,
                         manager->accel_callback_data_free);

      manager->accel_callback           = callback;
      manager->accel_callback_data      = callback_data;
      manager->accel_callback_data_free = callback_data_free;

#if defined(G_OS_UNIX) && ! defined(__APPLE__)

      if (manager->sensor_appeared)
        sensor_start_portal (manager, error);
#endif
    }

  return manager->watch_accelerometer;
}

void
sensors_unwatch_accelerometer (Gimp *gimp)
{
  GimpSensorManager *manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  manager = gimp->sensor_manager;

  if (manager->accel_callback_data_free)
    g_clear_pointer (&manager->accel_callback_data,
                     manager->accel_callback_data_free);

  manager->accel_callback           = NULL;
  manager->accel_callback_data      = NULL;
  manager->accel_callback_data_free = NULL;

#if defined(G_OS_UNIX) && ! defined(__APPLE__)
  sensor_stop_portal (manager);
#endif
}


/* Private functions */

#if defined(G_OS_UNIX) && ! defined(__APPLE__)
static void
sensor_start_portal (GimpSensorManager  *manager,
                     GError            **error)
{
  GVariant *ret;

  if (! manager->sensor_portal)
    manager->sensor_portal = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                            G_DBUS_PROXY_FLAGS_NONE, NULL,
                                                            "net.hadess.SensorProxy",
                                                            "/net/hadess/SensorProxy",
                                                            "net.hadess.SensorProxy",
                                                            NULL, error);

  if (! manager->sensor_portal)
    {
      manager->watch_accelerometer = FALSE;
      return;
    }

  g_signal_connect (G_OBJECT (manager->sensor_portal), "g-properties-changed",
                    G_CALLBACK (sensor_properties_changed), manager);


  /* Claim Accelerometer */

  ret = g_dbus_proxy_call_sync (manager->sensor_portal,
                                "ClaimAccelerometerWithRawData",
                                NULL,
                                G_DBUS_CALL_FLAGS_NONE,
                                -1, NULL, error);
  if (! ret)
    {
      if (error && *error &&
          ! g_error_matches (*error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_prefix_error (error, "Failed to claim accelerometer: ");
      else
        g_set_error_literal (error, G_IO_ERROR, 0, "Failed to claim accelerometer.");

      if (manager->accel_callback)
        manager->accel_callback (0.0, 0.0, 0.0, *error,
                                 manager->accel_callback_data);
    }
  else if (manager->accel_callback)
    {
      GVariant *v;

      v = g_dbus_proxy_get_cached_property (manager->sensor_portal, "HasAccelerometer");
      if (g_variant_get_boolean (v))
        {
          GVariant *x;
          GVariant *y;
          GVariant *z;

          x = g_dbus_proxy_get_cached_property (manager->sensor_portal, "AccelerometerX");
          y = g_dbus_proxy_get_cached_property (manager->sensor_portal, "AccelerometerY");
          z = g_dbus_proxy_get_cached_property (manager->sensor_portal, "AccelerometerZ");
          if (x && y && z && manager->accel_callback)
            manager->accel_callback (g_variant_get_double (x),
                                     g_variant_get_double (y),
                                     g_variant_get_double (z),
                                     NULL,
                                     manager->accel_callback_data);

          g_clear_pointer (&x, g_variant_unref);
          g_clear_pointer (&y, g_variant_unref);
          g_clear_pointer (&z, g_variant_unref);
        }
      g_variant_unref (v);
    }

  g_clear_pointer (&ret, g_variant_unref);

  manager->watch_accelerometer = (ret != NULL);
}

static void
sensor_stop_portal (GimpSensorManager *manager)
{
  g_clear_object (&manager->sensor_portal);
  manager->watch_accelerometer = FALSE;
}

static void
sensor_appeared_cb (GDBusConnection *connection,
                    const gchar     *name,
                    const gchar     *name_owner,
                    gpointer         user_data)
{
  GimpSensorManager *manager = GIMP_SENSOR_MANAGER (user_data);
  GError            *error   = NULL;

  if (manager->watch_accelerometer)
    sensor_start_portal (manager, &error);

  manager->sensor_appeared = TRUE;

  g_clear_error (&error);
}

static void
sensor_vanished_cb (GDBusConnection *connection,
                    const gchar     *name,
                    gpointer         user_data)
{
  GimpSensorManager *manager = GIMP_SENSOR_MANAGER (user_data);

  sensor_stop_portal (manager);
  manager->sensor_appeared = FALSE;
}

static void
sensor_properties_changed (GDBusProxy *proxy,
                           GVariant   *changed_properties,
                           GStrv       invalidated_properties,
                           gpointer    user_data)
{
  GimpSensorManager *manager  = GIMP_SENSOR_MANAGER (user_data);
  gboolean           modified = FALSE;
  GVariant          *v;
  GVariantDict       dict;

  g_variant_dict_init (&dict, changed_properties);

  if (g_variant_dict_contains (&dict, "AccelerometerX"))
    {
      v = g_dbus_proxy_get_cached_property (manager->sensor_portal, "AccelerometerX");
      manager->accel_x = g_variant_get_double (v);
      modified         = TRUE;
      g_variant_unref (v);
    }
  if (g_variant_dict_contains (&dict, "AccelerometerY"))
    {
      v = g_dbus_proxy_get_cached_property (manager->sensor_portal, "AccelerometerY");
      manager->accel_y = g_variant_get_double (v);
      modified         = TRUE;
      g_variant_unref (v);
    }
  if (g_variant_dict_contains (&dict, "AccelerometerZ"))
    {
      v = g_dbus_proxy_get_cached_property (manager->sensor_portal, "AccelerometerZ");
      manager->accel_z = g_variant_get_double (v);
      modified         = TRUE;
      g_variant_unref (v);
    }

  if (modified && manager->accel_callback)
    manager->accel_callback (manager->accel_x,
                             manager->accel_y,
                             manager->accel_z,
                             NULL, manager->accel_callback_data);

  g_variant_dict_clear (&dict);
}
#endif
