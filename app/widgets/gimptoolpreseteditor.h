/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_TOOL_PRESET_EDITOR_H__
#define __GIMP_TOOL_PRESET_EDITOR_H__


#include "gimpdataeditor.h"


#define GIMP_TYPE_TOOL_PRESET_EDITOR            (gimp_tool_preset_editor_get_type ())
#define GIMP_TOOL_PRESET_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_PRESET_EDITOR, GimpToolPresetEditor))
#define GIMP_TOOL_PRESET_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_PRESET_EDITOR, GimpToolPresetEditorClass))
#define GIMP_IS_TOOL_PRESET_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_PRESET_EDITOR))
#define GIMP_IS_TOOL_PRESET_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_PRESET_EDITOR))
#define GIMP_TOOL_PRESET_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_PRESET_EDITOR, GimpToolPresetEditorClass))


typedef struct _GimpToolPresetEditorPrivate GimpToolPresetEditorPrivate;
typedef struct _GimpToolPresetEditorClass   GimpToolPresetEditorClass;

struct _GimpToolPresetEditor
{
  GimpDataEditor               parent_instance;

  GimpToolPresetEditorPrivate *priv;
};

struct _GimpToolPresetEditorClass
{
  GimpDataEditorClass  parent_class;
};


GType       gimp_tool_preset_editor_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_tool_preset_editor_new      (GimpContext     *context,
                                              GimpMenuFactory *menu_factory);


#endif /* __GIMP_TOOL_PRESET_EDITOR_H__ */
