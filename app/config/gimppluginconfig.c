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

#include "config.h"

#include <gio/gio.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "config-types.h"

#include "ligmarc-blurbs.h"
#include "ligmapluginconfig.h"


enum
{
  PROP_0,
  PROP_FRACTALEXPLORER_PATH,
  PROP_GFIG_PATH,
  PROP_GFLARE_PATH,
  PROP_LIGMARESSIONIST_PATH,
  PROP_SCRIPT_FU_PATH
};


static void  ligma_plugin_config_finalize     (GObject      *object);
static void  ligma_plugin_config_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void  ligma_plugin_config_get_property (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);


G_DEFINE_TYPE (LigmaPluginConfig, ligma_plugin_config, LIGMA_TYPE_DIALOG_CONFIG)

#define parent_class ligma_plugin_config_parent_class


static void
ligma_plugin_config_class_init (LigmaPluginConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  gchar        *path;

  object_class->finalize     = ligma_plugin_config_finalize;
  object_class->set_property = ligma_plugin_config_set_property;
  object_class->get_property = ligma_plugin_config_get_property;

  path = ligma_config_build_data_path ("fractalexplorer");
  LIGMA_CONFIG_PROP_PATH (object_class,
                         PROP_FRACTALEXPLORER_PATH,
                         "fractalexplorer-path",
                         "Fractal Explorer path",
                         FRACTALEXPLORER_PATH_BLURB,
                         LIGMA_CONFIG_PATH_DIR_LIST, path,
                         LIGMA_PARAM_STATIC_STRINGS);
  g_free (path);

  path = ligma_config_build_data_path ("gfig");
  LIGMA_CONFIG_PROP_PATH (object_class,
                         PROP_GFIG_PATH,
                         "gfig-path",
                         "GFig path",
                         GFIG_PATH_BLURB,
                         LIGMA_CONFIG_PATH_DIR_LIST, path,
                         LIGMA_PARAM_STATIC_STRINGS);
  g_free (path);

  path = ligma_config_build_data_path ("gflare");
  LIGMA_CONFIG_PROP_PATH (object_class,
                         PROP_GFLARE_PATH,
                         "gflare-path",
                         "GFlare path",
                         GFLARE_PATH_BLURB,
                         LIGMA_CONFIG_PATH_DIR_LIST, path,
                         LIGMA_PARAM_STATIC_STRINGS);
  g_free (path);

  path = ligma_config_build_data_path ("ligmaressionist");
  LIGMA_CONFIG_PROP_PATH (object_class,
                         PROP_LIGMARESSIONIST_PATH,
                         "ligmaressionist-path",
                         "LIGMAressionist path",
                         LIGMARESSIONIST_PATH_BLURB,
                         LIGMA_CONFIG_PATH_DIR_LIST, path,
                         LIGMA_PARAM_STATIC_STRINGS);
  g_free (path);

  path = ligma_config_build_data_path ("scripts");
  LIGMA_CONFIG_PROP_PATH (object_class,
                         PROP_SCRIPT_FU_PATH,
                         "script-fu-path",
                         "Script-Fu path",
                         SCRIPT_FU_PATH_BLURB,
                         LIGMA_CONFIG_PATH_DIR_LIST, path,
                         LIGMA_PARAM_STATIC_STRINGS);
  g_free (path);
}

static void
ligma_plugin_config_init (LigmaPluginConfig *config)
{
}

static void
ligma_plugin_config_finalize (GObject *object)
{
  LigmaPluginConfig *plugin_config = LIGMA_PLUGIN_CONFIG (object);

  g_free (plugin_config->fractalexplorer_path);
  g_free (plugin_config->gfig_path);
  g_free (plugin_config->gflare_path);
  g_free (plugin_config->ligmaressionist_path);
  g_free (plugin_config->script_fu_path);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_plugin_config_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  LigmaPluginConfig *plugin_config = LIGMA_PLUGIN_CONFIG (object);

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

    case PROP_LIGMARESSIONIST_PATH:
      g_free (plugin_config->ligmaressionist_path);
      plugin_config->ligmaressionist_path = g_value_dup_string (value);
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
ligma_plugin_config_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  LigmaPluginConfig *plugin_config = LIGMA_PLUGIN_CONFIG (object);

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

    case PROP_LIGMARESSIONIST_PATH:
      g_value_set_string (value, plugin_config->ligmaressionist_path);
      break;

    case PROP_SCRIPT_FU_PATH:
      g_value_set_string (value, plugin_config->script_fu_path);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
