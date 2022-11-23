/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaspinscale.c
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
 *               2012 Øyvind Kolås    <pippin@ligma.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "ligmawidgetstypes.h"

#include "ligmaspinscale.h"

#include "libligma/libligma-intl.h"


#define RELATIVE_CHANGE_SPEED 0.1


enum
{
  PROP_0,
  PROP_LABEL
};

typedef enum
{
  TARGET_NONE,
  TARGET_NUMBER,
  TARGET_GRAB,
  TARGET_GRABBING,
  TARGET_RELATIVE
} SpinScaleTarget;


typedef struct _LigmaSpinScalePrivate LigmaSpinScalePrivate;

struct _LigmaSpinScalePrivate
{
  gchar           *label;
  gchar           *label_text;
  gchar           *label_pattern;

  GtkWindow       *mnemonic_window;
  guint            mnemonic_keyval;
  gboolean         mnemonics_visible;

  gboolean         constrain_drag;

  gboolean         scale_limits_set;
  gdouble          scale_lower;
  gdouble          scale_upper;
  gdouble          gamma;

  PangoLayout     *layout;
  gboolean         changing_value;
  gboolean         relative_change;
  gdouble          start_x;
  gdouble          start_value;
  gint             start_pointer_x;
  gint             start_pointer_y;
  SpinScaleTarget  target;
  gboolean         hover;
  gboolean         pointer_warp;
  gint             pointer_warp_x;
  gint             pointer_warp_start_x;
};

#define GET_PRIVATE(obj) ((LigmaSpinScalePrivate *) ligma_spin_scale_get_instance_private ((LigmaSpinScale *) (obj)))


static void       ligma_spin_scale_dispose              (GObject          *object);
static void       ligma_spin_scale_finalize             (GObject          *object);
static void       ligma_spin_scale_set_property         (GObject          *object,
                                                        guint             property_id,
                                                        const GValue     *value,
                                                        GParamSpec       *pspec);
static void       ligma_spin_scale_get_property         (GObject          *object,
                                                        guint             property_id,
                                                        GValue           *value,
                                                        GParamSpec       *pspec);

static void       ligma_spin_scale_get_preferred_width  (GtkWidget        *widget,
                                                        gint             *minimum_width,
                                                        gint             *natural_width);
static void       ligma_spin_scale_get_preferred_height (GtkWidget        *widget,
                                                        gint             *minimum_width,
                                                        gint             *natural_width);
static void       ligma_spin_scale_style_updated        (GtkWidget        *widget);
static gboolean   ligma_spin_scale_draw                 (GtkWidget        *widget,
                                                        cairo_t          *cr);
static gboolean   ligma_spin_scale_button_press         (GtkWidget        *widget,
                                                        GdkEventButton   *event);
static gboolean   ligma_spin_scale_button_release       (GtkWidget        *widget,
                                                        GdkEventButton   *event);
static gboolean   ligma_spin_scale_motion_notify        (GtkWidget        *widget,
                                                        GdkEventMotion   *event);
static gboolean   ligma_spin_scale_leave_notify         (GtkWidget        *widget,
                                                        GdkEventCrossing *event);
static void       ligma_spin_scale_hierarchy_changed    (GtkWidget        *widget,
                                                        GtkWidget        *old_toplevel);
static void       ligma_spin_scale_screen_changed       (GtkWidget        *widget,
                                                        GdkScreen        *old_screen);

static void       ligma_spin_scale_value_changed        (GtkSpinButton    *spin_button);
static void       ligma_spin_scale_settings_notify      (GtkSettings      *settings,
                                                        const GParamSpec *pspec,
                                                        LigmaSpinScale    *scale);
static void       ligma_spin_scale_mnemonics_notify     (GtkWindow        *window,
                                                        const GParamSpec *pspec,
                                                        LigmaSpinScale    *scale);
static void       ligma_spin_scale_setup_mnemonic       (LigmaSpinScale    *scale,
                                                        guint             previous_keyval);

static gdouble    odd_pow                              (gdouble           x,
                                                        gdouble           y);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaSpinScale, ligma_spin_scale,
                            LIGMA_TYPE_SPIN_BUTTON)

#define parent_class ligma_spin_scale_parent_class


static void
ligma_spin_scale_class_init (LigmaSpinScaleClass *klass)
{
  GObjectClass       *object_class      = G_OBJECT_CLASS (klass);
  GtkWidgetClass     *widget_class      = GTK_WIDGET_CLASS (klass);
  GtkSpinButtonClass *spin_button_class = GTK_SPIN_BUTTON_CLASS (klass);

  object_class->dispose              = ligma_spin_scale_dispose;
  object_class->finalize             = ligma_spin_scale_finalize;
  object_class->set_property         = ligma_spin_scale_set_property;
  object_class->get_property         = ligma_spin_scale_get_property;

  widget_class->get_preferred_width  = ligma_spin_scale_get_preferred_width;
  widget_class->get_preferred_height = ligma_spin_scale_get_preferred_height;
  widget_class->style_updated        = ligma_spin_scale_style_updated;
  widget_class->draw                 = ligma_spin_scale_draw;
  widget_class->button_press_event   = ligma_spin_scale_button_press;
  widget_class->button_release_event = ligma_spin_scale_button_release;
  widget_class->motion_notify_event  = ligma_spin_scale_motion_notify;
  widget_class->leave_notify_event   = ligma_spin_scale_leave_notify;
  widget_class->hierarchy_changed    = ligma_spin_scale_hierarchy_changed;
  widget_class->screen_changed       = ligma_spin_scale_screen_changed;

  spin_button_class->value_changed   = ligma_spin_scale_value_changed;

  g_object_class_install_property (object_class, PROP_LABEL,
                                   g_param_spec_string ("label", NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE));

  gtk_widget_class_set_css_name (widget_class, "LigmaSpinScale");
}

static void
ligma_spin_scale_init (LigmaSpinScale *scale)
{
  LigmaSpinScalePrivate *private = GET_PRIVATE (scale);

  gtk_widget_add_events (GTK_WIDGET (scale),
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_BUTTON1_MOTION_MASK |
                         GDK_LEAVE_NOTIFY_MASK);

  gtk_entry_set_alignment (GTK_ENTRY (scale), 1.0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (scale), TRUE);

  private->mnemonic_keyval = GDK_KEY_VoidSymbol;
  private->gamma           = 1.0;
}

static void
ligma_spin_scale_dispose (GObject *object)
{
  LigmaSpinScalePrivate *private = GET_PRIVATE (object);
  guint                 keyval;

  keyval = private->mnemonic_keyval;
  private->mnemonic_keyval = GDK_KEY_VoidSymbol;

  ligma_spin_scale_setup_mnemonic (LIGMA_SPIN_SCALE (object), keyval);

  g_clear_object (&private->layout);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_spin_scale_finalize (GObject *object)
{
  LigmaSpinScalePrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->label,         g_free);
  g_clear_pointer (&private->label_text,    g_free);
  g_clear_pointer (&private->label_pattern, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_spin_scale_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  LigmaSpinScale *scale = LIGMA_SPIN_SCALE (object);

  switch (property_id)
    {
    case PROP_LABEL:
      ligma_spin_scale_set_label (scale, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_spin_scale_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  LigmaSpinScale *scale = LIGMA_SPIN_SCALE (object);

  switch (property_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, ligma_spin_scale_get_label (scale));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_spin_scale_get_preferred_width (GtkWidget *widget,
                                     gint      *minimum_width,
                                     gint      *natural_width)
{
  LigmaSpinScalePrivate *private = GET_PRIVATE (widget);
  PangoContext         *context = gtk_widget_get_pango_context (widget);
  PangoFontMetrics     *metrics;

  GTK_WIDGET_CLASS (parent_class)->get_preferred_width (widget,
                                                        minimum_width,
                                                        natural_width);

  metrics = pango_context_get_metrics (context,
                                       pango_context_get_font_description (context),
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
ligma_spin_scale_get_preferred_height (GtkWidget *widget,
                                      gint      *minimum_height,
                                      gint      *natural_height)
{
  GTK_WIDGET_CLASS (parent_class)->get_preferred_height (widget,
                                                         minimum_height,
                                                         natural_height);
}

static void
ligma_spin_scale_style_updated (GtkWidget *widget)
{
  LigmaSpinScalePrivate *private = GET_PRIVATE (widget);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  g_clear_object (&private->layout);
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
      while (*p && q && *q != '_')
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
ligma_spin_scale_draw (GtkWidget *widget,
                      cairo_t   *cr)
{
  LigmaSpinScalePrivate *private = GET_PRIVATE (widget);

  cairo_save (cr);
  GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);
  cairo_restore (cr);

  if (private->label)
    {
      GtkStyleContext *style = gtk_widget_get_style_context (widget);
      GtkAllocation    allocation;
      GdkRectangle     text_area;
      GtkStateFlags    state;
      gint             minimum_width;
      gint             natural_width;
      PangoRectangle   ink;
      gint             layout_offset_x;
      gint             layout_offset_y;
      GdkRGBA          text_color;
      GdkRGBA          bar_text_color;
      gdouble          progress_fraction;
      gdouble          progress_x;
      gdouble          progress_y;
      gdouble          progress_width;
      gdouble          progress_height;

      gtk_widget_get_allocation (widget, &allocation);

      gtk_entry_get_text_area (GTK_ENTRY (widget), &text_area);

      state = gtk_widget_get_state_flags (widget);

      GTK_WIDGET_CLASS (parent_class)->get_preferred_width (widget,
                                                            &minimum_width,
                                                            &natural_width);

      if (! private->layout)
        {
          private->layout = gtk_widget_create_pango_layout (widget,
                                                            private->label_text);
          pango_layout_set_ellipsize (private->layout, PANGO_ELLIPSIZE_END);
          /* Needing to force right-to-left layout when the widget is
           * set so, even when the text is not RTL text. Without this,
           * such case is broken because on the left side, we'd have
           * both the value and the label texts.
           */
          pango_layout_set_auto_dir (private->layout, FALSE);

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
      pango_layout_get_pixel_extents (private->layout, &ink, NULL);

      gtk_entry_get_layout_offsets (GTK_ENTRY (widget), NULL, &layout_offset_y);

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        layout_offset_x = text_area.x + text_area.width - ink.width - 2;
      else
        layout_offset_x = text_area.x + 2;

      layout_offset_x -= ink.x;

      gtk_style_context_get_color (style, state, &text_color);

      gtk_style_context_save (style);
      gtk_style_context_add_class (style, GTK_STYLE_CLASS_PROGRESSBAR);
      gtk_style_context_get_color (style, state, &bar_text_color);
      gtk_style_context_restore (style);

      progress_fraction = gtk_entry_get_progress_fraction (GTK_ENTRY (widget));
      progress_y        = 0.0;
      progress_width    = (gdouble) text_area.width * progress_fraction;
      progress_height   = text_area.height;

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        progress_x = text_area.width + text_area.x - progress_width;
      else
        progress_x = text_area.x;

      cairo_save (cr);

      cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
      cairo_rectangle (cr, 0, 0, text_area.width, text_area.height);
      cairo_rectangle (cr, progress_x, progress_y,
                       progress_width, progress_height);
      cairo_clip (cr);
      cairo_set_fill_rule (cr, CAIRO_FILL_RULE_WINDING);

      /* Center the label on the widget text area. Do not necessarily
       * try to align the label with the number characters in the entry
       * layout, because the size might actually be different, in
       * particular when the label's actual font ends up different
       * (typically when displaying non-Western languages).
       */
      cairo_move_to (cr, layout_offset_x,
                     text_area.y - ink.y + text_area.height / 2 - ink.height / 2);
      gdk_cairo_set_source_rgba (cr, &text_color);
      pango_cairo_show_layout (cr, private->layout);

      cairo_restore (cr);

      cairo_rectangle (cr, progress_x, progress_y,
                       progress_width, progress_height);
      cairo_clip (cr);

      cairo_move_to (cr, layout_offset_x,
                     text_area.y - ink.y + text_area.height / 2 - ink.height / 2);
      gdk_cairo_set_source_rgba (cr, &bar_text_color);
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

  for (w = window;
       w && w != widget_window;
       w = gdk_window_get_parent (w))
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
ligma_spin_scale_event_to_widget_coords (GtkWidget *widget,
                                        GdkWindow *window,
                                        gdouble    event_x,
                                        gdouble    event_y,
                                        gdouble   *widget_x,
                                        gdouble   *widget_y)
{
  GdkRectangle text_area;
  gint         tx, ty;

  gtk_entry_get_text_area (GTK_ENTRY (widget), &text_area);

  if (gtk_widget_get_translation_to_window (widget, window, &tx, &ty))
    {
      event_x += tx;
      event_y += ty;
    }

  *widget_x = event_x - text_area.x;
  *widget_y = event_y - text_area.y;
}

static SpinScaleTarget
ligma_spin_scale_get_target (GtkWidget *widget,
                            gdouble    x,
                            gdouble    y,
                            GdkEvent  *event)
{
  GdkRectangle text_area;

  gtk_entry_get_text_area (GTK_ENTRY (widget), &text_area);

  if (x >= 0 && x < text_area.width &&
      y >= 0 && y < text_area.height)
    {
      PangoRectangle logical;
      gint           layout_x;
      gint           layout_y;

      if (! event)
        return TARGET_GRAB;

      gtk_entry_get_layout_offsets (GTK_ENTRY (widget), &layout_x, &layout_y);
      pango_layout_get_pixel_extents (gtk_entry_get_layout (GTK_ENTRY (widget)),
                                      NULL, &logical);

      layout_x -= text_area.x;
      layout_y -= text_area.y;

      if (event->type != GDK_MOTION_NOTIFY)
        {
          GdkEventButton *event_button = (GdkEventButton *) event;

          switch (event_button->button)
            {
            case 1:
              if (event_button->state & GDK_SHIFT_MASK)
                return TARGET_RELATIVE;
              else
                /* Button 1 target depends on the cursor position. */
                break;

            case 3:
              return TARGET_RELATIVE;

            default:
              return TARGET_NUMBER;
            }
        }
      else
        {
          GdkEventMotion *event_motion = (GdkEventMotion *) event;

          if (event_motion->state & GDK_SHIFT_MASK)
            return TARGET_RELATIVE;
        }

      /* For motion events or main button clicks, the target depends on
       * the position.
       */
      if (x >= layout_x && x < layout_x + logical.width  &&
          y >= layout_y && y < layout_y + logical.height)
        {
          return TARGET_NUMBER;
        }
      else if (event->type == GDK_MOTION_NOTIFY)
        {
          return TARGET_GRAB;
        }
      else
        {
          return TARGET_GRABBING;
        }
    }

  return TARGET_NONE;
}

static void
ligma_spin_scale_update_cursor (GtkWidget *widget,
                               GdkWindow *window)
{
  LigmaSpinScalePrivate *private = GET_PRIVATE (widget);
  GdkDisplay           *display = gtk_widget_get_display (widget);
  GdkCursor            *cursor  = NULL;

  switch (private->target)
    {
    case TARGET_NUMBER:
      cursor = gdk_cursor_new_from_name (display, "text");
      break;

    case TARGET_GRAB:
      cursor = gdk_cursor_new_from_name (display, "grab");
      break;

    case TARGET_GRABBING:
      cursor = gdk_cursor_new_from_name (display, "grabbing");
      break;

    case TARGET_RELATIVE:
      cursor = gdk_cursor_new_from_name (display, "col-resize");
      break;

    default:
      break;
    }

  gdk_window_set_cursor (window, cursor);

  g_clear_object (&cursor);
}

static void
ligma_spin_scale_update_target (GtkWidget *widget,
                               GdkWindow *window,
                               gdouble    x,
                               gdouble    y,
                               GdkEvent  *event)
{
  LigmaSpinScalePrivate *private = GET_PRIVATE (widget);
  SpinScaleTarget       target;

  target = ligma_spin_scale_get_target (widget, x, y, (GdkEvent *) event);

  if (target != private->target)
    {
      private->target = target;

      ligma_spin_scale_update_cursor (widget, window);
    }
}

static void
ligma_spin_scale_clear_target (GtkWidget *widget,
                              GdkWindow *window)
{
  LigmaSpinScalePrivate *private = GET_PRIVATE (widget);

  if (private->target != TARGET_NONE)
    {
      private->target = TARGET_NONE;

      ligma_spin_scale_update_cursor (widget, window);
    }
}

static void
ligma_spin_scale_get_limits (LigmaSpinScale *scale,
                            gdouble       *lower,
                            gdouble       *upper)
{
  LigmaSpinScalePrivate *private = GET_PRIVATE (scale);

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
ligma_spin_scale_change_value (GtkWidget *widget,
                              gdouble    x,
                              guint      state)
{
  LigmaSpinScalePrivate *private     = GET_PRIVATE (widget);
  GtkSpinButton        *spin_button = GTK_SPIN_BUTTON (widget);
  GtkAdjustment        *adjustment  = gtk_spin_button_get_adjustment (spin_button);
  GdkRectangle          text_area;
  gdouble               lower;
  gdouble               upper;
  gdouble               value;
  gint                  digits;
  gint                  power = 1;

  gtk_entry_get_text_area (GTK_ENTRY (widget), &text_area);

  ligma_spin_scale_get_limits (LIGMA_SPIN_SCALE (widget), &lower, &upper);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    x = text_area.width - x;

  if (private->relative_change)
    {
      gdouble step;

      step = (upper - lower) / text_area.width * RELATIVE_CHANGE_SPEED;

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        step *= x - (text_area.width - private->start_x);
      else
        step *= x - private->start_x;

      if (state & GDK_CONTROL_MASK)
        {
          gdouble page_inc = gtk_adjustment_get_page_increment (adjustment);

          step = RINT (step / page_inc) * page_inc;
        }

      value = private->start_value + step;
    }
  else
    {
      gdouble x0, x1;
      gdouble fraction;

      x0 = odd_pow (lower, 1.0 / private->gamma);
      x1 = odd_pow (upper, 1.0 / private->gamma);

      fraction = x / (gdouble) text_area.width;

      value = fraction * (x1 - x0) + x0;
      value = odd_pow (value, private->gamma);

      if (state & GDK_CONTROL_MASK)
        {
          gdouble page_inc = gtk_adjustment_get_page_increment (adjustment);

          value = RINT (value / page_inc) * page_inc;
        }
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

  if (private->constrain_drag)
    value = rint (value);

  gtk_adjustment_set_value (adjustment, value);
}

static gboolean
ligma_spin_scale_button_press (GtkWidget      *widget,
                              GdkEventButton *event)
{
  LigmaSpinScalePrivate *private = GET_PRIVATE (widget);
  gdouble               x, y;

  private->changing_value  = FALSE;
  private->relative_change = FALSE;
  private->pointer_warp    = FALSE;

  ligma_spin_scale_event_to_widget_coords (widget, event->window,
                                          event->x, event->y,
                                          &x, &y);

  ligma_spin_scale_update_target (widget, event->window, x, y, (GdkEvent *) event);

  switch (private->target)
    {
    case TARGET_GRAB:
    case TARGET_GRABBING:
      private->changing_value = TRUE;

      gtk_widget_grab_focus (widget);

      ligma_spin_scale_change_value (widget, x, event->state);

      ligma_spin_scale_update_cursor (widget, event->window);

      return TRUE;

    case TARGET_RELATIVE:
      private->changing_value = TRUE;

      gtk_widget_grab_focus (widget);

      private->relative_change = TRUE;
      private->start_x = x;
      private->start_value = gtk_adjustment_get_value (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget)));

      private->start_pointer_x = floor (event->x_root);
      private->start_pointer_y = floor (event->y_root);

      ligma_spin_scale_update_cursor (widget, event->window);

      return TRUE;

    case TARGET_NUMBER:
      gtk_widget_grab_focus (widget);
      return TRUE;

    default:
      break;
    }

  return GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, event);
}

static gboolean
ligma_spin_scale_button_release (GtkWidget      *widget,
                                GdkEventButton *event)
{
  LigmaSpinScalePrivate *private = GET_PRIVATE (widget);
  gdouble               x, y;

  ligma_spin_scale_event_to_widget_coords (widget, event->window,
                                          event->x, event->y,
                                          &x, &y);

  if (private->changing_value)
    {
      private->changing_value = FALSE;

      /* don't change the value if we're in the middle of a pointer warp, since
       * we didn't adjust start_x yet.  see the comment in
       * ligma_spin_scale_motion_notify().
       */
      if (! private->pointer_warp)
        ligma_spin_scale_change_value (widget, x, event->state);

      if (private->relative_change)
        {
          gdk_device_warp (gdk_event_get_device ((GdkEvent *) event),
                           gdk_event_get_screen ((GdkEvent *) event),
                           private->start_pointer_x,
                           private->start_pointer_y);
        }

      if (private->hover)
        {
          ligma_spin_scale_update_target (widget, event->window,
                                         0.0, 0.0, NULL);
        }
      else
        {
          ligma_spin_scale_clear_target (widget, event->window);
        }

      gtk_widget_queue_draw (widget);

      return TRUE;
    }

  return GTK_WIDGET_CLASS (parent_class)->button_release_event (widget, event);
}

static gboolean
ligma_spin_scale_motion_notify (GtkWidget      *widget,
                               GdkEventMotion *event)
{
  LigmaSpinScalePrivate *private = GET_PRIVATE (widget);
  gdouble               x, y;

  ligma_spin_scale_event_to_widget_coords (widget, event->window,
                                          event->x, event->y,
                                          &x, &y);

  gdk_event_request_motions (event);

  private->hover = TRUE;

  if (private->changing_value)
    {
      GdkScreen    *screen;
      GdkDisplay   *display;
      gint          pointer_x;
      gint          pointer_y;
      GdkMonitor   *monitor;
      GdkRectangle  monitor_geometry;

      screen  = gdk_event_get_screen ((GdkEvent *) event);
      display = gdk_screen_get_display (screen);

      pointer_x = floor (event->x_root);
      pointer_y = floor (event->y_root);

      monitor = gdk_display_get_monitor_at_point (display, pointer_x, pointer_y);
      gdk_monitor_get_geometry (monitor, &monitor_geometry);

      /* when applying a relative change, we wrap the pointer around the left
       * and right edges of the current monitor, so that the adjustment is not
       * limited by the monitor geometry.  when the pointer reaches one of the
       * monitor edges, we move it one pixel away from the opposite edge, so
       * that it can be subsequently moved in the other direction, and adjust
       * start_x accordingly.
       *
       * unfortunately, we can't rely on gdk_device_warp() to actually
       * move the pointer (for example, it doesn't work on wayland), and
       * there's no easy way to tell whether the pointer moved or not.  in
       * particular, even when the pointer doesn't move, gdk still simulates a
       * motion event, and reports the "new" pointer position until a real
       * motion event occurs.
       *
       * in order not to erroneously adjust start_x when
       * gdk_device_warp() fails, we remember that we *tried* to warp
       * the pointer, and defer the actual adjustment of start_x until
       * a future motion event, where the pointer's x coordinate is
       * different from the one passed to gdk_device_warp().  when
       * that happens, we "guess" whether the pointer got warped or
       * not by comparing its x coordinate to the one passed to
       * gdk_device_warp(): if their difference is less than half the
       * monitor width, then we assume the pointer got warped
       * (otherwise, the user must have very quickly moved the mouse
       * across half the screen.)  yes, this is an ugly ugly hack :)
       */

      if (private->pointer_warp)
        {
          if (pointer_x == private->pointer_warp_x)
            return TRUE;

          private->pointer_warp = FALSE;

          if (ABS (pointer_x - private->pointer_warp_x) < monitor_geometry.width / 2)
            private->start_x = private->pointer_warp_start_x;
        }

      ligma_spin_scale_change_value (widget, x, event->state);

      if (private->relative_change)
        {
          GtkAdjustment *adjustment;
          gdouble        value;
          gdouble        lower;
          gdouble        upper;

          adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget));

          value = gtk_adjustment_get_value (adjustment);
          lower = gtk_adjustment_get_lower (adjustment);
          upper = gtk_adjustment_get_upper (adjustment);

          if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
            {
              gdouble temp;

              value = -value;

              temp  = lower;
              lower = -upper;
              upper = -temp;
            }

          if (pointer_x <= monitor_geometry.x &&
              value > lower)
            {
              private->pointer_warp         = TRUE;
              private->pointer_warp_x       = (monitor_geometry.width - 1) + pointer_x - 1;
              private->pointer_warp_start_x = private->start_x + (monitor_geometry.width - 2);
            }
          else if (pointer_x >= monitor_geometry.x + (monitor_geometry.width - 1) &&
                   value < upper)
            {
              private->pointer_warp         = TRUE;
              private->pointer_warp_x       = pointer_x - (monitor_geometry.width - 1) + 1;
              private->pointer_warp_start_x = private->start_x - (monitor_geometry.width - 2);
            }

          if (private->pointer_warp)
            {
              gdk_device_warp (gdk_event_get_device ((GdkEvent *) event),
                               screen,
                               private->pointer_warp_x,
                               pointer_y);
            }
        }

      return TRUE;
    }

  GTK_WIDGET_CLASS (parent_class)->motion_notify_event (widget, event);

  if (! (event->state &
         (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)) &&
      private->hover)
    {
      ligma_spin_scale_update_target (widget, event->window,
                                     x, y, (GdkEvent *) event);
    }

  return FALSE;
}

static gboolean
ligma_spin_scale_leave_notify (GtkWidget        *widget,
                              GdkEventCrossing *event)
{
  LigmaSpinScalePrivate *private = GET_PRIVATE (widget);

  private->hover = FALSE;

  if (! (event->state &
         (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)))
    {
      ligma_spin_scale_clear_target (widget, event->window);
    }

  return GTK_WIDGET_CLASS (parent_class)->leave_notify_event (widget, event);
}

static void
ligma_spin_scale_hierarchy_changed (GtkWidget *widget,
                                   GtkWidget *old_toplevel)
{
  LigmaSpinScalePrivate *private = GET_PRIVATE (widget);

  ligma_spin_scale_setup_mnemonic (LIGMA_SPIN_SCALE (widget),
                                  private->mnemonic_keyval);
}

static void
ligma_spin_scale_screen_changed (GtkWidget *widget,
                                GdkScreen *old_screen)
{
  LigmaSpinScale        *scale   = LIGMA_SPIN_SCALE (widget);
  LigmaSpinScalePrivate *private = GET_PRIVATE (scale);
  GtkSettings          *settings;

  g_clear_object (&private->layout);

  if (old_screen)
    {
      settings = gtk_settings_get_for_screen (old_screen);

      g_signal_handlers_disconnect_by_func (settings,
                                            ligma_spin_scale_settings_notify,
                                            scale);
    }

  if (! gtk_widget_has_screen (widget))
    return;

  settings = gtk_widget_get_settings (widget);

  g_signal_connect (settings, "notify::gtk-enable-mnemonics",
                    G_CALLBACK (ligma_spin_scale_settings_notify),
                    scale);
  g_signal_connect (settings, "notify::gtk-enable-accels",
                    G_CALLBACK (ligma_spin_scale_settings_notify),
                    scale);

  ligma_spin_scale_settings_notify (settings, NULL, scale);
}

static void
ligma_spin_scale_value_changed (GtkSpinButton *spin_button)
{
  LigmaSpinScalePrivate *private    = GET_PRIVATE (spin_button);
  GtkAdjustment        *adjustment = gtk_spin_button_get_adjustment (spin_button);
  gdouble               lower;
  gdouble               upper;
  gdouble               value;
  gdouble               x0, x1;
  gdouble               x;

  ligma_spin_scale_get_limits (LIGMA_SPIN_SCALE (spin_button), &lower, &upper);

  value = CLAMP (gtk_adjustment_get_value (adjustment), lower, upper);

  x0 = odd_pow (lower, 1.0 / private->gamma);
  x1 = odd_pow (upper, 1.0 / private->gamma);
  x  = odd_pow (value, 1.0 / private->gamma);

  gtk_entry_set_progress_fraction (GTK_ENTRY (spin_button),
                                   (x - x0) / (x1 - x0));
}

static void
ligma_spin_scale_settings_notify (GtkSettings      *settings,
                                 const GParamSpec *pspec,
                                 LigmaSpinScale    *scale)
{
  GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (scale));

  if (GTK_IS_WINDOW (toplevel))
    ligma_spin_scale_mnemonics_notify (GTK_WINDOW (toplevel), NULL, scale);
}

static void
ligma_spin_scale_mnemonics_notify (GtkWindow        *window,
                                  const GParamSpec *pspec,
                                  LigmaSpinScale    *scale)
{
  LigmaSpinScalePrivate *private           = GET_PRIVATE (scale);
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

      g_clear_object (&private->layout);

      gtk_widget_queue_draw (GTK_WIDGET (scale));
    }
}

static void
ligma_spin_scale_setup_mnemonic (LigmaSpinScale *scale,
                                guint          previous_keyval)
{
  LigmaSpinScalePrivate *private = GET_PRIVATE (scale);
  GtkWidget            *widget  = GTK_WIDGET (scale);
  GtkWidget            *toplevel;

  if (private->mnemonic_window)
    {
      g_signal_handlers_disconnect_by_func (private->mnemonic_window,
                                            ligma_spin_scale_mnemonics_notify,
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
                        G_CALLBACK (ligma_spin_scale_mnemonics_notify),
                        scale);
     }
}

static gdouble
odd_pow (gdouble x,
         gdouble y)
{
  if (x >= 0.0)
    return pow (x, y);
  else
    return -pow (-x, y);
}


/*  public functions  */

GtkWidget *
ligma_spin_scale_new (GtkAdjustment *adjustment,
                     const gchar   *label,
                     gint           digits)
{
  g_return_val_if_fail (GTK_IS_ADJUSTMENT (adjustment), NULL);

  return g_object_new (LIGMA_TYPE_SPIN_SCALE,
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
ligma_spin_scale_set_label (LigmaSpinScale *scale,
                           const gchar   *label)
{
  LigmaSpinScalePrivate *private;
  guint                 accel_key = GDK_KEY_VoidSymbol;
  gchar                *text      = NULL;
  gchar                *pattern   = NULL;

  g_return_if_fail (LIGMA_IS_SPIN_SCALE (scale));

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

      ligma_spin_scale_setup_mnemonic (scale, previous);
    }

  g_clear_object (&private->layout);

  gtk_widget_queue_resize (GTK_WIDGET (scale));

  g_object_notify (G_OBJECT (scale), "label");
}

const gchar *
ligma_spin_scale_get_label (LigmaSpinScale *scale)
{
  g_return_val_if_fail (LIGMA_IS_SPIN_SCALE (scale), NULL);

  return GET_PRIVATE (scale)->label;
}

void
ligma_spin_scale_set_scale_limits (LigmaSpinScale *scale,
                                  gdouble        lower,
                                  gdouble        upper)
{
  LigmaSpinScalePrivate *private;
  GtkSpinButton        *spin_button;
  GtkAdjustment        *adjustment;

  g_return_if_fail (LIGMA_IS_SPIN_SCALE (scale));

  private     = GET_PRIVATE (scale);
  spin_button = GTK_SPIN_BUTTON (scale);
  adjustment  = gtk_spin_button_get_adjustment (spin_button);

  g_return_if_fail (lower >= gtk_adjustment_get_lower (adjustment));
  g_return_if_fail (upper <= gtk_adjustment_get_upper (adjustment));

  private->scale_limits_set = TRUE;
  private->scale_lower      = lower;
  private->scale_upper      = upper;
  private->gamma            = 1.0;

  ligma_spin_scale_value_changed (spin_button);
}

void
ligma_spin_scale_unset_scale_limits (LigmaSpinScale *scale)
{
  LigmaSpinScalePrivate *private;

  g_return_if_fail (LIGMA_IS_SPIN_SCALE (scale));

  private = GET_PRIVATE (scale);

  private->scale_limits_set = FALSE;
  private->scale_lower      = 0.0;
  private->scale_upper      = 0.0;

  ligma_spin_scale_value_changed (GTK_SPIN_BUTTON (scale));
}

gboolean
ligma_spin_scale_get_scale_limits (LigmaSpinScale *scale,
                                  gdouble       *lower,
                                  gdouble       *upper)
{
  LigmaSpinScalePrivate *private;

  g_return_val_if_fail (LIGMA_IS_SPIN_SCALE (scale), FALSE);

  private = GET_PRIVATE (scale);

  if (lower)
    *lower = private->scale_lower;

  if (upper)
    *upper = private->scale_upper;

  return private->scale_limits_set;
}

void
ligma_spin_scale_set_gamma (LigmaSpinScale *scale,
                           gdouble        gamma)
{
  LigmaSpinScalePrivate *private;

  g_return_if_fail (LIGMA_IS_SPIN_SCALE (scale));

  private = GET_PRIVATE (scale);

  private->gamma = gamma;

  ligma_spin_scale_value_changed (GTK_SPIN_BUTTON (scale));
}

gdouble
ligma_spin_scale_get_gamma (LigmaSpinScale *scale)
{
  g_return_val_if_fail (LIGMA_IS_SPIN_SCALE (scale), 1.0);

  return GET_PRIVATE (scale)->gamma;
}

/**
 * ligma_spin_scale_set_constrain_drag:
 * @scale: the #LigmaSpinScale.
 * @constrain: whether constraining to integer values when dragging with
 *             pointer.
 *
 * If @constrain_drag is TRUE, dragging the scale with the pointer will
 * only result into integer values. It will still possible to set the
 * scale to fractional values (if the spin scale "digits" is above 0)
 * for instance with keyboard edit.
 */
void
ligma_spin_scale_set_constrain_drag (LigmaSpinScale *scale,
                                    gboolean       constrain)
{
  LigmaSpinScalePrivate *private;

  g_return_if_fail (LIGMA_IS_SPIN_SCALE (scale));

  private = GET_PRIVATE (scale);

  private->constrain_drag = constrain;
}

gboolean
ligma_spin_scale_get_constrain_drag (LigmaSpinScale *scale)
{
  g_return_val_if_fail (LIGMA_IS_SPIN_SCALE (scale), 1.0);

  return GET_PRIVATE (scale)->constrain_drag;
}
