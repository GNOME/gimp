/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpDialogConfig class
 * Copyright (C) 2016  Michael Natterer <mitch@gimp.org>
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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "core/core-types.h" /* fill and stroke options */
#include "core/gimp.h"
#include "core/gimpstrokeoptions.h"

#include "config-types.h"

#include "gimprc-blurbs.h"
#include "gimpdialogconfig.h"

#include "gimp-intl.h"


enum
{
  PROP_0,

  PROP_GIMP,

  PROP_COLOR_PROFILE_POLICY,
  PROP_METADATA_ROTATION_POLICY,

  PROP_COLOR_PROFILE_PATH,

  PROP_IMAGE_CONVERT_PROFILE_INTENT,
  PROP_IMAGE_CONVERT_PROFILE_BPC,

  PROP_IMAGE_CONVERT_PRECISION_LAYER_DITHER_METHOD,
  PROP_IMAGE_CONVERT_PRECISION_TEXT_LAYER_DITHER_METHOD,
  PROP_IMAGE_CONVERT_PRECISION_CHANNEL_DITHER_METHOD,

  PROP_IMAGE_CONVERT_INDEXED_PALETTE_TYPE,
  PROP_IMAGE_CONVERT_INDEXED_MAX_COLORS,
  PROP_IMAGE_CONVERT_INDEXED_REMOVE_DUPLICATES,
  PROP_IMAGE_CONVERT_INDEXED_DITHER_TYPE,
  PROP_IMAGE_CONVERT_INDEXED_DITHER_ALPHA,
  PROP_IMAGE_CONVERT_INDEXED_DITHER_TEXT_LAYERS,

  PROP_IMAGE_RESIZE_FILL_TYPE,
  PROP_IMAGE_RESIZE_LAYER_SET,
  PROP_IMAGE_RESIZE_RESIZE_TEXT_LAYERS,

  PROP_LAYER_NEW_NAME,
  PROP_LAYER_NEW_MODE,
  PROP_LAYER_NEW_BLEND_SPACE,
  PROP_LAYER_NEW_COMPOSITE_SPACE,
  PROP_LAYER_NEW_COMPOSITE_MODE,
  PROP_LAYER_NEW_OPACITY,
  PROP_LAYER_NEW_FILL_TYPE,

  PROP_LAYER_RESIZE_FILL_TYPE,

  PROP_LAYER_ADD_MASK_TYPE,
  PROP_LAYER_ADD_MASK_INVERT,
  PROP_LAYER_ADD_MASK_EDIT_MASK,

  PROP_LAYER_MERGE_TYPE,
  PROP_LAYER_MERGE_ACTIVE_GROUP_ONLY,
  PROP_LAYER_MERGE_DISCARD_INVISIBLE,

  PROP_CHANNEL_NEW_NAME,
  PROP_CHANNEL_NEW_COLOR,

  PROP_PATH_NEW_NAME,

  PROP_PATH_EXPORT_PATH,
  PROP_PATH_EXPORT_ACTIVE_ONLY,

  PROP_PATH_IMPORT_PATH,
  PROP_PATH_IMPORT_MERGE,
  PROP_PATH_IMPORT_SCALE,

  PROP_SELECTION_FEATHER_RADIUS,
  PROP_SELECTION_FEATHER_EDGE_LOCK,

  PROP_SELECTION_GROW_RADIUS,

  PROP_SELECTION_SHRINK_RADIUS,
  PROP_SELECTION_SHRINK_EDGE_LOCK,

  PROP_SELECTION_BORDER_RADIUS,
  PROP_SELECTION_BORDER_STYLE,
  PROP_SELECTION_BORDER_EDGE_LOCK,

  PROP_FILL_OPTIONS,
  PROP_STROKE_OPTIONS
};


typedef struct _GimpDialogConfigPrivate GimpDialogConfigPrivate;

struct _GimpDialogConfigPrivate
{
  Gimp *gimp;
};

#define GET_PRIVATE(config) \
        ((GimpDialogConfigPrivate *) gimp_dialog_config_get_instance_private ((GimpDialogConfig *) (config)))


static void  gimp_dialog_config_constructed           (GObject      *object);
static void  gimp_dialog_config_finalize              (GObject      *object);
static void  gimp_dialog_config_set_property          (GObject      *object,
                                                       guint         property_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec);
static void  gimp_dialog_config_get_property          (GObject      *object,
                                                       guint         property_id,
                                                       GValue       *value,
                                                       GParamSpec   *pspec);

static void  gimp_dialog_config_fill_options_notify   (GObject      *object,
                                                       GParamSpec   *pspec,
                                                       gpointer      data);
static void  gimp_dialog_config_stroke_options_notify (GObject      *object,
                                                       GParamSpec   *pspec,
                                                       gpointer      data);


G_DEFINE_TYPE_WITH_PRIVATE (GimpDialogConfig, gimp_dialog_config,
                            GIMP_TYPE_GUI_CONFIG)

#define parent_class gimp_dialog_config_parent_class


static void
gimp_dialog_config_class_init (GimpDialogConfigClass *klass)
{
  GObjectClass *object_class     = G_OBJECT_CLASS (klass);
  GeglColor    *half_transparent = gegl_color_new ("black");

  gimp_color_set_alpha (half_transparent, 0.5);

  object_class->constructed  = gimp_dialog_config_constructed;
  object_class->finalize     = gimp_dialog_config_finalize;
  object_class->set_property = gimp_dialog_config_set_property;
  object_class->get_property = gimp_dialog_config_get_property;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_COLOR_PROFILE_POLICY,
                         "color-profile-policy",
                         "Color profile policy",
                         COLOR_PROFILE_POLICY_BLURB,
                         GIMP_TYPE_COLOR_PROFILE_POLICY,
                         GIMP_COLOR_PROFILE_POLICY_ASK,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_METADATA_ROTATION_POLICY,
                         "metadata-rotation-policy",
                         "Metadata rotation policy",
                         METADATA_ROTATION_POLICY_BLURB,
                         GIMP_TYPE_METADATA_ROTATION_POLICY,
                         GIMP_METADATA_ROTATION_POLICY_ASK,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_PATH (object_class, PROP_COLOR_PROFILE_PATH,
                         "color-profile-path",
                         "Default color profile folder path",
                         COLOR_PROFILE_PATH_BLURB,
                         GIMP_CONFIG_PATH_FILE,
                         NULL,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_IMAGE_CONVERT_PROFILE_INTENT,
                         "image-convert-profile-intent",
                         "Default rendering intent for color profile conversion",
                         IMAGE_CONVERT_PROFILE_INTENT_BLURB,
                         GIMP_TYPE_COLOR_RENDERING_INTENT,
                         GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_IMAGE_CONVERT_PROFILE_BPC,
                            "image-convert-profile-black-point-compensation",
                            "Default 'Black point compensation' for "
                            "color profile conversion",
                            IMAGE_CONVERT_PROFILE_BPC_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class,
                         PROP_IMAGE_CONVERT_PRECISION_LAYER_DITHER_METHOD,
                         "image-convert-precision-layer-dither-method",
                         "Default layer dither type for precision conversion",
                         IMAGE_CONVERT_PRECISION_LAYER_DITHER_METHOD_BLURB,
                         GEGL_TYPE_DITHER_METHOD,
                         GEGL_DITHER_NONE,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class,
                         PROP_IMAGE_CONVERT_PRECISION_TEXT_LAYER_DITHER_METHOD,
                         "image-convert-precision-text-layer-dither-method",
                         "Default text layer dither type for precision conversion",
                         IMAGE_CONVERT_PRECISION_TEXT_LAYER_DITHER_METHOD_BLURB,
                         GEGL_TYPE_DITHER_METHOD,
                         GEGL_DITHER_NONE,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class,
                         PROP_IMAGE_CONVERT_PRECISION_CHANNEL_DITHER_METHOD,
                         "image-convert-precision-channel-dither-method",
                         "Default channel dither type for precision conversion",
                         IMAGE_CONVERT_PRECISION_CHANNEL_DITHER_METHOD_BLURB,
                         GEGL_TYPE_DITHER_METHOD,
                         GEGL_DITHER_NONE,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class,
                         PROP_IMAGE_CONVERT_INDEXED_PALETTE_TYPE,
                         "image-convert-indexed-palette-type",
                         "Default palette type for indexed conversion",
                         IMAGE_CONVERT_INDEXED_PALETTE_TYPE_BLURB,
                         GIMP_TYPE_CONVERT_PALETTE_TYPE,
                         GIMP_CONVERT_PALETTE_GENERATE,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_INT (object_class,
                        PROP_IMAGE_CONVERT_INDEXED_MAX_COLORS,
                        "image-convert-indexed-max-colors",
                        "Default maximum number of colors for indexed conversion",
                        IMAGE_CONVERT_INDEXED_MAX_COLORS_BLURB,
                        2, 256, 256,
                        GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class,
                            PROP_IMAGE_CONVERT_INDEXED_REMOVE_DUPLICATES,
                            "image-convert-indexed-remove-duplicates",
                            "Default remove duplicates for indexed conversion",
                            IMAGE_CONVERT_INDEXED_REMOVE_DUPLICATES_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class,
                         PROP_IMAGE_CONVERT_INDEXED_DITHER_TYPE,
                         "image-convert-indexed-dither-type",
                         "Default dither type for indexed conversion",
                         IMAGE_CONVERT_INDEXED_DITHER_TYPE_BLURB,
                         GIMP_TYPE_CONVERT_DITHER_TYPE,
                         GIMP_CONVERT_DITHER_NONE,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class,
                            PROP_IMAGE_CONVERT_INDEXED_DITHER_ALPHA,
                            "image-convert-indexed-dither-alpha",
                            "Default dither alpha for indexed conversion",
                            IMAGE_CONVERT_INDEXED_DITHER_ALPHA_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class,
                            PROP_IMAGE_CONVERT_INDEXED_DITHER_TEXT_LAYERS,
                            "image-convert-indexed-dither-text-layers",
                            "Default dither text layers for indexed conversion",
                            IMAGE_CONVERT_INDEXED_DITHER_TEXT_LAYERS_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_IMAGE_RESIZE_FILL_TYPE,
                         "image-resize-fill-type",
                         "Default image resize fill type",
                         IMAGE_RESIZE_FILL_TYPE_BLURB,
                         GIMP_TYPE_FILL_TYPE,
                         GIMP_FILL_TRANSPARENT,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_IMAGE_RESIZE_LAYER_SET,
                         "image-resize-layer-set",
                         "Default image resize layer set",
                         IMAGE_RESIZE_LAYER_SET_BLURB,
                         GIMP_TYPE_ITEM_SET,
                         GIMP_ITEM_SET_NONE,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_IMAGE_RESIZE_RESIZE_TEXT_LAYERS,
                            "image-resize-resize-text-layers",
                            "Default image resize text layers",
                            IMAGE_RESIZE_RESIZE_TEXT_LAYERS_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_LAYER_NEW_NAME,
                           "layer-new-name",
                           "Default new layer name",
                           LAYER_NEW_NAME_BLURB,
                           _("Layer"),
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_LAYER_NEW_MODE,
                         "layer-new-mode",
                         "Default new layer mode",
                         LAYER_NEW_MODE_BLURB,
                         GIMP_TYPE_LAYER_MODE,
                         GIMP_LAYER_MODE_NORMAL,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_LAYER_NEW_BLEND_SPACE,
                         "layer-new-blend-space",
                         "Default new layer blend space",
                         LAYER_NEW_BLEND_SPACE_BLURB,
                         GIMP_TYPE_LAYER_COLOR_SPACE,
                         GIMP_LAYER_COLOR_SPACE_AUTO,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_LAYER_NEW_COMPOSITE_SPACE,
                         "layer-new-composite-space",
                         "Default new layer composite space",
                         LAYER_NEW_COMPOSITE_SPACE_BLURB,
                         GIMP_TYPE_LAYER_COLOR_SPACE,
                         GIMP_LAYER_COLOR_SPACE_AUTO,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_LAYER_NEW_COMPOSITE_MODE,
                         "layer-new-composite-mode",
                         "Default new layer composite mode",
                         LAYER_NEW_COMPOSITE_MODE_BLURB,
                         GIMP_TYPE_LAYER_COMPOSITE_MODE,
                         GIMP_LAYER_COMPOSITE_AUTO,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_LAYER_NEW_OPACITY,
                           "layer-new-opacity",
                           "Default new layer opacity",
                           LAYER_NEW_OPACITY_BLURB,
                           GIMP_OPACITY_TRANSPARENT, GIMP_OPACITY_OPAQUE,
                           GIMP_OPACITY_OPAQUE,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_LAYER_NEW_FILL_TYPE,
                         "layer-new-fill-type",
                         "Default new layer fill type",
                         LAYER_NEW_FILL_TYPE_BLURB,
                         GIMP_TYPE_FILL_TYPE,
                         GIMP_FILL_TRANSPARENT,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_LAYER_RESIZE_FILL_TYPE,
                         "layer-resize-fill-type",
                         "Default layer resize fill type",
                         LAYER_RESIZE_FILL_TYPE_BLURB,
                         GIMP_TYPE_FILL_TYPE,
                         GIMP_FILL_TRANSPARENT,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_LAYER_ADD_MASK_TYPE,
                         "layer-add-mask-type",
                         "Default layer mask type",
                         LAYER_ADD_MASK_TYPE_BLURB,
                         GIMP_TYPE_ADD_MASK_TYPE,
                         GIMP_ADD_MASK_WHITE,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_LAYER_ADD_MASK_INVERT,
                            "layer-add-mask-invert",
                            "Default layer mask invert",
                            LAYER_ADD_MASK_INVERT_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_LAYER_ADD_MASK_EDIT_MASK,
                            "layer-add-mask-edit-mask",
                            "Default layer mask: edit mask immediately",
                            LAYER_ADD_MASK_EDIT_MASK,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_LAYER_MERGE_TYPE,
                         "layer-merge-type",
                         "Default layer merge type",
                         LAYER_MERGE_TYPE_BLURB,
                         GIMP_TYPE_MERGE_TYPE,
                         GIMP_EXPAND_AS_NECESSARY,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_LAYER_MERGE_ACTIVE_GROUP_ONLY,
                            "layer-merge-active-group-only",
                            "Default layer merge active groups only",
                            LAYER_MERGE_ACTIVE_GROUP_ONLY_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_LAYER_MERGE_DISCARD_INVISIBLE,
                            "layer-merge-discard-invisible",
                            "Default layer merge discard invisible",
                            LAYER_MERGE_DISCARD_INVISIBLE_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_CHANNEL_NEW_NAME,
                           "channel-new-name",
                           "Default new channel name",
                           CHANNEL_NEW_NAME_BLURB,
                           _("Channel"),
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_COLOR (object_class, PROP_CHANNEL_NEW_COLOR,
                          "channel-new-color",
                          "Default new channel color and opacity",
                          CHANNEL_NEW_COLOR_BLURB,
                          TRUE, half_transparent,
                          GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_PATH_NEW_NAME,
                           "path-new-name",
                           "Default new path name",
                           PATH_NEW_NAME_BLURB,
                           _("Path"),
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_PATH (object_class, PROP_PATH_EXPORT_PATH,
                         "path-export-path",
                         "Default path export folder path",
                         PATH_EXPORT_PATH_BLURB,
                         GIMP_CONFIG_PATH_FILE,
                         NULL,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_PATH_EXPORT_ACTIVE_ONLY,
                            "path-export-active-only",
                            "Default export only the selected paths",
                            PATH_EXPORT_ACTIVE_ONLY_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_PATH (object_class, PROP_PATH_IMPORT_PATH,
                         "path-import-path",
                         "Default path import folder path",
                         PATH_IMPORT_PATH_BLURB,
                         GIMP_CONFIG_PATH_FILE,
                         NULL,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_PATH_IMPORT_MERGE,
                            "path-import-merge",
                            "Default merge imported path",
                            PATH_IMPORT_MERGE_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_PATH_IMPORT_SCALE,
                            "path-import-scale",
                            "Default scale imported path",
                            PATH_IMPORT_SCALE_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_SELECTION_FEATHER_RADIUS,
                           "selection-feather-radius",
                           "Selection feather radius",
                           SELECTION_FEATHER_RADIUS_BLURB,
                           /* NOTE: the max value is the max value of
                            * "std-dev-x|y" arguments in operation
                            * "gegl:gaussian-blur", multiplied by magic
                            * number 3.5 (see gimp_gegl_apply_feather())
                            * code).
                            */
                            0.0, 5250.0, 5.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SELECTION_FEATHER_EDGE_LOCK,
                            "selection-feather-edge-lock",
                            "Selection feather edge lock",
                            SELECTION_FEATHER_EDGE_LOCK_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_SELECTION_GROW_RADIUS,
                           "selection-grow-radius",
                           "Selection grow radius",
                           SELECTION_GROW_RADIUS_BLURB,
                           1.0, 32767.0, 1.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_SELECTION_SHRINK_RADIUS,
                           "selection-shrink-radius",
                           "Selection shrink radius",
                           SELECTION_SHRINK_RADIUS_BLURB,
                           1.0, 32767.0, 1.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SELECTION_SHRINK_EDGE_LOCK,
                            "selection-shrink-edge-lock",
                            "Selection shrink edge lock",
                            SELECTION_SHRINK_EDGE_LOCK_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_SELECTION_BORDER_RADIUS,
                           "selection-border-radius",
                           "Selection border radius",
                           SELECTION_BORDER_RADIUS_BLURB,
                           1.0, 32767.0, 5.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SELECTION_BORDER_EDGE_LOCK,
                            "selection-border-edge-lock",
                            "Selection border edge lock",
                            SELECTION_BORDER_EDGE_LOCK_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_SELECTION_BORDER_STYLE,
                         "selection-border-style",
                         "Selection border style",
                         SELECTION_BORDER_STYLE_BLURB,
                         GIMP_TYPE_CHANNEL_BORDER_STYLE,
                         GIMP_CHANNEL_BORDER_STYLE_SMOOTH,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_FILL_OPTIONS,
                           "fill-options",
                           "Fill Options",
                           FILL_OPTIONS_BLURB,
                           GIMP_TYPE_FILL_OPTIONS,
                           GIMP_PARAM_STATIC_STRINGS |
                           GIMP_CONFIG_PARAM_AGGREGATE);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_STROKE_OPTIONS,
                           "stroke-options",
                           "Stroke Options",
                           STROKE_OPTIONS_BLURB,
                           GIMP_TYPE_STROKE_OPTIONS,
                           GIMP_PARAM_STATIC_STRINGS |
                           GIMP_CONFIG_PARAM_AGGREGATE);

  g_object_unref (half_transparent);
}

static void
gimp_dialog_config_init (GimpDialogConfig *config)
{
  GeglColor *half_transparent = gegl_color_new ("black");

  gimp_color_set_alpha (half_transparent, 0.5);
  config->channel_new_color = half_transparent;
}

static void
gimp_dialog_config_constructed (GObject *object)
{
  GimpDialogConfig        *config = GIMP_DIALOG_CONFIG (object);
  GimpDialogConfigPrivate *priv   = GET_PRIVATE (object);
  GimpContext             *context;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_GIMP (priv->gimp));

  context = gimp_get_user_context (priv->gimp);

  config->fill_options = gimp_fill_options_new (priv->gimp, context, TRUE);
  gimp_context_set_serialize_properties (GIMP_CONTEXT (config->fill_options),
                                         0);

  g_signal_connect (config->fill_options, "notify",
                    G_CALLBACK (gimp_dialog_config_fill_options_notify),
                    config);

  config->stroke_options = gimp_stroke_options_new (priv->gimp, context, TRUE);
  gimp_context_set_serialize_properties (GIMP_CONTEXT (config->stroke_options),
                                         0);

  g_signal_connect (config->stroke_options, "notify",
                    G_CALLBACK (gimp_dialog_config_stroke_options_notify),
                    config);
}

static void
gimp_dialog_config_finalize (GObject *object)
{
  GimpDialogConfig *config = GIMP_DIALOG_CONFIG (object);

  g_clear_pointer (&config->color_profile_path,  g_free);
  g_clear_pointer (&config->layer_new_name,      g_free);
  g_clear_pointer (&config->channel_new_name,    g_free);
  g_clear_pointer (&config->path_new_name,       g_free);
  g_clear_pointer (&config->path_export_path,    g_free);
  g_clear_pointer (&config->path_import_path,    g_free);

  g_clear_object (&config->fill_options);
  g_clear_object (&config->stroke_options);
  g_clear_object (&config->channel_new_color);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_dialog_config_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpDialogConfig        *config = GIMP_DIALOG_CONFIG (object);
  GimpDialogConfigPrivate *priv   = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      priv->gimp = g_value_get_object (value); /* don't ref */
      break;

    case PROP_COLOR_PROFILE_POLICY:
      config->color_profile_policy = g_value_get_enum (value);
      break;
    case PROP_METADATA_ROTATION_POLICY:
      config->metadata_rotation_policy = g_value_get_enum (value);
      break;

    case PROP_COLOR_PROFILE_PATH:
      g_set_str (&config->color_profile_path, g_value_get_string (value));
      break;

    case PROP_IMAGE_CONVERT_PROFILE_INTENT:
      config->image_convert_profile_intent = g_value_get_enum (value);
      break;
    case PROP_IMAGE_CONVERT_PROFILE_BPC:
      config->image_convert_profile_bpc = g_value_get_boolean (value);
      break;

    case PROP_IMAGE_CONVERT_PRECISION_LAYER_DITHER_METHOD:
      config->image_convert_precision_layer_dither_method =
        g_value_get_enum (value);
      break;
    case PROP_IMAGE_CONVERT_PRECISION_TEXT_LAYER_DITHER_METHOD:
      config->image_convert_precision_text_layer_dither_method =
        g_value_get_enum (value);
      break;
    case PROP_IMAGE_CONVERT_PRECISION_CHANNEL_DITHER_METHOD:
      config->image_convert_precision_channel_dither_method =
        g_value_get_enum (value);
      break;

    case PROP_IMAGE_CONVERT_INDEXED_PALETTE_TYPE:
      config->image_convert_indexed_palette_type = g_value_get_enum (value);
      break;
    case PROP_IMAGE_CONVERT_INDEXED_MAX_COLORS:
      config->image_convert_indexed_max_colors = g_value_get_int (value);
      break;
    case PROP_IMAGE_CONVERT_INDEXED_REMOVE_DUPLICATES:
      config->image_convert_indexed_remove_duplicates = g_value_get_boolean (value);
      break;
    case PROP_IMAGE_CONVERT_INDEXED_DITHER_TYPE:
      config->image_convert_indexed_dither_type = g_value_get_enum (value);
      break;
    case PROP_IMAGE_CONVERT_INDEXED_DITHER_ALPHA:
      config->image_convert_indexed_dither_alpha = g_value_get_boolean (value);
      break;
    case PROP_IMAGE_CONVERT_INDEXED_DITHER_TEXT_LAYERS:
      config->image_convert_indexed_dither_text_layers = g_value_get_boolean (value);
      break;

    case PROP_IMAGE_RESIZE_FILL_TYPE:
      config->image_resize_fill_type = g_value_get_enum (value);
      break;
    case PROP_IMAGE_RESIZE_LAYER_SET:
      config->image_resize_layer_set = g_value_get_enum (value);
      break;
    case PROP_IMAGE_RESIZE_RESIZE_TEXT_LAYERS:
      config->image_resize_resize_text_layers = g_value_get_boolean (value);
      break;

    case PROP_LAYER_NEW_NAME:
      g_set_str (&config->layer_new_name, g_value_get_string (value));
      break;
    case PROP_LAYER_NEW_MODE:
      config->layer_new_mode = g_value_get_enum (value);
      break;
    case PROP_LAYER_NEW_BLEND_SPACE:
      config->layer_new_blend_space = g_value_get_enum (value);
      break;
    case PROP_LAYER_NEW_COMPOSITE_SPACE:
      config->layer_new_composite_space = g_value_get_enum (value);
      break;
    case PROP_LAYER_NEW_COMPOSITE_MODE:
      config->layer_new_composite_mode = g_value_get_enum (value);
      break;
    case PROP_LAYER_NEW_OPACITY:
      config->layer_new_opacity = g_value_get_double (value);
      break;
    case PROP_LAYER_NEW_FILL_TYPE:
      config->layer_new_fill_type = g_value_get_enum (value);
      break;

    case PROP_LAYER_RESIZE_FILL_TYPE:
      config->layer_resize_fill_type = g_value_get_enum (value);
      break;

    case PROP_LAYER_ADD_MASK_TYPE:
      config->layer_add_mask_type = g_value_get_enum (value);
      break;
    case PROP_LAYER_ADD_MASK_INVERT:
      config->layer_add_mask_invert = g_value_get_boolean (value);
      break;
    case PROP_LAYER_ADD_MASK_EDIT_MASK:
      config->layer_add_mask_edit_mask = g_value_get_boolean (value);
      break;

    case PROP_LAYER_MERGE_TYPE:
      config->layer_merge_type = g_value_get_enum (value);
      break;
    case PROP_LAYER_MERGE_ACTIVE_GROUP_ONLY:
      config->layer_merge_active_group_only = g_value_get_boolean (value);
      break;
    case PROP_LAYER_MERGE_DISCARD_INVISIBLE:
      config->layer_merge_discard_invisible = g_value_get_boolean (value);
      break;

    case PROP_CHANNEL_NEW_NAME:
      g_set_str (&config->channel_new_name, g_value_get_string (value));
      break;
    case PROP_CHANNEL_NEW_COLOR:
      g_clear_object (&config->channel_new_color);
      config->channel_new_color = gegl_color_duplicate (g_value_get_object (value));
      break;

    case PROP_PATH_NEW_NAME:
      g_set_str (&config->path_new_name, g_value_get_string (value));
      break;

    case PROP_PATH_EXPORT_PATH:
      g_set_str (&config->path_export_path, g_value_get_string (value));
      break;
    case PROP_PATH_EXPORT_ACTIVE_ONLY:
      config->path_export_active_only = g_value_get_boolean (value);
      break;

    case PROP_PATH_IMPORT_PATH:
      g_set_str (&config->path_import_path, g_value_get_string (value));
      break;
    case PROP_PATH_IMPORT_MERGE:
      config->path_import_merge = g_value_get_boolean (value);
      break;
    case PROP_PATH_IMPORT_SCALE:
      config->path_import_scale = g_value_get_boolean (value);
      break;

    case PROP_SELECTION_FEATHER_RADIUS:
      config->selection_feather_radius = g_value_get_double (value);
      break;
    case PROP_SELECTION_FEATHER_EDGE_LOCK:
      config->selection_feather_edge_lock = g_value_get_boolean (value);
      break;

    case PROP_SELECTION_GROW_RADIUS:
      config->selection_grow_radius = g_value_get_double (value);
      break;

    case PROP_SELECTION_SHRINK_RADIUS:
      config->selection_shrink_radius = g_value_get_double (value);
      break;
    case PROP_SELECTION_SHRINK_EDGE_LOCK:
      config->selection_shrink_edge_lock = g_value_get_boolean (value);
      break;

    case PROP_SELECTION_BORDER_RADIUS:
      config->selection_border_radius = g_value_get_double (value);
      break;
    case PROP_SELECTION_BORDER_EDGE_LOCK:
      config->selection_border_edge_lock = g_value_get_boolean (value);
      break;
    case PROP_SELECTION_BORDER_STYLE:
      config->selection_border_style = g_value_get_enum (value);
      break;

    case PROP_FILL_OPTIONS:
      if (g_value_get_object (value))
        gimp_config_sync (g_value_get_object (value) ,
                          G_OBJECT (config->fill_options), 0);
      break;
    case PROP_STROKE_OPTIONS:
      if (g_value_get_object (value))
        gimp_config_sync (g_value_get_object (value) ,
                          G_OBJECT (config->stroke_options), 0);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dialog_config_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpDialogConfig        *config = GIMP_DIALOG_CONFIG (object);
  GimpDialogConfigPrivate *priv   = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, priv->gimp);
      break;

    case PROP_COLOR_PROFILE_POLICY:
      g_value_set_enum (value, config->color_profile_policy);
      break;
    case PROP_METADATA_ROTATION_POLICY:
      g_value_set_enum (value, config->metadata_rotation_policy);
      break;

    case PROP_COLOR_PROFILE_PATH:
      g_value_set_string (value, config->color_profile_path);
      break;

    case PROP_IMAGE_CONVERT_PROFILE_INTENT:
      g_value_set_enum (value, config->image_convert_profile_intent);
      break;
    case PROP_IMAGE_CONVERT_PROFILE_BPC:
      g_value_set_boolean (value, config->image_convert_profile_bpc);
      break;

    case PROP_IMAGE_CONVERT_PRECISION_LAYER_DITHER_METHOD:
      g_value_set_enum (value,
                        config->image_convert_precision_layer_dither_method);
      break;
    case PROP_IMAGE_CONVERT_PRECISION_TEXT_LAYER_DITHER_METHOD:
      g_value_set_enum (value,
                        config->image_convert_precision_text_layer_dither_method);
      break;
    case PROP_IMAGE_CONVERT_PRECISION_CHANNEL_DITHER_METHOD:
      g_value_set_enum (value,
                        config->image_convert_precision_channel_dither_method);
      break;

    case PROP_IMAGE_CONVERT_INDEXED_PALETTE_TYPE:
      g_value_set_enum (value, config->image_convert_indexed_palette_type);
      break;
    case PROP_IMAGE_CONVERT_INDEXED_MAX_COLORS:
      g_value_set_int (value, config->image_convert_indexed_max_colors);
      break;
    case PROP_IMAGE_CONVERT_INDEXED_REMOVE_DUPLICATES:
      g_value_set_boolean (value, config->image_convert_indexed_remove_duplicates);
      break;
    case PROP_IMAGE_CONVERT_INDEXED_DITHER_TYPE:
      g_value_set_enum (value, config->image_convert_indexed_dither_type);
      break;
    case PROP_IMAGE_CONVERT_INDEXED_DITHER_ALPHA:
      g_value_set_boolean (value, config->image_convert_indexed_dither_alpha);
      break;
    case PROP_IMAGE_CONVERT_INDEXED_DITHER_TEXT_LAYERS:
      g_value_set_boolean (value, config->image_convert_indexed_dither_text_layers);
      break;

    case PROP_IMAGE_RESIZE_FILL_TYPE:
      g_value_set_enum (value, config->image_resize_fill_type);
      break;
    case PROP_IMAGE_RESIZE_LAYER_SET:
      g_value_set_enum (value, config->image_resize_layer_set);
      break;
    case PROP_IMAGE_RESIZE_RESIZE_TEXT_LAYERS:
      g_value_set_boolean (value, config->image_resize_resize_text_layers);
      break;

    case PROP_LAYER_NEW_NAME:
      g_value_set_string (value, config->layer_new_name);
      break;
    case PROP_LAYER_NEW_MODE:
      g_value_set_enum (value, config->layer_new_mode);
      break;
    case PROP_LAYER_NEW_BLEND_SPACE:
      g_value_set_enum (value, config->layer_new_blend_space);
      break;
    case PROP_LAYER_NEW_COMPOSITE_SPACE:
      g_value_set_enum (value, config->layer_new_composite_space);
      break;
    case PROP_LAYER_NEW_COMPOSITE_MODE:
      g_value_set_enum (value, config->layer_new_composite_mode);
      break;
    case PROP_LAYER_NEW_OPACITY:
      g_value_set_double (value, config->layer_new_opacity);
      break;
    case PROP_LAYER_NEW_FILL_TYPE:
      g_value_set_enum (value, config->layer_new_fill_type);
      break;

    case PROP_LAYER_RESIZE_FILL_TYPE:
      g_value_set_enum (value, config->layer_resize_fill_type);
      break;

    case PROP_LAYER_ADD_MASK_TYPE:
      g_value_set_enum (value, config->layer_add_mask_type);
      break;
    case PROP_LAYER_ADD_MASK_INVERT:
      g_value_set_boolean (value, config->layer_add_mask_invert);
      break;
    case PROP_LAYER_ADD_MASK_EDIT_MASK:
      g_value_set_boolean (value, config->layer_add_mask_edit_mask);
      break;

    case PROP_LAYER_MERGE_TYPE:
      g_value_set_enum (value, config->layer_merge_type);
      break;
    case PROP_LAYER_MERGE_ACTIVE_GROUP_ONLY:
      g_value_set_boolean (value, config->layer_merge_active_group_only);
      break;
    case PROP_LAYER_MERGE_DISCARD_INVISIBLE:
      g_value_set_boolean (value, config->layer_merge_discard_invisible);
      break;

    case PROP_CHANNEL_NEW_NAME:
      g_value_set_string (value, config->channel_new_name);
      break;
    case PROP_CHANNEL_NEW_COLOR:
      g_value_set_object (value, config->channel_new_color);
      break;

    case PROP_PATH_NEW_NAME:
      g_value_set_string (value, config->path_new_name);
      break;

    case PROP_PATH_EXPORT_PATH:
      g_value_set_string (value, config->path_export_path);
      break;
    case PROP_PATH_EXPORT_ACTIVE_ONLY:
      g_value_set_boolean (value, config->path_export_active_only);
      break;

    case PROP_PATH_IMPORT_PATH:
      g_value_set_string (value, config->path_import_path);
      break;
    case PROP_PATH_IMPORT_MERGE:
      g_value_set_boolean (value, config->path_import_merge);
      break;
    case PROP_PATH_IMPORT_SCALE:
      g_value_set_boolean (value, config->path_import_scale);
      break;

    case PROP_SELECTION_FEATHER_RADIUS:
      g_value_set_double (value, config->selection_feather_radius);
      break;
    case PROP_SELECTION_FEATHER_EDGE_LOCK:
      g_value_set_boolean (value, config->selection_feather_edge_lock);
      break;

    case PROP_SELECTION_GROW_RADIUS:
      g_value_set_double (value, config->selection_grow_radius);
      break;

    case PROP_SELECTION_SHRINK_RADIUS:
      g_value_set_double (value, config->selection_shrink_radius);
      break;
    case PROP_SELECTION_SHRINK_EDGE_LOCK:
      g_value_set_boolean (value, config->selection_shrink_edge_lock);
      break;

    case PROP_SELECTION_BORDER_RADIUS:
      g_value_set_double (value, config->selection_border_radius);
      break;
    case PROP_SELECTION_BORDER_EDGE_LOCK:
      g_value_set_boolean (value, config->selection_border_edge_lock);
      break;
    case PROP_SELECTION_BORDER_STYLE:
      g_value_set_enum (value, config->selection_border_style);
      break;

    case PROP_FILL_OPTIONS:
      g_value_set_object (value, config->fill_options);
      break;
    case PROP_STROKE_OPTIONS:
      g_value_set_object (value, config->stroke_options);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dialog_config_fill_options_notify (GObject    *object,
                                        GParamSpec *pspec,
                                        gpointer    data)
{
  /*  ignore notifications on parent class properties such as fg/bg  */
  if (pspec->owner_type == G_TYPE_FROM_INSTANCE (object))
    g_object_notify (G_OBJECT (data), "fill-options");
}

static void
gimp_dialog_config_stroke_options_notify (GObject    *object,
                                          GParamSpec *pspec,
                                          gpointer    data)
{
  /*  see above  */
  if (pspec->owner_type == G_TYPE_FROM_INSTANCE (object))
    g_object_notify (G_OBJECT (data), "stroke-options");
}
