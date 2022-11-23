/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_SELECTION_EDITOR_H__
#define __LIGMA_SELECTION_EDITOR_H__


#include "ligmaimageeditor.h"


#define LIGMA_TYPE_SELECTION_EDITOR            (ligma_selection_editor_get_type ())
#define LIGMA_SELECTION_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SELECTION_EDITOR, LigmaSelectionEditor))
#define LIGMA_SELECTION_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SELECTION_EDITOR, LigmaSelectionEditorClass))
#define LIGMA_IS_SELECTION_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SELECTION_EDITOR))
#define LIGMA_IS_SELECTION_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SELECTION_EDITOR))
#define LIGMA_SELECTION_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SELECTION_EDITOR, LigmaSelectionEditorClass))


typedef struct _LigmaSelectionEditorClass LigmaSelectionEditorClass;

struct _LigmaSelectionEditor
{
  LigmaImageEditor  parent_instance;

  GtkWidget       *view;

  GtkWidget       *all_button;
  GtkWidget       *none_button;
  GtkWidget       *invert_button;
  GtkWidget       *save_button;
  GtkWidget       *path_button;
  GtkWidget       *stroke_button;
};

struct _LigmaSelectionEditorClass
{
  LigmaImageEditorClass  parent_class;
};


GType       ligma_selection_editor_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_selection_editor_new      (LigmaMenuFactory *menu_factory);


#endif /* __LIGMA_SELECTION_EDITOR_H__ */
