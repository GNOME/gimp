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

#ifndef  __GIMP_MYBRUSH_CORE_H__
#define  __GIMP_MYBRUSH_CORE_H__


#include "gimppaintcore.h"


#define GIMP_TYPE_MYBRUSH_CORE            (gimp_mybrush_core_get_type ())
#define GIMP_MYBRUSH_CORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MYBRUSH_CORE, GimpMybrushCore))
#define GIMP_MYBRUSH_CORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MYBRUSH_CORE, GimpMybrushCoreClass))
#define GIMP_IS_MYBRUSH_CORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MYBRUSH_CORE))
#define GIMP_IS_MYBRUSH_CORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MYBRUSH_CORE))
#define GIMP_MYBRUSH_CORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MYBRUSH_CORE, GimpMybrushCoreClass))


typedef struct _GimpMybrushCorePrivate GimpMybrushCorePrivate;
typedef struct _GimpMybrushCoreClass   GimpMybrushCoreClass;

struct _GimpMybrushCore
{
  GimpPaintCore           parent_instance;

  GimpMybrushCorePrivate *private;
};

struct _GimpMybrushCoreClass
{
  GimpPaintCoreClass  parent_class;
};


void    gimp_mybrush_core_register (Gimp                      *gimp,
                                    GimpPaintRegisterCallback  callback);

GType   gimp_mybrush_core_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_MYBRUSH_CORE_H__  */
