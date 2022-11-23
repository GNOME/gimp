/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_DEVICE_INFO_COORDS_H__
#define __LIGMA_DEVICE_INFO_COORDS_H__


gboolean ligma_device_info_get_event_coords   (LigmaDeviceInfo  *info,
                                              GdkWindow       *window,
                                              const GdkEvent  *event,
                                              LigmaCoords      *coords);
void     ligma_device_info_get_device_coords  (LigmaDeviceInfo  *info,
                                              GdkWindow       *window,
                                              LigmaCoords      *coords);

void     ligma_device_info_get_time_coords    (LigmaDeviceInfo  *info,
                                              GdkTimeCoord    *event,
                                              LigmaCoords      *coords);

gboolean ligma_device_info_get_event_state    (LigmaDeviceInfo  *info,
                                              GdkWindow       *window,
                                              const GdkEvent  *event,
                                              GdkModifierType *state);
void     ligma_device_info_get_device_state   (LigmaDeviceInfo  *info,
                                              GdkWindow       *window,
                                              GdkModifierType *state);


#endif /* __LIGMA_DEVICE_INFO_COORDS_H__ */
