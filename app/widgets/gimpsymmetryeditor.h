/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasymmetryeditor.h
 * Copyright (C) 2015 Jehan <jehan@girinstud.io>
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

#ifndef __LIGMA_SYMMETRY_EDITOR_H__
#define __LIGMA_SYMMETRY_EDITOR_H__


#include "ligmaimageeditor.h"


#define LIGMA_TYPE_SYMMETRY_EDITOR            (ligma_symmetry_editor_get_type ())
#define LIGMA_SYMMETRY_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SYMMETRY_EDITOR, LigmaSymmetryEditor))
#define LIGMA_SYMMETRY_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SYMMETRY_EDITOR, LigmaSymmetryEditorClass))
#define LIGMA_IS_SYMMETRY_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SYMMETRY_EDITOR))
#define LIGMA_IS_SYMMETRY_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SYMMETRY_EDITOR))
#define LIGMA_SYMMETRY_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SYMMETRY_EDITOR, LigmaSymmetryEditorClass))


typedef struct _LigmaSymmetryEditorPrivate  LigmaSymmetryEditorPrivate;
typedef struct _LigmaSymmetryEditorClass    LigmaSymmetryEditorClass;

struct _LigmaSymmetryEditor
{
  LigmaImageEditor            parent_instance;

  LigmaSymmetryEditorPrivate *p;
};

struct _LigmaSymmetryEditorClass
{
  LigmaImageEditorClass  parent_class;
};


GType       ligma_symmetry_editor_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_symmetry_editor_new      (LigmaMenuFactory *menu_factory);


#endif  /*  __LIGMA_SYMMETRY_EDITOR_H__  */
