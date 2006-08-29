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
#include "gimpdocked.h"
#include "gimpgradienteditor.h"
#include "gimphelp-ids.h"
#include "gimpview.h"
#include "gimpviewrenderergradient.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


#define EPSILON 1e-10

#define GRAD_SCROLLBAR_STEP_SIZE 0.05
#define GRAD_SCROLLBAR_PAGE_SIZE 0.75

#define GRAD_VIEW_WIDTH     128
#define GRAD_VIEW_HEIGHT     96
#define GRAD_CONTROL_HEIGHT  10

#define GRAD_MOVE_TIME 150 /* ms between mouse click and detection of movement in gradient control */


#define GRAD_VIEW_EVENT_MASK (GDK_EXPOSURE_MASK            | \
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

static void  gimp_gradient_editor_docked_iface_init (GimpDockedInterface *face);

static GObject * gimp_gradient_editor_constructor   (GType               type,
                                                     guint               n_params,
                                                     GObjectConstructParam *params);

static void   gimp_gradient_editor_destroy          (GtkObject          *object);
static void   gimp_gradient_editor_unmap            (GtkWidget          *widget);
static void   gimp_gradient_editor_unrealize        (GtkWidget          *widget);
static void   gimp_gradient_editor_set_data         (GimpDataEditor     *editor,
                                                     GimpData           *data);

static void   gimp_gradient_editor_set_context      (GimpDocked         *docked,
                                                     GimpContext        *context);

static void   gimp_gradient_editor_gradient_dirty   (GimpGradientEditor *editor,
                                                     GimpGradient       *gradient);
static void   gradient_editor_drop_gradient         (GtkWidget          *widget,
                                                     gint                x,
                                                     gint                y,
                                                     GimpViewable       *viewable,
                                                     gpointer            data);
static void   gradient_editor_drop_color            (GtkWidget          *widget,
                                                     gint                x,
                                                     gint                y,
                                                     const GimpRGB      *color,
                                                     gpointer            data);
static void   gradient_editor_control_drop_color    (GtkWidget          *widget,
                                                     gint                x,
                                                     gint                y,
                                                     const GimpRGB      *color,
                                                     gpointer            data);
static void   gradient_editor_scrollbar_update      (GtkAdjustment      *adj,
                                                     GimpGradientEditor *editor);
static void   gradient_editor_instant_update_update (GtkWidget          *widget,
                                                     GimpGradientEditor *editor);

static void   gradient_editor_set_hint              (GimpGradientEditor *editor,
                                                     const gchar        *str1,
                                                     const gchar        *str2,
                                                     const gchar        *str3,
                                                     const gchar        *str4);


/* Gradient view functions */

static gboolean  view_events                      (GtkWidget          *widget,
                                                   GdkEvent           *event,
                                                   GimpGradientEditor *editor);
static void      view_set_hint                    (GimpGradientEditor *editor,
                                                   gint                x);

static void      view_set_foreground              (GimpGradientEditor *editor,
                                                   gint                x);
static void      view_set_background              (GimpGradientEditor *editor,
                                                   gint                x);

/* Gradient control functions */

static gboolean  control_events                   (GtkWidget          *widget,
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

static void      control_compress_left            (GimpGradient        *gradient,
                                                   GimpGradientSegment *range_l,
                                                   GimpGradientSegment *range_r,
                                                   GimpGradientSegment *drag_seg,
                                                   gdouble              pos);

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
                                                   gint                height,
                                                   gboolean            selected);
static void      control_draw_middle_handle       (GimpGradientEditor *editor,
                                                   GdkPixmap          *pixmap,
                                                   gdouble             pos,
                                                   gint                height,
                                                   gboolean            selected);
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
static gboolean  seg_in_selection                 (GimpGradient         *grad,
                                                   GimpGradientSegment  *seg,
                                                   GimpGradientSegment  *left,
                                                   GimpGradientSegment  *right);

static GtkWidget * gradient_hint_label_add        (GtkBox *box);


G_DEFINE_TYPE_WITH_CODE (GimpGradientEditor, gimp_gradient_editor,
                         GIMP_TYPE_DATA_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_gradient_editor_docked_iface_init))

#define parent_class gimp_gradient_editor_parent_class

static GimpDockedInterface *parent_docked_iface = NULL;


static void
gimp_gradient_editor_class_init (GimpGradientEditorClass *klass)
{
  GObjectClass        *object_class     = G_OBJECT_CLASS (klass);
  GtkObjectClass      *gtk_object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass      *widget_class     = GTK_WIDGET_CLASS (klass);
  GimpDataEditorClass *editor_class     = GIMP_DATA_EDITOR_CLASS (klass);

  object_class->constructor = gimp_gradient_editor_constructor;

  gtk_object_class->destroy = gimp_gradient_editor_destroy;

  widget_class->unmap       = gimp_gradient_editor_unmap;
  widget_class->unrealize   = gimp_gradient_editor_unrealize;

  editor_class->set_data    = gimp_gradient_editor_set_data;
  editor_class->title       = _("Gradient Editor");
}

static void
gimp_gradient_editor_docked_iface_init (GimpDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (GIMP_TYPE_DOCKED);

  iface->set_context = gimp_gradient_editor_set_context;
}

static void
gimp_gradient_editor_init (GimpGradientEditor *editor)
{
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *button;

  /* Frame for gradient view and gradient control */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /* Gradient view */
  editor->view_last_x      = 0;
  editor->view_button_down = FALSE;

  editor->view = gimp_view_new_full_by_types (NULL,
                                              GIMP_TYPE_VIEW,
                                              GIMP_TYPE_GRADIENT,
                                              GRAD_VIEW_WIDTH,
                                              GRAD_VIEW_HEIGHT, 0,
                                              FALSE, FALSE, FALSE);
  gtk_widget_set_size_request (editor->view,
                               GRAD_VIEW_WIDTH, GRAD_VIEW_HEIGHT);
  gtk_widget_set_events (editor->view, GRAD_VIEW_EVENT_MASK);
  gimp_view_set_expand (GIMP_VIEW (editor->view), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), editor->view, TRUE, TRUE, 0);
  gtk_widget_show (editor->view);

  g_signal_connect (editor->view, "event",
                    G_CALLBACK (view_events),
                    editor);

  gimp_dnd_viewable_dest_add (GTK_WIDGET (editor->view),
                              GIMP_TYPE_GRADIENT,
                              gradient_editor_drop_gradient,
                              editor);

  gimp_dnd_color_dest_add (GTK_WIDGET (editor->view),
                              gradient_editor_drop_color,
                              editor);

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
                               GRAD_VIEW_WIDTH, GRAD_CONTROL_HEIGHT);
  gtk_widget_set_events (editor->control, GRAD_CONTROL_EVENT_MASK);
  gtk_box_pack_start (GTK_BOX (vbox), editor->control, FALSE, FALSE, 0);
  gtk_widget_show (editor->control);

  g_signal_connect (editor->control, "event",
                    G_CALLBACK (control_events),
                    editor);

  gimp_dnd_color_dest_add (GTK_WIDGET (editor->control),
                              gradient_editor_control_drop_color,
                              editor);

  /*  Scrollbar  */
  editor->zoom_factor = 1;

  editor->scroll_data = gtk_adjustment_new (0.0, 0.0, 1.0,
                                            GRAD_SCROLLBAR_STEP_SIZE,
                                            GRAD_SCROLLBAR_PAGE_SIZE,
                                            1.0);

  g_signal_connect (editor->scroll_data, "value-changed",
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

  /* Instant update toggle */
  editor->instant_update = TRUE;

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_check_button_new_with_label (_("Instant update"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                editor->instant_update);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gradient_editor_instant_update_update),
                    editor);

  /* Hint bar */
  editor->hint_label1 = gradient_hint_label_add (GTK_BOX (editor));
  editor->hint_label2 = gradient_hint_label_add (GTK_BOX (editor));
  editor->hint_label3 = gradient_hint_label_add (GTK_BOX (editor));
  editor->hint_label4 = gradient_hint_label_add (GTK_BOX (editor));

  /* Initialize other data */
  editor->left_saved_segments = NULL;
  editor->left_saved_dirty    = FALSE;

  editor->right_saved_segments = NULL;
  editor->right_saved_dirty    = FALSE;

  /* Black, 50% Gray, White, Clear */
  gimp_rgba_set (&editor->saved_colors[0], 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);
  gimp_rgba_set (&editor->saved_colors[1], 0.5, 0.5, 0.5, GIMP_OPACITY_OPAQUE);
  gimp_rgba_set (&editor->saved_colors[2], 1.0, 1.0, 1.0, GIMP_OPACITY_OPAQUE);
  gimp_rgba_set (&editor->saved_colors[3], 0.0, 0.0, 0.0, GIMP_OPACITY_TRANSPARENT);

  /* Red, Yellow, Green, Cyan, Blue, Magenta */
  gimp_rgba_set (&editor->saved_colors[4], 1.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);
  gimp_rgba_set (&editor->saved_colors[5], 1.0, 1.0, 0.0, GIMP_OPACITY_OPAQUE);
  gimp_rgba_set (&editor->saved_colors[6], 0.0, 1.0, 0.0, GIMP_OPACITY_OPAQUE);
  gimp_rgba_set (&editor->saved_colors[7], 0.0, 1.0, 1.0, GIMP_OPACITY_OPAQUE);
  gimp_rgba_set (&editor->saved_colors[8], 0.0, 0.0, 1.0, GIMP_OPACITY_OPAQUE);
  gimp_rgba_set (&editor->saved_colors[9], 1.0, 0.0, 1.0, GIMP_OPACITY_OPAQUE);
}

static GObject *
gimp_gradient_editor_constructor (GType                  type,
                                  guint                  n_params,
                                  GObjectConstructParam *params)
{
  GObject            *object;
  GimpGradientEditor *editor;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor = GIMP_GRADIENT_EDITOR (object);

  gimp_editor_add_action_button (GIMP_EDITOR (editor), "gradient-editor",
                                 "gradient-editor-zoom-out", NULL);

  gimp_editor_add_action_button (GIMP_EDITOR (editor), "gradient-editor",
                                 "gradient-editor-zoom-in", NULL);

  gimp_editor_add_action_button (GIMP_EDITOR (editor), "gradient-editor",
                                 "gradient-editor-zoom-all", NULL);

  return object;
}

static void
gimp_gradient_editor_destroy (GtkObject *object)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (object);

  if (editor->color_dialog)
    gtk_dialog_response (GTK_DIALOG (editor->color_dialog),
                         GTK_RESPONSE_CANCEL);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_gradient_editor_unmap (GtkWidget *widget)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (widget);

  if (editor->color_dialog)
    gtk_dialog_response (GTK_DIALOG (editor->color_dialog),
                         GTK_RESPONSE_CANCEL);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
gimp_gradient_editor_unrealize (GtkWidget *widget)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (widget);

  if (editor->control_pixmap)
    {
      g_object_unref (editor->control_pixmap);
      editor->control_pixmap = NULL;
    }

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
gimp_gradient_editor_set_data (GimpDataEditor *editor,
                               GimpData       *data)
{
  GimpGradientEditor *gradient_editor = GIMP_GRADIENT_EDITOR (editor);
  GimpData           *old_data;

  if (gradient_editor->color_dialog)
    gtk_dialog_response (GTK_DIALOG (gradient_editor->color_dialog),
                         GTK_RESPONSE_CANCEL);

  old_data = gimp_data_editor_get_data (editor);

  if (old_data)
    g_signal_handlers_disconnect_by_func (old_data,
                                          gimp_gradient_editor_gradient_dirty,
                                          gradient_editor);

  GIMP_DATA_EDITOR_CLASS (parent_class)->set_data (editor, data);

  if (data)
    g_signal_connect_swapped (data, "dirty",
                              G_CALLBACK (gimp_gradient_editor_gradient_dirty),
                              gradient_editor);

  gimp_view_set_viewable (GIMP_VIEW (gradient_editor->view),
                          GIMP_VIEWABLE (data));

  control_update (gradient_editor, GIMP_GRADIENT (data), TRUE);
}

static void
gimp_gradient_editor_set_context (GimpDocked  *docked,
                                  GimpContext *context)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (docked);

  parent_docked_iface->set_context (docked, context);

  gimp_view_renderer_set_context (GIMP_VIEW (editor->view)->renderer,
                                  context);
}


/*  public functions  */

GtkWidget *
gimp_gradient_editor_new (GimpContext     *context,
                          GimpMenuFactory *menu_factory)
{
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return g_object_new (GIMP_TYPE_GRADIENT_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<GradientEditor>",
                       "ui-path",         "/gradient-editor-popup",
                       "data-factory",    context->gimp->gradient_factory,
                       "context",         context,
                       "data",            gimp_context_get_gradient (context),
                       NULL);
}

void
gimp_gradient_editor_update (GimpGradientEditor *editor)
{
  GimpGradient *gradient = NULL;

  g_return_if_fail (GIMP_IS_GRADIENT_EDITOR (editor));

  if (GIMP_DATA_EDITOR (editor)->data)
    gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  control_update (editor, gradient, FALSE);
}

void
gimp_gradient_editor_zoom (GimpGradientEditor *editor,
                           GimpZoomType        zoom_type)
{
  GtkAdjustment *adjustment;
  gdouble        old_value;
  gdouble        old_page_size;
  gdouble        value     = 0.0;
  gdouble        page_size = 1.0;

  g_return_if_fail (GIMP_IS_GRADIENT_EDITOR (editor));

  adjustment = GTK_ADJUSTMENT (editor->scroll_data);

  old_value     = adjustment->value;
  old_page_size = adjustment->page_size;

  switch (zoom_type)
    {
    case GIMP_ZOOM_IN:
      editor->zoom_factor++;

      page_size = 1.0 / editor->zoom_factor;
      value     = old_value + (old_page_size - page_size) / 2.0;
      break;

    case GIMP_ZOOM_OUT:
      if (editor->zoom_factor <= 1)
        return;

      editor->zoom_factor--;

      page_size = 1.0 / editor->zoom_factor;
      value     = old_value - (page_size - old_page_size) / 2.0;

      if (value < 0.0)
        value = 0.0;
      else if ((value + page_size) > 1.0)
        value = 1.0 - page_size;
      break;

    case GIMP_ZOOM_TO: /* abused as ZOOM_ALL */
      editor->zoom_factor = 1;

      value     = 0.0;
      page_size = 1.0;
      break;
    }

  adjustment->value          = value;
  adjustment->page_size      = page_size;
  adjustment->step_increment = page_size * GRAD_SCROLLBAR_STEP_SIZE;
  adjustment->page_increment = page_size * GRAD_SCROLLBAR_PAGE_SIZE;

  gtk_adjustment_changed (GTK_ADJUSTMENT (editor->scroll_data));
}


/*  private functions  */

static void
gimp_gradient_editor_gradient_dirty (GimpGradientEditor *editor,
                                     GimpGradient       *gradient)
{
  GimpGradientSegment *segment;
  gboolean             left_seen  = FALSE;
  gboolean             right_seen = FALSE;

  for (segment = gradient->segments; segment; segment = segment->next)
    {
      if (segment == editor->control_sel_l)
        left_seen = TRUE;

      if (segment == editor->control_sel_r)
        right_seen = TRUE;

      if (right_seen && ! left_seen)
        {
          GimpGradientSegment *tmp;

          tmp = editor->control_sel_l;
          editor->control_sel_l = editor->control_sel_r;
          editor->control_sel_r = tmp;

          right_seen = FALSE;
          left_seen  = TRUE;
        }
    }

  control_update (editor, gradient, ! (left_seen && right_seen));
}

static void
gradient_editor_drop_gradient (GtkWidget    *widget,
                               gint          x,
                               gint          y,
                               GimpViewable *viewable,
                               gpointer      data)
{
  gimp_data_editor_set_data (GIMP_DATA_EDITOR (data), GIMP_DATA (viewable));
}

static void
gradient_editor_drop_color (GtkWidget     *widget,
                            gint           x,
                            gint           y,
                            const GimpRGB *color,
                            gpointer       data)
{
  GimpGradientEditor  *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient        *gradient;
  gdouble              xpos;
  GimpGradientSegment *seg, *lseg, *rseg;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  xpos = control_calc_g_pos (editor, x);
  seg = gimp_gradient_get_segment_at (gradient, xpos);

  gimp_data_freeze (GIMP_DATA (gradient));

  gimp_gradient_segment_split_midpoint (gradient, editor->context,
                                        seg, &lseg, &rseg);

  if (lseg)
    {
      lseg->right = xpos;
      lseg->middle = (lseg->left + lseg->right) / 2.0;
      lseg->right_color = *color;
    }

  if (rseg)
    {
      rseg->left = xpos;
      rseg->middle = (rseg->left + rseg->right) / 2.0;
      rseg->left_color  = *color;
    }

  gimp_data_thaw (GIMP_DATA (gradient));
}

static void
gradient_editor_control_drop_color (GtkWidget     *widget,
                                    gint           x,
                                    gint           y,
                                    const GimpRGB *color,
                                    gpointer       data)
{
  GimpGradientEditor    *editor = GIMP_GRADIENT_EDITOR (data);
  GimpGradient          *gradient;
  gdouble                xpos;
  GimpGradientSegment   *seg, *lseg, *rseg;
  GradientEditorDragMode handle;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  xpos = control_calc_g_pos (editor, x);
  seg_get_closest_handle (gradient, xpos, &seg, &handle);

  if (seg)
    {
      if (handle == GRAD_DRAG_LEFT)
        {
          lseg = seg->prev;
          rseg = seg;
        }
      else
        return;
    }
  else
    {
      lseg = gimp_gradient_get_segment_at (gradient, xpos);
      rseg = NULL;
    }

  gimp_data_freeze (GIMP_DATA (gradient));

  if (lseg)
    lseg->right_color = *color;

  if (rseg)
    rseg->left_color  = *color;

  gimp_data_thaw (GIMP_DATA (gradient));
}

static void
gradient_editor_scrollbar_update (GtkAdjustment      *adjustment,
                                  GimpGradientEditor *editor)
{
  GimpViewRendererGradient *renderer;
  gchar                    *str1;
  gchar                    *str2;

  str1 = g_strdup_printf (_("Zoom factor: %d:1"),
                          editor->zoom_factor);

  str2 = g_strdup_printf (_("Displaying [%0.6f, %0.6f]"),
                          adjustment->value,
                          adjustment->value + adjustment->page_size);

  gradient_editor_set_hint (editor, str1, str2, NULL, NULL);

  g_free (str1);
  g_free (str2);

  renderer = GIMP_VIEW_RENDERER_GRADIENT (GIMP_VIEW (editor->view)->renderer);

  gimp_view_renderer_gradient_set_offsets (renderer,
                                           adjustment->value,
                                           adjustment->value +
                                           adjustment->page_size,
                                           editor->instant_update);
  gimp_gradient_editor_update (editor);
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
                          const gchar        *str3,
                          const gchar        *str4)
{
  gtk_label_set_text (GTK_LABEL (editor->hint_label1), str1);
  gtk_label_set_text (GTK_LABEL (editor->hint_label2), str2);
  gtk_label_set_text (GTK_LABEL (editor->hint_label3), str3);
  gtk_label_set_text (GTK_LABEL (editor->hint_label4), str4);

}


/***** Gradient view functions *****/

static gboolean
view_events (GtkWidget          *widget,
             GdkEvent           *event,
             GimpGradientEditor *editor)
{
  gint x, y;

  if (! GIMP_DATA_EDITOR (editor)->data)
    return TRUE;

  switch (event->type)
    {
    case GDK_LEAVE_NOTIFY:
      gradient_editor_set_hint (editor, NULL, NULL, NULL, NULL);
      break;

    case GDK_MOTION_NOTIFY:
      {
        GdkEventMotion *mevent = (GdkEventMotion *) event;

        gtk_widget_get_pointer (editor->view, &x, &y);

        if (x != editor->view_last_x)
          {
            editor->view_last_x = x;

            if (editor->view_button_down)
              {
                if (mevent->state & GDK_CONTROL_MASK)
                  view_set_background (editor, x);
                else
                  view_set_foreground (editor, x);
              }
            else
              {
                view_set_hint (editor, x);
              }
          }
      }
      break;

    case GDK_BUTTON_PRESS:
      {
        GdkEventButton *bevent = (GdkEventButton *) event;

        gtk_widget_get_pointer (editor->view, &x, &y);

        switch (bevent->button)
          {
          case 1:
            editor->view_last_x = x;
            editor->view_button_down = TRUE;
            if (bevent->state & GDK_CONTROL_MASK)
              view_set_background (editor, x);
            else
              view_set_foreground (editor, x);
            break;

          case 3:
            gimp_editor_popup_menu (GIMP_EDITOR (editor), NULL, NULL);
            break;

          default:
            break;
          }
      }
      break;

    case GDK_SCROLL:
      {
        GdkEventScroll *sevent = (GdkEventScroll *) event;

        if (sevent->state & GDK_CONTROL_MASK)
          {
            switch (sevent->direction)
              {
              case GDK_SCROLL_UP:
                gimp_gradient_editor_zoom (editor, GIMP_ZOOM_IN);
                break;

              case GDK_SCROLL_DOWN:
                gimp_gradient_editor_zoom (editor, GIMP_ZOOM_OUT);
                break;

              default:
                break;
              }
          }
        else
          {
            GtkAdjustment *adj   = GTK_ADJUSTMENT (editor->scroll_data);
            gfloat         value = adj->value;

            switch (sevent->direction)
              {
              case GDK_SCROLL_UP:
                value -= adj->page_increment / 2;
                break;

              case GDK_SCROLL_DOWN:
                value += adj->page_increment / 2;
                break;

              default:
                break;
              }

            value = CLAMP (value, adj->lower, adj->upper - adj->page_size);

            gtk_adjustment_set_value (adj, value);
          }
      }
      break;

    case GDK_BUTTON_RELEASE:
      if (editor->view_button_down)
        {
          GdkEventButton *bevent = (GdkEventButton *) event;

          gtk_widget_get_pointer (editor->view, &x, &y);

          editor->view_last_x = x;
          editor->view_button_down = FALSE;
          if (bevent->state & GDK_CONTROL_MASK)
            view_set_background (editor, x);
          else
            view_set_foreground (editor, x);
          break;
        }
      break;

    default:
      return FALSE;
    }

  return TRUE;
}

static void
view_set_hint (GimpGradientEditor *editor,
               gint                x)
{
  GimpDataEditor *data_editor;
  gdouble         xpos;
  GimpRGB         rgb;
  GimpHSV         hsv;
  gchar          *str1;
  gchar          *str2;
  gchar          *str3;
  gchar          *str4;

  data_editor = GIMP_DATA_EDITOR (editor);

  xpos = control_calc_g_pos (editor, x);

  gimp_gradient_get_color_at (GIMP_GRADIENT (data_editor->data),
                              editor->context, NULL,
                              xpos, FALSE, &rgb);

  gimp_rgb_to_hsv (&rgb, &hsv);

  str1 = g_strdup_printf (_("Position: %0.6f"), xpos);
  str2 = g_strdup_printf (_("RGB (%0.3f, %0.3f, %0.3f)"),
                          rgb.r, rgb.g, rgb.b);
  str3 = g_strdup_printf (_("HSV (%0.3f, %0.3f, %0.3f)"),
                          hsv.h * 360.0, hsv.s, hsv.v);
  str4 = g_strdup_printf (_("Luminance: %0.3f    Opacity: %0.3f"),
                          GIMP_RGB_LUMINANCE (rgb.r, rgb.g, rgb.b), rgb.a);


  gradient_editor_set_hint (editor, str1, str2, str3, str4);

  g_free (str1);
  g_free (str2);
  g_free (str3);
  g_free (str4);
}

static void
view_set_foreground (GimpGradientEditor *editor,
                     gint                x)
{
  GimpGradient *gradient;
  GimpContext  *user_context;
  GimpRGB       color;
  gdouble       xpos;
  gchar        *str2;
  gchar        *str3;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  user_context = gimp_get_user_context (GIMP_DATA_EDITOR (editor)->data_factory->gimp);

  xpos = control_calc_g_pos (editor, x);

  gimp_gradient_get_color_at (gradient, editor->context,
                              NULL, xpos, FALSE, &color);

  gimp_context_set_foreground (user_context, &color);

  str2 = g_strdup_printf (_("RGB (%d, %d, %d)"),
                          (gint) (color.r * 255.0),
                          (gint) (color.g * 255.0),
                          (gint) (color.b * 255.0));

  str3 = g_strdup_printf ("(%0.3f, %0.3f, %0.3f)", color.r, color.g, color.b);

  gradient_editor_set_hint (editor,
                            _("Foreground color set to:"), str2, str3, NULL);

  g_free (str2);
  g_free (str3);
}

static void
view_set_background (GimpGradientEditor *editor,
                     gint                x)
{
  GimpGradient *gradient;
  GimpContext  *user_context;
  GimpRGB       color;
  gdouble       xpos;
  gchar        *str2;
  gchar        *str3;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  user_context = gimp_get_user_context (GIMP_DATA_EDITOR (editor)->data_factory->gimp);

  xpos = control_calc_g_pos (editor, x);

  gimp_gradient_get_color_at (gradient, editor->context,
                              NULL, xpos, FALSE, &color);

  gimp_context_set_background (user_context, &color);

  str2 = g_strdup_printf (_("RGB (%d, %d, %d)"),
                          (gint) (color.r * 255.0),
                          (gint) (color.g * 255.0),
                          (gint) (color.b * 255.0));

  str3 = g_strdup_printf (_("(%0.3f, %0.3f, %0.3f)"),
                          color.r, color.g, color.b);

  gradient_editor_set_hint (editor,
                            _("Background color set to:"), str2, str3, NULL);

  g_free (str2);
  g_free (str3);
}

/***** Gradient control functions *****/

/* *** WARNING *** WARNING *** WARNING ***
 *
 * All the event-handling code for the gradient control widget is
 * extremely hairy.  You are not expected to understand it.  If you
 * find bugs, mail me unless you are very brave and you want to fix
 * them yourself ;-)
 */

static gboolean
control_events (GtkWidget          *widget,
                GdkEvent           *event,
                GimpGradientEditor *editor)
{
  GimpGradient        *gradient;
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
      gradient_editor_set_hint (editor, NULL, NULL, NULL, NULL);
      break;

    case GDK_BUTTON_PRESS:
      if (editor->control_drag_mode == GRAD_DRAG_NONE)
        {
          GdkEventButton *bevent = (GdkEventButton *) event;

          gtk_widget_get_pointer (editor->control, &x, &y);

          editor->control_last_x     = x;
          editor->control_click_time = bevent->time;

          control_button_press (editor,
                                x, y, bevent->button, bevent->state);

          if (editor->control_drag_mode != GRAD_DRAG_NONE)
            {
              gtk_grab_add (widget);

              if (GIMP_DATA_EDITOR (editor)->data_editable)
                {
                  g_signal_handlers_block_by_func (gradient,
                                                   gimp_gradient_editor_gradient_dirty,
                                                   editor);

                  if (! editor->instant_update)
                    gimp_data_freeze (GIMP_DATA (gradient));
                }
            }
        }
      break;

    case GDK_SCROLL:
      {
        GdkEventScroll *sevent = (GdkEventScroll *) event;

        if (sevent->state & GDK_SHIFT_MASK)
          {
            if (sevent->direction == GDK_SCROLL_UP)
              gimp_gradient_editor_zoom (editor, GIMP_ZOOM_IN);
            else
              gimp_gradient_editor_zoom (editor, GIMP_ZOOM_OUT);
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
      }
      break;

    case GDK_BUTTON_RELEASE:
      gradient_editor_set_hint (editor, NULL, NULL, NULL, NULL);

      if (editor->control_drag_mode != GRAD_DRAG_NONE)
        {
          if (GIMP_DATA_EDITOR (editor)->data_editable)
            {
              if (! editor->instant_update)
                gimp_data_thaw (GIMP_DATA (gradient));

              g_signal_handlers_unblock_by_func (gradient,
                                                 gimp_gradient_editor_gradient_dirty,
                                                 editor);
            }

          gtk_grab_remove (widget);

          gtk_widget_get_pointer (editor->control, &x, &y);

          time = ((GdkEventButton *) event)->time;

          if ((time - editor->control_click_time) >= GRAD_MOVE_TIME)
            {
              /* stuff was done in motion */
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

              gimp_gradient_editor_update (editor);
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

          if (GIMP_DATA_EDITOR (editor)->data_editable &&
              editor->control_drag_mode != GRAD_DRAG_NONE)
            {
              time = ((GdkEventButton *) event)->time;

              if ((time - editor->control_click_time) >= GRAD_MOVE_TIME)
                control_motion (editor, gradient, x);
            }
          else
            {
              gimp_gradient_editor_update (editor);

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
                                         gimp_get_mod_string (GDK_SHIFT_MASK),
                                         gimp_get_mod_separator ());

                  gradient_editor_set_hint (editor,
                                            NULL,
                                            _("Drag: move"),
                                            str,
                                            NULL);
                  g_free (str);
                }
              else
                {
                  str = g_strdup_printf (_("%s%sClick: extend selection"),
                                         gimp_get_mod_string (GDK_SHIFT_MASK),
                                         gimp_get_mod_separator ());

                  gradient_editor_set_hint (editor,
                                            NULL,
                                            _("Click: select"),
                                            str,
                                            NULL);
                  g_free (str);
                }
            }
          else
            {
              str = g_strdup_printf (_("%s%sClick: extend selection"),
                                     gimp_get_mod_string (GDK_SHIFT_MASK),
                                     gimp_get_mod_separator ());

              gradient_editor_set_hint (editor,
                                        NULL,
                                        _("Click: select"),
                                        str,
                                        NULL);
              g_free (str);
            }
          break;

        case GRAD_DRAG_MIDDLE:
          str = g_strdup_printf (_("%s%sClick: extend selection"),
                                 gimp_get_mod_string (GDK_SHIFT_MASK),
                                 gimp_get_mod_separator ());

          gradient_editor_set_hint (editor,
                                    NULL,
                                    _("Click: select    Drag: move"),
                                    str,
                                    NULL);
          g_free (str);
          break;

        default:
          g_warning ("%s: in_handle is true, but received handle type %d.",
                     G_STRFUNC, in_handle);
          break;
        }
    }
  else
    {
      gchar *str2;

      str  = g_strdup_printf (_("%s%sClick: extend selection"),
                              gimp_get_mod_string (GDK_SHIFT_MASK),
                              gimp_get_mod_separator ());
      str2 = g_strdup_printf (_("%s%sDrag: move & compress"),
                              gimp_get_mod_string (GDK_SHIFT_MASK),
                              gimp_get_mod_separator ());

      gradient_editor_set_hint (editor,
                                _("Click: select    Drag: move"),
                                str,
                                str2,
                                NULL);
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
  gdouble                 xpos;
  gboolean                in_handle;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  if (button == 3)
    {
      gimp_editor_popup_menu (GIMP_EDITOR (editor), NULL, NULL);
      return;
    }

  /* Find the closest handle */

  xpos = control_calc_g_pos (editor, x);

  seg_get_closest_handle (gradient, xpos, &seg, &handle);

  in_handle = control_point_in_handle (editor, gradient, x, y, seg, handle);

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
                      gimp_gradient_editor_update (editor);
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
                  gimp_gradient_editor_update (editor);
                }
            }
          else  /* seg == NULL */
            {
              /* Right handle of last segment */
              seg = gimp_gradient_segment_get_last (gradient->segments);

              if (state & GDK_SHIFT_MASK)
                {
                  control_extend_selection (editor, seg, xpos);
                  gimp_gradient_editor_update (editor);
                }
              else
                {
                  control_select_single_segment (editor, seg);
                  gimp_gradient_editor_update (editor);
                }
            }

          break;

        case GRAD_DRAG_MIDDLE:
          if (state & GDK_SHIFT_MASK)
            {
              control_extend_selection (editor, seg, xpos);
              gimp_gradient_editor_update (editor);
            }
          else
            {
              editor->control_drag_mode    = GRAD_DRAG_MIDDLE;
              editor->control_drag_segment = seg;
            }

          break;

        default:
          g_warning ("%s: in_handle is true, but received handle type %d.",
                     G_STRFUNC, in_handle);
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
      g_warning ("%s: Cannot handle drag mode %d.", G_STRFUNC, handle);
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
  GimpGradientSegment *seg = editor->control_drag_segment;
  gdouble              pos;
  gdouble              delta;
  gchar               *str = NULL;

  switch (editor->control_drag_mode)
    {
    case GRAD_DRAG_LEFT:
      pos = control_calc_g_pos (editor, x);

      if (! editor->control_compress)
        gimp_gradient_segment_set_left_pos (gradient, seg, pos);
      else
        control_compress_left (gradient,
                               editor->control_sel_l,
                               editor->control_sel_r,
                               seg, pos);

      str = g_strdup_printf (_("Handle position: %0.6f"), seg->left);
      break;

    case GRAD_DRAG_MIDDLE:
      pos = control_calc_g_pos (editor, x);

      gimp_gradient_segment_set_middle_pos (gradient, seg, pos);

      str = g_strdup_printf (_("Handle position: %0.6f"), seg->middle);
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
      break;

    default:
      g_warning ("%s: Attempting to move bogus handle %d.",
                 G_STRFUNC, editor->control_drag_mode);
      break;
    }

  gradient_editor_set_hint (editor, str, NULL, NULL, NULL);
  g_free (str);

  gimp_gradient_editor_update (editor);
}

static void
control_compress_left (GimpGradient        *gradient,
                       GimpGradientSegment *range_l,
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
    gimp_gradient_segment_range_compress (gradient,
                                          range_l->prev, range_l->prev,
                                          range_l->prev->left, pos);
  else
    gimp_gradient_segment_range_compress (gradient,
                                          range_l, drag_seg->prev,
                                          range_l->left, pos);

  /* Compress segments to the right of the handle */

  if (drag_seg != range_r->next)
    gimp_gradient_segment_range_compress (gradient,
                                          drag_seg, range_r,
                                          pos, range_r->right);
  else
    gimp_gradient_segment_range_compress (gradient,
                                          drag_seg, drag_seg,
                                          pos, drag_seg->right);
}

/*****/

static gdouble
control_move (GimpGradientEditor  *editor,
              GimpGradientSegment *range_l,
              GimpGradientSegment *range_r,
              gdouble              delta)
{
  GimpGradient *gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);
  gdouble       ret;

  ret = gimp_gradient_segment_range_move (gradient,
                                          range_l,
                                          range_r,
                                          delta,
                                          editor->control_compress);

  return ret;
}

/*****/

static void
control_update (GimpGradientEditor *editor,
                GimpGradient       *gradient,
                gboolean            reset_selection)
{
  GtkAdjustment *adjustment;
  gint           cwidth, cheight;
  gint           pwidth  = 0;
  gint           pheight = 0;

  if (! editor->control_sel_l || ! editor->control_sel_r)
    reset_selection = TRUE;

  if (reset_selection)
    {
      if (gradient)
        control_select_single_segment (editor, gradient->segments);
      else
        control_select_single_segment (editor, NULL);
    }

  if (! GTK_WIDGET_DRAWABLE (editor->control))
    return;

  /*  See whether we have to re-create the control pixmap
   *  depending on the view's width
   */
  cwidth  = editor->view->allocation.width;
  cheight = GRAD_CONTROL_HEIGHT;

  if (editor->control_pixmap)
    gdk_drawable_get_size (editor->control_pixmap, &pwidth, &pheight);

  if (! editor->control_pixmap || (cwidth != pwidth) || (cheight != pheight))
    {
      if (editor->control_pixmap)
        g_object_unref (editor->control_pixmap);

      editor->control_pixmap =
        gdk_pixmap_new (editor->control->window, cwidth, cheight, -1);
    }

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
  gint                    sel_l, sel_r;
  gdouble                 g_pos;
  GimpGradientSegment    *seg;
  GradientEditorDragMode  handle;
  gboolean                selected;

  /* Clear the pixmap */

  gdk_draw_rectangle (pixmap, editor->control->style->base_gc[GTK_STATE_NORMAL],
                      TRUE, 0, 0, width, height);

  if (! gradient)
    return;

  /* Draw selection */

  sel_l = control_calc_p_pos (editor, editor->control_sel_l->left);
  sel_r = control_calc_p_pos (editor, editor->control_sel_r->right);

  gdk_draw_rectangle (pixmap,
                      editor->control->style->base_gc[GTK_STATE_SELECTED],
                      TRUE, sel_l, 0, sel_r - sel_l + 1, height);

  /* Draw handles */

  selected = FALSE;

  for (seg = gradient->segments; seg; seg = seg->next)
    {
      if (seg == editor->control_sel_l)
        selected = TRUE;

      control_draw_normal_handle (editor, pixmap, seg->left, height,
                                  selected);
      control_draw_middle_handle (editor, pixmap, seg->middle, height,
                                  selected);

      /* Draw right handle only if this is the last segment */
      if (seg->next == NULL)
        control_draw_normal_handle (editor, pixmap, seg->right, height,
                                    selected);

      if (seg == editor->control_sel_r)
        selected = FALSE;
    }

  /* Draw the handle which is closest to the mouse position */

  g_pos = control_calc_g_pos (editor, editor->control_last_x);

  seg_get_closest_handle (gradient, CLAMP (g_pos, 0.0, 1.0), &seg, &handle);

  selected = (seg &&
              seg_in_selection (gradient, seg,
                                editor->control_sel_l, editor->control_sel_r));

  switch (handle)
    {
    case GRAD_DRAG_LEFT:
      if (seg)
        {
          control_draw_normal_handle (editor, pixmap, seg->left, height,
                                      selected);
        }
      else
        {
          seg = gimp_gradient_segment_get_last (gradient->segments);

          selected = (seg == editor->control_sel_r);

          control_draw_normal_handle (editor, pixmap, seg->right, height,
                                      selected);
        }

      break;

    case GRAD_DRAG_MIDDLE:
      control_draw_middle_handle (editor, pixmap, seg->middle, height,
                                  selected);
      break;

    default:
      break;
    }
}

static void
control_draw_normal_handle (GimpGradientEditor *editor,
                            GdkPixmap          *pixmap,
                            gdouble             pos,
                            gint                height,
                            gboolean            selected)
{
  GtkStateType state = selected ? GTK_STATE_SELECTED : GTK_STATE_NORMAL;

  control_draw_handle (pixmap,
                       editor->control->style->text_aa_gc[state],
                       editor->control->style->black_gc,
                       control_calc_p_pos (editor, pos), height);
}

static void
control_draw_middle_handle (GimpGradientEditor *editor,
                            GdkPixmap          *pixmap,
                            gdouble             pos,
                            gint                height,
                            gboolean            selected)
{
  GtkStateType state = selected ? GTK_STATE_SELECTED : GTK_STATE_NORMAL;

  control_draw_handle (pixmap,
                       editor->control->style->text_aa_gc[state],
                       editor->control->style->white_gc,
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
   * minimize mismatches between the rendered gradient view
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

static gboolean
seg_in_selection (GimpGradient         *grad,
                  GimpGradientSegment  *seg,
                  GimpGradientSegment  *left,
                  GimpGradientSegment  *right)
{
  GimpGradientSegment *s;

  for (s = left; s; s = s->next)
    {
      if (s == seg)
        return TRUE;

      if (s == right)
        break;
    }

  return FALSE;
}

static GtkWidget *
gradient_hint_label_add (GtkBox *box)
{
  GtkWidget *label = g_object_new (GTK_TYPE_LABEL,
                                   "xalign",           0.0,
                                   "yalign",           0.5,
                                   "single-line-mode", TRUE,
                                   NULL);
  gtk_box_pack_start (box, label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  return label;
}
