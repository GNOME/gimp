/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainereditor.h
 * Copyright (C) 2001-2011 Michael Natterer <mitch@gimp.org>
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


#define GIMP_TYPE_CONTAINER_EDITOR            (gimp_container_editor_get_type ())
#define GIMP_CONTAINER_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTAINER_EDITOR, GimpContainerEditor))
#define GIMP_CONTAINER_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTAINER_EDITOR, GimpContainerEditorClass))
#define GIMP_IS_CONTAINER_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTAINER_EDITOR))
#define GIMP_IS_CONTAINER_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTAINER_EDITOR))
#define GIMP_CONTAINER_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTAINER_EDITOR, GimpContainerEditorClass))


typedef struct _GimpContainerEditorPrivate GimpContainerEditorPrivate;
typedef struct _GimpContainerEditorClass   GimpContainerEditorClass;

struct _GimpContainerEditor
{
  GtkBox             parent_instance;

  GimpContainerView *view;

  GimpContainerEditorPrivate *priv;
};

struct _GimpContainerEditorClass
{
  GtkBoxClass  parent_class;

  void (* select_item)   (GimpContainerEditor *editor,
                          GimpViewable        *object);
  void (* activate_item) (GimpContainerEditor *editor,
                          GimpViewable        *object);
};


GType            gimp_container_editor_get_type           (void) G_GNUC_CONST;

GtkSelectionMode gimp_container_editor_get_selection_mode (GimpContainerEditor *editor);
void             gimp_container_editor_set_selection_mode (GimpContainerEditor *editor,
                                                           GtkSelectionMode     mode);

void             gimp_container_editor_bind_to_async_set  (GimpContainerEditor *editor,
                                                           GimpAsyncSet        *async_set,
                                                           const gchar         *message);
