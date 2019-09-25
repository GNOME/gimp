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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpflipoptions.h"

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

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_FLIP_TYPE,
                         "flip-type",
                         _("Flip Type"),
                         _("Direction of flipping"),
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
  GObject              *config     = G_OBJECT (tool_options);
  GimpFlipOptions      *options    = GIMP_FLIP_OPTIONS (tool_options);
  GimpTransformOptions *tr_options = GIMP_TRANSFORM_OPTIONS (tool_options);
  GtkWidget            *vbox;
  GtkWidget            *frame;
  GtkWidget            *combo;
  gchar                *str;
  GtkListStore         *clip_model;
  GdkModifierType       toggle_mask;

  vbox = gimp_transform_options_gui (tool_options, FALSE, FALSE, FALSE);

  toggle_mask = gimp_get_toggle_behavior_mask ();

  /*  tool toggle  */
  str = g_strdup_printf (_("Direction  (%s)"),
                         gimp_get_mod_string (toggle_mask));

  frame = gimp_prop_enum_radio_frame_new (config, "flip-type",
                                          str,
                                          GIMP_ORIENTATION_HORIZONTAL,
                                          GIMP_ORIENTATION_VERTICAL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  g_free (str);

  options->direction_frame = frame;

  /*  the clipping menu  */
  clip_model = gimp_enum_store_new_with_range (GIMP_TYPE_TRANSFORM_RESIZE,
                                               GIMP_TRANSFORM_RESIZE_ADJUST,
                                               GIMP_TRANSFORM_RESIZE_CLIP);

  combo = gimp_prop_enum_combo_box_new (config, "clip", 0, 0);
  gtk_combo_box_set_model (GTK_COMBO_BOX (combo), GTK_TREE_MODEL (clip_model));
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), tr_options->clip);
  gimp_int_combo_box_set_label (GIMP_INT_COMBO_BOX (combo), _("Clipping"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);

  g_object_unref (clip_model);

  return vbox;
}
