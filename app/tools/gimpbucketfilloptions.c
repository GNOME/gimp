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

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpdatafactory.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"

#include "widgets/gimpviewablebox.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpbucketfilloptions.h"
#include "gimppaintoptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_FILL_MODE,
  PROP_FILL_SELECTION,
  PROP_FILL_TRANSPARENT,
  PROP_SAMPLE_MERGED,
  PROP_THRESHOLD,
  PROP_FILL_CRITERION
};


static void   gimp_bucket_fill_options_set_property (GObject         *object,
                                                     guint            property_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);
static void   gimp_bucket_fill_options_get_property (GObject         *object,
                                                     guint            property_id,
                                                     GValue          *value,
                                                     GParamSpec      *pspec);

static void   gimp_bucket_fill_options_reset        (GimpToolOptions *tool_options);
static void   gimp_bucket_fill_options_notify (GimpBucketFillOptions *options,
                                               GParamSpec            *pspec,
                                               GtkWidget             *widget);


G_DEFINE_TYPE (GimpBucketFillOptions, gimp_bucket_fill_options,
               GIMP_TYPE_PAINT_OPTIONS)

#define parent_class gimp_bucket_fill_options_parent_class


static void
gimp_bucket_fill_options_class_init (GimpBucketFillOptionsClass *klass)
{
  GObjectClass         *object_class  = G_OBJECT_CLASS (klass);
  GimpToolOptionsClass *options_class = GIMP_TOOL_OPTIONS_CLASS (klass);

  object_class->set_property = gimp_bucket_fill_options_set_property;
  object_class->get_property = gimp_bucket_fill_options_get_property;

  options_class->reset       = gimp_bucket_fill_options_reset;

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_FILL_MODE,
                                 "fill-mode", NULL,
                                 GIMP_TYPE_BUCKET_FILL_MODE,
                                 GIMP_FG_BUCKET_FILL,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FILL_SELECTION,
                                    "fill-selection", NULL,
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FILL_TRANSPARENT,
                                    "fill-transparent",
                                    N_("Allow completely transparent regions "
                                       "to be filled"),
                                    TRUE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SAMPLE_MERGED,
                                    "sample-merged",
                                    N_("Base filled area on all visible "
                                       "layers"),
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_THRESHOLD,
                                   "threshold",
                                   N_("Maximum color difference"),
                                   0.0, 255.0, 15.0,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_FILL_CRITERION,
                                 "fill-criterion", NULL,
                                 GIMP_TYPE_SELECT_CRITERION,
                                 GIMP_SELECT_CRITERION_COMPOSITE,
                                 GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_bucket_fill_options_init (GimpBucketFillOptions *options)
{
}

static void
gimp_bucket_fill_options_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GimpBucketFillOptions *options = GIMP_BUCKET_FILL_OPTIONS (object);

  switch (property_id)
    {
    case PROP_FILL_MODE:
      options->fill_mode = g_value_get_enum (value);
      break;
    case PROP_FILL_SELECTION:
      options->fill_selection = g_value_get_boolean (value);
      break;
    case PROP_FILL_TRANSPARENT:
      options->fill_transparent = g_value_get_boolean (value);
      break;
    case PROP_SAMPLE_MERGED:
      options->sample_merged = g_value_get_boolean (value);
      break;
    case PROP_THRESHOLD:
      options->threshold = g_value_get_double (value);
      break;
    case PROP_FILL_CRITERION:
      options->fill_criterion = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_bucket_fill_options_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  GimpBucketFillOptions *options = GIMP_BUCKET_FILL_OPTIONS (object);

  switch (property_id)
    {
    case PROP_FILL_MODE:
      g_value_set_enum (value, options->fill_mode);
      break;
    case PROP_FILL_SELECTION:
      g_value_set_boolean (value, options->fill_selection);
      break;
    case PROP_FILL_TRANSPARENT:
      g_value_set_boolean (value, options->fill_transparent);
      break;
    case PROP_SAMPLE_MERGED:
      g_value_set_boolean (value, options->sample_merged);
      break;
    case PROP_THRESHOLD:
      g_value_set_double (value, options->threshold);
      break;
    case PROP_FILL_CRITERION:
      g_value_set_enum (value, options->fill_criterion);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_bucket_fill_options_reset (GimpToolOptions *tool_options)
{
  GParamSpec *pspec;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (tool_options),
                                        "threshold");

  if (pspec)
    G_PARAM_SPEC_DOUBLE (pspec)->default_value =
      GIMP_GUI_CONFIG (tool_options->tool_info->gimp->config)->default_threshold;

  GIMP_TOOL_OPTIONS_CLASS (parent_class)->reset (tool_options);
}

GtkWidget *
gimp_bucket_fill_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_paint_options_gui (tool_options);
  GtkWidget *vbox2;
  GtkWidget *table;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *combo;
  gchar     *str;

  /*  fill type  */
  str = g_strdup_printf (_("Fill Type  (%s)"),
                         gimp_get_mod_string (GDK_CONTROL_MASK)),
  frame = gimp_prop_enum_radio_frame_new (config, "fill-mode", str, 0, 0);
  g_free (str);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gimp_prop_pattern_box_new (NULL, GIMP_CONTEXT (tool_options), 2,
                                    "pattern-view-type", "pattern-view-size");
  gimp_enum_radio_frame_add (GTK_FRAME (frame), hbox, GIMP_PATTERN_BUCKET_FILL);

  /*  fill selection  */
  str = g_strdup_printf (_("Affected Area  (%s)"),
                         gimp_get_mod_string (GDK_SHIFT_MASK));
  frame = gimp_prop_boolean_radio_frame_new (config, "fill-selection",
                                             str,
                                             _("Fill whole selection"),
                                             _("Fill similar colors"));
  g_free (str);
  gtk_box_reorder_child (GTK_BOX (gtk_bin_get_child (GTK_BIN (frame))),
                         g_object_get_data (G_OBJECT (frame), "radio-button"),
                         1);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  frame = gimp_frame_new (_("Finding Similar Colors"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_set_sensitive (frame,
                            ! GIMP_BUCKET_FILL_OPTIONS (config)->fill_selection);
  g_signal_connect_object (config, "notify::fill-selection",
                           G_CALLBACK (gimp_bucket_fill_options_notify),
                           G_OBJECT (frame), 0);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  /*  the fill transparent areas toggle  */
  button = gimp_prop_check_button_new (config, "fill-transparent",
                                       _("Fill transparent areas"));
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  the sample merged toggle  */
  button = gimp_prop_check_button_new (config, "sample-merged",
                                       _("Sample merged"));
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  the threshold scale  */
  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  gimp_prop_scale_entry_new (config, "threshold",
                             GTK_TABLE (table), 0, 0,
                             _("Threshold:"),
                             1.0, 16.0, 1,
                             FALSE, 0.0, 0.0);

  /*  the fill criterion combo  */
  combo = gimp_prop_enum_combo_box_new (config, "fill-criterion", 0, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Fill by:"), 0.0, 0.5,
                             combo, 2, FALSE);

  return vbox;
}

static void
gimp_bucket_fill_options_notify (GimpBucketFillOptions *options,
                                 GParamSpec            *pspec,
                                 GtkWidget             *widget)
{
  gtk_widget_set_sensitive (widget, ! options->fill_selection);
}
