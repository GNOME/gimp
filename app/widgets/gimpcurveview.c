/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcurve.h"
#include "core/gimpcurve-map.h"
#include "core/gimpmarshal.h"

#include "gimpclipboard.h"
#include "gimpcurveview.h"


enum
{
  PROP_0,
  PROP_GIMP,
  PROP_BASE_LINE,
  PROP_GRID_ROWS,
  PROP_GRID_COLUMNS,
  PROP_X_AXIS_LABEL,
  PROP_Y_AXIS_LABEL
};

enum
{
  CUT_CLIPBOARD,
  COPY_CLIPBOARD,
  PASTE_CLIPBOARD,
  LAST_SIGNAL
};


typedef struct
{
  GimpCurve *curve;
  GimpRGB    color;
  gboolean   color_set;
} BGCurve;


static void       gimp_curve_view_finalize        (GObject          *object);
static void       gimp_curve_view_dispose         (GObject          *object);
static void       gimp_curve_view_set_property    (GObject          *object,
                                                   guint             property_id,
                                                   const GValue     *value,
                                                   GParamSpec       *pspec);
static void       gimp_curve_view_get_property    (GObject          *object,
                                                   guint             property_id,
                                                   GValue           *value,
                                                   GParamSpec       *pspec);

static void       gimp_curve_view_style_set       (GtkWidget        *widget,
                                                   GtkStyle         *prev_style);
static gboolean   gimp_curve_view_draw            (GtkWidget        *widget,
                                                   cairo_t          *cr);
static gboolean   gimp_curve_view_button_press    (GtkWidget        *widget,
                                                   GdkEventButton   *bevent);
static gboolean   gimp_curve_view_button_release  (GtkWidget        *widget,
                                                   GdkEventButton   *bevent);
static gboolean   gimp_curve_view_motion_notify   (GtkWidget        *widget,
                                                   GdkEventMotion   *bevent);
static gboolean   gimp_curve_view_leave_notify    (GtkWidget        *widget,
                                                   GdkEventCrossing *cevent);
static gboolean   gimp_curve_view_key_press       (GtkWidget        *widget,
                                                   GdkEventKey      *kevent);

static void       gimp_curve_view_cut_clipboard   (GimpCurveView    *view);
static void       gimp_curve_view_copy_clipboard  (GimpCurveView    *view);
static void       gimp_curve_view_paste_clipboard (GimpCurveView    *view);

static void       gimp_curve_view_set_cursor      (GimpCurveView    *view,
                                                   gdouble           x,
                                                   gdouble           y);
static void       gimp_curve_view_unset_cursor    (GimpCurveView *view);


G_DEFINE_TYPE (GimpCurveView, gimp_curve_view,
               GIMP_TYPE_HISTOGRAM_VIEW)

#define parent_class gimp_curve_view_parent_class

static guint curve_view_signals[LAST_SIGNAL] = { 0 };


static void
gimp_curve_view_class_init (GimpCurveViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkBindingSet  *binding_set;

  object_class->finalize             = gimp_curve_view_finalize;
  object_class->dispose              = gimp_curve_view_dispose;
  object_class->set_property         = gimp_curve_view_set_property;
  object_class->get_property         = gimp_curve_view_get_property;

  widget_class->style_set            = gimp_curve_view_style_set;
  widget_class->draw                 = gimp_curve_view_draw;
  widget_class->button_press_event   = gimp_curve_view_button_press;
  widget_class->button_release_event = gimp_curve_view_button_release;
  widget_class->motion_notify_event  = gimp_curve_view_motion_notify;
  widget_class->leave_notify_event   = gimp_curve_view_leave_notify;
  widget_class->key_press_event      = gimp_curve_view_key_press;

  klass->cut_clipboard               = gimp_curve_view_cut_clipboard;
  klass->copy_clipboard              = gimp_curve_view_copy_clipboard;
  klass->paste_clipboard             = gimp_curve_view_paste_clipboard;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_BASE_LINE,
                                   g_param_spec_boolean ("base-line",
                                                         NULL, NULL,
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_GRID_ROWS,
                                   g_param_spec_int ("grid-rows", NULL, NULL,
                                                     0, 100, 8,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_GRID_COLUMNS,
                                   g_param_spec_int ("grid-columns", NULL, NULL,
                                                     0, 100, 8,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_X_AXIS_LABEL,
                                   g_param_spec_string ("x-axis-label", NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_Y_AXIS_LABEL,
                                   g_param_spec_string ("y-axis-label", NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE));

  curve_view_signals[CUT_CLIPBOARD] =
    g_signal_new ("cut-clipboard",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GimpCurveViewClass, cut_clipboard),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  curve_view_signals[COPY_CLIPBOARD] =
    g_signal_new ("copy-clipboard",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GimpCurveViewClass, copy_clipboard),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  curve_view_signals[PASTE_CLIPBOARD] =
    g_signal_new ("paste-clipboard",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GimpCurveViewClass, paste_clipboard),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  binding_set = gtk_binding_set_by_class (klass);

  gtk_binding_entry_add_signal (binding_set, GDK_KEY_x, GDK_CONTROL_MASK,
                                "cut-clipboard", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_c, GDK_CONTROL_MASK,
                                "copy-clipboard", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_v, GDK_CONTROL_MASK,
                                "paste-clipboard", 0);
}

static void
gimp_curve_view_init (GimpCurveView *view)
{
  view->curve       = NULL;
  view->selected    = 0;
  view->last_x      = 0.0;
  view->last_y      = 0.0;
  view->cursor_type = -1;
  view->xpos        = -1.0;
  view->cursor_x    = -1.0;
  view->cursor_y    = -1.0;
  view->range_x_min = 0.0;
  view->range_x_max = 1.0;
  view->range_y_min = 0.0;
  view->range_y_max = 1.0;

  view->x_axis_label = NULL;
  view->y_axis_label = NULL;

  gtk_widget_set_can_focus (GTK_WIDGET (view), TRUE);
  gtk_widget_add_events (GTK_WIDGET (view),
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_BUTTON1_MOTION_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_KEY_PRESS_MASK      |
                         GDK_LEAVE_NOTIFY_MASK);
}

static void
gimp_curve_view_finalize (GObject *object)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (object);

  if (view->layout)
    {
      g_object_unref (view->layout);
      view->layout = NULL;
    }

  if (view->cursor_layout)
    {
      g_object_unref (view->cursor_layout);
      view->cursor_layout = NULL;
    }

  if (view->x_axis_label)
    {
      g_free (view->x_axis_label);
      view->x_axis_label = NULL;
    }

  if (view->y_axis_label)
    {
      g_free (view->y_axis_label);
      view->y_axis_label = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_curve_view_dispose (GObject *object)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (object);

  gimp_curve_view_set_curve (view, NULL, NULL);

  if (view->bg_curves)
    gimp_curve_view_remove_all_backgrounds (view);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_curve_view_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (object);

  switch (property_id)
    {
    case PROP_GIMP:
      view->gimp = g_value_get_object (value); /* don't ref */
      break;
    case PROP_GRID_ROWS:
      view->grid_rows = g_value_get_int (value);
      break;
    case PROP_GRID_COLUMNS:
      view->grid_columns = g_value_get_int (value);
      break;
    case PROP_BASE_LINE:
      view->draw_base_line = g_value_get_boolean (value);
      break;
    case PROP_X_AXIS_LABEL:
      gimp_curve_view_set_x_axis_label (view, g_value_get_string (value));
      break;
    case PROP_Y_AXIS_LABEL:
      gimp_curve_view_set_y_axis_label (view, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_curve_view_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, view->gimp);
      break;
    case PROP_GRID_ROWS:
      g_value_set_int (value, view->grid_rows);
      break;
    case PROP_GRID_COLUMNS:
      g_value_set_int (value, view->grid_columns);
      break;
    case PROP_BASE_LINE:
      g_value_set_boolean (value, view->draw_base_line);
      break;
    case PROP_X_AXIS_LABEL:
      g_value_set_string (value, view->x_axis_label);
      break;
    case PROP_Y_AXIS_LABEL:
      g_value_set_string (value, view->y_axis_label);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_curve_view_style_set (GtkWidget *widget,
                           GtkStyle  *prev_style)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (widget);

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  if (view->layout)
    {
      g_object_unref (view->layout);
      view->layout = NULL;
    }

  if (view->cursor_layout)
    {
      g_object_unref (view->cursor_layout);
      view->cursor_layout = NULL;
    }
}

static void
gimp_curve_view_draw_grid (GimpCurveView *view,
                           cairo_t       *cr,
                           gint           width,
                           gint           height,
                           gint           border)
{
  gint i;

  for (i = 1; i < view->grid_rows; i++)
    {
      gint y = i * (height - 1) / view->grid_rows;

      if ((view->grid_rows % 2) == 0 && (i == view->grid_rows / 2))
        continue;

      cairo_move_to (cr, border,             border + y);
      cairo_line_to (cr, border + width - 1, border + y);
    }

  for (i = 1; i < view->grid_columns; i++)
    {
      gint x = i * (width - 1) / view->grid_columns;

      if ((view->grid_columns % 2) == 0 && (i == view->grid_columns / 2))
        continue;

      cairo_move_to (cr, border + x, border);
      cairo_line_to (cr, border + x, border + height - 1);
    }

  if (view->draw_base_line)
    {
      cairo_move_to (cr, border, border + height - 1);
      cairo_line_to (cr, border + width - 1, border);
    }

  cairo_set_line_width (cr, 0.6);
  cairo_stroke (cr);

  if ((view->grid_rows % 2) == 0)
    {
      gint y = (height - 1) / 2;

      cairo_move_to (cr, border,             border + y);
      cairo_line_to (cr, border + width - 1, border + y);
    }

  if ((view->grid_columns % 2) == 0)
    {
      gint x = (width - 1) / 2;

      cairo_move_to (cr, border + x, border);
      cairo_line_to (cr, border + x, border + height - 1);
    }

  cairo_set_line_width (cr, 1.0);
  cairo_stroke (cr);
}

static void
gimp_curve_view_draw_point (GimpCurveView *view,
                            cairo_t       *cr,
                            gint           i,
                            gint           width,
                            gint           height,
                            gint           border)
{
  gdouble x, y;

  gimp_curve_get_point (view->curve, i, &x, &y);

  if (x < 0.0)
    return;

  y = 1.0 - y;

#define RADIUS 3

  cairo_move_to (cr,
                 border + (gdouble) (width  - 1) * x + RADIUS,
                 border + (gdouble) (height - 1) * y);
  cairo_arc (cr,
             border + (gdouble) (width  - 1) * x,
             border + (gdouble) (height - 1) * y,
             RADIUS,
             0, 2 * G_PI);
}

static void
gimp_curve_view_draw_curve (GimpCurveView *view,
                            cairo_t       *cr,
                            GimpCurve     *curve,
                            gint           width,
                            gint           height,
                            gint           border)
{
  gdouble x, y;
  gint    i;

  x = 0.0;
  y = 1.0 - gimp_curve_map_value (curve, 0.0);

  cairo_move_to (cr,
                 border + (gdouble) (width  - 1) * x,
                 border + (gdouble) (height - 1)* y);

  for (i = 1; i < 256; i++)
    {
      x = (gdouble) i / 255.0;
      y = 1.0 - gimp_curve_map_value (curve, x);

      cairo_line_to (cr,
                     border + (gdouble) (width  - 1) * x,
                     border + (gdouble) (height - 1) * y);
    }

  cairo_stroke (cr);
}

static gboolean
gimp_curve_view_draw (GtkWidget *widget,
                      cairo_t   *cr)
{
  GimpCurveView *view  = GIMP_CURVE_VIEW (widget);
  GtkStyle      *style = gtk_widget_get_style (widget);
  GtkAllocation  allocation;
  GList         *list;
  gint           border;
  gint           width;
  gint           height;
  gint           layout_x;
  gint           layout_y;
  gdouble        x, y;
  gint           i;

  GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);

  if (! view->curve)
    return FALSE;

  gtk_widget_get_allocation (widget, &allocation);

  border = GIMP_HISTOGRAM_VIEW (view)->border_width;
  width  = allocation.width  - 2 * border;
  height = allocation.height - 2 * border;

  if (gtk_widget_has_focus (widget))
    {
      gtk_paint_focus (style, cr,
                       gtk_widget_get_state (widget),
                       widget, NULL,
                       border - 2, border - 2,
                       width + 4, height + 4);
    }

  cairo_set_line_width (cr, 1.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
  cairo_translate (cr, 0.5, 0.5);

  /*  Draw the grid lines  */
  gdk_cairo_set_source_color (cr, &style->dark[GTK_STATE_NORMAL]);

  gimp_curve_view_draw_grid (view, cr, width, height, border);

  /*  Draw the axis labels  */

  if (view->x_axis_label)
    {
      if (! view->layout)
        view->layout = gtk_widget_create_pango_layout (widget, NULL);

      pango_layout_set_text (view->layout, view->x_axis_label, -1);
      pango_layout_get_pixel_size (view->layout, &layout_x, &layout_y);

      cairo_move_to (cr,
                     width - border - layout_x,
                     height - border - layout_y);

      gdk_cairo_set_source_color (cr, &style->text[GTK_STATE_NORMAL]);
      pango_cairo_show_layout (cr, view->layout);
    }

  if (view->y_axis_label)
    {
      if (! view->layout)
        view->layout = gtk_widget_create_pango_layout (widget, NULL);

      pango_layout_set_text (view->layout, view->y_axis_label, -1);
      pango_layout_get_pixel_size (view->layout, &layout_x, &layout_y);

      cairo_save (cr);

      cairo_move_to (cr,
                     2 * border,
                     2 * border + layout_x);
      cairo_rotate (cr, - G_PI / 2);

      gdk_cairo_set_source_color (cr, &style->text[GTK_STATE_NORMAL]);
      pango_cairo_show_layout (cr, view->layout);

      cairo_restore (cr);
    }


  /*  Draw the background curves  */
  for (list = view->bg_curves; list; list = g_list_next (list))
    {
      BGCurve *bg = list->data;

      if (bg->color_set)
        {
          cairo_set_source_rgba (cr,
                                 bg->color.r,
                                 bg->color.g,
                                 bg->color.b,
                                 0.5);
        }
      else
        {
          cairo_set_source_rgba (cr,
                                 style->text[GTK_STATE_NORMAL].red / 65535.0,
                                 style->text[GTK_STATE_NORMAL].green / 65535.0,
                                 style->text[GTK_STATE_NORMAL].blue / 65535.0,
                                 0.5);
        }

      gimp_curve_view_draw_curve (view, cr, bg->curve,
                                  width, height, border);
    }

  /*  Draw the curve  */
  if (view->curve_color)
    gimp_cairo_set_source_rgb (cr, view->curve_color);
  else
    gdk_cairo_set_source_color (cr, &style->text[GTK_STATE_NORMAL]);

  gimp_curve_view_draw_curve (view, cr, view->curve,
                              width, height, border);

  /*  Draw the points  */
  if (gimp_curve_get_curve_type (view->curve) == GIMP_CURVE_SMOOTH)
    {
      gdk_cairo_set_source_color (cr, &style->text[GTK_STATE_NORMAL]);

      /*  Draw the unselected points  */
      for (i = 0; i < view->curve->n_points; i++)
        {
          if (i == view->selected)
            continue;

          gimp_curve_view_draw_point (view, cr, i, width, height, border);
        }

      cairo_stroke (cr);

      /*  Draw the selected point  */
      if (view->selected != -1)
        {
          gimp_curve_view_draw_point (view, cr, view->selected,
                                      width, height, border);
          cairo_fill (cr);
       }
    }

  if (view->xpos >= 0.0)
    {
      gchar buf[32];

      gdk_cairo_set_source_color (cr, &style->text[GTK_STATE_NORMAL]);

      /* draw the color line */
      cairo_move_to (cr,
                     border + ROUND ((gdouble) (width - 1) * view->xpos),
                     border + 1);
      cairo_line_to (cr,
                     border + ROUND ((gdouble) (width - 1) * view->xpos),
                     border + height - 1);
      cairo_stroke (cr);

      if (view->range_x_max == 255.0)
        {
          /*  stupid heuristic: special-case for 0..255  */

          g_snprintf (buf, sizeof (buf), "x:%3d",
                      (gint) (view->xpos *
                              (view->range_x_max - view->range_x_min) +
                              view->range_x_min));
        }
      else if (view->range_x_max == 100.0)
        {
          /*  and for 0..100  */

          g_snprintf (buf, sizeof (buf), "x:%0.2f",
                      view->xpos *
                      (view->range_x_max - view->range_x_min) +
                      view->range_x_min);
        }
      else
        {
          g_snprintf (buf, sizeof (buf), "x:%0.3f",
                      view->xpos *
                      (view->range_x_max - view->range_x_min) +
                      view->range_x_min);
        }

      if (! view->layout)
        view->layout = gtk_widget_create_pango_layout (widget, NULL);

      pango_layout_set_text (view->layout, buf, -1);
      pango_layout_get_pixel_size (view->layout, &layout_x, &layout_y);

      if (view->xpos < 0.5)
        layout_x = border;
      else
        layout_x = -(layout_x + border);

      cairo_move_to (cr,
                     border + (gdouble) width * view->xpos + layout_x,
                     border + height - border - layout_y);
      pango_cairo_show_layout (cr, view->layout);
    }

  if (view->cursor_x >= 0.0 && view->cursor_x <= 1.0 &&
      view->cursor_y >= 0.0 && view->cursor_y <= 1.0)
    {
      gchar  buf[32];
      gint   w, h;

      if (! view->cursor_layout)
        view->cursor_layout = gtk_widget_create_pango_layout (widget, NULL);

      if (view->range_x_max == 255.0 &&
          view->range_y_max == 255.0)
        {
          /*  stupid heuristic: special-case for 0..255  */

          g_snprintf (buf, sizeof (buf), "x:%3d y:%3d",
                      (gint) (view->cursor_x *
                              (view->range_x_max - view->range_x_min) +
                              view->range_x_min),
                      (gint) ((1.0 - view->cursor_y) *
                              (view->range_y_max - view->range_y_min) +
                              view->range_y_min));
        }
      else if (view->range_x_max == 100.0 &&
               view->range_y_max == 100.0)
        {
          /*  and for 0..100  */

          g_snprintf (buf, sizeof (buf), "x:%0.2f y:%0.2f",
                      view->cursor_x *
                      (view->range_x_max - view->range_x_min) +
                      view->range_x_min,
                      (1.0 - view->cursor_y) *
                      (view->range_y_max - view->range_y_min) +
                      view->range_y_min);
        }
      else
        {
          g_snprintf (buf, sizeof (buf), "x:%0.3f y:%0.3f",
                      view->cursor_x *
                      (view->range_x_max - view->range_x_min) +
                      view->range_x_min,
                      (1.0 - view->cursor_y) *
                      (view->range_y_max - view->range_y_min) +
                      view->range_y_min);
        }

      pango_layout_set_text (view->cursor_layout, buf, -1);
      pango_layout_get_pixel_extents (view->cursor_layout,
                                      NULL, &view->cursor_rect);

      x = border * 2 + 3;
      y = border * 2 + 3;
      w = view->cursor_rect.width;
      h = view->cursor_rect.height;

      if (view->x_axis_label)
        x += border + view->cursor_rect.height; /* coincidentially the right value */

      cairo_push_group (cr);

      gdk_cairo_set_source_color (cr, &style->text[GTK_STATE_NORMAL]);
      cairo_rectangle (cr, x + 0.5, y + 0.5, w, h);
      cairo_fill_preserve (cr);

      cairo_set_line_width (cr, 6);
      cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
      cairo_stroke (cr);

      gdk_cairo_set_source_color (cr, &style->base[GTK_STATE_NORMAL]);
      cairo_move_to (cr, x, y);
      pango_cairo_show_layout (cr, view->cursor_layout);

      cairo_pop_group_to_source (cr);
      cairo_paint_with_alpha (cr, 0.6);
    }

  return FALSE;
}

static void
set_cursor (GimpCurveView *view,
            GdkCursorType  new_cursor)
{
  if (new_cursor != view->cursor_type)
    {
      GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (view));
      GdkCursor  *cursor  = gdk_cursor_new_for_display (display, new_cursor);

      gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (view)), cursor);
      gdk_cursor_unref (cursor);

      view->cursor_type = new_cursor;
    }
}

static gboolean
gimp_curve_view_button_press (GtkWidget      *widget,
                              GdkEventButton *bevent)
{
  GimpCurveView *view  = GIMP_CURVE_VIEW (widget);
  GimpCurve     *curve = view->curve;
  GtkAllocation  allocation;
  gint           border;
  gint           width, height;
  gdouble        x;
  gdouble        y;
  gint           closest_point;
  gint           i;

  if (! curve || bevent->button != 1)
    return TRUE;

  gtk_widget_get_allocation (widget, &allocation);

  border = GIMP_HISTOGRAM_VIEW (view)->border_width;
  width  = allocation.width  - 2 * border;
  height = allocation.height - 2 * border;

  x = (gdouble) (bevent->x - border) / (gdouble) width;
  y = (gdouble) (bevent->y - border) / (gdouble) height;

  x = CLAMP (x, 0.0, 1.0);
  y = CLAMP (y, 0.0, 1.0);

  closest_point = gimp_curve_get_closest_point (curve, x);

  view->grabbed = TRUE;

  set_cursor (view, GDK_TCROSS);

  switch (gimp_curve_get_curve_type (curve))
    {
    case GIMP_CURVE_SMOOTH:
      /*  determine the leftmost and rightmost points  */
      view->leftmost = -1.0;
      for (i = closest_point - 1; i >= 0; i--)
        {
          gdouble point_x;

          gimp_curve_get_point (curve, i, &point_x, NULL);

          if (point_x >= 0.0)
            {
              view->leftmost = point_x;
              break;
            }
        }

      view->rightmost = 2.0;
      for (i = closest_point + 1; i < curve->n_points; i++)
        {
          gdouble point_x;

          gimp_curve_get_point (curve, i, &point_x, NULL);

          if (point_x >= 0.0)
            {
              view->rightmost = point_x;
              break;
            }
        }

      gimp_curve_view_set_selected (view, closest_point);

      gimp_curve_set_point (curve, view->selected, x, 1.0 - y);
      break;

    case GIMP_CURVE_FREE:
      view->last_x = x;
      view->last_y = y;

      gimp_curve_set_curve (curve, x, 1.0 - y);
      break;
    }

  if (! gtk_widget_has_focus (widget))
    gtk_widget_grab_focus (widget);

  return TRUE;
}

static gboolean
gimp_curve_view_button_release (GtkWidget      *widget,
                                GdkEventButton *bevent)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (widget);

  if (bevent->button != 1)
    return TRUE;

  view->grabbed = FALSE;

  set_cursor (view, GDK_FLEUR);

  return TRUE;
}

static gboolean
gimp_curve_view_motion_notify (GtkWidget      *widget,
                               GdkEventMotion *mevent)
{
  GimpCurveView  *view       = GIMP_CURVE_VIEW (widget);
  GimpCurve      *curve      = view->curve;
  GtkAllocation   allocation;
  GdkCursorType   new_cursor = GDK_X_CURSOR;
  gint            border;
  gint            width, height;
  gdouble         x;
  gdouble         y;
  gdouble         point_x;
  gint            closest_point;

  if (! curve)
    return TRUE;

  gtk_widget_get_allocation (widget, &allocation);

  border = GIMP_HISTOGRAM_VIEW (view)->border_width;
  width  = allocation.width  - 2 * border;
  height = allocation.height - 2 * border;

  x = (gdouble) (mevent->x - border) / (gdouble) width;
  y = (gdouble) (mevent->y - border) / (gdouble) height;

  x = CLAMP (x, 0.0, 1.0);
  y = CLAMP (y, 0.0, 1.0);

  closest_point = gimp_curve_get_closest_point (curve, x);

  switch (gimp_curve_get_curve_type (curve))
    {
    case GIMP_CURVE_SMOOTH:
      if (! view->grabbed) /*  If no point is grabbed...  */
        {
          gimp_curve_get_point (curve, closest_point, &point_x, NULL);

          if (point_x >= 0.0)
            new_cursor = GDK_FLEUR;
          else
            new_cursor = GDK_TCROSS;
        }
      else /*  Else, drag the grabbed point  */
        {
          new_cursor = GDK_TCROSS;

          gimp_data_freeze (GIMP_DATA (curve));

          gimp_curve_set_point (curve, view->selected, -1.0, -1.0);

          if (x > view->leftmost && x < view->rightmost)
            {
              gint n_points = gimp_curve_get_n_points (curve);

              closest_point = ROUND (x * (gdouble) (n_points - 1));

              gimp_curve_get_point (curve, closest_point, &point_x, NULL);

              if (point_x < 0.0)
                gimp_curve_view_set_selected (view, closest_point);

              gimp_curve_set_point (curve, view->selected, x, 1.0 - y);
            }

          gimp_data_thaw (GIMP_DATA (curve));
        }
      break;

    case GIMP_CURVE_FREE:
      if (view->grabbed)
        {
          gint    n_samples = gimp_curve_get_n_samples (curve);
          gdouble x1, x2;
          gdouble y1, y2;

          if (view->last_x > x)
            {
              x1 = x;
              x2 = view->last_x;
              y1 = y;
              y2 = view->last_y;
            }
          else
            {
              x1 = view->last_x;
              x2 = x;
              y1 = view->last_y;
              y2 = y;
            }

          if (x2 != x1)
            {
              gint from = ROUND (x1 * (gdouble) (n_samples - 1));
              gint to   = ROUND (x2 * (gdouble) (n_samples - 1));
              gint i;

              gimp_data_freeze (GIMP_DATA (curve));

              for (i = from; i <= to; i++)
                {
                  gdouble xpos = (gdouble) i / (gdouble) (n_samples - 1);
                  gdouble ypos = (y1 + ((y2 - y1) * (xpos - x1)) / (x2 - x1));

                  xpos = CLAMP (xpos, 0.0, 1.0);
                  ypos = CLAMP (ypos, 0.0, 1.0);

                  gimp_curve_set_curve (curve, xpos, 1.0 - ypos);
                }

              gimp_data_thaw (GIMP_DATA (curve));
            }
          else
            {
              gimp_curve_set_curve (curve, x, 1.0 - y);
            }

          view->last_x = x;
          view->last_y = y;
        }

      if (mevent->state & GDK_BUTTON1_MASK)
        new_cursor = GDK_TCROSS;
      else
        new_cursor = GDK_PENCIL;

      break;
    }

  set_cursor (view, new_cursor);

  gimp_curve_view_set_cursor (view, x, y);

  return TRUE;
}

static gboolean
gimp_curve_view_leave_notify (GtkWidget        *widget,
                              GdkEventCrossing *cevent)
{
  GimpCurveView *view = GIMP_CURVE_VIEW (widget);

  gimp_curve_view_unset_cursor (view);

  return TRUE;
}

static gboolean
gimp_curve_view_key_press (GtkWidget   *widget,
                           GdkEventKey *kevent)
{
  GimpCurveView *view    = GIMP_CURVE_VIEW (widget);
  GimpCurve     *curve   = view->curve;
  gboolean       handled = FALSE;

  if (! view->grabbed && curve &&
      gimp_curve_get_curve_type (curve) == GIMP_CURVE_SMOOTH)
    {
      gint    i = view->selected;
      gdouble x, y;

      gimp_curve_get_point (curve, i, NULL, &y);

      switch (kevent->keyval)
        {
        case GDK_KEY_Left:
          for (i = i - 1; i >= 0 && ! handled; i--)
            {
              gimp_curve_get_point (curve, i, &x, NULL);

              if (x >= 0.0)
                {
                  gimp_curve_view_set_selected (view, i);

                  handled = TRUE;
                }
            }
          break;

        case GDK_KEY_Right:
          for (i = i + 1; i < curve->n_points && ! handled; i++)
            {
              gimp_curve_get_point (curve, i, &x, NULL);

              if (x >= 0.0)
                {
                  gimp_curve_view_set_selected (view, i);

                  handled = TRUE;
                }
            }
          break;

        case GDK_KEY_Up:
          if (y < 1.0)
            {
              y = y + (kevent->state & GDK_SHIFT_MASK ?
                       (16.0 / 255.0) : (1.0 / 255.0));

              gimp_curve_move_point (curve, i, CLAMP (y, 0.0, 1.0));

              handled = TRUE;
            }
          break;

        case GDK_KEY_Down:
          if (y > 0)
            {
              y = y - (kevent->state & GDK_SHIFT_MASK ?
                       (16.0 / 255.0) : (1.0 / 255.0));

              gimp_curve_move_point (curve, i, CLAMP (y, 0.0, 1.0));

              handled = TRUE;
            }
          break;

        case GDK_KEY_Delete:
          gimp_curve_delete_point (curve, i);
          break;

        default:
          break;
        }
    }

  if (handled)
    {
      set_cursor (view, GDK_TCROSS);

      return TRUE;
    }

  return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, kevent);
}

static void
gimp_curve_view_cut_clipboard (GimpCurveView *view)
{
  g_printerr ("%s\n", G_STRFUNC);

  if (! view->curve || ! view->gimp)
    {
      gtk_widget_error_bell (GTK_WIDGET (view));
      return;
    }

  gimp_curve_view_copy_clipboard (view);

  gimp_curve_reset (view->curve, FALSE);
}

static void
gimp_curve_view_copy_clipboard (GimpCurveView *view)
{
  GimpCurve *copy;

  g_printerr ("%s\n", G_STRFUNC);

  if (! view->curve || ! view->gimp)
    {
      gtk_widget_error_bell (GTK_WIDGET (view));
      return;
    }

  copy = GIMP_CURVE (gimp_data_duplicate (GIMP_DATA (view->curve)));
  gimp_clipboard_set_curve (view->gimp, copy);
  g_object_unref (copy);
}

static void
gimp_curve_view_paste_clipboard (GimpCurveView *view)
{
  GimpCurve *copy;

  g_printerr ("%s\n", G_STRFUNC);

  if (! view->curve || ! view->gimp)
    {
      gtk_widget_error_bell (GTK_WIDGET (view));
      return;
    }

  copy = gimp_clipboard_get_curve (view->gimp);

  if (copy)
    {
      gimp_config_copy (GIMP_CONFIG (copy),
                        GIMP_CONFIG (view->curve), 0);
      g_object_unref (copy);
    }
}


/*  public functions  */

GtkWidget *
gimp_curve_view_new (void)
{
  return g_object_new (GIMP_TYPE_CURVE_VIEW, NULL);
}

static void
gimp_curve_view_curve_dirty (GimpCurve     *curve,
                             GimpCurveView *view)
{
  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
gimp_curve_view_set_curve (GimpCurveView *view,
                           GimpCurve     *curve,
                           const GimpRGB *color)
{
  g_return_if_fail (GIMP_IS_CURVE_VIEW (view));
  g_return_if_fail (curve == NULL || GIMP_IS_CURVE (curve));

  if (view->curve == curve)
    return;

  if (view->curve)
    {
      g_signal_handlers_disconnect_by_func (view->curve,
                                            gimp_curve_view_curve_dirty,
                                            view);
      g_object_unref (view->curve);
    }

  view->curve = curve;

  if (view->curve)
    {
      g_object_ref (view->curve);
      g_signal_connect (view->curve, "dirty",
                        G_CALLBACK (gimp_curve_view_curve_dirty),
                        view);
    }

  if (view->curve_color)
    g_free (view->curve_color);

  if (color)
    view->curve_color = g_memdup (color, sizeof (GimpRGB));
  else
    view->curve_color = NULL;

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

GimpCurve *
gimp_curve_view_get_curve (GimpCurveView *view)
{
  g_return_val_if_fail (GIMP_IS_CURVE_VIEW (view), NULL);

  return view->curve;
}

void
gimp_curve_view_add_background (GimpCurveView *view,
                                GimpCurve     *curve,
                                const GimpRGB *color)
{
  GList   *list;
  BGCurve *bg;

  g_return_if_fail (GIMP_IS_CURVE_VIEW (view));
  g_return_if_fail (GIMP_IS_CURVE (curve));

  for (list = view->bg_curves; list; list = g_list_next (list))
    {
      bg = list->data;

      g_return_if_fail (curve != bg->curve);
    }

  bg = g_slice_new0 (BGCurve);

  bg->curve = g_object_ref (curve);

  if (color)
    {
      bg->color     = *color;
      bg->color_set = TRUE;
    }

  g_signal_connect (bg->curve, "dirty",
                    G_CALLBACK (gimp_curve_view_curve_dirty),
                    view);

  view->bg_curves = g_list_append (view->bg_curves, bg);

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
gimp_curve_view_remove_background (GimpCurveView *view,
                                   GimpCurve     *curve)
{
  GList *list;

  g_return_if_fail (GIMP_IS_CURVE_VIEW (view));
  g_return_if_fail (GIMP_IS_CURVE (curve));

  for (list = view->bg_curves; list; list = g_list_next (list))
    {
      BGCurve *bg = list->data;

      if (bg->curve == curve)
        {
          g_signal_handlers_disconnect_by_func (bg->curve,
                                                gimp_curve_view_curve_dirty,
                                                view);
          g_object_unref (bg->curve);

          view->bg_curves = g_list_remove (view->bg_curves, bg);

          g_slice_free (BGCurve, bg);

          gtk_widget_queue_draw (GTK_WIDGET (view));

          break;
        }
    }

  if (! list)
    g_return_if_reached ();
}

void
gimp_curve_view_remove_all_backgrounds (GimpCurveView *view)
{
  g_return_if_fail (GIMP_IS_CURVE_VIEW (view));

  while (view->bg_curves)
    {
      BGCurve *bg = view->bg_curves->data;

      g_signal_handlers_disconnect_by_func (bg->curve,
                                            gimp_curve_view_curve_dirty,
                                            view);
      g_object_unref (bg->curve);

      view->bg_curves = g_list_remove (view->bg_curves, bg);

      g_slice_free (BGCurve, bg);
    }

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
gimp_curve_view_set_selected (GimpCurveView *view,
                              gint           selected)
{
  g_return_if_fail (GIMP_IS_CURVE_VIEW (view));

  view->selected = selected;

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
gimp_curve_view_set_range_x (GimpCurveView *view,
                             gdouble        min,
                             gdouble        max)
{
  g_return_if_fail (GIMP_IS_CURVE_VIEW (view));

  view->range_x_min = min;
  view->range_x_max = max;

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
gimp_curve_view_set_range_y (GimpCurveView *view,
                             gdouble        min,
                             gdouble        max)
{
  g_return_if_fail (GIMP_IS_CURVE_VIEW (view));

  view->range_y_min = min;
  view->range_y_max = max;

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
gimp_curve_view_set_xpos (GimpCurveView *view,
                          gdouble        x)
{
  g_return_if_fail (GIMP_IS_CURVE_VIEW (view));

  view->xpos = x;

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
gimp_curve_view_set_x_axis_label (GimpCurveView *view,
                                  const gchar   *label)
{
  g_return_if_fail (GIMP_IS_CURVE_VIEW (view));

  if (view->x_axis_label)
    g_free (view->x_axis_label);

  view->x_axis_label = g_strdup (label);

  g_object_notify (G_OBJECT (view), "x-axis-label");

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
gimp_curve_view_set_y_axis_label (GimpCurveView *view,
                                  const gchar   *label)
{
  g_return_if_fail (GIMP_IS_CURVE_VIEW (view));

  if (view->y_axis_label)
    g_free (view->y_axis_label);

  view->y_axis_label = g_strdup (label);

  g_object_notify (G_OBJECT (view), "y-axis-label");

  gtk_widget_queue_draw (GTK_WIDGET (view));
}


/*  private functions  */

static void
gimp_curve_view_set_cursor (GimpCurveView *view,
                            gdouble        x,
                            gdouble        y)
{
  view->cursor_x = x;
  view->cursor_y = y;

  /* TODO: only invalidate the cursor label area */
  gtk_widget_queue_draw (GTK_WIDGET (view));
}

static void
gimp_curve_view_unset_cursor (GimpCurveView *view)
{
  view->cursor_x = -1.0;
  view->cursor_y = -1.0;

  /* TODO: only invalidate the cursor label area */
  gtk_widget_queue_draw (GTK_WIDGET (view));
}
