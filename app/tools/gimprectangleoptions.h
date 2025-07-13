/* GIMP - The GNU Image Manipulation Program
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

#pragma once


typedef enum
{
  GIMP_RECTANGLE_OPTIONS_PROP_0,

  GIMP_RECTANGLE_OPTIONS_PROP_AUTO_SHRINK,
  GIMP_RECTANGLE_OPTIONS_PROP_SHRINK_MERGED,
  GIMP_RECTANGLE_OPTIONS_PROP_HIGHLIGHT,
  GIMP_RECTANGLE_OPTIONS_PROP_HIGHLIGHT_OPACITY,
  GIMP_RECTANGLE_OPTIONS_PROP_GUIDE,

  GIMP_RECTANGLE_OPTIONS_PROP_X,
  GIMP_RECTANGLE_OPTIONS_PROP_Y,
  GIMP_RECTANGLE_OPTIONS_PROP_WIDTH,
  GIMP_RECTANGLE_OPTIONS_PROP_HEIGHT,
  GIMP_RECTANGLE_OPTIONS_PROP_POSITION_UNIT,
  GIMP_RECTANGLE_OPTIONS_PROP_SIZE_UNIT,

  GIMP_RECTANGLE_OPTIONS_PROP_FIXED_RULE_ACTIVE,
  GIMP_RECTANGLE_OPTIONS_PROP_FIXED_RULE,
  GIMP_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_WIDTH,
  GIMP_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_HEIGHT,
  GIMP_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_SIZE_WIDTH,
  GIMP_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_SIZE_HEIGHT,
  GIMP_RECTANGLE_OPTIONS_PROP_DEFAULT_FIXED_SIZE_WIDTH,
  GIMP_RECTANGLE_OPTIONS_PROP_DEFAULT_FIXED_SIZE_HEIGHT,
  GIMP_RECTANGLE_OPTIONS_PROP_OVERRIDDEN_FIXED_SIZE,
  GIMP_RECTANGLE_OPTIONS_PROP_ASPECT_NUMERATOR,
  GIMP_RECTANGLE_OPTIONS_PROP_ASPECT_DENOMINATOR,
  GIMP_RECTANGLE_OPTIONS_PROP_DEFAULT_ASPECT_NUMERATOR,
  GIMP_RECTANGLE_OPTIONS_PROP_DEFAULT_ASPECT_DENOMINATOR,
  GIMP_RECTANGLE_OPTIONS_PROP_OVERRIDDEN_FIXED_ASPECT,
  GIMP_RECTANGLE_OPTIONS_PROP_USE_STRING_CURRENT,
  GIMP_RECTANGLE_OPTIONS_PROP_FIXED_UNIT,
  GIMP_RECTANGLE_OPTIONS_PROP_FIXED_CENTER,

  GIMP_RECTANGLE_OPTIONS_PROP_LAST = GIMP_RECTANGLE_OPTIONS_PROP_FIXED_CENTER
} GimpRectangleOptionsProp;


#define GIMP_TYPE_RECTANGLE_OPTIONS               (gimp_rectangle_options_get_type ())
#define GIMP_IS_RECTANGLE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_RECTANGLE_OPTIONS))
#define GIMP_RECTANGLE_OPTIONS(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_RECTANGLE_OPTIONS, GimpRectangleOptions))
#define GIMP_RECTANGLE_OPTIONS_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GIMP_TYPE_RECTANGLE_OPTIONS, GimpRectangleOptionsInterface))

#define GIMP_RECTANGLE_OPTIONS_GET_PRIVATE(obj)   (gimp_rectangle_options_get_private (GIMP_RECTANGLE_OPTIONS (obj)))


typedef struct _GimpRectangleOptions          GimpRectangleOptions;
typedef struct _GimpRectangleOptionsInterface GimpRectangleOptionsInterface;
typedef struct _GimpRectangleOptionsPrivate   GimpRectangleOptionsPrivate;

struct _GimpRectangleOptionsInterface
{
  GTypeInterface base_iface;
};

struct _GimpRectangleOptionsPrivate
{
  gboolean                auto_shrink;
  gboolean                shrink_merged;

  gboolean                highlight;
  gdouble                 highlight_opacity;
  GimpGuidesType          guide;

  gdouble                 x;
  gdouble                 y;
  gdouble                 width;
  gdouble                 height;

  GimpUnit               *position_unit;
  GimpUnit               *size_unit;

  gboolean                fixed_rule_active;
  GimpRectangleFixedRule  fixed_rule;

  gdouble                 desired_fixed_width;
  gdouble                 desired_fixed_height;

  gdouble                 desired_fixed_size_width;
  gdouble                 desired_fixed_size_height;

  gdouble                 default_fixed_size_width;
  gdouble                 default_fixed_size_height;
  gboolean                overridden_fixed_size;

  gdouble                 aspect_numerator;
  gdouble                 aspect_denominator;

  gdouble                 default_aspect_numerator;
  gdouble                 default_aspect_denominator;
  gboolean                overridden_fixed_aspect;

  gboolean                fixed_center;

  /* This gboolean is not part of the actual rectangle tool options,
   * and should be refactored out along with the pointers to widgets.
   */
  gboolean                use_string_current;

  GimpUnit               *fixed_unit;

  /* options gui */

  GtkWidget              *auto_shrink_button;

  GtkWidget              *fixed_width_entry;
  GtkWidget              *fixed_height_entry;

  GtkWidget              *fixed_aspect_hbox;
  GtkWidget              *aspect_button_box;
  GtkListStore           *aspect_history;

  GtkWidget              *fixed_size_hbox;
  GtkWidget              *size_button_box;
  GtkListStore           *size_history;

  GtkWidget              *position_entry;
  GtkWidget              *size_entry;
};


GType       gimp_rectangle_options_get_type           (void) G_GNUC_CONST;

GtkWidget * gimp_rectangle_options_gui                (GimpToolOptions      *tool_options);

void        gimp_rectangle_options_connect            (GimpRectangleOptions *options,
                                                       GimpImage            *image,
                                                       GCallback             shrink_callback,
                                                       gpointer              shrink_object);
void        gimp_rectangle_options_disconnect         (GimpRectangleOptions *options,
                                                       GCallback             shrink_callback,
                                                       gpointer              shrink_object);

gboolean    gimp_rectangle_options_fixed_rule_active  (GimpRectangleOptions *rectangle_options,
                                                       GimpRectangleFixedRule fixed_rule);

GimpRectangleOptionsPrivate *
            gimp_rectangle_options_get_private        (GimpRectangleOptions *options);


/*  convenience functions  */

void        gimp_rectangle_options_install_properties (GObjectClass *klass);
void        gimp_rectangle_options_set_property       (GObject      *object,
                                                       guint         property_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec);
void        gimp_rectangle_options_get_property       (GObject      *object,
                                                       guint         property_id,
                                                       GValue       *value,
                                                       GParamSpec   *pspec);


/*  testing helper functions  */

GtkWidget * gimp_rectangle_options_get_size_entry     (GimpRectangleOptions *rectangle_options);
