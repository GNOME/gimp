/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmahuesaturationconfig.h
 * Copyright (C) 2007 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_HUE_SATURATION_CONFIG_H__
#define __LIGMA_HUE_SATURATION_CONFIG_H__


#include "ligmaoperationsettings.h"


#define LIGMA_TYPE_HUE_SATURATION_CONFIG            (ligma_hue_saturation_config_get_type ())
#define LIGMA_HUE_SATURATION_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_HUE_SATURATION_CONFIG, LigmaHueSaturationConfig))
#define LIGMA_HUE_SATURATION_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_HUE_SATURATION_CONFIG, LigmaHueSaturationConfigClass))
#define LIGMA_IS_HUE_SATURATION_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_HUE_SATURATION_CONFIG))
#define LIGMA_IS_HUE_SATURATION_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_HUE_SATURATION_CONFIG))
#define LIGMA_HUE_SATURATION_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_HUE_SATURATION_CONFIG, LigmaHueSaturationConfigClass))


typedef struct _LigmaHueSaturationConfigClass LigmaHueSaturationConfigClass;

struct _LigmaHueSaturationConfig
{
  LigmaOperationSettings  parent_instance;

  LigmaHueRange           range;

  gdouble                hue[7];
  gdouble                saturation[7];
  gdouble                lightness[7];

  gdouble                overlap;
};

struct _LigmaHueSaturationConfigClass
{
  LigmaOperationSettingsClass  parent_class;
};


GType   ligma_hue_saturation_config_get_type    (void) G_GNUC_CONST;

void    ligma_hue_saturation_config_reset_range (LigmaHueSaturationConfig *config);


#endif /* __LIGMA_HUE_SATURATION_CONFIG_H__ */
