/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmamemsizeentry.c
 * Copyright (C) 2000-2003  Sven Neumann <sven@ligma.org>
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

#include "ligmawidgetstypes.h"

#include "ligmamemsizeentry.h"
#include "ligmawidgets.h"

#include "libligma/libligma-intl.h"


/**
 * SECTION: ligmamemsizeentry
 * @title: LigmaMemSizeEntry
 * @short_description: A composite widget to enter a memory size.
 *
 * Similar to a #LigmaSizeEntry but instead of lengths, this widget is
 * used to let the user enter memory sizes. A combo box allows one to
 * switch between Kilobytes, Megabytes and Gigabytes. Used in the LIGMA
 * preferences dialog.
 **/


enum
{
  VALUE_CHANGED,
  LAST_SIGNAL
};


struct _LigmaMemsizeEntryPrivate
{
  guint64        value;
  guint64        lower;
  guint64        upper;

  guint          shift;

  /* adjustment is owned by spinbutton. Do not unref() it. */
  GtkAdjustment *adjustment;
  GtkWidget     *spinbutton;
  GtkWidget     *menu;
};

#define GET_PRIVATE(obj) (((LigmaMemsizeEntry *) (obj))->priv)


static void  ligma_memsize_entry_adj_callback  (GtkAdjustment    *adj,
                                               LigmaMemsizeEntry *entry);
static void  ligma_memsize_entry_unit_callback (GtkWidget        *widget,
                                               LigmaMemsizeEntry *entry);

static guint64 ligma_memsize_entry_get_rounded_value (LigmaMemsizeEntry *entry,
                                                     guint64           value);

G_DEFINE_TYPE_WITH_PRIVATE (LigmaMemsizeEntry, ligma_memsize_entry, GTK_TYPE_BOX)

#define parent_class ligma_memsize_entry_parent_class

static guint ligma_memsize_entry_signals[LAST_SIGNAL] = { 0 };


static void
ligma_memsize_entry_class_init (LigmaMemsizeEntryClass *klass)
{
  klass->value_changed = NULL;

  ligma_memsize_entry_signals[VALUE_CHANGED] =
    g_signal_new ("value-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaMemsizeEntryClass, value_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}

static void
ligma_memsize_entry_init (LigmaMemsizeEntry *entry)
{
  entry->priv = ligma_memsize_entry_get_instance_private (entry);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (entry),
                                  GTK_ORIENTATION_HORIZONTAL);

  gtk_box_set_spacing (GTK_BOX (entry), 4);
}

static void
ligma_memsize_entry_adj_callback (GtkAdjustment    *adj,
                                 LigmaMemsizeEntry *entry)
{
  LigmaMemsizeEntryPrivate *private = GET_PRIVATE (entry);
  guint64                  size    = gtk_adjustment_get_value (adj);

  if (ligma_memsize_entry_get_rounded_value (entry, private->value) != size)
    /* Do not allow losing accuracy if the converted/displayed value
     * stays the same.
     */
    private->value = size << private->shift;

  g_signal_emit (entry, ligma_memsize_entry_signals[VALUE_CHANGED], 0);
}

static void
ligma_memsize_entry_unit_callback (GtkWidget        *widget,
                                  LigmaMemsizeEntry *entry)
{
  LigmaMemsizeEntryPrivate *private = GET_PRIVATE (entry);
  guint                    shift;

  ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (widget), (gint *) &shift);

#if _MSC_VER < 1300
#  define CAST (gint64)
#else
#  define CAST
#endif

  if (shift != private->shift)
    {
      private->shift = shift;

      gtk_adjustment_configure (private->adjustment,
                                ligma_memsize_entry_get_rounded_value (entry, private->value),
                                CAST private->lower >> shift,
                                CAST private->upper >> shift,
                                gtk_adjustment_get_step_increment (private->adjustment),
                                gtk_adjustment_get_page_increment (private->adjustment),
                                gtk_adjustment_get_page_size (private->adjustment));
    }

#undef CAST
}

/**
 * ligma_memsize_entry_get_rounded_value:
 * @entry: #LigmaMemsizeEntry whose set unit is used.
 * @value: value to convert to @entry unit, and rounded.
 *
 * Returns: the proper integer value to be displayed for current unit.
 *          This value has been appropriately rounded to the nearest
 *          integer, away from zero.
 */
static guint64
ligma_memsize_entry_get_rounded_value (LigmaMemsizeEntry *entry,
                                      guint64           value)
{
  LigmaMemsizeEntryPrivate *private = GET_PRIVATE (entry);
  guint64                  converted;

#if _MSC_VER < 1300
#  define CAST (gint64)
#else
#  define CAST
#endif

  converted = (CAST value >> private->shift) +
              ((CAST private->value >> (private->shift - 1)) & 1);

#undef CAST

  return converted;
}


/**
 * ligma_memsize_entry_new:
 * @value: the initial value (in Bytes)
 * @lower: the lower limit for the value (in Bytes)
 * @upper: the upper limit for the value (in Bytes)
 *
 * Creates a new #LigmaMemsizeEntry which is a #GtkHBox with a #GtkSpinButton
 * and a #GtkOptionMenu all setup to allow the user to enter memory sizes.
 *
 * Returns: Pointer to the new #LigmaMemsizeEntry.
 **/
GtkWidget *
ligma_memsize_entry_new (guint64  value,
                        guint64  lower,
                        guint64  upper)
{
  LigmaMemsizeEntry        *entry;
  LigmaMemsizeEntryPrivate *private;
  guint                    shift;

#if _MSC_VER < 1300
#  define CAST (gint64)
#else
#  define CAST
#endif

  g_return_val_if_fail (value >= lower && value <= upper, NULL);

  entry = g_object_new (LIGMA_TYPE_MEMSIZE_ENTRY, NULL);

  private = GET_PRIVATE (entry);

  for (shift = 30; shift > 10; shift -= 10)
    {
      if (value > (G_GUINT64_CONSTANT (1) << shift) &&
          value % (G_GUINT64_CONSTANT (1) << shift) == 0)
        break;
    }

  private->value = value;
  private->lower = lower;
  private->upper = upper;
  private->shift = shift;

  private->adjustment = gtk_adjustment_new (ligma_memsize_entry_get_rounded_value (entry,
                                                                                  private->value),
                                            CAST (lower >> shift),
                                            CAST (upper >> shift),
                                            1, 8, 0);

  private->spinbutton = ligma_spin_button_new (private->adjustment, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (private->spinbutton), TRUE);

#undef CAST

  gtk_entry_set_width_chars (GTK_ENTRY (private->spinbutton), 7);
  gtk_box_pack_start (GTK_BOX (entry), private->spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (private->spinbutton);

  g_signal_connect (private->adjustment, "value-changed",
                    G_CALLBACK (ligma_memsize_entry_adj_callback),
                    entry);

  private->menu = ligma_int_combo_box_new (_("Kibibyte"), 10,
                                          _("Mebibyte"), 20,
                                          _("Gibibyte"), 30,
                                          NULL);

  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (private->menu), shift);

  g_signal_connect (private->menu, "changed",
                    G_CALLBACK (ligma_memsize_entry_unit_callback),
                    entry);

  gtk_box_pack_start (GTK_BOX (entry), private->menu, FALSE, FALSE, 0);
  gtk_widget_show (private->menu);

  return GTK_WIDGET (entry);
}

/**
 * ligma_memsize_entry_set_value:
 * @entry: a #LigmaMemsizeEntry
 * @value: the new value (in Bytes)
 *
 * Sets the @entry's value. Please note that the #LigmaMemsizeEntry rounds
 * the value to full Kilobytes.
 **/
void
ligma_memsize_entry_set_value (LigmaMemsizeEntry *entry,
                              guint64           value)
{
  LigmaMemsizeEntryPrivate *private;
  guint                    shift;

  g_return_if_fail (LIGMA_IS_MEMSIZE_ENTRY (entry));

  private = GET_PRIVATE (entry);

  g_return_if_fail (value >= private->lower && value <= private->upper);

  for (shift = 30; shift > 10; shift -= 10)
    {
      if (value > (G_GUINT64_CONSTANT (1) << shift) &&
          value % (G_GUINT64_CONSTANT (1) << shift) == 0)
        break;
    }

  if (shift != private->shift)
    ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (private->menu), shift);

  gtk_adjustment_set_value (private->adjustment,
                            (gdouble) ligma_memsize_entry_get_rounded_value (entry, value));

#undef CASE
}

/**
 * ligma_memsize_entry_get_value:
 * @entry: a #LigmaMemsizeEntry
 *
 * Retrieves the current value from a #LigmaMemsizeEntry.
 *
 * Returns: the current value of @entry (in Bytes).
 **/
guint64
ligma_memsize_entry_get_value (LigmaMemsizeEntry *entry)
{
  g_return_val_if_fail (LIGMA_IS_MEMSIZE_ENTRY (entry), 0);

  return GET_PRIVATE (entry)->value;
}

/**
 * ligma_memsize_entry_get_spinbutton:
 * @entry: a #LigmaMemsizeEntry
 *
 * Returns: (transfer none) (type GtkSpinButton): the entry's #GtkSpinbutton.
 *
 * Since: 3.0
 **/
GtkWidget *
ligma_memsize_entry_get_spinbutton (LigmaMemsizeEntry *entry)
{
  g_return_val_if_fail (LIGMA_IS_MEMSIZE_ENTRY (entry), 0);

  return GET_PRIVATE (entry)->spinbutton;
}
