/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmavectoroptions.c
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

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "ligmavectoroptions.h"
#include "ligmatooloptions-gui.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_VECTORS_EDIT_MODE,
  PROP_VECTORS_POLYGONAL
};


static void   ligma_vector_options_set_property (GObject      *object,
                                                guint         property_id,
                                                const GValue *value,
                                                GParamSpec   *pspec);
static void   ligma_vector_options_get_property (GObject      *object,
                                                guint         property_id,
                                                GValue       *value,
                                                GParamSpec   *pspec);


G_DEFINE_TYPE (LigmaVectorOptions, ligma_vector_options, LIGMA_TYPE_TOOL_OPTIONS)


static void
ligma_vector_options_class_init (LigmaVectorOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_vector_options_set_property;
  object_class->get_property = ligma_vector_options_get_property;

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_VECTORS_EDIT_MODE,
                         "vectors-edit-mode",
                         _("Edit Mode"),
                         NULL,
                         LIGMA_TYPE_VECTOR_MODE,
                         LIGMA_VECTOR_MODE_DESIGN,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_VECTORS_POLYGONAL,
                            "vectors-polygonal",
                            _("Polygonal"),
                            _("Restrict editing to polygons"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_vector_options_init (LigmaVectorOptions *options)
{
}

static void
ligma_vector_options_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  LigmaVectorOptions *options = LIGMA_VECTOR_OPTIONS (object);

  switch (property_id)
    {
    case PROP_VECTORS_EDIT_MODE:
      options->edit_mode = g_value_get_enum (value);
      break;
    case PROP_VECTORS_POLYGONAL:
      options->polygonal = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static void
ligma_vector_options_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  LigmaVectorOptions *options = LIGMA_VECTOR_OPTIONS (object);

  switch (property_id)
    {
    case PROP_VECTORS_EDIT_MODE:
      g_value_set_enum (value, options->edit_mode);
      break;
    case PROP_VECTORS_POLYGONAL:
      g_value_set_boolean (value, options->polygonal);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
button_append_modifier (GtkWidget       *button,
                        GdkModifierType  modifiers)
{
  gchar *str = g_strdup_printf ("%s (%s)",
                                gtk_button_get_label (GTK_BUTTON (button)),
                                ligma_get_mod_string (modifiers));

  gtk_button_set_label (GTK_BUTTON (button), str);
  g_free (str);
}

GtkWidget *
ligma_vector_options_gui (LigmaToolOptions *tool_options)
{
  GObject           *config  = G_OBJECT (tool_options);
  LigmaVectorOptions *options = LIGMA_VECTOR_OPTIONS (tool_options);
  GtkWidget         *vbox    = ligma_tool_options_gui (tool_options);
  GtkWidget         *frame;
  GtkWidget         *button;
  gchar             *str;

  /*  tool toggle  */
  frame = ligma_prop_enum_radio_frame_new (config, "vectors-edit-mode", NULL,
                                          0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  button = g_object_get_data (G_OBJECT (frame), "radio-button");

  if (GTK_IS_RADIO_BUTTON (button))
    {
      GSList *list = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));

      /* LIGMA_VECTOR_MODE_MOVE  */
      button_append_modifier (list->data, GDK_MOD1_MASK);

      if (list->next)   /* LIGMA_VECTOR_MODE_EDIT  */
        button_append_modifier (list->next->data,
                                ligma_get_toggle_behavior_mask ());
    }

  button = ligma_prop_check_button_new (config, "vectors-polygonal", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  str = g_strdup_printf (_("Path to Selection\n"
                           "%s  Add\n"
                           "%s  Subtract\n"
                           "%s  Intersect"),
                         ligma_get_mod_string (ligma_get_extend_selection_mask ()),
                         ligma_get_mod_string (ligma_get_modify_selection_mask ()),
                         ligma_get_mod_string (ligma_get_extend_selection_mask () |
                                              ligma_get_modify_selection_mask ()));

  button = ligma_button_new ();
  /*  Create a selection from the current path  */
  gtk_button_set_label (GTK_BUTTON (button), _("Selection from Path"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (button, FALSE);
  ligma_help_set_help_data (button, str, LIGMA_HELP_PATH_SELECTION_REPLACE);
  gtk_widget_show (button);

  g_free (str);

  options->to_selection_button = button;

  button = gtk_button_new_with_label (_("Fill Path"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (button, FALSE);
  ligma_help_set_help_data (button, NULL, LIGMA_HELP_PATH_FILL);
  gtk_widget_show (button);

  options->fill_button = button;

  button = gtk_button_new_with_label (_("Stroke Path"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (button, FALSE);
  ligma_help_set_help_data (button, NULL, LIGMA_HELP_PATH_STROKE);
  gtk_widget_show (button);

  options->stroke_button = button;

  return vbox;
}
