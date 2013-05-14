/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpspinscale.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpspinscale.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_LABEL
};

typedef enum
{
  TARGET_NUMBER,
  TARGET_UPPER,
  TARGET_LOWER
} SpinScaleTarget;


typedef struct _GimpSpinScalePrivate GimpSpinScalePrivate;

struct _GimpSpinScalePrivate
{
  gchar       *label;

  gboolean     scale_limits_set;
  gdouble      scale_lower;
  gdouble      scale_upper;

  PangoLayout *layout;
  gboolean     changing_value;
  gboolean     relative_change;
  gdouble      start_x;
  gdouble      start_value;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                                       GIMP_TYPE_SPIN_SCALE, \
                                                       GimpSpinScalePrivate))


static void       gimp_spin_scale_dispose        (GObject          *object);
static void       gimp_spin_scale_finalize       (GObject          *object);
static void       gimp_spin_scale_set_property   (GObject          *object,
                                                  guint             property_id,
                                                  const GValue     *value,
                                                  GParamSpec       *pspec);
static void       gimp_spin_scale_get_property   (GObject          *object,
                                                  guint             property_id,
                                                  GValue           *value,
                                                  GParamSpec       *pspec);

static void       gimp_spin_scale_size_request   (GtkWidget        *widget,
                                                  GtkRequisition   *requisition);
static void       gimp_spin_scale_style_set      (GtkWidget        *widget,
                                                  GtkStyle         *prev_style);
static gboolean   gimp_spin_scale_expose         (GtkWidget        *widget,
                                                  GdkEventExpose   *event);
static gboolean   gimp_spin_scale_button_press   (GtkWidget        *widget,
                                                  GdkEventButton   *event);
static gboolean   gimp_spin_scale_button_release (GtkWidget        *widget,
                                                  GdkEventButton   *event);
static gboolean   gimp_spin_scale_motion_notify  (GtkWidget        *widget,
                                                  GdkEventMotion   *event);
static gboolean   gimp_spin_scale_leave_notify   (GtkWidget        *widget,
                                                  GdkEventCrossing *event);

static void       gimp_spin_scale_value_changed  (GtkSpinButton    *spin_button);


G_DEFINE_TYPE (GimpSpinScale, gimp_spin_scale, GTK_TYPE_SPIN_BUTTON);

#define parent_class gimp_spin_scale_parent_class


static void
gimp_spin_scale_class_init (GimpSpinScaleClass *klass)
{
  GObjectClass       *object_class      = G_OBJECT_CLASS (klass);
  GtkWidgetClass     *widget_class      = GTK_WIDGET_CLASS (klass);
  GtkSpinButtonClass *spin_button_class = GTK_SPIN_BUTTON_CLASS (klass);

  object_class->dispose              = gimp_spin_scale_dispose;
  object_class->finalize             = gimp_spin_scale_finalize;
  object_class->set_property         = gimp_spin_scale_set_property;
  object_class->get_property         = gimp_spin_scale_get_property;

  widget_class->size_request         = gimp_spin_scale_size_request;
  widget_class->style_set            = gimp_spin_scale_style_set;
  widget_class->expose_event         = gimp_spin_scale_expose;
  widget_class->button_press_event   = gimp_spin_scale_button_press;
  widget_class->button_release_event = gimp_spin_scale_button_release;
  widget_class->motion_notify_event  = gimp_spin_scale_motion_notify;
  widget_class->leave_notify_event   = gimp_spin_scale_leave_notify;

  spin_button_class->value_changed   = gimp_spin_scale_value_changed;

  g_object_class_install_property (object_class, PROP_LABEL,
                                   g_param_spec_string ("label", NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (GimpSpinScalePrivate));
}

static void
gimp_spin_scale_init (GimpSpinScale *scale)
{
  gtk_widget_add_events (GTK_WIDGET (scale),
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_BUTTON1_MOTION_MASK |
                         GDK_LEAVE_NOTIFY_MASK);

  gtk_entry_set_alignment (GTK_ENTRY (scale), 1.0);
  gtk_entry_set_has_frame (GTK_ENTRY (scale), FALSE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (scale), TRUE);
}

static void
gimp_spin_scale_dispose (GObject *object)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (object);

  if (private->layout)
    {
      g_object_unref (private->layout);
      private->layout = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_spin_scale_finalize (GObject *object)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (object);

  if (private->label)
    {
      g_free (private->label);
      private->label = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_spin_scale_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_LABEL:
      g_free (private->label);
      private->label = g_value_dup_string (value);
      if (private->layout)
        {
          g_object_unref (private->layout);
          private->layout = NULL;
        }
      gtk_widget_queue_resize (GTK_WIDGET (object));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_spin_scale_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, private->label);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_spin_scale_size_request (GtkWidget      *widget,
                              GtkRequisition *requisition)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);
  GtkStyle             *style   = gtk_widget_get_style (widget);
  PangoContext         *context = gtk_widget_get_pango_context (widget);
  PangoFontMetrics     *metrics;
  gint                  height;

  GTK_WIDGET_CLASS (parent_class)->size_request (widget, requisition);

  metrics = pango_context_get_metrics (context, style->font_desc,
                                       pango_context_get_language (context));

  height = PANGO_PIXELS (pango_font_metrics_get_ascent (metrics) +
                         pango_font_metrics_get_descent (metrics));

  requisition->height += height;

  if (private->label)
    {
      gint char_width;
      gint digit_width;
      gint char_pixels;

      char_width = pango_font_metrics_get_approximate_char_width (metrics);
      digit_width = pango_font_metrics_get_approximate_digit_width (metrics);
      char_pixels = PANGO_PIXELS (MAX (char_width, digit_width));

      /* ~3 chars for the ellipses */
      requisition->width += char_pixels * 3;
    }

  pango_font_metrics_unref (metrics);
}

static void
gimp_spin_scale_style_set (GtkWidget *widget,
                           GtkStyle  *prev_style)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  if (private->layout)
    {
      g_object_unref (private->layout);
      private->layout = NULL;
    }
}

static gboolean
gimp_spin_scale_expose (GtkWidget      *widget,
                        GdkEventExpose *event)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);
  GtkStyle             *style   = gtk_widget_get_style (widget);
  cairo_t              *cr;
  gboolean              rtl;
  gint                  w, h;

  GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);

  cr = gdk_cairo_create (event->window);
  gdk_cairo_region (cr, event->region);
  cairo_clip (cr);

  rtl = (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL);

  w = gdk_window_get_width (event->window);
  h = gdk_window_get_height (event->window);

  cairo_set_line_width (cr, 1.0);

  if (event->window == gtk_entry_get_text_window (GTK_ENTRY (widget)))
    {
      /* let spinbutton-side line of rectangle disappear */
      if (rtl)
        cairo_rectangle (cr, -0.5, 0.5, w, h - 1.0);
      else
        cairo_rectangle (cr, 0.5, 0.5, w, h - 1.0);

      gdk_cairo_set_source_color (cr,
                                  &style->text[gtk_widget_get_state (widget)]);
      cairo_stroke (cr);
    }
  else
    {
      /* let text-box-side line of rectangle disappear */
      if (rtl)
        cairo_rectangle (cr, 0.5, 0.5, w, h - 1.0);
      else
        cairo_rectangle (cr, -0.5, 0.5, w, h - 1.0);

      gdk_cairo_set_source_color (cr,
                                  &style->text[gtk_widget_get_state (widget)]);
      cairo_stroke (cr);

      if (rtl)
        cairo_rectangle (cr, 1.5, 1.5, w - 2.0, h - 3.0);
      else
        cairo_rectangle (cr, 0.5, 1.5, w - 2.0, h - 3.0);

      gdk_cairo_set_source_color (cr,
                                  &style->base[gtk_widget_get_state (widget)]);
      cairo_stroke (cr);
    }

  if (private->label &&
      gtk_widget_is_drawable (widget) &&
      event->window == gtk_entry_get_text_window (GTK_ENTRY (widget)))
    {
      GtkRequisition requisition;
      GtkAllocation  allocation;
      PangoRectangle logical;
      gint           layout_offset_x;
      gint           layout_offset_y;
      GtkStateType   state;
      GdkColor       text_color;
      GdkColor       bar_text_color;
      gint           window_width;
      gint           window_height;
      gdouble        progress_fraction;
      gint           progress_x;
      gint           progress_y;
      gint           progress_width;
      gint           progress_height;

      GTK_WIDGET_CLASS (parent_class)->size_request (widget, &requisition);
      gtk_widget_get_allocation (widget, &allocation);

      if (! private->layout)
        {
          private->layout = gtk_widget_create_pango_layout (widget,
                                                            private->label);
          pango_layout_set_ellipsize (private->layout, PANGO_ELLIPSIZE_END);
        }

      pango_layout_set_width (private->layout,
                              PANGO_SCALE *
                              (allocation.width - requisition.width));
      pango_layout_get_pixel_extents (private->layout, NULL, &logical);

      gtk_entry_get_layout_offsets (GTK_ENTRY (widget), NULL, &layout_offset_y);

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        layout_offset_x = w - logical.width - 2;
      else
        layout_offset_x = 2;

      layout_offset_x -= logical.x;

      state = GTK_STATE_SELECTED;
      if (! gtk_widget_get_sensitive (widget))
        state = GTK_STATE_INSENSITIVE;
      text_color     = style->text[gtk_widget_get_state (widget)];
      bar_text_color = style->fg[state];

      window_width  = gdk_window_get_width  (event->window);
      window_height = gdk_window_get_height (event->window);

      progress_fraction = gtk_entry_get_progress_fraction (GTK_ENTRY (widget));

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        {
          progress_fraction = 1.0 - progress_fraction;

          progress_x      = window_width * progress_fraction;
          progress_y      = 0;
          progress_width  = window_width - progress_x;
          progress_height = window_height;
        }
      else
        {
          progress_x      = 0;
          progress_y      = 0;
          progress_width  = window_width * progress_fraction;
          progress_height = window_height;
        }

      cairo_save (cr);

      cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
      cairo_rectangle (cr, 0, 0, window_width, window_height);
      cairo_rectangle (cr, progress_x, progress_y,
                       progress_width, progress_height);
      cairo_clip (cr);
      cairo_set_fill_rule (cr, CAIRO_FILL_RULE_WINDING);

      cairo_move_to (cr, layout_offset_x, layout_offset_y);
      gdk_cairo_set_source_color (cr, &text_color);
      pango_cairo_show_layout (cr, private->layout);

      cairo_restore (cr);

      cairo_rectangle (cr, progress_x, progress_y,
                       progress_width, progress_height);
      cairo_clip (cr);

      cairo_move_to (cr, layout_offset_x, layout_offset_y);
      gdk_cairo_set_source_color (cr, &bar_text_color);
      pango_cairo_show_layout (cr, private->layout);
    }

  cairo_destroy (cr);

  return FALSE;
}

static SpinScaleTarget
gimp_spin_scale_get_target (GtkWidget *widget,
                            gdouble    x,
                            gdouble    y)
{
  GtkAllocation   allocation;
  PangoRectangle  logical;
  gint            layout_x;
  gint            layout_y;

  gtk_widget_get_allocation (widget, &allocation);
  gtk_entry_get_layout_offsets (GTK_ENTRY (widget), &layout_x, &layout_y);
  pango_layout_get_pixel_extents (gtk_entry_get_layout (GTK_ENTRY (widget)),
                                  NULL, &logical);

  if (x > layout_x && x < layout_x + logical.width &&
      y > layout_y && y < layout_y + logical.height)
    {
      return TARGET_NUMBER;
    }
  else if (y > allocation.height / 2)
    {
      return TARGET_LOWER;
    }

  return TARGET_UPPER;
}

static void
gimp_spin_scale_get_limits (GimpSpinScale *scale,
                            gdouble       *lower,
                            gdouble       *upper)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (scale);

  if (private->scale_limits_set)
    {
      *lower = private->scale_lower;
      *upper = private->scale_upper;
    }
  else
    {
      GtkSpinButton *spin_button = GTK_SPIN_BUTTON (scale);
      GtkAdjustment *adjustment  = gtk_spin_button_get_adjustment (spin_button);

      *lower = gtk_adjustment_get_lower (adjustment);
      *upper = gtk_adjustment_get_upper (adjustment);
    }
}

static void
gimp_spin_scale_change_value (GtkWidget *widget,
                              gdouble    x)
{
  GimpSpinScalePrivate *private     = GET_PRIVATE (widget);
  GtkSpinButton        *spin_button = GTK_SPIN_BUTTON (widget);
  GtkAdjustment        *adjustment  = gtk_spin_button_get_adjustment (spin_button);
  GdkWindow            *text_window = gtk_entry_get_text_window (GTK_ENTRY (widget));
  gdouble               lower;
  gdouble               upper;
  gint                  width;
  gdouble               value;

  gimp_spin_scale_get_limits (GIMP_SPIN_SCALE (widget), &lower, &upper);

  width = gdk_window_get_width (text_window);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    x = width - x;

  if (private->relative_change)
    {
      gdouble diff;
      gdouble step;

      step = (upper - lower) / width / 10.0;

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        diff = x - (width - private->start_x);
      else
        diff = x - private->start_x;

      value = (private->start_value + diff * step);
    }
  else
    {
      gdouble fraction;

      fraction = x / (gdouble) width;

      value = fraction * (upper - lower) + lower;
    }

  gtk_adjustment_set_value (adjustment, value);
}

static gboolean
gimp_spin_scale_button_press (GtkWidget      *widget,
                              GdkEventButton *event)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);

  private->changing_value  = FALSE;
  private->relative_change = FALSE;

  if (event->window == gtk_entry_get_text_window (GTK_ENTRY (widget)))
    {
      switch (gimp_spin_scale_get_target (widget, event->x, event->y))
        {
        case TARGET_UPPER:
          private->changing_value = TRUE;

          gtk_widget_grab_focus (widget);

          gimp_spin_scale_change_value (widget, event->x);

          return TRUE;

        case TARGET_LOWER:
          private->changing_value = TRUE;

          gtk_widget_grab_focus (widget);

          private->relative_change = TRUE;
          private->start_x = event->x;
          private->start_value = gtk_adjustment_get_value (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget)));

          return TRUE;

        default:
          break;
        }
    }

  return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event);
}

static gboolean
gimp_spin_scale_button_release (GtkWidget      *widget,
                                GdkEventButton *event)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);

  if (private->changing_value)
    {
      private->changing_value = FALSE;

      gimp_spin_scale_change_value (widget, event->x);

      return TRUE;
    }

  return GTK_WIDGET_CLASS (parent_class)->button_release_event (widget, event);
}

static gboolean
gimp_spin_scale_motion_notify (GtkWidget      *widget,
                               GdkEventMotion *event)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);

  gdk_event_request_motions (event);

  if (private->changing_value)
    {
      gimp_spin_scale_change_value (widget, event->x);

      return TRUE;
    }

  GTK_WIDGET_CLASS (parent_class)->motion_notify_event (widget, event);

  if (! (event->state &
         (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)) &&
      event->window == gtk_entry_get_text_window (GTK_ENTRY (widget)))
    {
      GdkDisplay *display = gtk_widget_get_display (widget);
      GdkCursor  *cursor  = NULL;

      switch (gimp_spin_scale_get_target (widget, event->x, event->y))
        {
        case TARGET_NUMBER:
          cursor = gdk_cursor_new_for_display (display, GDK_XTERM);
          break;

        case TARGET_UPPER:
          cursor = gdk_cursor_new_for_display (display, GDK_SB_UP_ARROW);
          break;

        case TARGET_LOWER:
          cursor = gdk_cursor_new_for_display (display, GDK_SB_H_DOUBLE_ARROW);
          break;
        }

      gdk_window_set_cursor (event->window, cursor);
      gdk_cursor_unref (cursor);
    }

  return FALSE;
}

static gboolean
gimp_spin_scale_leave_notify (GtkWidget        *widget,
                              GdkEventCrossing *event)
{
  gdk_window_set_cursor (event->window, NULL);

  return GTK_WIDGET_CLASS (parent_class)->leave_notify_event (widget, event);
}

static void
gimp_spin_scale_value_changed (GtkSpinButton *spin_button)
{
  GtkAdjustment *adjustment = gtk_spin_button_get_adjustment (spin_button);
  gdouble        lower;
  gdouble        upper;
  gdouble        value;

  gimp_spin_scale_get_limits (GIMP_SPIN_SCALE (spin_button), &lower, &upper);

  value = CLAMP (gtk_adjustment_get_value (adjustment), lower, upper);

  gtk_entry_set_progress_fraction (GTK_ENTRY (spin_button),
                                   (value - lower) / (upper - lower));
}


/*  public functions  */

GtkWidget *
gimp_spin_scale_new (GtkAdjustment *adjustment,
                     const gchar   *label,
                     gint           digits)
{
  g_return_val_if_fail (GTK_IS_ADJUSTMENT (adjustment), NULL);

  return g_object_new (GIMP_TYPE_SPIN_SCALE,
                       "adjustment", adjustment,
                       "label",      label,
                       "digits",     digits,
                       NULL);
}

void
gimp_spin_scale_set_scale_limits (GimpSpinScale *scale,
                                  gdouble        lower,
                                  gdouble        upper)
{
  GimpSpinScalePrivate *private;
  GtkSpinButton        *spin_button;
  GtkAdjustment        *adjustment;

  g_return_if_fail (GIMP_IS_SPIN_SCALE (scale));

  private     = GET_PRIVATE (scale);
  spin_button = GTK_SPIN_BUTTON (scale);
  adjustment  = gtk_spin_button_get_adjustment (spin_button);

  g_return_if_fail (lower >= gtk_adjustment_get_lower (adjustment));
  g_return_if_fail (upper <= gtk_adjustment_get_upper (adjustment));

  private->scale_limits_set = TRUE;
  private->scale_lower      = lower;
  private->scale_upper      = upper;

  gimp_spin_scale_value_changed (spin_button);
}

void
gimp_spin_scale_unset_scale_limits (GimpSpinScale *scale)
{
  GimpSpinScalePrivate *private;

  g_return_if_fail (GIMP_IS_SPIN_SCALE (scale));

  private = GET_PRIVATE (scale);

  private->scale_limits_set = FALSE;
  private->scale_lower      = 0.0;
  private->scale_upper      = 0.0;

  gimp_spin_scale_value_changed (GTK_SPIN_BUTTON (scale));
}

gboolean
gimp_spin_scale_get_scale_limits (GimpSpinScale *scale,
                                  gdouble       *lower,
                                  gdouble       *upper)
{
  GimpSpinScalePrivate *private;

  g_return_val_if_fail (GIMP_IS_SPIN_SCALE (scale), FALSE);

  private = GET_PRIVATE (scale);

  if (lower)
    *lower = private->scale_lower;

  if (upper)
    *upper = private->scale_upper;

  return private->scale_limits_set;
}

