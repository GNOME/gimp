/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadasheditor.h
 * Copyright (C) 2003 Simon Budig  <simon@ligma.org>
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

#ifndef __LIGMA_DASH_EDITOR_H__
#define __LIGMA_DASH_EDITOR_H__


#define LIGMA_TYPE_DASH_EDITOR            (ligma_dash_editor_get_type ())
#define LIGMA_DASH_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DASH_EDITOR, LigmaDashEditor))
#define LIGMA_DASH_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DASH_EDITOR, LigmaDashEditorClass))
#define LIGMA_IS_DASH_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DASH_EDITOR))
#define LIGMA_IS_DASH_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DASH_EDITOR))
#define LIGMA_DASH_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DASH_EDITOR, LigmaDashEditorClass))


typedef struct _LigmaDashEditorClass  LigmaDashEditorClass;

struct _LigmaDashEditor
{
  GtkDrawingArea     parent_instance;

  LigmaStrokeOptions *stroke_options;
  gdouble            dash_length;

  /* GUI stuff */
  gint               n_segments;
  gboolean          *segments;

  /* coordinates of the first block main dash pattern */
  gint               x0;
  gint               y0;
  gint               block_width;
  gint               block_height;

  gboolean           edit_mode;
  gint               edit_button_x0;
};

struct _LigmaDashEditorClass
{
  GtkDrawingAreaClass  parent_class;
};


GType       ligma_dash_editor_get_type    (void) G_GNUC_CONST;

GtkWidget * ligma_dash_editor_new         (LigmaStrokeOptions *stroke_options);

void        ligma_dash_editor_shift_left  (LigmaDashEditor    *editor);
void        ligma_dash_editor_shift_right (LigmaDashEditor    *editor);


#endif /* __LIGMA_DASH_EDITOR_H__ */
