/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "ligmacolorpickeroptions.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_SAMPLE_AVERAGE, /* overrides a LigmaColorOptions property */
  PROP_PICK_TARGET,
  PROP_USE_INFO_WINDOW,
  PROP_FRAME1_MODE,
  PROP_FRAME2_MODE
};


static void   ligma_color_picker_options_set_property (GObject      *object,
                                                      guint         property_id,
                                                      const GValue *value,
                                                      GParamSpec   *pspec);
static void   ligma_color_picker_options_get_property (GObject      *object,
                                                      guint         property_id,
                                                      GValue       *value,
                                                      GParamSpec   *pspec);


G_DEFINE_TYPE (LigmaColorPickerOptions, ligma_color_picker_options,
               LIGMA_TYPE_COLOR_OPTIONS)


static void
ligma_color_picker_options_class_init (LigmaColorPickerOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_color_picker_options_set_property;
  object_class->get_property = ligma_color_picker_options_get_property;

  /* override a LigmaColorOptions property to get a different default value */
  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_SAMPLE_AVERAGE,
                            "sample-average",
                            _("Sample average"),
                            _("Use averaged color value from "
                              "nearby pixels"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_PICK_TARGET,
                         "pick-target",
                         _("Pick Target"),
                         _("Choose what the color picker will do"),
                         LIGMA_TYPE_COLOR_PICK_TARGET,
                         LIGMA_COLOR_PICK_TARGET_FOREGROUND,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_INFO_WINDOW,
                            "use-info-window",
                            _("Use info window"),
                            _("Open a floating dialog to view picked "
                              "color values in various color models"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_FRAME1_MODE,
                         "frame1-mode",
                         "Frame 1 Mode", NULL,
                         LIGMA_TYPE_COLOR_PICK_MODE,
                         LIGMA_COLOR_PICK_MODE_PIXEL,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_FRAME2_MODE,
                         "frame2-mode",
                         "Frame 2 Mode", NULL,
                         LIGMA_TYPE_COLOR_PICK_MODE,
                         LIGMA_COLOR_PICK_MODE_RGB_PERCENT,
                         LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_color_picker_options_init (LigmaColorPickerOptions *options)
{
}

static void
ligma_color_picker_options_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  LigmaColorPickerOptions *options = LIGMA_COLOR_PICKER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SAMPLE_AVERAGE:
      LIGMA_COLOR_OPTIONS (options)->sample_average = g_value_get_boolean (value);
      break;
    case PROP_PICK_TARGET:
      options->pick_target = g_value_get_enum (value);
      break;
    case PROP_USE_INFO_WINDOW:
      options->use_info_window = g_value_get_boolean (value);
      break;
    case PROP_FRAME1_MODE:
      options->frame1_mode = g_value_get_enum (value);
      break;
    case PROP_FRAME2_MODE:
      options->frame2_mode = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_color_picker_options_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  LigmaColorPickerOptions *options = LIGMA_COLOR_PICKER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SAMPLE_AVERAGE:
      g_value_set_boolean (value,
                           LIGMA_COLOR_OPTIONS (options)->sample_average);
      break;
    case PROP_PICK_TARGET:
      g_value_set_enum (value, options->pick_target);
      break;
    case PROP_USE_INFO_WINDOW:
      g_value_set_boolean (value, options->use_info_window);
      break;
    case PROP_FRAME1_MODE:
      g_value_set_enum (value, options->frame1_mode);
      break;
    case PROP_FRAME2_MODE:
      g_value_set_enum (value, options->frame2_mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
ligma_color_picker_options_gui (LigmaToolOptions *tool_options)
{
  GObject         *config = G_OBJECT (tool_options);
  GtkWidget       *vbox   = ligma_color_options_gui (tool_options);
  GtkWidget       *button;
  GtkWidget       *frame;
  gchar           *str;
  GdkModifierType  extend_mask = ligma_get_extend_selection_mask ();
  GdkModifierType  toggle_mask = ligma_get_toggle_behavior_mask ();

  /*  the pick FG/BG frame  */
  str = g_strdup_printf (_("Pick Target  (%s)"),
                         ligma_get_mod_string (toggle_mask));
  frame = ligma_prop_enum_radio_frame_new (config, "pick-target", str, -1, -1);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  g_free (str);

  /*  the use_info_window toggle button  */
  str = g_strdup_printf (_("Use info window  (%s)"),
                         ligma_get_mod_string (extend_mask));
  button = ligma_prop_check_button_new (config, "use-info-window", str);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_free (str);

  return vbox;
}
