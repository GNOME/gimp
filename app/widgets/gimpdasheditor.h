/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdasheditor.h
 * Copyright (C) 2003 Simon Budig  <simon@gimp.org>
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

#ifndef __GIMP_DASH_EDITOR_H__
#define __GIMP_DASH_EDITOR_H__


#define GIMP_TYPE_DASH_EDITOR            (gimp_dash_editor_get_type ())
#define GIMP_DASH_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DASH_EDITOR, GimpDashEditor))
#define GIMP_DASH_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DASH_EDITOR, GimpDashEditorClass))
#define GIMP_IS_DASH_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DASH_EDITOR))
#define GIMP_IS_DASH_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DASH_EDITOR))
#define GIMP_DASH_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DASH_EDITOR, GimpDashEditorClass))


typedef struct _GimpDashEditorClass  GimpDashEditorClass;

struct _GimpDashEditor
{
  GtkDrawingArea     parent_instance;

  GimpStrokeOptions *stroke_options;
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

struct _GimpDashEditorClass
{
  GtkDrawingAreaClass  parent_class;
};


GType       gimp_dash_editor_get_type    (void) G_GNUC_CONST;

GtkWidget * gimp_dash_editor_new         (GimpStrokeOptions *stroke_options);

void        gimp_dash_editor_shift_left  (GimpDashEditor    *editor);
void        gimp_dash_editor_shift_right (GimpDashEditor    *editor);


#endif /* __GIMP_DASH_EDITOR_H__ */
