/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#ifndef __GRADIENT_HEADER_H__
#define __GRADIENT_HEADER_H__

#include <gtk/gtk.h>

#define GRAD_LIST_WIDTH  300
#define GRAD_LIST_HEIGHT 80

#define GRAD_SCROLLBAR_STEP_SIZE 0.05
#define GRAD_SCROLLBAR_PAGE_SIZE 0.75

#define GRAD_CLOSE_BUTTON_WIDTH 45
#define GRAD_PREVIEW_WIDTH      600
#define GRAD_PREVIEW_HEIGHT     64
#define GRAD_CONTROL_HEIGHT     10

#define GRAD_CHECK_SIZE 8
#define GRAD_CHECK_SIZE_SM 4

#define GRAD_CHECK_DARK  (1.0 / 3.0)
#define GRAD_CHECK_LIGHT (2.0 / 3.0)

#define GRAD_COLOR_BOX_WIDTH  24
#define GRAD_COLOR_BOX_HEIGHT 16

#define GRAD_NUM_COLORS 10

#define GRAD_MOVE_TIME 150 /* ms between mouse click and detection of movement in gradient control */

#define GRAD_PREVIEW_EVENT_MASK (GDK_EXPOSURE_MASK | GDK_LEAVE_NOTIFY_MASK | \
				 GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK | \
				 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK)

#define GRAD_CONTROL_EVENT_MASK (GDK_EXPOSURE_MASK |		\
				 GDK_LEAVE_NOTIFY_MASK |	\
				 GDK_POINTER_MOTION_MASK |	\
				 GDK_POINTER_MOTION_HINT_MASK |	\
				 GDK_BUTTON_PRESS_MASK |	\
				 GDK_BUTTON_RELEASE_MASK |	\
				 GDK_BUTTON1_MOTION_MASK)

#define GRAD_UPDATE_PREVIEW   0x0001
#define GRAD_UPDATE_CONTROL   0x0002
#define GRAD_RESET_CONTROL    0X0004


/***** Types *****/

/* Gradient segment type */

typedef enum
{
  GRAD_LINEAR = 0,
  GRAD_CURVED,
  GRAD_SINE,
  GRAD_SPHERE_INCREASING,
  GRAD_SPHERE_DECREASING
} grad_type_t;

typedef enum
{
  GRAD_RGB = 0,  /* normal RGB */
  GRAD_HSV_CCW,  /* counterclockwise hue */
  GRAD_HSV_CW    /* clockwise hue */
} grad_color_t;

typedef struct _grad_segment_t
{
  double       left, middle, right; /* Left pos, midpoint, right pos */
  double       r0, g0, b0, a0;   	/* Left color */
  double       r1, g1, b1, a1;   	/* Right color */
  grad_type_t  type;             	/* Segment's blending function */
  grad_color_t color;             /* Segment's coloring type */

  struct _grad_segment_t *prev, *next; /* For linked list of segments */
} grad_segment_t;


/* Complete gradient type */

struct _gradient_t
{
  char           *name;
  grad_segment_t *segments;
  grad_segment_t *last_visited;
  int             dirty;
  char           *filename;
  GdkPixmap      *pixmap;
};


/* Gradient editor type */

typedef enum
{
  GRAD_DRAG_NONE = 0,
  GRAD_DRAG_LEFT,
  GRAD_DRAG_MIDDLE,
  GRAD_DRAG_ALL
} control_drag_mode_t;

typedef struct
{
  GtkWidget *shell;
  GdkGC *gc;
  GtkWidget *hint_label;
  GtkWidget *clist;
  GtkWidget *scrollbar;
  GtkWidget *preview;
  GtkWidget *control;

  /* Zoom and scrollbar */

  unsigned int  zoom_factor;
  GtkObject    *scroll_data;

  /* Instant update */

  int instant_update;

  /* Gradient preview */

  guchar   *preview_rows[2]; /* For caching redraw info */
  gint      preview_last_x;
  int       preview_button_down;

  /* Gradient control */

  GdkPixmap           *control_pixmap;
  grad_segment_t      *control_drag_segment;   /* Segment which is being dragged */
  grad_segment_t      *control_sel_l;          /* Left segment of selection */
  grad_segment_t      *control_sel_r;          /* Right segment of selection */
  control_drag_mode_t  control_drag_mode;      /* What is being dragged? */
  guint32              control_click_time;     /* Time when mouse was pressed */
  int                  control_compress;       /* Compressing/expanding handles */
  gint                 control_last_x;         /* Last mouse position when dragging */
  double               control_last_gx;        /* Last position (wrt gradient) when dragging */
  double               control_orig_pos;       /* Original click position when dragging */

  GtkWidget           *control_main_popup;              /* Popup menu */
  GtkWidget           *control_blending_label;          /* Blending function label */
  GtkWidget           *control_coloring_label;          /* Coloring type label */
  GtkWidget           *control_splitm_label;            /* Split at midpoint label */
  GtkWidget           *control_splitu_label;            /* Split uniformly label */
  GtkWidget           *control_delete_menu_item;        /* Delete menu item */
  GtkWidget           *control_delete_label;            /* Delete label */
  GtkWidget           *control_recenter_label;          /* Re-center label */
  GtkWidget           *control_redistribute_label;      /* Re-distribute label */
  GtkWidget           *control_flip_label;              /* Flip label */
  GtkWidget           *control_replicate_label;         /* Replicate label */
  GtkWidget           *control_blend_colors_menu_item;  /* Blend colors menu item */
  GtkWidget           *control_blend_opacity_menu_item; /* Blend opacity menu item */
  GtkWidget           *control_left_load_popup;         /* Left endpoint load menu */
  GtkWidget           *control_left_save_popup;         /* Left endpoint save menu */
  GtkWidget           *control_right_load_popup;        /* Right endpoint load menu */
  GtkWidget           *control_right_save_popup;        /* Right endpoint save menu */
  GtkWidget           *control_blending_popup;          /* Blending function menu */
  GtkWidget           *control_coloring_popup;          /* Coloring type menu */
  GtkWidget           *control_sel_ops_popup;           /* Selection ops menu */

  GtkAccelGroup *accel_group;

  /* Blending and coloring menus */

  GtkWidget *control_blending_items[5 + 1]; /* Add 1 for the "Varies" item */
  GtkWidget *control_coloring_items[3 + 1];

  /* Split uniformly dialog */

  int split_parts;

  /* Replicate dialog */

  int replicate_times;

  /* Saved colors */

  struct
  {
    double r, g, b, a;
  } saved_colors[GRAD_NUM_COLORS];

  GtkWidget *left_load_color_boxes[GRAD_NUM_COLORS + 3];
  GtkWidget *left_load_labels[GRAD_NUM_COLORS + 3];

  GtkWidget *left_save_color_boxes[GRAD_NUM_COLORS];
  GtkWidget *left_save_labels[GRAD_NUM_COLORS];

  GtkWidget *right_load_color_boxes[GRAD_NUM_COLORS + 3];
  GtkWidget *right_load_labels[GRAD_NUM_COLORS + 3];

  GtkWidget *right_save_color_boxes[GRAD_NUM_COLORS];
  GtkWidget *right_save_labels[GRAD_NUM_COLORS];

  /* Color dialogs */

  GtkWidget      *left_color_preview;
  grad_segment_t *left_saved_segments;
  int             left_saved_dirty;

  GtkWidget      *right_color_preview;
  grad_segment_t *right_saved_segments;
  int             right_saved_dirty;
} gradient_editor_t;

/* Selection dialog functions */
void sel_update_dialogs               (gint row, gradient_t *grad);
void grad_sel_free_all                (void);
void grad_sel_refill_all              (void);
void grad_sel_rename_all              (gint n, gradient_t *grad);
void grad_sel_new_all                 (gint n, gradient_t *grad);
void grad_sel_copy_all                (gint n, gradient_t *grad);
void grad_sel_delete_all              (gint n);
void grad_create_gradient_editor_init (gint need_show);
void ed_insert_in_gradients_listbox   (GdkGC *, GtkWidget *, gradient_t *grad,
				       int pos, int select);
gint grad_set_grad_to_name            (gchar *name);
gint ed_set_list_of_gradients         (GdkGC *, GtkWidget *, gradient_t *);

/* Varibles used */
extern gradient_t        *curr_gradient;   /* The active gradient   */
extern GSList            *gradients_list;  /* The list of gradients */
extern gradient_t        *grad_default_gradient;
extern gradient_editor_t *g_editor;        /* The gradient editor   */
extern int                num_gradients;

#define G_SAMPLE 40

typedef struct _GradSelect _GradSelect, *GradSelectP;

struct _GradSelect
{
  GtkWidget         *shell;
  GtkWidget         *frame;
  GtkWidget         *preview;
  GtkWidget         *clist;
  gchar             *callback_name;
  gradient_t        *grad;
  gint              sample_size;
  GdkColor          black;
  GdkGC             *gc;
};

GradSelectP gsel_new_selection (gchar * title, gchar * initial_gradient);
void        grad_select_free   (GradSelectP gsp);

extern GSList      *grad_active_dialogs;     /* List of active dialogs    */
extern GradSelectP  gradient_select_dialog;  /* The main selection dialog */

#endif  /*  __GRADIENT_HEADER_H__  */
