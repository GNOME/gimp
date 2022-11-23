/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "ligmaregionselectoptions.h"
#include "ligmaregionselecttool.h"
#include "ligmafuzzyselecttool.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_SELECT_TRANSPARENT,
  PROP_SAMPLE_MERGED,
  PROP_DIAGONAL_NEIGHBORS,
  PROP_THRESHOLD,
  PROP_SELECT_CRITERION,
  PROP_DRAW_MASK
};


static void   ligma_region_select_options_config_iface_init (LigmaConfigInterface *config_iface);

static void   ligma_region_select_options_set_property (GObject         *object,
                                                       guint            property_id,
                                                       const GValue    *value,
                                                       GParamSpec      *pspec);
static void   ligma_region_select_options_get_property (GObject         *object,
                                                       guint            property_id,
                                                       GValue          *value,
                                                       GParamSpec      *pspec);

static void   ligma_region_select_options_reset        (LigmaConfig      *config);


G_DEFINE_TYPE_WITH_CODE (LigmaRegionSelectOptions, ligma_region_select_options,
                         LIGMA_TYPE_SELECTION_OPTIONS,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                ligma_region_select_options_config_iface_init))

#define parent_class ligma_region_select_options_parent_class

static LigmaConfigInterface *parent_config_iface = NULL;


static void
ligma_region_select_options_class_init (LigmaRegionSelectOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_region_select_options_set_property;
  object_class->get_property = ligma_region_select_options_get_property;

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_SELECT_TRANSPARENT,
                            "select-transparent",
                            _("Select transparent areas"),
                            _("Allow completely transparent regions "
                              "to be selected"),
                            TRUE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_SAMPLE_MERGED,
                            "sample-merged",
                            _("Sample merged"),
                            _("Base selection on all visible layers"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_DIAGONAL_NEIGHBORS,
                            "diagonal-neighbors",
                            _("Diagonal neighbors"),
                            _("Treat diagonally neighboring pixels as "
                              "connected"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_THRESHOLD,
                           "threshold",
                           _("Threshold"),
                           _("Maximum color difference"),
                           0.0, 255.0, 15.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_SELECT_CRITERION,
                         "select-criterion",
                         _("Select by"),
                         _("Selection criterion"),
                         LIGMA_TYPE_SELECT_CRITERION,
                         LIGMA_SELECT_CRITERION_COMPOSITE,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_DRAW_MASK,
                            "draw-mask",
                            _("Draw mask"),
                            _("Draw the selected region's mask"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_region_select_options_config_iface_init (LigmaConfigInterface *config_iface)
{
  parent_config_iface = g_type_interface_peek_parent (config_iface);

  config_iface->reset = ligma_region_select_options_reset;
}

static void
ligma_region_select_options_init (LigmaRegionSelectOptions *options)
{
}

static void
ligma_region_select_options_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  LigmaRegionSelectOptions *options = LIGMA_REGION_SELECT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SELECT_TRANSPARENT:
      options->select_transparent = g_value_get_boolean (value);
      break;

    case PROP_SAMPLE_MERGED:
      options->sample_merged = g_value_get_boolean (value);
      break;

    case PROP_DIAGONAL_NEIGHBORS:
      options->diagonal_neighbors = g_value_get_boolean (value);
      break;

    case PROP_THRESHOLD:
      options->threshold = g_value_get_double (value);
      break;

    case PROP_SELECT_CRITERION:
      options->select_criterion = g_value_get_enum (value);
      break;

    case PROP_DRAW_MASK:
      options->draw_mask = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_region_select_options_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  LigmaRegionSelectOptions *options = LIGMA_REGION_SELECT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SELECT_TRANSPARENT:
      g_value_set_boolean (value, options->select_transparent);
      break;

    case PROP_SAMPLE_MERGED:
      g_value_set_boolean (value, options->sample_merged);
      break;

    case PROP_DIAGONAL_NEIGHBORS:
      g_value_set_boolean (value, options->diagonal_neighbors);
      break;

    case PROP_THRESHOLD:
      g_value_set_double (value, options->threshold);
      break;

    case PROP_SELECT_CRITERION:
      g_value_set_enum (value, options->select_criterion);
      break;

    case PROP_DRAW_MASK:
      g_value_set_boolean (value, options->draw_mask);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_region_select_options_reset (LigmaConfig *config)
{
  LigmaToolOptions *tool_options = LIGMA_TOOL_OPTIONS (config);
  GParamSpec      *pspec;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                        "threshold");

  if (pspec)
    G_PARAM_SPEC_DOUBLE (pspec)->default_value =
      tool_options->tool_info->ligma->config->default_threshold;

  parent_config_iface->reset (config);
}

GtkWidget *
ligma_region_select_options_gui (LigmaToolOptions *tool_options)
{
  GObject   *config  = G_OBJECT (tool_options);
  GtkWidget *vbox    = ligma_selection_options_gui (tool_options);
  GtkWidget *button;
  GtkWidget *scale;
  GtkWidget *combo;
  GType      tool_type;

  tool_type = tool_options->tool_info->tool_type;

  /*  the select transparent areas toggle  */
  button = ligma_prop_check_button_new (config, "select-transparent", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  /*  the sample merged toggle  */
  button = ligma_prop_check_button_new (config, "sample-merged", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  /*  the diagonal neighbors toggle  */
  if (tool_type == LIGMA_TYPE_FUZZY_SELECT_TOOL)
    {
      button = ligma_prop_check_button_new (config, "diagonal-neighbors", NULL);
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    }

  /*  the threshold scale  */
  scale = ligma_prop_spin_scale_new (config, "threshold",
                                    1.0, 16.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  /*  the select criterion combo  */
  combo = ligma_prop_enum_combo_box_new (config, "select-criterion", 0, 0);
  ligma_int_combo_box_set_label (LIGMA_INT_COMBO_BOX (combo), _("Select by"));
  gtk_box_pack_start (GTK_BOX (vbox), combo, TRUE, TRUE, 0);

  /*  the show mask toggle  */
  button = ligma_prop_check_button_new (config, "draw-mask", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  return vbox;
}
