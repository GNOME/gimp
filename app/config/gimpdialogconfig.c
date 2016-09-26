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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

  PROP_IMAGE_CONVERT_PROFILE_INTENT,
  PROP_IMAGE_CONVERT_PROFILE_BPC,

  PROP_IMAGE_CONVERT_PRECISION_LAYER_DITHER_METHOD,
  PROP_IMAGE_CONVERT_PRECISION_TEXT_LAYER_DITHER_METHOD,
  PROP_IMAGE_CONVERT_PRECISION_CHANNEL_DITHER_METHOD,

  PROP_LAYER_NEW_NAME,
  PROP_LAYER_NEW_FILL_TYPE,

  PROP_LAYER_ADD_MASK_TYPE,
  PROP_LAYER_ADD_MASK_INVERT,

  PROP_LAYER_MERGE_TYPE,
  PROP_LAYER_MERGE_ACTIVE_GROUP_ONLY,
  PROP_LAYER_MERGE_DISCARD_INVISIBLE,

  PROP_CHANNEL_NEW_NAME,
  PROP_CHANNEL_NEW_COLOR,

  PROP_VECTORS_NEW_NAME,

  PROP_VECTORS_EXPORT_PATH,
  PROP_VECTORS_EXPORT_ACTIVE_ONLY,

  PROP_VECTORS_IMPORT_PATH,
  PROP_VECTORS_IMPORT_MERGE,
  PROP_VECTORS_IMPORT_SCALE,

  PROP_SELECTION_FEATHER_RADIUS,

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
        G_TYPE_INSTANCE_GET_PRIVATE (config, \
                                     GIMP_TYPE_DIALOG_CONFIG, \
                                     GimpDialogConfigPrivate)


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


G_DEFINE_TYPE (GimpDialogConfig, gimp_dialog_config, GIMP_TYPE_GUI_CONFIG)

#define parent_class gimp_dialog_config_parent_class


static void
gimp_dialog_config_class_init (GimpDialogConfigClass *klass)
{
  GObjectClass *object_class     = G_OBJECT_CLASS (klass);
  GimpRGB       half_transparent = { 0.0, 0.0, 0.0, 0.5 };

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

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_IMAGE_CONVERT_PROFILE_INTENT,
                         "image-convert-profile-intent",
                         "Default rendering intent fo color profile conversion",
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

  GIMP_CONFIG_PROP_STRING (object_class, PROP_LAYER_NEW_NAME,
                           "layer-new-name",
                           "Default new layer name",
                           LAYER_NEW_NAME_BLURB,
                           _("Layer"),
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_LAYER_NEW_FILL_TYPE,
                         "layer-new-fill-type",
                         "Default new layer fill type",
                         LAYER_NEW_FILL_TYPE_BLURB,
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

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_LAYER_MERGE_TYPE,
                         "layer-merge-type",
                         "Default layer merge type",
                         LAYER_MERGE_TYPE_BLURB,
                         GIMP_TYPE_MERGE_TYPE,
                         GIMP_EXPAND_AS_NECESSARY,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_LAYER_MERGE_ACTIVE_GROUP_ONLY,
                            "layer-merge-active-group-only",
                            "Default layer merge active group only",
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

  GIMP_CONFIG_PROP_RGB (object_class, PROP_CHANNEL_NEW_COLOR,
                        "channel-new-color",
                        "Default new channel color and opacity",
                        CHANNEL_NEW_COLOR_BLURB,
                        TRUE,
                        &half_transparent,
                        GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_STRING (object_class, PROP_VECTORS_NEW_NAME,
                           "path-new-name",
                           "Default new path name",
                           VECTORS_NEW_NAME_BLURB,
                           _("Path"),
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_PATH (object_class, PROP_VECTORS_EXPORT_PATH,
                         "path-export-path",
                         "Default path export folder path",
                         VECTORS_EXPORT_PATH_BLURB,
                         GIMP_CONFIG_PATH_FILE,
                         NULL,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_VECTORS_EXPORT_ACTIVE_ONLY,
                            "path-export-active-only",
                            "Default export only the active path",
                            VECTORS_EXPORT_ACTIVE_ONLY_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_PATH (object_class, PROP_VECTORS_IMPORT_PATH,
                         "path-import-path",
                         "Default path import folder path",
                         VECTORS_IMPORT_PATH_BLURB,
                         GIMP_CONFIG_PATH_FILE,
                         NULL,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_VECTORS_IMPORT_MERGE,
                            "path-import-merge",
                            "Default merge imported vectors",
                            VECTORS_IMPORT_MERGE_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_VECTORS_IMPORT_SCALE,
                            "path-import-scale",
                            "Default scale imported vectors",
                            VECTORS_IMPORT_SCALE_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_SELECTION_FEATHER_RADIUS,
                           "selection-feather-radius",
                           "Selection feather radius",
                           SELECTION_FEATHER_RADIUS_BLURB,
                           0.0, 32767.0, 5.0,
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

  g_type_class_add_private (klass, sizeof (GimpDialogConfigPrivate));
}

static void
gimp_dialog_config_init (GimpDialogConfig *config)
{
}

static void
gimp_dialog_config_constructed (GObject *object)
{
  GimpDialogConfig        *config = GIMP_DIALOG_CONFIG (object);
  GimpDialogConfigPrivate *priv   = GET_PRIVATE (object);
  GimpContext             *context;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_GIMP (priv->gimp));

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

  if (config->layer_new_name)
    {
      g_free (config->layer_new_name);
      config->layer_new_name = NULL;
    }

  if (config->channel_new_name)
    {
      g_free (config->channel_new_name);
      config->channel_new_name = NULL;
    }

  if (config->vectors_new_name)
    {
      g_free (config->vectors_new_name);
      config->vectors_new_name = NULL;
    }

  if (config->fill_options)
    {
      g_object_unref (config->fill_options);
      config->fill_options = NULL;
    }

  if (config->stroke_options)
    {
      g_object_unref (config->stroke_options);
      config->stroke_options = NULL;
    }

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

    case PROP_LAYER_NEW_NAME:
      if (config->layer_new_name)
        g_free (config->layer_new_name);
      config->layer_new_name = g_value_dup_string (value);
      break;
    case PROP_LAYER_NEW_FILL_TYPE:
      config->layer_new_fill_type = g_value_get_enum (value);
      break;

    case PROP_LAYER_ADD_MASK_TYPE:
      config->layer_add_mask_type = g_value_get_enum (value);
      break;
    case PROP_LAYER_ADD_MASK_INVERT:
      config->layer_add_mask_invert = g_value_get_boolean (value);
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
      if (config->channel_new_name)
        g_free (config->channel_new_name);
      config->channel_new_name = g_value_dup_string (value);
      break;
    case PROP_CHANNEL_NEW_COLOR:
      gimp_value_get_rgb (value, &config->channel_new_color);
      break;

    case PROP_VECTORS_NEW_NAME:
      if (config->vectors_new_name)
        g_free (config->vectors_new_name);
      config->vectors_new_name = g_value_dup_string (value);
      break;

    case PROP_VECTORS_EXPORT_PATH:
      if (config->vectors_export_path)
        g_free (config->vectors_export_path);
      config->vectors_export_path = g_value_dup_string (value);
      break;
    case PROP_VECTORS_EXPORT_ACTIVE_ONLY:
      config->vectors_export_active_only = g_value_get_boolean (value);
      break;

    case PROP_VECTORS_IMPORT_PATH:
      if (config->vectors_import_path)
        g_free (config->vectors_import_path);
      config->vectors_import_path = g_value_dup_string (value);
      break;
    case PROP_VECTORS_IMPORT_MERGE:
      config->vectors_import_merge = g_value_get_boolean (value);
      break;
    case PROP_VECTORS_IMPORT_SCALE:
      config->vectors_import_scale = g_value_get_boolean (value);
      break;

    case PROP_SELECTION_FEATHER_RADIUS:
      config->selection_feather_radius = g_value_get_double (value);
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

    case PROP_LAYER_NEW_NAME:
      g_value_set_string (value, config->layer_new_name);
      break;
    case PROP_LAYER_NEW_FILL_TYPE:
      g_value_set_enum (value, config->layer_new_fill_type);
      break;

    case PROP_LAYER_ADD_MASK_TYPE:
      g_value_set_enum (value, config->layer_add_mask_type);
      break;
    case PROP_LAYER_ADD_MASK_INVERT:
      g_value_set_boolean (value, config->layer_add_mask_invert);
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
      gimp_value_set_rgb (value, &config->channel_new_color);
      break;

    case PROP_VECTORS_NEW_NAME:
      g_value_set_string (value, config->vectors_new_name);
      break;

    case PROP_VECTORS_EXPORT_PATH:
      g_value_set_string (value, config->vectors_export_path);
      break;
    case PROP_VECTORS_EXPORT_ACTIVE_ONLY:
      g_value_set_boolean (value, config->vectors_export_active_only);
      break;

    case PROP_VECTORS_IMPORT_PATH:
      g_value_set_string (value, config->vectors_import_path);
      break;
    case PROP_VECTORS_IMPORT_MERGE:
      g_value_set_boolean (value, config->vectors_import_merge);
      break;
    case PROP_VECTORS_IMPORT_SCALE:
      g_value_set_boolean (value, config->vectors_import_scale);
      break;

    case PROP_SELECTION_FEATHER_RADIUS:
      g_value_set_double (value, config->selection_feather_radius);
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
  g_object_notify (G_OBJECT (data), "fill-options");
}

static void
gimp_dialog_config_stroke_options_notify (GObject    *object,
                                          GParamSpec *pspec,
                                          gpointer    data)
{
  g_object_notify (G_OBJECT (data), "stroke-options");
}
