/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcellrendererdashes.h
 * Copyright (C) 2005 Sven Neumann <sven@gimp.org>
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

#pragma once


#define GIMP_TYPE_CELL_RENDERER_DASHES            (gimp_cell_renderer_dashes_get_type ())
#define GIMP_CELL_RENDERER_DASHES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CELL_RENDERER_DASHES, GimpCellRendererDashes))
#define GIMP_CELL_RENDERER_DASHES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CELL_RENDERER_DASHES, GimpCellRendererDashesClass))
#define GIMP_IS_CELL_RENDERER_DASHES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CELL_RENDERER_DASHES))
#define GIMP_IS_CELL_RENDERER_DASHES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CELL_RENDERER_DASHES))
#define GIMP_CELL_RENDERER_DASHES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CELL_RENDERER_DASHES, GimpCellRendererDashesClass))


typedef struct _GimpCellRendererDashesClass GimpCellRendererDashesClass;

struct _GimpCellRendererDashes
{
  GtkCellRenderer   parent_instance;

  gboolean         *segments;
};

struct _GimpCellRendererDashesClass
{
  GtkCellRendererClass  parent_class;
};


GType             gimp_cell_renderer_dashes_get_type (void) G_GNUC_CONST;

GtkCellRenderer * gimp_cell_renderer_dashes_new      (void);
