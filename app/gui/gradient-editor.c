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
#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "color_notebook.h"
#include "context_manager.h"
#include "cursorutil.h"
#include "datafiles.h"
#include "errors.h"
#include "gimpcontainer.h"
#include "gimpcontainerlistview.h"
#include "gimpcontext.h"
#include "gimpdatafactory.h"
#include "gimpdnd.h"
#include "gimpgradient.h"
#include "gimplist.h"
#include "gimprc.h"
#include "gradient_editor.h"

#include "libgimp/gimpenv.h"
#include "libgimp/gimplimits.h"

#include "libgimp/gimpintl.h"

#include "pixmaps/zoom_in.xpm"
#include "pixmaps/zoom_out.xpm"


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


typedef struct
{
  GtkWidget   *shell;

  GtkWidget   *hint_label;
  GtkWidget   *view;
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
} GradientEditor;


/***** Local functions *****/

static void       gradient_editor_create           (void);

static void       gradient_editor_drop_gradient    (GtkWidget    *widget,
						    GimpViewable *viewable,
						    gpointer      data);
static void       gradient_editor_gradient_changed (GimpContext  *context,
						    GimpGradient *gradient,
						    gpointer      data);

/* Gradient editor functions */

static GtkWidget * ed_create_button               (gchar           *label,
						   gchar           *help_data,
						   GtkSignalFunc    signal_func,
						   gpointer         data);

static void      ed_update_editor                 (gint             flags);

static void      ed_set_hint                      (gchar           *str);

static void      ed_initialize_saved_colors       (void);

/* Main dialog button callbacks & functions */

static void      ed_new_gradient_callback         (GtkWidget       *widget,
						   gpointer         data);
static void      ed_do_new_gradient_callback      (GtkWidget       *widget,
						   gchar           *gradient_name,
						   gpointer         data);

static void      ed_copy_gradient_callback        (GtkWidget       *widget,
						   gpointer         data);
static void      ed_do_copy_gradient_callback     (GtkWidget       *widget,
						   gchar           *gradient_name,
						   gpointer         data);

static void      ed_delete_gradient_callback      (GtkWidget       *widget,
						   gpointer         data);
static void      ed_do_delete_gradient_callback   (GtkWidget       *widget,
						   gboolean         delete,
						   gpointer         data);

static void      ed_rename_gradient_callback      (GtkWidget       *widget,
						   gpointer         data);
static void      ed_do_rename_gradient_callback   (GtkWidget       *widget,
						   gchar           *gradient_name,
						   gpointer         data);

static void      ed_save_pov_callback             (GtkWidget       *widget,
						   gpointer         data);
static void      ed_do_save_pov_callback          (GtkWidget       *widget,
						   gpointer         data);
static void      ed_cancel_save_pov_callback      (GtkWidget       *widget,
						   gpointer         data);
static gint      ed_delete_save_pov_callback      (GtkWidget       *widget,
						   GdkEvent        *event,
						   gpointer         data);

static void      ed_refresh_grads_callback        (GtkWidget       *widget,
						   gpointer         data);
static void      ed_close_callback                (GtkWidget       *widget,
						   gpointer         data);

/* Zoom, scrollbar & instant update callbacks */

static void      ed_scrollbar_update              (GtkAdjustment   *adjustment,
						   gpointer         data);
static void      ed_zoom_all_callback             (GtkWidget       *widget,
						   gpointer         data);
static void      ed_zoom_out_callback             (GtkWidget       *widget,
						   gpointer         data);
static void      ed_zoom_in_callback              (GtkWidget       *widget,
						   gpointer         data);
static void      ed_instant_update_update         (GtkWidget       *widget,
						   gpointer         data);

/* Gradient preview functions */

static gint      preview_events                   (GtkWidget       *widget,
						   GdkEvent        *event,
						   gpointer         data);
static void      preview_set_hint                 (gint             x);

static void      preview_set_foreground           (gint             x);
static void      preview_set_background           (gint             x);

static void      preview_update                   (gboolean         recalculate);
static void      preview_fill_image               (gint             width,
						   gint             height,
						   gdouble          left,
						   gdouble          right);

/* Gradient control functions */

static gint      control_events                   (GtkWidget       *widget,
						   GdkEvent        *event,
						   gpointer         data);
static void      control_do_hint                  (gint             x,
						   gint             y);
static void      control_button_press             (gint             x,
						   gint             y,
						   guint            button,
						   guint            state);
static gboolean  control_point_in_handle          (GimpGradient    *gradient,
						   gint             x,
						   gint             y,
						   GimpGradientSegment  *seg,
						   control_drag_mode_t handle);
static void      control_select_single_segment    (GimpGradientSegment  *seg);
static void      control_extend_selection         (GimpGradientSegment  *seg,
						   gdouble          pos);
static void      control_motion                   (GimpGradient    *gradient,
						   gint             x);

static void      control_compress_left            (GimpGradientSegment  *range_l,
						   GimpGradientSegment  *range_r,
						   GimpGradientSegment  *drag_seg,
						   gdouble          pos);
static void      control_compress_range           (GimpGradientSegment  *range_l,
						   GimpGradientSegment  *range_r,
						   gdouble          new_l,
						   gdouble          new_r);

static double    control_move                     (GimpGradientSegment  *range_l,
						   GimpGradientSegment  *range_r,
						   gdouble          delta);

/* Control update/redraw functions */

static void      control_update                   (GimpGradient    *gradient,
						   gboolean         recalculate);
static void      control_draw                     (GimpGradient    *gradient,
						   GdkPixmap       *pixmap,
						   gint             width,
						   gint             height,
						   gdouble          left,
						   gdouble          right);
static void      control_draw_normal_handle       (GdkPixmap       *pixmap,
						   gdouble          pos,
						   gint             height);
static void      control_draw_middle_handle       (GdkPixmap       *pixmap,
						   gdouble          pos,
						   gint             height);
static void      control_draw_handle              (GdkPixmap       *pixmap,
						   GdkGC           *border_gc,
						   GdkGC           *fill_gc,
						   gint             xpos,
						   gint             height);

static gint      control_calc_p_pos               (gdouble          pos);
static gdouble   control_calc_g_pos               (gint             pos);

/* Control popup functions */

static void      cpopup_create_main_menu          (void);
static void      cpopup_do_popup                  (void);

static GtkWidget * cpopup_create_color_item           (GtkWidget  **color_box,
						       GtkWidget  **label);
static GtkWidget * cpopup_create_menu_item_with_label (gchar       *str,
						       GtkWidget  **label);

static void      cpopup_adjust_menus              (void);
static void      cpopup_adjust_blending_menu      (void);
static void      cpopup_adjust_coloring_menu      (void);
static void      cpopup_check_selection_params    (gint            *equal_blending,
						   gint            *equal_coloring);

static void      cpopup_render_color_box          (GtkPreview      *preview,
						   GimpRGB         *color);

static GtkWidget * cpopup_create_load_menu        (GtkWidget      **color_boxes,
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
static GtkWidget * cpopup_create_save_menu        (GtkWidget      **color_boxes,
						   GtkWidget      **labels,
						   GtkSignalFunc    callback);

static void      cpopup_update_saved_color        (gint             n,
						   GimpRGB         *color);

static void      cpopup_load_left_callback        (GtkWidget       *widget,
						   gpointer         data);
static void      cpopup_save_left_callback        (GtkWidget       *widget,
						   gpointer         data);
static void      cpopup_load_right_callback       (GtkWidget       *widget,
						   gpointer         data);
static void      cpopup_save_right_callback       (GtkWidget       *widget,
						   gpointer         data);

static GimpGradientSegment * cpopup_save_selection     (void);
static void             cpopup_replace_selection  (GimpGradientSegment  *replace_seg);

/* ----- */

static void   cpopup_set_left_color_callback      (GtkWidget          *widget,
						   gpointer            data);
static void   cpopup_set_right_color_callback     (GtkWidget          *widget,
						   gpointer            data);

static void   cpopup_left_color_changed           (ColorNotebook      *cnb,
						   const GimpRGB      *color,
						   ColorNotebookState  state,
						   gpointer            data);
static void   cpopup_right_color_changed          (ColorNotebook      *cnb,
						   const GimpRGB      *color,
						   ColorNotebookState  state,
						   gpointer            data);

/* ----- */

static GtkWidget * cpopup_create_blending_menu    (void);
static void        cpopup_blending_callback       (GtkWidget       *widget,
						   gpointer         data);
static GtkWidget * cpopup_create_coloring_menu    (void);
static void        cpopup_coloring_callback       (GtkWidget       *widget,
						   gpointer         data);

/* ----- */

static void  cpopup_split_midpoint_callback       (GtkWidget       *widget,
						   gpointer         data);
static void  cpopup_split_midpoint                (GimpGradientSegment  *lseg,
						   GimpGradientSegment **newl,
						   GimpGradientSegment **newr);

static void  cpopup_split_uniform_callback        (GtkWidget       *widget,
						   gpointer         data);
static void  cpopup_split_uniform_scale_update    (GtkAdjustment   *adjustment,
						   gpointer         data);
static void  cpopup_split_uniform_split_callback  (GtkWidget       *widget,
						   gpointer         data);
static void  cpopup_split_uniform_cancel_callback (GtkWidget       *widget,
						   gpointer         data);
static void  cpopup_split_uniform                 (GimpGradientSegment  *lseg,
						   gint             parts,
						   GimpGradientSegment **newl,
						   GimpGradientSegment **newr);

static void  cpopup_delete_callback               (GtkWidget       *widget,
						   gpointer         data);
static void  cpopup_recenter_callback             (GtkWidget       *widget,
						   gpointer         data);
static void  cpopup_redistribute_callback         (GtkWidget       *widget,
						   gpointer         data);

/* Control popup -> Selection operations functions */

static GtkWidget * cpopup_create_sel_ops_menu     (void);

static void      cpopup_flip_callback             (GtkWidget       *widget,
						   gpointer         data);

static void      cpopup_replicate_callback        (GtkWidget       *widget,
						   gpointer         data);
static void      cpopup_replicate_scale_update    (GtkAdjustment   *widget,
						   gpointer         data);
static void      cpopup_do_replicate_callback     (GtkWidget       *widget,
						   gpointer         data);
static void      cpopup_replicate_cancel_callback (GtkWidget       *widget,
						   gpointer         data);

static void      cpopup_blend_colors              (GtkWidget       *widget,
						   gpointer         data);
static void      cpopup_blend_opacity             (GtkWidget       *widget,
						   gpointer         data);

/* Blend function */

static void      cpopup_blend_endpoints           (GimpRGB         *left,
						   GimpRGB         *right,
						   gint             blend_colors,
						   gint             blend_opacity);

/* Segment functions */

static void      seg_get_closest_handle           (GimpGradient         *grad,
						   gdouble               pos,
						   GimpGradientSegment **seg,
						   control_drag_mode_t  *handle);


/***** Local variables *****/

static GradientEditor *g_editor = NULL;


/***** Public gradient editor functions *****/

void
gradient_editor_set_gradient (GimpGradient *gradient)
{
  if (! g_editor)
    gradient_editor_create ();

  if (gimp_container_have (global_gradient_factory->container,
			   GIMP_OBJECT (gradient)))
    {
      gimp_context_set_gradient (g_editor->context, gradient);
    }

  if (! GTK_WIDGET_VISIBLE (g_editor->shell))
    gtk_widget_show (g_editor->shell);
  else
    gdk_window_raise (g_editor->shell->window);
}

void
gradient_editor_free (void)
{
}


/***** The main gradient editor dialog *****/

static void 
gradient_editor_create (void)
{
  GtkWidget   *vbox;
  GtkWidget   *hbox;
  GtkWidget   *gvbox;
  GtkWidget   *button;
  GtkWidget   *frame;
  gint         i;

  /* If the editor already exists, just show it */
  if (g_editor)
    return;

  g_editor = g_new (GradientEditor, 1);

  /* Shell and main vbox */
  g_editor->shell =
    gimp_dialog_new (_("Gradient Editor"), "gradient_editor",
		     gimp_standard_help_func,
		     "dialogs/gradient_editor/gradient_editor.html",
		     GTK_WIN_POS_NONE,
		     FALSE, TRUE, FALSE,

		     _("Refresh"), ed_refresh_grads_callback,
		     NULL, NULL, NULL, FALSE, FALSE,
		     _("Close"), ed_close_callback,
		     NULL, NULL, NULL, TRUE, TRUE,

		     NULL);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (g_editor->shell)->vbox), vbox);
  gtk_widget_show (vbox);

  /* Gradients list box */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  g_editor->context = gimp_context_new (NULL, NULL);

  gtk_signal_connect (GTK_OBJECT (g_editor->context), "gradient_changed",
                      GTK_SIGNAL_FUNC (gradient_editor_gradient_changed),
                      g_editor);

  g_editor->view =
    gimp_container_list_view_new (global_gradient_factory->container,
				  g_editor->context,
				  16, 10, 6);
  gtk_box_pack_start (GTK_BOX (hbox), g_editor->view, TRUE, TRUE, 0); 
  gtk_widget_show (g_editor->view);

  gimp_gtk_drag_dest_set_by_type (g_editor->view,
                                  GTK_DEST_DEFAULT_ALL,
                                  GIMP_TYPE_GRADIENT,
                                  GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_set (GTK_WIDGET (g_editor->view),
                              GIMP_TYPE_GRADIENT,
                              gradient_editor_drop_gradient,
                              g_editor);

  /* Frame & vbox for gradient functions */
  frame = gtk_frame_new (_("Gradient Ops"));
  gtk_box_pack_end (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

  gvbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (gvbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), gvbox);

  /* Buttons for gradient functions */
  button = ed_create_button (_("New Gradient"),
			     "dialogs/gradient_editor/new_gradient.html",
			     GTK_SIGNAL_FUNC (ed_new_gradient_callback),
			     NULL);
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_box_pack_start (GTK_BOX (gvbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = ed_create_button (_("Copy Gradient"),
			     "dialogs/gradient_editor/copy_gradient.html",
			     GTK_SIGNAL_FUNC (ed_copy_gradient_callback),
			     NULL);
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_box_pack_start (GTK_BOX (gvbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = ed_create_button (_("Delete Gradient"),
			     "dialogs/gradient_editor/delete_gradient.html",
			     GTK_SIGNAL_FUNC (ed_delete_gradient_callback),
			     NULL);
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_box_pack_start (GTK_BOX (gvbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = ed_create_button (_("Rename Gradient"),
			     "dialogs/gradient_editor/rename_gradient.html",
			     GTK_SIGNAL_FUNC (ed_rename_gradient_callback),
			     NULL);
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_box_pack_start (GTK_BOX (gvbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = ed_create_button (_("Save as POV-Ray"),
			     "dialogs/gradient_editor/save_as_povray.html",
			     GTK_SIGNAL_FUNC (ed_save_pov_callback),
			     NULL);
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_box_pack_start (GTK_BOX (gvbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gtk_widget_show (gvbox);
  gtk_widget_show (frame);

  /*  Horizontal box for zoom controls, scrollbar and instant update toggle  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  Zoom all button */
  button = ed_create_button (_("Zoom all"), NULL,
			     GTK_SIGNAL_FUNC (ed_zoom_all_callback),
			     g_editor);
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  + and - buttons  */
  gtk_widget_realize (g_editor->shell);

  button = gimp_pixmap_button_new (zoom_in_xpm, NULL);
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (ed_zoom_in_callback),
		      g_editor);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gimp_pixmap_button_new (zoom_out_xpm, NULL);
  GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (ed_zoom_out_callback),
		      g_editor);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /*  Scrollbar  */
  g_editor->zoom_factor = 1;

  g_editor->scroll_data = gtk_adjustment_new (0.0, 0.0, 1.0,
					      1.0 * GRAD_SCROLLBAR_STEP_SIZE,
					      1.0 * GRAD_SCROLLBAR_PAGE_SIZE,
					      1.0);

  gtk_signal_connect (g_editor->scroll_data, "value_changed",
		      GTK_SIGNAL_FUNC (ed_scrollbar_update),
		      g_editor);
  gtk_signal_connect (g_editor->scroll_data, "changed",
		      GTK_SIGNAL_FUNC (ed_scrollbar_update),
		      g_editor);

  g_editor->scrollbar =
    gtk_hscrollbar_new (GTK_ADJUSTMENT (g_editor->scroll_data));
  gtk_range_set_update_policy (GTK_RANGE (g_editor->scrollbar),
			       GTK_UPDATE_CONTINUOUS);
  gtk_box_pack_start (GTK_BOX (hbox), g_editor->scrollbar, TRUE, TRUE, 0);
  gtk_widget_hide (g_editor->scrollbar);

  /* Instant update toggle */
  g_editor->instant_update = TRUE;

  button = gtk_check_button_new_with_label (_("Instant update"));
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (ed_instant_update_update),
		      g_editor);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_widget_show (button);

  /* Frame for gradient preview and gradient control */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gvbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), gvbox); 
  gtk_widget_show (gvbox);

  /* Gradient preview */
  g_editor->preview_rows[0]     = NULL;
  g_editor->preview_rows[1]     = NULL;
  g_editor->preview_last_x      = 0;
  g_editor->preview_button_down = FALSE;

  g_editor->preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (g_editor->preview), GDK_RGB_DITHER_MAX);
  gtk_preview_size (GTK_PREVIEW (g_editor->preview),
		    GRAD_PREVIEW_WIDTH, GRAD_PREVIEW_HEIGHT);

  /*  Enable auto-resizing of the preview but ensure a minimal size  */
  gtk_widget_set_usize (g_editor->preview,
			GRAD_PREVIEW_WIDTH, GRAD_PREVIEW_HEIGHT);
  gtk_preview_set_expand (GTK_PREVIEW (g_editor->preview), TRUE);

  gtk_widget_set_events (g_editor->preview, GRAD_PREVIEW_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (g_editor->preview), "event",
		      GTK_SIGNAL_FUNC (preview_events),
		      g_editor);

  gimp_gtk_drag_dest_set_by_type (g_editor->preview,
                                  GTK_DEST_DEFAULT_ALL,
                                  GIMP_TYPE_GRADIENT,
                                  GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_set (GTK_WIDGET (g_editor->preview),
                              GIMP_TYPE_GRADIENT,
                              gradient_editor_drop_gradient,
                              g_editor);

  gtk_box_pack_start (GTK_BOX (gvbox), g_editor->preview, TRUE, TRUE, 0);
  gtk_widget_show (g_editor->preview);

  /* Gradient control */
  g_editor->control_pixmap                  = NULL;
  g_editor->control_drag_segment            = NULL;
  g_editor->control_sel_l                   = NULL;
  g_editor->control_sel_r                   = NULL;
  g_editor->control_drag_mode               = GRAD_DRAG_NONE;
  g_editor->control_click_time              = 0;
  g_editor->control_compress                = FALSE;
  g_editor->control_last_x                  = 0;
  g_editor->control_last_gx                 = 0.0;
  g_editor->control_orig_pos                = 0.0;
  g_editor->control_main_popup              = NULL;
  g_editor->control_blending_label          = NULL;
  g_editor->control_coloring_label          = NULL;
  g_editor->control_split_m_label           = NULL;
  g_editor->control_split_u_label           = NULL;
  g_editor->control_delete_menu_item        = NULL;
  g_editor->control_delete_label            = NULL;
  g_editor->control_recenter_label          = NULL;
  g_editor->control_redistribute_label      = NULL;
  g_editor->control_flip_label              = NULL;
  g_editor->control_replicate_label         = NULL;
  g_editor->control_blend_colors_menu_item  = NULL;
  g_editor->control_blend_opacity_menu_item = NULL;
  g_editor->control_left_load_popup         = NULL;
  g_editor->control_left_save_popup         = NULL;
  g_editor->control_right_load_popup        = NULL;
  g_editor->control_right_save_popup        = NULL;
  g_editor->control_blending_popup          = NULL;
  g_editor->control_coloring_popup          = NULL;
  g_editor->control_sel_ops_popup           = NULL;

  g_editor->accel_group = NULL;

  for (i = 0;
       i < (sizeof (g_editor->control_blending_items) /
	    sizeof (g_editor->control_blending_items[0]));
       i++)
    g_editor->control_blending_items[i] = NULL;

  for (i = 0;
       i < (sizeof (g_editor->control_coloring_items) /
	    sizeof (g_editor->control_coloring_items[0]));
       i++)
    g_editor->control_coloring_items[i] = NULL;

  g_editor->control = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (g_editor->control),
			 GRAD_PREVIEW_WIDTH, GRAD_CONTROL_HEIGHT);
  gtk_widget_set_events (g_editor->control, GRAD_CONTROL_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (g_editor->control), "event",
		      (GdkEventFunc) control_events,
		      g_editor);
  gtk_box_pack_start (GTK_BOX (gvbox), g_editor->control, TRUE, TRUE, 0);
  gtk_widget_show (g_editor->control);

  /* Hint bar and close button */
  g_editor->hint_label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (g_editor->hint_label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), g_editor->hint_label, FALSE, FALSE, 0);
  gtk_widget_show (g_editor->hint_label);

  /* Initialize other data */
  g_editor->left_color_preview  = NULL;
  g_editor->left_saved_segments = NULL;
  g_editor->left_saved_dirty    = FALSE;

  g_editor->right_color_preview  = NULL;
  g_editor->right_saved_segments = NULL;
  g_editor->right_saved_dirty    = FALSE;

  ed_initialize_saved_colors ();
  cpopup_create_main_menu ();

  if (gimp_container_num_children (global_gradient_factory->container))
    {
      gimp_context_set_gradient (g_editor->context,
				 GIMP_GRADIENT (gimp_container_get_child_by_index (global_gradient_factory->container, 0)));
    }
  else
    {
      GimpGradient *gradient;

      gradient = gimp_gradient_new (_("Default"));

      gimp_container_add (global_gradient_factory->container,
			  GIMP_OBJECT (gradient));
    }

  gtk_widget_show (g_editor->shell);
}

static void
gradient_editor_drop_gradient (GtkWidget    *widget,
			       GimpViewable *viewable,
			       gpointer      data)
{
  gradient_editor_set_gradient (GIMP_GRADIENT (viewable));
}

static void
gradient_editor_gradient_changed (GimpContext  *context,
				  GimpGradient *gradient,
				  gpointer      data)
{
  ed_update_editor (GRAD_UPDATE_PREVIEW | GRAD_RESET_CONTROL);
}

/*****/

static void
ed_update_editor (int flags)
{
  GimpGradient *gradient;

  gradient = gimp_context_get_gradient (g_editor->context);

  if (flags & GRAD_UPDATE_GRADIENT)
    {
      preview_update (TRUE);
    }

  if (flags & GRAD_UPDATE_PREVIEW)
    preview_update (TRUE);

  if (flags & GRAD_UPDATE_CONTROL)
    control_update (gradient, FALSE);

  if (flags & GRAD_RESET_CONTROL)
    control_update (gradient, TRUE);
}

static GtkWidget *
ed_create_button (gchar         *label,
		  gchar         *help_data,
		  GtkSignalFunc  signal_func,
		  gpointer       data)
{
  GtkWidget *button;

  button = gtk_button_new_with_label (label);
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_widget_show (button); 

  if (signal_func != NULL)
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			signal_func,
			data);

  if (help_data)
    gimp_help_set_help_data (button, NULL, help_data);

  return button;
}

static void
ed_set_hint (gchar *str)
{
  gtk_label_set_text (GTK_LABEL (g_editor->hint_label), str);
}

static void
ed_initialize_saved_colors (void)
{
  int i;

  for (i = 0; i < (GRAD_NUM_COLORS + 3); i++)
    {
      g_editor->left_load_color_boxes[i] = NULL;
      g_editor->left_load_labels[i]      = NULL;

      g_editor->right_load_color_boxes[i] = NULL;
      g_editor->right_load_labels[i]      = NULL;
    }

  for (i = 0; i < GRAD_NUM_COLORS; i++)
    {
      g_editor->left_save_color_boxes[i] = NULL;
      g_editor->left_save_labels[i]      = NULL;

      g_editor->right_save_color_boxes[i] = NULL;
      g_editor->right_save_labels[i]      = NULL;
    }

  g_editor->saved_colors[0].r = 0.0; /* Black */
  g_editor->saved_colors[0].g = 0.0;
  g_editor->saved_colors[0].b = 0.0;
  g_editor->saved_colors[0].a = 1.0;

  g_editor->saved_colors[1].r = 0.5; /* 50% Gray */
  g_editor->saved_colors[1].g = 0.5;
  g_editor->saved_colors[1].b = 0.5;
  g_editor->saved_colors[1].a = 1.0;

  g_editor->saved_colors[2].r = 1.0; /* White */
  g_editor->saved_colors[2].g = 1.0;
  g_editor->saved_colors[2].b = 1.0;
  g_editor->saved_colors[2].a = 1.0;

  g_editor->saved_colors[3].r = 0.0; /* Clear */
  g_editor->saved_colors[3].g = 0.0;
  g_editor->saved_colors[3].b = 0.0;
  g_editor->saved_colors[3].a = 0.0;

  g_editor->saved_colors[4].r = 1.0; /* Red */
  g_editor->saved_colors[4].g = 0.0;
  g_editor->saved_colors[4].b = 0.0;
  g_editor->saved_colors[4].a = 1.0;

  g_editor->saved_colors[5].r = 1.0; /* Yellow */
  g_editor->saved_colors[5].g = 1.0;
  g_editor->saved_colors[5].b = 0.0;
  g_editor->saved_colors[5].a = 1.0;

  g_editor->saved_colors[6].r = 0.0; /* Green */
  g_editor->saved_colors[6].g = 1.0;
  g_editor->saved_colors[6].b = 0.0;
  g_editor->saved_colors[6].a = 1.0;

  g_editor->saved_colors[7].r = 0.0; /* Cyan */
  g_editor->saved_colors[7].g = 1.0;
  g_editor->saved_colors[7].b = 1.0;
  g_editor->saved_colors[7].a = 1.0;

  g_editor->saved_colors[8].r = 0.0; /* Blue */
  g_editor->saved_colors[8].g = 0.0;
  g_editor->saved_colors[8].b = 1.0;
  g_editor->saved_colors[8].a = 1.0;

  g_editor->saved_colors[9].r = 1.0; /* Magenta */
  g_editor->saved_colors[9].g = 0.0;
  g_editor->saved_colors[9].b = 1.0;
  g_editor->saved_colors[9].a = 1.0;
}

/***** Main gradient editor dialog callbacks *****/

/***** the "new gradient" dialog functions *****/

static void
ed_new_gradient_callback (GtkWidget *widget,
			  gpointer   data)
{
  GtkWidget *qbox;

  qbox = gimp_query_string_box (_("New gradient"),
				gimp_standard_help_func,
				"dialogs/gradient_editor/new_gradient.html",
				_("Enter a name for the new gradient"),
				_("untitled"),
				NULL, NULL,
				ed_do_new_gradient_callback, NULL);
  gtk_widget_show (qbox);
}

static void
ed_do_new_gradient_callback (GtkWidget *widget,
			     gchar     *gradient_name,
			     gpointer   data)
{
  GimpGradient *grad;

  grad = gimp_gradient_new (gradient_name);

  g_free (gradient_name);

  gimp_data_dirty (GIMP_DATA (grad));

  gimp_container_add (global_gradient_factory->container,
		      GIMP_OBJECT (grad));

  gimp_context_set_gradient (g_editor->context, grad);

  ed_update_editor (GRAD_UPDATE_PREVIEW | GRAD_RESET_CONTROL);
}

/***** The "copy gradient" dialog functions *****/

static void
ed_copy_gradient_callback (GtkWidget *widget,
			   gpointer   data)
{
  GimpGradient *gradient;
  GtkWidget    *qbox;
  gchar        *name;

  gradient = gimp_context_get_gradient (g_editor->context);

  name = g_strdup_printf (_("%s copy"), GIMP_OBJECT (gradient)->name);

  qbox = gimp_query_string_box (_("Copy gradient"),
				gimp_standard_help_func,
				"dialogs/gradient_editor/copy_gradient.html",
				_("Enter a name for the copied gradient"),
				name,
				NULL, NULL,
				ed_do_copy_gradient_callback, NULL);
  gtk_widget_show (qbox);

  g_free (name);
}

static void
ed_do_copy_gradient_callback (GtkWidget *widget,
			      gchar     *gradient_name,
			      gpointer   data)
{
  GimpGradient        *grad;
  GimpGradientSegment *head, *prev, *cur, *orig;

  if (!gradient_name)
    {
      g_warning ("received NULL in call_data");
      return;
    }

  grad = GIMP_GRADIENT (gtk_type_new (GIMP_TYPE_GRADIENT));

  gimp_object_set_name (GIMP_OBJECT (grad), gradient_name);

  g_free (gradient_name);

  gimp_data_dirty (GIMP_DATA (grad));

  prev = NULL;
  orig = gimp_context_get_gradient (g_editor->context)->segments;
  head = NULL;

  while (orig)
    {
      cur = gimp_gradient_segment_new ();

      *cur = *orig;  /* Copy everything */

      cur->prev = prev;
      cur->next = NULL;

      if (prev)
	prev->next = cur;
      else
	head = cur;  /* Remember head */

      prev = cur;
      orig = orig->next;
    }

  grad->segments = head;

  gimp_container_add (global_gradient_factory->container, GIMP_OBJECT (grad));

  gimp_context_set_gradient (g_editor->context, grad);

  ed_update_editor (GRAD_UPDATE_PREVIEW | GRAD_RESET_CONTROL);
}

/***** The "rename gradient" dialog functions *****/

static void
ed_rename_gradient_callback (GtkWidget *widget,
			     gpointer   data)
{
  GimpGradient *gradient;
  GtkWidget    *qbox;

  gradient = gimp_context_get_gradient (g_editor->context);

  qbox = gimp_query_string_box (_("Rename gradient"),
				gimp_standard_help_func,
				"dialogs/gradient_editor/rename_gradient.html",
				_("Enter a new name for the gradient"),
				GIMP_OBJECT (gradient)->name,
				NULL, NULL,
				ed_do_rename_gradient_callback,
				gradient);
  gtk_widget_show (qbox);
}

static void
ed_do_rename_gradient_callback (GtkWidget *widget,
				gchar     *gradient_name,
				gpointer   data)
{
  GimpGradient *grad = (GimpGradient *) data;

  g_return_if_fail (grad != NULL);
  g_return_if_fail (gradient_name != NULL);

  gimp_object_set_name (GIMP_OBJECT (grad), gradient_name);

  g_free (gradient_name);

  gimp_data_dirty (GIMP_DATA (grad));

  ed_update_editor (GRAD_UPDATE_PREVIEW | GRAD_RESET_CONTROL);
}

/***** The "delete gradient" dialog functions *****/

static void
ed_delete_gradient_callback (GtkWidget *widget,
			     gpointer   data)
{
  GimpGradient *gradient;
  GtkWidget    *dialog;
  gchar        *str;

  gradient = gimp_context_get_gradient (g_editor->context);

  if (! (gimp_container_num_children (global_gradient_factory->container) &&
	 gradient))
    return;

  gtk_widget_set_sensitive (g_editor->shell, FALSE);

  str = g_strdup_printf (_("Are you sure you want to delete\n"
			   "\"%s\" from the list and from disk?"),
			 GIMP_OBJECT (gradient)->name);

  dialog =
    gimp_query_boolean_box (_("Delete Gradient"),
			    gimp_standard_help_func,
			    "dialogs/gradient_editor/delete_gradient.html",
			    FALSE,
			    str,
			    _("Delete"), _("Cancel"),
			    NULL, NULL,
			    ed_do_delete_gradient_callback,
			    gradient);

  g_free (str);

  gtk_widget_show (dialog);
}

static void
ed_do_delete_gradient_callback (GtkWidget *widget,
				gboolean   delete,
				gpointer   data)
{
  GimpGradient *delete_gradient;

  gtk_widget_set_sensitive (g_editor->shell, TRUE);

  if (! delete)
    return;

  delete_gradient = (GimpGradient *) data;

  if (! gimp_container_have (global_gradient_factory->container,
			     GIMP_OBJECT (delete_gradient)))
    return;

  if (GIMP_DATA (delete_gradient)->filename)
    gimp_data_delete_from_disk (GIMP_DATA (delete_gradient));

  gimp_container_remove (global_gradient_factory->container,
			 GIMP_OBJECT (delete_gradient));
}

/***** The "save as pov" dialog functions *****/

static void
ed_save_pov_callback (GtkWidget *widget,
		      gpointer   data)
{
  GimpGradient *gradient;
  GtkWidget    *window;

  gradient = gimp_context_get_gradient (g_editor->context);

  if (! gradient)
    return;

  window = gtk_file_selection_new (_("Save as POV-Ray"));
  gtk_window_set_wmclass (GTK_WINDOW (window), "save_gradient", "Gimp");
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);

  gtk_container_set_border_width (GTK_CONTAINER (window), 2);
  gtk_container_set_border_width (GTK_CONTAINER (GTK_FILE_SELECTION (window)->button_area), 2);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->ok_button),
		      "clicked",
		      GTK_SIGNAL_FUNC (ed_do_save_pov_callback),
		      window);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->cancel_button),
		      "clicked",
		      GTK_SIGNAL_FUNC (ed_cancel_save_pov_callback),
		      window);

  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (ed_delete_save_pov_callback),
		      window);

  /*  Connect the "F1" help key  */
  gimp_help_connect_help_accel (window, gimp_standard_help_func,
				"dialogs/gradient_editor/save_as_povray.html");

  gtk_widget_show (window);
  gtk_widget_set_sensitive (g_editor->shell, FALSE);
}

static void
ed_do_save_pov_callback (GtkWidget *widget,
			 gpointer   data)
{
  GimpGradient        *gradient;
  gchar               *filename;
  FILE                *file;
  GimpGradientSegment *seg;

  gradient = gimp_context_get_gradient (g_editor->context);

  if (! gradient)
    return;

  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (data));

  file = fopen (filename, "wb");

  if (!file)
    {
      g_message ("Could not open \"%s\"", filename);
    }
  else
    {
      fprintf (file, "/* color_map file created by the GIMP */\n");
      fprintf (file, "/* http://www.gimp.org/               */\n");

      fprintf (file, "color_map {\n");

      for (seg = gradient->segments; seg; seg = seg->next)
	{
	  /* Left */
	  fprintf (file, "\t[%f color rgbt <%f, %f, %f, %f>]\n",
		   seg->left,
		   seg->left_color.r,
		   seg->left_color.g,
		   seg->left_color.b,
		   1.0 - seg->left_color.a);

	  /* Middle */
	  fprintf (file, "\t[%f color rgbt <%f, %f, %f, %f>]\n",
		   seg->middle,
		   (seg->left_color.r + seg->right_color.r) / 2.0,
		   (seg->left_color.g + seg->right_color.g) / 2.0,
		   (seg->left_color.b + seg->right_color.b) / 2.0,
		   1.0 - (seg->left_color.a + seg->right_color.a) / 2.0);

	  /* Right */
	  fprintf (file, "\t[%f color rgbt <%f, %f, %f, %f>]\n",
		   seg->right,
		   seg->right_color.r,
		   seg->right_color.g,
		   seg->right_color.b,
		   1.0 - seg->right_color.a);
	}

      fprintf (file, "} /* color_map */\n");
      fclose (file);
    }

  gtk_widget_destroy (GTK_WIDGET (data));
  gtk_widget_set_sensitive (g_editor->shell, TRUE);
}

static void
ed_cancel_save_pov_callback (GtkWidget *widget,
			     gpointer   data)
{
  gtk_widget_destroy (GTK_WIDGET (data));
  gtk_widget_set_sensitive (g_editor->shell, TRUE);
}

static gint
ed_delete_save_pov_callback (GtkWidget *widget,
			     GdkEvent  *event,
			     gpointer   data)
{
  ed_cancel_save_pov_callback (widget, data);

  return TRUE;
}

/***** The main dialog action area button callbacks *****/

static void
ed_refresh_grads_callback (GtkWidget *widget,
			   gpointer   data)
{
  gimp_data_factory_data_init (global_gradient_factory, FALSE);

  if (! gimp_container_num_children (global_gradient_factory->container))
    {
      GimpGradient *gradient;

      gradient = gimp_gradient_new (_("Default"));

      gimp_data_dirty (GIMP_DATA (gradient));

      gimp_container_add (global_gradient_factory->container,
			  GIMP_OBJECT (gradient));
    }

  ed_update_editor (GRAD_UPDATE_PREVIEW | GRAD_RESET_CONTROL); 
}

static void
ed_close_callback (GtkWidget *widget,
		   gpointer   data)
{
  if (GTK_WIDGET_VISIBLE (g_editor->shell))
    gtk_widget_hide (g_editor->shell);
}

/***** Zoom, scrollbar & instant update callbacks *****/

static void
ed_scrollbar_update (GtkAdjustment *adjustment,
		     gpointer       data)
{
  gchar *str;

  str = g_strdup_printf (_("Zoom factor: %d:1    Displaying [%0.6f, %0.6f]"),
			 g_editor->zoom_factor, adjustment->value,
			 adjustment->value + adjustment->page_size);

  ed_set_hint (str);
  g_free (str);

  ed_update_editor (GRAD_UPDATE_PREVIEW | GRAD_UPDATE_CONTROL);
}

static void
ed_zoom_all_callback (GtkWidget *widget,
		      gpointer   data)
{
  GtkAdjustment *adjustment;

  adjustment = GTK_ADJUSTMENT (g_editor->scroll_data);

  g_editor->zoom_factor = 1;

  gtk_widget_hide (g_editor->scrollbar);

  adjustment->value     	   = 0.0;
  adjustment->page_size 	   = 1.0;
  adjustment->step_increment = 1.0 * GRAD_SCROLLBAR_STEP_SIZE;
  adjustment->page_increment = 1.0 * GRAD_SCROLLBAR_PAGE_SIZE;

  gtk_signal_emit_by_name (g_editor->scroll_data, "changed");
}

static void
ed_zoom_out_callback (GtkWidget *widget,
		      gpointer   data)
{
  GtkAdjustment *adjustment;
  gdouble        old_value;
  gdouble        value;
  gdouble        old_page_size;
  gdouble        page_size;

  if (g_editor->zoom_factor <= 1)
    return;

  adjustment = GTK_ADJUSTMENT (g_editor->scroll_data);

  old_value     = adjustment->value;
  old_page_size = adjustment->page_size;

  g_editor->zoom_factor--;

  if (g_editor->zoom_factor==1)
    gtk_widget_hide (g_editor->scrollbar);
  else
    gtk_widget_show (g_editor->scrollbar);

  page_size = 1.0 / g_editor->zoom_factor;
  value     = old_value - (page_size - old_page_size) / 2.0;

  if (value < 0.0)
    value = 0.0;
  else if ((value + page_size) > 1.0)
    value = 1.0 - page_size;

  adjustment->value          = value;
  adjustment->page_size      = page_size;
  adjustment->step_increment = page_size * GRAD_SCROLLBAR_STEP_SIZE;
  adjustment->page_increment = page_size * GRAD_SCROLLBAR_PAGE_SIZE;

  gtk_signal_emit_by_name (g_editor->scroll_data, "changed");
}

static void
ed_zoom_in_callback (GtkWidget *widget,
		     gpointer   data)
{
  GtkAdjustment *adjustment;
  gdouble        old_value;
  gdouble        old_page_size;
  gdouble        page_size;

  adjustment = GTK_ADJUSTMENT (g_editor->scroll_data);

  old_value     = adjustment->value;
  old_page_size = adjustment->page_size;

  g_editor->zoom_factor++;

  page_size = 1.0 / g_editor->zoom_factor;

  adjustment->value    	     = old_value + (old_page_size - page_size) / 2.0;
  adjustment->page_size      = page_size;
  adjustment->step_increment = page_size * GRAD_SCROLLBAR_STEP_SIZE;
  adjustment->page_increment = page_size * GRAD_SCROLLBAR_PAGE_SIZE;

  gtk_signal_emit_by_name (g_editor->scroll_data, "changed");
  gtk_widget_show (g_editor->scrollbar);
}

static void
ed_instant_update_update (GtkWidget *widget,
			  gpointer   data)
{
  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      g_editor->instant_update = TRUE;
      gtk_range_set_update_policy (GTK_RANGE (g_editor->scrollbar),
				   GTK_UPDATE_CONTINUOUS);
    }
  else
    {
      g_editor->instant_update = FALSE;
      gtk_range_set_update_policy (GTK_RANGE (g_editor->scrollbar),
				   GTK_UPDATE_DELAYED);
    }
}

/***** Gradient preview functions *****/

static gint
preview_events (GtkWidget *widget,
	     GdkEvent  *event,
	     gpointer   data)
{
  gint            x, y;
  GdkEventButton *bevent;
  GdkEventMotion *mevent;

  switch (event->type)
    {
    case GDK_EXPOSE:
      preview_update (FALSE);
      break;

    case GDK_LEAVE_NOTIFY:
      ed_set_hint ("");
      break;

    case GDK_MOTION_NOTIFY:
      gtk_widget_get_pointer (g_editor->preview, &x, &y);

      mevent = (GdkEventMotion *) event;

      if (x != g_editor->preview_last_x)
	{
	  g_editor->preview_last_x = x;

	  if (g_editor->preview_button_down)
	    {
	      if (mevent->state & GDK_CONTROL_MASK)
		preview_set_background (x);
	      else
		preview_set_foreground (x);
	    }
	  else
	    {
	      preview_set_hint (x);
	    }
	}
      break;

    case GDK_BUTTON_PRESS:
      gtk_widget_get_pointer (g_editor->preview, &x, &y);

      bevent = (GdkEventButton *) event;

      switch (bevent->button)
	{
	case 1:
	  g_editor->preview_last_x = x;
	  g_editor->preview_button_down = TRUE;
	  if (bevent->state & GDK_CONTROL_MASK)
	    preview_set_background (x);
	  else
	    preview_set_foreground (x);
	  break;

	case 3:
	  cpopup_do_popup ();
	  break;

	  /*  wheelmouse support  */
	case 4:
	  {
	    GtkAdjustment *adj = GTK_ADJUSTMENT (g_editor->scroll_data);
	    gfloat new_value = adj->value - adj->page_increment / 2;
	    new_value =
	      CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	    gtk_adjustment_set_value (adj, new_value);
	  }
	  break;

	case 5:
	  {
	    GtkAdjustment *adj = GTK_ADJUSTMENT (g_editor->scroll_data);
	    gfloat new_value = adj->value + adj->page_increment / 2;
	    new_value =
	      CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	    gtk_adjustment_set_value (adj, new_value);
	  }
	  break;

	default:
	  break;
	}

      break;

    case GDK_BUTTON_RELEASE:
      if (g_editor->preview_button_down)
	{
	  gtk_widget_get_pointer (g_editor->preview, &x, &y);

	  bevent = (GdkEventButton *) event;

	  g_editor->preview_last_x = x;
	  g_editor->preview_button_down = FALSE;
	  if (bevent->state & GDK_CONTROL_MASK)
	    preview_set_background (x);
	  else
	    preview_set_foreground (x);
	  break;
	}

      break;

    default:
      break;
    }

  return FALSE;
}

/*****/

static void
preview_set_hint (gint x)
{
  gdouble  xpos;
  GimpRGB  rgb;
  GimpHSV  hsv;
  gchar   *str;

  xpos = control_calc_g_pos (x);

  gimp_gradient_get_color_at (gimp_context_get_gradient (g_editor->context),
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

  ed_set_hint (str);
  g_free (str);
}

/*****/

static void
preview_set_foreground (gint x)
{
  GimpRGB  color;
  gdouble  xpos;
  gchar   *str;

  xpos = control_calc_g_pos (x);
  gimp_gradient_get_color_at (gimp_context_get_gradient (g_editor->context),
			      xpos, &color);

  gimp_context_set_foreground (gimp_context_get_user (), &color);

  str = g_strdup_printf (_("Foreground color set to RGB (%d, %d, %d) <-> "
			   "(%0.3f, %0.3f, %0.3f)"),
			 (gint) (color.r * 255.0),
			 (gint) (color.g * 255.0),
			 (gint) (color.b * 255.0),
			 color.r, color.g, color.b);

  ed_set_hint (str);
  g_free (str);
}

static void
preview_set_background (gint x)
{
  GimpRGB  color;
  gdouble  xpos;
  gchar   *str;

  xpos = control_calc_g_pos (x);
  gimp_gradient_get_color_at (gimp_context_get_gradient (g_editor->context),
			      xpos, &color);

  gimp_context_set_background (gimp_context_get_user (), &color);

  str = g_strdup_printf (_("Background color to RGB (%d, %d, %d) <-> "
			   "(%0.3f, %0.3f, %0.3f)"),
			 (gint) (color.r * 255.0),
			 (gint) (color.g * 255.0),
			 (gint) (color.b * 255.0),
			 color.r, color.g, color.b);

  ed_set_hint (str);
  g_free (str);
}

/*****/

static void
preview_update (gboolean recalculate)
{
  glong          rowsiz;
  GtkAdjustment *adjustment;
  guint16        width;
  guint16        height;

  static guint16  last_width  = 0;
  static guint16  last_height = 0;

  if (! GTK_WIDGET_DRAWABLE (g_editor->preview))
    return;

  /*  See whether we have to re-create the preview widget
   *  (note that the preview automatically follows the size of it's container)
   */
  width  = g_editor->preview->allocation.width;
  height = g_editor->preview->allocation.height;

  if (! g_editor->preview_rows[0] ||
      ! g_editor->preview_rows[1] ||
      (width  != last_width)      ||
      (height != last_height))
    {
      if (g_editor->preview_rows[0])
	g_free (g_editor->preview_rows[0]);

      if (g_editor->preview_rows[1])
	g_free (g_editor->preview_rows[1]);

      rowsiz = width * 3 * sizeof (guchar);

      g_editor->preview_rows[0] = g_malloc (rowsiz);
      g_editor->preview_rows[1] = g_malloc (rowsiz);

      recalculate = TRUE; /* Force recalculation */
    }

  last_width = width;
  last_height = height;

  /* Have to redraw? */
  if (recalculate)
    {
      adjustment = GTK_ADJUSTMENT (g_editor->scroll_data);

      preview_fill_image (width, height,
			  adjustment->value,
			  adjustment->value + adjustment->page_size);

      gtk_widget_draw (g_editor->preview, NULL);
    }
}

/*****/

static void
preview_fill_image (gint    width,
		    gint    height,
		    gdouble left,
		    gdouble right)
{
  GimpGradient *gradient;
  guchar       *p0, *p1;
  gint          x, y;
  gdouble       dx, cur_x;
  GimpRGB       color;
  gdouble       c0, c1;

  gradient = gimp_context_get_gradient (g_editor->context);

  dx    = (right - left) / (width - 1);
  cur_x = left;
  p0    = g_editor->preview_rows[0];
  p1    = g_editor->preview_rows[1];

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
	gtk_preview_draw_row (GTK_PREVIEW (g_editor->preview),
			      g_editor->preview_rows[1], 0, y, width);
      else
	gtk_preview_draw_row (GTK_PREVIEW (g_editor->preview),
			      g_editor->preview_rows[0], 0, y, width);
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
control_events (GtkWidget *widget,
		GdkEvent  *event,
		gpointer   data)
{
  GimpGradient        *gradient;
  GdkEventButton      *bevent;
  GimpGradientSegment *seg;
  gint                 x, y;
  guint32              time;

  gradient = gimp_context_get_gradient (g_editor->context);

  switch (event->type)
    {
    case GDK_EXPOSE:
      control_update (gradient, FALSE);
      break;

    case GDK_LEAVE_NOTIFY:
      ed_set_hint ("");
      break;

    case GDK_BUTTON_PRESS:
      if (g_editor->control_drag_mode == GRAD_DRAG_NONE)
	{
	  gtk_widget_get_pointer (g_editor->control, &x, &y);

	  bevent = (GdkEventButton *) event;

	  g_editor->control_last_x     = x;
	  g_editor->control_click_time = bevent->time;

	  control_button_press (x, y, bevent->button, bevent->state);

	  if (g_editor->control_drag_mode != GRAD_DRAG_NONE)
	    gtk_grab_add (widget);
	}
      break;

    case GDK_BUTTON_RELEASE:
      ed_set_hint ("");

      if (g_editor->control_drag_mode != GRAD_DRAG_NONE)
	{
	  gtk_grab_remove (widget);

	  gtk_widget_get_pointer (g_editor->control, &x, &y);

	  time = ((GdkEventButton *) event)->time;

	  if ((time - g_editor->control_click_time) >= GRAD_MOVE_TIME)
	    {
	      ed_update_editor (GRAD_UPDATE_GRADIENT); /* Possible move */
	    }
	  else if ((g_editor->control_drag_mode == GRAD_DRAG_MIDDLE) ||
		   (g_editor->control_drag_mode == GRAD_DRAG_ALL))
	    {
	      seg = g_editor->control_drag_segment;

	      if ((g_editor->control_drag_mode == GRAD_DRAG_ALL) &&
		  g_editor->control_compress)
		control_extend_selection (seg, control_calc_g_pos (x));
	      else
		control_select_single_segment (seg);

	      ed_update_editor (GRAD_UPDATE_CONTROL);
	    }

	  g_editor->control_drag_mode = GRAD_DRAG_NONE;
	  g_editor->control_compress  = FALSE;

	  control_do_hint (x, y);
	}
      break;

    case GDK_MOTION_NOTIFY:
      gtk_widget_get_pointer (g_editor->control, &x, &y);

      if (x != g_editor->control_last_x)
	{
	  g_editor->control_last_x = x;

	  if (g_editor->control_drag_mode != GRAD_DRAG_NONE)
	    {
	      time = ((GdkEventButton *) event)->time;

	      if ((time - g_editor->control_click_time) >= GRAD_MOVE_TIME)
		control_motion (gradient, x);
	    }
	  else
	    {
	      ed_update_editor (GRAD_UPDATE_CONTROL);

	      control_do_hint (x, y);
	    }
	}
      break;

    default:
      break;
    }

  return FALSE;
}

/*****/

static void
control_do_hint (gint x,
		 gint y)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg;
  control_drag_mode_t  handle;
  gboolean             in_handle;
  double               pos;

  gradient = gimp_context_get_gradient (g_editor->context);

  pos = control_calc_g_pos (x);

  if ((pos < 0.0) || (pos > 1.0))
    return;

  seg_get_closest_handle (gradient, pos, &seg, &handle);

  in_handle = control_point_in_handle (gradient, x, y, seg, handle);

  if (in_handle)
    {
      switch (handle)
	{
	case GRAD_DRAG_LEFT:
	  if (seg != NULL)
	    {
	      if (seg->prev != NULL)
		ed_set_hint (_("Drag: move    Shift+drag: move & compress"));
	      else
		ed_set_hint (_("Click: select    Shift+click: extend selection"));
	    }
	  else
	    {
	      ed_set_hint (_("Click: select    Shift+click: extend selection"));
	    }

	  break;

	case GRAD_DRAG_MIDDLE:
	  ed_set_hint (_("Click: select    Shift+click: extend selection    "
			 "Drag: move"));

	  break;

	default:
	  g_warning ("in_handle is true yet we got handle type %d",
		     (int) handle);
	  break;
	}
    }
  else
    ed_set_hint (_("Click: select    Shift+click: extend selection    "
		   "Drag: move    Shift+drag: move & compress"));
}

/*****/

static void
control_button_press (gint  x,
		      gint  y,
		      guint button,
		      guint state)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg;
  control_drag_mode_t  handle;
  double               xpos;
  gboolean             in_handle;

  gradient = gimp_context_get_gradient (g_editor->context);

  switch (button)
    {
    case 1:
      break;

    case 3:
      cpopup_do_popup();
      return;

      /*  wheelmouse support  */
    case 4:
      {
	GtkAdjustment *adj = GTK_ADJUSTMENT (g_editor->scroll_data);
	gfloat new_value   = adj->value - adj->page_increment / 2;

	new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	gtk_adjustment_set_value (adj, new_value);
      }
      return;

    case 5:
      {
	GtkAdjustment *adj = GTK_ADJUSTMENT (g_editor->scroll_data);
	gfloat new_value   = adj->value + adj->page_increment / 2;

	new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	gtk_adjustment_set_value (adj, new_value);
      }
      return;

    default:
      return;
    }

  /* Find the closest handle */

  xpos = control_calc_g_pos (x);

  seg_get_closest_handle (gradient, xpos, &seg, &handle);

  in_handle = control_point_in_handle (gradient, x, y, seg, handle);

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
		      g_editor->control_drag_mode    = GRAD_DRAG_LEFT;
		      g_editor->control_drag_segment = seg;
		      g_editor->control_compress     = TRUE;
		    }
		  else
		    {
		      control_extend_selection (seg, xpos);
		      ed_update_editor (GRAD_UPDATE_CONTROL);
		    }
		}
	      else if (seg->prev != NULL)
		{
		  g_editor->control_drag_mode    = GRAD_DRAG_LEFT;
		  g_editor->control_drag_segment = seg;
		}
	      else
		{
		  control_select_single_segment (seg);
		  ed_update_editor (GRAD_UPDATE_CONTROL);
		}

	      return;
	    }
	  else  /* seg == NULL */
	    {
	      /* Right handle of last segment */
	      seg = gimp_gradient_segment_get_last (gradient->segments);

	      if (state & GDK_SHIFT_MASK)
		{
		  control_extend_selection (seg, xpos);
		  ed_update_editor (GRAD_UPDATE_CONTROL);
		}
	      else
		{
		  control_select_single_segment (seg);
		  ed_update_editor (GRAD_UPDATE_CONTROL);
		}

	      return;
	    }

	  break;

	case GRAD_DRAG_MIDDLE:
	  if (state & GDK_SHIFT_MASK)
	    {
	      control_extend_selection (seg, xpos);
	      ed_update_editor (GRAD_UPDATE_CONTROL);
	    }
	  else
	    {
	      g_editor->control_drag_mode    = GRAD_DRAG_MIDDLE;
	      g_editor->control_drag_segment = seg;
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

      g_editor->control_drag_mode    = GRAD_DRAG_ALL;
      g_editor->control_drag_segment = seg;
      g_editor->control_last_gx      = xpos;
      g_editor->control_orig_pos     = xpos;

      if (state & GDK_SHIFT_MASK)
	g_editor->control_compress = TRUE;

      return;
    }
}

/*****/

static gboolean
control_point_in_handle (GimpGradient        *gradient,
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
	  handle_pos = control_calc_p_pos (seg->left);
	}
      else
	{
	  seg = gimp_gradient_segment_get_last (gradient->segments);

	  handle_pos = control_calc_p_pos (seg->right);
	}

      break;

    case GRAD_DRAG_MIDDLE:
      handle_pos = control_calc_p_pos (seg->middle);
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
control_select_single_segment (GimpGradientSegment *seg)
{
  g_editor->control_sel_l = seg;
  g_editor->control_sel_r = seg;
}

/*****/

static void
control_extend_selection (GimpGradientSegment *seg,
			  double          pos)
{
  if (fabs (pos - g_editor->control_sel_l->left) <
      fabs (pos - g_editor->control_sel_r->right))
    g_editor->control_sel_l = seg;
  else
    g_editor->control_sel_r = seg;
}

/*****/

static void
control_motion (GimpGradient *gradient,
		gint          x)
{
  GimpGradientSegment *seg;
  gdouble         pos;
  gdouble         delta;
  gchar          *str = NULL;

  seg = g_editor->control_drag_segment;

  switch (g_editor->control_drag_mode)
    {
    case GRAD_DRAG_LEFT:
      pos = control_calc_g_pos (x);

      if (!g_editor->control_compress)
	seg->prev->right = seg->left = CLAMP (pos,
					      seg->prev->middle + EPSILON,
					      seg->middle - EPSILON);
      else
	control_compress_left (g_editor->control_sel_l,
			       g_editor->control_sel_r,
			       seg, pos);

      str = g_strdup_printf (_("Handle position: %0.6f"), seg->left);
      ed_set_hint (str);

      break;

    case GRAD_DRAG_MIDDLE:
      pos = control_calc_g_pos (x);
      seg->middle = CLAMP (pos, seg->left + EPSILON, seg->right - EPSILON);

      str = g_strdup_printf (_("Handle position: %0.6f"), seg->middle);
      ed_set_hint (str);

      break;

    case GRAD_DRAG_ALL:
      pos    = control_calc_g_pos (x);
      delta  = pos - g_editor->control_last_gx;

      if ((seg->left >= g_editor->control_sel_l->left) &&
	  (seg->right <= g_editor->control_sel_r->right))
	delta = control_move (g_editor->control_sel_l,
			      g_editor->control_sel_r, delta);
      else
	delta = control_move (seg, seg, delta);

      g_editor->control_last_gx += delta;

      str = g_strdup_printf (_("Distance: %0.6f"),
			     g_editor->control_last_gx - g_editor->control_orig_pos);
      ed_set_hint (str);

      break;

    default:
      gimp_fatal_error ("Attempt to move bogus handle %d",
			(gint) g_editor->control_drag_mode);
      break;
    }

  if (str)
    g_free (str);

  gimp_data_dirty (GIMP_DATA (gradient));

  if (g_editor->instant_update)
    ed_update_editor (GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
  else
    ed_update_editor (GRAD_UPDATE_CONTROL);
}

/*****/

static void
control_compress_left (GimpGradientSegment *range_l,
		       GimpGradientSegment *range_r,
		       GimpGradientSegment *drag_seg,
		       double               pos)
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

/*****/

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
control_move (GimpGradientSegment *range_l,
	      GimpGradientSegment *range_r,
	      gdouble         delta)
{
  gdouble         lbound, rbound;
  gint            is_first, is_last;
  GimpGradientSegment *seg, *aseg;

  /* First or last segments in gradient? */

  is_first = (range_l->prev == NULL);
  is_last  = (range_r->next == NULL);

  /* Calculate drag bounds */

  if (!g_editor->control_compress)
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
      if (!g_editor->control_compress)
	range_l->prev->right = range_l->left;
      else
	control_compress_range (range_l->prev, range_l->prev,
				range_l->prev->left, range_l->left);
    }

  if (!is_last)
    {
      if (!g_editor->control_compress)
	range_r->next->left = range_r->right;
      else
	control_compress_range (range_r->next, range_r->next,
				range_r->right, range_r->next->right);
    }

  return delta;
}

/*****/

static void
control_update (GimpGradient *gradient,
		gboolean      recalculate)
{
  GtkAdjustment *adjustment;
  gint           cwidth, cheight;
  gint           pwidth, pheight;

  if (! GTK_WIDGET_DRAWABLE (g_editor->control))
    return;

  /*  See whether we have to re-create the control pixmap
   *  depending on the preview's width
   */
  cwidth  = g_editor->preview->allocation.width;
  cheight = GRAD_CONTROL_HEIGHT;

  if (g_editor->control_pixmap)
    gdk_window_get_size (g_editor->control_pixmap, &pwidth, &pheight);

  if (!g_editor->control_pixmap ||
      (cwidth != pwidth) ||
      (cheight != pheight))
    {
      if (g_editor->control_pixmap)
	gdk_pixmap_unref (g_editor->control_pixmap);

      g_editor->control_pixmap =
	gdk_pixmap_new (g_editor->control->window, cwidth, cheight, -1);

      recalculate = TRUE;
    }

  /* Avaoid segfault on first invocation */
  if (cwidth < GRAD_PREVIEW_WIDTH)
    return;

  /* Have to reset the selection? */
  if (recalculate)
    control_select_single_segment (gradient->segments);

  /* Redraw pixmap */
  adjustment = GTK_ADJUSTMENT (g_editor->scroll_data);

  control_draw (gradient,
		g_editor->control_pixmap,
		cwidth, cheight,
		adjustment->value,
		adjustment->value + adjustment->page_size);

  gdk_draw_pixmap (g_editor->control->window, g_editor->control->style->black_gc,
		   g_editor->control_pixmap, 0, 0, 0, 0, cwidth, cheight);
}

/*****/

static void
control_draw (GimpGradient *gradient,
	      GdkPixmap    *pixmap,
	      gint          width,
	      gint          height,
	      gdouble       left,
	      gdouble       right)
{
  gint 	               sel_l, sel_r;
  gdouble              g_pos;
  GimpGradientSegment      *seg;
  control_drag_mode_t  handle;

  /* Clear the pixmap */

  gdk_draw_rectangle (pixmap, g_editor->control->style->bg_gc[GTK_STATE_NORMAL],
		      TRUE, 0, 0, width, height);

  /* Draw selection */

  sel_l = control_calc_p_pos (g_editor->control_sel_l->left);
  sel_r = control_calc_p_pos (g_editor->control_sel_r->right);

  gdk_draw_rectangle (pixmap,
		      g_editor->control->style->dark_gc[GTK_STATE_NORMAL],
		      TRUE, sel_l, 0, sel_r - sel_l + 1, height);

  /* Draw handles */

  seg = gradient->segments;

  while (seg)
    {
      control_draw_normal_handle (pixmap, seg->left, height);
      control_draw_middle_handle (pixmap, seg->middle, height);

      /* Draw right handle only if this is the last segment */

      if (seg->next == NULL)
	control_draw_normal_handle (pixmap, seg->right, height);

      /* Next! */

      seg = seg->next;
    }

  /* Draw the handle which is closest to the mouse position */

  g_pos = control_calc_g_pos (g_editor->control_last_x);

  seg_get_closest_handle (gradient, CLAMP (g_pos, 0.0, 1.0), &seg, &handle);

  switch (handle)
    {
    case GRAD_DRAG_LEFT:
      if (seg)
	{
	  control_draw_normal_handle (pixmap, seg->left, height);
	}
      else
	{
	  seg = gimp_gradient_segment_get_last (gradient->segments);

	  control_draw_normal_handle (pixmap, seg->right, height);
	}

      break;

    case GRAD_DRAG_MIDDLE:
      control_draw_middle_handle (pixmap, seg->middle, height);
      break;

    default:
      break;
    }
}

/*****/

static void
control_draw_normal_handle (GdkPixmap *pixmap,
			    gdouble    pos,
			    gint       height)
{
  control_draw_handle (pixmap,
		       g_editor->control->style->black_gc,
		       g_editor->control->style->black_gc,
		       control_calc_p_pos (pos), height);
}

static void
control_draw_middle_handle (GdkPixmap *pixmap,
			    gdouble    pos,
			    gint       height)
{
  control_draw_handle (pixmap,
		       g_editor->control->style->black_gc,
		       g_editor->control->style->bg_gc[GTK_STATE_PRELIGHT],
		       control_calc_p_pos(pos), height);
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
control_calc_p_pos (gdouble pos)
{
  gint           pwidth, pheight;
  GtkAdjustment *adjustment;

  /* Calculate the position (in widget's coordinates) of the
   * requested point from the gradient.  Rounding is done to
   * minimize mismatches between the rendered gradient preview
   * and the gradient control's handles.
   */

  adjustment = GTK_ADJUSTMENT (g_editor->scroll_data);
  gdk_window_get_size (g_editor->control_pixmap, &pwidth, &pheight);

  return RINT ((pwidth - 1) * (pos - adjustment->value) / adjustment->page_size);
}

/*****/

static gdouble
control_calc_g_pos (gint pos)
{
  gint           pwidth, pheight;
  GtkAdjustment *adjustment;

  /* Calculate the gradient position that corresponds to widget's coordinates */

  adjustment = GTK_ADJUSTMENT (g_editor->scroll_data);
  gdk_window_get_size (g_editor->control_pixmap, &pwidth, &pheight);

  return adjustment->page_size * pos / (pwidth - 1) + adjustment->value;
}

/***** Control popup functions *****/

static void
cpopup_create_main_menu (void)
{
  GtkWidget     *menu;
  GtkWidget     *menuitem;
  GtkWidget     *label;
  GtkAccelGroup *accel_group;

  menu = gtk_menu_new ();
  accel_group = gtk_accel_group_new ();

  g_editor->accel_group = accel_group;

  gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);
  gtk_window_add_accel_group (GTK_WINDOW (g_editor->shell), accel_group);

  /* Left endpoint */
  menuitem = cpopup_create_color_item (&g_editor->left_color_preview, &label);
  gtk_label_set_text (GTK_LABEL (label), _("Left endpoint's color"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      (GtkSignalFunc) cpopup_set_left_color_callback,
		      NULL);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'L', 0,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  menuitem = gtk_menu_item_new_with_label (_("Load from"));
  g_editor->control_left_load_popup =
    cpopup_create_load_menu (g_editor->left_load_color_boxes,
			     g_editor->left_load_labels,
			     _("Left neighbor's right endpoint"),
			     _("Right endpoint"),
			     (GtkSignalFunc) cpopup_load_left_callback,
			     'L', GDK_CONTROL_MASK,
			     'L', GDK_MOD1_MASK,
			     'F', GDK_CONTROL_MASK);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem),
			     g_editor->control_left_load_popup);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Save to"));
  g_editor->control_left_save_popup =
    cpopup_create_save_menu (g_editor->left_save_color_boxes,
			     g_editor->left_save_labels,
			     (GtkSignalFunc) cpopup_save_left_callback);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM(menuitem),
			     g_editor->control_left_save_popup);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  /* Right endpoint */
  menuitem = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = cpopup_create_color_item (&g_editor->right_color_preview, &label);
  gtk_label_set_text (GTK_LABEL (label), _("Right endpoint's color"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      (GtkSignalFunc) cpopup_set_right_color_callback,
		      NULL);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'R', 0,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  menuitem = gtk_menu_item_new_with_label (_("Load from"));
  g_editor->control_right_load_popup =
    cpopup_create_load_menu (g_editor->right_load_color_boxes,
			     g_editor->right_load_labels,
			     _("Right neighbor's left endpoint"),
			     _("Left endpoint"),
			     (GtkSignalFunc) cpopup_load_right_callback,
			     'R', GDK_CONTROL_MASK,
			     'R', GDK_MOD1_MASK,
			     'F', GDK_MOD1_MASK);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM(menuitem),
			     g_editor->control_right_load_popup);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Save to"));
  g_editor->control_right_save_popup =
    cpopup_create_save_menu (g_editor->right_save_color_boxes,
			     g_editor->right_save_labels,
			     (GtkSignalFunc) cpopup_save_right_callback);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM(menuitem),
			     g_editor->control_right_save_popup);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  /* Blending function */
  menuitem = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = cpopup_create_menu_item_with_label ("", &g_editor->control_blending_label);
  g_editor->control_blending_popup = cpopup_create_blending_menu ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM(menuitem),
			     g_editor->control_blending_popup);
  gtk_menu_append (GTK_MENU(menu), menuitem);
  gtk_widget_show (menuitem);

  /* Coloring type */
  menuitem = cpopup_create_menu_item_with_label ("", &g_editor->control_coloring_label);
  g_editor->control_coloring_popup = cpopup_create_coloring_menu ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM(menuitem),
			     g_editor->control_coloring_popup);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  /* Operations */
  menuitem = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  /* Split at midpoint */
  menuitem = cpopup_create_menu_item_with_label ("", &g_editor->control_split_m_label);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      (GtkSignalFunc) cpopup_split_midpoint_callback,
		      NULL);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_widget_add_accelerator(menuitem, "activate",
			     accel_group,
			     'S', 0,
			     GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  /* Split uniformly */
  menuitem = cpopup_create_menu_item_with_label ("", &g_editor->control_split_u_label);
  gtk_signal_connect (GTK_OBJECT(menuitem), "activate",
		      (GtkSignalFunc) cpopup_split_uniform_callback,
		      NULL);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'U', 0,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  /* Delete */
  menuitem = cpopup_create_menu_item_with_label ("", &g_editor->control_delete_label);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      (GtkSignalFunc) cpopup_delete_callback,
		      NULL);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  g_editor->control_delete_menu_item = menuitem;
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'D', 0,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  /* Recenter */
  menuitem = cpopup_create_menu_item_with_label ("", &g_editor->control_recenter_label);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      (GtkSignalFunc) cpopup_recenter_callback,
		      NULL);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'C', 0,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  /* Redistribute */
  menuitem = cpopup_create_menu_item_with_label ("", &g_editor->control_redistribute_label);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      (GtkSignalFunc) cpopup_redistribute_callback,
		      NULL);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'C', GDK_CONTROL_MASK,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  /* Selection ops */
  menuitem = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Selection operations"));
  g_editor->control_sel_ops_popup = cpopup_create_sel_ops_menu ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem),
			     g_editor->control_sel_ops_popup);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  /* Done */
  g_editor->control_main_popup = menu;
}

static void
cpopup_do_popup (void)
{
  cpopup_adjust_menus ();
  gtk_menu_popup (GTK_MENU (g_editor->control_main_popup),
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
  gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (accel_label), menuitem);
  gtk_widget_show (accel_label);

  if (label)
    *label = accel_label;

  return menuitem;
}

/***** Update all menus *****/

static void
cpopup_adjust_menus (void)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg;
  gint                 i;
  GimpRGB              fg;

  gradient = gimp_context_get_gradient (g_editor->context);

  /* Render main menu color boxes */
  cpopup_render_color_box (GTK_PREVIEW (g_editor->left_color_preview),
			   &g_editor->control_sel_l->left_color);

  cpopup_render_color_box (GTK_PREVIEW (g_editor->right_color_preview),
			   &g_editor->control_sel_r->right_color);

  /* Render load color from endpoint color boxes */

  if (g_editor->control_sel_l->prev != NULL)
    seg = g_editor->control_sel_l->prev;
  else
    seg = gimp_gradient_segment_get_last (g_editor->control_sel_l);

  cpopup_render_color_box (GTK_PREVIEW (g_editor->left_load_color_boxes[0]),
			   &seg->right_color);

  cpopup_render_color_box (GTK_PREVIEW (g_editor->left_load_color_boxes[1]),
			   &g_editor->control_sel_r->right_color);

  if (g_editor->control_sel_r->next != NULL)
    seg = g_editor->control_sel_r->next;
  else
    seg = gradient->segments;

  cpopup_render_color_box (GTK_PREVIEW (g_editor->right_load_color_boxes[0]),
			   &seg->left_color);

  cpopup_render_color_box (GTK_PREVIEW (g_editor->right_load_color_boxes[1]),
			   &g_editor->control_sel_l->left_color);

  /* Render Foreground color boxes */

  gimp_context_get_foreground (gimp_context_get_user (), &fg);

  cpopup_render_color_box (GTK_PREVIEW (g_editor->left_load_color_boxes[2]),
			   &fg);

  cpopup_render_color_box (GTK_PREVIEW (g_editor->right_load_color_boxes[2]),
			   &fg);
	
  /* Render saved color boxes */

  for (i = 0; i < GRAD_NUM_COLORS; i++)
    cpopup_update_saved_color (i, &g_editor->saved_colors[i]);

  /* Adjust labels */

  if (g_editor->control_sel_l == g_editor->control_sel_r)
    {
      gtk_label_set_text (GTK_LABEL (g_editor->control_blending_label),
			  _("Blending function for segment"));
      gtk_label_set_text (GTK_LABEL (g_editor->control_coloring_label),
			  _("Coloring type for segment"));
      gtk_label_set_text (GTK_LABEL (g_editor->control_split_m_label),
			  _("Split segment at midpoint"));
      gtk_label_set_text (GTK_LABEL (g_editor->control_split_u_label),
			  _("Split segment uniformly"));
      gtk_label_set_text (GTK_LABEL (g_editor->control_delete_label),
			  _("Delete segment"));
      gtk_label_set_text (GTK_LABEL (g_editor->control_recenter_label),
			  _("Re-center segment's midpoint"));
      gtk_label_set_text (GTK_LABEL (g_editor->control_redistribute_label),
			  _("Re-distribute handles in segment"));
      gtk_label_set_text (GTK_LABEL (g_editor->control_flip_label),
			  _("Flip segment"));
      gtk_label_set_text (GTK_LABEL (g_editor->control_replicate_label),
			  _("Replicate segment"));
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (g_editor->control_blending_label),
			  _("Blending function for selection"));
      gtk_label_set_text (GTK_LABEL (g_editor->control_coloring_label),
			  _("Coloring type for selection"));
      gtk_label_set_text (GTK_LABEL (g_editor->control_split_m_label),
			  _("Split segments at midpoints"));
      gtk_label_set_text (GTK_LABEL (g_editor->control_split_u_label),
			  _("Split segments uniformly"));
      gtk_label_set_text (GTK_LABEL (g_editor->control_delete_label),
			  _("Delete selection"));
      gtk_label_set_text (GTK_LABEL (g_editor->control_recenter_label),
			  _("Re-center midpoints in selection"));
      gtk_label_set_text (GTK_LABEL (g_editor->control_redistribute_label),
			  _("Re-distribute handles in selection"));
      gtk_label_set_text (GTK_LABEL (g_editor->control_flip_label),
			  _("Flip selection"));
      gtk_label_set_text (GTK_LABEL (g_editor->control_replicate_label),
			  _("Replicate selection"));
    }

  /* Adjust blending and coloring menus */
  cpopup_adjust_blending_menu ();
  cpopup_adjust_coloring_menu ();

  /* Can invoke delete? */
  if ((g_editor->control_sel_l->prev == NULL) &&
      (g_editor->control_sel_r->next == NULL))
    gtk_widget_set_sensitive (g_editor->control_delete_menu_item, FALSE);
  else
    gtk_widget_set_sensitive (g_editor->control_delete_menu_item, TRUE);

  /* Can invoke blend colors / opacity? */
  if (g_editor->control_sel_l == g_editor->control_sel_r)
    {
      gtk_widget_set_sensitive (g_editor->control_blend_colors_menu_item, FALSE);
      gtk_widget_set_sensitive (g_editor->control_blend_opacity_menu_item, FALSE);
    }
  else
    {
      gtk_widget_set_sensitive (g_editor->control_blend_colors_menu_item, TRUE);
      gtk_widget_set_sensitive (g_editor->control_blend_opacity_menu_item, TRUE);
    }
}

static void
cpopup_adjust_blending_menu (void)
{
  gint  equal;
  glong i, num_items;
  gint  type;

  cpopup_check_selection_params (&equal, NULL);

  /* Block activate signals */
  num_items = (sizeof (g_editor->control_blending_items) /
	       sizeof (g_editor->control_blending_items[0]));

  type = (int) g_editor->control_sel_l->type;

  for (i = 0; i < num_items; i++)
    gtk_signal_handler_block_by_data
      (GTK_OBJECT (g_editor->control_blending_items[i]), (gpointer) i);

  /* Set state */
  if (equal)
    {
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (g_editor->control_blending_items[type]), TRUE);
      gtk_widget_hide (g_editor->control_blending_items[num_items - 1]);
    }
  else
    {
      gtk_widget_show (g_editor->control_blending_items[num_items - 1]);
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (g_editor->control_blending_items[num_items - 1]), TRUE);
    }

  /* Unblock signals */
  for (i = 0; i < num_items; i++)
    gtk_signal_handler_unblock_by_data (GTK_OBJECT (g_editor->control_blending_items[i]), (gpointer) i);
}

static void
cpopup_adjust_coloring_menu (void)
{
  gint  equal;
  glong i, num_items;
  gint  coloring;

  cpopup_check_selection_params (NULL, &equal);

  /* Block activate signals */
  num_items = (sizeof (g_editor->control_coloring_items) /
	       sizeof (g_editor->control_coloring_items[0]));

  coloring = (int) g_editor->control_sel_l->color;

  for (i = 0; i < num_items; i++)
    gtk_signal_handler_block_by_data (GTK_OBJECT (g_editor->control_coloring_items[i]), (gpointer) i);

  /* Set state */
  if (equal)
    {
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (g_editor->control_coloring_items[coloring]), TRUE);
      gtk_widget_hide (g_editor->control_coloring_items[num_items - 1]);
    }
  else
    {
      gtk_widget_show (g_editor->control_coloring_items[num_items - 1]);
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (g_editor->control_coloring_items[num_items - 1]), TRUE);
    }

  /* Unblock signals */
  for (i = 0; i < num_items; i++)
    gtk_signal_handler_unblock_by_data (GTK_OBJECT (g_editor->control_coloring_items[i]), (gpointer) i);
}

static void
cpopup_check_selection_params (gint *equal_blending,
			       gint *equal_coloring)
{
  GimpGradientSegmentType     type;
  GimpGradientSegmentColor    color;
  int             etype, ecolor;
  GimpGradientSegment *seg, *aseg;

  type  = g_editor->control_sel_l->type;
  color = g_editor->control_sel_l->color;

  etype  = 1;
  ecolor = 1;

  seg = g_editor->control_sel_l;

  do
    {
      etype  = etype && (seg->type == type);
      ecolor = ecolor && (seg->color == color);

      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != g_editor->control_sel_r);

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
cpopup_create_load_menu (GtkWidget    **color_boxes,
			 GtkWidget    **labels,
			 gchar         *label1,
			 gchar         *label2,
			 GtkSignalFunc  callback,
			 gchar accel_key_0, guint8 accel_mods_0,
			 gchar accel_key_1, guint8 accel_mods_1,
			 gchar accel_key_2, guint8 accel_mods_2)
{
  GtkWidget     *menu;
  GtkWidget     *menuitem;
  GtkAccelGroup *accel_group;
  gint i;

  menu = gtk_menu_new ();
  accel_group = g_editor->accel_group;

  gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);

  /* Create items */
  for (i = 0; i < (GRAD_NUM_COLORS + 3); i++)
    {
      if (i == 3)
	{
	  /* Insert separator between "to fetch" and "saved" colors */
	  menuitem = gtk_menu_item_new ();
	  gtk_menu_append (GTK_MENU (menu), menuitem);
	  gtk_widget_show (menuitem);
	}

      menuitem = cpopup_create_color_item (&color_boxes[i], &labels[i]);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  callback, (gpointer) ((long) i));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_widget_show (menuitem);

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
cpopup_create_save_menu (GtkWidget     **color_boxes,
			 GtkWidget     **labels,
			 GtkSignalFunc   callback)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  gint       i;

  menu = gtk_menu_new ();

  for (i = 0; i < GRAD_NUM_COLORS; i++)
    {
      menuitem = cpopup_create_color_item (&color_boxes[i], &labels[i]);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  callback, (gpointer) ((long) i));
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_widget_show (menuitem);
    }

  return menu;
}

/*****/

static void
cpopup_update_saved_color (gint     n,
			   GimpRGB *color)
{
  gchar *str;

  cpopup_render_color_box (GTK_PREVIEW (g_editor->left_load_color_boxes[n + 3]),
			   color);
  cpopup_render_color_box (GTK_PREVIEW (g_editor->left_save_color_boxes[n]),
			   color);
  cpopup_render_color_box (GTK_PREVIEW (g_editor->right_load_color_boxes[n + 3]),
			   color);
  cpopup_render_color_box (GTK_PREVIEW (g_editor->right_save_color_boxes[n]),
			   color);

  str = g_strdup_printf (_("RGBA (%0.3f, %0.3f, %0.3f, %0.3f)"),
			 color->r,
			 color->g,
			 color->b,
			 color->a);

  gtk_label_set_text (GTK_LABEL (g_editor->left_load_labels[n + 3]), str);
  gtk_label_set_text (GTK_LABEL (g_editor->left_save_labels[n]), str);
  gtk_label_set_text (GTK_LABEL (g_editor->right_load_labels[n + 3]), str);
  gtk_label_set_text (GTK_LABEL (g_editor->right_save_labels[n]), str);

  g_free (str);

  g_editor->saved_colors[n] = *color;
}

/*****/

static void
cpopup_load_left_callback (GtkWidget *widget,
			   gpointer   data)
{
  GimpGradientSegment *seg;
  GimpRGB         fg;

  switch ((long) data)
    {
    case 0: /* Fetch from left neighbor's right endpoint */
      if (g_editor->control_sel_l->prev != NULL)
	seg = g_editor->control_sel_l->prev;
      else
	seg = gimp_gradient_segment_get_last (g_editor->control_sel_l);

      cpopup_blend_endpoints (&seg->right_color,
			      &g_editor->control_sel_r->right_color,
			      TRUE, TRUE);
      break;

    case 1: /* Fetch from right endpoint */
      cpopup_blend_endpoints (&g_editor->control_sel_r->right_color,
			      &g_editor->control_sel_r->right_color,
			      TRUE, TRUE);
      break;

    case 2: /* Fetch from FG color */
      gimp_context_get_foreground (gimp_context_get_user (), &fg);
      cpopup_blend_endpoints (&fg,
			      &g_editor->control_sel_r->right_color,
			      TRUE, TRUE);
      break;

    default: /* Load a color */
      cpopup_blend_endpoints (&g_editor->saved_colors[(long) data - 3],
			      &g_editor->control_sel_r->right_color,
			      TRUE, TRUE);
      break;
    }

  gimp_data_dirty (GIMP_DATA (gimp_context_get_gradient (g_editor->context)));

  ed_update_editor (GRAD_UPDATE_GRADIENT);
}

static void
cpopup_save_left_callback (GtkWidget *widget,
			   gpointer   data)
{
  g_editor->saved_colors[(long) data] = g_editor->control_sel_l->left_color;
}

static void
cpopup_load_right_callback (GtkWidget *widget,
			    gpointer   data)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg;
  GimpRGB              fg;

  gradient = gimp_context_get_gradient (g_editor->context);

  switch ((long) data)
    {
    case 0: /* Fetch from right neighbor's left endpoint */
      if (g_editor->control_sel_r->next != NULL)
	seg = g_editor->control_sel_r->next;
      else
	seg = gradient->segments;

      cpopup_blend_endpoints (&g_editor->control_sel_r->left_color,
			      &seg->left_color,
			      TRUE, TRUE);
      break;

    case 1: /* Fetch from left endpoint */
      cpopup_blend_endpoints (&g_editor->control_sel_l->left_color,
			      &g_editor->control_sel_l->left_color,
			      TRUE, TRUE);
      break;

    case 2: /* Fetch from FG color */
      gimp_context_get_foreground (gimp_context_get_user (), &fg);
      cpopup_blend_endpoints (&g_editor->control_sel_l->left_color,
			      &fg,
			      TRUE, TRUE);
      break;

    default: /* Load a color */
      cpopup_blend_endpoints (&g_editor->control_sel_l->left_color,
			      &g_editor->saved_colors[(long) data - 3],
			      TRUE, TRUE);
      break;
    }

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (GRAD_UPDATE_GRADIENT);
}

static void
cpopup_save_right_callback (GtkWidget *widget,
			    gpointer   data)
{
  g_editor->saved_colors[(long) data] = g_editor->control_sel_r->right_color;
}

/*****/

static GimpGradientSegment *
cpopup_save_selection (void)
{
  GimpGradientSegment *seg, *prev, *tmp;
  GimpGradientSegment *oseg, *oaseg;

  prev = NULL;
  oseg = g_editor->control_sel_l;
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
  while (oaseg != g_editor->control_sel_r);

  return tmp;
}

/*****/

static void
cpopup_replace_selection (GimpGradientSegment *replace_seg)
{
  GimpGradient        *gradient;
  GimpGradientSegment *lseg, *rseg;
  GimpGradientSegment *replace_last;

  gradient = gimp_context_get_gradient (g_editor->context);

  /* Remember left and right segments */

  lseg = g_editor->control_sel_l->prev;
  rseg = g_editor->control_sel_r->next;

  replace_last = gimp_gradient_segment_get_last (replace_seg);

  /* Free old selection */

  g_editor->control_sel_r->next = NULL;

  gimp_gradient_segments_free (g_editor->control_sel_l);

  /* Link in new segments */

  if (lseg)
    lseg->next = replace_seg;
  else
    gradient->segments = replace_seg;

  replace_seg->prev = lseg;

  if (rseg)
    rseg->prev = replace_last;

  replace_last->next = rseg;

  g_editor->control_sel_l = replace_seg;
  g_editor->control_sel_r = replace_last;

  gradient->last_visited = NULL; /* Force re-search */
}

/***** Color dialogs for left and right endpoint *****/

static void
cpopup_set_left_color_callback (GtkWidget *widget,
				gpointer   data)
{
  GimpGradient *gradient;

  gradient = gimp_context_get_gradient (g_editor->context);

  g_editor->left_saved_dirty    = GIMP_DATA (gradient)->dirty;
  g_editor->left_saved_segments = cpopup_save_selection ();

  color_notebook_new (_("Left Endpoint Color"),
		      &g_editor->control_sel_l->left_color,
		      (GtkSignalFunc) cpopup_left_color_changed,
		      NULL,
		      g_editor->instant_update,
		      TRUE);

  gtk_widget_set_sensitive (g_editor->shell, FALSE);
}

static void
cpopup_set_right_color_callback (GtkWidget *widget,
				 gpointer   data)
{
  GimpGradient *gradient;

  gradient = gimp_context_get_gradient (g_editor->context);

  g_editor->right_saved_dirty    = GIMP_DATA (gradient)->dirty;
  g_editor->right_saved_segments = cpopup_save_selection ();

  color_notebook_new (_("Right Endpoint Color"),
		      &g_editor->control_sel_l->right_color,
		      (GtkSignalFunc) cpopup_right_color_changed,
		      NULL,
		      g_editor->instant_update,
		      TRUE);

  gtk_widget_set_sensitive (g_editor->shell, FALSE);
}

static void
cpopup_left_color_changed (ColorNotebook      *cnb,
			   const GimpRGB      *color,
			   ColorNotebookState  state,
			   gpointer            data)
{
  GimpGradient *gradient;

  gradient = gimp_context_get_gradient (g_editor->context);

  switch (state)
    {
    case COLOR_NOTEBOOK_OK:
      cpopup_blend_endpoints ((GimpRGB *) color,
			      &g_editor->control_sel_r->right_color,
			      TRUE, TRUE);
      gimp_gradient_segments_free (g_editor->left_saved_segments);
      gimp_data_dirty (GIMP_DATA (gradient));
      color_notebook_free (cnb);
      gtk_widget_set_sensitive (g_editor->shell, TRUE);
      break;

    case COLOR_NOTEBOOK_UPDATE:
      cpopup_blend_endpoints ((GimpRGB *) color,
			      &g_editor->control_sel_r->right_color,
			      TRUE, TRUE);
      gimp_data_dirty (GIMP_DATA (gradient));
      break;

    case COLOR_NOTEBOOK_CANCEL:
      cpopup_replace_selection (g_editor->left_saved_segments);
      ed_update_editor (GRAD_UPDATE_GRADIENT);
      GIMP_DATA (gradient)->dirty = g_editor->left_saved_dirty;
      color_notebook_free (cnb);
      gtk_widget_set_sensitive (g_editor->shell, TRUE);
      break;
    }

  ed_update_editor (GRAD_UPDATE_GRADIENT);
}

static void
cpopup_right_color_changed (ColorNotebook      *cnb,
			    const GimpRGB      *color,
			    ColorNotebookState  state,
			    gpointer            data)
{
  GimpGradient *gradient;

  gradient = gimp_context_get_gradient (g_editor->context);

  switch (state)
    {
    case COLOR_NOTEBOOK_UPDATE:
      cpopup_blend_endpoints (&g_editor->control_sel_r->left_color,
			      (GimpRGB *) color,
			      TRUE, TRUE);
      gimp_data_dirty (GIMP_DATA (gradient));
      break;

    case COLOR_NOTEBOOK_OK:
      cpopup_blend_endpoints (&g_editor->control_sel_r->left_color,
			      (GimpRGB *) color,
			      TRUE, TRUE);
      gimp_gradient_segments_free (g_editor->right_saved_segments);
      gimp_data_dirty (GIMP_DATA (gradient));
      color_notebook_free (cnb);
      gtk_widget_set_sensitive (g_editor->shell, TRUE);
      break;

    case COLOR_NOTEBOOK_CANCEL:
      cpopup_replace_selection (g_editor->right_saved_segments);
      GIMP_DATA (gradient)->dirty = g_editor->right_saved_dirty;
      color_notebook_free (cnb);
      gtk_widget_set_sensitive (g_editor->shell, TRUE);
      break;
    }

  ed_update_editor (GRAD_UPDATE_GRADIENT);
}

/***** Blending menu *****/

static GtkWidget *
cpopup_create_blending_menu (void)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  GSList    *group;
  gint i;
  gint num_items;

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

  num_items = (sizeof (g_editor->control_blending_items) /
	       sizeof (g_editor->control_blending_items[0]));

  for (i = 0; i < num_items; i++)
    {
      if (i == (num_items - 1))
	menuitem = gtk_radio_menu_item_new_with_label(group, _("(Varies)"));
      else
	menuitem =
	  gtk_radio_menu_item_new_with_label (group,
					      gettext (blending_types[i]));

      group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));

      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  (GtkSignalFunc) cpopup_blending_callback,
			  (gpointer) ((long) i));

      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_widget_show (menuitem);

      g_editor->control_blending_items[i] = menuitem;
    }

  /* "Varies" is always disabled */
  gtk_widget_set_sensitive (g_editor->control_blending_items[num_items - 1], FALSE);

  return menu;
}

static void
cpopup_blending_callback (GtkWidget *widget,
			  gpointer   data)
{
  GimpGradient            *gradient;
  GimpGradientSegmentType  type;
  GimpGradientSegment     *seg, *aseg;

  gradient = gimp_context_get_gradient (g_editor->context);

  if (!GTK_CHECK_MENU_ITEM (widget)->active)
    return; /* Do nothing if the menu item is being deactivated */

  type = (GimpGradientSegmentType) data;
  seg  = g_editor->control_sel_l;

  do
    {
      seg->type = type;

      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != g_editor->control_sel_r);

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (GRAD_UPDATE_GRADIENT);
}

/***** Coloring menu *****/

static GtkWidget *
cpopup_create_coloring_menu (void)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  GSList    *group;
  gint       i;
  gint       num_items;

  static const gchar *coloring_types[] =
  {
    N_("Plain RGB"),
    N_("HSV (counter-clockwise hue)"),
    N_("HSV (clockwise hue)")
  };

  menu  = gtk_menu_new ();
  group = NULL;

  num_items = (sizeof (g_editor->control_coloring_items) /
	       sizeof (g_editor->control_coloring_items[0]));

  for (i = 0; i < num_items; i++)
    {
      if (i == (num_items - 1))
	menuitem = gtk_radio_menu_item_new_with_label(group, _("(Varies)"));
      else
	menuitem = gtk_radio_menu_item_new_with_label (group, gettext (coloring_types[i]));

      group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menuitem));

      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  (GtkSignalFunc) cpopup_coloring_callback,
			  (gpointer) ((long) i));

      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_widget_show (menuitem);

      g_editor->control_coloring_items[i] = menuitem;
    }

  /* "Varies" is always disabled */
  gtk_widget_set_sensitive (g_editor->control_coloring_items[num_items - 1], FALSE);

  return menu;
}

static void
cpopup_coloring_callback (GtkWidget *widget,
			  gpointer   data)
{
  GimpGradient             *gradient;
  GimpGradientSegmentColor  color;
  GimpGradientSegment      *seg, *aseg;

  gradient = gimp_context_get_gradient (g_editor->context);

  if (! GTK_CHECK_MENU_ITEM (widget)->active)
    return; /* Do nothing if the menu item is being deactivated */

  color = (GimpGradientSegmentColor) data;
  seg   = g_editor->control_sel_l;

  do
    {
      seg->color = color;

      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != g_editor->control_sel_r);

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (GRAD_UPDATE_GRADIENT);
}

/*****/

static void
cpopup_split_midpoint_callback (GtkWidget *widget,
				gpointer   data)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg, *lseg, *rseg;

  gradient = gimp_context_get_gradient (g_editor->context);

  seg = g_editor->control_sel_l;

  do
    {
      cpopup_split_midpoint (seg, &lseg, &rseg);
      seg = rseg->next;
    }
  while (lseg != g_editor->control_sel_r);

  g_editor->control_sel_r = rseg;

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
}

static void
cpopup_split_midpoint (GimpGradientSegment  *lseg,
		       GimpGradientSegment **newl,
		       GimpGradientSegment **newr)
{
  GimpRGB         color;
  GimpGradientSegment *newseg;

  /* Get color at original segment's midpoint */
  gimp_gradient_get_color_at (gimp_context_get_gradient (g_editor->context),
			      lseg->middle, &color);

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
cpopup_split_uniform_callback (GtkWidget *widget,
			       gpointer   data)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *scale;
  GtkObject *scale_data;

  /*  Create dialog window  */
  dialog =
    gimp_dialog_new ((g_editor->control_sel_l == g_editor->control_sel_r) ?
		     _("Split segment uniformly") :
		     _("Split segments uniformly"),
		     "gradient_segment_split_uniformly",
		     gimp_standard_help_func,
		     "dialogs/gradient_editor/split_segments_uniformly.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     _("Split"), cpopup_split_uniform_split_callback,
		     NULL, NULL, NULL, TRUE, FALSE,
		     _("Cancel"), cpopup_split_uniform_cancel_callback,
		     NULL, NULL, NULL, FALSE, TRUE,

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
    gtk_label_new ((g_editor->control_sel_l == g_editor->control_sel_r) ?
		   _("in which you want to split the selected segment") :
		   _("in which you want to split the segments in the selection"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  Scale  */
  g_editor->split_parts = 2;
  scale_data  = gtk_adjustment_new (2.0, 2.0, 21.0, 1.0, 1.0, 1.0);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 4);
  gtk_widget_show (scale);

  gtk_signal_connect (scale_data, "value_changed",
		      (GtkSignalFunc) cpopup_split_uniform_scale_update,
		      NULL);

  /*  Show!  */
  gtk_widget_show (dialog);
  gtk_widget_set_sensitive (g_editor->shell, FALSE);
}

static void
cpopup_split_uniform_scale_update (GtkAdjustment *adjustment,
				   gpointer       data)
{
  g_editor->split_parts = (gint) (adjustment->value + 0.5);
}

static void
cpopup_split_uniform_split_callback (GtkWidget *widget,
				     gpointer   data)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg, *aseg, *lseg, *rseg, *lsel;

  gradient = gimp_context_get_gradient (g_editor->context);

  gtk_widget_destroy (GTK_WIDGET (data));
  gtk_widget_set_sensitive (g_editor->shell, TRUE);

  seg  = g_editor->control_sel_l;
  lsel = NULL;

  do
    {
      aseg = seg;

      cpopup_split_uniform (seg, g_editor->split_parts, &lseg, &rseg);

      if (seg == g_editor->control_sel_l)
	lsel = lseg;

      seg = rseg->next;
    }
  while (aseg != g_editor->control_sel_r);

  g_editor->control_sel_l = lsel;
  g_editor->control_sel_r = rseg;

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
}

static void
cpopup_split_uniform_cancel_callback (GtkWidget *widget,
				      gpointer   data)
{
  gtk_widget_destroy (GTK_WIDGET (data));
  gtk_widget_set_sensitive (g_editor->shell, TRUE);
}

static void
cpopup_split_uniform (GimpGradientSegment  *lseg,
		      gint                  parts,
		      GimpGradientSegment **newl,
		      GimpGradientSegment **newr)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg, *prev, *tmp;
  gdouble              seg_len;
  gint                 i;

  gradient = gimp_context_get_gradient (g_editor->context);

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
cpopup_delete_callback (GtkWidget *widget,
			gpointer   data)
{
  GimpGradient        *gradient;
  GimpGradientSegment *lseg, *rseg, *seg, *aseg, *next;
  double               join;

  gradient = gimp_context_get_gradient (g_editor->context);

  /* Remember segments to the left and to the right of the selection */

  lseg = g_editor->control_sel_l->prev;
  rseg = g_editor->control_sel_r->next;

  /* Cannot delete all the segments in the gradient */

  if ((lseg == NULL) && (rseg == NULL))
    return;

  /* Calculate join point */

  join = (g_editor->control_sel_l->left + g_editor->control_sel_r->right) / 2.0;

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

  seg = g_editor->control_sel_l;

  do
    {
      next = seg->next;
      aseg = seg;

      gimp_gradient_segment_free (seg);

      seg = next;
    }
  while (aseg != g_editor->control_sel_r);

  /* Change selection */

  if (rseg)
    {
      g_editor->control_sel_l = rseg;
      g_editor->control_sel_r = rseg;
    }
  else
    {
      g_editor->control_sel_l = lseg;
      g_editor->control_sel_r = lseg;
    }

  if (lseg == NULL)
    gradient->segments = rseg;

  /* Done */

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
}

static void
cpopup_recenter_callback (GtkWidget *wiodget,
			  gpointer   data)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg, *aseg;

  gradient = gimp_context_get_gradient (g_editor->context);

  seg = g_editor->control_sel_l;

  do
    {
      seg->middle = (seg->left + seg->right) / 2.0;

      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != g_editor->control_sel_r);

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
}

static void
cpopup_redistribute_callback (GtkWidget *widget,
			      gpointer   data)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg, *aseg;
  gdouble              left, right, seg_len;
  gint                 num_segs;
  gint                 i;

  gradient = gimp_context_get_gradient (g_editor->context);

  /* Count number of segments in selection */

  num_segs = 0;
  seg      = g_editor->control_sel_l;

  do
    {
      num_segs++;
      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != g_editor->control_sel_r);

  /* Calculate new segment length */

  left    = g_editor->control_sel_l->left;
  right   = g_editor->control_sel_r->right;
  seg_len = (right - left) / num_segs;

  /* Redistribute */

  seg = g_editor->control_sel_l;

  for (i = 0; i < num_segs; i++)
    {
      seg->left   = left + i * seg_len;
      seg->right  = left + (i + 1) * seg_len;
      seg->middle = (seg->left + seg->right) / 2.0;

      seg = seg->next;
    }

  /* Fix endpoints to squish accumulative error */

  g_editor->control_sel_l->left  = left;
  g_editor->control_sel_r->right = right;

  /* Done */

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
}


/***** Control popup -> selection options functions *****/

static GtkWidget *
cpopup_create_sel_ops_menu (void)
{
  GtkWidget     *menu;
  GtkWidget     *menuitem;
  GtkAccelGroup *accel_group;

  menu = gtk_menu_new ();
  accel_group = g_editor->accel_group;

  gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);

  /* Flip */
  menuitem =
    cpopup_create_menu_item_with_label ("", &g_editor->control_flip_label);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      (GtkSignalFunc) cpopup_flip_callback,
		      NULL);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'F', 0,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  /* Replicate */
  menuitem =
    cpopup_create_menu_item_with_label ("", &g_editor->control_replicate_label);
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      (GtkSignalFunc) cpopup_replicate_callback,
		      NULL);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'M', 0,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  /* Blend colors / opacity */
  menuitem = gtk_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);

  menuitem = gtk_menu_item_new_with_label (_("Blend endpoints' colors"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      (GtkSignalFunc) cpopup_blend_colors,
		      NULL);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  g_editor->control_blend_colors_menu_item = menuitem;
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'B', 0,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

  menuitem = gtk_menu_item_new_with_label (_("Blend endpoints' opacity"));
  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
		      (GtkSignalFunc) cpopup_blend_opacity,
		      NULL);
  gtk_menu_append (GTK_MENU (menu), menuitem);
  gtk_widget_show (menuitem);
  gtk_widget_add_accelerator (menuitem, "activate",
			      accel_group,
			      'B', GDK_CONTROL_MASK,
			      GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);
  g_editor->control_blend_opacity_menu_item = menuitem;

  return menu;
}

static void
cpopup_flip_callback (GtkWidget *widget,
		      gpointer   data)
{
  GimpGradient        *gradient;
  GimpGradientSegment *oseg, *oaseg;
  GimpGradientSegment *seg, *prev, *tmp;
  GimpGradientSegment *lseg, *rseg;
  gdouble              left, right;

  gradient = gimp_context_get_gradient (g_editor->context);

  left  = g_editor->control_sel_l->left;
  right = g_editor->control_sel_r->right;

  /* Build flipped segments */

  prev = NULL;
  oseg = g_editor->control_sel_r;
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
  while (oaseg != g_editor->control_sel_l);

  seg->right = right; /* Squish accumulative error */

  /* Free old segments */

  lseg = g_editor->control_sel_l->prev;
  rseg = g_editor->control_sel_r->next;

  oseg = g_editor->control_sel_l;

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

  g_editor->control_sel_l = tmp;
  g_editor->control_sel_r = seg;

  /* Done */

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
}

static void
cpopup_replicate_callback (GtkWidget *widget,
			   gpointer   data)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *scale;
  GtkObject *scale_data;

  /*  Create dialog window  */
  dialog =
    gimp_dialog_new ((g_editor->control_sel_l == g_editor->control_sel_r) ?
		     _("Replicate segment") :
		     _("Replicate selection"),
		     "gradient_segment_replicate",
		     gimp_standard_help_func,
		     "dialogs/gradient_editor/replicate_segment.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     _("Replicate"), cpopup_do_replicate_callback,
		     NULL, NULL, NULL, FALSE, FALSE,
		     _("Cancel"), cpopup_replicate_cancel_callback,
		     NULL, NULL, NULL, TRUE, TRUE,

		     NULL);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
  gtk_widget_show (vbox);

  /*  Instructions  */
  label = gtk_label_new (_("Please select the number of times"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  label = gtk_label_new ((g_editor->control_sel_l == g_editor->control_sel_r) ?
			 _("you want to replicate the selected segment") :
			 _("you want to replicate the selection"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  Scale  */
  g_editor->replicate_times = 2;
  scale_data  = gtk_adjustment_new (2.0, 2.0, 21.0, 1.0, 1.0, 1.0);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, TRUE, 4);
  gtk_widget_show (scale);

  gtk_signal_connect (scale_data, "value_changed",
		      (GtkSignalFunc) cpopup_replicate_scale_update,
		      NULL);

  /*  Show!  */
  gtk_widget_show (dialog);
  gtk_widget_set_sensitive (g_editor->shell, FALSE);
}

static void
cpopup_replicate_scale_update (GtkAdjustment *adjustment,
			       gpointer       data)
{
  g_editor->replicate_times = (int) (adjustment->value + 0.5);
}

static void
cpopup_do_replicate_callback (GtkWidget *widget,
			      gpointer   data)
{
  GimpGradient        *gradient;
  gdouble              sel_left, sel_right, sel_len;
  gdouble              new_left;
  gdouble              factor;
  GimpGradientSegment *prev, *seg, *tmp;
  GimpGradientSegment *oseg, *oaseg;
  GimpGradientSegment *lseg, *rseg;
  gint                 i;

  gradient = gimp_context_get_gradient (g_editor->context);

  gtk_widget_destroy (GTK_WIDGET (data));
  gtk_widget_set_sensitive (g_editor->shell, TRUE);

  /* Remember original parameters */
  sel_left  = g_editor->control_sel_l->left;
  sel_right = g_editor->control_sel_r->right;
  sel_len   = sel_right - sel_left;

  factor = 1.0 / g_editor->replicate_times;

  /* Build replicated segments */

  prev = NULL;
  seg  = NULL;
  tmp  = NULL;

  for (i = 0; i < g_editor->replicate_times; i++)
    {
      /* Build one cycle */

      new_left  = sel_left + i * factor * sel_len;

      oseg = g_editor->control_sel_l;

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
      while (oaseg != g_editor->control_sel_r);
    }

  seg->right = sel_right; /* Squish accumulative error */

  /* Free old segments */

  lseg = g_editor->control_sel_l->prev;
  rseg = g_editor->control_sel_r->next;

  oseg = g_editor->control_sel_l;

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

  g_editor->control_sel_l = tmp;
  g_editor->control_sel_r = seg;

  /* Done */

  gimp_data_dirty (GIMP_DATA (gradient));

  ed_update_editor (GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
}

static void
cpopup_replicate_cancel_callback (GtkWidget *widget,
				  gpointer   data)
{
  gtk_widget_destroy (GTK_WIDGET (data));
  gtk_widget_set_sensitive (g_editor->shell, TRUE);
}

static void
cpopup_blend_colors (GtkWidget *widget,
		     gpointer   data)
{
  cpopup_blend_endpoints (&g_editor->control_sel_l->left_color,
			  &g_editor->control_sel_r->right_color,
			  TRUE, FALSE);

  gimp_data_dirty (GIMP_DATA (gimp_context_get_gradient (g_editor->context)));

  ed_update_editor (GRAD_UPDATE_GRADIENT);
}

static void
cpopup_blend_opacity (GtkWidget *widget,
		      gpointer   data)
{
  cpopup_blend_endpoints (&g_editor->control_sel_l->left_color,
			  &g_editor->control_sel_r->right_color,
			  FALSE, TRUE);

  gimp_data_dirty (GIMP_DATA (gimp_context_get_gradient (g_editor->context)));

  ed_update_editor (GRAD_UPDATE_GRADIENT);
}

/***** Main blend function *****/

static void
cpopup_blend_endpoints (GimpRGB  *rgb1,
			GimpRGB  *rgb2,
			gboolean  blend_colors,
			gboolean  blend_opacity)
{
  GimpRGB              d;
  gdouble              left, len;
  GimpGradientSegment *seg;
  GimpGradientSegment *aseg;

  d.r = rgb2->r - rgb1->r;
  d.g = rgb2->g - rgb1->g;
  d.b = rgb2->b - rgb1->b;
  d.a = rgb2->a - rgb1->a;

  left  = g_editor->control_sel_l->left;
  len   = g_editor->control_sel_r->right - left;

  seg = g_editor->control_sel_l;

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
  while (aseg != g_editor->control_sel_r);
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
