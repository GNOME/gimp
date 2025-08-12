/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcomponenteditor.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "gimpimageeditor.h"


#define GIMP_TYPE_COMPONENT_EDITOR            (gimp_component_editor_get_type ())
#define GIMP_COMPONENT_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COMPONENT_EDITOR, GimpComponentEditor))
#define GIMP_COMPONENT_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COMPONENT_EDITOR, GimpComponentEditorClass))
#define GIMP_IS_COMPONENT_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COMPONENT_EDITOR))
#define GIMP_IS_COMPONENT_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COMPONENT_EDITOR))
#define GIMP_COMPONENT_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COMPONENT_EDITOR, GimpComponentEditorClass))


typedef struct _GimpComponentEditorClass  GimpComponentEditorClass;

struct _GimpComponentEditor
{
  GimpImageEditor    parent_instance;

  gint               view_size;

  GtkTreeModel      *model;
  GtkTreeView       *view;
  GtkTreeSelection  *selection;

  GtkTreeViewColumn *eye_column;
  GtkCellRenderer   *eye_cell;
  GtkCellRenderer   *renderer_cell;

  GimpChannelType    clicked_component;
};

struct _GimpComponentEditorClass
{
  GimpImageEditorClass  parent_class;
};


GType       gimp_component_editor_get_type      (void) G_GNUC_CONST;

GtkWidget * gimp_component_editor_new           (gint                 view_size,
                                                 GimpMenuFactory     *menu_factory);
void        gimp_component_editor_set_view_size (GimpComponentEditor *editor,
                                                 gint                 view_size);
