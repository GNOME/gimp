/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolpolygon.h
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_TOOL_POLYGON_H__
#define __GIMP_TOOL_POLYGON_H__


#include "gimptoolwidget.h"


#define GIMP_TYPE_TOOL_POLYGON            (gimp_tool_polygon_get_type ())
#define GIMP_TOOL_POLYGON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_POLYGON, GimpToolPolygon))
#define GIMP_TOOL_POLYGON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_POLYGON, GimpToolPolygonClass))
#define GIMP_IS_TOOL_POLYGON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_POLYGON))
#define GIMP_IS_TOOL_POLYGON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_POLYGON))
#define GIMP_TOOL_POLYGON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_POLYGON, GimpToolPolygonClass))


typedef struct _GimpToolPolygon        GimpToolPolygon;
typedef struct _GimpToolPolygonPrivate GimpToolPolygonPrivate;
typedef struct _GimpToolPolygonClass   GimpToolPolygonClass;

struct _GimpToolPolygon
{
  GimpToolWidget          parent_instance;

  GimpToolPolygonPrivate *private;
};

struct _GimpToolPolygonClass
{
  GimpToolWidgetClass  parent_class;

  /*  signals  */
  void (* change_complete) (GimpToolPolygon *polygon);
};


GType            gimp_tool_polygon_get_type   (void) G_GNUC_CONST;

GimpToolWidget * gimp_tool_polygon_new        (GimpDisplayShell   *shell);

gboolean         gimp_tool_polygon_is_closed  (GimpToolPolygon    *polygon);
void             gimp_tool_polygon_get_points (GimpToolPolygon    *polygon,
                                               const GimpVector2 **points,
                                               gint               *n_points);


#endif /* __GIMP_TOOL_POLYGON_H__ */
