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

#ifndef __GIMP_CLONE_H__
#define __GIMP_CLONE_H__


#include "gimpsourcecore.h"


#define GIMP_TYPE_CLONE            (gimp_clone_get_type ())
#define GIMP_CLONE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CLONE, GimpClone))
#define GIMP_CLONE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CLONE, GimpCloneClass))
#define GIMP_IS_CLONE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CLONE))
#define GIMP_IS_CLONE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CLONE))
#define GIMP_CLONE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CLONE, GimpCloneClass))


typedef struct _GimpCloneClass GimpCloneClass;

struct _GimpClone
{
  GimpSourceCore  parent_instance;
};

struct _GimpCloneClass
{
  GimpSourceCoreClass  parent_class;
};


void    gimp_clone_register (Gimp                      *gimp,
                             GimpPaintRegisterCallback  callback);

GType   gimp_clone_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_CLONE_H__  */
