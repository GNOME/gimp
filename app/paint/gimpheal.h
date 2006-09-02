/* The GIMP -- an image manipulation program
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

#ifndef __GIMP_HEAL_H__
#define __GIMP_HEAL_H__


#include "gimpbrushcore.h"


#define GIMP_TYPE_HEAL            (gimp_heal_get_type ())
#define GIMP_HEAL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_HEAL, GimpHeal))
#define GIMP_HEAL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_HEAL, GimpHealClass))
#define GIMP_IS_HEAL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_HEAL))
#define GIMP_IS_HEAL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_HEAL))
#define GIMP_HEAL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_HEAL, GimpHealClass))


typedef struct _GimpHeal      GimpHeal;
typedef struct _GimpHealClass GimpHealClass;

struct _GimpHeal
{
  GimpBrushCore  parent_instance;

  gboolean       set_source;

  GimpDrawable  *src_drawable;
  gdouble        src_x;
  gdouble        src_y;

  gdouble        orig_src_x;
  gdouble        orig_src_y;

  gdouble        offset_x;
  gdouble        offset_y;
  gboolean       first_stroke;
};

struct _GimpHealClass
{
  GimpBrushCoreClass  parent_class;
};


void    gimp_heal_register (Gimp                      *gimp,
                            GimpPaintRegisterCallback  callback);

GType   gimp_heal_get_type (void) G_GNUC_CONST;


#endif  /*  __GIMP_HEAL_H__  */
