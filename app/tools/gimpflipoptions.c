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

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimptoolinfo.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpflipoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_FLIP_TYPE
};


static void   gimp_flip_options_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void   gimp_flip_options_get_property (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);


G_DEFINE_TYPE (GimpFlipOptions, gimp_flip_options,
               GIMP_TYPE_TRANSFORM_OPTIONS)


static void
gimp_flip_options_class_init (GimpFlipOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_flip_options_set_property;
  object_class->get_property = gimp_flip_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_FLIP_TYPE,
                                 "flip-type", NULL,
                                 GIMP_TYPE_ORIENTATION_TYPE,
                                 GIMP_ORIENTATION_HORIZONTAL,
                                 GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_flip_options_init (GimpFlipOptions *options)
{
}

static void
gimp_flip_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpFlipOptions *options = GIMP_FLIP_OPTIONS (object);

  switch (property_id)
    {
    case PROP_FLIP_TYPE:
      options->flip_type = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_flip_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpFlipOptions *options = GIMP_FLIP_OPTIONS (object);

  switch (property_id)
    {
    case PROP_FLIP_TYPE:
      g_value_set_enum (value, options->flip_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_flip_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_tool_options_gui (tool_options);
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *frame;
  gchar     *str;

  hbox = gimp_prop_enum_stock_box_new (config, "type", "gimp", 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Affect:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (hbox), label, 0);
  gtk_widget_show (label);

  /*  tool toggle  */
  str = g_strdup_printf (_("Flip Type  (%s)"),
                         gimp_get_mod_string (GDK_CONTROL_MASK));

  frame = gimp_prop_enum_radio_frame_new (config, "flip-type",
                                          str,
                                          GIMP_ORIENTATION_HORIZONTAL,
                                          GIMP_ORIENTATION_VERTICAL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_free (str);

  return vbox;
}
