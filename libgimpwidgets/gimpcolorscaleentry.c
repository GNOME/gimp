/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorscaleentry.c
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

#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"
#include "libligmabase/ligmabase.h"

#include "ligmawidgets.h"


/**
 * SECTION: ligmacolorscaleentry
 * @title: LigmaColorScaleEntry
 * @short_description: Widget containing a color scale, a spin button
 *                     and a label.
 *
 * This widget is a subclass of #LigmaScaleEntry showing a
 * #LigmaColorScale instead of a #GtkScale.
 **/

struct _LigmaColorScaleEntry
{
  LigmaScaleEntry parent_instance;
};

static GtkWidget * ligma_color_scale_entry_new_range_widget (GtkAdjustment  *adjustment);

G_DEFINE_TYPE (LigmaColorScaleEntry, ligma_color_scale_entry, LIGMA_TYPE_SCALE_ENTRY)

#define parent_class ligma_color_scale_entry_parent_class


static void
ligma_color_scale_entry_class_init (LigmaColorScaleEntryClass *klass)
{
  LigmaScaleEntryClass *entry_class = LIGMA_SCALE_ENTRY_CLASS (klass);

  entry_class->new_range_widget = ligma_color_scale_entry_new_range_widget;
}

static void
ligma_color_scale_entry_init (LigmaColorScaleEntry *entry)
{
}

static GtkWidget *
ligma_color_scale_entry_new_range_widget (GtkAdjustment  *adjustment)
{
  GtkWidget *scale;

  g_return_val_if_fail (GTK_IS_ADJUSTMENT (adjustment), NULL);

  scale = ligma_color_scale_new (GTK_ORIENTATION_HORIZONTAL,
                                LIGMA_COLOR_SELECTOR_VALUE);

  gtk_range_set_adjustment (GTK_RANGE (scale), adjustment);

  return scale;
}

/**
 * ligma_color_scale_entry_new:
 * @text:           The text for the #GtkLabel.
 * @value:          The initial value.
 * @lower:          The lower boundary.
 * @upper:          The upper boundary.
 * @digits:         The number of decimal digits.
 *
 * Returns: (transfer full): The new #LigmaColorScale widget.
 **/
GtkWidget *
ligma_color_scale_entry_new (const gchar *text,
                            gdouble      value,
                            gdouble      lower,
                            gdouble      upper,
                            guint        digits)
{
  GtkWidget *entry;

  entry = g_object_new (LIGMA_TYPE_COLOR_SCALE_ENTRY,
                        "label",          text,
                        "value",          value,
                        "lower",          lower,
                        "upper",          upper,
                        "digits",         digits,
                        NULL);

  return entry;
}
