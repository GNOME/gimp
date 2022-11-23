/* LIGMA - The GNU Image Manipulation Program
 *
 * ligmacageoptions.c
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
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

#include "ligmacageoptions.h"
#include "ligmatooloptions-gui.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_CAGE_MODE,
  PROP_FILL_PLAIN_COLOR
};


static void   ligma_cage_options_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void   ligma_cage_options_get_property (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);


G_DEFINE_TYPE (LigmaCageOptions, ligma_cage_options,
               LIGMA_TYPE_TOOL_OPTIONS)

#define parent_class ligma_cage_options_parent_class


static void
ligma_cage_options_class_init (LigmaCageOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_cage_options_set_property;
  object_class->get_property = ligma_cage_options_get_property;

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_CAGE_MODE,
                         "cage-mode",
                         NULL, NULL,
                         LIGMA_TYPE_CAGE_MODE,
                         LIGMA_CAGE_MODE_CAGE_CHANGE,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_FILL_PLAIN_COLOR,
                            "fill-plain-color",
                            _("Fill the original position\n"
                              "of the cage with a color"),
                            NULL,
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_cage_options_init (LigmaCageOptions *options)
{
}

static void
ligma_cage_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaCageOptions *options = LIGMA_CAGE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_CAGE_MODE:
      options->cage_mode = g_value_get_enum (value);
      break;
    case PROP_FILL_PLAIN_COLOR:
      options->fill_plain_color = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_cage_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaCageOptions *options = LIGMA_CAGE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_CAGE_MODE:
      g_value_set_enum (value, options->cage_mode);
      break;
    case PROP_FILL_PLAIN_COLOR:
      g_value_set_boolean (value, options->fill_plain_color);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
ligma_cage_options_gui (LigmaToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = ligma_tool_options_gui (tool_options);
  GtkWidget *mode;
  GtkWidget *button;

  mode = ligma_prop_enum_radio_box_new (config, "cage-mode", 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), mode, FALSE, FALSE, 0);

  button = ligma_prop_check_button_new (config, "fill-plain-color", NULL);
  gtk_box_pack_start (GTK_BOX (vbox),  button, FALSE, FALSE, 0);

  return vbox;
}
