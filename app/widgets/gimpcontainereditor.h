/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontainereditor.h
 * Copyright (C) 2001-2011 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_CONTAINER_EDITOR_H__
#define __LIGMA_CONTAINER_EDITOR_H__


#define LIGMA_TYPE_CONTAINER_EDITOR            (ligma_container_editor_get_type ())
#define LIGMA_CONTAINER_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CONTAINER_EDITOR, LigmaContainerEditor))
#define LIGMA_CONTAINER_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CONTAINER_EDITOR, LigmaContainerEditorClass))
#define LIGMA_IS_CONTAINER_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CONTAINER_EDITOR))
#define LIGMA_IS_CONTAINER_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CONTAINER_EDITOR))
#define LIGMA_CONTAINER_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CONTAINER_EDITOR, LigmaContainerEditorClass))


typedef struct _LigmaContainerEditorPrivate LigmaContainerEditorPrivate;
typedef struct _LigmaContainerEditorClass   LigmaContainerEditorClass;

struct _LigmaContainerEditor
{
  GtkBox             parent_instance;

  LigmaContainerView *view;

  LigmaContainerEditorPrivate *priv;
};

struct _LigmaContainerEditorClass
{
  GtkBoxClass  parent_class;

  void (* select_item)   (LigmaContainerEditor *editor,
                          LigmaViewable        *object);
  void (* activate_item) (LigmaContainerEditor *editor,
                          LigmaViewable        *object);
};


GType            ligma_container_editor_get_type           (void) G_GNUC_CONST;

GtkSelectionMode ligma_container_editor_get_selection_mode (LigmaContainerEditor *editor);
void             ligma_container_editor_set_selection_mode (LigmaContainerEditor *editor,
                                                           GtkSelectionMode     mode);

void             ligma_container_editor_bind_to_async_set  (LigmaContainerEditor *editor,
                                                           LigmaAsyncSet        *async_set,
                                                           const gchar         *message);


#endif  /*  __LIGMA_CONTAINER_EDITOR_H__  */
