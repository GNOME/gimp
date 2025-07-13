/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcanvasgroup.h
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include "gimpcanvasitem.h"


#define GIMP_TYPE_CANVAS_GROUP            (gimp_canvas_group_get_type ())
#define GIMP_CANVAS_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CANVAS_GROUP, GimpCanvasGroup))
#define GIMP_CANVAS_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CANVAS_GROUP, GimpCanvasGroupClass))
#define GIMP_IS_CANVAS_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CANVAS_GROUP))
#define GIMP_IS_CANVAS_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CANVAS_GROUP))
#define GIMP_CANVAS_GROUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CANVAS_GROUP, GimpCanvasGroupClass))


typedef struct _GimpCanvasGroupPrivate GimpCanvasGroupPrivate;
typedef struct _GimpCanvasGroupClass   GimpCanvasGroupClass;

struct _GimpCanvasGroup
{
  GimpCanvasItem          parent_instance;

  GimpCanvasGroupPrivate *priv;

};

struct _GimpCanvasGroupClass
{
  GimpCanvasItemClass  parent_class;
};


GType            gimp_canvas_group_get_type           (void) G_GNUC_CONST;

GimpCanvasItem * gimp_canvas_group_new                (GimpDisplayShell *shell);

void             gimp_canvas_group_add_item           (GimpCanvasGroup  *group,
                                                       GimpCanvasItem   *item);
void             gimp_canvas_group_remove_item        (GimpCanvasGroup  *group,
                                                       GimpCanvasItem   *item);

void             gimp_canvas_group_set_group_stroking (GimpCanvasGroup  *group,
                                                       gboolean          group_stroking);
void             gimp_canvas_group_set_group_filling  (GimpCanvasGroup  *group,
                                                       gboolean          group_filling);
