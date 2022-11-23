/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvaspassepartout.h
 * Copyright (C) 2010 Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_CANVAS_PASSE_PARTOUT_H__
#define __LIGMA_CANVAS_PASSE_PARTOUT_H__


#include "ligmacanvasrectangle.h"


#define LIGMA_TYPE_CANVAS_PASSE_PARTOUT            (ligma_canvas_passe_partout_get_type ())
#define LIGMA_CANVAS_PASSE_PARTOUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CANVAS_PASSE_PARTOUT, LigmaCanvasPassePartout))
#define LIGMA_CANVAS_PASSE_PARTOUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CANVAS_PASSE_PARTOUT, LigmaCanvasPassePartoutClass))
#define LIGMA_IS_CANVAS_PASSE_PARTOUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CANVAS_PASSE_PARTOUT))
#define LIGMA_IS_CANVAS_PASSE_PARTOUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CANVAS_PASSE_PARTOUT))
#define LIGMA_CANVAS_PASSE_PARTOUT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CANVAS_PASSE_PARTOUT, LigmaCanvasPassePartoutClass))


typedef struct _LigmaCanvasPassePartout      LigmaCanvasPassePartout;
typedef struct _LigmaCanvasPassePartoutClass LigmaCanvasPassePartoutClass;

struct _LigmaCanvasPassePartout
{
  LigmaCanvasRectangle  parent_instance;
};

struct _LigmaCanvasPassePartoutClass
{
  LigmaCanvasRectangleClass  parent_class;
};


GType            ligma_canvas_passe_partout_get_type (void) G_GNUC_CONST;

LigmaCanvasItem * ligma_canvas_passe_partout_new      (LigmaDisplayShell *shell,
                                                     gdouble           x,
                                                     gdouble           y,
                                                     gdouble           width,
                                                     gdouble           height);


#endif /* __LIGMA_CANVAS_PASSE_PARTOUT_H__ */
