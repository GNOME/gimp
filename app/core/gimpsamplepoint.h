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

#pragma once

#include "gimpauxitem.h"


#define GIMP_SAMPLE_POINT_POSITION_UNDEFINED G_MININT


#define GIMP_TYPE_SAMPLE_POINT            (gimp_sample_point_get_type ())
#define GIMP_SAMPLE_POINT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SAMPLE_POINT, GimpSamplePoint))
#define GIMP_SAMPLE_POINT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SAMPLE_POINT, GimpSamplePointClass))
#define GIMP_IS_SAMPLE_POINT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SAMPLE_POINT))
#define GIMP_IS_SAMPLE_POINT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SAMPLE_POINT))
#define GIMP_SAMPLE_POINT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SAMPLE_POINT, GimpSamplePointClass))


typedef struct _GimpSamplePointPrivate GimpSamplePointPrivate;
typedef struct _GimpSamplePointClass   GimpSamplePointClass;

struct _GimpSamplePoint
{
  GimpAuxItem             parent_instance;

  GimpSamplePointPrivate *priv;
};

struct _GimpSamplePointClass
{
  GimpAuxItemClass  parent_class;
};


GType             gimp_sample_point_get_type      (void) G_GNUC_CONST;

GimpSamplePoint * gimp_sample_point_new           (guint32            sample_point_ID);

void              gimp_sample_point_get_position  (GimpSamplePoint   *sample_point,
                                                   gint              *position_x,
                                                   gint              *position_y);
void              gimp_sample_point_set_position  (GimpSamplePoint   *sample_point,
                                                   gint               position_x,
                                                   gint               position_y);

GimpColorPickMode gimp_sample_point_get_pick_mode (GimpSamplePoint   *sample_point);
void              gimp_sample_point_set_pick_mode (GimpSamplePoint   *sample_point,
                                                   GimpColorPickMode  pick_mode);
