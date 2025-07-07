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

#include "config.h"

#include <gio/gio.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "config-types.h"

#include "gimprc-blurbs.h"
#include "gimppluginconfig.h"


enum
{
  PROP_0,
  PROP_FRACTALEXPLORER_PATH,
  PROP_GFIG_PATH,
  PROP_GFLARE_PATH,
  PROP_GIMPRESSIONIST_PATH,
  PROP_SCRIPT_FU_PATH
};


static void  gimp_plugin_config_finalize     (GObject      *object);
static void  gimp_plugin_config_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void  gimp_plugin_config_get_property (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);


G_DEFINE_TYPE (GimpPluginConfig, gimp_plugin_config, GIMP_TYPE_DIALOG_CONFIG)

#define parent_class gimp_plugin_config_parent_class


static void
gimp_plugin_config_class_init (GimpPluginConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  gchar        *path;

  object_class->finalize     = gimp_plugin_config_finalize;
  object_class->set_property = gimp_plugin_config_set_property;
  object_class->get_property = gimp_plugin_config_get_property;

  path = gimp_config_build_data_path ("fractalexplorer");
  GIMP_CONFIG_PROP_PATH (object_class,
                         PROP_FRACTALEXPLORER_PATH,
                         "fractalexplorer-path",
                         "Fractal Explorer path",
                         FRACTALEXPLORER_PATH_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS);
  g_free (path);

  path = gimp_config_build_data_path ("gfig");
  GIMP_CONFIG_PROP_PATH (object_class,
                         PROP_GFIG_PATH,
                         "gfig-path",
                         "GFig path",
                         GFIG_PATH_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS);
  g_free (path);

  path = gimp_config_build_data_path ("gflare");
  GIMP_CONFIG_PROP_PATH (object_class,
                         PROP_GFLARE_PATH,
                         "gflare-path",
                         "GFlare path",
                         GFLARE_PATH_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS);
  g_free (path);

  path = gimp_config_build_data_path ("gimpressionist");
  GIMP_CONFIG_PROP_PATH (object_class,
                         PROP_GIMPRESSIONIST_PATH,
                         "gimpressionist-path",
                         "GIMPressionist path",
                         GIMPRESSIONIST_PATH_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS);
  g_free (path);

  path = gimp_config_build_data_path ("scripts");
  GIMP_CONFIG_PROP_PATH (object_class,
                         PROP_SCRIPT_FU_PATH,
                         "script-fu-path",
                         "Script-Fu path",
                         SCRIPT_FU_PATH_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS);
  g_free (path);
}

static void
gimp_plugin_config_init (GimpPluginConfig *config)
{
}

static void
gimp_plugin_config_finalize (GObject *object)
{
  GimpPluginConfig *plugin_config = GIMP_PLUGIN_CONFIG (object);

  g_clear_pointer (&plugin_config->fractalexplorer_path, g_free);
  g_clear_pointer (&plugin_config->gfig_path,            g_free);
  g_clear_pointer (&plugin_config->gflare_path,          g_free);
  g_clear_pointer (&plugin_config->gimpressionist_path,  g_free);
  g_clear_pointer (&plugin_config->script_fu_path,       g_free);

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
      g_set_str (&plugin_config->fractalexplorer_path, g_value_get_string (value));
      break;
    case PROP_GFIG_PATH:
      g_set_str (&plugin_config->gfig_path, g_value_get_string (value));
      break;
    case PROP_GFLARE_PATH:
      g_set_str (&plugin_config->gflare_path, g_value_get_string (value));
      break;
    case PROP_GIMPRESSIONIST_PATH:
      g_set_str (&plugin_config->gimpressionist_path, g_value_get_string (value));
      break;
    case PROP_SCRIPT_FU_PATH:
      g_set_str (&plugin_config->script_fu_path, g_value_get_string (value));
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
