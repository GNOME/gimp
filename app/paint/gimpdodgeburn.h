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

#ifndef __GIMP_DODGE_BURN_H__
#define __GIMP_DODGE_BURN_H__


#include "gimpbrushcore.h"


#define GIMP_TYPE_DODGE_BURN            (gimp_dodge_burn_get_type ())
#define GIMP_DODGE_BURN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DODGE_BURN, GimpDodgeBurn))
#define GIMP_IS_DODGE_BURN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DODGE_BURN))
#define GIMP_DODGE_BURN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DODGEBURN, GimpDodgeBurnClass))
#define GIMP_IS_DODGE_BURN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DODGE_BURN))


typedef struct _GimpDodgeBurnClass GimpDodgeBurnClass;

struct _GimpDodgeBurn
{
  GimpBrushCore  parent_instance;
};

struct _GimpDodgeBurnClass
{
  GimpBrushCoreClass  parent_class;
};


void    gimp_dodge_burn_register (Gimp                      *gimp,
                                  GimpPaintRegisterCallback  callback);

GType   gimp_dodge_burn_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_DODGE_BURN_H__  */
