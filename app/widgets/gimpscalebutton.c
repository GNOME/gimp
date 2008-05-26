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

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "gimpscalebutton.h"


static gboolean  gimp_scale_button_image_expose (GtkWidget       *widget,
                                                 GdkEventExpose  *event,
                                                 GimpScaleButton *button);


G_DEFINE_TYPE (GimpScaleButton, gimp_scale_button, GTK_TYPE_SCALE_BUTTON)


static void
gimp_scale_button_class_init (GimpScaleButtonClass *klass)
{

}

static void
gimp_scale_button_init (GimpScaleButton *button)
{
  GtkWidget *image;

  image = gtk_bin_get_child (GTK_BIN (button));

  g_signal_connect (image, "expose-event",
                    G_CALLBACK (gimp_scale_button_image_expose),
                    button);
}

static gboolean
gimp_scale_button_image_expose (GtkWidget       *widget,
                                GdkEventExpose  *event,
                                GimpScaleButton *button)
{
  GtkAdjustment *adj;
  cairo_t       *cr;
  gint           value;
  gint           steps;
  gint           i;

  steps = MIN (widget->allocation.width, widget->allocation.height) / 2;

  adj = gtk_scale_button_get_adjustment (GTK_SCALE_BUTTON (button));

  if (steps < 1)
    return TRUE;

  value = 0.5 + ((adj->value - adj->lower) * (gdouble) steps /
                 (adj->upper - adj->lower));

  cr = gdk_cairo_create (widget->window);

  gdk_cairo_rectangle (cr, &event->area);
  cairo_clip (cr);

  cairo_set_line_width (cr, 0.5);

  cairo_translate (cr,
                   widget->allocation.x,
                   widget->allocation.y + widget->allocation.height + 0.5);
  cairo_scale (cr, 2.0, -2.0);

  for (i = 0; i < value; i++)
    {
      cairo_move_to (cr, 0, i);
      cairo_line_to (cr, i + 0.5, i);
    }

  gdk_cairo_set_source_color (cr, &widget->style->fg[widget->state]);
  cairo_stroke (cr);

  for ( ; i < steps; i++)
    {
      cairo_move_to (cr, 0, i);
      cairo_line_to (cr, i + 0.5, i);
    }

  gdk_cairo_set_source_color (cr, &widget->style->fg[GTK_STATE_INSENSITIVE]);
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
                       "adjustment", adj,
                       "size",       GTK_ICON_SIZE_MENU,
                       NULL);
}
