/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaGrid
 * Copyright (C) 2003  Henrik Brix Andersen <brix@ligma.org>
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

#ifndef __LIGMA_GRID_H__
#define __LIGMA_GRID_H__


#include "ligmaobject.h"


#define LIGMA_TYPE_GRID            (ligma_grid_get_type ())
#define LIGMA_GRID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_GRID, LigmaGrid))
#define LIGMA_GRID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_GRID, LigmaGridClass))
#define LIGMA_IS_GRID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_GRID))
#define LIGMA_IS_GRID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_GRID))
#define LIGMA_GRID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_GRID, LigmaGridClass))


typedef struct _LigmaGridClass  LigmaGridClass;

struct _LigmaGrid
{
  LigmaObject     parent_instance;

  LigmaGridStyle  style;
  LigmaRGB        fgcolor;
  LigmaRGB        bgcolor;
  gdouble        xspacing;
  gdouble        yspacing;
  LigmaUnit       spacing_unit;
  gdouble        xoffset;
  gdouble        yoffset;
  LigmaUnit       offset_unit;
};


struct _LigmaGridClass
{
  LigmaObjectClass  parent_class;
};


GType          ligma_grid_get_type               (void) G_GNUC_CONST;

LigmaGridStyle  ligma_grid_get_style              (LigmaGrid           *grid);

void           ligma_grid_get_fgcolor            (LigmaGrid           *grid,
                                                 LigmaRGB            *fgcolor);
void           ligma_grid_get_bgcolor            (LigmaGrid           *grid,
                                                 LigmaRGB            *bgcolor);

void           ligma_grid_get_spacing            (LigmaGrid           *grid,
                                                 gdouble            *xspacing,
                                                 gdouble            *yspacing);
void           ligma_grid_get_offset             (LigmaGrid           *grid,
                                                 gdouble            *xoffset,
                                                 gdouble            *yoffset);

const gchar  * ligma_grid_parasite_name          (void) G_GNUC_CONST;
LigmaParasite * ligma_grid_to_parasite            (LigmaGrid           *grid);
LigmaGrid     * ligma_grid_from_parasite          (const LigmaParasite *parasite);


#endif /* __LIGMA_GRID_H__ */
