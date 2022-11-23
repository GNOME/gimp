/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaPluginConfig class
 * Copyright (C) 2001  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_PLUGIN_CONFIG_H__
#define __LIGMA_PLUGIN_CONFIG_H__

#include "config/ligmadialogconfig.h"


#define LIGMA_TYPE_PLUGIN_CONFIG            (ligma_plugin_config_get_type ())
#define LIGMA_PLUGIN_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PLUGIN_CONFIG, LigmaPluginConfig))
#define LIGMA_PLUGIN_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PLUGIN_CONFIG, LigmaPluginConfigClass))
#define LIGMA_IS_PLUGIN_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PLUGIN_CONFIG))
#define LIGMA_IS_PLUGIN_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PLUGIN_CONFIG))


typedef struct _LigmaPluginConfigClass LigmaPluginConfigClass;

struct _LigmaPluginConfig
{
  LigmaDialogConfig    parent_instance;

  gchar              *fractalexplorer_path;
  gchar              *gfig_path;
  gchar              *gflare_path;
  gchar              *ligmaressionist_path;
  gchar              *script_fu_path;
};

struct _LigmaPluginConfigClass
{
  LigmaGuiConfigClass  parent_class;
};


GType  ligma_plugin_config_get_type (void) G_GNUC_CONST;


#endif /* LIGMA_PLUGIN_CONFIG_H__ */
