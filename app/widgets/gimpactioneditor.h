/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaactioneditor.h
 * Copyright (C) 2008  Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_ACTION_EDITOR_H__
#define __LIGMA_ACTION_EDITOR_H__


#define LIGMA_TYPE_ACTION_EDITOR            (ligma_action_editor_get_type ())
#define LIGMA_ACTION_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_ACTION_EDITOR, LigmaActionEditor))
#define LIGMA_ACTION_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_ACTION_EDITOR, LigmaActionEditorClass))
#define LIGMA_IS_ACTION_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_ACTION_EDITOR))
#define LIGMA_IS_ACTION_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_ACTION_EDITOR))
#define LIGMA_ACTION_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_ACTION_EDITOR, LigmaActionEditorClass))


typedef struct _LigmaActionEditorClass LigmaActionEditorClass;

struct _LigmaActionEditor
{
  GtkBox     parent_instance;

  GtkWidget *view;
};

struct _LigmaActionEditorClass
{
  GtkBoxClass  parent_class;
};


GType       ligma_action_editor_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_action_editor_new      (LigmaUIManager *manager,
                                         const gchar   *select_action,
                                         gboolean       show_shortcuts);


#endif  /*  __LIGMA_ACTION_EDITOR_H__  */
