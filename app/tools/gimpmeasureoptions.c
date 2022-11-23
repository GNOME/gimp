/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmameasuretool.c
 * Copyright (C) 1999 Sven Neumann <sven@ligma.org>
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

#include "widgets/ligmawidgets-utils.h"

#include "ligmameasureoptions.h"
#include "ligmatooloptions-gui.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_ORIENTATION,
  PROP_USE_INFO_WINDOW
};


static void   ligma_measure_options_set_property (GObject      *object,
                                                 guint         property_id,
                                                 const GValue *value,
                                                 GParamSpec   *pspec);
static void   ligma_measure_options_get_property (GObject      *object,
                                                 guint         property_id,
                                                 GValue       *value,
                                                 GParamSpec   *pspec);


G_DEFINE_TYPE (LigmaMeasureOptions, ligma_measure_options,
               LIGMA_TYPE_TRANSFORM_OPTIONS)


static void
ligma_measure_options_class_init (LigmaMeasureOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_measure_options_set_property;
  object_class->get_property = ligma_measure_options_get_property;

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_ORIENTATION,
                         "orientation",
                         _("Orientation"),
                         _("Orientation against which the angle is measured"),
                         LIGMA_TYPE_COMPASS_ORIENTATION,
                         LIGMA_COMPASS_ORIENTATION_AUTO,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_INFO_WINDOW,
                            "use-info-window",
                            _("Use info window"),
                            _("Open a floating dialog to view details "
                              "about measurements"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_measure_options_init (LigmaMeasureOptions *options)
{
}

static void
ligma_measure_options_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  LigmaMeasureOptions *options = LIGMA_MEASURE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_ORIENTATION:
      options->orientation = g_value_get_enum (value);
      break;
    case PROP_USE_INFO_WINDOW:
      options->use_info_window = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_measure_options_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  LigmaMeasureOptions *options = LIGMA_MEASURE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, options->orientation);
      break;
    case PROP_USE_INFO_WINDOW:
      g_value_set_boolean (value, options->use_info_window);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
ligma_measure_options_gui (LigmaToolOptions *tool_options)
{
  GObject            *config  = G_OBJECT (tool_options);
  LigmaMeasureOptions *options = LIGMA_MEASURE_OPTIONS (tool_options);
  GtkWidget          *vbox    = ligma_tool_options_gui (tool_options);
  GtkWidget          *frame;
  GtkWidget          *button;
  GtkWidget          *vbox2;
  gchar              *str;
  GdkModifierType     toggle_mask = ligma_get_toggle_behavior_mask ();

  /*  the orientation frame  */
  str = g_strdup_printf (_("Orientation  (%s)"),
                         ligma_get_mod_string (toggle_mask));
  frame = ligma_prop_enum_radio_frame_new (config, "orientation", str, -1, -1);
  g_free (str);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /*  the use_info_window toggle button  */
  button = ligma_prop_check_button_new (config, "use-info-window", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  the straighten frame  */
  frame = ligma_frame_new (_("Straighten"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  the transform options  */
  vbox2 = ligma_transform_options_gui (tool_options, FALSE, TRUE, TRUE);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  /*  the straighten button  */
  button = gtk_button_new_with_label (_("Straighten"));
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (button, FALSE);
  ligma_help_set_help_data (button,
                           _("Rotate the active layer, selection or path "
                             "by the measured angle"),
                           NULL);
  gtk_widget_show (button);

  options->straighten_button = button;

  return vbox;
}
