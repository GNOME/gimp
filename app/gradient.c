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


/* alt: Added previews and some other buttons to gradient dialog. 
 *
 * hof: Hofer Wolfgang, 1998.01.27   avoid resize bug by keeping 
 *                                   preview widgetsize constant.
 * hof: Hofer Wolfgang, 1997.10.17   Added "Load From FG color"
 */

/* Release date: 1997/06/02
 *
 * - Added the following procedural_db calls:
 *     gimp_gradients_get_list
 *     gimp_gradients_get_active
 *     gimp_gradients_set_active
 *     gimp_gradients_sample_uniform
 *     gimp_gradients_sample_custom
 *   Many thanks to Eiichi and Marcelo for their suggestions!
 */


/* Release date: 1997/05/07
 *
 * - Added accelerator keys for the popup functions.  This allows for
 * very fast operation of the editor.
 *
 * - Added Replicate Selection function.
 *
 * - Re-arranged the pop-up menu a bit; it was getting too big.  I am
 * still not entirely happy with it.
 *
 * - Added grad_dump_gradient(); it is useful for debugging.
 */


/* Release date: 1997/04/30
 *
 * - All `dangerous' dialogs now de-sensitize the main editor window.
 * This fixes a lot of potential bugs when the dialogs are active.
 *
 * - Fixed two bugs due to uninitialized variables, one in
 * prev_events() (thanks to Marcelo for pointing it out) and another
 * in cpopup_render_color_box() (me), and removed all warnings due to
 * those.
 *
 * - Removed the printf()'s in the pop-up menu (they were only used
 * for debugging).
 */


/* Release date: 1997/04/22
 *
 * - Added GtkRadioMenuItems to the blending and coloring pop-up
 * menus.  You no longer have to remember the dang type for each
 * segment.
 *
 * - Added midpoint capabilities to sinuosidal and spherical sigments.
 * Many thanks to Marcelo for the patches!
 *
 * - Added a *real* Cancel function to the color pickers.  I don't
 * know why nobody killed me for not having done it before.
 */


/* Release date: 1997/04/21
 *
 * - Re-wrote the old pop-up menu code, which was *horrible*.  The
 * memory leaks *should* go away once I write
 * grad_free_gradient_editor().  I'll do it once I'm finished adding
 * crap to gradient_editor_t.
 *
 * - Added "fetch from" color buttons.  Yeah, we all missed them.  You
 * should shed happiness tears when you see it.
 *
 * - Added an eyedropper function to the preview widget.  You can now
 * click on the preview widget and the foreground color will be set to
 * the gradient's color under the cursor.  This is still missing the
 * eyedropper cursor shape.
 *
 * - You can now invoke the pop-up menu from the preview widget.  Even
 * my hand gets unsteady at times.
 *
 * - Cool functions: Flip selection, Blend colors.  Can't live without
 * them.
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
 * - Add Save Dirty Gradients function.
 *
 * - Add Reload function.
 *
 * - See that the default-gradient really gets selected on startup.
 *
 * - Provide a way of renaming a gradient (instead of having to do
 *   copy/delete).
 *
 * - Fix the flicker in the hint bar.
 *
 * - Maybe add a little preview of each gradient to the listbox?
 *
 * - Better handling of bogus gradient files and inconsistent
 *   segments.  Do not loop indefinitely in seg_get_segment_at() if
 *   there is a missing segment between two others.
 *
 * - Add a Gradient brush mode (color changes as you move it).
 */

#include "config.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "appenv.h"
#include "colormaps.h"
#include "cursorutil.h"
#include "datafiles.h"
#include "errors.h"
#include "general.h"
#include "gimprc.h"
#include "gradient.h"
#include "interface.h"
#include "palette.h"
#include "session.h"
#include "gradient_header.h"

#include "libgimp/gimpintl.h"

/***** Magic numbers *****/

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif /* M_PI */

#define EPSILON 1e-10


/***** Local functions *****/

/* Gradient editor functions */
static void       ed_fetch_foreground(double *fg_r, double *fg_g, double *fg_b, double *fg_a);
static void       ed_update_editor(int flags);

static GtkWidget *ed_create_button(gchar *label, double xalign, double yalign,
				   GtkSignalFunc signal_func, gpointer user_data);

static void       ed_set_hint(char *str);


static void       ed_list_item_update(GtkWidget *widget, 
				      gint row,
				      gint column,
				      GdkEventButton *event,
				      gpointer data);

static void       ed_initialize_saved_colors(void);

static gint       ed_close_callback(GtkWidget *widget, gpointer client_data);
static void       ed_refresh_callback(GtkWidget *widget, gpointer client_data);
static void       ed_new_gradient_callback(GtkWidget *widget, gpointer client_data);
static void       ed_do_rename_gradient_callback(GtkWidget *widget, gpointer client_data, gpointer call_data);
static void       ed_do_new_gradient_callback(GtkWidget *widget, gpointer client_data, gpointer call_data);
static void       ed_copy_gradient_callback(GtkWidget *widget, gpointer client_data);
static void       ed_do_copy_gradient_callback(GtkWidget *widget, gpointer client_data, gpointer call_data);
static void       ed_delete_gradient_callback(GtkWidget *widget, gpointer client_data);
static void       ed_do_delete_gradient_callback(GtkWidget *widget, gpointer client_data);
static void       ed_cancel_delete_gradient_callback(GtkWidget *widget, gpointer client_data);
static void       ed_rename_grads_callback(GtkWidget *widget, gpointer client_data);
static void       ed_save_grads_callback(GtkWidget *widget, gpointer client_data);
static void       ed_save_pov_callback(GtkWidget *widget, gpointer client_data);
static void       ed_do_save_pov_callback(GtkWidget *widget, gpointer client_data);
static void       ed_cancel_save_pov_callback(GtkWidget *widget, gpointer client_data);
static void       ed_scrollbar_update(GtkAdjustment *adjustment, gpointer data);
static void       ed_zoom_all_callback(GtkWidget *widget, gpointer client_data);
static void       ed_zoom_out_callback(GtkWidget *widget, gpointer client_data);
static void       ed_zoom_in_callback(GtkWidget *widget, gpointer client_data);
static void       ed_instant_update_update(GtkWidget *widget, gpointer data);

/* Gradient preview functions */

static gint prev_events(GtkWidget *widget, GdkEvent *event);
static void prev_set_hint(gint x);
static void prev_set_foreground(gint x);
static void prev_update(int recalculate);
static void prev_fill_image(int width, int height, double left, double right);

/* Gradient control functions */

static gint   control_events(GtkWidget *widget, GdkEvent *event);
static void   control_do_hint(gint x, gint y);
static void   control_button_press(gint x, gint y, guint button, guint state);
static int    control_point_in_handle(gint x, gint y, grad_segment_t *seg, control_drag_mode_t handle);
static void   control_select_single_segment(grad_segment_t *seg);
static void   control_extend_selection(grad_segment_t *seg, double pos);
static void   control_motion(gint x);

static void   control_compress_left(grad_segment_t *range_l, grad_segment_t *range_r,
				    grad_segment_t *drag_seg, double pos);
static void   control_compress_range(grad_segment_t *range_l, grad_segment_t *range_r,
				     double new_l, double new_r);

static double control_move(grad_segment_t *range_l, grad_segment_t *range_r, double delta);

static void   control_update(int recalculate);
static void   control_draw(GdkPixmap *pixmap, int width, int height, double left, double right);
static void   control_draw_normal_handle(GdkPixmap *pixmap, double pos, int height);
static void   control_draw_middle_handle(GdkPixmap *pixmap, double pos, int height);
static void   control_draw_handle(GdkPixmap *pixmap, GdkGC *border_gc, GdkGC *fill_gc, int xpos, int height);
static int    control_calc_p_pos(double pos);
static double control_calc_g_pos(int pos);

/* Control popup functions */

static void       cpopup_create_main_menu(void);
static void       cpopup_do_popup(void);
static GtkWidget *cpopup_create_color_item(GtkWidget **color_box, GtkWidget **label);
static void       cpopup_adjust_menus(void);
static void       cpopup_adjust_blending_menu(void);
static void       cpopup_adjust_coloring_menu(void);
static void       cpopup_check_selection_params(int *equal_blending, int *equal_coloring);
static GtkWidget *cpopup_create_menu_item_with_label(char *str, GtkWidget **label);
static void       cpopup_render_color_box(GtkPreview *preview, double r, double g, double b, double a);

static GtkWidget *cpopup_create_load_menu(GtkWidget **color_boxes, GtkWidget **labels,
					  char *label1, char *label2, GtkSignalFunc callback,
					  gchar accel_key_0, guint8 accel_mods_0,
					  gchar accel_key_1, guint8 accel_mods_1,
					  gchar accel_key_2, guint8 accel_mods_2);
static GtkWidget *cpopup_create_save_menu(GtkWidget **color_boxes, GtkWidget **labels, GtkSignalFunc callback);
static void       cpopup_update_saved_color(int n, double r, double g, double b, double a);
static void       cpopup_load_left_callback(GtkWidget *widget, gpointer data);
static void       cpopup_save_left_callback(GtkWidget *widget, gpointer data);
static void       cpopup_load_right_callback(GtkWidget *widget, gpointer data);
static void       cpopup_save_right_callback(GtkWidget *widget, gpointer data);

static GtkWidget *cpopup_create_blending_menu(void);
static void       cpopup_blending_callback(GtkWidget *widget, gpointer data);
static GtkWidget *cpopup_create_coloring_menu(void);
static void       cpopup_coloring_callback(GtkWidget *widget, gpointer data);

static GtkWidget *cpopup_create_sel_ops_menu(void);

static void       cpopup_blend_colors(GtkWidget *widget, gpointer data);
static void       cpopup_blend_opacity(GtkWidget *widget, gpointer data);

static void       cpopup_set_color_selection_color(GtkColorSelection *cs,
						   double r, double g, double b, double a);
static void       cpopup_get_color_selection_color(GtkColorSelection *cs,
						   double *r, double *g, double *b, double *a);

static void       cpopup_create_color_dialog(char *title, double r, double g, double b, double a,
					     GtkSignalFunc color_changed_callback,
					     GtkSignalFunc ok_callback,
					     GtkSignalFunc cancel_callback,
					     GtkSignalFunc delete_callback);

static grad_segment_t *cpopup_save_selection(void);
static void            cpopup_free_selection(grad_segment_t *seg);
static void            cpopup_replace_selection(grad_segment_t *replace_seg);

static void       cpopup_set_left_color_callback(GtkWidget *widget, gpointer data);
static void       cpopup_left_color_changed(GtkWidget *widget, gpointer client_data);
static void       cpopup_left_color_dialog_ok(GtkWidget *widget, gpointer client_data);
static void       cpopup_left_color_dialog_cancel(GtkWidget *widget, gpointer client_data);
static int        cpopup_left_color_dialog_delete(GtkWidget *widget, GdkEvent *event, gpointer data);
static void       cpopup_set_right_color_callback(GtkWidget *widget, gpointer data);
static void       cpopup_right_color_changed(GtkWidget *widget, gpointer client_data);
static void       cpopup_right_color_dialog_ok(GtkWidget *widget, gpointer client_data);
static void       cpopup_right_color_dialog_cancel(GtkWidget *widget, gpointer client_data);
static int        cpopup_right_color_dialog_delete(GtkWidget *widget, GdkEvent *event, gpointer data);

static void       cpopup_split_midpoint_callback(GtkWidget *widget, gpointer data);
static void       cpopup_split_uniform_callback(GtkWidget *widget, gpointer data);
static void       cpopup_delete_callback(GtkWidget *widget, gpointer data);
static void       cpopup_recenter_callback(GtkWidget *widget, gpointer data);
static void       cpopup_redistribute_callback(GtkWidget *widget, gpointer data);
static void       cpopup_flip_callback(GtkWidget *widget, gpointer data);
static void       cpopup_replicate_callback(GtkWidget *widget, gpointer data);

static void       cpopup_split_uniform_scale_update(GtkAdjustment *adjustment, gpointer data);
static void       cpopup_split_uniform_split_callback(GtkWidget *widget, gpointer client_data);
static void       cpopup_split_uniform_cancel_callback(GtkWidget *widget, gpointer client_data);

static void       cpopup_replicate_scale_update(GtkAdjustment *adjustment, gpointer data);
static void       cpopup_do_replicate_callback(GtkWidget *widget, gpointer client_data);
static void       cpopup_replicate_cancel_callback(GtkWidget *widget, gpointer client_data);

static void       cpopup_blend_endpoints(double r0, double g0, double b0, double a0,
					 double r1, double g1, double b1, double a1,
					 int blend_colors, int blend_opacity);
static void 	  cpopup_split_midpoint(grad_segment_t *lseg, grad_segment_t **newl, grad_segment_t **newr);
static void 	  cpopup_split_uniform(grad_segment_t *lseg, int parts,
				       grad_segment_t **newl, grad_segment_t **newr);

/* Gradient functions */

static gradient_t *grad_new_gradient(void);
static void        grad_free_gradient(gradient_t *grad);
static void        grad_free_gradients(void);
static void        grad_load_gradient(char *filename);
static void        grad_save_gradient(gradient_t *grad, char *filename);
static void        grad_save_all(int need_free);

static gradient_t *grad_create_default_gradient(void);

static int         grad_insert_in_gradients_list(gradient_t *grad);

static void        grad_dump_gradient(gradient_t *grad, FILE *file);


/* Segment functions */

static grad_segment_t *seg_new_segment(void);
static void            seg_free_segment(grad_segment_t *seg);
static void            seg_free_segments(grad_segment_t *seg);

static grad_segment_t *seg_get_segment_at(gradient_t *grad, double pos);
static grad_segment_t *seg_get_last_segment(grad_segment_t *seg);
static void            seg_get_closest_handle(gradient_t *grad, double pos,
					      grad_segment_t **seg, control_drag_mode_t *handle);

/* Calculation functions */

static double calc_linear_factor(double middle, double pos);
static double calc_curved_factor(double middle, double pos);
static double calc_sine_factor(double middle, double pos);
static double calc_sphere_increasing_factor(double middle, double pos);
static double calc_sphere_decreasing_factor(double middle, double pos);

static void calc_rgb_to_hsv(double *r, double *g, double *b);
static void calc_hsv_to_rgb(double *h, double *s, double *v);

/* Files and paths functions */

static char *build_user_filename(char *name, char *path_str);

/* Procedural database functions */

static Argument *gradients_get_list_invoker(Argument *args);
static Argument *gradients_get_active_invoker(Argument *args);
static Argument *gradients_set_active_invoker(Argument *args);
static Argument *gradients_sample_uniform_invoker(Argument *args);
static Argument *gradients_sample_custom_invoker(Argument *args);


/***** Local variables *****/

GdkColor black;
static int         num_gradients         = 0;
gradient_t *curr_gradient         = NULL; /* The active gradient */
GSList     *gradients_list        = NULL; /* The list of gradients */
gradient_t *grad_default_gradient = NULL;
gradient_editor_t *g_editor       = NULL; /* The gradient editor */

static const char *blending_types[] = {
	N_("Linear"),
	N_("Curved"),
	N_("Sinusoidal"),
	N_("Spherical (increasing)"),
	N_("Spherical (decreasing)")
}; /* blending_types */

static const char *coloring_types[] = {
	N_("Plain RGB"),
	N_("HSV (counter-clockwise hue)"),
	N_("HSV (clockwise hue)")
}; /* coloring_types */


/***** Public functions *****/

/*****/

void
gradients_init(int no_data)
{
  if(!no_data)
    datafiles_read_directories(gradient_path, grad_load_gradient, 0);


  if (grad_default_gradient != NULL)
    curr_gradient = grad_default_gradient;
  else if (gradients_list != NULL)
    curr_gradient = (gradient_t *) gradients_list->data;
  else {
		curr_gradient = grad_create_default_gradient();
		curr_gradient->name     = g_strdup(_("Default"));
		curr_gradient->filename = build_user_filename(curr_gradient->name, gradient_path);
		curr_gradient->dirty    = FALSE;

		grad_insert_in_gradients_list(curr_gradient);
	} /* else */
} /* gradients_init */


/*****/

void
gradients_free(void)
{
	grad_free_gradients();
} /* gradients_free */


/*****/

void
grad_get_color_at(double pos, double *r, double *g, double *b, double *a)
{
	double          factor;
	grad_segment_t *seg;
	double          seg_len, middle;
	double          h0, s0, v0;
	double          h1, s1, v1;

	/* if there is no gradient return a totally transparent black */
	if (curr_gradient == NULL) 
	  {
	    r = 0; g = 0; b = 0; a = 0;
	    return;
	  }

	if (pos < 0.0)
		pos = 0.0;
	else if (pos > 1.0)
		pos = 1.0;

	seg = seg_get_segment_at(curr_gradient, pos);

	seg_len = seg->right - seg->left;

	if (seg_len < EPSILON) {
		middle = 0.5;
		pos    = 0.5;
	} else {
		middle = (seg->middle - seg->left) / seg_len;
		pos    = (pos - seg->left) / seg_len;
	} /* else */

	switch (seg->type) {
		case GRAD_LINEAR:
			factor = calc_linear_factor(middle, pos);
			break;

		case GRAD_CURVED:
			factor = calc_curved_factor(middle, pos);
			break;

		case GRAD_SINE:
			factor = calc_sine_factor(middle, pos);
			break;

		case GRAD_SPHERE_INCREASING:
			factor = calc_sphere_increasing_factor(middle, pos);
			break;

		case GRAD_SPHERE_DECREASING:
			factor = calc_sphere_decreasing_factor(middle, pos);
			break;

		default:
			grad_dump_gradient(curr_gradient, stderr);
			fatal_error(_("grad_get_color_at(): aieee, unknown gradient type %d"), (int) seg->type);
			factor = 0.0; /* Shut up -Wall */
			break;
	} /* switch */

	/* Calculate color components */

	*a = seg->a0 + (seg->a1 - seg->a0) * factor;

	if (seg->color == GRAD_RGB) {
		*r = seg->r0 + (seg->r1 - seg->r0) * factor;
		*g = seg->g0 + (seg->g1 - seg->g0) * factor;
		*b = seg->b0 + (seg->b1 - seg->b0) * factor;
	} else {
		h0 = seg->r0;
		s0 = seg->g0;
		v0 = seg->b0;

		h1 = seg->r1;
		s1 = seg->g1;
		v1 = seg->b1;

		calc_rgb_to_hsv(&h0, &s0, &v0);
		calc_rgb_to_hsv(&h1, &s1, &v1);

		s0 = s0 + (s1 - s0) * factor;
		v0 = v0 + (v1 - v0) * factor;

		switch (seg->color) {
			case GRAD_HSV_CCW:
				if (h0 < h1)
					h0 = h0 + (h1 - h0) * factor;
				else {
					h0 = h0 + (1.0 - (h0 - h1)) * factor;
					if (h0 > 1.0)
						h0 -= 1.0;
				} /* else */

				break;

			case GRAD_HSV_CW:
				if (h1 < h0)
					h0 = h0 - (h0 - h1) * factor;
				else {
					h0 = h0 - (1.0 - (h1 - h0)) * factor;
					if (h0 < 0.0)
						h0 += 1.0;
				} /* else */

				break;

			default:
				grad_dump_gradient(curr_gradient, stderr);
				fatal_error(_("grad_get_color_at(): aieee, unknown coloring mode %d"),
					    (int) seg->color);
				break;
		} /* switch */

		*r = h0;
		*g = s0;
		*b = v0;

		calc_hsv_to_rgb(r, g, b);
	} /* else */
} /* grad_get_color_at */


/*****/


void 
grad_create_gradient_editor_init(gint need_show)
{
	GtkWidget *topvbox;
	GtkWidget *vbox;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *gvbox;
	GtkWidget *button;
	GtkWidget *frame;
	GtkWidget *separator;
	GtkWidget *scrolled_win;
	GdkColormap *colormap;
	int        i;
	int select_pos;

	/* If the editor already exists, just show it */

	if (g_editor) {
		if (!GTK_WIDGET_VISIBLE(g_editor->shell))
		  {
			gtk_widget_show(g_editor->shell);
			/* Do this because os a proble with scrolling
			 * clists when not visible on screen.
			 */
			grad_set_grad_to_name(curr_gradient->name);
		  }
		else
			gdk_window_raise(g_editor->shell->window);

		return;
	} /* if */

	/* Create editor */

	if(no_data)
	  gradients_init(FALSE);
	g_editor = g_malloc(sizeof(gradient_editor_t));

	/* Shell and main vbox */

	g_editor->shell = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_wmclass (GTK_WINDOW(g_editor->shell), "gradient_editor", "Gimp");
	gtk_container_border_width(GTK_CONTAINER(g_editor->shell), 0);
	gtk_window_set_title(GTK_WINDOW(g_editor->shell), _("Gradient Editor"));
	
	/* handle window manager close signals */
	gtk_signal_connect (GTK_OBJECT (g_editor->shell), "delete_event",
			    GTK_SIGNAL_FUNC (ed_close_callback),
			    NULL);

	topvbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(topvbox), 0);
	gtk_container_add(GTK_CONTAINER(g_editor->shell), topvbox);
	gtk_widget_show(topvbox);

	/* Hint bar and close button */

	table = gtk_table_new(2, 2, FALSE);
	gtk_container_border_width(GTK_CONTAINER(table), 0);
	gtk_box_pack_end(GTK_BOX(topvbox), table, FALSE, FALSE, 0);
	gtk_widget_show(table);

	separator = gtk_hseparator_new();
	gtk_table_attach(GTK_TABLE(table), separator, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_widget_show(separator);

	g_editor->hint_label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(g_editor->hint_label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), g_editor->hint_label, 0, 1, 1, 2,
			 GTK_EXPAND | GTK_FILL, GTK_FILL, 8, 2);
	gtk_widget_show(g_editor->hint_label);

	button = ed_create_button(_("Close"), 0.5, 0.5, (GtkSignalFunc) ed_close_callback, NULL);
	gtk_widget_set_usize(button, GRAD_CLOSE_BUTTON_WIDTH, 0);
	gtk_table_attach(GTK_TABLE(table), button, 1, 2, 0, 2, GTK_FILL, GTK_EXPAND | GTK_FILL, 8, 0);
	gtk_widget_show(button);

	/* Vbox for everything else */

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 8);
	gtk_box_pack_start(GTK_BOX(topvbox), vbox, TRUE, TRUE, 0);
	gtk_widget_show(vbox);

	/* Gradients list box */

	label = gtk_label_new(_("Gradients: "));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	hbox = gtk_hbox_new(FALSE, 8);
	gtk_container_border_width(GTK_CONTAINER(hbox), 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show(hbox);

	/* clist preview of gradients */

	scrolled_win = gtk_scrolled_window_new (NULL, NULL);

	g_editor->clist = gtk_clist_new(2);
	gtk_clist_set_shadow_type(GTK_CLIST(g_editor->clist), GTK_SHADOW_IN);

	gtk_clist_set_row_height(GTK_CLIST(g_editor->clist), 18);

	gtk_clist_set_column_width(GTK_CLIST(g_editor->clist), 0, 52);
	gtk_clist_set_column_title(GTK_CLIST(g_editor->clist), 0, _("Gradient"));
	gtk_clist_set_column_title(GTK_CLIST(g_editor->clist), 1, _("Name"));
	gtk_clist_column_titles_show(GTK_CLIST(g_editor->clist));

 	gtk_box_pack_start(GTK_BOX(hbox), scrolled_win, TRUE, TRUE, 0); 
	gtk_container_add (GTK_CONTAINER (scrolled_win), g_editor->clist);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_ALWAYS);

	gtk_widget_show(scrolled_win);
 	gtk_widget_show(g_editor->clist);

	colormap = gtk_widget_get_colormap(g_editor->clist);
	gdk_color_parse("black", &black);
	gdk_color_alloc(colormap, &black);
	
	gtk_signal_connect(GTK_OBJECT(g_editor->clist), "select_row",
			   GTK_SIGNAL_FUNC(ed_list_item_update),
			   (gpointer) NULL);

	/* Buttons for gradient functions */

	gvbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(gvbox), 0);
	gtk_box_pack_end(GTK_BOX(hbox), gvbox, FALSE, FALSE, 0);
	gtk_widget_show(gvbox);

	/* Buttons for gradient functions */

	button = ed_create_button(_("New gradient"), 0.0, 0.5,
				  (GtkSignalFunc) ed_new_gradient_callback, NULL);
	gtk_box_pack_start(GTK_BOX(gvbox), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	button = ed_create_button(_("Copy gradient"), 0.0, 0.5,
				  (GtkSignalFunc) ed_copy_gradient_callback, NULL);
	gtk_box_pack_start(GTK_BOX(gvbox), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	button = ed_create_button(_("Delete gradient"), 0.0, 0.5,
				  (GtkSignalFunc) ed_delete_gradient_callback, NULL);
	gtk_box_pack_start(GTK_BOX(gvbox), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	button = ed_create_button(_("Save as POV-Ray"), 0.0, 0.5,
				  (GtkSignalFunc) ed_save_pov_callback, NULL);
	gtk_box_pack_start(GTK_BOX(gvbox), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	button = ed_create_button(_("Save Gradients"), 0.0, 0.5,
				  (GtkSignalFunc) ed_save_grads_callback, NULL);
	gtk_box_pack_start(GTK_BOX(gvbox), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	button = ed_create_button(_("Rename Gradient"), 0.0, 0.5,
				  (GtkSignalFunc) ed_rename_grads_callback, NULL);
	gtk_box_pack_start(GTK_BOX(gvbox), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	button = ed_create_button(_("Refresh gradients"), 0.0, 0.5,
				  (GtkSignalFunc) ed_refresh_callback, NULL);
	gtk_box_pack_start(GTK_BOX(gvbox), button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	/* Horizontal box for zoom controls, scrollbar, and instant update toggle */

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(hbox), 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
	gtk_widget_show(hbox);

	/* Zoom buttons */

	button = ed_create_button(_("Zoom all"), 0.5, 0.5, (GtkSignalFunc) ed_zoom_all_callback, g_editor);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	button = ed_create_button(_("Zoom -"), 0.5, 0.5, (GtkSignalFunc) ed_zoom_out_callback, g_editor);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	button = ed_create_button(_("Zoom +"), 0.5, 0.5, (GtkSignalFunc) ed_zoom_in_callback, g_editor);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_widget_show(button);

	/* Scrollbar */

	g_editor->zoom_factor = 1;

	g_editor->scroll_data = gtk_adjustment_new(0.0, 0.0, 1.0,
						   1.0 * GRAD_SCROLLBAR_STEP_SIZE,
						   1.0 * GRAD_SCROLLBAR_PAGE_SIZE,
						   1.0);

	gtk_signal_connect(g_editor->scroll_data, "value_changed",
			   (GtkSignalFunc) ed_scrollbar_update,
			   g_editor);
	gtk_signal_connect(g_editor->scroll_data, "changed",
			   (GtkSignalFunc) ed_scrollbar_update,
			   g_editor);

	g_editor->scrollbar = gtk_hscrollbar_new(GTK_ADJUSTMENT(g_editor->scroll_data));
	gtk_range_set_update_policy(GTK_RANGE(g_editor->scrollbar), GTK_UPDATE_CONTINUOUS);
	gtk_box_pack_start(GTK_BOX(hbox), g_editor->scrollbar, TRUE, TRUE, 4);
	gtk_widget_show(g_editor->scrollbar);

	/* Instant update toggle */

	g_editor->instant_update = 1;

	button = gtk_check_button_new_with_label(_("Instant update"));
	gtk_box_pack_end(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(button), "toggled",
			   (GtkSignalFunc) ed_instant_update_update,
			   g_editor);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
	gtk_widget_show(button);

	/* hbox for that holds the frame for gradient preview and gradient control; 
           this is only here, because resizing the preview doesn't work (and is disabled) 
           to keep the preview and controls together */
	
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	/* Frame for gradient preview and gradient control */

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	gvbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frame), gvbox); 
	gtk_widget_show(gvbox);

	/* Gradient preview */

	g_editor->preview_rows[0]     = NULL;
	g_editor->preview_rows[1]     = NULL;
	g_editor->preview_last_x      = 0;
	g_editor->preview_button_down = 0;

	g_editor->preview = gtk_preview_new(GTK_PREVIEW_COLOR);
	gtk_preview_set_dither (GTK_PREVIEW (g_editor->preview),
				GDK_RGB_DITHER_MAX);
	gtk_preview_size(GTK_PREVIEW(g_editor->preview),
			 GRAD_PREVIEW_WIDTH, GRAD_PREVIEW_HEIGHT);
	gtk_widget_set_events(g_editor->preview, GRAD_PREVIEW_EVENT_MASK);
	gtk_signal_connect(GTK_OBJECT(g_editor->preview), "event",
			   (GtkSignalFunc) prev_events,
			   g_editor);
	gtk_box_pack_start(GTK_BOX(gvbox), g_editor->preview, TRUE, TRUE, 0);
	gtk_widget_show(g_editor->preview);

	/* Gradient control */

	g_editor->control_pixmap                  = NULL;
	g_editor->control_drag_segment            = NULL;
	g_editor->control_sel_l                   = NULL;
	g_editor->control_sel_r                   = NULL;
	g_editor->control_drag_mode               = GRAD_DRAG_NONE;
	g_editor->control_click_time              = 0;
	g_editor->control_compress                = 0;
	g_editor->control_last_x                  = 0;
	g_editor->control_last_gx                 = 0.0;
	g_editor->control_orig_pos                = 0.0;
	g_editor->control_main_popup              = NULL;
	g_editor->control_blending_label          = NULL;
	g_editor->control_coloring_label          = NULL;
	g_editor->control_splitm_label            = NULL;
	g_editor->control_splitu_label            = NULL;
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
	     i < (sizeof(g_editor->control_blending_items) / sizeof(g_editor->control_blending_items[0]));
	     i++)
		g_editor->control_blending_items[i] = NULL;

	for (i = 0;
	     i < (sizeof(g_editor->control_coloring_items) / sizeof(g_editor->control_coloring_items[0]));
	     i++)
		g_editor->control_coloring_items[i] = NULL;

	g_editor->control = gtk_drawing_area_new();
	gtk_drawing_area_size(GTK_DRAWING_AREA(g_editor->control),
			      GRAD_PREVIEW_WIDTH, GRAD_CONTROL_HEIGHT);
	gtk_widget_set_events(g_editor->control, GRAD_CONTROL_EVENT_MASK);
	gtk_signal_connect(GTK_OBJECT(g_editor->control), "event",
			   (GtkSignalFunc) control_events,
			   g_editor);
	gtk_box_pack_start(GTK_BOX(gvbox), g_editor->control, FALSE, TRUE, 0);
	gtk_widget_show(g_editor->control);

	/* Initialize other data */

	g_editor->left_color_preview            = NULL;
	g_editor->left_saved_segments           = NULL;
	g_editor->left_saved_dirty              = 0;

	g_editor->right_color_preview           = NULL;
	g_editor->right_saved_segments          = NULL;
	g_editor->right_saved_dirty             = 0;

	ed_initialize_saved_colors();
	cpopup_create_main_menu();

	/* Show everything */
	gtk_widget_realize(g_editor->shell);
	g_editor->gc = gdk_gc_new(g_editor->shell->window);
	select_pos = ed_set_list_of_gradients(g_editor->gc,
					      g_editor->clist,
					      curr_gradient);

	if(need_show)
	  gtk_widget_show(g_editor->shell);

	if(select_pos != -1)
	  gtk_clist_moveto(GTK_CLIST(g_editor->clist),select_pos,0,0.0,0.0);

} /* grad_create_gradient_editor */


/***** Gradient editor functions *****/

/*****/

static void
ed_fetch_foreground(double *fg_r, double *fg_g, double *fg_b, double *fg_a)
{
	unsigned char r, g, b;
	
 	palette_get_foreground (&r, &g, &b);
 	
 	*fg_r = (double) r / 255.0;
 	*fg_g = (double) g / 255.0;
 	*fg_b = (double) b / 255.0;
	*fg_a = 1.0;                 /* opacity 100 % */
} /* ed_fetch_foreground */


/*****/

static void
ed_update_editor(int flags)
{
	if (flags & GRAD_UPDATE_PREVIEW)
		prev_update(1);

	if (flags & GRAD_UPDATE_CONTROL)
		control_update(0);

	if (flags & GRAD_RESET_CONTROL)
		control_update(1);
} /* ed_update_editor */


/*****/

static GtkWidget *
ed_create_button(gchar *label, double xalign, double yalign, GtkSignalFunc signal_func, gpointer user_data)
{
	GtkWidget *button;
	GtkWidget *text;

	button = gtk_button_new();
	text   = gtk_label_new(gettext(label));

	gtk_misc_set_alignment(GTK_MISC(text), xalign, yalign);
	gtk_container_add(GTK_CONTAINER(button), text);
	gtk_widget_show(text);

	if (signal_func != NULL)
		gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC (signal_func), user_data);

	return button;
} /* ed_create_button */


/*****/

static void
ed_set_hint(char *str)
{
	gtk_label_set(GTK_LABEL(g_editor->hint_label), gettext(str));
	gdk_flush();
} /* ed_set_hint */


/*****/

gint
ed_set_list_of_gradients(GdkGC *gc, GtkWidget *clist,gradient_t *sel_gradient)
{
	GSList     *list;
	gradient_t *grad;
	int         n;
	int         select_pos = -1;

	list = gradients_list;
	n    = 0;

	if(sel_gradient == NULL)
	  sel_gradient = curr_gradient;

	gtk_clist_freeze(GTK_CLIST(clist));

	while (list) {
		grad = list->data;

		if (grad == sel_gradient) {	       
			ed_insert_in_gradients_listbox(gc,clist,grad, n, 1);
			select_pos = n;
		}
		else {
			ed_insert_in_gradients_listbox(gc,clist,grad, n, 0);
		}

		list = g_slist_next(list);
		n++;
	} /* while */

	gtk_clist_thaw(GTK_CLIST(clist));

	return select_pos;
} /* ed_set_list_of_gradients */


/*****/

static void
fill_clist_prev(gradient_t *grad, guchar *buf, gint width, gint height, double left, double right)
{
	guchar *p0, *p1,*even,*odd;
	int     x, y;
	double  dx, cur_x;
	double  r, g, b, a;
	double  c0, c1;
	gradient_t *oldgrad = curr_gradient;
	curr_gradient = grad;

	dx    = (right - left) / (width - 1);
	cur_x = left;
	p0    = even = g_malloc(width*3);
	p1    = odd = g_malloc(width*3);

	/* Create lines to fill the image */

	for (x = 0; x < width; x++) {
		grad_get_color_at(cur_x, &r, &g, &b, &a);

		if ((x / GRAD_CHECK_SIZE_SM) & 1) {
			c0 = GRAD_CHECK_LIGHT;
			c1 = GRAD_CHECK_DARK;
		} else {
			c0 = GRAD_CHECK_DARK;
			c1 = GRAD_CHECK_LIGHT;
		} /* else */

		*p0++ = (c0 + (r - c0) * a) * 255.0;
		*p0++ = (c0 + (g - c0) * a) * 255.0;
		*p0++ = (c0 + (b - c0) * a) * 255.0;

		*p1++ = (c1 + (r - c1) * a) * 255.0;
		*p1++ = (c1 + (g - c1) * a) * 255.0;
		*p1++ = (c1 + (b - c1) * a) * 255.0;

		cur_x += dx;
	} /* for */

	for (y = 0; y < height; y++)
	  {
	    if ((y / GRAD_CHECK_SIZE_SM) & 1)
	      {
 		memcpy(buf+(width*y*3),odd,width*3); 
	      }
	    else
	      {
 		memcpy(buf+(width*y*3),even,width*3); 
	      }
	  }

	g_free(even);
	g_free(odd);
	curr_gradient = oldgrad;
}

/*****/
static void 
draw_small_preview(GdkGC *gc, GtkWidget * clist,gradient_t *grad,int pos)
{
	char rgb_buf[48*16*3];

	fill_clist_prev(grad,rgb_buf,48,16,0.0,1.0);

	gdk_draw_rgb_image (grad->pixmap,
                    gc,
                    0,
                    0,
                    48,
                    16,
                    GDK_RGB_DITHER_NORMAL,
                    rgb_buf,
                    48*3);

    	gdk_gc_set_foreground(gc, &black);
	gdk_draw_rectangle(grad->pixmap, gc, FALSE, 0, 0, 47, 15);
 	gtk_clist_set_text(GTK_CLIST(clist),pos,1,grad->name);  
}


/*****/

void
ed_insert_in_gradients_listbox(GdkGC * gc,
			       GtkWidget * clist,
			       gradient_t *grad, 
			       int pos, 
			       int select)
{
	char *string[2];

	string[0] = NULL;
	string[1] = grad->name;

	gtk_clist_insert(GTK_CLIST(clist), pos,string);

	if(grad->pixmap == NULL)
	  grad->pixmap = gdk_pixmap_new(g_editor->shell->window,
					48, 16, gtk_widget_get_visual(g_editor->shell)->depth);
	
	draw_small_preview(gc,clist,grad,pos);

	gtk_clist_set_pixmap(GTK_CLIST(clist), pos, 0, grad->pixmap, NULL);

	if (select)
	{
	  gtk_clist_select_row(GTK_CLIST(clist),pos,-1);
	  gtk_clist_moveto(GTK_CLIST(clist),pos,0,0.5,0.0);
	}

} /* ed_insert_in_gradients_listbox */


/*****/

static void
ed_list_item_update(GtkWidget *widget, 
		    gint row,
		   gint column,
		   GdkEventButton *event,
		   gpointer data)
{
	/* Update current gradient */
        GSList* tmp = g_slist_nth(gradients_list,row);
 	curr_gradient = (gradient_t *)(tmp->data);

	ed_update_editor(GRAD_UPDATE_PREVIEW | GRAD_RESET_CONTROL);
} /* ed_list_item_update */


/*****/

static void
ed_initialize_saved_colors(void)
{
	int i;

	for (i = 0; i < (GRAD_NUM_COLORS + 3); i++) {
		g_editor->left_load_color_boxes[i] = NULL;
		g_editor->left_load_labels[i]      = NULL;

		g_editor->right_load_color_boxes[i] = NULL;
		g_editor->right_load_labels[i]      = NULL;
	} /* for */

	for (i = 0; i < GRAD_NUM_COLORS; i++) {
		g_editor->left_save_color_boxes[i] = NULL;
		g_editor->left_save_labels[i]      = NULL;

		g_editor->right_save_color_boxes[i] = NULL;
		g_editor->right_save_labels[i]      = NULL;
	} /* for */

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
} /* ed_initialize_saved_colors */


/*****/

static gint
ed_close_callback(GtkWidget *widget, gpointer client_data)
{
	if (GTK_WIDGET_VISIBLE(g_editor->shell))
		gtk_widget_hide(g_editor->shell);

	return TRUE;
} /* ed_close_callback */


/*****/

static void
ed_new_gradient_callback(GtkWidget *widget, gpointer client_data)
{
	query_string_box(_("New gradient"),
			 _("Enter a name for the new gradient"),
			 _("untitled"),
			 NULL, NULL,
			 ed_do_new_gradient_callback, NULL);
} /* ed_new_gradient_callback */


/*****/

static void
ed_do_new_gradient_callback(GtkWidget *widget, gpointer client_data, gpointer call_data)
{
	gradient_t *grad;
	char       *gradient_name;
	int         pos;

	gradient_name = (char *) call_data;

	if (!gradient_name) {
		g_message (_("ed_do_new_gradient_callback(): oops, received NULL in call_data"));
		return;
	} /* if */

	grad = grad_create_default_gradient();

	grad->name     = gradient_name; /* We don't need to copy since this memory is ours */
	grad->dirty    = 1;
	grad->filename = build_user_filename(grad->name, gradient_path);

	/* Put new gradient in list */

	pos = grad_insert_in_gradients_list(grad);
	gtk_clist_freeze(GTK_CLIST(g_editor->clist));
	ed_insert_in_gradients_listbox(g_editor->gc,g_editor->clist,grad, pos, 1);
	gtk_clist_thaw(GTK_CLIST(g_editor->clist));
	gtk_clist_moveto(GTK_CLIST(g_editor->clist),pos,0,0.5,0.0);

	curr_gradient = grad;

	ed_update_editor(GRAD_UPDATE_PREVIEW | GRAD_RESET_CONTROL);

	grad_sel_new_all(pos,grad);
} /* ed_do_new_gradient_callback */

/*****/

static void
ed_do_rename_gradient_callback(GtkWidget *widget, gpointer client_data, gpointer call_data)
{
	gradient_t *grad = (gradient_t *)client_data;
	gradient_t *grad_list = NULL;
	char       *gradient_name;
	GSList     *tmp;
	int         n;

	gradient_name = (char *) call_data;

	if (!gradient_name) {
		g_message (_("ed_do_rename_gradient_callback(): oops, received NULL in call_data"));
		return;
	} /* if */

	n   = 0;
	tmp = gradients_list;

	while (tmp) {
		grad_list = tmp->data;

		if(grad_list == grad)
		        break;

		n++;
		tmp = g_slist_next(tmp);
	} /* while */

	if (!grad || !grad_list) {
		g_message (_("ed_do_rename_gradient_callback(): oops, can't find gradient to rename"));
		return;
	} /* if */

	/* leak? */
	grad->name     = gradient_name; /* We don't need to copy since this memory is ours */
	grad->dirty    = 1;

	/* Delete file and free gradient */
	unlink(grad->filename);

	grad->filename = build_user_filename(grad->name, gradient_path);

 	gtk_clist_set_text(GTK_CLIST(g_editor->clist),n,1,grad->name);  

	ed_update_editor(GRAD_UPDATE_PREVIEW | GRAD_RESET_CONTROL);
	
	grad_sel_rename_all(n,grad);
} /* ed_do_rename_gradient_callback */


/*****/

static void
ed_copy_gradient_callback(GtkWidget *widget, gpointer client_data)
{
	char *name;

	if (curr_gradient == NULL) 
               return;

	name = g_strdup_printf(_("%s copy"), curr_gradient->name);

	query_string_box(_("Copy gradient"),
			 _("Enter a name for the copied gradient"),
			 name,
			 NULL, NULL,
			 ed_do_copy_gradient_callback, NULL);

	g_free(name);
} /* ed_copy_gradient_callback */


/*****/

static void
ed_do_copy_gradient_callback(GtkWidget *widget, gpointer client_data, gpointer call_data)
{
	gradient_t     *grad;
	char           *gradient_name;
	int             pos;
	grad_segment_t *head, *prev, *cur, *orig;

	gradient_name = (char *) call_data;

	if (!gradient_name) {
		g_message (_("ed_do_copy_gradient_callback(): oops, received NULL in call_data"));
		return;
	} /* if */

	/* Copy current gradient */

	grad = grad_new_gradient();

	grad->name     = gradient_name; /* We don't need to copy since this memory is ours */
	grad->dirty    = 1;
	grad->filename = build_user_filename(grad->name, gradient_path);

	prev = NULL;
	orig = curr_gradient->segments;
	head = NULL;

	while (orig) {
		cur = seg_new_segment();

		*cur = *orig; /* Copy everything */

		cur->prev = prev;
		cur->next = NULL;

		if (prev)
			prev->next = cur;
		else
			head = cur; /* Remember head */

		prev = cur;
		orig = orig->next;
	} /* while */

	grad->segments = head;

	/* Put new gradient in list */

	pos = grad_insert_in_gradients_list(grad);
	gtk_clist_freeze(GTK_CLIST(g_editor->clist));
	ed_insert_in_gradients_listbox(g_editor->gc,g_editor->clist,grad, pos, 1);
	gtk_clist_thaw(GTK_CLIST(g_editor->clist));

	curr_gradient = grad;

	ed_update_editor(GRAD_UPDATE_PREVIEW | GRAD_RESET_CONTROL);

	grad_sel_copy_all(pos,grad);
} /* ed_do_copy_gradient_callback */


/*****/

static void
ed_delete_gradient_callback(GtkWidget *widget, gpointer client_data)
{
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *button;
	char      *str;

	if (num_gradients <= 1)
		return;

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), _("Delete gradient"));
	gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
	gtk_container_border_width(GTK_CONTAINER(dialog), 0);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 8);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), vbox,
			   FALSE, FALSE, 0);
	gtk_widget_show(vbox);

	/* Question */

	label = gtk_label_new(_("Are you sure you want to delete"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	str = g_strdup_printf(_("\"%s\" from the list and from disk?"), curr_gradient->name);

	label = gtk_label_new(str);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	g_free(str);

	/* Buttons */

	button = ed_create_button(_("Delete"), 0.5, 0.5,
				  (GtkSignalFunc) ed_do_delete_gradient_callback,
				  (gpointer) dialog);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
			   button, TRUE, TRUE, 0);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	button = ed_create_button(_("Cancel"), 0.5, 0.5,
				  (GtkSignalFunc) ed_cancel_delete_gradient_callback,
				  (gpointer) dialog);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
			   button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	/* Show! */

	gtk_widget_show(dialog);
	gtk_widget_set_sensitive(g_editor->shell, FALSE);
} /* ed_delete_gradient_callback */


/*****/

static void
ed_do_delete_gradient_callback(GtkWidget *widget, gpointer client_data)
{
	GSList     *tmp;
	int         n;
	int         real_pos;
	gradient_t *g;

	gtk_widget_destroy(GTK_WIDGET(client_data));
	gtk_widget_set_sensitive(g_editor->shell, TRUE);

	/* See which gradient we will have to select once the current one is deleted */

	real_pos = n = 0;
	tmp = gradients_list;

	while (tmp) {
		g = tmp->data;

		if (g == curr_gradient) {
		        real_pos = n;
			if (tmp->next == NULL)
				n--; /* Will have to select the *previous* one */

			break; /* We found the one we want */
		} /* if */

		n++; /* Next gradient */
		tmp = g_slist_next(tmp);
	} /* while */

	if (tmp == NULL)
		fatal_error(_("ed_do_delete_gradient_callback(): aieee, could not find gradient to delete!"));

	/* Delete gradient from gradients list */

	gradients_list = g_slist_remove(gradients_list, curr_gradient);

	/* Delete file and free gradient */

	unlink(curr_gradient->filename);
	grad_free_gradient(curr_gradient);

	/* Delete gradient from listbox */

	gtk_clist_remove(GTK_CLIST(g_editor->clist), real_pos);

	/* Select new gradient */

	curr_gradient = g_slist_nth(gradients_list, n)->data;
	gtk_clist_select_row(GTK_CLIST(g_editor->clist),n,-1);
	gtk_clist_moveto(GTK_CLIST(g_editor->clist),n,0,0.5,0.0);

	/* Update! */

	ed_update_editor(GRAD_UPDATE_PREVIEW | GRAD_RESET_CONTROL);

	grad_sel_delete_all(real_pos);

} /* ed_do_delete_gradient_callback */


/*****/

static void
ed_cancel_delete_gradient_callback(GtkWidget *widget, gpointer client_data)
{
	gtk_widget_destroy(GTK_WIDGET(client_data));
	gtk_widget_set_sensitive(g_editor->shell, TRUE);
} /* ed_cancel_delete_gradient_callback */

/*****/

static void
ed_save_grads_callback(GtkWidget *widget, gpointer client_data)
{
        grad_save_all(0);
}


/*****/

static void
ed_rename_grads_callback(GtkWidget *widget, gpointer client_data)
{
        if(curr_gradient == NULL)
	  return;

	query_string_box(_("Rename gradient"),
			 _("Enter a new name for the gradient"),
			 curr_gradient->name,
			 NULL, NULL,
			 ed_do_rename_gradient_callback, curr_gradient);
}

/*****/

static void
ed_save_pov_callback(GtkWidget *widget, gpointer client_data)
{
	GtkWidget *window;

	if (curr_gradient == NULL) return;

	window = gtk_file_selection_new(_("Save as POV-Ray"));
	gtk_window_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);

	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(window)->ok_button),
			   "clicked", (GtkSignalFunc) ed_do_save_pov_callback,
			   window);

	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(window)->cancel_button),
			   "clicked", (GtkSignalFunc) ed_cancel_save_pov_callback,
			   window);

	gtk_widget_show(window);
	gtk_widget_set_sensitive(g_editor->shell, FALSE);
} /* ed_save_pov_callback */


/*****/

static void
ed_refresh_callback(GtkWidget *widget, gpointer client_data)
{
        int select_pos;
	gtk_clist_freeze(GTK_CLIST(g_editor->clist));
	gtk_clist_clear(GTK_CLIST(g_editor->clist));
	gtk_clist_thaw(GTK_CLIST(g_editor->clist));
	
	grad_sel_free_all();

	grad_free_gradients();

	gradients_init(FALSE);

	select_pos = ed_set_list_of_gradients(g_editor->gc,
					      g_editor->clist,
					      curr_gradient);

	ed_update_editor(GRAD_UPDATE_PREVIEW | GRAD_RESET_CONTROL); 

	if(select_pos != -1)
	  gtk_clist_moveto(GTK_CLIST(g_editor->clist),select_pos,0,0.5,0.0);

	grad_sel_refill_all();
} /* ed_refresh_callback */

/*****/

static void
ed_do_save_pov_callback(GtkWidget *widget, gpointer client_data)
{
	char           *filename;
	FILE           *file;
	grad_segment_t *seg;

	filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(client_data));

	file = fopen(filename, "wb");

	if (!file)
		g_message (_("ed_do_save_pov_callback(): oops, could not open \"%s\""), filename);
	else {
		fprintf(file, "/* color_map file created by the GIMP */\n");
		fprintf(file, "/* http://www.gimp.org/               */\n");

		fprintf(file, "color_map {\n");

		seg = curr_gradient->segments;

		while (seg) {
			/* Left */

			fprintf(file, "\t[%f color rgbt <%f, %f, %f, %f>]\n",
				seg->left,
				seg->r0, seg->g0, seg->b0, 1.0 - seg->a0);

			/* Middle */

			fprintf(file, "\t[%f color rgbt <%f, %f, %f, %f>]\n",
				seg->middle,
				(seg->r0 + seg->r1) / 2.0,
				(seg->g0 + seg->g1) / 2.0,
				(seg->b0 + seg->b1) / 2.0,
				1.0 - (seg->a0 + seg->a1) / 2.0);

			/* Right */

			fprintf(file, "\t[%f color rgbt <%f, %f, %f, %f>]\n",
				seg->right,
				seg->r1, seg->g1, seg->b1, 1.0 - seg->a1);

			/* Next! */

			seg = seg->next;
		} /* while */

		fprintf(file, "} /* color_map */\n");
		fclose(file);
	} /* else */

	gtk_widget_destroy(GTK_WIDGET(client_data));
	gtk_widget_set_sensitive(g_editor->shell, TRUE);
} /* ed_do_save_pov_callback */


/*****/

static void
ed_cancel_save_pov_callback(GtkWidget *widget, gpointer client_data)
{
	gtk_widget_destroy(GTK_WIDGET(client_data));
	gtk_widget_set_sensitive(g_editor->shell, TRUE);
} /* ed_cancel_save_pov_callback */


/*****/

static void
ed_scrollbar_update(GtkAdjustment *adjustment, gpointer data)
{
	char           str[256];

	g_snprintf(str, sizeof(str), _("Zoom factor: %d:1    Displaying [%0.6f, %0.6f]"),
		   g_editor->zoom_factor, adjustment->value, adjustment->value + adjustment->page_size);

	ed_set_hint(str);

	ed_update_editor(GRAD_UPDATE_PREVIEW | GRAD_UPDATE_CONTROL);
} /* ed_scrollbar_update */


/*****/

static void
ed_zoom_all_callback(GtkWidget *widget, gpointer client_data)
{
	GtkAdjustment *adjustment;

	adjustment = GTK_ADJUSTMENT(g_editor->scroll_data);

	g_editor->zoom_factor = 1;

	adjustment->value     	   = 0.0;
	adjustment->page_size 	   = 1.0;
	adjustment->step_increment = 1.0 * GRAD_SCROLLBAR_STEP_SIZE;
	adjustment->page_increment = 1.0 * GRAD_SCROLLBAR_PAGE_SIZE;

	gtk_signal_emit_by_name(g_editor->scroll_data, "changed");
} /* ed_zoom_all_callback */


/*****/

static void
ed_zoom_out_callback(GtkWidget *widget, gpointer client_data)
{
	GtkAdjustment *adjustment;
	double 	       old_value, value;
	double 	       old_page_size, page_size;

	if (g_editor->zoom_factor <= 1)
		return;

	adjustment = GTK_ADJUSTMENT(g_editor->scroll_data);

	old_value     = adjustment->value;
	old_page_size = adjustment->page_size;

	g_editor->zoom_factor--;

	page_size = 1.0 / g_editor->zoom_factor;
	value     = old_value - (page_size - old_page_size) / 2.0;

	if (value < 0.0)
		value = 0.0;
	else if ((value + page_size) > 1.0)
		value = 1.0 - page_size;

	adjustment->value     	   = value;
	adjustment->page_size 	   = page_size;
	adjustment->step_increment = page_size * GRAD_SCROLLBAR_STEP_SIZE;
	adjustment->page_increment = page_size * GRAD_SCROLLBAR_PAGE_SIZE;

	gtk_signal_emit_by_name(g_editor->scroll_data, "changed");
} /* ed_zoom_out_callback */


/*****/

static void
ed_zoom_in_callback(GtkWidget *widget, gpointer client_data)
{
	GtkAdjustment *adjustment;
	double 	       old_value;
	double 	       old_page_size, page_size;

	adjustment = GTK_ADJUSTMENT(g_editor->scroll_data);

	old_value     = adjustment->value;
	old_page_size = adjustment->page_size;

	g_editor->zoom_factor++;

	page_size = 1.0 / g_editor->zoom_factor;

	adjustment->value     	   = old_value + (old_page_size - page_size) / 2.0;
	adjustment->page_size 	   = page_size;
	adjustment->step_increment = page_size * GRAD_SCROLLBAR_STEP_SIZE;
	adjustment->page_increment = page_size * GRAD_SCROLLBAR_PAGE_SIZE;

	gtk_signal_emit_by_name(g_editor->scroll_data, "changed");
} /* ed_zoom_in_callback */


/*****/

static void
ed_instant_update_update(GtkWidget *widget, gpointer data)
{
	if (GTK_TOGGLE_BUTTON(widget)->active) {
		g_editor->instant_update = 1;
		gtk_range_set_update_policy(GTK_RANGE(g_editor->scrollbar), GTK_UPDATE_CONTINUOUS);
	} else {
		g_editor->instant_update = 0;
		gtk_range_set_update_policy(GTK_RANGE(g_editor->scrollbar), GTK_UPDATE_DELAYED);
	} /* else */
} /* ed_instant_update_update */


/***** Gradient preview functions *****/

/*****/

static gint
prev_events(GtkWidget *widget, GdkEvent *event)
{
	gint            x, y;
	GdkEventButton *bevent;

	/* ignore events when no gradient is present */
	if (curr_gradient == NULL) 
	        return FALSE;

	switch (event->type) {
		case GDK_EXPOSE:
			prev_update(0);
			break;

		case GDK_LEAVE_NOTIFY:
			ed_set_hint("");
			break;

		case GDK_MOTION_NOTIFY:
			gtk_widget_get_pointer(g_editor->preview, &x, &y);

			if (x != g_editor->preview_last_x) {
				g_editor->preview_last_x = x;

				if (g_editor->preview_button_down)
					prev_set_foreground(x);
				else
					prev_set_hint(x);
			} /* if */

			break;

		case GDK_BUTTON_PRESS:
			gtk_widget_get_pointer(g_editor->preview, &x, &y);

			bevent = (GdkEventButton *) event;

			switch (bevent->button) {
				case 1:
					g_editor->preview_last_x = x;
					g_editor->preview_button_down = 1;
					prev_set_foreground(x);
					break;

				case 3:
					cpopup_do_popup();
					break;

				default:
					break;
			} /* switch */

			break;

		case GDK_BUTTON_RELEASE:
			if (g_editor->preview_button_down) {
				gtk_widget_get_pointer(g_editor->preview, &x, &y);
				g_editor->preview_last_x = x;
				g_editor->preview_button_down = 0;
				prev_set_foreground(x);
			} /* if */

			break;

		default:
			break;
	} /* switch */

	return FALSE;
} /* prev_events */


/*****/

static void
prev_set_hint(gint x)
{
	double xpos;
	double r, g, b, a;
	double h, s, v;
	char   str[256];

	xpos = control_calc_g_pos(x);

	grad_get_color_at(xpos, &r, &g, &b, &a);

	h = r;
	s = g;
	v = b;

	calc_rgb_to_hsv(&h, &s, &v);

	g_snprintf(str, sizeof(str), _("Position: %0.6f    "
		   "RGB (%0.3f, %0.3f, %0.3f)    "
		   "HSV (%0.3f, %0.3f, %0.3f)    "
		   "Opacity: %0.3f"),
		   xpos, r, g, b, h * 360.0, s, v, a);

	ed_set_hint(str);
} /* prev_set_hint */


/*****/

static void
prev_set_foreground(gint x)
{
	double xpos;
	double r, g, b, a;
	char   str[512];

	xpos = control_calc_g_pos(x);
	grad_get_color_at(xpos, &r, &g, &b, &a);

	palette_set_foreground(r * 255.0, g * 255.0, b * 255.0);

	g_snprintf(str, sizeof(str), _("Foreground color set to RGB (%d, %d, %d) <-> (%0.3f, %0.3f, %0.3f)"),
		   (int) (r * 255.0),
		   (int) (g * 255.0),
		   (int) (b * 255.0),
		   r, g, b);

	ed_set_hint(str);
} /* prev_set_foreground */


/*****/

static void
prev_update(int recalculate)
{
	long           rowsiz;
	GtkAdjustment *adjustment;
	guint16        width, height;
	guint16        pwidth, pheight;
	GSList     *tmp;
	int         n;
	gradient_t *g;
	static gradient_t *last_grad = NULL;
	static int last_row = -1;

	/* We only update if we can draw to the widget and a gradient is present */

	if (curr_gradient == NULL) 
	        return;
	if (!GTK_WIDGET_DRAWABLE(g_editor->preview))
		return;

	/* See whether we have to re-create the preview widget */

	width   = g_editor->preview->allocation.width;
	height  = g_editor->preview->allocation.height;

	/* hof: do not change preview size on Window resize.
	 *      The original code allows expansion of the preview
	 *      on window resize events. But once expanded, there is no way to shrink
	 *      the window back to the original size.
	 *  A full Bugfix should change the preview size according to the users     
	 *  window resize actions.   
	 */

	       width   = GRAD_PREVIEW_WIDTH;
	       height  = GRAD_PREVIEW_HEIGHT;
	
	pwidth  = GTK_PREVIEW(g_editor->preview)->buffer_width;
	pheight = GTK_PREVIEW(g_editor->preview)->buffer_height;

	if (!g_editor->preview_rows[0] || !g_editor->preview_rows[1] ||
	    (width != pwidth) || (height != pheight)) {
		if (g_editor->preview_rows[0])
			g_free(g_editor->preview_rows[0]);

		if (g_editor->preview_rows[1])
			g_free(g_editor->preview_rows[1]);

		gtk_preview_size(GTK_PREVIEW(g_editor->preview), width, height);

		rowsiz = width * 3 * sizeof(guchar);

		g_editor->preview_rows[0] = g_malloc(rowsiz);
		g_editor->preview_rows[1] = g_malloc(rowsiz);

		recalculate = 1; /* Force recalculation */
	} /* if */

	/* Have to redraw? */

	if (recalculate) {
		adjustment = GTK_ADJUSTMENT(g_editor->scroll_data);

		prev_fill_image(width, height,
				adjustment->value,
				adjustment->value + adjustment->page_size);

		gtk_widget_draw(g_editor->preview, NULL);
	} /* if */

	if(last_grad != curr_gradient || last_row < 0) {
	        n = 0;
		tmp = gradients_list;
		
		while (tmp) {
		  g = tmp->data;
		  
		  if (g == curr_gradient) {
		    break; /* We found the one we want */
		  } /* if */
		  
		  n++; /* Next gradient */
		  tmp = g_slist_next(tmp);
		} /* while */
		last_grad = curr_gradient;
		last_row = n;
	} /* if */

	draw_small_preview(g_editor->gc,g_editor->clist,curr_gradient,last_row);
	/* Update any others that are on screen */
	sel_update_dialogs(last_row,curr_gradient);
} /* prev_update */


/*****/

static void
prev_fill_image(int width, int height, double left, double right)
{
	guchar *p0, *p1;
	int     x, y;
	double  dx, cur_x;
	double  r, g, b, a;
	double  c0, c1;

	dx    = (right - left) / (width - 1);
	cur_x = left;
	p0    = g_editor->preview_rows[0];
	p1    = g_editor->preview_rows[1];

	/* Create lines to fill the image */

	for (x = 0; x < width; x++) {
		grad_get_color_at(cur_x, &r, &g, &b, &a);

		if ((x / GRAD_CHECK_SIZE) & 1) {
			c0 = GRAD_CHECK_LIGHT;
			c1 = GRAD_CHECK_DARK;
		} else {
			c0 = GRAD_CHECK_DARK;
			c1 = GRAD_CHECK_LIGHT;
		} /* else */

		*p0++ = (c0 + (r - c0) * a) * 255.0;
		*p0++ = (c0 + (g - c0) * a) * 255.0;
		*p0++ = (c0 + (b - c0) * a) * 255.0;

		*p1++ = (c1 + (r - c1) * a) * 255.0;
		*p1++ = (c1 + (g - c1) * a) * 255.0;
		*p1++ = (c1 + (b - c1) * a) * 255.0;

		cur_x += dx;
	} /* for */

	/* Fill image */

	for (y = 0; y < height; y++)
		if ((y / GRAD_CHECK_SIZE) & 1)
			gtk_preview_draw_row(GTK_PREVIEW(g_editor->preview),
					     g_editor->preview_rows[1], 0, y, width);
		else
			gtk_preview_draw_row(GTK_PREVIEW(g_editor->preview),
					     g_editor->preview_rows[0], 0, y, width);
} /* prev_fill_image */


/***** Gradient control functions *****/

/* *** WARNING *** WARNING *** WARNING ***
 *
 * All the event-handling code for the gradient control widget is
 * extremely hairy.  You are not expected to understand it.  If you
 * find bugs, mail me unless you are very brave and you want to fix
 * them yourself ;-)
 */

/*****/

static gint
control_events(GtkWidget *widget, GdkEvent *event)
{
	gint           	    x, y;
	guint32        	    time;
	GdkEventButton 	   *bevent;
	grad_segment_t 	   *seg;

	switch (event->type) {
		case GDK_EXPOSE:
			control_update(0);
			break;

		case GDK_LEAVE_NOTIFY:
			ed_set_hint("");
			break;

		case GDK_BUTTON_PRESS:
			if (g_editor->control_drag_mode == GRAD_DRAG_NONE) {
				gtk_widget_get_pointer(g_editor->control, &x, &y);

				bevent = (GdkEventButton *) event;

				g_editor->control_last_x     = x;
				g_editor->control_click_time = bevent->time;

				control_button_press(x, y, bevent->button, bevent->state);

				if (g_editor->control_drag_mode != GRAD_DRAG_NONE)
					gtk_grab_add(widget);
			} /* if */

			break;

		case GDK_BUTTON_RELEASE:
			ed_set_hint("");

			if (g_editor->control_drag_mode != GRAD_DRAG_NONE) {
				gtk_grab_remove(widget);

				gtk_widget_get_pointer(g_editor->control, &x, &y);

				time = ((GdkEventButton *) event)->time;

				if ((time - g_editor->control_click_time) >= GRAD_MOVE_TIME)
					ed_update_editor(GRAD_UPDATE_PREVIEW); /* Possible move */
				else
					if ((g_editor->control_drag_mode == GRAD_DRAG_MIDDLE) ||
					    (g_editor->control_drag_mode == GRAD_DRAG_ALL)) {
						seg = g_editor->control_drag_segment;

						if ((g_editor->control_drag_mode == GRAD_DRAG_ALL) &&
						    g_editor->control_compress)
							control_extend_selection(seg, control_calc_g_pos(x));
						else
							control_select_single_segment(seg);

						ed_update_editor(GRAD_UPDATE_CONTROL);
					} /* if */

				g_editor->control_drag_mode = GRAD_DRAG_NONE;
				g_editor->control_compress  = 0;

				control_do_hint(x, y);
			} /* if */

			break;

		case GDK_MOTION_NOTIFY:
			gtk_widget_get_pointer(g_editor->control, &x, &y);

			if (x != g_editor->control_last_x) {
				g_editor->control_last_x = x;

				if (g_editor->control_drag_mode != GRAD_DRAG_NONE) {
					time = ((GdkEventButton *) event)->time;

					if ((time - g_editor->control_click_time) >= GRAD_MOVE_TIME)
						control_motion(x);
				} else {
					ed_update_editor(GRAD_UPDATE_CONTROL);

					control_do_hint(x, y);
				} /* else */
			} /* if */

			break;

		default:
			break;
	} /* switch */

	return FALSE;
} /* control_events */


/*****/

static void
control_do_hint(gint x, gint y)
{
	grad_segment_t      *seg;
	control_drag_mode_t  handle;
	int                  in_handle;
	double               pos;

	pos = control_calc_g_pos(x);

	if ((pos < 0.0) || (pos > 1.0))
		return;

	seg_get_closest_handle(curr_gradient, pos, &seg, &handle);

	in_handle = control_point_in_handle(x, y, seg, handle);

	if (in_handle) {
		switch (handle) {
			case GRAD_DRAG_LEFT:
				if (seg != NULL) {
					if (seg->prev != NULL)
						ed_set_hint(_("Drag: move    Shift+drag: move & compress"));
					else
						ed_set_hint(_("Click: select    Shift+click: extend selection"));
				} else
					ed_set_hint(_("Click: select    Shift+click: extend selection"));

				break;

			case GRAD_DRAG_MIDDLE:
				ed_set_hint(_("Click: select    Shift+click: extend selection    "
					    "Drag: move"));

				break;

			default:
				g_message (_("control_do_hint: oops, in_handle is true "
					"yet we got handle type %d"), (int) handle);
				break;
		} /* switch */
	} else
		ed_set_hint(_("Click: select    Shift+click: extend selection    "
			    "Drag: move    Shift+drag: move & compress"));
} /* control_do_hint */


/*****/

static void
control_button_press(gint x, gint y, guint button, guint state)
{
	grad_segment_t      *seg;
	control_drag_mode_t  handle;
	double               xpos;
	int                  in_handle;

	/* See which button was pressed */

	switch (button) {
		case 1:
			break;

		case 3:
			cpopup_do_popup();
			return;

		default:
			return;
	} /* switch */

	/* Find the closest handle */

	xpos = control_calc_g_pos(x);

	seg_get_closest_handle(curr_gradient, xpos, &seg, &handle);

	in_handle = control_point_in_handle(x, y, seg, handle);

	/* Now see what we have */

	if (in_handle)
		switch (handle) {
			case GRAD_DRAG_LEFT:
				if (seg != NULL) {
					/* Left handle of some segment */

					if (state & GDK_SHIFT_MASK) {
						if (seg->prev != NULL) {
							g_editor->control_drag_mode    = GRAD_DRAG_LEFT;
							g_editor->control_drag_segment = seg;
							g_editor->control_compress     = 1;
						} else {
							control_extend_selection(seg, xpos);

							ed_update_editor(GRAD_UPDATE_CONTROL);
						} /* else */
					} else
						if (seg->prev != NULL) {
							g_editor->control_drag_mode    = GRAD_DRAG_LEFT;
							g_editor->control_drag_segment = seg;
						} else {
							control_select_single_segment(seg);

							ed_update_editor(GRAD_UPDATE_CONTROL);
						} /* else */

					return;
				} else {
					/* Right handle of last segment */

					seg = seg_get_last_segment(curr_gradient->segments);

					if (state & GDK_SHIFT_MASK) {
						control_extend_selection(seg, xpos);

						ed_update_editor(GRAD_UPDATE_CONTROL);
					} else {
						control_select_single_segment(seg);

						ed_update_editor(GRAD_UPDATE_CONTROL);
					} /* else */

					return;
				} /* else */

				break;

			case GRAD_DRAG_MIDDLE:
				if (state & GDK_SHIFT_MASK) {
					control_extend_selection(seg, xpos);

					ed_update_editor(GRAD_UPDATE_CONTROL);
				} else {
					g_editor->control_drag_mode    = GRAD_DRAG_MIDDLE;
					g_editor->control_drag_segment = seg;
				} /* else */

				return;

			default:
				g_message (_("control_button_press(): oops, in_handle is true "
					"yet we got handle type %d"), (int) handle);
				return;
		} /* switch */
	else {
		seg = seg_get_segment_at(curr_gradient, xpos);

		g_editor->control_drag_mode    = GRAD_DRAG_ALL;
		g_editor->control_drag_segment = seg;
		g_editor->control_last_gx      = xpos;
		g_editor->control_orig_pos     = xpos;

		if (state & GDK_SHIFT_MASK)
			g_editor->control_compress = 1;

		return;
	} /* else */
} /* control_button_press */


/*****/

static int
control_point_in_handle(gint x, gint y, grad_segment_t *seg, control_drag_mode_t handle)
{
	gint handle_pos;

	switch (handle) {
		case GRAD_DRAG_LEFT:
			if (seg)
				handle_pos = control_calc_p_pos(seg->left);
			else {
				seg = seg_get_last_segment(curr_gradient->segments);

				handle_pos = control_calc_p_pos(seg->right);
			} /* else */

			break;

		case GRAD_DRAG_MIDDLE:
			handle_pos = control_calc_p_pos(seg->middle);
			break;

		default:
			g_message (_("control_point_in_handle(): oops, can not handle drag mode %d"),
				(int) handle);
			return 0;
	} /* switch */

	y /= 2;

	if ((x >= (handle_pos - y)) && (x <= (handle_pos + y)))
		return 1;
	else
		return 0;
} /* control_point_in_handle */


/*****/

static void
control_select_single_segment(grad_segment_t *seg)
{
	g_editor->control_sel_l = seg;
	g_editor->control_sel_r = seg;
} /* control_select_single_segment */


/*****/

static void
control_extend_selection(grad_segment_t *seg, double pos)
{
	if (fabs(pos - g_editor->control_sel_l->left) < fabs(pos - g_editor->control_sel_r->right))
		g_editor->control_sel_l = seg;
	else
		g_editor->control_sel_r = seg;
} /* control_extend_selection */


/*****/

static void
control_motion(gint x)
{
	grad_segment_t *seg;
	double          pos;
	double          delta;
	char            str[256];

	seg = g_editor->control_drag_segment;

	switch (g_editor->control_drag_mode) {
		case GRAD_DRAG_LEFT:
			pos = control_calc_g_pos(x);

			if (!g_editor->control_compress)
				seg->prev->right = seg->left = BOUNDS(pos,
								      seg->prev->middle + EPSILON,
								      seg->middle - EPSILON);
			else
				control_compress_left(g_editor->control_sel_l,
						      g_editor->control_sel_r,
						      seg, pos);

			g_snprintf(str, sizeof(str), _("Handle position: %0.6f"), seg->left);
			ed_set_hint(str);

			break;

		case GRAD_DRAG_MIDDLE:
			pos = control_calc_g_pos(x);
			seg->middle = BOUNDS(pos, seg->left + EPSILON, seg->right - EPSILON);

			g_snprintf(str, sizeof(str), _("Handle position: %0.6f"), seg->middle);
			ed_set_hint(str);

			break;

		case GRAD_DRAG_ALL:
			pos    = control_calc_g_pos(x);
			delta  = pos - g_editor->control_last_gx;

			if ((seg->left >= g_editor->control_sel_l->left) &&
			    (seg->right <= g_editor->control_sel_r->right))
				delta = control_move(g_editor->control_sel_l, g_editor->control_sel_r, delta);
			else
				delta = control_move(seg, seg, delta);

			g_editor->control_last_gx += delta;

			g_snprintf(str, sizeof(str), _("Distance: %0.6f"),
				g_editor->control_last_gx - g_editor->control_orig_pos);
			ed_set_hint(str);

			break;

		default:
			fatal_error(_("control_motion(): aieee, attempt to move bogus handle %d"),
				    (int) g_editor->control_drag_mode);
			break;
	} /* switch */

	curr_gradient->dirty = 1;

	if (g_editor->instant_update)
		ed_update_editor(GRAD_UPDATE_PREVIEW | GRAD_UPDATE_CONTROL);
	else
		ed_update_editor(GRAD_UPDATE_CONTROL);
} /* control_motion */


/*****/

static void
control_compress_left(grad_segment_t *range_l, grad_segment_t *range_r,
		      grad_segment_t *drag_seg, double pos)
{
	grad_segment_t *seg;
	double          lbound, rbound;
	int             k;

	/* Check what we have to compress */

	if (!((drag_seg->left >= range_l->left) &&
	      ((drag_seg->right <= range_r->right) || (drag_seg == range_r->next)))) {
		/* We are compressing a segment outside the selection */

		range_l = range_r = drag_seg;
	} /* else */

	/* Calculate left bound for dragged hadle */

	if (drag_seg == range_l)
		lbound = range_l->prev->left + 2.0 * EPSILON;
	else {
		/* Count number of segments to the left of the dragged handle */

		seg = drag_seg;
		k   = 0;

		while (seg != range_l) {
			k++;
			seg = seg->prev;
		} /* while */

		/* 2*k handles have to fit */

		lbound = range_l->left + 2.0 * k * EPSILON;
	} /* else */

	/* Calculate right bound for dragged handle */

	if (drag_seg == range_r->next)
		rbound = range_r->next->right - 2.0 * EPSILON;
	else {
		/* Count number of segments to the right of the dragged handle */

		seg = drag_seg;
		k   = 1;

		while (seg != range_r) {
			k++;
			seg = seg->next;
		} /* while */

		/* 2*k handles have to fit */

		rbound = range_r->right - 2.0 * k * EPSILON;
	} /* else */

	/* Calculate position */

	pos = BOUNDS(pos, lbound, rbound);

	/* Compress segments to the left of the handle */

	if (drag_seg == range_l)
		control_compress_range(range_l->prev, range_l->prev, range_l->prev->left, pos);
	else
		control_compress_range(range_l, drag_seg->prev, range_l->left, pos);

	/* Compress segments to the right of the handle */

	if (drag_seg != range_r->next)
		control_compress_range(drag_seg, range_r, pos, range_r->right);
	else
		control_compress_range(drag_seg, drag_seg, pos, drag_seg->right);
} /* control_compress_left */


/*****/

static void
control_compress_range(grad_segment_t *range_l, grad_segment_t *range_r,
		       double new_l, double new_r)
{
	double          orig_l, orig_r;
	double          scale;
	grad_segment_t *seg, *aseg;

	orig_l = range_l->left;
	orig_r = range_r->right;

	scale = (new_r - new_l) / (orig_r - orig_l);

	seg = range_l;

	do {
		seg->left   = new_l + (seg->left - orig_l) * scale;
		seg->middle = new_l + (seg->middle - orig_l) * scale;
		seg->right  = new_l + (seg->right - orig_l) * scale;

		/* Next */

		aseg = seg;
		seg  = seg->next;
	} while (aseg != range_r);
} /* control_compress_range */


/*****/

static double
control_move(grad_segment_t *range_l, grad_segment_t *range_r, double delta)
{
	double          lbound, rbound;
	int             is_first, is_last;
	grad_segment_t *seg, *aseg;

	/* First or last segments in gradient? */

	is_first = (range_l->prev == NULL);
	is_last  = (range_r->next == NULL);

	/* Calculate drag bounds */

	if (!g_editor->control_compress) {
		if (!is_first)
			lbound = range_l->prev->middle + EPSILON;
		else
			lbound = range_l->left + EPSILON;

		if (!is_last)
			rbound = range_r->next->middle - EPSILON;
		else
			rbound = range_r->right - EPSILON;
	} else {
		if (!is_first)
			lbound = range_l->prev->left + 2.0 * EPSILON;
		else
			lbound = range_l->left + EPSILON;

		if (!is_last)
			rbound = range_r->next->right - 2.0 * EPSILON;
		else
			rbound = range_r->right - EPSILON;
	} /* if */

	/* Fix the delta if necessary */

	if (delta < 0.0) {
		if (!is_first) {
			if (range_l->left + delta < lbound)
				delta = lbound - range_l->left;
		} else
			if (range_l->middle + delta < lbound)
				delta = lbound - range_l->middle;
	} else {
		if (!is_last) {
			if (range_r->right + delta > rbound)
				delta = rbound - range_r->right;
		} else
			if (range_r->middle + delta > rbound)
				delta = rbound - range_r->middle;
	} /* else */

	/* Move all the segments inside the range */

	seg = range_l;

	do {
		if (!((seg == range_l) && is_first))
			seg->left   += delta;

		seg->middle += delta;

		if (!((seg == range_r) && is_last))
			seg->right  += delta;

		/* Next */

		aseg = seg;
		seg  = seg->next;
	} while (aseg != range_r);

	/* Fix the segments that surround the range */

	if (!is_first)
	  {
		if (!g_editor->control_compress)
			range_l->prev->right = range_l->left;
		else
			control_compress_range(range_l->prev, range_l->prev,
					       range_l->prev->left, range_l->left);
	  }
	
	if (!is_last)
	  {
		if (!g_editor->control_compress)
			range_r->next->left = range_r->right;
		else
			control_compress_range(range_r->next, range_r->next,
					       range_r->right, range_r->next->right);
	  }
	
	return delta;
} /* control_move */


/*****/

static void
control_update(int recalculate)
{
	gint 	       cwidth, cheight;
	gint 	       pwidth, pheight;
	GtkAdjustment *adjustment;

	/* We only update if we can redraw and a gradient is present */

	if (curr_gradient == NULL) 
	        return;	
	if (!GTK_WIDGET_DRAWABLE(g_editor->control))
		return;

	/* See whether we have to re-create the control pixmap */

	gdk_window_get_size(g_editor->control->window, &cwidth, &cheight);

	/* as long as we have that ugly workaround in prev_update() don't
	   change the size of the controls either when the window is resized */

        	cwidth   = GRAD_PREVIEW_WIDTH;
		cheight  = GRAD_PREVIEW_HEIGHT;

	if (g_editor->control_pixmap)
		gdk_window_get_size(g_editor->control_pixmap, &pwidth, &pheight);

	if (!g_editor->control_pixmap || (cwidth != pwidth) || (cheight != pheight)) {
		if (g_editor->control_pixmap)
			gdk_pixmap_unref(g_editor->control_pixmap);

		g_editor->control_pixmap = gdk_pixmap_new(g_editor->control->window, cwidth, cheight, -1);

		recalculate = 1;
	} /* if */

	/* Have to reset the selection? */

	if (recalculate)
		control_select_single_segment(curr_gradient->segments);

	/* Redraw pixmap */

	adjustment = GTK_ADJUSTMENT(g_editor->scroll_data);

	control_draw(g_editor->control_pixmap,
		     cwidth, cheight,
		     adjustment->value,
		     adjustment->value + adjustment->page_size);

	gdk_draw_pixmap(g_editor->control->window, g_editor->control->style->black_gc,
			g_editor->control_pixmap, 0, 0, 0, 0, cwidth, cheight);
} /* control_update */


/*****/

static void
control_draw(GdkPixmap *pixmap, int width, int height, double left, double right)
{
	int 		     sel_l, sel_r;
	double               g_pos;
	grad_segment_t      *seg;
	control_drag_mode_t  handle;

	/* Clear the pixmap */

	gdk_draw_rectangle(pixmap, g_editor->control->style->bg_gc[GTK_STATE_NORMAL],
			   TRUE, 0, 0, width, height);

	/* Draw selection */

	sel_l = control_calc_p_pos(g_editor->control_sel_l->left);
	sel_r = control_calc_p_pos(g_editor->control_sel_r->right);

	gdk_draw_rectangle(pixmap, g_editor->control->style->dark_gc[GTK_STATE_NORMAL],
			   TRUE, sel_l, 0, sel_r - sel_l + 1, height);

	/* Draw handles */

	seg = curr_gradient->segments;

	while (seg) {
		control_draw_normal_handle(pixmap, seg->left, height);
		control_draw_middle_handle(pixmap, seg->middle, height);

		/* Draw right handle only if this is the last segment */

		if (seg->next == NULL)
			control_draw_normal_handle(pixmap, seg->right, height);

		/* Next! */

		seg = seg->next;
	} /* while */

	/* Draw the handle which is closest to the mouse position */

	g_pos = control_calc_g_pos(g_editor->control_last_x);

	seg_get_closest_handle(curr_gradient, BOUNDS(g_pos, 0.0, 1.0), &seg, &handle);

	switch (handle) {
		case GRAD_DRAG_LEFT:
			if (seg)
				control_draw_normal_handle(pixmap, seg->left, height);
			else {
				seg = seg_get_last_segment(curr_gradient->segments);
				control_draw_normal_handle(pixmap, seg->right, height);
			} /* else */

			break;

		case GRAD_DRAG_MIDDLE:
			control_draw_middle_handle(pixmap, seg->middle, height);
			break;

		default:
			break;
	} /* switch */
} /* control_draw */


/*****/

static void
control_draw_normal_handle(GdkPixmap *pixmap, double pos, int height)
{
	control_draw_handle(pixmap,
			    g_editor->control->style->black_gc,
			    g_editor->control->style->black_gc,
			    control_calc_p_pos(pos), height);
} /* control_draw_normal_handle */


/*****/

static void
control_draw_middle_handle(GdkPixmap *pixmap, double pos, int height)
{
	control_draw_handle(pixmap,
			    g_editor->control->style->black_gc,
			    g_editor->control->style->bg_gc[GTK_STATE_PRELIGHT],
			    control_calc_p_pos(pos), height);
} /* control_draw_middle_handle */


/*****/

static void
control_draw_handle(GdkPixmap *pixmap, GdkGC *border_gc, GdkGC *fill_gc, int xpos, int height)
{
	int y;
	int left, right, bottom;

	for (y = 0; y < height; y++)
		gdk_draw_line(pixmap, fill_gc, xpos - y / 2, y, xpos + y / 2, y);

	bottom = height - 1;
	left   = xpos - bottom / 2;
	right  = xpos + bottom / 2;

	gdk_draw_line(pixmap, border_gc, xpos, 0, left, bottom);
	gdk_draw_line(pixmap, border_gc, xpos, 0, right, bottom);
	gdk_draw_line(pixmap, border_gc, left, bottom, right, bottom);
} /* control_draw_handle */


/*****/

static int
control_calc_p_pos(double pos)
{
	gint           pwidth, pheight;
	GtkAdjustment *adjustment;

	/* Calculate the position (in widget's coordinates) of the
	 * requested point from the gradient.  Rounding is done to
	 * minimize mismatches between the rendered gradient preview
	 * and the gradient control's handles.
	 */

	adjustment = GTK_ADJUSTMENT(g_editor->scroll_data);
	gdk_window_get_size(g_editor->control_pixmap, &pwidth, &pheight);

	return (int) ((pwidth - 1) * (pos - adjustment->value) / adjustment->page_size + 0.5);
} /* control_calc_p_pos */


/*****/

static double
control_calc_g_pos(int pos)
{
	gint   	       pwidth, pheight;
	GtkAdjustment *adjustment;

	/* Calculate the gradient position that corresponds to widget's coordinates */

	adjustment = GTK_ADJUSTMENT(g_editor->scroll_data);
	gdk_window_get_size(g_editor->control_pixmap, &pwidth, &pheight);

	return adjustment->page_size * pos / (pwidth - 1) + adjustment->value;
} /* control_calc_g_pos */


/***** Control popup functions *****/

/*****/

static void
cpopup_create_main_menu(void)
{
	GtkWidget           *menu;
	GtkWidget           *menuitem;
	GtkWidget           *label;
	GtkAccelGroup       *accel_group;

	menu      = gtk_menu_new();
	accel_group = gtk_accel_group_new();

	g_editor->accel_group = accel_group;

	gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
	gtk_window_add_accel_group (GTK_WINDOW (g_editor->shell), accel_group);

	/* Left endpoint */

	menuitem = cpopup_create_color_item(&g_editor->left_color_preview, &label);
	gtk_label_set(GTK_LABEL(label), _("Left endpoint's color"));
	gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			   (GtkSignalFunc) cpopup_set_left_color_callback,
			   NULL);
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);
	gtk_widget_add_accelerator(menuitem,
				   "activate",
				   accel_group,
				   'L', 0,
				   GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

	menuitem = gtk_menu_item_new_with_label(_("Load from"));
	g_editor->control_left_load_popup = cpopup_create_load_menu(g_editor->left_load_color_boxes,
								    g_editor->left_load_labels,
								    _("Left neighbor's right endpoint"),
								    _("Right endpoint"),
								    (GtkSignalFunc)
								    cpopup_load_left_callback,
								    'L', GDK_CONTROL_MASK,
								    'L', GDK_MOD1_MASK,
								    'F', GDK_CONTROL_MASK);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), g_editor->control_left_load_popup);
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);

	menuitem = gtk_menu_item_new_with_label(_("Save to"));
	g_editor->control_left_save_popup = cpopup_create_save_menu(g_editor->left_save_color_boxes,
								    g_editor->left_save_labels,
								    (GtkSignalFunc)
								    cpopup_save_left_callback);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), g_editor->control_left_save_popup);
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);

	/* Right endpoint */

	menuitem = gtk_menu_item_new();
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);

	menuitem = cpopup_create_color_item(&g_editor->right_color_preview, &label);
	gtk_label_set(GTK_LABEL(label), _("Right endpoint's color"));
	gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			   (GtkSignalFunc) cpopup_set_right_color_callback,
			   NULL);
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);
        gtk_widget_add_accelerator(menuitem,
				   "activate",
				   accel_group,
				   'R', 0,
				   GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

	menuitem = gtk_menu_item_new_with_label(_("Load from"));
	g_editor->control_right_load_popup = cpopup_create_load_menu(g_editor->right_load_color_boxes,
								     g_editor->right_load_labels,
								     _("Right neighbor's left endpoint"),
								     _("Left endpoint"),
								     (GtkSignalFunc)
								     cpopup_load_right_callback,
								    'R', GDK_CONTROL_MASK,
								    'R', GDK_MOD1_MASK,
								    'F', GDK_MOD1_MASK);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), g_editor->control_right_load_popup);
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);

	menuitem = gtk_menu_item_new_with_label(_("Save to"));
	g_editor->control_right_save_popup = cpopup_create_save_menu(g_editor->right_save_color_boxes,
								     g_editor->right_save_labels,
								     (GtkSignalFunc)
								     cpopup_save_right_callback);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), g_editor->control_right_save_popup);
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);

	/* Blending function */

	menuitem = gtk_menu_item_new();
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);

	menuitem = cpopup_create_menu_item_with_label("", &g_editor->control_blending_label);
	g_editor->control_blending_popup = cpopup_create_blending_menu();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), g_editor->control_blending_popup);
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);

	/* Coloring type */

	menuitem = cpopup_create_menu_item_with_label("", &g_editor->control_coloring_label);
	g_editor->control_coloring_popup = cpopup_create_coloring_menu();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), g_editor->control_coloring_popup);
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);

	/* Operations */

	menuitem = gtk_menu_item_new();
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);

	/* Split at midpoint */

	menuitem = cpopup_create_menu_item_with_label("", &g_editor->control_splitm_label);
	gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			   (GtkSignalFunc) cpopup_split_midpoint_callback,
			   NULL);
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);
        gtk_widget_add_accelerator(menuitem,
				   "activate",
				   accel_group,
				   'S', 0,
				   GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

	/* Split uniformly */

	menuitem = cpopup_create_menu_item_with_label("", &g_editor->control_splitu_label);
	gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			   (GtkSignalFunc) cpopup_split_uniform_callback,
			   NULL);
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);
        gtk_widget_add_accelerator(menuitem,
				   "activate",
				   accel_group,
				   'U', 0,
				   GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

	/* Delete */

	menuitem = cpopup_create_menu_item_with_label("", &g_editor->control_delete_label);
	gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			   (GtkSignalFunc) cpopup_delete_callback,
			   NULL);
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);
	g_editor->control_delete_menu_item = menuitem;
        gtk_widget_add_accelerator(menuitem,
				   "activate",
				   accel_group,
				   'D', 0,
				   GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

	/* Recenter */

	menuitem = cpopup_create_menu_item_with_label("", &g_editor->control_recenter_label);
	gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			   (GtkSignalFunc) cpopup_recenter_callback,
			   NULL);
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);
        gtk_widget_add_accelerator(menuitem,
				   "activate",
				   accel_group,
				   'C', 0,
				   GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

	/* Redistribute */

	menuitem = cpopup_create_menu_item_with_label("", &g_editor->control_redistribute_label);
	gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			   (GtkSignalFunc) cpopup_redistribute_callback,
			   NULL);
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);
        gtk_widget_add_accelerator(menuitem,
				   "activate",
				   accel_group,
				   'C', GDK_CONTROL_MASK,
				   GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

	/* Selection ops */

	menuitem = gtk_menu_item_new();
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);

	menuitem = gtk_menu_item_new_with_label(_("Selection operations"));
	g_editor->control_sel_ops_popup = cpopup_create_sel_ops_menu();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), g_editor->control_sel_ops_popup);
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);

	/* Done */

	g_editor->control_main_popup = menu;
} /* cpopup_create_main_menu */


/*****/

static void
cpopup_do_popup(void)
{
	cpopup_adjust_menus();
	gtk_menu_popup(GTK_MENU(g_editor->control_main_popup), NULL, NULL, NULL, NULL, 3, 0);
} /* cpopup_do_popup */


/*****/

static GtkWidget *
cpopup_create_color_item(GtkWidget **color_box, GtkWidget **label)
{
	GtkWidget *menuitem;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *wcolor_box;
	GtkWidget *wlabel;

	menuitem = gtk_menu_item_new();

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(menuitem), hbox);
	gtk_widget_show(hbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
	gtk_widget_show(vbox);

	wcolor_box = gtk_preview_new(GTK_PREVIEW_COLOR);
	gtk_preview_set_dither (GTK_PREVIEW (wcolor_box), GDK_RGB_DITHER_MAX);
	gtk_preview_size(GTK_PREVIEW(wcolor_box), GRAD_COLOR_BOX_WIDTH, GRAD_COLOR_BOX_HEIGHT);
	gtk_box_pack_start(GTK_BOX(vbox), wcolor_box, FALSE, FALSE, 2);
	gtk_widget_show(wcolor_box);

	if (color_box)
		*color_box = wcolor_box;

	wlabel = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(wlabel), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), wlabel, TRUE, TRUE, 4);
	gtk_widget_show(wlabel);

	if (label)
		*label = wlabel;

	return menuitem;
} /* cpopup_create_color_item */


/*****/

static void
cpopup_adjust_menus(void)
{
	grad_segment_t *seg;
	int             i;
	double          fg_r, fg_g, fg_b;
	double          fg_a;

	/* Render main menu color boxes */

	cpopup_render_color_box(GTK_PREVIEW(g_editor->left_color_preview),
				g_editor->control_sel_l->r0,
				g_editor->control_sel_l->g0,
				g_editor->control_sel_l->b0,
				g_editor->control_sel_l->a0);

	cpopup_render_color_box(GTK_PREVIEW(g_editor->right_color_preview),
				g_editor->control_sel_r->r1,
				g_editor->control_sel_r->g1,
				g_editor->control_sel_r->b1,
				g_editor->control_sel_r->a1);

	/* Render load color from endpoint color boxes */

	if (g_editor->control_sel_l->prev != NULL)
		seg = g_editor->control_sel_l->prev;
	else
		seg = seg_get_last_segment(g_editor->control_sel_l);

	cpopup_render_color_box(GTK_PREVIEW(g_editor->left_load_color_boxes[0]),
				seg->r1,
				seg->g1,
				seg->b1,
				seg->a1);

	cpopup_render_color_box(GTK_PREVIEW(g_editor->left_load_color_boxes[1]),
				g_editor->control_sel_r->r1,
				g_editor->control_sel_r->g1,
				g_editor->control_sel_r->b1,
				g_editor->control_sel_r->a1);

	if (g_editor->control_sel_r->next != NULL)
		seg = g_editor->control_sel_r->next;
	else
		seg = curr_gradient->segments;

	cpopup_render_color_box(GTK_PREVIEW(g_editor->right_load_color_boxes[0]),
				seg->r0,
				seg->g0,
				seg->b0,
				seg->a0);

	cpopup_render_color_box(GTK_PREVIEW(g_editor->right_load_color_boxes[1]),
				g_editor->control_sel_l->r0,
				g_editor->control_sel_l->g0,
				g_editor->control_sel_l->b0,
				g_editor->control_sel_l->a0);

	/* Render Foreground color boxes */

	ed_fetch_foreground(&fg_r, &fg_g, &fg_b, &fg_a);

	cpopup_render_color_box(GTK_PREVIEW(g_editor->left_load_color_boxes[2]),
				fg_r,
				fg_g,
				fg_b,
				fg_a);

	cpopup_render_color_box(GTK_PREVIEW(g_editor->right_load_color_boxes[2]),
				fg_r,
				fg_g,
				fg_b,
				fg_a);
	
	/* Render saved color boxes */

	for (i = 0; i < GRAD_NUM_COLORS; i++)
		cpopup_update_saved_color(i,
					  g_editor->saved_colors[i].r,
					  g_editor->saved_colors[i].g,
					  g_editor->saved_colors[i].b,
					  g_editor->saved_colors[i].a);

	/* Adjust labels */

	if (g_editor->control_sel_l == g_editor->control_sel_r) {
		gtk_label_set(GTK_LABEL(g_editor->control_blending_label),
			      _("Blending function for segment"));
		gtk_label_set(GTK_LABEL(g_editor->control_coloring_label),
			      _("Coloring type for segment"));
		gtk_label_set(GTK_LABEL(g_editor->control_splitm_label),
			      _("Split segment at midpoint"));
		gtk_label_set(GTK_LABEL(g_editor->control_splitu_label),
			      _("Split segment uniformly"));
		gtk_label_set(GTK_LABEL(g_editor->control_delete_label),
			      _("Delete segment"));
		gtk_label_set(GTK_LABEL(g_editor->control_recenter_label),
			      _("Re-center segment's midpoint"));
		gtk_label_set(GTK_LABEL(g_editor->control_redistribute_label),
			      _("Re-distribute handles in segment"));
		gtk_label_set(GTK_LABEL(g_editor->control_flip_label),
			      _("Flip segment"));
		gtk_label_set(GTK_LABEL(g_editor->control_replicate_label),
			      _("Replicate segment"));
	} else {
		gtk_label_set(GTK_LABEL(g_editor->control_blending_label),
			      _("Blending function for selection"));
		gtk_label_set(GTK_LABEL(g_editor->control_coloring_label),
			      _("Coloring type for selection"));
		gtk_label_set(GTK_LABEL(g_editor->control_splitm_label),
			      _("Split segments at midpoints"));
		gtk_label_set(GTK_LABEL(g_editor->control_splitu_label),
			      _("Split segments uniformly"));
		gtk_label_set(GTK_LABEL(g_editor->control_delete_label),
			      _("Delete selection"));
		gtk_label_set(GTK_LABEL(g_editor->control_recenter_label),
			      _("Re-center midpoints in selection"));
		gtk_label_set(GTK_LABEL(g_editor->control_redistribute_label),
			      _("Re-distribute handles in selection"));
		gtk_label_set(GTK_LABEL(g_editor->control_flip_label),
			      _("Flip selection"));
		gtk_label_set(GTK_LABEL(g_editor->control_replicate_label),
			      _("Replicate selection"));
	} /* else */

	/* Adjust blending and coloring menus */

	cpopup_adjust_blending_menu();
	cpopup_adjust_coloring_menu();

	/* Can invoke delete? */

	if ((g_editor->control_sel_l->prev == NULL) && (g_editor->control_sel_r->next == NULL))
		gtk_widget_set_sensitive(g_editor->control_delete_menu_item, FALSE);
	else
		gtk_widget_set_sensitive(g_editor->control_delete_menu_item, TRUE);

	/* Can invoke blend colors / opacity? */

	if (g_editor->control_sel_l == g_editor->control_sel_r) {
		gtk_widget_set_sensitive(g_editor->control_blend_colors_menu_item, FALSE);
		gtk_widget_set_sensitive(g_editor->control_blend_opacity_menu_item, FALSE);
	} else {
		gtk_widget_set_sensitive(g_editor->control_blend_colors_menu_item, TRUE);
		gtk_widget_set_sensitive(g_editor->control_blend_opacity_menu_item, TRUE);
	} /* else */
} /* cpopup_adjust_menus */


/*****/

static void
cpopup_adjust_blending_menu(void)
{
	int  equal;
	long i, num_items;
	int  type;

	cpopup_check_selection_params(&equal, NULL);

	/* Block activate signals */

	num_items = sizeof(g_editor->control_blending_items) / sizeof(g_editor->control_blending_items[0]);

	type = (int) g_editor->control_sel_l->type;

	for (i = 0; i < num_items; i++)
		gtk_signal_handler_block_by_data(GTK_OBJECT(g_editor->control_blending_items[i]),
						 (gpointer) i);

	/* Set state */

	if (equal) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(g_editor->control_blending_items[type]),
					      TRUE);
		gtk_widget_hide(g_editor->control_blending_items[num_items - 1]);
	} else {
		gtk_widget_show(g_editor->control_blending_items[num_items - 1]);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(g_editor->control_blending_items
								  [num_items - 1]),
					      TRUE);
	} /* else */

	/* Unblock signals */

	for (i = 0; i < num_items; i++)
		gtk_signal_handler_unblock_by_data(GTK_OBJECT(g_editor->control_blending_items[i]),
						   (gpointer) i);
} /* cpopup_adjust_blending_menu */


/*****/

static void
cpopup_adjust_coloring_menu(void)
{
	int  equal;
	long i, num_items;
	int  coloring;

	cpopup_check_selection_params(NULL, &equal);

	/* Block activate signals */

	num_items = sizeof(g_editor->control_coloring_items) / sizeof(g_editor->control_coloring_items[0]);

	coloring = (int) g_editor->control_sel_l->color;

	for (i = 0; i < num_items; i++)
		gtk_signal_handler_block_by_data(GTK_OBJECT(g_editor->control_coloring_items[i]),
						 (gpointer) i);

	/* Set state */

	if (equal) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(g_editor->control_coloring_items
								  [coloring]),
					      TRUE);
		gtk_widget_hide(g_editor->control_coloring_items[num_items - 1]);
	} else {
		gtk_widget_show(g_editor->control_coloring_items[num_items - 1]);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(g_editor->control_coloring_items
								  [num_items - 1]),
					      TRUE);
	} /* else */

	/* Unblock signals */

	for (i = 0; i < num_items; i++)
		gtk_signal_handler_unblock_by_data(GTK_OBJECT(g_editor->control_coloring_items[i]),
						   (gpointer) i);
} /* cpopup_adjust_coloring_menu */


/*****/

static void
cpopup_check_selection_params(int *equal_blending, int *equal_coloring)
{
	grad_type_t     type;
	grad_color_t    color;
	int             etype, ecolor;
	grad_segment_t *seg, *aseg;

	type  = g_editor->control_sel_l->type;
	color = g_editor->control_sel_l->color;

	etype  = 1;
	ecolor = 1;

	seg = g_editor->control_sel_l;

	do {
		etype  = etype && (seg->type == type);
		ecolor = ecolor && (seg->color == color);

		aseg = seg;
		seg  = seg->next;
	} while (aseg != g_editor->control_sel_r);

	if (equal_blending)
		*equal_blending = etype;

	if (equal_coloring)
		*equal_coloring = ecolor;
} /* cpopup_check_selection_params */


/*****/

static GtkWidget *
cpopup_create_menu_item_with_label(char *str, GtkWidget **label)
{
	GtkWidget *menuitem;
	GtkWidget *accel_label;

	menuitem = gtk_menu_item_new();

	accel_label = gtk_accel_label_new(str);
	gtk_misc_set_alignment(GTK_MISC(accel_label), 0.0, 0.5);
	gtk_container_add(GTK_CONTAINER(menuitem), accel_label);
	gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (accel_label), menuitem);
	gtk_widget_show(accel_label);

	if (label)
		*label = accel_label;

	return menuitem;
} /* cpopup_create_menu_item_with_label */


/*****/

static void
cpopup_render_color_box(GtkPreview *preview, double r, double g, double b, double a)
{
	guchar  rows[3][GRAD_COLOR_BOX_WIDTH * 3];
	int     x, y;
	int     r0, g0, b0;
	int     r1, g1, b1;
	guchar *p0, *p1, *p2;

	/* Fill rows */

	r0 = (GRAD_CHECK_DARK + (r - GRAD_CHECK_DARK) * a) * 255.0;
	r1 = (GRAD_CHECK_LIGHT + (r - GRAD_CHECK_LIGHT) * a) * 255.0;

	g0 = (GRAD_CHECK_DARK + (g - GRAD_CHECK_DARK) * a) * 255.0;
	g1 = (GRAD_CHECK_LIGHT + (g - GRAD_CHECK_LIGHT) * a) * 255.0;

	b0 = (GRAD_CHECK_DARK + (b - GRAD_CHECK_DARK) * a) * 255.0;
	b1 = (GRAD_CHECK_LIGHT + (b - GRAD_CHECK_LIGHT) * a) * 255.0;

	p0 = rows[0];
	p1 = rows[1];
	p2 = rows[2];

	for (x = 0; x < GRAD_COLOR_BOX_WIDTH; x++) {
		if ((x == 0) || (x == (GRAD_COLOR_BOX_WIDTH - 1))) {
			*p0++ = 0;
			*p0++ = 0;
			*p0++ = 0;

			*p1++ = 0;
			*p1++ = 0;
			*p1++ = 0;
		} else
			if ((x / GRAD_CHECK_SIZE) & 1) {
				*p0++ = r1;
				*p0++ = g1;
				*p0++ = b1;

				*p1++ = r0;
				*p1++ = g0;
				*p1++ = b0;
			} else {
				*p0++ = r0;
				*p0++ = g0;
				*p0++ = b0;

				*p1++ = r1;
				*p1++ = g1;
				*p1++ = b1;
			} /* else */

		*p2++ = 0;
		*p2++ = 0;
		*p2++ = 0;
	} /* for */

	/* Fill preview */

	gtk_preview_draw_row(preview, rows[2], 0, 0, GRAD_COLOR_BOX_WIDTH);

	for (y = 1; y < (GRAD_COLOR_BOX_HEIGHT - 1); y++)
		if ((y / GRAD_CHECK_SIZE) & 1)
			gtk_preview_draw_row(preview, rows[1], 0, y, GRAD_COLOR_BOX_WIDTH);
		else
			gtk_preview_draw_row(preview, rows[0], 0, y, GRAD_COLOR_BOX_WIDTH);

	gtk_preview_draw_row(preview, rows[2], 0, y, GRAD_COLOR_BOX_WIDTH);
} /* cpopup_render_color_box */


/*****/

static GtkWidget *
cpopup_create_load_menu(GtkWidget **color_boxes, GtkWidget **labels,
			char *label1, char *label2, GtkSignalFunc callback,
			gchar accel_key_0, guint8 accel_mods_0,
			gchar accel_key_1, guint8 accel_mods_1,
			gchar accel_key_2, guint8 accel_mods_2)
{
	GtkWidget           *menu;
	GtkWidget           *menuitem;
	GtkAccelGroup       *accel_group;
	int                  i;

	menu      = gtk_menu_new();
	accel_group = g_editor->accel_group;

	gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);

	/* Create items */

	for (i = 0; i < (GRAD_NUM_COLORS + 3); i++) {
		if (i == 3) {
			/* Insert separator between "to fetch" and "saved" colors */

			menuitem = gtk_menu_item_new();
			gtk_menu_append(GTK_MENU(menu), menuitem);
			gtk_widget_show(menuitem);
		} /* if */

		menuitem = cpopup_create_color_item(&color_boxes[i], &labels[i]);
		gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
				   callback, (gpointer) ((long) i)); /* FIXME: I don't like this cast */
		gtk_menu_append(GTK_MENU(menu), menuitem);
		gtk_widget_show(menuitem);

		switch (i) {
		case 0:
		  gtk_widget_add_accelerator(menuitem,
					     "activate",
					     accel_group,
					     accel_key_0, accel_mods_0,
					     GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);
		  break;
		  
		case 1:
		  gtk_widget_add_accelerator(menuitem,
					     "activate",
					     accel_group,
					     accel_key_1, accel_mods_1,
					     GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);
		  break;
		  
		case 2:
		  gtk_widget_add_accelerator(menuitem,
					     "activate",
					     accel_group,
					     accel_key_2, accel_mods_2,
					     GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);
		  break;
		  
		default:
		  break;
		} /* switch */
	} /* for */

	/* Set labels */

	gtk_label_set(GTK_LABEL(labels[0]), label1);
	gtk_label_set(GTK_LABEL(labels[1]), label2);
	gtk_label_set(GTK_LABEL(labels[2]), _("FG color"));

	return menu;
} /* cpopup_create_load_menu */


/*****/

static GtkWidget *
cpopup_create_save_menu(GtkWidget **color_boxes, GtkWidget **labels, GtkSignalFunc callback)
{
	GtkWidget *menu;
	GtkWidget *menuitem;
	int        i;

	menu = gtk_menu_new();

	for (i = 0; i < GRAD_NUM_COLORS; i++) {
		menuitem = cpopup_create_color_item(&color_boxes[i], &labels[i]);
		gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
				   callback, (gpointer) ((long) i)); /* FIXME: I don't like this cast */
		gtk_menu_append(GTK_MENU(menu), menuitem);
		gtk_widget_show(menuitem);
	} /* for */

	return menu;
} /* cpopup_create_save_menu */


/*****/

static void
cpopup_update_saved_color(int n, double r, double g, double b, double a)
{
	char str[256];

	cpopup_render_color_box(GTK_PREVIEW(g_editor->left_load_color_boxes[n + 3]),
				r, g, b, a);
	cpopup_render_color_box(GTK_PREVIEW(g_editor->left_save_color_boxes[n]),
				r, g, b, a);
	cpopup_render_color_box(GTK_PREVIEW(g_editor->right_load_color_boxes[n + 3]),
				r, g, b, a);
	cpopup_render_color_box(GTK_PREVIEW(g_editor->right_save_color_boxes[n]),
				r, g, b, a);

	g_snprintf(str, sizeof(str), _("RGBA (%0.3f, %0.3f, %0.3f, %0.3f)"), r, g, b, a);

	gtk_label_set(GTK_LABEL(g_editor->left_load_labels[n + 3]), str);
	gtk_label_set(GTK_LABEL(g_editor->left_save_labels[n]), str);
	gtk_label_set(GTK_LABEL(g_editor->right_load_labels[n + 3]), str);
	gtk_label_set(GTK_LABEL(g_editor->right_save_labels[n]), str);

	g_editor->saved_colors[n].r = r;
	g_editor->saved_colors[n].g = g;
	g_editor->saved_colors[n].b = b;
	g_editor->saved_colors[n].a = a;
} /* cpopup_update_saved_color */


/*****/

static void
cpopup_load_left_callback(GtkWidget *widget, gpointer data)
{
	grad_segment_t *seg;
	double          fg_r, fg_g, fg_b;
	double          fg_a;

	switch ((long) data) {
		case 0: /* Fetch from left neighbor's right endpoint */
			if (g_editor->control_sel_l->prev != NULL)
				seg = g_editor->control_sel_l->prev;
			else
				seg = seg_get_last_segment(g_editor->control_sel_l);

			cpopup_blend_endpoints(seg->r1, seg->g1, seg->b1, seg->a1,
					       g_editor->control_sel_r->r1,
					       g_editor->control_sel_r->g1,
					       g_editor->control_sel_r->b1,
					       g_editor->control_sel_r->a1,
					       TRUE, TRUE);
			break;

		case 1: /* Fetch from right endpoint */
			cpopup_blend_endpoints(g_editor->control_sel_r->r1,
					       g_editor->control_sel_r->g1,
					       g_editor->control_sel_r->b1,
					       g_editor->control_sel_r->a1,
					       g_editor->control_sel_r->r1,
					       g_editor->control_sel_r->g1,
					       g_editor->control_sel_r->b1,
					       g_editor->control_sel_r->a1,
					       TRUE, TRUE);
			break;

		case 2: /* Fetch from FG color */
	                ed_fetch_foreground(&fg_r, &fg_g, &fg_b, &fg_a);
			cpopup_blend_endpoints(fg_r,
					       fg_g,
					       fg_b,
					       fg_a,
					       g_editor->control_sel_r->r1,
					       g_editor->control_sel_r->g1,
					       g_editor->control_sel_r->b1,
					       g_editor->control_sel_r->a1,
					       TRUE, TRUE);
			break;

		default: /* Load a color */
			cpopup_blend_endpoints(g_editor->saved_colors[(long) data - 3].r,
					       g_editor->saved_colors[(long) data - 3].g,
					       g_editor->saved_colors[(long) data - 3].b,
					       g_editor->saved_colors[(long) data - 3].a,
					       g_editor->control_sel_r->r1,
					       g_editor->control_sel_r->g1,
					       g_editor->control_sel_r->b1,
					       g_editor->control_sel_r->a1,
					       TRUE, TRUE);
			break;
	} /* switch */

	curr_gradient->dirty = 1;
	ed_update_editor(GRAD_UPDATE_PREVIEW);
} /* cpopup_load_left_callback */


/*****/

static void
cpopup_save_left_callback(GtkWidget *widget, gpointer data)
{
	g_editor->saved_colors[(long) data].r = g_editor->control_sel_l->r0;
	g_editor->saved_colors[(long) data].g = g_editor->control_sel_l->g0;
	g_editor->saved_colors[(long) data].b = g_editor->control_sel_l->b0;
	g_editor->saved_colors[(long) data].a = g_editor->control_sel_l->a0;
} /* cpopup_save_left_callback */


/*****/

static void
cpopup_load_right_callback(GtkWidget *widget, gpointer data)
{
	grad_segment_t *seg;
	double          fg_r, fg_g, fg_b;
	double          fg_a;

	switch ((long) data) {
		case 0: /* Fetch from right neighbor's left endpoint */
			if (g_editor->control_sel_r->next != NULL)
				seg = g_editor->control_sel_r->next;
			else
				seg = curr_gradient->segments;

			cpopup_blend_endpoints(g_editor->control_sel_r->r0,
					       g_editor->control_sel_r->g0,
					       g_editor->control_sel_r->b0,
					       g_editor->control_sel_r->a0,
					       seg->r0, seg->g0, seg->b0, seg->a0,
					       TRUE, TRUE);
			break;

		case 1: /* Fetch from left endpoint */
			cpopup_blend_endpoints(g_editor->control_sel_l->r0,
					       g_editor->control_sel_l->g0,
					       g_editor->control_sel_l->b0,
					       g_editor->control_sel_l->a0,
					       g_editor->control_sel_l->r0,
					       g_editor->control_sel_l->g0,
					       g_editor->control_sel_l->b0,
					       g_editor->control_sel_l->a0,
					       TRUE, TRUE);
			break;

		case 2: /* Fetch from FG color */
	                ed_fetch_foreground(&fg_r, &fg_g, &fg_b, &fg_a);
			cpopup_blend_endpoints(g_editor->control_sel_l->r0,
					       g_editor->control_sel_l->g0,
					       g_editor->control_sel_l->b0,
					       g_editor->control_sel_l->a0,
					       fg_r,
					       fg_g,
					       fg_b,
					       fg_a,
					       TRUE, TRUE);
			break;

		default: /* Load a color */
			cpopup_blend_endpoints(g_editor->control_sel_l->r0,
					       g_editor->control_sel_l->g0,
					       g_editor->control_sel_l->b0,
					       g_editor->control_sel_l->a0,
					       g_editor->saved_colors[(long) data - 3].r,
					       g_editor->saved_colors[(long) data - 3].g,
					       g_editor->saved_colors[(long) data - 3].b,
					       g_editor->saved_colors[(long) data - 3].a,
					       TRUE, TRUE);
			break;
	} /* switch */

	curr_gradient->dirty = 1;
	ed_update_editor(GRAD_UPDATE_PREVIEW);
} /* cpopup_load_right_callback */


/*****/

static void
cpopup_save_right_callback(GtkWidget *widget, gpointer data)
{
	g_editor->saved_colors[(long) data].r = g_editor->control_sel_r->r1;
	g_editor->saved_colors[(long) data].g = g_editor->control_sel_r->g1;
	g_editor->saved_colors[(long) data].b = g_editor->control_sel_r->b1;
	g_editor->saved_colors[(long) data].a = g_editor->control_sel_r->a1;
} /* cpopup_save_right_callback */


/*****/

static GtkWidget *
cpopup_create_blending_menu(void)
{
	GtkWidget *menu;
	GtkWidget *menuitem;
	GSList    *group;
	int        i;
	int        num_items;

	menu  = gtk_menu_new();
	group = NULL;

	num_items = sizeof(g_editor->control_blending_items) / sizeof(g_editor->control_blending_items[0]);

	for (i = 0; i < num_items; i++) {
		if (i == (num_items - 1))
			menuitem = gtk_radio_menu_item_new_with_label(group, _("(Varies)"));
		else
			menuitem = gtk_radio_menu_item_new_with_label(group, gettext(blending_types[i]));

		group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(menuitem));

		gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
				   (GtkSignalFunc) cpopup_blending_callback,
				   (gpointer) ((long) i)); /* FIXME: I don't like this cast */

		gtk_menu_append(GTK_MENU(menu), menuitem);
		gtk_widget_show(menuitem);

		g_editor->control_blending_items[i] = menuitem;
	} /* for */

	/* "Varies" is always disabled */

	gtk_widget_set_sensitive(g_editor->control_blending_items[num_items - 1], FALSE);

	return menu;
} /* cpopup_create_blending_menu */


/*****/

static void
cpopup_blending_callback(GtkWidget *widget, gpointer data)
{
	grad_type_t     type;
	grad_segment_t *seg, *aseg;

	if (!GTK_CHECK_MENU_ITEM(widget)->active)
		return; /* Do nothing if the menu item is being deactivated */

	type = (grad_type_t) data;
	seg  = g_editor->control_sel_l;

	do {
		seg->type = type;

		aseg = seg;
		seg  = seg->next;
	} while (aseg != g_editor->control_sel_r);

	curr_gradient->dirty = 1;
	ed_update_editor(GRAD_UPDATE_PREVIEW);
} /* cpopup_blending_callback */


/*****/

static GtkWidget *
cpopup_create_coloring_menu(void)
{
	GtkWidget *menu;
	GtkWidget *menuitem;
	GSList    *group;
	int        i;
	int        num_items;

	menu  = gtk_menu_new();
	group = NULL;

	num_items = sizeof(g_editor->control_coloring_items) / sizeof(g_editor->control_coloring_items[0]);

	for (i = 0; i < num_items; i++) {
		if (i == (num_items - 1))
			menuitem = gtk_radio_menu_item_new_with_label(group, _("(Varies)"));
		else
			menuitem = gtk_radio_menu_item_new_with_label(group, gettext(coloring_types[i]));

		group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(menuitem));

		gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
				   (GtkSignalFunc) cpopup_coloring_callback,
				   (gpointer) ((long) i)); /* FIXME: I don't like this cast */

		gtk_menu_append(GTK_MENU(menu), menuitem);
		gtk_widget_show(menuitem);

		g_editor->control_coloring_items[i] = menuitem;
	} /* for */

	/* "Varies" is always disabled */

	gtk_widget_set_sensitive(g_editor->control_coloring_items[num_items - 1], FALSE);

	return menu;
} /* cpopup_create_coloring_menu */


/*****/

static void
cpopup_coloring_callback(GtkWidget *widget, gpointer data)
{
	grad_color_t    color;
	grad_segment_t *seg, *aseg;

	if (!GTK_CHECK_MENU_ITEM(widget)->active)
		return; /* Do nothing if the menu item is being deactivated */

	color = (grad_color_t) data;
	seg   = g_editor->control_sel_l;

	do {
		seg->color = color;

		aseg = seg;
		seg  = seg->next;
	} while (aseg != g_editor->control_sel_r);

	curr_gradient->dirty = 1;
	ed_update_editor(GRAD_UPDATE_PREVIEW);
} /* cpopup_coloring_callback */


/*****/

static GtkWidget *
cpopup_create_sel_ops_menu(void)
{
	GtkWidget           *menu;
	GtkWidget           *menuitem;
	GtkAccelGroup       *accel_group;

	menu      = gtk_menu_new();
	accel_group = g_editor->accel_group;

	gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);

	/* Flip */

	menuitem = cpopup_create_menu_item_with_label("", &g_editor->control_flip_label);
	gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			   (GtkSignalFunc) cpopup_flip_callback,
			   NULL);
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);
        gtk_widget_add_accelerator(menuitem,
				   "activate",
				   accel_group,
				   'F', 0,
				   GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

	/* Replicate */

	menuitem = cpopup_create_menu_item_with_label("", &g_editor->control_replicate_label);
	gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			   (GtkSignalFunc) cpopup_replicate_callback,
			   NULL);
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);
        gtk_widget_add_accelerator(menuitem,
				   "activate",
				   accel_group,
				   'M', 0,
				   GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

	/* Blend colors / opacity */

	menuitem = gtk_menu_item_new();
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);

	menuitem = gtk_menu_item_new_with_label(_("Blend endpoints' colors"));
	gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			   (GtkSignalFunc) cpopup_blend_colors,
			   NULL);
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);
	g_editor->control_blend_colors_menu_item = menuitem;
        gtk_widget_add_accelerator(menuitem,
				   "activate",
				   accel_group,
				   'B', 0,
				   GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);

	menuitem = gtk_menu_item_new_with_label(_("Blend endpoints' opacity"));
	gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
			   (GtkSignalFunc) cpopup_blend_opacity,
			   NULL);
	gtk_menu_append(GTK_MENU(menu), menuitem);
	gtk_widget_show(menuitem);
        gtk_widget_add_accelerator(menuitem,
				   "activate",
				   accel_group,
				   'B', GDK_CONTROL_MASK,
				   GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);
	g_editor->control_blend_opacity_menu_item = menuitem;

	return menu;
} /* cpopup_create_sel_ops_menu */


/*****/

static void
cpopup_blend_colors(GtkWidget *widget, gpointer data)
{
	cpopup_blend_endpoints(g_editor->control_sel_l->r0,
			       g_editor->control_sel_l->g0,
			       g_editor->control_sel_l->b0,
			       g_editor->control_sel_l->a0,
			       g_editor->control_sel_r->r1,
			       g_editor->control_sel_r->g1,
			       g_editor->control_sel_r->b1,
			       g_editor->control_sel_r->a1,
			       TRUE, FALSE);

	curr_gradient->dirty = 1;
	ed_update_editor(GRAD_UPDATE_PREVIEW);
} /* cpopup_blend_colors */


/*****/

static void
cpopup_blend_opacity(GtkWidget *widget, gpointer data)
{
	cpopup_blend_endpoints(g_editor->control_sel_l->r0,
			       g_editor->control_sel_l->g0,
			       g_editor->control_sel_l->b0,
			       g_editor->control_sel_l->a0,
			       g_editor->control_sel_r->r1,
			       g_editor->control_sel_r->g1,
			       g_editor->control_sel_r->b1,
			       g_editor->control_sel_r->a1,
			       FALSE, TRUE);

	curr_gradient->dirty = 1;
	ed_update_editor(GRAD_UPDATE_PREVIEW);
} /* cpopup_blend_opacity */


/*****/

static void
cpopup_set_color_selection_color(GtkColorSelection *cs,
				 double r, double g, double b, double a)
{
	gdouble color[4];

	color[0] = r;
	color[1] = g;
	color[2] = b;
	color[3] = a;

	gtk_color_selection_set_color(cs, color);
} /* cpopup_set_color_selection_color */


/*****/

static void
cpopup_get_color_selection_color(GtkColorSelection *cs,
				 double *r, double *g, double *b, double *a)
{
	gdouble color[4];

	gtk_color_selection_get_color(cs, color);

	*r = color[0];
	*g = color[1];
	*b = color[2];
	*a = color[3];
} /* cpopup_get_color_selection_color */


/*****/

static void
cpopup_create_color_dialog(char *title, double r, double g, double b, double a,
			   GtkSignalFunc color_changed_callback,
			   GtkSignalFunc ok_callback,
			   GtkSignalFunc cancel_callback,
			   GtkSignalFunc delete_callback)
{
	GtkWidget               *window;
	GtkColorSelection       *cs;
	GtkColorSelectionDialog *csd;

	window = gtk_color_selection_dialog_new(title);

	csd = GTK_COLOR_SELECTION_DIALOG(window);
	cs  = GTK_COLOR_SELECTION(csd->colorsel);

	gtk_color_selection_set_opacity(cs, TRUE);
	gtk_color_selection_set_update_policy(cs,
					      g_editor->instant_update ?
					      GTK_UPDATE_CONTINUOUS :
					      GTK_UPDATE_DELAYED);


	/* FIXME: this is a hack; we set the color twice so that the
         * color selector remembers it as its "old" color, too
	 */

	cpopup_set_color_selection_color(cs, r, g, b, a);
	cpopup_set_color_selection_color(cs, r, g, b, a);

	gtk_signal_connect(GTK_OBJECT(csd), "delete_event",
			   delete_callback, NULL);

	gtk_signal_connect(GTK_OBJECT(cs), "color_changed",
			   color_changed_callback, window);

	gtk_signal_connect(GTK_OBJECT(csd->ok_button), "clicked",
			   ok_callback, window);

	gtk_signal_connect(GTK_OBJECT(csd->cancel_button), "clicked",
			   cancel_callback, window);

	gtk_window_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE);
	gtk_widget_show(window);
} /* cpopup_create_color_dialog */


/*****/

static grad_segment_t *
cpopup_save_selection(void)
{
	grad_segment_t *seg, *prev, *tmp;
	grad_segment_t *oseg, *oaseg;

	prev = NULL;
	oseg = g_editor->control_sel_l;
	tmp  = NULL;

	do {
		seg = seg_new_segment();

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
	} while (oaseg != g_editor->control_sel_r);

	return tmp;
} /* cpopup_save_selection */


/*****/

static void
cpopup_free_selection(grad_segment_t *seg)
{
	seg_free_segments(seg);
} /* cpopup_free_selection */


/*****/

static void
cpopup_replace_selection(grad_segment_t *replace_seg)
{
	grad_segment_t *lseg, *rseg;
	grad_segment_t *replace_last;

	/* Remember left and right segments */

	lseg = g_editor->control_sel_l->prev;
	rseg = g_editor->control_sel_r->next;

	replace_last = seg_get_last_segment(replace_seg);

	/* Free old selection */

	g_editor->control_sel_r->next = NULL;

	seg_free_segments(g_editor->control_sel_l);

	/* Link in new segments */

	if (lseg)
		lseg->next = replace_seg;
	else
		curr_gradient->segments = replace_seg;

	replace_seg->prev = lseg;

	if (rseg)
		rseg->prev = replace_last;

	replace_last->next = rseg;

	g_editor->control_sel_l = replace_seg;
	g_editor->control_sel_r = replace_last;

	curr_gradient->last_visited = NULL; /* Force re-search */
} /* cpopup_replace_selection */


/*****/

static void
cpopup_set_left_color_callback(GtkWidget *widget, gpointer data)
{
	g_editor->left_saved_dirty    = curr_gradient->dirty;
	g_editor->left_saved_segments = cpopup_save_selection();

	cpopup_create_color_dialog(_("Left endpoint's color"),
				   g_editor->control_sel_l->r0,
				   g_editor->control_sel_l->g0,
				   g_editor->control_sel_l->b0,
				   g_editor->control_sel_l->a0,
				   (GtkSignalFunc) cpopup_left_color_changed,
				   (GtkSignalFunc) cpopup_left_color_dialog_ok,
				   (GtkSignalFunc) cpopup_left_color_dialog_cancel,
				   (GtkSignalFunc) cpopup_left_color_dialog_delete);

	gtk_widget_set_sensitive(g_editor->shell, FALSE);
} /* cpopup_set_left_color_callback */


/*****/

static void
cpopup_left_color_changed(GtkWidget *widget, gpointer client_data)
{
	GtkColorSelection *cs;
	double             r, g, b, a;

	cs = GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(client_data)->colorsel);

	cpopup_get_color_selection_color(cs, &r, &g, &b, &a);

	cpopup_blend_endpoints(r, g, b, a,
			       g_editor->control_sel_r->r1,
			       g_editor->control_sel_r->g1,
			       g_editor->control_sel_r->b1,
			       g_editor->control_sel_r->a1,
			       TRUE, TRUE);

	ed_update_editor(GRAD_UPDATE_PREVIEW);
} /* cpopup_left_color_changed */


/*****/

static void
cpopup_left_color_dialog_ok(GtkWidget *widget, gpointer client_data)
{
	GtkColorSelection *cs;
	double             r, g, b, a;

	cs = GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(client_data)->colorsel);

	cpopup_get_color_selection_color(cs, &r, &g, &b, &a);

	cpopup_blend_endpoints(r, g, b, a,
			       g_editor->control_sel_r->r1,
			       g_editor->control_sel_r->g1,
			       g_editor->control_sel_r->b1,
			       g_editor->control_sel_r->a1,
			       TRUE, TRUE);

	curr_gradient->dirty = 1;
	cpopup_free_selection(g_editor->left_saved_segments);
	ed_update_editor(GRAD_UPDATE_PREVIEW);

	gtk_widget_destroy(GTK_WIDGET(client_data));
	gtk_widget_set_sensitive(g_editor->shell, TRUE);
} /* cpopup_left_color_dialog_ok */


/*****/

static void
cpopup_left_color_dialog_cancel(GtkWidget *widget, gpointer client_data)
{
	curr_gradient->dirty = g_editor->left_saved_dirty;
	cpopup_replace_selection(g_editor->left_saved_segments);
	ed_update_editor(GRAD_UPDATE_PREVIEW);

	gtk_widget_destroy(GTK_WIDGET(client_data));
	gtk_widget_set_sensitive(g_editor->shell, TRUE);
} /* cpopup_left_color_dialog_cancel */


/*****/

static int
cpopup_left_color_dialog_delete(GtkWidget *widget, GdkEvent *event,
				gpointer data)
{
	curr_gradient->dirty = g_editor->left_saved_dirty;
	cpopup_replace_selection(g_editor->left_saved_segments);
	ed_update_editor(GRAD_UPDATE_PREVIEW);

	gtk_widget_set_sensitive(g_editor->shell, TRUE);
	return FALSE;
} /* cpopup_left_color_dialog_delete */

/*****/

static void
cpopup_set_right_color_callback(GtkWidget *widget, gpointer data)
{
	g_editor->right_saved_dirty    = curr_gradient->dirty;
	g_editor->right_saved_segments = cpopup_save_selection();

	cpopup_create_color_dialog(_("Right endpoint's color"),
				   g_editor->control_sel_r->r1,
				   g_editor->control_sel_r->g1,
				   g_editor->control_sel_r->b1,
				   g_editor->control_sel_r->a1,
				   (GtkSignalFunc) cpopup_right_color_changed,
				   (GtkSignalFunc) cpopup_right_color_dialog_ok,
				   (GtkSignalFunc) cpopup_right_color_dialog_cancel,
				   (GtkSignalFunc) cpopup_right_color_dialog_delete);

	gtk_widget_set_sensitive(g_editor->shell, FALSE);
} /* cpopup_set_right_color_callback */


/*****/

static void
cpopup_right_color_changed(GtkWidget *widget, gpointer client_data)
{
	GtkColorSelection *cs;
	double             r, g, b, a;

	cs = GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(client_data)->colorsel);

	cpopup_get_color_selection_color(cs, &r, &g, &b, &a);

	cpopup_blend_endpoints(g_editor->control_sel_l->r0,
			       g_editor->control_sel_l->g0,
			       g_editor->control_sel_l->b0,
			       g_editor->control_sel_l->a0,
			       r, g, b, a,
			       TRUE, TRUE);

	ed_update_editor(GRAD_UPDATE_PREVIEW);
} /* cpopup_right_color_changed */


/*****/

static void
cpopup_right_color_dialog_ok(GtkWidget *widget, gpointer client_data)
{
	GtkColorSelection *cs;
	double             r, g, b, a;

	cs = GTK_COLOR_SELECTION(GTK_COLOR_SELECTION_DIALOG(client_data)->colorsel);

	cpopup_get_color_selection_color(cs, &r, &g, &b, &a);

	cpopup_blend_endpoints(g_editor->control_sel_l->r0,
			       g_editor->control_sel_l->g0,
			       g_editor->control_sel_l->b0,
			       g_editor->control_sel_l->a0,
			       r, g, b, a,
			       TRUE, TRUE);

	curr_gradient->dirty = 1;
	cpopup_free_selection(g_editor->right_saved_segments);
	ed_update_editor(GRAD_UPDATE_PREVIEW);

	gtk_widget_destroy(GTK_WIDGET(client_data));
	gtk_widget_set_sensitive(g_editor->shell, TRUE);
} /* cpopup_right_color_dialog_ok */


/*****/

static void
cpopup_right_color_dialog_cancel(GtkWidget *widget, gpointer client_data)
{
	curr_gradient->dirty = g_editor->right_saved_dirty;
	cpopup_replace_selection(g_editor->right_saved_segments);
	ed_update_editor(GRAD_UPDATE_PREVIEW);

	gtk_widget_destroy(GTK_WIDGET(client_data));
	gtk_widget_set_sensitive(g_editor->shell, TRUE);
} /* cpopup_right_color_dialog_cancel */


/*****/

static int
cpopup_right_color_dialog_delete(GtkWidget *widget, GdkEvent *event,
				 gpointer data)
{
	curr_gradient->dirty = g_editor->right_saved_dirty;
	cpopup_replace_selection(g_editor->right_saved_segments);
	ed_update_editor(GRAD_UPDATE_PREVIEW);

	gtk_widget_set_sensitive(g_editor->shell, TRUE);
	return FALSE;
} /* cpopup_right_color_dialog_delete */


/*****/

static void
cpopup_split_midpoint_callback(GtkWidget *widget, gpointer data)
{
	grad_segment_t *seg, *lseg, *rseg;

	seg = g_editor->control_sel_l;

	do {
		cpopup_split_midpoint(seg, &lseg, &rseg);
		seg = rseg->next;
	} while (lseg != g_editor->control_sel_r);

	g_editor->control_sel_r = rseg;

	curr_gradient->last_visited = NULL; /* Force re-search */
	curr_gradient->dirty = 1;
	ed_update_editor(GRAD_UPDATE_PREVIEW | GRAD_UPDATE_CONTROL);
} /* cpopup_split_midpoint_callback */


/*****/

static void
cpopup_split_uniform_callback(GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *scale;
	GtkWidget *button;
	GtkObject *scale_data;

	/* Create dialog window */

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog),
			     (g_editor->control_sel_l == g_editor->control_sel_r) ?
			     _("Split segment uniformly") :
			     _("Split segments uniformly"));
	gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
	gtk_container_border_width(GTK_CONTAINER(dialog), 0);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 8);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), vbox,
			   FALSE, FALSE, 0);
	gtk_widget_show(vbox);

	/* Instructions */

	label = gtk_label_new(_("Please select the number of uniform parts"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	label = gtk_label_new((g_editor->control_sel_l == g_editor->control_sel_r) ?
			      _("in which you want to split the selected segment") :
			      _("in which you want to split the segments in the selection"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	/* Scale */

	g_editor->split_parts = 2;
	scale_data  = gtk_adjustment_new(2.0, 2.0, 21.0, 1.0, 1.0, 1.0);

	scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
	gtk_scale_set_digits(GTK_SCALE(scale), 0);
	gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_TOP);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, TRUE, 8);
	gtk_widget_show(scale);

	gtk_signal_connect(scale_data, "value_changed",
			   (GtkSignalFunc) cpopup_split_uniform_scale_update,
			   NULL);

	/* Buttons */

	button = ed_create_button(_("Split"), 0.5, 0.5,
				  (GtkSignalFunc) cpopup_split_uniform_split_callback,
				  (gpointer) dialog);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
			   button, TRUE, TRUE, 0);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	button = ed_create_button(_("Cancel"), 0.5, 0.5,
				  (GtkSignalFunc) cpopup_split_uniform_cancel_callback,
				  (gpointer) dialog);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
			   button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	/* Show! */

	gtk_widget_show(dialog);
	gtk_widget_set_sensitive(g_editor->shell, FALSE);
} /* cpopup_split_uniform_callback */


/*****/

static void
cpopup_delete_callback(GtkWidget *widget, gpointer data)
{
	grad_segment_t *lseg, *rseg, *seg, *aseg, *next;
	double          join;

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
		control_compress_range(lseg, lseg, lseg->left, join);

	if (rseg != NULL)
		control_compress_range(rseg, rseg, join, rseg->right);

	/* Link */

	if (lseg)
		lseg->next = rseg;

	if (rseg)
		rseg->prev = lseg;

	/* Delete old segments */

	seg = g_editor->control_sel_l;

	do {
		next = seg->next;
		aseg = seg;

		seg_free_segment(seg);

		seg = next;
	} while (aseg != g_editor->control_sel_r);

	/* Change selection */

	if (rseg) {
		g_editor->control_sel_l = rseg;
		g_editor->control_sel_r = rseg;
	} else {
		g_editor->control_sel_l = lseg;
		g_editor->control_sel_r = lseg;
	} /* else */

	if (lseg == NULL)
		curr_gradient->segments = rseg;

	/* Done */

	curr_gradient->last_visited = NULL; /* Force re-search */
	curr_gradient->dirty = 1;

	ed_update_editor(GRAD_UPDATE_PREVIEW | GRAD_UPDATE_CONTROL);
} /* cpopup_delete_callback */


/*****/

static void
cpopup_recenter_callback(GtkWidget *wiodget, gpointer data)
{
	grad_segment_t *seg, *aseg;

	seg = g_editor->control_sel_l;

	do {
		seg->middle = (seg->left + seg->right) / 2.0;

		aseg = seg;
		seg  = seg->next;
	} while (aseg != g_editor->control_sel_r);

	curr_gradient->dirty = 1;
	ed_update_editor(GRAD_UPDATE_PREVIEW | GRAD_UPDATE_CONTROL);
} /* cpopup_recenter_callback */


/*****/

static void
cpopup_redistribute_callback(GtkWidget *widget, gpointer data)
{
	grad_segment_t *seg, *aseg;
	double          left, right, seg_len;
	int             num_segs;
	int             i;

	/* Count number of segments in selection */

	num_segs = 0;
	seg      = g_editor->control_sel_l;

	do {
		num_segs++;
		aseg = seg;
		seg  = seg->next;
	} while (aseg != g_editor->control_sel_r);

	/* Calculate new segment length */

	left    = g_editor->control_sel_l->left;
	right   = g_editor->control_sel_r->right;
	seg_len = (right - left) / num_segs;

	/* Redistribute */

	seg = g_editor->control_sel_l;

	for (i = 0; i < num_segs; i++) {
		seg->left   = left + i * seg_len;
		seg->right  = left + (i + 1) * seg_len;
		seg->middle = (seg->left + seg->right) / 2.0;

		seg = seg->next;
	} /* for */

	/* Fix endpoints to squish accumulative error */

	g_editor->control_sel_l->left  = left;
	g_editor->control_sel_r->right = right;

	/* Done */

	curr_gradient->dirty = 1;
	ed_update_editor(GRAD_UPDATE_PREVIEW | GRAD_UPDATE_CONTROL);
} /* cpopup_redistribute_callback */


/*****/

static void
cpopup_flip_callback(GtkWidget *widget, gpointer data)
{
	grad_segment_t *oseg, *oaseg;
	grad_segment_t *seg, *prev, *tmp;
	grad_segment_t *lseg, *rseg;
	double          left, right;

	left  = g_editor->control_sel_l->left;
	right = g_editor->control_sel_r->right;

	/* Build flipped segments */

	prev = NULL;
	oseg = g_editor->control_sel_r;
	tmp  = NULL;

	do {
		seg = seg_new_segment();

		if (prev == NULL) {
			seg->left = left;
			tmp = seg; /* Remember first segment */
		} else
			seg->left = left + right - oseg->right;

		seg->middle = left + right - oseg->middle;
		seg->right  = left + right - oseg->left;

		seg->r0 = oseg->r1;
		seg->g0 = oseg->g1;
		seg->b0 = oseg->b1;
		seg->a0 = oseg->a1;

		seg->r1 = oseg->r0;
		seg->g1 = oseg->g0;
		seg->b1 = oseg->b0;
		seg->a1 = oseg->a0;

		switch (oseg->type) {
			case GRAD_SPHERE_INCREASING:
				seg->type = GRAD_SPHERE_DECREASING;
				break;

			case GRAD_SPHERE_DECREASING:
				seg->type = GRAD_SPHERE_INCREASING;
				break;

			default:
				seg->type = oseg->type;
		} /* switch */

		switch (oseg->color) {
			case GRAD_HSV_CCW:
				seg->color = GRAD_HSV_CW;
				break;

			case GRAD_HSV_CW:
				seg->color = GRAD_HSV_CCW;
				break;

			default:
				seg->color = oseg->color;
		} /* switch */

		seg->prev = prev;
		seg->next = NULL;

		if (prev)
			prev->next = seg;

		prev = seg;

		oaseg = oseg;
		oseg  = oseg->prev; /* Move backwards! */
	} while (oaseg != g_editor->control_sel_l);

	seg->right = right; /* Squish accumulative error */

	/* Free old segments */

	lseg = g_editor->control_sel_l->prev;
	rseg = g_editor->control_sel_r->next;

	oseg = g_editor->control_sel_l;

	do {
		oaseg = oseg->next;
		seg_free_segment(oseg);
		oseg = oaseg;
	} while (oaseg != rseg);

	/* Link in new segments */

	if (lseg)
		lseg->next = tmp;
	else
		curr_gradient->segments = tmp;

	tmp->prev = lseg;

	seg->next = rseg;

	if (rseg)
		rseg->prev = seg;

	/* Reset selection */

	g_editor->control_sel_l = tmp;
	g_editor->control_sel_r = seg;

	/* Done */

	curr_gradient->last_visited = NULL; /* Force re-search */
	curr_gradient->dirty = 1;

	ed_update_editor(GRAD_UPDATE_PREVIEW | GRAD_UPDATE_CONTROL);
} /* cpopup_flip_callback */


/*****/

static void
cpopup_replicate_callback(GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *scale;
	GtkWidget *button;
	GtkObject *scale_data;

	/* Create dialog window */

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog),
			     (g_editor->control_sel_l == g_editor->control_sel_r) ?
			     _("Replicate segment") :
			     _("Replicate selection"));
	gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
	gtk_container_border_width(GTK_CONTAINER(dialog), 0);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_border_width(GTK_CONTAINER(vbox), 8);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), vbox,
			   FALSE, FALSE, 0);
	gtk_widget_show(vbox);

	/* Instructions */

	label = gtk_label_new(_("Please select the number of times"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	label = gtk_label_new((g_editor->control_sel_l == g_editor->control_sel_r) ?
			      _("you want to replicate the selected segment") :
			      _("you want to replicate the selection"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	/* Scale */

	g_editor->replicate_times = 2;
	scale_data  = gtk_adjustment_new(2.0, 2.0, 21.0, 1.0, 1.0, 1.0);

	scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
	gtk_scale_set_digits(GTK_SCALE(scale), 0);
	gtk_scale_set_value_pos(GTK_SCALE(scale), GTK_POS_TOP);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, TRUE, 8);
	gtk_widget_show(scale);

	gtk_signal_connect(scale_data, "value_changed",
			   (GtkSignalFunc) cpopup_replicate_scale_update,
			   NULL);

	/* Buttons */

	button = ed_create_button(_("Replicate"), 0.5, 0.5,
				  (GtkSignalFunc) cpopup_do_replicate_callback,
				  (gpointer) dialog);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
			   button, TRUE, TRUE, 0);
	gtk_widget_grab_default(button);
	gtk_widget_show(button);

	button = ed_create_button(_("Cancel"), 0.5, 0.5,
				  (GtkSignalFunc) cpopup_replicate_cancel_callback,
				  (gpointer) dialog);
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
			   button, TRUE, TRUE, 0);
	gtk_widget_show(button);

	/* Show! */

	gtk_widget_show(dialog);
	gtk_widget_set_sensitive(g_editor->shell, FALSE);
} /* cpopup_replicate_callback */


/*****/

static void
cpopup_split_uniform_scale_update(GtkAdjustment *adjustment, gpointer data)
{
	g_editor->split_parts = (int) (adjustment->value + 0.5); /* We have to round */
} /* cpopup_split_uniform_scale_update */


/*****/

static void
cpopup_split_uniform_split_callback(GtkWidget *widget, gpointer client_data)
{
	grad_segment_t *seg, *aseg, *lseg, *rseg, *lsel;

	gtk_widget_destroy(GTK_WIDGET(client_data));
	gtk_widget_set_sensitive(g_editor->shell, TRUE);

	seg  = g_editor->control_sel_l;
	lsel = NULL;

	do {
		aseg = seg;

		cpopup_split_uniform(seg, g_editor->split_parts, &lseg, &rseg);

		if (seg == g_editor->control_sel_l)
			lsel = lseg;

		seg = rseg->next;
	} while (aseg != g_editor->control_sel_r);

	g_editor->control_sel_l = lsel;
	g_editor->control_sel_r = rseg;

	curr_gradient->last_visited = NULL; /* Force re-search */
	curr_gradient->dirty = 1;
	ed_update_editor(GRAD_UPDATE_PREVIEW | GRAD_UPDATE_CONTROL);
} /* cpopup_split_uniform_split_callback */


/*****/

static void
cpopup_split_uniform_cancel_callback(GtkWidget *widget, gpointer client_data)
{
	gtk_widget_destroy(GTK_WIDGET(client_data));
	gtk_widget_set_sensitive(g_editor->shell, TRUE);
} /* cpopup_split_uniform_cancel_callback */


/*****/

static void
cpopup_replicate_scale_update(GtkAdjustment *adjustment, gpointer data)
{
	g_editor->replicate_times = (int) (adjustment->value + 0.5); /* We have to round */
} /* cpopup_replicate_scale_update */


/*****/

static void
cpopup_do_replicate_callback(GtkWidget *widget, gpointer client_data)
{
	double          sel_left, sel_right, sel_len;
	double          new_left;
	double          factor;
	grad_segment_t *prev, *seg, *tmp;
	grad_segment_t *oseg, *oaseg;
	grad_segment_t *lseg, *rseg;
	int             i;

	gtk_widget_destroy(GTK_WIDGET(client_data));
	gtk_widget_set_sensitive(g_editor->shell, TRUE);

	/* Remember original parameters */

	sel_left  = g_editor->control_sel_l->left;
	sel_right = g_editor->control_sel_r->right;
	sel_len   = sel_right - sel_left;

	factor = 1.0 / g_editor->replicate_times;

	/* Build replicated segments */

	prev = NULL;
	seg  = NULL;
	tmp  = NULL;

	for (i = 0; i < g_editor->replicate_times; i++) {
		/* Build one cycle */

		new_left  = sel_left + i * factor * sel_len;

		oseg = g_editor->control_sel_l;

		do {
			seg = seg_new_segment();

			if (prev == NULL) {
				seg->left = sel_left;
				tmp = seg; /* Remember first segment */
			} else
				seg->left = new_left + factor * (oseg->left - sel_left);

			seg->middle = new_left + factor * (oseg->middle - sel_left);
			seg->right  = new_left + factor * (oseg->right - sel_left);

			seg->r0 = oseg->r0;
			seg->g0 = oseg->g0;
			seg->b0 = oseg->b0;
			seg->a0 = oseg->a0;

			seg->r1 = oseg->r1;
			seg->g1 = oseg->g1;
			seg->b1 = oseg->b1;
			seg->a1 = oseg->a1;

			seg->type  = oseg->type;
			seg->color = oseg->color;

			seg->prev = prev;
			seg->next = NULL;

			if (prev)
				prev->next = seg;

			prev = seg;

			oaseg = oseg;
			oseg  = oseg->next;
		} while (oaseg != g_editor->control_sel_r);
	} /* for */

	seg->right = sel_right; /* Squish accumulative error */

	/* Free old segments */

	lseg = g_editor->control_sel_l->prev;
	rseg = g_editor->control_sel_r->next;

	oseg = g_editor->control_sel_l;

	do {
		oaseg = oseg->next;
		seg_free_segment(oseg);
		oseg = oaseg;
	} while (oaseg != rseg);

	/* Link in new segments */

	if (lseg)
		lseg->next = tmp;
	else
		curr_gradient->segments = tmp;

	tmp->prev = lseg;

	seg->next = rseg;

	if (rseg)
		rseg->prev = seg;

	/* Reset selection */

	g_editor->control_sel_l = tmp;
	g_editor->control_sel_r = seg;

	/* Done */

	curr_gradient->last_visited = NULL; /* Force re-search */
	curr_gradient->dirty = 1;

	ed_update_editor(GRAD_UPDATE_PREVIEW | GRAD_UPDATE_CONTROL);
} /* cpopup_do_replicate_callback */


/*****/

static void
cpopup_replicate_cancel_callback(GtkWidget *widget, gpointer client_data)
{
	gtk_widget_destroy(GTK_WIDGET(client_data));
	gtk_widget_set_sensitive(g_editor->shell, TRUE);
} /* cpopup_replicate_cancel_callback */


/*****/

static void
cpopup_blend_endpoints(double r0, double g0, double b0, double a0,
		       double r1, double g1, double b1, double a1,
		       int blend_colors, int blend_opacity)
{
	double          dr, dg, db, da;
	double          left, len;
	grad_segment_t *seg, *aseg;

	dr = r1 - r0;
	dg = g1 - g0;
	db = b1 - b0;
	da = a1 - a0;

	left  = g_editor->control_sel_l->left;
	len   = g_editor->control_sel_r->right - left;

	seg = g_editor->control_sel_l;

	do {
		if (blend_colors) {
			seg->r0 = r0 + (seg->left - left) / len * dr;
			seg->g0 = g0 + (seg->left - left) / len * dg;
			seg->b0 = b0 + (seg->left - left) / len * db;

			seg->r1 = r0 + (seg->right - left) / len * dr;
			seg->g1 = g0 + (seg->right - left) / len * dg;
			seg->b1 = b0 + (seg->right - left) / len * db;
		} /* if */

		if (blend_opacity) {
			seg->a0 = a0 + (seg->left - left) / len * da;
			seg->a1 = a0 + (seg->right - left) / len * da;
		} /* if */

		aseg = seg;
		seg = seg->next;
	} while (aseg != g_editor->control_sel_r);
} /* cpopup_blend_endpoints */


/*****/

static void
cpopup_split_midpoint(grad_segment_t *lseg, grad_segment_t **newl, grad_segment_t **newr)
{
	double          r, g, b, a;
	grad_segment_t *newseg;

	/* Get color at original segment's midpoint */

	grad_get_color_at(lseg->middle, &r, &g, &b, &a);

	/* Create a new segment and insert it in the list */

	newseg = seg_new_segment();

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

	newseg->r1 = lseg->r1;
	newseg->g1 = lseg->g1;
	newseg->b1 = lseg->b1;
	newseg->a1 = lseg->a1;

	lseg->r1 = newseg->r0 = r;
	lseg->g1 = newseg->g0 = g;
	lseg->b1 = newseg->b0 = b;
	lseg->a1 = newseg->a0 = a;

	/* Set parameters of new segment */

	newseg->type  = lseg->type;
	newseg->color = lseg->color;

	/* Done */

	*newl = lseg;
	*newr = newseg;
} /* cpopup_split_midpoint */


/*****/

static void
cpopup_split_uniform(grad_segment_t *lseg, int parts,
		     grad_segment_t **newl, grad_segment_t **newr)
{
	grad_segment_t *seg, *prev, *tmp;
	double          seg_len;
	int             i;

	seg_len = (lseg->right - lseg->left) / parts; /* Length of divisions */

	seg  = NULL;
	prev = NULL;
	tmp  = NULL;

	for (i = 0; i < parts; i++) {
		seg = seg_new_segment();

		if (i == 0)
			tmp = seg; /* Remember first segment */

		seg->left   = lseg->left + i * seg_len;
		seg->right  = lseg->left + (i + 1) * seg_len;
		seg->middle = (seg->left + seg->right) / 2.0;

		grad_get_color_at(seg->left, &seg->r0, &seg->g0, &seg->b0, &seg->a0);
		grad_get_color_at(seg->right, &seg->r1, &seg->g1, &seg->b1, &seg->a1);

		seg->type  = lseg->type;
		seg->color = lseg->color;

		seg->prev = prev;
		seg->next = NULL;

		if (prev)
			prev->next = seg;

		prev = seg;
	} /* for */

	/* Fix edges */

	tmp->r0 = lseg->r0;
	tmp->g0 = lseg->g0;
	tmp->b0 = lseg->b0;
	tmp->a0 = lseg->a0;

	seg->r1 = lseg->r1;
	seg->g1 = lseg->g1;
	seg->b1 = lseg->b1;
	seg->a1 = lseg->a1;

	tmp->left  = lseg->left;
	seg->right = lseg->right; /* To squish accumulative error */

	/* Link in list */

	tmp->prev = lseg->prev;
	seg->next = lseg->next;

	if (lseg->prev)
		lseg->prev->next = tmp;
	else
		curr_gradient->segments = tmp; /* We are on leftmost segment */

	if (lseg->next)
		lseg->next->prev = seg;

	curr_gradient->last_visited = NULL; /* Force re-search */

	/* Done */

	*newl = tmp;
	*newr = seg;

	/* Delete old segment */

	seg_free_segment(lseg);
} /* cpopup_split_uniform */


/***** Gradient functions *****/

/*****/

static gradient_t *
grad_new_gradient(void)
{
	gradient_t *grad;

	grad = g_malloc(sizeof(gradient_t));

	grad->name     	   = NULL;
	grad->segments 	   = NULL;
	grad->last_visited = NULL;
	grad->dirty        = 0;
	grad->filename     = NULL;
	grad->pixmap       = NULL;

	return grad;
} /* grad_new_gradient */


/*****/

static void
grad_free_gradient(gradient_t *grad)
{
	g_assert(grad != NULL);

	if (grad->name)
		g_free(grad->name);

	if (grad->segments)
		seg_free_segments(grad->segments);

	if (grad->filename)
		g_free(grad->filename);

	g_free(grad);
} /* grad_free_gradient */


/*****/

static void
grad_save_all(int need_free)
{
	GSList     *node;
	gradient_t *grad;

	node = gradients_list;

	while (node) {
		grad = node->data;

		/* If gradient has dirty flag set, save it */

		if (grad->dirty)
			grad_save_gradient(grad, grad->filename);

		if(need_free)
		        grad_free_gradient(grad);

		node = g_slist_next(node);
	} /* while */
}

/*****/

static void
grad_free_gradients(void)
{
        grad_save_all(1);  
  
	g_slist_free(gradients_list);

	num_gradients  = 0;
	gradients_list = NULL;
	curr_gradient  = NULL;
} /* grad_free_gradients */


/*****/

static void
grad_load_gradient(char *filename)
{
	FILE           *file;
	gradient_t     *grad;
	grad_segment_t *seg, *prev;
	int             num_segments;
	int             i;
	int             type, color;
	char            line[1024];

	g_assert(filename != NULL);

	file = fopen(filename, "rb");
	if (!file)
		return;

	fgets(line, 1024, file);
	if (strcmp(line, "GIMP Gradient\n") != 0)
		return;

	grad = grad_new_gradient();

	grad->filename = g_strdup(filename);
	grad->name     = g_strdup(g_basename(filename));

	fgets(line, 1024, file);
	num_segments = atoi(line);

	if (num_segments < 1) {
		g_message (_("grad_load_gradient(): invalid number of segments in \"%s\""), filename);
		g_free(grad);
		return;
	} /* if */

	prev = NULL;

	for (i = 0; i < num_segments; i++) {
		seg = seg_new_segment();
		seg->prev = prev;

		if (prev)
			prev->next = seg;
		else
			grad->segments = seg;

		fgets(line, 1024, file);

		if (sscanf(line, "%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%d%d",
			   &(seg->left), &(seg->middle), &(seg->right),
			   &(seg->r0), &(seg->g0), &(seg->b0), &(seg->a0),
			   &(seg->r1), &(seg->g1), &(seg->b1), &(seg->a1),
			   &type, &color) != 13) {
			g_message (_("grad_load_gradient(): badly formatted "
				   "gradient segment %d in \"%s\" --- bad things may "
				   "happen soon"), i, filename);
		} else {
			seg->type  = (grad_type_t) type;
			seg->color = (grad_color_t) color;
		} /* else */

		prev = seg;
	} /* for */

	fclose(file);

	grad_insert_in_gradients_list(grad);

	/* Check if this gradient is the default one */

	if (strcmp(default_gradient, grad->name) == 0)
		grad_default_gradient = grad;
} /* grad_load_gradient */


/*****/

static void
grad_save_gradient(gradient_t *grad, char *filename)
{
	FILE           *file;
	int             num_segments;
	grad_segment_t *seg;

	g_assert(grad != NULL);

	if (!filename) {
		g_message (_("grad_save_gradient(): can not save gradient with NULL filename"));
		return;
	} /* if */

	file = fopen(filename, "wb");
	if (!file) {
		g_message (_("grad_save_gradient(): can't open \"%s\""), filename);
		return;
	} /* if */

	/* File format is:
	 *
	 *   GIMP Gradient
	 *   number_of_segments
	 *   left middle right r0 g0 b0 a0 r1 g1 b1 a1 type coloring
	 *   left middle right r0 g0 b0 a0 r1 g1 b1 a1 type coloring
	 *   ...
	 */

	fprintf(file, "GIMP Gradient\n");

	/* Count number of segments */

	num_segments = 0;
	seg          = grad->segments;

	while (seg) {
		num_segments++;
		seg = seg->next;
	} /* while */

	/* Write rest of file */

	fprintf(file, "%d\n", num_segments);

	for (seg = grad->segments; seg; seg = seg->next)
		fprintf(file, "%f %f %f %f %f %f %f %f %f %f %f %d %d\n",
			seg->left, seg->middle, seg->right,
			seg->r0, seg->g0, seg->b0, seg->a0,
			seg->r1, seg->g1, seg->b1, seg->a1,
			(int) seg->type, (int) seg->color);

	fclose(file);

	grad->dirty = 0;
} /* grad_save_gradient */


/*****/

static gradient_t *
grad_create_default_gradient(void)
{
	gradient_t *grad;

	grad = grad_new_gradient();
	grad->segments = seg_new_segment();

	return grad;
} /* grad_create_default_gradient */


/*****/

static int
grad_insert_in_gradients_list(gradient_t *grad)
{
	GSList     *tmp;
	gradient_t *g;
	int         n;

	/* We insert gradients in alphabetical order.  Find the index
	 * of the gradient after which we will insert the current one.
	 */

	n   = 0;
	tmp = gradients_list;

	while (tmp) {
		g = tmp->data;

		if (strcmp(grad->name, g->name) <= 0)
			break; /* We found the one we want */

		n++;
		tmp = g_slist_next(tmp);
	} /* while */

	num_gradients++;
	gradients_list = g_slist_insert(gradients_list, grad, n);

	return n;
} /* grad_insert_in_gradients_list */


/*****/

static void
grad_dump_gradient(gradient_t *grad, FILE *file)
{
	grad_segment_t *seg;

	fprintf(file, "Name: \"%s\"\n", grad->name);
	fprintf(file, "Dirty: %d\n", grad->dirty);
	fprintf(file, "Filename: \"%s\"\n", grad->filename);

	seg = grad->segments;

	while (seg) {
		fprintf(file, "%c%p | %f %f %f | %f %f %f %f | %f %f %f %f | %d %d | %p %p\n",
			(seg == grad->last_visited) ? '>' : ' ',
			seg,
			seg->left, seg->middle, seg->right,
			seg->r0, seg->g0, seg->b0, seg->a0,
			seg->r1, seg->g1, seg->b1, seg->a1,
			(int) seg->type,
			(int) seg->color,
			seg->prev, seg->next);

		seg = seg->next;
	} /* while */
} /* grad_dump_gradient */


/***** Segment functions *****/

/*****/

static grad_segment_t *
seg_new_segment(void)
{
	grad_segment_t *seg;

	seg = g_malloc(sizeof(grad_segment_t));

	seg->left   = 0.0;
	seg->middle = 0.5;
	seg->right  = 1.0;

	seg->r0 = seg->g0 = seg->b0 = 0.0;
	seg->r1 = seg->g1 = seg->b1 = seg->a0 = seg->a1 = 1.0;

	seg->type  = GRAD_LINEAR;
	seg->color = GRAD_RGB;

	seg->prev = seg->next = NULL;

	return seg;
} /* seg_new_segment */


/*****/

static void
seg_free_segment(grad_segment_t *seg)
{
	g_assert(seg != NULL);

	g_free(seg);
} /* seg_free_segment */


/*****/

static void
seg_free_segments(grad_segment_t *seg)
{
	grad_segment_t *tmp;

	g_assert(seg != NULL);

	while (seg) {
		tmp = seg->next;
		seg_free_segment(seg);
		seg = tmp;
	} /* while */
} /* seg_free_segments */


/*****/

static grad_segment_t *
seg_get_segment_at(gradient_t *grad, double pos)
{
	grad_segment_t *seg;

	g_assert(grad != NULL);

	pos = BOUNDS(pos, 0.0, 1.0); /* to handle FP imprecision at the edges of the gradient */

	if (grad->last_visited)
		seg = grad->last_visited;
	else
		seg = grad->segments;

	while (seg)
		if (pos >= seg->left) {
			if (pos <= seg->right) {
				grad->last_visited = seg; /* for speed */
				return seg;
			} else
				seg = seg->next;
		} else
			seg = seg->prev;

	/* Oops: we should have found a segment, but we didn't */

	grad_dump_gradient(curr_gradient, stderr);
	fatal_error(_("seg_get_segment_at(): aieee, no matching segment for position %0.15f"), pos);
	return NULL; /* To shut up -Wall */
} /* seg_get_segment_at */


/*****/

static grad_segment_t *
seg_get_last_segment(grad_segment_t *seg)
{
	if (!seg)
		return NULL;

	while (seg->next)
		seg = seg->next;

	return seg;
} /* seg_get_last_segment */


/*****/

static void
seg_get_closest_handle(gradient_t *grad, double pos,
		       grad_segment_t **seg, control_drag_mode_t *handle)
{
	double l_delta, m_delta, r_delta;

	*seg = seg_get_segment_at(grad, pos);

	m_delta = fabs(pos - (*seg)->middle);

	if (pos < (*seg)->middle) {
		l_delta = fabs(pos - (*seg)->left);

		if (l_delta < m_delta)
			*handle = GRAD_DRAG_LEFT;
		else
			*handle = GRAD_DRAG_MIDDLE;
	} else {
		r_delta = fabs(pos - (*seg)->right);

		if (m_delta < r_delta)
			*handle = GRAD_DRAG_MIDDLE;
		else {
			*seg = (*seg)->next;
			*handle = GRAD_DRAG_LEFT;
		} /* else */
	} /* else */
} /* seg_get_closest_handle */


/***** Calculation functions *****/

/*****/

static double
calc_linear_factor(double middle, double pos)
{
	if (pos <= middle) {
		if (middle < EPSILON)
			return 0.0;
		else
			return 0.5 * pos / middle;
	} else {
		pos -= middle;
		middle = 1.0 - middle;

		if (middle < EPSILON)
			return 1.0;
		else
			return 0.5 + 0.5 * pos / middle;
	} /* else */
} /* calc_linear_factor */


/*****/

static double
calc_curved_factor(double middle, double pos)
{
	if (middle < EPSILON)
		middle = EPSILON;

	return pow(pos, log(0.5) / log(middle));
} /* calc_curved_factor */


/*****/

static double
calc_sine_factor(double middle, double pos)
{
	pos = calc_linear_factor(middle, pos);

	return (sin((-M_PI / 2.0) + M_PI * pos) + 1.0) / 2.0;
} /* calc_sine_factor */


/*****/

static double
calc_sphere_increasing_factor(double middle, double pos)
{
	pos = calc_linear_factor(middle, pos) - 1.0;

	return sqrt(1.0 - pos * pos); /* Works for convex increasing and concave decreasing */
} /* calc_sphere_increasing_factor */


/*****/

static double
calc_sphere_decreasing_factor(double middle, double pos)
{
	pos = calc_linear_factor(middle, pos);

	return 1.0 - sqrt(1.0 - pos * pos); /* Works for convex decreasing and concave increasing */
} /* calc_sphere_decreasing_factor */


/*****/

static void
calc_rgb_to_hsv(double *r, double *g, double *b)
{
	double red, green, blue;
	double h, s, v;
	double min, max;
	double delta;

	red   = *r;
	green = *g;
	blue  = *b;

	h = 0.0; /* Shut up -Wall */

	if (red > green) {
		if (red > blue)
			max = red;
		else
			max = blue;

		if (green < blue)
			min = green;
		else
			min = blue;
	} else {
		if (green > blue)
			max = green;
		else
			max = blue;

		if (red < blue)
			min = red;
		else
			min = blue;
	} /* else */

	v = max;

	if (max != 0.0)
		s = (max - min) / max;
	else
		s = 0.0;

	if (s == 0.0)
		h = 0.0;
	else {
		delta = max - min;

		if (red == max)
			h = (green - blue) / delta;
		else if (green == max)
			h = 2 + (blue - red) / delta;
		else if (blue == max)
			h = 4 + (red - green) / delta;

		h /= 6.0;

		if (h < 0.0)
			h += 1.0;
		else if (h > 1.0)
			h -= 1.0;
	} /* else */

	*r = h;
	*g = s;
	*b = v;
} /* calc_rgb_to_hsv */


/*****/

static void
calc_hsv_to_rgb(double *h, double *s, double *v)
{
	double hue, saturation, value;
	double f, p, q, t;

	if (*s == 0.0) {
		*h = *v;
		*s = *v;
		*v = *v; /* heh */
	} else {
		hue        = *h * 6.0;
		saturation = *s;
		value      = *v;

		if (hue == 6.0)
			hue = 0.0;

		f = hue - (int) hue;
		p = value * (1.0 - saturation);
		q = value * (1.0 - saturation * f);
		t = value * (1.0 - saturation * (1.0 - f));

		switch ((int) hue) {
			case 0:
				*h = value;
				*s = t;
				*v = p;
				break;

			case 1:
				*h = q;
				*s = value;
				*v = p;
				break;

			case 2:
				*h = p;
				*s = value;
				*v = t;
				break;

			case 3:
				*h = p;
				*s = q;
				*v = value;
				break;

			case 4:
				*h = t;
				*s = p;
				*v = value;
				break;

			case 5:
				*h = value;
				*s = p;
				*v = q;
				break;
		} /* switch */
	} /* else */
} /* calc_hsv_to_rgb */


/***** Files and paths functions *****/

/*****/

static char *
build_user_filename(char *name, char *path_str)
{
	char *home;
	char *local_path;
	char *first_token;
	char *token;
	char *path;
	char *filename;

	g_assert(name != NULL);

	if (!path_str)
		return NULL; /* Perhaps this is not a good idea */

	/* Get the first path specified in the list */

	home        = g_get_home_dir ();
	local_path  = g_strdup(path_str);
	first_token = local_path;
	token       = xstrsep(&first_token, G_SEARCHPATH_SEPARATOR_S);
	filename    = NULL;

	if (token) {
		if (*token == '~') {
			if (!home)
				return NULL;
			path = g_strdup_printf("%s%s", home, token + 1);
		} else {
			path = g_strdup(token);
		} /* else */

		filename = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s",
					   path, name);

		g_free(path);
	} /* if */

	g_free(local_path);

	return filename;
} /* build_user_filename */


/***** Procedural database declarations and functions *****/

/***** gradients_get_list *****/

ProcArg gradients_get_list_out_args[] = {
	{ PDB_INT32,
	  "num_gradients",
	  "The number of loaded gradients"
	},
	{ PDB_STRINGARRAY,
	  "gradient_names",
	  "The list of gradient names"
	}
}; /* gradients_get_list_out_args */


ProcRecord gradients_get_list_proc = {
	"gimp_gradients_get_list",
	"Retrieve the list of loaded gradients",
	"This procedure returns a list of the gradients that are currently loaded "
	"in the gradient editor.  You can later use the gimp_gradients_set_active "
	"function to set the active gradient.",
	"Federico Mena Quintero",
	"Federico Mena Quintero",
	"1997",
	PDB_INTERNAL,

	/* Input arguments */

	0,
	NULL,

	/* Output arguments */

	sizeof(gradients_get_list_out_args) / sizeof(gradients_get_list_out_args[0]),
	gradients_get_list_out_args,

	/* Exec method */

	{ { gradients_get_list_invoker } }
}; /* gradients_get_list_proc */


static Argument *
gradients_get_list_invoker(Argument *args)
{
	Argument   *return_args;
	gradient_t *grad;
	GSList     *list;
	char      **gradients;
	int         i;
	int         success;

	gradients = g_malloc(sizeof(char *) * num_gradients);

	list = gradients_list;

	success = (list != NULL);

	i = 0;

	while (list) {
		grad           = list->data;
		gradients[i++] = g_strdup(grad->name);
		list           = g_slist_next(list);
	} /* while */

	return_args = procedural_db_return_args(&gradients_get_list_proc, success);

	if (success) {
		return_args[1].value.pdb_int = num_gradients;
		return_args[2].value.pdb_pointer = gradients;
	} /* if */

	return return_args;
} /* gradients_get_list_invoker */


/***** gradients_get_active *****/

ProcArg gradients_get_active_out_args[] = {
	{ PDB_STRING,
	  "name",
	  "The name of the active gradient"
	}
}; /* gradients_get_active_out_args */


ProcRecord gradients_get_active_proc = {
	"gimp_gradients_get_active",
	"Retrieve the name of the active gradient",
	"This procedure returns the name of the active gradient in hte gradient editor.",
	"Federico Mena Quintero",
	"Federico Mena Quintero",
	"1997",
	PDB_INTERNAL,

	/* Input arguments */

	0,
	NULL,

	/* Output arguments */

	sizeof(gradients_get_active_out_args) / sizeof(gradients_get_active_out_args[0]),
	gradients_get_active_out_args,

	/* Exec method */

	{ { gradients_get_active_invoker } }
}; /* gradients_get_active_proc */


Argument *
gradients_get_active_invoker(Argument *args)
{
	Argument *return_args;
	int       success;

	success = (curr_gradient != NULL);

	return_args = procedural_db_return_args(&gradients_get_active_proc, success);

	if (success)
		return_args[1].value.pdb_pointer = g_strdup(curr_gradient->name);

	return return_args;
} /* gradients_get_active_invoker */


/***** gradients_set_active *****/

ProcArg gradients_set_active_args[] = {
	{ PDB_STRING,
	  "name",
	  "The name of the gradient to set"
	}
}; /* gradients_set_active_args */


ProcRecord gradients_set_active_proc = {
	"gimp_gradients_set_active",
	"Sets the specified gradient as the active gradient",
	"This procedure lets you set the specified gradient as the active "
	"or \"current\" one.  The name is simply a string which corresponds "
	"to one of the loaded gradients in the gradient editor.  If no "
	"matching gradient is found, this procedure will return an error. "
	"Otherwise, the specified gradient will become active and will be used "
	"for subsequent custom gradient operations.",
	"Federico Mena Quintero",
	"Federico Mena Quintero",
	"1997",
	PDB_INTERNAL,

	/* Input arguments */

	sizeof(gradients_set_active_args) / sizeof(gradients_set_active_args[0]),
	gradients_set_active_args,

	/* Output arguments */

	0,
	NULL,

	/* Exec method */

	{ { gradients_set_active_invoker } }
}; /* gradients_set_active_proc */

gint
grad_set_grad_to_name(gchar *name)
{
  GSList *list;
  gradient_t *grad;

  int         n = 0;

  list = gradients_list;
  
  while (list) {
    grad = list->data;
    
    if (strcmp(grad->name, name) == 0) {
      /* We found it! */
      
      /* Select that gradient in the listbox */
      /* Only if gradient editor has been created */
      
      if(g_editor)
	{
	  gtk_clist_select_row(GTK_CLIST(g_editor->clist),n,-1);
	  gtk_clist_moveto(GTK_CLIST(g_editor->clist),n,0,0.5,0.0);
	}
      else
	{
	  /* force internal structs to use selected gradient */
	  GSList* tmp = g_slist_nth(gradients_list,n);
	  if(tmp)
	    curr_gradient = (gradient_t *)(tmp->data);
	}
      return TRUE;
    } /* if */
    
    n++;
    list = g_slist_next(list);
  } /* while */
  
  return FALSE;
}


static Argument *
gradients_set_active_invoker(Argument *args)
{
	char       *name;
	int         success;

	name = args[0].value.pdb_pointer;

	success = (name != NULL);

	if (success) {
		/* See if we have a gradient with the specified name */
  	        success = grad_set_grad_to_name(name);
	} /* if */

	return procedural_db_return_args(&gradients_set_active_proc, success);
} /* gradients_set_active_invoker */


/***** gradients_sample_uniform *****/

ProcArg gradients_sample_uniform_args[] = {
	{ PDB_INT32,
	  "num_samples",
	  "The number of samples to take"
	}
}; /* gradients_sample_uniform_args */


ProcArg gradients_sample_uniform_out_args[] = {
	{ PDB_INT32,
	  "array_length",
	  "Length of the color_samples array (4 * num_samples)"
	},
	{ PDB_FLOATARRAY,
	  "color_samples",
	  "Color samples: { R1, G1, B1, A1, ..., Rn, Gn, Bn, An }"
	}
}; /* gradients_sample_uniform_out_args */


ProcRecord gradients_sample_uniform_proc = {
	"gimp_gradients_sample_uniform",
	"Sample the active gradient in uniform parts",
	"This procedure samples the active gradient from the gradient editor in the "
	"specified number of uniform parts.  It returns a list of floating-point values "
	"which correspond to the RGBA values for each sample.  The minimum number of "
	"samples to take is 2, in which case the returned colors will correspond to the "
	"{ 0.0, 1.0 } positions in the gradient.  For example, if the number of samples "
	"is 3, the procedure will return the colors at positions { 0.0, 0.5, 1.0 }.",
	"Federico Mena Quintero",
	"Federico Mena Quintero",
	"1997",
	PDB_INTERNAL,

	/* Input arguments */

	sizeof(gradients_sample_uniform_args) / sizeof(gradients_sample_uniform_args[0]),
	gradients_sample_uniform_args,

	/* Output arguments */

	sizeof(gradients_sample_uniform_out_args) / sizeof(gradients_sample_uniform_out_args[0]),
	gradients_sample_uniform_out_args,

	/* Exec method */

	{ { gradients_sample_uniform_invoker } }
}; /* gradients_sample_uniform_proc */


static Argument *
gradients_sample_uniform_invoker(Argument *args)
{
	Argument *return_args;
	gdouble  *values, *pv;
	double    pos, delta;
	double    r, g, b, a;
	int       i;
	int       success;

	i = args[0].value.pdb_int;

	success = (i >= 2);

	return_args = procedural_db_return_args(&gradients_sample_uniform_proc, success);

	if (success) {
		pos   = 0.0;
		delta = 1.0 / (i - 1);

		values = g_malloc(i * 4 * sizeof(gdouble));
		pv     = values;

		return_args[1].value.pdb_int = i * 4;

		while (i--) {
			grad_get_color_at(pos, &r, &g, &b, &a);

			*pv++ = r;
			*pv++ = g;
			*pv++ = b;
			*pv++ = a;

			pos += delta;
		} /* while */

		return_args[2].value.pdb_pointer = values;
	} /* if */

	return return_args;
} /* gradients_sample_uniform_invoker */


/***** gradients_sample_custom *****/

ProcArg gradients_sample_custom_args[] = {
	{ PDB_INT32,
	  "num_samples",
	  "The number of samples to take"
	},
	{ PDB_FLOATARRAY,
	  "positions",
	  "The list of positions to sample along the gradient"
	}
}; /* gradients_sample_custom_args */


ProcArg gradients_sample_custom_out_args[] = {
	{ PDB_INT32,
	  "array_length",
	  "Length of the color_samples array (4 * num_samples)"
	},
	{ PDB_FLOATARRAY,
	  "color_samples",
	  "Color samples: { R1, G1, B1, A1, ..., Rn, Gn, Bn, An }"
	}
}; /* gradients_sample_custom_out_args */


ProcRecord gradients_sample_custom_proc = {
	"gimp_gradients_sample_custom",
	"Sample the active gradient in custom positions",
	"This procedure samples the active gradient from the gradient editor in "
	"the specified number of points.  The procedure will sample the gradient "
	"in the specified positions from the list.  The left endpoint of the "
	"gradient corresponds to position 0.0, and the right endpoint corresponds "
	"to 1.0.  The procedure returns a list of floating-point values which "
	"correspond to the RGBA values for each sample.",
	"Federico Mena Quintero",
	"Federico Mena Quintero",
	"1997",
	PDB_INTERNAL,

	/* Input arguments */

	sizeof(gradients_sample_custom_args) / sizeof(gradients_sample_custom_args[0]),
	gradients_sample_custom_args,

	/* Output arguments */

	sizeof(gradients_sample_custom_out_args) / sizeof(gradients_sample_custom_out_args[0]),
	gradients_sample_custom_out_args,

	/* Exec method */

	{ { gradients_sample_custom_invoker } }
}; /* gradients_sample_custom_proc */


static Argument *
gradients_sample_custom_invoker(Argument *args)
{
	Argument *return_args;
	gdouble  *values, *pv;
	gdouble  *pos;
	double    r, g, b, a;
	int       i;
	int       success;

	i = args[0].value.pdb_int;

	success = (i > 0);

	return_args = procedural_db_return_args(&gradients_sample_custom_proc, success);

	if (success) {
		pos = args[1].value.pdb_pointer;

		values = g_malloc(i * 4 * sizeof(gdouble));
		pv     = values;

		return_args[1].value.pdb_int = i * 4;

		while (i--) {
			grad_get_color_at(*pos, &r, &g, &b, &a);

			*pv++ = r;
			*pv++ = g;
			*pv++ = b;
			*pv++ = a;

			pos++;
		} /* while */

		return_args[2].value.pdb_pointer = values;
	} /* if */

	return return_args;
} /* gradients_sample_custom_invoker */

