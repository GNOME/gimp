/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpselectionoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_OPERATION,
  PROP_ANTIALIAS,
  PROP_FEATHER,
  PROP_FEATHER_RADIUS
};


static void   gimp_selection_options_set_property  (GObject       *object,
                                                    guint          property_id,
                                                    const GValue  *value,
                                                    GParamSpec    *pspec);
static void   gimp_selection_options_get_property  (GObject       *object,
                                                    guint          property_id,
                                                    GValue        *value,
                                                    GParamSpec    *pspec);
static void   gimp_selection_options_style_updated (GimpGuiConfig *config,
                                                    GParamSpec    *pspec,
                                                    GtkWidget     *box);



G_DEFINE_TYPE (GimpSelectionOptions, gimp_selection_options,
               GIMP_TYPE_TOOL_OPTIONS)

#define parent_class gimp_selection_options_parent_class


static void
gimp_selection_options_class_init (GimpSelectionOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_selection_options_set_property;
  object_class->get_property = gimp_selection_options_get_property;

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_OPERATION,
                         "operation",
                         NULL, NULL,
                         GIMP_TYPE_CHANNEL_OPS,
                         GIMP_CHANNEL_OP_REPLACE,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_ANTIALIAS,
                            "antialias",
                            _("Antialiasing"),
                            _("Smooth edges"),
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_FEATHER,
                            "feather",
                            _("Feather edges"),
                            _("Enable feathering of selection edges"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_FEATHER_RADIUS,
                           "feather-radius",
                           _("Radius"),
                           _("Radius of feathering"),
                           0.0, 100.0, 10.0,
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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static const gchar *
gimp_selection_options_get_modifiers (GimpChannelOps operation)
{
  GdkModifierType extend_mask;
  GdkModifierType modify_mask;
  GdkModifierType modifiers = 0;

  extend_mask = gimp_get_extend_selection_mask ();
  modify_mask = gimp_get_modify_selection_mask ();

  switch (operation)
    {
    case GIMP_CHANNEL_OP_ADD:
      modifiers = extend_mask;
      break;

    case GIMP_CHANNEL_OP_SUBTRACT:
      modifiers = modify_mask;
      break;

    case GIMP_CHANNEL_OP_REPLACE:
      modifiers = 0;
      break;

    case GIMP_CHANNEL_OP_INTERSECT:
      modifiers = extend_mask | modify_mask;
      break;
    }

  return gimp_get_mod_string (modifiers);
}

GtkWidget *
gimp_selection_options_gui (GimpToolOptions *tool_options)
{
  GObject              *config     = G_OBJECT (tool_options);
  GimpContext          *context    = GIMP_CONTEXT (tool_options);
  GimpGuiConfig        *gui_config = GIMP_GUI_CONFIG (context->gimp->config);
  GimpSelectionOptions *options    = GIMP_SELECTION_OPTIONS (tool_options);
  GtkWidget            *vbox       = gimp_tool_options_gui (tool_options);
  GtkWidget            *button;

  /*  the selection operation radio buttons  */
  {
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *box;
    GList     *children;
    GList     *list;
    gint       i;

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    options->mode_box = hbox;

    label = gtk_label_new (_("Mode:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    box = gimp_prop_enum_icon_box_new (config, "operation",
                                       "gimp-selection", 0, 0);

    g_signal_connect_object (gui_config,
                             "notify::override-theme-icon-size",
                             G_CALLBACK (gimp_selection_options_style_updated),
                             box, G_CONNECT_AFTER);
    g_signal_connect_object (gui_config,
                             "notify::custom-icon-size",
                             G_CALLBACK (gimp_selection_options_style_updated),
                             box, G_CONNECT_AFTER);
    gimp_selection_options_style_updated (gui_config, NULL, box);

    gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 0);

    children = gtk_container_get_children (GTK_CONTAINER (box));

    /*  add modifier keys to the tooltips  */
    for (list = children, i = 0; list; list = list->next, i++)
      {
        GtkWidget   *button   = list->data;
        const gchar *modifier = gimp_selection_options_get_modifiers (i);
        gchar       *tooltip;

        if (! modifier)
          continue;

        tooltip = gtk_widget_get_tooltip_text (button);

        if (tooltip)
          {
            gchar *tip = g_strdup_printf ("%s  <b>%s</b>", tooltip, modifier);

            gimp_help_set_help_data_with_markup (button, tip, NULL);

            g_free (tip);
            g_free (tooltip);
          }
        else
          {
            gimp_help_set_help_data (button, modifier, NULL);
          }
      }

    /*  move GIMP_CHANNEL_OP_REPLACE to the front  */
    gtk_box_reorder_child (GTK_BOX (box),
                           GTK_WIDGET (children->next->next->data), 0);

    g_list_free (children);
  }

  /*  the antialias toggle button  */
  button = gimp_prop_check_button_new (config, "antialias", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  options->antialias_toggle = button;

  /*  the feather frame  */
  {
    GtkWidget *frame;
    GtkWidget *scale;

    /*  the feather radius scale  */
    scale = gimp_prop_spin_scale_new (config, "feather-radius",
                                      1.0, 10.0, 1);

    frame = gimp_prop_expanding_frame_new (config, "feather", NULL,
                                           scale, NULL);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  }

  return vbox;
}

static void
gimp_selection_options_style_updated (GimpGuiConfig *config,
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
