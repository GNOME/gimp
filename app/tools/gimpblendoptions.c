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

#include "core/gimpdatafactory.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpviewablebox.h"

#include "gimpblendoptions.h"
#include "gimppaintoptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_OFFSET,
  PROP_GRADIENT_TYPE,
  PROP_GRADIENT_REPEAT,  /*  overrides a GimpPaintOptions property  */
  PROP_SUPERSAMPLE,
  PROP_SUPERSAMPLE_DEPTH,
  PROP_SUPERSAMPLE_THRESHOLD,
  PROP_DITHER
};


static void   gimp_blend_options_set_property    (GObject          *object,
                                                  guint             property_id,
                                                  const GValue     *value,
                                                  GParamSpec       *pspec);
static void   gimp_blend_options_get_property    (GObject          *object,
                                                  guint             property_id,
                                                  GValue           *value,
                                                  GParamSpec       *pspec);

static void   blend_options_gradient_type_notify (GimpBlendOptions *options,
                                                  GParamSpec       *pspec,
                                                  GtkWidget        *repeat_combo);


G_DEFINE_TYPE (GimpBlendOptions, gimp_blend_options, GIMP_TYPE_PAINT_OPTIONS)


static void
gimp_blend_options_class_init (GimpBlendOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_blend_options_set_property;
  object_class->get_property = gimp_blend_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_OFFSET,
                                   "offset", NULL,
                                   0.0, 100.0, 0.0,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_GRADIENT_TYPE,
                                 "gradient-type", NULL,
                                 GIMP_TYPE_GRADIENT_TYPE,
                                 GIMP_GRADIENT_LINEAR,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_GRADIENT_REPEAT,
                                 "gradient-repeat", NULL,
                                 GIMP_TYPE_REPEAT_MODE,
                                 GIMP_REPEAT_NONE,
                                 GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SUPERSAMPLE,
                                    "supersample", NULL,
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_SUPERSAMPLE_DEPTH,
                                "supersample-depth", NULL,
                                0, 6, 3,
                                GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_SUPERSAMPLE_THRESHOLD,
                                   "supersample-threshold", NULL,
                                   0.0, 4.0, 0.2,
                                   GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_DITHER,
                                    "dither", NULL,
                                    TRUE,
                                    GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_blend_options_init (GimpBlendOptions *options)
{
}

static void
gimp_blend_options_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpBlendOptions *options = GIMP_BLEND_OPTIONS (object);

  switch (property_id)
    {
    case PROP_OFFSET:
      options->offset = g_value_get_double (value);
      break;
    case PROP_GRADIENT_TYPE:
      options->gradient_type = g_value_get_enum (value);
      break;
    case PROP_GRADIENT_REPEAT:
      GIMP_PAINT_OPTIONS (options)->gradient_options->gradient_repeat =
        g_value_get_enum (value);
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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_blend_options_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpBlendOptions *options = GIMP_BLEND_OPTIONS (object);

  switch (property_id)
    {
    case PROP_OFFSET:
      g_value_set_double (value, options->offset);
      break;
    case PROP_GRADIENT_TYPE:
      g_value_set_enum (value, options->gradient_type);
      break;
    case PROP_GRADIENT_REPEAT:
      g_value_set_enum (value,
                        GIMP_PAINT_OPTIONS (options)->gradient_options->gradient_repeat);
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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_blend_options_gui (GimpToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = gimp_paint_options_gui (tool_options);
  GtkWidget *vbox2;
  GtkWidget *frame;
  GtkWidget *scale;
  GtkWidget *combo;
  GtkWidget *button;

  /*  the gradient  */
  button = gimp_prop_gradient_box_new (NULL, GIMP_CONTEXT (tool_options),
                                       _("Gradient"), 2,
                                       "gradient-view-type",
                                       "gradient-view-size",
                                       "gradient-reverse",
                                       "gimp-gradient-editor");
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  the gradient type menu  */
  combo = gimp_prop_enum_combo_box_new (config, "gradient-type", 0, 0);
  gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo), _("Shape"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gimp_enum_combo_box_set_icon_prefix (GIMP_ENUM_COMBO_BOX (combo),
                                       "gimp-gradient");
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  /*  the repeat option  */
  combo = gimp_prop_enum_combo_box_new (config, "gradient-repeat", 0, 0);
  gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo), _("Repeat"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  g_signal_connect (config, "notify::gradient-type",
                    G_CALLBACK (blend_options_gradient_type_notify),
                    combo);

  /*  the offset scale  */
  scale = gimp_prop_spin_scale_new (config, "offset",
                                    _("Offset"),
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /*  the dither toggle  */
  button = gimp_prop_check_button_new (config, "dither",
                                       _("Dithering"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  supersampling options  */
  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  frame = gimp_prop_expanding_frame_new (config, "supersample",
                                         _("Adaptive supersampling"),
                                         vbox2, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /*  max depth scale  */
  scale = gimp_prop_spin_scale_new (config, "supersample-depth",
                                    _("Max depth"),
                                    1.0, 1.0, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /*  threshold scale  */
  scale = gimp_prop_spin_scale_new (config, "supersample-threshold",
                                    _("Threshold"),
                                    0.01, 0.1, 2);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  return vbox;
}

static void
blend_options_gradient_type_notify (GimpBlendOptions *options,
                                    GParamSpec       *pspec,
                                    GtkWidget        *repeat_combo)
{
  gtk_widget_set_sensitive (repeat_combo, options->gradient_type < 6);
}
