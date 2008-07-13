/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpscalebutton.c
 * Copyright (C) 2008 Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#define __GTK_SCALE_BUTTON_H__
#define __GTK_VOLUME_BUTTON_H__

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "gimpscalebutton.h"


static void      gimp_scale_button_value_changed  (GtkScaleButton  *button,
                                                   gdouble          value);
static void      gimp_scale_button_update_tooltip (GimpScaleButton *button);
static gboolean  gimp_scale_button_image_expose   (GtkWidget       *widget,
                                                   GdkEventExpose  *event,
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

  gtk_widget_hide (GTK_SCALE_BUTTON (button)->plus_button);
  gtk_widget_set_no_show_all (GTK_SCALE_BUTTON (button)->plus_button, TRUE);

  gtk_widget_hide (GTK_SCALE_BUTTON (button)->minus_button);
  gtk_widget_set_no_show_all (GTK_SCALE_BUTTON (button)->minus_button, TRUE);

  g_signal_connect (image, "expose-event",
                    G_CALLBACK (gimp_scale_button_image_expose),
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

  adj = gimp_gtk_scale_button_get_adjustment (GTK_SCALE_BUTTON (button));

  value = gtk_adjustment_get_value (adj);
  lower = adj->lower;
  upper = adj->upper;

  /*  use U+2009 THIN SPACE to seperate the percent sign from the number */

  text = g_strdup_printf ("%d\342\200\211%%",
                          (gint) (0.5 + ((value - lower) * 100.0 /
                                         (upper - lower))));

  gtk_widget_set_tooltip_text (GTK_WIDGET (button), text);
  g_free (text);
}

static gboolean
gimp_scale_button_image_expose (GtkWidget       *widget,
                                GdkEventExpose  *event,
                                GimpScaleButton *button)
{
  GtkStyle      *style = gtk_widget_get_style (widget);
  GtkAdjustment *adj;
  cairo_t       *cr;
  gint           value;
  gint           steps;
  gint           i;

  steps = MIN (widget->allocation.width, widget->allocation.height) / 2;

  adj = gimp_gtk_scale_button_get_adjustment (GTK_SCALE_BUTTON (button));

  if (steps < 1)
    return TRUE;

  value = 0.5 + ((adj->value - adj->lower) * (gdouble) steps /
                 (adj->upper - adj->lower));

  cr = gdk_cairo_create (widget->window);

  gdk_cairo_rectangle (cr, &event->area);
  cairo_clip (cr);

  cairo_set_line_width (cr, 0.5);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    {
      cairo_translate (cr,
                       widget->allocation.x + widget->allocation.width - 0.5,
                       widget->allocation.y + widget->allocation.height);
      cairo_scale (cr, -2.0, -2.0);
    }
  else
    {
      cairo_translate (cr,
                       widget->allocation.x + 0.5,
                       widget->allocation.y + widget->allocation.height);
      cairo_scale (cr, 2.0, -2.0);
    }

  for (i = 0; i < value; i++)
    {
      cairo_move_to (cr, i, 0);
      cairo_line_to (cr, i, i + 0.5);
    }

  gdk_cairo_set_source_color (cr, &style->fg[widget->state]);
  cairo_stroke (cr);

  for ( ; i < steps; i++)
    {
      cairo_move_to (cr, i, 0);
      cairo_line_to (cr, i, i + 0.5);
    }

  gdk_cairo_set_source_color (cr, &style->fg[GTK_STATE_INSENSITIVE]);
  cairo_stroke (cr);

  cairo_destroy (cr);

  return TRUE;
}

GtkWidget *
gimp_scale_button_new (gdouble value,
                       gdouble min,
                       gdouble max)
{
  GtkObject *adj;
  gdouble    step;

  g_return_val_if_fail (value >= min && value <= max, NULL);

  step = (max - min) / 10.0;
  adj  = gtk_adjustment_new (value, min, max, step, step, 0);

  return g_object_new (GIMP_TYPE_SCALE_BUTTON,
                       "orientation", GTK_ORIENTATION_HORIZONTAL,
                       "adjustment",  adj,
                       "size",        GTK_ICON_SIZE_MENU,
                       NULL);
}
