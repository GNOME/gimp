/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpscalebutton.c
 * Copyright (C) 2008 Sven Neumann <sven@gimp.org>
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

#include "widgets-types.h"

#include "gimpscalebutton.h"


static void      gimp_scale_button_value_changed  (GtkScaleButton  *button,
                                                   gdouble          value);
static void      gimp_scale_button_update_tooltip (GimpScaleButton *button);
static gboolean  gimp_scale_button_image_draw     (GtkWidget       *widget,
                                                   cairo_t         *cr,
                                                   GimpScaleButton *button);


G_DEFINE_TYPE (GimpScaleButton, gimp_scale_button, GTK_TYPE_SCALE_BUTTON)

#define parent_class gimp_scale_button_parent_class


static void
gimp_scale_button_class_init (GimpScaleButtonClass *klass)
{
  GtkScaleButtonClass *button_class = GTK_SCALE_BUTTON_CLASS (klass);

  button_class->value_changed = gimp_scale_button_value_changed;
}

static void
gimp_scale_button_init (GimpScaleButton *button)
{
  GtkWidget *image = gtk_bin_get_child (GTK_BIN (button));
  GtkWidget *plusminus;

  plusminus = gtk_scale_button_get_plus_button (GTK_SCALE_BUTTON (button));
  gtk_widget_hide (plusminus);
  gtk_widget_set_no_show_all (plusminus, TRUE);

  plusminus = gtk_scale_button_get_minus_button (GTK_SCALE_BUTTON (button));
  gtk_widget_hide (plusminus);
  gtk_widget_set_no_show_all (plusminus, TRUE);

  g_signal_connect (image, "draw",
                    G_CALLBACK (gimp_scale_button_image_draw),
                    button);

  /* GtkScaleButton doesn't emit "value-changed" when the adjustment changes */
  g_signal_connect (button, "notify::adjustment",
                    G_CALLBACK (gimp_scale_button_update_tooltip),
                    NULL);

  gimp_scale_button_update_tooltip (button);
}

static void
gimp_scale_button_value_changed (GtkScaleButton *button,
                                 gdouble         value)
{
  if (GTK_SCALE_BUTTON_CLASS (parent_class)->value_changed)
    GTK_SCALE_BUTTON_CLASS (parent_class)->value_changed (button, value);

  gimp_scale_button_update_tooltip (GIMP_SCALE_BUTTON (button));
}

static void
gimp_scale_button_update_tooltip (GimpScaleButton *button)
{
  GtkAdjustment *adj;
  gchar         *text;
  gdouble        value;
  gdouble        lower;
  gdouble        upper;

  adj = gtk_scale_button_get_adjustment (GTK_SCALE_BUTTON (button));

  value = gtk_adjustment_get_value (adj);
  lower = gtk_adjustment_get_lower (adj);
  upper = gtk_adjustment_get_upper (adj);

  /*  use U+2009 THIN SPACE to separate the percent sign from the number */

  text = g_strdup_printf ("%d\342\200\211%%",
                          (gint) (0.5 + ((value - lower) * 100.0 /
                                         (upper - lower))));

  gtk_widget_set_tooltip_text (GTK_WIDGET (button), text);
  g_free (text);
}

static gboolean
gimp_scale_button_image_draw (GtkWidget       *widget,
                              cairo_t         *cr,
                              GimpScaleButton *button)
{
  GtkStyleContext *style = gtk_widget_get_style_context (widget);
  GtkAllocation    allocation;
  GtkAdjustment   *adj;
  GdkRGBA          color;
  gint             value;
  gint             steps;
  gint             i;

  gtk_widget_get_allocation (widget, &allocation);

  steps = MIN (allocation.width, allocation.height) / 2;

  adj = gtk_scale_button_get_adjustment (GTK_SCALE_BUTTON (button));

  if (steps < 1)
    return TRUE;

  value = 0.5 + ((gtk_adjustment_get_value (adj) -
                  gtk_adjustment_get_lower (adj)) * (gdouble) steps /
                 (gtk_adjustment_get_upper (adj) -
                  gtk_adjustment_get_lower (adj)));

  cairo_set_line_width (cr, 0.5);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    {
      cairo_translate (cr, allocation.width - 0.5, allocation.height);
      cairo_scale (cr, -2.0, -2.0);
    }
  else
    {
      cairo_translate (cr, 0.5, allocation.height);
      cairo_scale (cr, 2.0, -2.0);
    }

  for (i = 0; i < value; i++)
    {
      cairo_move_to (cr, i, 0);
      cairo_line_to (cr, i, i + 0.5);
    }

  gtk_style_context_get_color (style, gtk_widget_get_state_flags (widget),
                               &color);
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_stroke (cr);

  for ( ; i < steps; i++)
    {
      cairo_move_to (cr, i, 0);
      cairo_line_to (cr, i, i + 0.5);
    }

  gtk_style_context_get_background_color (style, GTK_STATE_FLAG_INSENSITIVE,
                                          &color);
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_stroke (cr);

  return TRUE;
}

GtkWidget *
gimp_scale_button_new (gdouble value,
                       gdouble min,
                       gdouble max)
{
  GtkAdjustment *adj;
  gdouble        step;

  g_return_val_if_fail (value >= min && value <= max, NULL);

  step = (max - min) / 10.0;
  adj  = gtk_adjustment_new (value, min, max, step, step, 0);

  return g_object_new (GIMP_TYPE_SCALE_BUTTON,
                       "orientation", GTK_ORIENTATION_HORIZONTAL,
                       "adjustment",  adj,
                       "size",        GTK_ICON_SIZE_MENU,
                       NULL);
}
