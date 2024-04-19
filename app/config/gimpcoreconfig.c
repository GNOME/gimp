/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpCoreConfig class
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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "config-types.h"

#include "core/core-types.h"
#include "core/gimp-utils.h"
#include "core/gimpgrid.h"
#include "core/gimptemplate.h"

#include "gimprc-blurbs.h"
#include "gimpcoreconfig.h"

#include "gimp-intl.h"


#define GIMP_DEFAULT_BRUSH         "2. Hardness 050"
#define GIMP_DEFAULT_DYNAMICS      "Pressure Size"
#define GIMP_DEFAULT_PATTERN       "Pine"
#define GIMP_DEFAULT_PALETTE       "Default"
#define GIMP_DEFAULT_GRADIENT      "FG to BG (RGB)"
#define GIMP_DEFAULT_TOOL_PRESET   "Current Options"
#define GIMP_DEFAULT_FONT          "Sans-serif"
#define GIMP_DEFAULT_MYPAINT_BRUSH "Fixme"
#define GIMP_DEFAULT_COMMENT       "Created with GIMP"


enum
{
  PROP_0,
  PROP_LANGUAGE,
  PROP_PREV_LANGUAGE,
  PROP_CONFIG_VERSION,
  PROP_INTERPOLATION_TYPE,
  PROP_DEFAULT_THRESHOLD,
  PROP_PLUG_IN_PATH,
  PROP_MODULE_PATH,
  PROP_INTERPRETER_PATH,
  PROP_ENVIRON_PATH,
  PROP_BRUSH_PATH,
  PROP_BRUSH_PATH_WRITABLE,
  PROP_DYNAMICS_PATH,
  PROP_DYNAMICS_PATH_WRITABLE,
  PROP_MYPAINT_BRUSH_PATH,
  PROP_MYPAINT_BRUSH_PATH_WRITABLE,
  PROP_PATTERN_PATH,
  PROP_PATTERN_PATH_WRITABLE,
  PROP_PALETTE_PATH,
  PROP_PALETTE_PATH_WRITABLE,
  PROP_GRADIENT_PATH,
  PROP_GRADIENT_PATH_WRITABLE,
  PROP_TOOL_PRESET_PATH,
  PROP_TOOL_PRESET_PATH_WRITABLE,
  PROP_FONT_PATH,
  PROP_FONT_PATH_WRITABLE,
  PROP_DEFAULT_BRUSH,
  PROP_DEFAULT_DYNAMICS,
  PROP_DEFAULT_MYPAINT_BRUSH,
  PROP_DEFAULT_PATTERN,
  PROP_DEFAULT_PALETTE,
  PROP_DEFAULT_GRADIENT,
  PROP_DEFAULT_TOOL_PRESET,
  PROP_DEFAULT_FONT,
  PROP_GLOBAL_BRUSH,
  PROP_GLOBAL_DYNAMICS,
  PROP_GLOBAL_PATTERN,
  PROP_GLOBAL_PALETTE,
  PROP_GLOBAL_GRADIENT,
  PROP_GLOBAL_FONT,
  PROP_GLOBAL_EXPAND,
  PROP_DEFAULT_IMAGE,
  PROP_DEFAULT_GRID,
  PROP_UNDO_LEVELS,
  PROP_UNDO_SIZE,
  PROP_UNDO_PREVIEW_SIZE,
  PROP_FILTER_HISTORY_SIZE,
  PROP_PLUGINRC_PATH,
  PROP_LAYER_PREVIEWS,
  PROP_GROUP_LAYER_PREVIEWS,
  PROP_LAYER_PREVIEW_SIZE,
  PROP_THUMBNAIL_SIZE,
  PROP_THUMBNAIL_FILESIZE_LIMIT,
  PROP_COLOR_MANAGEMENT,
  PROP_SAVE_DOCUMENT_HISTORY,
  PROP_QUICK_MASK_COLOR,
  PROP_IMPORT_PROMOTE_FLOAT,
  PROP_IMPORT_PROMOTE_DITHER,
  PROP_IMPORT_ADD_ALPHA,
  PROP_IMPORT_RAW_PLUG_IN,
  PROP_EXPORT_FILE_TYPE,
  PROP_EXPORT_COLOR_PROFILE,
  PROP_EXPORT_COMMENT,
  PROP_EXPORT_THUMBNAIL,
  PROP_EXPORT_METADATA_EXIF,
  PROP_EXPORT_METADATA_XMP,
  PROP_EXPORT_METADATA_IPTC,
  PROP_DEBUG_POLICY,
  PROP_CHECK_UPDATES,
  PROP_CHECK_UPDATE_TIMESTAMP,
  PROP_LAST_RELEASE_TIMESTAMP,
  PROP_LAST_RELEASE_COMMENT,
  PROP_LAST_REVISION,
  PROP_LAST_KNOWN_RELEASE,
#ifdef G_OS_WIN32
  PROP_WIN32_POINTER_INPUT_API,
#endif
  PROP_ITEMS_SELECT_METHOD,

  /* ignored, only for backward compatibility: */
  PROP_INSTALL_COLORMAP,
  PROP_MIN_COLORS
};


static void  gimp_core_config_finalize               (GObject      *object);
static void  gimp_core_config_set_property           (GObject      *object,
                                                      guint         property_id,
                                                      const GValue *value,
                                                      GParamSpec   *pspec);
static void  gimp_core_config_get_property           (GObject      *object,
                                                      guint         property_id,
                                                      GValue       *value,
                                                      GParamSpec   *pspec);
static void gimp_core_config_default_image_notify    (GObject      *object,
                                                      GParamSpec   *pspec,
                                                      gpointer      data);
static void gimp_core_config_default_grid_notify     (GObject      *object,
                                                      GParamSpec   *pspec,
                                                      gpointer      data);
static void gimp_core_config_color_management_notify (GObject      *object,
                                                      GParamSpec   *pspec,
                                                      gpointer      data);


G_DEFINE_TYPE (GimpCoreConfig, gimp_core_config, GIMP_TYPE_GEGL_CONFIG)

#define parent_class gimp_core_config_parent_class


static void
gimp_core_config_class_init (GimpCoreConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  gchar        *path;
  gchar        *mypaint_brushes;
  GeglColor    *red          = gegl_color_new ("red");
  guint64       undo_size;

  gimp_color_set_alpha (red, 0.5);

  object_class->finalize     = gimp_core_config_finalize;
  object_class->set_property = gimp_core_config_set_property;
  object_class->get_property = gimp_core_config_get_property;

  GIMP_CONFIG_PROP_STRING (object_class, PROP_LANGUAGE,
                           "language",
                           "Language",
                           LANGUAGE_BLURB,
                           NULL,  /* take from environment */
                           GIMP_PARAM_STATIC_STRINGS |
                           GIMP_CONFIG_PARAM_RESTART);

  /* The language which was being used previously. If the "language"
   * property was at default (i.e. System language), this
   * must map to the actually used language for UI display, because if
   * this changed (for whatever reasons, e.g. changed environment
   * variables, or actually changing system language), we want to reload
   * plug-ins.
   */
  GIMP_CONFIG_PROP_STRING (object_class, PROP_PREV_LANGUAGE,
                           "prev-language",
                           "Language used in previous run",
                           NULL, NULL,
                           GIMP_PARAM_STATIC_STRINGS);

  /* This is the version of the config files, which must map to the
   * version of GIMP. It is used right now only to detect the last run
   * version in order to detect an update. It could be used later also
   * to have more fine-grained config updates (not just on minor
   * versions as we do now, but also on changes in micro versions).
   */
  GIMP_CONFIG_PROP_STRING (object_class, PROP_CONFIG_VERSION,
                           "config-version",
                           "Version of GIMP config files",
                           CONFIG_VERSION_BLURB,
                           NULL,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_INTERPOLATION_TYPE,
                         "interpolation-type",
                         "Interpolation",
                         INTERPOLATION_TYPE_BLURB,
                         GIMP_TYPE_INTERPOLATION_TYPE,
                         GIMP_INTERPOLATION_CUBIC,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_INT (object_class, PROP_DEFAULT_THRESHOLD,
                        "default-threshold",
                        "Default threshold",
                        DEFAULT_THRESHOLD_BLURB,
                        0, 255, 15,
                        GIMP_PARAM_STATIC_STRINGS);

  path = gimp_config_build_plug_in_path ("plug-ins");
  GIMP_CONFIG_PROP_PATH (object_class, PROP_PLUG_IN_PATH,
                         "plug-in-path",
                         "Plug-in path",
                         PLUG_IN_PATH_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_RESTART);
  g_free (path);

  path = gimp_config_build_plug_in_path ("modules");
  GIMP_CONFIG_PROP_PATH (object_class, PROP_MODULE_PATH,
                         "module-path",
                         "Module path",
                         MODULE_PATH_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_RESTART);
  g_free (path);

  path = gimp_config_build_plug_in_path ("interpreters");
  GIMP_CONFIG_PROP_PATH (object_class, PROP_INTERPRETER_PATH,
                         "interpreter-path",
                         "Interpreter path",
                         INTERPRETER_PATH_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_RESTART);
  g_free (path);

  path = gimp_config_build_plug_in_path ("environ");
  GIMP_CONFIG_PROP_PATH (object_class, PROP_ENVIRON_PATH,
                         "environ-path",
                         "Environment path",
                         ENVIRON_PATH_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_RESTART);
  g_free (path);

  path = gimp_config_build_data_path ("brushes");
  GIMP_CONFIG_PROP_PATH (object_class, PROP_BRUSH_PATH,
                         "brush-path",
                         "Brush path",
                         BRUSH_PATH_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_CONFIRM);
  g_free (path);

  path = gimp_config_build_writable_path ("brushes");
  GIMP_CONFIG_PROP_PATH (object_class, PROP_BRUSH_PATH_WRITABLE,
                         "brush-path-writable",
                         "Writable brush path",
                         BRUSH_PATH_WRITABLE_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_CONFIRM);
  g_free (path);

  path = gimp_config_build_data_path ("dynamics");
  GIMP_CONFIG_PROP_PATH (object_class, PROP_DYNAMICS_PATH,
                         "dynamics-path",
                         "Dynamics path",
                         DYNAMICS_PATH_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_CONFIRM);
  g_free (path);

  path = gimp_config_build_writable_path ("dynamics");
  GIMP_CONFIG_PROP_PATH (object_class, PROP_DYNAMICS_PATH_WRITABLE,
                         "dynamics-path-writable",
                         "Writable dynamics path",
                         DYNAMICS_PATH_WRITABLE_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_CONFIRM);
  g_free (path);

#ifdef ENABLE_RELOCATABLE_RESOURCES
  mypaint_brushes = g_build_filename ("${gimp_installation_dir}",
                                      "share", "mypaint-data",
                                      "1.0", "brushes", NULL);
#else
  mypaint_brushes = g_strdup (MYPAINT_BRUSHES_DIR);
#endif

  path = g_build_path (G_SEARCHPATH_SEPARATOR_S,
                       "~/.mypaint/brushes",
                       mypaint_brushes,
                       NULL);
  g_free (mypaint_brushes);

  GIMP_CONFIG_PROP_PATH (object_class, PROP_MYPAINT_BRUSH_PATH,
                         "mypaint-brush-path",
                         "MyPaint brush path",
                         MYPAINT_BRUSH_PATH_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_CONFIRM);
  g_free (path);

  path = g_build_path (G_SEARCHPATH_SEPARATOR_S,
                       "~/.mypaint/brushes",
                       NULL);
  GIMP_CONFIG_PROP_PATH (object_class, PROP_MYPAINT_BRUSH_PATH_WRITABLE,
                         "mypaint-brush-path-writable",
                         "Writable MyPaint brush path",
                         MYPAINT_BRUSH_PATH_WRITABLE_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_CONFIRM);
  g_free (path);

  path = gimp_config_build_data_path ("patterns");
  GIMP_CONFIG_PROP_PATH (object_class, PROP_PATTERN_PATH,
                         "pattern-path",
                         "Pattern path",
                         PATTERN_PATH_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_CONFIRM);
  g_free (path);

  path = gimp_config_build_writable_path ("patterns");
  GIMP_CONFIG_PROP_PATH (object_class, PROP_PATTERN_PATH_WRITABLE,
                         "pattern-path-writable",
                         "Writable pattern path",
                         PATTERN_PATH_WRITABLE_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_CONFIRM);
  g_free (path);

  path = gimp_config_build_data_path ("palettes");
  GIMP_CONFIG_PROP_PATH (object_class, PROP_PALETTE_PATH,
                         "palette-path",
                         "Palette path",
                         PALETTE_PATH_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_CONFIRM);
  g_free (path);

  path = gimp_config_build_writable_path ("palettes");
  GIMP_CONFIG_PROP_PATH (object_class, PROP_PALETTE_PATH_WRITABLE,
                         "palette-path-writable",
                         "Writable palette path",
                         PALETTE_PATH_WRITABLE_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_CONFIRM);
  g_free (path);

  path = gimp_config_build_data_path ("gradients");
  GIMP_CONFIG_PROP_PATH (object_class, PROP_GRADIENT_PATH,
                         "gradient-path",
                         "Gradient path",
                         GRADIENT_PATH_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_CONFIRM);
  g_free (path);

  path = gimp_config_build_writable_path ("gradients");
  GIMP_CONFIG_PROP_PATH (object_class, PROP_GRADIENT_PATH_WRITABLE,
                         "gradient-path-writable",
                         "Writable gradient path",
                         GRADIENT_PATH_WRITABLE_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_CONFIRM);
  g_free (path);

  path = gimp_config_build_data_path ("tool-presets");
  GIMP_CONFIG_PROP_PATH (object_class, PROP_TOOL_PRESET_PATH,
                         "tool-preset-path",
                         "Tool preset path",
                         TOOL_PRESET_PATH_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_CONFIRM);
  g_free (path);

  path = gimp_config_build_writable_path ("tool-presets");
  GIMP_CONFIG_PROP_PATH (object_class, PROP_TOOL_PRESET_PATH_WRITABLE,
                         "tool-preset-path-writable",
                         "Writable tool preset path",
                         TOOL_PRESET_PATH_WRITABLE_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_CONFIRM);
  g_free (path);

  path = gimp_config_build_data_path ("fonts");
  GIMP_CONFIG_PROP_PATH (object_class, PROP_FONT_PATH,
                         "font-path",
                         "Font path",
                         FONT_PATH_BLURB,
                         GIMP_CONFIG_PATH_DIR_LIST, path,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_CONFIRM);
  g_free (path);

  GIMP_CONFIG_PROP_PATH (object_class, PROP_FONT_PATH_WRITABLE,
                         "font-path-writable",
                         "Writable font path",
                         NULL,
                         GIMP_CONFIG_PATH_DIR_LIST, NULL,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_IGNORE);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_DEFAULT_BRUSH,
                           "default-brush",
                           "Default brush",
                           DEFAULT_BRUSH_BLURB,
                           GIMP_DEFAULT_BRUSH,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_DEFAULT_DYNAMICS,
                           "default-dynamics",
                           "Default dynamics",
                           DEFAULT_DYNAMICS_BLURB,
                           GIMP_DEFAULT_DYNAMICS,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_DEFAULT_MYPAINT_BRUSH,
                           "default-mypaint-brush",
                           "Default MyPaint brush",
                           DEFAULT_MYPAINT_BRUSH_BLURB,
                           GIMP_DEFAULT_MYPAINT_BRUSH,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_DEFAULT_PATTERN,
                           "default-pattern",
                           "Default pattern",
                           DEFAULT_PATTERN_BLURB,
                           GIMP_DEFAULT_PATTERN,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_DEFAULT_PALETTE,
                           "default-palette",
                           "Default palette",
                           DEFAULT_PALETTE_BLURB,
                           GIMP_DEFAULT_PALETTE,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_DEFAULT_GRADIENT,
                           "default-gradient",
                           "Default gradient",
                           DEFAULT_GRADIENT_BLURB,
                           GIMP_DEFAULT_GRADIENT,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_DEFAULT_TOOL_PRESET,
                           "default-tool-preset",
                           "Default tool preset",
                           DEFAULT_TOOL_PRESET_BLURB,
                           GIMP_DEFAULT_TOOL_PRESET,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_DEFAULT_FONT,
                           "default-font",
                           "Default font",
                           DEFAULT_FONT_BLURB,
                           GIMP_DEFAULT_FONT,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_GLOBAL_BRUSH,
                            "global-brush",
                            "Global brush",
                            GLOBAL_BRUSH_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_GLOBAL_DYNAMICS,
                            "global-dynamics",
                            "Global dynamics",
                            GLOBAL_DYNAMICS_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_GLOBAL_PATTERN,
                            "global-pattern",
                            "Global pattern",
                            GLOBAL_PATTERN_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_GLOBAL_PALETTE,
                            "global-palette",
                            "Global palette",
                            GLOBAL_PALETTE_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_GLOBAL_GRADIENT,
                            "global-gradient",
                            "Global gradient",
                            GLOBAL_GRADIENT_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_GLOBAL_FONT,
                            "global-font",
                            "Global font",
                            GLOBAL_FONT_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_GLOBAL_EXPAND,
                            "global-expand",
                            "Global expand",
                            GLOBAL_EXPAND_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_DEFAULT_IMAGE,
                           "default-image",
                           "Default image",
                           DEFAULT_IMAGE_BLURB,
                           GIMP_TYPE_TEMPLATE,
                           GIMP_PARAM_STATIC_STRINGS |
                           GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_DEFAULT_GRID,
                           "default-grid",
                           "Default grid",
                           DEFAULT_GRID_BLURB,
                           GIMP_TYPE_GRID,
                           GIMP_PARAM_STATIC_STRINGS |
                           GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_PROP_INT (object_class, PROP_UNDO_LEVELS,
                        "undo-levels",
                        "Undo levels",
                        UNDO_LEVELS_BLURB,
                        0, 1 << 20, 5,
                        GIMP_PARAM_STATIC_STRINGS |
                        GIMP_CONFIG_PARAM_CONFIRM);

  undo_size = gimp_get_physical_memory_size ();

  if (undo_size > 0)
    undo_size = undo_size / 8; /* 1/8th of the memory */
  else
    undo_size = 1 << 26; /* 64GB */

  GIMP_CONFIG_PROP_MEMSIZE (object_class, PROP_UNDO_SIZE,
                            "undo-size",
                            "Undo size",
                            UNDO_SIZE_BLURB,
                            0, GIMP_MAX_MEMSIZE, undo_size,
                            GIMP_PARAM_STATIC_STRINGS |
                            GIMP_CONFIG_PARAM_CONFIRM);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_UNDO_PREVIEW_SIZE,
                         "undo-preview-size",
                         "Undo preview size",
                         UNDO_PREVIEW_SIZE_BLURB,
                         GIMP_TYPE_VIEW_SIZE,
                         GIMP_VIEW_SIZE_LARGE,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_RESTART);

  GIMP_CONFIG_PROP_INT (object_class, PROP_FILTER_HISTORY_SIZE,
                        "plug-in-history-size", /* compat name */
                        "Filter history size",
                        FILTER_HISTORY_SIZE_BLURB,
                        0, 256, 10,
                        GIMP_PARAM_STATIC_STRINGS |
                        GIMP_CONFIG_PARAM_RESTART);

  GIMP_CONFIG_PROP_PATH (object_class,
                         PROP_PLUGINRC_PATH,
                         "pluginrc-path",
                         "plugninrc path",
                         PLUGINRC_PATH_BLURB,
                         GIMP_CONFIG_PATH_FILE,
                         "${gimp_dir}" G_DIR_SEPARATOR_S "pluginrc",
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_RESTART);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_LAYER_PREVIEWS,
                            "layer-previews",
                            "Layer previews",
                            LAYER_PREVIEWS_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_GROUP_LAYER_PREVIEWS,
                            "group-layer-previews",
                            "Layer group previews",
                            GROUP_LAYER_PREVIEWS_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_LAYER_PREVIEW_SIZE,
                         "layer-preview-size",
                         "Layer preview size",
                         LAYER_PREVIEW_SIZE_BLURB,
                         GIMP_TYPE_VIEW_SIZE,
                         GIMP_VIEW_SIZE_MEDIUM,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_THUMBNAIL_SIZE,
                         "thumbnail-size",
                         "Thumbnail size",
                         THUMBNAIL_SIZE_BLURB,
                         GIMP_TYPE_THUMBNAIL_SIZE,
                         GIMP_THUMBNAIL_SIZE_NORMAL,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_MEMSIZE (object_class, PROP_THUMBNAIL_FILESIZE_LIMIT,
                            "thumbnail-filesize-limit",
                            "Thumbnail file size limit",
                            THUMBNAIL_FILESIZE_LIMIT_BLURB,
                            0, GIMP_MAX_MEMSIZE, 1 << 22,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_COLOR_MANAGEMENT,
                           "color-management",
                           "Color management",
                           COLOR_MANAGEMENT_BLURB,
                           GIMP_TYPE_COLOR_CONFIG,
                           GIMP_PARAM_STATIC_STRINGS |
                           GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_CHECK_UPDATES,
                            "check-updates",
                            "Check for updates",
                            CHECK_UPDATES_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_INT64 (object_class, PROP_CHECK_UPDATE_TIMESTAMP,
                          "check-update-timestamp",
                          "timestamp of the last update check",
                          CHECK_UPDATE_TIMESTAMP_BLURB,
                          0, G_MAXINT64, 0,
                          GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_INT64 (object_class, PROP_LAST_RELEASE_TIMESTAMP,
                          "last-release-timestamp",
                          "timestamp of the last release",
                          LAST_RELEASE_TIMESTAMP_BLURB,
                          0, G_MAXINT64, 0,
                          GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_LAST_RELEASE_COMMENT,
                           "last-release-comment",
                           "Comment for last release",
                           LAST_KNOWN_RELEASE_BLURB,
                           NULL,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_LAST_KNOWN_RELEASE,
                           "last-known-release",
                           "last known release of GIMP",
                           LAST_KNOWN_RELEASE_BLURB,
                           NULL,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_INT (object_class, PROP_LAST_REVISION,
                        "last-revision",
                        "Last revision of current release",
                        LAST_RELEASE_TIMESTAMP_BLURB,
                        0, G_MAXINT, 0,
                        GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SAVE_DOCUMENT_HISTORY,
                            "save-document-history",
                            "Save document history",
                            SAVE_DOCUMENT_HISTORY_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_COLOR (object_class, PROP_QUICK_MASK_COLOR,
                          "quick-mask-color",
                          "Quick mask color",
                          QUICK_MASK_COLOR_BLURB,
                          TRUE, red,
                          GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_IMPORT_PROMOTE_FLOAT,
                            "import-promote-float",
                            "Import promote float",
                            IMPORT_PROMOTE_FLOAT_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_IMPORT_PROMOTE_DITHER,
                            "import-promote-dither",
                            "Import promote dither",
                            IMPORT_PROMOTE_DITHER_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_IMPORT_ADD_ALPHA,
                            "import-add-alpha",
                            "Import add alpha",
                            IMPORT_ADD_ALPHA_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_PATH (object_class, PROP_IMPORT_RAW_PLUG_IN,
                         "import-raw-plug-in",
                         "Import raw plug-in",
                         IMPORT_RAW_PLUG_IN_BLURB,
                         GIMP_CONFIG_PATH_FILE,
                         "",
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_RESTART);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_EXPORT_FILE_TYPE,
                         "export-file-type",
                         "Default export file type",
                         EXPORT_FILE_TYPE_BLURB,
                         GIMP_TYPE_EXPORT_FILE_TYPE,
                         GIMP_EXPORT_FILE_PNG,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_EXPORT_COLOR_PROFILE,
                            "export-color-profile",
                            "Export Color Profile",
                            EXPORT_COLOR_PROFILE_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_EXPORT_COMMENT,
                            "export-comment",
                            "Export Comment",
                            EXPORT_COMMENT_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_EXPORT_THUMBNAIL,
                            "export-thumbnail",
                            "Export Thumbnail",
                            EXPORT_THUMBNAIL_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_EXPORT_METADATA_EXIF,
                            "export-metadata-exif",
                            "Export Exif metadata",
                            EXPORT_METADATA_EXIF_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_EXPORT_METADATA_XMP,
                            "export-metadata-xmp",
                            "Export XMP metadata",
                            EXPORT_METADATA_XMP_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_EXPORT_METADATA_IPTC,
                            "export-metadata-iptc",
                            "Export IPTC metadata",
                            EXPORT_METADATA_IPTC_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_DEBUG_POLICY,
                         "debug-policy",
                         "Try generating backtrace upon errors",
                         GENERATE_BACKTRACE_BLURB,
                         GIMP_TYPE_DEBUG_POLICY,
#ifdef GIMP_UNSTABLE
                         GIMP_DEBUG_POLICY_WARNING,
#else
                         GIMP_DEBUG_POLICY_FATAL,
#endif
                         GIMP_PARAM_STATIC_STRINGS);

#ifdef G_OS_WIN32
  GIMP_CONFIG_PROP_ENUM (object_class, PROP_WIN32_POINTER_INPUT_API,
                         "win32-pointer-input-api",
                         "Pointer Input API",
                         WIN32_POINTER_INPUT_API_BLURB,
                         GIMP_TYPE_WIN32_POINTER_INPUT_API,
                         GIMP_WIN32_POINTER_INPUT_API_WINDOWS_INK,
                         GIMP_PARAM_STATIC_STRINGS |
                         GIMP_CONFIG_PARAM_RESTART);
#endif

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_ITEMS_SELECT_METHOD,
                         "items-select-method",
                         _("Pattern syntax for searching and selecting items:"),
                         ITEMS_SELECT_METHOD_BLURB,
                         GIMP_TYPE_SELECT_METHOD,
                         GIMP_SELECT_PLAIN_TEXT,
                         GIMP_PARAM_STATIC_STRINGS);

  /*  only for backward compatibility:  */
  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_INSTALL_COLORMAP,
                            "install-colormap",
                            NULL, NULL,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS |
                            GIMP_CONFIG_PARAM_IGNORE);

  GIMP_CONFIG_PROP_INT (object_class, PROP_MIN_COLORS,
                        "min-colors",
                        NULL, NULL,
                        27, 256, 144,
                        GIMP_PARAM_STATIC_STRINGS |
                        GIMP_CONFIG_PARAM_IGNORE);

  g_object_unref (red);
}

static void
gimp_core_config_init (GimpCoreConfig *config)
{
  GeglColor *red = gegl_color_new ("red");

  gimp_color_set_alpha (red, 0.5);
  config->quick_mask_color = red;

  config->default_image = g_object_new (GIMP_TYPE_TEMPLATE,
                                        "name",    "Default Image",
                                        "comment", GIMP_DEFAULT_COMMENT,
                                        NULL);
  g_signal_connect (config->default_image, "notify",
                    G_CALLBACK (gimp_core_config_default_image_notify),
                    config);

  config->default_grid = g_object_new (GIMP_TYPE_GRID,
                                       "name", "Default Grid",
                                       NULL);
  g_signal_connect (config->default_grid, "notify",
                    G_CALLBACK (gimp_core_config_default_grid_notify),
                    config);

  config->color_management = g_object_new (GIMP_TYPE_COLOR_CONFIG, NULL);
  g_signal_connect (config->color_management, "notify",
                    G_CALLBACK (gimp_core_config_color_management_notify),
                    config);
}

static void
gimp_core_config_finalize (GObject *object)
{
  GimpCoreConfig *core_config = GIMP_CORE_CONFIG (object);

  g_free (core_config->language);
  g_free (core_config->prev_language);
  g_free (core_config->plug_in_path);
  g_free (core_config->module_path);
  g_free (core_config->interpreter_path);
  g_free (core_config->environ_path);
  g_free (core_config->brush_path);
  g_free (core_config->brush_path_writable);
  g_free (core_config->dynamics_path);
  g_free (core_config->dynamics_path_writable);
  g_free (core_config->mypaint_brush_path);
  g_free (core_config->mypaint_brush_path_writable);
  g_free (core_config->pattern_path);
  g_free (core_config->pattern_path_writable);
  g_free (core_config->palette_path);
  g_free (core_config->palette_path_writable);
  g_free (core_config->gradient_path);
  g_free (core_config->gradient_path_writable);
  g_free (core_config->tool_preset_path);
  g_free (core_config->tool_preset_path_writable);
  g_free (core_config->font_path);
  g_free (core_config->font_path_writable);
  g_free (core_config->default_brush);
  g_free (core_config->default_dynamics);
  g_free (core_config->default_mypaint_brush);
  g_free (core_config->default_pattern);
  g_free (core_config->default_palette);
  g_free (core_config->default_gradient);
  g_free (core_config->default_tool_preset);
  g_free (core_config->default_font);
  g_free (core_config->plug_in_rc_path);
  g_free (core_config->import_raw_plug_in);

  g_clear_pointer (&core_config->last_known_release, g_free);
  g_clear_pointer (&core_config->last_release_comment, g_free);
  g_clear_pointer (&core_config->config_version, g_free);

  g_clear_object (&core_config->default_image);
  g_clear_object (&core_config->default_grid);
  g_clear_object (&core_config->color_management);
  g_clear_object (&core_config->quick_mask_color);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_core_config_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpCoreConfig *core_config = GIMP_CORE_CONFIG (object);

  switch (property_id)
    {
    case PROP_LANGUAGE:
      g_free (core_config->language);
      core_config->language = g_value_dup_string (value);
      break;
    case PROP_PREV_LANGUAGE:
      g_free (core_config->prev_language);
      core_config->prev_language = g_value_dup_string (value);
      break;
    case PROP_INTERPOLATION_TYPE:
      core_config->interpolation_type = g_value_get_enum (value);
      break;
    case PROP_DEFAULT_THRESHOLD:
      core_config->default_threshold = g_value_get_int (value);
      break;
    case PROP_PLUG_IN_PATH:
      g_free (core_config->plug_in_path);
      core_config->plug_in_path = g_value_dup_string (value);
      break;
    case PROP_MODULE_PATH:
      g_free (core_config->module_path);
      core_config->module_path = g_value_dup_string (value);
      break;
    case PROP_INTERPRETER_PATH:
      g_free (core_config->interpreter_path);
      core_config->interpreter_path = g_value_dup_string (value);
      break;
    case PROP_ENVIRON_PATH:
      g_free (core_config->environ_path);
      core_config->environ_path = g_value_dup_string (value);
      break;
    case PROP_BRUSH_PATH:
      g_free (core_config->brush_path);
      core_config->brush_path = g_value_dup_string (value);
      break;
    case PROP_BRUSH_PATH_WRITABLE:
      g_free (core_config->brush_path_writable);
      core_config->brush_path_writable = g_value_dup_string (value);
      break;
    case PROP_DYNAMICS_PATH:
      g_free (core_config->dynamics_path);
      core_config->dynamics_path = g_value_dup_string (value);
      break;
    case PROP_DYNAMICS_PATH_WRITABLE:
      g_free (core_config->dynamics_path_writable);
      core_config->dynamics_path_writable = g_value_dup_string (value);
      break;
    case PROP_MYPAINT_BRUSH_PATH:
      g_free (core_config->mypaint_brush_path);
      core_config->mypaint_brush_path = g_value_dup_string (value);
      break;
    case PROP_MYPAINT_BRUSH_PATH_WRITABLE:
      g_free (core_config->mypaint_brush_path_writable);
      core_config->mypaint_brush_path_writable = g_value_dup_string (value);
      break;
    case PROP_PATTERN_PATH:
      g_free (core_config->pattern_path);
      core_config->pattern_path = g_value_dup_string (value);
      break;
    case PROP_PATTERN_PATH_WRITABLE:
      g_free (core_config->pattern_path_writable);
      core_config->pattern_path_writable = g_value_dup_string (value);
      break;
    case PROP_PALETTE_PATH:
      g_free (core_config->palette_path);
      core_config->palette_path = g_value_dup_string (value);
      break;
    case PROP_PALETTE_PATH_WRITABLE:
      g_free (core_config->palette_path_writable);
      core_config->palette_path_writable = g_value_dup_string (value);
      break;
    case PROP_GRADIENT_PATH:
      g_free (core_config->gradient_path);
      core_config->gradient_path = g_value_dup_string (value);
      break;
    case PROP_GRADIENT_PATH_WRITABLE:
      g_free (core_config->gradient_path_writable);
      core_config->gradient_path_writable = g_value_dup_string (value);
      break;
    case PROP_TOOL_PRESET_PATH:
      g_free (core_config->tool_preset_path);
      core_config->tool_preset_path = g_value_dup_string (value);
      break;
    case PROP_TOOL_PRESET_PATH_WRITABLE:
      g_free (core_config->tool_preset_path_writable);
      core_config->tool_preset_path_writable = g_value_dup_string (value);
      break;
    case PROP_FONT_PATH:
      g_free (core_config->font_path);
      core_config->font_path = g_value_dup_string (value);
      break;
    case PROP_FONT_PATH_WRITABLE:
      g_free (core_config->font_path_writable);
      core_config->font_path_writable = g_value_dup_string (value);
      break;
    case PROP_DEFAULT_BRUSH:
      g_free (core_config->default_brush);
      core_config->default_brush = g_value_dup_string (value);
      break;
    case PROP_DEFAULT_DYNAMICS:
      g_free (core_config->default_dynamics);
      core_config->default_dynamics = g_value_dup_string (value);
      break;
    case PROP_DEFAULT_MYPAINT_BRUSH:
      g_free (core_config->default_mypaint_brush);
      core_config->default_mypaint_brush = g_value_dup_string (value);
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
    case PROP_DEFAULT_TOOL_PRESET:
      g_free (core_config->default_tool_preset);
      core_config->default_tool_preset = g_value_dup_string (value);
      break;
    case PROP_DEFAULT_FONT:
      g_free (core_config->default_font);
      core_config->default_font = g_value_dup_string (value);
      break;
    case PROP_GLOBAL_BRUSH:
      core_config->global_brush = g_value_get_boolean (value);
      break;
    case PROP_GLOBAL_DYNAMICS:
      core_config->global_dynamics = g_value_get_boolean (value);
      break;
    case PROP_GLOBAL_PATTERN:
      core_config->global_pattern = g_value_get_boolean (value);
      break;
    case PROP_GLOBAL_PALETTE:
      core_config->global_palette = g_value_get_boolean (value);
      break;
    case PROP_GLOBAL_GRADIENT:
      core_config->global_gradient = g_value_get_boolean (value);
      break;
    case PROP_GLOBAL_FONT:
      core_config->global_font = g_value_get_boolean (value);
      break;
    case PROP_GLOBAL_EXPAND:
      core_config->global_expand = g_value_get_boolean (value);
      break;
    case PROP_DEFAULT_IMAGE:
      if (g_value_get_object (value))
        gimp_config_sync (g_value_get_object (value) ,
                          G_OBJECT (core_config->default_image), 0);
      break;
    case PROP_DEFAULT_GRID:
      if (g_value_get_object (value))
        gimp_config_sync (g_value_get_object (value),
                          G_OBJECT (core_config->default_grid), 0);
      break;
    case PROP_FILTER_HISTORY_SIZE:
      core_config->filter_history_size = g_value_get_int (value);
      break;
    case PROP_UNDO_LEVELS:
      core_config->levels_of_undo = g_value_get_int (value);
      break;
    case PROP_UNDO_SIZE:
      core_config->undo_size = g_value_get_uint64 (value);
      break;
    case PROP_UNDO_PREVIEW_SIZE:
      core_config->undo_preview_size = g_value_get_enum (value);
      break;
    case PROP_PLUGINRC_PATH:
      g_free (core_config->plug_in_rc_path);
      core_config->plug_in_rc_path = g_value_dup_string (value);
      break;
    case PROP_LAYER_PREVIEWS:
      core_config->layer_previews = g_value_get_boolean (value);
      break;
    case PROP_GROUP_LAYER_PREVIEWS:
      core_config->group_layer_previews = g_value_get_boolean (value);
      break;
    case PROP_LAYER_PREVIEW_SIZE:
      core_config->layer_preview_size = g_value_get_enum (value);
      break;
    case PROP_THUMBNAIL_SIZE:
      core_config->thumbnail_size = g_value_get_enum (value);
      break;
    case PROP_THUMBNAIL_FILESIZE_LIMIT:
      core_config->thumbnail_filesize_limit = g_value_get_uint64 (value);
      break;
    case PROP_COLOR_MANAGEMENT:
      if (g_value_get_object (value))
        gimp_config_sync (g_value_get_object (value),
                          G_OBJECT (core_config->color_management), 0);
      break;
    case PROP_CHECK_UPDATES:
      core_config->check_updates = g_value_get_boolean (value);
      break;
    case PROP_CHECK_UPDATE_TIMESTAMP:
      core_config->check_update_timestamp = g_value_get_int64 (value);
      break;
    case PROP_LAST_RELEASE_TIMESTAMP:
      core_config->last_release_timestamp = g_value_get_int64 (value);
      break;
    case PROP_LAST_RELEASE_COMMENT:
      g_clear_pointer (&core_config->last_release_comment, g_free);
      core_config->last_release_comment = g_value_dup_string (value);
      break;
    case PROP_LAST_REVISION:
      core_config->last_revision = g_value_get_int (value);
      break;
    case PROP_LAST_KNOWN_RELEASE:
      g_clear_pointer (&core_config->last_known_release, g_free);
      core_config->last_known_release = g_value_dup_string (value);
      break;
    case PROP_CONFIG_VERSION:
      g_clear_pointer (&core_config->config_version, g_free);
      core_config->config_version = g_value_dup_string (value);
      break;
    case PROP_SAVE_DOCUMENT_HISTORY:
      core_config->save_document_history = g_value_get_boolean (value);
      break;
    case PROP_QUICK_MASK_COLOR:
      g_clear_object (&core_config->quick_mask_color);
      core_config->quick_mask_color = gegl_color_duplicate (g_value_get_object (value));
      break;
    case PROP_IMPORT_PROMOTE_FLOAT:
      core_config->import_promote_float = g_value_get_boolean (value);
      break;
    case PROP_IMPORT_PROMOTE_DITHER:
      core_config->import_promote_dither = g_value_get_boolean (value);
      break;
    case PROP_IMPORT_ADD_ALPHA:
      core_config->import_add_alpha = g_value_get_boolean (value);
      break;
    case PROP_IMPORT_RAW_PLUG_IN:
      g_free (core_config->import_raw_plug_in);
      core_config->import_raw_plug_in = g_value_dup_string (value);
      break;
    case PROP_EXPORT_FILE_TYPE:
      core_config->export_file_type = g_value_get_enum (value);
      break;
    case PROP_EXPORT_COLOR_PROFILE:
      core_config->export_color_profile = g_value_get_boolean (value);
      break;
    case PROP_EXPORT_COMMENT:
      core_config->export_comment = g_value_get_boolean (value);
      break;
    case PROP_EXPORT_THUMBNAIL:
      core_config->export_thumbnail = g_value_get_boolean (value);
      break;
    case PROP_EXPORT_METADATA_EXIF:
      core_config->export_metadata_exif = g_value_get_boolean (value);
      break;
    case PROP_EXPORT_METADATA_XMP:
      core_config->export_metadata_xmp = g_value_get_boolean (value);
      break;
    case PROP_EXPORT_METADATA_IPTC:
      core_config->export_metadata_iptc = g_value_get_boolean (value);
      break;
    case PROP_DEBUG_POLICY:
      core_config->debug_policy = g_value_get_enum (value);
      break;
#ifdef G_OS_WIN32
    case PROP_WIN32_POINTER_INPUT_API:
      {
        GimpWin32PointerInputAPI api = g_value_get_enum (value);
        gboolean have_wintab         = gimp_win32_have_wintab ();
        gboolean have_windows_ink    = gimp_win32_have_windows_ink ();
        gboolean api_is_wintab       = (api == GIMP_WIN32_POINTER_INPUT_API_WINTAB);
        gboolean api_is_windows_ink  = (api == GIMP_WIN32_POINTER_INPUT_API_WINDOWS_INK);

        if (api_is_wintab && !have_wintab && have_windows_ink)
          {
            core_config->win32_pointer_input_api = GIMP_WIN32_POINTER_INPUT_API_WINDOWS_INK;
          }
        else if (api_is_windows_ink && !have_windows_ink && have_wintab)
          {
            core_config->win32_pointer_input_api = GIMP_WIN32_POINTER_INPUT_API_WINTAB;
          }
        else
          {
            core_config->win32_pointer_input_api = api;
          }
      }
      break;
#endif
    case PROP_ITEMS_SELECT_METHOD:
      core_config->items_select_method = g_value_get_enum (value);
      break;

    case PROP_INSTALL_COLORMAP:
    case PROP_MIN_COLORS:
      /*  ignored  */
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
  GimpCoreConfig *core_config = GIMP_CORE_CONFIG (object);

  switch (property_id)
    {
    case PROP_LANGUAGE:
      g_value_set_string (value, core_config->language);
      break;
    case PROP_PREV_LANGUAGE:
      g_value_set_string (value, core_config->prev_language);
      break;
    case PROP_INTERPOLATION_TYPE:
      g_value_set_enum (value, core_config->interpolation_type);
      break;
    case PROP_DEFAULT_THRESHOLD:
      g_value_set_int (value, core_config->default_threshold);
      break;
    case PROP_PLUG_IN_PATH:
      g_value_set_string (value, core_config->plug_in_path);
      break;
    case PROP_MODULE_PATH:
      g_value_set_string (value, core_config->module_path);
      break;
    case PROP_INTERPRETER_PATH:
      g_value_set_string (value, core_config->interpreter_path);
      break;
    case PROP_ENVIRON_PATH:
      g_value_set_string (value, core_config->environ_path);
      break;
    case PROP_BRUSH_PATH:
      g_value_set_string (value, core_config->brush_path);
      break;
    case PROP_BRUSH_PATH_WRITABLE:
      g_value_set_string (value, core_config->brush_path_writable);
      break;
    case PROP_DYNAMICS_PATH:
      g_value_set_string (value, core_config->dynamics_path);
      break;
    case PROP_DYNAMICS_PATH_WRITABLE:
      g_value_set_string (value, core_config->dynamics_path_writable);
      break;
    case PROP_MYPAINT_BRUSH_PATH:
      g_value_set_string (value, core_config->mypaint_brush_path);
      break;
    case PROP_MYPAINT_BRUSH_PATH_WRITABLE:
      g_value_set_string (value, core_config->mypaint_brush_path_writable);
      break;
    case PROP_PATTERN_PATH:
      g_value_set_string (value, core_config->pattern_path);
      break;
    case PROP_PATTERN_PATH_WRITABLE:
      g_value_set_string (value, core_config->pattern_path_writable);
      break;
    case PROP_PALETTE_PATH:
      g_value_set_string (value, core_config->palette_path);
      break;
    case PROP_PALETTE_PATH_WRITABLE:
      g_value_set_string (value, core_config->palette_path_writable);
      break;
    case PROP_GRADIENT_PATH:
      g_value_set_string (value, core_config->gradient_path);
      break;
    case PROP_GRADIENT_PATH_WRITABLE:
      g_value_set_string (value, core_config->gradient_path_writable);
      break;
    case PROP_TOOL_PRESET_PATH:
      g_value_set_string (value, core_config->tool_preset_path);
      break;
    case PROP_TOOL_PRESET_PATH_WRITABLE:
      g_value_set_string (value, core_config->tool_preset_path_writable);
      break;
    case PROP_FONT_PATH:
      g_value_set_string (value, core_config->font_path);
      break;
    case PROP_FONT_PATH_WRITABLE:
      g_value_set_string (value, core_config->font_path_writable);
      break;
    case PROP_DEFAULT_BRUSH:
      g_value_set_string (value, core_config->default_brush);
      break;
    case PROP_DEFAULT_DYNAMICS:
      g_value_set_string (value, core_config->default_dynamics);
      break;
    case PROP_DEFAULT_MYPAINT_BRUSH:
      g_value_set_string (value, core_config->default_mypaint_brush);
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
    case PROP_DEFAULT_TOOL_PRESET:
      g_value_set_string (value, core_config->default_tool_preset);
      break;
    case PROP_DEFAULT_FONT:
      g_value_set_string (value, core_config->default_font);
      break;
    case PROP_GLOBAL_BRUSH:
      g_value_set_boolean (value, core_config->global_brush);
      break;
    case PROP_GLOBAL_DYNAMICS:
      g_value_set_boolean (value, core_config->global_dynamics);
      break;
    case PROP_GLOBAL_PATTERN:
      g_value_set_boolean (value, core_config->global_pattern);
      break;
    case PROP_GLOBAL_PALETTE:
      g_value_set_boolean (value, core_config->global_palette);
      break;
    case PROP_GLOBAL_GRADIENT:
      g_value_set_boolean (value, core_config->global_gradient);
      break;
    case PROP_GLOBAL_FONT:
      g_value_set_boolean (value, core_config->global_font);
      break;
    case PROP_GLOBAL_EXPAND:
      g_value_set_boolean (value, core_config->global_expand);
      break;
    case PROP_DEFAULT_IMAGE:
      g_value_set_object (value, core_config->default_image);
      break;
    case PROP_DEFAULT_GRID:
      g_value_set_object (value, core_config->default_grid);
      break;
    case PROP_FILTER_HISTORY_SIZE:
      g_value_set_int (value, core_config->filter_history_size);
      break;
    case PROP_UNDO_LEVELS:
      g_value_set_int (value, core_config->levels_of_undo);
      break;
    case PROP_UNDO_SIZE:
      g_value_set_uint64 (value, core_config->undo_size);
      break;
    case PROP_UNDO_PREVIEW_SIZE:
      g_value_set_enum (value, core_config->undo_preview_size);
      break;
    case PROP_PLUGINRC_PATH:
      g_value_set_string (value, core_config->plug_in_rc_path);
      break;
    case PROP_LAYER_PREVIEWS:
      g_value_set_boolean (value, core_config->layer_previews);
      break;
    case PROP_GROUP_LAYER_PREVIEWS:
      g_value_set_boolean (value, core_config->group_layer_previews);
      break;
    case PROP_LAYER_PREVIEW_SIZE:
      g_value_set_enum (value, core_config->layer_preview_size);
      break;
    case PROP_THUMBNAIL_SIZE:
      g_value_set_enum (value, core_config->thumbnail_size);
      break;
    case PROP_THUMBNAIL_FILESIZE_LIMIT:
      g_value_set_uint64 (value, core_config->thumbnail_filesize_limit);
      break;
    case PROP_COLOR_MANAGEMENT:
      g_value_set_object (value, core_config->color_management);
      break;
    case PROP_CHECK_UPDATES:
      g_value_set_boolean (value, core_config->check_updates);
      break;
    case PROP_CHECK_UPDATE_TIMESTAMP:
      g_value_set_int64 (value, core_config->check_update_timestamp);
      break;
    case PROP_LAST_RELEASE_TIMESTAMP:
      g_value_set_int64 (value, core_config->last_release_timestamp);
      break;
    case PROP_LAST_RELEASE_COMMENT:
      g_value_set_string (value, core_config->last_release_comment);
      break;
    case PROP_LAST_REVISION:
      g_value_set_int (value, core_config->last_revision);
      break;
    case PROP_LAST_KNOWN_RELEASE:
      g_value_set_string (value, core_config->last_known_release);
      break;
    case PROP_CONFIG_VERSION:
      g_value_set_string (value, core_config->config_version);
      break;
    case PROP_SAVE_DOCUMENT_HISTORY:
      g_value_set_boolean (value, core_config->save_document_history);
      break;
    case PROP_QUICK_MASK_COLOR:
      g_value_set_object (value, core_config->quick_mask_color);
      break;
    case PROP_IMPORT_PROMOTE_FLOAT:
      g_value_set_boolean (value, core_config->import_promote_float);
      break;
    case PROP_IMPORT_PROMOTE_DITHER:
      g_value_set_boolean (value, core_config->import_promote_dither);
      break;
    case PROP_IMPORT_ADD_ALPHA:
      g_value_set_boolean (value, core_config->import_add_alpha);
      break;
    case PROP_IMPORT_RAW_PLUG_IN:
      g_value_set_string (value, core_config->import_raw_plug_in);
      break;
    case PROP_EXPORT_FILE_TYPE:
      g_value_set_enum (value, core_config->export_file_type);
      break;
    case PROP_EXPORT_COLOR_PROFILE:
      g_value_set_boolean (value, core_config->export_color_profile);
      break;
    case PROP_EXPORT_COMMENT:
      g_value_set_boolean (value, core_config->export_comment);
      break;
    case PROP_EXPORT_THUMBNAIL:
      g_value_set_boolean (value, core_config->export_thumbnail);
      break;
    case PROP_EXPORT_METADATA_EXIF:
      g_value_set_boolean (value, core_config->export_metadata_exif);
      break;
    case PROP_EXPORT_METADATA_XMP:
      g_value_set_boolean (value, core_config->export_metadata_xmp);
      break;
    case PROP_EXPORT_METADATA_IPTC:
      g_value_set_boolean (value, core_config->export_metadata_iptc);
      break;
    case PROP_DEBUG_POLICY:
      g_value_set_enum (value, core_config->debug_policy);
      break;
#ifdef G_OS_WIN32
    case PROP_WIN32_POINTER_INPUT_API:
      g_value_set_enum (value, core_config->win32_pointer_input_api);
      break;
#endif
    case PROP_ITEMS_SELECT_METHOD:
      g_value_set_enum (value, core_config->items_select_method);
      break;

    case PROP_INSTALL_COLORMAP:
    case PROP_MIN_COLORS:
      /*  ignored  */
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_core_config_default_image_notify (GObject    *object,
                                       GParamSpec *pspec,
                                       gpointer    data)
{
  g_object_notify (G_OBJECT (data), "default-image");
}

static void
gimp_core_config_default_grid_notify (GObject    *object,
                                      GParamSpec *pspec,
                                      gpointer    data)
{
  g_object_notify (G_OBJECT (data), "default-grid");
}

static void
gimp_core_config_color_management_notify (GObject    *object,
                                          GParamSpec *pspec,
                                          gpointer    data)
{
  g_object_notify (G_OBJECT (data), "color-management");
}
