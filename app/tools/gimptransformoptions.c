/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimprotatetool.h"
#include "gimpscaletool.h"
#include "gimptooloptions-gui.h"
#include "gimptransformoptions.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_TYPE,
  PROP_DIRECTION,
  PROP_INTERPOLATION,
  PROP_CLIP,
  PROP_SHOW_PREVIEW,
  PROP_PREVIEW_OPACITY,
  PROP_GRID_TYPE,
  PROP_GRID_SIZE,
  PROP_CONSTRAIN,
};


static void     gimp_transform_options_set_property (GObject         *object,
                                                     guint            property_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);
static void     gimp_transform_options_get_property (GObject         *object,
                                                     guint            property_id,
                                                     GValue          *value,
                                                     GParamSpec      *pspec);

static void     gimp_transform_options_reset        (GimpToolOptions *tool_options);

static gboolean gimp_transform_options_sync_grid    (GBinding        *binding,
                                                     const GValue    *source_value,
                                                     GValue          *target_value,
                                                     gpointer         user_data);


G_DEFINE_TYPE (GimpTransformOptions, gimp_transform_options,
               GIMP_TYPE_TOOL_OPTIONS)

#define parent_class gimp_transform_options_parent_class


static void
gimp_transform_options_class_init (GimpTransformOptionsClass *klass)
{
  GObjectClass         *object_class  = G_OBJECT_CLASS (klass);
  GimpToolOptionsClass *options_class = GIMP_TOOL_OPTIONS_CLASS (klass);

  object_class->set_property = gimp_transform_options_set_property;
  object_class->get_property = gimp_transform_options_get_property;

  options_class->reset       = gimp_transform_options_reset;

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_TYPE,
                                 "type", NULL,
                                 GIMP_TYPE_TRANSFORM_TYPE,
                                 GIMP_TRANSFORM_TYPE_LAYER,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_DIRECTION,
                                 "direction",
                                 N_("Direction of transformation"),
                                 GIMP_TYPE_TRANSFORM_DIRECTION,
                                 GIMP_TRANSFORM_FORWARD,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_INTERPOLATION,
                                 "interpolation",
                                 N_("Interpolation method"),
                                 GIMP_TYPE_INTERPOLATION_TYPE,
                                 GIMP_INTERPOLATION_LINEAR,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_CLIP,
                                 "clip",
                                 N_("How to clip"),
                                 GIMP_TYPE_TRANSFORM_RESIZE,
                                 GIMP_TRANSFORM_RESIZE_ADJUST,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SHOW_PREVIEW,
                                    "show-preview",
                                    N_("Show a preview of the transformed image"),
                                    TRUE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_PREVIEW_OPACITY,
                                   "preview-opacity",
                                   N_("Opacity of the preview image"),
                                   0.0, 1.0, 1.0,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_GRID_TYPE,
                                 "grid-type",
                                 N_("Composition guides such as rule of thirds"),
                                 GIMP_TYPE_GUIDES_TYPE,
                                 GIMP_GUIDES_N_LINES,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_GRID_SIZE,
                                "grid-size",
                                N_("Size of a grid cell for variable number of composition guides"),
                                1, 128, 15,
                                GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_CONSTRAIN,
                                    "constrain",
                                    NULL,
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_transform_options_init (GimpTransformOptions *options)
{
  options->recursion_level = 3;
}

static void
gimp_transform_options_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_OPTIONS (object);

  switch (property_id)
    {
    case PROP_TYPE:
      options->type = g_value_get_enum (value);
      break;
    case PROP_DIRECTION:
      options->direction = g_value_get_enum (value);
      break;
    case PROP_INTERPOLATION:
      options->interpolation = g_value_get_enum (value);
      break;
    case PROP_CLIP:
      options->clip = g_value_get_enum (value);
      break;
    case PROP_SHOW_PREVIEW:
      options->show_preview = g_value_get_boolean (value);
      break;
    case PROP_PREVIEW_OPACITY:
      options->preview_opacity = g_value_get_double (value);
      break;
    case PROP_GRID_TYPE:
      options->grid_type = g_value_get_enum (value);
      break;
    case PROP_GRID_SIZE:
      options->grid_size = g_value_get_int (value);
      break;
    case PROP_CONSTRAIN:
      options->constrain = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_transform_options_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GimpTransformOptions *options = GIMP_TRANSFORM_OPTIONS (object);

  switch (property_id)
    {
    case PROP_TYPE:
      g_value_set_enum (value, options->type);
      break;
    case PROP_DIRECTION:
      g_value_set_enum (value, options->direction);
      break;
    case PROP_INTERPOLATION:
      g_value_set_enum (value, options->interpolation);
      break;
    case PROP_CLIP:
      g_value_set_enum (value, options->clip);
      break;
    case PROP_SHOW_PREVIEW:
      g_value_set_boolean (value, options->show_preview);
      break;
    case PROP_PREVIEW_OPACITY:
      g_value_set_double (value, options->preview_opacity);
      break;
    case PROP_GRID_TYPE:
      g_value_set_enum (value, options->grid_type);
      break;
    case PROP_GRID_SIZE:
      g_value_set_int (value, options->grid_size);
      break;
    case PROP_CONSTRAIN:
      g_value_set_boolean (value, options->constrain);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_transform_options_reset (GimpToolOptions *tool_options)
{
  GParamSpec *pspec;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (tool_options),
                                        "interpolation");

  if (pspec)
    G_PARAM_SPEC_ENUM (pspec)->default_value =
      tool_options->tool_info->gimp->config->interpolation_type;

  GIMP_TOOL_OPTIONS_CLASS (parent_class)->reset (tool_options);
}

/**
 * gimp_transform_options_gui:
 * @tool_options: a #GimpToolOptions
 *
 * Build the Transform Tool Options.
 *
 * Return value: a container holding the transform tool options
 **/
GtkWidget *
gimp_transform_options_gui (GimpToolOptions *tool_options)
{
  GObject     *config = G_OBJECT (tool_options);
  GtkWidget   *vbox   = gimp_tool_options_gui (tool_options);
  GtkWidget   *hbox;
  GtkWidget   *box;
  GtkWidget   *label;
  GtkWidget   *frame;
  GtkWidget   *combo;
  GtkWidget   *scale;
  GtkWidget   *grid_box;
  const gchar *constrain_label = NULL;
  const gchar *constrain_tip   = NULL;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Transform:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  box = gimp_prop_enum_stock_box_new (config, "type", "gimp", 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 0);
  gtk_widget_show (box);

  frame = gimp_prop_enum_radio_frame_new (config, "direction",
                                          _("Direction"), 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  the interpolation menu  */
  frame = gimp_frame_new (_("Interpolation:"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  combo = gimp_prop_enum_combo_box_new (config, "interpolation", 0, 0);
  gtk_container_add (GTK_CONTAINER (frame), combo);
  gtk_widget_show (combo);

  /*  the clipping menu  */
  frame = gimp_frame_new (_("Clipping:"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  combo = gimp_prop_enum_combo_box_new (config, "clip", 0, 0);
  gtk_container_add (GTK_CONTAINER (frame), combo);
  gtk_widget_show (combo);

  /*  the preview frame  */
  scale = gimp_prop_opacity_spin_scale_new (config, "preview-opacity",
                                            _("Image opacity"));
  frame = gimp_prop_expanding_frame_new (config, "show-preview",
                                         _("Show image preview"),
                                         scale, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  the guides frame  */
  frame = gimp_frame_new (_("Guides"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), grid_box);
  gtk_widget_show (grid_box);

  /*  the guides type menu  */
  combo = gimp_prop_enum_combo_box_new (config, "grid-type", 0, 0);
  gtk_box_pack_start (GTK_BOX (grid_box), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  /*  the grid density scale  */
  scale = gimp_prop_spin_scale_new (config, "grid-size", NULL,
                                    1.8, 8.0, 0);
  gtk_box_pack_start (GTK_BOX (grid_box), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  g_object_bind_property_full (config, "grid-type",
                               scale,  "visible",
                               G_BINDING_SYNC_CREATE,
                               gimp_transform_options_sync_grid,
                               NULL,
                               NULL, NULL);

  if (tool_options->tool_info->tool_type == GIMP_TYPE_ROTATE_TOOL)
    {
      constrain_label = _("15 degrees  (%s)");
      constrain_tip   = _("Limit rotation steps to 15 degrees");
    }
  else if (tool_options->tool_info->tool_type == GIMP_TYPE_SCALE_TOOL)
    {
      constrain_label = _("Keep aspect  (%s)");
      constrain_tip   = _("Keep the original aspect ratio");
    }

  if (constrain_label)
    {
      GtkWidget       *button;
      gchar           *label;
      GdkModifierType  constrain_mask;

      constrain_mask = gimp_get_constrain_behavior_mask ();

      label = g_strdup_printf (constrain_label,
                               gimp_get_mod_string (constrain_mask));

      button = gimp_prop_check_button_new (config, "constrain", label);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      gimp_help_set_help_data (button, constrain_tip, NULL);

      g_free (label);
    }

  return vbox;
}

gboolean
gimp_transform_options_show_preview (GimpTransformOptions *options)
{
  g_return_val_if_fail (GIMP_IS_TRANSFORM_OPTIONS (options), FALSE);

  return (options->show_preview                           &&
          options->type      == GIMP_TRANSFORM_TYPE_LAYER &&
          options->direction == GIMP_TRANSFORM_FORWARD);
}


/*  private functions  */

static gboolean
gimp_transform_options_sync_grid (GBinding     *binding,
                                  const GValue *source_value,
                                  GValue       *target_value,
                                  gpointer      user_data)
{
  GimpGuidesType type = g_value_get_enum (source_value);

  g_value_set_boolean (target_value,
                       type == GIMP_GUIDES_N_LINES ||
                       type == GIMP_GUIDES_SPACING);

  return TRUE;
}
