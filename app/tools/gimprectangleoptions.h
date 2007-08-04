/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_RECTANGLE_OPTIONS_H__
#define __GIMP_RECTANGLE_OPTIONS_H__


typedef enum
{
  GIMP_RECTANGLE_OPTIONS_PROP_0,
  GIMP_RECTANGLE_OPTIONS_PROP_AUTO_SHRINK,
  GIMP_RECTANGLE_OPTIONS_PROP_SHRINK_MERGED,
  GIMP_RECTANGLE_OPTIONS_PROP_HIGHLIGHT,
  GIMP_RECTANGLE_OPTIONS_PROP_GUIDE,
  GIMP_RECTANGLE_OPTIONS_PROP_X0,
  GIMP_RECTANGLE_OPTIONS_PROP_Y0,
  GIMP_RECTANGLE_OPTIONS_PROP_WIDTH,
  GIMP_RECTANGLE_OPTIONS_PROP_HEIGHT,
  GIMP_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_WIDTH,
  GIMP_RECTANGLE_OPTIONS_PROP_DESIRED_FIXED_HEIGHT,
  GIMP_RECTANGLE_OPTIONS_PROP_ASPECT_NUMERATOR,
  GIMP_RECTANGLE_OPTIONS_PROP_ASPECT_DENOMINATOR,
  GIMP_RECTANGLE_OPTIONS_PROP_FIXED_RULE_ACTIVE,
  GIMP_RECTANGLE_OPTIONS_PROP_FIXED_RULE,
  GIMP_RECTANGLE_OPTIONS_PROP_FIXED_CENTER,
  GIMP_RECTANGLE_OPTIONS_PROP_CENTER_X,
  GIMP_RECTANGLE_OPTIONS_PROP_CENTER_Y,
  GIMP_RECTANGLE_OPTIONS_PROP_UNIT,
  GIMP_RECTANGLE_OPTIONS_PROP_LAST = GIMP_RECTANGLE_OPTIONS_PROP_UNIT
} GimpRectangleOptionsProp;


#define GIMP_TYPE_RECTANGLE_OPTIONS               (gimp_rectangle_options_interface_get_type ())
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
  gboolean                    auto_shrink;
  gboolean                    shrink_merged;

  gboolean                    highlight;
  GimpRectangleGuide          guide;

  gdouble                     x0;
  gdouble                     y0;
  gdouble                     width;
  gdouble                     height;

  gdouble                     desired_fixed_width;
  gdouble                     desired_fixed_height;

  gdouble                     aspect_numerator;
  gdouble                     aspect_denominator;

  gboolean                    fixed_rule_active;
  GimpRectangleToolFixedRule  fixed_rule;

  gboolean                    fixed_center;
  gdouble                     center_x;
  gdouble                     center_y;

  GimpUnit                    unit;

  /* options gui */
  GtkWidget                  *auto_shrink_button;

  GtkWidget                  *second_row_hbox;
  GtkWidget                  *entry_alignment;

  GtkWidget                  *fixed_width_entry;
  GtkWidget                  *fixed_height_entry;

  GtkWidget                  *fixed_aspect_entry;
  GtkWidget                  *aspect_button_box;

  GtkWidget                  *fixed_size_entry;
  GtkWidget                  *size_button_box;

  GtkWidget                  *width_entry;
  GtkWidget                  *height_entry;

};


GType       gimp_rectangle_options_interface_get_type  (void) G_GNUC_CONST;

GtkWidget * gimp_rectangle_options_gui                 (GimpToolOptions      *tool_options);

gboolean    gimp_rectangle_options_fixed_rule_active   (GimpRectangleOptions *rectangle_options,
                                                        GimpRectangleToolFixedRule fixed_rule);

GimpRectangleOptionsPrivate *
            gimp_rectangle_options_get_private         (GimpRectangleOptions *options);


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

#endif  /* __GIMP_RECTANGLE_OPTIONS_H__ */
