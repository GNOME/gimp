/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmatoolinfo.h"

#include "widgets/ligmapropwidgets.h"
#include "widgets/ligmawidgets-utils.h"

#include "ligmatooloptions-gui.h"
#include "ligmatransformoptions.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_TYPE,
  PROP_DIRECTION,
  PROP_INTERPOLATION,
  PROP_CLIP
};


static void     ligma_transform_options_config_iface_init (LigmaConfigInterface *config_iface);

static void     ligma_transform_options_set_property (GObject         *object,
                                                     guint            property_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);
static void     ligma_transform_options_get_property (GObject         *object,
                                                     guint            property_id,
                                                     GValue          *value,
                                                     GParamSpec      *pspec);

static void     ligma_transform_options_reset        (LigmaConfig      *config);

G_DEFINE_TYPE_WITH_CODE (LigmaTransformOptions, ligma_transform_options,
                         LIGMA_TYPE_TOOL_OPTIONS,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                ligma_transform_options_config_iface_init))

#define parent_class ligma_transform_options_parent_class

static LigmaConfigInterface *parent_config_iface = NULL;


static void
ligma_transform_options_class_init (LigmaTransformOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_transform_options_set_property;
  object_class->get_property = ligma_transform_options_get_property;

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_TYPE,
                         "type",
                         NULL, NULL,
                         LIGMA_TYPE_TRANSFORM_TYPE,
                         LIGMA_TRANSFORM_TYPE_LAYER,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_DIRECTION,
                         "direction",
                         _("Direction"),
                         _("Direction of transformation"),
                         LIGMA_TYPE_TRANSFORM_DIRECTION,
                         LIGMA_TRANSFORM_FORWARD,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_INTERPOLATION,
                         "interpolation",
                         _("Interpolation"),
                         _("Interpolation method"),
                         LIGMA_TYPE_INTERPOLATION_TYPE,
                         LIGMA_INTERPOLATION_LINEAR,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_CLIP,
                         "clip",
                         _("Clipping"),
                         _("How to clip"),
                         LIGMA_TYPE_TRANSFORM_RESIZE,
                         LIGMA_TRANSFORM_RESIZE_ADJUST,
                         LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_transform_options_config_iface_init (LigmaConfigInterface *config_iface)
{
  parent_config_iface = g_type_interface_peek_parent (config_iface);

  config_iface->reset = ligma_transform_options_reset;
}

static void
ligma_transform_options_init (LigmaTransformOptions *options)
{
}

static void
ligma_transform_options_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  LigmaTransformOptions *options = LIGMA_TRANSFORM_OPTIONS (object);

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
ligma_transform_options_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  LigmaTransformOptions *options = LIGMA_TRANSFORM_OPTIONS (object);

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
ligma_transform_options_reset (LigmaConfig *config)
{
  LigmaToolOptions *tool_options = LIGMA_TOOL_OPTIONS (config);
  GParamSpec      *pspec;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                        "interpolation");

  if (pspec)
    G_PARAM_SPEC_ENUM (pspec)->default_value =
      tool_options->tool_info->ligma->config->interpolation_type;

  parent_config_iface->reset (config);
}

/**
 * ligma_transform_options_gui:
 * @tool_options:  a #LigmaToolOptions
 * @direction:     whether to show the direction frame
 * @interpolation: whether to show the interpolation menu
 * @clipping:      whether to show the clipping menu
 *
 * Build the Transform Tool Options.
 *
 * Returns: a container holding the transform tool options
 **/
GtkWidget *
ligma_transform_options_gui (LigmaToolOptions *tool_options,
                            gboolean         direction,
                            gboolean         interpolation,
                            gboolean         clipping)
{
  GObject              *config  = G_OBJECT (tool_options);
  LigmaTransformOptions *options = LIGMA_TRANSFORM_OPTIONS (tool_options);
  GtkWidget            *vbox    = ligma_tool_options_gui (tool_options);
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

  box = ligma_prop_enum_icon_box_new (config, "type", "ligma", 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 0);

  if (direction)
    {
      frame = ligma_prop_enum_radio_frame_new (config, "direction", NULL,
                                              0, 0);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

      options->direction_frame = frame;
    }

  /*  the interpolation menu  */
  if (interpolation)
    {
      combo = ligma_prop_enum_combo_box_new (config, "interpolation", 0, 0);
      ligma_int_combo_box_set_label (LIGMA_INT_COMBO_BOX (combo), _("Interpolation"));
      g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
      gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
    }

  /*  the clipping menu  */
  if (clipping)
    {
      combo = ligma_prop_enum_combo_box_new (config, "clip", 0, 0);
      ligma_int_combo_box_set_label (LIGMA_INT_COMBO_BOX (combo), _("Clipping"));
      g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
      gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
    }

  return vbox;
}
