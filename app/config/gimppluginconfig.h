/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpPluginConfig class
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
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

#include "config/gimpdialogconfig.h"


#define GIMP_TYPE_PLUGIN_CONFIG            (gimp_plugin_config_get_type ())
#define GIMP_PLUGIN_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PLUGIN_CONFIG, GimpPluginConfig))
#define GIMP_PLUGIN_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PLUGIN_CONFIG, GimpPluginConfigClass))
#define GIMP_IS_PLUGIN_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PLUGIN_CONFIG))
#define GIMP_IS_PLUGIN_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PLUGIN_CONFIG))


typedef struct _GimpPluginConfigClass GimpPluginConfigClass;

struct _GimpPluginConfig
{
  GimpDialogConfig    parent_instance;

  gchar              *fractalexplorer_path;
  gchar              *gfig_path;
  gchar              *gflare_path;
  gchar              *gimpressionist_path;
  gchar              *script_fu_path;
};

struct _GimpPluginConfigClass
{
  GimpGuiConfigClass  parent_class;
};


GType  gimp_plugin_config_get_type (void) G_GNUC_CONST;
