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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "gimpwidgetstypes.h"

#include "gimpspinscale.h"

#include "libgimp/libgimp-intl.h"


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


struct _GimpSpinScale
{
  GimpSpinButton   parent_instance;

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
static void       gimp_spin_scale_style_updated        (GtkWidget        *widget);
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

static gdouble    odd_pow                              (gdouble           x,
                                                        gdouble           y);


G_DEFINE_TYPE (GimpSpinScale, gimp_spin_scale, GIMP_TYPE_SPIN_BUTTON)

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
  widget_class->style_updated        = gimp_spin_scale_style_updated;
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

  gtk_widget_class_set_css_name (widget_class, "GimpSpinScale");
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
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (scale), TRUE);

  scale->mnemonic_keyval = GDK_KEY_VoidSymbol;
  scale->gamma           = 1.0;
}

static void
gimp_spin_scale_dispose (GObject *object)
{
  GimpSpinScale *scale = GIMP_SPIN_SCALE (object);
  guint          keyval;

  keyval = scale->mnemonic_keyval;
  scale->mnemonic_keyval = GDK_KEY_VoidSymbol;

  gimp_spin_scale_setup_mnemonic (GIMP_SPIN_SCALE (object), keyval);

  g_clear_object (&scale->layout);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_spin_scale_finalize (GObject *object)
{
  GimpSpinScale *scale = GIMP_SPIN_SCALE (object);

  g_clear_pointer (&scale->label,         g_free);
  g_clear_pointer (&scale->label_text,    g_free);
  g_clear_pointer (&scale->label_pattern, g_free);

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
  GimpSpinScale    *scale = GIMP_SPIN_SCALE (widget);
  PangoContext     *context = gtk_widget_get_pango_context (widget);
  PangoFontMetrics *metrics;

  GTK_WIDGET_CLASS (parent_class)->get_preferred_width (widget,
                                                        minimum_width,
                                                        natural_width);

  metrics = pango_context_get_metrics (context,
                                       pango_context_get_font_description (context),
                                       pango_context_get_language (context));

  if (scale->label)
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
  GTK_WIDGET_CLASS (parent_class)->get_preferred_height (widget,
                                                         minimum_height,
                                                         natural_height);
}

static void
gimp_spin_scale_style_updated (GtkWidget *widget)
{
  GimpSpinScale *scale = GIMP_SPIN_SCALE (widget);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  g_clear_object (&scale->layout);
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
gimp_spin_scale_draw (GtkWidget *widget,
                      cairo_t   *cr)
{
  GimpSpinScale *scale = GIMP_SPIN_SCALE (widget);

  cairo_save (cr);
  GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);
  cairo_restore (cr);

  if (scale->label)
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

      if (! scale->layout)
        {
          scale->layout = gtk_widget_create_pango_layout (widget,
                                                          scale->label_text);
          pango_layout_set_ellipsize (scale->layout, PANGO_ELLIPSIZE_END);
          /* Needing to force right-to-left layout when the widget is
           * set so, even when the text is not RTL text. Without this,
           * such case is broken because on the left side, we'd have
           * both the value and the label texts.
           */
          pango_layout_set_auto_dir (scale->layout, FALSE);

          if (scale->mnemonics_visible)
            {
              PangoAttrList *attrs;

              attrs = pattern_to_attrs (scale->label_text,
                                        scale->label_pattern);
              if (attrs)
                {
                  pango_layout_set_attributes (scale->layout, attrs);
                  pango_attr_list_unref (attrs);
                }
            }
        }

      pango_layout_set_width (scale->layout,
                              PANGO_SCALE *
                              (allocation.width - minimum_width));
      pango_layout_get_pixel_extents (scale->layout, &ink, NULL);

      gtk_entry_get_layout_offsets (GTK_ENTRY (widget), NULL, &layout_offset_y);

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        layout_offset_x = text_area.x + text_area.width - ink.width - 6;
      else
        layout_offset_x = text_area.x + 6;

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
      pango_cairo_show_layout (cr, scale->layout);

      cairo_restore (cr);

      cairo_rectangle (cr, progress_x, progress_y,
                       progress_width, progress_height);
      cairo_clip (cr);

      cairo_move_to (cr, layout_offset_x,
                     text_area.y - ink.y + text_area.height / 2 - ink.height / 2);
      gdk_cairo_set_source_rgba (cr, &bar_text_color);
      pango_cairo_show_layout (cr, scale->layout);
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
gimp_spin_scale_event_to_widget_coords (GtkWidget *widget,
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
gimp_spin_scale_get_target (GtkWidget *widget,
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
      if (x >= layout_x && x < layout_x + logical.width &&
          y >= (layout_y + logical.height)/4 && y < 3 * (layout_y + logical.height)/4)
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
gimp_spin_scale_update_cursor (GtkWidget *widget,
                               GdkWindow *window)
{
  GimpSpinScale *scale   = GIMP_SPIN_SCALE (widget);
  GdkDisplay    *display = gtk_widget_get_display (widget);
  GdkCursor     *cursor  = NULL;

  switch (scale->target)
    {
    case TARGET_NUMBER:
      cursor = gdk_cursor_new_from_name (display, "text");
      break;

    case TARGET_GRAB:
      cursor = gdk_cursor_new_from_name (display, "pointer");
      break;

    case TARGET_GRABBING:
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
gimp_spin_scale_update_target (GtkWidget *widget,
                               GdkWindow *window,
                               gdouble    x,
                               gdouble    y,
                               GdkEvent  *event)
{
  GimpSpinScale   *scale = GIMP_SPIN_SCALE (widget);
  SpinScaleTarget  target;

  target = gimp_spin_scale_get_target (widget, x, y, (GdkEvent *) event);

  if (target != scale->target)
    {
      scale->target = target;

      gimp_spin_scale_update_cursor (widget, window);
    }
}

static void
gimp_spin_scale_clear_target (GtkWidget *widget,
                              GdkWindow *window)
{
  GimpSpinScale *scale = GIMP_SPIN_SCALE (widget);

  if (scale->target != TARGET_NONE)
    {
      scale->target = TARGET_NONE;

      gimp_spin_scale_update_cursor (widget, window);
    }
}

static void
gimp_spin_scale_get_limits (GimpSpinScale *scale,
                            gdouble       *lower,
                            gdouble       *upper)
{
  if (scale->scale_limits_set)
    {
      *lower = scale->scale_lower;
      *upper = scale->scale_upper;
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
                              gdouble    x,
                              guint      state)
{
  GimpSpinScale  *scale = GIMP_SPIN_SCALE (widget);
  GtkSpinButton  *spin_button = GTK_SPIN_BUTTON (widget);
  GtkAdjustment  *adjustment  = gtk_spin_button_get_adjustment (spin_button);
  GdkRectangle    text_area;
  gdouble         lower;
  gdouble         upper;
  gdouble         value;
  gint            digits;
  gint            power = 1;

  gtk_entry_get_text_area (GTK_ENTRY (widget), &text_area);

  gimp_spin_scale_get_limits (GIMP_SPIN_SCALE (widget), &lower, &upper);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    x = text_area.width - x;

  if (scale->relative_change)
    {
      gdouble step;

      step = (upper - lower) / text_area.width * RELATIVE_CHANGE_SPEED;

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        step *= x - (text_area.width - scale->start_x);
      else
        step *= x - scale->start_x;

      if (state & GDK_CONTROL_MASK)
        {
          gdouble page_inc = gtk_adjustment_get_page_increment (adjustment);

          step = RINT (step / page_inc) * page_inc;
        }

      value = scale->start_value + step;
    }
  else
    {
      gdouble x0, x1;
      gdouble fraction;

      x0 = odd_pow (lower, 1.0 / scale->gamma);
      x1 = odd_pow (upper, 1.0 / scale->gamma);

      fraction = x / (gdouble) text_area.width;

      value = fraction * (x1 - x0) + x0;
      value = odd_pow (value, scale->gamma);

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

  if (scale->constrain_drag)
    value = rint (value);

  gtk_adjustment_set_value (adjustment, value);
}

static gboolean
gimp_spin_scale_button_press (GtkWidget      *widget,
                              GdkEventButton *event)
{
  GimpSpinScale *scale = GIMP_SPIN_SCALE (widget);
  gdouble        x, y;

  scale->changing_value  = FALSE;
  scale->relative_change = FALSE;
  scale->pointer_warp    = FALSE;

  gimp_spin_scale_event_to_widget_coords (widget, event->window,
                                          event->x, event->y,
                                          &x, &y);

  gimp_spin_scale_update_target (widget, event->window, x, y, (GdkEvent *) event);

  switch (scale->target)
    {
    case TARGET_GRAB:
    case TARGET_GRABBING:
      scale->changing_value = TRUE;

      gtk_widget_grab_focus (widget);

      gimp_spin_scale_change_value (widget, x, event->state);

      gimp_spin_scale_update_cursor (widget, event->window);

      return TRUE;

    case TARGET_RELATIVE:
      scale->changing_value = TRUE;

      gtk_widget_grab_focus (widget);

      scale->relative_change = TRUE;
      scale->start_x = x;
      scale->start_value = gtk_adjustment_get_value (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget)));

      scale->start_pointer_x = floor (event->x_root);
      scale->start_pointer_y = floor (event->y_root);

      gimp_spin_scale_update_cursor (widget, event->window);

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
gimp_spin_scale_button_release (GtkWidget      *widget,
                                GdkEventButton *event)
{
  GimpSpinScale *scale = GIMP_SPIN_SCALE (widget);
  gdouble        x, y;

  gimp_spin_scale_event_to_widget_coords (widget, event->window,
                                          event->x, event->y,
                                          &x, &y);

  if (scale->changing_value)
    {
      scale->changing_value = FALSE;

      /* don't change the value if we're in the middle of a pointer warp, since
       * we didn't adjust start_x yet.  see the comment in
       * gimp_spin_scale_motion_notify().
       */
      if (! scale->pointer_warp)
        gimp_spin_scale_change_value (widget, x, event->state);

      if (scale->relative_change)
        {
          gdk_device_warp (gdk_event_get_device ((GdkEvent *) event),
                           gdk_event_get_screen ((GdkEvent *) event),
                           scale->start_pointer_x,
                           scale->start_pointer_y);
        }

      if (scale->hover)
        {
          gimp_spin_scale_update_target (widget, event->window,
                                         0.0, 0.0, NULL);
        }
      else
        {
          gimp_spin_scale_clear_target (widget, event->window);
        }

      gtk_widget_queue_draw (widget);

      return TRUE;
    }

  return GTK_WIDGET_CLASS (parent_class)->button_release_event (widget, event);
}

static gboolean
gimp_spin_scale_motion_notify (GtkWidget      *widget,
                               GdkEventMotion *event)
{
  GimpSpinScale *scale = GIMP_SPIN_SCALE (widget);
  gdouble        x, y;

  gimp_spin_scale_event_to_widget_coords (widget, event->window,
                                          event->x, event->y,
                                          &x, &y);

  gdk_event_request_motions (event);

  scale->hover = TRUE;

  if (scale->changing_value)
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

      if (scale->pointer_warp)
        {
          if (pointer_x == scale->pointer_warp_x)
            return TRUE;

          scale->pointer_warp = FALSE;

          if (ABS (pointer_x - scale->pointer_warp_x) < monitor_geometry.width / 2)
            scale->start_x = scale->pointer_warp_start_x;
        }

      gimp_spin_scale_change_value (widget, x, event->state);

      if (scale->relative_change)
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
              scale->pointer_warp         = TRUE;
              scale->pointer_warp_x       = (monitor_geometry.width - 1) + pointer_x - 1;
              scale->pointer_warp_start_x = scale->start_x + (monitor_geometry.width - 2);
            }
          else if (pointer_x >= monitor_geometry.x + (monitor_geometry.width - 1) &&
                   value < upper)
            {
              scale->pointer_warp         = TRUE;
              scale->pointer_warp_x       = pointer_x - (monitor_geometry.width - 1) + 1;
              scale->pointer_warp_start_x = scale->start_x - (monitor_geometry.width - 2);
            }

          if (scale->pointer_warp)
            {
              gdk_device_warp (gdk_event_get_device ((GdkEvent *) event),
                               screen,
                               scale->pointer_warp_x,
                               pointer_y);
            }
        }

      return TRUE;
    }

  GTK_WIDGET_CLASS (parent_class)->motion_notify_event (widget, event);

  if (! (event->state &
         (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)) &&
      scale->hover)
    {
      gimp_spin_scale_update_target (widget, event->window,
                                     x, y, (GdkEvent *) event);
    }

  return FALSE;
}

static gboolean
gimp_spin_scale_leave_notify (GtkWidget        *widget,
                              GdkEventCrossing *event)
{
  GimpSpinScale *scale = GIMP_SPIN_SCALE (widget);

  scale->hover = FALSE;

  if (! (event->state &
         (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)))
    {
      gimp_spin_scale_clear_target (widget, event->window);
    }

  return GTK_WIDGET_CLASS (parent_class)->leave_notify_event (widget, event);
}

static void
gimp_spin_scale_hierarchy_changed (GtkWidget *widget,
                                   GtkWidget *old_toplevel)
{
  GimpSpinScale *scale = GIMP_SPIN_SCALE (widget);

  gimp_spin_scale_setup_mnemonic (GIMP_SPIN_SCALE (widget),
                                  scale->mnemonic_keyval);
}

static void
gimp_spin_scale_screen_changed (GtkWidget *widget,
                                GdkScreen *old_screen)
{
  GimpSpinScale *scale   = GIMP_SPIN_SCALE (widget);
  GtkSettings   *settings;

  g_clear_object (&scale->layout);

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
  GimpSpinScale *scale = GIMP_SPIN_SCALE (spin_button);
  GtkAdjustment *adjustment = gtk_spin_button_get_adjustment (spin_button);
  gdouble        lower;
  gdouble        upper;
  gdouble        value;
  gdouble        x0, x1;
  gdouble        x;

  gimp_spin_scale_get_limits (GIMP_SPIN_SCALE (spin_button), &lower, &upper);

  value = CLAMP (gtk_adjustment_get_value (adjustment), lower, upper);

  x0 = odd_pow (lower, 1.0 / scale->gamma);
  x1 = odd_pow (upper, 1.0 / scale->gamma);
  x  = odd_pow (value, 1.0 / scale->gamma);

  gtk_entry_set_progress_fraction (GTK_ENTRY (spin_button),
                                   (x - x0) / (x1 - x0));
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
  gboolean mnemonics_visible = FALSE;
  gboolean enable_mnemonics;
  gboolean auto_mnemonics;

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

  if (scale->mnemonics_visible != mnemonics_visible)
    {
      scale->mnemonics_visible = mnemonics_visible;

      g_clear_object (&scale->layout);

      gtk_widget_queue_draw (GTK_WIDGET (scale));
    }
}

static void
gimp_spin_scale_setup_mnemonic (GimpSpinScale *scale,
                                guint          previous_keyval)
{
  GtkWidget *widget  = GTK_WIDGET (scale);
  GtkWidget *toplevel;

  if (scale->mnemonic_window)
    {
      g_signal_handlers_disconnect_by_func (scale->mnemonic_window,
                                            gimp_spin_scale_mnemonics_notify,
                                            scale);

      gtk_window_remove_mnemonic (scale->mnemonic_window,
                                  previous_keyval,
                                  widget);
      scale->mnemonic_window = NULL;
    }

  toplevel = gtk_widget_get_toplevel (widget);

  if (gtk_widget_is_toplevel (toplevel) &&
      scale->mnemonic_keyval != GDK_KEY_VoidSymbol)
    {
      gtk_window_add_mnemonic (GTK_WINDOW (toplevel),
                               scale->mnemonic_keyval,
                               widget);
      scale->mnemonic_window = GTK_WINDOW (toplevel);

      g_signal_connect (toplevel, "notify::mnemonics-visible",
                        G_CALLBACK (gimp_spin_scale_mnemonics_notify),
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
  guint  accel_key = GDK_KEY_VoidSymbol;
  gchar *text      = NULL;
  gchar *pattern   = NULL;

  g_return_if_fail (GIMP_IS_SPIN_SCALE (scale));

  if (label == scale->label)
    return;

  if (label && ! separate_uline_pattern (label, &accel_key, &text, &pattern))
    return;

  g_free (scale->label);
  scale->label = g_strdup (label);

  g_free (scale->label_text);
  scale->label_text = text; /* don't dup */

  g_free (scale->label_pattern);
  scale->label_pattern = pattern; /* don't dup */

  if (scale->mnemonic_keyval != accel_key)
    {
      guint previous = scale->mnemonic_keyval;

      scale->mnemonic_keyval = accel_key;

      gimp_spin_scale_setup_mnemonic (scale, previous);
    }

  g_clear_object (&scale->layout);

  gtk_widget_queue_resize (GTK_WIDGET (scale));

  g_object_notify (G_OBJECT (scale), "label");
}

const gchar *
gimp_spin_scale_get_label (GimpSpinScale *scale)
{
  g_return_val_if_fail (GIMP_IS_SPIN_SCALE (scale), NULL);

  return scale->label;
}

void
gimp_spin_scale_set_scale_limits (GimpSpinScale *scale,
                                  gdouble        lower,
                                  gdouble        upper)
{
  GtkSpinButton *spin_button;
  GtkAdjustment *adjustment;

  g_return_if_fail (GIMP_IS_SPIN_SCALE (scale));

  spin_button = GTK_SPIN_BUTTON (scale);
  adjustment  = gtk_spin_button_get_adjustment (spin_button);

  g_return_if_fail (lower >= gtk_adjustment_get_lower (adjustment));
  g_return_if_fail (upper <= gtk_adjustment_get_upper (adjustment));

  scale->scale_limits_set = TRUE;
  scale->scale_lower      = lower;
  scale->scale_upper      = upper;
  scale->gamma            = 1.0;

  gimp_spin_scale_value_changed (spin_button);
}

void
gimp_spin_scale_unset_scale_limits (GimpSpinScale *scale)
{
  g_return_if_fail (GIMP_IS_SPIN_SCALE (scale));

  scale->scale_limits_set = FALSE;
  scale->scale_lower      = 0.0;
  scale->scale_upper      = 0.0;

  gimp_spin_scale_value_changed (GTK_SPIN_BUTTON (scale));
}

gboolean
gimp_spin_scale_get_scale_limits (GimpSpinScale *scale,
                                  gdouble       *lower,
                                  gdouble       *upper)
{
  g_return_val_if_fail (GIMP_IS_SPIN_SCALE (scale), FALSE);

  if (lower)
    *lower = scale->scale_lower;

  if (upper)
    *upper = scale->scale_upper;

  return scale->scale_limits_set;
}

void
gimp_spin_scale_set_gamma (GimpSpinScale *scale,
                           gdouble        gamma)
{
  g_return_if_fail (GIMP_IS_SPIN_SCALE (scale));

  scale->gamma = gamma;

  gimp_spin_scale_value_changed (GTK_SPIN_BUTTON (scale));
}

gdouble
gimp_spin_scale_get_gamma (GimpSpinScale *scale)
{
  g_return_val_if_fail (GIMP_IS_SPIN_SCALE (scale), 1.0);

  return scale->gamma;
}

/**
 * gimp_spin_scale_set_constrain_drag:
 * @scale: the #GimpSpinScale.
 * @constrain: whether constraining to integer values when dragging with
 *             pointer.
 *
 * If @constrain_drag is TRUE, dragging the scale with the pointer will
 * only result into integer values. It will still possible to set the
 * scale to fractional values (if the spin scale "digits" is above 0)
 * for instance with keyboard edit.
 */
void
gimp_spin_scale_set_constrain_drag (GimpSpinScale *scale,
                                    gboolean       constrain)
{
  g_return_if_fail (GIMP_IS_SPIN_SCALE (scale));

  scale->constrain_drag = constrain;
}

gboolean
gimp_spin_scale_get_constrain_drag (GimpSpinScale *scale)
{
  g_return_val_if_fail (GIMP_IS_SPIN_SCALE (scale), 1.0);

  return scale->constrain_drag;
}

/**
 * gimp_spin_scale_get_mnemonic_keyval:
 * @scale: the #GimpSpinScale.
 *
 * If @scale has been set with a mnemonic key in its label text, this function
 * returns the keyval used for the mnemonic accelerator.
 *
 * Returns: the keyval usable for accelerators, or [const@Gdk.KEY_VoidSymbol].
 */
const guint
gimp_spin_scale_get_mnemonic_keyval (GimpSpinScale *scale)
{
  g_return_val_if_fail (GIMP_IS_SPIN_SCALE (scale), GDK_KEY_VoidSymbol);

  return scale->mnemonic_keyval;
}
