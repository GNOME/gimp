/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmalevelsconfig.h
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

#ifndef __LIGMA_LEVELS_CONFIG_H__
#define __LIGMA_LEVELS_CONFIG_H__


#include "ligmaoperationsettings.h"


#define LIGMA_TYPE_LEVELS_CONFIG            (ligma_levels_config_get_type ())
#define LIGMA_LEVELS_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_LEVELS_CONFIG, LigmaLevelsConfig))
#define LIGMA_LEVELS_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_LEVELS_CONFIG, LigmaLevelsConfigClass))
#define LIGMA_IS_LEVELS_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_LEVELS_CONFIG))
#define LIGMA_IS_LEVELS_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_LEVELS_CONFIG))
#define LIGMA_LEVELS_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_LEVELS_CONFIG, LigmaLevelsConfigClass))


typedef struct _LigmaLevelsConfigClass LigmaLevelsConfigClass;

struct _LigmaLevelsConfig
{
  LigmaOperationSettings  parent_instance;

  LigmaTRCType            trc;

  LigmaHistogramChannel   channel;

  gdouble                low_input[5];
  gdouble                high_input[5];

  gboolean               clamp_input;

  gdouble                gamma[5];

  gdouble                low_output[5];
  gdouble                high_output[5];

  gboolean               clamp_output;
};

struct _LigmaLevelsConfigClass
{
  LigmaOperationSettingsClass  parent_class;
};


GType      ligma_levels_config_get_type         (void) G_GNUC_CONST;

void       ligma_levels_config_reset_channel    (LigmaLevelsConfig      *config);

void       ligma_levels_config_stretch          (LigmaLevelsConfig      *config,
                                                LigmaHistogram         *histogram,
                                                gboolean               is_color);
void       ligma_levels_config_stretch_channel  (LigmaLevelsConfig      *config,
                                                LigmaHistogram         *histogram,
                                                LigmaHistogramChannel   channel);
void       ligma_levels_config_adjust_by_colors (LigmaLevelsConfig      *config,
                                                LigmaHistogramChannel   channel,
                                                const LigmaRGB         *black,
                                                const LigmaRGB         *gray,
                                                const LigmaRGB         *white);

LigmaCurvesConfig *
           ligma_levels_config_to_curves_config (LigmaLevelsConfig      *config);

gboolean   ligma_levels_config_load_cruft       (LigmaLevelsConfig      *config,
                                                GInputStream          *input,
                                                GError               **error);
gboolean   ligma_levels_config_save_cruft       (LigmaLevelsConfig      *config,
                                                GOutputStream         *output,
                                                GError               **error);


#endif /* __LIGMA_LEVELS_CONFIG_H__ */
