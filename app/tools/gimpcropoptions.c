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

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimprectangleoptions.h"
#include "gimpcropoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_LAYER_ONLY = GIMP_RECTANGLE_OPTIONS_PROP_LAST + 1,
  PROP_ALLOW_GROWING,
  PROP_FILL_TYPE
};


static void   gimp_crop_options_rectangle_options_iface_init (GimpRectangleOptionsInterface *iface);

static void   gimp_crop_options_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void   gimp_crop_options_get_property (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_CODE (GimpCropOptions, gimp_crop_options,
                         GIMP_TYPE_TOOL_OPTIONS,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_RECTANGLE_OPTIONS,
                                                gimp_crop_options_rectangle_options_iface_init))


static void
gimp_crop_options_class_init (GimpCropOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_crop_options_set_property;
  object_class->get_property = gimp_crop_options_get_property;

  /* The 'highlight' property is defined here because we want different
   * default values for the Crop and the Rectangle Select tools.
   */
  GIMP_CONFIG_PROP_BOOLEAN (object_class,
                            GIMP_RECTANGLE_OPTIONS_PROP_HIGHLIGHT,
                            "highlight",
                            _("Highlight"),
                            _("Dim everything outside selection"),
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class,
                           GIMP_RECTANGLE_OPTIONS_PROP_HIGHLIGHT_OPACITY,
                           "highlight-opacity",
                           _("Highlight opacity"),
                           _("How much to dim everything outside selection"),
                           0.0, 1.0, 0.5,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_LAYER_ONLY,
                            "layer-only",
                            _("Current layer only"),
                            _("Crop only currently selected layer"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_ALLOW_GROWING,
                            "allow-growing",
                            _("Allow growing"),
                            _("Allow resizing canvas by dragging cropping frame "
                              "beyond image boundary"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_FILL_TYPE,
                         "fill-type",
                         _("Fill with"),
                         _("How to fill new areas created by 'Allow growing'"),
                         GIMP_TYPE_FILL_TYPE,
                         GIMP_FILL_TRANSPARENT,
                         GIMP_PARAM_STATIC_STRINGS);

  gimp_rectangle_options_install_properties (object_class);
}

static void
gimp_crop_options_init (GimpCropOptions *options)
{
}

static void
gimp_crop_options_rectangle_options_iface_init (GimpRectangleOptionsInterface *iface)
{
}

static void
gimp_crop_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpCropOptions *options = GIMP_CROP_OPTIONS (object);

  switch (property_id)
    {
    case PROP_LAYER_ONLY:
      options->layer_only = g_value_get_boolean (value);
      break;

    case PROP_ALLOW_GROWING:
      options->allow_growing = g_value_get_boolean (value);
      break;

    case PROP_FILL_TYPE:
      options->fill_type = g_value_get_enum (value);
      break;

    default:
      gimp_rectangle_options_set_property (object, property_id, value, pspec);
      break;
    }
}

static void
gimp_crop_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpCropOptions *options = GIMP_CROP_OPTIONS (object);

  switch (property_id)
    {
    case PROP_LAYER_ONLY:
      g_value_set_boolean (value, options->layer_only);
      break;

    case PROP_ALLOW_GROWING:
      g_value_set_boolean (value, options->allow_growing);
      break;

    case PROP_FILL_TYPE:
      g_value_set_enum (value, options->fill_type);
      break;

    default:
      gimp_rectangle_options_get_property (object, property_id, value, pspec);
      break;
    }
}

GtkWidget *
gimp_crop_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_tool_options_gui (tool_options);
  GtkWidget *vbox_rectangle;
  GtkWidget *button;
  GtkWidget *combo;
  GtkWidget *frame;

  /*  layer toggle  */
  button = gimp_prop_check_button_new (config, "layer-only", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  fill type combo  */
  combo = gimp_prop_enum_combo_box_new (config, "fill-type", 0, 0);
  gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo), _("Fill with"));

  /*  allow growing toggle/frame  */
  frame = gimp_prop_expanding_frame_new (config, "allow-growing", NULL,
                                         combo, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  rectangle options  */
  vbox_rectangle = gimp_rectangle_options_gui (tool_options);
  gtk_box_pack_start (GTK_BOX (vbox), vbox_rectangle, FALSE, FALSE, 0);
  gtk_widget_show (vbox_rectangle);

  return vbox;
}
