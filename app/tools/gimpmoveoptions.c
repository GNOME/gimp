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

#include "config/gimpguiconfig.h"

#include "core/gimp.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpmoveoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_MOVE_TYPE,
  PROP_MOVE_CURRENT,
  PROP_MOVE_TOOL_CHANGES_ACTIVE
};


static void   gimp_move_options_set_property  (GObject         *object,
                                               guint            property_id,
                                               const GValue    *value,
                                               GParamSpec      *pspec);
static void   gimp_move_options_get_property  (GObject         *object,
                                               guint            property_id,
                                               GValue          *value,
                                               GParamSpec      *pspec);
static void   gimp_move_options_style_updated (GimpGuiConfig   *config,
                                               GParamSpec      *pspec,
                                               GtkWidget       *box);


G_DEFINE_TYPE (GimpMoveOptions, gimp_move_options, GIMP_TYPE_TOOL_OPTIONS)


static void
gimp_move_options_class_init (GimpMoveOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_move_options_set_property;
  object_class->get_property = gimp_move_options_get_property;

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_MOVE_TYPE,
                         "move-type",
                         NULL, NULL,
                         GIMP_TYPE_TRANSFORM_TYPE,
                         GIMP_TRANSFORM_TYPE_LAYER,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_MOVE_CURRENT,
                            "move-current",
                            NULL, NULL,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_MOVE_TOOL_CHANGES_ACTIVE,
                            "move-tool-changes-active",
                            "Move tool changes active layer",
                            _("If enabled, the move tool sets the edited layer or path as active.  "
                              "This used to be the default behavior in older versions."),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_move_options_init (GimpMoveOptions *options)
{
}

static void
gimp_move_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpMoveOptions *options = GIMP_MOVE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_MOVE_TYPE:
      options->move_type = g_value_get_enum (value);
      break;
    case PROP_MOVE_CURRENT:
      options->move_current = g_value_get_boolean (value);
      break;
    case PROP_MOVE_TOOL_CHANGES_ACTIVE:
      options->move_tool_changes_active = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_move_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpMoveOptions *options = GIMP_MOVE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_MOVE_TYPE:
      g_value_set_enum (value, options->move_type);
      break;
    case PROP_MOVE_CURRENT:
      g_value_set_boolean (value, options->move_current);
      break;
    case PROP_MOVE_TOOL_CHANGES_ACTIVE:
      g_value_set_boolean (value, options->move_tool_changes_active);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_move_options_style_updated (GimpGuiConfig *config,
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
gimp_move_options_notify_type (GimpMoveOptions *move_options,
                               GParamSpec      *pspec,
                               GtkWidget       *widget)
{
  if (move_options->move_type == GIMP_TRANSFORM_TYPE_SELECTION)
    {
      if (GTK_IS_FRAME(widget))
        {
          gtk_widget_set_visible (gtk_bin_get_child (GTK_BIN (widget)), FALSE);
          gtk_frame_set_label (GTK_FRAME (widget), _ ("Move selection"));
        }
      else
        {
          gtk_widget_set_visible (widget, FALSE);
        }
    }
  else
    {
      if (GTK_IS_FRAME (widget))
        {
          const gchar *false_label = NULL;
          const gchar *true_label  = NULL;
          GtkWidget   *button;
          GSList      *group;
          gchar       *title;

          title = g_strdup_printf (_ ("Tool Toggle  (%s)"),
                                   gimp_get_mod_string (gimp_get_extend_selection_mask ()));
          gtk_frame_set_label (GTK_FRAME (widget), title);
          g_free (title);

          switch (move_options->move_type)
            {
            case GIMP_TRANSFORM_TYPE_LAYER:
              false_label = _ ("Pick a layer or guide");
              true_label  = _ ("Move the selected layers");
              break;

            case GIMP_TRANSFORM_TYPE_PATH:
              false_label = _ ("Pick a path");
              true_label  = _ ("Move the active path");
              break;

            default: /* GIMP_TRANSFORM_TYPE_SELECTION */
              g_return_if_reached ();
            }

          button = g_object_get_data (G_OBJECT (widget), "radio-button");

          group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
          gtk_button_set_label (GTK_BUTTON (group->data), true_label);

          group = g_slist_next (group);
          gtk_button_set_label (GTK_BUTTON (group->data), false_label);

          gtk_widget_set_visible (gtk_bin_get_child (GTK_BIN (widget)), TRUE);
        }
      else
        {
          gtk_widget_set_visible (widget, TRUE);

          if (!move_options->move_current)
            gtk_widget_set_sensitive (widget, TRUE);
          else
            gtk_widget_set_sensitive (widget, FALSE);
        }
    }
}

GtkWidget *
gimp_move_options_gui (GimpToolOptions *tool_options)
{
  GObject         *config     = G_OBJECT (tool_options);
  GimpContext     *context    = GIMP_CONTEXT (tool_options);
  GimpGuiConfig   *gui_config = GIMP_GUI_CONFIG (context->gimp->config);
  GimpMoveOptions *options     = GIMP_MOVE_OPTIONS (tool_options);
  GtkWidget       *vbox        = gimp_tool_options_gui (tool_options);
  GtkWidget       *hbox;
  GtkWidget       *box;
  GtkWidget       *label;
  GtkWidget       *frame;
  GtkWidget       *button2;
  gchar           *title;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  options->type_box = hbox;

  label = gtk_label_new (_("Move:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  box = gimp_prop_enum_icon_box_new (config, "move-type", "gimp",
                                     GIMP_TRANSFORM_TYPE_LAYER,
                                     GIMP_TRANSFORM_TYPE_PATH);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 0);
  gtk_widget_show (box);

  g_signal_connect_object (gui_config,
                           "notify::override-theme-icon-size",
                           G_CALLBACK (gimp_move_options_style_updated),
                           box, G_CONNECT_AFTER);
  g_signal_connect_object (gui_config,
                           "notify::custom-icon-size",
                           G_CALLBACK (gimp_move_options_style_updated),
                           box, G_CONNECT_AFTER);
  gimp_move_options_style_updated (gui_config, NULL, box);

  /*  tool toggle  */
  title =
    g_strdup_printf (_("Tool Toggle  (%s)"),
                     gimp_get_mod_string (gimp_get_extend_selection_mask ()));

  frame = gimp_prop_boolean_radio_frame_new (config, "move-current",
                                             title, "true", "false");

  gimp_move_options_notify_type (GIMP_MOVE_OPTIONS (config), NULL, frame);

  g_signal_connect_object (config, "notify::move-type",
                           G_CALLBACK (gimp_move_options_notify_type),
                           frame, 0);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  button2 = gimp_prop_check_button_new (config, "move-tool-changes-active", NULL);

  if (!options->move_current)
    gtk_widget_set_sensitive (button2, TRUE);
  else
    gtk_widget_set_sensitive (button2, FALSE);

  gimp_move_options_notify_type (GIMP_MOVE_OPTIONS (config), NULL, button2);

  g_signal_connect_object (config, "notify::move-type",
                           G_CALLBACK (gimp_move_options_notify_type), button2, 0);

  g_signal_connect_object (config, "notify::move-current",
                           G_CALLBACK (gimp_move_options_notify_type), button2, 0);

  gtk_box_pack_start (GTK_BOX (vbox), button2, FALSE, FALSE, 0);

  g_free (title);

  return vbox;
}
