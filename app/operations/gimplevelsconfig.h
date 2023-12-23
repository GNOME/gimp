/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimplevelsconfig.h
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_LEVELS_CONFIG_H__
#define __GIMP_LEVELS_CONFIG_H__


#include "gimpoperationsettings.h"


#define GIMP_TYPE_LEVELS_CONFIG            (gimp_levels_config_get_type ())
#define GIMP_LEVELS_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LEVELS_CONFIG, GimpLevelsConfig))
#define GIMP_LEVELS_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_LEVELS_CONFIG, GimpLevelsConfigClass))
#define GIMP_IS_LEVELS_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LEVELS_CONFIG))
#define GIMP_IS_LEVELS_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_LEVELS_CONFIG))
#define GIMP_LEVELS_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_LEVELS_CONFIG, GimpLevelsConfigClass))


typedef struct _GimpLevelsConfigClass GimpLevelsConfigClass;

struct _GimpLevelsConfig
{
  GimpOperationSettings  parent_instance;

  GimpTRCType            trc;

  GimpHistogramChannel   channel;

  gdouble                low_input[5];
  gdouble                high_input[5];

  gboolean               clamp_input;

  gdouble                gamma[5];

  gdouble                low_output[5];
  gdouble                high_output[5];

  gboolean               clamp_output;
};

struct _GimpLevelsConfigClass
{
  GimpOperationSettingsClass  parent_class;
};


GType      gimp_levels_config_get_type         (void) G_GNUC_CONST;

void       gimp_levels_config_reset_channel    (GimpLevelsConfig      *config);

void       gimp_levels_config_stretch          (GimpLevelsConfig      *config,
                                                GimpHistogram         *histogram,
                                                gboolean               is_color);
void       gimp_levels_config_stretch_channel  (GimpLevelsConfig      *config,
                                                GimpHistogram         *histogram,
                                                GimpHistogramChannel   channel);
void       gimp_levels_config_adjust_by_colors (GimpLevelsConfig      *config,
                                                GimpHistogramChannel   channel,
                                                GeglColor             *black,
                                                GeglColor             *gray,
                                                GeglColor             *white);

GimpCurvesConfig *
           gimp_levels_config_to_curves_config (GimpLevelsConfig      *config);

gboolean   gimp_levels_config_load_cruft       (GimpLevelsConfig      *config,
                                                GInputStream          *input,
                                                GError               **error);
gboolean   gimp_levels_config_save_cruft       (GimpLevelsConfig      *config,
                                                GOutputStream         *output,
                                                GError               **error);


#endif /* __GIMP_LEVELS_CONFIG_H__ */
