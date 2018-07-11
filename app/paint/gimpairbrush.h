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

#ifndef __GIMP_AIRBRUSH_H__
#define __GIMP_AIRBRUSH_H__


#include "gimppaintbrush.h"


#define GIMP_TYPE_AIRBRUSH            (gimp_airbrush_get_type ())
#define GIMP_AIRBRUSH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_AIRBRUSH, GimpAirbrush))
#define GIMP_AIRBRUSH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_AIRBRUSH, GimpAirbrushClass))
#define GIMP_IS_AIRBRUSH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_AIRBRUSH))
#define GIMP_IS_AIRBRUSH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_AIRBRUSH))
#define GIMP_AIRBRUSH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_AIRBRUSH, GimpAirbrushClass))


typedef struct _GimpAirbrushClass GimpAirbrushClass;

struct _GimpAirbrush
{
  GimpPaintbrush    parent_instance;

  guint             timeout_id;

  GimpSymmetry     *sym;
  GimpDrawable     *drawable;
  GimpPaintOptions *paint_options;
  GimpCoords        coords;
};

struct _GimpAirbrushClass
{
  GimpPaintbrushClass  parent_class;

  /*  signals  */
  void (* stamp) (GimpAirbrush *airbrush);
};


void    gimp_airbrush_register (Gimp                      *gimp,
                                GimpPaintRegisterCallback  callback);

GType   gimp_airbrush_get_type (void) G_GNUC_CONST;

void    gimp_airbrush_stamp    (GimpAirbrush              *airbrush);


#endif  /*  __GIMP_AIRBRUSH_H__  */
