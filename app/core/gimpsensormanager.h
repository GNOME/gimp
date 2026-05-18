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

#pragma once

#define GIMP_TYPE_SENSOR_MANAGER (gimp_sensor_manager_get_type ())
G_DECLARE_FINAL_TYPE (GimpSensorManager, gimp_sensor_manager, GIMP, SENSOR_MANAGER, GimpObject)

GimpSensorManager * gimp_sensor_manager_new (void);

/**
 * GimpSensorsAccelCallback:
 * @x: accelerator's X value.
 * @y: accelerator's Y value.
 * @z: accelerator's Z value.
 * @error: optional error.
 * @data: (closure): user data.
 *
 * If @error is not %NULL, the coordinates (@x, @y, @z) must be ignored.
 * You should never free @error.
 **/
typedef void (* GimpSensorsAccelCallback)  (gdouble                    x,
                                            gdouble                    y,
                                            gdouble                    z,
                                            GError                    *error,
                                            gpointer                   data);

void         sensors_get_accel_coords      (Gimp                      *gimp,
                                            gdouble                   *x,
                                            gdouble                   *y,
                                            gdouble                   *z);
gboolean     sensors_watch_accelerometer   (Gimp                      *gimp,
                                            GimpSensorsAccelCallback   callback,
                                            gpointer                   callback_data,
                                            GDestroyNotify             callback_data_free,
                                            GError                   **error);
void         sensors_unwatch_accelerometer (Gimp                      *gimp);
