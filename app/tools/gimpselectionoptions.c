/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED (GtkTooltips)
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpregionselecttool.h"
#include "gimpiscissorstool.h"
#include "gimpselectionoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_OPERATION,
  PROP_ANTIALIAS,
  PROP_FEATHER,
  PROP_FEATHER_RADIUS,
  PROP_SELECT_TRANSPARENT,
  PROP_SAMPLE_MERGED,
  PROP_THRESHOLD,
  PROP_SELECT_CRITERION,
  PROP_INTERACTIVE
};


static void   gimp_selection_options_set_property (GObject         *object,
                                                   guint            property_id,
                                                   const GValue    *value,
                                                   GParamSpec      *pspec);
static void   gimp_selection_options_get_property (GObject         *object,
                                                   guint            property_id,
                                                   GValue          *value,
                                                   GParamSpec      *pspec);

static void   gimp_selection_options_reset        (GimpToolOptions *tool_options);


G_DEFINE_TYPE (GimpSelectionOptions, gimp_selection_options,
               GIMP_TYPE_TOOL_OPTIONS)

#define parent_class gimp_selection_options_parent_class


static void
gimp_selection_options_class_init (GimpSelectionOptionsClass *klass)
{
  GObjectClass         *object_class  = G_OBJECT_CLASS (klass);
  GimpToolOptionsClass *options_class = GIMP_TOOL_OPTIONS_CLASS (klass);

  object_class->set_property = gimp_selection_options_set_property;
  object_class->get_property = gimp_selection_options_get_property;

  options_class->reset       = gimp_selection_options_reset;

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_OPERATION,
                                 "operation", NULL,
                                 GIMP_TYPE_CHANNEL_OPS,
                                 GIMP_CHANNEL_OP_REPLACE,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_ANTIALIAS,
                                    "antialias",
                                    N_("Smooth edges"),
                                    TRUE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FEATHER,
                                    "feather", NULL,
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_FEATHER_RADIUS,
                                   "feather-radius", NULL,
                                   0.0, 100.0, 10.0,
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SELECT_TRANSPARENT,
                                    "select-transparent",
                                    N_("Allow completely transparent regions "
                                       "to be selected"),
                                    TRUE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SAMPLE_MERGED,
                                    "sample-merged",
                                    N_("Base selection on all visible layers"),
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_THRESHOLD,
                                   "threshold",
                                   N_("Maximum color difference"),
                                   0.0, 255.0, 15.0,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_SELECT_CRITERION,
                                 "select-criterion", NULL,
                                 GIMP_TYPE_SELECT_CRITERION,
                                 GIMP_SELECT_CRITERION_COMPOSITE,
                                 GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_INTERACTIVE,
                                    "interactive", NULL,
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_selection_options_init (GimpSelectionOptions *options)
{
}

static void
gimp_selection_options_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpSelectionOptions *options = GIMP_SELECTION_OPTIONS (object);

  switch (property_id)
    {
    case PROP_OPERATION:
      options->operation = g_value_get_enum (value);
      break;
    case PROP_ANTIALIAS:
      options->antialias = g_value_get_boolean (value);
      break;
    case PROP_FEATHER:
      options->feather = g_value_get_boolean (value);
      break;
    case PROP_FEATHER_RADIUS:
      options->feather_radius = g_value_get_double (value);
      break;

    case PROP_SELECT_TRANSPARENT:
      options->select_transparent = g_value_get_boolean (value);
      break;
    case PROP_SAMPLE_MERGED:
      options->sample_merged = g_value_get_boolean (value);
      break;
    case PROP_THRESHOLD:
      options->threshold = g_value_get_double (value);
      break;
    case PROP_SELECT_CRITERION:
      options->select_criterion = g_value_get_enum (value);
      break;

    case PROP_INTERACTIVE:
      options->interactive = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_selection_options_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GimpSelectionOptions *options = GIMP_SELECTION_OPTIONS (object);

  switch (property_id)
    {
    case PROP_OPERATION:
      g_value_set_enum (value, options->operation);
      break;
    case PROP_ANTIALIAS:
      g_value_set_boolean (value, options->antialias);
      break;
    case PROP_FEATHER:
      g_value_set_boolean (value, options->feather);
      break;
    case PROP_FEATHER_RADIUS:
      g_value_set_double (value, options->feather_radius);
      break;

    case PROP_SELECT_TRANSPARENT:
      g_value_set_boolean (value, options->select_transparent);
      break;
    case PROP_SAMPLE_MERGED:
      g_value_set_boolean (value, options->sample_merged);
      break;
    case PROP_THRESHOLD:
      g_value_set_double (value, options->threshold);
      break;
    case PROP_SELECT_CRITERION:
      g_value_set_enum (value, options->select_criterion);
      break;

    case PROP_INTERACTIVE:
      g_value_set_boolean (value, options->interactive);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_selection_options_reset (GimpToolOptions *tool_options)
{
  GParamSpec *pspec;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (tool_options),
                                        "threshold");

  if (pspec)
    G_PARAM_SPEC_DOUBLE (pspec)->default_value =
      GIMP_GUI_CONFIG (tool_options->tool_info->gimp->config)->default_threshold;

  GIMP_TOOL_OPTIONS_CLASS (parent_class)->reset (tool_options);
}

static const gchar *
gimp_selection_options_get_modifier (GimpChannelOps operation)
{
  GdkModifierType mod = 0;

  switch (operation)
    {
    case GIMP_CHANNEL_OP_ADD:
      mod = GDK_SHIFT_MASK;
      break;

    case GIMP_CHANNEL_OP_SUBTRACT:
      mod = GDK_CONTROL_MASK;
      break;

    case GIMP_CHANNEL_OP_REPLACE:
      mod = 0;
      break;

    case GIMP_CHANNEL_OP_INTERSECT:
      mod = GDK_CONTROL_MASK | GDK_SHIFT_MASK;
      break;
    }

  return gimp_get_mod_string (mod);
}

GtkWidget *
gimp_selection_options_gui (GimpToolOptions *tool_options)
{
  GObject              *config  = G_OBJECT (tool_options);
  GimpSelectionOptions *options = GIMP_SELECTION_OPTIONS (tool_options);
  GtkWidget            *vbox    = gimp_tool_options_gui (tool_options);
  GtkWidget            *button;

  /*  the selection operation radio buttons  */
  {
    GtkWidget *hbox;
    GtkWidget *label;
    GList     *children;
    GList     *list;
    gint       i;

    hbox = gimp_prop_enum_stock_box_new (config, "operation",
                                         "gimp-selection", 0, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    children = gtk_container_get_children (GTK_CONTAINER (hbox));

    /*  add modifier keys to the tooltips  */
    for (list = children, i = 0; list; list = list->next, i++)
      {
        GtkWidget       *button   = list->data;
        GtkTooltipsData *data     = gtk_tooltips_data_get (button);
        const gchar     *modifier = gimp_selection_options_get_modifier (i);

        if (! modifier)
          continue;

        if (data && data->tip_text)
          {
            gchar *tip = g_strdup_printf ("%s  (%s)", data->tip_text, modifier);

            gimp_help_set_help_data (button, tip, NULL);
            g_free (tip);
          }
        else
          {
            gimp_help_set_help_data (button, modifier, NULL);
          }
      }

    /*  move GIMP_CHANNEL_OP_REPLACE to the front  */
    gtk_box_reorder_child (GTK_BOX (hbox),
                           GTK_WIDGET (children->next->next->data), 0);

    g_list_free (children);

    label = gtk_label_new (_("Mode:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_box_reorder_child (GTK_BOX (hbox), label, 0);
    gtk_widget_show (label);
  }

  /*  the antialias toggle button  */
  button = gimp_prop_check_button_new (config, "antialias",
                                       _("Antialiasing"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  options->antialias_toggle = button;

  /*  the feather frame  */
  {
    GtkWidget *frame;
    GtkWidget *table;

    table = gtk_table_new (1, 3, FALSE);
    gtk_table_set_col_spacings (GTK_TABLE (table), 2);

    frame = gimp_prop_expanding_frame_new (config, "feather",
                                           _("Feather edges"),
                                           table, NULL);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show (frame);

    /*  the feather radius scale  */
    gimp_prop_scale_entry_new (config, "feather-radius",
                               GTK_TABLE (table), 0, 0,
                               _("Radius:"),
                               1.0, 10.0, 1,
                               FALSE, 0.0, 0.0);
  }

  /* selection tool with an interactive boundary that can be toggled */
  if (tool_options->tool_info->tool_type == GIMP_TYPE_ISCISSORS_TOOL)
    {
      button = gimp_prop_check_button_new (config, "interactive",
                                           _("Interactive boundary"));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
    }

  /*  selection tools which operate on colors or contiguous regions  */
  if (g_type_is_a (tool_options->tool_info->tool_type,
                   GIMP_TYPE_REGION_SELECT_TOOL))
    {
      GtkWidget *table;
      GtkWidget *combo;

      /*  the select transparent areas toggle  */
      button = gimp_prop_check_button_new (config, "select-transparent",
                                           _("Select transparent areas"));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      /*  the sample merged toggle  */
      button = gimp_prop_check_button_new (config, "sample-merged",
                                           _("Sample merged"));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      /*  the threshold scale  */
      table = gtk_table_new (2, 3, FALSE);
      gtk_table_set_col_spacings (GTK_TABLE (table), 2);
      gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
      gtk_widget_show (table);

      gimp_prop_scale_entry_new (config, "threshold",
                                 GTK_TABLE (table), 0, 0,
                                 _("Threshold:"),
                                 1.0, 16.0, 1,
                                 FALSE, 0.0, 0.0);

      /*  the select criterion combo  */
      combo = gimp_prop_enum_combo_box_new (config, "select-criterion", 0, 0);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                                 _("Select by:"), 0.0, 0.5,
                                 combo, 2, FALSE);
    }

  return vbox;
}

