/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_SMUDGE_H__
#define __GIMP_SMUDGE_H__


#include "base/pixel-region.h"

#include "gimpbrushcore.h"


#define GIMP_TYPE_SMUDGE            (gimp_smudge_get_type ())
#define GIMP_SMUDGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SMUDGE, GimpSmudge))
#define GIMP_SMUDGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SMUDGE, GimpSmudgeClass))
#define GIMP_IS_SMUDGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SMUDGE))
#define GIMP_IS_SMUDGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SMUDGE))
#define GIMP_SMUDGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SMUDGE, GimpSmudgeClass))


typedef struct _GimpSmudgeClass GimpSmudgeClass;

struct _GimpSmudge
{
  GimpBrushCore  parent_instance;

  gboolean       initialized;
  PixelRegion    accumPR;
  guchar        *accum_data;
};

struct _GimpSmudgeClass
{
  GimpBrushCoreClass  parent_class;
};


void    gimp_smudge_register (Gimp                      *gimp,
                              GimpPaintRegisterCallback  callback);

GType   gimp_smudge_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_SMUDGE_H__  */
