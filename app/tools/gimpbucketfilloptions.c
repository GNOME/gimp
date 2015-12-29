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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpdatafactory.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"

#include "widgets/gimppropwidgets.h"
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


static void   gimp_bucket_fill_options_config_iface_init (GimpConfigInterface *config_iface);

static void   gimp_bucket_fill_options_set_property (GObject         *object,
                                                     guint            property_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);
static void   gimp_bucket_fill_options_get_property (GObject         *object,
                                                     guint            property_id,
                                                     GValue          *value,
                                                     GParamSpec      *pspec);

static void   gimp_bucket_fill_options_reset        (GimpConfig      *config);


G_DEFINE_TYPE_WITH_CODE (GimpBucketFillOptions, gimp_bucket_fill_options,
                         GIMP_TYPE_PAINT_OPTIONS,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_bucket_fill_options_config_iface_init))

#define parent_class gimp_bucket_fill_options_parent_class

static GimpConfigInterface *parent_config_iface = NULL;


static void
gimp_bucket_fill_options_class_init (GimpBucketFillOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_bucket_fill_options_set_property;
  object_class->get_property = gimp_bucket_fill_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_FILL_MODE,
                                 "fill-mode", NULL,
                                 GIMP_TYPE_BUCKET_FILL_MODE,
                                 GIMP_BUCKET_FILL_FG,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FILL_SELECTION,
                                    "fill-selection",
                                    _("Which area will be filled"),
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_FILL_TRANSPARENT,
                                    "fill-transparent",
                                    _("Allow completely transparent regions "
                                      "to be filled"),
                                    TRUE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SAMPLE_MERGED,
                                    "sample-merged",
                                    _("Base filled area on all visible "
                                      "layers"),
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_THRESHOLD,
                                   "threshold",
                                   _("Maximum color difference"),
                                   0.0, 255.0, 15.0,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_FILL_CRITERION,
                                 "fill-criterion",
                                 _("Criterion used for determining color similarity"),
                                 GIMP_TYPE_SELECT_CRITERION,
                                 GIMP_SELECT_CRITERION_COMPOSITE,
                                 GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_bucket_fill_options_config_iface_init (GimpConfigInterface *config_iface)
{
  parent_config_iface = g_type_interface_peek_parent (config_iface);

  config_iface->reset = gimp_bucket_fill_options_reset;
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
gimp_bucket_fill_options_reset (GimpConfig *config)
{
  GimpToolOptions *tool_options = GIMP_TOOL_OPTIONS (config);
  GParamSpec      *pspec;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                        "threshold");

  if (pspec)
    G_PARAM_SPEC_DOUBLE (pspec)->default_value =
      tool_options->tool_info->gimp->config->default_threshold;

  parent_config_iface->reset (config);
}

GtkWidget *
gimp_bucket_fill_options_gui (GimpToolOptions *tool_options)
{
  GObject         *config = G_OBJECT (tool_options);
  GtkWidget       *vbox   = gimp_paint_options_gui (tool_options);
  GtkWidget       *vbox2;
  GtkWidget       *frame;
  GtkWidget       *hbox;
  GtkWidget       *button;
  GtkWidget       *scale;
  GtkWidget       *combo;
  gchar           *str;
  GdkModifierType  extend_mask = gimp_get_extend_selection_mask ();
  GdkModifierType  toggle_mask = gimp_get_toggle_behavior_mask ();

  /*  fill type  */
  str = g_strdup_printf (_("Fill Type  (%s)"),
                         gimp_get_mod_string (toggle_mask)),
  frame = gimp_prop_enum_radio_frame_new (config, "fill-mode", str, 0, 0);
  g_free (str);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gimp_prop_pattern_box_new (NULL, GIMP_CONTEXT (tool_options),
                                    NULL, 2,
                                    "pattern-view-type", "pattern-view-size");
  gimp_enum_radio_frame_add (GTK_FRAME (frame), hbox,
                             GIMP_BUCKET_FILL_PATTERN, TRUE);

  /*  fill selection  */
  str = g_strdup_printf (_("Affected Area  (%s)"),
                         gimp_get_mod_string (extend_mask));
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

  g_object_bind_property (config, "fill-selection",
                          frame,  "sensitive",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_INVERT_BOOLEAN);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
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
  scale = gimp_prop_spin_scale_new (config, "threshold",
                                    _("Threshold"),
                                    1.0, 16.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /*  the fill criterion combo  */
  combo = gimp_prop_enum_combo_box_new (config, "fill-criterion", 0, 0);
  gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo), _("Fill by"));
  gtk_box_pack_start (GTK_BOX (vbox2), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  return vbox;
}
