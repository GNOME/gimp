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

#include "gimpalignoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_ALIGN_TYPE
};


static void   gimp_align_options_set_property (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec);
static void   gimp_align_options_get_property (GObject      *object,
                                               guint         property_id,
                                               GValue       *value,
                                               GParamSpec   *pspec);


G_DEFINE_TYPE (GimpAlignOptions, gimp_align_options, GIMP_TYPE_TOOL_OPTIONS)


static void
gimp_align_options_class_init (GimpAlignOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_align_options_set_property;
  object_class->get_property = gimp_align_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_ALIGN_TYPE,
                                 "align-type", NULL,
                                 GIMP_TYPE_TRANSFORM_TYPE,
                                 GIMP_TRANSFORM_TYPE_LAYER,
                                 GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_align_options_init (GimpAlignOptions *options)
{
}

static void
gimp_align_options_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpAlignOptions *options = GIMP_ALIGN_OPTIONS (object);

  switch (property_id)
    {
    case PROP_ALIGN_TYPE:
      options->align_type = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_align_options_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpAlignOptions *options = GIMP_ALIGN_OPTIONS (object);

  switch (property_id)
    {
    case PROP_ALIGN_TYPE:
      g_value_set_enum (value, options->align_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_align_options_gui (GimpToolOptions *tool_options)
{
  GtkWidget *vbox;
  GtkWidget *controls_container;

#if 0
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *hbox;
  GtkWidget *label;
#endif

  vbox = gimp_tool_options_gui (tool_options);

#if 0
  hbox = gimp_prop_enum_stock_box_new (config, "align-type", "gimp", 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Affect:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (hbox), label, 0);
  gtk_widget_show (label);
#endif

  controls_container = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), controls_container, FALSE, FALSE, 0);
  gtk_widget_show (controls_container);
  g_object_set_data (G_OBJECT (tool_options),
                     "controls-container", controls_container);

  return vbox;
}
