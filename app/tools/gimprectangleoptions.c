/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimptooloptions.h"

#include "gimprectangleoptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


#define GIMP_RECTANGLE_OPTIONS_GET_PRIVATE(obj) (gimp_rectangle_options_get_private ((GimpRectangleOptions *) (obj)))

typedef struct _GimpRectangleOptionsPrivate GimpRectangleOptionsPrivate;

struct _GimpRectangleOptionsPrivate
{
  gboolean highlight;
  GimpRectangleGuide guide;

  gboolean fixed_width;
  gdouble  width;

  gboolean fixed_height;
  gdouble  height;

  gboolean fixed_aspect;
  gdouble  aspect;
  gboolean aspect_square;        /* if set, overrides fixed-aspect setting */

  gboolean fixed_center;
  gdouble  center_x;
  gdouble  center_y;

  GimpUnit unit;

  gboolean controls_expanded;    /* show entries for width, height, etc    */

  GtkWidget *dimensions_entry;
};

static void gimp_rectangle_options_iface_base_init     (GimpRectangleOptionsInterface *rectangle_options_iface);

static GimpRectangleOptionsPrivate *
            gimp_rectangle_options_get_private         (GimpRectangleOptions *options);

gboolean    gimp_rectangle_options_get_highlight       (GimpRectangleOptions *options);
void        gimp_rectangle_options_set_guide           (GimpRectangleOptions *options,
                                                        GimpRectangleGuide    guide);
GimpRectangleGuide
            gimp_rectangle_options_get_guide           (GimpRectangleOptions *options);

void        gimp_rectangle_options_set_fixed_width     (GimpRectangleOptions *options,
                                                        gboolean              fixed_width);
gboolean    gimp_rectangle_options_get_fixed_width     (GimpRectangleOptions *options);
void        gimp_rectangle_options_set_width           (GimpRectangleOptions *options,
                                                        gdouble               width);
gdouble     gimp_rectangle_options_get_width           (GimpRectangleOptions *options);

void        gimp_rectangle_options_set_fixed_height    (GimpRectangleOptions *options,
                                                        gboolean              fixed_height);
gboolean    gimp_rectangle_options_get_fixed_height    (GimpRectangleOptions *options);
void        gimp_rectangle_options_set_height          (GimpRectangleOptions *options,
                                                        gdouble               height);
gdouble     gimp_rectangle_options_get_height          (GimpRectangleOptions *options);

void        gimp_rectangle_options_set_fixed_aspect    (GimpRectangleOptions *options,
                                                        gboolean              fixed_aspect);
gboolean    gimp_rectangle_options_get_fixed_aspect    (GimpRectangleOptions *options);
void        gimp_rectangle_options_set_aspect          (GimpRectangleOptions *options,
                                                        gdouble               aspect);
gdouble     gimp_rectangle_options_get_aspect          (GimpRectangleOptions *options);
void        gimp_rectangle_options_set_aspect_square   (GimpRectangleOptions *options,
                                                        gboolean              square);
gboolean    gimp_rectangle_options_get_aspect_square   (GimpRectangleOptions *options);

void        gimp_rectangle_options_set_fixed_center    (GimpRectangleOptions *options,
                                                        gboolean              fixed_center);
gboolean    gimp_rectangle_options_get_fixed_center    (GimpRectangleOptions *options);
void        gimp_rectangle_options_set_center_x        (GimpRectangleOptions *options,
                                                        gdouble               center_x);
gdouble     gimp_rectangle_options_get_center_x        (GimpRectangleOptions *options);
void        gimp_rectangle_options_set_center_y        (GimpRectangleOptions *options,
                                                        gdouble               center_y);
gdouble     gimp_rectangle_options_get_center_y        (GimpRectangleOptions *options);

void        gimp_rectangle_options_set_unit            (GimpRectangleOptions *options,
                                                        GimpUnit              unit);
GimpUnit    gimp_rectangle_options_get_unit            (GimpRectangleOptions *options);

void        gimp_rectangle_options_set_controls_expanded (GimpRectangleOptions *options,
                                                          gboolean              expanded);
gboolean    gimp_rectangle_options_get_controls_expanded (GimpRectangleOptions *options);

void        gimp_rectangle_options_set_entry           (GimpRectangleOptions *options,
                                                        GtkWidget            *entry);
GtkWidget * gimp_rectangle_options_get_entry           (GimpRectangleOptions *options);

static void rectangle_dimensions_changed               (GtkWidget            *widget,
                                                        GimpRectangleOptions *options);


GType
gimp_rectangle_options_interface_get_type (void)
{
  static GType iface_type = 0;

  if (!iface_type)
    {
      static const GTypeInfo iface_info =
      {
        sizeof (GimpRectangleOptionsInterface),
        (GBaseInitFunc)     gimp_rectangle_options_iface_base_init,
        (GBaseFinalizeFunc) NULL,
      };

      iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                           "GimpRectangleOptionsInterface",
                                           &iface_info, 0);

      g_type_interface_add_prerequisite (iface_type, GIMP_TYPE_TOOL_OPTIONS);
    }

  return iface_type;
}

static void
gimp_rectangle_options_iface_base_init (GimpRectangleOptionsInterface *iface)
{
  static gboolean initialized = FALSE;

  if (! initialized)
    {
      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("highlight",
                                                                 NULL, NULL,
                                                                 TRUE,
                                                                 GIMP_CONFIG_PARAM_FLAGS |
                                                                 GIMP_PARAM_STATIC_STRINGS));
      g_object_interface_install_property (iface,
                                           g_param_spec_enum ("guide",
                                                              NULL, NULL,
                                                              GIMP_TYPE_RECTANGLE_GUIDE,
                                                              GIMP_RECTANGLE_GUIDE_NONE,
                                                              GIMP_CONFIG_PARAM_FLAGS |
                                                              GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("new-fixed-width",
                                                                 NULL, NULL,
                                                                 FALSE,
                                                                 GIMP_CONFIG_PARAM_FLAGS |
                                                                 GIMP_PARAM_STATIC_STRINGS));
      g_object_interface_install_property (iface,
                                           g_param_spec_double ("width",
                                                                NULL, NULL,
                                                                0.0, GIMP_MAX_IMAGE_SIZE,
                                                                0.0,
                                                                GIMP_CONFIG_PARAM_FLAGS |
                                                                GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("new-fixed-height",
                                                                 NULL, NULL,
                                                                 FALSE,
                                                                 GIMP_CONFIG_PARAM_FLAGS |
                                                                 GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("height",
                                                                NULL, NULL,
                                                                0.0, GIMP_MAX_IMAGE_SIZE,
                                                                0.0,
                                                                GIMP_CONFIG_PARAM_FLAGS |
                                                                GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("fixed-aspect",
                                                                 NULL, NULL,
                                                                 FALSE,
                                                                 GIMP_CONFIG_PARAM_FLAGS |
                                                                 GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("aspect",
                                                                NULL, NULL,
                                                                0.0, GIMP_MAX_IMAGE_SIZE,
                                                                0.0,
                                                                GIMP_CONFIG_PARAM_FLAGS |
                                                                GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("aspect-square",
                                                                 NULL, NULL,
                                                                 FALSE,
                                                                 GIMP_CONFIG_PARAM_FLAGS |
                                                                 GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("fixed-center",
                                                                 NULL, NULL,
                                                                 FALSE,
                                                                 GIMP_CONFIG_PARAM_FLAGS |
                                                                 GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("center-x",
                                                                NULL, NULL,
                                                                -GIMP_MAX_IMAGE_SIZE,
                                                                GIMP_MAX_IMAGE_SIZE,
                                                                0.0,
                                                                GIMP_CONFIG_PARAM_FLAGS |
                                                                GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_double ("center-y",
                                                                NULL, NULL,
                                                                -GIMP_MAX_IMAGE_SIZE,
                                                                GIMP_MAX_IMAGE_SIZE,
                                                                0.0,
                                                                GIMP_CONFIG_PARAM_FLAGS |
                                                                GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           gimp_param_spec_unit ("unit",
                                                                 NULL, NULL,
                                                                 TRUE, TRUE,
                                                                 GIMP_UNIT_PIXEL,
                                                                 GIMP_CONFIG_PARAM_FLAGS |
                                                                 GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("controls-expanded",
                                                                 NULL, NULL,
                                                                 FALSE,
                                                                 GIMP_CONFIG_PARAM_FLAGS |
                                                                 GIMP_PARAM_STATIC_STRINGS));

      g_object_interface_install_property (iface,
                                           g_param_spec_object ("dimensions-entry",
                                                                NULL, NULL,
                                                                GTK_TYPE_WIDGET,
                                                                GIMP_PARAM_READWRITE));

      initialized = TRUE;
    }
}

/*static void
gimp_rectangle_options_private_dispose (GimpRectangleOptions        *options,
                                        GimpRectangleOptionsPrivate *private)
{
}*/

static void
gimp_rectangle_options_private_finalize (GimpRectangleOptionsPrivate *private)
{
  g_free (private);
}

static GimpRectangleOptionsPrivate *
gimp_rectangle_options_get_private (GimpRectangleOptions *options)
{
  GimpRectangleOptionsPrivate *private;

  static GQuark private_key = 0;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), NULL);

  if (! private_key)
    private_key = g_quark_from_static_string ("gimp-rectangle-options-private");

  private = g_object_get_qdata (G_OBJECT (options), private_key);

  if (! private)
    {
      private = g_new0 (GimpRectangleOptionsPrivate, 1);

      g_object_set_qdata_full (G_OBJECT (options), private_key, private,
                               (GDestroyNotify) gimp_rectangle_options_private_finalize);

      /*g_signal_connect (options, "destroy",
                        G_CALLBACK (gimp_rectangle_options_private_dispose),
                        private);*/
    }

  return private;
}

/**
 * gimp_rectangle_options_install_properties:
 * @klass: the class structure for a type deriving from #GObject
 *
 * Installs the necessary properties for a class implementing
 * #GimpRectangleOptions. A #GimpRectangleOptionsProp property is installed
 * for each property, using the values from the #GimpRectangleOptionsProp
 * enumeration. The caller must make sure itself that the enumeration
 * values don't collide with some other property values they
 * are using (that's what %GIMP_RECTANGLE_OPTIONS_PROP_LAST is good for).
 **/
void
gimp_rectangle_options_install_properties (GObjectClass *klass)
{
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_HIGHLIGHT,
                                    "highlight");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_GUIDE,
                                    "guide");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_FIXED_WIDTH,
                                    "new-fixed-width");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_WIDTH,
                                    "width");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_FIXED_HEIGHT,
                                    "new-fixed-height");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_HEIGHT,
                                    "height");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_FIXED_ASPECT,
                                    "fixed-aspect");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_ASPECT,
                                    "aspect");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_ASPECT_SQUARE,
                                    "aspect-square");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_FIXED_CENTER,
                                    "fixed-center");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_CENTER_X,
                                    "center-x");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_CENTER_Y,
                                    "center-y");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_UNIT,
                                    "unit");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_CONTROLS_EXPANDED,
                                    "controls-expanded");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_OPTIONS_PROP_DIMENSIONS_ENTRY,
                                    "dimensions-entry");
}

void
gimp_rectangle_options_set_highlight (GimpRectangleOptions *options,
                                      gboolean              highlight)
{
  GimpRectangleOptionsPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  private->highlight = highlight;
  g_object_notify (G_OBJECT (options), "highlight");
}

gboolean
gimp_rectangle_options_get_highlight (GimpRectangleOptions *options)
{
  GimpRectangleOptionsPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), FALSE);

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  return private->highlight;
}

void
gimp_rectangle_options_set_guide (GimpRectangleOptions *options,
                                  GimpRectangleGuide    guide)
{
  GimpRectangleOptionsPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  private->guide = guide;
  g_object_notify (G_OBJECT (options), "guide");
}

GimpRectangleGuide
gimp_rectangle_options_get_guide (GimpRectangleOptions *options)
{
  GimpRectangleOptionsPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), FALSE);

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  return private->guide;
}

void
gimp_rectangle_options_set_fixed_width (GimpRectangleOptions *options,
                                        gboolean              fixed_width)
{
  GimpRectangleOptionsPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  private->fixed_width = fixed_width;
  g_object_notify (G_OBJECT (options), "new-fixed-width");
}

gboolean
gimp_rectangle_options_get_fixed_width (GimpRectangleOptions *options)
{
  GimpRectangleOptionsPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), FALSE);

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  return private->fixed_width;
}

void
gimp_rectangle_options_set_width (GimpRectangleOptions *options,
                                  gdouble               width)
{
  GimpRectangleOptionsPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  private->width = width;
  g_object_notify (G_OBJECT (options), "width");
}

gdouble
gimp_rectangle_options_get_width (GimpRectangleOptions *options)
{
  GimpRectangleOptionsPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), 0);

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  return private->width;
}

void
gimp_rectangle_options_set_fixed_height (GimpRectangleOptions *options,
                                         gboolean              fixed_height)
{
  GimpRectangleOptionsPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  private->fixed_height = fixed_height;
  g_object_notify (G_OBJECT (options), "new-fixed-height");
}

gboolean
gimp_rectangle_options_get_fixed_height (GimpRectangleOptions *options)
{
  GimpRectangleOptionsPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), FALSE);

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  return private->fixed_height;
}

void
gimp_rectangle_options_set_height (GimpRectangleOptions *options,
                                   gdouble               height)
{
  GimpRectangleOptionsPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  private->height = height;
  g_object_notify (G_OBJECT (options), "height");
}

gdouble
gimp_rectangle_options_get_height (GimpRectangleOptions *options)
{
  GimpRectangleOptionsPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), 0);

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  return private->height;
}

void
gimp_rectangle_options_set_fixed_aspect (GimpRectangleOptions *options,
                                         gboolean              fixed_aspect)
{
  GimpRectangleOptionsPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  private->fixed_aspect = fixed_aspect;
  g_object_notify (G_OBJECT (options), "fixed-aspect");
}

gboolean
gimp_rectangle_options_get_fixed_aspect (GimpRectangleOptions *options)
{
  GimpRectangleOptionsPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), FALSE);

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  return private->fixed_aspect;
}

void
gimp_rectangle_options_set_aspect (GimpRectangleOptions *options,
                                   gdouble               aspect)
{
  GimpRectangleOptionsPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  private->aspect = aspect;
  g_object_notify (G_OBJECT (options), "aspect");
}

gdouble
gimp_rectangle_options_get_aspect (GimpRectangleOptions *options)
{
  GimpRectangleOptionsPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), 0);

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  return private->aspect;
}

void
gimp_rectangle_options_set_aspect_square (GimpRectangleOptions *options,
                                          gboolean              square)
{
  GimpRectangleOptionsPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  private->aspect_square = square;
  g_object_notify (G_OBJECT (options), "aspect-square");
}

gboolean
gimp_rectangle_options_get_aspect_square (GimpRectangleOptions *options)
{
  GimpRectangleOptionsPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), FALSE);

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  return private->aspect_square;
}

void
gimp_rectangle_options_set_fixed_center (GimpRectangleOptions *options,
                                         gboolean              fixed_center)
{
  GimpRectangleOptionsPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  private->fixed_center = fixed_center;
  g_object_notify (G_OBJECT (options), "fixed-center");
}

gboolean
gimp_rectangle_options_get_fixed_center (GimpRectangleOptions *options)
{
  GimpRectangleOptionsPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), FALSE);

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  return private->fixed_center;
}

void
gimp_rectangle_options_set_center_x (GimpRectangleOptions *options,
                                     gdouble               center_x)
{
  GimpRectangleOptionsPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  private->center_x = center_x;
  g_object_notify (G_OBJECT (options), "center-x");
}

gdouble
gimp_rectangle_options_get_center_x (GimpRectangleOptions *options)
{
  GimpRectangleOptionsPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), 0);

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  return private->center_x;
}

void
gimp_rectangle_options_set_center_y (GimpRectangleOptions *options,
                                     gdouble               center_y)
{
  GimpRectangleOptionsPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  private->center_y = center_y;
  g_object_notify (G_OBJECT (options), "center-y");
}

gdouble
gimp_rectangle_options_get_center_y (GimpRectangleOptions *options)
{
  GimpRectangleOptionsPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), 0);

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  return private->center_y;
}

void
gimp_rectangle_options_set_unit (GimpRectangleOptions *options,
                                 GimpUnit              unit)
{
  GimpRectangleOptionsPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  private->unit = unit;
  g_object_notify (G_OBJECT (options), "unit");
}

GimpUnit
gimp_rectangle_options_get_unit (GimpRectangleOptions *options)
{
  GimpRectangleOptionsPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), GIMP_UNIT_PIXEL);

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  return private->unit;
}

void
gimp_rectangle_options_set_controls_expanded (GimpRectangleOptions *options,
                                              gboolean              expanded)
{
  GimpRectangleOptionsPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  private->controls_expanded = expanded;
  g_object_notify (G_OBJECT (options), "controls-expanded");
}

gboolean
gimp_rectangle_options_get_controls_expanded (GimpRectangleOptions *options)
{
  GimpRectangleOptionsPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), FALSE);

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  return private->controls_expanded;
}

void
gimp_rectangle_options_set_entry (GimpRectangleOptions *options,
                                  GtkWidget            *entry)
{
  GimpRectangleOptionsPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options));

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  private->dimensions_entry = entry;
  g_object_notify (G_OBJECT (options), "dimensions-entry");
}

GtkWidget *
gimp_rectangle_options_get_entry (GimpRectangleOptions *options)
{
  GimpRectangleOptionsPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_OPTIONS (options), NULL);

  private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  return private->dimensions_entry;
}

void
gimp_rectangle_options_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpRectangleOptions *options = GIMP_RECTANGLE_OPTIONS (object);

  switch (property_id)
    {
    case GIMP_RECTANGLE_OPTIONS_PROP_HIGHLIGHT:
      gimp_rectangle_options_set_highlight (options, g_value_get_boolean (value));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_GUIDE:
      gimp_rectangle_options_set_guide (options, g_value_get_enum (value));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_WIDTH:
      gimp_rectangle_options_set_fixed_width (options, g_value_get_boolean (value));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_WIDTH:
      gimp_rectangle_options_set_width (options, g_value_get_double (value));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_HEIGHT:
      gimp_rectangle_options_set_fixed_height (options, g_value_get_boolean (value));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_HEIGHT:
      gimp_rectangle_options_set_height (options, g_value_get_double (value));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_ASPECT:
      gimp_rectangle_options_set_fixed_aspect (options, g_value_get_boolean (value));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_ASPECT:
      gimp_rectangle_options_set_aspect (options, g_value_get_double (value));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_ASPECT_SQUARE:
      gimp_rectangle_options_set_aspect_square (options, g_value_get_boolean (value));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_CENTER:
      gimp_rectangle_options_set_fixed_center (options, g_value_get_boolean (value));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_CENTER_X:
      gimp_rectangle_options_set_center_x (options, g_value_get_double (value));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_CENTER_Y:
      gimp_rectangle_options_set_center_y (options, g_value_get_double (value));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_UNIT:
      gimp_rectangle_options_set_unit (options, g_value_get_int (value));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_CONTROLS_EXPANDED:
      gimp_rectangle_options_set_controls_expanded (options, g_value_get_boolean (value));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_DIMENSIONS_ENTRY:
      gimp_rectangle_options_set_entry (options, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_rectangle_options_get_property (GObject      *object,
                                     guint         property_id,
                                     GValue       *value,
                                     GParamSpec   *pspec)
{
  GimpRectangleOptions *options = GIMP_RECTANGLE_OPTIONS (object);

  switch (property_id)
    {
    case GIMP_RECTANGLE_OPTIONS_PROP_HIGHLIGHT:
      g_value_set_boolean (value, gimp_rectangle_options_get_highlight (options));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_GUIDE:
      g_value_set_enum (value, gimp_rectangle_options_get_guide (options));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_WIDTH:
      g_value_set_boolean (value, gimp_rectangle_options_get_fixed_width (options));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_WIDTH:
      g_value_set_double (value, gimp_rectangle_options_get_width (options));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_HEIGHT:
      g_value_set_boolean (value, gimp_rectangle_options_get_fixed_height (options));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_HEIGHT:
      g_value_set_double (value, gimp_rectangle_options_get_height (options));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_ASPECT:
      g_value_set_boolean (value, gimp_rectangle_options_get_fixed_aspect (options));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_ASPECT:
      g_value_set_double (value, gimp_rectangle_options_get_aspect (options));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_ASPECT_SQUARE:
      g_value_set_boolean (value, gimp_rectangle_options_get_aspect_square (options));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_FIXED_CENTER:
      g_value_set_boolean (value, gimp_rectangle_options_get_fixed_center (options));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_CENTER_X:
      g_value_set_double (value, gimp_rectangle_options_get_center_x (options));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_CENTER_Y:
      g_value_set_double (value, gimp_rectangle_options_get_center_y (options));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_UNIT:
      g_value_set_int (value, gimp_rectangle_options_get_unit (options));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_CONTROLS_EXPANDED:
      g_value_set_boolean (value, gimp_rectangle_options_get_controls_expanded (options));
      break;
    case GIMP_RECTANGLE_OPTIONS_PROP_DIMENSIONS_ENTRY:
      g_value_set_object (value, gimp_rectangle_options_get_entry (options));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_rectangle_options_gui (GimpToolOptions *tool_options)
{
  GObject     *config  = G_OBJECT (tool_options);
  GtkWidget   *vbox    = gimp_tool_options_gui (tool_options);
  GtkWidget   *button;
  GtkWidget   *combo;
  GtkWidget   *table;
  GtkWidget   *entry;
  GtkWidget   *hbox;
  GtkWidget   *label;
  GtkWidget   *spinbutton;
  GtkWidget   *vbox2;
  GtkWidget   *expander;
  GtkObject   *adjustment;

  button = gimp_prop_check_button_new (config, "fixed-center",
                                       _("Expand from center"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_prop_check_button_new (config, "aspect-square",
                                       _("Make square"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_prop_check_button_new (config, "highlight",
                                       _("Highlight"));
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  combo = gimp_prop_enum_combo_box_new (config, "guide", 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  expander = gimp_prop_expander_new (config, "controls-expanded",
                                     _("Rectangle Controls"));
  gtk_box_pack_start (GTK_BOX (vbox), expander, FALSE, FALSE, 0);
  gtk_widget_show (expander);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (expander), vbox2);
  gtk_widget_show (vbox2);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);

  label = gtk_label_new (_("Fix"));
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_widget_show (label);

  button = gimp_prop_check_button_new (config, "new-fixed-width",
                                       _("Width"));
  gtk_widget_show (button);
  gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 1, 2);
  entry = gimp_prop_size_entry_new (config, "width", "unit", "%a",
                                    GIMP_SIZE_ENTRY_UPDATE_SIZE, 300);
  gimp_size_entry_show_unit_menu (GIMP_SIZE_ENTRY (entry), FALSE);
  gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 1, 2);
  gtk_widget_show (entry);

  button = gimp_prop_check_button_new (config, "new-fixed-height",
                                       _("Height"));
  gtk_widget_show (button);
  gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 2, 3);
  entry = gimp_prop_size_entry_new (config, "height", "unit", "%a",
                                    GIMP_SIZE_ENTRY_UPDATE_SIZE, 300);
  gimp_size_entry_show_unit_menu (GIMP_SIZE_ENTRY (entry), FALSE);
  gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, 2, 3);
  gtk_widget_show (entry);

  button = gimp_prop_check_button_new (config, "fixed-aspect",
                                       _("Aspect"));
  gtk_widget_show (button);
  gtk_table_attach_defaults (GTK_TABLE (table), button, 0, 1, 3, 4);
  spinbutton = gimp_prop_spin_button_new (config, "aspect", 0.01, 0.1, 4);
  gtk_table_attach_defaults (GTK_TABLE (table), spinbutton, 1, 2, 3, 4);
  gtk_widget_show (spinbutton);

  gtk_widget_show (table);

  entry = gimp_size_entry_new (2, GIMP_UNIT_PIXEL, "%a",
                               TRUE, TRUE, FALSE, 4,
                               GIMP_SIZE_ENTRY_UPDATE_SIZE);
  g_object_set (tool_options, "dimensions-entry", entry, NULL);

  gtk_table_set_row_spacings (GTK_TABLE (entry), 3);
  gtk_table_set_col_spacings (GTK_TABLE (entry), 3);

  spinbutton = gimp_spin_button_new (&adjustment, 1, 0, 1, 1, 10, 1, 1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 4);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), spinbutton, 1, 2, 2, 3);
  gtk_widget_show (spinbutton);

  spinbutton = gimp_spin_button_new (&adjustment, 1, 0, 1, 1, 10, 1, 1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 4);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), spinbutton, 2, 3, 2, 3);
  gtk_widget_show (spinbutton);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry), _("1 "), 1, 0, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry), _("2 "), 2, 0, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry), _("X"), 0, 1, 0.5);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry), _("Y"), 0, 2, 0.5);

  gtk_box_pack_start (GTK_BOX (vbox2), entry, FALSE, FALSE, 0);

  g_signal_connect (entry, "value-changed",
                    G_CALLBACK (rectangle_dimensions_changed),
                    tool_options);

  gtk_widget_show (entry);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 0, 0);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 1, 0);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 2, 0);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 3, 0);

  return vbox;
}

static void
rectangle_dimensions_changed (GtkWidget            *widget,
                              GimpRectangleOptions *options)
{
  g_object_notify (G_OBJECT (options), "dimensions-entry");
}
