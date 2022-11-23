/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabrusheditor.h
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

#ifndef  __LIGMA_BRUSH_EDITOR_H__
#define  __LIGMA_BRUSH_EDITOR_H__


#include "ligmadataeditor.h"


#define LIGMA_TYPE_BRUSH_EDITOR            (ligma_brush_editor_get_type ())
#define LIGMA_BRUSH_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BRUSH_EDITOR, LigmaBrushEditor))
#define LIGMA_BRUSH_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BRUSH_EDITOR, LigmaBrushEditorClass))
#define LIGMA_IS_BRUSH_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BRUSH_EDITOR))
#define LIGMA_IS_BRUSH_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BRUSH_EDITOR))
#define LIGMA_BRUSH_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BRUSH_EDITOR, LigmaBrushEditorClass))


typedef struct _LigmaBrushEditorClass LigmaBrushEditorClass;

struct _LigmaBrushEditor
{
  LigmaDataEditor  parent_instance;

  GtkWidget      *shape_group;
  GtkWidget      *options_box;
  GtkAdjustment  *radius_data;
  GtkAdjustment  *spikes_data;
  GtkAdjustment  *hardness_data;
  GtkAdjustment  *angle_data;
  GtkAdjustment  *aspect_ratio_data;
  GtkAdjustment  *spacing_data;
};

struct _LigmaBrushEditorClass
{
  LigmaDataEditorClass  parent_class;
};


GType       ligma_brush_editor_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_brush_editor_new      (LigmaContext     *context,
                                        LigmaMenuFactory *menu_factory);


#endif  /*  __LIGMA_BRUSH_EDITOR_H__  */
