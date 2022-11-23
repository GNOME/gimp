/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacomponenteditor.h
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_COMPONENT_EDITOR_H__
#define __LIGMA_COMPONENT_EDITOR_H__


#include "ligmaimageeditor.h"


#define LIGMA_TYPE_COMPONENT_EDITOR            (ligma_component_editor_get_type ())
#define LIGMA_COMPONENT_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COMPONENT_EDITOR, LigmaComponentEditor))
#define LIGMA_COMPONENT_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COMPONENT_EDITOR, LigmaComponentEditorClass))
#define LIGMA_IS_COMPONENT_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COMPONENT_EDITOR))
#define LIGMA_IS_COMPONENT_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COMPONENT_EDITOR))
#define LIGMA_COMPONENT_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COMPONENT_EDITOR, LigmaComponentEditorClass))


typedef struct _LigmaComponentEditorClass  LigmaComponentEditorClass;

struct _LigmaComponentEditor
{
  LigmaImageEditor    parent_instance;

  gint               view_size;

  GtkTreeModel      *model;
  GtkTreeView       *view;
  GtkTreeSelection  *selection;

  GtkTreeViewColumn *eye_column;
  GtkCellRenderer   *eye_cell;
  GtkCellRenderer   *renderer_cell;

  LigmaChannelType    clicked_component;
};

struct _LigmaComponentEditorClass
{
  LigmaImageEditorClass  parent_class;
};


GType       ligma_component_editor_get_type      (void) G_GNUC_CONST;

GtkWidget * ligma_component_editor_new           (gint                 view_size,
                                                 LigmaMenuFactory     *menu_factory);
void        ligma_component_editor_set_view_size (LigmaComponentEditor *editor,
                                                 gint                 view_size);


#endif  /*  __LIGMA_COMPONENT_EDITOR_H__  */
