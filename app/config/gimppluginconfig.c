/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpPluginConfig class
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "config-types.h"

#include "gimpconfig-params.h"
#include "gimpconfig-types.h"
#include "gimpconfig-utils.h"

#include "gimprc-blurbs.h"
#include "gimppluginconfig.h"


static void  gimp_plugin_config_class_init   (GimpPluginConfigClass *klass);
static void  gimp_plugin_config_finalize     (GObject       *object);
static void  gimp_plugin_config_set_property (GObject       *object,
                                              guint          property_id,
                                              const GValue  *value,
                                              GParamSpec    *pspec);
static void  gimp_plugin_config_get_property (GObject       *object,
                                              guint          property_id,
                                              GValue        *value,
                                              GParamSpec    *pspec);

enum
{
  PROP_0,
  PROP_FRACTALEXPLORER_PATH,
  PROP_GFIG_PATH,
  PROP_GFLARE_PATH,
  PROP_GIMPRESSIONIST_PATH,
  PROP_SCRIPT_FU_PATH
};

static GObjectClass *parent_class = NULL;


GType
gimp_plugin_config_get_type (void)
{
  static GType config_type = 0;

  if (! config_type)
    {
      static const GTypeInfo config_info =
      {
        sizeof (GimpPluginConfigClass),
	NULL,           /* base_init      */
        NULL,           /* base_finalize  */
	(GClassInitFunc) gimp_plugin_config_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpPluginConfig),
	0,              /* n_preallocs    */
	NULL            /* instance_init  */
      };

      config_type = g_type_register_static (GIMP_TYPE_GUI_CONFIG,
                                            "GimpPluginConfig",
                                            &config_info, 0);
    }

  return config_type;
}

static void
gimp_plugin_config_class_init (GimpPluginConfigClass *klass)
{
  GObjectClass *object_class;

  parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_plugin_config_finalize;
  object_class->set_property = gimp_plugin_config_set_property;
  object_class->get_property = gimp_plugin_config_get_property;

  GIMP_CONFIG_INSTALL_PROP_PATH (object_class,
                                 PROP_FRACTALEXPLORER_PATH,
                                 "fractalexplorer-path",
                                 FRACTALEXPLORER_PATH_BLURB,
				 GIMP_PARAM_PATH_DIR_LIST,
                                 gimp_config_build_data_path ("fractalexplorer"),
                                 0);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class,
                                 PROP_GFIG_PATH,
                                 "gfig-path", GFIG_PATH_BLURB,
				 GIMP_PARAM_PATH_DIR_LIST,
                                 gimp_config_build_data_path ("gfig"),
                                 0);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class,
                                 PROP_GFLARE_PATH,
                                 "gflare-path", GFLARE_PATH_BLURB,
				 GIMP_PARAM_PATH_DIR_LIST,
                                 gimp_config_build_data_path ("gflare"),
                                 0);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class,
                                 PROP_GIMPRESSIONIST_PATH,
                                 "gimpressionist-path",
                                 GIMPRESSIONIST_PATH_BLURB,
				 GIMP_PARAM_PATH_DIR_LIST,
                                 gimp_config_build_data_path ("gimpressionist"),
                                 0);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class,
                                 PROP_SCRIPT_FU_PATH,
                                 "script-fu-path",
                                 SCRIPT_FU_PATH_BLURB,
				 GIMP_PARAM_PATH_DIR_LIST,
                                 gimp_config_build_data_path ("scripts"),
                                 0);
}

static void
gimp_plugin_config_finalize (GObject *object)
{
  GimpPluginConfig *plugin_config = GIMP_PLUGIN_CONFIG (object);

  g_free (plugin_config->fractalexplorer_path);
  g_free (plugin_config->gfig_path);
  g_free (plugin_config->gflare_path);
  g_free (plugin_config->gimpressionist_path);
  g_free (plugin_config->script_fu_path);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_plugin_config_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpPluginConfig *plugin_config = GIMP_PLUGIN_CONFIG (object);

  switch (property_id)
    {
    case PROP_FRACTALEXPLORER_PATH:
      g_free (plugin_config->fractalexplorer_path);
      plugin_config->fractalexplorer_path = g_value_dup_string (value);
      break;

    case PROP_GFIG_PATH:
      g_free (plugin_config->gfig_path);
      plugin_config->gfig_path = g_value_dup_string (value);
      break;

    case PROP_GFLARE_PATH:
      g_free (plugin_config->gflare_path);
      plugin_config->gflare_path = g_value_dup_string (value);
      break;

    case PROP_GIMPRESSIONIST_PATH:
      g_free (plugin_config->gimpressionist_path);
      plugin_config->gimpressionist_path = g_value_dup_string (value);
      break;

    case PROP_SCRIPT_FU_PATH:
      g_free (plugin_config->script_fu_path);
      plugin_config->script_fu_path = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_plugin_config_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpPluginConfig *plugin_config = GIMP_PLUGIN_CONFIG (object);

  switch (property_id)
    {
    case PROP_FRACTALEXPLORER_PATH:
      g_value_set_string (value, plugin_config->fractalexplorer_path);
      break;

    case PROP_GFIG_PATH:
      g_value_set_string (value, plugin_config->gfig_path);
      break;

    case PROP_GFLARE_PATH:
      g_value_set_string (value, plugin_config->gflare_path);
      break;

    case PROP_GIMPRESSIONIST_PATH:
      g_value_set_string (value, plugin_config->gimpressionist_path);
      break;

    case PROP_SCRIPT_FU_PATH:
      g_value_set_string (value, plugin_config->script_fu_path);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
