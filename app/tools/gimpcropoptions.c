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
#include "widgets/ligmawidgets-utils.h"

#include "ligmarectangleoptions.h"
#include "ligmacropoptions.h"
#include "ligmatooloptions-gui.h"

#include "ligma-intl.h"


enum
{
  PROP_LAYER_ONLY = LIGMA_RECTANGLE_OPTIONS_PROP_LAST + 1,
  PROP_ALLOW_GROWING,
  PROP_FILL_TYPE,
  PROP_DELETE_PIXELS
};


static void   ligma_crop_options_rectangle_options_iface_init (LigmaRectangleOptionsInterface *iface);

static void   ligma_crop_options_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void   ligma_crop_options_get_property (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_CODE (LigmaCropOptions, ligma_crop_options,
                         LIGMA_TYPE_TOOL_OPTIONS,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_RECTANGLE_OPTIONS,
                                                ligma_crop_options_rectangle_options_iface_init))


static void
ligma_crop_options_class_init (LigmaCropOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_crop_options_set_property;
  object_class->get_property = ligma_crop_options_get_property;

  /* The 'highlight' property is defined here because we want different
   * default values for the Crop and the Rectangle Select tools.
   */
  LIGMA_CONFIG_PROP_BOOLEAN (object_class,
                            LIGMA_RECTANGLE_OPTIONS_PROP_HIGHLIGHT,
                            "highlight",
                            _("Highlight"),
                            _("Dim everything outside selection"),
                            TRUE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class,
                           LIGMA_RECTANGLE_OPTIONS_PROP_HIGHLIGHT_OPACITY,
                           "highlight-opacity",
                           _("Highlight opacity"),
                           _("How much to dim everything outside selection"),
                           0.0, 1.0, 0.5,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_LAYER_ONLY,
                            "layer-only",
                            _("Selected layers only"),
                            _("Crop only currently selected layers"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_DELETE_PIXELS,
                            "delete-pixels",
                            _("Delete cropped pixels"),
                            _("Discard non-locked layer data that falls out of the crop region"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_ALLOW_GROWING,
                            "allow-growing",
                            _("Allow growing"),
                            _("Allow resizing canvas by dragging cropping frame "
                              "beyond image boundary"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_FILL_TYPE,
                         "fill-type",
                         _("Fill with"),
                         _("How to fill new areas created by 'Allow growing'"),
                         LIGMA_TYPE_FILL_TYPE,
                         LIGMA_FILL_TRANSPARENT,
                         LIGMA_PARAM_STATIC_STRINGS);

  ligma_rectangle_options_install_properties (object_class);
}

static void
ligma_crop_options_init (LigmaCropOptions *options)
{
}

static void
ligma_crop_options_rectangle_options_iface_init (LigmaRectangleOptionsInterface *iface)
{
}

static void
ligma_crop_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaCropOptions *options = LIGMA_CROP_OPTIONS (object);

  switch (property_id)
    {
    case PROP_LAYER_ONLY:
      options->layer_only = g_value_get_boolean (value);
      break;

    case PROP_DELETE_PIXELS:
      options->delete_pixels = g_value_get_boolean (value);
      break;

    case PROP_ALLOW_GROWING:
      options->allow_growing = g_value_get_boolean (value);
      break;

    case PROP_FILL_TYPE:
      options->fill_type = g_value_get_enum (value);
      break;

    default:
      ligma_rectangle_options_set_property (object, property_id, value, pspec);
      break;
    }
}

static void
ligma_crop_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaCropOptions *options = LIGMA_CROP_OPTIONS (object);

  switch (property_id)
    {
    case PROP_LAYER_ONLY:
      g_value_set_boolean (value, options->layer_only);
      break;

    case PROP_DELETE_PIXELS:
      g_value_set_boolean (value, options->delete_pixels);
      break;

    case PROP_ALLOW_GROWING:
      g_value_set_boolean (value, options->allow_growing);
      break;

    case PROP_FILL_TYPE:
      g_value_set_enum (value, options->fill_type);
      break;

    default:
      ligma_rectangle_options_get_property (object, property_id, value, pspec);
      break;
    }
}

GtkWidget *
ligma_crop_options_gui (LigmaToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = ligma_tool_options_gui (tool_options);
  GtkWidget *vbox_rectangle;
  GtkWidget *button;
  GtkWidget *button2;
  GtkWidget *combo;
  GtkWidget *frame;

  /*  layer toggle  */
  button = ligma_prop_check_button_new (config, "layer-only", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  /*  delete pixels toggle  */
  button2 = ligma_prop_check_button_new (config, "delete-pixels", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button2, FALSE, FALSE, 0);

  g_object_bind_property (G_OBJECT (config),  "layer-only",
                          G_OBJECT (button2), "sensitive",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_INVERT_BOOLEAN);

  /*  fill type combo  */
  combo = ligma_prop_enum_combo_box_new (config, "fill-type", 0, 0);
  ligma_int_combo_box_set_label (LIGMA_INT_COMBO_BOX (combo), _("Fill with"));

  /*  allow growing toggle/frame  */
  frame = ligma_prop_expanding_frame_new (config, "allow-growing", NULL,
                                         combo, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  /*  rectangle options  */
  vbox_rectangle = ligma_rectangle_options_gui (tool_options);
  gtk_box_pack_start (GTK_BOX (vbox), vbox_rectangle, FALSE, FALSE, 0);
  gtk_widget_show (vbox_rectangle);

  return vbox;
}
