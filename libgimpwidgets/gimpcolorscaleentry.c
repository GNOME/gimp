/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpcolorscaleentry.c
 * Copyright (C) 2020 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "gimpwidgets.h"


/**
 * SECTION: gimpcolorscaleentry
 * @title: GimpColorScaleEntry
 * @short_description: Widget containing a color scale, a spin button
 *                     and a label.
 *
 * This widget is a subclass of #GimpScaleEntry showing a
 * #GimpColorScale instead of a #GtkScale.
 **/

struct _GimpColorScaleEntry
{
  GimpScaleEntry parent_instance;
};

static GtkWidget * gimp_color_scale_entry_new_range_widget (GtkAdjustment  *adjustment);

G_DEFINE_TYPE (GimpColorScaleEntry, gimp_color_scale_entry, GIMP_TYPE_SCALE_ENTRY)

#define parent_class gimp_color_scale_entry_parent_class


static void
gimp_color_scale_entry_class_init (GimpColorScaleEntryClass *klass)
{
  GimpScaleEntryClass *entry_class = GIMP_SCALE_ENTRY_CLASS (klass);

  entry_class->new_range_widget = gimp_color_scale_entry_new_range_widget;
}

static void
gimp_color_scale_entry_init (GimpColorScaleEntry *entry)
{
}

static GtkWidget *
gimp_color_scale_entry_new_range_widget (GtkAdjustment  *adjustment)
{
  GtkWidget *scale;

  g_return_val_if_fail (GTK_IS_ADJUSTMENT (adjustment), NULL);

  scale = gimp_color_scale_new (GTK_ORIENTATION_HORIZONTAL,
                                GIMP_COLOR_SELECTOR_VALUE);

  gtk_range_set_adjustment (GTK_RANGE (scale), adjustment);

  return scale;
}

/**
 * gimp_color_scale_entry_new:
 * @text:           The text for the #GtkLabel.
 * @value:          The initial value.
 * @lower:          The lower boundary.
 * @upper:          The upper boundary.
 * @digits:         The number of decimal digits.
 *
 * Returns: (transfer full): The new #GimpColorScale widget.
 **/
GtkWidget *
gimp_color_scale_entry_new (const gchar *text,
                            gdouble      value,
                            gdouble      lower,
                            gdouble      upper,
                            guint        digits)
{
  GtkWidget *entry;

  entry = g_object_new (GIMP_TYPE_COLOR_SCALE_ENTRY,
                        "label",          text,
                        "value",          value,
                        "lower",          lower,
                        "upper",          upper,
                        "digits",         digits,
                        NULL);

  return entry;
}
