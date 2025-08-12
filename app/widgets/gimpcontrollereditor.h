/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontrollereditor.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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


#define GIMP_TYPE_CONTROLLER_EDITOR            (gimp_controller_editor_get_type ())
#define GIMP_CONTROLLER_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONTROLLER_EDITOR, GimpControllerEditor))
#define GIMP_CONTROLLER_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONTROLLER_EDITOR, GimpControllerEditorClass))
#define GIMP_IS_CONTROLLER_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONTROLLER_EDITOR))
#define GIMP_IS_CONTROLLER_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONTROLLER_EDITOR))
#define GIMP_CONTROLLER_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONTROLLER_EDITOR, GimpControllerEditorClass))


typedef struct _GimpControllerEditorClass GimpControllerEditorClass;

struct _GimpControllerEditor
{
  GtkBox              parent_instance;

  GimpControllerInfo *info;
  GimpContext        *context;

  GtkTreeSelection   *sel;

  GtkWidget          *grab_button;
  GtkWidget          *edit_button;
  GtkWidget          *delete_button;

  GtkWidget          *edit_dialog;
  GtkTreeSelection   *edit_sel;
};

struct _GimpControllerEditorClass
{
  GtkBoxClass   parent_class;
};


GType       gimp_controller_editor_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_controller_editor_new      (GimpControllerInfo *info,
                                             GimpContext        *context);
