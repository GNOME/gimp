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
  PROP_SHAPE_TYPE,
  PROP_RASTERIZE_ON_COMMIT,
  PROP_NUMBER_OF_SIDES,
  PROP_STROKE_WIDTH,
  PROP_STROKE_UNIT,
  PROP_ENABLE_FILL
};


static void   gimp_shape_options_set_property      (GObject         *object,
                                                    guint            property_id,
                                                    const GValue    *value,
                                                    GParamSpec      *pspec);
static void   gimp_shape_options_get_property      (GObject         *object,
                                                    guint            property_id,
                                                    GValue          *value,
                                                    GParamSpec      *pspec);

static void   gimp_shape_options_shape_type_notify (GimpShapeOptions *options,
                                                    GParamSpec       *pspec,
                                                    GtkWidget        *spinbutton);

G_DEFINE_TYPE (GimpShapeOptions, gimp_shape_options, GIMP_TYPE_TOOL_OPTIONS)


static void
gimp_shape_options_class_init (GimpShapeOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_shape_options_set_property;
  object_class->get_property = gimp_shape_options_get_property;

  GIMP_CONFIG_PROP_INT (object_class, PROP_SHAPE_TYPE,
                        "shape-type",
                        _("Shape"),
                        NULL,
                        GIMP_SHAPE_MODE_LINE, GIMP_SHAPE_MODE_LAST - 1,
                        GIMP_SHAPE_MODE_LINE,
                        GIMP_PARAM_STATIC_STRINGS |
                        GIMP_CONFIG_PARAM_CONFIRM);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_RASTERIZE_ON_COMMIT,
                            "rasterize-on-commit",
                            "Merge down on commit",
                            NULL,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_INT (object_class, PROP_NUMBER_OF_SIDES,
                        "number-of-sides",
                        _("Number of sides"),
                        _("For polygons, this determines how many "
                          "sides the shape will have."),
                        3, 50, 3,
                        GIMP_PARAM_STATIC_STRINGS |
                        GIMP_CONFIG_PARAM_CONFIRM);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_STROKE_WIDTH,
                           "stroke-width",
                           NULL, NULL,
                           1.0, 2000.0, 6.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_UNIT (object_class, PROP_STROKE_UNIT,
                         "stroke-width-unit",
                         NULL, NULL,
                         TRUE, FALSE, gimp_unit_pixel (),
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_ENABLE_FILL,
                            "enable-fill",
                            "Enable fill",
                            NULL,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_shape_options_init (GimpShapeOptions *options)
{
}

static void
gimp_shape_options_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpShapeOptions *options = GIMP_SHAPE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SHAPE_TYPE:
      options->shape_type = g_value_get_int (value);
      break;
    case PROP_RASTERIZE_ON_COMMIT:
      options->rasterize_on_commit = g_value_get_boolean (value);
      break;
    case PROP_NUMBER_OF_SIDES:
      options->number_of_sides = g_value_get_int (value);
      break;
    case PROP_STROKE_WIDTH:
      options->stroke_width = g_value_get_double (value);
      break;
    case PROP_STROKE_UNIT:
      options->stroke_unit = g_value_get_object (value);
      break;
    case PROP_ENABLE_FILL:
      options->enable_fill = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_shape_options_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpShapeOptions *options = GIMP_SHAPE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SHAPE_TYPE:
      g_value_set_int (value, options->shape_type);
      break;
    case PROP_RASTERIZE_ON_COMMIT:
      g_value_set_boolean (value, options->rasterize_on_commit);
      break;
    case PROP_NUMBER_OF_SIDES:
      g_value_set_int (value, options->number_of_sides);
      break;
    case PROP_STROKE_WIDTH:
      g_value_set_double (value, options->stroke_width);
      break;
    case PROP_STROKE_UNIT:
      g_value_set_object (value, options->stroke_unit);
      break;
    case PROP_ENABLE_FILL:
      g_value_set_boolean (value, options->enable_fill);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_shape_options_shape_type_notify (GimpShapeOptions *options,
                                      GParamSpec       *pspec,
                                      GtkWidget        *spinbutton)
{
  gboolean needs_sides = (options->shape_type == GIMP_SHAPE_MODE_POLYGON ||
                          options->shape_type == GIMP_SHAPE_MODE_STAR);

  gtk_widget_set_sensitive (spinbutton, needs_sides);
}

GtkWidget *
gimp_shape_options_gui (GimpToolOptions *tool_options)
{
  GObject          *config  = G_OBJECT (tool_options);
  GimpShapeOptions *options = GIMP_SHAPE_OPTIONS (tool_options);
  GtkWidget         *vbox   = gimp_tool_options_gui (tool_options);
  GtkWidget         *hbox;
  GtkWidget         *button;
  GtkWidget         *entry;
  GtkWidget         *grid;
  GtkWidget         *label;
  GtkWidget         *scale;
  GtkWidget         *combo;
  GtkListStore      *combo_store;

  /*  Shape Type  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  label = gtk_label_new (_("Shape:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  combo_store = gimp_int_store_new (_("Line"),      GIMP_SHAPE_MODE_LINE,
                                    _("Rectangle"), GIMP_SHAPE_MODE_RECTANGLE,
                                    _("Circle"),    GIMP_SHAPE_MODE_ARC,
                                    _("Polygon"),   GIMP_SHAPE_MODE_POLYGON,
                                    _("Star"),      GIMP_SHAPE_MODE_STAR,
                                    NULL);

  combo = gimp_prop_int_combo_box_new (config, "shape-type",
                                       GIMP_INT_STORE (combo_store));

  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);
  gtk_widget_set_visible (combo, TRUE);

  scale = gimp_prop_label_spin_new (config, "number-of-sides", 0);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_set_visible (scale, TRUE);

  g_signal_connect_object (config, "notify::shape-type",
                           G_CALLBACK (gimp_shape_options_shape_type_notify),
                           scale, 0);
  gimp_shape_options_shape_type_notify (options, NULL, scale);

  button = gimp_prop_check_button_new (config, "rasterize-on-commit", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  button = gimp_prop_check_button_new (config, "enable-fill", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_set_visible (grid, TRUE);

  entry = gimp_prop_size_entry_new (config,
                                    "stroke-width", FALSE, "stroke-width-unit",
                                    "%n", GIMP_SIZE_ENTRY_UPDATE_SIZE, 72.0);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0, _("Width:"), 0.0, 0.5,
                            entry, 2);

  return vbox;
}
