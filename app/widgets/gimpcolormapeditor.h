/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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


#define GIMP_TYPE_COLORMAP_EDITOR            (gimp_colormap_editor_get_type ())
#define GIMP_COLORMAP_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLORMAP_EDITOR, GimpColormapEditor))
#define GIMP_COLORMAP_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLORMAP_EDITOR, GimpColormapEditorClass))
#define GIMP_IS_COLORMAP_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLORMAP_EDITOR))
#define GIMP_IS_COLORMAP_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLORMAP_EDITOR))
#define GIMP_COLORMAP_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLORMAP_EDITOR, GimpColormapEditorClass))


typedef struct _GimpColormapEditorClass GimpColormapEditorClass;

struct _GimpColormapEditor
{
  GimpImageEditor  parent_instance;

  GtkWidget       *selection;
  GtkWidget       *color_dialog;
};

struct _GimpColormapEditorClass
{
  GimpImageEditorClass  parent_class;
};


GType       gimp_colormap_editor_get_type           (void) G_GNUC_CONST;

GtkWidget * gimp_colormap_editor_new                (GimpMenuFactory    *menu_factory);

void        gimp_colormap_editor_edit_color         (GimpColormapEditor *editor);
void        gimp_colormap_editor_delete_color       (GimpColormapEditor *editor);
gboolean    gimp_colormap_editor_is_color_deletable (GimpColormapEditor *editor);

gint        gimp_colormap_editor_get_index          (GimpColormapEditor *editor,
                                                     GeglColor          *search);
gboolean    gimp_colormap_editor_set_index          (GimpColormapEditor *editor,
                                                     gint                index,
                                                     GeglColor          *color);

gint        gimp_colormap_editor_max_index          (GimpColormapEditor *editor);
