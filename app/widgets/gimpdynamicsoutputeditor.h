/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadynamicsoutputeditor.h
 * Copyright (C) 2010 Alexia Death
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

#ifndef __LIGMA_DYNAMICS_OUTPUT_EDITOR_H__
#define __LIGMA_DYNAMICS_OUTPUT_EDITOR_H__


#define LIGMA_TYPE_DYNAMICS_OUTPUT_EDITOR            (ligma_dynamics_output_editor_get_type ())
#define LIGMA_DYNAMICS_OUTPUT_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DYNAMICS_OUTPUT_EDITOR, LigmaDynamicsOutputEditor))
#define LIGMA_DYNAMICS_OUTPUT_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DYNAMICS_OUTPUT_EDITOR, LigmaDynamicsOutputEditorClass))
#define LIGMA_IS_DYNAMICS_OUTPUT_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DYNAMICS_OUTPUT_EDITOR))
#define LIGMA_IS_DYNAMICS_OUTPUT_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DYNAMICS_OUTPUT_EDITOR))
#define LIGMA_DYNAMICS_OUTPUT_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DYNAMICS_OUTPUT_EDITOR, LigmaDynamicsOutputEditorClass))


typedef struct _LigmaDynamicsOutputEditorClass LigmaDynamicsOutputEditorClass;

struct _LigmaDynamicsOutputEditor
{
  GtkBox  parent_instance;
};

struct _LigmaDynamicsOutputEditorClass
{
  GtkBoxClass  parent_class;
};


GType       ligma_dynamics_output_editor_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_dynamics_output_editor_new      (LigmaDynamicsOutput *output);


#endif /* __LIGMA_DYNAMICS_OUTPUT_EDITOR_H__ */
