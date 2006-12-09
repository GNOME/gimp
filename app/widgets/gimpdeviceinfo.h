/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __GIMP_DEVICE_INFO_H__
#define __GIMP_DEVICE_INFO_H__


#include <stdio.h>

#include "core/gimpcontext.h"


G_BEGIN_DECLS


#define GIMP_DEVICE_INFO_CONTEXT_MASK (GIMP_CONTEXT_TOOL_MASK       | \
                                       GIMP_CONTEXT_PAINT_INFO_MASK | \
                                       GIMP_CONTEXT_FOREGROUND_MASK | \
                                       GIMP_CONTEXT_BACKGROUND_MASK | \
                                       GIMP_CONTEXT_BRUSH_MASK      | \
                                       GIMP_CONTEXT_PATTERN_MASK    | \
                                       GIMP_CONTEXT_GRADIENT_MASK)


#define GIMP_TYPE_DEVICE_INFO            (gimp_device_info_get_type ())
#define GIMP_DEVICE_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DEVICE_INFO, GimpDeviceInfo))
#define GIMP_DEVICE_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DEVICE_INFO, GimpDeviceInfoClass))
#define GIMP_IS_DEVICE_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DEVICE_INFO))
#define GIMP_IS_DEVICE_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DEVICE_INFO))
#define GIMP_DEVICE_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DEVICE_INFO, GimpDeviceInfoClass))


typedef struct _GimpDeviceInfoClass GimpDeviceInfoClass;

struct _GimpDeviceInfo
{
  GimpContext    parent_instance;

  GdkDevice     *device;
  GdkDisplay    *display;

  /*  either "device" or the options below are set  */

  GdkInputMode   mode;
  gint           num_axes;
  GdkAxisUse    *axes;
  gint           num_keys;
  GdkDeviceKey  *keys;
};

struct _GimpDeviceInfoClass
{
  GimpContextClass  parent_class;

  void (* changed) (GimpDeviceInfo *device_info);
};


GType            gimp_device_info_get_type          (void) G_GNUC_CONST;

GimpDeviceInfo * gimp_device_info_new               (Gimp           *gimp,
                                                     const gchar    *name);

GimpDeviceInfo * gimp_device_info_set_from_device   (GimpDeviceInfo *device_info,
                                                     GdkDevice      *device,
                                                     GdkDisplay     *display);
void             gimp_device_info_changed           (GimpDeviceInfo *device_info);

GimpDeviceInfo * gimp_device_info_get_by_device     (GdkDevice      *device);
void             gimp_device_info_changed_by_device (GdkDevice      *device);


G_END_DECLS

#endif /* __GIMP_DEVICE_INFO_H__ */
