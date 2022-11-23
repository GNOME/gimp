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

#ifndef __LIGMA_PAINTBRUSH_H__
#define __LIGMA_PAINTBRUSH_H__


#include "ligmabrushcore.h"


#define LIGMA_TYPE_PAINTBRUSH            (ligma_paintbrush_get_type ())
#define LIGMA_PAINTBRUSH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PAINTBRUSH, LigmaPaintbrush))
#define LIGMA_PAINTBRUSH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PAINTBRUSH, LigmaPaintbrushClass))
#define LIGMA_IS_PAINTBRUSH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PAINTBRUSH))
#define LIGMA_IS_PAINTBRUSH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PAINTBRUSH))
#define LIGMA_PAINTBRUSH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PAINTBRUSH, LigmaPaintbrushClass))


typedef struct _LigmaPaintbrushClass LigmaPaintbrushClass;

struct _LigmaPaintbrush
{
  LigmaBrushCore      parent_instance;

  GeglBuffer        *paint_buffer;
  const LigmaTempBuf *paint_pixmap;
  LigmaRGB            paint_color;
};

struct _LigmaPaintbrushClass
{
  LigmaBrushCoreClass  parent_class;

  /*  virtual functions  */
  gboolean   (* get_color_history_color) (LigmaPaintbrush            *paintbrush,
                                          LigmaDrawable              *drawable,
                                          LigmaPaintOptions          *paint_options,
                                          LigmaRGB                   *color);
  void       (* get_paint_params)        (LigmaPaintbrush            *paintbrush,
                                          LigmaDrawable              *drawable,
                                          LigmaPaintOptions          *paint_options,
                                          LigmaSymmetry              *sym,
                                          gdouble                    grad_point,
                                          LigmaLayerMode             *paint_mode,
                                          LigmaPaintApplicationMode  *paint_appl_mode,
                                          const LigmaTempBuf        **paint_pixmap,
                                          LigmaRGB                   *paint_color);
};


void    ligma_paintbrush_register (Ligma                      *ligma,
                                  LigmaPaintRegisterCallback  callback);

GType   ligma_paintbrush_get_type (void) G_GNUC_CONST;


/*  protected  */

void    _ligma_paintbrush_motion  (LigmaPaintCore             *paint_core,
                                  LigmaDrawable              *drawable,
                                  LigmaPaintOptions          *paint_options,
                                  LigmaSymmetry              *sym,
                                  gdouble                    opacity);


#endif  /*  __LIGMA_PAINTBRUSH_H__  */
