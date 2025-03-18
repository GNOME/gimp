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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpcoreconfig.h"
#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimptooloptions-gui.h"
#include "gimptransformoptions.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_TYPE,
  PROP_DIRECTION,
  PROP_INTERPOLATION,
  PROP_CLIP
};


static void     gimp_transform_options_config_iface_init (GimpConfigInterface *config_iface);

static void     gimp_transform_options_set_property  (GObject         *object,
                                                      guint            property_id,
                                                      const GValue    *value,
                                                      GParamSpec      *pspec);
static void     gimp_transform_options_get_property  (GObject         *object,
                                                      guint            property_id,
                                                      GValue          *value,
                                                      GParamSpec      *pspec);
static void     gimp_transform_options_style_updated (GimpGuiConfig   *config,
                                                      GParamSpec      *pspec,
                                                      GtkWidget       *box);

static void     gimp_transform_options_reset        (GimpConfig      *config);

G_DEFINE_TYPE_WITH_CODE (GimpTransformOptions, gimp_transform_options,
                         GIMP_TYPE_TOOL_OPTIONS,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_transform_options_config_iface_init))

#define parent_class gimp_transform_options_parent_class

static GimpConfigInterface *parent_config_iface = NULL;


static void
gimp_transform_options_class_init (GimpTransformOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_transform_options_set_property;
  object_class->get_property = gimp_transform_options_get_property;

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_TYPE,
                         "type",
                         NULL, NULL,
                         GIMP_TYPE_TRANSFORM_TYPE,
                         GIMP_TRANSFORM_TYPE_LAYER,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_DIRECTION,
                         "direction",
                         _("Direction"),
                         _("Direction of transformation"),
                         GIMP_TYPE_TRANSFORM_DIRECTION,
                         GIMP_TRANSFORM_FORWARD,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_INTERPOLATION,
                         "interpolation",
                         _("Interpolation"),
                         _("Interpolation method"),
                         GIMP_TYPE_INTERPOLATION_TYPE,
                         GIMP_INTERPOLATION_LINEAR,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_CLIP,
                         "clip",
                         _("Clipping"),
                         _("How to clip"),
                         GIMP_TYPE_TRANSFORM_RESIZE,
                         GIMP_TRANSFORM_RESIZE_ADJUST,
                         GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_transform_options_config_iface_init (GimpConfigInterface *config_iface)
{
  parent_config_iface = g_type_interface_peek_parent (config_iface);

  config_iface->reset = gimp_transform_options_reset;
}

static void
gimp_transform_options_init (GimpTransformOptions *options)
{
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
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_transform_options_style_updated (GimpGuiConfig *config,
                                      GParamSpec    *pspec,
                                      GtkWidget     *box)
{
  GtkIconSize icon_size = GTK_ICON_SIZE_MENU;

  if (config->override_icon_size)
    {
      switch (config->custom_icon_size)
        {
        case GIMP_ICON_SIZE_LARGE:
          icon_size = GTK_ICON_SIZE_LARGE_TOOLBAR;
          break;

        case GIMP_ICON_SIZE_HUGE:
          icon_size = GTK_ICON_SIZE_DND;
          break;

        case GIMP_ICON_SIZE_MEDIUM:
        case GIMP_ICON_SIZE_SMALL:
        default:
          icon_size = GTK_ICON_SIZE_MENU;
        }
    }

  gimp_enum_icon_box_set_icon_size (box, icon_size);
}

static void
gimp_transform_options_reset (GimpConfig *config)
{
  GimpToolOptions *tool_options = GIMP_TOOL_OPTIONS (config);
  GParamSpec      *pspec;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                        "interpolation");

  if (pspec)
    G_PARAM_SPEC_ENUM (pspec)->default_value =
      tool_options->tool_info->gimp->config->interpolation_type;

  parent_config_iface->reset (config);
}

/**
 * gimp_transform_options_gui:
 * @tool_options:  a #GimpToolOptions
 * @direction:     whether to show the direction frame
 * @interpolation: whether to show the interpolation menu
 * @clipping:      whether to show the clipping menu
 *
 * Build the Transform Tool Options.
 *
 * Returns: a container holding the transform tool options
 **/
GtkWidget *
gimp_transform_options_gui (GimpToolOptions *tool_options,
                            gboolean         direction,
                            gboolean         interpolation,
                            gboolean         clipping)
{
  GObject              *config     = G_OBJECT (tool_options);
  GimpContext          *context    = GIMP_CONTEXT (tool_options);
  GimpGuiConfig        *gui_config = GIMP_GUI_CONFIG (context->gimp->config);
  GimpTransformOptions *options    = GIMP_TRANSFORM_OPTIONS (tool_options);
  GtkWidget            *vbox       = gimp_tool_options_gui (tool_options);
  GtkWidget            *hbox;
  GtkWidget            *box;
  GtkWidget            *label;
  GtkWidget            *frame;
  GtkWidget            *combo;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  options->type_box = hbox;

  label = gtk_label_new (_("Transform:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  box = gimp_prop_enum_icon_box_new (config, "type", "gimp", 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 0);

  g_signal_connect_object (gui_config,
                           "notify::override-theme-icon-size",
                           G_CALLBACK (gimp_transform_options_style_updated),
                           box, G_CONNECT_AFTER);
  g_signal_connect_object (gui_config,
                           "notify::custom-icon-size",
                           G_CALLBACK (gimp_transform_options_style_updated),
                           box, G_CONNECT_AFTER);
  gimp_transform_options_style_updated (gui_config, NULL, box);

  if (direction)
    {
      frame = gimp_prop_enum_radio_frame_new (config, "direction", NULL,
                                              0, 0);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

      options->direction_frame = frame;
    }

  /*  the interpolation menu  */
  if (interpolation)
    {
      combo = gimp_prop_enum_combo_box_new (config, "interpolation", 0, 0);
      gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo), _("Interpolation"));
      g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
      gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
    }

  /*  the clipping menu  */
  if (clipping)
    {
      combo = gimp_prop_enum_combo_box_new (config, "clip", 0, 0);
      gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo), _("Clipping"));
      g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
      gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
    }

  return vbox;
}
