/* The GIMP -- an image manipulation program
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
  PROP_CROP_MODE
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
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_CROP_MODE,
                                 "crop-mode", NULL,
                                 GIMP_TYPE_CROP_MODE,
                                 GIMP_CROP_MODE_CROP,
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

  if (property_id <= GIMP_RECTANGLE_OPTIONS_PROP_LAST)
    gimp_rectangle_options_set_property (object, property_id, value, pspec);
  else switch (property_id)
    {
    case PROP_LAYER_ONLY:
      options->layer_only = g_value_get_boolean (value);
      break;
    case PROP_CROP_MODE:
      options->crop_mode = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
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

  if (property_id <= GIMP_RECTANGLE_OPTIONS_PROP_LAST)
    gimp_rectangle_options_get_property (object, property_id, value, pspec);
  else switch (property_id)
    {
    case PROP_LAYER_ONLY:
      g_value_set_boolean (value, options->layer_only);
      break;
    case PROP_CROP_MODE:
      g_value_set_enum (value, options->crop_mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_crop_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_tool_options_gui (tool_options);
  GtkWidget *vbox_rectangle;
  GtkWidget *frame;
  GtkWidget *button;
  gchar     *str;

  /*  tool toggle  */
  str = g_strdup_printf (_("Tool Toggle  (%s)"),
                         gimp_get_mod_string (GDK_CONTROL_MASK));

  frame = gimp_prop_enum_radio_frame_new (config, "crop-mode",
                                          str, 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_free (str);

  /*  layer toggle  */
  button = gimp_prop_check_button_new (config, "layer-only",
                                       _("Current layer only"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  rectangle options  */
  vbox_rectangle = gimp_rectangle_options_gui (tool_options);
  gtk_box_pack_start (GTK_BOX (vbox), vbox_rectangle, FALSE, FALSE, 0);
  gtk_widget_show (vbox_rectangle);

  return vbox;
}
