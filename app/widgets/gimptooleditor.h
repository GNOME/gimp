/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatooleditor.h
 * Copyright (C) 2001-2009 Michael Natterer <mitch@ligma.org>
 *                         Stephen Griffiths <scgmk5@gmail.com>
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

#ifndef __LIGMA_TOOL_EDITOR_H__
#define __LIGMA_TOOL_EDITOR_H__


#include "ligmacontainertreeview.h"


#define LIGMA_TYPE_TOOL_EDITOR            (ligma_tool_editor_get_type ())
#define LIGMA_TOOL_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_EDITOR, LigmaToolEditor))
#define LIGMA_TOOL_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_EDITOR, LigmaToolEditorClass))
#define LIGMA_IS_TOOL_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_EDITOR))
#define LIGMA_IS_TOOL_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_EDITOR))
#define LIGMA_TOOL_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_EDITOR, LigmaToolEditorClass))


typedef struct _LigmaToolEditorPrivate LigmaToolEditorPrivate;
typedef struct _LigmaToolEditorClass   LigmaToolEditorClass;

struct _LigmaToolEditor
{
  LigmaContainerTreeView  parent_instance;

  LigmaToolEditorPrivate *priv;
};

struct _LigmaToolEditorClass
{
  LigmaContainerTreeViewClass  parent_class;
};


GType       ligma_tool_editor_get_type       (void) G_GNUC_CONST;

GtkWidget * ligma_tool_editor_new            (LigmaContainer  *container,
                                             LigmaContext    *context,
                                             gint            view_size,
                                             gint            view_border_width);

void        ligma_tool_editor_revert_changes (LigmaToolEditor *tool_editor);


#endif  /*  __LIGMA_TOOL_EDITOR_H__  */
