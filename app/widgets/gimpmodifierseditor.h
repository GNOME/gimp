/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmamodifierseditor.h
 * Copyright (C) 2022 Jehan
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

#ifndef __LIGMA_MODIFIERS_EDITOR_H__
#define __LIGMA_MODIFIERS_EDITOR_H__


#define LIGMA_TYPE_MODIFIERS_EDITOR            (ligma_modifiers_editor_get_type ())
#define LIGMA_MODIFIERS_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_MODIFIERS_EDITOR, LigmaModifiersEditor))
#define LIGMA_MODIFIERS_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_MODIFIERS_EDITOR, LigmaModifiersEditorClass))
#define LIGMA_IS_MODIFIERS_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_MODIFIERS_EDITOR))
#define LIGMA_IS_MODIFIERS_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_MODIFIERS_EDITOR))
#define LIGMA_MODIFIERS_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_MODIFIERS_EDITOR, LigmaModifiersEditorClass))


typedef struct _LigmaModifiersEditorPrivate LigmaModifiersEditorPrivate;
typedef struct _LigmaModifiersEditorClass   LigmaModifiersEditorClass;

struct _LigmaModifiersEditor
{
  LigmaFrame                  parent_instance;

  LigmaModifiersEditorPrivate *priv;
};

struct _LigmaModifiersEditorClass
{
  LigmaFrameClass             parent_class;
};


GType          ligma_modifiers_editor_get_type (void) G_GNUC_CONST;

GtkWidget    * ligma_modifiers_editor_new      (LigmaModifiersManager *manager);

void           ligma_modifiers_editor_clear    (LigmaModifiersEditor  *editor);


#endif /* __LIGMA_MODIFIERS_EDITOR_H__ */
