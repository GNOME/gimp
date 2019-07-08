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

#include "core/gimpdata.h"
#include "core/gimpdatafactory.h"
#include "core/gimp-gradients.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpviewablebox.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpgradientoptions.h"
#include "gimppaintoptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_OFFSET,
  PROP_GRADIENT_TYPE,
  PROP_DISTANCE_METRIC,
  PROP_SUPERSAMPLE,
  PROP_SUPERSAMPLE_DEPTH,
  PROP_SUPERSAMPLE_THRESHOLD,
  PROP_DITHER,
  PROP_INSTANT,
  PROP_MODIFY_ACTIVE
};


static void   gimp_gradient_options_set_property           (GObject             *object,
                                                            guint                property_id,
                                                            const GValue        *value,
                                                            GParamSpec          *pspec);
static void   gimp_gradient_options_get_property           (GObject             *object,
                                                            guint                property_id,
                                                            GValue              *value,
                                                            GParamSpec          *pspec);

static void   gradient_options_repeat_gradient_type_notify (GimpGradientOptions *options,
                                                            GParamSpec          *pspec,
                                                            GtkWidget           *repeat_combo);
static void   gradient_options_metric_gradient_type_notify (GimpGradientOptions *options,
                                                            GParamSpec          *pspec,
                                                            GtkWidget           *repeat_combo);


G_DEFINE_TYPE (GimpGradientOptions, gimp_gradient_options,
               GIMP_TYPE_PAINT_OPTIONS)


static void
gimp_gradient_options_class_init (GimpGradientOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_gradient_options_set_property;
  object_class->get_property = gimp_gradient_options_get_property;

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_OFFSET,
                           "offset",
                           _("Offset"),
                           NULL,
                           0.0, 100.0, 0.0,
                           GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_PROP_ENUM (object_class, PROP_GRADIENT_TYPE,
                         "gradient-type",
                         _("Shape"),
                         NULL,
                         GIMP_TYPE_GRADIENT_TYPE,
                         GIMP_GRADIENT_LINEAR,
                         GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_PROP_ENUM (object_class, PROP_DISTANCE_METRIC,
                         "distance-metric",
                         _("Metric"),
                         _("Metric to use for the distance calculation"),
                         GEGL_TYPE_DISTANCE_METRIC,
                         GEGL_DISTANCE_METRIC_EUCLIDEAN,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SUPERSAMPLE,
                            "supersample",
                            _("Adaptive Supersampling"),
                            NULL,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_PROP_INT (object_class, PROP_SUPERSAMPLE_DEPTH,
                        "supersample-depth",
                        _("Max depth"),
                        NULL,
                        1, 9, 3,
                        GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_SUPERSAMPLE_THRESHOLD,
                           "supersample-threshold",
                           _("Threshold"),
                           NULL,
                           0.0, 4.0, 0.2,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_DITHER,
                            "dither",
                            _("Dithering"),
                            NULL,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_INSTANT,
                            "instant",
                            _("Instant mode"),
                            _("Commit gradient instantly"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_MODIFY_ACTIVE,
                            "modify-active",
                            _("Modify active gradient"),
                            _("Modify the active gradient in-place"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_gradient_options_init (GimpGradientOptions *options)
{
}

static void
gimp_gradient_options_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GimpGradientOptions *options = GIMP_GRADIENT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_OFFSET:
      options->offset = g_value_get_double (value);
      break;
    case PROP_GRADIENT_TYPE:
      options->gradient_type = g_value_get_enum (value);
      break;
    case PROP_DISTANCE_METRIC:
      options->distance_metric = g_value_get_enum (value);
      break;

    case PROP_SUPERSAMPLE:
      options->supersample = g_value_get_boolean (value);
      break;
    case PROP_SUPERSAMPLE_DEPTH:
      options->supersample_depth = g_value_get_int (value);
      break;
    case PROP_SUPERSAMPLE_THRESHOLD:
      options->supersample_threshold = g_value_get_double (value);
      break;

    case PROP_DITHER:
      options->dither = g_value_get_boolean (value);
      break;

    case PROP_INSTANT:
      options->instant = g_value_get_boolean (value);
      break;
    case PROP_MODIFY_ACTIVE:
      options->modify_active = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_gradient_options_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpGradientOptions *options = GIMP_GRADIENT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_OFFSET:
      g_value_set_double (value, options->offset);
      break;
    case PROP_GRADIENT_TYPE:
      g_value_set_enum (value, options->gradient_type);
      break;
    case PROP_DISTANCE_METRIC:
      g_value_set_enum (value, options->distance_metric);
      break;

    case PROP_SUPERSAMPLE:
      g_value_set_boolean (value, options->supersample);
      break;
    case PROP_SUPERSAMPLE_DEPTH:
      g_value_set_int (value, options->supersample_depth);
      break;
    case PROP_SUPERSAMPLE_THRESHOLD:
      g_value_set_double (value, options->supersample_threshold);
      break;

    case PROP_DITHER:
      g_value_set_boolean (value, options->dither);
      break;

    case PROP_INSTANT:
      g_value_set_boolean (value, options->instant);
      break;
    case PROP_MODIFY_ACTIVE:
      g_value_set_boolean (value, options->modify_active);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_gradient_options_gui (GimpToolOptions *tool_options)
{
  GObject             *config  = G_OBJECT (tool_options);
  GimpContext         *context = GIMP_CONTEXT (tool_options);
  GimpGradientOptions *options = GIMP_GRADIENT_OPTIONS (tool_options);
  GtkWidget           *vbox    = gimp_paint_options_gui (tool_options);
  GtkWidget           *vbox2;
  GtkWidget           *frame;
  GtkWidget           *scale;
  GtkWidget           *combo;
  GtkWidget           *button;
  GtkWidget           *label;
  gchar               *str;
  GdkModifierType      extend_mask;
  GimpGradient        *gradient;

  extend_mask = gimp_get_extend_selection_mask ();

  /*  the gradient  */
  button = gimp_prop_gradient_box_new (NULL, GIMP_CONTEXT (tool_options),
                                       _("Gradient"), 2,
                                       "gradient-view-type",
                                       "gradient-view-size",
                                       "gradient-reverse",
                                       "gradient-blend-color-space",
                                       "gimp-gradient-editor",
                                       _("Edit this gradient"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  the blend color space  */
  combo = gimp_prop_enum_combo_box_new (config, "gradient-blend-color-space",
                                        0, 0);
  gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo),
                                _("Blend Color Space"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  /*  the gradient type menu  */
  combo = gimp_prop_enum_combo_box_new (config, "gradient-type", 0, 0);
  gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo), _("Shape"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gimp_enum_combo_box_set_icon_prefix (GIMP_ENUM_COMBO_BOX (combo),
                                       "gimp-gradient");
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  /*  the distance metric menu  */
  combo = gimp_prop_enum_combo_box_new (config, "distance-metric", 0, 0);
  gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo), _("Metric"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  g_signal_connect (config, "notify::gradient-type",
                    G_CALLBACK (gradient_options_metric_gradient_type_notify),
                    combo);
  gradient_options_metric_gradient_type_notify (options, NULL, combo);

  /*  the repeat option  */
  combo = gimp_prop_enum_combo_box_new (config, "gradient-repeat", 0, 0);
  gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo), _("Repeat"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  g_signal_connect (config, "notify::gradient-type",
                    G_CALLBACK (gradient_options_repeat_gradient_type_notify),
                    combo);
  gradient_options_repeat_gradient_type_notify (options, NULL, combo);

  /*  the offset scale  */
  scale = gimp_prop_spin_scale_new (config, "offset", NULL,
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /*  the dither toggle  */
  button = gimp_prop_check_button_new (config, "dither", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  supersampling options  */
  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  frame = gimp_prop_expanding_frame_new (config, "supersample", NULL,
                                         vbox2, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  max depth scale  */
  scale = gimp_prop_spin_scale_new (config, "supersample-depth", NULL,
                                    1.0, 1.0, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /*  threshold scale  */
  scale = gimp_prop_spin_scale_new (config, "supersample-threshold", NULL,
                                    0.01, 0.1, 2);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /* the instant toggle */
  str = g_strdup_printf (_("Instant mode  (%s)"),
                          gimp_get_mod_string (extend_mask));

  button = gimp_prop_check_button_new (config, "instant", str);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_free (str);

  options->instant_toggle = button;

  /*  the modify active toggle  */
  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  frame = gimp_prop_expanding_frame_new (config, "modify-active", NULL,
                                         vbox2, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  options->modify_active_frame = frame;

  label = gtk_label_new (_("The active gradient is non-writable "
                           "and cannot be edited directly. "
                           "Uncheck this option "
                           "to edit a copy of it."));
  gtk_box_pack_start (GTK_BOX (vbox2), label, TRUE, TRUE, 0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_width_chars (GTK_LABEL (label), 24);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);

  options->modify_active_hint = label;

  gradient = gimp_context_get_gradient (GIMP_CONTEXT (options));

  gtk_widget_set_sensitive (options->modify_active_frame,
                            gradient !=
                            gimp_gradients_get_custom (context->gimp));
  gtk_widget_set_visible (options->modify_active_hint,
                          gradient &&
                          ! gimp_data_is_writable (GIMP_DATA (gradient)));

  return vbox;
}

static void
gradient_options_repeat_gradient_type_notify (GimpGradientOptions *options,
                                              GParamSpec       *pspec,
                                              GtkWidget        *repeat_combo)
{
  gtk_widget_set_sensitive (repeat_combo,
                            options->gradient_type < GIMP_GRADIENT_SHAPEBURST_ANGULAR);
}

static void
gradient_options_metric_gradient_type_notify (GimpGradientOptions *options,
                                              GParamSpec       *pspec,
                                              GtkWidget        *repeat_combo)
{
  gtk_widget_set_sensitive (repeat_combo,
                            options->gradient_type >= GIMP_GRADIENT_SHAPEBURST_ANGULAR &&
                            options->gradient_type <= GIMP_GRADIENT_SHAPEBURST_DIMPLED);
}
