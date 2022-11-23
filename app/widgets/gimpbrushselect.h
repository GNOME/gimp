/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmabrushselect.h
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_BRUSH_SELECT_H__
#define __LIGMA_BRUSH_SELECT_H__

#include "ligmapdbdialog.h"

G_BEGIN_DECLS


#define LIGMA_TYPE_BRUSH_SELECT            (ligma_brush_select_get_type ())
#define LIGMA_BRUSH_SELECT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_BRUSH_SELECT, LigmaBrushSelect))
#define LIGMA_BRUSH_SELECT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_BRUSH_SELECT, LigmaBrushSelectClass))
#define LIGMA_IS_BRUSH_SELECT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_BRUSH_SELECT))
#define LIGMA_IS_BRUSH_SELECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_BRUSH_SELECT))
#define LIGMA_BRUSH_SELECT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_BRUSH_SELECT, LigmaBrushSelectClass))


typedef struct _LigmaBrushSelectClass  LigmaBrushSelectClass;

struct _LigmaBrushSelect
{
  LigmaPdbDialog  parent_instance;

  gdouble        initial_opacity;
  LigmaLayerMode  initial_mode;

  gint           spacing;
  GtkAdjustment *opacity_data;
  GtkWidget     *layer_mode_box;
};

struct _LigmaBrushSelectClass
{
  LigmaPdbDialogClass  parent_class;
};


GType  ligma_brush_select_get_type (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __LIGMA_BRUSH_SELECT_H__ */
