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

#include "gimpclone.h"


#define GIMP_TYPE_PERSPECTIVE_CLONE            (gimp_perspective_clone_get_type ())
#define GIMP_PERSPECTIVE_CLONE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PERSPECTIVE_CLONE, GimpPerspectiveClone))
#define GIMP_PERSPECTIVE_CLONE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PERSPECTIVE_CLONE, GimpPerspectiveCloneClass))
#define GIMP_IS_PERSPECTIVE_CLONE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PERSPECTIVE_CLONE))
#define GIMP_IS_PERSPECTIVE_CLONE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PERSPECTIVE_CLONE))
#define GIMP_PERSPECTIVE_CLONE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PERSPECTIVE_CLONE, GimpPerspectiveCloneClass))


typedef struct _GimpPerspectiveCloneClass GimpPerspectiveCloneClass;

struct _GimpPerspectiveClone
{
  GimpClone      parent_instance;

  gdouble        src_x_fv;     /* source coords in front_view perspective */
  gdouble        src_y_fv;

  gdouble        dest_x_fv;    /* destination coords in front_view perspective */
  gdouble        dest_y_fv;

  GimpMatrix3    transform;
  GimpMatrix3    transform_inv;

  GeglNode      *node;
  GeglNode      *crop;
  GeglNode      *transform_node;
  GeglNode      *src_node;
  GeglNode      *dest_node;
};

struct _GimpPerspectiveCloneClass
{
  GimpCloneClass  parent_class;
};


void    gimp_perspective_clone_register      (Gimp                      *gimp,
                                              GimpPaintRegisterCallback  callback);

GType   gimp_perspective_clone_get_type         (void) G_GNUC_CONST;

void    gimp_perspective_clone_set_transform    (GimpPerspectiveClone   *clone,
                                                 GimpMatrix3            *transform);
void    gimp_perspective_clone_get_source_point (GimpPerspectiveClone   *clone,
                                                 gdouble                 x,
                                                 gdouble                 y,
                                                 gdouble                *newx,
                                                 gdouble                *newy);
