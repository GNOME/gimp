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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpconfig-params.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpcropoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_LAYER_ONLY,
  PROP_ALLOW_ENLARGE,
  PROP_KEEP_ASPECT,
  PROP_CROP_MODE
};


static void   gimp_crop_options_class_init   (GimpCropOptionsClass *klass);

static void   gimp_crop_options_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void   gimp_crop_options_get_property (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);


static GimpToolOptionsClass *parent_class = NULL;


GType
gimp_crop_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpCropOptionsClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_crop_options_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpCropOptions),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) NULL
      };

      type = g_type_register_static (GIMP_TYPE_TOOL_OPTIONS,
                                     "GimpCropOptions",
                                     &info, 0);
    }

  return type;
}

static void
gimp_crop_options_class_init (GimpCropOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = gimp_crop_options_set_property;
  object_class->get_property = gimp_crop_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_LAYER_ONLY,
                                    "layer-only", NULL,
                                    FALSE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_ALLOW_ENLARGE,
                                    "allow-enlarge", NULL,
                                    FALSE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_KEEP_ASPECT,
                                    "keep-aspect", NULL,
                                    FALSE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_CROP_MODE,
                                 "crop-mode", NULL,
                                 GIMP_TYPE_CROP_MODE,
                                 GIMP_CROP_MODE_CROP,
                                 0);
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
    case PROP_ALLOW_ENLARGE:
      options->allow_enlarge = g_value_get_boolean (value);
      break;
    case PROP_KEEP_ASPECT:
      options->keep_aspect = g_value_get_boolean (value);
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

  switch (property_id)
    {
    case PROP_LAYER_ONLY:
      g_value_set_boolean (value, options->layer_only);
      break;
    case PROP_ALLOW_ENLARGE:
      g_value_set_boolean (value, options->allow_enlarge);
      break;
    case PROP_KEEP_ASPECT:
      g_value_set_boolean (value, options->keep_aspect);
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
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *button;
  gchar     *str;

  vbox = gimp_tool_options_gui (tool_options);

  /*  tool toggle  */
  str = g_strdup_printf (_("Tool Toggle  %s"),
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

  /*  enlarge toggle  */
  str = g_strdup_printf (_("Allow enlarging  %s"),
                         gimp_get_mod_string (GDK_MOD1_MASK));

  button = gimp_prop_check_button_new (config, "allow-enlarge", str);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_free (str);

  /*  layer toggle  */
  str = g_strdup_printf (_("Keep aspect ratio  %s"),
                         gimp_get_mod_string (GDK_SHIFT_MASK));
  button = gimp_prop_check_button_new (config, "keep-aspect", str);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_free (str);

  return vbox;
}
