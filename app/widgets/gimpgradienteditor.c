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

#include <stdlib.h>
#include <string.h>

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimpgradient.h"

#include "gimpdnd.h"
#include "gimpgradienteditor.h"
#include "gimpitemfactory.h"
#include "gimpwidgets-utils.h"

#include "gui/color-notebook.h"

#include "libgimp/gimpintl.h"


#define EPSILON 1e-10

#define GRAD_SCROLLBAR_STEP_SIZE 0.05
#define GRAD_SCROLLBAR_PAGE_SIZE 0.75

#define GRAD_PREVIEW_WIDTH  600
#define GRAD_PREVIEW_HEIGHT  64
#define GRAD_CONTROL_HEIGHT  10

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


/*  local function prototypes  */

static void   gimp_gradient_editor_class_init (GimpGradientEditorClass *klass);
static void   gimp_gradient_editor_init       (GimpGradientEditor      *editor);

static void   gimp_gradient_editor_set_data         (GimpDataEditor     *editor,
                                                     GimpData           *data);

static void   gradient_editor_drop_gradient         (GtkWidget          *widget,
                                                     GimpViewable       *viewable,
                                                     gpointer            data);
static void   gradient_editor_scrollbar_update      (GtkAdjustment      *adj,
                                                     GimpGradientEditor *editor);
static void   gradient_editor_zoom_all_callback     (GtkWidget          *widget,
                                                     GimpGradientEditor *editor);
static void   gradient_editor_zoom_out_callback     (GtkWidget          *widget,
                                                     GimpGradientEditor *editor);
static void   gradient_editor_zoom_in_callback      (GtkWidget          *widget,
                                                     GimpGradientEditor *editor);
static void   gradient_editor_instant_update_update (GtkWidget          *widget,
                                                     GimpGradientEditor *editor);

static void   gradient_editor_set_hint              (GimpGradientEditor *editor,
                                                     const gchar        *str1,
                                                     const gchar        *str2,
                                                     const gchar        *str3);


/* Gradient preview functions */

static gint      preview_events                   (GtkWidget          *widget,
						   GdkEvent           *event,
						   GimpGradientEditor *editor);
static void      preview_set_hint                 (GimpGradientEditor *editor,
						   gint                x);

static void      preview_set_foreground           (GimpGradientEditor *editor,
						   gint                x);
static void      preview_set_background           (GimpGradientEditor *editor,
						   gint                x);

static void      preview_update                   (GimpGradientEditor *editor,
						   gboolean            recalculate);
static void      preview_fill_image               (GimpGradientEditor *editor,
						   gint                width,
						   gint                height,
						   gdouble             left,
						   gdouble             right);

/* Gradient control functions */

static gint      control_events                   (GtkWidget          *widget,
						   GdkEvent           *event,
						   GimpGradientEditor *editor);
static void      control_do_hint                  (GimpGradientEditor *editor,
						   gint                x,
						   gint                y);
static void      control_button_press             (GimpGradientEditor *editor,
						   gint                x,
						   gint                y,
						   guint               button,
						   guint               state);
static gboolean  control_point_in_handle          (GimpGradientEditor *editor,
						   GimpGradient       *gradient,
						   gint                x,
						   gint                y,
						   GimpGradientSegment *seg,
						   GradientEditorDragMode handle);
static void      control_select_single_segment    (GimpGradientEditor  *editor,
						   GimpGradientSegment *seg);
static void      control_extend_selection         (GimpGradientEditor  *editor,
						   GimpGradientSegment *seg,
						   gdouble              pos);
static void      control_motion                   (GimpGradientEditor  *editor,
						   GimpGradient        *gradient,
						   gint                 x);

static void      control_compress_left            (GimpGradientSegment *range_l,
						   GimpGradientSegment *range_r,
						   GimpGradientSegment *drag_seg,
						   gdouble              pos);
static void      control_compress_range           (GimpGradientSegment *range_l,
						   GimpGradientSegment *range_r,
						   gdouble              new_l,
						   gdouble              new_r);

static double    control_move                     (GimpGradientEditor  *editor,
						   GimpGradientSegment *range_l,
						   GimpGradientSegment *range_r,
						   gdouble              delta);

/* Control update/redraw functions */

static void      control_update                   (GimpGradientEditor *editor,
						   GimpGradient       *gradient,
						   gboolean            recalculate);
static void      control_draw                     (GimpGradientEditor *editor,
						   GimpGradient       *gradient,
						   GdkPixmap          *pixmap,
						   gint                width,
						   gint                height,
						   gdouble             left,
						   gdouble             right);
static void      control_draw_normal_handle       (GimpGradientEditor *editor,
						   GdkPixmap          *pixmap,
						   gdouble             pos,
						   gint                height);
static void      control_draw_middle_handle       (GimpGradientEditor *editor,
						   GdkPixmap          *pixmap,
						   gdouble             pos,
						   gint                height);
static void      control_draw_handle              (GdkPixmap          *pixmap,
						   GdkGC              *border_gc,
						   GdkGC              *fill_gc,
						   gint                xpos,
						   gint                height);

static gint      control_calc_p_pos               (GimpGradientEditor *editor,
						   gdouble             pos);
static gdouble   control_calc_g_pos               (GimpGradientEditor *editor,
						   gint                pos);

/* Segment functions */

static void      seg_get_closest_handle           (GimpGradient         *grad,
						   gdouble               pos,
						   GimpGradientSegment **seg,
						   GradientEditorDragMode *handle);


static GimpDataEditorClass *parent_class = NULL;


GType
gimp_gradient_editor_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpGradientEditorClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_gradient_editor_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpGradientEditor),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_gradient_editor_init,
      };

      type = g_type_register_static (GIMP_TYPE_DATA_EDITOR,
                                     "GimpGradientEditor",
                                     &info, 0);
    }

  return type;
}

static void
gimp_gradient_editor_class_init (GimpGradientEditorClass *klass)
{
  GimpDataEditorClass *editor_class;

  editor_class = GIMP_DATA_EDITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  editor_class->set_data = gimp_gradient_editor_set_data;
}

static void
gimp_gradient_editor_init (GimpGradientEditor *editor)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *button;

  /* Frame for gradient preview and gradient control */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /* Gradient preview */
  editor->preview_rows[0]     = NULL;
  editor->preview_rows[1]     = NULL;
  editor->preview_last_x      = 0;
  editor->preview_button_down = FALSE;

  editor->preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_set_dither (GTK_PREVIEW (editor->preview), GDK_RGB_DITHER_MAX);
  gtk_preview_size (GTK_PREVIEW (editor->preview),
		    GRAD_PREVIEW_WIDTH, GRAD_PREVIEW_HEIGHT);

  /*  Enable auto-resizing of the preview but ensure a minimal size  */
  gtk_widget_set_size_request (editor->preview,
                               GRAD_PREVIEW_WIDTH, GRAD_PREVIEW_HEIGHT);
  gtk_preview_set_expand (GTK_PREVIEW (editor->preview), TRUE);

  gtk_widget_set_events (editor->preview, GRAD_PREVIEW_EVENT_MASK);

  g_signal_connect (editor->preview, "event",
		    G_CALLBACK (preview_events),
		    editor);

  gimp_dnd_viewable_dest_add (GTK_WIDGET (editor->preview),
                              GIMP_TYPE_GRADIENT,
                              gradient_editor_drop_gradient,
                              editor);

  gtk_box_pack_start (GTK_BOX (vbox), editor->preview, TRUE, TRUE, 0);
  gtk_widget_show (editor->preview);

  /* Gradient control */
  editor->control_pixmap       = NULL;
  editor->control_drag_segment = NULL;
  editor->control_sel_l        = NULL;
  editor->control_sel_r        = NULL;
  editor->control_drag_mode    = GRAD_DRAG_NONE;
  editor->control_click_time   = 0;
  editor->control_compress     = FALSE;
  editor->control_last_x       = 0;
  editor->control_last_gx      = 0.0;
  editor->control_orig_pos     = 0.0;

  editor->control = gtk_drawing_area_new ();
  gtk_widget_set_size_request (editor->control,
                               GRAD_PREVIEW_WIDTH, GRAD_CONTROL_HEIGHT);
  gtk_widget_set_events (editor->control, GRAD_CONTROL_EVENT_MASK);
  gtk_box_pack_start (GTK_BOX (vbox), editor->control, FALSE, FALSE, 0);
  gtk_widget_show (editor->control);

  g_signal_connect (editor->control, "event",
		    G_CALLBACK (control_events),
		    editor);

  /*  Scrollbar  */
  editor->zoom_factor = 1;

  editor->scroll_data = gtk_adjustment_new (0.0, 0.0, 1.0,
                                            GRAD_SCROLLBAR_STEP_SIZE,
                                            GRAD_SCROLLBAR_PAGE_SIZE,
                                            1.0);

  g_signal_connect (editor->scroll_data, "value_changed",
		    G_CALLBACK (gradient_editor_scrollbar_update),
		    editor);
  g_signal_connect (editor->scroll_data, "changed",
		    G_CALLBACK (gradient_editor_scrollbar_update),
		    editor);

  editor->scrollbar = gtk_hscrollbar_new (GTK_ADJUSTMENT (editor->scroll_data));
  gtk_range_set_update_policy (GTK_RANGE (editor->scrollbar),
			       GTK_UPDATE_CONTINUOUS);
  gtk_box_pack_start (GTK_BOX (editor), editor->scrollbar, FALSE, FALSE, 0);
  gtk_widget_show (editor->scrollbar);

  /*  Horizontal box for zoom controls and instant update toggle  */
  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  +, - and Zoom Fit buttons  */
  gimp_editor_add_button (GIMP_EDITOR (editor),
                          GTK_STOCK_ZOOM_FIT,
                          _("Zoom All"), NULL,
                          G_CALLBACK (gradient_editor_zoom_all_callback),
                          NULL,
                          editor);

  gimp_editor_add_button (GIMP_EDITOR (editor),
                          GTK_STOCK_ZOOM_IN,
                          _("Zoom In"), NULL,
                          G_CALLBACK (gradient_editor_zoom_in_callback),
                          NULL,
                          editor);

  gimp_editor_add_button (GIMP_EDITOR (editor),
                          GTK_STOCK_ZOOM_OUT,
                          _("Zoom Out"), NULL,
                          G_CALLBACK (gradient_editor_zoom_out_callback),
                          NULL,
                          editor);

  /* Instant update toggle */
  editor->instant_update = TRUE;

  button = gtk_check_button_new_with_label (_("Instant update"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
		    G_CALLBACK (gradient_editor_instant_update_update),
		    editor);

  /* Hint bar */
  editor->hint_label1 = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (editor->hint_label1), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), editor->hint_label1, FALSE, FALSE, 0);
  gtk_widget_show (editor->hint_label1);

  editor->hint_label2 = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (editor->hint_label2), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (editor), editor->hint_label2, FALSE, FALSE, 0);
  gtk_widget_show (editor->hint_label2);

  editor->hint_label3 = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (editor->hint_label3), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (editor), editor->hint_label3, FALSE, FALSE, 0);
  gtk_widget_show (editor->hint_label3);

  /* Initialize other data */
  editor->left_saved_segments = NULL;
  editor->left_saved_dirty    = FALSE;

  editor->right_saved_segments = NULL;
  editor->right_saved_dirty    = FALSE;

  editor->saved_colors[0].r = 0.0; /* Black */
  editor->saved_colors[0].g = 0.0;
  editor->saved_colors[0].b = 0.0;
  editor->saved_colors[0].a = GIMP_OPACITY_OPAQUE;

  editor->saved_colors[1].r = 0.5; /* 50% Gray */
  editor->saved_colors[1].g = 0.5;
  editor->saved_colors[1].b = 0.5;
  editor->saved_colors[1].a = GIMP_OPACITY_OPAQUE;

  editor->saved_colors[2].r = 1.0; /* White */
  editor->saved_colors[2].g = 1.0;
  editor->saved_colors[2].b = 1.0;
  editor->saved_colors[2].a = GIMP_OPACITY_OPAQUE;

  editor->saved_colors[3].r = 0.0; /* Clear */
  editor->saved_colors[3].g = 0.0;
  editor->saved_colors[3].b = 0.0;
  editor->saved_colors[3].a = GIMP_OPACITY_TRANSPARENT;

  editor->saved_colors[4].r = 1.0; /* Red */
  editor->saved_colors[4].g = 0.0;
  editor->saved_colors[4].b = 0.0;
  editor->saved_colors[4].a = GIMP_OPACITY_OPAQUE;

  editor->saved_colors[5].r = 1.0; /* Yellow */
  editor->saved_colors[5].g = 1.0;
  editor->saved_colors[5].b = 0.0;
  editor->saved_colors[5].a = GIMP_OPACITY_OPAQUE;

  editor->saved_colors[6].r = 0.0; /* Green */
  editor->saved_colors[6].g = 1.0;
  editor->saved_colors[6].b = 0.0;
  editor->saved_colors[6].a = GIMP_OPACITY_OPAQUE;

  editor->saved_colors[7].r = 0.0; /* Cyan */
  editor->saved_colors[7].g = 1.0;
  editor->saved_colors[7].b = 1.0;
  editor->saved_colors[7].a = GIMP_OPACITY_OPAQUE;

  editor->saved_colors[8].r = 0.0; /* Blue */
  editor->saved_colors[8].g = 0.0;
  editor->saved_colors[8].b = 1.0;
  editor->saved_colors[8].a = GIMP_OPACITY_OPAQUE;

  editor->saved_colors[9].r = 1.0; /* Magenta */
  editor->saved_colors[9].g = 0.0;
  editor->saved_colors[9].b = 1.0;
  editor->saved_colors[9].a = GIMP_OPACITY_OPAQUE;
}

static void
gimp_gradient_editor_set_data (GimpDataEditor *editor,
                               GimpData       *data)
{
  GimpGradientEditor *gradient_editor;

  gradient_editor = GIMP_GRADIENT_EDITOR (editor);

  GIMP_DATA_EDITOR_CLASS (parent_class)->set_data (editor, data);

  if (editor->data)
    {
      if (gradient_editor->color_notebook)
        {
          color_notebook_free (gradient_editor->color_notebook);
          gradient_editor->color_notebook = NULL;
          gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
        }
    }

  if (data && ! data->internal)
    gtk_widget_set_sensitive (gradient_editor->control, TRUE);
  else
    gtk_widget_set_sensitive (gradient_editor->control, FALSE);

  gimp_gradient_editor_update (gradient_editor,
                               GRAD_UPDATE_PREVIEW | GRAD_RESET_CONTROL);
}


/*  public functions  */

GimpDataEditor *
gimp_gradient_editor_new (Gimp *gimp)
{
  GimpGradientEditor *editor;

  editor = g_object_new (GIMP_TYPE_GRADIENT_EDITOR, NULL);

  gimp_data_editor_construct (GIMP_DATA_EDITOR (editor),
                              gimp,
                              GIMP_TYPE_GRADIENT);

  return GIMP_DATA_EDITOR (editor);
}

void
gimp_gradient_editor_update (GimpGradientEditor       *editor,
                             GradientEditorUpdateMask  flags)
{
  if (GIMP_DATA_EDITOR (editor)->data)
    {
      GimpGradient *gradient;

      gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

      if (flags & GRAD_UPDATE_GRADIENT)
        preview_update (editor, TRUE);

      if (flags & GRAD_UPDATE_PREVIEW)
        preview_update (editor, TRUE);

      if (flags & GRAD_UPDATE_CONTROL)
        control_update (editor, gradient, FALSE);

      if (flags & GRAD_RESET_CONTROL)
        control_update (editor, gradient, TRUE);
    }
}


/*  private functions  */

static void
gradient_editor_drop_gradient (GtkWidget    *widget,
			       GimpViewable *viewable,
			       gpointer      data)
{
  gimp_data_editor_set_data (GIMP_DATA_EDITOR (data), GIMP_DATA (viewable));
}

static void
gradient_editor_scrollbar_update (GtkAdjustment      *adjustment,
                                  GimpGradientEditor *editor)
{
  gchar *str1;
  gchar *str2;

  str1 = g_strdup_printf (_("Zoom factor: %d:1"),
                          editor->zoom_factor);

  str2 = g_strdup_printf (_("Displaying [%0.6f, %0.6f]"),
                          adjustment->value,
                          adjustment->value + adjustment->page_size);

  gradient_editor_set_hint (editor, str1, str2, "");

  g_free (str1);
  g_free (str2);

  gimp_gradient_editor_update (editor,
                               GRAD_UPDATE_PREVIEW | GRAD_UPDATE_CONTROL);
}

static void
gradient_editor_zoom_all_callback (GtkWidget          *widget,
                                   GimpGradientEditor *editor)
{
  GtkAdjustment *adjustment;

  adjustment = GTK_ADJUSTMENT (editor->scroll_data);

  editor->zoom_factor = 1;

  adjustment->value          = 0.0;
  adjustment->page_size      = 1.0;
  adjustment->step_increment = GRAD_SCROLLBAR_STEP_SIZE;
  adjustment->page_increment = GRAD_SCROLLBAR_PAGE_SIZE;

  gtk_adjustment_changed (GTK_ADJUSTMENT (editor->scroll_data));
}

static void
gradient_editor_zoom_out_callback (GtkWidget          *widget,
                                   GimpGradientEditor *editor)
{
  GtkAdjustment *adjustment;
  gdouble        old_value;
  gdouble        value;
  gdouble        old_page_size;
  gdouble        page_size;

  if (editor->zoom_factor <= 1)
    return;

  adjustment = GTK_ADJUSTMENT (editor->scroll_data);

  old_value     = adjustment->value;
  old_page_size = adjustment->page_size;

  editor->zoom_factor--;

  page_size = 1.0 / editor->zoom_factor;
  value     = old_value - (page_size - old_page_size) / 2.0;

  if (value < 0.0)
    value = 0.0;
  else if ((value + page_size) > 1.0)
    value = 1.0 - page_size;

  adjustment->value          = value;
  adjustment->page_size      = page_size;
  adjustment->step_increment = page_size * GRAD_SCROLLBAR_STEP_SIZE;
  adjustment->page_increment = page_size * GRAD_SCROLLBAR_PAGE_SIZE;

  gtk_adjustment_changed (GTK_ADJUSTMENT (editor->scroll_data));
}

static void
gradient_editor_zoom_in_callback (GtkWidget          *widget,
                                  GimpGradientEditor *editor)
{
  GtkAdjustment *adjustment;
  gdouble        old_value;
  gdouble        old_page_size;
  gdouble        page_size;

  adjustment = GTK_ADJUSTMENT (editor->scroll_data);

  old_value     = adjustment->value;
  old_page_size = adjustment->page_size;

  editor->zoom_factor++;

  page_size = 1.0 / editor->zoom_factor;

  adjustment->value    	     = old_value + (old_page_size - page_size) / 2.0;
  adjustment->page_size      = page_size;
  adjustment->step_increment = page_size * GRAD_SCROLLBAR_STEP_SIZE;
  adjustment->page_increment = page_size * GRAD_SCROLLBAR_PAGE_SIZE;

  gtk_adjustment_changed (GTK_ADJUSTMENT (editor->scroll_data));
}

static void
gradient_editor_instant_update_update (GtkWidget          *widget,
                                       GimpGradientEditor *editor)
{
  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      editor->instant_update = TRUE;
      gtk_range_set_update_policy (GTK_RANGE (editor->scrollbar),
				   GTK_UPDATE_CONTINUOUS);
    }
  else
    {
      editor->instant_update = FALSE;
      gtk_range_set_update_policy (GTK_RANGE (editor->scrollbar),
				   GTK_UPDATE_DELAYED);
    }
}

static void
gradient_editor_set_hint (GimpGradientEditor *editor,
                          const gchar        *str1,
                          const gchar        *str2,
                          const gchar        *str3)
{
  gtk_label_set_text (GTK_LABEL (editor->hint_label1), str1);
  gtk_label_set_text (GTK_LABEL (editor->hint_label2), str2);
  gtk_label_set_text (GTK_LABEL (editor->hint_label3), str3);
}


/***** Gradient preview functions *****/

static gboolean
preview_events (GtkWidget          *widget,
		GdkEvent           *event,
		GimpGradientEditor *editor)
{
  gint            x, y;
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  GdkEventScroll *sevent;

  if (! GIMP_DATA_EDITOR (editor)->data)
    return TRUE;

  switch (event->type)
    {
    case GDK_EXPOSE:
      preview_update (editor, FALSE);
      return FALSE;

    case GDK_LEAVE_NOTIFY:
      gradient_editor_set_hint (editor, "", "", "");
      break;

    case GDK_MOTION_NOTIFY:
      gtk_widget_get_pointer (editor->preview, &x, &y);

      mevent = (GdkEventMotion *) event;

      if (x != editor->preview_last_x)
	{
	  editor->preview_last_x = x;

	  if (editor->preview_button_down)
	    {
	      if (mevent->state & GDK_CONTROL_MASK)
		preview_set_background (editor, x);
	      else
		preview_set_foreground (editor, x);
	    }
	  else
	    {
	      preview_set_hint (editor, x);
	    }
	}
      break;

    case GDK_BUTTON_PRESS:
      gtk_widget_get_pointer (editor->preview, &x, &y);

      bevent = (GdkEventButton *) event;

      switch (bevent->button)
	{
	case 1:
	  editor->preview_last_x = x;
	  editor->preview_button_down = TRUE;
	  if (bevent->state & GDK_CONTROL_MASK)
	    preview_set_background (editor, x);
	  else
	    preview_set_foreground (editor, x);
	  break;

	case 3:
          if (! GIMP_DATA_EDITOR (editor)->data->internal)
            {
              GimpItemFactory *factory;

              factory = gimp_item_factory_from_path ("<GradientEditor>");

              gimp_item_factory_popup_with_data (factory, editor, NULL);
            }
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
	    gradient_editor_zoom_in_callback (NULL, editor);
	  else
	    gradient_editor_zoom_out_callback (NULL, editor);
	}
      else
	{
	  GtkAdjustment *adj = GTK_ADJUSTMENT (editor->scroll_data);

	  gfloat new_value = adj->value + ((sevent->direction == GDK_SCROLL_UP) ?
					   -adj->page_increment / 2 :
					   adj->page_increment / 2);

	  new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);

	  gtk_adjustment_set_value (adj, new_value);
	}
      break;

    case GDK_BUTTON_RELEASE:
      if (editor->preview_button_down)
	{
	  gtk_widget_get_pointer (editor->preview, &x, &y);

	  bevent = (GdkEventButton *) event;

	  editor->preview_last_x = x;
	  editor->preview_button_down = FALSE;
	  if (bevent->state & GDK_CONTROL_MASK)
	    preview_set_background (editor, x);
	  else
	    preview_set_foreground (editor, x);
	  break;
	}
      break;

    default:
      break;
    }

  return TRUE;
}

static void
preview_set_hint (GimpGradientEditor *editor,
		  gint                x)
{
  GimpDataEditor *data_editor;
  gdouble         xpos;
  GimpRGB         rgb;
  GimpHSV         hsv;
  gchar          *str1;
  gchar          *str2;
  gchar          *str3;

  data_editor = GIMP_DATA_EDITOR (editor);

  xpos = control_calc_g_pos (editor, x);

  gimp_gradient_get_color_at (GIMP_GRADIENT (data_editor->data),
                              xpos, &rgb);

  gimp_rgb_to_hsv (&rgb, &hsv);

  str1 = g_strdup_printf (_("Position: %0.6f"), xpos);

  str2 = g_strdup_printf (_("RGB (%0.3f, %0.3f, %0.3f)    Opacity: %0.3f"),
                          rgb.r, rgb.g, rgb.b, rgb.a);

  str3 = g_strdup_printf (_("HSV (%0.3f, %0.3f, %0.3f)"),
                          hsv.h * 360.0, hsv.s, hsv.v);

  gradient_editor_set_hint (editor, str1, str2, str3);

  g_free (str1);
  g_free (str2);
  g_free (str3);
}

static void
preview_set_foreground (GimpGradientEditor *editor,
			gint                x)
{
  GimpGradient *gradient;
  GimpContext  *user_context;
  GimpRGB       color;
  gdouble       xpos;
  gchar        *str2;
  gchar        *str3;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  user_context = gimp_get_user_context (GIMP_DATA_EDITOR (editor)->gimp);

  xpos = control_calc_g_pos (editor, x);

  gimp_gradient_get_color_at (gradient, xpos, &color);

  gimp_context_set_foreground (user_context, &color);

  str2 = g_strdup_printf (_("RGB (%d, %d, %d)"),
                          (gint) (color.r * 255.0),
                          (gint) (color.g * 255.0),
                          (gint) (color.b * 255.0));

  str3 = g_strdup_printf ("(%0.3f, %0.3f, %0.3f)", color.r, color.g, color.b);

  gradient_editor_set_hint (editor, _("Foreground color set to:"), str2, str3);

  g_free (str2);
  g_free (str3);
}

static void
preview_set_background (GimpGradientEditor *editor,
			gint                x)
{
  GimpGradient *gradient;
  GimpContext  *user_context;
  GimpRGB       color;
  gdouble       xpos;
  gchar        *str2;
  gchar        *str3;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  user_context = gimp_get_user_context (GIMP_DATA_EDITOR (editor)->gimp);

  xpos = control_calc_g_pos (editor, x);

  gimp_gradient_get_color_at (gradient, xpos, &color);

  gimp_context_set_background (user_context, &color);

  str2 = g_strdup_printf (_("RGB (%d, %d, %d)"),
                          (gint) (color.r * 255.0),
                          (gint) (color.g * 255.0),
                          (gint) (color.b * 255.0));

  str3 = g_strdup_printf (_("(%0.3f, %0.3f, %0.3f)"),
                          color.r, color.g, color.b);

  gradient_editor_set_hint (editor, _("Background color set to:"), str2, str3);

  g_free (str2);
  g_free (str3);
}

/*****/

static void
preview_update (GimpGradientEditor *editor,
		gboolean            recalculate)
{
  glong          rowsiz;
  GtkAdjustment *adjustment;
  guint16        width;
  guint16        height;

  static guint16  last_width  = 0;
  static guint16  last_height = 0;

  if (! GTK_WIDGET_DRAWABLE (editor->preview))
    return;

  /*  See whether we have to re-create the preview widget
   *  (note that the preview automatically follows the size of it's container)
   */
  width  = editor->preview->allocation.width;
  height = editor->preview->allocation.height;

  if (! editor->preview_rows[0] ||
      ! editor->preview_rows[1] ||
      (width  != last_width)      ||
      (height != last_height))
    {
      if (editor->preview_rows[0])
	g_free (editor->preview_rows[0]);

      if (editor->preview_rows[1])
	g_free (editor->preview_rows[1]);

      rowsiz = width * 3 * sizeof (guchar);

      editor->preview_rows[0] = g_malloc (rowsiz);
      editor->preview_rows[1] = g_malloc (rowsiz);

      recalculate = TRUE; /* Force recalculation */
    }

  last_width = width;
  last_height = height;

  /* Have to redraw? */
  if (recalculate)
    {
      adjustment = GTK_ADJUSTMENT (editor->scroll_data);

      preview_fill_image (editor,
			  width, height,
			  adjustment->value,
			  adjustment->value + adjustment->page_size);

      gtk_widget_queue_draw (editor->preview);
    }
}

static void
preview_fill_image (GimpGradientEditor *editor,
		    gint                width,
		    gint                height,
		    gdouble             left,
		    gdouble             right)
{
  GimpGradient *gradient;
  guchar       *p0, *p1;
  gint          x, y;
  gdouble       dx, cur_x;
  GimpRGB       color;
  gdouble       c0, c1;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  dx    = (right - left) / (width - 1);
  cur_x = left;
  p0    = editor->preview_rows[0];
  p1    = editor->preview_rows[1];

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
	gtk_preview_draw_row (GTK_PREVIEW (editor->preview),
			      editor->preview_rows[1], 0, y, width);
      else
	gtk_preview_draw_row (GTK_PREVIEW (editor->preview),
			      editor->preview_rows[0], 0, y, width);
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
control_events (GtkWidget          *widget,
		GdkEvent           *event,
		GimpGradientEditor *editor)
{
  GimpGradient        *gradient;
  GdkEventButton      *bevent;
  GimpGradientSegment *seg;
  gint                 x, y;
  guint32              time;

  if (! GIMP_DATA_EDITOR (editor)->data)
    return TRUE;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  switch (event->type)
    {
    case GDK_EXPOSE:
      control_update (editor, gradient, FALSE);
      break;

    case GDK_LEAVE_NOTIFY:
      gradient_editor_set_hint (editor, "", "", "");
      break;

    case GDK_BUTTON_PRESS:
      if (editor->control_drag_mode == GRAD_DRAG_NONE)
	{
	  gtk_widget_get_pointer (editor->control, &x, &y);

	  bevent = (GdkEventButton *) event;

	  editor->control_last_x     = x;
	  editor->control_click_time = bevent->time;

	  control_button_press (editor,
				x, y, bevent->button, bevent->state);

	  if (editor->control_drag_mode != GRAD_DRAG_NONE)
	    gtk_grab_add (widget);
	}
      break;

    case GDK_BUTTON_RELEASE:
      gradient_editor_set_hint (editor, "", "", "");

      if (editor->control_drag_mode != GRAD_DRAG_NONE)
	{
	  gtk_grab_remove (widget);

	  gtk_widget_get_pointer (editor->control, &x, &y);

	  time = ((GdkEventButton *) event)->time;

	  if ((time - editor->control_click_time) >= GRAD_MOVE_TIME)
	    {
	      if (! editor->instant_update)
		gimp_data_dirty (GIMP_DATA (gradient));

              /* Possible move */
	      gimp_gradient_editor_update (editor, GRAD_UPDATE_GRADIENT);
	    }
	  else if ((editor->control_drag_mode == GRAD_DRAG_MIDDLE) ||
		   (editor->control_drag_mode == GRAD_DRAG_ALL))
	    {
	      seg = editor->control_drag_segment;

	      if ((editor->control_drag_mode == GRAD_DRAG_ALL) &&
		  editor->control_compress)
		control_extend_selection (editor, seg,
					  control_calc_g_pos (editor,
							      x));
	      else
		control_select_single_segment (editor, seg);

	      gimp_gradient_editor_update (editor, GRAD_UPDATE_CONTROL);
	    }

	  editor->control_drag_mode = GRAD_DRAG_NONE;
	  editor->control_compress  = FALSE;

	  control_do_hint (editor, x, y);
	}
      break;

    case GDK_MOTION_NOTIFY:
      gtk_widget_get_pointer (editor->control, &x, &y);

      if (x != editor->control_last_x)
	{
	  editor->control_last_x = x;

	  if (editor->control_drag_mode != GRAD_DRAG_NONE)
	    {
	      time = ((GdkEventButton *) event)->time;

	      if ((time - editor->control_click_time) >= GRAD_MOVE_TIME)
		control_motion (editor, gradient, x);
	    }
	  else
	    {
	      gimp_gradient_editor_update (editor, GRAD_UPDATE_CONTROL);

	      control_do_hint (editor, x, y);
	    }
	}
      break;

    default:
      break;
    }

  return TRUE;
}

static void
control_do_hint (GimpGradientEditor *editor,
		 gint                x,
		 gint                y)
{
  GimpGradient           *gradient;
  GimpGradientSegment    *seg;
  GradientEditorDragMode  handle;
  gboolean                in_handle;
  gdouble                 pos;
  gchar                  *str;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  pos = control_calc_g_pos (editor, x);

  if ((pos < 0.0) || (pos > 1.0))
    return;

  seg_get_closest_handle (gradient, pos, &seg, &handle);

  in_handle = control_point_in_handle (editor, gradient,
				       x, y, seg, handle);

  if (in_handle)
    {
      switch (handle)
	{
	case GRAD_DRAG_LEFT:
	  if (seg != NULL)
	    {
	      if (seg->prev != NULL)
                {
                  str = g_strdup_printf (_("%s%sDrag: move & compress"),
                                         gimp_get_mod_name_shift (),
                                         gimp_get_mod_separator ());

                  gradient_editor_set_hint (editor,
                                            "",
                                            _("Drag: move"),
                                            str);
                  g_free (str);
                }
	      else
                {
                  str = g_strdup_printf (_("%s%sClick: extend selection"),
                                         gimp_get_mod_name_shift (),
                                         gimp_get_mod_separator ());

                  gradient_editor_set_hint (editor,
                                            "",
                                            _("Click: select"),
                                            str);
                  g_free (str);
                }
	    }
	  else
	    {
              str = g_strdup_printf (_("%s%sClick: extend selection"),
                                     gimp_get_mod_name_shift (),
                                     gimp_get_mod_separator ());

	      gradient_editor_set_hint (editor,
                                        "",
                                        _("Click: select"),
                                        str);
              g_free (str);
	    }
	  break;

	case GRAD_DRAG_MIDDLE:
          str = g_strdup_printf (_("%s%sClick: extend selection"),
                                 gimp_get_mod_name_shift (),
                                 gimp_get_mod_separator ());

	  gradient_editor_set_hint (editor,
                                    "",
                                    _("Click: select    Drag: move"),
                                    str);
          g_free (str);
	  break;

	default:
          g_warning ("%s: in_handle is true, but received handle type %d.",
                     G_STRLOC, in_handle);
	  break;
	}
    }
  else
    {
      gchar *str2;

      str  = g_strdup_printf (_("%s%sClick: extend selection"),
                              gimp_get_mod_name_shift (),
                              gimp_get_mod_separator ());
      str2 = g_strdup_printf (_("%s%sDrag: move & compress"),
                              gimp_get_mod_name_shift (),
                              gimp_get_mod_separator ());

      gradient_editor_set_hint (editor,
                                _("Click: select    Drag: move"),
                                str,
                                str2);
      g_free (str);
      g_free (str2);
    }
}

static void
control_button_press (GimpGradientEditor *editor,
		      gint                x,
		      gint                y,
		      guint               button,
		      guint               state)
{
  GimpGradient           *gradient;
  GimpGradientSegment    *seg;
  GradientEditorDragMode  handle;
  double                  xpos;
  gboolean                in_handle;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  switch (button)
    {
    case 1:
      break;

    case 3:
      {
        GimpItemFactory *factory;

        factory = gimp_item_factory_from_path ("<GradientEditor>");

        gimp_item_factory_popup_with_data (factory, editor, NULL);
      }
      return;

      /*  wheelmouse support  */
    case 4:
      {
	GtkAdjustment *adj = GTK_ADJUSTMENT (editor->scroll_data);
	gfloat new_value   = adj->value - adj->page_increment / 2;

	new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	gtk_adjustment_set_value (adj, new_value);
      }
      return;

    case 5:
      {
	GtkAdjustment *adj = GTK_ADJUSTMENT (editor->scroll_data);
	gfloat new_value   = adj->value + adj->page_increment / 2;

	new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	gtk_adjustment_set_value (adj, new_value);
      }
      return;

    default:
      return;
    }

  /* Find the closest handle */

  xpos = control_calc_g_pos (editor, x);

  seg_get_closest_handle (gradient, xpos, &seg, &handle);

  in_handle = control_point_in_handle (editor, gradient,
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
		      editor->control_drag_mode    = GRAD_DRAG_LEFT;
		      editor->control_drag_segment = seg;
		      editor->control_compress     = TRUE;
		    }
		  else
		    {
		      control_extend_selection (editor, seg, xpos);
		      gimp_gradient_editor_update (editor, GRAD_UPDATE_CONTROL);
		    }
		}
	      else if (seg->prev != NULL)
		{
		  editor->control_drag_mode    = GRAD_DRAG_LEFT;
		  editor->control_drag_segment = seg;
		}
	      else
		{
		  control_select_single_segment (editor, seg);
		  gimp_gradient_editor_update (editor, GRAD_UPDATE_CONTROL);
		}

	      return;
	    }
	  else  /* seg == NULL */
	    {
	      /* Right handle of last segment */
	      seg = gimp_gradient_segment_get_last (gradient->segments);

	      if (state & GDK_SHIFT_MASK)
		{
		  control_extend_selection (editor, seg, xpos);
		  gimp_gradient_editor_update (editor, GRAD_UPDATE_CONTROL);
		}
	      else
		{
		  control_select_single_segment (editor, seg);
		  gimp_gradient_editor_update (editor, GRAD_UPDATE_CONTROL);
		}

	      return;
	    }

	  break;

	case GRAD_DRAG_MIDDLE:
	  if (state & GDK_SHIFT_MASK)
	    {
	      control_extend_selection (editor, seg, xpos);
	      gimp_gradient_editor_update (editor, GRAD_UPDATE_CONTROL);
	    }
	  else
	    {
	      editor->control_drag_mode    = GRAD_DRAG_MIDDLE;
	      editor->control_drag_segment = seg;
	    }

	  return;

	default:
          g_warning ("%s: in_handle is true, but received handle type %d.",
                     G_STRLOC, in_handle);
	  return;
	}
    }
  else  /* !in_handle */
    {
      seg = gimp_gradient_get_segment_at (gradient, xpos);

      editor->control_drag_mode    = GRAD_DRAG_ALL;
      editor->control_drag_segment = seg;
      editor->control_last_gx      = xpos;
      editor->control_orig_pos     = xpos;

      if (state & GDK_SHIFT_MASK)
	editor->control_compress = TRUE;

      return;
    }
}

static gboolean
control_point_in_handle (GimpGradientEditor     *editor,
			 GimpGradient           *gradient,
			 gint                    x,
			 gint                    y,
			 GimpGradientSegment    *seg,
			 GradientEditorDragMode  handle)
{
  gint handle_pos;

  switch (handle)
    {
    case GRAD_DRAG_LEFT:
      if (seg)
	{
	  handle_pos = control_calc_p_pos (editor, seg->left);
	}
      else
	{
	  seg = gimp_gradient_segment_get_last (gradient->segments);

	  handle_pos = control_calc_p_pos (editor, seg->right);
	}

      break;

    case GRAD_DRAG_MIDDLE:
      handle_pos = control_calc_p_pos (editor, seg->middle);
      break;

    default:
      g_warning ("%s: Cannot handle drag mode %d.", G_STRLOC, handle);
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
control_select_single_segment (GimpGradientEditor  *editor,
			       GimpGradientSegment *seg)
{
  editor->control_sel_l = seg;
  editor->control_sel_r = seg;
}

static void
control_extend_selection (GimpGradientEditor  *editor,
			  GimpGradientSegment *seg,
			  gdouble              pos)
{
  if (fabs (pos - editor->control_sel_l->left) <
      fabs (pos - editor->control_sel_r->right))
    editor->control_sel_l = seg;
  else
    editor->control_sel_r = seg;
}

/*****/

static void
control_motion (GimpGradientEditor *editor,
		GimpGradient       *gradient,
		gint                x)
{
  GimpGradientSegment *seg;
  gdouble              pos;
  gdouble              delta;
  gchar               *str = NULL;

  seg = editor->control_drag_segment;

  switch (editor->control_drag_mode)
    {
    case GRAD_DRAG_LEFT:
      pos = control_calc_g_pos (editor, x);

      if (! editor->control_compress)
	seg->prev->right = seg->left = CLAMP (pos,
					      seg->prev->middle + EPSILON,
					      seg->middle - EPSILON);
      else
	control_compress_left (editor->control_sel_l,
			       editor->control_sel_r,
			       seg, pos);

      str = g_strdup_printf (_("Handle position: %0.6f"), seg->left);
      gradient_editor_set_hint (editor, str, "", "");

      break;

    case GRAD_DRAG_MIDDLE:
      pos = control_calc_g_pos (editor, x);
      seg->middle = CLAMP (pos, seg->left + EPSILON, seg->right - EPSILON);

      str = g_strdup_printf (_("Handle position: %0.6f"), seg->middle);
      gradient_editor_set_hint (editor, str, "", "");

      break;

    case GRAD_DRAG_ALL:
      pos    = control_calc_g_pos (editor, x);
      delta  = pos - editor->control_last_gx;

      if ((seg->left >= editor->control_sel_l->left) &&
	  (seg->right <= editor->control_sel_r->right))
	delta = control_move (editor,
			      editor->control_sel_l,
			      editor->control_sel_r, delta);
      else
	delta = control_move (editor, seg, seg, delta);

      editor->control_last_gx += delta;

      str = g_strdup_printf (_("Distance: %0.6f"),
			     editor->control_last_gx -
			     editor->control_orig_pos);
      gradient_editor_set_hint (editor, str, "", "");

      break;

    default:
      g_warning ("%s: Attempting to move bogus handle %d.",
                 G_STRLOC, editor->control_drag_mode);
      break;
    }

  if (str)
    g_free (str);

  if (editor->instant_update)
    {
      gimp_data_dirty (GIMP_DATA (gradient));

      gimp_gradient_editor_update (editor,
                                   GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
    }
  else
    {
      gimp_gradient_editor_update (editor, GRAD_UPDATE_CONTROL);
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
			gdouble              new_l,
			gdouble              new_r)
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
control_move (GimpGradientEditor  *editor,
	      GimpGradientSegment *range_l,
	      GimpGradientSegment *range_r,
	      gdouble              delta)
{
  gdouble              lbound, rbound;
  gint                 is_first, is_last;
  GimpGradientSegment *seg, *aseg;

  /* First or last segments in gradient? */

  is_first = (range_l->prev == NULL);
  is_last  = (range_r->next == NULL);

  /* Calculate drag bounds */

  if (! editor->control_compress)
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
      if (! editor->control_compress)
	range_l->prev->right = range_l->left;
      else
	control_compress_range (range_l->prev, range_l->prev,
				range_l->prev->left, range_l->left);
    }

  if (!is_last)
    {
      if (! editor->control_compress)
	range_r->next->left = range_r->right;
      else
	control_compress_range (range_r->next, range_r->next,
				range_r->right, range_r->next->right);
    }

  return delta;
}

/*****/

static void
control_update (GimpGradientEditor *editor,
		GimpGradient       *gradient,
		gboolean            recalculate)
{
  GtkAdjustment *adjustment;
  gint           cwidth, cheight;
  gint           pwidth  = 0;
  gint           pheight = 0;

  if (! GTK_WIDGET_DRAWABLE (editor->control))
    return;

  /*  See whether we have to re-create the control pixmap
   *  depending on the preview's width
   */
  cwidth  = editor->preview->allocation.width;
  cheight = GRAD_CONTROL_HEIGHT;

  if (editor->control_pixmap)
    gdk_drawable_get_size (editor->control_pixmap, &pwidth, &pheight);

  if (! editor->control_pixmap || (cwidth != pwidth) || (cheight != pheight))
    {
      if (editor->control_pixmap)
	g_object_unref (editor->control_pixmap);

      editor->control_pixmap =
	gdk_pixmap_new (editor->control->window, cwidth, cheight, -1);

      recalculate = TRUE;
    }

  /* Have to reset the selection? */
  if (recalculate)
    control_select_single_segment (editor, gradient->segments);

  /* Redraw pixmap */
  adjustment = GTK_ADJUSTMENT (editor->scroll_data);

  control_draw (editor,
		gradient,
		editor->control_pixmap,
		cwidth, cheight,
		adjustment->value,
		adjustment->value + adjustment->page_size);

  gdk_draw_drawable (editor->control->window,
		     editor->control->style->black_gc,
		     editor->control_pixmap,
		     0, 0, 0, 0,
		     cwidth, cheight);
}

static void
control_draw (GimpGradientEditor *editor,
	      GimpGradient       *gradient,
	      GdkPixmap          *pixmap,
	      gint                width,
	      gint                height,
	      gdouble             left,
	      gdouble             right)
{
  gint 	                  sel_l, sel_r;
  gdouble                 g_pos;
  GimpGradientSegment    *seg;
  GradientEditorDragMode  handle;

  /* Clear the pixmap */

  gdk_draw_rectangle (pixmap, editor->control->style->bg_gc[GTK_STATE_NORMAL],
		      TRUE, 0, 0, width, height);

  /* Draw selection */

  sel_l = control_calc_p_pos (editor,
			      editor->control_sel_l->left);
  sel_r = control_calc_p_pos (editor,
			      editor->control_sel_r->right);

  gdk_draw_rectangle (pixmap,
		      editor->control->style->dark_gc[GTK_STATE_NORMAL],
		      TRUE, sel_l, 0, sel_r - sel_l + 1, height);

  /* Draw handles */

  seg = gradient->segments;

  while (seg)
    {
      control_draw_normal_handle (editor, pixmap, seg->left, height);
      control_draw_middle_handle (editor, pixmap, seg->middle, height);

      /* Draw right handle only if this is the last segment */

      if (seg->next == NULL)
	control_draw_normal_handle (editor, pixmap, seg->right, height);

      /* Next! */

      seg = seg->next;
    }

  /* Draw the handle which is closest to the mouse position */

  g_pos = control_calc_g_pos (editor, editor->control_last_x);

  seg_get_closest_handle (gradient, CLAMP (g_pos, 0.0, 1.0), &seg, &handle);

  switch (handle)
    {
    case GRAD_DRAG_LEFT:
      if (seg)
	{
	  control_draw_normal_handle (editor, pixmap,
				      seg->left, height);
	}
      else
	{
	  seg = gimp_gradient_segment_get_last (gradient->segments);

	  control_draw_normal_handle (editor, pixmap,
				      seg->right, height);
	}

      break;

    case GRAD_DRAG_MIDDLE:
      control_draw_middle_handle (editor, pixmap, seg->middle, height);
      break;

    default:
      break;
    }
}

static void
control_draw_normal_handle (GimpGradientEditor *editor,
			    GdkPixmap          *pixmap,
			    gdouble             pos,
			    gint                height)
{
  control_draw_handle (pixmap,
		       editor->control->style->black_gc,
		       editor->control->style->black_gc,
		       control_calc_p_pos (editor, pos), height);
}

static void
control_draw_middle_handle (GimpGradientEditor *editor,
			    GdkPixmap          *pixmap,
			    gdouble             pos,
			    gint                height)
{
  control_draw_handle (pixmap,
		       editor->control->style->black_gc,
		       editor->control->style->bg_gc[GTK_STATE_PRELIGHT],
		       control_calc_p_pos (editor, pos), height);
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
control_calc_p_pos (GimpGradientEditor *editor,
		    gdouble             pos)
{
  gint           pwidth, pheight;
  GtkAdjustment *adjustment;

  /* Calculate the position (in widget's coordinates) of the
   * requested point from the gradient.  Rounding is done to
   * minimize mismatches between the rendered gradient preview
   * and the gradient control's handles.
   */

  adjustment = GTK_ADJUSTMENT (editor->scroll_data);
  gdk_drawable_get_size (editor->control_pixmap, &pwidth, &pheight);

  return RINT ((pwidth - 1) * (pos - adjustment->value) / adjustment->page_size);
}

static gdouble
control_calc_g_pos (GimpGradientEditor *editor,
		    gint                pos)
{
  gint           pwidth, pheight;
  GtkAdjustment *adjustment;

  /* Calculate the gradient position that corresponds to widget's coordinates */

  adjustment = GTK_ADJUSTMENT (editor->scroll_data);
  gdk_drawable_get_size (editor->control_pixmap, &pwidth, &pheight);

  return adjustment->page_size * pos / (pwidth - 1) + adjustment->value;
}

/***** Segment functions *****/

static void
seg_get_closest_handle (GimpGradient            *grad,
			gdouble                  pos,
			GimpGradientSegment    **seg,
			GradientEditorDragMode  *handle)
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
