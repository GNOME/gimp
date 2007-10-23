/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimprectangleoptions.h"
#include "gimpcropoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_LAYER_ONLY = GIMP_RECTANGLE_OPTIONS_PROP_LAST + 1,
  PROP_ALLOW_GROWING
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

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_LAYER_ONLY,
                                    "layer-only", NULL,
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_ALLOW_GROWING,
                                    "allow-growing", NULL,
                                    FALSE,
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
  switch (property_id)
    {
    case PROP_LAYER_ONLY:
      GIMP_CROP_OPTIONS (object)->layer_only = g_value_get_boolean (value);
      break;

    case PROP_ALLOW_GROWING:
      GIMP_CROP_OPTIONS (object)->allow_growing = g_value_get_boolean (value);
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
  switch (property_id)
    {
    case PROP_LAYER_ONLY:
      g_value_set_boolean (value, GIMP_CROP_OPTIONS (object)->layer_only);
      break;

    case PROP_ALLOW_GROWING:
      g_value_set_boolean (value, GIMP_CROP_OPTIONS (object)->allow_growing);
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

  /*  layer toggle  */
  button = gimp_prop_check_button_new (config, "layer-only",
                                       _("Current layer only"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  allow growing toggle  */
  button = gimp_prop_check_button_new (config, "allow-growing",
                                       _("Allow growing"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  rectangle options  */
  vbox_rectangle = gimp_rectangle_options_gui (tool_options);
  gtk_box_pack_start (GTK_BOX (vbox), vbox_rectangle, FALSE, FALSE, 0);
  gtk_widget_show (vbox_rectangle);

  return vbox;
}
