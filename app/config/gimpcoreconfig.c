/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpCoreConfig class
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
#include "gimpcoreconfig.h"

#include "libgimp/gimpintl.h"


static void  gimp_core_config_class_init   (GimpCoreConfigClass *klass);
static void  gimp_core_config_finalize     (GObject             *object);
static void  gimp_core_config_set_property (GObject             *object,
                                            guint                property_id,
                                            const GValue        *value,
                                            GParamSpec          *pspec);
static void  gimp_core_config_get_property (GObject             *object,
                                            guint                property_id,
                                            GValue              *value,
                                            GParamSpec          *pspec);


#define DEFAULT_BRUSH     "Circle (11)"
#define DEFAULT_PATTERN   "Pine"
#define DEFAULT_PALETTE   "Default"
#define DEFAULT_GRADIENT  "FG to BG (RGB)"
#define DEFAULT_COMMENT   "Created with The GIMP"

enum
{
  PROP_0,
  PROP_INTERPOLATION_TYPE,
  PROP_PLUG_IN_PATH,
  PROP_TOOL_PLUG_IN_PATH,
  PROP_MODULE_PATH,
  PROP_ENVIRON_PATH,
  PROP_BRUSH_PATH,
  PROP_PATTERN_PATH,
  PROP_PALETTE_PATH,
  PROP_GRADIENT_PATH,
  PROP_DEFAULT_BRUSH,
  PROP_DEFAULT_PATTERN,
  PROP_DEFAULT_PALETTE,
  PROP_DEFAULT_GRADIENT,
  PROP_DEFAULT_COMMENT,
  PROP_DEFAULT_IMAGE_TYPE,
  PROP_DEFAULT_IMAGE_WIDTH,
  PROP_DEFAULT_IMAGE_HEIGHT,
  PROP_DEFAULT_UNIT,
  PROP_DEFAULT_XRESOLUTION,
  PROP_DEFAULT_YRESOLUTION,
  PROP_DEFAULT_RESOLUTION_UNIT,
  PROP_UNDO_LEVELS,
  PROP_UNDO_SIZE,
  PROP_PLUGINRC_PATH,
  PROP_MODULE_LOAD_INHIBIT,
  PROP_LAYER_PREVIEWS,
  PROP_LAYER_PREVIEW_SIZE,
  PROP_THUMBNAIL_SIZE,
  PROP_GAMMA_CORRECTION,
  PROP_INSTALL_COLORMAP,
  PROP_MIN_COLORS
};

static GObjectClass *parent_class = NULL;


GType 
gimp_core_config_get_type (void)
{
  static GType config_type = 0;

  if (! config_type)
    {
      static const GTypeInfo config_info =
      {
        sizeof (GimpCoreConfigClass),
	NULL,           /* base_init      */
        NULL,           /* base_finalize  */
	(GClassInitFunc) gimp_core_config_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpCoreConfig),
	0,              /* n_preallocs    */
	NULL            /* instance_init  */
      };

      config_type = g_type_register_static (GIMP_TYPE_BASE_CONFIG, 
                                            "GimpCoreConfig", 
                                            &config_info, 0);
    }

  return config_type;
}

static void
gimp_core_config_class_init (GimpCoreConfigClass *klass)
{
  GObjectClass *object_class;

  parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = gimp_core_config_finalize;
  object_class->set_property = gimp_core_config_set_property;
  object_class->get_property = gimp_core_config_get_property;

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_INTERPOLATION_TYPE,
                                 "interpolation-type",
                                 INTERPOLATION_TYPE_BLURB,
                                 GIMP_TYPE_INTERPOLATION_TYPE, 
                                 GIMP_INTERPOLATION_LINEAR,
                                 0);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_PLUG_IN_PATH,
                                 "plug-in-path", PLUG_IN_PATH_BLURB,
				 GIMP_PARAM_PATH_DIR_LIST,
                                 gimp_config_build_plug_in_path ("plug-ins"),
                                 GIMP_PARAM_RESTART);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_TOOL_PLUG_IN_PATH,
                                 "tool-plug-in-path", TOOL_PLUG_IN_PATH_BLURB,
				 GIMP_PARAM_PATH_DIR_LIST,
                                 gimp_config_build_plug_in_path ("tool-plug-ins"),
                                 GIMP_PARAM_RESTART);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_MODULE_PATH,
                                 "module-path", MODULE_PATH_BLURB,
				 GIMP_PARAM_PATH_DIR_LIST,
                                 gimp_config_build_plug_in_path ("modules"),
                                 GIMP_PARAM_RESTART);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_ENVIRON_PATH,
                                 "environ-path", ENVIRON_PATH_BLURB,
				 GIMP_PARAM_PATH_DIR_LIST,
                                 gimp_config_build_plug_in_path ("environ"),
                                 GIMP_PARAM_RESTART);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_BRUSH_PATH,
                                 "brush-path", BRUSH_PATH_BLURB,
				 GIMP_PARAM_PATH_DIR_LIST,
                                 gimp_config_build_data_path ("brushes"),
                                 GIMP_PARAM_RESTART);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_PATTERN_PATH,
                                 "pattern-path", PATTERN_PATH_BLURB,
				 GIMP_PARAM_PATH_DIR_LIST,
                                 gimp_config_build_data_path ("patterns"),
                                 GIMP_PARAM_RESTART);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_PALETTE_PATH,
                                 "palette-path", PALETTE_PATH_BLURB,
				 GIMP_PARAM_PATH_DIR_LIST,
                                 gimp_config_build_data_path ("palettes"),
                                 GIMP_PARAM_RESTART);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_GRADIENT_PATH,
                                 "gradient-path", GRADIENT_PATH_BLURB,
				 GIMP_PARAM_PATH_DIR_LIST,
                                 gimp_config_build_data_path ("gradients"),
                                 GIMP_PARAM_RESTART);
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_DEFAULT_BRUSH,
                                   "default-brush", DEFAULT_BRUSH_BLURB,
                                   DEFAULT_BRUSH,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_DEFAULT_PATTERN,
                                   "default-pattern", DEFAULT_PATTERN_BLURB,
                                   DEFAULT_PATTERN,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_DEFAULT_PALETTE,
                                   "default-palette", DEFAULT_PALETTE_BLURB,
                                   DEFAULT_PALETTE,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_DEFAULT_GRADIENT,
                                   "default-gradient", DEFAULT_GRADIENT_BLURB,
                                   DEFAULT_GRADIENT,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_DEFAULT_COMMENT,
                                   "default-comment", DEFAULT_COMMENT_BLURB,
                                   DEFAULT_COMMENT,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_DEFAULT_IMAGE_TYPE,
                                 "default-image-type",
                                 DEFAULT_IMAGE_TYPE_BLURB,
                                 GIMP_TYPE_IMAGE_BASE_TYPE, GIMP_RGB,
                                 0);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_DEFAULT_IMAGE_WIDTH,
                                "default-image-width",
                                DEFAULT_IMAGE_WIDTH_BLURB,
                                1, 0x8000, 256,
                                0);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_DEFAULT_IMAGE_HEIGHT,
                                "default-image-height",
                                DEFAULT_IMAGE_HEIGHT_BLURB,
                                1, 0x8000, 256,
                                0);
  GIMP_CONFIG_INSTALL_PROP_UNIT (object_class, PROP_DEFAULT_UNIT,
                                 "default-unit", DEFAULT_UNIT_BLURB,
                                 FALSE, GIMP_UNIT_INCH,
                                 0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_DEFAULT_XRESOLUTION,
                                   "default-xresolution",
                                   DEFAULT_XRESOLUTION_BLURB,
                                   GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION,
                                   72.0,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_DEFAULT_YRESOLUTION,
                                   "default-yresolution",
                                   DEFAULT_YRESOLUTION_BLURB,
                                   GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION,
                                   72.0,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_UNIT (object_class, PROP_DEFAULT_RESOLUTION_UNIT,
                                 "default-resolution-unit",
                                 DEFAULT_RESOLUTION_UNIT_BLURB,
                                 FALSE, GIMP_UNIT_INCH,
                                 0);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_UNDO_LEVELS,
                                "undo-levels", UNDO_LEVELS_BLURB,
                                0, G_MAXINT, 5,
                                GIMP_PARAM_CONFIRM);
  GIMP_CONFIG_INSTALL_PROP_MEMSIZE (object_class, PROP_UNDO_SIZE,
                                    "undo-size", UNDO_SIZE_BLURB,
                                    0, G_MAXULONG, 1 << 20,
                                    GIMP_PARAM_CONFIRM);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class,
                                 PROP_PLUGINRC_PATH,
                                 "pluginrc-path", PLUGINRC_PATH_BLURB,
				 GIMP_PARAM_PATH_FILE,
                                 "${gimp_dir}" G_DIR_SEPARATOR_S "pluginrc",
                                 GIMP_PARAM_RESTART);
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_MODULE_LOAD_INHIBIT,
                                   "module-load-inhibit",
                                   MODULE_LOAD_INHIBIT_BLURB,
                                   NULL,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_LAYER_PREVIEWS,
                                    "layer-previews", LAYER_PREVIEWS_BLURB,
                                    TRUE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_LAYER_PREVIEW_SIZE,
                                 "layer-preview-size", LAYER_PREVIEW_SIZE_BLURB,
                                 GIMP_TYPE_PREVIEW_SIZE,
                                 GIMP_PREVIEW_SIZE_SMALL,
                                 0);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_THUMBNAIL_SIZE,
                                 "thumbnail-size", THUMBNAIL_SIZE_BLURB,
                                 GIMP_TYPE_THUMBNAIL_SIZE,
                                 GIMP_THUMBNAIL_SIZE_NORMAL,
                                 0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_GAMMA_CORRECTION,
                                   "gamma-correction",
                                   GAMMA_CORRECTION_BLURB,
                                   0.0, 100.0, 1.0,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_INSTALL_COLORMAP,
                                    "install-colormap", INSTALL_COLORMAP_BLURB,
                                    FALSE,
                                    GIMP_PARAM_RESTART);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_MIN_COLORS,
                                "min-colors", MIN_COLORS_BLURB,
                                27, 256, 144,
                                GIMP_PARAM_RESTART);
}

static void
gimp_core_config_finalize (GObject *object)
{
  GimpCoreConfig *core_config;

  core_config = GIMP_CORE_CONFIG (object);
  
  g_free (core_config->plug_in_path);
  g_free (core_config->tool_plug_in_path);
  g_free (core_config->module_path);
  g_free (core_config->environ_path);
  g_free (core_config->brush_path);
  g_free (core_config->pattern_path);
  g_free (core_config->palette_path);
  g_free (core_config->gradient_path);
  g_free (core_config->default_brush);
  g_free (core_config->default_pattern);
  g_free (core_config->default_palette);
  g_free (core_config->default_gradient);
  g_free (core_config->default_comment);
  g_free (core_config->plug_in_rc_path);
  g_free (core_config->module_load_inhibit);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_core_config_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpCoreConfig *core_config;

  core_config = GIMP_CORE_CONFIG (object);

  switch (property_id)
    {
    case PROP_INTERPOLATION_TYPE:
      core_config->interpolation_type = g_value_get_enum (value);
      break;
    case PROP_PLUG_IN_PATH:
      g_free (core_config->plug_in_path);
      core_config->plug_in_path = g_value_dup_string (value);
      break;
    case PROP_TOOL_PLUG_IN_PATH:
      g_free (core_config->tool_plug_in_path);
      core_config->tool_plug_in_path = g_value_dup_string (value);
      break;
    case PROP_MODULE_PATH:
      g_free (core_config->module_path);
      core_config->module_path = g_value_dup_string (value);
      break;
    case PROP_ENVIRON_PATH:
      g_free (core_config->environ_path);
      core_config->environ_path = g_value_dup_string (value);
      break;
    case PROP_BRUSH_PATH:
      g_free (core_config->brush_path);
      core_config->brush_path = g_value_dup_string (value);
      break;
    case PROP_PATTERN_PATH:
      g_free (core_config->pattern_path);
      core_config->pattern_path = g_value_dup_string (value);
      break;
    case PROP_PALETTE_PATH:
      g_free (core_config->palette_path);
      core_config->palette_path = g_value_dup_string (value);
      break;
    case PROP_GRADIENT_PATH:
      g_free (core_config->gradient_path);
      core_config->gradient_path = g_value_dup_string (value);
      break;
    case PROP_DEFAULT_BRUSH:
      g_free (core_config->default_brush);
      core_config->default_brush = g_value_dup_string (value);
      break;
    case PROP_DEFAULT_PATTERN:
      g_free (core_config->default_pattern);
      core_config->default_pattern = g_value_dup_string (value);
      break;
    case PROP_DEFAULT_PALETTE:
      g_free (core_config->default_palette);
      core_config->default_palette = g_value_dup_string (value);
      break;
    case PROP_DEFAULT_GRADIENT:
      g_free (core_config->default_gradient);
      core_config->default_gradient = g_value_dup_string (value);
      break;
    case PROP_DEFAULT_COMMENT:
      g_free (core_config->default_comment);
      core_config->default_comment = g_value_dup_string (value);
      break;
    case PROP_DEFAULT_IMAGE_TYPE:
      core_config->default_image_type = g_value_get_enum (value);
      break;
    case PROP_DEFAULT_IMAGE_WIDTH:
      core_config->default_image_width = g_value_get_int (value);
      break;
    case PROP_DEFAULT_IMAGE_HEIGHT:
      core_config->default_image_height = g_value_get_int (value);
      break;
    case PROP_DEFAULT_UNIT:
      core_config->default_unit = g_value_get_int (value);
      break;
    case PROP_DEFAULT_XRESOLUTION:
      core_config->default_xresolution = g_value_get_double (value);
      break;
    case PROP_DEFAULT_YRESOLUTION:
      core_config->default_yresolution = g_value_get_double (value);
      break;
    case PROP_DEFAULT_RESOLUTION_UNIT:
      core_config->default_resolution_unit = g_value_get_int (value);
      break;
    case PROP_UNDO_LEVELS:
      core_config->levels_of_undo = g_value_get_int (value);
      break;
    case PROP_UNDO_SIZE:
      core_config->undo_size = g_value_get_ulong (value);
      break;
    case PROP_PLUGINRC_PATH:
      g_free (core_config->plug_in_rc_path);
      core_config->plug_in_rc_path = g_value_dup_string (value);
      break;
    case PROP_MODULE_LOAD_INHIBIT:
      g_free (core_config->module_load_inhibit);
      core_config->module_load_inhibit = g_value_dup_string (value);
      break;
    case PROP_LAYER_PREVIEWS:
      core_config->layer_previews = g_value_get_boolean (value);
      break;
    case PROP_LAYER_PREVIEW_SIZE:
      core_config->layer_preview_size = g_value_get_enum (value);
      break;
    case PROP_THUMBNAIL_SIZE:
      core_config->thumbnail_size = g_value_get_enum (value);
      break;
    case PROP_GAMMA_CORRECTION:
      core_config->gamma_val = g_value_get_double (value);
      break;
    case PROP_INSTALL_COLORMAP:
      core_config->install_cmap = g_value_get_boolean (value);
      break;
    case PROP_MIN_COLORS:
      core_config->min_colors = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_core_config_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpCoreConfig *core_config;

  core_config = GIMP_CORE_CONFIG (object);

  switch (property_id)
    {
    case PROP_INTERPOLATION_TYPE:
      g_value_set_enum (value, core_config->interpolation_type);
      break;
    case PROP_PLUG_IN_PATH:
      g_value_set_string (value, core_config->plug_in_path);
      break;
    case PROP_TOOL_PLUG_IN_PATH:
      g_value_set_string (value, core_config->tool_plug_in_path);
      break;
    case PROP_MODULE_PATH:
      g_value_set_string (value, core_config->module_path);
      break;
    case PROP_ENVIRON_PATH:
      g_value_set_string (value, core_config->environ_path);
      break;
    case PROP_BRUSH_PATH:
      g_value_set_string (value, core_config->brush_path);
      break;
    case PROP_PATTERN_PATH:
      g_value_set_string (value, core_config->pattern_path);
      break;
    case PROP_PALETTE_PATH:
      g_value_set_string (value, core_config->palette_path);
      break;
    case PROP_GRADIENT_PATH:
      g_value_set_string (value, core_config->gradient_path);
      break;
    case PROP_DEFAULT_BRUSH:
      g_value_set_string (value, core_config->default_brush);
      break;
    case PROP_DEFAULT_PATTERN:
      g_value_set_string (value, core_config->default_pattern);
      break;
    case PROP_DEFAULT_PALETTE:
      g_value_set_string (value, core_config->default_palette);
      break;
    case PROP_DEFAULT_GRADIENT:
      g_value_set_string (value, core_config->default_gradient);
      break;
    case PROP_DEFAULT_COMMENT:
      g_value_set_string (value, core_config->default_comment);
      break;
    case PROP_DEFAULT_IMAGE_TYPE:
      g_value_set_enum (value, core_config->default_image_type);
      break;
    case PROP_DEFAULT_IMAGE_WIDTH:
      g_value_set_int (value, core_config->default_image_width);
      break;
    case PROP_DEFAULT_IMAGE_HEIGHT:
      g_value_set_int (value, core_config->default_image_height);
      break;
    case PROP_DEFAULT_UNIT:
      g_value_set_int (value, core_config->default_unit);
      break;
    case PROP_DEFAULT_XRESOLUTION:
      g_value_set_double (value, core_config->default_xresolution);
      break;
    case PROP_DEFAULT_YRESOLUTION:
      g_value_set_double (value, core_config->default_yresolution);
      break;
    case PROP_DEFAULT_RESOLUTION_UNIT:
      g_value_set_int (value, core_config->default_resolution_unit);
      break;
    case PROP_UNDO_LEVELS:
      g_value_set_int (value, core_config->levels_of_undo);
      break;
    case PROP_UNDO_SIZE:
      g_value_set_ulong (value, core_config->undo_size);
      break;
    case PROP_PLUGINRC_PATH:
      g_value_set_string (value, core_config->plug_in_rc_path);
      break;
    case PROP_MODULE_LOAD_INHIBIT:
      g_value_set_string (value, core_config->module_load_inhibit);
      break;
    case PROP_LAYER_PREVIEWS:
      g_value_set_boolean (value, core_config->layer_previews);
      break;
    case PROP_LAYER_PREVIEW_SIZE:
      g_value_set_enum (value, core_config->layer_preview_size);
      break;
    case PROP_THUMBNAIL_SIZE:
      g_value_set_enum (value, core_config->thumbnail_size);
      break;
    case PROP_GAMMA_CORRECTION:
      g_value_set_double (value, core_config->gamma_val);
      break;
    case PROP_INSTALL_COLORMAP:
      g_value_set_boolean (value, core_config->install_cmap);
      break;
    case PROP_MIN_COLORS:
      g_value_set_int (value, core_config->min_colors);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
