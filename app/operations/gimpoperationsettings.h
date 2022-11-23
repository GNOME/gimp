/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationsettings.h
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

#ifndef __LIGMA_OPERATION_SETTINGS_H__
#define __LIGMA_OPERATION_SETTINGS_H__


#include "core/ligmasettings.h"


#define LIGMA_TYPE_OPERATION_SETTINGS            (ligma_operation_settings_get_type ())
#define LIGMA_OPERATION_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_OPERATION_SETTINGS, LigmaOperationSettings))
#define LIGMA_OPERATION_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_OPERATION_SETTINGS, LigmaOperationSettingsClass))
#define LIGMA_IS_OPERATION_SETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_OPERATION_SETTINGS))
#define LIGMA_IS_OPERATION_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_OPERATION_SETTINGS))
#define LIGMA_OPERATION_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_OPERATION_SETTINGS, LigmaOperationSettingsClass))


typedef struct _LigmaOperationSettingsClass LigmaOperationSettingsClass;

struct _LigmaOperationSettings
{
  LigmaSettings         parent_instance;

  LigmaTransformResize  clip;
  LigmaFilterRegion     region;
  LigmaLayerMode        mode;
  gdouble              opacity;
  gboolean             gamma_hack;
};

struct _LigmaOperationSettingsClass
{
  LigmaSettingsClass  parent_class;
};


GType      ligma_operation_settings_get_type              (void) G_GNUC_CONST;

void       ligma_operation_settings_sync_drawable_filter  (LigmaOperationSettings *settings,
                                                          LigmaDrawableFilter    *filter);


/*  protected  */

gboolean   ligma_operation_settings_config_serialize_base (LigmaConfig            *config,
                                                          LigmaConfigWriter      *writer,
                                                          gpointer               data);
gboolean   ligma_operation_settings_config_equal_base     (LigmaConfig            *a,
                                                          LigmaConfig            *b);
void       ligma_operation_settings_config_reset_base     (LigmaConfig            *config);
gboolean   ligma_operation_settings_config_copy_base      (LigmaConfig            *src,
                                                          LigmaConfig            *dest,
                                                          GParamFlags            flags);


#endif /* __LIGMA_OPERATION_SETTINGS_H__ */
