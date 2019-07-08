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

#ifndef __GIMP_CLONE_OPTIONS_H__
#define __GIMP_CLONE_OPTIONS_H__


#include "gimpsourceoptions.h"


#define GIMP_TYPE_CLONE_OPTIONS            (gimp_clone_options_get_type ())
#define GIMP_CLONE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CLONE_OPTIONS, GimpCloneOptions))
#define GIMP_CLONE_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CLONE_OPTIONS, GimpCloneOptionsClass))
#define GIMP_IS_CLONE_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CLONE_OPTIONS))
#define GIMP_IS_CLONE_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CLONE_OPTIONS))
#define GIMP_CLONE_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CLONE_OPTIONS, GimpCloneOptionsClass))


typedef struct _GimpCloneOptionsClass GimpCloneOptionsClass;

struct _GimpCloneOptions
{
  GimpSourceOptions  parent_instance;

  GimpCloneType      clone_type;
};

struct _GimpCloneOptionsClass
{
  GimpSourceOptionsClass  parent_class;
};


GType   gimp_clone_options_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_CLONE_OPTIONS_H__  */
