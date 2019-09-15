/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpDisplayOptions
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "config-types.h"

#include "gimprc-blurbs.h"

#include "gimpdisplayoptions.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_SHOW_MENUBAR,
  PROP_SHOW_STATUSBAR,
  PROP_SHOW_RULERS,
  PROP_SHOW_SCROLLBARS,
  PROP_SHOW_SELECTION,
  PROP_SHOW_LAYER_BOUNDARY,
  PROP_SHOW_CANVAS_BOUNDARY,
  PROP_SHOW_GUIDES,
  PROP_SHOW_GRID,
  PROP_SHOW_SAMPLE_POINTS,
  PROP_SNAP_TO_GUIDES,
  PROP_SNAP_TO_GRID,
  PROP_SNAP_TO_CANVAS,
  PROP_SNAP_TO_PATH,
  PROP_PADDING_MODE,
  PROP_PADDING_COLOR,
  PROP_PADDING_IN_SHOW_ALL
};


static void   gimp_display_options_set_property (GObject      *object,
                                                 guint         property_id,
                                                 const GValue *value,
                                                 GParamSpec   *pspec);
static void   gimp_display_options_get_property (GObject      *object,
                                                 guint         property_id,
                                                 GValue       *value,
                                                 GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_CODE (GimpDisplayOptions,
                         gimp_display_options,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG, NULL))

typedef struct _GimpDisplayOptions      GimpDisplayOptionsFullscreen;
typedef struct _GimpDisplayOptionsClass GimpDisplayOptionsFullscreenClass;

#define gimp_display_options_fullscreen_init gimp_display_options_init

G_DEFINE_TYPE_WITH_CODE (GimpDisplayOptionsFullscreen,
                         gimp_display_options_fullscreen,
                         GIMP_TYPE_DISPLAY_OPTIONS,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG, NULL))

typedef struct _GimpDisplayOptions      GimpDisplayOptionsNoImage;
typedef struct _GimpDisplayOptionsClass GimpDisplayOptionsNoImageClass;

#define gimp_display_options_no_image_init gimp_display_options_init

G_DEFINE_TYPE_WITH_CODE (GimpDisplayOptionsNoImage,
                         gimp_display_options_no_image,
                         GIMP_TYPE_DISPLAY_OPTIONS,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG, NULL))


static void
gimp_display_options_class_init (GimpDisplayOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GimpRGB       white;

  gimp_rgba_set (&white, 1.0, 1.0, 1.0, GIMP_OPACITY_OPAQUE);

  object_class->set_property = gimp_display_options_set_property;
  object_class->get_property = gimp_display_options_get_property;

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_MENUBAR,
                            "show-menubar",
                            "Show menubar",
                            SHOW_MENUBAR_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_STATUSBAR,
                            "show-statusbar",
                            "Show statusbar",
                            SHOW_STATUSBAR_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_RULERS,
                            "show-rulers",
                            "Show rulers",
                            SHOW_RULERS_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_SCROLLBARS,
                            "show-scrollbars",
                            "Show scrollbars",
                            SHOW_SCROLLBARS_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_SELECTION,
                            "show-selection",
                            "Show selection",
                            SHOW_SELECTION_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_LAYER_BOUNDARY,
                            "show-layer-boundary",
                            "Show layer boundary",
                            SHOW_LAYER_BOUNDARY_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_CANVAS_BOUNDARY,
                            "show-canvas-boundary",
                            "Show canvas boundary",
                            SHOW_CANVAS_BOUNDARY_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_GUIDES,
                            "show-guides",
                            "Show guides",
                            SHOW_GUIDES_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_GRID,
                            "show-grid",
                            "Show grid",
                            SHOW_GRID_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_SAMPLE_POINTS,
                            "show-sample-points",
                            "Show sample points",
                            SHOW_SAMPLE_POINTS_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SNAP_TO_GUIDES,
                            "snap-to-guides",
                            "Snap to guides",
                            SNAP_TO_GUIDES_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SNAP_TO_GRID,
                            "snap-to-grid",
                            "Snap to grid",
                            SNAP_TO_GRID_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SNAP_TO_CANVAS,
                            "snap-to-canvas",
                            "Snap to canvas",
                            SNAP_TO_CANVAS_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SNAP_TO_PATH,
                            "snap-to-path",
                            "Snap to path",
                            SNAP_TO_PATH_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_PADDING_MODE,
                         "padding-mode",
                         "Padding mode",
                         CANVAS_PADDING_MODE_BLURB,
                         GIMP_TYPE_CANVAS_PADDING_MODE,
                         GIMP_CANVAS_PADDING_MODE_DEFAULT,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_RGB (object_class, PROP_PADDING_COLOR,
                        "padding-color",
                        "Padding color",
                        CANVAS_PADDING_COLOR_BLURB,
                        FALSE, &white,
                        GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_PADDING_IN_SHOW_ALL,
                            "padding-in-show-all",
                            "Keep padding in \"Show All\" mode",
                            CANVAS_PADDING_IN_SHOW_ALL_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_display_options_fullscreen_class_init (GimpDisplayOptionsFullscreenClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GimpRGB       black;

  gimp_rgba_set (&black, 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);

  object_class->set_property = gimp_display_options_set_property;
  object_class->get_property = gimp_display_options_get_property;

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_MENUBAR,
                            "show-menubar",
                            "Show menubar",
                            SHOW_MENUBAR_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_STATUSBAR,
                            "show-statusbar",
                            "Show statusbar",
                            SHOW_STATUSBAR_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_RULERS,
                            "show-rulers",
                            "Show rulers",
                            SHOW_RULERS_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_SCROLLBARS,
                            "show-scrollbars",
                            "Show scrollbars",
                            SHOW_SCROLLBARS_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_SELECTION,
                            "show-selection",
                            "Show selection",
                            SHOW_SELECTION_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_LAYER_BOUNDARY,
                            "show-layer-boundary",
                            "Show layer boundary",
                            SHOW_LAYER_BOUNDARY_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_CANVAS_BOUNDARY,
                            "show-canvas-boundary",
                            "Show canvas boundary",
                            SHOW_CANVAS_BOUNDARY_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_GUIDES,
                            "show-guides",
                            "Show guides",
                            SHOW_GUIDES_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_GRID,
                            "show-grid",
                            "Show grid",
                            SHOW_GRID_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_SAMPLE_POINTS,
                            "show-sample-points",
                            "Show sample points",
                            SHOW_SAMPLE_POINTS_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SNAP_TO_GUIDES,
                            "snap-to-guides",
                            "Snap to guides",
                            SNAP_TO_GUIDES_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SNAP_TO_GRID,
                            "snap-to-grid",
                            "Snap to grid",
                            SHOW_SCROLLBARS_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SNAP_TO_CANVAS,
                            "snap-to-canvas",
                            "Snap to canvas",
                            SNAP_TO_CANVAS_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SNAP_TO_PATH,
                            "snap-to-path",
                            "Snap to path",
                            SNAP_TO_PATH_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_PADDING_MODE,
                         "padding-mode",
                         "Padding mode",
                         CANVAS_PADDING_MODE_BLURB,
                         GIMP_TYPE_CANVAS_PADDING_MODE,
                         GIMP_CANVAS_PADDING_MODE_CUSTOM,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_RGB (object_class, PROP_PADDING_COLOR,
                        "padding-color",
                        "Padding color",
                        CANVAS_PADDING_COLOR_BLURB,
                        FALSE, &black,
                        GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_PADDING_IN_SHOW_ALL,
                            "padding-in-show-all",
                            "Keep padding in \"Show All\" mode",
                            CANVAS_PADDING_IN_SHOW_ALL_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_display_options_no_image_class_init (GimpDisplayOptionsNoImageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_display_options_set_property;
  object_class->get_property = gimp_display_options_get_property;

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_RULERS,
                            "show-rulers",
                            "Show rulers",
                            SHOW_RULERS_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_SCROLLBARS,
                            "show-scrollbars",
                            "Show scrollbars",
                            SHOW_SCROLLBARS_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_SELECTION,
                            "show-selection",
                            "Show selection",
                            SHOW_SELECTION_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_LAYER_BOUNDARY,
                            "show-layer-boundary",
                            "Show layer boundary",
                            SHOW_LAYER_BOUNDARY_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_CANVAS_BOUNDARY,
                            "show-canvas-boundary",
                            "Show canvas boundary",
                            SHOW_CANVAS_BOUNDARY_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_GUIDES,
                            "show-guides",
                            "Show guides",
                            SHOW_GUIDES_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_GRID,
                            "show-grid",
                            "Show grid",
                            SHOW_GRID_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SHOW_SAMPLE_POINTS,
                            "show-sample-points",
                            "Show sample points",
                            SHOW_SAMPLE_POINTS_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SNAP_TO_GUIDES,
                            "snap-to-guides",
                            "Snap to guides",
                            SNAP_TO_GUIDES_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SNAP_TO_GRID,
                            "snap-to-grid",
                            "Snap to grid",
                            SHOW_SCROLLBARS_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SNAP_TO_CANVAS,
                            "snap-to-canvas",
                            "Snap to canvas",
                            SNAP_TO_CANVAS_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SNAP_TO_PATH,
                            "snap-to-path",
                            "Snap tp path",
                            SNAP_TO_PATH_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_display_options_init (GimpDisplayOptions *options)
{
  options->padding_mode_set = FALSE;
}

static void
gimp_display_options_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpDisplayOptions *options = GIMP_DISPLAY_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SHOW_MENUBAR:
      options->show_menubar = g_value_get_boolean (value);
      break;
    case PROP_SHOW_STATUSBAR:
      options->show_statusbar = g_value_get_boolean (value);
      break;
    case PROP_SHOW_RULERS:
      options->show_rulers = g_value_get_boolean (value);
      break;
    case PROP_SHOW_SCROLLBARS:
      options->show_scrollbars = g_value_get_boolean (value);
      break;
    case PROP_SHOW_SELECTION:
      options->show_selection = g_value_get_boolean (value);
      break;
    case PROP_SHOW_LAYER_BOUNDARY:
      options->show_layer_boundary = g_value_get_boolean (value);
      break;
    case PROP_SHOW_CANVAS_BOUNDARY:
      options->show_canvas_boundary = g_value_get_boolean (value);
      break;
    case PROP_SHOW_GUIDES:
      options->show_guides = g_value_get_boolean (value);
      break;
    case PROP_SHOW_GRID:
      options->show_grid = g_value_get_boolean (value);
      break;
    case PROP_SHOW_SAMPLE_POINTS:
      options->show_sample_points = g_value_get_boolean (value);
      break;
    case PROP_SNAP_TO_GUIDES:
      options->snap_to_guides = g_value_get_boolean (value);
      break;
    case PROP_SNAP_TO_GRID:
      options->snap_to_grid = g_value_get_boolean (value);
      break;
    case PROP_SNAP_TO_CANVAS:
      options->snap_to_canvas = g_value_get_boolean (value);
      break;
    case PROP_SNAP_TO_PATH:
      options->snap_to_path = g_value_get_boolean (value);
      break;
    case PROP_PADDING_MODE:
      options->padding_mode = g_value_get_enum (value);
      break;
    case PROP_PADDING_COLOR:
      options->padding_color = *(GimpRGB *) g_value_get_boxed (value);
      break;
    case PROP_PADDING_IN_SHOW_ALL:
      options->padding_in_show_all = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_display_options_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpDisplayOptions *options = GIMP_DISPLAY_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SHOW_MENUBAR:
      g_value_set_boolean (value, options->show_menubar);
      break;
    case PROP_SHOW_STATUSBAR:
      g_value_set_boolean (value, options->show_statusbar);
      break;
    case PROP_SHOW_RULERS:
      g_value_set_boolean (value, options->show_rulers);
      break;
    case PROP_SHOW_SCROLLBARS:
      g_value_set_boolean (value, options->show_scrollbars);
      break;
    case PROP_SHOW_SELECTION:
      g_value_set_boolean (value, options->show_selection);
      break;
    case PROP_SHOW_LAYER_BOUNDARY:
      g_value_set_boolean (value, options->show_layer_boundary);
      break;
    case PROP_SHOW_CANVAS_BOUNDARY:
      g_value_set_boolean (value, options->show_canvas_boundary);
      break;
    case PROP_SHOW_GUIDES:
      g_value_set_boolean (value, options->show_guides);
      break;
    case PROP_SHOW_GRID:
      g_value_set_boolean (value, options->show_grid);
      break;
    case PROP_SHOW_SAMPLE_POINTS:
      g_value_set_boolean (value, options->show_sample_points);
      break;
    case PROP_SNAP_TO_GUIDES:
      g_value_set_boolean (value, options->snap_to_guides);
      break;
    case PROP_SNAP_TO_GRID:
      g_value_set_boolean (value, options->snap_to_grid);
      break;
    case PROP_SNAP_TO_CANVAS:
      g_value_set_boolean (value, options->snap_to_canvas);
      break;
    case PROP_SNAP_TO_PATH:
      g_value_set_boolean (value, options->snap_to_path);
      break;
    case PROP_PADDING_MODE:
      g_value_set_enum (value, options->padding_mode);
      break;
    case PROP_PADDING_COLOR:
      g_value_set_boxed (value, &options->padding_color);
      break;
    case PROP_PADDING_IN_SHOW_ALL:
      g_value_set_boolean (value, options->padding_in_show_all);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
