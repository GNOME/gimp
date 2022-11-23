/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatemplateeditor.h
 * Copyright (C) 2002 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_TEMPLATE_EDITOR_H__
#define __LIGMA_TEMPLATE_EDITOR_H__


#define LIGMA_TYPE_TEMPLATE_EDITOR            (ligma_template_editor_get_type ())
#define LIGMA_TEMPLATE_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TEMPLATE_EDITOR, LigmaTemplateEditor))
#define LIGMA_TEMPLATE_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TEMPLATE_EDITOR, LigmaTemplateEditorClass))
#define LIGMA_IS_TEMPLATE_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TEMPLATE_EDITOR))
#define LIGMA_IS_TEMPLATE_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TEMPLATE_EDITOR))
#define LIGMA_TEMPLATE_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TEMPLATE_EDITOR, LigmaTemplateEditorClass))


typedef struct _LigmaTemplateEditorClass LigmaTemplateEditorClass;

struct _LigmaTemplateEditor
{
  GtkBox  parent_instance;
};

struct _LigmaTemplateEditorClass
{
  GtkBoxClass   parent_class;
};


GType          ligma_template_editor_get_type      (void) G_GNUC_CONST;

GtkWidget    * ligma_template_editor_new           (LigmaTemplate       *template,
                                                   Ligma               *ligma,
                                                   gboolean            edit_template);

LigmaTemplate * ligma_template_editor_get_template  (LigmaTemplateEditor *editor);

void           ligma_template_editor_show_advanced (LigmaTemplateEditor *editor,
                                                   gboolean            expanded);
GtkWidget    * ligma_template_editor_get_size_se   (LigmaTemplateEditor *editor);
GtkWidget    * ligma_template_editor_get_resolution_se
                                                  (LigmaTemplateEditor *editor);
GtkWidget    * ligma_template_editor_get_resolution_chain
                                                  (LigmaTemplateEditor *editor);


#endif  /*  __LIGMA_TEMPLATE_EDITOR_H__  */
