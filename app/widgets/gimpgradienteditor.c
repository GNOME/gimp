/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

#include <gegl.h>
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

#define GRAD_VIEW_SIZE            96
#define GRAD_CONTROL_HEIGHT       14
#define GRAD_CURRENT_COLOR_WIDTH  16

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

static void   gimp_gradient_editor_constructed      (GObject            *object);
static void   gimp_gradient_editor_dispose          (GObject            *object);

static void   gimp_gradient_editor_unmap            (GtkWidget          *widget);
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

static void      view_pick_color                  (GimpGradientEditor *editor,
                                                   GimpColorPickMode   pick_mode,
                                                   GimpColorPickState  pick_state,
                                                   gint                x);

/* Gradient control functions */

static gboolean  control_events                   (GtkWidget          *widget,
                                                   GdkEvent           *event,
                                                   GimpGradientEditor *editor);
static gboolean  control_draw                     (GtkWidget          *widget,
                                                   cairo_t            *cr,
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
static void      control_draw_all                 (GimpGradientEditor *editor,
                                                   GimpGradient       *gradient,
                                                   cairo_t            *cr,
                                                   gint                width,
                                                   gint                height,
                                                   gdouble             left,
                                                   gdouble             right);
static void      control_draw_normal_handle       (GimpGradientEditor *editor,
                                                   cairo_t            *cr,
                                                   gdouble             pos,
                                                   gint                height,
                                                   gboolean            selected);
static void      control_draw_middle_handle       (GimpGradientEditor *editor,
                                                   cairo_t            *cr,
                                                   gdouble             pos,
                                                   gint                height,
                                                   gboolean            selected);
static void      control_draw_handle              (cairo_t            *cr,
                                                   const GdkRGBA      *border,
                                                   const GdkRGBA      *fill,
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
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass      *widget_class = GTK_WIDGET_CLASS (klass);
  GimpDataEditorClass *editor_class = GIMP_DATA_EDITOR_CLASS (klass);

  object_class->constructed = gimp_gradient_editor_constructed;
  object_class->dispose     = gimp_gradient_editor_dispose;

  widget_class->unmap       = gimp_gradient_editor_unmap;

  editor_class->set_data    = gimp_gradient_editor_set_data;
  editor_class->title       = _("Gradient Editor");

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boxed ("handle-color",
                                                               NULL, NULL,
                                                               GDK_TYPE_RGBA,
                                                               GIMP_PARAM_READABLE));
  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_boxed ("handle-color-selected",
                                                               NULL, NULL,
                                                               GDK_TYPE_RGBA,
                                                               GIMP_PARAM_READABLE));
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
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (editor);
  GtkWidget      *frame;
  GtkWidget      *vbox;
  GtkWidget      *hbox;
  GtkWidget      *hint_vbox;
  GimpRGB         transp;
  GtkCssProvider *css;
  const gchar    *str;

  gimp_rgba_set (&transp, 0.0, 0.0, 0.0, 0.0);

  /* Frame for gradient view and gradient control */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /* Gradient view */
  editor->view_last_x      = 0;
  editor->view_button_down = FALSE;

  data_editor->view = gimp_view_new_full_by_types (NULL,
                                                   GIMP_TYPE_VIEW,
                                                   GIMP_TYPE_GRADIENT,
                                                   GRAD_VIEW_SIZE,
                                                   GRAD_VIEW_SIZE, 0,
                                                   FALSE, FALSE, FALSE);
  gtk_widget_set_size_request (data_editor->view, -1, GRAD_VIEW_SIZE);
  gtk_widget_set_events (data_editor->view, GRAD_VIEW_EVENT_MASK);
  gimp_view_set_expand (GIMP_VIEW (data_editor->view), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), data_editor->view, TRUE, TRUE, 0);
  gtk_widget_show (data_editor->view);

  g_signal_connect (data_editor->view, "event",
                    G_CALLBACK (view_events),
                    editor);

  gimp_dnd_viewable_dest_add (GTK_WIDGET (data_editor->view),
                              GIMP_TYPE_GRADIENT,
                              gradient_editor_drop_gradient,
                              editor);

  gimp_dnd_color_dest_add (GTK_WIDGET (data_editor->view),
                              gradient_editor_drop_color,
                              editor);

  /* Gradient control */
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

  gimp_dnd_color_dest_add (GTK_WIDGET (editor->control),
                              gradient_editor_control_drop_color,
                              editor);

  /*  Scrollbar  */
  editor->zoom_factor = 1;

  editor->scroll_data = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 1.0,
                                                            GRAD_SCROLLBAR_STEP_SIZE,
                                                            GRAD_SCROLLBAR_PAGE_SIZE,
                                                            1.0));

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

  editor->current_color = gimp_color_area_new (&transp,
                                               GIMP_COLOR_AREA_SMALL_CHECKS,
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

  str =
    "GimpGradientEditor {\n"
    "  -GimpGradientEditor-handle-color: mix (@text_color, @base_color, 0.5);\n"
    "  -GimpGradientEditor-handle-color-selected: mix (@selected_fg_color, @selected_bg_color, 0.5);\n"
    "}\n";

  css = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (css, str, -1, NULL);
  gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (editor)),
                                  GTK_STYLE_PROVIDER (css),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (css);
}

static void
gimp_gradient_editor_constructed (GObject *object)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_editor_add_action_button (GIMP_EDITOR (editor), "gradient-editor",
                                 "gradient-editor-zoom-out", NULL);

  gimp_editor_add_action_button (GIMP_EDITOR (editor), "gradient-editor",
                                 "gradient-editor-zoom-in", NULL);

  gimp_editor_add_action_button (GIMP_EDITOR (editor), "gradient-editor",
                                 "gradient-editor-zoom-all", NULL);
}

static void
gimp_gradient_editor_dispose (GObject *object)
{
  GimpGradientEditor *editor = GIMP_GRADIENT_EDITOR (object);

  if (editor->color_dialog)
    gtk_dialog_response (GTK_DIALOG (editor->color_dialog),
                         GTK_RESPONSE_CANCEL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
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

  gimp_view_set_viewable (GIMP_VIEW (editor->view),
                          GIMP_VIEWABLE (data));

  control_update (gradient_editor, GIMP_GRADIENT (data), TRUE);
}

static void
gimp_gradient_editor_set_context (GimpDocked  *docked,
                                  GimpContext *context)
{
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (docked);

  parent_docked_iface->set_context (docked, context);

  gimp_view_renderer_set_context (GIMP_VIEW (data_editor->view)->renderer,
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

  adjustment = editor->scroll_data;

  old_value     = gtk_adjustment_get_value (adjustment);
  old_page_size = gtk_adjustment_get_page_size (adjustment);

  switch (zoom_type)
    {
    case GIMP_ZOOM_IN_MAX:
    case GIMP_ZOOM_IN_MORE:
    case GIMP_ZOOM_IN:
      editor->zoom_factor++;

      page_size = 1.0 / editor->zoom_factor;
      value     = old_value + (old_page_size - page_size) / 2.0;
      break;

    case GIMP_ZOOM_OUT_MORE:
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

    case GIMP_ZOOM_OUT_MAX:
    case GIMP_ZOOM_TO: /* abused as ZOOM_ALL */
      editor->zoom_factor = 1;

      value     = 0.0;
      page_size = 1.0;
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

  gimp_gradient_segment_split_midpoint (gradient,
                                        GIMP_DATA_EDITOR (editor)->context,
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
  GimpDataEditor           *data_editor = GIMP_DATA_EDITOR (editor);
  GimpViewRendererGradient *renderer;
  gchar                    *str1;
  gchar                    *str2;

  str1 = g_strdup_printf (_("Zoom factor: %d:1"),
                          editor->zoom_factor);

  str2 = g_strdup_printf (_("Displaying [%0.4f, %0.4f]"),
                          gtk_adjustment_get_value (adjustment),
                          gtk_adjustment_get_value (adjustment) +
                          gtk_adjustment_get_page_size (adjustment));

  gradient_editor_set_hint (editor, str1, str2, NULL, NULL);

  g_free (str1);
  g_free (str2);

  renderer = GIMP_VIEW_RENDERER_GRADIENT (GIMP_VIEW (data_editor->view)->renderer);

  gimp_view_renderer_gradient_set_offsets (renderer,
                                           gtk_adjustment_get_value (adjustment),
                                           gtk_adjustment_get_value (adjustment) +
                                           gtk_adjustment_get_page_size (adjustment));

  gimp_view_renderer_update (GIMP_VIEW_RENDERER (renderer));
  gimp_gradient_editor_update (editor);
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
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (editor);

  if (! data_editor->data)
    return TRUE;

  switch (event->type)
    {
    case GDK_LEAVE_NOTIFY:
      gradient_editor_set_hint (editor, NULL, NULL, NULL, NULL);
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
                                 (mevent->state & gimp_get_toggle_behavior_mask ()) ?
                                 GIMP_COLOR_PICK_MODE_BACKGROUND :
                                 GIMP_COLOR_PICK_MODE_FOREGROUND,
                                 GIMP_COLOR_PICK_STATE_UPDATE,
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
            gimp_editor_popup_menu (GIMP_EDITOR (editor), NULL, NULL);
          }
        else if (bevent->button == 1)
          {
            editor->view_last_x      = bevent->x;
            editor->view_button_down = TRUE;

            view_pick_color (editor,
                             (bevent->state & gimp_get_toggle_behavior_mask ()) ?
                             GIMP_COLOR_PICK_MODE_BACKGROUND :
                             GIMP_COLOR_PICK_MODE_FOREGROUND,
                             GIMP_COLOR_PICK_STATE_START,
                             bevent->x);
          }
      }
      break;

    case GDK_SCROLL:
      {
        GdkEventScroll *sevent = (GdkEventScroll *) event;

        if (sevent->state & gimp_get_toggle_behavior_mask ())
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
            GtkAdjustment *adj   = editor->scroll_data;
            gfloat         value = gtk_adjustment_get_value (adj);

            switch (sevent->direction)
              {
              case GDK_SCROLL_UP:
                value -= gtk_adjustment_get_page_increment (adj) / 2;
                break;

              case GDK_SCROLL_DOWN:
                value += gtk_adjustment_get_page_increment (adj) / 2;
                break;

              default:
                break;
              }

            value = CLAMP (value,
                           gtk_adjustment_get_lower (adj),
                           gtk_adjustment_get_upper (adj) -
                           gtk_adjustment_get_page_size (adj));

            gtk_adjustment_set_value (adj, value);
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
                           (bevent->state & gimp_get_toggle_behavior_mask ()) ?
                           GIMP_COLOR_PICK_MODE_BACKGROUND :
                           GIMP_COLOR_PICK_MODE_FOREGROUND,
                           GIMP_COLOR_PICK_STATE_END,
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
view_set_hint (GimpGradientEditor *editor,
               gint                x)
{
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (editor);
  GimpRGB         rgb;
  GimpHSV         hsv;
  gdouble         xpos;
  gchar          *str1;
  gchar          *str2;
  gchar          *str3;
  gchar          *str4;

  xpos = control_calc_g_pos (editor, x);

  gimp_gradient_get_color_at (GIMP_GRADIENT (data_editor->data),
                              data_editor->context, NULL,
                              xpos, FALSE, &rgb);

  gimp_color_area_set_color (GIMP_COLOR_AREA (editor->current_color), &rgb);

  gimp_rgb_to_hsv (&rgb, &hsv);

  str1 = g_strdup_printf (_("Position: %0.4f"), xpos);
  str2 = g_strdup_printf (_("RGB (%0.3f, %0.3f, %0.3f)"),
                          rgb.r, rgb.g, rgb.b);
  str3 = g_strdup_printf (_("HSV (%0.1f, %0.1f, %0.1f)"),
                          hsv.h * 360.0, hsv.s * 100.0, hsv.v * 100.0);
  str4 = g_strdup_printf (_("Luminance: %0.1f    Opacity: %0.1f"),
                          GIMP_RGB_LUMINANCE (rgb.r, rgb.g, rgb.b) * 100.0,
                          rgb.a * 100.0);

  gradient_editor_set_hint (editor, str1, str2, str3, str4);

  g_free (str1);
  g_free (str2);
  g_free (str3);
  g_free (str4);
}

static void
view_pick_color (GimpGradientEditor *editor,
                 GimpColorPickMode   pick_mode,
                 GimpColorPickState  pick_state,
                 gint                x)
{
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (editor);
  GimpRGB         color;
  gdouble         xpos;
  gchar          *str2;
  gchar          *str3;

  xpos = control_calc_g_pos (editor, x);

  gimp_gradient_get_color_at (GIMP_GRADIENT (data_editor->data),
                              data_editor->context, NULL,
                              xpos, FALSE, &color);

  gimp_color_area_set_color (GIMP_COLOR_AREA (editor->current_color), &color);

  str2 = g_strdup_printf (_("RGB (%d, %d, %d)"),
                          (gint) (color.r * 255.0),
                          (gint) (color.g * 255.0),
                          (gint) (color.b * 255.0));

  str3 = g_strdup_printf ("(%0.3f, %0.3f, %0.3f)", color.r, color.g, color.b);

  if (pick_mode == GIMP_COLOR_PICK_MODE_FOREGROUND)
    {
      gimp_context_set_foreground (data_editor->context, &color);

      gradient_editor_set_hint (editor, _("Foreground color set to:"),
                                str2, str3, NULL);
    }
  else
    {
      gimp_context_set_background (data_editor->context, &color);

      gradient_editor_set_hint (editor, _("Background color set to:"),
                                str2, str3, NULL);
    }

  g_free (str2);
  g_free (str3);
}

/***** Gradient control functions *****/

static gboolean
control_events (GtkWidget          *widget,
                GdkEvent           *event,
                GimpGradientEditor *editor)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg;

  if (! GIMP_DATA_EDITOR (editor)->data)
    return TRUE;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  switch (event->type)
    {
    case GDK_LEAVE_NOTIFY:
      gradient_editor_set_hint (editor, NULL, NULL, NULL, NULL);
      break;

    case GDK_BUTTON_PRESS:
      if (editor->control_drag_mode == GRAD_DRAG_NONE)
        {
          GdkEventButton *bevent = (GdkEventButton *) event;

          editor->control_last_x     = bevent->x;
          editor->control_click_time = bevent->time;

          control_button_press (editor,
                                bevent->x, bevent->y,
                                bevent->button, bevent->state);

          if (editor->control_drag_mode != GRAD_DRAG_NONE)
            {
              gtk_grab_add (widget);

              if (GIMP_DATA_EDITOR (editor)->data_editable)
                {
                  g_signal_handlers_block_by_func (gradient,
                                                   gimp_gradient_editor_gradient_dirty,
                                                   editor);
                }
            }
        }
      break;

    case GDK_SCROLL:
      {
        GdkEventScroll *sevent = (GdkEventScroll *) event;

        if (sevent->state & gimp_get_toggle_behavior_mask ())
          {
            if (sevent->direction == GDK_SCROLL_UP)
              gimp_gradient_editor_zoom (editor, GIMP_ZOOM_IN);
            else
              gimp_gradient_editor_zoom (editor, GIMP_ZOOM_OUT);
          }
        else
          {
            GtkAdjustment *adj = editor->scroll_data;
            gfloat         new_value;

            new_value = (gtk_adjustment_get_value (adj) +
                         ((sevent->direction == GDK_SCROLL_UP) ?
                          - gtk_adjustment_get_page_increment (adj) / 2 :
                          gtk_adjustment_get_page_increment (adj) / 2));

            new_value = CLAMP (new_value,
                               gtk_adjustment_get_lower (adj),
                               gtk_adjustment_get_upper (adj) -
                               gtk_adjustment_get_page_size (adj));

            gtk_adjustment_set_value (adj, new_value);
          }
      }
      break;

    case GDK_BUTTON_RELEASE:
      {
        GdkEventButton *bevent = (GdkEventButton *) event;

        gradient_editor_set_hint (editor, NULL, NULL, NULL, NULL);

        if (editor->control_drag_mode != GRAD_DRAG_NONE)
          {
            if (GIMP_DATA_EDITOR (editor)->data_editable)
              {
                g_signal_handlers_unblock_by_func (gradient,
                                                   gimp_gradient_editor_gradient_dirty,
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

                gimp_gradient_editor_update (editor);
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

            if (GIMP_DATA_EDITOR (editor)->data_editable &&
                editor->control_drag_mode != GRAD_DRAG_NONE)
              {

                if ((mevent->time - editor->control_click_time) >= GRAD_MOVE_TIME)
                  control_motion (editor, gradient, mevent->x);
              }
            else
              {
                gimp_gradient_editor_update (editor);

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
              GimpGradientEditor *editor)
{
  GtkAdjustment *adj = editor->scroll_data;
  GtkAllocation  allocation;

  gtk_widget_get_allocation (widget, &allocation);

  control_draw_all (editor,
                    GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data),
                    cr,
                    allocation.width,
                    allocation.height,
                    gtk_adjustment_get_value (adj),
                    gtk_adjustment_get_value (adj) +
                    gtk_adjustment_get_page_size (adj));

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
                  str = g_strdup_printf (_("%s-Drag: move & compress"),
                                         gimp_get_mod_string (GDK_SHIFT_MASK));

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
                                         gimp_get_mod_string (GDK_SHIFT_MASK));

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
                                     gimp_get_mod_string (GDK_SHIFT_MASK));

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
                                 gimp_get_mod_string (GDK_SHIFT_MASK));

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
                              gimp_get_mod_string (GDK_SHIFT_MASK));
      str2 = g_strdup_printf (_("%s-Drag: move & compress"),
                              gimp_get_mod_string (GDK_SHIFT_MASK));

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

      str = g_strdup_printf (_("Handle position: %0.4f"), seg->left);
      break;

    case GRAD_DRAG_MIDDLE:
      pos = control_calc_g_pos (editor, x);

      gimp_gradient_segment_set_middle_pos (gradient, seg, pos);

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

  return gimp_gradient_segment_range_move (gradient,
                                           range_l,
                                           range_r,
                                           delta,
                                           editor->control_compress);
}

/*****/

static void
control_update (GimpGradientEditor *editor,
                GimpGradient       *gradient,
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
control_draw_all (GimpGradientEditor *editor,
                  GimpGradient       *gradient,
                  cairo_t            *cr,
                  gint                width,
                  gint                height,
                  gdouble             left,
                  gdouble             right)
{
  GtkStyleContext        *control_style;
  GimpGradientSegment    *seg;
  GradientEditorDragMode  handle;
  GdkRGBA                 color;
  gint                    sel_l;
  gint                    sel_r;
  gdouble                 g_pos;
  gboolean                selected;

  if (! gradient)
    return;

  /* Draw selection */

  control_style = gtk_widget_get_style_context (editor->control);

  sel_l = control_calc_p_pos (editor, editor->control_sel_l->left);
  sel_r = control_calc_p_pos (editor, editor->control_sel_r->right);

  gtk_style_context_add_class (control_style, GTK_STYLE_CLASS_ENTRY);

  gtk_style_context_get_background_color (control_style, 0,
                                          &color);
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_fill (cr);

  gtk_style_context_get_background_color (control_style, GTK_STATE_FLAG_SELECTED,
                                          &color);
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_rectangle (cr, sel_l, 0, sel_r - sel_l + 1, height);
  cairo_fill (cr);

  gtk_style_context_remove_class (control_style, GTK_STYLE_CLASS_ENTRY);

  /* Draw handles */

  selected = FALSE;

  for (seg = gradient->segments; seg; seg = seg->next)
    {
      if (seg == editor->control_sel_l)
        selected = TRUE;

      control_draw_normal_handle (editor, cr, seg->left,   height, selected);
      control_draw_middle_handle (editor, cr, seg->middle, height, selected);

      /* Draw right handle only if this is the last segment */
      if (seg->next == NULL)
        control_draw_normal_handle (editor, cr, seg->right, height, selected);

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
          control_draw_normal_handle (editor, cr, seg->left, height, selected);
        }
      else
        {
          seg = gimp_gradient_segment_get_last (gradient->segments);

          selected = (seg == editor->control_sel_r);

          control_draw_normal_handle (editor, cr, seg->right, height, selected);
        }

      break;

    case GRAD_DRAG_MIDDLE:
      control_draw_middle_handle (editor, cr, seg->middle, height, selected);
      break;

    default:
      break;
    }
}

static void
control_draw_normal_handle (GimpGradientEditor *editor,
                            cairo_t            *cr,
                            gdouble             pos,
                            gint                height,
                            gboolean            selected)
{
  GdkRGBA border;
  GdkRGBA fill = { 0.0, 0.0, 0.0, 1.0 };

  gimp_get_style_color (GTK_WIDGET (editor),
                        selected ? "handle-color-selected" : "handle-color",
                        &border);

  control_draw_handle (cr, &border, &fill,
                       control_calc_p_pos (editor, pos), height);
}

static void
control_draw_middle_handle (GimpGradientEditor *editor,
                            cairo_t            *cr,
                            gdouble             pos,
                            gint                height,
                            gboolean            selected)
{
  GdkRGBA border;
  GdkRGBA fill = { 1.0, 1.0, 1.0, 1.0 };

  gimp_get_style_color (GTK_WIDGET (editor),
                        selected ? "handle-color-selected" : "handle-color",
                        &border);

  control_draw_handle (cr, &border, &fill,
                       control_calc_p_pos (editor, pos), height);
}

static void
control_draw_handle (cairo_t        *cr,
                     const GdkRGBA *border,
                     const GdkRGBA *fill,
                     gint           xpos,
                     gint           height)
{
  cairo_move_to (cr, xpos, 0);
  cairo_line_to (cr, xpos - height / 2.0, height);
  cairo_line_to (cr, xpos + height / 2.0, height);
  cairo_line_to (cr, xpos, 0);

  gdk_cairo_set_source_rgba (cr, fill);
  cairo_fill_preserve (cr);

  gdk_cairo_set_source_rgba (cr, border);
  cairo_set_line_width (cr, 1);
  cairo_stroke (cr);
}

/*****/

static gint
control_calc_p_pos (GimpGradientEditor *editor,
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
control_calc_g_pos (GimpGradientEditor *editor,
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
