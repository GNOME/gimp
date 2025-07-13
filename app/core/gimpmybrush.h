/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmybrush.h
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

#include "gimpdata.h"


#define GIMP_TYPE_MYBRUSH            (gimp_mybrush_get_type ())
#define GIMP_MYBRUSH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MYBRUSH, GimpMybrush))
#define GIMP_MYBRUSH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MYBRUSH, GimpMybrushClass))
#define GIMP_IS_MYBRUSH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MYBRUSH))
#define GIMP_IS_MYBRUSH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MYBRUSH))
#define GIMP_MYBRUSH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MYBRUSH, GimpMybrushClass))


typedef struct _GimpMybrushPrivate GimpMybrushPrivate;
typedef struct _GimpMybrushClass   GimpMybrushClass;

struct _GimpMybrush
{
  GimpData            parent_instance;

  GimpMybrushPrivate *priv;
};

struct _GimpMybrushClass
{
  GimpDataClass  parent_class;
};


GType         gimp_mybrush_get_type             (void) G_GNUC_CONST;

GimpData    * gimp_mybrush_new                  (GimpContext *context,
                                                 const gchar *name);
GimpData    * gimp_mybrush_get_standard         (GimpContext *context);

const gchar * gimp_mybrush_get_brush_json       (GimpMybrush *brush);

gdouble       gimp_mybrush_get_radius           (GimpMybrush *brush);
gdouble       gimp_mybrush_get_opaque           (GimpMybrush *brush);
gdouble       gimp_mybrush_get_hardness         (GimpMybrush *brush);
gdouble       gimp_mybrush_get_gain             (GimpMybrush *brush);
gdouble       gimp_mybrush_get_pigment          (GimpMybrush *brush);
gdouble       gimp_mybrush_get_posterize        (GimpMybrush *brush);
gdouble       gimp_mybrush_get_posterize_num    (GimpMybrush *brush);
gdouble       gimp_mybrush_get_offset_by_random (GimpMybrush *brush);
gboolean      gimp_mybrush_get_is_eraser        (GimpMybrush *brush);
