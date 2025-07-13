/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationsettings.h
 * Copyright (C) 2020 Ell
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

#pragma once

#include "core/gimpsettings.h"


#define GIMP_TYPE_OPERATION_SETTINGS            (gimp_operation_settings_get_type ())
#define GIMP_OPERATION_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_OPERATION_SETTINGS, GimpOperationSettings))
#define GIMP_OPERATION_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_OPERATION_SETTINGS, GimpOperationSettingsClass))
#define GIMP_IS_OPERATION_SETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_OPERATION_SETTINGS))
#define GIMP_IS_OPERATION_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_OPERATION_SETTINGS))
#define GIMP_OPERATION_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_OPERATION_SETTINGS, GimpOperationSettingsClass))


typedef struct _GimpOperationSettingsClass GimpOperationSettingsClass;

struct _GimpOperationSettings
{
  GimpSettings         parent_instance;

  GimpTransformResize  clip;
  GimpFilterRegion     region;
  GimpLayerMode        mode;
  gdouble              opacity;
};

struct _GimpOperationSettingsClass
{
  GimpSettingsClass  parent_class;
};


GType      gimp_operation_settings_get_type              (void) G_GNUC_CONST;

void       gimp_operation_settings_sync_drawable_filter  (GimpOperationSettings *settings,
                                                          GimpDrawableFilter    *filter);


/*  protected  */

gboolean   gimp_operation_settings_config_serialize_base (GimpConfig            *config,
                                                          GimpConfigWriter      *writer,
                                                          gpointer               data);
gboolean   gimp_operation_settings_config_equal_base     (GimpConfig            *a,
                                                          GimpConfig            *b);
void       gimp_operation_settings_config_reset_base     (GimpConfig            *config);
gboolean   gimp_operation_settings_config_copy_base      (GimpConfig            *src,
                                                          GimpConfig            *dest,
                                                          GParamFlags            flags);
