/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptooleditor.h
 * Copyright (C) 2001-2009 Michael Natterer <mitch@gimp.org>
 *                         Stephen Griffiths <scgmk5@gmail.com>
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

#include "gimpcontainertreeview.h"


#define GIMP_TYPE_TOOL_EDITOR            (gimp_tool_editor_get_type ())
#define GIMP_TOOL_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_EDITOR, GimpToolEditor))
#define GIMP_TOOL_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_EDITOR, GimpToolEditorClass))
#define GIMP_IS_TOOL_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_EDITOR))
#define GIMP_IS_TOOL_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_EDITOR))
#define GIMP_TOOL_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_EDITOR, GimpToolEditorClass))


typedef struct _GimpToolEditorPrivate GimpToolEditorPrivate;
typedef struct _GimpToolEditorClass   GimpToolEditorClass;

struct _GimpToolEditor
{
  GimpContainerTreeView  parent_instance;

  GimpToolEditorPrivate *priv;
};

struct _GimpToolEditorClass
{
  GimpContainerTreeViewClass  parent_class;
};


GType       gimp_tool_editor_get_type       (void) G_GNUC_CONST;

GtkWidget * gimp_tool_editor_new            (GimpContainer  *container,
                                             GimpContext    *context,
                                             gint            view_size,
                                             gint            view_border_width);

void        gimp_tool_editor_revert_changes (GimpToolEditor *tool_editor);
