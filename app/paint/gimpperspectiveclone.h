/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_PERSPECTIVE_CLONE_H__
#define __LIGMA_PERSPECTIVE_CLONE_H__


#include "ligmaclone.h"


#define LIGMA_TYPE_PERSPECTIVE_CLONE            (ligma_perspective_clone_get_type ())
#define LIGMA_PERSPECTIVE_CLONE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PERSPECTIVE_CLONE, LigmaPerspectiveClone))
#define LIGMA_PERSPECTIVE_CLONE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PERSPECTIVE_CLONE, LigmaPerspectiveCloneClass))
#define LIGMA_IS_PERSPECTIVE_CLONE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PERSPECTIVE_CLONE))
#define LIGMA_IS_PERSPECTIVE_CLONE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PERSPECTIVE_CLONE))
#define LIGMA_PERSPECTIVE_CLONE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PERSPECTIVE_CLONE, LigmaPerspectiveCloneClass))


typedef struct _LigmaPerspectiveCloneClass LigmaPerspectiveCloneClass;

struct _LigmaPerspectiveClone
{
  LigmaClone      parent_instance;

  gdouble        src_x_fv;     /* source coords in front_view perspective */
  gdouble        src_y_fv;

  gdouble        dest_x_fv;    /* destination coords in front_view perspective */
  gdouble        dest_y_fv;

  LigmaMatrix3    transform;
  LigmaMatrix3    transform_inv;

  GeglNode      *node;
  GeglNode      *crop;
  GeglNode      *transform_node;
  GeglNode      *src_node;
  GeglNode      *dest_node;
};

struct _LigmaPerspectiveCloneClass
{
  LigmaCloneClass  parent_class;
};


void    ligma_perspective_clone_register      (Ligma                      *ligma,
                                              LigmaPaintRegisterCallback  callback);

GType   ligma_perspective_clone_get_type         (void) G_GNUC_CONST;

void    ligma_perspective_clone_set_transform    (LigmaPerspectiveClone   *clone,
                                                 LigmaMatrix3            *transform);
void    ligma_perspective_clone_get_source_point (LigmaPerspectiveClone   *clone,
                                                 gdouble                 x,
                                                 gdouble                 y,
                                                 gdouble                *newx,
                                                 gdouble                *newy);


#endif  /*  __LIGMA_PERSPECTIVE_CLONE_H__  */
