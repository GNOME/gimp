/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_CURVE_H__
#define __GIMP_CURVE_H__


#include "gimpdata.h"


#define GIMP_CURVE_NUM_POINTS 17 /* TODO: get rid of this limit */


#define GIMP_TYPE_CURVE            (gimp_curve_get_type ())
#define GIMP_CURVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CURVE, GimpCurve))
#define GIMP_CURVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CURVE, GimpCurveClass))
#define GIMP_IS_CURVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CURVE))
#define GIMP_IS_CURVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CURVE))
#define GIMP_CURVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CURVE, GimpCurveClass))


typedef struct _GimpCurveClass GimpCurveClass;

struct _GimpCurve
{
  GimpData       parent_instance;

  GimpCurveType  curve_type;

  gint           points[GIMP_CURVE_NUM_POINTS][2];
  guchar         curve[256];

};

struct _GimpCurveClass
{
  GimpDataClass  parent_class;
};


GType           gimp_curve_get_type          (void) G_GNUC_CONST;

GimpData      * gimp_curve_new               (const gchar   *name);
GimpData      * gimp_curve_get_standard      (void);

void            gimp_curve_reset             (GimpCurve     *curve,
                                              gboolean       reset_type);

void            gimp_curve_set_curve_type    (GimpCurve     *curve,
                                              GimpCurveType  curve_type);
GimpCurveType   gimp_curve_get_curve_type    (GimpCurve     *curve);

gint            gimp_curve_get_closest_point (GimpCurve     *curve,
                                              gint           x);
void            gimp_curve_set_point         (GimpCurve     *curve,
                                              gint           point,
                                              gint           x,
                                              gint           y);
void            gimp_curve_move_point        (GimpCurve     *curve,
                                              gint           point,
                                              gint           y);

void            gimp_curve_set_curve         (GimpCurve     *curve,
                                              gint           x,
                                              gint           y);

void            gimp_curve_get_uchar         (GimpCurve     *curve,
                                              guchar        *dest_array);

/* FIXME: make private */
void            gimp_curve_calculate         (GimpCurve     *curve);


#endif /* __GIMP_CURVE_H__ */
