/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Gradient editor module copyight (C) 1996-1997 Federico Mena Quintero
 * federico@nuclecu.unam.mx
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GRADIENT_EDITOR_H__
#define __GRADIENT_EDITOR_H__


#define GRAD_NUM_COLORS 10


typedef enum
{
  GRAD_DRAG_NONE = 0,
  GRAD_DRAG_LEFT,
  GRAD_DRAG_MIDDLE,
  GRAD_DRAG_ALL
} GradientEditorDragMode;

typedef enum
{
  GRAD_UPDATE_GRADIENT = 1 << 0,
  GRAD_UPDATE_PREVIEW  = 1 << 1,
  GRAD_UPDATE_CONTROL  = 1 << 2,
  GRAD_RESET_CONTROL   = 1 << 3
} GradientEditorUpdateMask;


struct _GradientEditor
{
  GtkWidget   *shell;

  GtkWidget   *name;

  GtkWidget   *hint_label;
  GtkWidget   *scrollbar;
  GtkWidget   *preview;
  GtkWidget   *control;

  GimpContext *context;

  /*  Zoom and scrollbar  */
  guint        zoom_factor;
  GtkObject   *scroll_data;

  /*  Instant update  */
  guint        instant_update : 1;

  /*  Gradient preview  */
  guchar      *preview_rows[2]; /* For caching redraw info */
  gint         preview_last_x;
  guint        preview_button_down : 1;

  /*  Gradient control  */
  GdkPixmap              *control_pixmap;
  GimpGradientSegment    *control_drag_segment; /* Segment which is being dragged */
  GimpGradientSegment    *control_sel_l;        /* Left segment of selection */
  GimpGradientSegment    *control_sel_r;        /* Right segment of selection */
  GradientEditorDragMode  control_drag_mode;    /* What is being dragged? */
  guint32                 control_click_time;   /* Time when mouse was pressed */
  guint                   control_compress : 1; /* Compressing/expanding handles */
  gint                    control_last_x;       /* Last mouse position when dragging */
  gdouble                 control_last_gx;      /* Last position (wrt gradient) when dragging */
  gdouble                 control_orig_pos;     /* Original click position when dragging */

  /*  Split uniformly dialog  */
  gint          split_parts;

  /*  Replicate dialog  */
  gint          replicate_times;

  /*  Saved colors  */
  GimpRGB       saved_colors[GRAD_NUM_COLORS];

  /*  Color dialogs  */
  GimpGradientSegment *left_saved_segments;
  GimpGradientSegment *right_saved_segments;
  
  guint left_saved_dirty : 1;
  guint right_saved_dirty : 1;
};


GradientEditor * gradient_editor_new (Gimp                     *gimp);

void   gradient_editor_set_gradient  (GradientEditor           *gradient_editor,
				      GimpGradient             *gradient);
void   gradient_editor_free          (GradientEditor           *gradient_editor);

void   gradient_editor_update        (GradientEditor           *gradient_editor,
                                      GradientEditorUpdateMask  flags);


#endif  /* __GRADIENT_EDITOR_H__ */
