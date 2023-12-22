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

#ifndef __GIMP_PAINTBRUSH_H__
#define __GIMP_PAINTBRUSH_H__


#include "gimpbrushcore.h"


#define GIMP_TYPE_PAINTBRUSH            (gimp_paintbrush_get_type ())
#define GIMP_PAINTBRUSH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PAINTBRUSH, GimpPaintbrush))
#define GIMP_PAINTBRUSH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PAINTBRUSH, GimpPaintbrushClass))
#define GIMP_IS_PAINTBRUSH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PAINTBRUSH))
#define GIMP_IS_PAINTBRUSH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PAINTBRUSH))
#define GIMP_PAINTBRUSH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PAINTBRUSH, GimpPaintbrushClass))


typedef struct _GimpPaintbrushClass GimpPaintbrushClass;

struct _GimpPaintbrush
{
  GimpBrushCore      parent_instance;

  GeglBuffer        *paint_buffer;
  const GimpTempBuf *paint_pixmap;
  GeglColor         *paint_color;
};

struct _GimpPaintbrushClass
{
  GimpBrushCoreClass  parent_class;

  /*  virtual functions  */
  gboolean   (* get_color_history_color) (GimpPaintbrush            *paintbrush,
                                          GimpDrawable              *drawable,
                                          GimpPaintOptions          *paint_options,
                                          GeglColor                **color);
  void       (* get_paint_params)        (GimpPaintbrush            *paintbrush,
                                          GimpDrawable              *drawable,
                                          GimpPaintOptions          *paint_options,
                                          GimpSymmetry              *sym,
                                          gdouble                    grad_point,
                                          GimpLayerMode             *paint_mode,
                                          GimpPaintApplicationMode  *paint_appl_mode,
                                          const GimpTempBuf        **paint_pixmap,
                                          GeglColor                **paint_color);
};


void    gimp_paintbrush_register (Gimp                      *gimp,
                                  GimpPaintRegisterCallback  callback);

GType   gimp_paintbrush_get_type (void) G_GNUC_CONST;


/*  protected  */

void    _gimp_paintbrush_motion  (GimpPaintCore             *paint_core,
                                  GimpDrawable              *drawable,
                                  GimpPaintOptions          *paint_options,
                                  GimpSymmetry              *sym,
                                  gdouble                    opacity);


#endif  /*  __GIMP_PAINTBRUSH_H__  */
