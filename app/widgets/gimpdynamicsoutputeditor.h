/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdynamicsoutputeditor.h
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

#pragma once


#define GIMP_TYPE_DYNAMICS_OUTPUT_EDITOR            (gimp_dynamics_output_editor_get_type ())
#define GIMP_DYNAMICS_OUTPUT_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DYNAMICS_OUTPUT_EDITOR, GimpDynamicsOutputEditor))
#define GIMP_DYNAMICS_OUTPUT_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DYNAMICS_OUTPUT_EDITOR, GimpDynamicsOutputEditorClass))
#define GIMP_IS_DYNAMICS_OUTPUT_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DYNAMICS_OUTPUT_EDITOR))
#define GIMP_IS_DYNAMICS_OUTPUT_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DYNAMICS_OUTPUT_EDITOR))
#define GIMP_DYNAMICS_OUTPUT_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DYNAMICS_OUTPUT_EDITOR, GimpDynamicsOutputEditorClass))


typedef struct _GimpDynamicsOutputEditorClass GimpDynamicsOutputEditorClass;

struct _GimpDynamicsOutputEditor
{
  GtkBox  parent_instance;
};

struct _GimpDynamicsOutputEditorClass
{
  GtkBoxClass  parent_class;
};


GType       gimp_dynamics_output_editor_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_dynamics_output_editor_new      (GimpDynamicsOutput *output);
