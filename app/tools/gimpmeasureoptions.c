/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmeasuretool.c
 * Copyright (C) 1999 Sven Neumann <sven@gimp.org>
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

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpmeasureoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_ORIENTATION,
  PROP_USE_INFO_WINDOW
};


static void   gimp_measure_options_set_property (GObject      *object,
                                                 guint         property_id,
                                                 const GValue *value,
                                                 GParamSpec   *pspec);
static void   gimp_measure_options_get_property (GObject      *object,
                                                 guint         property_id,
                                                 GValue       *value,
                                                 GParamSpec   *pspec);


G_DEFINE_TYPE (GimpMeasureOptions, gimp_measure_options,
               GIMP_TYPE_TRANSFORM_OPTIONS)


static void
gimp_measure_options_class_init (GimpMeasureOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_measure_options_set_property;
  object_class->get_property = gimp_measure_options_get_property;

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_ORIENTATION,
                         "orientation",
                         _("Orientation"),
                         _("Orientation against which the angle is measured"),
                         GIMP_TYPE_COMPASS_ORIENTATION,
                         GIMP_COMPASS_ORIENTATION_AUTO,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_INFO_WINDOW,
                            "use-info-window",
                            _("Use info window"),
                            _("Open a floating dialog to view details "
                              "about measurements"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_measure_options_init (GimpMeasureOptions *options)
{
}

static void
gimp_measure_options_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpMeasureOptions *options = GIMP_MEASURE_OPTIONS (object);

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
gimp_measure_options_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpMeasureOptions *options = GIMP_MEASURE_OPTIONS (object);

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
gimp_measure_options_gui (GimpToolOptions *tool_options)
{
  GObject            *config  = G_OBJECT (tool_options);
  GimpMeasureOptions *options = GIMP_MEASURE_OPTIONS (tool_options);
  GtkWidget          *vbox    = gimp_tool_options_gui (tool_options);
  GtkWidget          *frame;
  GtkWidget          *button;
  GtkWidget          *vbox2;
  gchar              *str;
  GdkModifierType     toggle_mask = gimp_get_toggle_behavior_mask ();

  /*  the orientation frame  */
  str = g_strdup_printf (_("Orientation  (%s)"),
                         gimp_get_mod_string (toggle_mask));
  frame = gimp_prop_enum_radio_frame_new (config, "orientation", str, -1, -1);
  g_free (str);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /*  the use_info_window toggle button  */
  button = gimp_prop_check_button_new (config, "use-info-window", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  the straighten frame  */
  frame = gimp_frame_new (_("Straighten"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  the transform options  */
  vbox2 = gimp_transform_options_gui (tool_options, FALSE, TRUE, TRUE);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  /*  the straighten button  */
  button = gtk_button_new_with_label (_("Straighten"));
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (button, FALSE);
  gimp_help_set_help_data (button,
                           _("Rotate the active layer, selection or path "
                             "by the measured angle"),
                           NULL);
  gtk_widget_show (button);

  options->straighten_button = button;

  return vbox;
}
