/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacellrendererdashes.h
 * Copyright (C) 2005 Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_CELL_RENDERER_DASHES_H__
#define __LIGMA_CELL_RENDERER_DASHES_H__


#define LIGMA_TYPE_CELL_RENDERER_DASHES            (ligma_cell_renderer_dashes_get_type ())
#define LIGMA_CELL_RENDERER_DASHES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CELL_RENDERER_DASHES, LigmaCellRendererDashes))
#define LIGMA_CELL_RENDERER_DASHES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CELL_RENDERER_DASHES, LigmaCellRendererDashesClass))
#define LIGMA_IS_CELL_RENDERER_DASHES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CELL_RENDERER_DASHES))
#define LIGMA_IS_CELL_RENDERER_DASHES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CELL_RENDERER_DASHES))
#define LIGMA_CELL_RENDERER_DASHES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CELL_RENDERER_DASHES, LigmaCellRendererDashesClass))


typedef struct _LigmaCellRendererDashesClass LigmaCellRendererDashesClass;

struct _LigmaCellRendererDashes
{
  GtkCellRenderer   parent_instance;

  gboolean         *segments;
};

struct _LigmaCellRendererDashesClass
{
  GtkCellRendererClass  parent_class;
};


GType             ligma_cell_renderer_dashes_get_type (void) G_GNUC_CONST;

GtkCellRenderer * ligma_cell_renderer_dashes_new      (void);


#endif /* __LIGMA_CELL_RENDERER_DASHES_H__ */
