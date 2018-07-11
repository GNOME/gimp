/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcircle.h
 * Copyright (C) 2014 Michael Natterer <mitch@gimp.org>
 *
 * Based on code from the color-rotate plug-in
 * Copyright (C) 1997-1999 Sven Anders (anderss@fmi.uni-passau.de)
 *                         Based on code from Pavel Grinfeld (pavel@ml.com)
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

#ifndef __GIMP_CIRCLE_H__
#define __GIMP_CIRCLE_H__


#define GIMP_TYPE_CIRCLE            (gimp_circle_get_type ())
#define GIMP_CIRCLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CIRCLE, GimpCircle))
#define GIMP_CIRCLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CIRCLE, GimpCircleClass))
#define GIMP_IS_CIRCLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_CIRCLE))
#define GIMP_IS_CIRCLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CIRCLE))
#define GIMP_CIRCLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CIRCLE, GimpCircleClass))


typedef struct _GimpCirclePrivate GimpCirclePrivate;
typedef struct _GimpCircleClass   GimpCircleClass;

struct _GimpCircle
{
  GtkWidget          parent_instance;

  GimpCirclePrivate *priv;
};

struct _GimpCircleClass
{
  GtkWidgetClass  parent_class;

  void (* reset_target) (GimpCircle *circle);
};


GType          gimp_circle_get_type                (void) G_GNUC_CONST;

GtkWidget    * gimp_circle_new                     (void);

gboolean       _gimp_circle_has_grab               (GimpCircle *circle);
gdouble        _gimp_circle_get_angle_and_distance (GimpCircle *circle,
                                                    gdouble     event_x,
                                                    gdouble     event_y,
                                                    gdouble    *distance);


#endif /* __GIMP_CIRCLE_H__ */
