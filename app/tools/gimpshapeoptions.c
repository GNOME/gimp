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

#include "config/gimpguiconfig.h"

#include "core/gimp.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpshapeoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_RASTERIZE_ON_COMMIT,
  PROP_SHAPE_TYPE
};


static void   gimp_move_options_set_property  (GObject         *object,
                                               guint            property_id,
                                               const GValue    *value,
                                               GParamSpec      *pspec);
static void   gimp_move_options_get_property  (GObject         *object,
                                               guint            property_id,
                                               GValue          *value,
                                               GParamSpec      *pspec);


G_DEFINE_TYPE (GimpShapeOptions, gimp_shape_options, GIMP_TYPE_TOOL_OPTIONS)


static void
gimp_shape_options_class_init (GimpShapeOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_move_options_set_property;
  object_class->get_property = gimp_move_options_get_property;

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_RASTERIZE_ON_COMMIT,
                            "rasterize-on-commit",
                            "Merge down on commit",
                            NULL,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_INT (object_class, PROP_SHAPE_TYPE,
                        "shape-type",
                        "Shape",
                        NULL,
                        0, 2, 0,
                        GIMP_PARAM_STATIC_STRINGS |
                        GIMP_CONFIG_PARAM_CONFIRM);

}

static void
gimp_shape_options_init (GimpShapeOptions *options)
{
}

static void
gimp_move_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpShapeOptions *options = GIMP_SHAPE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_RASTERIZE_ON_COMMIT:
      options->rasterize_on_commit = g_value_get_boolean (value);
      break;
    case PROP_SHAPE_TYPE:
      options->shape_type = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_move_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpShapeOptions *options = GIMP_SHAPE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_RASTERIZE_ON_COMMIT:
      g_value_set_boolean (value, options->rasterize_on_commit);
      break;
    case PROP_SHAPE_TYPE:
      g_value_set_int (value, options->shape_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_shape_options_gui (GimpToolOptions *tool_options)
{
  GObject          *config  = G_OBJECT (tool_options);
  GimpContext      *context = GIMP_CONTEXT (tool_options);
  GtkWidget        *vbox    = gimp_tool_options_gui (tool_options);
  GtkWidget        *button;
  GtkWidget        *label;
  GtkWidget        *combo;
  GtkListStore     *combo_store;

  /*  Shape Type  */
  label = gtk_label_new (_("Shape:"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  combo_store = gimp_int_store_new (_("Rectangle"), 0,
                                    _("Circle"),    1,
                                    _("Triangle"),  2,
                                    NULL);

  combo = gimp_prop_int_combo_box_new (config, "shape-type",
                                       GIMP_INT_STORE (combo_store));

  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_set_visible (combo, TRUE);

  button = gimp_prop_check_button_new (config, "rasterize-on-commit", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  return vbox;
}
