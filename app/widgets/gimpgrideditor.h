/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaGridEditor
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

#ifndef __LIGMA_GRID_EDITOR_H__
#define __LIGMA_GRID_EDITOR_H__


#define LIGMA_TYPE_GRID_EDITOR            (ligma_grid_editor_get_type ())
#define LIGMA_GRID_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_GRID_EDITOR, LigmaGridEditor))
#define LIGMA_GRID_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_GRID_EDITOR, LigmaGridEditorClass))
#define LIGMA_IS_GRID_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_GRID_EDITOR))
#define LIGMA_IS_GRID_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_GRID_EDITOR))
#define LIGMA_GRID_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_GRID_EDITOR, LigmaGridEditorClass))


typedef struct _LigmaGridEditorClass LigmaGridEditorClass;

struct _LigmaGridEditor
{
  GtkBox       parent_instance;

  LigmaGrid    *grid;
  LigmaContext *context;
  gdouble      xresolution;
  gdouble      yresolution;
};

struct _LigmaGridEditorClass
{
  GtkBoxClass  parent_class;
};


GType       ligma_grid_editor_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_grid_editor_new      (LigmaGrid    *grid,
                                       LigmaContext *context,
                                       gdouble      xresolution,
                                       gdouble      yresolution);


#endif /*  __LIGMA_GRID_EDITOR_H__  */
