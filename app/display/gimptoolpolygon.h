/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolpolygon.h
 * Copyright (C) 2017 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_TOOL_POLYGON_H__
#define __LIGMA_TOOL_POLYGON_H__


#include "ligmatoolwidget.h"


#define LIGMA_TYPE_TOOL_POLYGON            (ligma_tool_polygon_get_type ())
#define LIGMA_TOOL_POLYGON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_POLYGON, LigmaToolPolygon))
#define LIGMA_TOOL_POLYGON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_POLYGON, LigmaToolPolygonClass))
#define LIGMA_IS_TOOL_POLYGON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_POLYGON))
#define LIGMA_IS_TOOL_POLYGON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_POLYGON))
#define LIGMA_TOOL_POLYGON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_POLYGON, LigmaToolPolygonClass))


typedef struct _LigmaToolPolygon        LigmaToolPolygon;
typedef struct _LigmaToolPolygonPrivate LigmaToolPolygonPrivate;
typedef struct _LigmaToolPolygonClass   LigmaToolPolygonClass;

struct _LigmaToolPolygon
{
  LigmaToolWidget          parent_instance;

  LigmaToolPolygonPrivate *private;
};

struct _LigmaToolPolygonClass
{
  LigmaToolWidgetClass  parent_class;

  /*  signals  */
  void (* change_complete) (LigmaToolPolygon *polygon);
};


GType            ligma_tool_polygon_get_type   (void) G_GNUC_CONST;

LigmaToolWidget * ligma_tool_polygon_new        (LigmaDisplayShell   *shell);

gboolean         ligma_tool_polygon_is_closed  (LigmaToolPolygon    *polygon);
void             ligma_tool_polygon_get_points (LigmaToolPolygon    *polygon,
                                               const LigmaVector2 **points,
                                               gint               *n_points);


#endif /* __LIGMA_TOOL_POLYGON_H__ */
