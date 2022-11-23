/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Gradient editor module copyight (C) 1996-1997 Federico Mena Quintero
 * federico@nuclecu.unam.mx
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

#ifndef __LIGMA_GRADIENT_EDITOR_H__
#define __LIGMA_GRADIENT_EDITOR_H__


#include "ligmadataeditor.h"


#define GRAD_NUM_COLORS 10


typedef enum
{
  GRAD_DRAG_NONE = 0,
  GRAD_DRAG_LEFT,
  GRAD_DRAG_MIDDLE,
  GRAD_DRAG_ALL
} GradientEditorDragMode;


#define LIGMA_TYPE_GRADIENT_EDITOR            (ligma_gradient_editor_get_type ())
#define LIGMA_GRADIENT_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_GRADIENT_EDITOR, LigmaGradientEditor))
#define LIGMA_GRADIENT_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_GRADIENT_EDITOR, LigmaGradientEditorClass))
#define LIGMA_IS_GRADIENT_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_GRADIENT_EDITOR))
#define LIGMA_IS_GRADIENT_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_GRADIENT_EDITOR))
#define LIGMA_GRADIENT_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_GRADIENT_EDITOR, LigmaGradientEditorClass))


typedef struct _LigmaGradientEditorClass LigmaGradientEditorClass;

struct _LigmaGradientEditor
{
  LigmaDataEditor          parent_instance;

  GtkWidget              *current_color;
  GtkWidget              *hint_label1;
  GtkWidget              *hint_label2;
  GtkWidget              *hint_label3;
  GtkWidget              *hint_label4;
  GtkWidget              *scrollbar;
  GtkWidget              *control;

  /*  Zoom and scrollbar  */
  gdouble                 zoom_factor;
  GtkAdjustment          *scroll_data;
  GtkGesture             *zoom_gesture;
  gdouble                 last_zoom_scale;

  /*  Gradient view  */
  gint                    view_last_x;          /* -1 if mouse has left focus */
  gboolean                view_button_down;

  /*  Gradient control  */
  LigmaGradientSegment    *control_drag_segment; /* Segment which is being dragged */
  LigmaGradientSegment    *control_sel_l;        /* Left segment of selection */
  LigmaGradientSegment    *control_sel_r;        /* Right segment of selection */
  GradientEditorDragMode  control_drag_mode;    /* What is being dragged? */
  guint32                 control_click_time;   /* Time when mouse was pressed */
  gboolean                control_compress;     /* Compressing/expanding handles */
  gint                    control_last_x;       /* Last mouse position, -1 if out of focus */
  gdouble                 control_last_gx;      /* Last position (wrt gradient) when dragging */
  gdouble                 control_orig_pos;     /* Original click position when dragging */

  LigmaGradientBlendColorSpace  blend_color_space;

  /*  Saved colors  */
  LigmaRGB                 saved_colors[GRAD_NUM_COLORS];

  /*  Color dialog  */
  GtkWidget              *color_dialog;
  LigmaGradientSegment    *saved_segments;
  gboolean                saved_dirty;
};

struct _LigmaGradientEditorClass
{
  LigmaDataEditorClass  parent_class;
};


GType       ligma_gradient_editor_get_type         (void) G_GNUC_CONST;

GtkWidget * ligma_gradient_editor_new              (LigmaContext          *context,
                                                   LigmaMenuFactory      *menu_factory);

void        ligma_gradient_editor_get_selection    (LigmaGradientEditor   *editor,
                                                   LigmaGradient        **gradient,
                                                   LigmaGradientSegment **left,
                                                   LigmaGradientSegment **right);
void        ligma_gradient_editor_set_selection    (LigmaGradientEditor   *editor,
                                                   LigmaGradientSegment  *left,
                                                   LigmaGradientSegment  *right);

void        ligma_gradient_editor_edit_left_color  (LigmaGradientEditor   *editor);
void        ligma_gradient_editor_edit_right_color (LigmaGradientEditor   *editor);

void        ligma_gradient_editor_zoom             (LigmaGradientEditor   *editor,
                                                   LigmaZoomType          zoom_type,
                                                   gdouble               delta,
                                                   gdouble               zoom_focus_x);


#endif  /* __LIGMA_GRADIENT_EDITOR_H__ */
