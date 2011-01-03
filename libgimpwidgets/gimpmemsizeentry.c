/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmemsizeentry.c
 * Copyright (C) 2000-2003  Sven Neumann <sven@gimp.org>
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
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpmemsizeentry.h"
#include "gimpwidgets.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpmemsizeentry
 * @title: GimpMemSizeEntry
 * @short_description: A composite widget to enter a memory size.
 *
 * Similar to a #GimpSizeEntry but instead of lengths, this widget is
 * used to let the user enter memory sizes. A combo box allows one to
 * switch between Kilobytes, Megabytes and Gigabytes. Used in the GIMP
 * preferences dialog.
 **/


enum
{
  VALUE_CHANGED,
  LAST_SIGNAL
};


typedef struct _GimpMemsizeEntryPrivate GimpMemsizeEntryPrivate;

struct _GimpMemsizeEntryPrivate
{
  guint64        value;
  guint64        lower;
  guint64        upper;

  guint          shift;

  GtkAdjustment *adjustment;
  GtkWidget     *spinbutton;
  GtkWidget     *menu;
};

#define GET_PRIVATE(obj) G_TYPE_INSTANCE_GET_PRIVATE (obj, \
                                                      GIMP_TYPE_MEMSIZE_ENTRY, \
                                                      GimpMemsizeEntryPrivate)


static void  gimp_memsize_entry_finalize      (GObject          *object);

static void  gimp_memsize_entry_adj_callback  (GtkAdjustment    *adj,
                                               GimpMemsizeEntry *entry);
static void  gimp_memsize_entry_unit_callback (GtkWidget        *widget,
                                               GimpMemsizeEntry *entry);


G_DEFINE_TYPE (GimpMemsizeEntry, gimp_memsize_entry, GTK_TYPE_BOX)

#define parent_class gimp_memsize_entry_parent_class

static guint gimp_memsize_entry_signals[LAST_SIGNAL] = { 0 };


static void
gimp_memsize_entry_class_init (GimpMemsizeEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_memsize_entry_finalize;

  klass->value_changed   = NULL;

  gimp_memsize_entry_signals[VALUE_CHANGED] =
    g_signal_new ("value-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpMemsizeEntryClass, value_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  g_type_class_add_private (object_class, sizeof (GimpMemsizeEntryPrivate));
}

static void
gimp_memsize_entry_init (GimpMemsizeEntry *entry)
{
  GimpMemsizeEntryPrivate *private = GET_PRIVATE (entry);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (entry),
                                  GTK_ORIENTATION_HORIZONTAL);

  gtk_box_set_spacing (GTK_BOX (entry), 4);

  private->value      = 0;
  private->lower      = 0;
  private->upper      = 0;
  private->shift      = 0;
  private->adjustment = NULL;
  private->menu       = NULL;
}

static void
gimp_memsize_entry_finalize (GObject *object)
{
  GimpMemsizeEntryPrivate *private = GET_PRIVATE (object);

  if (private->adjustment)
    {
      g_object_unref (private->adjustment);
      private->adjustment = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_memsize_entry_adj_callback (GtkAdjustment    *adj,
                                 GimpMemsizeEntry *entry)
{
  GimpMemsizeEntryPrivate *private = GET_PRIVATE (entry);
  guint64                  size    = gtk_adjustment_get_value (adj);

  private->value = size << private->shift;

  g_signal_emit (entry, gimp_memsize_entry_signals[VALUE_CHANGED], 0);
}

static void
gimp_memsize_entry_unit_callback (GtkWidget        *widget,
                                  GimpMemsizeEntry *entry)
{
  GimpMemsizeEntryPrivate *private = GET_PRIVATE (entry);
  guint                    shift;

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), (gint *) &shift);

#if _MSC_VER < 1300
#  define CAST (gint64)
#else
#  define CAST
#endif

  if (shift != private->shift)
    {
      private->shift = shift;

      gtk_adjustment_configure (private->adjustment,
                                CAST private->value >> shift,
                                CAST private->lower >> shift,
                                CAST private->upper >> shift,
                                gtk_adjustment_get_step_increment (private->adjustment),
                                gtk_adjustment_get_page_increment (private->adjustment),
                                gtk_adjustment_get_page_size (private->adjustment));
    }

#undef CAST
}


/**
 * gimp_memsize_entry_new:
 * @value: the initial value (in Bytes)
 * @lower: the lower limit for the value (in Bytes)
 * @upper: the upper limit for the value (in Bytes)
 *
 * Creates a new #GimpMemsizeEntry which is a #GtkHBox with a #GtkSpinButton
 * and a #GtkOptionMenu all setup to allow the user to enter memory sizes.
 *
 * Returns: Pointer to the new #GimpMemsizeEntry.
 **/
GtkWidget *
gimp_memsize_entry_new (guint64  value,
                        guint64  lower,
                        guint64  upper)
{
  GimpMemsizeEntry        *entry;
  GimpMemsizeEntryPrivate *private;
  GtkAdjustment           *adj;
  guint                    shift;

#if _MSC_VER < 1300
#  define CAST (gint64)
#else
#  define CAST
#endif

  g_return_val_if_fail (value >= lower && value <= upper, NULL);

  entry = g_object_new (GIMP_TYPE_MEMSIZE_ENTRY, NULL);

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

  adj = gtk_adjustment_new (CAST (value >> shift),
                            CAST (lower >> shift),
                            CAST (upper >> shift),
                            1, 8, 0);

  private->spinbutton = gtk_spin_button_new (adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (private->spinbutton), TRUE);

#undef CAST

  private->adjustment = GTK_ADJUSTMENT (adj);
  g_object_ref_sink (private->adjustment);

  gtk_entry_set_width_chars (GTK_ENTRY (private->spinbutton), 7);
  gtk_box_pack_start (GTK_BOX (entry), private->spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (private->spinbutton);

  g_signal_connect (private->adjustment, "value-changed",
                    G_CALLBACK (gimp_memsize_entry_adj_callback),
                    entry);

  private->menu = gimp_int_combo_box_new (_("Kilobytes"), 10,
                                        _("Megabytes"), 20,
                                        _("Gigabytes"), 30,
                                        NULL);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (private->menu), shift);

  g_signal_connect (private->menu, "changed",
                    G_CALLBACK (gimp_memsize_entry_unit_callback),
                    entry);

  gtk_box_pack_start (GTK_BOX (entry), private->menu, FALSE, FALSE, 0);
  gtk_widget_show (private->menu);

  return GTK_WIDGET (entry);
}

/**
 * gimp_memsize_entry_set_value:
 * @entry: a #GimpMemsizeEntry
 * @value: the new value (in Bytes)
 *
 * Sets the @entry's value. Please note that the #GimpMemsizeEntry rounds
 * the value to full Kilobytes.
 **/
void
gimp_memsize_entry_set_value (GimpMemsizeEntry *entry,
                              guint64           value)
{
  GimpMemsizeEntryPrivate *private;
  guint                    shift;

  g_return_if_fail (GIMP_IS_MEMSIZE_ENTRY (entry));

  private = GET_PRIVATE (entry);

  g_return_if_fail (value >= private->lower && value <= private->upper);

  for (shift = 30; shift > 10; shift -= 10)
    {
      if (value > (G_GUINT64_CONSTANT (1) << shift) &&
          value % (G_GUINT64_CONSTANT (1) << shift) == 0)
        break;
    }

  if (shift != private->shift)
    {
      private->shift = shift;
      private->value = value;

      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (private->menu), shift);
    }

#if _MSC_VER < 1300
#  define CAST (gint64)
#else
#  define CAST
#endif

  gtk_adjustment_set_value (private->adjustment, CAST (value >> shift));

#undef CASE
}

/**
 * gimp_memsize_entry_get_value:
 * @entry: a #GimpMemsizeEntry
 *
 * Retrieves the current value from a #GimpMemsizeEntry.
 *
 * Returns: the current value of @entry (in Bytes).
 **/
guint64
gimp_memsize_entry_get_value (GimpMemsizeEntry *entry)
{
  g_return_val_if_fail (GIMP_IS_MEMSIZE_ENTRY (entry), 0);

  return GET_PRIVATE (entry)->value;
}

/**
 * gimp_memsize_entry_get_spinbutton:
 * @entry: a #GimpMemsizeEntry
 *
 * Returns: the entry's #GtkSpinbutton.
 *
 * Since: GIMP 3.0
 **/
GtkWidget *
gimp_memsize_entry_get_spinbutton (GimpMemsizeEntry *entry)
{
  g_return_val_if_fail (GIMP_IS_MEMSIZE_ENTRY (entry), 0);

  return GET_PRIVATE (entry)->spinbutton;
}
