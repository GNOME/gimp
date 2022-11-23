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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURIGHTE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
 * Everyone on #ligma - many suggestions
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"
#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmadatafactory.h"
#include "core/ligmagradient.h"

#include "ligmacolordialog.h"
#include "ligmadialogfactory.h"
#include "ligmadnd.h"
#include "ligmadocked.h"
#include "ligmagradienteditor.h"
#include "ligmahelp-ids.h"
#include "ligmauimanager.h"
#include "ligmaview.h"
#include "ligmaviewrenderergradient.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


#define EPSILON 1e-10

#define GRAD_SCROLLBAR_STEP_SIZE 0.05
#define GRAD_SCROLLBAR_PAGE_SIZE 0.5

#define GRAD_VIEW_SIZE            96
#define GRAD_CONTROL_HEIGHT       14
#define GRAD_CURRENT_COLOR_WIDTH  16

#define GRAD_MOVE_TIME 150 /* ms between mouse click and detection of movement in gradient control */


#define GRAD_VIEW_EVENT_MASK (GDK_EXPOSURE_MASK            | \
                              GDK_LEAVE_NOTIFY_MASK        | \
                              GDK_POINTER_MOTION_MASK      | \
                              GDK_POINTER_MOTION_HINT_MASK | \
                              GDK_BUTTON_PRESS_MASK        | \
                              GDK_BUTTON_RELEASE_MASK      | \
                              GDK_SCROLL_MASK              | \
                              GDK_SMOOTH_SCROLL_MASK       | \
                              GDK_TOUCHPAD_GESTURE_MASK)

#define GRAD_CONTROL_EVENT_MASK (GDK_EXPOSURE_MASK            | \
                                 GDK_LEAVE_NOTIFY_MASK        | \
                                 GDK_POINTER_MOTION_MASK      | \
                                 GDK_POINTER_MOTION_HINT_MASK | \
                                 GDK_BUTTON_PRESS_MASK        | \
                                 GDK_BUTTON_RELEASE_MASK      | \
                                 GDK_SCROLL_MASK              | \
                                 GDK_SMOOTH_SCROLL_MASK       | \
                                 GDK_BUTTON1_MOTION_MASK)


/*  local function prototypes  */

static void  ligma_gradient_editor_docked_iface_init (LigmaDockedInterface *face);

static void   ligma_gradient_editor_constructed      (GObject            *object);
static void   ligma_gradient_editor_dispose          (GObject            *object);

static void   ligma_gradient_editor_unmap            (GtkWidget          *widget);
static void   ligma_gradient_editor_set_data         (LigmaDataEditor     *editor,
                                                     LigmaData           *data);

static void   ligma_gradient_editor_set_context      (LigmaDocked         *docked,
                                                     LigmaContext        *context);

static void   ligma_gradient_editor_update          (LigmaGradientEditor  *editor);
static void   ligma_gradient_editor_gradient_dirty   (LigmaGradientEditor *editor,
                                                     LigmaGradient       *gradient);
static void   gradient_editor_drop_gradient         (GtkWidget          *widget,
                                                     gint                x,
                                                     gint                y,
                                                     LigmaViewable       *viewable,
                                                     gpointer            data);
static void   gradient_editor_drop_color            (GtkWidget          *widget,
                                                     gint                x,
                                                     gint                y,
                                                     const LigmaRGB      *color,
                                                     gpointer            data);
static void   gradient_editor_control_drop_color    (GtkWidget          *widget,
                                                     gint                x,
                                                     gint                y,
                                                     const LigmaRGB      *color,
                                                     gpointer            data);
static void   gradient_editor_scrollbar_update      (GtkAdjustment      *adj,
                                                     LigmaGradientEditor *editor);

static void   gradient_editor_set_hint              (LigmaGradientEditor *editor,
                                                     const gchar        *str1,
                                                     const gchar        *str2,
                                                     const gchar        *str3,
                                                     const gchar        *str4);

static LigmaGradientSegment *
              gradient_editor_save_selection        (LigmaGradientEditor *editor);
static void   gradient_editor_replace_selection     (LigmaGradientEditor *editor,
                                                     LigmaGradientSegment *replace_seg);

static void   gradient_editor_left_color_update     (LigmaColorDialog    *dialog,
                                                     const LigmaRGB      *color,
                                                     LigmaColorDialogState state,
                                                     LigmaGradientEditor *editor);
static void   gradient_editor_right_color_update    (LigmaColorDialog    *dialog,
                                                     const LigmaRGB      *color,
                                                     LigmaColorDialogState state,
                                                     LigmaGradientEditor *editor);


/* Gradient view functions */

static gboolean  view_events                      (GtkWidget          *widget,
                                                   GdkEvent           *event,
                                                   LigmaGradientEditor *editor);
static void      view_set_hint                    (LigmaGradientEditor *editor,
                                                   gint                x);

static void      view_pick_color                  (LigmaGradientEditor *editor,
                                                   LigmaColorPickTarget pick_target,
                                                   LigmaColorPickState  pick_state,
                                                   gint                x);

static void      view_zoom_gesture_begin          (GtkGestureZoom     *gesture,
                                                   GdkEventSequence   *sequence,
                                                   LigmaGradientEditor *editor);

static void      view_zoom_gesture_update         (GtkGestureZoom     *gesture,
                                                   GdkEventSequence   *sequence,
                                                   LigmaGradientEditor *editor);

static gdouble   view_get_normalized_last_x_pos   (LigmaGradientEditor *editor);

/* Gradient control functions */

static gboolean  control_events                   (GtkWidget          *widget,
                                                   GdkEvent           *event,
                                                   LigmaGradientEditor *editor);
static gboolean  control_draw                     (GtkWidget          *widget,
                                                   cairo_t            *cr,
                                                   LigmaGradientEditor *editor);
static void      control_do_hint                  (LigmaGradientEditor *editor,
                                                   gint                x,
                                                   gint                y);
static void      control_button_press             (LigmaGradientEditor *editor,
                                                   GdkEventButton     *bevent);
static gboolean  control_point_in_handle          (LigmaGradientEditor *editor,
                                                   LigmaGradient       *gradient,
                                                   gint                x,
                                                   gint                y,
                                                   LigmaGradientSegment *seg,
                                                   GradientEditorDragMode handle);
static void      control_select_single_segment    (LigmaGradientEditor  *editor,
                                                   LigmaGradientSegment *seg);
static void      control_extend_selection         (LigmaGradientEditor  *editor,
                                                   LigmaGradientSegment *seg,
                                                   gdouble              pos);
static void      control_motion                   (LigmaGradientEditor  *editor,
                                                   LigmaGradient        *gradient,
                                                   gint                 x);

static void      control_compress_left            (LigmaGradient        *gradient,
                                                   LigmaGradientSegment *range_l,
                                                   LigmaGradientSegment *range_r,
                                                   LigmaGradientSegment *drag_seg,
                                                   gdouble              pos);

static double    control_move                     (LigmaGradientEditor  *editor,
                                                   LigmaGradientSegment *range_l,
                                                   LigmaGradientSegment *range_r,
                                                   gdouble              delta);

static gdouble   control_get_normalized_last_x_pos (LigmaGradientEditor *editor);

/* Control update/redraw functions */

static void      control_update                   (LigmaGradientEditor *editor,
                                                   LigmaGradient       *gradient,
                                                   gboolean            recalculate);
static void      control_draw_all                 (LigmaGradientEditor *editor,
                                                   LigmaGradient       *gradient,
                                                   cairo_t            *cr,
                                                   gint                width,
                                                   gint                height,
                                                   gdouble             left,
                                                   gdouble             right);
static void      control_draw_handle              (LigmaGradientEditor *editor,
                                                   GtkStyleContext    *style,
                                                   cairo_t            *cr,
                                                   gdouble             pos,
                                                   gint                height,
                                                   gboolean            middle,
                                                   GtkStateFlags       flags);

static gint      control_calc_p_pos               (LigmaGradientEditor *editor,
                                                   gdouble             pos);
static gdouble   control_calc_g_pos               (LigmaGradientEditor *editor,
                                                   gint                pos);

/* Segment functions */

static void      seg_get_closest_handle           (LigmaGradient         *grad,
                                                   gdouble               pos,
                                                   LigmaGradientSegment **seg,
                                                   GradientEditorDragMode *handle);
static gboolean  seg_in_selection                 (LigmaGradient         *grad,
                                                   LigmaGradientSegment  *seg,
                                                   LigmaGradientSegment  *left,
                                                   LigmaGradientSegment  *right);

static GtkWidget * gradient_hint_label_add        (GtkBox *box);


G_DEFINE_TYPE_WITH_CODE (LigmaGradientEditor, ligma_gradient_editor,
                         LIGMA_TYPE_DATA_EDITOR,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED,
                                                ligma_gradient_editor_docked_iface_init))

#define parent_class ligma_gradient_editor_parent_class

static LigmaDockedInterface *parent_docked_iface = NULL;


static void
ligma_gradient_editor_class_init (LigmaGradientEditorClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass      *widget_class = GTK_WIDGET_CLASS (klass);
  LigmaDataEditorClass *editor_class = LIGMA_DATA_EDITOR_CLASS (klass);

  object_class->constructed = ligma_gradient_editor_constructed;
  object_class->dispose     = ligma_gradient_editor_dispose;

  widget_class->unmap       = ligma_gradient_editor_unmap;

  editor_class->set_data    = ligma_gradient_editor_set_data;
  editor_class->title       = _("Gradient Editor");
}

static void
ligma_gradient_editor_docked_iface_init (LigmaDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (LIGMA_TYPE_DOCKED);

  iface->set_context = ligma_gradient_editor_set_context;
}

static void
ligma_gradient_editor_init (LigmaGradientEditor *editor)
{
  LigmaDataEditor *data_editor = LIGMA_DATA_EDITOR (editor);
  GtkWidget      *frame;
  GtkWidget      *vbox;
  GtkWidget      *hbox;
  GtkWidget      *hint_vbox;
  LigmaRGB         transp;

  ligma_rgba_set (&transp, 0.0, 0.0, 0.0, 0.0);

  /* Frame for gradient view and gradient control */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  data_editor->view = ligma_view_new_full_by_types (NULL,
                                                   LIGMA_TYPE_VIEW,
                                                   LIGMA_TYPE_GRADIENT,
                                                   GRAD_VIEW_SIZE,
                                                   GRAD_VIEW_SIZE, 0,
                                                   FALSE, FALSE, FALSE);
  gtk_widget_set_size_request (data_editor->view, -1, GRAD_VIEW_SIZE);
  gtk_widget_set_events (data_editor->view, GRAD_VIEW_EVENT_MASK);
  ligma_view_set_expand (LIGMA_VIEW (data_editor->view), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), data_editor->view, TRUE, TRUE, 0);
  gtk_widget_show (data_editor->view);

  g_signal_connect (data_editor->view, "event",
                    G_CALLBACK (view_events),
                    editor);

  ligma_dnd_viewable_dest_add (GTK_WIDGET (data_editor->view),
                              LIGMA_TYPE_GRADIENT,
                              gradient_editor_drop_gradient,
                              editor);

  ligma_dnd_color_dest_add (GTK_WIDGET (data_editor->view),
                              gradient_editor_drop_color,
                              editor);

  editor->zoom_gesture = gtk_gesture_zoom_new (GTK_WIDGET (data_editor->view));
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (editor->zoom_gesture),
                                              GTK_PHASE_CAPTURE);


  g_signal_connect (editor->zoom_gesture, "begin",
                    G_CALLBACK (view_zoom_gesture_begin),
                    editor);
  g_signal_connect (editor->zoom_gesture, "update",
                    G_CALLBACK (view_zoom_gesture_update),
                    editor);

  /* Gradient control */
  editor->control = gtk_drawing_area_new ();
  gtk_widget_set_size_request (editor->control, -1, GRAD_CONTROL_HEIGHT);
  gtk_widget_set_events (editor->control, GRAD_CONTROL_EVENT_MASK);
  gtk_box_pack_start (GTK_BOX (vbox), editor->control, FALSE, FALSE, 0);
  gtk_widget_show (editor->control);

  g_signal_connect (editor->control, "event",
                    G_CALLBACK (control_events),
                    editor);

  g_signal_connect (editor->control, "draw",
                    G_CALLBACK (control_draw),
                    editor);

  ligma_dnd_color_dest_add (GTK_WIDGET (editor->control),
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

  editor->scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL,
                                         editor->scroll_data);
  gtk_box_pack_start (GTK_BOX (editor), editor->scrollbar, FALSE, FALSE, 0);
  gtk_widget_show (editor->scrollbar);

  /* Box for current color and the hint labels */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* Frame showing current active color */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  editor->current_color = ligma_color_area_new (&transp,
                                               LIGMA_COLOR_AREA_SMALL_CHECKS,
                                               GDK_BUTTON1_MASK |
                                               GDK_BUTTON2_MASK);
  gtk_container_add (GTK_CONTAINER (frame), editor->current_color);
  gtk_widget_set_size_request (editor->current_color,
                               GRAD_CURRENT_COLOR_WIDTH, -1);
  gtk_widget_show (editor->current_color);

  /* Hint box */
  hint_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), hint_vbox, TRUE, TRUE, 0);
  gtk_widget_show (hint_vbox);

  editor->hint_label1 = gradient_hint_label_add (GTK_BOX (hint_vbox));
  editor->hint_label2 = gradient_hint_label_add (GTK_BOX (hint_vbox));
  editor->hint_label3 = gradient_hint_label_add (GTK_BOX (hint_vbox));
  editor->hint_label4 = gradient_hint_label_add (GTK_BOX (hint_vbox));

  /* Black, 50% Gray, White, Clear */
  ligma_rgba_set (&editor->saved_colors[0], 0.0, 0.0, 0.0, LIGMA_OPACITY_OPAQUE);
  ligma_rgba_set (&editor->saved_colors[1], 0.5, 0.5, 0.5, LIGMA_OPACITY_OPAQUE);
  ligma_rgba_set (&editor->saved_colors[2], 1.0, 1.0, 1.0, LIGMA_OPACITY_OPAQUE);
  ligma_rgba_set (&editor->saved_colors[3], 0.0, 0.0, 0.0, LIGMA_OPACITY_TRANSPARENT);

  /* Red, Yellow, Green, Cyan, Blue, Magenta */
  ligma_rgba_set (&editor->saved_colors[4], 1.0, 0.0, 0.0, LIGMA_OPACITY_OPAQUE);
  ligma_rgba_set (&editor->saved_colors[5], 1.0, 1.0, 0.0, LIGMA_OPACITY_OPAQUE);
  ligma_rgba_set (&editor->saved_colors[6], 0.0, 1.0, 0.0, LIGMA_OPACITY_OPAQUE);
  ligma_rgba_set (&editor->saved_colors[7], 0.0, 1.0, 1.0, LIGMA_OPACITY_OPAQUE);
  ligma_rgba_set (&editor->saved_colors[8], 0.0, 0.0, 1.0, LIGMA_OPACITY_OPAQUE);
  ligma_rgba_set (&editor->saved_colors[9], 1.0, 0.0, 1.0, LIGMA_OPACITY_OPAQUE);
}

static void
ligma_gradient_editor_constructed (GObject *object)
{
  LigmaGradientEditor *editor = LIGMA_GRADIENT_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_editor_add_action_button (LIGMA_EDITOR (editor), "gradient-editor",
                                 "gradient-editor-zoom-out", NULL);

  ligma_editor_add_action_button (LIGMA_EDITOR (editor), "gradient-editor",
                                 "gradient-editor-zoom-in", NULL);

  ligma_editor_add_action_button (LIGMA_EDITOR (editor), "gradient-editor",
                                 "gradient-editor-zoom-all", NULL);
}

static void
ligma_gradient_editor_dispose (GObject *object)
{
  LigmaGradientEditor *editor = LIGMA_GRADIENT_EDITOR (object);

  if (editor->color_dialog)
    gtk_dialog_response (GTK_DIALOG (editor->color_dialog),
                         GTK_RESPONSE_CANCEL);

  g_clear_object (&editor->zoom_gesture);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_gradient_editor_unmap (GtkWidget *widget)
{
  LigmaGradientEditor *editor = LIGMA_GRADIENT_EDITOR (widget);

  if (editor->color_dialog)
    gtk_dialog_response (GTK_DIALOG (editor->color_dialog),
                         GTK_RESPONSE_CANCEL);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
ligma_gradient_editor_set_data (LigmaDataEditor *editor,
                               LigmaData       *data)
{
  LigmaGradientEditor *gradient_editor = LIGMA_GRADIENT_EDITOR (editor);
  LigmaData           *old_data;

  if (gradient_editor->color_dialog)
    gtk_dialog_response (GTK_DIALOG (gradient_editor->color_dialog),
                         GTK_RESPONSE_CANCEL);

  old_data = ligma_data_editor_get_data (editor);

  if (old_data)
    g_signal_handlers_disconnect_by_func (old_data,
                                          ligma_gradient_editor_gradient_dirty,
                                          gradient_editor);

  LIGMA_DATA_EDITOR_CLASS (parent_class)->set_data (editor, data);

  if (data)
    g_signal_connect_swapped (data, "dirty",
                              G_CALLBACK (ligma_gradient_editor_gradient_dirty),
                              gradient_editor);

  ligma_view_set_viewable (LIGMA_VIEW (editor->view),
                          LIGMA_VIEWABLE (data));

  control_update (gradient_editor, LIGMA_GRADIENT (data), TRUE);
}

static void
ligma_gradient_editor_set_context (LigmaDocked  *docked,
                                  LigmaContext *context)
{
  LigmaDataEditor *data_editor = LIGMA_DATA_EDITOR (docked);

  parent_docked_iface->set_context (docked, context);

  ligma_view_renderer_set_context (LIGMA_VIEW (data_editor->view)->renderer,
                                  context);
}


/*  public functions  */

GtkWidget *
ligma_gradient_editor_new (LigmaContext     *context,
                          LigmaMenuFactory *menu_factory)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return g_object_new (LIGMA_TYPE_GRADIENT_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<GradientEditor>",
                       "ui-path",         "/gradient-editor-popup",
                       "data-factory",    context->ligma->gradient_factory,
                       "context",         context,
                       "data",            ligma_context_get_gradient (context),
                       NULL);
}

void
ligma_gradient_editor_get_selection (LigmaGradientEditor   *editor,
                                    LigmaGradient        **gradient,
                                    LigmaGradientSegment **left,
                                    LigmaGradientSegment **right)
{
  g_return_if_fail (LIGMA_IS_GRADIENT_EDITOR (editor));

  if (gradient)
    *gradient = LIGMA_GRADIENT (LIGMA_DATA_EDITOR (editor)->data);

  if (left)
    *left = editor->control_sel_l;

  if (right)
    *right = editor->control_sel_r;
}

void
ligma_gradient_editor_set_selection (LigmaGradientEditor  *editor,
                                    LigmaGradientSegment *left,
                                    LigmaGradientSegment *right)
{
  g_return_if_fail (LIGMA_IS_GRADIENT_EDITOR (editor));
  g_return_if_fail (left != NULL);
  g_return_if_fail (right != NULL);

  editor->control_sel_l = left;
  editor->control_sel_r = right;
}

void
ligma_gradient_editor_edit_left_color (LigmaGradientEditor *editor)
{
  LigmaGradient *gradient;

  g_return_if_fail (LIGMA_IS_GRADIENT_EDITOR (editor));

  gradient = LIGMA_GRADIENT (LIGMA_DATA_EDITOR (editor)->data);

  if (! gradient              ||
      ! editor->control_sel_l ||
      editor->control_sel_l->left_color_type != LIGMA_GRADIENT_COLOR_FIXED)
    return;

  editor->saved_dirty    = ligma_data_is_dirty (LIGMA_DATA (gradient));
  editor->saved_segments = gradient_editor_save_selection (editor);

  editor->color_dialog =
    ligma_color_dialog_new (LIGMA_VIEWABLE (gradient),
                           LIGMA_DATA_EDITOR (editor)->context,
                           TRUE,
                           _("Left Endpoint Color"),
                           LIGMA_ICON_GRADIENT,
                           _("Gradient Segment's Left Endpoint Color"),
                           GTK_WIDGET (editor),
                           ligma_dialog_factory_get_singleton (),
                           "ligma-gradient-editor-color-dialog",
                           &editor->control_sel_l->left_color,
                           TRUE, TRUE);

  g_signal_connect (editor->color_dialog, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &editor->color_dialog);

  g_signal_connect (editor->color_dialog, "update",
                    G_CALLBACK (gradient_editor_left_color_update),
                    editor);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
  ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)),
                          ligma_editor_get_popup_data (LIGMA_EDITOR (editor)));

  gtk_window_present (GTK_WINDOW (editor->color_dialog));
}

void
ligma_gradient_editor_edit_right_color (LigmaGradientEditor *editor)
{
  LigmaGradient *gradient;

  g_return_if_fail (LIGMA_IS_GRADIENT_EDITOR (editor));

  gradient = LIGMA_GRADIENT (LIGMA_DATA_EDITOR (editor)->data);

  if (! gradient              ||
      ! editor->control_sel_r ||
      editor->control_sel_r->right_color_type != LIGMA_GRADIENT_COLOR_FIXED)
    return;

  editor->saved_dirty    = ligma_data_is_dirty (LIGMA_DATA (gradient));
  editor->saved_segments = gradient_editor_save_selection (editor);

  editor->color_dialog =
    ligma_color_dialog_new (LIGMA_VIEWABLE (gradient),
                           LIGMA_DATA_EDITOR (editor)->context,
                           TRUE,
                           _("Right Endpoint Color"),
                           LIGMA_ICON_GRADIENT,
                           _("Gradient Segment's Right Endpoint Color"),
                           GTK_WIDGET (editor),
                           ligma_dialog_factory_get_singleton (),
                           "ligma-gradient-editor-color-dialog",
                           &editor->control_sel_l->right_color,
                           TRUE, TRUE);

  g_signal_connect (editor->color_dialog, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &editor->color_dialog);

  g_signal_connect (editor->color_dialog, "update",
                    G_CALLBACK (gradient_editor_right_color_update),
                    editor);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
  ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)),
                          ligma_editor_get_popup_data (LIGMA_EDITOR (editor)));

  gtk_window_present (GTK_WINDOW (editor->color_dialog));
}

void
ligma_gradient_editor_zoom (LigmaGradientEditor *editor,
                           LigmaZoomType        zoom_type,
                           gdouble             delta,
                           gdouble             zoom_focus_x)
{
  GtkAdjustment *adjustment;
  gdouble        old_value;
  gdouble        old_page_size;
  gdouble        value     = 0.0;
  gdouble        page_size = 1.0;

  g_return_if_fail (LIGMA_IS_GRADIENT_EDITOR (editor));

  if (zoom_type == LIGMA_ZOOM_SMOOTH)
    {
      if (delta < 0)
        zoom_type = LIGMA_ZOOM_IN;
      else if (delta > 0)
        zoom_type = LIGMA_ZOOM_OUT;
      else
        return;

      delta = ABS (delta);
    }
  else if (zoom_type != LIGMA_ZOOM_PINCH)
    {
      delta = 1.0;
    }

  adjustment = editor->scroll_data;

  old_value     = gtk_adjustment_get_value (adjustment);
  old_page_size = gtk_adjustment_get_page_size (adjustment);

  switch (zoom_type)
    {
    case LIGMA_ZOOM_IN_MAX:
    case LIGMA_ZOOM_IN_MORE:
    case LIGMA_ZOOM_IN:
      editor->zoom_factor += delta;

      page_size = 1.0 / editor->zoom_factor;
      value     = old_value + (old_page_size - page_size) * zoom_focus_x;
      break;

    case LIGMA_ZOOM_OUT_MORE:
    case LIGMA_ZOOM_OUT:
      editor->zoom_factor -= delta;

      if (editor->zoom_factor < 1)
        editor->zoom_factor = 1;

      page_size = 1.0 / editor->zoom_factor;
      value     = old_value - (page_size - old_page_size) * zoom_focus_x;

      if (value < 0.0)
        value = 0.0;
      else if ((value + page_size) > 1.0)
        value = 1.0 - page_size;
      break;

    case LIGMA_ZOOM_OUT_MAX:
    case LIGMA_ZOOM_TO: /* abused as ZOOM_ALL */
      editor->zoom_factor = 1;

      value     = 0.0;
      page_size = 1.0;
      break;

    case LIGMA_ZOOM_PINCH:
      if (delta > 0.0)
        editor->zoom_factor = editor->zoom_factor * (1.0 + delta);
      else if (delta < 0.0)
        editor->zoom_factor = editor->zoom_factor / (1.0 + -delta);
      else
        return;

      if (editor->zoom_factor < 1)
        editor->zoom_factor = 1;

      page_size = 1.0 / editor->zoom_factor;
      value     = old_value + (old_page_size - page_size) * zoom_focus_x;

      if (value < 0.0)
        value = 0.0;
      else if ((value + page_size) > 1.0)
        value = 1.0 - page_size;
      break;

    case LIGMA_ZOOM_SMOOTH: /* can't happen, see above switch() */
      break;
    }

  gtk_adjustment_configure (adjustment,
                            value,
                            gtk_adjustment_get_lower (adjustment),
                            gtk_adjustment_get_upper (adjustment),
                            page_size * GRAD_SCROLLBAR_STEP_SIZE,
                            page_size * GRAD_SCROLLBAR_PAGE_SIZE,
                            page_size);
}


/*  private functions  */

static void
ligma_gradient_editor_update (LigmaGradientEditor *editor)
{
  LigmaGradient *gradient = LIGMA_GRADIENT (LIGMA_DATA_EDITOR (editor)->data);

  control_update (editor, gradient, FALSE);
}

static void
ligma_gradient_editor_gradient_dirty (LigmaGradientEditor *editor,
                                     LigmaGradient       *gradient)
{
  LigmaGradientSegment *segment;
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
          LigmaGradientSegment *tmp;

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
                               LigmaViewable *viewable,
                               gpointer      data)
{
  ligma_data_editor_set_data (LIGMA_DATA_EDITOR (data), LIGMA_DATA (viewable));
}

static void
gradient_editor_drop_color (GtkWidget     *widget,
                            gint           x,
                            gint           y,
                            const LigmaRGB *color,
                            gpointer       data)
{
  LigmaGradientEditor  *editor = LIGMA_GRADIENT_EDITOR (data);
  LigmaGradient        *gradient;
  gdouble              xpos;
  LigmaGradientSegment *seg, *lseg, *rseg;

  gradient = LIGMA_GRADIENT (LIGMA_DATA_EDITOR (editor)->data);

  xpos = control_calc_g_pos (editor, x);
  seg = ligma_gradient_get_segment_at (gradient, xpos);

  ligma_data_freeze (LIGMA_DATA (gradient));

  ligma_gradient_segment_split_midpoint (gradient,
                                        LIGMA_DATA_EDITOR (editor)->context,
                                        seg,
                                        editor->blend_color_space,
                                        &lseg, &rseg);

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

  ligma_data_thaw (LIGMA_DATA (gradient));
}

static void
gradient_editor_control_drop_color (GtkWidget     *widget,
                                    gint           x,
                                    gint           y,
                                    const LigmaRGB *color,
                                    gpointer       data)
{
  LigmaGradientEditor    *editor = LIGMA_GRADIENT_EDITOR (data);
  LigmaGradient          *gradient;
  gdouble                xpos;
  LigmaGradientSegment   *seg, *lseg, *rseg;
  GradientEditorDragMode handle;

  gradient = LIGMA_GRADIENT (LIGMA_DATA_EDITOR (editor)->data);

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
      lseg = ligma_gradient_get_segment_at (gradient, xpos);
      rseg = NULL;
    }

  ligma_data_freeze (LIGMA_DATA (gradient));

  if (lseg)
    lseg->right_color = *color;

  if (rseg)
    rseg->left_color  = *color;

  ligma_data_thaw (LIGMA_DATA (gradient));
}

static void
gradient_editor_scrollbar_update (GtkAdjustment      *adjustment,
                                  LigmaGradientEditor *editor)
{
  LigmaDataEditor           *data_editor = LIGMA_DATA_EDITOR (editor);
  LigmaViewRendererGradient *renderer;
  gchar                    *str1;
  gchar                    *str2;

  str1 = g_strdup_printf (_("Zoom factor: %f:1"),
                          editor->zoom_factor);

  str2 = g_strdup_printf (_("Displaying [%0.4f, %0.4f]"),
                          gtk_adjustment_get_value (adjustment),
                          gtk_adjustment_get_value (adjustment) +
                          gtk_adjustment_get_page_size (adjustment));

  gradient_editor_set_hint (editor, str1, str2, NULL, NULL);

  g_free (str1);
  g_free (str2);

  renderer = LIGMA_VIEW_RENDERER_GRADIENT (LIGMA_VIEW (data_editor->view)->renderer);

  ligma_view_renderer_gradient_set_offsets (renderer,
                                           gtk_adjustment_get_value (adjustment),
                                           gtk_adjustment_get_value (adjustment) +
                                           gtk_adjustment_get_page_size (adjustment));

  ligma_view_renderer_update (LIGMA_VIEW_RENDERER (renderer));
  ligma_gradient_editor_update (editor);
}

static void
gradient_editor_set_hint (LigmaGradientEditor *editor,
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

static LigmaGradientSegment *
gradient_editor_save_selection (LigmaGradientEditor *editor)
{
  LigmaGradientSegment *seg, *prev, *tmp;
  LigmaGradientSegment *oseg, *oaseg;

  prev = NULL;
  oseg = editor->control_sel_l;
  tmp  = NULL;

  do
    {
      seg = ligma_gradient_segment_new ();

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
  while (oaseg != editor->control_sel_r);

  return tmp;
}

static void
gradient_editor_replace_selection (LigmaGradientEditor  *editor,
                                   LigmaGradientSegment *replace_seg)
{
  LigmaGradient        *gradient;
  LigmaGradientSegment *lseg, *rseg;
  LigmaGradientSegment *replace_last;

  gradient = LIGMA_GRADIENT (LIGMA_DATA_EDITOR (editor)->data);

  /* Remember left and right segments */

  lseg = editor->control_sel_l->prev;
  rseg = editor->control_sel_r->next;

  replace_last = ligma_gradient_segment_get_last (replace_seg);

  /* Free old selection */

  editor->control_sel_r->next = NULL;

  ligma_gradient_segments_free (editor->control_sel_l);

  /* Link in new segments */

  if (lseg)
    lseg->next = replace_seg;
  else
    gradient->segments = replace_seg;

  replace_seg->prev = lseg;

  if (rseg)
    rseg->prev = replace_last;

  replace_last->next = rseg;

  editor->control_sel_l = replace_seg;
  editor->control_sel_r = replace_last;
}

static void
gradient_editor_left_color_update (LigmaColorDialog      *dialog,
                                   const LigmaRGB        *color,
                                   LigmaColorDialogState  state,
                                   LigmaGradientEditor   *editor)
{
  LigmaGradient *gradient = LIGMA_GRADIENT (LIGMA_DATA_EDITOR (editor)->data);

  switch (state)
    {
    case LIGMA_COLOR_DIALOG_UPDATE:
      ligma_gradient_segment_range_blend (gradient,
                                         editor->control_sel_l,
                                         editor->control_sel_r,
                                         color,
                                         &editor->control_sel_r->right_color,
                                         TRUE, TRUE);
      break;

    case LIGMA_COLOR_DIALOG_OK:
      ligma_gradient_segment_range_blend (gradient,
                                         editor->control_sel_l,
                                         editor->control_sel_r,
                                         color,
                                         &editor->control_sel_r->right_color,
                                         TRUE, TRUE);
      ligma_gradient_segments_free (editor->saved_segments);
      gtk_widget_destroy (editor->color_dialog);
      editor->color_dialog = NULL;
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
      ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)),
                              ligma_editor_get_popup_data (LIGMA_EDITOR (editor)));
      break;

    case LIGMA_COLOR_DIALOG_CANCEL:
      gradient_editor_replace_selection (editor, editor->saved_segments);
      if (! editor->saved_dirty)
        ligma_data_clean (LIGMA_DATA (gradient));
      ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (gradient));
      gtk_widget_destroy (editor->color_dialog);
      editor->color_dialog = NULL;
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
      ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)),
                              ligma_editor_get_popup_data (LIGMA_EDITOR (editor)));
      break;
    }
}

static void
gradient_editor_right_color_update (LigmaColorDialog      *dialog,
                                    const LigmaRGB        *color,
                                    LigmaColorDialogState  state,
                                    LigmaGradientEditor   *editor)
{
  LigmaGradient *gradient = LIGMA_GRADIENT (LIGMA_DATA_EDITOR (editor)->data);

  switch (state)
    {
    case LIGMA_COLOR_DIALOG_UPDATE:
      ligma_gradient_segment_range_blend (gradient,
                                         editor->control_sel_l,
                                         editor->control_sel_r,
                                         &editor->control_sel_l->left_color,
                                         color,
                                         TRUE, TRUE);
      break;

    case LIGMA_COLOR_DIALOG_OK:
      ligma_gradient_segment_range_blend (gradient,
                                         editor->control_sel_l,
                                         editor->control_sel_r,
                                         &editor->control_sel_l->left_color,
                                         color,
                                         TRUE, TRUE);
      ligma_gradient_segments_free (editor->saved_segments);
      gtk_widget_destroy (editor->color_dialog);
      editor->color_dialog = NULL;
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
      ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)),
                              ligma_editor_get_popup_data (LIGMA_EDITOR (editor)));
      break;

    case LIGMA_COLOR_DIALOG_CANCEL:
      gradient_editor_replace_selection (editor, editor->saved_segments);
      if (! editor->saved_dirty)
        ligma_data_clean (LIGMA_DATA (gradient));
      ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (gradient));
      gtk_widget_destroy (editor->color_dialog);
      editor->color_dialog = NULL;
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
      ligma_ui_manager_update (ligma_editor_get_ui_manager (LIGMA_EDITOR (editor)),
                              ligma_editor_get_popup_data (LIGMA_EDITOR (editor)));
      break;
    }
}


/***** Gradient view functions *****/

static gboolean
view_events (GtkWidget          *widget,
             GdkEvent           *event,
             LigmaGradientEditor *editor)
{
  LigmaDataEditor *data_editor = LIGMA_DATA_EDITOR (editor);

  if (! data_editor->data)
    return TRUE;

  switch (event->type)
    {
    case GDK_LEAVE_NOTIFY:
      gradient_editor_set_hint (editor, NULL, NULL, NULL, NULL);
      editor->view_last_x = -1;
      break;

    case GDK_MOTION_NOTIFY:
      {
        GdkEventMotion *mevent = (GdkEventMotion *) event;

        if (mevent->x != editor->view_last_x)
          {
            editor->view_last_x = mevent->x;

            if (editor->view_button_down)
              {
                view_pick_color (editor,
                                 (mevent->state & ligma_get_toggle_behavior_mask ()) ?
                                 LIGMA_COLOR_PICK_TARGET_BACKGROUND :
                                 LIGMA_COLOR_PICK_TARGET_FOREGROUND,
                                 LIGMA_COLOR_PICK_STATE_UPDATE,
                                 mevent->x);
              }
            else
              {
                view_set_hint (editor, mevent->x);
              }
          }

        gdk_event_request_motions (mevent);
      }
      break;

    case GDK_BUTTON_PRESS:
      {
        GdkEventButton *bevent = (GdkEventButton *) event;

        if (gdk_event_triggers_context_menu ((GdkEvent *) bevent))
          {
            ligma_editor_popup_menu_at_pointer (LIGMA_EDITOR (editor), event);
          }
        else if (bevent->button == 1)
          {
            editor->view_last_x      = bevent->x;
            editor->view_button_down = TRUE;

            view_pick_color (editor,
                             (bevent->state & ligma_get_toggle_behavior_mask ()) ?
                             LIGMA_COLOR_PICK_TARGET_BACKGROUND :
                             LIGMA_COLOR_PICK_TARGET_FOREGROUND,
                             LIGMA_COLOR_PICK_STATE_START,
                             bevent->x);
          }
      }
      break;

    case GDK_SCROLL:
      {
        GdkEventScroll *sevent = (GdkEventScroll *) event;

        if (sevent->state & ligma_get_toggle_behavior_mask ())
          {
            gdouble delta;

            switch (sevent->direction)
              {
              case GDK_SCROLL_UP:
                ligma_gradient_editor_zoom (editor, LIGMA_ZOOM_IN, 1.0,
                                           view_get_normalized_last_x_pos (editor));
                break;

              case GDK_SCROLL_DOWN:
                ligma_gradient_editor_zoom (editor, LIGMA_ZOOM_OUT, 1.0,
                                           view_get_normalized_last_x_pos (editor));
                break;

              case GDK_SCROLL_SMOOTH:
                gdk_event_get_scroll_deltas (event, NULL, &delta);
                ligma_gradient_editor_zoom (editor, LIGMA_ZOOM_SMOOTH, delta,
                                           view_get_normalized_last_x_pos (editor));
                break;

              default:
                break;
              }
          }
        else
          {
            gdouble value;

            ligma_scroll_adjustment_values (sevent,
                                           editor->scroll_data, NULL,
                                           &value, NULL);

            gtk_adjustment_set_value (editor->scroll_data, value);
          }
      }
      break;

    case GDK_BUTTON_RELEASE:
      if (editor->view_button_down)
        {
          GdkEventButton *bevent = (GdkEventButton *) event;

          editor->view_last_x      = bevent->x;
          editor->view_button_down = FALSE;

          view_pick_color (editor,
                           (bevent->state & ligma_get_toggle_behavior_mask ()) ?
                           LIGMA_COLOR_PICK_TARGET_BACKGROUND :
                           LIGMA_COLOR_PICK_TARGET_FOREGROUND,
                           LIGMA_COLOR_PICK_STATE_END,
                           bevent->x);
          break;
        }
      break;

    default:
      return FALSE;
    }

  return TRUE;
}

static void
view_set_hint (LigmaGradientEditor *editor,
               gint                x)
{
  LigmaDataEditor *data_editor = LIGMA_DATA_EDITOR (editor);
  LigmaRGB         rgb;
  LigmaHSV         hsv;
  gdouble         xpos;
  gchar          *str1;
  gchar          *str2;
  gchar          *str3;
  gchar          *str4;

  xpos = control_calc_g_pos (editor, x);

  ligma_gradient_get_color_at (LIGMA_GRADIENT (data_editor->data),
                              data_editor->context, NULL,
                              xpos, FALSE, FALSE, &rgb);

  ligma_color_area_set_color (LIGMA_COLOR_AREA (editor->current_color), &rgb);

  ligma_rgb_to_hsv (&rgb, &hsv);

  str1 = g_strdup_printf (_("Position: %0.4f"), xpos);
  str2 = g_strdup_printf (_("RGB (%0.3f, %0.3f, %0.3f)"),
                          rgb.r, rgb.g, rgb.b);
  str3 = g_strdup_printf (_("HSV (%0.1f, %0.1f, %0.1f)"),
                          hsv.h * 360.0, hsv.s * 100.0, hsv.v * 100.0);
  str4 = g_strdup_printf (_("Luminance: %0.1f    Opacity: %0.1f"),
                          LIGMA_RGB_LUMINANCE (rgb.r, rgb.g, rgb.b) * 100.0,
                          rgb.a * 100.0);

  gradient_editor_set_hint (editor, str1, str2, str3, str4);

  g_free (str1);
  g_free (str2);
  g_free (str3);
  g_free (str4);
}

static void
view_pick_color (LigmaGradientEditor  *editor,
                 LigmaColorPickTarget  pick_target,
                 LigmaColorPickState   pick_state,
                 gint                 x)
{
  LigmaDataEditor *data_editor = LIGMA_DATA_EDITOR (editor);
  LigmaRGB         color;
  gdouble         xpos;
  gchar          *str2;
  gchar          *str3;

  xpos = control_calc_g_pos (editor, x);

  ligma_gradient_get_color_at (LIGMA_GRADIENT (data_editor->data),
                              data_editor->context, NULL,
                              xpos, FALSE, FALSE, &color);

  ligma_color_area_set_color (LIGMA_COLOR_AREA (editor->current_color), &color);

  str2 = g_strdup_printf (_("RGB (%d, %d, %d)"),
                          (gint) (color.r * 255.0),
                          (gint) (color.g * 255.0),
                          (gint) (color.b * 255.0));

  str3 = g_strdup_printf ("(%0.3f, %0.3f, %0.3f)", color.r, color.g, color.b);

  if (pick_target == LIGMA_COLOR_PICK_TARGET_FOREGROUND)
    {
      ligma_context_set_foreground (data_editor->context, &color);

      gradient_editor_set_hint (editor, _("Foreground color set to:"),
                                str2, str3, NULL);
    }
  else
    {
      ligma_context_set_background (data_editor->context, &color);

      gradient_editor_set_hint (editor, _("Background color set to:"),
                                str2, str3, NULL);
    }

  g_free (str2);
  g_free (str3);
}

static void
view_zoom_gesture_begin (GtkGestureZoom     *gesture,
                         GdkEventSequence   *sequence,
                         LigmaGradientEditor *editor)
{
  editor->last_zoom_scale = gtk_gesture_zoom_get_scale_delta (gesture);
}

static void
view_zoom_gesture_update (GtkGestureZoom     *gesture,
                          GdkEventSequence   *sequence,
                          LigmaGradientEditor *editor)
{
  gdouble current_scale = gtk_gesture_zoom_get_scale_delta (gesture);
  gdouble delta = (current_scale - editor->last_zoom_scale) / editor->last_zoom_scale;
  editor->last_zoom_scale = current_scale;

  ligma_gradient_editor_zoom (editor, LIGMA_ZOOM_PINCH, delta,
                             view_get_normalized_last_x_pos (editor));
}

static gdouble
view_get_normalized_last_x_pos (LigmaGradientEditor *editor)
{
  GtkAllocation allocation;
  gdouble       normalized;
  if (editor->view_last_x < 0)
    return 0.5;

  gtk_widget_get_allocation (LIGMA_DATA_EDITOR (editor)->view, &allocation);
  normalized = (double) editor->view_last_x / (allocation.width - 1);

  if (normalized < 0)
    return 0;
  if (normalized > 1.0)
    return 1;
  return normalized;
}

/***** Gradient control functions *****/

static gboolean
control_events (GtkWidget          *widget,
                GdkEvent           *event,
                LigmaGradientEditor *editor)
{
  LigmaGradient        *gradient;
  LigmaGradientSegment *seg;

  if (! LIGMA_DATA_EDITOR (editor)->data)
    return TRUE;

  gradient = LIGMA_GRADIENT (LIGMA_DATA_EDITOR (editor)->data);

  switch (event->type)
    {
    case GDK_LEAVE_NOTIFY:
      gradient_editor_set_hint (editor, NULL, NULL, NULL, NULL);
      editor->control_last_x = -1;
      break;

    case GDK_BUTTON_PRESS:
      if (editor->control_drag_mode == GRAD_DRAG_NONE)
        {
          GdkEventButton *bevent = (GdkEventButton *) event;

          editor->control_last_x     = bevent->x;
          editor->control_click_time = bevent->time;

          control_button_press (editor, bevent);

          if (editor->control_drag_mode != GRAD_DRAG_NONE)
            {
              gtk_grab_add (widget);

              if (LIGMA_DATA_EDITOR (editor)->data_editable)
                {
                  g_signal_handlers_block_by_func (gradient,
                                                   ligma_gradient_editor_gradient_dirty,
                                                   editor);
                }
            }
        }
      break;

    case GDK_SCROLL:
      {
        GdkEventScroll *sevent = (GdkEventScroll *) event;

        if (sevent->state & ligma_get_toggle_behavior_mask ())
          {
            gdouble delta;

            switch (sevent->direction)
              {
              case GDK_SCROLL_UP:
                ligma_gradient_editor_zoom (editor, LIGMA_ZOOM_IN, 1.0,
                                           control_get_normalized_last_x_pos (editor));
                break;

              case GDK_SCROLL_DOWN:
                ligma_gradient_editor_zoom (editor, LIGMA_ZOOM_OUT, 1.0,
                                           control_get_normalized_last_x_pos (editor));
                break;

              case GDK_SCROLL_SMOOTH:
                gdk_event_get_scroll_deltas (event, NULL, &delta);
                ligma_gradient_editor_zoom (editor, LIGMA_ZOOM_SMOOTH, delta,
                                           control_get_normalized_last_x_pos (editor));
                break;

              default:
                break;
              }
          }
        else
          {
            gdouble value;

            ligma_scroll_adjustment_values (sevent,
                                           editor->scroll_data, NULL,
                                           &value, NULL);

            gtk_adjustment_set_value (editor->scroll_data, value);
          }
      }
      break;

    case GDK_BUTTON_RELEASE:
      {
        GdkEventButton *bevent = (GdkEventButton *) event;

        gradient_editor_set_hint (editor, NULL, NULL, NULL, NULL);

        if (editor->control_drag_mode != GRAD_DRAG_NONE)
          {
            if (LIGMA_DATA_EDITOR (editor)->data_editable)
              {
                g_signal_handlers_unblock_by_func (gradient,
                                                   ligma_gradient_editor_gradient_dirty,
                                                   editor);
              }

            gtk_grab_remove (widget);

            if ((bevent->time - editor->control_click_time) >= GRAD_MOVE_TIME)
              {
                /* stuff was done in motion */
              }
            else if ((editor->control_drag_mode == GRAD_DRAG_MIDDLE) ||
                     (editor->control_drag_mode == GRAD_DRAG_ALL))
              {
                seg = editor->control_drag_segment;

                if ((editor->control_drag_mode == GRAD_DRAG_ALL) &&
                    editor->control_compress)
                  {
                    control_extend_selection (editor, seg,
                                              control_calc_g_pos (editor,
                                                                  bevent->x));
                  }
                else
                  {
                    control_select_single_segment (editor, seg);
                  }

                ligma_gradient_editor_update (editor);
              }

            editor->control_drag_mode = GRAD_DRAG_NONE;
            editor->control_compress  = FALSE;

            control_do_hint (editor, bevent->x, bevent->y);
          }
      }
      break;

    case GDK_MOTION_NOTIFY:
      {
        GdkEventMotion *mevent = (GdkEventMotion *) event;

        if (mevent->x != editor->control_last_x)
          {
            editor->control_last_x = mevent->x;

            if (LIGMA_DATA_EDITOR (editor)->data_editable &&
                editor->control_drag_mode != GRAD_DRAG_NONE)
              {

                if ((mevent->time - editor->control_click_time) >= GRAD_MOVE_TIME)
                  control_motion (editor, gradient, mevent->x);
              }
            else
              {
                ligma_gradient_editor_update (editor);

                control_do_hint (editor, mevent->x, mevent->y);
              }
          }

        gdk_event_request_motions (mevent);
      }
      break;

    default:
      return FALSE;
    }

  return TRUE;
}

static gboolean
control_draw (GtkWidget          *widget,
              cairo_t            *cr,
              LigmaGradientEditor *editor)
{
  GtkAdjustment *adj = editor->scroll_data;
  GtkAllocation  allocation;

  gtk_widget_get_allocation (widget, &allocation);

  control_draw_all (editor,
                    LIGMA_GRADIENT (LIGMA_DATA_EDITOR (editor)->data),
                    cr,
                    allocation.width,
                    allocation.height,
                    gtk_adjustment_get_value (adj),
                    gtk_adjustment_get_value (adj) +
                    gtk_adjustment_get_page_size (adj));

  return TRUE;
}

static void
control_do_hint (LigmaGradientEditor *editor,
                 gint                x,
                 gint                y)
{
  LigmaGradient           *gradient;
  LigmaGradientSegment    *seg;
  GradientEditorDragMode  handle;
  gboolean                in_handle;
  gdouble                 pos;
  gchar                  *str;

  gradient = LIGMA_GRADIENT (LIGMA_DATA_EDITOR (editor)->data);

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
                  str = g_strdup_printf (_("%s-Drag: move & compress"),
                                         ligma_get_mod_string (GDK_SHIFT_MASK));

                  gradient_editor_set_hint (editor,
                                            NULL,
                                            _("Drag: move"),
                                            str,
                                            NULL);
                  g_free (str);
                }
              else
                {
                  str = g_strdup_printf (_("%s-Click: extend selection"),
                                         ligma_get_mod_string (GDK_SHIFT_MASK));

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
              str = g_strdup_printf (_("%s-Click: extend selection"),
                                     ligma_get_mod_string (GDK_SHIFT_MASK));

              gradient_editor_set_hint (editor,
                                        NULL,
                                        _("Click: select"),
                                        str,
                                        NULL);
              g_free (str);
            }
          break;

        case GRAD_DRAG_MIDDLE:
          str = g_strdup_printf (_("%s-Click: extend selection"),
                                 ligma_get_mod_string (GDK_SHIFT_MASK));

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

      str  = g_strdup_printf (_("%s-Click: extend selection"),
                              ligma_get_mod_string (GDK_SHIFT_MASK));
      str2 = g_strdup_printf (_("%s-Drag: move & compress"),
                              ligma_get_mod_string (GDK_SHIFT_MASK));

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
control_button_press (LigmaGradientEditor *editor,
                      GdkEventButton     *bevent)
{
  LigmaGradient           *gradient;
  LigmaGradientSegment    *seg;
  GradientEditorDragMode  handle;
  gdouble                 xpos;
  gboolean                in_handle;

  gradient = LIGMA_GRADIENT (LIGMA_DATA_EDITOR (editor)->data);

  if (gdk_event_triggers_context_menu ((GdkEvent *) bevent))
    {
      ligma_editor_popup_menu_at_pointer (LIGMA_EDITOR (editor), (GdkEvent *) bevent);
      return;
    }

  /* Find the closest handle */

  xpos = control_calc_g_pos (editor, bevent->x);

  seg_get_closest_handle (gradient, xpos, &seg, &handle);

  in_handle = control_point_in_handle (editor, gradient, bevent->x, bevent->y, seg, handle);

  /* Now see what we have */

  if (in_handle)
    {
      switch (handle)
        {
        case GRAD_DRAG_LEFT:
          if (seg != NULL)
            {
              /* Left handle of some segment */
              if (bevent->state & GDK_SHIFT_MASK)
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
                      ligma_gradient_editor_update (editor);
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
                  ligma_gradient_editor_update (editor);
                }
            }
          else  /* seg == NULL */
            {
              /* Right handle of last segment */
              seg = ligma_gradient_segment_get_last (gradient->segments);

              if (bevent->state & GDK_SHIFT_MASK)
                {
                  control_extend_selection (editor, seg, xpos);
                  ligma_gradient_editor_update (editor);
                }
              else
                {
                  control_select_single_segment (editor, seg);
                  ligma_gradient_editor_update (editor);
                }
            }

          break;

        case GRAD_DRAG_MIDDLE:
          if (bevent->state & GDK_SHIFT_MASK)
            {
              control_extend_selection (editor, seg, xpos);
              ligma_gradient_editor_update (editor);
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
      seg = ligma_gradient_get_segment_at (gradient, xpos);

      editor->control_drag_mode    = GRAD_DRAG_ALL;
      editor->control_drag_segment = seg;
      editor->control_last_gx      = xpos;
      editor->control_orig_pos     = xpos;

      if (bevent->state & GDK_SHIFT_MASK)
        editor->control_compress = TRUE;
    }
}

static gboolean
control_point_in_handle (LigmaGradientEditor     *editor,
                         LigmaGradient           *gradient,
                         gint                    x,
                         gint                    y,
                         LigmaGradientSegment    *seg,
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
          seg = ligma_gradient_segment_get_last (gradient->segments);

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
control_select_single_segment (LigmaGradientEditor  *editor,
                               LigmaGradientSegment *seg)
{
  editor->control_sel_l = seg;
  editor->control_sel_r = seg;
}

static void
control_extend_selection (LigmaGradientEditor  *editor,
                          LigmaGradientSegment *seg,
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
control_motion (LigmaGradientEditor *editor,
                LigmaGradient       *gradient,
                gint                x)
{
  LigmaGradientSegment *seg = editor->control_drag_segment;
  gdouble              pos;
  gdouble              delta;
  gchar               *str = NULL;

  switch (editor->control_drag_mode)
    {
    case GRAD_DRAG_LEFT:
      pos = control_calc_g_pos (editor, x);

      if (! editor->control_compress)
        ligma_gradient_segment_set_left_pos (gradient, seg, pos);
      else
        control_compress_left (gradient,
                               editor->control_sel_l,
                               editor->control_sel_r,
                               seg, pos);

      str = g_strdup_printf (_("Handle position: %0.4f"), seg->left);
      break;

    case GRAD_DRAG_MIDDLE:
      pos = control_calc_g_pos (editor, x);

      ligma_gradient_segment_set_middle_pos (gradient, seg, pos);

      str = g_strdup_printf (_("Handle position: %0.4f"), seg->middle);
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

      str = g_strdup_printf (_("Distance: %0.4f"),
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

  ligma_gradient_editor_update (editor);
}

static void
control_compress_left (LigmaGradient        *gradient,
                       LigmaGradientSegment *range_l,
                       LigmaGradientSegment *range_r,
                       LigmaGradientSegment *drag_seg,
                       gdouble              pos)
{
  LigmaGradientSegment *seg;
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
    ligma_gradient_segment_range_compress (gradient,
                                          range_l->prev, range_l->prev,
                                          range_l->prev->left, pos);
  else
    ligma_gradient_segment_range_compress (gradient,
                                          range_l, drag_seg->prev,
                                          range_l->left, pos);

  /* Compress segments to the right of the handle */

  if (drag_seg != range_r->next)
    ligma_gradient_segment_range_compress (gradient,
                                          drag_seg, range_r,
                                          pos, range_r->right);
  else
    ligma_gradient_segment_range_compress (gradient,
                                          drag_seg, drag_seg,
                                          pos, drag_seg->right);
}

/*****/

static gdouble
control_move (LigmaGradientEditor  *editor,
              LigmaGradientSegment *range_l,
              LigmaGradientSegment *range_r,
              gdouble              delta)
{
  LigmaGradient *gradient = LIGMA_GRADIENT (LIGMA_DATA_EDITOR (editor)->data);

  return ligma_gradient_segment_range_move (gradient,
                                           range_l,
                                           range_r,
                                           delta,
                                           editor->control_compress);
}

/*****/

static gdouble
control_get_normalized_last_x_pos (LigmaGradientEditor *editor)
{
  GtkAllocation allocation;
  gdouble       normalized;
  if (editor->control_last_x < 0)
    return 0.5;

  gtk_widget_get_allocation (editor->control, &allocation);
  normalized = (double) editor->control_last_x / (allocation.width - 1);

  if (normalized < 0)
    return 0;
  if (normalized > 1.0)
    return 1;
  return normalized;
}

/*****/

static void
control_update (LigmaGradientEditor *editor,
                LigmaGradient       *gradient,
                gboolean            reset_selection)
{
  if (! editor->control_sel_l || ! editor->control_sel_r)
    reset_selection = TRUE;

  if (reset_selection)
    {
      if (gradient)
        control_select_single_segment (editor, gradient->segments);
      else
        control_select_single_segment (editor, NULL);
    }

  gtk_widget_queue_draw (editor->control);
}

static void
control_draw_all (LigmaGradientEditor *editor,
                  LigmaGradient       *gradient,
                  cairo_t            *cr,
                  gint                width,
                  gint                height,
                  gdouble             left,
                  gdouble             right)
{
  GtkStyleContext        *style;
  LigmaGradientSegment    *seg;
  GradientEditorDragMode  handle;
  gint                    sel_l;
  gint                    sel_r;
  gdouble                 g_pos;
  GtkStateFlags           flags;

  if (! gradient)
    return;

  /* Draw selection */

  style = gtk_widget_get_style_context (editor->control);

  sel_l = control_calc_p_pos (editor, editor->control_sel_l->left);
  sel_r = control_calc_p_pos (editor, editor->control_sel_r->right);

  gtk_style_context_save (style);

  gtk_style_context_add_class (style, GTK_STYLE_CLASS_VIEW);
  gtk_render_background (style, cr, 0, 0, width, height);

  gtk_style_context_set_state (style, GTK_STATE_FLAG_SELECTED);
  gtk_render_background (style, cr,sel_l, 0, sel_r - sel_l + 1, height);

  gtk_style_context_restore (style);

  /* Draw handles */

  flags = GTK_STATE_FLAG_NORMAL;

  for (seg = gradient->segments; seg; seg = seg->next)
    {
      if (seg == editor->control_sel_l)
        flags = GTK_STATE_FLAG_SELECTED;

      control_draw_handle (editor, style, cr, seg->left,
                           height, FALSE, flags);
      control_draw_handle (editor, style, cr, seg->middle,
                           height, TRUE,  flags);

      /* Draw right handle only if this is the last segment */
      if (seg->next == NULL)
        control_draw_handle (editor, style, cr, seg->right,
                             height, FALSE, flags);

      if (seg == editor->control_sel_r)
        flags = GTK_STATE_FLAG_NORMAL;
    }

  /* Draw the handle which is closest to the mouse position */

  flags = GTK_STATE_FLAG_PRELIGHT;

  g_pos = control_calc_g_pos (editor, editor->control_last_x);

  seg_get_closest_handle (gradient, CLAMP (g_pos, 0.0, 1.0), &seg, &handle);

  if (seg && seg_in_selection (gradient, seg,
                               editor->control_sel_l, editor->control_sel_r))
    flags |= GTK_STATE_FLAG_SELECTED;

  switch (handle)
    {
    case GRAD_DRAG_LEFT:
      if (seg)
        {
          control_draw_handle (editor, style, cr, seg->left,
                               height, FALSE, flags);
        }
      else
        {
          seg = ligma_gradient_segment_get_last (gradient->segments);

          if (seg == editor->control_sel_r)
            flags |= GTK_STATE_FLAG_SELECTED;

          control_draw_handle (editor, style, cr, seg->right,
                               height, FALSE, flags);
        }

      break;

    case GRAD_DRAG_MIDDLE:
      control_draw_handle (editor, style, cr, seg->middle,
                           height, TRUE, flags);
      break;

    default:
      break;
    }
}

static void
control_draw_handle (LigmaGradientEditor *editor,
                     GtkStyleContext    *style,
                     cairo_t            *cr,
                     gdouble             pos,
                     gint                height,
                     gboolean            middle,
                     GtkStateFlags       flags)
{
  GdkRGBA  color;
  gint     xpos     = control_calc_p_pos (editor, pos);
  gboolean selected = (flags & GTK_STATE_FLAG_SELECTED) != 0;

  cairo_save (cr);

  cairo_move_to (cr, xpos, 0);
  cairo_line_to (cr, xpos - height / 2.0, height);
  cairo_line_to (cr, xpos + height / 2.0, height);
  cairo_line_to (cr, xpos, 0);

  gtk_style_context_save (style);

  gtk_style_context_add_class (style, GTK_STYLE_CLASS_VIEW);

  gtk_style_context_save (style);

  gtk_style_context_set_state (style, flags);

  gtk_style_context_get_color (style, gtk_style_context_get_state (style),
                               &color);

  gtk_style_context_restore (style);

  if (middle)
    color.alpha = 0.5;

  gdk_cairo_set_source_rgba (cr, &color);
  cairo_fill_preserve (cr);

  if (selected)
    gtk_style_context_set_state (style, flags &= ~GTK_STATE_FLAG_SELECTED);
  else
    gtk_style_context_set_state (style, flags |= GTK_STATE_FLAG_SELECTED);

  gtk_style_context_get_color (style, gtk_style_context_get_state (style),
                               &color);

  gdk_cairo_set_source_rgba (cr, &color);
  cairo_set_line_width (cr, 1);
  cairo_stroke (cr);

  gtk_style_context_restore (style);

  cairo_restore (cr);
}

/*****/

static gint
control_calc_p_pos (LigmaGradientEditor *editor,
                    gdouble             pos)
{
  GtkAdjustment *adjustment = editor->scroll_data;
  GtkAllocation  allocation;
  gint           pwidth;

  gtk_widget_get_allocation (editor->control, &allocation);

  pwidth = allocation.width;

  /* Calculate the position (in widget's coordinates) of the
   * requested point from the gradient.  Rounding is done to
   * minimize mismatches between the rendered gradient view
   * and the gradient control's handles.
   */

  return RINT ((pwidth - 1) * (pos - gtk_adjustment_get_value (adjustment)) /
               gtk_adjustment_get_page_size (adjustment));
}

static gdouble
control_calc_g_pos (LigmaGradientEditor *editor,
                    gint                pos)
{
  GtkAdjustment *adjustment = editor->scroll_data;
  GtkAllocation  allocation;
  gint           pwidth;

  gtk_widget_get_allocation (editor->control, &allocation);

  pwidth = allocation.width;

  /* Calculate the gradient position that corresponds to widget's coordinates */

  return (gtk_adjustment_get_page_size (adjustment) * pos / (pwidth - 1) +
          gtk_adjustment_get_value (adjustment));
}

/***** Segment functions *****/

static void
seg_get_closest_handle (LigmaGradient            *grad,
                        gdouble                  pos,
                        LigmaGradientSegment    **seg,
                        GradientEditorDragMode  *handle)
{
  gdouble l_delta, m_delta, r_delta;

  *seg = ligma_gradient_get_segment_at (grad, pos);

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
seg_in_selection (LigmaGradient         *grad,
                  LigmaGradientSegment  *seg,
                  LigmaGradientSegment  *left,
                  LigmaGradientSegment  *right)
{
  LigmaGradientSegment *s;

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
