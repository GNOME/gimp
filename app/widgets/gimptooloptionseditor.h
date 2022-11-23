/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatooloptionseditor.h
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

#ifndef __LIGMA_TOOL_OPTIONS_EDITOR_H__
#define __LIGMA_TOOL_OPTIONS_EDITOR_H__


#include "ligmaeditor.h"


#define LIGMA_TYPE_TOOL_OPTIONS_EDITOR            (ligma_tool_options_editor_get_type ())
#define LIGMA_TOOL_OPTIONS_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_OPTIONS_EDITOR, LigmaToolOptionsEditor))
#define LIGMA_TOOL_OPTIONS_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_OPTIONS_EDITOR, LigmaToolOptionsEditorClass))
#define LIGMA_IS_TOOL_OPTIONS_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_OPTIONS_EDITOR))
#define LIGMA_IS_TOOL_OPTIONS_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_OPTIONS_EDITOR))
#define LIGMA_TOOL_OPTIONS_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_OPTIONS_EDITOR, LigmaToolOptionsEditorClass))


typedef struct _LigmaToolOptionsEditorPrivate  LigmaToolOptionsEditorPrivate;
typedef struct _LigmaToolOptionsEditorClass    LigmaToolOptionsEditorClass;

struct _LigmaToolOptionsEditor
{
  LigmaEditor                    parent_instance;

  LigmaToolOptionsEditorPrivate *p;
};

struct _LigmaToolOptionsEditorClass
{
  LigmaEditorClass  parent_class;
};


GType             ligma_tool_options_editor_get_type         (void) G_GNUC_CONST;
GtkWidget       * ligma_tool_options_editor_new              (Ligma                  *ligma,
                                                             LigmaMenuFactory       *menu_factory);
LigmaToolOptions * ligma_tool_options_editor_get_tool_options (LigmaToolOptionsEditor *editor);


#endif  /*  __LIGMA_TOOL_OPTIONS_EDITOR_H__  */
