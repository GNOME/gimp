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

#ifndef __GIMP_RECTANGLE_OPTIONS_H__
#define __GIMP_RECTANGLE_OPTIONS_H__


#define GIMP_TYPE_RECTANGLE_OPTIONS               (gimp_rectangle_options_interface_get_type ())
#define GIMP_IS_RECTANGLE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_RECTANGLE_OPTIONS))
#define GIMP_RECTANGLE_OPTIONS(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_RECTANGLE_OPTIONS, GimpRectangleOptions))
#define GIMP_RECTANGLE_OPTIONS_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GIMP_TYPE_RECTANGLE_OPTIONS, GimpRectangleOptionsInterface))


typedef struct _GimpRectangleOptions          GimpRectangleOptions;
typedef struct _GimpRectangleOptionsInterface GimpRectangleOptionsInterface;

struct _GimpRectangleOptionsInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  void       (* set_highlight)    (GimpRectangleOptions *options,
                                   gboolean              highlight);
  gboolean   (* get_highlight)    (GimpRectangleOptions *options);

  void       (* set_fixed_width)  (GimpRectangleOptions *options,
                                   gboolean              fixed_width);
  gboolean   (* get_fixed_width)  (GimpRectangleOptions *options);
  void       (* set_width)        (GimpRectangleOptions *options,
                                   gdouble               width);
  gdouble    (* get_width)        (GimpRectangleOptions *options);

  void       (* set_fixed_height) (GimpRectangleOptions *options,
                                   gboolean              fixed_height);
  gboolean   (* get_fixed_height) (GimpRectangleOptions *options);
  void       (* set_height)       (GimpRectangleOptions *options,
                                   gdouble               height);
  gdouble    (* get_height)       (GimpRectangleOptions *options);

  void       (* set_fixed_aspect) (GimpRectangleOptions *options,
                                   gboolean              fixed_aspect);
  gboolean   (* get_fixed_aspect) (GimpRectangleOptions *options);
  void       (* set_aspect)       (GimpRectangleOptions *options,
                                   gdouble               aspect);
  gdouble    (* get_aspect)       (GimpRectangleOptions *options);

  void       (* set_fixed_center) (GimpRectangleOptions *options,
                                   gboolean              fixed_center);
  gboolean   (* get_fixed_center) (GimpRectangleOptions *options);
  void       (* set_center_x)     (GimpRectangleOptions *options,
                                   gdouble               center_x);
  gdouble    (* get_center_x)     (GimpRectangleOptions *options);
  void       (* set_center_y)     (GimpRectangleOptions *options,
                                   gdouble               center_y);
  gdouble    (* get_center_y)     (GimpRectangleOptions *options);

  void       (* set_unit)         (GimpRectangleOptions *options,
                                   GimpUnit              unit);
  GimpUnit   (* get_unit)         (GimpRectangleOptions *options);
};


GType       gimp_rectangle_options_interface_get_type  (void) G_GNUC_CONST;

void        gimp_rectangle_options_set_highlight       (GimpRectangleOptions *options,
                                                        gboolean              highlight);
gboolean    gimp_rectangle_options_get_highlight       (GimpRectangleOptions *options);

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

GtkWidget * gimp_rectangle_options_gui                 (GimpToolOptions *tool_options);


#endif  /* __GIMP_RECTANGLE_OPTIONS_H__ */
