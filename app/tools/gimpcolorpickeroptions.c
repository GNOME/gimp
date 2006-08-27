/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "widgets/gimpwidgets-utils.h"

#include "gimpcolorpickeroptions.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_SAMPLE_AVERAGE, /* overrides a GimpColorOptions property */
  PROP_PICK_MODE,
  PROP_ADD_TO_PALETTE,
  PROP_USE_INFO_WINDOW
};


static void   gimp_color_picker_options_set_property (GObject      *object,
                                                      guint         property_id,
                                                      const GValue *value,
                                                      GParamSpec   *pspec);
static void   gimp_color_picker_options_get_property (GObject      *object,
                                                      guint         property_id,
                                                      GValue       *value,
                                                      GParamSpec   *pspec);


G_DEFINE_TYPE (GimpColorPickerOptions, gimp_color_picker_options,
               GIMP_TYPE_COLOR_OPTIONS)


static void
gimp_color_picker_options_class_init (GimpColorPickerOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_color_picker_options_set_property;
  object_class->get_property = gimp_color_picker_options_get_property;

  /* override a GimpColorOptions property to get a different default value */
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SAMPLE_AVERAGE,
                                    "sample-average", NULL,
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_PICK_MODE,
                                 "pick-mode", NULL,
                                 GIMP_TYPE_COLOR_PICK_MODE,
                                 GIMP_COLOR_PICK_MODE_FOREGROUND,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_ADD_TO_PALETTE,
                                    "add-to-palette", NULL,
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_USE_INFO_WINDOW,
                                    "use-info-window", NULL,
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_color_picker_options_init (GimpColorPickerOptions *options)
{
}

static void
gimp_color_picker_options_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GimpColorPickerOptions *options = GIMP_COLOR_PICKER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SAMPLE_AVERAGE:
      GIMP_COLOR_OPTIONS (options)->sample_average = g_value_get_boolean (value);
      break;
    case PROP_PICK_MODE:
      options->pick_mode = g_value_get_enum (value);
      break;
    case PROP_ADD_TO_PALETTE:
      options->add_to_palette = g_value_get_boolean (value);
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
gimp_color_picker_options_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GimpColorPickerOptions *options = GIMP_COLOR_PICKER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SAMPLE_AVERAGE:
      g_value_set_boolean (value,
                           GIMP_COLOR_OPTIONS (options)->sample_average);
      break;
    case PROP_PICK_MODE:
      g_value_set_enum (value, options->pick_mode);
      break;
    case PROP_ADD_TO_PALETTE:
      g_value_set_boolean (value, options->add_to_palette);
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
gimp_color_picker_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_color_options_gui (tool_options);
  GtkWidget *button;
  GtkWidget *frame;
  gchar     *str;

  /*  the sample merged toggle button  */
  button = gimp_prop_check_button_new (config, "sample-merged",
                                       _("Sample merged"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  the pick FG/BG frame  */
  str = g_strdup_printf (_("Pick Mode  (%s)"),
                         gimp_get_mod_string (GDK_CONTROL_MASK));
  frame = gimp_prop_enum_radio_frame_new (config, "pick-mode", str, -1, -1);
  g_free (str);

  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  /*  the use_info_window toggle button  */
  str = g_strdup_printf (_("Use info window  (%s)"),
                         gimp_get_mod_string (GDK_SHIFT_MASK));
  button = gimp_prop_check_button_new (config, "use-info-window", str);
  g_free (str);

  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  return vbox;
}
