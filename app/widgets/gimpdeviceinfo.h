/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_DEVICE_INFO_H__
#define __GIMP_DEVICE_INFO_H__


#include "core/gimpcontext.h"


G_BEGIN_DECLS


#define GIMP_DEVICE_INFO_CONTEXT_MASK (GIMP_CONTEXT_PROP_MASK_TOOL       | \
                                       GIMP_CONTEXT_PROP_MASK_PAINT_INFO | \
                                       GIMP_CONTEXT_PROP_MASK_FOREGROUND | \
                                       GIMP_CONTEXT_PROP_MASK_BACKGROUND | \
                                       GIMP_CONTEXT_PROP_MASK_BRUSH      | \
                                       GIMP_CONTEXT_PROP_MASK_DYNAMICS   | \
                                       GIMP_CONTEXT_PROP_MASK_PATTERN    | \
                                       GIMP_CONTEXT_PROP_MASK_GRADIENT)


typedef struct _GimpDeviceKey GimpDeviceKey;

struct _GimpDeviceKey
{
  guint           keyval;
  GdkModifierType modifiers;
};


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
  gint           n_axes;
  GdkAxisUse    *axes;
  gint           n_keys;
  GimpDeviceKey *keys;

  /*  curves  */

  GimpCurve     *pressure_curve;
};

struct _GimpDeviceInfoClass
{
  GimpContextClass  parent_class;

  void (* changed) (GimpDeviceInfo *device_info);
};


GType            gimp_device_info_get_type          (void) G_GNUC_CONST;

GimpDeviceInfo * gimp_device_info_new               (Gimp            *gimp,
                                                     GdkDevice       *device,
                                                     GdkDisplay      *display);

GdkDevice      * gimp_device_info_get_device        (GimpDeviceInfo  *info,
                                                     GdkDisplay     **display);
void             gimp_device_info_set_device        (GimpDeviceInfo  *info,
                                                     GdkDevice       *device,
                                                     GdkDisplay      *display);

void             gimp_device_info_set_default_tool  (GimpDeviceInfo  *info);

GdkInputMode     gimp_device_info_get_mode          (GimpDeviceInfo  *info);
void             gimp_device_info_set_mode          (GimpDeviceInfo  *info,
                                                     GdkInputMode     mode);

gboolean         gimp_device_info_has_cursor        (GimpDeviceInfo  *info);

gint             gimp_device_info_get_n_axes        (GimpDeviceInfo  *info);
GdkAxisUse       gimp_device_info_get_axis_use      (GimpDeviceInfo  *info,
                                                     gint             axis);
void             gimp_device_info_set_axis_use      (GimpDeviceInfo  *info,
                                                     gint             axis,
                                                     GdkAxisUse       use);

gint             gimp_device_info_get_n_keys        (GimpDeviceInfo  *info);
void             gimp_device_info_get_key           (GimpDeviceInfo  *info,
                                                     gint             key,
                                                     guint           *keyval,
                                                     GdkModifierType *modifiers);
void             gimp_device_info_set_key           (GimpDeviceInfo  *info,
                                                     gint             key,
                                                     guint            keyval,
                                                     GdkModifierType  modifiers);

GimpCurve      * gimp_device_info_get_curve         (GimpDeviceInfo  *info,
                                                     GdkAxisUse       use);
gdouble          gimp_device_info_map_axis          (GimpDeviceInfo  *info,
                                                     GdkAxisUse       use,
                                                     gdouble          value);

void             gimp_device_info_changed           (GimpDeviceInfo  *info);

GimpDeviceInfo * gimp_device_info_get_by_device     (GdkDevice       *device);

gint             gimp_device_info_compare           (GimpDeviceInfo  *a,
                                                     GimpDeviceInfo  *b);


G_END_DECLS

#endif /* __GIMP_DEVICE_INFO_H__ */
