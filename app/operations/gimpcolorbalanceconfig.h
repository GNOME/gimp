/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcolorbalanceconfig.h
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

#pragma once

#include "gimpoperationsettings.h"


#define GIMP_TYPE_COLOR_BALANCE_CONFIG            (gimp_color_balance_config_get_type ())
#define GIMP_COLOR_BALANCE_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_BALANCE_CONFIG, GimpColorBalanceConfig))
#define GIMP_COLOR_BALANCE_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_COLOR_BALANCE_CONFIG, GimpColorBalanceConfigClass))
#define GIMP_IS_COLOR_BALANCE_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_BALANCE_CONFIG))
#define GIMP_IS_COLOR_BALANCE_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_COLOR_BALANCE_CONFIG))
#define GIMP_COLOR_BALANCE_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_COLOR_BALANCE_CONFIG, GimpColorBalanceConfigClass))


typedef struct _GimpColorBalanceConfigClass GimpColorBalanceConfigClass;

struct _GimpColorBalanceConfig
{
  GimpOperationSettings  parent_instance;

  GimpTransferMode       range;

  gdouble                cyan_red[3];
  gdouble                magenta_green[3];
  gdouble                yellow_blue[3];

  gboolean               preserve_luminosity;
};

struct _GimpColorBalanceConfigClass
{
  GimpOperationSettingsClass  parent_class;
};


GType   gimp_color_balance_config_get_type    (void) G_GNUC_CONST;

void    gimp_color_balance_config_reset_range (GimpColorBalanceConfig *config);
