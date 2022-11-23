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

#ifndef __LIGMA_DEVICE_INFO_H__
#define __LIGMA_DEVICE_INFO_H__


#include "core/ligmatoolpreset.h"


G_BEGIN_DECLS


typedef struct _LigmaDeviceKey LigmaDeviceKey;

struct _LigmaDeviceKey
{
  guint           keyval;
  GdkModifierType modifiers;
};


#define LIGMA_TYPE_DEVICE_INFO            (ligma_device_info_get_type ())
#define LIGMA_DEVICE_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DEVICE_INFO, LigmaDeviceInfo))
#define LIGMA_DEVICE_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DEVICE_INFO, LigmaDeviceInfoClass))
#define LIGMA_IS_DEVICE_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DEVICE_INFO))
#define LIGMA_IS_DEVICE_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DEVICE_INFO))
#define LIGMA_DEVICE_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DEVICE_INFO, LigmaDeviceInfoClass))


typedef struct _LigmaDeviceInfoPrivate LigmaDeviceInfoPrivate;
typedef struct _LigmaDeviceInfoClass   LigmaDeviceInfoClass;

struct _LigmaDeviceInfo
{
  LigmaToolPreset  parent_instance;

  LigmaDeviceInfoPrivate *priv;
};

struct _LigmaDeviceInfoClass
{
  LigmaToolPresetClass  parent_class;
};


GType             ligma_device_info_get_type             (void) G_GNUC_CONST;

LigmaDeviceInfo  * ligma_device_info_new                  (Ligma            *ligma,
                                                         GdkDevice       *device,
                                                         GdkDisplay      *display);

GdkDevice       * ligma_device_info_get_device           (LigmaDeviceInfo  *info,
                                                         GdkDisplay     **display);
gboolean          ligma_device_info_set_device           (LigmaDeviceInfo  *info,
                                                         GdkDevice       *device,
                                                         GdkDisplay      *display);

void              ligma_device_info_set_default_tool     (LigmaDeviceInfo  *info);

void              ligma_device_info_save_tool            (LigmaDeviceInfo  *info);
void              ligma_device_info_restore_tool         (LigmaDeviceInfo  *info);

GdkInputMode      ligma_device_info_get_mode             (LigmaDeviceInfo  *info);
void              ligma_device_info_set_mode             (LigmaDeviceInfo  *info,
                                                         GdkInputMode     mode);

gboolean          ligma_device_info_has_cursor           (LigmaDeviceInfo  *info);

GdkInputSource    ligma_device_info_get_source           (LigmaDeviceInfo  *info);

const gchar     * ligma_device_info_get_vendor_id        (LigmaDeviceInfo  *info);
const gchar     * ligma_device_info_get_product_id       (LigmaDeviceInfo  *info);

GdkDeviceToolType ligma_device_info_get_tool_type        (LigmaDeviceInfo  *info);
guint64           ligma_device_info_get_tool_serial      (LigmaDeviceInfo  *info);
guint64           ligma_device_info_get_tool_hardware_id (LigmaDeviceInfo  *info);

gint             ligma_device_info_get_n_axes            (LigmaDeviceInfo  *info);
gboolean         ligma_device_info_ignore_axis           (LigmaDeviceInfo  *info,
                                                         gint             axis);
const gchar    * ligma_device_info_get_axis_name         (LigmaDeviceInfo  *info,
                                                         gint             axis);
GdkAxisUse       ligma_device_info_get_axis_use          (LigmaDeviceInfo  *info,
                                                         gint             axis);
void             ligma_device_info_set_axis_use          (LigmaDeviceInfo  *info,
                                                         gint             axis,
                                                         GdkAxisUse       use);

gint             ligma_device_info_get_n_keys            (LigmaDeviceInfo  *info);
void             ligma_device_info_get_key               (LigmaDeviceInfo  *info,
                                                         gint             key,
                                                         guint           *keyval,
                                                         GdkModifierType *modifiers);
void             ligma_device_info_set_key               (LigmaDeviceInfo  *info,
                                                         gint             key,
                                                         guint            keyval,
                                                         GdkModifierType  modifiers);

LigmaCurve      * ligma_device_info_get_curve             (LigmaDeviceInfo  *info,
                                                         GdkAxisUse       use);
gdouble          ligma_device_info_map_axis              (LigmaDeviceInfo  *info,
                                                         GdkAxisUse       use,
                                                         gdouble          value);

void             ligma_device_info_changed               (LigmaDeviceInfo  *info);

LigmaDeviceInfo * ligma_device_info_get_by_device         (GdkDevice       *device);

gint             ligma_device_info_compare               (LigmaDeviceInfo  *a,
                                                         LigmaDeviceInfo  *b);


G_END_DECLS

#endif /* __LIGMA_DEVICE_INFO_H__ */
