/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacolorbalanceconfig.h
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

#ifndef __LIGMA_COLOR_BALANCE_CONFIG_H__
#define __LIGMA_COLOR_BALANCE_CONFIG_H__


#include "ligmaoperationsettings.h"


#define LIGMA_TYPE_COLOR_BALANCE_CONFIG            (ligma_color_balance_config_get_type ())
#define LIGMA_COLOR_BALANCE_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_BALANCE_CONFIG, LigmaColorBalanceConfig))
#define LIGMA_COLOR_BALANCE_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  LIGMA_TYPE_COLOR_BALANCE_CONFIG, LigmaColorBalanceConfigClass))
#define LIGMA_IS_COLOR_BALANCE_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_BALANCE_CONFIG))
#define LIGMA_IS_COLOR_BALANCE_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  LIGMA_TYPE_COLOR_BALANCE_CONFIG))
#define LIGMA_COLOR_BALANCE_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  LIGMA_TYPE_COLOR_BALANCE_CONFIG, LigmaColorBalanceConfigClass))


typedef struct _LigmaColorBalanceConfigClass LigmaColorBalanceConfigClass;

struct _LigmaColorBalanceConfig
{
  LigmaOperationSettings  parent_instance;

  LigmaTransferMode       range;

  gdouble                cyan_red[3];
  gdouble                magenta_green[3];
  gdouble                yellow_blue[3];

  gboolean               preserve_luminosity;
};

struct _LigmaColorBalanceConfigClass
{
  LigmaOperationSettingsClass  parent_class;
};


GType   ligma_color_balance_config_get_type    (void) G_GNUC_CONST;

void    ligma_color_balance_config_reset_range (LigmaColorBalanceConfig *config);


#endif /* __LIGMA_COLOR_BALANCE_CONFIG_H__ */
