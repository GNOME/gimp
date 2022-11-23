/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontrollereditor.h
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CONTROLLER_EDITOR_H__
#define __LIGMA_CONTROLLER_EDITOR_H__


#define LIGMA_TYPE_CONTROLLER_EDITOR            (ligma_controller_editor_get_type ())
#define LIGMA_CONTROLLER_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CONTROLLER_EDITOR, LigmaControllerEditor))
#define LIGMA_CONTROLLER_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CONTROLLER_EDITOR, LigmaControllerEditorClass))
#define LIGMA_IS_CONTROLLER_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CONTROLLER_EDITOR))
#define LIGMA_IS_CONTROLLER_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CONTROLLER_EDITOR))
#define LIGMA_CONTROLLER_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CONTROLLER_EDITOR, LigmaControllerEditorClass))


typedef struct _LigmaControllerEditorClass LigmaControllerEditorClass;

struct _LigmaControllerEditor
{
  GtkBox              parent_instance;

  LigmaControllerInfo *info;
  LigmaContext        *context;

  GtkTreeSelection   *sel;

  GtkWidget          *grab_button;
  GtkWidget          *edit_button;
  GtkWidget          *delete_button;

  GtkWidget          *edit_dialog;
  GtkTreeSelection   *edit_sel;
};

struct _LigmaControllerEditorClass
{
  GtkBoxClass   parent_class;
};


GType       ligma_controller_editor_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_controller_editor_new      (LigmaControllerInfo *info,
                                             LigmaContext        *context);


#endif  /*  __LIGMA_CONTROLLER_EDITOR_H__  */
