/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "gimpcolorpickeroptions.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_SAMPLE_MERGED,
  PROP_SAMPLE_AVERAGE,
  PROP_AVERAGE_RADIUS,
  PROP_UPDATE_ACTIVE
};


static void   gimp_color_picker_options_init       (GimpColorPickerOptions      *options);
static void   gimp_color_picker_options_class_init (GimpColorPickerOptionsClass *options_class);

static void   gimp_color_picker_options_set_property (GObject      *object,
                                                      guint         property_id,
                                                      const GValue *value,
                                                      GParamSpec   *pspec);
static void   gimp_color_picker_options_get_property (GObject      *object,
                                                      guint         property_id,
                                                      GValue       *value,
                                                      GParamSpec   *pspec);


static GimpToolOptionsClass *parent_class = NULL;


GType
gimp_color_picker_options_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpColorPickerOptionsClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_color_picker_options_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpColorPickerOptions),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_color_picker_options_init,
      };

      type = g_type_register_static (GIMP_TYPE_TOOL_OPTIONS,
                                     "GimpColorPickerOptions",
                                     &info, 0);
    }

  return type;
}

static void 
gimp_color_picker_options_class_init (GimpColorPickerOptionsClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = gimp_color_picker_options_set_property;
  object_class->get_property = gimp_color_picker_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SAMPLE_MERGED,
                                    "sample-merged", NULL,
                                    FALSE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SAMPLE_AVERAGE,
                                    "sample-average", NULL,
                                    FALSE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_AVERAGE_RADIUS,
                                   "average-radius", NULL,
                                   1.0, 15.0, 1.0,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_UPDATE_ACTIVE,
                                    "update-active", NULL,
                                    TRUE,
                                    0);
}

static void
gimp_color_picker_options_init (GimpColorPickerOptions *options)
{
}

static void
gimp_color_picker_options_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GimpColorPickerOptions *options;

  options = GIMP_COLOR_PICKER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SAMPLE_MERGED:
      options->sample_merged = g_value_get_boolean (value);
      break;
    case PROP_SAMPLE_AVERAGE:
      options->sample_average = g_value_get_boolean (value);
      break;
    case PROP_AVERAGE_RADIUS:
      options->average_radius = g_value_get_double (value);
      break;
    case PROP_UPDATE_ACTIVE:
      options->update_active = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_picker_options_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GimpColorPickerOptions *options;

  options = GIMP_COLOR_PICKER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SAMPLE_MERGED:
      g_value_set_boolean (value, options->sample_merged);
      break;
    case PROP_SAMPLE_AVERAGE:
      g_value_set_boolean (value, options->sample_average);
      break;
    case PROP_AVERAGE_RADIUS:
      g_value_set_double (value, options->average_radius);
      break;
    case PROP_UPDATE_ACTIVE:
      g_value_set_boolean (value, options->update_active);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_color_picker_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *button;

  config = G_OBJECT (tool_options);

  vbox = gimp_tool_options_gui (tool_options);

  /*  the sample merged toggle button  */
  button = gimp_prop_check_button_new (config, "sample-merged",
                                       _("Sample Merged"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  the sample average options  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  button = gimp_prop_check_button_new (config, "sample-average",
                                       _("Sample Average"));
  gtk_frame_set_label_widget (GTK_FRAME (frame), button);
  gtk_widget_show (button);

  gtk_widget_set_sensitive (table,
                            GIMP_COLOR_PICKER_OPTIONS (config)->sample_average);
  g_object_set_data (G_OBJECT (button), "set_sensitive", table);

  gimp_prop_scale_entry_new (config, "average-radius",
                             GTK_TABLE (table), 0, 0,
                             _("Radius:"),
                             1.0, 3.0, 0,
                             FALSE, 0.0, 0.0);

  /*  the update active color toggle button  */
  button = gimp_prop_check_button_new (config, "update-active",
                                       _("Update Active Color"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  return vbox;
}
