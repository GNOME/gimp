/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimptoolinfo.h"

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
  PROP_PREVIEW_TYPE,
  PROP_GRID_TYPE,
  PROP_GRID_SIZE,
  PROP_CONSTRAIN
};


static void   gimp_transform_options_set_property   (GObject         *object,
                                                     guint            property_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);
static void   gimp_transform_options_get_property   (GObject         *object,
                                                     guint            property_id,
                                                     GValue          *value,
                                                     GParamSpec      *pspec);

static void   gimp_transform_options_reset          (GimpToolOptions *tool_options);

static void   gimp_transform_options_preview_notify (GimpTransformOptions *options,
                                                     GParamSpec           *pspec,
                                                     GtkWidget            *density_box);


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
                                 "direction", NULL,
                                 GIMP_TYPE_TRANSFORM_DIRECTION,
                                 GIMP_TRANSFORM_FORWARD,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_INTERPOLATION,
                                 "interpolation", NULL,
                                 GIMP_TYPE_INTERPOLATION_TYPE,
                                 GIMP_INTERPOLATION_LINEAR,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_CLIP,
                                 "clip", NULL,
                                 GIMP_TYPE_TRANSFORM_RESIZE,
                                 GIMP_TRANSFORM_RESIZE_ADJUST,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_PREVIEW_TYPE,
                                 "preview-type", NULL,
                                 GIMP_TYPE_TRANSFORM_PREVIEW_TYPE,
                                 GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_GRID_TYPE,
                                 "grid-type", NULL,
                                 GIMP_TYPE_TRANSFORM_GRID_TYPE,
                                 GIMP_TRANSFORM_GRID_TYPE_N_LINES,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_GRID_SIZE,
                                "grid-size", NULL,
                                1, 128, 15,
                                GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_CONSTRAIN,
                                    "constrain", NULL,
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
    case PROP_PREVIEW_TYPE:
      options->preview_type = g_value_get_enum (value);
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
    case PROP_PREVIEW_TYPE:
      g_value_set_enum (value, options->preview_type);
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

GtkWidget *
gimp_transform_options_gui (GimpToolOptions *tool_options)
{
  GObject              *config  = G_OBJECT (tool_options);
  GimpTransformOptions *options = GIMP_TRANSFORM_OPTIONS (tool_options);
  GtkWidget            *vbox    = gimp_tool_options_gui (tool_options);
  GtkWidget            *hbox;
  GtkWidget            *label;
  GtkWidget            *frame;
  GtkWidget            *table;
  GtkWidget            *combo;
  GtkWidget            *button;
  const gchar          *constrain = NULL;

  hbox = gimp_prop_enum_stock_box_new (config, "type", "gimp", 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Transform:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (hbox), label, 0);
  gtk_widget_show (label);

  frame = gimp_prop_enum_radio_frame_new (config, "direction",
                                          _("Direction"), 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  the interpolation menu  */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Interpolation:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = gimp_prop_enum_combo_box_new (config, "interpolation", 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  /*  the clipping menu  */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Clipping:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = gimp_prop_enum_combo_box_new (config, "clip", 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  /*  the preview frame  */
  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  the preview type menu  */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_frame_set_label_widget (GTK_FRAME (frame), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Preview:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = gimp_prop_enum_combo_box_new (config, "preview-type", 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  /*  the grid type menu  */
  button = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), button);
  gtk_widget_show (button);

  combo = gimp_prop_enum_combo_box_new (config, "grid-type", 0, 0);
  gtk_box_pack_start (GTK_BOX (button), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  /*  the grid density scale  */
  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 2);
  gtk_box_pack_start (GTK_BOX (button), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  gtk_widget_set_sensitive (button,
                            options->preview_type ==
                            GIMP_TRANSFORM_PREVIEW_TYPE_GRID ||
                            options->preview_type ==
                            GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE_GRID);

  g_signal_connect (config, "notify::preview-type",
                    G_CALLBACK (gimp_transform_options_preview_notify),
                    button);

  gimp_prop_scale_entry_new (config, "grid-size",
                             GTK_TABLE (table), 0, 0,
                             NULL,
                             1.0, 8.0, 0,
                             FALSE, 0.0, 0.0);

  if (tool_options->tool_info->tool_type == GIMP_TYPE_ROTATE_TOOL)
    {
      constrain = (_("15 degrees  (%s)"));
    }
  else if (tool_options->tool_info->tool_type == GIMP_TYPE_SCALE_TOOL)
    {
      constrain = (_("Keep aspect  (%s)"));
    }

  if (constrain)
    {
      gchar *label = g_strdup_printf (constrain,
                                      gimp_get_mod_string (GDK_CONTROL_MASK));

      button = gimp_prop_check_button_new (config, "constrain", label);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      g_free (label);
    }

  return vbox;
}


/*  private functions  */

static void
gimp_transform_options_preview_notify (GimpTransformOptions *options,
                                       GParamSpec           *pspec,
                                       GtkWidget            *density_box)
{
  gtk_widget_set_sensitive (density_box,
                            options->preview_type ==
                            GIMP_TRANSFORM_PREVIEW_TYPE_GRID ||
                            options->preview_type ==
                            GIMP_TRANSFORM_PREVIEW_TYPE_IMAGE_GRID);
}
