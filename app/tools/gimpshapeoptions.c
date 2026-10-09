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
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpdatafactory.h"
#include "core/gimpfilloptions.h"
#include "core/gimppattern.h"
#include "core/gimpstrokeoptions.h"

#include "widgets/gimpstrokeeditor.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpshapeoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_SHAPE_TYPE,
  PROP_SHAPE_MODE,
  PROP_DRAW_ON_LAYERS,
  PROP_NUMBER_OF_SIDES,
  PROP_FILL_STYLE,
  PROP_FILL_FOREGROUND,
  PROP_FILL_PATTERN,
  PROP_FILL_ANTIALIAS,
  PROP_STROKE_STYLE,
  PROP_STROKE_FOREGROUND,
  PROP_STROKE_PATTERN,
  PROP_STROKE_ANTIALIAS,
  PROP_STROKE_WIDTH,
  PROP_STROKE_UNIT,
  PROP_STROKE_CAP_STYLE,
  PROP_STROKE_JOIN_STYLE,
  PROP_STROKE_MITER_LIMIT,
  PROP_STROKE_DASH_OFFSET
};

static void      gimp_shape_options_config_iface_init   (GimpConfigInterface *config_iface);
static gboolean  gimp_shape_options_serialize_property  (GimpConfig          *config,
                                                         guint                property_id,
                                                         const GValue        *value,
                                                         GParamSpec          *pspec,
                                                         GimpConfigWriter    *writer);
static gboolean  gimp_shape_options_deserialize_property (GimpConfig          *config,
                                                         guint                property_id,
                                                         GValue              *value,
                                                         GParamSpec          *pspec,
                                                         GScanner            *scanner,
                                                         GTokenType          *expected);

static void   gimp_shape_options_finalize               (GObject             *object);
static void   gimp_shape_options_set_property           (GObject             *object,
                                                         guint                property_id,
                                                         const GValue        *value,
                                                         GParamSpec          *pspec);
static void   gimp_shape_options_get_property           (GObject             *object,
                                                         guint                property_id,
                                                         GValue              *value,
                                                         GParamSpec          *pspec);

static void   gimp_shape_options_shape_type_notify      (GimpShapeOptions    *options,
                                                         GParamSpec          *pspec,
                                                         GtkWidget           *spinbutton);
static void   gimp_shape_options_fill_style_notify      (GimpShapeOptions    *options);

G_DEFINE_TYPE_WITH_CODE (GimpShapeOptions, gimp_shape_options,
                         GIMP_TYPE_TOOL_OPTIONS,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_shape_options_config_iface_init))

#define parent_class gimp_shape_options_parent_class

static GimpConfigInterface *parent_config_iface = NULL;

static void
gimp_shape_options_class_init (GimpShapeOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GeglColor    *white        = gegl_color_new ("white");
  GeglColor    *black        = gegl_color_new ("black");

  object_class->finalize     = gimp_shape_options_finalize;
  object_class->set_property = gimp_shape_options_set_property;
  object_class->get_property = gimp_shape_options_get_property;

  GIMP_CONFIG_PROP_INT (object_class, PROP_SHAPE_TYPE,
                        "shape-type",
                        _("Shape"),
                        NULL,
                        GIMP_SHAPE_TYPE_LINE, GIMP_SHAPE_TYPE_LAST - 1,
                        GIMP_SHAPE_TYPE_LINE,
                        GIMP_PARAM_STATIC_STRINGS |
                        GIMP_CONFIG_PARAM_CONFIRM);

  GIMP_CONFIG_PROP_INT (object_class, PROP_SHAPE_MODE,
                        "shape-mode",
                        _("Mode"),
                        NULL,
                        GIMP_SHAPE_MODE_FILL_STROKE,
                        GIMP_SHAPE_MODE_STROKE_ONLY,
                        GIMP_SHAPE_MODE_FILL_STROKE,
                        GIMP_PARAM_STATIC_STRINGS |
                        GIMP_CONFIG_PARAM_CONFIRM);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_DRAW_ON_LAYERS,
                            "draw-on-layers",
                            _("Draw directly on selected rasters"),
                            _("If checked, shapes will be drawn directly "
                              "on any selected raster item instead of as "
                              "a vector layer"),
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

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_FILL_STYLE,
                         "fill-custom-style",
                         NULL, NULL,
                         GIMP_TYPE_CUSTOM_STYLE,
                         GIMP_CUSTOM_STYLE_SOLID_COLOR,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_COLOR (object_class, PROP_FILL_FOREGROUND,
                          "fill-foreground",
                          NULL, NULL,
                          TRUE, white,
                          GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_FILL_PATTERN,
                           "fill-pattern",
                           NULL, NULL,
                           GIMP_TYPE_PATTERN,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_FILL_ANTIALIAS,
                            "fill-antialias",
                            NULL, NULL,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_STROKE_STYLE,
                         "stroke-custom-style",
                         NULL, NULL,
                         GIMP_TYPE_CUSTOM_STYLE,
                         GIMP_CUSTOM_STYLE_SOLID_COLOR,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_COLOR (object_class, PROP_STROKE_FOREGROUND,
                          "stroke-foreground",
                          NULL, NULL,
                          TRUE, black,
                          GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_STROKE_PATTERN,
                           "stroke-pattern",
                           NULL, NULL,
                           GIMP_TYPE_PATTERN,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_STROKE_ANTIALIAS,
                            "stroke-antialias",
                            NULL, NULL,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_STROKE_WIDTH,
                           "stroke-width",
                           NULL, NULL,
                           0.0, 2000.0, 6.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_UNIT (object_class, PROP_STROKE_UNIT,
                         "stroke-unit",
                         NULL, NULL,
                         TRUE, FALSE, gimp_unit_pixel (),
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_STROKE_CAP_STYLE,
                         "stroke-cap-style",
                         NULL, NULL,
                         GIMP_TYPE_CAP_STYLE, GIMP_CAP_BUTT,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_STROKE_JOIN_STYLE,
                         "stroke-join-style",
                         NULL, NULL,
                         GIMP_TYPE_JOIN_STYLE, GIMP_JOIN_MITER,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_STROKE_MITER_LIMIT,
                           "stroke-miter-limit",
                           NULL, NULL,
                           0.0, 100.0, 10.0,
                           GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_STROKE_DASH_OFFSET,
                           "stroke-dash-offset",
                           NULL, NULL,
                           0.0, 2000.0, 0.0,
                           GIMP_PARAM_STATIC_STRINGS);

  g_clear_object (&white);
  g_clear_object (&black);
}

static void
gimp_shape_options_config_iface_init (GimpConfigInterface *config_iface)
{
  parent_config_iface = g_type_interface_peek_parent (config_iface);

  config_iface->serialize_property   = gimp_shape_options_serialize_property;
  config_iface->deserialize_property = gimp_shape_options_deserialize_property;
}

static void
gimp_shape_options_init (GimpShapeOptions *options)
{
  GeglColor *white = gegl_color_new ("white");
  GeglColor *black = gegl_color_new ("black");

  options->fill_foreground   = white;
  options->stroke_foreground = black;
}

static void
gimp_shape_options_finalize (GObject *object)
{
  GimpShapeOptions *options = GIMP_SHAPE_OPTIONS (object);

  g_clear_object (&options->fill_options);
  g_clear_object (&options->stroke_options);
  g_clear_object (&options->fill_foreground);

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
    case PROP_SHAPE_MODE:
      options->shape_mode = g_value_get_int (value);
      break;
    case PROP_DRAW_ON_LAYERS:
      options->draw_on_layers = g_value_get_boolean (value);
      break;
    case PROP_NUMBER_OF_SIDES:
      options->number_of_sides = g_value_get_int (value);
      break;

    case PROP_FILL_STYLE:
      options->fill_style = g_value_get_enum (value);
      break;
    case PROP_FILL_FOREGROUND:
      g_set_object (&options->fill_foreground, g_value_get_object (value));
      break;
    case PROP_FILL_PATTERN:
      g_set_object (&options->fill_pattern, g_value_get_object (value));
      break;
    case PROP_FILL_ANTIALIAS:
      options->fill_antialias = g_value_get_boolean (value);
      break;

    case PROP_STROKE_STYLE:
      options->stroke_style = g_value_get_enum (value);
      break;
    case PROP_STROKE_FOREGROUND:
      g_set_object (&options->stroke_foreground, g_value_get_object (value));
      break;
    case PROP_STROKE_PATTERN:
      g_set_object (&options->stroke_pattern, g_value_get_object (value));
      break;
    case PROP_STROKE_ANTIALIAS:
      options->stroke_antialias = g_value_get_boolean (value);
      break;
    case PROP_STROKE_WIDTH:
      options->stroke_width = g_value_get_double (value);
      break;
    case PROP_STROKE_UNIT:
      options->stroke_unit = g_value_get_object (value);
      break;
    case PROP_STROKE_CAP_STYLE:
      options->stroke_cap_style = g_value_get_enum (value);
      break;
    case PROP_STROKE_JOIN_STYLE:
      options->stroke_join_style = g_value_get_enum (value);
      break;
    case PROP_STROKE_MITER_LIMIT:
      options->stroke_miter_limit = g_value_get_double (value);
      break;
    case PROP_STROKE_DASH_OFFSET:
      options->stroke_dash_offset = g_value_get_double (value);
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
    case PROP_SHAPE_MODE:
      g_value_set_int (value, options->shape_mode);
      break;
    case PROP_DRAW_ON_LAYERS:
      g_value_set_boolean (value, options->draw_on_layers);
      break;
    case PROP_NUMBER_OF_SIDES:
      g_value_set_int (value, options->number_of_sides);
      break;

    case PROP_FILL_STYLE:
      g_value_set_enum (value, options->fill_style);
      break;
    case PROP_FILL_FOREGROUND:
      g_value_set_object (value, options->fill_foreground);
      break;
    case PROP_FILL_PATTERN:
      g_value_set_object (value, options->fill_pattern);
      break;
    case PROP_FILL_ANTIALIAS:
      g_value_set_boolean (value, options->fill_antialias);
      break;

    case PROP_STROKE_STYLE:
      g_value_set_enum (value, options->stroke_style);
      break;
    case PROP_STROKE_FOREGROUND:
      g_value_set_object (value, options->stroke_foreground);
      break;
    case PROP_STROKE_PATTERN:
      g_value_set_object (value, options->stroke_pattern);
      break;
    case PROP_STROKE_ANTIALIAS:
      g_value_set_boolean (value, options->stroke_antialias);
      break;

    case PROP_STROKE_WIDTH:
      g_value_set_double (value, options->stroke_width);
      break;
    case PROP_STROKE_UNIT:
      g_value_set_object (value, options->stroke_unit);
      break;
    case PROP_STROKE_CAP_STYLE:
      g_value_set_enum (value, options->stroke_cap_style);
      break;
    case PROP_STROKE_JOIN_STYLE:
      g_value_set_enum (value, options->stroke_join_style);
      break;
    case PROP_STROKE_MITER_LIMIT:
      g_value_set_double (value, options->stroke_miter_limit);
      break;
    case PROP_STROKE_DASH_OFFSET:
      g_value_set_double (value, options->stroke_dash_offset);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_shape_options_serialize_property (GimpConfig       *config,
                                       guint             property_id,
                                       const GValue     *value,
                                       GParamSpec       *pspec,
                                       GimpConfigWriter *writer)
{
  if (property_id == PROP_FILL_PATTERN ||
      property_id == PROP_STROKE_PATTERN)
    {
      GimpObject *serialize_obj = g_value_get_object (value);

      gimp_config_writer_open (writer, pspec->name);

      if (serialize_obj)
        gimp_config_writer_string (writer,
                                   gimp_object_get_name (serialize_obj));
      else
        gimp_config_writer_print (writer, "NULL", 4);

      gimp_config_writer_close (writer);

      return TRUE;
    }

  return FALSE;
}

static gboolean
gimp_shape_options_deserialize_property (GimpConfig *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec,
                                         GScanner   *scanner,
                                         GTokenType *expected)
{
  if (property_id == PROP_FILL_PATTERN ||
      property_id == PROP_STROKE_PATTERN)
    {
      gchar *object_name;

      if (gimp_scanner_parse_identifier (scanner, "NULL"))
        {
          g_value_set_object (value, NULL);
        }
      else if (gimp_scanner_parse_string (scanner, &object_name))
        {
          GimpContext   *context = GIMP_CONTEXT (object);
          GimpContainer *container;
          GimpObject    *deserialize_obj;

          if (! object_name)
            object_name = g_strdup ("");

          container =
            gimp_data_factory_get_container (context->gimp->pattern_factory);

          deserialize_obj = gimp_container_get_child_by_name (container,
                                                              object_name);

          g_value_set_object (value, deserialize_obj);

          g_free (object_name);
        }
      else
        {
          *expected = G_TOKEN_STRING;
        }

      return TRUE;
    }

  return FALSE;
}

static void
gimp_shape_options_shape_type_notify (GimpShapeOptions *options,
                                      GParamSpec       *pspec,
                                      GtkWidget        *spinbutton)
{
  gboolean needs_sides = (options->shape_type == GIMP_SHAPE_TYPE_POLYGON ||
                          options->shape_type == GIMP_SHAPE_TYPE_STAR    ||
                          options->shape_type == GIMP_SHAPE_TYPE_SPIRAL);

  gtk_widget_set_sensitive (spinbutton, needs_sides);
}

static void
gimp_shape_options_fill_style_notify (GimpShapeOptions *options)
{
  if (options->fill_style == GIMP_CUSTOM_STYLE_SOLID_COLOR ||
      ! gimp_context_get_pattern (GIMP_CONTEXT (options->fill_options)))
    gimp_fill_options_set_style (options->fill_options,
                                 GIMP_FILL_STYLE_FG_COLOR);
  else
    gimp_fill_options_set_style (options->fill_options,
                                 GIMP_FILL_STYLE_PATTERN);


  if (options->stroke_style == GIMP_CUSTOM_STYLE_SOLID_COLOR ||
      ! gimp_context_get_pattern (GIMP_CONTEXT (options->stroke_options)))
    gimp_fill_options_set_style (GIMP_FILL_OPTIONS (options->stroke_options),
                                 GIMP_FILL_STYLE_FG_COLOR);
  else
    gimp_fill_options_set_style (GIMP_FILL_OPTIONS (options->stroke_options),
                                 GIMP_FILL_STYLE_PATTERN);
}

GtkWidget *
gimp_shape_options_gui (GimpToolOptions *tool_options)
{
  GObject          *config  = G_OBJECT (tool_options);
  GimpShapeOptions *options = GIMP_SHAPE_OPTIONS (tool_options);
  GtkWidget        *vbox    = gimp_tool_options_gui (tool_options);
  GtkWidget        *grid;
  GtkWidget        *frame;
  GtkWidget        *fill_editor;
  GtkWidget        *stroke_editor;
  GtkWidget        *button;
  GtkWidget        *label;
  GtkWidget        *scale;
  GtkWidget        *combo;
  GtkListStore     *combo_store;
  GtkSizeGroup     *size_group;

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_set_visible (grid, TRUE);

  /*  Shape Type  */
  label = gtk_label_new (_("Shape"));
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  combo_store = gimp_int_store_new (_("Line"),      GIMP_SHAPE_TYPE_LINE,
                                    _("Rectangle"), GIMP_SHAPE_TYPE_RECTANGLE,
                                    _("Ellipse"),   GIMP_SHAPE_TYPE_ARC,
                                    _("Polygon"),   GIMP_SHAPE_TYPE_POLYGON,
                                    _("Star"),      GIMP_SHAPE_TYPE_STAR,
                                    _("Spiral"),    GIMP_SHAPE_TYPE_SPIRAL,
                                    NULL);

  combo = gimp_prop_int_combo_box_new (config, "shape-type",
                                       GIMP_INT_STORE (combo_store));

  gtk_grid_attach (GTK_GRID (grid), combo, 1, 0, 1, 1);
  gtk_size_group_add_widget (size_group, combo);
  gtk_widget_set_visible (combo, TRUE);

  /* Shape mode */
  label = gtk_label_new (_("Mode"));
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  combo_store = gimp_int_store_new (_("Fill and Stroke"), GIMP_SHAPE_MODE_FILL_STROKE,
                                    _("Fill"),            GIMP_SHAPE_MODE_FILL_ONLY,
                                    _("Stroke"),          GIMP_SHAPE_MODE_STROKE_ONLY,
                                    NULL);

  combo = gimp_prop_int_combo_box_new (config, "shape-mode",
                                       GIMP_INT_STORE (combo_store));

  gtk_grid_attach (GTK_GRID (grid), combo, 1, 1, 1, 1);
  gtk_size_group_add_widget (size_group, combo);
  gtk_widget_set_visible (combo, TRUE);

  label = gtk_label_new (_("Sides"));
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  scale = gimp_prop_spin_button_new (config, "number-of-sides", 1, 5, 0);
  gtk_grid_attach (GTK_GRID (grid), scale, 1, 2, 1, 1);
  gtk_size_group_add_widget (size_group, scale);
  gtk_widget_set_visible (scale, TRUE);

  g_object_unref (size_group);

  g_signal_connect_object (config, "notify::shape-type",
                           G_CALLBACK (gimp_shape_options_shape_type_notify),
                           scale, 0);
  gimp_shape_options_shape_type_notify (options, NULL, scale);

  button = gimp_prop_check_button_new (config, "draw-on-layers", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  /* Fill settings */
  frame = gimp_frame_new (_("Fill"));

  options->fill_options = gimp_fill_options_new (GIMP_CONTEXT (options)->gimp,
                                                 NULL, FALSE);

#define FILL_BIND(a)                                 \
  g_object_bind_property (options, "fill-" #a,       \
                          options->fill_options, #a, \
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE)
  FILL_BIND (custom-style);
  FILL_BIND (foreground);
  FILL_BIND (pattern);
  FILL_BIND (antialias);

  fill_editor = gimp_fill_editor_new (options->fill_options,
                                      TRUE, TRUE);
  gtk_widget_set_visible (fill_editor, TRUE);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), fill_editor);
  gtk_widget_set_visible (frame, TRUE);

  /* Stroke settings */
  frame = gimp_frame_new (_("Stroke"));

    options->stroke_options =
      gimp_stroke_options_new (GIMP_CONTEXT (options)->gimp,
                               NULL, FALSE);

#define STROKE_BIND(a)                                 \
  g_object_bind_property (options, "stroke-" #a,       \
                          options->stroke_options, #a, \
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE)
  STROKE_BIND (custom-style);
  STROKE_BIND (foreground);
  STROKE_BIND (pattern);
  STROKE_BIND (antialias);
  STROKE_BIND (width);
  STROKE_BIND (unit);
  STROKE_BIND (cap-style);
  STROKE_BIND (join-style);
  STROKE_BIND (miter-limit);
  STROKE_BIND (dash-offset);

  stroke_editor = gimp_stroke_editor_new (options->stroke_options, 72.0,
                                          TRUE, TRUE);
  gtk_widget_set_visible (stroke_editor, TRUE);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), stroke_editor);
  gtk_widget_set_visible (frame, TRUE);

  /* Update fill and stroke styles */
  g_signal_connect_object (config, "notify::fill-custom-style",
                           G_CALLBACK (gimp_shape_options_fill_style_notify),
                           scale, 0);
  g_signal_connect_object (config, "notify::stroke-custom-style",
                           G_CALLBACK (gimp_shape_options_fill_style_notify),
                           scale, 0);
  gimp_shape_options_fill_style_notify (options);

  return vbox;
}
