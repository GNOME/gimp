/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpspinscale.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
 *               2012 Øyvind Kolås    <pippin@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpmath/gimpmath.h"

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
  TARGET_LOWER,
  TARGET_NONE
} SpinScaleTarget;


typedef struct _GimpSpinScalePrivate GimpSpinScalePrivate;

struct _GimpSpinScalePrivate
{
  gchar       *label;
  gchar       *label_text;
  gchar       *label_pattern;

  GtkWindow   *mnemonic_window;
  guint        mnemonic_keyval;
  gboolean     mnemonics_visible;

  gboolean     scale_limits_set;
  gdouble      scale_lower;
  gdouble      scale_upper;
  gdouble      gamma;

  PangoLayout *layout;
  gboolean     changing_value;
  gboolean     relative_change;
  gdouble      start_x;
  gdouble      start_value;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                                       GIMP_TYPE_SPIN_SCALE, \
                                                       GimpSpinScalePrivate))


static void       gimp_spin_scale_dispose              (GObject          *object);
static void       gimp_spin_scale_finalize             (GObject          *object);
static void       gimp_spin_scale_set_property         (GObject          *object,
                                                        guint             property_id,
                                                        const GValue     *value,
                                                        GParamSpec       *pspec);
static void       gimp_spin_scale_get_property         (GObject          *object,
                                                        guint             property_id,
                                                        GValue           *value,
                                                        GParamSpec       *pspec);

static void       gimp_spin_scale_get_preferred_width  (GtkWidget        *widget,
                                                        gint             *minimum_width,
                                                        gint             *natural_width);
static void       gimp_spin_scale_get_preferred_height (GtkWidget        *widget,
                                                        gint             *minimum_width,
                                                        gint             *natural_width);
static void       gimp_spin_scale_style_set            (GtkWidget        *widget,
                                                        GtkStyle         *prev_style);
static gboolean   gimp_spin_scale_draw                 (GtkWidget        *widget,
                                                        cairo_t          *cr);
static gboolean   gimp_spin_scale_button_press         (GtkWidget        *widget,
                                                        GdkEventButton   *event);
static gboolean   gimp_spin_scale_button_release       (GtkWidget        *widget,
                                                        GdkEventButton   *event);
static gboolean   gimp_spin_scale_motion_notify        (GtkWidget        *widget,
                                                        GdkEventMotion   *event);
static gboolean   gimp_spin_scale_leave_notify         (GtkWidget        *widget,
                                                        GdkEventCrossing *event);
static void       gimp_spin_scale_hierarchy_changed    (GtkWidget        *widget,
                                                        GtkWidget        *old_toplevel);
static void       gimp_spin_scale_screen_changed       (GtkWidget        *widget,
                                                        GdkScreen        *old_screen);

static void       gimp_spin_scale_value_changed        (GtkSpinButton    *spin_button);
static void       gimp_spin_scale_settings_notify      (GtkSettings      *settings,
                                                        const GParamSpec *pspec,
                                                        GimpSpinScale    *scale);
static void       gimp_spin_scale_mnemonics_notify     (GtkWindow        *window,
                                                        const GParamSpec *pspec,
                                                        GimpSpinScale    *scale);
static void       gimp_spin_scale_setup_mnemonic       (GimpSpinScale    *scale,
                                                        guint             previous_keyval);


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

  widget_class->get_preferred_width  = gimp_spin_scale_get_preferred_width;
  widget_class->get_preferred_height = gimp_spin_scale_get_preferred_height;
  widget_class->style_set            = gimp_spin_scale_style_set;
  widget_class->draw                 = gimp_spin_scale_draw;
  widget_class->button_press_event   = gimp_spin_scale_button_press;
  widget_class->button_release_event = gimp_spin_scale_button_release;
  widget_class->motion_notify_event  = gimp_spin_scale_motion_notify;
  widget_class->leave_notify_event   = gimp_spin_scale_leave_notify;
  widget_class->hierarchy_changed    = gimp_spin_scale_hierarchy_changed;
  widget_class->screen_changed       = gimp_spin_scale_screen_changed;

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
  GimpSpinScalePrivate *private = GET_PRIVATE (scale);

  gtk_widget_add_events (GTK_WIDGET (scale),
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_BUTTON1_MOTION_MASK |
                         GDK_LEAVE_NOTIFY_MASK);

  gtk_entry_set_alignment (GTK_ENTRY (scale), 1.0);
  gtk_entry_set_has_frame (GTK_ENTRY (scale), FALSE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (scale), TRUE);

  private->mnemonic_keyval = GDK_KEY_VoidSymbol;
  private->gamma           = 1.0;
}

static void
gimp_spin_scale_dispose (GObject *object)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (object);
  guint                 keyval;

  keyval = private->mnemonic_keyval;
  private->mnemonic_keyval = GDK_KEY_VoidSymbol;

  gimp_spin_scale_setup_mnemonic (GIMP_SPIN_SCALE (object), keyval);

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

  if (private->label_text)
    {
      g_free (private->label_text);
      private->label_text = NULL;
    }

  if (private->label_pattern)
    {
      g_free (private->label_pattern);
      private->label_pattern = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_spin_scale_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpSpinScale *scale = GIMP_SPIN_SCALE (object);

  switch (property_id)
    {
    case PROP_LABEL:
      gimp_spin_scale_set_label (scale, g_value_get_string (value));
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
  GimpSpinScale *scale = GIMP_SPIN_SCALE (object);

  switch (property_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, gimp_spin_scale_get_label (scale));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_spin_scale_get_preferred_width (GtkWidget *widget,
                                     gint      *minimum_width,
                                     gint      *natural_width)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);
  GtkStyle             *style   = gtk_widget_get_style (widget);
  PangoContext         *context = gtk_widget_get_pango_context (widget);
  PangoFontMetrics     *metrics;

  GTK_WIDGET_CLASS (parent_class)->get_preferred_width (widget,
                                                        minimum_width,
                                                        natural_width);

  metrics = pango_context_get_metrics (context, style->font_desc,
                                       pango_context_get_language (context));

  if (private->label)
    {
      gint char_width;
      gint digit_width;
      gint char_pixels;

      char_width = pango_font_metrics_get_approximate_char_width (metrics);
      digit_width = pango_font_metrics_get_approximate_digit_width (metrics);
      char_pixels = PANGO_PIXELS (MAX (char_width, digit_width));

      /* ~3 chars for the ellipses */
      *minimum_width += char_pixels * 3;
      *natural_width += char_pixels * 3;
    }

  pango_font_metrics_unref (metrics);
}

static void
gimp_spin_scale_get_preferred_height (GtkWidget *widget,
                                      gint      *minimum_height,
                                      gint      *natural_height)
{
  GtkStyle         *style   = gtk_widget_get_style (widget);
  PangoContext     *context = gtk_widget_get_pango_context (widget);
  PangoFontMetrics *metrics;
  gint              height;

  GTK_WIDGET_CLASS (parent_class)->get_preferred_height (widget,
                                                         minimum_height,
                                                         natural_height);

  metrics = pango_context_get_metrics (context, style->font_desc,
                                       pango_context_get_language (context));

  height = PANGO_PIXELS (pango_font_metrics_get_ascent (metrics) +
                         pango_font_metrics_get_descent (metrics));

  *minimum_height += height;
  *natural_height += height;

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

static PangoAttrList *
pattern_to_attrs (const gchar *text,
                  const gchar *pattern)
{
  PangoAttrList *attrs = pango_attr_list_new ();
  const char    *p     = text;
  const char    *q     = pattern;
  const char    *start;

  while (TRUE)
    {
      while (*p && *q && *q != '_')
        {
          p = g_utf8_next_char (p);
          q++;
        }
      start = p;
      while (*p && *q && *q == '_')
        {
          p = g_utf8_next_char (p);
          q++;
        }

      if (p > start)
        {
          PangoAttribute *attr = pango_attr_underline_new (PANGO_UNDERLINE_LOW);

          attr->start_index = start - text;
          attr->end_index   = p - text;

          pango_attr_list_insert (attrs, attr);
        }
      else
        break;
    }

  return attrs;
}

static gboolean
gimp_spin_scale_draw (GtkWidget *widget,
                      cairo_t   *cr)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);
  GtkStyle             *style   = gtk_widget_get_style (widget);
  GtkAllocation         allocation;

  cairo_save (cr);
  GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);
  cairo_restore (cr);

  gtk_widget_get_allocation (widget, &allocation);

  cairo_set_line_width (cr, 1.0);

  cairo_rectangle (cr, 0.5, 0.5,
                   allocation.width - 1.0, allocation.height - 1.0);
  gdk_cairo_set_source_color (cr,
                              &style->text[gtk_widget_get_state (widget)]);
  cairo_stroke (cr);

  if (private->label)
    {
      GdkRectangle   text_area;
      gint           minimum_width;
      gint           natural_width;
      PangoRectangle logical;
      gint           layout_offset_x;
      gint           layout_offset_y;
      GtkStateType   state;
      GdkColor       text_color;
      GdkColor       bar_text_color;
      gdouble        progress_fraction;
      gint           progress_x;
      gint           progress_y;
      gint           progress_width;
      gint           progress_height;

      gtk_entry_get_text_area (GTK_ENTRY (widget), &text_area);

      GTK_WIDGET_CLASS (parent_class)->get_preferred_width (widget,
                                                            &minimum_width,
                                                            &natural_width);

      if (! private->layout)
        {
          private->layout = gtk_widget_create_pango_layout (widget,
                                                            private->label_text);
          pango_layout_set_ellipsize (private->layout, PANGO_ELLIPSIZE_END);

          if (private->mnemonics_visible)
            {
              PangoAttrList *attrs;

              attrs = pattern_to_attrs (private->label_text,
                                        private->label_pattern);
              if (attrs)
                {
                  pango_layout_set_attributes (private->layout, attrs);
                  pango_attr_list_unref (attrs);
                }
            }
        }

      pango_layout_set_width (private->layout,
                              PANGO_SCALE *
                              (allocation.width - minimum_width));
      pango_layout_get_pixel_extents (private->layout, NULL, &logical);

      gtk_entry_get_layout_offsets (GTK_ENTRY (widget), NULL, &layout_offset_y);

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        layout_offset_x = text_area.x + text_area.width - logical.width - 2;
      else
        layout_offset_x = text_area.x + 2;

      layout_offset_x -= logical.x;

      state = GTK_STATE_SELECTED;
      if (! gtk_widget_get_sensitive (widget))
        state = GTK_STATE_INSENSITIVE;
      text_color     = style->text[gtk_widget_get_state (widget)];
      bar_text_color = style->fg[state];

      progress_fraction = gtk_entry_get_progress_fraction (GTK_ENTRY (widget));

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        {
          progress_fraction = 1.0 - progress_fraction;

          progress_x      = text_area.width * progress_fraction;
          progress_y      = 0;
          progress_width  = text_area.width - progress_x;
          progress_height = text_area.height;
        }
      else
        {
          progress_x      = 0;
          progress_y      = 0;
          progress_width  = text_area.width * progress_fraction;
          progress_height = text_area.height;
        }

      cairo_save (cr);

      cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
      cairo_rectangle (cr, 0, 0, text_area.width, text_area.height);
      cairo_rectangle (cr, progress_x, progress_y,
                       progress_width, progress_height);
      cairo_clip (cr);
      cairo_set_fill_rule (cr, CAIRO_FILL_RULE_WINDING);

      cairo_move_to (cr, layout_offset_x, text_area.y + layout_offset_y);
      gdk_cairo_set_source_color (cr, &text_color);
      pango_cairo_show_layout (cr, private->layout);

      cairo_restore (cr);

      cairo_rectangle (cr, progress_x, progress_y,
                       progress_width, progress_height);
      cairo_clip (cr);

      cairo_move_to (cr, layout_offset_x, text_area.y + layout_offset_y);
      gdk_cairo_set_source_color (cr, &bar_text_color);
      pango_cairo_show_layout (cr, private->layout);
    }

  return FALSE;
}

/* Returns TRUE if a translation should be done */
static gboolean
gtk_widget_get_translation_to_window (GtkWidget *widget,
                                      GdkWindow *window,
                                      int       *x,
                                      int       *y)
{
  GdkWindow *w, *widget_window;

  if (!gtk_widget_get_has_window (widget))
    {
      GtkAllocation allocation;

      gtk_widget_get_allocation (widget, &allocation);

      *x = -allocation.x;
      *y = -allocation.y;
    }
  else
    {
      *x = 0;
      *y = 0;
    }

  widget_window = gtk_widget_get_window (widget);

  for (w = window; w && w != widget_window; w = gdk_window_get_parent (w))
    {
      int wx, wy;
      gdk_window_get_position (w, &wx, &wy);
      *x += wx;
      *y += wy;
    }

  if (w == NULL)
    {
      *x = 0;
      *y = 0;
      return FALSE;
    }

  return TRUE;
}

static void
gimp_spin_scale_event_to_widget_coords (GtkWidget *widget,
                                        GdkWindow *window,
                                        gdouble    event_x,
                                        gdouble    event_y,
                                        gint      *widget_x,
                                        gint      *widget_y)
{
  gint tx, ty;

  if (gtk_widget_get_translation_to_window (widget, window, &tx, &ty))
    {
      event_x += tx;
      event_y += ty;
    }

  *widget_x = event_x;
  *widget_y = event_y;
}

static SpinScaleTarget
gimp_spin_scale_get_target (GtkWidget *widget,
                            gdouble    x,
                            gdouble    y)
{
  GdkRectangle    text_area;
  GtkAllocation   allocation;
  PangoRectangle  logical;
  gint            layout_x;
  gint            layout_y;

  gtk_widget_get_allocation (widget, &allocation);
  gtk_entry_get_layout_offsets (GTK_ENTRY (widget), &layout_x, &layout_y);
  pango_layout_get_pixel_extents (gtk_entry_get_layout (GTK_ENTRY (widget)),
                                  NULL, &logical);

  gtk_entry_get_text_area (GTK_ENTRY (widget), &text_area);

  if (x >= text_area.x && x < text_area.width &&
      y >= text_area.y && y < text_area.height)
    {
      x -= text_area.x;
      y -= text_area.y;

      if (x > layout_x && x < layout_x + logical.width &&
          y > layout_y && y < layout_y + logical.height)
        {
          return TARGET_NUMBER;
        }
      else if (y > text_area.height / 2)
        {
          return TARGET_LOWER;
        }

      return TARGET_UPPER;
    }

  return TARGET_NONE;
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
  GdkRectangle          text_area;
  gdouble               lower;
  gdouble               upper;
  gdouble               value;
  gint                  digits;
  gint                  power = 1;

  gtk_entry_get_text_area (GTK_ENTRY (widget), &text_area);

  gimp_spin_scale_get_limits (GIMP_SPIN_SCALE (widget), &lower, &upper);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    x = text_area.width - x;

  if (private->relative_change)
    {
      gdouble diff;
      gdouble step;

      step = (upper - lower) / text_area.width / 10.0;

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        diff = x - (text_area.width - private->start_x);
      else
        diff = x - private->start_x;

      value = (private->start_value + diff * step);
    }
  else
    {
      gdouble fraction;

      fraction = x / (gdouble) text_area.width;
      if (fraction > 0.0)
        fraction = pow (fraction, private->gamma);

      value = fraction * (upper - lower) + lower;
    }

  digits = gtk_spin_button_get_digits (spin_button);
  while (digits--)
    power *= 10;

  /*  round the value to the possible precision of the spinbutton, so
   *  a focus-out will not change the value again, causing inadvertend
   *  adjustment signals.
   */
  value *= power;
  value = RINT (value);
  value /= power;

  gtk_adjustment_set_value (adjustment, value);
}

static gboolean
gimp_spin_scale_button_press (GtkWidget      *widget,
                              GdkEventButton *event)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);
  gint                  x, y;

  private->changing_value  = FALSE;
  private->relative_change = FALSE;

  gimp_spin_scale_event_to_widget_coords (widget, event->window,
                                          event->x, event->y,
                                          &x, &y);

  switch (gimp_spin_scale_get_target (widget, x, y))
    {
    case TARGET_UPPER:
      private->changing_value = TRUE;

      gtk_widget_grab_focus (widget);

      gimp_spin_scale_change_value (widget, x);

      return TRUE;

    case TARGET_LOWER:
      private->changing_value = TRUE;

      gtk_widget_grab_focus (widget);

      private->relative_change = TRUE;
      private->start_x = x;
      private->start_value = gtk_adjustment_get_value (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget)));

      return TRUE;

    default:
      break;
    }

  return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event);
}

static gboolean
gimp_spin_scale_button_release (GtkWidget      *widget,
                                GdkEventButton *event)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);
  gint                  x, y;

  gimp_spin_scale_event_to_widget_coords (widget, event->window,
                                          event->x, event->y,
                                          &x, &y);

  if (private->changing_value)
    {
      private->changing_value = FALSE;

      gimp_spin_scale_change_value (widget, x);

      return TRUE;
    }

  return GTK_WIDGET_CLASS (parent_class)->button_release_event (widget, event);
}

static gboolean
gimp_spin_scale_motion_notify (GtkWidget      *widget,
                               GdkEventMotion *event)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);
  gint                  x, y;

  gimp_spin_scale_event_to_widget_coords (widget, event->window,
                                          event->x, event->y,
                                          &x, &y);

  gdk_event_request_motions (event);

  if (private->changing_value)
    {
      gimp_spin_scale_change_value (widget, x);

      return TRUE;
    }

  GTK_WIDGET_CLASS (parent_class)->motion_notify_event (widget, event);

  if (! (event->state &
         (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)))
    {
      GdkDisplay *display = gtk_widget_get_display (widget);
      GdkCursor  *cursor  = NULL;

      switch (gimp_spin_scale_get_target (widget, x, y))
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

        default:
          break;
        }

      if (cursor)
        {
          gdk_window_set_cursor (event->window, cursor);
          g_object_unref (cursor);
        }
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
gimp_spin_scale_hierarchy_changed (GtkWidget *widget,
                                   GtkWidget *old_toplevel)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (widget);

  gimp_spin_scale_setup_mnemonic (GIMP_SPIN_SCALE (widget),
                                  private->mnemonic_keyval);
}

static void
gimp_spin_scale_screen_changed (GtkWidget *widget,
                                GdkScreen *old_screen)
{
  GimpSpinScale        *scale   = GIMP_SPIN_SCALE (widget);
  GimpSpinScalePrivate *private = GET_PRIVATE (scale);
  GtkSettings          *settings;

  if (private->layout)
    {
      g_object_unref (private->layout);
      private->layout = NULL;
    }

  if (old_screen)
    {
      settings = gtk_settings_get_for_screen (old_screen);

      g_signal_handlers_disconnect_by_func (settings,
                                            gimp_spin_scale_settings_notify,
                                            scale);
    }

  if (! gtk_widget_has_screen (widget))
    return;

  settings = gtk_widget_get_settings (widget);

  g_signal_connect (settings, "notify::gtk-enable-mnemonics",
                    G_CALLBACK (gimp_spin_scale_settings_notify),
                    scale);
  g_signal_connect (settings, "notify::gtk-enable-accels",
                    G_CALLBACK (gimp_spin_scale_settings_notify),
                    scale);

  gimp_spin_scale_settings_notify (settings, NULL, scale);
}

static void
gimp_spin_scale_value_changed (GtkSpinButton *spin_button)
{
  GimpSpinScalePrivate *private    = GET_PRIVATE (spin_button);
  GtkAdjustment        *adjustment = gtk_spin_button_get_adjustment (spin_button);
  gdouble               lower;
  gdouble               upper;
  gdouble               value;

  gimp_spin_scale_get_limits (GIMP_SPIN_SCALE (spin_button), &lower, &upper);

  value = CLAMP (gtk_adjustment_get_value (adjustment), lower, upper);

  gtk_entry_set_progress_fraction (GTK_ENTRY (spin_button),
                                   pow ((value - lower) / (upper - lower),
                                        1.0 / private->gamma));
}

static void
gimp_spin_scale_settings_notify (GtkSettings      *settings,
                                 const GParamSpec *pspec,
                                 GimpSpinScale    *scale)
{
  GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (scale));

  if (GTK_IS_WINDOW (toplevel))
    gimp_spin_scale_mnemonics_notify (GTK_WINDOW (toplevel), NULL, scale);
}

static void
gimp_spin_scale_mnemonics_notify (GtkWindow        *window,
                                  const GParamSpec *pspec,
                                  GimpSpinScale    *scale)
{
  GimpSpinScalePrivate *private           = GET_PRIVATE (scale);
  gboolean              mnemonics_visible = FALSE;
  gboolean              enable_mnemonics;
  gboolean              auto_mnemonics;

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (scale)),
                "gtk-enable-mnemonics", &enable_mnemonics,
                "gtk-auto-mnemonics",   &auto_mnemonics,
                NULL);

  if (enable_mnemonics &&
      (! auto_mnemonics ||
       gtk_widget_is_sensitive (GTK_WIDGET (scale))))
    {
      g_object_get (window,
                    "mnemonics-visible", &mnemonics_visible,
                    NULL);
    }

  if (private->mnemonics_visible != mnemonics_visible)
    {
      private->mnemonics_visible = mnemonics_visible;

      if (private->layout)
        {
          g_object_unref (private->layout);
          private->layout = NULL;
        }

      gtk_widget_queue_draw (GTK_WIDGET (scale));
    }
}

static void
gimp_spin_scale_setup_mnemonic (GimpSpinScale *scale,
                                guint          previous_keyval)
{
  GimpSpinScalePrivate *private = GET_PRIVATE (scale);
  GtkWidget            *widget  = GTK_WIDGET (scale);
  GtkWidget            *toplevel;

  if (private->mnemonic_window)
    {
      g_signal_handlers_disconnect_by_func (private->mnemonic_window,
                                            gimp_spin_scale_mnemonics_notify,
                                            scale);

      gtk_window_remove_mnemonic (private->mnemonic_window,
                                  previous_keyval,
                                  widget);
      private->mnemonic_window = NULL;
    }

  toplevel = gtk_widget_get_toplevel (widget);

  if (gtk_widget_is_toplevel (toplevel) &&
      private->mnemonic_keyval != GDK_KEY_VoidSymbol)
    {
      gtk_window_add_mnemonic (GTK_WINDOW (toplevel),
                               private->mnemonic_keyval,
                               widget);
      private->mnemonic_window = GTK_WINDOW (toplevel);

      g_signal_connect (toplevel, "notify::mnemonics-visible",
                        G_CALLBACK (gimp_spin_scale_mnemonics_notify),
                        scale);
     }
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

static gboolean
separate_uline_pattern (const gchar  *str,
                        guint        *accel_key,
                        gchar       **new_str,
                        gchar       **pattern)
{
  gboolean underscore;
  const gchar *src;
  gchar *dest;
  gchar *pattern_dest;

  *accel_key = GDK_KEY_VoidSymbol;
  *new_str = g_new (gchar, strlen (str) + 1);
  *pattern = g_new (gchar, g_utf8_strlen (str, -1) + 1);

  underscore = FALSE;

  src = str;
  dest = *new_str;
  pattern_dest = *pattern;

  while (*src)
    {
      gunichar c;
      const gchar *next_src;

      c = g_utf8_get_char (src);
      if (c == (gunichar)-1)
        {
          g_warning ("Invalid input string");
          g_free (*new_str);
          g_free (*pattern);

          return FALSE;
        }
      next_src = g_utf8_next_char (src);

      if (underscore)
        {
          if (c == '_')
            *pattern_dest++ = ' ';
          else
            {
              *pattern_dest++ = '_';
              if (*accel_key == GDK_KEY_VoidSymbol)
                *accel_key = gdk_keyval_to_lower (gdk_unicode_to_keyval (c));
            }

          while (src < next_src)
            *dest++ = *src++;

          underscore = FALSE;
        }
      else
        {
          if (c == '_')
            {
              underscore = TRUE;
              src = next_src;
            }
          else
            {
              while (src < next_src)
                *dest++ = *src++;

              *pattern_dest++ = ' ';
            }
        }
    }

  *dest = 0;
  *pattern_dest = 0;

  return TRUE;
}

void
gimp_spin_scale_set_label (GimpSpinScale *scale,
                           const gchar   *label)
{
  GimpSpinScalePrivate *private;
  guint                 accel_key = GDK_KEY_VoidSymbol;
  gchar                *text      = NULL;
  gchar                *pattern   = NULL;

  g_return_if_fail (GIMP_IS_SPIN_SCALE (scale));

  private = GET_PRIVATE (scale);

  if (label == private->label)
    return;

  if (label && ! separate_uline_pattern (label, &accel_key, &text, &pattern))
    return;

  g_free (private->label);
  private->label = g_strdup (label);

  g_free (private->label_text);
  private->label_text = text; /* don't dup */

  g_free (private->label_pattern);
  private->label_pattern = pattern; /* don't dup */

  if (private->mnemonic_keyval != accel_key)
    {
      guint previous = private->mnemonic_keyval;

      private->mnemonic_keyval = accel_key;

      gimp_spin_scale_setup_mnemonic (scale, previous);
    }

  if (private->layout)
    {
      g_object_unref (private->layout);
      private->layout = NULL;
    }

  gtk_widget_queue_resize (GTK_WIDGET (scale));

  g_object_notify (G_OBJECT (scale), "label");
}

const gchar *
gimp_spin_scale_get_label (GimpSpinScale *scale)
{
  g_return_val_if_fail (GIMP_IS_SPIN_SCALE (scale), NULL);

  return GET_PRIVATE (scale)->label;
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
  private->gamma            = 1.0;

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

void
gimp_spin_scale_set_gamma (GimpSpinScale *scale,
                           gdouble        gamma)
{
  GimpSpinScalePrivate *private;

  g_return_if_fail (GIMP_IS_SPIN_SCALE (scale));

  private = GET_PRIVATE (scale);

  private->gamma = gamma;

  gimp_spin_scale_value_changed (GTK_SPIN_BUTTON (scale));
}

gdouble
gimp_spin_scale_get_gamma (GimpSpinScale *scale)
{
  g_return_val_if_fail (GIMP_IS_SPIN_SCALE (scale), 1.0);

  return GET_PRIVATE (scale)->gamma;
}
