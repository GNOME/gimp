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

#include "gimpobject.h"


#define GIMP_TYPE_PROJECTION            (gimp_projection_get_type ())
#define GIMP_PROJECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PROJECTION, GimpProjection))
#define GIMP_PROJECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PROJECTION, GimpProjectionClass))
#define GIMP_IS_PROJECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PROJECTION))
#define GIMP_IS_PROJECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PROJECTION))
#define GIMP_PROJECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PROJECTION, GimpProjectionClass))


typedef struct _GimpProjectionPrivate GimpProjectionPrivate;
typedef struct _GimpProjectionClass   GimpProjectionClass;

struct _GimpProjection
{
  GimpObject             parent_instance;

  GimpProjectionPrivate *priv;
};

struct _GimpProjectionClass
{
  GimpObjectClass  parent_class;

  void (* update) (GimpProjection *proj,
                   gboolean        now,
                   gint            x,
                   gint            y,
                   gint            width,
                   gint            height);
};


GType            gimp_projection_get_type          (void) G_GNUC_CONST;

GimpProjection * gimp_projection_new               (GimpProjectable   *projectable);

void             gimp_projection_set_priority      (GimpProjection    *projection,
                                                    gint               priority);
gint             gimp_projection_get_priority      (GimpProjection    *projection);

void             gimp_projection_set_priority_rect (GimpProjection    *proj,
                                                    gint               x,
                                                    gint               y,
                                                    gint               width,
                                                    gint               height);

void             gimp_projection_stop_rendering    (GimpProjection    *proj);

void             gimp_projection_flush             (GimpProjection    *proj);
void             gimp_projection_flush_now         (GimpProjection    *proj,
                                                    gboolean           direct);
void             gimp_projection_finish_draw       (GimpProjection    *proj);

gint64           gimp_projection_estimate_memsize  (GimpImageBaseType  type,
                                                    GimpComponentType  component_type,
                                                    gint               width,
                                                    gint               height);
