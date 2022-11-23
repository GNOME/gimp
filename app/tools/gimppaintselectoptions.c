/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "widgets/ligmapropwidgets.h"
#include "widgets/ligmawidgets-constructors.h"
#include "widgets/ligmawidgets-utils.h"

#include "core/ligmatooloptions.h"
#include "ligmapaintselectoptions.h"
#include "ligmatooloptions-gui.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_MODE,
  PROP_STROKE_WIDTH,
  PROP_SHOW_SCRIBBLES,
};


static void   ligma_paint_select_options_set_property      (GObject      *object,
                                                           guint         property_id,
                                                           const GValue *value,
                                                           GParamSpec   *pspec);
static void   ligma_paint_select_options_get_property      (GObject      *object,
                                                           guint         property_id,
                                                           GValue       *value,
                                                           GParamSpec   *pspec);


G_DEFINE_TYPE (LigmaPaintSelectOptions, ligma_paint_select_options,
               LIGMA_TYPE_TOOL_OPTIONS)


static void
ligma_paint_select_options_class_init (LigmaPaintSelectOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_paint_select_options_set_property;
  object_class->get_property = ligma_paint_select_options_get_property;

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_MODE,
                         "mode",
                         _("Mode"),
                         _("Paint over areas to mark pixels for "
                           "inclusion or exclusion from selection"),
                         LIGMA_TYPE_PAINT_SELECT_MODE,
                         LIGMA_PAINT_SELECT_MODE_ADD,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_INT  (object_class, PROP_STROKE_WIDTH,
                         "stroke-width",
                         _("Stroke width"),
                         _("Size of the brush used for refinements"),
                         1, 6000, 50,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN  (object_class, PROP_SHOW_SCRIBBLES,
                         "show-scribbles",
                         _("Show scribbles"),
                         _("Show scribbles"),
                         FALSE,
                         LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_paint_select_options_init (LigmaPaintSelectOptions *options)
{
}

static void
ligma_paint_select_options_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  LigmaPaintSelectOptions *options = LIGMA_PAINT_SELECT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_MODE:
      options->mode = g_value_get_enum (value);
      break;

    case PROP_STROKE_WIDTH:
      options->stroke_width = g_value_get_int (value);
      break;

    case PROP_SHOW_SCRIBBLES:
      options->show_scribbles = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_paint_select_options_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  LigmaPaintSelectOptions *options = LIGMA_PAINT_SELECT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_MODE:
      g_value_set_enum (value, options->mode);
      break;

    case PROP_STROKE_WIDTH:
      g_value_set_int (value, options->stroke_width);
      break;

    case PROP_SHOW_SCRIBBLES:
      g_value_set_boolean (value, options->show_scribbles);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_paint_select_options_reset_stroke_width (GtkWidget       *button,
                                              LigmaToolOptions *tool_options)
{
  g_object_set (tool_options, "stroke-width", 10, NULL);
}

GtkWidget *
ligma_paint_select_options_gui (LigmaToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = ligma_tool_options_gui (tool_options);
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *scale;

  frame = ligma_prop_enum_radio_frame_new (config, "mode", NULL,
                                          0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* stroke width */
  scale = ligma_prop_spin_scale_new (config, "stroke-width",
                                    1.0, 10.0, 2);
  ligma_spin_scale_set_scale_limits (LIGMA_SPIN_SCALE (scale), 1.0, 1000.0);
  ligma_spin_scale_set_gamma (LIGMA_SPIN_SCALE (scale), 1.7);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);

  button = ligma_icon_button_new (LIGMA_ICON_RESET, NULL);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_image_set_from_icon_name (GTK_IMAGE (gtk_bin_get_child (GTK_BIN (button))),
                                LIGMA_ICON_RESET, GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (ligma_paint_select_options_reset_stroke_width),
                    tool_options);

  ligma_help_set_help_data (button,
                           _("Reset stroke width native size"), NULL);

  /* show scribbles */
  button = ligma_prop_check_button_new (config, "show-scribbles", "Show scribbles");
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  return vbox;
}
