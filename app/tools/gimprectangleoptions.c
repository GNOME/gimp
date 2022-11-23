/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligmaimage.h"
#include "core/ligmatooloptions.h"

#include "widgets/ligmapropwidgets.h"

#include "ligmarectangleoptions.h"
#include "ligmatooloptions-gui.h"

#include "ligma-intl.h"


#define SB_WIDTH 5


enum
{
  COLUMN_LEFT_NUMBER,
  COLUMN_RIGHT_NUMBER,
  COLUMN_TEXT,
  N_COLUMNS
};


/*  local function prototypes  */

static void     ligma_rectangle_options_fixed_rule_changed     (GtkWidget                   *combo_box,
                                                               LigmaRectangleOptionsPrivate *private);

static void     ligma_rectangle_options_string_current_updates (LigmaNumberPairEntry         *entry,
                                                               GParamSpec                  *param,
                                                               LigmaRectangleOptions        *rectangle_options);
static void     ligma_rectangle_options_setup_ratio_completion (LigmaRectangleOptions        *rectangle_options,
                                                               GtkWidget                   *entry,
                                                               GtkListStore                *history);

static gboolean ligma_number_pair_entry_history_select         (GtkEntryCompletion          *completion,
                                                               GtkTreeModel                *model,
                                                               GtkTreeIter                 *iter,
                                                               LigmaNumberPairEntry         *entry);

static void     ligma_number_pair_entry_history_add            (GtkWidget                   *entry,
                                                               GtkTreeModel                *model);


G_DEFINE_INTERFACE (LigmaRectangleOptions, ligma_rectangle_options, LIGMA_TYPE_TOOL_OPTIONS)


static void
ligma_rectangle_options_default_init (LigmaRectangleOptionsInterface *iface)
{
  g_object_interface_install_property (iface,
                                       g_param_spec_boolean ("auto-shrink",
                                                             NULL,
                                                             _("Automatically shrink to the nearest "
                                                               "rectangular shape in a layer"),
                                                             FALSE,
                                                             LIGMA_CONFIG_PARAM_FLAGS |
                                                             LIGMA_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
                                       g_param_spec_boolean ("shrink-merged",
                                                             _("Shrink merged"),
                                                             _("Use all visible layers when shrinking "
                                                               "the selection"),
                                                             FALSE,
                                                             LIGMA_CONFIG_PARAM_FLAGS |
                                                             LIGMA_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
                                       g_param_spec_enum ("guide",
                                                          NULL,
                                                          _("Composition guides such as rule of thirds"),
                                                          LIGMA_TYPE_GUIDES_TYPE,
                                                          LIGMA_GUIDES_NONE,
                                                          LIGMA_CONFIG_PARAM_FLAGS |
                                                          LIGMA_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
                                       g_param_spec_double ("x",
                                                            NULL,
                                                            _("X coordinate of top left corner"),
                                                            -LIGMA_MAX_IMAGE_SIZE,
                                                            LIGMA_MAX_IMAGE_SIZE,
                                                            0.0,
                                                            LIGMA_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT));

  g_object_interface_install_property (iface,
                                       g_param_spec_double ("y",
                                                            NULL,
                                                            _("Y coordinate of top left corner"),
                                                            -LIGMA_MAX_IMAGE_SIZE,
                                                            LIGMA_MAX_IMAGE_SIZE,
                                                            0.0,
                                                            LIGMA_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT));

  g_object_interface_install_property (iface,
                                       g_param_spec_double ("width",
                                                            NULL,
                                                            _("Width of selection"),
                                                            0.0, LIGMA_MAX_IMAGE_SIZE,
                                                            0.0,
                                                            LIGMA_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT));

  g_object_interface_install_property (iface,
                                       g_param_spec_double ("height",
                                                            NULL,
                                                            _("Height of selection"),
                                                            0.0, LIGMA_MAX_IMAGE_SIZE,
                                                            0.0,
                                                            LIGMA_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT));

  g_object_interface_install_property (iface,
                                       ligma_param_spec_unit ("position-unit",
                                                             NULL,
                                                             _("Unit of top left corner coordinate"),
                                                             TRUE, TRUE,
                                                             LIGMA_UNIT_PIXEL,
                                                             LIGMA_PARAM_READWRITE |
                                                             G_PARAM_CONSTRUCT));

  g_object_interface_install_property (iface,
                                       ligma_param_spec_unit ("size-unit",
                                                             NULL,
                                                             _("Unit of selection size"),
                                                             TRUE, TRUE,
                                                             LIGMA_UNIT_PIXEL,
                                                             LIGMA_PARAM_READWRITE |
                                                             G_PARAM_CONSTRUCT));

  g_object_interface_install_property (iface,
                                       g_param_spec_boolean ("fixed-rule-active",
                                                             NULL,
                                                             _("Enable lock of aspect ratio, "
                                                               "width, height or size"),
                                                             FALSE,
                                                             LIGMA_CONFIG_PARAM_FLAGS |
                                                             LIGMA_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
                                       g_param_spec_enum ("fixed-rule",
                                                          NULL,
                                                          _("Choose what has to be locked"),
                                                          LIGMA_TYPE_RECTANGLE_FIXED_RULE,
                                                          LIGMA_RECTANGLE_FIXED_ASPECT,
                                                          LIGMA_CONFIG_PARAM_FLAGS |
                                                          LIGMA_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
                                       g_param_spec_double ("desired-fixed-width",
                                                            NULL,
                                                            _("Custom fixed width"),
                                                            0.0, LIGMA_MAX_IMAGE_SIZE,
                                                            100.0,
                                                            LIGMA_CONFIG_PARAM_FLAGS |
                                                            LIGMA_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
                                       g_param_spec_double ("desired-fixed-height",
                                                            NULL,
                                                            _("Custom fixed height"),
                                                            0.0, LIGMA_MAX_IMAGE_SIZE,
                                                            100.0,
                                                            LIGMA_CONFIG_PARAM_FLAGS |
                                                            LIGMA_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
                                       g_param_spec_double ("desired-fixed-size-width",
                                                            NULL, NULL,
                                                            0.0, LIGMA_MAX_IMAGE_SIZE,
                                                            100.0,
                                                            LIGMA_CONFIG_PARAM_FLAGS |
                                                            LIGMA_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
                                       g_param_spec_double ("desired-fixed-size-height",
                                                            NULL, NULL,
                                                            0.0, LIGMA_MAX_IMAGE_SIZE,
                                                            100.0,
                                                            LIGMA_CONFIG_PARAM_FLAGS |
                                                            LIGMA_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
                                       g_param_spec_double ("default-fixed-size-width",
                                                            NULL, NULL,
                                                            0.0, LIGMA_MAX_IMAGE_SIZE,
                                                            100.0,
                                                            LIGMA_PARAM_READWRITE |
                                                            LIGMA_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
                                       g_param_spec_double ("default-fixed-size-height",
                                                            NULL, NULL,
                                                            0.0, LIGMA_MAX_IMAGE_SIZE,
                                                            100.0,
                                                            LIGMA_PARAM_READWRITE |
                                                            LIGMA_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
                                       g_param_spec_boolean ("overridden-fixed-size",
                                                             NULL, NULL,
                                                             FALSE,
                                                             LIGMA_CONFIG_PARAM_FLAGS |
                                                             LIGMA_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
                                       g_param_spec_double ("aspect-numerator",
                                                            NULL, NULL,
                                                            0.0, LIGMA_MAX_IMAGE_SIZE,
                                                            1.0,
                                                            LIGMA_CONFIG_PARAM_FLAGS |
                                                            LIGMA_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
                                       g_param_spec_double ("aspect-denominator",
                                                            NULL, NULL,
                                                            0.0, LIGMA_MAX_IMAGE_SIZE,
                                                            1.0,
                                                            LIGMA_CONFIG_PARAM_FLAGS |
                                                            LIGMA_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
                                       g_param_spec_double ("default-aspect-numerator",
                                                            NULL, NULL,
                                                            0.001, LIGMA_MAX_IMAGE_SIZE,
                                                            1.0,
                                                            LIGMA_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT));

  g_object_interface_install_property (iface,
                                       g_param_spec_double ("default-aspect-denominator",
                                                            NULL, NULL,
                                                            0.001, LIGMA_MAX_IMAGE_SIZE,
                                                            1.0,
                                                            LIGMA_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT));

  g_object_interface_install_property (iface,
                                       g_param_spec_boolean ("overridden-fixed-aspect",
                                                             NULL, NULL,
                                                             FALSE,
                                                             LIGMA_CONFIG_PARAM_FLAGS |
                                                             LIGMA_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
                                       g_param_spec_boolean ("use-string-current",
                                                             NULL, NULL,
                                                             FALSE,
                                                             LIGMA_PARAM_READWRITE |
                                                             LIGMA_PARAM_STATIC_STRINGS));

  g_object_interface_install_property (iface,
                                       ligma_param_spec_unit ("fixed-unit",
                                                             NULL,
                                                             _("Unit of fixed width, height or size"),
                                                             TRUE, TRUE,
                                                             LIGMA_UNIT_PIXEL,
                                                             LIGMA_PARAM_READWRITE |
                                                             G_PARAM_CONSTRUCT));

  g_object_interface_install_property (iface,
                                       g_param_spec_boolean ("fixed-center",
                                                             _("Expand from center"),
                                                             _("Expand selection from center outwards"),
                                                             FALSE,
                                                             LIGMA_CONFIG_PARAM_FLAGS |
                                                             LIGMA_PARAM_STATIC_STRINGS));
}

static void
ligma_rectangle_options_private_finalize (LigmaRectangleOptionsPrivate *private)
{
  g_clear_object (&private->aspect_history);
  g_clear_object (&private->size_history);

  g_slice_free (LigmaRectangleOptionsPrivate, private);
}

LigmaRectangleOptionsPrivate *
ligma_rectangle_options_get_private (LigmaRectangleOptions *options)
{
  LigmaRectangleOptionsPrivate *private;

  static GQuark private_key = 0;

  g_return_val_if_fail (LIGMA_IS_RECTANGLE_OPTIONS (options), NULL);

  if (! private_key)
    private_key = g_quark_from_static_string ("ligma-rectangle-options-private");

  private = g_object_get_qdata (G_OBJECT (options), private_key);

  if (! private)
    {
      private = g_slice_new0 (LigmaRectangleOptionsPrivate);

      private->aspect_history = gtk_list_store_new (N_COLUMNS,
                                                    G_TYPE_DOUBLE,
                                                    G_TYPE_DOUBLE,
                                                    G_TYPE_STRING);

      private->size_history = gtk_list_store_new (N_COLUMNS,
                                                  G_TYPE_DOUBLE,
                                                  G_TYPE_DOUBLE,
                                                  G_TYPE_STRING);

      g_object_set_qdata_full (G_OBJECT (options), private_key, private,
                               (GDestroyNotify) ligma_rectangle_options_private_finalize);
    }

  return private;
}

/**
 * ligma_rectangle_options_install_properties:
 * @klass: the class structure for a type deriving from #GObject
 *
 * Installs the necessary properties for a class implementing
 * #LigmaRectangleOptions. A #LigmaRectangleOptionsProp property is installed
 * for each property, using the values from the #LigmaRectangleOptionsProp
 * enumeration. The caller must make sure itself that the enumeration
 * values don't collide with some other property values they
 * are using (that's what %LIGMA_RECTANGLE_OPTIONS_PROP_LAST is good for).
 **/
void
ligma_rectangle_options_install_properties (GObjectClass *klass)
{
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_AUTO_SHRINK,
                                    "auto-shrink");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_SHRINK_MERGED,
                                    "shrink-merged");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_GUIDE,
                                    "guide");

  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_X,
                                    "x");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_Y,
                                    "y");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_WIDTH,
                                    "width");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_HEIGHT,
                                    "height");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_POSITION_UNIT,
                                    "position-unit");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_SIZE_UNIT,
                                    "size-unit");

  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_FIXED_RULE_ACTIVE,
                                    "fixed-rule-active");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_FIXED_RULE,
                                    "fixed-rule");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_WIDTH,
                                    "desired-fixed-width");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_HEIGHT,
                                    "desired-fixed-height");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_SIZE_WIDTH,
                                    "desired-fixed-size-width");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_SIZE_HEIGHT,
                                    "desired-fixed-size-height");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_DEFAULT_FIXED_SIZE_WIDTH,
                                    "default-fixed-size-width");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_DEFAULT_FIXED_SIZE_HEIGHT,
                                    "default-fixed-size-height");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_OVERRIDDEN_FIXED_SIZE,
                                    "overridden-fixed-size");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_ASPECT_NUMERATOR,
                                    "aspect-numerator");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_ASPECT_DENOMINATOR,
                                    "aspect-denominator");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_DEFAULT_ASPECT_NUMERATOR,
                                    "default-aspect-numerator");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_DEFAULT_ASPECT_DENOMINATOR,
                                    "default-aspect-denominator");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_OVERRIDDEN_FIXED_ASPECT,
                                    "overridden-fixed-aspect");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_USE_STRING_CURRENT,
                                    "use-string-current");
  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_FIXED_UNIT,
                                    "fixed-unit");

  g_object_class_override_property (klass,
                                    LIGMA_RECTANGLE_OPTIONS_PROP_FIXED_CENTER,
                                    "fixed-center");
}

void
ligma_rectangle_options_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  LigmaRectangleOptions        *options  = LIGMA_RECTANGLE_OPTIONS (object);
  LigmaRectangleOptionsPrivate *private;

  private = LIGMA_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  switch (property_id)
    {
    case LIGMA_RECTANGLE_OPTIONS_PROP_AUTO_SHRINK:
      private->auto_shrink = g_value_get_boolean (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_SHRINK_MERGED:
      private->shrink_merged = g_value_get_boolean (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_HIGHLIGHT:
      private->highlight = g_value_get_boolean (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_HIGHLIGHT_OPACITY:
      private->highlight_opacity = g_value_get_double (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_GUIDE:
      private->guide = g_value_get_enum (value);
      break;

    case LIGMA_RECTANGLE_OPTIONS_PROP_X:
      private->x = g_value_get_double (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_Y:
      private->y = g_value_get_double (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_WIDTH:
      private->width = g_value_get_double (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_HEIGHT:
      private->height = g_value_get_double (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_POSITION_UNIT:
      private->position_unit = g_value_get_int (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_SIZE_UNIT:
      private->size_unit = g_value_get_int (value);
      break;

    case LIGMA_RECTANGLE_OPTIONS_PROP_FIXED_RULE_ACTIVE:
      private->fixed_rule_active = g_value_get_boolean (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_FIXED_RULE:
      private->fixed_rule = g_value_get_enum (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_WIDTH:
      private->desired_fixed_width = g_value_get_double (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_HEIGHT:
      private->desired_fixed_height = g_value_get_double (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_SIZE_WIDTH:
      private->desired_fixed_size_width = g_value_get_double (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_SIZE_HEIGHT:
      private->desired_fixed_size_height = g_value_get_double (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_DEFAULT_FIXED_SIZE_WIDTH:
      private->default_fixed_size_width = g_value_get_double (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_DEFAULT_FIXED_SIZE_HEIGHT:
      private->default_fixed_size_height = g_value_get_double (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_OVERRIDDEN_FIXED_SIZE:
      private->overridden_fixed_size = g_value_get_boolean (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_ASPECT_NUMERATOR:
      private->aspect_numerator = g_value_get_double (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_ASPECT_DENOMINATOR:
      private->aspect_denominator = g_value_get_double (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_DEFAULT_ASPECT_NUMERATOR:
      private->default_aspect_numerator = g_value_get_double (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_DEFAULT_ASPECT_DENOMINATOR:
      private->default_aspect_denominator = g_value_get_double (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_OVERRIDDEN_FIXED_ASPECT:
      private->overridden_fixed_aspect = g_value_get_boolean (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_USE_STRING_CURRENT:
      private->use_string_current = g_value_get_boolean (value);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_FIXED_UNIT:
      private->fixed_unit = g_value_get_int (value);
      break;

    case LIGMA_RECTANGLE_OPTIONS_PROP_FIXED_CENTER:
      private->fixed_center = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
ligma_rectangle_options_get_property (GObject      *object,
                                     guint         property_id,
                                     GValue       *value,
                                     GParamSpec   *pspec)
{
  LigmaRectangleOptions        *options  = LIGMA_RECTANGLE_OPTIONS (object);
  LigmaRectangleOptionsPrivate *private;

  private = LIGMA_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  switch (property_id)
    {
    case LIGMA_RECTANGLE_OPTIONS_PROP_AUTO_SHRINK:
      g_value_set_boolean (value, private->auto_shrink);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_SHRINK_MERGED:
      g_value_set_boolean (value, private->shrink_merged);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_HIGHLIGHT:
      g_value_set_boolean (value, private->highlight);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_HIGHLIGHT_OPACITY:
      g_value_set_double (value, private->highlight_opacity);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_GUIDE:
      g_value_set_enum (value, private->guide);
      break;

    case LIGMA_RECTANGLE_OPTIONS_PROP_X:
      g_value_set_double (value, private->x);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_Y:
      g_value_set_double (value, private->y);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_WIDTH:
      g_value_set_double (value, private->width);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_HEIGHT:
      g_value_set_double (value, private->height);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_POSITION_UNIT:
      g_value_set_int (value, private->position_unit);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_SIZE_UNIT:
      g_value_set_int (value, private->size_unit);
      break;

    case LIGMA_RECTANGLE_OPTIONS_PROP_FIXED_RULE_ACTIVE:
      g_value_set_boolean (value, private->fixed_rule_active);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_FIXED_RULE:
      g_value_set_enum (value, private->fixed_rule);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_WIDTH:
      g_value_set_double (value, private->desired_fixed_width);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_HEIGHT:
      g_value_set_double (value, private->desired_fixed_height);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_SIZE_WIDTH:
      g_value_set_double (value, private->desired_fixed_size_width);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_SIZE_HEIGHT:
      g_value_set_double (value, private->desired_fixed_size_height);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_DEFAULT_FIXED_SIZE_WIDTH:
      g_value_set_double (value, private->default_fixed_size_width);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_DEFAULT_FIXED_SIZE_HEIGHT:
      g_value_set_double (value, private->default_fixed_size_height);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_OVERRIDDEN_FIXED_SIZE:
      g_value_set_boolean (value, private->overridden_fixed_size);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_ASPECT_NUMERATOR:
      g_value_set_double (value, private->aspect_numerator);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_ASPECT_DENOMINATOR:
      g_value_set_double (value, private->aspect_denominator);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_DEFAULT_ASPECT_NUMERATOR:
      g_value_set_double (value, private->default_aspect_numerator);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_DEFAULT_ASPECT_DENOMINATOR:
      g_value_set_double (value, private->default_aspect_denominator);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_OVERRIDDEN_FIXED_ASPECT:
      g_value_set_boolean (value, private->overridden_fixed_aspect);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_USE_STRING_CURRENT:
      g_value_set_boolean (value, private->use_string_current);
      break;
    case LIGMA_RECTANGLE_OPTIONS_PROP_FIXED_UNIT:
      g_value_set_int (value, private->fixed_unit);
      break;

    case LIGMA_RECTANGLE_OPTIONS_PROP_FIXED_CENTER:
      g_value_set_boolean (value, private->fixed_center);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * ligma_rectangle_options_get_size_entry:
 * @rectangle_options:
 *
 * Returns: GtkEntry used to enter desired size of rectangle. For
 *          testing purposes.
 **/
GtkWidget *
ligma_rectangle_options_get_size_entry (LigmaRectangleOptions *rectangle_options)
{
  LigmaRectangleOptionsPrivate *private;

  private = LIGMA_RECTANGLE_OPTIONS_GET_PRIVATE (rectangle_options);

  return private->size_entry;
}

/**
 * ligma_rectangle_options_fixed_rule_changed:
 * @combo_box:
 * @private:
 *
 * Updates tool options widgets depending on current fixed rule state.
 */
static void
ligma_rectangle_options_fixed_rule_changed (GtkWidget                   *combo_box,
                                           LigmaRectangleOptionsPrivate *private)
{
  /* Setup sensitivity for Width and Height entries */

  gtk_widget_set_sensitive (ligma_size_entry_get_help_widget (
                              LIGMA_SIZE_ENTRY (private->size_entry), 0),
                            ! (private->fixed_rule_active &&
                               (private->fixed_rule ==
                                LIGMA_RECTANGLE_FIXED_WIDTH ||
                                private->fixed_rule ==
                                LIGMA_RECTANGLE_FIXED_SIZE)));

  gtk_widget_set_sensitive (ligma_size_entry_get_help_widget (
                              LIGMA_SIZE_ENTRY (private->size_entry), 1),
                            ! (private->fixed_rule_active &&
                               (private->fixed_rule ==
                                LIGMA_RECTANGLE_FIXED_HEIGHT ||
                                private->fixed_rule ==
                                LIGMA_RECTANGLE_FIXED_SIZE)));

  /* Setup current fixed rule entries */

  gtk_widget_hide (private->fixed_width_entry);
  gtk_widget_hide (private->fixed_height_entry);
  gtk_widget_hide (private->fixed_aspect_hbox);
  gtk_widget_hide (private->fixed_size_hbox);

  switch (private->fixed_rule)
    {
    case LIGMA_RECTANGLE_FIXED_ASPECT:
      gtk_widget_show (private->fixed_aspect_hbox);
      break;

    case LIGMA_RECTANGLE_FIXED_WIDTH:
      gtk_widget_show (private->fixed_width_entry);
      break;

    case LIGMA_RECTANGLE_FIXED_HEIGHT:
      gtk_widget_show (private->fixed_height_entry);
      break;

    case LIGMA_RECTANGLE_FIXED_SIZE:
      gtk_widget_show (private->fixed_size_hbox);
      break;
    }
}

static void
ligma_rectangle_options_string_current_updates (LigmaNumberPairEntry  *entry,
                                               GParamSpec           *param,
                                               LigmaRectangleOptions *rectangle_options)
{
  LigmaRectangleOptionsPrivate *private;
  gboolean                     user_override;

  private = LIGMA_RECTANGLE_OPTIONS_GET_PRIVATE (rectangle_options);

  user_override = ligma_number_pair_entry_get_user_override (entry);

  ligma_number_pair_entry_set_default_text (entry,
                                           private->use_string_current ?
                                           /* Current, as in what is currently in use. */
                                           _("Current") : NULL);

  gtk_widget_set_sensitive (private->aspect_button_box,
                            ! private->use_string_current || user_override);
}

static GtkWidget *
ligma_rectangle_options_prop_dimension_frame_new (GObject      *config,
                                                 const gchar  *x_property_name,
                                                 const gchar  *y_property_name,
                                                 const gchar  *unit_property_name,
                                                 const gchar  *table_label,
                                                 GtkSizeGroup *label_group,
                                                 GtkWidget   **entry)
{
  LigmaUnit       unit_value;
  GtkWidget     *frame;
  GtkWidget     *hbox;
  GtkWidget     *label;
  GtkWidget     *menu;
  GtkWidget     *spinbutton;
  GtkAdjustment *adjustment;

  g_object_get (config,
                unit_property_name, &unit_value,
                NULL);

  frame = ligma_frame_new (NULL);

  /*  title  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_frame_set_label_widget (GTK_FRAME (frame), hbox);
  gtk_widget_show (hbox);

  label = gtk_label_new (table_label);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_size_group_add_widget (label_group, label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  menu = ligma_prop_unit_combo_box_new (config, unit_property_name);
  gtk_box_pack_end (GTK_BOX (hbox), menu, FALSE, FALSE, 0);

  /*  content  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  spinbutton = ligma_spin_button_new (adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), SB_WIDTH);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  *entry = ligma_size_entry_new (1, unit_value, "%a", TRUE, TRUE, FALSE,
                                SB_WIDTH, LIGMA_SIZE_ENTRY_UPDATE_SIZE);
  gtk_grid_set_column_spacing (GTK_GRID (*entry), 0);
  ligma_size_entry_show_unit_menu (LIGMA_SIZE_ENTRY (*entry), FALSE);
  ligma_size_entry_add_field (LIGMA_SIZE_ENTRY (*entry),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), *entry, FALSE, FALSE, 0);
  gtk_widget_show (*entry);

  ligma_prop_coordinates_connect (config,
                                 x_property_name, y_property_name,
                                 unit_property_name,
                                 *entry, NULL, 300, 300);

  return frame;
}

GtkWidget *
ligma_rectangle_options_gui (LigmaToolOptions *tool_options)
{
  LigmaRectangleOptionsPrivate *private;
  GObject                     *config = G_OBJECT (tool_options);
  GtkWidget                   *vbox   = ligma_tool_options_gui (tool_options);
  GtkWidget                   *button;
  GtkWidget                   *combo;
  GtkWidget                   *frame;
  GtkSizeGroup                *label_group;

  private = LIGMA_RECTANGLE_OPTIONS_GET_PRIVATE (tool_options);

  /* Fixed Center */
  button = ligma_prop_check_button_new (config, "fixed-center", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  /* Rectangle fixed-rules (e.g. aspect or width). */
  {
    GtkWidget    *vbox2;
    GtkWidget    *hbox;
    GtkWidget    *entry;
    GtkSizeGroup *size_group;
    GList        *children;

    frame = ligma_frame_new (NULL);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show (frame);

    /* Setup frame title widgets */

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_frame_set_label_widget (GTK_FRAME (frame), hbox);
    gtk_widget_show (hbox);

    button = ligma_prop_check_button_new (config, "fixed-rule-active", NULL);
    gtk_widget_destroy (gtk_bin_get_child (GTK_BIN (button)));
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, TRUE, 0);

    g_signal_connect (button, "toggled",
                      G_CALLBACK (ligma_rectangle_options_fixed_rule_changed),
                      private);

    combo = ligma_prop_enum_combo_box_new (config, "fixed-rule", 0, 0);
    ligma_int_combo_box_set_label (LIGMA_INT_COMBO_BOX (combo), _("Fixed"));
    g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
    gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);

    g_signal_connect (combo, "changed",
                      G_CALLBACK (ligma_rectangle_options_fixed_rule_changed),
                      private);

    /* Setup frame content */

    vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (frame), vbox2);
    gtk_widget_show (vbox2);

    size_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);

    /* Fixed aspect entry/buttons */
    private->fixed_aspect_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start (GTK_BOX (vbox2), private->fixed_aspect_hbox,
                        FALSE, FALSE, 0);
    gtk_size_group_add_widget (size_group, private->fixed_aspect_hbox);
    g_object_unref (size_group);
    /* don't show */

    g_object_add_weak_pointer (G_OBJECT (private->fixed_aspect_hbox),
                               (gpointer) &private->fixed_aspect_hbox);

    entry = ligma_prop_number_pair_entry_new (config,
                                             "aspect-numerator",
                                             "aspect-denominator",
                                             "default-aspect-numerator",
                                             "default-aspect-denominator",
                                             "overridden-fixed-aspect",
                                             FALSE, TRUE,
                                             ":/" "xX*",
                                             TRUE,
                                             0.001, LIGMA_MAX_IMAGE_SIZE);
    gtk_box_pack_start (GTK_BOX (private->fixed_aspect_hbox), entry,
                        TRUE, TRUE, 0);

    g_signal_connect (entry, "notify::user-override",
                      G_CALLBACK (ligma_rectangle_options_string_current_updates),
                      config);
    g_signal_connect_swapped (config, "notify::use-string-current",
                              G_CALLBACK (ligma_rectangle_options_string_current_updates),
                              entry);

    ligma_rectangle_options_setup_ratio_completion (LIGMA_RECTANGLE_OPTIONS (tool_options),
                                                   entry,
                                                   private->aspect_history);

    private->aspect_button_box =
      ligma_prop_enum_icon_box_new (G_OBJECT (entry),
                                   "aspect", "ligma", -1, -1);
    gtk_box_pack_start (GTK_BOX (private->fixed_aspect_hbox),
                        private->aspect_button_box, FALSE, FALSE, 0);

    g_object_add_weak_pointer (G_OBJECT (private->aspect_button_box),
                               (gpointer) &private->aspect_button_box);

    /* hide "square" */
    children =
      gtk_container_get_children (GTK_CONTAINER (private->aspect_button_box));
    gtk_widget_hide (children->data);
    g_list_free (children);

    /* Fixed width entry */
    private->fixed_width_entry =
      ligma_prop_size_entry_new (config,
                                "desired-fixed-width", TRUE, "fixed-unit", "%a",
                                LIGMA_SIZE_ENTRY_UPDATE_SIZE, 300);
    gtk_box_pack_start (GTK_BOX (vbox2), private->fixed_width_entry,
                        FALSE, FALSE, 0);
    gtk_size_group_add_widget (size_group, private->fixed_width_entry);
    gtk_widget_hide (private->fixed_width_entry);

    g_object_add_weak_pointer (G_OBJECT (private->fixed_width_entry),
                               (gpointer) &private->fixed_width_entry);

    /* Fixed height entry */
    private->fixed_height_entry =
      ligma_prop_size_entry_new (config,
                                "desired-fixed-height", TRUE, "fixed-unit", "%a",
                                LIGMA_SIZE_ENTRY_UPDATE_SIZE, 300);
    gtk_box_pack_start (GTK_BOX (vbox2), private->fixed_height_entry,
                        FALSE, FALSE, 0);
    gtk_size_group_add_widget (size_group, private->fixed_height_entry);
    gtk_widget_hide (private->fixed_height_entry);

    g_object_add_weak_pointer (G_OBJECT (private->fixed_height_entry),
                               (gpointer) &private->fixed_height_entry);

    /* Fixed size entry */
    private->fixed_size_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start (GTK_BOX (vbox2), private->fixed_size_hbox,
                        FALSE, FALSE, 0);
    gtk_size_group_add_widget (size_group, private->fixed_size_hbox);
    gtk_widget_hide (private->fixed_size_hbox);

    g_object_add_weak_pointer (G_OBJECT (private->fixed_size_hbox),
                               (gpointer) &private->fixed_size_hbox);

    entry = ligma_prop_number_pair_entry_new (config,
                                             "desired-fixed-size-width",
                                             "desired-fixed-size-height",
                                             "default-fixed-size-width",
                                             "default-fixed-size-height",
                                             "overridden-fixed-size",
                                             TRUE, FALSE,
                                             "xX*" ":/",
                                             FALSE,
                                             1, LIGMA_MAX_IMAGE_SIZE);
    gtk_box_pack_start (GTK_BOX (private->fixed_size_hbox), entry,
                        TRUE, TRUE, 0);

    ligma_rectangle_options_setup_ratio_completion (LIGMA_RECTANGLE_OPTIONS (tool_options),
                                                   entry,
                                                   private->size_history);

    private->size_button_box =
      ligma_prop_enum_icon_box_new (G_OBJECT (entry),
                                   "aspect", "ligma", -1, -1);
    gtk_box_pack_start (GTK_BOX (private->fixed_size_hbox),
                        private->size_button_box, FALSE, FALSE, 0);

    /* hide "square" */
    children =
      gtk_container_get_children (GTK_CONTAINER (private->size_button_box));
    gtk_widget_hide (children->data);
    g_list_free (children);
  }

  label_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  /* X, Y */
  frame = ligma_rectangle_options_prop_dimension_frame_new (config,
                                                           "x", "y",
                                                           "position-unit",
                                                           _("Position:"),
                                                           label_group,
                                                           &private->position_entry);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /* Width, Height */
  frame = ligma_rectangle_options_prop_dimension_frame_new (config,
                                                           "width", "height",
                                                           "size-unit",
                                                           _("Size:"),
                                                           label_group,
                                                           &private->size_entry);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_object_unref (label_group);

  /*  the Highlight frame  */
  {
    GtkWidget *scale;

    scale = ligma_prop_spin_scale_new (config, "highlight-opacity",
                                      0.01, 0.1, 0);
    ligma_prop_widget_set_factor (scale, 100.0, 1.0, 10.0, 1);

    frame = ligma_prop_expanding_frame_new (config, "highlight", NULL,
                                           scale, NULL);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  }

  /*  Guide  */
  combo = ligma_prop_enum_combo_box_new (config, "guide",
                                        LIGMA_GUIDES_NONE,
                                        LIGMA_GUIDES_DIAGONALS);
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);

  /*  Auto Shrink  */
  private->auto_shrink_button = gtk_button_new_with_label (_("Auto Shrink"));
  gtk_box_pack_start (GTK_BOX (vbox), private->auto_shrink_button,
                      FALSE, FALSE, 0);
  gtk_widget_set_sensitive (private->auto_shrink_button, FALSE);
  gtk_widget_show (private->auto_shrink_button);

  g_object_add_weak_pointer (G_OBJECT (private->auto_shrink_button),
                             (gpointer) &private->auto_shrink_button);

  button = ligma_prop_check_button_new (config, "shrink-merged", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  /* Setup initial fixed rule widgets */
  ligma_rectangle_options_fixed_rule_changed (NULL, private);

  return vbox;
}

void
ligma_rectangle_options_connect (LigmaRectangleOptions *options,
                                LigmaImage            *image,
                                GCallback             shrink_callback,
                                gpointer              shrink_object)
{
  LigmaRectangleOptionsPrivate *options_private;
  gdouble                      xres;
  gdouble                      yres;

  g_return_if_fail (LIGMA_IS_RECTANGLE_OPTIONS (options));
  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (shrink_callback != NULL);
  g_return_if_fail (shrink_object != NULL);

  options_private = LIGMA_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  ligma_image_get_resolution (image, &xres, &yres);

  if (options_private->fixed_width_entry)
    {
      GtkWidget *entry = options_private->fixed_width_entry;

      ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (entry), 0, xres, FALSE);
      ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (entry), 0,
                                0, ligma_image_get_width (image));
    }

  if (options_private->fixed_height_entry)
    {
      GtkWidget *entry = options_private->fixed_height_entry;

      ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (entry), 0, yres, FALSE);
      ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (entry), 0,
                                0, ligma_image_get_height (image));
    }

  if (options_private->position_entry)
    {
      GtkWidget *entry = options_private->position_entry;

      ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (entry), 0, xres, FALSE);
      ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (entry), 0,
                                0, ligma_image_get_width (image));

      ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (entry), 1, yres, FALSE);
      ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (entry), 1,
                                0, ligma_image_get_height (image));
    }

  if (options_private->size_entry)
    {
      GtkWidget *entry = options_private->size_entry;

      ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (entry), 0, xres, FALSE);
      ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (entry), 0,
                                0, ligma_image_get_width (image));

      ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (entry), 1, yres, FALSE);
      ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (entry), 1,
                                0, ligma_image_get_height (image));
    }

  if (options_private->auto_shrink_button)
    {
      g_signal_connect_swapped (options_private->auto_shrink_button, "clicked",
                                shrink_callback,
                                shrink_object);

      gtk_widget_set_sensitive (options_private->auto_shrink_button, TRUE);
    }
}

void
ligma_rectangle_options_disconnect (LigmaRectangleOptions *options,
                                   GCallback             shrink_callback,
                                   gpointer              shrink_object)
{
  LigmaRectangleOptionsPrivate *options_private;

  g_return_if_fail (LIGMA_IS_RECTANGLE_OPTIONS (options));
  g_return_if_fail (shrink_callback != NULL);
  g_return_if_fail (shrink_object != NULL);

  options_private = LIGMA_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  if (options_private->auto_shrink_button)
    {
      gtk_widget_set_sensitive (options_private->auto_shrink_button, FALSE);

      g_signal_handlers_disconnect_by_func (options_private->auto_shrink_button,
                                            shrink_callback,
                                            shrink_object);
    }
}

/**
 * ligma_rectangle_options_fixed_rule_active:
 * @rectangle_options:
 * @fixed_rule:
 *
 * Returns: %TRUE if @fixed_rule is active, %FALSE otherwise.
 */
gboolean
ligma_rectangle_options_fixed_rule_active (LigmaRectangleOptions   *rectangle_options,
                                          LigmaRectangleFixedRule  fixed_rule)
{
  LigmaRectangleOptionsPrivate *private;

  g_return_val_if_fail (LIGMA_IS_RECTANGLE_OPTIONS (rectangle_options), FALSE);

  private = LIGMA_RECTANGLE_OPTIONS_GET_PRIVATE (rectangle_options);

  return private->fixed_rule_active &&
         private->fixed_rule == fixed_rule;
}

static void
ligma_rectangle_options_setup_ratio_completion (LigmaRectangleOptions *rectangle_options,
                                               GtkWidget            *entry,
                                               GtkListStore         *history)
{
  GtkEntryCompletion *completion;

  completion = g_object_new (GTK_TYPE_ENTRY_COMPLETION,
                             "model", history,
                             "inline-completion", TRUE,
                             NULL);

  gtk_entry_completion_set_text_column (completion, COLUMN_TEXT);
  gtk_entry_set_completion (GTK_ENTRY (entry), completion);
  g_object_unref (completion);

  g_signal_connect (entry, "ratio-changed",
                    G_CALLBACK (ligma_number_pair_entry_history_add),
                    history);

  g_signal_connect (completion, "match-selected",
                    G_CALLBACK (ligma_number_pair_entry_history_select),
                    entry);
}

static gboolean
ligma_number_pair_entry_history_select (GtkEntryCompletion  *completion,
                                       GtkTreeModel        *model,
                                       GtkTreeIter         *iter,
                                       LigmaNumberPairEntry *entry)
{
  gdouble left_number;
  gdouble right_number;

  gtk_tree_model_get (model, iter,
                      COLUMN_LEFT_NUMBER,  &left_number,
                      COLUMN_RIGHT_NUMBER, &right_number,
                      -1);

  ligma_number_pair_entry_set_values (entry, left_number, right_number);

  return TRUE;
}

static void
ligma_number_pair_entry_history_add (GtkWidget    *entry,
                                    GtkTreeModel *model)
{
  GValue       value = G_VALUE_INIT;
  GtkTreeIter  iter;
  gboolean     iter_valid;
  gdouble      left_number;
  gdouble      right_number;
  const gchar *text;

  text = gtk_entry_get_text (GTK_ENTRY (entry));
  ligma_number_pair_entry_get_values (LIGMA_NUMBER_PAIR_ENTRY (entry),
                                     &left_number,
                                     &right_number);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gtk_tree_model_get_value (model, &iter, COLUMN_TEXT, &value);

      if (strcmp (text, g_value_get_string (&value)) == 0)
        {
          g_value_unset (&value);
          break;
        }

      g_value_unset (&value);
    }

  if (iter_valid)
    {
      gtk_list_store_move_after (GTK_LIST_STORE (model), &iter, NULL);
    }
  else
    {
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);

      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          COLUMN_LEFT_NUMBER,  left_number,
                          COLUMN_RIGHT_NUMBER, right_number,
                          COLUMN_TEXT,        text,
                          -1);

      /* FIXME: limit the size of the history */
    }
}
