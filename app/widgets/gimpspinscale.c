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


typedef struct _GimpSpinScalePrivate GimpSpinScalePrivate;

struct _GimpSpinScalePrivate
{
  gchar    *label;
  gboolean  changing_value;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                                       GIMP_TYPE_SPIN_SCALE, \
                                                       GimpSpinScalePrivate))


static void       gimp_spin_scale_dispose        (GObject        *object);
static void       gimp_spin_scale_finalize       (GObject        *object);
static void       gimp_spin_scale_set_property   (GObject        *object,
                                                  guint           property_id,
                                                  const GValue   *value,
                                                  GParamSpec     *pspec);
static void       gimp_spin_scale_get_property   (GObject        *object,
                                                  guint           property_id,
                                                  GValue         *value,
                                                  GParamSpec     *pspec);

static void       gimp_spin_scale_style_set      (GtkWidget      *widget,
                                                  GtkStyle       *prev_style);
static gboolean   gimp_spin_scale_expose         (GtkWidget      *widget,
                                                  GdkEventExpose *event);
static gboolean   gimp_spin_scale_button_press   (GtkWidget      *widget,
                                                  GdkEventButton *event);
static gboolean   gimp_spin_scale_button_release (GtkWidget      *widget,
                                                  GdkEventButton *event);
static gboolean   gimp_spin_scale_button_motion  (GtkWidget      *widget,
                                                  GdkEventMotion *event);

static void       gimp_spin_scale_value_changed  (GtkSpinButton  *spin_button);


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

  widget_class->style_set            = gimp_spin_scale_style_set;
  widget_class->expose_event         = gimp_spin_scale_expose;
  widget_class->button_press_event   = gimp_spin_scale_button_press;
  widget_class->button_release_event = gimp_spin_scale_button_release;
  widget_class->motion_notify_event  = gimp_spin_scale_button_motion;

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
  gtk_entry_set_alignment (GTK_ENTRY (scale), 1.0);
  gtk_entry_set_has_frame (GTK_ENTRY (scale), FALSE);
}

static void
gimp_spin_scale_dispose (GObject *object)
{
  /* GimpSpinScalePrivate *priv = GET_PRIVATE (object); */

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
      gtk_widget_queue_draw (GTK_WIDGET (object));
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
gimp_spin_scale_style_set (GtkWidget *widget,
                           GtkStyle  *prev_style)
{
  GtkStyle         *style   = gtk_widget_get_style (widget);
  PangoContext     *context = gtk_widget_get_pango_context (widget);
  PangoFontMetrics *metrics;
  gint              height;
  GtkBorder         border;

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  metrics = pango_context_get_metrics (context, style->font_desc,
                                       pango_context_get_language (context));

  height = PANGO_PIXELS (pango_font_metrics_get_ascent (metrics));

  pango_font_metrics_unref (metrics);

  border.left   = 2;
  border.right  = 2;
  border.top    = height + 2;
  border.bottom = 2;

  gtk_entry_set_inner_border (GTK_ENTRY (widget), &border);
}

static gboolean
gimp_spin_scale_expose (GtkWidget      *widget,
                        GdkEventExpose *event)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);
  GtkStyle             *style   = gtk_widget_get_style (widget);
  cairo_t              *cr;
  gint                  w, h;

  GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);

  cr = gdk_cairo_create (event->window);
  gdk_cairo_region (cr, event->region);
  cairo_clip (cr);

#if GTK_CHECK_VERSION (2, 24, 0)
  w = gdk_window_get_width (event->window);
  h = gdk_window_get_height (event->window);
#else
  gdk_drawable_get_size (event->window, &w, &h);
#endif

  cairo_set_line_width (cr, 1.0);

  if (event->window == gtk_entry_get_text_window (GTK_ENTRY (widget)))
    {
      /* let right line of rectangle disappear */
      cairo_rectangle (cr, 0.5, 0.5, w, h - 1.0);
      gdk_cairo_set_source_color (cr,
                                  &style->text[gtk_widget_get_state (widget)]);
      cairo_stroke (cr);
    }
  else
    {
      /* let left line of rectangle disappear */
      cairo_rectangle (cr, -0.5, 0.5, w, h - 1.0);
      gdk_cairo_set_source_color (cr,
                                  &style->text[gtk_widget_get_state (widget)]);
      cairo_stroke (cr);

      cairo_rectangle (cr, 0.5, 1.5, w - 2.0, h - 3.0);
      gdk_cairo_set_source_color (cr,
                                  &style->base[gtk_widget_get_state (widget)]);
      cairo_stroke (cr);
    }

  cairo_destroy (cr);

  if (private->label &&
      gtk_widget_is_drawable (widget) &&
      event->window == gtk_entry_get_text_window (GTK_ENTRY (widget)))
    {
      PangoLayout     *layout;
      const GtkBorder *border;

      cr = gdk_cairo_create (event->window);
      gdk_cairo_region (cr, event->region);
      cairo_clip (cr);

      border = gtk_entry_get_inner_border (GTK_ENTRY (widget));

      if (border)
        cairo_move_to (cr, border->left, border->left /* sic! */);
      else
        cairo_move_to (cr, 2, 2);

      gdk_cairo_set_source_color (cr,
                                  &style->text[gtk_widget_get_state (widget)]);

      layout = gtk_widget_create_pango_layout (widget, private->label);
      pango_cairo_show_layout (cr, layout);
      cairo_destroy (cr);

      g_object_unref (layout);
    }

  return FALSE;
}

static gboolean
gimp_spin_scale_on_number (GtkWidget *widget,
                           gdouble    x,
                           gdouble    y)
{
  gint layout_x;
  gint layout_y;

  gtk_entry_get_layout_offsets (GTK_ENTRY (widget), &layout_x, &layout_y);

  if (x > layout_x && y > layout_y)
    {
      return TRUE;
    }

  return FALSE;
}

static void
gimp_spin_scale_set_value (GtkWidget *widget,
                           gdouble    x)
{
  GtkSpinButton *spin_button = GTK_SPIN_BUTTON (widget);
  GtkAdjustment *adjustment  = gtk_spin_button_get_adjustment (spin_button);
  GdkWindow     *text_window = gtk_entry_get_text_window (GTK_ENTRY (widget));
  gint           width;
  gdouble        fraction;
  gdouble        value;

#if GTK_CHECK_VERSION (2, 24, 0)
  width = gdk_window_get_width (text_window);
#else
  gdk_drawable_get_size (text_window, &width, NULL);
#endif

  fraction = x / (gdouble) width;

  value = (fraction * (gtk_adjustment_get_upper (adjustment) -
                       gtk_adjustment_get_lower (adjustment)) +
           gtk_adjustment_get_lower (adjustment));

  gtk_adjustment_set_value (adjustment, value);
}

static gboolean
gimp_spin_scale_button_press (GtkWidget      *widget,
                              GdkEventButton *event)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);

  private->changing_value = FALSE;

  if (event->window == gtk_entry_get_text_window (GTK_ENTRY (widget)))
    {
      if (! gimp_spin_scale_on_number (widget, event->x, event->y))
        {
          private->changing_value = TRUE;

          gtk_widget_grab_focus (widget);

          gimp_spin_scale_set_value (widget, event->x);

          return TRUE;
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

      gimp_spin_scale_set_value (widget, event->x);

      return TRUE;
    }

  return GTK_WIDGET_CLASS (parent_class)->button_release_event (widget, event);
}

static gboolean
gimp_spin_scale_button_motion (GtkWidget      *widget,
                               GdkEventMotion *event)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);

  if (private->changing_value)
    {
      gimp_spin_scale_set_value (widget, event->x);

      return TRUE;
    }

  GTK_WIDGET_CLASS (parent_class)->motion_notify_event (widget, event);

  if (! (event->state &
         (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)) &&
      event->window == gtk_entry_get_text_window (GTK_ENTRY (widget)))
    {
      GdkDisplay *display = gtk_widget_get_display (widget);
      GdkCursor  *cursor;

      if (gimp_spin_scale_on_number (widget, event->x, event->y))
        cursor = gdk_cursor_new_for_display (display, GDK_XTERM);
      else
        cursor = gdk_cursor_new_for_display (display, GDK_LEFT_PTR);

      gdk_window_set_cursor (event->window, cursor);
      gdk_cursor_unref (cursor);
    }

  return FALSE;
}

static void
gimp_spin_scale_value_changed (GtkSpinButton *spin_button)
{
  GtkAdjustment *adjustment = gtk_spin_button_get_adjustment (spin_button);

  gtk_entry_set_progress_fraction (GTK_ENTRY (spin_button),
                                   (gtk_adjustment_get_value (adjustment) -
                                    gtk_adjustment_get_lower (adjustment)) /
                                   (gtk_adjustment_get_upper (adjustment) -
                                    gtk_adjustment_get_lower (adjustment)));
}

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
