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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURIGHTE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Special thanks to:
 *
 * Luis Albarran (luis4@mindspring.com) - Nice UI suggestions
 *
 * Miguel de Icaza (miguel@nuclecu.unam.mx) - Pop-up menu suggestion
 *
 * Marcelo Malheiros (malheiro@dca.fee.unicamp.br) - many, many
 * suggestions, nice gradient files
 *
 * Adam Moss (adam@uunet.pipex.com) - idea for the hint bar
 *
 * Everyone on #gimp - many suggestions
 */

/* TODO:
 *
 * - Fix memory leaks: grad_free_gradient_editor() and any others
 * which I may have missed.
 *
 * - Add all of Marcelo's neat suggestions:
 *   - Hue rotate, saturation, brightness, contrast.
 *
 * - Better handling of bogus gradient files and inconsistent
 *   segments.  Do not loop indefinitely in seg_get_segment_at() if
 *   there is a missing segment between two others.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimpgradient.h"

#include "widgets/gimpdnd.h"

#include "color-notebook.h"
#include "gradient-editor.h"

#include "errors.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"


#define EPSILON 1e-10

#define GRAD_SCROLLBAR_STEP_SIZE 0.05
#define GRAD_SCROLLBAR_PAGE_SIZE 0.75

#define GRAD_PREVIEW_WIDTH  600
#define GRAD_PREVIEW_HEIGHT  64
#define GRAD_CONTROL_HEIGHT  10

#define GRAD_COLOR_BOX_WIDTH  24
#define GRAD_COLOR_BOX_HEIGHT 16

#define GRAD_NUM_COLORS 10

#define GRAD_MOVE_TIME 150 /* ms between mouse click and detection of movement in gradient control */


#define GRAD_PREVIEW_EVENT_MASK (GDK_EXPOSURE_MASK            | \
                                 GDK_LEAVE_NOTIFY_MASK        | \
				 GDK_POINTER_MOTION_MASK      | \
                                 GDK_POINTER_MOTION_HINT_MASK | \
				 GDK_BUTTON_PRESS_MASK        | \
                                 GDK_BUTTON_RELEASE_MASK)

#define GRAD_CONTROL_EVENT_MASK (GDK_EXPOSURE_MASK            | \
				 GDK_LEAVE_NOTIFY_MASK        | \
				 GDK_POINTER_MOTION_MASK      | \
				 GDK_POINTER_MOTION_HINT_MASK | \
				 GDK_BUTTON_PRESS_MASK        | \
				 GDK_BUTTON_RELEASE_MASK      | \
				 GDK_BUTTON1_MOTION_MASK)


enum
{
  GRAD_UPDATE_GRADIENT = 1 << 0,
  GRAD_UPDATE_PREVIEW  = 1 << 1,
  GRAD_UPDATE_CONTROL  = 1 << 2,
  GRAD_RESET_CONTROL   = 1 << 3
};

/* Gradient editor type */

typedef enum
{
  GRAD_DRAG_NONE = 0,
  GRAD_DRAG_LEFT,
  GRAD_DRAG_MIDDLE,
  GRAD_DRAG_ALL
} control_drag_mode_t;


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
  gboolean     instant_update;

  /*  Gradient preview  */
  guchar      *preview_rows[2]; /* For caching redraw info */
  gint         preview_last_x;
  gboolean     preview_button_down;

  /*  Gradient control  */
  GdkPixmap           *control_pixmap;
  GimpGradientSegment *control_drag_segment; /* Segment which is being dragged */
  GimpGradientSegment *control_sel_l;        /* Left segment of selection */
  GimpGradientSegment *control_sel_r;        /* Right segment of selection */
  control_drag_mode_t  control_drag_mode;    /* What is being dragged? */
  guint32              control_click_time;   /* Time when mouse was pressed */
  gboolean             control_compress;     /* Compressing/expanding handles */
  gint                 control_last_x;       /* Last mouse position when dragging */
  gdouble              control_last_gx;      /* Last position (wrt gradient) when dragging */
  gdouble              control_orig_pos;     /* Original click position when dragging */

  GtkWidget     *control_main_popup;              /* Popup menu */
  GtkWidget     *control_blending_label;          /* Blending function label */
  GtkWidget     *control_coloring_label;          /* Coloring type label */
  GtkWidget     *control_split_m_label;           /* Split at midpoint label */
  GtkWidget     *control_split_u_label;           /* Split uniformly label */
  GtkWidget     *control_delete_menu_item;        /* Delete menu item */
  GtkWidget     *control_delete_label;            /* Delete label */
  GtkWidget     *control_recenter_label;          /* Re-center label */
  GtkWidget     *control_redistribute_label;      /* Re-distribute label */
  GtkWidget     *control_flip_label;              /* Flip label */
  GtkWidget     *control_replicate_label;         /* Replicate label */
  GtkWidget     *control_blend_colors_menu_item;  /* Blend colors menu item */
  GtkWidget     *control_blend_opacity_menu_item; /* Blend opacity menu item */
  GtkWidget     *control_left_load_popup;         /* Left endpoint load menu */
  GtkWidget     *control_left_save_popup;         /* Left endpoint save menu */
  GtkWidget     *control_right_load_popup;        /* Right endpoint load menu */
  GtkWidget     *control_right_save_popup;        /* Right endpoint save menu */
  GtkWidget     *control_blending_popup;          /* Blending function menu */
  GtkWidget     *control_coloring_popup;          /* Coloring type menu */
  GtkWidget     *control_sel_ops_popup;           /* Selection ops menu */

  GtkAccelGroup *accel_group;

  /*  Blending and coloring menus  */
  GtkWidget     *control_blending_items[5 + 1]; /* Add 1 for the "Varies" item */
  GtkWidget     *control_coloring_items[3 + 1];

  /*  Split uniformly dialog  */
  gint          split_parts;

  /*  Replicate dialog  */
  gint          replicate_times;

  /*  Saved colors  */
  GimpRGB       saved_colors[GRAD_NUM_COLORS];

  GtkWidget    *left_load_color_boxes[GRAD_NUM_COLORS + 3];
  GtkWidget    *left_load_labels[GRAD_NUM_COLORS + 3];

  GtkWidget    *left_save_color_boxes[GRAD_NUM_COLORS];
  GtkWidget    *left_save_labels[GRAD_NUM_COLORS];

  GtkWidget    *right_load_color_boxes[GRAD_NUM_COLORS + 3];
  GtkWidget    *right_load_labels[GRAD_NUM_COLORS + 3];

  GtkWidget    *right_save_color_boxes[GRAD_NUM_COLORS];
  GtkWidget    *right_save_labels[GRAD_NUM_COLORS];

  /*  Color dialogs  */
  GtkWidget           *left_color_preview;
  GimpGradientSegment *left_saved_segments;
  gboolean             left_saved_dirty;

  GtkWidget           *right_color_preview;
  GimpGradientSegment *right_saved_segments;
  gboolean             right_saved_dirty;
};


/***** Local functions *****/

static void       gradient_editor_name_activate    (GtkWidget      *widget,
						    GradientEditor *gradient_editor);
static void       gradient_editor_name_focus_out   (GtkWidget      *widget,
						    GdkEvent       *event,
						    GradientEditor *gradient_editor);
static void       gradient_editor_drop_gradient    (GtkWidget      *widget,
						    GimpViewable   *viewable,
						    gpointer        data);
static void       gradient_editor_gradient_changed (GimpContext    *context,
						    GimpGradient   *gradient,
						    GradientEditor *gradient_editor);

/* Gradient editor functions */

static void      ed_update_editor                 (GradientEditor  *gradient_editor,
						   gint             flags);

static void      ed_set_hint                      (GradientEditor  *gradient_editor,
						   gchar           *str);

static void      ed_initialize_saved_colors       (GradientEditor  *gradient_editor);

/* Main dialog button callbacks & functions */

static void      ed_close_callback                (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);

/* Zoom, scrollbar & instant update callbacks */

static void      ed_scrollbar_update              (GtkAdjustment   *adjustment,
						   GradientEditor  *gradient_editor);
static void      ed_zoom_all_callback             (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);
static void      ed_zoom_out_callback             (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);
static void      ed_zoom_in_callback              (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);
static void      ed_instant_update_update         (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);

/* Gradient preview functions */

static gint      preview_events                   (GtkWidget       *widget,
						   GdkEvent        *event,
						   GradientEditor  *gradient_editor);
static void      preview_set_hint                 (GradientEditor  *gradient_editor,
						   gint             x);

static void      preview_set_foreground           (GradientEditor  *gradient_editor,
						   gint             x);
static void      preview_set_background           (GradientEditor  *gradient_editor,
						   gint             x);

static void      preview_update                   (GradientEditor  *gradient_editor,
						   gboolean         recalculate);
static void      preview_fill_image               (GradientEditor  *gradient_editor,
						   gint             width,
						   gint             height,
						   gdouble          left,
						   gdouble          right);

/* Gradient control functions */

static gint      control_events                   (GtkWidget       *widget,
						   GdkEvent        *event,
						   GradientEditor  *gradient_editor);
static void      control_do_hint                  (GradientEditor  *gradient_editor,
						   gint             x,
						   gint             y);
static void      control_button_press             (GradientEditor  *gradient_editor,
						   gint             x,
						   gint             y,
						   guint            button,
						   guint            state);
static gboolean  control_point_in_handle          (GradientEditor  *gradient_editor,
						   GimpGradient    *gradient,
						   gint             x,
						   gint             y,
						   GimpGradientSegment  *seg,
						   control_drag_mode_t handle);
static void      control_select_single_segment    (GradientEditor       *gradient_editor,
						   GimpGradientSegment  *seg);
static void      control_extend_selection         (GradientEditor       *gradient_editor,
						   GimpGradientSegment  *seg,
						   gdouble               pos);
static void      control_motion                   (GradientEditor  *gradient_editor,
						   GimpGradient    *gradient,
						   gint             x);

static void      control_compress_left            (GimpGradientSegment *range_l,
						   GimpGradientSegment *range_r,
						   GimpGradientSegment *drag_seg,
						   gdouble              pos);
static void      control_compress_range           (GimpGradientSegment *range_l,
						   GimpGradientSegment *range_r,
						   gdouble              new_l,
						   gdouble              new_r);

static double    control_move                     (GradientEditor      *gradient_editor,
						   GimpGradientSegment *range_l,
						   GimpGradientSegment *range_r,
						   gdouble              delta);

/* Control update/redraw functions */

static void      control_update                   (GradientEditor  *gradient_editor,
						   GimpGradient    *gradient,
						   gboolean         recalculate);
static void      control_draw                     (GradientEditor  *gradient_editor,
						   GimpGradient    *gradient,
						   GdkPixmap       *pixmap,
						   gint             width,
						   gint             height,
						   gdouble          left,
						   gdouble          right);
static void      control_draw_normal_handle       (GradientEditor  *gradient_editor,
						   GdkPixmap       *pixmap,
						   gdouble          pos,
						   gint             height);
static void      control_draw_middle_handle       (GradientEditor  *gradient_editor,
						   GdkPixmap       *pixmap,
						   gdouble          pos,
						   gint             height);
static void      control_draw_handle              (GdkPixmap       *pixmap,
						   GdkGC           *border_gc,
						   GdkGC           *fill_gc,
						   gint             xpos,
						   gint             height);

static gint      control_calc_p_pos               (GradientEditor  *gradient_editor,
						   gdouble          pos);
static gdouble   control_calc_g_pos               (GradientEditor  *gradient_editor,
						   gint             pos);

/* Control popup functions */

static void      cpopup_create_main_menu          (GradientEditor  *gradient_editor);						   
static void      cpopup_do_popup                  (GradientEditor  *gradient_editor);						   
static GtkWidget * cpopup_create_color_item           (GtkWidget  **color_box,
						       GtkWidget  **label);
static GtkWidget * cpopup_create_menu_item_with_label (gchar       *str,
						       GtkWidget  **label);

static void      cpopup_adjust_menus              (GradientEditor  *gradient_editor);
static void      cpopup_adjust_blending_menu      (GradientEditor  *gradient_editor);
static void      cpopup_adjust_coloring_menu      (GradientEditor  *gradient_editor);
static void      cpopup_check_selection_params    (GradientEditor  *gradient_editor,
						   gint            *equal_blending,
						   gint            *equal_coloring);

static void      cpopup_render_color_box          (GtkPreview      *preview,
						   GimpRGB         *color);

static GtkWidget * cpopup_create_load_menu        (GradientEditor  *gradient_editor,
						   GtkWidget      **color_boxes,
						   GtkWidget      **labels,
						   gchar           *label1,
						   gchar           *label2,
						   GtkSignalFunc    callback,
						   gchar            accel_key_0,
						   guint8           accel_mods_0,
						   gchar            accel_key_1,
						   guint8           accel_mods_1,
						   gchar            accel_key_2,
						   guint8           accel_mods_2);
static GtkWidget * cpopup_create_save_menu        (GradientEditor  *gradient_editor,
						   GtkWidget      **color_boxes,
						   GtkWidget      **labels,
						   GtkSignalFunc    callback);

static void      cpopup_update_saved_color        (GradientEditor  *gradient_editor,
						   gint             n,
						   GimpRGB         *color);

static void      cpopup_load_left_callback        (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);
static void      cpopup_save_left_callback        (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);
static void      cpopup_load_right_callback       (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);
static void      cpopup_save_right_callback       (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);

static GimpGradientSegment * cpopup_save_selection(GradientEditor  *gradient_editor);
static void             cpopup_replace_selection  (GradientEditor  *gradient_editor,
						   GimpGradientSegment  *replace_seg);

/* ----- */

static void   cpopup_set_left_color_callback      (GtkWidget          *widget,
						   GradientEditor     *gradient_editor);
static void   cpopup_set_right_color_callback     (GtkWidget          *widget,
						   GradientEditor     *gradient_editor);

static void   cpopup_left_color_changed           (ColorNotebook      *cnb,
						   const GimpRGB      *color,
						   ColorNotebookState  state,
						   gpointer            data);
static void   cpopup_right_color_changed          (ColorNotebook      *cnb,
						   const GimpRGB      *color,
						   ColorNotebookState  state,
						   gpointer            data);

/* ----- */

static GtkWidget * cpopup_create_blending_menu    (GradientEditor       *gradient_editor);
static void        cpopup_blending_callback       (GtkWidget            *widget,
						   GradientEditor       *gradient_editor);
static GtkWidget * cpopup_create_coloring_menu    (GradientEditor       *gradient_editor);
static void        cpopup_coloring_callback       (GtkWidget            *widget,
						   GradientEditor       *gradient_editor);

/* ----- */

static void  cpopup_split_midpoint_callback       (GtkWidget            *widget,
						   GradientEditor       *gradient_editor);
static void  cpopup_split_midpoint                (GimpGradient         *gradient,
						   GimpGradientSegment  *lseg,
						   GimpGradientSegment **newl,
						   GimpGradientSegment **newr);

static void  cpopup_split_uniform_callback        (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);
static void  cpopup_split_uniform_scale_update    (GtkAdjustment   *adjustment,
						   GradientEditor  *gradient_editor);
static void  cpopup_split_uniform_split_callback  (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);
static void  cpopup_split_uniform_cancel_callback (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);
static void  cpopup_split_uniform                 (GimpGradient         *gradient,
						   GimpGradientSegment  *lseg,
						   gint                  parts,
						   GimpGradientSegment **newl,
						   GimpGradientSegment **newr);

static void  cpopup_delete_callback               (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);
static void  cpopup_recenter_callback             (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);
static void  cpopup_redistribute_callback         (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);

/* Control popup -> Selection operations functions */

static GtkWidget * cpopup_create_sel_ops_menu     (GradientEditor  *gradient_editor);

static void      cpopup_flip_callback             (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);

static void      cpopup_replicate_callback        (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);
static void      cpopup_replicate_scale_update    (GtkAdjustment   *widget,
						   GradientEditor  *gradient_editor);
static void      cpopup_do_replicate_callback     (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);
static void      cpopup_replicate_cancel_callback (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);

static void      cpopup_blend_colors              (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);
static void      cpopup_blend_opacity             (GtkWidget       *widget,
						   GradientEditor  *gradient_editor);

/* Blend function */

static void      cpopup_blend_endpoints           (GradientEditor  *gradient_editor,
						   GimpRGB         *left,
						   GimpRGB         *right,
						   gint             blend_colors,
						   gint             blend_opacity);

/* Segment functions */

static void      seg_get_closest_handle           (GimpGradient         *grad,
						   gdouble               pos,
						   GimpGradientSegment **seg,
						   control_drag_mode_t  *handle);


/***** Public gradient editor functions *****/

GradientEditor *
gradient_editor_new (Gimp *gimp)
{
  GradientEditor *gradient_editor;
  GtkWidget      *main_vbox;
  GtkWidget      *hbox;
  GtkWidget      *vbox;
  GtkWidget      *vbox2;
  GtkWidget      *button;
  GtkWidget      *image;
  GtkWidget      *frame;
  gint            i;

  gradient_editor = g_new (GradientEditor, 1);

  gradient_editor->context = gimp_create_context (gimp, NULL, NULL);

  g_signal_connect (G_OBJECT (gradient_editor->context), "gradient_changed",
		    G_CALLBACK (gradient_editor_gradient_changed),
		    gradient_editor);

  /* Shell and main vbox */
  gradient_editor->shell =
    gimp_dialog_new (_("Gradient Editor"), "gradient_editor",
		     gimp_standard_help_func,
		     "dialogs/gradient_editor/gradient_editor.html",
		     GTK_WIN_POS_NONE,
		     FALSE, TRUE, FALSE,

		     "_delete_event_", ed_close_callback,
		     gradient_editor, NULL, NULL, FALSE, TRUE,

		     NULL);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (gradient_editor->shell)->vbox),
		     main_vbox);
  gtk_widget_show (main_vbox);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /* Gradient's name */
  gradient_editor->name = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (vbox), gradient_editor->name, FALSE, FALSE, 0);
  gtk_widget_show (gradient_editor->name);

  g_signal_connect (G_OBJECT (gradient_editor->name), "activate",
		    G_CALLBACK (gradient_editor_name_activate),
		    gradient_editor);
  g_signal_connect (G_OBJECT (gradient_editor->name), "focus_out_event",
		    G_CALLBACK (gradient_editor_name_focus_out),
		    gradient_editor);

  /* Frame for gradient preview and gradient control */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  /* Gradient preview */
  gradient_editor->preview_rows[0]     = NULL;
  gradient_editor->preview_rows[1]     = NULL;
  gradient_editor->preview_last_x      = 0;
  gradient_editor->preview_button_down = FALSE;

  gradient_editor->preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (gradient_editor->preview), GDK_RGB_DITHER_MAX);
  gtk_preview_size (GTK_PREVIEW (gradient_editor->preview),
		    GRAD_PREVIEW_WIDTH, GRAD_PREVIEW_HEIGHT);

  /*  Enable auto-resizing of the preview but ensure a minimal size  */
  gtk_widget_set_usize (gradient_editor->preview,
			GRAD_PREVIEW_WIDTH, GRAD_PREVIEW_HEIGHT);
  gtk_preview_set_expand (GTK_PREVIEW (gradient_editor->preview), TRUE);

  gtk_widget_set_events (gradient_editor->preview, GRAD_PREVIEW_EVENT_MASK);

  g_signal_connect (G_OBJECT (gradient_editor->preview), "event",
		    G_CALLBACK (preview_events),
		    gradient_editor);

  gimp_gtk_drag_dest_set_by_type (gradient_editor->preview,
                                  GTK_DEST_DEFAULT_ALL,
                                  GIMP_TYPE_GRADIENT,
                                  GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_set (GTK_WIDGET (gradient_editor->preview),
                              GIMP_TYPE_GRADIENT,
                              gradient_editor_drop_gradient,
                              gradient_editor);

  gtk_box_pack_start (GTK_BOX (vbox2), gradient_editor->preview, TRUE, TRUE, 0);
  gtk_widget_show (gradient_editor->preview);

  /* Gradient control */
  gradient_editor->control_pixmap                  = NULL;
  gradient_editor->control_drag_segment            = NULL;
  gradient_editor->control_sel_l                   = NULL;
  gradient_editor->control_sel_r                   = NULL;
  gradient_editor->control_drag_mode               = GRAD_DRAG_NONE;
  gradient_editor->control_click_time              = 0;
  gradient_editor->control_compress                = FALSE;
  gradient_editor->control_last_x                  = 0;
  gradient_editor->control_last_gx                 = 0.0;
  gradient_editor->control_orig_pos                = 0.0;
  gradient_editor->control_main_popup              = NULL;
  gradient_editor->control_blending_label          = NULL;
  gradient_editor->control_coloring_label          = NULL;
  gradient_editor->control_split_m_label           = NULL;
  gradient_editor->control_split_u_label           = NULL;
  gradient_editor->control_delete_menu_item        = NULL;
  gradient_editor->control_delete_label            = NULL;
  gradient_editor->control_recenter_label          = NULL;
  gradient_editor->control_redistribute_label      = NULL;
  gradient_editor->control_flip_label              = NULL;
  gradient_editor->control_replicate_label         = NULL;
  gradient_editor->control_blend_colors_menu_item  = NULL;
  gradient_editor->control_blend_opacity_menu_item = NULL;
  gradient_editor->control_left_load_popup         = NULL;
  gradient_editor->control_left_save_popup         = NULL;
  gradient_editor->control_right_load_popup        = NULL;
  gradient_editor->control_right_save_popup        = NULL;
  gradient_editor->control_blending_popup          = NULL;
  gradient_editor->control_coloring_popup          = NULL;
  gradient_editor->control_sel_ops_popup           = NULL;

  gradient_editor->accel_group = NULL;

  for (i = 0;
       i < (sizeof (gradient_editor->control_blending_items) /
	    sizeof (gradient_editor->control_blending_items[0]));
       i++)
    gradient_editor->control_blending_items[i] = NULL;

  for (i = 0;
       i < (sizeof (gradient_editor->control_coloring_items) /
	    sizeof (gradient_editor->control_coloring_items[0]));
       i++)
    gradient_editor->control_coloring_items[i] = NULL;

  gradient_editor->control = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (gradient_editor->control),
			 GRAD_PREVIEW_WIDTH, GRAD_CONTROL_HEIGHT);
  gtk_widget_set_events (gradient_editor->control, GRAD_CONTROL_EVENT_MASK);
  gtk_box_pack_start (GTK_BOX (vbox2), gradient_editor->control,
		      FALSE, FALSE, 0);
  gtk_widget_show (gradient_editor->control);

  g_signal_connect (G_OBJECT (gradient_editor->control), "event",
		    G_CALLBACK (control_events),
		    gradient_editor);

  /*  Scrollbar  */
  gradient_editor->zoom_factor = 1;

  gradient_editor->scroll_data =
    gtk_adjustment_new (0.0, 0.0, 1.0,
			1.0 * GRAD_SCROLLBAR_STEP_SIZE,
			1.0 * GRAD_SCROLLBAR_PAGE_SIZE,
			1.0);

  g_signal_connect (G_OBJECT (gradient_editor->scroll_data), "value_changed",
		    G_CALLBACK (ed_scrollbar_update),
		    gradient_editor);
  g_signal_connect (G_OBJECT (gradient_editor->scroll_data), "changed",
		    G_CALLBACK (ed_scrollbar_update),
		    gradient_editor);

  gradient_editor->scrollbar =
    gtk_hscrollbar_new (GTK_ADJUSTMENT (gradient_editor->scroll_data));
  gtk_range_set_update_policy (GTK_RANGE (gradient_editor->scrollbar),
			       GTK_UPDATE_CONTINUOUS);
  gtk_box_pack_start (GTK_BOX (vbox), gradient_editor->scrollbar,
		      FALSE, FALSE, 0);
  gtk_widget_show (gradient_editor->scrollbar);

  /*  Horizontal box for zoom controls and instant update toggle  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  + and - buttons  */
  button = gtk_button_new ();
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_ZOOM_OUT, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (ed_zoom_out_callback),
		    gradient_editor);

  button = gtk_button_new ();
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_ZOOM_IN, GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (ed_zoom_in_callback),
		    gradient_editor);

  /*  Zoom all button */
  button = gtk_button_new_with_label (_("Zoom all"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button); 

  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (ed_zoom_all_callback),
		    gradient_editor);

  /* Instant update toggle */
  gradient_editor->instant_update = TRUE;

  button = gtk_check_button_new_with_label (_("Instant update"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (G_OBJECT (button), "toggled",
		    G_CALLBACK (ed_instant_update_update),
		    gradient_editor);

  /* Hint bar */
  hbox = GTK_DIALOG (gradient_editor->shell)->action_area;
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_box_set_homogeneous (GTK_BOX (hbox), FALSE);

  gradient_editor->hint_label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (gradient_editor->hint_label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), gradient_editor->hint_label,
		      FALSE, FALSE, 0);
  gtk_widget_show (gradient_editor->hint_label);

  /* Initialize other data */
  gradient_editor->left_color_preview  = NULL;
  gradient_editor->left_saved_segments = NULL;
  gradient_editor->left_saved_dirty    = FALSE;

  gradient_editor->right_color_preview  = NULL;
  gradient_editor->right_saved_segments = NULL;
  gradient_editor->right_saved_dirty    = FALSE;

  ed_initialize_saved_colors (gradient_editor);
  cpopup_create_main_menu (gradient_editor);

  if (gimp_container_num_children (gimp->gradient_factory->container))
    {
      gimp_context_set_gradient (gradient_editor->context,
				 GIMP_GRADIENT (gimp_container_get_child_by_index (gimp->gradient_factory->container, 0)));
    }
  else
    {
      GimpGradient *gradient;

      gradient = GIMP_GRADIENT (gimp_gradient_new (_("Default")));

      gimp_container_add (gimp->gradient_factory->container,
			  GIMP_OBJECT (gradient));
    }

  gtk_widget_show (gradient_editor->shell);

  return gradient_editor;
}

void
gradient_editor_set_gradient (GradientEditor *gradient_editor,
			      GimpGradient   *gradient)
{
  g_return_if_fail (gradient_editor != NULL);
  g_return_if_fail (GIMP_IS_GRADIENT (gradient));

  gimp_context_set_gradient (gradient_editor->context, gradient);
}

void
gradient_editor_free (GradientEditor *gradient_editor)
{
  g_return_if_fail (gradient_editor != NULL);
}


/*  private functions  */

static void
gradient_editor_name_activate (GtkWidget      *widget,
			       GradientEditor *gradient_editor)
{
  GimpGradient *gradient;
  const gchar  *entry_text;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  entry_text = gtk_entry_get_text (GTK_ENTRY (widget));

  gimp_object_set_name (GIMP_OBJECT (gradient), entry_text);
}

static void
gradient_editor_name_focus_out (GtkWidget      *widget,
				GdkEvent       *event,
				GradientEditor *gradient_editor)
{
  gradient_editor_name_activate (widget, gradient_editor);
}

static void
gradient_editor_drop_gradient (GtkWidget    *widget,
			       GimpViewable *viewable,
			       gpointer      data)
{
  GradientEditor *gradient_editor;

  gradient_editor = (GradientEditor *) data;

  gradient_editor_set_gradient (gradient_editor,
				GIMP_GRADIENT (viewable));
}

static void
gradient_editor_gradient_changed (GimpContext    *context,
				  GimpGradient   *gradient,
				  GradientEditor *gradient_editor)
{
  gtk_entry_set_text (GTK_ENTRY (gradient_editor->name),
		      gimp_object_get_name (GIMP_OBJECT (gradient)));

  ed_update_editor (gradient_editor, GRAD_UPDATE_PREVIEW | GRAD_RESET_CONTROL);
}

/*****/

static void
ed_update_editor (GradientEditor *gradient_editor,
		  gint            flags)
{
  GimpGradient *gradient;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  if (flags & GRAD_UPDATE_GRADIENT)
    {
      preview_update (gradient_editor, TRUE);
    }

  if (flags & GRAD_UPDATE_PREVIEW)
    preview_update (gradient_editor, TRUE);

  if (flags & GRAD_UPDATE_CONTROL)
    control_update (gradient_editor, gradient, FALSE);

  if (flags & GRAD_RESET_CONTROL)
    control_update (gradient_editor, gradient, TRUE);
}

static void
ed_set_hint (GradientEditor *gradient_editor,
	     gchar          *str)
{
  gtk_label_set_text (GTK_LABEL (gradient_editor->hint_label), str);
}

static void
ed_initialize_saved_colors (GradientEditor *gradient_editor)
{
  gint i;

  for (i = 0; i < (GRAD_NUM_COLORS + 3); i++)
    {
      gradient_editor->left_load_color_boxes[i] = NULL;
      gradient_editor->left_load_labels[i]      = NULL;

      gradient_editor->right_load_color_boxes[i] = NULL;
      gradient_editor->right_load_labels[i]      = NULL;
    }

  for (i = 0; i < GRAD_NUM_COLORS; i++)
    {
      gradient_editor->left_save_color_boxes[i] = NULL;
      gradient_editor->left_save_labels[i]      = NULL;

      gradient_editor->right_save_color_boxes[i] = NULL;
      gradient_editor->right_save_labels[i]      = NULL;
    }

  gradient_editor->saved_colors[0].r = 0.0; /* Black */
  gradient_editor->saved_colors[0].g = 0.0;
  gradient_editor->saved_colors[0].b = 0.0;
  gradient_editor->saved_colors[0].a = 1.0;

  gradient_editor->saved_colors[1].r = 0.5; /* 50% Gray */
  gradient_editor->saved_colors[1].g = 0.5;
  gradient_editor->saved_colors[1].b = 0.5;
  gradient_editor->saved_colors[1].a = 1.0;

  gradient_editor->saved_colors[2].r = 1.0; /* White */
  gradient_editor->saved_colors[2].g = 1.0;
  gradient_editor->saved_colors[2].b = 1.0;
  gradient_editor->saved_colors[2].a = 1.0;

  gradient_editor->saved_colors[3].r = 0.0; /* Clear */
  gradient_editor->saved_colors[3].g = 0.0;
  gradient_editor->saved_colors[3].b = 0.0;
  gradient_editor->saved_colors[3].a = 0.0;

  gradient_editor->saved_colors[4].r = 1.0; /* Red */
  gradient_editor->saved_colors[4].g = 0.0;
  gradient_editor->saved_colors[4].b = 0.0;
  gradient_editor->saved_colors[4].a = 1.0;

  gradient_editor->saved_colors[5].r = 1.0; /* Yellow */
  gradient_editor->saved_colors[5].g = 1.0;
  gradient_editor->saved_colors[5].b = 0.0;
  gradient_editor->saved_colors[5].a = 1.0;

  gradient_editor->saved_colors[6].r = 0.0; /* Green */
  gradient_editor->saved_colors[6].g = 1.0;
  gradient_editor->saved_colors[6].b = 0.0;
  gradient_editor->saved_colors[6].a = 1.0;

  gradient_editor->saved_colors[7].r = 0.0; /* Cyan */
  gradient_editor->saved_colors[7].g = 1.0;
  gradient_editor->saved_colors[7].b = 1.0;
  gradient_editor->saved_colors[7].a = 1.0;

  gradient_editor->saved_colors[8].r = 0.0; /* Blue */
  gradient_editor->saved_colors[8].g = 0.0;
  gradient_editor->saved_colors[8].b = 1.0;
  gradient_editor->saved_colors[8].a = 1.0;

  gradient_editor->saved_colors[9].r = 1.0; /* Magenta */
  gradient_editor->saved_colors[9].g = 0.0;
  gradient_editor->saved_colors[9].b = 1.0;
  gradient_editor->saved_colors[9].a = 1.0;
}


/***** Main gradient editor dialog callbacks *****/

static void
ed_close_callback (GtkWidget      *widget,
		   GradientEditor *gradient_editor)
{
  if (GTK_WIDGET_VISIBLE (gradient_editor->shell))
    gtk_widget_hide (gradient_editor->shell);
}


/***** Zoom, scrollbar & instant update callbacks *****/

static void
ed_scrollbar_update (GtkAdjustment  *adjustment,
		     GradientEditor *gradient_editor)
{
  gchar *str;

  str = g_strdup_printf (_("Zoom factor: %d:1    Displaying [%0.6f, %0.6f]"),
			 gradient_editor->zoom_factor, adjustment->value,
			 adjustment->value + adjustment->page_size);

  ed_set_hint (gradient_editor, str);
  g_free (str);

  ed_update_editor (gradient_editor, GRAD_UPDATE_PREVIEW | GRAD_UPDATE_CONTROL);
}

static void
ed_zoom_all_callback (GtkWidget      *widget,
		      GradientEditor *gradient_editor)
{
  GtkAdjustment *adjustment;

  adjustment = GTK_ADJUSTMENT (gradient_editor->scroll_data);

  gradient_editor->zoom_factor = 1;

  adjustment->value     	   = 0.0;
  adjustment->page_size 	   = 1.0;
  adjustment->step_increment = 1.0 * GRAD_SCROLLBAR_STEP_SIZE;
  adjustment->page_increment = 1.0 * GRAD_SCROLLBAR_PAGE_SIZE;

  gtk_adjustment_changed (GTK_ADJUSTMENT (gradient_editor->scroll_data));
}

static void
ed_zoom_out_callback (GtkWidget      *widget,
		      GradientEditor *gradient_editor)
{
  GtkAdjustment *adjustment;
  gdouble        old_value;
  gdouble        value;
  gdouble        old_page_size;
  gdouble        page_size;

  if (gradient_editor->zoom_factor <= 1)
    return;

  adjustment = GTK_ADJUSTMENT (gradient_editor->scroll_data);

  old_value     = adjustment->value;
  old_page_size = adjustment->page_size;

  gradient_editor->zoom_factor--;

  page_size = 1.0 / gradient_editor->zoom_factor;
  value     = old_value - (page_size - old_page_size) / 2.0;

  if (value < 0.0)
    value = 0.0;
  else if ((value + page_size) > 1.0)
    value = 1.0 - page_size;

  adjustment->value          = value;
  adjustment->page_size      = page_size;
  adjustment->step_increment = page_size * GRAD_SCROLLBAR_STEP_SIZE;
  adjustment->page_increment = page_size * GRAD_SCROLLBAR_PAGE_SIZE;

  gtk_adjustment_changed (GTK_ADJUSTMENT (gradient_editor->scroll_data));
}

static void
ed_zoom_in_callback (GtkWidget      *widget,
		     GradientEditor *gradient_editor)
{
  GtkAdjustment *adjustment;
  gdouble        old_value;
  gdouble        old_page_size;
  gdouble        page_size;

  adjustment = GTK_ADJUSTMENT (gradient_editor->scroll_data);

  old_value     = adjustment->value;
  old_page_size = adjustment->page_size;

  gradient_editor->zoom_factor++;

  page_size = 1.0 / gradient_editor->zoom_factor;

  adjustment->value    	     = old_value + (old_page_size - page_size) / 2.0;
  adjustment->page_size      = page_size;
  adjustment->step_increment = page_size * GRAD_SCROLLBAR_STEP_SIZE;
  adjustment->page_increment = page_size * GRAD_SCROLLBAR_PAGE_SIZE;

  gtk_adjustment_changed (GTK_ADJUSTMENT (gradient_editor->scroll_data));
}

static void
ed_instant_update_update (GtkWidget      *widget,
			  GradientEditor *gradient_editor)
{
  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      gradient_editor->instant_update = TRUE;
      gtk_range_set_update_policy (GTK_RANGE (gradient_editor->scrollbar),
				   GTK_UPDATE_CONTINUOUS);
    }
  else
    {
      gradient_editor->instant_update = FALSE;
      gtk_range_set_update_policy (GTK_RANGE (gradient_editor->scrollbar),
				   GTK_UPDATE_DELAYED);
    }
}

/***** Gradient preview functions *****/

static gint
preview_events (GtkWidget      *widget,
		GdkEvent       *event,
		GradientEditor *gradient_editor)
{
  gint            x, y;
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  GdkEventScroll *sevent;

  switch (event->type)
    {
    case GDK_EXPOSE:
      preview_update (gradient_editor, FALSE);

      return FALSE;

    case GDK_LEAVE_NOTIFY:
      ed_set_hint (gradient_editor, "");

      break;

    case GDK_MOTION_NOTIFY:
      gtk_widget_get_pointer (gradient_editor->preview, &x, &y);

      mevent = (GdkEventMotion *) event;

      if (x != gradient_editor->preview_last_x)
	{
	  gradient_editor->preview_last_x = x;

	  if (gradient_editor->preview_button_down)
	    {
	      if (mevent->state & GDK_CONTROL_MASK)
		preview_set_background (gradient_editor, x);
	      else
		preview_set_foreground (gradient_editor, x);
	    }
	  else
	    {
	      preview_set_hint (gradient_editor, x);
	    }
	}

      break;

    case GDK_BUTTON_PRESS:
      gtk_widget_get_pointer (gradient_editor->preview, &x, &y);

      bevent = (GdkEventButton *) event;

      switch (bevent->button)
	{
	case 1:
	  gradient_editor->preview_last_x = x;
	  gradient_editor->preview_button_down = TRUE;
	  if (bevent->state & GDK_CONTROL_MASK)
	    preview_set_background (gradient_editor, x);
	  else
	    preview_set_foreground (gradient_editor, x);
	  break;

	case 3:
	  cpopup_do_popup (gradient_editor);
	  break;

	default:
	  break;
	}

      break;

    case GDK_SCROLL:
      sevent = (GdkEventScroll *) event;

      if (sevent->state & GDK_SHIFT_MASK)
	{
	  if (sevent->direction == GDK_SCROLL_UP)
	    ed_zoom_in_callback (NULL, gradient_editor);
	  else
	    ed_zoom_out_callback (NULL, gradient_editor);
	}
      else
	{
	  GtkAdjustment *adj = GTK_ADJUSTMENT (gradient_editor->scroll_data);

	  gfloat new_value = adj->value + ((sevent->direction == GDK_SCROLL_UP) ?
					   -adj->page_increment / 2 :
					   adj->page_increment / 2);

	  new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);

	  gtk_adjustment_set_value (adj, new_value);
	}

      break;

    case GDK_BUTTON_RELEASE:
      if (gradient_editor->preview_button_down)
	{
	  gtk_widget_get_pointer (gradient_editor->preview, &x, &y);

	  bevent = (GdkEventButton *) event;

	  gradient_editor->preview_last_x = x;
	  gradient_editor->preview_button_down = FALSE;
	  if (bevent->state & GDK_CONTROL_MASK)
	    preview_set_background (gradient_editor, x);
	  else
	    preview_set_foreground (gradient_editor, x);
	  break;
	}

      break;

    default:
      break;
    }

  return TRUE;
}

static void
preview_set_hint (GradientEditor *gradient_editor,
		  gint            x)
{
  gdouble  xpos;
  GimpRGB  rgb;
  GimpHSV  hsv;
  gchar   *str;

  xpos = control_calc_g_pos (gradient_editor, x);

  gimp_gradient_get_color_at
    (gimp_context_get_gradient (gradient_editor->context),
     xpos, &rgb);

  gimp_rgb_to_hsv (&rgb, &hsv);

  str = g_strdup_printf (_("Position: %0.6f    "
			   "RGB (%0.3f, %0.3f, %0.3f)    "
			   "HSV (%0.3f, %0.3f, %0.3f)    "
			   "Opacity: %0.3f"),
			 xpos,
			 rgb.r,
			 rgb.g,
			 rgb.b,
			 hsv.h * 360.0,
			 hsv.s,
			 hsv.v,
			 rgb.a);

  ed_set_hint (gradient_editor, str);
  g_free (str);
}

static void
preview_set_foreground (GradientEditor *gradient_editor,
			gint            x)
{
  GimpRGB  color;
  gdouble  xpos;
  gchar   *str;

  xpos = control_calc_g_pos (gradient_editor, x);
  gimp_gradient_get_color_at
    (gimp_context_get_gradient (gradient_editor->context),
     xpos, &color);

  gimp_context_set_foreground (gimp_get_user_context (gradient_editor->context->gimp), &color);

  str = g_strdup_printf (_("Foreground color set to RGB (%d, %d, %d) <-> "
			   "(%0.3f, %0.3f, %0.3f)"),
			 (gint) (color.r * 255.0),
			 (gint) (color.g * 255.0),
			 (gint) (color.b * 255.0),
			 color.r, color.g, color.b);

  ed_set_hint (gradient_editor, str);
  g_free (str);
}

static void
preview_set_background (GradientEditor *gradient_editor,
			gint            x)
{
  GimpRGB  color;
  gdouble  xpos;
  gchar   *str;

  xpos = control_calc_g_pos (gradient_editor, x);
  gimp_gradient_get_color_at
    (gimp_context_get_gradient (gradient_editor->context),
     xpos, &color);

  gimp_context_set_background (gimp_get_user_context (gradient_editor->context->gimp), &color);

  str = g_strdup_printf (_("Background color to RGB (%d, %d, %d) <-> "
			   "(%0.3f, %0.3f, %0.3f)"),
			 (gint) (color.r * 255.0),
			 (gint) (color.g * 255.0),
			 (gint) (color.b * 255.0),
			 color.r, color.g, color.b);

  ed_set_hint (gradient_editor, str);
  g_free (str);
}

/*****/

static void
preview_update (GradientEditor *gradient_editor,
		gboolean        recalculate)
{
  glong          rowsiz;
  GtkAdjustment *adjustment;
  guint16        width;
  guint16        height;

  static guint16  last_width  = 0;
  static guint16  last_height = 0;

  if (! GTK_WIDGET_DRAWABLE (gradient_editor->preview))
    return;

  /*  See whether we have to re-create the preview widget
   *  (note that the preview automatically follows the size of it's container)
   */
  width  = gradient_editor->preview->allocation.width;
  height = gradient_editor->preview->allocation.height;

  if (! gradient_editor->preview_rows[0] ||
      ! gradient_editor->preview_rows[1] ||
      (width  != last_width)      ||
      (height != last_height))
    {
      if (gradient_editor->preview_rows[0])
	g_free (gradient_editor->preview_rows[0]);

      if (gradient_editor->preview_rows[1])
	g_free (gradient_editor->preview_rows[1]);

      rowsiz = width * 3 * sizeof (guchar);

      gradient_editor->preview_rows[0] = g_malloc (rowsiz);
      gradient_editor->preview_rows[1] = g_malloc (rowsiz);

      recalculate = TRUE; /* Force recalculation */
    }

  last_width = width;
  last_height = height;

  /* Have to redraw? */
  if (recalculate)
    {
      adjustment = GTK_ADJUSTMENT (gradient_editor->scroll_data);

      preview_fill_image (gradient_editor,
			  width, height,
			  adjustment->value,
			  adjustment->value + adjustment->page_size);

      gtk_widget_draw (gradient_editor->preview, NULL);
    }
}

static void
preview_fill_image (GradientEditor *gradient_editor,
		    gint            width,
		    gint            height,
		    gdouble         left,
		    gdouble         right)
{
  GimpGradient *gradient;
  guchar       *p0, *p1;
  gint          x, y;
  gdouble       dx, cur_x;
  GimpRGB       color;
  gdouble       c0, c1;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  dx    = (right - left) / (width - 1);
  cur_x = left;
  p0    = gradient_editor->preview_rows[0];
  p1    = gradient_editor->preview_rows[1];

  /* Create lines to fill the image */
  for (x = 0; x < width; x++)
    {
      gimp_gradient_get_color_at (gradient, cur_x, &color);

      if ((x / GIMP_CHECK_SIZE) & 1)
	{
	  c0 = GIMP_CHECK_LIGHT;
	  c1 = GIMP_CHECK_DARK;
	}
      else
	{
	  c0 = GIMP_CHECK_DARK;
	  c1 = GIMP_CHECK_LIGHT;
	}

      *p0++ = (c0 + (color.r - c0) * color.a) * 255.0;
      *p0++ = (c0 + (color.g - c0) * color.a) * 255.0;
      *p0++ = (c0 + (color.b - c0) * color.a) * 255.0;

      *p1++ = (c1 + (color.r - c1) * color.a) * 255.0;
      *p1++ = (c1 + (color.g - c1) * color.a) * 255.0;
      *p1++ = (c1 + (color.b - c1) * color.a) * 255.0;

      cur_x += dx;
    }

  /* Fill image */
  for (y = 0; y < height; y++)
    {
      if ((y / GIMP_CHECK_SIZE) & 1)
	gtk_preview_draw_row (GTK_PREVIEW (gradient_editor->preview),
			      gradient_editor->preview_rows[1], 0, y, width);
      else
	gtk_preview_draw_row (GTK_PREVIEW (gradient_editor->preview),
			      gradient_editor->preview_rows[0], 0, y, width);
    }
}

/***** Gradient control functions *****/

/* *** WARNING *** WARNING *** WARNING ***
 *
 * All the event-handling code for the gradient control widget is
 * extremely hairy.  You are not expected to understand it.  If you
 * find bugs, mail me unless you are very brave and you want to fix
 * them yourself ;-)
 */

static gint
control_events (GtkWidget      *widget,
		GdkEvent       *event,
		GradientEditor *gradient_editor)
{
  GimpGradient        *gradient;
  GdkEventButton      *bevent;
  GimpGradientSegment *seg;
  gint                 x, y;
  guint32              time;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  switch (event->type)
    {
    case GDK_EXPOSE:
      control_update (gradient_editor, gradient, FALSE);
      break;

    case GDK_LEAVE_NOTIFY:
      ed_set_hint (gradient_editor, "");
      break;

    case GDK_BUTTON_PRESS:
      if (gradient_editor->control_drag_mode == GRAD_DRAG_NONE)
	{
	  gtk_widget_get_pointer (gradient_editor->control, &x, &y);

	  bevent = (GdkEventButton *) event;

	  gradient_editor->control_last_x     = x;
	  gradient_editor->control_click_time = bevent->time;

	  control_button_press (gradient_editor,
				x, y, bevent->button, bevent->state);

	  if (gradient_editor->control_drag_mode != GRAD_DRAG_NONE)
	    gtk_grab_add (widget);
	}
      break;

    case GDK_BUTTON_RELEASE:
      ed_set_hint (gradient_editor, "");

      if (gradient_editor->control_drag_mode != GRAD_DRAG_NONE)
	{
	  gtk_grab_remove (widget);

	  gtk_widget_get_pointer (gradient_editor->control, &x, &y);

	  time = ((GdkEventButton *) event)->time;

	  if ((time - gradient_editor->control_click_time) >= GRAD_MOVE_TIME)
	    {
	      if (! gradient_editor->instant_update)
		gimp_data_dirty (GIMP_DATA (gradient));

	      ed_update_editor (gradient_editor,
				GRAD_UPDATE_GRADIENT); /* Possible move */
	    }
	  else if ((gradient_editor->control_drag_mode == GRAD_DRAG_MIDDLE) ||
		   (gradient_editor->control_drag_mode == GRAD_DRAG_ALL))
	    {
	      seg = gradient_editor->control_drag_segment;

	      if ((gradient_editor->control_drag_mode == GRAD_DRAG_ALL) &&
		  gradient_editor->control_compress)
		control_extend_selection (gradient_editor, seg,
					  control_calc_g_pos (gradient_editor,
							      x));
	      else
		control_select_single_segment (gradient_editor, seg);

	      ed_update_editor (gradient_editor, GRAD_UPDATE_CONTROL);
	    }

	  gradient_editor->control_drag_mode = GRAD_DRAG_NONE;
	  gradient_editor->control_compress  = FALSE;

	  control_do_hint (gradient_editor, x, y);
	}
      break;

    case GDK_MOTION_NOTIFY:
      gtk_widget_get_pointer (gradient_editor->control, &x, &y);

      if (x != gradient_editor->control_last_x)
	{
	  gradient_editor->control_last_x = x;

	  if (gradient_editor->control_drag_mode != GRAD_DRAG_NONE)
	    {
	      time = ((GdkEventButton *) event)->time;

	      if ((time - gradient_editor->control_click_time) >= GRAD_MOVE_TIME)
		control_motion (gradient_editor, gradient, x);
	    }
	  else
	    {
	      ed_update_editor (gradient_editor, GRAD_UPDATE_CONTROL);

	      control_do_hint (gradient_editor, x, y);
	    }
	}
      break;

    default:
      break;
    }

  return FALSE;
}

static void
control_do_hint (GradientEditor *gradient_editor,
		 gint            x,
		 gint            y)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg;
  control_drag_mode_t  handle;
  gboolean             in_handle;
  double               pos;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  pos = control_calc_g_pos (gradient_editor, x);

  if ((pos < 0.0) || (pos > 1.0))
    return;

  seg_get_closest_handle (gradient, pos, &seg, &handle);

  in_handle = control_point_in_handle (gradient_editor, gradient,
				       x, y, seg, handle);

  if (in_handle)
    {
      switch (handle)
	{
	case GRAD_DRAG_LEFT:
	  if (seg != NULL)
	    {
	      if (seg->prev != NULL)
		ed_set_hint (gradient_editor,
			     _("Drag: move    Shift+drag: move & compress"));
	      else
		ed_set_hint (gradient_editor,
			     _("Click: select    Shift+click: extend selection"));
	    }
	  else
	    {
	      ed_set_hint (gradient_editor,
			   _("Click: select    Shift+click: extend selection"));
	    }

	  break;

	case GRAD_DRAG_MIDDLE:
	  ed_set_hint (gradient_editor,
		       _("Click: select    Shift+click: extend selection    "
			 "Drag: move"));

	  break;

	default:
	  g_warning ("in_handle is true yet we got handle type %d",
		     (int) handle);
	  break;
	}
    }
  else
    ed_set_hint (gradient_editor,
		 _("Click: select    Shift+click: extend selection    "
		   "Drag: move    Shift+drag: move & compress"));
}

static void
control_button_press (GradientEditor *gradient_editor,
		      gint            x,
		      gint            y,
		      guint           button,
		      guint           state)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg;
  control_drag_mode_t  handle;
  double               xpos;
  gboolean             in_handle;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  switch (button)
    {
    case 1:
      break;

    case 3:
      cpopup_do_popup (gradient_editor);
      return;

      /*  wheelmouse support  */
    case 4:
      {
	GtkAdjustment *adj = GTK_ADJUSTMENT (gradient_editor->scroll_data);
	gfloat new_value   = adj->value - adj->page_increment / 2;

	new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	gtk_adjustment_set_value (adj, new_value);
      }
      return;

    case 5:
      {
	GtkAdjustment *adj = GTK_ADJUSTMENT (gradient_editor->scroll_data);
	gfloat new_value   = adj->value + adj->page_increment / 2;

	new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	gtk_adjustment_set_value (adj, new_value);
      }
      return;

    default:
      return;
    }

  /* Find the closest handle */

  xpos = control_calc_g_pos (gradient_editor, x);

  seg_get_closest_handle (gradient, xpos, &seg, &handle);

  in_handle = control_point_in_handle (gradient_editor, gradient,
				       x, y, seg, handle);

  /* Now see what we have */

  if (in_handle)
    {
      switch (handle)
	{
	case GRAD_DRAG_LEFT:
	  if (seg != NULL)
	    {
	      /* Left handle of some segment */
	      if (state & GDK_SHIFT_MASK)
		{
		  if (seg->prev != NULL)
		    {
		      gradient_editor->control_drag_mode    = GRAD_DRAG_LEFT;
		      gradient_editor->control_drag_segment = seg;
		      gradient_editor->control_compress     = TRUE;
		    }
		  else
		    {
		      control_extend_selection (gradient_editor, seg, xpos);
		      ed_update_editor (gradient_editor, GRAD_UPDATE_CONTROL);
		    }
		}
	      else if (seg->prev != NULL)
		{
		  gradient_editor->control_drag_mode    = GRAD_DRAG_LEFT;
		  gradient_editor->control_drag_segment = seg;
		}
	      else
		{
		  control_select_single_segment (gradient_editor, seg);
		  ed_update_editor (gradient_editor, GRAD_UPDATE_CONTROL);
		}

	      return;
	    }
	  else  /* seg == NULL */
	    {
	      /* Right handle of last segment */
	      seg = gimp_gradient_segment_get_last (gradient->segments);

	      if (state & GDK_SHIFT_MASK)
		{
		  control_extend_selection (gradient_editor, seg, xpos);
		  ed_update_editor (gradient_editor, GRAD_UPDATE_CONTROL);
		}
	      else
		{
		  control_select_single_segment (gradient_editor, seg);
		  ed_update_editor (gradient_editor, GRAD_UPDATE_CONTROL);
		}

	      return;
	    }

	  break;

	case GRAD_DRAG_MIDDLE:
	  if (state & GDK_SHIFT_MASK)
	    {
	      control_extend_selection (gradient_editor, seg, xpos);
	      ed_update_editor (gradient_editor, GRAD_UPDATE_CONTROL);
	    }
	  else
	    {
	      gradient_editor->control_drag_mode    = GRAD_DRAG_MIDDLE;
	      gradient_editor->control_drag_segment = seg;
	    }

	  return;

	default:
	  g_warning ("in_handle is true yet we got handle type %d",
		     (int) handle);
	  return;
	}
    }
  else  /* !in_handle */
    {
      seg = gimp_gradient_get_segment_at (gradient, xpos);

      gradient_editor->control_drag_mode    = GRAD_DRAG_ALL;
      gradient_editor->control_drag_segment = seg;
      gradient_editor->control_last_gx      = xpos;
      gradient_editor->control_orig_pos     = xpos;

      if (state & GDK_SHIFT_MASK)
	gradient_editor->control_compress = TRUE;

      return;
    }
}

static gboolean
control_point_in_handle (GradientEditor      *gradient_editor,
			 GimpGradient        *gradient,
			 gint                 x,
			 gint                 y,
			 GimpGradientSegment *seg,
			 control_drag_mode_t  handle)
{
  gint handle_pos;

  switch (handle)
    {
    case GRAD_DRAG_LEFT:
      if (seg)
	{
	  handle_pos = control_calc_p_pos (gradient_editor, seg->left);
	}
      else
	{
	  seg = gimp_gradient_segment_get_last (gradient->segments);

	  handle_pos = control_calc_p_pos (gradient_editor, seg->right);
	}

      break;

    case GRAD_DRAG_MIDDLE:
      handle_pos = control_calc_p_pos (gradient_editor, seg->middle);
      break;

    default:
      g_warning ("can not handle drag mode %d", (gint) handle);
      return FALSE;
    }

  y /= 2;

  if ((x >= (handle_pos - y)) && (x <= (handle_pos + y)))
    return TRUE;
  else
    return FALSE;
}

/*****/

static void
control_select_single_segment (GradientEditor      *gradient_editor,
			       GimpGradientSegment *seg)
{
  gradient_editor->control_sel_l = seg;
  gradient_editor->control_sel_r = seg;
}

static void
control_extend_selection (GradientEditor      *gradient_editor,
			  GimpGradientSegment *seg,
			  gdouble              pos)
{
  if (fabs (pos - gradient_editor->control_sel_l->left) <
      fabs (pos - gradient_editor->control_sel_r->right))
    gradient_editor->control_sel_l = seg;
  else
    gradient_editor->control_sel_r = seg;
}

/*****/

static void
control_motion (GradientEditor *gradient_editor,
		GimpGradient   *gradient,
		gint            x)
{
  GimpGradientSegment *seg;
  gdouble         pos;
  gdouble         delta;
  gchar          *str = NULL;

  seg = gradient_editor->control_drag_segment;

  switch (gradient_editor->control_drag_mode)
    {
    case GRAD_DRAG_LEFT:
      pos = control_calc_g_pos (gradient_editor, x);

      if (! gradient_editor->control_compress)
	seg->prev->right = seg->left = CLAMP (pos,
					      seg->prev->middle + EPSILON,
					      seg->middle - EPSILON);
      else
	control_compress_left (gradient_editor->control_sel_l,
			       gradient_editor->control_sel_r,
			       seg, pos);

      str = g_strdup_printf (_("Handle position: %0.6f"), seg->left);
      ed_set_hint (gradient_editor, str);

      break;

    case GRAD_DRAG_MIDDLE:
      pos = control_calc_g_pos (gradient_editor, x);
      seg->middle = CLAMP (pos, seg->left + EPSILON, seg->right - EPSILON);

      str = g_strdup_printf (_("Handle position: %0.6f"), seg->middle);
      ed_set_hint (gradient_editor, str);

      break;

    case GRAD_DRAG_ALL:
      pos    = control_calc_g_pos (gradient_editor, x);
      delta  = pos - gradient_editor->control_last_gx;

      if ((seg->left >= gradient_editor->control_sel_l->left) &&
	  (seg->right <= gradient_editor->control_sel_r->right))
	delta = control_move (gradient_editor,
			      gradient_editor->control_sel_l,
			      gradient_editor->control_sel_r, delta);
      else
	delta = control_move (gradient_editor, seg, seg, delta);

      gradient_editor->control_last_gx += delta;

      str = g_strdup_printf (_("Distance: %0.6f"),
			     gradient_editor->control_last_gx -
			     gradient_editor->control_orig_pos);
      ed_set_hint (gradient_editor, str);

      break;

    default:
      gimp_fatal_error ("Attempt to move bogus handle %d",
			(gint) gradient_editor->control_drag_mode);
      break;
    }

  if (str)
    g_free (str);

  if (gradient_editor->instant_update)
    {
      gimp_data_dirty (GIMP_DATA (gradient));

      ed_update_editor (gradient_editor,
			GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
    }
  else
    {
      ed_update_editor (gradient_editor, GRAD_UPDATE_CONTROL);
    }
}

static void
control_compress_left (GimpGradientSegment *range_l,
		       GimpGradientSegment *range_r,
		       GimpGradientSegment *drag_seg,
		       gdouble              pos)
{
  GimpGradientSegment *seg;
  gdouble              lbound, rbound;
  gint                 k;

  /* Check what we have to compress */

  if (!((drag_seg->left >= range_l->left) &&
	((drag_seg->right <= range_r->right) || (drag_seg == range_r->next))))
    {
      /* We are compressing a segment outside the selection */

      range_l = range_r = drag_seg;
    }

  /* Calculate left bound for dragged hadle */

  if (drag_seg == range_l)
    lbound = range_l->prev->left + 2.0 * EPSILON;
  else
    {
      /* Count number of segments to the left of the dragged handle */

      seg = drag_seg;
      k   = 0;

      while (seg != range_l)
	{
	  k++;
	  seg = seg->prev;
	}

      /* 2*k handles have to fit */

      lbound = range_l->left + 2.0 * k * EPSILON;
    }

  /* Calculate right bound for dragged handle */

  if (drag_seg == range_r->next)
    rbound = range_r->next->right - 2.0 * EPSILON;
  else
    {
      /* Count number of segments to the right of the dragged handle */

      seg = drag_seg;
      k   = 1;

      while (seg != range_r)
	{
	  k++;
	  seg = seg->next;
	}

      /* 2*k handles have to fit */

      rbound = range_r->right - 2.0 * k * EPSILON;
    }

  /* Calculate position */

  pos = CLAMP (pos, lbound, rbound);

  /* Compress segments to the left of the handle */

  if (drag_seg == range_l)
    control_compress_range (range_l->prev, range_l->prev,
			    range_l->prev->left, pos);
  else
    control_compress_range (range_l, drag_seg->prev, range_l->left, pos);

  /* Compress segments to the right of the handle */

  if (drag_seg != range_r->next)
    control_compress_range (drag_seg, range_r, pos, range_r->right);
  else
    control_compress_range (drag_seg, drag_seg, pos, drag_seg->right);
}

static void
control_compress_range (GimpGradientSegment *range_l,
			GimpGradientSegment *range_r,
			gdouble         new_l,
			gdouble         new_r)
{
  gdouble         orig_l, orig_r;
  gdouble         scale;
  GimpGradientSegment *seg, *aseg;

  orig_l = range_l->left;
  orig_r = range_r->right;

  scale = (new_r - new_l) / (orig_r - orig_l);

  seg = range_l;

  do
    {
      seg->left   = new_l + (seg->left - orig_l) * scale;
      seg->middle = new_l + (seg->middle - orig_l) * scale;
      seg->right  = new_l + (seg->right - orig_l) * scale;

      /* Next */

      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != range_r);
}

/*****/

static gdouble
control_move (GradientEditor      *gradient_editor,
	      GimpGradientSegment *range_l,
	      GimpGradientSegment *range_r,
	      gdouble             delta)
{
  gdouble              lbound, rbound;
  gint                 is_first, is_last;
  GimpGradientSegment *seg, *aseg;

  /* First or last segments in gradient? */

  is_first = (range_l->prev == NULL);
  is_last  = (range_r->next == NULL);

  /* Calculate drag bounds */

  if (! gradient_editor->control_compress)
    {
      if (!is_first)
	lbound = range_l->prev->middle + EPSILON;
      else
	lbound = range_l->left + EPSILON;

      if (!is_last)
	rbound = range_r->next->middle - EPSILON;
      else
	rbound = range_r->right - EPSILON;
    }
  else
    {
      if (!is_first)
	lbound = range_l->prev->left + 2.0 * EPSILON;
      else
	lbound = range_l->left + EPSILON;

      if (!is_last)
	rbound = range_r->next->right - 2.0 * EPSILON;
      else
	rbound = range_r->right - EPSILON;
    }

  /* Fix the delta if necessary */

  if (delta < 0.0)
    {
      if (!is_first)
	{
	  if (range_l->left + delta < lbound)
	    delta = lbound - range_l->left;
	}
      else
	if (range_l->middle + delta < lbound)
	  delta = lbound - range_l->middle;
    }
  else
    {
      if (!is_last)
	{
	  if (range_r->right + delta > rbound)
	    delta = rbound - range_r->right;
	}
      else
	if (range_r->middle + delta > rbound)
	  delta = rbound - range_r->middle;
    }

  /* Move all the segments inside the range */

  seg = range_l;

  do
    {
      if (!((seg == range_l) && is_first))
	seg->left   += delta;

      seg->middle += delta;

      if (!((seg == range_r) && is_last))
	seg->right  += delta;

      /* Next */

      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != range_r);

  /* Fix the segments that surround the range */

  if (!is_first)
    {
      if (! gradient_editor->control_compress)
	range_l->prev->right = range_l->left;
      else
	control_compress_range (range_l->prev, range_l->prev,
				range_l->prev->left, range_l->left);
    }

  if (!is_last)
    {
      if (! gradient_editor->control_compress)
	range_r->next->left = range_r->right;
      else
	control_compress_range (range_r->next, range_r->next,
				range_r->right, range_r->next->right);
    }

  return delta;
}

/*****/

static void
control_update (GradientEditor *gradient_editor,
		GimpGradient   *gradient,
		gboolean        recalculate)
{
  GtkAdjustment *adjustment;
  gint           cwidth, cheight;
  gint           pwidth, pheight;

  if (! GTK_WIDGET_DRAWABLE (gradient_editor->control))
    return;

  /*  See whether we have to re-create the control pixmap
   *  depending on the preview's width
   */
  cwidth  = gradient_editor->preview->allocation.width;
  cheight = GRAD_CONTROL_HEIGHT;

  if (gradient_editor->control_pixmap)
    gdk_drawable_get_size (gradient_editor->control_pixmap, &pwidth, &pheight);

  if (! gradient_editor->control_pixmap ||
      (cwidth != pwidth) ||
      (cheight != pheight))
    {
      if (gradient_editor->control_pixmap)
	gdk_drawable_unref (gradient_editor->control_pixmap);

      gradient_editor->control_pixmap =
	gdk_pixmap_new (gradient_editor->control->window, cwidth, cheight, -1);

      recalculate = TRUE;
    }

  /* Avaoid segfault on first invocation */
  if (cwidth < GRAD_PREVIEW_WIDTH)
    return;

  /* Have to reset the selection? */
  if (recalculate)
    control_select_single_segment (gradient_editor, gradient->segments);

  /* Redraw pixmap */
  adjustment = GTK_ADJUSTMENT (gradient_editor->scroll_data);

  control_draw (gradient_editor,
		gradient,
		gradient_editor->control_pixmap,
		cwidth, cheight,
		adjustment->value,
		adjustment->value + adjustment->page_size);

  gdk_draw_drawable (gradient_editor->control->window,
		     gradient_editor->control->style->black_gc,
		     gradient_editor->control_pixmap,
		     0, 0, 0, 0,
		     cwidth, cheight);
}

static void
control_draw (GradientEditor *gradient_editor,
	      GimpGradient   *gradient,
	      GdkPixmap      *pixmap,
	      gint            width,
	      gint            height,
	      gdouble         left,
	      gdouble         right)
{
  gint 	               sel_l, sel_r;
  gdouble              g_pos;
  GimpGradientSegment *seg;
  control_drag_mode_t  handle;

  /* Clear the pixmap */

  gdk_draw_rectangle (pixmap, gradient_editor->control->style->bg_gc[GTK_STATE_NORMAL],
		      TRUE, 0, 0, width, height);

  /* Draw selection */

  sel_l = control_calc_p_pos (gradient_editor,
			      gradient_editor->control_sel_l->left);
  sel_r = control_calc_p_pos (gradient_editor,
			      gradient_editor->control_sel_r->right);

  gdk_draw_rectangle (pixmap,
		      gradient_editor->control->style->dark_gc[GTK_STATE_NORMAL],
		      TRUE, sel_l, 0, sel_r - sel_l + 1, height);

  /* Draw handles */

  seg = gradient->segments;

  while (seg)
    {
      control_draw_normal_handle (gradient_editor, pixmap, seg->left, height);
      control_draw_middle_handle (gradient_editor, pixmap, seg->middle, height);

      /* Draw right handle only if this is the last segment */

      if (seg->next == NULL)
	control_draw_normal_handle (gradient_editor, pixmap, seg->right, height);

      /* Next! */

      seg = seg->next;
    }

  /* Draw the handle which is closest to the mouse position */

  g_pos = control_calc_g_pos (gradient_editor, gradient_editor->control_last_x);

  seg_get_closest_handle (gradient, CLAMP (g_pos, 0.0, 1.0), &seg, &handle);

  switch (handle)
    {
    case GRAD_DRAG_LEFT:
      if (seg)
	{
	  control_draw_normal_handle (gradient_editor, pixmap,
				      seg->left, height);
	}
      else
	{
	  seg = gimp_gradient_segment_get_last (gradient->segments);

	  control_draw_normal_handle (gradient_editor, pixmap,
				      seg->right, height);
	}

      break;

    case GRAD_DRAG_MIDDLE:
      control_draw_middle_handle (gradient_editor, pixmap, seg->middle, height);
      break;

    default:
      break;
    }
}

static void
control_draw_normal_handle (GradientEditor *gradient_editor,
			    GdkPixmap      *pixmap,
			    gdouble         pos,
			    gint            height)
{
  control_draw_handle (pixmap,
		       gradient_editor->control->style->black_gc,
		       gradient_editor->control->style->black_gc,
		       control_calc_p_pos (gradient_editor, pos), height);
}

static void
control_draw_middle_handle (GradientEditor *gradient_editor,
			    GdkPixmap      *pixmap,
			    gdouble         pos,
			    gint            height)
{
  control_draw_handle (pixmap,
		       gradient_editor->control->style->black_gc,
		       gradient_editor->control->style->bg_gc[GTK_STATE_PRELIGHT],
		       control_calc_p_pos (gradient_editor, pos), height);
}

static void
control_draw_handle (GdkPixmap *pixmap,
		     GdkGC     *border_gc,
		     GdkGC     *fill_gc,
		     gint       xpos,
		     gint       height)
{
  gint y;
  gint left, right, bottom;

  for (y = 0; y < height; y++)
    gdk_draw_line (pixmap, fill_gc, xpos - y / 2, y, xpos + y / 2, y);

  bottom = height - 1;
  left   = xpos - bottom / 2;
  right  = xpos + bottom / 2;

  gdk_draw_line (pixmap, border_gc, xpos, 0, left, bottom);
  gdk_draw_line (pixmap, border_gc, xpos, 0, right, bottom);
  gdk_draw_line (pixmap, border_gc, left, bottom, right, bottom);
}

/*****/

static gint
control_calc_p_pos (GradientEditor *gradient_editor,
		    gdouble         pos)
{
  gint           pwidth, pheight;
  GtkAdjustment *adjustment;

  /* Calculate the position (in widget's coordinates) of the
   * requested point from the gradient.  Rounding is done to
   * minimize mismatches between the rendered gradient preview
   * and the gradient control's handles.
   */

  adjustment = GTK_ADJUSTMENT (gradient_editor->scroll_data);
  gdk_drawable_get_size (gradient_editor->control_pixmap, &pwidth, &pheight);

  return RINT ((pwidth - 1) * (pos - adjustment->value) / adjustment->page_size);
}

static gdouble
control_calc_g_pos (GradientEditor *gradient_editor,
		    gint            pos)
{
  gint           pwidth, pheight;
  GtkAdjustment *adjustment;

  /* Calculate the gradient position that corresponds to widget's coordinates */

  adjustment = GTK_ADJUSTMENT (gradient_editor->scroll_data);
  gdk_drawable_get_size (gradient_editor->control_pixmap, &pwidth, &pheight);

  return adjustment->page_size * pos / (pwidth - 1) + adjustment->value;
}

/***** Control popup functions *****/

static void
cpopup_create_main_menu (GradientEditor *gradient_editor)
{
  GtkWidget     *menu;
  GtkWidget     *menuitem;
  GtkWidget     *label;
  GtkAccelGroup *accel_group;

  menu = gtk_menu_new ();
  accel_group = gtk_accel_group_new ();

  gradient_editor->accel_group = accel_group;

  gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);
  gtk_window_add_accel_group (GTK_WINDOW (gradient_editor->shell), accel_group);

  /* Left endpoint */
  menuitem = cpopup_create_color_item (&gradient_editor->left_color_preview,
				       &label);
  gtk_label_set_text (GTK_LABEL (label), _("Left endpoint's color"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  g_signal_connect (G_OBJECT (menuitem), "activate",
		    G_CALLBACK (cpopup_set_left_color_callback),
		      gradient_editor);
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'L', 0,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  menuitem = gtk_menu_item_new_with_label (_("Load from"));
  gradient_editor->control_left_load_popup =
    cpopup_create_load_menu (gradient_editor,
			     gradient_editor->left_load_color_boxes,
			     gradient_editor->left_load_labels,
			     _("Left neighbor's right endpoint"),
			     _("Right endpoint"),
			     G_CALLBACK (cpopup_load_left_callback),
			     'L', GDK_CONTROL_MASK,
			     'L', GDK_MOD1_MASK,
			     'F', GDK_CONTROL_MASK);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem),
			     gradient_editor->control_left_load_popup);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Save to"));
  gradient_editor->control_left_save_popup =
    cpopup_create_save_menu (gradient_editor,
			     gradient_editor->left_save_color_boxes,
			     gradient_editor->left_save_labels,
			     G_CALLBACK (cpopup_save_left_callback));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM(menuitem),
			     gradient_editor->control_left_save_popup);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  /* Right endpoint */
  menuitem = gtk_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = cpopup_create_color_item (&gradient_editor->right_color_preview,
				       &label);
  gtk_label_set_text (GTK_LABEL (label), _("Right endpoint's color"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  g_signal_connect (G_OBJECT (menuitem), "activate",
		    G_CALLBACK (cpopup_set_right_color_callback),
		    gradient_editor);
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'R', 0,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  menuitem = gtk_menu_item_new_with_label (_("Load from"));
  gradient_editor->control_right_load_popup =
    cpopup_create_load_menu (gradient_editor,
			     gradient_editor->right_load_color_boxes,
			     gradient_editor->right_load_labels,
			     _("Right neighbor's left endpoint"),
			     _("Left endpoint"),
			     G_CALLBACK (cpopup_load_right_callback),
			     'R', GDK_CONTROL_MASK,
			     'R', GDK_MOD1_MASK,
			     'F', GDK_MOD1_MASK);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM(menuitem),
			     gradient_editor->control_right_load_popup);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Save to"));
  gradient_editor->control_right_save_popup =
    cpopup_create_save_menu (gradient_editor,
			     gradient_editor->right_save_color_boxes,
			     gradient_editor->right_save_labels,
			     G_CALLBACK (cpopup_save_right_callback));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM(menuitem),
			     gradient_editor->control_right_save_popup);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  /* Blending function */
  menuitem = gtk_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = cpopup_create_menu_item_with_label ("", &gradient_editor->control_blending_label);
  gradient_editor->control_blending_popup =
    cpopup_create_blending_menu (gradient_editor);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM(menuitem),
			     gradient_editor->control_blending_popup);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  /* Coloring type */
  menuitem = cpopup_create_menu_item_with_label ("", &gradient_editor->control_coloring_label);
  gradient_editor->control_coloring_popup =
    cpopup_create_coloring_menu (gradient_editor);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM(menuitem),
			     gradient_editor->control_coloring_popup);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  /* Operations */
  menuitem = gtk_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  /* Split at midpoint */
  menuitem = cpopup_create_menu_item_with_label ("", &gradient_editor->control_split_m_label);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  g_signal_connect (G_OBJECT (menuitem), "activate",
		    G_CALLBACK (cpopup_split_midpoint_callback),
		    gradient_editor);
  gtk_widget_add_accelerator(menuitem, "activate",
			     accel_group,
			     'S', 0,
			     GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  /* Split uniformly */
  menuitem = cpopup_create_menu_item_with_label ("", &gradient_editor->control_split_u_label);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  g_signal_connect (G_OBJECT (menuitem), "activate",
		    G_CALLBACK (cpopup_split_uniform_callback),
		    gradient_editor);
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'U', 0,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  /* Delete */
  menuitem = cpopup_create_menu_item_with_label ("", &gradient_editor->control_delete_label);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);
  gradient_editor->control_delete_menu_item = menuitem;

  g_signal_connect (G_OBJECT (menuitem), "activate",
		    G_CALLBACK (cpopup_delete_callback),
		    gradient_editor);
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'D', 0,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  /* Recenter */
  menuitem = cpopup_create_menu_item_with_label ("", &gradient_editor->control_recenter_label);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  g_signal_connect (G_OBJECT (menuitem), "activate",
		    G_CALLBACK (cpopup_recenter_callback),
		    gradient_editor);
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'C', 0,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  /* Redistribute */
  menuitem = cpopup_create_menu_item_with_label ("", &gradient_editor->control_redistribute_label);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  g_signal_connect (G_OBJECT (menuitem), "activate",
		    G_CALLBACK (cpopup_redistribute_callback),
		    gradient_editor);
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'C', GDK_CONTROL_MASK,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  /* Selection ops */
  menuitem = gtk_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Selection operations"));
  gradient_editor->control_sel_ops_popup =
    cpopup_create_sel_ops_menu (gradient_editor);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem),
			     gradient_editor->control_sel_ops_popup);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  /* Done */
  gradient_editor->control_main_popup = menu;
}

static void
cpopup_do_popup (GradientEditor *gradient_editor)
{
  cpopup_adjust_menus (gradient_editor);

  gtk_menu_popup (GTK_MENU (gradient_editor->control_main_popup),
		  NULL, NULL, NULL, NULL, 3, 0);
}

/***** Create a single menu item *****/

static GtkWidget *
cpopup_create_color_item (GtkWidget **color_box,
			  GtkWidget **label)
{
  GtkWidget *menuitem;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *wcolor_box;
  GtkWidget *wlabel;

  menuitem = gtk_menu_item_new();

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (menuitem), hbox);
  gtk_widget_show (hbox);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  wcolor_box = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (wcolor_box), GDK_RGB_DITHER_MAX);
  gtk_preview_size (GTK_PREVIEW (wcolor_box),
		    GRAD_COLOR_BOX_WIDTH, GRAD_COLOR_BOX_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox), wcolor_box, FALSE, FALSE, 2);
  gtk_widget_show (wcolor_box);

  if (color_box)
    *color_box = wcolor_box;

  wlabel = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (wlabel), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), wlabel, FALSE, FALSE, 4);
  gtk_widget_show (wlabel);

  if (label)
    *label = wlabel;

  return menuitem;
}

static GtkWidget *
cpopup_create_menu_item_with_label (gchar      *str,
				    GtkWidget **label)
{
  GtkWidget *menuitem;
  GtkWidget *accel_label;

  menuitem = gtk_menu_item_new ();

  accel_label = gtk_accel_label_new (str);
  gtk_misc_set_alignment (GTK_MISC (accel_label), 0.0, 0.5);
  gtk_container_add (GTK_CONTAINER (menuitem), accel_label);
  gtk_accel_label_set_accel_object (GTK_ACCEL_LABEL (accel_label), 
                                    G_OBJECT (menuitem));
  gtk_widget_show (accel_label);

  if (label)
    *label = accel_label;

  return menuitem;
}

/***** Update all menus *****/

static void
cpopup_adjust_menus (GradientEditor *gradient_editor)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg;
  gint                 i;
  GimpRGB              fg;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  /* Render main menu color boxes */
  cpopup_render_color_box (GTK_PREVIEW (gradient_editor->left_color_preview),
			   &gradient_editor->control_sel_l->left_color);

  cpopup_render_color_box (GTK_PREVIEW (gradient_editor->right_color_preview),
			   &gradient_editor->control_sel_r->right_color);

  /* Render load color from endpoint color boxes */

  if (gradient_editor->control_sel_l->prev != NULL)
    seg = gradient_editor->control_sel_l->prev;
  else
    seg = gimp_gradient_segment_get_last (gradient_editor->control_sel_l);

  cpopup_render_color_box (GTK_PREVIEW (gradient_editor->left_load_color_boxes[0]),
			   &seg->right_color);

  cpopup_render_color_box (GTK_PREVIEW (gradient_editor->left_load_color_boxes[1]),
			   &gradient_editor->control_sel_r->right_color);

  if (gradient_editor->control_sel_r->next != NULL)
    seg = gradient_editor->control_sel_r->next;
  else
    seg = gradient->segments;

  cpopup_render_color_box (GTK_PREVIEW (gradient_editor->right_load_color_boxes[0]),
			   &seg->left_color);

  cpopup_render_color_box (GTK_PREVIEW (gradient_editor->right_load_color_boxes[1]),
			   &gradient_editor->control_sel_l->left_color);

  /* Render Foreground color boxes */

  gimp_context_get_foreground (gimp_get_user_context (gradient_editor->context->gimp), &fg);

  cpopup_render_color_box (GTK_PREVIEW (gradient_editor->left_load_color_boxes[2]),
			   &fg);

  cpopup_render_color_box (GTK_PREVIEW (gradient_editor->right_load_color_boxes[2]),
			   &fg);
	
  /* Render saved color boxes */

  for (i = 0; i < GRAD_NUM_COLORS; i++)
    cpopup_update_saved_color (gradient_editor,
			       i, &gradient_editor->saved_colors[i]);

  /* Adjust labels */

  if (gradient_editor->control_sel_l == gradient_editor->control_sel_r)
    {
      gtk_label_set_text (GTK_LABEL (gradient_editor->control_blending_label),
			  _("Blending function for segment"));
      gtk_label_set_text (GTK_LABEL (gradient_editor->control_coloring_label),
			  _("Coloring type for segment"));
      gtk_label_set_text (GTK_LABEL (gradient_editor->control_split_m_label),
			  _("Split segment at midpoint"));
      gtk_label_set_text (GTK_LABEL (gradient_editor->control_split_u_label),
			  _("Split segment uniformly"));
      gtk_label_set_text (GTK_LABEL (gradient_editor->control_delete_label),
			  _("Delete segment"));
      gtk_label_set_text (GTK_LABEL (gradient_editor->control_recenter_label),
			  _("Re-center segment's midpoint"));
      gtk_label_set_text (GTK_LABEL (gradient_editor->control_redistribute_label),
			  _("Re-distribute handles in segment"));
      gtk_label_set_text (GTK_LABEL (gradient_editor->control_flip_label),
			  _("Flip segment"));
      gtk_label_set_text (GTK_LABEL (gradient_editor->control_replicate_label),
			  _("Replicate segment"));
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (gradient_editor->control_blending_label),
			  _("Blending function for selection"));
      gtk_label_set_text (GTK_LABEL (gradient_editor->control_coloring_label),
			  _("Coloring type for selection"));
      gtk_label_set_text (GTK_LABEL (gradient_editor->control_split_m_label),
			  _("Split segments at midpoints"));
      gtk_label_set_text (GTK_LABEL (gradient_editor->control_split_u_label),
			  _("Split segments uniformly"));
      gtk_label_set_text (GTK_LABEL (gradient_editor->control_delete_label),
			  _("Delete selection"));
      gtk_label_set_text (GTK_LABEL (gradient_editor->control_recenter_label),
			  _("Re-center midpoints in selection"));
      gtk_label_set_text (GTK_LABEL (gradient_editor->control_redistribute_label),
			  _("Re-distribute handles in selection"));
      gtk_label_set_text (GTK_LABEL (gradient_editor->control_flip_label),
			  _("Flip selection"));
      gtk_label_set_text (GTK_LABEL (gradient_editor->control_replicate_label),
			  _("Replicate selection"));
    }

  /* Adjust blending and coloring menus */
  cpopup_adjust_blending_menu (gradient_editor);
  cpopup_adjust_coloring_menu (gradient_editor);

  /* Can invoke delete? */
  if ((gradient_editor->control_sel_l->prev == NULL) &&
      (gradient_editor->control_sel_r->next == NULL))
    gtk_widget_set_sensitive (gradient_editor->control_delete_menu_item, FALSE);
  else
    gtk_widget_set_sensitive (gradient_editor->control_delete_menu_item, TRUE);

  /* Can invoke blend colors / opacity? */
  if (gradient_editor->control_sel_l == gradient_editor->control_sel_r)
    {
      gtk_widget_set_sensitive (gradient_editor->control_blend_colors_menu_item,
				FALSE);
      gtk_widget_set_sensitive (gradient_editor->control_blend_opacity_menu_item,
				FALSE);
    }
  else
    {
      gtk_widget_set_sensitive (gradient_editor->control_blend_colors_menu_item,
				TRUE);
      gtk_widget_set_sensitive (gradient_editor->control_blend_opacity_menu_item,
				TRUE);
    }
}

static void
cpopup_adjust_blending_menu (GradientEditor *gradient_editor)
{
  gint  equal;
  glong i, num_items;
  gint  type;

  cpopup_check_selection_params (gradient_editor, &equal, NULL);

  /* Block activate signals */
  num_items = (sizeof (gradient_editor->control_blending_items) /
	       sizeof (gradient_editor->control_blending_items[0]));

  type = (int) gradient_editor->control_sel_l->type;

  for (i = 0; i < num_items; i++)
    gtk_signal_handler_block_by_data
      (GTK_OBJECT (gradient_editor->control_blending_items[i]),
       gradient_editor);

  /* Set state */
  if (equal)
    {
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gradient_editor->control_blending_items[type]), TRUE);
      gtk_widget_hide (gradient_editor->control_blending_items[num_items - 1]);
    }
  else
    {
      gtk_widget_show (gradient_editor->control_blending_items[num_items - 1]);
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gradient_editor->control_blending_items[num_items - 1]), TRUE);
    }

  /* Unblock signals */
  for (i = 0; i < num_items; i++)
    gtk_signal_handler_unblock_by_data (GTK_OBJECT (gradient_editor->control_blending_items[i]),
					gradient_editor);
}

static void
cpopup_adjust_coloring_menu (GradientEditor *gradient_editor)
{
  gint  equal;
  glong i, num_items;
  gint  coloring;

  cpopup_check_selection_params (gradient_editor, NULL, &equal);

  /* Block activate signals */
  num_items = (sizeof (gradient_editor->control_coloring_items) /
	       sizeof (gradient_editor->control_coloring_items[0]));

  coloring = (int) gradient_editor->control_sel_l->color;

  for (i = 0; i < num_items; i++)
    gtk_signal_handler_block_by_data (GTK_OBJECT (gradient_editor->control_coloring_items[i]),
				      gradient_editor);

  /* Set state */
  if (equal)
    {
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gradient_editor->control_coloring_items[coloring]), TRUE);
      gtk_widget_hide (gradient_editor->control_coloring_items[num_items - 1]);
    }
  else
    {
      gtk_widget_show (gradient_editor->control_coloring_items[num_items - 1]);
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (gradient_editor->control_coloring_items[num_items - 1]), TRUE);
    }

  /* Unblock signals */
  for (i = 0; i < num_items; i++)
    gtk_signal_handler_unblock_by_data (GTK_OBJECT (gradient_editor->control_coloring_items[i]),
					gradient_editor);
}

static void
cpopup_check_selection_params (GradientEditor *gradient_editor,
			       gint           *equal_blending,
			       gint           *equal_coloring)
{
  GimpGradientSegmentType  type;
  GimpGradientSegmentColor color;
  gint                     etype, ecolor;
  GimpGradientSegment     *seg, *aseg;

  type  = gradient_editor->control_sel_l->type;
  color = gradient_editor->control_sel_l->color;

  etype  = 1;
  ecolor = 1;

  seg = gradient_editor->control_sel_l;

  do
    {
      etype  = etype && (seg->type == type);
      ecolor = ecolor && (seg->color == color);

      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != gradient_editor->control_sel_r);

  if (equal_blending)
    *equal_blending = etype;

  if (equal_coloring)
    *equal_coloring = ecolor;
}

/*****/

static void
cpopup_render_color_box (GtkPreview *preview,
			 GimpRGB    *color)
{
  guchar  rows[3][GRAD_COLOR_BOX_WIDTH * 3];
  gint    x, y;
  gint    r0, g0, b0;
  gint    r1, g1, b1;
  guchar *p0, *p1, *p2;

  /* Fill rows */

  r0 = (GIMP_CHECK_DARK + (color->r - GIMP_CHECK_DARK) * color->a) * 255.0;
  r1 = (GIMP_CHECK_LIGHT + (color->r - GIMP_CHECK_LIGHT) * color->a) * 255.0;

  g0 = (GIMP_CHECK_DARK + (color->g - GIMP_CHECK_DARK) * color->a) * 255.0;
  g1 = (GIMP_CHECK_LIGHT + (color->g - GIMP_CHECK_LIGHT) * color->a) * 255.0;

  b0 = (GIMP_CHECK_DARK + (color->b - GIMP_CHECK_DARK) * color->a) * 255.0;
  b1 = (GIMP_CHECK_LIGHT + (color->b - GIMP_CHECK_LIGHT) * color->a) * 255.0;

  p0 = rows[0];
  p1 = rows[1];
  p2 = rows[2];

  for (x = 0; x < GRAD_COLOR_BOX_WIDTH; x++)
    {
      if ((x == 0) || (x == (GRAD_COLOR_BOX_WIDTH - 1)))
	{
	  *p0++ = 0;
	  *p0++ = 0;
	  *p0++ = 0;

	  *p1++ = 0;
	  *p1++ = 0;
	  *p1++ = 0;
	}
      else
	if ((x / GIMP_CHECK_SIZE) & 1)
	  {
	    *p0++ = r1;
	    *p0++ = g1;
	    *p0++ = b1;

	    *p1++ = r0;
	    *p1++ = g0;
	    *p1++ = b0;
	  }
	else
	  {
	    *p0++ = r0;
	    *p0++ = g0;
	    *p0++ = b0;

	    *p1++ = r1;
	    *p1++ = g1;
	    *p1++ = b1;
	  }

      *p2++ = 0;
      *p2++ = 0;
      *p2++ = 0;
    }

  /* Fill preview */

  gtk_preview_draw_row (preview, rows[2], 0, 0, GRAD_COLOR_BOX_WIDTH);

  for (y = 1; y < (GRAD_COLOR_BOX_HEIGHT - 1); y++)
    if ((y / GIMP_CHECK_SIZE) & 1)
      gtk_preview_draw_row (preview, rows[1], 0, y, GRAD_COLOR_BOX_WIDTH);
    else
      gtk_preview_draw_row (preview, rows[0], 0, y, GRAD_COLOR_BOX_WIDTH);

  gtk_preview_draw_row (preview, rows[2], 0, y, GRAD_COLOR_BOX_WIDTH);
}

/***** Creale load & save menus *****/

static GtkWidget *
cpopup_create_load_menu (GradientEditor  *gradient_editor,
			 GtkWidget      **color_boxes,
			 GtkWidget      **labels,
			 gchar           *label1,
			 gchar           *label2,
			 GCallback        callback,
			 gchar accel_key_0, guint8 accel_mods_0,
			 gchar accel_key_1, guint8 accel_mods_1,
			 gchar accel_key_2, guint8 accel_mods_2)
{
  GtkWidget     *menu;
  GtkWidget     *menuitem;
  GtkAccelGroup *accel_group;
  gint           i;

  menu = gtk_menu_new ();
  accel_group = gradient_editor->accel_group;

  gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);

  /* Create items */
  for (i = 0; i < (GRAD_NUM_COLORS + 3); i++)
    {
      if (i == 3)
	{
	  /* Insert separator between "to fetch" and "saved" colors */
	  menuitem = gtk_menu_item_new ();
	  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	  gtk_widget_show (menuitem);
	}

      menuitem = cpopup_create_color_item (&color_boxes[i], &labels[i]);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
      gtk_widget_show (menuitem);

      g_object_set_data (G_OBJECT (menuitem), "user_data",
			 GINT_TO_POINTER (i));
      g_signal_connect (G_OBJECT (menuitem), "activate",
			callback,
			gradient_editor);

      switch (i)
	{
	case 0:
	  gtk_widget_add_accelerator (menuitem, "activate",
				      accel_group,
				      accel_key_0, accel_mods_0,
				      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);
	  break;
		  
	case 1:
	  gtk_widget_add_accelerator (menuitem, "activate",
				      accel_group,
				      accel_key_1, accel_mods_1,
				      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);
	  break;
		  
	case 2:
	  gtk_widget_add_accelerator (menuitem, "activate",
				      accel_group,
				      accel_key_2, accel_mods_2,
				      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);
	  break;
		  
	default:
	  break;
	}
    }

  /* Set labels */
  gtk_label_set_text (GTK_LABEL (labels[0]), label1);
  gtk_label_set_text (GTK_LABEL (labels[1]), label2);
  gtk_label_set_text (GTK_LABEL (labels[2]), _("FG color"));

  return menu;
}

static GtkWidget *
cpopup_create_save_menu (GradientEditor  *gradient_editor,
			 GtkWidget      **color_boxes,
			 GtkWidget      **labels,
			 GCallback        callback)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  gint       i;

  menu = gtk_menu_new ();

  for (i = 0; i < GRAD_NUM_COLORS; i++)
    {
      menuitem = cpopup_create_color_item (&color_boxes[i], &labels[i]);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
      gtk_widget_show (menuitem);

      g_object_set_data (G_OBJECT (menuitem), "user_data", GINT_TO_POINTER (i));
      g_signal_connect (G_OBJECT (menuitem), "activate",
			  callback,
			  gradient_editor);
    }

  return menu;
}

static void
cpopup_update_saved_color (GradientEditor *gradient_editor,
			   gint            n,
			   GimpRGB        *color)
{
  gchar *str;

  cpopup_render_color_box (GTK_PREVIEW (gradient_editor->left_load_color_boxes[n + 3]),
			   color);
  cpopup_render_color_box (GTK_PREVIEW (gradient_editor->left_save_color_boxes[n]),
			   color);
  cpopup_render_color_box (GTK_PREVIEW (gradient_editor->right_load_color_boxes[n + 3]),
			   color);
  cpopup_render_color_box (GTK_PREVIEW (gradient_editor->right_save_color_boxes[n]),
			   color);

  str = g_strdup_printf (_("RGBA (%0.3f, %0.3f, %0.3f, %0.3f)"),
			 color->r,
			 color->g,
			 color->b,
			 color->a);

  gtk_label_set_text (GTK_LABEL (gradient_editor->left_load_labels[n + 3]), str);
  gtk_label_set_text (GTK_LABEL (gradient_editor->left_save_labels[n]), str);
  gtk_label_set_text (GTK_LABEL (gradient_editor->right_load_labels[n + 3]), str);
  gtk_label_set_text (GTK_LABEL (gradient_editor->right_save_labels[n]), str);

  g_free (str);

  gradient_editor->saved_colors[n] = *color;
}

/*****/

static void
cpopup_load_left_callback (GtkWidget      *widget,
			   GradientEditor *gradient_editor)
{
  GimpGradientSegment *seg;
  GimpRGB              fg;
  gint                 i;

  i = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "user_data"));

  switch (i)
    {
    case 0: /* Fetch from left neighbor's right endpoint */
      if (gradient_editor->control_sel_l->prev != NULL)
	seg = gradient_editor->control_sel_l->prev;
      else
	seg = gimp_gradient_segment_get_last (gradient_editor->control_sel_l);

      cpopup_blend_endpoints (gradient_editor,
			      &seg->right_color,
			      &gradient_editor->control_sel_r->right_color,
			      TRUE, TRUE);
      break;

    case 1: /* Fetch from right endpoint */
      cpopup_blend_endpoints (gradient_editor,
			      &gradient_editor->control_sel_r->right_color,
			      &gradient_editor->control_sel_r->right_color,
			      TRUE, TRUE);
      break;

    case 2: /* Fetch from FG color */
      gimp_context_get_foreground (gimp_get_user_context (gradient_editor->context->gimp), &fg);
      cpopup_blend_endpoints (gradient_editor,
			      &fg,
			      &gradient_editor->control_sel_r->right_color,
			      TRUE, TRUE);
      break;

    default: /* Load a color */
      cpopup_blend_endpoints (gradient_editor,
			      &gradient_editor->saved_colors[i - 3],
			      &gradient_editor->control_sel_r->right_color,
			      TRUE, TRUE);
      break;
    }

  gimp_data_dirty (GIMP_DATA (gimp_context_get_gradient (gradient_editor->context)));

  ed_update_editor (gradient_editor, GRAD_UPDATE_GRADIENT);
}

static void
cpopup_save_left_callback (GtkWidget      *widget,
			   GradientEditor *gradient_editor)
{
  gint i;

  i = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "user_data"));

  gradient_editor->saved_colors[i] = gradient_editor->control_sel_l->left_color;
}

static void
cpopup_load_right_callback (GtkWidget      *widget,
			    GradientEditor *gradient_editor)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg;
  GimpRGB              fg;
  gint                 i;

  i = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "user_data"));

  gradient = gimp_context_get_gradient (gradient_editor->context);

  switch (i)
    {
    case 0: /* Fetch from right neighbor's left endpoint */
      if (gradient_editor->control_sel_r->next != NULL)
	seg = gradient_editor->control_sel_r->next;
      else
	seg = gradient->segments;

      cpopup_blend_endpoints (gradient_editor,
			      &gradient_editor->control_sel_r->left_color,
			      &seg->left_color,
			      TRUE, TRUE);
      break;

    case 1: /* Fetch from left endpoint */
      cpopup_blend_endpoints (gradient_editor,
			      &gradient_editor->control_sel_l->left_color,
			      &gradient_editor->control_sel_l->left_color,
			      TRUE, TRUE);
      break;

    case 2: /* Fetch from FG color */
      gimp_context_get_foreground (gimp_get_user_context (gradient_editor->context->gimp), &fg);
      cpopup_blend_endpoints (gradient_editor,
			      &gradient_editor->control_sel_l->left_color,
			      &fg,
			      TRUE, TRUE);
      break;

    default: /* Load a color */
      cpopup_blend_endpoints (gradient_editor,
			      &gradient_editor->control_sel_l->left_color,
			      &gradient_editor->saved_colors[i - 3],
			      TRUE, TRUE);
      break;
    }

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (gradient_editor, GRAD_UPDATE_GRADIENT);
}

static void
cpopup_save_right_callback (GtkWidget      *widget,
			    GradientEditor *gradient_editor)
{
  gint i;

  i = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "user_data"));

  gradient_editor->saved_colors[i] = gradient_editor->control_sel_r->right_color;
}

/*****/

static GimpGradientSegment *
cpopup_save_selection (GradientEditor *gradient_editor)
{
  GimpGradientSegment *seg, *prev, *tmp;
  GimpGradientSegment *oseg, *oaseg;

  prev = NULL;
  oseg = gradient_editor->control_sel_l;
  tmp  = NULL;

  do
    {
      seg = gimp_gradient_segment_new ();

      *seg = *oseg; /* Copy everything */

      if (prev == NULL)
	tmp = seg; /* Remember first segment */
      else
	prev->next = seg;

      seg->prev = prev;
      seg->next = NULL;

      prev  = seg;
      oaseg = oseg;
      oseg  = oseg->next;
    }
  while (oaseg != gradient_editor->control_sel_r);

  return tmp;
}

static void
cpopup_replace_selection (GradientEditor      *gradient_editor,
			  GimpGradientSegment *replace_seg)
{
  GimpGradient        *gradient;
  GimpGradientSegment *lseg, *rseg;
  GimpGradientSegment *replace_last;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  /* Remember left and right segments */

  lseg = gradient_editor->control_sel_l->prev;
  rseg = gradient_editor->control_sel_r->next;

  replace_last = gimp_gradient_segment_get_last (replace_seg);

  /* Free old selection */

  gradient_editor->control_sel_r->next = NULL;

  gimp_gradient_segments_free (gradient_editor->control_sel_l);

  /* Link in new segments */

  if (lseg)
    lseg->next = replace_seg;
  else
    gradient->segments = replace_seg;

  replace_seg->prev = lseg;

  if (rseg)
    rseg->prev = replace_last;

  replace_last->next = rseg;

  gradient_editor->control_sel_l = replace_seg;
  gradient_editor->control_sel_r = replace_last;

  gradient->last_visited = NULL; /* Force re-search */
}

/***** Color dialogs for left and right endpoint *****/

static void
cpopup_set_left_color_callback (GtkWidget      *widget,
				GradientEditor *gradient_editor)
{
  GimpGradient *gradient;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  gradient_editor->left_saved_dirty    = GIMP_DATA (gradient)->dirty;
  gradient_editor->left_saved_segments = cpopup_save_selection (gradient_editor);

  color_notebook_new (_("Left Endpoint Color"),
		      &gradient_editor->control_sel_l->left_color,
		      (ColorNotebookCallback) cpopup_left_color_changed,
		      gradient_editor,
		      gradient_editor->instant_update,
		      TRUE);

  gtk_widget_set_sensitive (gradient_editor->shell, FALSE);
}

static void
cpopup_set_right_color_callback (GtkWidget      *widget,
				 GradientEditor *gradient_editor)
{
  GimpGradient *gradient;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  gradient_editor->right_saved_dirty    = GIMP_DATA (gradient)->dirty;
  gradient_editor->right_saved_segments = cpopup_save_selection (gradient_editor);

  color_notebook_new (_("Right Endpoint Color"),
		      &gradient_editor->control_sel_l->right_color,
		      (ColorNotebookCallback) cpopup_right_color_changed,
		      gradient_editor,
		      gradient_editor->instant_update,
		      TRUE);

  gtk_widget_set_sensitive (gradient_editor->shell, FALSE);
}

static void
cpopup_left_color_changed (ColorNotebook      *cnb,
			   const GimpRGB      *color,
			   ColorNotebookState  state,
			   gpointer            data)
{
  GradientEditor *gradient_editor;
  GimpGradient   *gradient;

  gradient_editor = (GradientEditor *) data;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  switch (state)
    {
    case COLOR_NOTEBOOK_OK:
      cpopup_blend_endpoints (gradient_editor,
			      (GimpRGB *) color,
			      &gradient_editor->control_sel_r->right_color,
			      TRUE, TRUE);
      gimp_gradient_segments_free (gradient_editor->left_saved_segments);
      gimp_data_dirty (GIMP_DATA (gradient));
      color_notebook_free (cnb);
      gtk_widget_set_sensitive (gradient_editor->shell, TRUE);
      break;

    case COLOR_NOTEBOOK_UPDATE:
      cpopup_blend_endpoints (gradient_editor,
			      (GimpRGB *) color,
			      &gradient_editor->control_sel_r->right_color,
			      TRUE, TRUE);
      gimp_data_dirty (GIMP_DATA (gradient));
      break;

    case COLOR_NOTEBOOK_CANCEL:
      cpopup_replace_selection (gradient_editor,
				gradient_editor->left_saved_segments);
      ed_update_editor (gradient_editor, GRAD_UPDATE_GRADIENT);
      GIMP_DATA (gradient)->dirty = gradient_editor->left_saved_dirty;
      color_notebook_free (cnb);
      gtk_widget_set_sensitive (gradient_editor->shell, TRUE);
      break;
    }

  ed_update_editor (gradient_editor, GRAD_UPDATE_GRADIENT);
}

static void
cpopup_right_color_changed (ColorNotebook      *cnb,
			    const GimpRGB      *color,
			    ColorNotebookState  state,
			    gpointer            data)
{
  GradientEditor *gradient_editor;
  GimpGradient   *gradient;

  gradient_editor = (GradientEditor *) data;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  switch (state)
    {
    case COLOR_NOTEBOOK_UPDATE:
      cpopup_blend_endpoints (gradient_editor,
			      &gradient_editor->control_sel_r->left_color,
			      (GimpRGB *) color,
			      TRUE, TRUE);
      gimp_data_dirty (GIMP_DATA (gradient));
      break;

    case COLOR_NOTEBOOK_OK:
      cpopup_blend_endpoints (gradient_editor,
			      &gradient_editor->control_sel_r->left_color,
			      (GimpRGB *) color,
			      TRUE, TRUE);
      gimp_gradient_segments_free (gradient_editor->right_saved_segments);
      gimp_data_dirty (GIMP_DATA (gradient));
      color_notebook_free (cnb);
      gtk_widget_set_sensitive (gradient_editor->shell, TRUE);
      break;

    case COLOR_NOTEBOOK_CANCEL:
      cpopup_replace_selection (gradient_editor,
				gradient_editor->right_saved_segments);
      GIMP_DATA (gradient)->dirty = gradient_editor->right_saved_dirty;
      color_notebook_free (cnb);
      gtk_widget_set_sensitive (gradient_editor->shell, TRUE);
      break;
    }

  ed_update_editor (gradient_editor, GRAD_UPDATE_GRADIENT);
}

/***** Blending menu *****/

static GtkWidget *
cpopup_create_blending_menu (GradientEditor *gradient_editor)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  GSList    *group;
  gint       i;

  static const gchar *blending_types[] =
  {
    N_("Linear"),
    N_("Curved"),
    N_("Sinusoidal"),
    N_("Spherical (increasing)"),
    N_("Spherical (decreasing)")
  };

  menu  = gtk_menu_new ();
  group = NULL;

  for (i = 0; i < G_N_ELEMENTS (gradient_editor->control_blending_items); i++)
    {
      if (i == (G_N_ELEMENTS (gradient_editor->control_blending_items) - 1))
	{
	  menuitem = gtk_radio_menu_item_new_with_label (group, _("(Varies)"));
	}
      else
	{
	  menuitem =
	    gtk_radio_menu_item_new_with_label (group,
						gettext (blending_types[i]));
	}

      group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menuitem));

      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
      gtk_widget_show (menuitem);

      gradient_editor->control_blending_items[i] = menuitem;

      g_object_set_data (G_OBJECT (menuitem), "user_data", GINT_TO_POINTER (i));
      g_signal_connect (G_OBJECT (menuitem), "activate",
			G_CALLBACK (cpopup_blending_callback),
			gradient_editor);
    }

  /* "Varies" is always disabled */
  gtk_widget_set_sensitive (gradient_editor->control_blending_items[G_N_ELEMENTS (gradient_editor->control_blending_items) - 1], FALSE);

  return menu;
}

static void
cpopup_blending_callback (GtkWidget      *widget,
			  GradientEditor *gradient_editor)
{
  GimpGradient            *gradient;
  GimpGradientSegmentType  type;
  GimpGradientSegment     *seg, *aseg;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  if (!GTK_CHECK_MENU_ITEM (widget)->active)
    return; /* Do nothing if the menu item is being deactivated */

  type = (GimpGradientSegmentType)
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "user_data"));

  seg = gradient_editor->control_sel_l;

  do
    {
      seg->type = type;

      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != gradient_editor->control_sel_r);

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (gradient_editor, GRAD_UPDATE_GRADIENT);
}

/***** Coloring menu *****/

static GtkWidget *
cpopup_create_coloring_menu (GradientEditor *gradient_editor)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  GSList    *group;
  gint       i;

  static const gchar *coloring_types[] =
  {
    N_("Plain RGB"),
    N_("HSV (counter-clockwise hue)"),
    N_("HSV (clockwise hue)")
  };

  menu  = gtk_menu_new ();
  group = NULL;

  for (i = 0; i < G_N_ELEMENTS (gradient_editor->control_coloring_items); i++)
    {
      if (i == (G_N_ELEMENTS (gradient_editor->control_coloring_items) - 1))
	{
	  menuitem = gtk_radio_menu_item_new_with_label(group, _("(Varies)"));
	}
      else
	{
	  menuitem =
	    gtk_radio_menu_item_new_with_label (group,
						gettext (coloring_types[i]));
	}

      group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menuitem));

      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
      gtk_widget_show (menuitem);

      gradient_editor->control_coloring_items[i] = menuitem;

      g_object_set_data (G_OBJECT (menuitem), "user_data", GINT_TO_POINTER (i));
      g_signal_connect (G_OBJECT (menuitem), "activate",
			G_CALLBACK (cpopup_coloring_callback),
			gradient_editor);
    }

  /* "Varies" is always disabled */
  gtk_widget_set_sensitive (gradient_editor->control_coloring_items[G_N_ELEMENTS (gradient_editor->control_coloring_items) - 1], FALSE);

  return menu;
}

static void
cpopup_coloring_callback (GtkWidget      *widget,
			  GradientEditor *gradient_editor)
{
  GimpGradient             *gradient;
  GimpGradientSegmentColor  color;
  GimpGradientSegment      *seg, *aseg;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  if (! GTK_CHECK_MENU_ITEM (widget)->active)
    return; /* Do nothing if the menu item is being deactivated */

  color = (GimpGradientSegmentColor)
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "user_data"));

  seg = gradient_editor->control_sel_l;

  do
    {
      seg->color = color;

      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != gradient_editor->control_sel_r);

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (gradient_editor, GRAD_UPDATE_GRADIENT);
}

/*****/

static void
cpopup_split_midpoint_callback (GtkWidget      *widget,
				GradientEditor *gradient_editor)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg, *lseg, *rseg;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  seg = gradient_editor->control_sel_l;

  do
    {
      cpopup_split_midpoint (gradient, seg, &lseg, &rseg);
      seg = rseg->next;
    }
  while (lseg != gradient_editor->control_sel_r);

  gradient_editor->control_sel_r = rseg;

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (gradient_editor, GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
}

static void
cpopup_split_midpoint (GimpGradient         *gradient,
		       GimpGradientSegment  *lseg,
		       GimpGradientSegment **newl,
		       GimpGradientSegment **newr)
{
  GimpRGB         color;
  GimpGradientSegment *newseg;

  /* Get color at original segment's midpoint */
  gimp_gradient_get_color_at (gradient, lseg->middle, &color);

  /* Create a new segment and insert it in the list */

  newseg = gimp_gradient_segment_new ();

  newseg->prev = lseg;
  newseg->next = lseg->next;

  lseg->next = newseg;

  if (newseg->next)
    newseg->next->prev = newseg;

  /* Set coordinates of new segment */

  newseg->left   = lseg->middle;
  newseg->right  = lseg->right;
  newseg->middle = (newseg->left + newseg->right) / 2.0;

  /* Set coordinates of original segment */

  lseg->right  = newseg->left;
  lseg->middle = (lseg->left + lseg->right) / 2.0;

  /* Set colors of both segments */

  newseg->right_color = lseg->right_color;

  lseg->right_color.r = newseg->left_color.r = color.r;
  lseg->right_color.g = newseg->left_color.g = color.g;
  lseg->right_color.b = newseg->left_color.b = color.b;
  lseg->right_color.a = newseg->left_color.a = color.a;

  /* Set parameters of new segment */

  newseg->type  = lseg->type;
  newseg->color = lseg->color;

  /* Done */

  *newl = lseg;
  *newr = newseg;
}

/*****/

static void
cpopup_split_uniform_callback (GtkWidget      *widget,
			       GradientEditor *gradient_editor)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *scale;
  GtkObject *scale_data;

  /*  Create dialog window  */
  dialog =
    gimp_dialog_new ((gradient_editor->control_sel_l ==
		      gradient_editor->control_sel_r) ?
		     _("Split segment uniformly") :
		     _("Split segments uniformly"),
		     "gradient_segment_split_uniformly",
		     gimp_standard_help_func,
		     "dialogs/gradient_editor/split_segments_uniformly.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     _("Split"), cpopup_split_uniform_split_callback,
		     gradient_editor, NULL, NULL, TRUE, FALSE,
		     GTK_STOCK_CANCEL, cpopup_split_uniform_cancel_callback,
		     gradient_editor, NULL, NULL, FALSE, TRUE,

		     NULL);

  /*  The main vbox  */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
  gtk_widget_show (vbox);

  /*  Instructions  */
  label = gtk_label_new (_("Please select the number of uniform parts"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  label =
    gtk_label_new ((gradient_editor->control_sel_l ==
		    gradient_editor->control_sel_r) ?
		   _("in which you want to split the selected segment") :
		   _("in which you want to split the segments in the selection"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  Scale  */
  gradient_editor->split_parts = 2;
  scale_data = gtk_adjustment_new (2.0, 2.0, 21.0, 1.0, 1.0, 1.0);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 4);
  gtk_widget_show (scale);

  g_signal_connect (G_OBJECT (scale_data), "value_changed",
		    G_CALLBACK (cpopup_split_uniform_scale_update),
		    gradient_editor);

  /*  Show!  */
  gtk_widget_show (dialog);
  gtk_widget_set_sensitive (gradient_editor->shell, FALSE);
}

static void
cpopup_split_uniform_scale_update (GtkAdjustment  *adjustment,
				   GradientEditor *gradient_editor)
{
  gradient_editor->split_parts = ROUND (adjustment->value + 0.5);
}

static void
cpopup_split_uniform_split_callback (GtkWidget      *widget,
				     GradientEditor *gradient_editor)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg, *aseg, *lseg, *rseg, *lsel;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  gtk_widget_destroy (gtk_widget_get_toplevel (widget));
  gtk_widget_set_sensitive (gradient_editor->shell, TRUE);

  seg  = gradient_editor->control_sel_l;
  lsel = NULL;

  do
    {
      aseg = seg;

      cpopup_split_uniform (gradient, seg,
			    gradient_editor->split_parts, &lseg, &rseg);

      if (seg == gradient_editor->control_sel_l)
	lsel = lseg;

      seg = rseg->next;
    }
  while (aseg != gradient_editor->control_sel_r);

  gradient_editor->control_sel_l = lsel;
  gradient_editor->control_sel_r = rseg;

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (gradient_editor, GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
}

static void
cpopup_split_uniform_cancel_callback (GtkWidget      *widget,
				      GradientEditor *gradient_editor)
{
  gtk_widget_destroy (gtk_widget_get_toplevel (widget));
  gtk_widget_set_sensitive (gradient_editor->shell, TRUE);
}

static void
cpopup_split_uniform (GimpGradient         *gradient,
		      GimpGradientSegment  *lseg,
		      gint                  parts,
		      GimpGradientSegment **newl,
		      GimpGradientSegment **newr)
{
  GimpGradientSegment *seg, *prev, *tmp;
  gdouble              seg_len;
  gint                 i;

  seg_len = (lseg->right - lseg->left) / parts; /* Length of divisions */

  seg  = NULL;
  prev = NULL;
  tmp  = NULL;

  for (i = 0; i < parts; i++)
    {
      seg = gimp_gradient_segment_new ();

      if (i == 0)
	tmp = seg; /* Remember first segment */

      seg->left   = lseg->left + i * seg_len;
      seg->right  = lseg->left + (i + 1) * seg_len;
      seg->middle = (seg->left + seg->right) / 2.0;

      gimp_gradient_get_color_at (gradient, seg->left,  &seg->left_color);
      gimp_gradient_get_color_at (gradient, seg->right, &seg->right_color);

      seg->type  = lseg->type;
      seg->color = lseg->color;

      seg->prev = prev;
      seg->next = NULL;

      if (prev)
	prev->next = seg;

      prev = seg;
    }

  /* Fix edges */

  tmp->left_color = lseg->left_color;

  seg->right_color = lseg->right_color;

  tmp->left  = lseg->left;
  seg->right = lseg->right; /* To squish accumulative error */

  /* Link in list */

  tmp->prev = lseg->prev;
  seg->next = lseg->next;

  if (lseg->prev)
    lseg->prev->next = tmp;
  else
    gradient->segments = tmp; /* We are on leftmost segment */

  if (lseg->next)
    lseg->next->prev = seg;

  gradient->last_visited = NULL; /* Force re-search */

  /* Done */

  *newl = tmp;
  *newr = seg;

  /* Delete old segment */

  gimp_gradient_segment_free (lseg);
}

/*****/

static void
cpopup_delete_callback (GtkWidget      *widget,
			GradientEditor *gradient_editor)
{
  GimpGradient        *gradient;
  GimpGradientSegment *lseg, *rseg, *seg, *aseg, *next;
  double               join;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  /* Remember segments to the left and to the right of the selection */

  lseg = gradient_editor->control_sel_l->prev;
  rseg = gradient_editor->control_sel_r->next;

  /* Cannot delete all the segments in the gradient */

  if ((lseg == NULL) && (rseg == NULL))
    return;

  /* Calculate join point */

  join = (gradient_editor->control_sel_l->left +
	  gradient_editor->control_sel_r->right) / 2.0;

  if (lseg == NULL)
    join = 0.0;
  else if (rseg == NULL)
    join = 1.0;

  /* Move segments */

  if (lseg != NULL)
    control_compress_range (lseg, lseg, lseg->left, join);

  if (rseg != NULL)
    control_compress_range (rseg, rseg, join, rseg->right);

  /* Link */

  if (lseg)
    lseg->next = rseg;

  if (rseg)
    rseg->prev = lseg;

  /* Delete old segments */

  seg = gradient_editor->control_sel_l;

  do
    {
      next = seg->next;
      aseg = seg;

      gimp_gradient_segment_free (seg);

      seg = next;
    }
  while (aseg != gradient_editor->control_sel_r);

  /* Change selection */

  if (rseg)
    {
      gradient_editor->control_sel_l = rseg;
      gradient_editor->control_sel_r = rseg;
    }
  else
    {
      gradient_editor->control_sel_l = lseg;
      gradient_editor->control_sel_r = lseg;
    }

  if (lseg == NULL)
    gradient->segments = rseg;

  /* Done */

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (gradient_editor, GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
}

static void
cpopup_recenter_callback (GtkWidget      *widget,
			  GradientEditor *gradient_editor)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg, *aseg;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  seg = gradient_editor->control_sel_l;

  do
    {
      seg->middle = (seg->left + seg->right) / 2.0;

      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != gradient_editor->control_sel_r);

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (gradient_editor, GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
}

static void
cpopup_redistribute_callback (GtkWidget      *widget,
			      GradientEditor *gradient_editor)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg, *aseg;
  gdouble              left, right, seg_len;
  gint                 num_segs;
  gint                 i;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  /* Count number of segments in selection */

  num_segs = 0;
  seg      = gradient_editor->control_sel_l;

  do
    {
      num_segs++;
      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != gradient_editor->control_sel_r);

  /* Calculate new segment length */

  left    = gradient_editor->control_sel_l->left;
  right   = gradient_editor->control_sel_r->right;
  seg_len = (right - left) / num_segs;

  /* Redistribute */

  seg = gradient_editor->control_sel_l;

  for (i = 0; i < num_segs; i++)
    {
      seg->left   = left + i * seg_len;
      seg->right  = left + (i + 1) * seg_len;
      seg->middle = (seg->left + seg->right) / 2.0;

      seg = seg->next;
    }

  /* Fix endpoints to squish accumulative error */

  gradient_editor->control_sel_l->left  = left;
  gradient_editor->control_sel_r->right = right;

  /* Done */

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (gradient_editor, GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
}


/***** Control popup -> selection options functions *****/

static GtkWidget *
cpopup_create_sel_ops_menu (GradientEditor *gradient_editor)
{
  GtkWidget     *menu;
  GtkWidget     *menuitem;
  GtkAccelGroup *accel_group;

  menu = gtk_menu_new ();
  accel_group = gradient_editor->accel_group;

  gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);

  /* Flip */
  menuitem =
    cpopup_create_menu_item_with_label ("", &gradient_editor->control_flip_label);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  g_signal_connect (G_OBJECT (menuitem), "activate",
		    G_CALLBACK (cpopup_flip_callback),
		    gradient_editor);
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'F', 0,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  /* Replicate */
  menuitem =
    cpopup_create_menu_item_with_label ("", &gradient_editor->control_replicate_label);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  g_signal_connect (G_OBJECT (menuitem), "activate",
		    G_CALLBACK (cpopup_replicate_callback),
		    gradient_editor);
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'M', 0,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  /* Blend colors / opacity */
  menuitem = gtk_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Blend endpoints' colors"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);
  gradient_editor->control_blend_colors_menu_item = menuitem;

  g_signal_connect (G_OBJECT (menuitem), "activate",
		    G_CALLBACK (cpopup_blend_colors),
		    gradient_editor);
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'B', 0,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  menuitem = gtk_menu_item_new_with_label (_("Blend endpoints' opacity"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  gtk_widget_show (menuitem);
  gradient_editor->control_blend_opacity_menu_item = menuitem;

  g_signal_connect (G_OBJECT (menuitem), "activate",
		    G_CALLBACK (cpopup_blend_opacity),
		    gradient_editor);
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'B', GDK_CONTROL_MASK,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  return menu;
}

static void
cpopup_flip_callback (GtkWidget      *widget,
		      GradientEditor *gradient_editor)
{
  GimpGradient        *gradient;
  GimpGradientSegment *oseg, *oaseg;
  GimpGradientSegment *seg, *prev, *tmp;
  GimpGradientSegment *lseg, *rseg;
  gdouble              left, right;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  left  = gradient_editor->control_sel_l->left;
  right = gradient_editor->control_sel_r->right;

  /* Build flipped segments */

  prev = NULL;
  oseg = gradient_editor->control_sel_r;
  tmp  = NULL;

  do
    {
      seg = gimp_gradient_segment_new ();

      if (prev == NULL)
	{
	  seg->left = left;
	  tmp = seg; /* Remember first segment */
	}
      else
	seg->left = left + right - oseg->right;

      seg->middle = left + right - oseg->middle;
      seg->right  = left + right - oseg->left;

      seg->left_color = oseg->right_color;

      seg->right_color = oseg->left_color;

      switch (oseg->type)
	{
	case GRAD_SPHERE_INCREASING:
	  seg->type = GRAD_SPHERE_DECREASING;
	  break;

	case GRAD_SPHERE_DECREASING:
	  seg->type = GRAD_SPHERE_INCREASING;
	  break;

	default:
	  seg->type = oseg->type;
	}

      switch (oseg->color)
	{
	case GRAD_HSV_CCW:
	  seg->color = GRAD_HSV_CW;
	  break;

	case GRAD_HSV_CW:
	  seg->color = GRAD_HSV_CCW;
	  break;

	default:
	  seg->color = oseg->color;
	}

      seg->prev = prev;
      seg->next = NULL;

      if (prev)
	prev->next = seg;

      prev = seg;

      oaseg = oseg;
      oseg  = oseg->prev; /* Move backwards! */
    }
  while (oaseg != gradient_editor->control_sel_l);

  seg->right = right; /* Squish accumulative error */

  /* Free old segments */

  lseg = gradient_editor->control_sel_l->prev;
  rseg = gradient_editor->control_sel_r->next;

  oseg = gradient_editor->control_sel_l;

  do
    {
      oaseg = oseg->next;
      gimp_gradient_segment_free (oseg);
      oseg = oaseg;
    }
  while (oaseg != rseg);

  /* Link in new segments */

  if (lseg)
    lseg->next = tmp;
  else
    gradient->segments = tmp;

  tmp->prev = lseg;

  seg->next = rseg;

  if (rseg)
    rseg->prev = seg;

  /* Reset selection */

  gradient_editor->control_sel_l = tmp;
  gradient_editor->control_sel_r = seg;

  /* Done */

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (gradient_editor, GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
}

static void
cpopup_replicate_callback (GtkWidget      *widget,
			   GradientEditor *gradient_editor)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *scale;
  GtkObject *scale_data;

  /*  Create dialog window  */
  dialog =
    gimp_dialog_new ((gradient_editor->control_sel_l ==
		      gradient_editor->control_sel_r) ?
		     _("Replicate segment") :
		     _("Replicate selection"),
		     "gradient_segment_replicate",
		     gimp_standard_help_func,
		     "dialogs/gradient_editor/replicate_segment.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     _("Replicate"), cpopup_do_replicate_callback,
		     gradient_editor, NULL, NULL, FALSE, FALSE,
		     GTK_STOCK_CANCEL, cpopup_replicate_cancel_callback,
		     gradient_editor, NULL, NULL, TRUE, TRUE,

		     NULL);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
  gtk_widget_show (vbox);

  /*  Instructions  */
  label = gtk_label_new (_("Please select the number of times"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  label = gtk_label_new ((gradient_editor->control_sel_l ==
			  gradient_editor->control_sel_r) ?
			 _("you want to replicate the selected segment") :
			 _("you want to replicate the selection"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  Scale  */
  gradient_editor->replicate_times = 2;
  scale_data  = gtk_adjustment_new (2.0, 2.0, 21.0, 1.0, 1.0, 1.0);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, TRUE, 4);
  gtk_widget_show (scale);

  g_signal_connect (G_OBJECT (scale_data), "value_changed",
		    G_CALLBACK (cpopup_replicate_scale_update),
		    gradient_editor);

  /*  Show!  */
  gtk_widget_show (dialog);
  gtk_widget_set_sensitive (gradient_editor->shell, FALSE);
}

static void
cpopup_replicate_scale_update (GtkAdjustment  *adjustment,
			       GradientEditor *gradient_editor)
{
  gradient_editor->replicate_times = (int) (adjustment->value + 0.5);
}

static void
cpopup_do_replicate_callback (GtkWidget      *widget,
			      GradientEditor *gradient_editor)
{
  GimpGradient        *gradient;
  gdouble              sel_left, sel_right, sel_len;
  gdouble              new_left;
  gdouble              factor;
  GimpGradientSegment *prev, *seg, *tmp;
  GimpGradientSegment *oseg, *oaseg;
  GimpGradientSegment *lseg, *rseg;
  gint                 i;

  gradient = gimp_context_get_gradient (gradient_editor->context);

  gtk_widget_destroy (gtk_widget_get_toplevel (widget));
  gtk_widget_set_sensitive (gradient_editor->shell, TRUE);

  /* Remember original parameters */
  sel_left  = gradient_editor->control_sel_l->left;
  sel_right = gradient_editor->control_sel_r->right;
  sel_len   = sel_right - sel_left;

  factor = 1.0 / gradient_editor->replicate_times;

  /* Build replicated segments */

  prev = NULL;
  seg  = NULL;
  tmp  = NULL;

  for (i = 0; i < gradient_editor->replicate_times; i++)
    {
      /* Build one cycle */

      new_left  = sel_left + i * factor * sel_len;

      oseg = gradient_editor->control_sel_l;

      do
	{
	  seg = gimp_gradient_segment_new ();

	  if (prev == NULL)
	    {
	      seg->left = sel_left;
	      tmp = seg; /* Remember first segment */
	    }
	  else
	    seg->left = new_left + factor * (oseg->left - sel_left);

	  seg->middle = new_left + factor * (oseg->middle - sel_left);
	  seg->right  = new_left + factor * (oseg->right - sel_left);

	  seg->left_color = oseg->right_color;

	  seg->right_color = oseg->right_color;

	  seg->type  = oseg->type;
	  seg->color = oseg->color;

	  seg->prev = prev;
	  seg->next = NULL;

	  if (prev)
	    prev->next = seg;

	  prev = seg;

	  oaseg = oseg;
	  oseg  = oseg->next;
	}
      while (oaseg != gradient_editor->control_sel_r);
    }

  seg->right = sel_right; /* Squish accumulative error */

  /* Free old segments */

  lseg = gradient_editor->control_sel_l->prev;
  rseg = gradient_editor->control_sel_r->next;

  oseg = gradient_editor->control_sel_l;

  do
    {
      oaseg = oseg->next;
      gimp_gradient_segment_free (oseg);
      oseg = oaseg;
    }
  while (oaseg != rseg);

  /* Link in new segments */

  if (lseg)
    lseg->next = tmp;
  else
    gradient->segments = tmp;

  tmp->prev = lseg;

  seg->next = rseg;

  if (rseg)
    rseg->prev = seg;

  /* Reset selection */

  gradient_editor->control_sel_l = tmp;
  gradient_editor->control_sel_r = seg;

  /* Done */

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (gradient_editor, GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
}

static void
cpopup_replicate_cancel_callback (GtkWidget      *widget,
				  GradientEditor *gradient_editor)
{
  gtk_widget_destroy (gtk_widget_get_toplevel (widget));
  gtk_widget_set_sensitive (gradient_editor->shell, TRUE);
}

static void
cpopup_blend_colors (GtkWidget      *widget,
		     GradientEditor *gradient_editor)
{
  cpopup_blend_endpoints (gradient_editor,
			  &gradient_editor->control_sel_l->left_color,
			  &gradient_editor->control_sel_r->right_color,
			  TRUE, FALSE);

  gimp_data_dirty (GIMP_DATA (gimp_context_get_gradient (gradient_editor->context)));

  ed_update_editor (gradient_editor, GRAD_UPDATE_GRADIENT);
}

static void
cpopup_blend_opacity (GtkWidget      *widget,
		      GradientEditor *gradient_editor)
{
  cpopup_blend_endpoints (gradient_editor,
			  &gradient_editor->control_sel_l->left_color,
			  &gradient_editor->control_sel_r->right_color,
			  FALSE, TRUE);

  gimp_data_dirty (GIMP_DATA (gimp_context_get_gradient (gradient_editor->context)));

  ed_update_editor (gradient_editor, GRAD_UPDATE_GRADIENT);
}

/***** Main blend function *****/

static void
cpopup_blend_endpoints (GradientEditor *gradient_editor,
			GimpRGB        *rgb1,
			GimpRGB        *rgb2,
			gboolean        blend_colors,
			gboolean        blend_opacity)
{
  GimpRGB              d;
  gdouble              left, len;
  GimpGradientSegment *seg;
  GimpGradientSegment *aseg;

  d.r = rgb2->r - rgb1->r;
  d.g = rgb2->g - rgb1->g;
  d.b = rgb2->b - rgb1->b;
  d.a = rgb2->a - rgb1->a;

  left  = gradient_editor->control_sel_l->left;
  len   = gradient_editor->control_sel_r->right - left;

  seg = gradient_editor->control_sel_l;

  do
    {
      if (blend_colors)
	{
	  seg->left_color.r  = rgb1->r + (seg->left - left) / len * d.r;
	  seg->left_color.g  = rgb1->g + (seg->left - left) / len * d.g;
	  seg->left_color.b  = rgb1->b + (seg->left - left) / len * d.b;

	  seg->right_color.r = rgb1->r + (seg->right - left) / len * d.r;
	  seg->right_color.g = rgb1->g + (seg->right - left) / len * d.g;
	  seg->right_color.b = rgb1->b + (seg->right - left) / len * d.b;
	}

      if (blend_opacity)
	{
	  seg->left_color.a  = rgb1->a + (seg->left - left) / len * d.a;
	  seg->right_color.a = rgb1->a + (seg->right - left) / len * d.a;
	}

      aseg = seg;
      seg = seg->next;
    }
  while (aseg != gradient_editor->control_sel_r);
}

/***** Segment functions *****/

static void
seg_get_closest_handle (GimpGradient         *grad,
			gdouble               pos,
			GimpGradientSegment **seg,
			control_drag_mode_t  *handle)
{
  gdouble l_delta, m_delta, r_delta;

  *seg = gimp_gradient_get_segment_at (grad, pos);

  m_delta = fabs (pos - (*seg)->middle);

  if (pos < (*seg)->middle)
    {
      l_delta = fabs (pos - (*seg)->left);

      if (l_delta < m_delta)
	*handle = GRAD_DRAG_LEFT;
      else
	*handle = GRAD_DRAG_MIDDLE;
    }
  else
    {
      r_delta = fabs (pos - (*seg)->right);

      if (m_delta < r_delta)
	{
	  *handle = GRAD_DRAG_MIDDLE;
	}
      else
	{
	  *seg = (*seg)->next;
	  *handle = GRAD_DRAG_LEFT;
	}
    }
}
