/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpsizeentry.c
 * Copyright (C) 1999-2000 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpwidgets.h"

#include "gimpsizeentry.h"


#define SIZE_MAX_VALUE 500000.0

#define GIMP_SIZE_ENTRY_DIGITS(unit) (MIN (gimp_unit_get_digits (unit), 5) + 1)


enum
{
  VALUE_CHANGED,
  REFVAL_CHANGED,
  UNIT_CHANGED,
  LAST_SIGNAL
};


struct _GimpSizeEntryField
{
  GimpSizeEntry *gse;

  gdouble        resolution;
  gdouble        lower;
  gdouble        upper;

  GtkObject     *value_adjustment;
  GtkWidget     *value_spinbutton;
  gdouble        value;
  gdouble        min_value;
  gdouble        max_value;

  GtkObject     *refval_adjustment;
  GtkWidget     *refval_spinbutton;
  gdouble        refval;
  gdouble        min_refval;
  gdouble        max_refval;
  gint           refval_digits;

  gint           stop_recursion;
};


static void   gimp_size_entry_finalize        (GObject            *object);

static void   gimp_size_entry_update_value    (GimpSizeEntryField *gsef,
                                               gdouble             value);
static void   gimp_size_entry_value_callback  (GtkWidget          *widget,
                                               gpointer            data);
static void   gimp_size_entry_update_refval   (GimpSizeEntryField *gsef,
                                               gdouble             refval);
static void   gimp_size_entry_refval_callback (GtkWidget          *widget,
                                               gpointer            data);
static void   gimp_size_entry_update_unit     (GimpSizeEntry      *gse,
                                               GimpUnit            unit);
static void   gimp_size_entry_unit_callback   (GtkWidget          *widget,
                                               GimpSizeEntry      *sizeentry);


G_DEFINE_TYPE (GimpSizeEntry, gimp_size_entry, GTK_TYPE_TABLE)

#define parent_class gimp_size_entry_parent_class

static guint gimp_size_entry_signals[LAST_SIGNAL] = { 0 };


static void
gimp_size_entry_class_init (GimpSizeEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  gimp_size_entry_signals[VALUE_CHANGED] =
    g_signal_new ("value-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpSizeEntryClass, value_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_size_entry_signals[REFVAL_CHANGED] =
    g_signal_new ("refval-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpSizeEntryClass, refval_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_size_entry_signals[UNIT_CHANGED] =
    g_signal_new ("unit-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpSizeEntryClass, unit_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->finalize = gimp_size_entry_finalize;

  klass->value_changed   = NULL;
  klass->refval_changed  = NULL;
  klass->unit_changed    = NULL;
}

static void
gimp_size_entry_init (GimpSizeEntry *gse)
{
  gse->fields            = NULL;
  gse->number_of_fields  = 0;
  gse->unitmenu          = NULL;
  gse->unit              = GIMP_UNIT_PIXEL;
  gse->menu_show_pixels  = TRUE;
  gse->menu_show_percent = TRUE;
  gse->show_refval       = FALSE;
  gse->update_policy     = GIMP_SIZE_ENTRY_UPDATE_NONE;
}

static void
gimp_size_entry_finalize (GObject *object)
{
  GimpSizeEntry *gse = GIMP_SIZE_ENTRY (object);

  if (gse->fields)
    {
      GSList *list;

      for (list = gse->fields; list; list = list->next)
        g_slice_free (GimpSizeEntryField, list->data);

      g_slist_free (gse->fields);
      gse->fields = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gimp_size_entry_new:
 * @number_of_fields:  The number of input fields.
 * @unit:              The initial unit.
 * @unit_format:       A printf-like unit-format string as is used with
 *                     gimp_unit_menu_new().
 * @menu_show_pixels:  %TRUE if the unit menu shold contain an item for
 *                     GIMP_UNIT_PIXEL (ignored if the @update_policy is not
 *                     GIMP_SIZE_ENTRY_UPDATE_NONE).
 * @menu_show_percent: %TRUE if the unit menu shold contain an item for
 *                     GIMP_UNIT_PERCENT.
 * @show_refval:       %TRUE if you want an extra "reference value"
 *                     spinbutton per input field.
 * @spinbutton_width:  The minimal horizontal size of the #GtkSpinButton's.
 * @update_policy:     How the automatic pixel <-> real-world-unit
 *                     calculations should be done.
 *
 * Creates a new #GimpSizeEntry widget.
 *
 * To have all automatic calculations performed correctly, set up the
 * widget in the following order:
 *
 * 1. gimp_size_entry_new()
 *
 * 2. (for each additional input field) gimp_size_entry_add_field()
 *
 * 3. gimp_size_entry_set_unit()
 *
 * For each input field:
 *
 * 4. gimp_size_entry_set_resolution()
 *
 * 5. gimp_size_entry_set_refval_boundaries()
 *    (or gimp_size_entry_set_value_boundaries())
 *
 * 6. gimp_size_entry_set_size()
 *
 * 7. gimp_size_entry_set_refval() (or gimp_size_entry_set_value())
 *
 * The #GimpSizeEntry is derived from #GtkTable and will have
 * an empty border of one cell width on each side plus an empty column left
 * of the #GimpUnitMenu to allow the caller to add labels or a
 * #GimpChainButton.
 *
 * Returns: A Pointer to the new #GimpSizeEntry widget.
 **/
GtkWidget *
gimp_size_entry_new (gint                       number_of_fields,
                     GimpUnit                   unit,
                     const gchar               *unit_format,
                     gboolean                   menu_show_pixels,
                     gboolean                   menu_show_percent,
                     gboolean                   show_refval,
                     gint                       spinbutton_width,
                     GimpSizeEntryUpdatePolicy  update_policy)
{
  GimpSizeEntry *gse;
  gint           i;

  g_return_val_if_fail ((number_of_fields >= 0) && (number_of_fields <= 16),
                        NULL);

  gse = g_object_new (GIMP_TYPE_SIZE_ENTRY, NULL);

  gse->number_of_fields = number_of_fields;
  gse->unit             = unit;
  gse->show_refval      = show_refval;
  gse->update_policy    = update_policy;

  gtk_table_resize (GTK_TABLE (gse),
                    1 + gse->show_refval + 2,
                    number_of_fields + 1 + 3);

  /*  show the 'pixels' menu entry only if we are a 'size' sizeentry and
   *  don't have the reference value spinbutton
   */
  if ((update_policy == GIMP_SIZE_ENTRY_UPDATE_RESOLUTION) ||
      (show_refval == TRUE))
    gse->menu_show_pixels = FALSE;
  else
    gse->menu_show_pixels = menu_show_pixels;

  /*  show the 'percent' menu entry only if we are a 'size' sizeentry
   */
  if (update_policy == GIMP_SIZE_ENTRY_UPDATE_RESOLUTION)
    gse->menu_show_percent = FALSE;
  else
    gse->menu_show_percent = menu_show_percent;

  for (i = 0; i < number_of_fields; i++)
    {
      GimpSizeEntryField *gsef = g_slice_new0 (GimpSizeEntryField);
      gint                digits;

      gse->fields = g_slist_append (gse->fields, gsef);

      gsef->gse               = gse;
      gsef->resolution        = 1.0; /*  just to avoid division by zero  */
      gsef->lower             = 0.0;
      gsef->upper             = 100.0;
      gsef->value             = 0;
      gsef->min_value         = 0;
      gsef->max_value         = SIZE_MAX_VALUE;
      gsef->refval_adjustment = NULL;
      gsef->value_adjustment  = NULL;
      gsef->refval            = 0;
      gsef->min_refval        = 0;
      gsef->max_refval        = SIZE_MAX_VALUE;
      gsef->refval_digits     =
        (update_policy == GIMP_SIZE_ENTRY_UPDATE_SIZE) ? 0 : 3;
      gsef->stop_recursion    = 0;

      digits = ((unit == GIMP_UNIT_PIXEL) ?
                gsef->refval_digits : ((unit == GIMP_UNIT_PERCENT) ?
                                       2 : GIMP_SIZE_ENTRY_DIGITS (unit)));

      gsef->value_spinbutton = gimp_spin_button_new (&gsef->value_adjustment,
                                                     gsef->value,
                                                     gsef->min_value,
                                                     gsef->max_value,
                                                     1.0, 10.0, 0.0,
                                                     1.0, digits);

      if (spinbutton_width > 0)
        {
          if (spinbutton_width < 17)
            gtk_entry_set_width_chars (GTK_ENTRY (gsef->value_spinbutton),
                                       spinbutton_width);
          else
            gtk_widget_set_size_request (gsef->value_spinbutton,
                                         spinbutton_width, -1);
        }

      gtk_table_attach_defaults (GTK_TABLE (gse), gsef->value_spinbutton,
                                 i+1, i+2,
                                 gse->show_refval+1, gse->show_refval+2);
      g_signal_connect (gsef->value_adjustment, "value-changed",
                        G_CALLBACK (gimp_size_entry_value_callback),
                        gsef);

      gtk_widget_show (gsef->value_spinbutton);

      if (gse->show_refval)
        {
          gsef->refval_spinbutton =
            gimp_spin_button_new (&gsef->refval_adjustment,
                                  gsef->refval,
                                  gsef->min_refval, gsef->max_refval,
                                  1.0, 10.0, 0.0, 1.0, gsef->refval_digits);

          gtk_widget_set_size_request (gsef->refval_spinbutton,
                                       spinbutton_width, -1);
          gtk_table_attach_defaults (GTK_TABLE (gse), gsef->refval_spinbutton,
                                     i + 1, i + 2, 1, 2);
          g_signal_connect (gsef->refval_adjustment,
                            "value-changed",
                            G_CALLBACK (gimp_size_entry_refval_callback),
                            gsef);

          gtk_widget_show (gsef->refval_spinbutton);
        }

      if (gse->menu_show_pixels && (unit == GIMP_UNIT_PIXEL) &&
          ! gse->show_refval)
        gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                    gsef->refval_digits);
    }

  gse->unitmenu = gimp_unit_menu_new (unit_format, unit,
                                      gse->menu_show_pixels,
                                      gse->menu_show_percent, TRUE);
  gtk_table_attach (GTK_TABLE (gse), gse->unitmenu,
                    i+2, i+3,
                    gse->show_refval+1, gse->show_refval+2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  g_signal_connect (gse->unitmenu, "unit-changed",
                    G_CALLBACK (gimp_size_entry_unit_callback),
                    gse);
  gtk_widget_show (gse->unitmenu);

  return GTK_WIDGET (gse);
}


/**
 * gimp_size_entry_add_field:
 * @gse:               The sizeentry you want to add a field to.
 * @value_spinbutton:  The spinbutton to display the field's value.
 * @refval_spinbutton: The spinbutton to display the field's reference value.
 *
 * Adds an input field to the #GimpSizeEntry.
 *
 * The new input field will have the index 0. If you specified @show_refval
 * as %TRUE in gimp_size_entry_new() you have to pass an additional
 * #GtkSpinButton to hold the reference value. If @show_refval was %FALSE,
 * @refval_spinbutton will be ignored.
 **/
void
gimp_size_entry_add_field  (GimpSizeEntry *gse,
                            GtkSpinButton *value_spinbutton,
                            GtkSpinButton *refval_spinbutton)
{
  GimpSizeEntryField *gsef;
  gint                digits;

  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail (GTK_IS_SPIN_BUTTON (value_spinbutton));

  if (gse->show_refval)
    {
      g_return_if_fail (GTK_IS_SPIN_BUTTON (refval_spinbutton));
    }

  gsef = g_slice_new0 (GimpSizeEntryField);

  gse->fields = g_slist_prepend (gse->fields, gsef);
  gse->number_of_fields++;

  gsef->gse            = gse;
  gsef->resolution     = 1.0; /*  just to avoid division by zero  */
  gsef->lower          = 0.0;
  gsef->upper          = 100.0;
  gsef->value          = 0;
  gsef->min_value      = 0;
  gsef->max_value      = SIZE_MAX_VALUE;
  gsef->refval         = 0;
  gsef->min_refval     = 0;
  gsef->max_refval     = SIZE_MAX_VALUE;
  gsef->refval_digits  =
    (gse->update_policy == GIMP_SIZE_ENTRY_UPDATE_SIZE) ? 0 : 3;
  gsef->stop_recursion = 0;

  gsef->value_adjustment =
    GTK_OBJECT (gtk_spin_button_get_adjustment (value_spinbutton));
  gsef->value_spinbutton = GTK_WIDGET (value_spinbutton);
  g_signal_connect (gsef->value_adjustment, "value-changed",
                    G_CALLBACK (gimp_size_entry_value_callback),
                    gsef);

  if (gse->show_refval)
    {
      gsef->refval_adjustment =
        GTK_OBJECT (gtk_spin_button_get_adjustment (refval_spinbutton));
      gsef->refval_spinbutton = GTK_WIDGET (refval_spinbutton);
      g_signal_connect (gsef->refval_adjustment, "value-changed",
                        G_CALLBACK (gimp_size_entry_refval_callback),
                        gsef);
    }

  digits = ((gse->unit == GIMP_UNIT_PIXEL) ? gsef->refval_digits :
            (gse->unit == GIMP_UNIT_PERCENT) ? 2 :
            GIMP_SIZE_ENTRY_DIGITS (gse->unit));

  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (value_spinbutton), digits);

  if (gse->menu_show_pixels &&
      !gse->show_refval &&
      (gse->unit == GIMP_UNIT_PIXEL))
    {
      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                  gsef->refval_digits);
    }
}

/**
 * gimp_size_entry_attach_label:
 * @gse:       The sizeentry you want to add a label to.
 * @text:      The text of the label.
 * @row:       The row where the label will be attached.
 * @column:    The column where the label will be attached.
 * @alignment: The horizontal alignment of the label.
 *
 * Attaches a #GtkLabel to the #GimpSizeEntry (which is a #GtkTable).
 *
 * Returns: A pointer to the new #GtkLabel widget.
 **/
GtkWidget *
gimp_size_entry_attach_label (GimpSizeEntry *gse,
                              const gchar   *text,
                              gint           row,
                              gint           column,
                              gfloat         alignment)
{
  GtkWidget *label;

  g_return_val_if_fail (GIMP_IS_SIZE_ENTRY (gse), NULL);
  g_return_val_if_fail (text != NULL, NULL);

  label = gtk_label_new_with_mnemonic (text);

  if (column == 0)
    {
      GtkTableChild *child;
      GList         *list;

      for (list = GTK_TABLE (gse)->children; list; list = g_list_next (list))
        {
          child = (GtkTableChild *) list->data;

          if (child->left_attach == 1 && child->top_attach == row)
            {
              gtk_label_set_mnemonic_widget (GTK_LABEL (label),
                                             child->widget);
              break;
            }
        }
    }

  gtk_misc_set_alignment (GTK_MISC (label), alignment, 0.5);

  gtk_table_attach (GTK_TABLE (gse), label, column, column+1, row, row+1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  return label;
}


/**
 * gimp_size_entry_set_resolution:
 * @gse:        The sizeentry you want to set a resolution for.
 * @field:      The index of the field you want to set the resolution for.
 * @resolution: The new resolution (in dpi) for the chosen @field.
 * @keep_size:  %TRUE if the @field's size in pixels should stay the same.
 *              %FALSE if the @field's size in units should stay the same.
 *
 * Sets the resolution (in dpi) for field # @field of the #GimpSizeEntry.
 *
 * The @resolution passed will be clamped to fit in
 * [#GIMP_MIN_RESOLUTION..#GIMP_MAX_RESOLUTION].
 *
 * This function does nothing if the #GimpSizeEntryUpdatePolicy specified in
 * gimp_size_entry_new() doesn't equal to #GIMP_SIZE_ENTRY_UPDATE_SIZE.
 **/
void
gimp_size_entry_set_resolution (GimpSizeEntry *gse,
                                gint           field,
                                gdouble        resolution,
                                gboolean       keep_size)
{
  GimpSizeEntryField *gsef;
  gfloat              val;

  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));

  resolution = CLAMP (resolution, GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION);

  gsef = (GimpSizeEntryField*) g_slist_nth_data (gse->fields, field);
  gsef->resolution = resolution;

  val = gsef->value;

  gsef->stop_recursion = 0;
  gimp_size_entry_set_refval_boundaries (gse, field,
                                         gsef->min_refval, gsef->max_refval);

  if (! keep_size)
    gimp_size_entry_set_value (gse, field, val);
}


/**
 * gimp_size_entry_set_size:
 * @gse:   The sizeentry you want to set a size for.
 * @field: The index of the field you want to set the size for.
 * @lower: The reference value which will be treated as 0%.
 * @upper: The reference value which will be treated as 100%.
 *
 * Sets the pixel values for field # @field of the #GimpSizeEntry
 * which will be treated as 0% and 100%.
 *
 * These values will be used if you specified @menu_show_percent as %TRUE
 * in gimp_size_entry_new() and the user has selected GIMP_UNIT_PERCENT in
 * the #GimpSizeEntry's #GimpUnitMenu.
 *
 * This function does nothing if the #GimpSizeEntryUpdatePolicy specified in
 * gimp_size_entry_new() doesn't equal to GIMP_SIZE_ENTRY_UPDATE_SIZE.
 **/
void
gimp_size_entry_set_size (GimpSizeEntry *gse,
                          gint           field,
                          gdouble        lower,
                          gdouble        upper)
{
  GimpSizeEntryField *gsef;

  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));
  g_return_if_fail (lower <= upper);

  gsef = (GimpSizeEntryField*) g_slist_nth_data (gse->fields, field);
  gsef->lower = lower;
  gsef->upper = upper;

  gimp_size_entry_set_refval (gse, field, gsef->refval);
}


/**
 * gimp_size_entry_set_value_boundaries:
 * @gse:   The sizeentry you want to set value boundaries for.
 * @field: The index of the field you want to set value boundaries for.
 * @lower: The new lower boundary of the value of the chosen @field.
 * @upper: The new upper boundary of the value of the chosen @field.
 *
 * Limits the range of possible values which can be entered in field # @field
 * of the #GimpSizeEntry.
 *
 * The current value of the @field will be clamped to fit in the @field's
 * new boundaries.
 *
 * NOTE: In most cases you won't be interested in this function because the
 *       #GimpSizeEntry's purpose is to shield the programmer from unit
 *       calculations. Use gimp_size_entry_set_refval_boundaries() instead.
 *       Whatever you do, don't mix these calls. A size entry should either
 *       be clamped by the value or the reference value.
 **/
void
gimp_size_entry_set_value_boundaries (GimpSizeEntry *gse,
                                      gint           field,
                                      gdouble        lower,
                                      gdouble        upper)
{
  GimpSizeEntryField *gsef;

  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));
  g_return_if_fail (lower <= upper);

  gsef = (GimpSizeEntryField*) g_slist_nth_data (gse->fields, field);
  gsef->min_value        = lower;
  gsef->max_value        = upper;

  GTK_ADJUSTMENT (gsef->value_adjustment)->lower = gsef->min_value;
  GTK_ADJUSTMENT (gsef->value_adjustment)->upper = gsef->max_value;

  if (gsef->stop_recursion) /* this is a hack (but useful ;-) */
    return;

  gsef->stop_recursion++;
  switch (gsef->gse->update_policy)
    {
    case GIMP_SIZE_ENTRY_UPDATE_NONE:
      break;

    case GIMP_SIZE_ENTRY_UPDATE_SIZE:
      switch (gse->unit)
        {
        case GIMP_UNIT_PIXEL:
          gimp_size_entry_set_refval_boundaries (gse, field,
                                                 gsef->min_value,
                                                 gsef->max_value);
          break;
        case GIMP_UNIT_PERCENT:
          gimp_size_entry_set_refval_boundaries (gse, field,
                                                 gsef->lower +
                                                 (gsef->upper - gsef->lower) *
                                                 gsef->min_value / 100,
                                                 gsef->lower +
                                                 (gsef->upper - gsef->lower) *
                                                 gsef->max_value / 100);
          break;
        default:
          gimp_size_entry_set_refval_boundaries (gse, field,
                                                 gsef->min_value *
                                                 gsef->resolution /
                                                 gimp_unit_get_factor (gse->unit),
                                                 gsef->max_value *
                                                 gsef->resolution /
                                                 gimp_unit_get_factor (gse->unit));
          break;
        }
      break;

    case GIMP_SIZE_ENTRY_UPDATE_RESOLUTION:
      gimp_size_entry_set_refval_boundaries (gse, field,
                                             gsef->min_value *
                                             gimp_unit_get_factor (gse->unit),
                                             gsef->max_value *
                                             gimp_unit_get_factor (gse->unit));
      break;

    default:
      break;
    }
  gsef->stop_recursion--;

  gimp_size_entry_set_value (gse, field, gsef->value);
}

/**
 * gimp_size_entry_get_value;
 * @gse:   The sizeentry you want to know a value of.
 * @field: The index of the field you want to know the value of.
 *
 * Returns the value of field # @field of the #GimpSizeEntry.
 *
 * The @value returned is a distance or resolution
 * in the #GimpUnit the user has selected in the #GimpSizeEntry's
 * #GimpUnitMenu.
 *
 * NOTE: In most cases you won't be interested in this value because the
 *       #GimpSizeEntry's purpose is to shield the programmer from unit
 *       calculations. Use gimp_size_entry_get_refval() instead.
 *
 * Returns: The value of the chosen @field.
 **/
gdouble
gimp_size_entry_get_value (GimpSizeEntry *gse,
                           gint           field)
{
  GimpSizeEntryField *gsef;

  g_return_val_if_fail (GIMP_IS_SIZE_ENTRY (gse), 0);
  g_return_val_if_fail ((field >= 0) && (field < gse->number_of_fields), 0);

  gsef = (GimpSizeEntryField *) g_slist_nth_data (gse->fields, field);
  return gsef->value;
}

static void
gimp_size_entry_update_value (GimpSizeEntryField *gsef,
                              gdouble             value)
{
  if (gsef->stop_recursion > 1)
    return;

  gsef->value = value;

  switch (gsef->gse->update_policy)
    {
    case GIMP_SIZE_ENTRY_UPDATE_NONE:
      break;

    case GIMP_SIZE_ENTRY_UPDATE_SIZE:
      switch (gsef->gse->unit)
        {
        case GIMP_UNIT_PIXEL:
          gsef->refval = value;
          break;
        case GIMP_UNIT_PERCENT:
          gsef->refval =
            CLAMP (gsef->lower + (gsef->upper - gsef->lower) * value / 100,
                   gsef->min_refval, gsef->max_refval);
          break;
        default:
          gsef->refval =
            CLAMP (value * gsef->resolution /
                   gimp_unit_get_factor (gsef->gse->unit),
                   gsef->min_refval, gsef->max_refval);
          break;
        }
      if (gsef->gse->show_refval)
        gtk_adjustment_set_value (GTK_ADJUSTMENT (gsef->refval_adjustment),
                                  gsef->refval);
      break;

    case GIMP_SIZE_ENTRY_UPDATE_RESOLUTION:
      gsef->refval =
        CLAMP (value * gimp_unit_get_factor (gsef->gse->unit),
               gsef->min_refval, gsef->max_refval);
      if (gsef->gse->show_refval)
        gtk_adjustment_set_value (GTK_ADJUSTMENT (gsef->refval_adjustment),
                                  gsef->refval);
      break;

    default:
      break;
    }

  g_signal_emit (gsef->gse, gimp_size_entry_signals[VALUE_CHANGED], 0);
}

/**
 * gimp_size_entry_set_value;
 * @gse:   The sizeentry you want to set a value for.
 * @field: The index of the field you want to set a value for.
 * @value: The new value for @field.
 *
 * Sets the value for field # @field of the #GimpSizeEntry.
 *
 * The @value passed is treated to be a distance or resolution
 * in the #GimpUnit the user has selected in the #GimpSizeEntry's
 * #GimpUnitMenu.
 *
 * NOTE: In most cases you won't be interested in this value because the
 *       #GimpSizeEntry's purpose is to shield the programmer from unit
 *       calculations. Use gimp_size_entry_set_refval() instead.
 **/
void
gimp_size_entry_set_value (GimpSizeEntry *gse,
                           gint           field,
                           gdouble        value)
{
  GimpSizeEntryField *gsef;

  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));

  gsef = (GimpSizeEntryField *) g_slist_nth_data (gse->fields, field);

  value = CLAMP (value, gsef->min_value, gsef->max_value);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (gsef->value_adjustment), value);
  gimp_size_entry_update_value (gsef, value);
}


static void
gimp_size_entry_value_callback (GtkWidget *widget,
                                gpointer   data)
{
  GimpSizeEntryField *gsef;
  gdouble             new_value;

  gsef = (GimpSizeEntryField *) data;

  new_value = GTK_ADJUSTMENT (widget)->value;

  if (gsef->value != new_value)
    gimp_size_entry_update_value (gsef, new_value);
}


/**
 * gimp_size_entry_set_refval_boundaries:
 * @gse:   The sizeentry you want to set the reference value boundaries for.
 * @field: The index of the field you want to set the reference value
 *         boundaries for.
 * @lower: The new lower boundary of the reference value of the chosen @field.
 * @upper: The new upper boundary of the reference value of the chosen @field.
 *
 * Limits the range of possible reference values which can be entered in
 * field # @field of the #GimpSizeEntry.
 *
 * The current reference value of the @field will be clamped to fit in the
 * @field's new boundaries.
 **/
void
gimp_size_entry_set_refval_boundaries (GimpSizeEntry *gse,
                                       gint           field,
                                       gdouble        lower,
                                       gdouble        upper)
{
  GimpSizeEntryField *gsef;

  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));
  g_return_if_fail (lower <= upper);

  gsef = (GimpSizeEntryField *) g_slist_nth_data (gse->fields, field);
  gsef->min_refval = lower;
  gsef->max_refval = upper;

  if (gse->show_refval)
    {
      GTK_ADJUSTMENT (gsef->refval_adjustment)->lower = gsef->min_refval;
      GTK_ADJUSTMENT (gsef->refval_adjustment)->upper = gsef->max_refval;
    }

  if (gsef->stop_recursion) /* this is a hack (but useful ;-) */
    return;

  gsef->stop_recursion++;
  switch (gsef->gse->update_policy)
    {
    case GIMP_SIZE_ENTRY_UPDATE_NONE:
      break;

    case GIMP_SIZE_ENTRY_UPDATE_SIZE:
      switch (gse->unit)
        {
        case GIMP_UNIT_PIXEL:
          gimp_size_entry_set_value_boundaries (gse, field,
                                                gsef->min_refval,
                                                gsef->max_refval);
          break;
        case GIMP_UNIT_PERCENT:
          gimp_size_entry_set_value_boundaries (gse, field,
                                                100 * (gsef->min_refval -
                                                       gsef->lower) /
                                                (gsef->upper - gsef->lower),
                                                100 * (gsef->max_refval -
                                                       gsef->lower) /
                                                (gsef->upper - gsef->lower));
          break;
        default:
          gimp_size_entry_set_value_boundaries (gse, field,
                                                gsef->min_refval *
                                                gimp_unit_get_factor (gse->unit) /
                                                gsef->resolution,
                                                gsef->max_refval *
                                                gimp_unit_get_factor (gse->unit) /
                                                gsef->resolution);
          break;
        }
      break;

    case GIMP_SIZE_ENTRY_UPDATE_RESOLUTION:
      gimp_size_entry_set_value_boundaries (gse, field,
                                            gsef->min_refval /
                                            gimp_unit_get_factor (gse->unit),
                                            gsef->max_refval /
                                            gimp_unit_get_factor (gse->unit));
      break;

    default:
      break;
    }
  gsef->stop_recursion--;

  gimp_size_entry_set_refval (gse, field, gsef->refval);
}

/**
 * gimp_size_entry_set_refval_digits:
 * @gse:    The sizeentry you want to set the reference value digits for.
 * @field:  The index of the field you want to set the reference value for.
 * @digits: The new number of decimal digits for the #GtkSpinButton which
 *          displays @field's reference value.
 *
 * Sets the decimal digits of field # @field of the #GimpSizeEntry to
 * @digits.
 *
 * If you don't specify this value explicitly, the reference value's number
 * of digits will equal to 0 for #GIMP_SIZE_ENTRY_UPDATE_SIZE and to 2 for
 * #GIMP_SIZE_ENTRY_UPDATE_RESOLUTION.
 **/
void
gimp_size_entry_set_refval_digits (GimpSizeEntry *gse,
                                   gint           field,
                                   gint           digits)
{
  GimpSizeEntryField *gsef;

  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));
  g_return_if_fail ((digits >= 0) && (digits <= 6));

  gsef = (GimpSizeEntryField*) g_slist_nth_data (gse->fields, field);
  gsef->refval_digits = digits;

  if (gse->update_policy == GIMP_SIZE_ENTRY_UPDATE_SIZE)
    {
      if (gse->show_refval)
        gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->refval_spinbutton),
                                    gsef->refval_digits);
      else if (gse->unit == GIMP_UNIT_PIXEL)
        gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                    gsef->refval_digits);
    }
}

/**
 * gimp_size_entry_get_refval;
 * @gse:   The sizeentry you want to know a reference value of.
 * @field: The index of the field you want to know the reference value of.
 *
 * Returns the reference value for field # @field of the #GimpSizeEntry.
 *
 * The reference value is either a distance in pixels or a resolution
 * in dpi, depending on which #GimpSizeEntryUpdatePolicy you chose in
 * gimp_size_entry_new().
 *
 * Returns: The reference value of the chosen @field.
 **/
gdouble
gimp_size_entry_get_refval (GimpSizeEntry *gse,
                            gint           field)
{
  GimpSizeEntryField *gsef;

  /*  return 1.0 to avoid division by zero  */
  g_return_val_if_fail (GIMP_IS_SIZE_ENTRY (gse), 1.0);
  g_return_val_if_fail ((field >= 0) && (field < gse->number_of_fields), 1.0);

  gsef = (GimpSizeEntryField*) g_slist_nth_data (gse->fields, field);
  return gsef->refval;
}

static void
gimp_size_entry_update_refval (GimpSizeEntryField *gsef,
                               gdouble             refval)
{
  if (gsef->stop_recursion > 1)
    return;

  gsef->refval = refval;

  switch (gsef->gse->update_policy)
    {
    case GIMP_SIZE_ENTRY_UPDATE_NONE:
      break;

    case GIMP_SIZE_ENTRY_UPDATE_SIZE:
      switch (gsef->gse->unit)
        {
        case GIMP_UNIT_PIXEL:
          gsef->value = refval;
          break;
        case GIMP_UNIT_PERCENT:
          gsef->value =
            CLAMP (100 * (refval - gsef->lower) / (gsef->upper - gsef->lower),
                   gsef->min_value, gsef->max_value);
          break;
        default:
          gsef->value =
            CLAMP (refval * gimp_unit_get_factor (gsef->gse->unit) /
                   gsef->resolution,
                   gsef->min_value, gsef->max_value);
          break;
        }
      gtk_adjustment_set_value (GTK_ADJUSTMENT (gsef->value_adjustment),
                                gsef->value);
      break;

    case GIMP_SIZE_ENTRY_UPDATE_RESOLUTION:
      gsef->value =
        CLAMP (refval / gimp_unit_get_factor (gsef->gse->unit),
               gsef->min_value, gsef->max_value);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (gsef->value_adjustment),
                                gsef->value);
      break;

    default:
      break;
    }

  g_signal_emit (gsef->gse, gimp_size_entry_signals[REFVAL_CHANGED], 0);
}

/**
 * gimp_size_entry_set_refval;
 * @gse:    The sizeentry you want to set a reference value for.
 * @field:  The index of the field you want to set the reference value for.
 * @refval: The new reference value for @field.
 *
 * Sets the reference value for field # @field of the #GimpSizeEntry.
 *
 * The @refval passed is either a distance in pixels or a resolution in dpi,
 * depending on which #GimpSizeEntryUpdatePolicy you chose in
 * gimp_size_entry_new().
 **/
void
gimp_size_entry_set_refval (GimpSizeEntry *gse,
                            gint           field,
                            gdouble        refval)
{
  GimpSizeEntryField *gsef;

  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));

  gsef = (GimpSizeEntryField *) g_slist_nth_data (gse->fields, field);

  refval = CLAMP (refval, gsef->min_refval, gsef->max_refval);

  if (gse->show_refval)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (gsef->refval_adjustment),
                              refval);

  gimp_size_entry_update_refval (gsef, refval);
}

static void
gimp_size_entry_refval_callback (GtkWidget *widget,
                                 gpointer   data)
{
  GimpSizeEntryField *gsef;
  gdouble             new_refval;

  gsef = (GimpSizeEntryField *) data;

  new_refval = GTK_ADJUSTMENT (widget)->value;

  if (gsef->refval != new_refval)
    gimp_size_entry_update_refval (gsef, new_refval);
}


/**
 * gimp_size_entry_get_unit:
 * @gse: The sizeentry you want to know the unit of.
 *
 * Returns the #GimpUnit the user has selected in the #GimpSizeEntry's
 * #GimpUnitMenu.
 *
 * Returns: The sizeentry's unit.
 **/
GimpUnit
gimp_size_entry_get_unit (GimpSizeEntry *gse)
{
  g_return_val_if_fail (GIMP_IS_SIZE_ENTRY (gse), GIMP_UNIT_INCH);

  return gse->unit;
}

static void
gimp_size_entry_update_unit (GimpSizeEntry *gse,
                             GimpUnit       unit)
{
  GimpSizeEntryField *gsef;
  gint                i;
  gint                digits;

  gse->unit = unit;

  digits = gimp_unit_menu_get_pixel_digits (GIMP_UNIT_MENU (gse->unitmenu));

  for (i = 0; i < gse->number_of_fields; i++)
    {
      gsef = (GimpSizeEntryField *) g_slist_nth_data (gse->fields, i);

      if (gse->update_policy == GIMP_SIZE_ENTRY_UPDATE_SIZE)
        {
          if (unit == GIMP_UNIT_PIXEL)
            gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                        gsef->refval_digits + digits);
          else if (unit == GIMP_UNIT_PERCENT)
            gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                        2 + digits);
          else
            gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                        GIMP_SIZE_ENTRY_DIGITS (unit) + digits);
        }
      else if (gse->update_policy == GIMP_SIZE_ENTRY_UPDATE_RESOLUTION)
        {
          digits = (gimp_unit_get_digits (GIMP_UNIT_INCH) -
                    gimp_unit_get_digits (unit));
          gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                      MAX (3 + digits, 3));
        }

      gsef->stop_recursion = 0; /* hack !!! */

      gimp_size_entry_set_refval_boundaries (gse, i,
                                             gsef->min_refval,
                                             gsef->max_refval);
    }

  g_signal_emit (gse, gimp_size_entry_signals[UNIT_CHANGED], 0);
}


/**
 * gimp_size_entry_set_unit:
 * @gse:  The sizeentry you want to change the unit for.
 * @unit: The new unit.
 *
 * Sets the #GimpSizeEntry's unit. The reference value for all fields will
 * stay the same but the value in units or pixels per unit will change
 * according to which #GimpSizeEntryUpdatePolicy you chose in
 * gimp_size_entry_new().
 **/
void
gimp_size_entry_set_unit (GimpSizeEntry *gse,
                          GimpUnit       unit)
{
  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail (gse->menu_show_pixels || (unit != GIMP_UNIT_PIXEL));
  g_return_if_fail (gse->menu_show_percent || (unit != GIMP_UNIT_PERCENT));

  gimp_unit_menu_set_unit (GIMP_UNIT_MENU (gse->unitmenu), unit);
  gimp_size_entry_update_unit (gse, unit);
}

static void
gimp_size_entry_unit_callback (GtkWidget     *widget,
                               GimpSizeEntry *gse)
{
  GimpUnit new_unit;

  new_unit = gimp_unit_menu_get_unit (GIMP_UNIT_MENU (widget));

  if (gse->unit != new_unit)
    gimp_size_entry_update_unit (gse, new_unit);
}

/**
 * gimp_size_entry_show_unit_menu:
 * @gse: a #GimpSizeEntry
 * @show: Boolean
 *
 * Controls whether a unit menu is shown in the size entry.  If
 * @show is #TRUE, the menu is shown; otherwise it is hidden.
 *
 * Since: GIMP 2.4
 **/
void
gimp_size_entry_show_unit_menu (GimpSizeEntry *gse,
                                gboolean       show)
{
  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));

  if (show)
    gtk_widget_show (gse->unitmenu);
  else
    gtk_widget_hide (gse->unitmenu);
}


/**
 * gimp_size_entry_set_pixel_digits:
 * @gse: a #GimpSizeEntry
 * @digits: the number of digits to display for a pixel size
 *
 * Similar to gimp_unit_menu_set_pixel_digits(), this function allows
 * you set up a #GimpSizeEntry so that sub-pixel sizes can be entered.
 **/
void
gimp_size_entry_set_pixel_digits (GimpSizeEntry *gse,
                                  gint           digits)
{
  GimpUnitMenu *menu;

  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));

  menu = GIMP_UNIT_MENU (gse->unitmenu);

  gimp_unit_menu_set_pixel_digits (menu, digits);
  gimp_size_entry_update_unit (gse, gimp_unit_menu_get_unit (menu));
}


/**
 * gimp_size_entry_grab_focus:
 * @gse: The sizeentry you want to grab the keyboard focus.
 *
 * This function is rather ugly and just a workaround for the fact that
 * it's impossible to implement gtk_widget_grab_focus() for a #GtkTable.
 **/
void
gimp_size_entry_grab_focus (GimpSizeEntry *gse)
{
  GimpSizeEntryField *gsef;

  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));

  gsef = gse->fields->data;
  if (gsef)
    gtk_widget_grab_focus (gse->show_refval ?
                           gsef->refval_spinbutton : gsef->value_spinbutton);
}

/**
 * gimp_size_entry_set_activates_default:
 * @gse: A #GimpSizeEntr
 * @setting: %TRUE to activate window's default widget on Enter keypress
 *
 * Iterates over all entries in the #GimpSizeEntry and calls
 * gtk_entry_set_activates_default() on them.
 *
 * Since: GIMP 2.4
 **/
void
gimp_size_entry_set_activates_default (GimpSizeEntry *gse,
                                       gboolean       setting)
{
  GSList *list;

  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));

  for (list = gse->fields; list; list = g_slist_next (list))
    {
      GimpSizeEntryField *gsef = list->data;

      if (gsef->value_spinbutton)
        gtk_entry_set_activates_default (GTK_ENTRY (gsef->value_spinbutton),
                                         setting);

      if (gsef->refval_spinbutton)
        gtk_entry_set_activates_default (GTK_ENTRY (gsef->refval_spinbutton),
                                         setting);
    }
}

/**
 * gimp_size_entry_get_help_widget:
 * @gse: a #GimpSizeEntry
 * @field: the index of the widget you want to get a pointer to
 *
 * You shouldn't fiddle with the internals of a #GimpSizeEntry but
 * if you want to set tooltips using gimp_help_set_help_data() you
 * can use this function to get a pointer to the spinbuttons.
 *
 * Return value: a #GtkWidget pointer that you can attach a tooltip to.
 **/
GtkWidget *
gimp_size_entry_get_help_widget (GimpSizeEntry *gse,
                                 gint           field)
{
  GimpSizeEntryField *gsef;

  g_return_val_if_fail (GIMP_IS_SIZE_ENTRY (gse), NULL);
  g_return_val_if_fail ((field >= 0) && (field < gse->number_of_fields), NULL);

  gsef = g_slist_nth_data (gse->fields, field);
  if (!gsef)
    return NULL;

  return (gsef->refval_spinbutton ?
          gsef->refval_spinbutton : gsef->value_spinbutton);
}
