/* GIMP - The GNU Image Manipulation Program
 *
 * gimpnpointdeformationoptions.h
 * Copyright (C) 2013 Marek Dvoroznak <dvoromar@gmail.com>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpspinscale.h"

#include "gimpnpointdeformationoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"

enum
{
    PROP_0,
    PROP_SQUARE_SIZE,
    PROP_RIGIDITY,
    PROP_ASAP_DEFORMATION,
    PROP_MLS_WEIGHTS,
    PROP_MLS_WEIGHTS_ALPHA,
    PROP_MESH_VISIBLE,
    PROP_PAUSE_DEFORMATION
};


static void   gimp_n_point_deformation_options_set_property (GObject      *object,
                                                             guint         property_id,
                                                             const GValue *value,
                                                             GParamSpec   *pspec);
static void   gimp_n_point_deformation_options_get_property (GObject      *object,
                                                             guint         property_id,
                                                             GValue       *value,
                                                             GParamSpec   *pspec);


G_DEFINE_TYPE (GimpNPointDeformationOptions, gimp_n_point_deformation_options,
               GIMP_TYPE_TOOL_OPTIONS)

#define parent_class gimp_n_point_deformation_options_parent_class


static void
gimp_n_point_deformation_options_class_init (GimpNPointDeformationOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_n_point_deformation_options_set_property;
  object_class->get_property = gimp_n_point_deformation_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_DOUBLE  (object_class, PROP_SQUARE_SIZE,
                                    "square-size", _("Square Size"),
                                    5.0, 1000.0, 20.0,
                                    GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_DOUBLE  (object_class, PROP_RIGIDITY,
                                    "rigidity", _("Rigidity"),
                                    1.0, 10000.0, 100.0,
                                    GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_ASAP_DEFORMATION,
                                    "ASAP-deformation", _("Deformation Type"),
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_MLS_WEIGHTS,
                                    "MLS-weights", _("MLS Weights"),
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_DOUBLE  (object_class, PROP_MLS_WEIGHTS_ALPHA,
                                    "MLS-weights-alpha", _("MLS Weights Alpha"),
                                    0.1, 2.0, 1.0,
                                    GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_MESH_VISIBLE,
                                    "mesh-visible", _("Mesh Visible"),
                                    TRUE,
                                    GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_PAUSE_DEFORMATION,
                                    "pause-deformation", _("Pause Deformation"),
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_n_point_deformation_options_init (GimpNPointDeformationOptions *options)
{
}

static void
gimp_n_point_deformation_options_set_property (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec)
{
  GimpNPointDeformationOptions *options = GIMP_N_POINT_DEFORMATION_OPTIONS (object);

  switch (property_id)
    {
      case PROP_SQUARE_SIZE:
        options->square_size = g_value_get_double (value);
        break;
      case PROP_RIGIDITY:
        options->rigidity = g_value_get_double (value);
        break;
      case PROP_ASAP_DEFORMATION:
        options->ASAP_deformation = g_value_get_boolean (value);
        break;
      case PROP_MLS_WEIGHTS:
        options->MLS_weights = g_value_get_boolean (value);
        break;
      case PROP_MLS_WEIGHTS_ALPHA:
        options->square_size = g_value_get_double (value);
        break;
      case PROP_MESH_VISIBLE:
        options->mesh_visible = g_value_get_boolean (value);

        if (options->check_mesh_visible)
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->check_mesh_visible),
                                        options->mesh_visible);
        break;
      case PROP_PAUSE_DEFORMATION:
        options->deformation_is_paused = g_value_get_boolean (value);

        if (options->check_pause_deformation)
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->check_pause_deformation),
                                        options->deformation_is_paused);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
gimp_n_point_deformation_options_get_property (GObject    *object,
                                               guint       property_id,
                                               GValue     *value,
                                               GParamSpec *pspec)
{
  GimpNPointDeformationOptions *options = GIMP_N_POINT_DEFORMATION_OPTIONS (object);

  switch (property_id)
    {
      case PROP_SQUARE_SIZE:
        g_value_set_double (value, options->square_size);
        break;
      case PROP_RIGIDITY:
        g_value_set_double (value, options->rigidity);
        break;
      case PROP_ASAP_DEFORMATION:
        g_value_set_boolean (value, options->ASAP_deformation);
        break;
      case PROP_MLS_WEIGHTS:
        g_value_set_boolean (value, options->MLS_weights);
        break;
      case PROP_MLS_WEIGHTS_ALPHA:
        g_value_set_double (value, options->square_size);
        break;
      case PROP_MESH_VISIBLE:
        g_value_set_boolean (value, options->mesh_visible);
        break;
      case PROP_PAUSE_DEFORMATION:
        g_value_set_boolean (value, options->deformation_is_paused);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

GtkWidget *
gimp_n_point_deformation_options_gui (GimpToolOptions *tool_options)
{
  GimpNPointDeformationOptions *npd_options = GIMP_N_POINT_DEFORMATION_OPTIONS (tool_options);
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_tool_options_gui (tool_options);
  GtkWidget *widget;

  widget = gtk_label_new ("Note: These options are temporary.");
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  widget = gimp_prop_check_button_new (config, "mesh-visible", _("Show Mesh"));
  npd_options->check_mesh_visible = widget;
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  widget = gimp_prop_spin_scale_new (config, "square-size", _("Square Size"), 1.0, 10.0, 0);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (widget), 5.0, 1000.0);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  widget = gimp_prop_spin_scale_new (config, "rigidity", _("Rigidity"), 1.0, 10.0, 0);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (widget), 1.0, 10000.0);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  widget = gimp_prop_boolean_combo_box_new (config, "ASAP-deformation", _("ASAP"), _("ARAP"));
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  widget = gimp_prop_boolean_combo_box_new (config, "MLS-weights", _("Enabled"), _("Disabled"));
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  widget = gimp_prop_spin_scale_new (config, "MLS-weights-alpha", _("MLS Weights Alpha"), 0.1, 0.1, 1);
  gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (widget), 0.1, 2.0);
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  widget = gimp_prop_check_button_new (config, "pause-deformation", _("Pause Deformation"));
  npd_options->check_pause_deformation = widget;
  gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  return vbox;
}

gboolean
gimp_n_point_deformation_options_is_deformation_paused (GimpNPointDeformationOptions *npd_options)
{
  return npd_options->deformation_is_paused;
}

void
gimp_n_point_deformation_options_set_pause_deformation (GimpNPointDeformationOptions *npd_options,
                                                        gboolean                      is_active)
{
  g_object_set (G_OBJECT (npd_options), "pause-deformation", is_active, NULL);
}

void
gimp_n_point_deformation_options_toggle_pause_deformation (GimpNPointDeformationOptions *npd_options)
{
  gimp_n_point_deformation_options_set_pause_deformation (npd_options,
                                                          !npd_options->deformation_is_paused);
}
