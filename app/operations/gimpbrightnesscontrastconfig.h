/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabrightnesscontrastconfig.h
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

#ifndef __LIGMA_BRIGHTNESS_CONTRAST_CONFIG_H__
#define __LIGMA_BRIGHTNESS_CONTRAST_CONFIG_H__


#include "ligmaoperationsettings.h"


#define LIGMA_TYPE_BRIGHTNESS_CONTRAST_CONFIG            (ligma_brightness_contrast_config_get_type ())
#define LIGMA_BRIGHTNESS_CONTRAST_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BRIGHTNESS_CONTRAST_CONFIG, LigmaBrightnessContrastConfig))
#define LIGMA_BRIGHTNESS_CONTRAST_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_BRIGHTNESS_CONTRAST_CONFIG, LigmaBrightnessContrastConfigClass))
#define LIGMA_IS_BRIGHTNESS_CONTRAST_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BRIGHTNESS_CONTRAST_CONFIG))
#define LIGMA_IS_BRIGHTNESS_CONTRAST_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_BRIGHTNESS_CONTRAST_CONFIG))
#define LIGMA_BRIGHTNESS_CONTRAST_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_BRIGHTNESS_CONTRAST_CONFIG, LigmaBrightnessContrastConfigClass))


typedef struct _LigmaBrightnessContrastConfigClass LigmaBrightnessContrastConfigClass;

struct _LigmaBrightnessContrastConfig
{
  LigmaOperationSettings  parent_instance;

  gdouble                brightness;
  gdouble                contrast;
};

struct _LigmaBrightnessContrastConfigClass
{
  LigmaOperationSettingsClass  parent_class;
};


GType   ligma_brightness_contrast_config_get_type (void) G_GNUC_CONST;

LigmaLevelsConfig *
ligma_brightness_contrast_config_to_levels_config (LigmaBrightnessContrastConfig *config);


#endif /* __LIGMA_BRIGHTNESS_CONTRAST_CONFIG_H__ */
