/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#ifndef __LIGMA_DYNAMICS_EDITOR_H__
#define __LIGMA_DYNAMICS_EDITOR_H__


#include "ligmadataeditor.h"


#define LIGMA_TYPE_DYNAMICS_EDITOR            (ligma_dynamics_editor_get_type ())
#define LIGMA_DYNAMICS_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DYNAMICS_EDITOR, LigmaDynamicsEditor))
#define LIGMA_DYNAMICS_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DYNAMICS_EDITOR, LigmaDynamicsEditorClass))
#define LIGMA_IS_DYNAMICS_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DYNAMICS_EDITOR))
#define LIGMA_IS_DYNAMICS_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DYNAMICS_EDITOR))
#define LIGMA_DYNAMICS_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DYNAMICS_EDITOR, LigmaDynamicsEditorClass))


typedef struct _LigmaDynamicsEditorClass LigmaDynamicsEditorClass;

struct _LigmaDynamicsEditor
{
  LigmaDataEditor  parent_instance;

  LigmaDynamics   *dynamics_model;

  GtkWidget      *view_selector;
  GtkWidget      *notebook;
};

struct _LigmaDynamicsEditorClass
{
  LigmaDataEditorClass  parent_class;
};


GType       ligma_dynamics_editor_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_dynamics_editor_new      (LigmaContext      *context,
                                           LigmaMenuFactory  *menu_factory);


#endif /* __LIGMA_DYNAMICS_EDITOR_H__ */
