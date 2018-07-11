/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrusheditor.h
 * Copyright 1998 Jay Cox <jaycox@earthlink.net>
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

#ifndef  __GIMP_BRUSH_EDITOR_H__
#define  __GIMP_BRUSH_EDITOR_H__


#include "gimpdataeditor.h"


#define GIMP_TYPE_BRUSH_EDITOR            (gimp_brush_editor_get_type ())
#define GIMP_BRUSH_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BRUSH_EDITOR, GimpBrushEditor))
#define GIMP_BRUSH_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BRUSH_EDITOR, GimpBrushEditorClass))
#define GIMP_IS_BRUSH_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BRUSH_EDITOR))
#define GIMP_IS_BRUSH_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BRUSH_EDITOR))
#define GIMP_BRUSH_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_BRUSH_EDITOR, GimpBrushEditorClass))


typedef struct _GimpBrushEditorClass GimpBrushEditorClass;

struct _GimpBrushEditor
{
  GimpDataEditor  parent_instance;

  GtkWidget      *shape_group;
  GtkWidget      *options_box;
  GtkAdjustment  *radius_data;
  GtkAdjustment  *spikes_data;
  GtkAdjustment  *hardness_data;
  GtkAdjustment  *angle_data;
  GtkAdjustment  *aspect_ratio_data;
  GtkAdjustment  *spacing_data;
};

struct _GimpBrushEditorClass
{
  GimpDataEditorClass  parent_class;
};


GType       gimp_brush_editor_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_brush_editor_new      (GimpContext     *context,
                                        GimpMenuFactory *menu_factory);


#endif  /*  __GIMP_BRUSH_EDITOR_H__  */
