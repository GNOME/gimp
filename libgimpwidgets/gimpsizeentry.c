/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmasizeentry.c
 * Copyright (C) 1999-2000 Sven Neumann <sven@ligma.org>
 *                         Michael Natterer <mitch@ligma.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"

#include "ligmawidgets.h"

#include "ligmaeevl.h"
#include "ligmasizeentry.h"


/**
 * SECTION: ligmasizeentry
 * @title: LigmaSizeEntry
 * @short_description: Widget for entering pixel values and resolutions.
 * @see_also: #LigmaUnit, #LigmaUnitComboBox, ligma_coordinates_new()
 *
 * This widget is used to enter pixel distances/sizes and resolutions.
 *
 * You can specify the number of fields the widget should provide. For
 * each field automatic mappings are performed between the field's
 * "reference value" and its "value".
 *
 * There is a #LigmaUnitComboBox right of the entry fields which lets
 * you specify the #LigmaUnit of the displayed values.
 *
 * For each field, there can be one or two #GtkSpinButton's to enter
 * "value" and "reference value". If you specify @show_refval as
 * %FALSE in ligma_size_entry_new() there will be only one
 * #GtkSpinButton and the #LigmaUnitComboBox will contain an item for
 * selecting LIGMA_UNIT_PIXEL.
 *
 * The "reference value" is either of LIGMA_UNIT_PIXEL or dpi,
 * depending on which #LigmaSizeEntryUpdatePolicy you specify in
 * ligma_size_entry_new().  The "value" is either the size in pixels
 * mapped to the size in a real-world-unit (see #LigmaUnit) or the dpi
 * value mapped to pixels per real-world-unit.
 **/


#define SIZE_MAX_VALUE 500000.0

#define LIGMA_SIZE_ENTRY_DIGITS(unit) (MIN (ligma_unit_get_digits (unit), 5) + 1)


enum
{
  VALUE_CHANGED,
  REFVAL_CHANGED,
  UNIT_CHANGED,
  LAST_SIGNAL
};


struct _LigmaSizeEntryField
{
  LigmaSizeEntry *gse;

  gdouble        resolution;
  gdouble        lower;
  gdouble        upper;

  GtkAdjustment *value_adjustment;
  GtkWidget     *value_spinbutton;
  gdouble        value;
  gdouble        min_value;
  gdouble        max_value;

  GtkAdjustment *refval_adjustment;
  GtkWidget     *refval_spinbutton;
  gdouble        refval;
  gdouble        min_refval;
  gdouble        max_refval;
  gint           refval_digits;

  gint           stop_recursion;
};


struct _LigmaSizeEntryPrivate
{
  GSList                    *fields;
  gint                       number_of_fields;

  GtkWidget                 *unit_combo;
  LigmaUnit                   unit;
  gboolean                   menu_show_pixels;
  gboolean                   menu_show_percent;

  gboolean                   show_refval;
  LigmaSizeEntryUpdatePolicy  update_policy;
};

#define GET_PRIVATE(obj) (((LigmaSizeEntry *) (obj))->priv)


static void      ligma_size_entry_finalize            (GObject            *object);
static void      ligma_size_entry_update_value        (LigmaSizeEntryField *gsef,
                                                      gdouble             value);
static void      ligma_size_entry_value_callback      (GtkAdjustment      *adjustment,
                                                      gpointer            data);
static void      ligma_size_entry_update_refval       (LigmaSizeEntryField *gsef,
                                                      gdouble             refval);
static void      ligma_size_entry_refval_callback     (GtkAdjustment      *adjustment,
                                                      gpointer            data);
static void      ligma_size_entry_update_unit         (LigmaSizeEntry      *gse,
                                                      LigmaUnit            unit);
static void      ligma_size_entry_unit_callback       (GtkWidget          *widget,
                                                      LigmaSizeEntry      *sizeentry);
static void      ligma_size_entry_attach_eevl         (GtkSpinButton      *spin_button,
                                                      LigmaSizeEntryField *gsef);
static gint      ligma_size_entry_eevl_input_callback (GtkSpinButton      *spinner,
                                                      gdouble            *return_val,
                                                      gpointer           *data);
static gboolean  ligma_size_entry_eevl_unit_resolver  (const gchar        *ident,
                                                      LigmaEevlQuantity   *factor,
                                                      gdouble            *offset,
                                                      gpointer            data);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaSizeEntry, ligma_size_entry, GTK_TYPE_GRID)

#define parent_class ligma_size_entry_parent_class

static guint ligma_size_entry_signals[LAST_SIGNAL] = { 0 };


static void
ligma_size_entry_class_init (LigmaSizeEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  ligma_size_entry_signals[VALUE_CHANGED] =
    g_signal_new ("value-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaSizeEntryClass, value_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_size_entry_signals[REFVAL_CHANGED] =
    g_signal_new ("refval-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaSizeEntryClass, refval_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_size_entry_signals[UNIT_CHANGED] =
    g_signal_new ("unit-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaSizeEntryClass, unit_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize = ligma_size_entry_finalize;

  klass->value_changed   = NULL;
  klass->refval_changed  = NULL;
  klass->unit_changed    = NULL;
}

static void
ligma_size_entry_init (LigmaSizeEntry *gse)
{
  LigmaSizeEntryPrivate *priv;

  gse->priv = ligma_size_entry_get_instance_private (gse);

  priv = gse->priv;

  priv->unit              = LIGMA_UNIT_PIXEL;
  priv->menu_show_pixels  = TRUE;
  priv->menu_show_percent = TRUE;
  priv->show_refval       = FALSE;
  priv->update_policy     = LIGMA_SIZE_ENTRY_UPDATE_NONE;
}

static void
ligma_size_entry_finalize (GObject *object)
{
  LigmaSizeEntryPrivate *priv = GET_PRIVATE (object);

  if (priv->fields)
    {
      GSList *list;

      for (list = priv->fields; list; list = list->next)
        g_slice_free (LigmaSizeEntryField, list->data);

      g_slist_free (priv->fields);
      priv->fields = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * ligma_size_entry_new:
 * @number_of_fields:  The number of input fields.
 * @unit:              The initial unit.
 * @unit_format:       A printf-like unit-format string as is used with
 *                     ligma_unit_menu_new().
 * @menu_show_pixels:  %TRUE if the unit menu should contain an item for
 *                     LIGMA_UNIT_PIXEL (ignored if the @update_policy is not
 *                     LIGMA_SIZE_ENTRY_UPDATE_NONE).
 * @menu_show_percent: %TRUE if the unit menu should contain an item for
 *                     LIGMA_UNIT_PERCENT.
 * @show_refval:       %TRUE if you want an extra "reference value"
 *                     spinbutton per input field.
 * @spinbutton_width:  The minimal horizontal size of the #GtkSpinButton's.
 * @update_policy:     How the automatic pixel <-> real-world-unit
 *                     calculations should be done.
 *
 * Creates a new #LigmaSizeEntry widget.
 *
 * To have all automatic calculations performed correctly, set up the
 * widget in the following order:
 *
 * 1. ligma_size_entry_new()
 *
 * 2. (for each additional input field) ligma_size_entry_add_field()
 *
 * 3. ligma_size_entry_set_unit()
 *
 * For each input field:
 *
 * 4. ligma_size_entry_set_resolution()
 *
 * 5. ligma_size_entry_set_refval_boundaries()
 *    (or ligma_size_entry_set_value_boundaries())
 *
 * 6. ligma_size_entry_set_size()
 *
 * 7. ligma_size_entry_set_refval() (or ligma_size_entry_set_value())
 *
 * The #LigmaSizeEntry is derived from #GtkGrid and will have
 * an empty border of one cell width on each side plus an empty column left
 * of the #LigmaUnitComboBox to allow the caller to add labels or a
 * #LigmaChainButton.
 *
 * Returns: A Pointer to the new #LigmaSizeEntry widget.
 **/
GtkWidget *
ligma_size_entry_new (gint                       number_of_fields,
                     LigmaUnit                   unit,
                     const gchar               *unit_format,
                     gboolean                   menu_show_pixels,
                     gboolean                   menu_show_percent,
                     gboolean                   show_refval,
                     gint                       spinbutton_width,
                     LigmaSizeEntryUpdatePolicy  update_policy)
{
  LigmaSizeEntry        *gse;
  LigmaSizeEntryPrivate *priv;
  LigmaUnitStore        *store;
  gint                  i;

  g_return_val_if_fail ((number_of_fields >= 0) && (number_of_fields <= 16),
                        NULL);

  gse = g_object_new (LIGMA_TYPE_SIZE_ENTRY, NULL);

  priv = GET_PRIVATE (gse);

  priv->number_of_fields = number_of_fields;
  priv->unit             = unit;
  priv->show_refval      = show_refval;
  priv->update_policy    = update_policy;

  /*  show the 'pixels' menu entry only if we are a 'size' sizeentry and
   *  don't have the reference value spinbutton
   */
  if ((update_policy == LIGMA_SIZE_ENTRY_UPDATE_RESOLUTION) ||
      (show_refval == TRUE))
    priv->menu_show_pixels = FALSE;
  else
    priv->menu_show_pixels = menu_show_pixels;

  /*  show the 'percent' menu entry only if we are a 'size' sizeentry
   */
  if (update_policy == LIGMA_SIZE_ENTRY_UPDATE_RESOLUTION)
    priv->menu_show_percent = FALSE;
  else
    priv->menu_show_percent = menu_show_percent;

  for (i = 0; i < number_of_fields; i++)
    {
      LigmaSizeEntryField *gsef = g_slice_new0 (LigmaSizeEntryField);
      gint                digits;

      priv->fields = g_slist_append (priv->fields, gsef);

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
        (update_policy == LIGMA_SIZE_ENTRY_UPDATE_SIZE) ? 0 : 3;
      gsef->stop_recursion    = 0;

      digits = ((unit == LIGMA_UNIT_PIXEL) ?
                gsef->refval_digits : ((unit == LIGMA_UNIT_PERCENT) ?
                                       2 : LIGMA_SIZE_ENTRY_DIGITS (unit)));

      gsef->value_adjustment = gtk_adjustment_new (gsef->value,
                                                   gsef->min_value,
                                                   gsef->max_value,
                                                   1.0, 10.0, 0.0);
      gsef->value_spinbutton = ligma_spin_button_new (gsef->value_adjustment,
                                                     1.0, digits);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                   TRUE);

      ligma_size_entry_attach_eevl (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                   gsef);

      if (spinbutton_width > 0)
        {
          if (spinbutton_width < 17)
            gtk_entry_set_width_chars (GTK_ENTRY (gsef->value_spinbutton),
                                       spinbutton_width);
          else
            gtk_widget_set_size_request (gsef->value_spinbutton,
                                         spinbutton_width, -1);
        }

      gtk_grid_attach (GTK_GRID (gse), gsef->value_spinbutton,
                       i+1, priv->show_refval+1, 1, 1);
      g_signal_connect (gsef->value_adjustment, "value-changed",
                        G_CALLBACK (ligma_size_entry_value_callback),
                        gsef);

      gtk_widget_show (gsef->value_spinbutton);

      if (priv->show_refval)
        {
          gsef->refval_adjustment = gtk_adjustment_new (gsef->refval,
                                                        gsef->min_refval,
                                                        gsef->max_refval,
                                                        1.0, 10.0, 0.0);
          gsef->refval_spinbutton = ligma_spin_button_new (gsef->refval_adjustment,
                                                          1.0,
                                                          gsef->refval_digits);
          gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (gsef->refval_spinbutton),
                                       TRUE);

          gtk_widget_set_size_request (gsef->refval_spinbutton,
                                       spinbutton_width, -1);
          gtk_grid_attach (GTK_GRID (gse), gsef->refval_spinbutton,
                           i + 1, 1, 1, 1);
          g_signal_connect (gsef->refval_adjustment,
                            "value-changed",
                            G_CALLBACK (ligma_size_entry_refval_callback),
                            gsef);

          gtk_widget_show (gsef->refval_spinbutton);
        }

      if (priv->menu_show_pixels && (unit == LIGMA_UNIT_PIXEL) &&
          ! priv->show_refval)
        gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                    gsef->refval_digits);
    }

  store = ligma_unit_store_new (priv->number_of_fields);
  ligma_unit_store_set_has_pixels (store, priv->menu_show_pixels);
  ligma_unit_store_set_has_percent (store, priv->menu_show_percent);

  if (unit_format)
    {
      gchar *short_format = g_strdup (unit_format);
      gchar *p;

      p = strstr (short_format, "%s");
      if (p)
        strcpy (p, "%a");

      p = strstr (short_format, "%p");
      if (p)
        strcpy (p, "%a");

      g_object_set (store,
                    "short-format", short_format,
                    "long-format",  unit_format,
                    NULL);

      g_free (short_format);
    }

  priv->unit_combo = ligma_unit_combo_box_new_with_model (store);
  g_object_unref (store);

  ligma_unit_combo_box_set_active (LIGMA_UNIT_COMBO_BOX (priv->unit_combo), unit);

  gtk_grid_attach (GTK_GRID (gse), priv->unit_combo,
                    i+2, priv->show_refval+1, 1, 1);
  g_signal_connect (priv->unit_combo, "changed",
                    G_CALLBACK (ligma_size_entry_unit_callback),
                    gse);
  gtk_widget_show (priv->unit_combo);

  return GTK_WIDGET (gse);
}


/**
 * ligma_size_entry_add_field:
 * @gse:               The sizeentry you want to add a field to.
 * @value_spinbutton:  The spinbutton to display the field's value.
 * @refval_spinbutton: (nullable): The spinbutton to display the field's reference value.
 *
 * Adds an input field to the #LigmaSizeEntry.
 *
 * The new input field will have the index 0. If you specified @show_refval
 * as %TRUE in ligma_size_entry_new() you have to pass an additional
 * #GtkSpinButton to hold the reference value. If @show_refval was %FALSE,
 * @refval_spinbutton will be ignored.
 **/
void
ligma_size_entry_add_field  (LigmaSizeEntry *gse,
                            GtkSpinButton *value_spinbutton,
                            GtkSpinButton *refval_spinbutton)
{
  LigmaSizeEntryPrivate *priv;
  LigmaSizeEntryField   *gsef;
  gint                  digits;

  g_return_if_fail (LIGMA_IS_SIZE_ENTRY (gse));
  g_return_if_fail (GTK_IS_SPIN_BUTTON (value_spinbutton));

  priv = GET_PRIVATE (gse);

  if (priv->show_refval)
    {
      g_return_if_fail (GTK_IS_SPIN_BUTTON (refval_spinbutton));
    }

  gsef = g_slice_new0 (LigmaSizeEntryField);

  priv->fields = g_slist_prepend (priv->fields, gsef);
  priv->number_of_fields++;

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
    (priv->update_policy == LIGMA_SIZE_ENTRY_UPDATE_SIZE) ? 0 : 3;
  gsef->stop_recursion = 0;

  gsef->value_adjustment = gtk_spin_button_get_adjustment (value_spinbutton);
  gsef->value_spinbutton = GTK_WIDGET (value_spinbutton);
  g_signal_connect (gsef->value_adjustment, "value-changed",
                    G_CALLBACK (ligma_size_entry_value_callback),
                    gsef);

  ligma_size_entry_attach_eevl (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                               gsef);

  if (priv->show_refval)
    {
      gsef->refval_adjustment = gtk_spin_button_get_adjustment (refval_spinbutton);
      gsef->refval_spinbutton = GTK_WIDGET (refval_spinbutton);
      g_signal_connect (gsef->refval_adjustment, "value-changed",
                        G_CALLBACK (ligma_size_entry_refval_callback),
                        gsef);
    }

  digits = ((priv->unit == LIGMA_UNIT_PIXEL) ? gsef->refval_digits :
            (priv->unit == LIGMA_UNIT_PERCENT) ? 2 :
            LIGMA_SIZE_ENTRY_DIGITS (priv->unit));

  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (value_spinbutton), digits);

  if (priv->menu_show_pixels &&
      !priv->show_refval &&
      (priv->unit == LIGMA_UNIT_PIXEL))
    {
      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                  gsef->refval_digits);
    }
}

LigmaSizeEntryUpdatePolicy
ligma_size_entry_get_update_policy (LigmaSizeEntry *gse)
{
  g_return_val_if_fail (LIGMA_IS_SIZE_ENTRY (gse), LIGMA_SIZE_ENTRY_UPDATE_SIZE);

  return GET_PRIVATE (gse)->update_policy;
}

gint
ligma_size_entry_get_n_fields (LigmaSizeEntry *gse)
{
  g_return_val_if_fail (LIGMA_IS_SIZE_ENTRY (gse), 0);

  return GET_PRIVATE (gse)->number_of_fields;
}

/**
 * ligma_size_entry_get_unit_combo:
 * @gse: a #LigmaSizeEntry.
 *
 * Returns: (transfer none) (type LigmaUnitComboBox): the size entry's #LigmaUnitComboBox.
 **/
GtkWidget *
ligma_size_entry_get_unit_combo (LigmaSizeEntry *gse)
{
  g_return_val_if_fail (LIGMA_IS_SIZE_ENTRY (gse), NULL);

  return GET_PRIVATE (gse)->unit_combo;
}

/**
 * ligma_size_entry_attach_label:
 * @gse:       The sizeentry you want to add a label to.
 * @text:      The text of the label.
 * @row:       The row where the label will be attached.
 * @column:    The column where the label will be attached.
 * @alignment: The horizontal alignment of the label.
 *
 * Attaches a #GtkLabel to the #LigmaSizeEntry (which is a #GtkGrid).
 *
 * Returns: (transfer none): A pointer to the new #GtkLabel widget.
 **/
GtkWidget *
ligma_size_entry_attach_label (LigmaSizeEntry *gse,
                              const gchar   *text,
                              gint           row,
                              gint           column,
                              gfloat         alignment)
{
  GtkWidget *label;

  g_return_val_if_fail (LIGMA_IS_SIZE_ENTRY (gse), NULL);
  g_return_val_if_fail (text != NULL, NULL);

  label = gtk_label_new_with_mnemonic (text);

  if (column == 0)
    {
      GList *children;
      GList *list;

      children = gtk_container_get_children (GTK_CONTAINER (gse));

      for (list = children; list; list = g_list_next (list))
        {
          GtkWidget *child = list->data;
          gint       left_attach;
          gint       top_attach;

          gtk_container_child_get (GTK_CONTAINER (gse), child,
                                   "left-attach", &left_attach,
                                   "top-attach",  &top_attach,
                                   NULL);

          if (left_attach == 1 && top_attach == row)
            {
              gtk_label_set_mnemonic_widget (GTK_LABEL (label), child);
              break;
            }
        }

      g_list_free (children);
    }

  gtk_label_set_xalign (GTK_LABEL (label), alignment);

  gtk_grid_attach (GTK_GRID (gse), label, column, row, 1, 1);
  gtk_widget_show (label);

  return label;
}


/**
 * ligma_size_entry_set_resolution:
 * @gse:        The sizeentry you want to set a resolution for.
 * @field:      The index of the field you want to set the resolution for.
 * @resolution: The new resolution (in dpi) for the chosen @field.
 * @keep_size:  %TRUE if the @field's size in pixels should stay the same.
 *              %FALSE if the @field's size in units should stay the same.
 *
 * Sets the resolution (in dpi) for field # @field of the #LigmaSizeEntry.
 *
 * The @resolution passed will be clamped to fit in
 * [#LIGMA_MIN_RESOLUTION..#LIGMA_MAX_RESOLUTION].
 *
 * This function does nothing if the #LigmaSizeEntryUpdatePolicy specified in
 * ligma_size_entry_new() doesn't equal to #LIGMA_SIZE_ENTRY_UPDATE_SIZE.
 **/
void
ligma_size_entry_set_resolution (LigmaSizeEntry *gse,
                                gint           field,
                                gdouble        resolution,
                                gboolean       keep_size)
{
  LigmaSizeEntryPrivate *priv;
  LigmaSizeEntryField   *gsef;
  gfloat                val;

  g_return_if_fail (LIGMA_IS_SIZE_ENTRY (gse));

  priv = GET_PRIVATE (gse);

  g_return_if_fail ((field >= 0) && (field < priv->number_of_fields));

  resolution = CLAMP (resolution, LIGMA_MIN_RESOLUTION, LIGMA_MAX_RESOLUTION);

  gsef = (LigmaSizeEntryField*) g_slist_nth_data (priv->fields, field);
  gsef->resolution = resolution;

  val = gsef->value;

  gsef->stop_recursion = 0;
  ligma_size_entry_set_refval_boundaries (gse, field,
                                         gsef->min_refval, gsef->max_refval);

  if (! keep_size)
    ligma_size_entry_set_value (gse, field, val);
}


/**
 * ligma_size_entry_set_size:
 * @gse:   The sizeentry you want to set a size for.
 * @field: The index of the field you want to set the size for.
 * @lower: The reference value which will be treated as 0%.
 * @upper: The reference value which will be treated as 100%.
 *
 * Sets the pixel values for field # @field of the #LigmaSizeEntry
 * which will be treated as 0% and 100%.
 *
 * These values will be used if you specified @menu_show_percent as %TRUE
 * in ligma_size_entry_new() and the user has selected LIGMA_UNIT_PERCENT in
 * the #LigmaSizeEntry's #LigmaUnitComboBox.
 *
 * This function does nothing if the #LigmaSizeEntryUpdatePolicy specified in
 * ligma_size_entry_new() doesn't equal to LIGMA_SIZE_ENTRY_UPDATE_SIZE.
 **/
void
ligma_size_entry_set_size (LigmaSizeEntry *gse,
                          gint           field,
                          gdouble        lower,
                          gdouble        upper)
{
  LigmaSizeEntryPrivate *priv;
  LigmaSizeEntryField   *gsef;

  g_return_if_fail (LIGMA_IS_SIZE_ENTRY (gse));

  priv = GET_PRIVATE (gse);

  g_return_if_fail ((field >= 0) && (field < priv->number_of_fields));
  g_return_if_fail (lower <= upper);

  gsef = (LigmaSizeEntryField*) g_slist_nth_data (priv->fields, field);
  gsef->lower = lower;
  gsef->upper = upper;

  ligma_size_entry_set_refval (gse, field, gsef->refval);
}


/**
 * ligma_size_entry_set_value_boundaries:
 * @gse:   The sizeentry you want to set value boundaries for.
 * @field: The index of the field you want to set value boundaries for.
 * @lower: The new lower boundary of the value of the chosen @field.
 * @upper: The new upper boundary of the value of the chosen @field.
 *
 * Limits the range of possible values which can be entered in field # @field
 * of the #LigmaSizeEntry.
 *
 * The current value of the @field will be clamped to fit in the @field's
 * new boundaries.
 *
 * NOTE: In most cases you won't be interested in this function because the
 *       #LigmaSizeEntry's purpose is to shield the programmer from unit
 *       calculations. Use ligma_size_entry_set_refval_boundaries() instead.
 *       Whatever you do, don't mix these calls. A size entry should either
 *       be clamped by the value or the reference value.
 **/
void
ligma_size_entry_set_value_boundaries (LigmaSizeEntry *gse,
                                      gint           field,
                                      gdouble        lower,
                                      gdouble        upper)
{
  LigmaSizeEntryPrivate *priv;
  LigmaSizeEntryField   *gsef;

  g_return_if_fail (LIGMA_IS_SIZE_ENTRY (gse));

  priv = GET_PRIVATE (gse);

  g_return_if_fail ((field >= 0) && (field < priv->number_of_fields));
  g_return_if_fail (lower <= upper);

  gsef = (LigmaSizeEntryField*) g_slist_nth_data (priv->fields, field);
  gsef->min_value        = lower;
  gsef->max_value        = upper;

  g_object_freeze_notify (G_OBJECT (gsef->value_adjustment));

  gtk_adjustment_set_lower (gsef->value_adjustment, gsef->min_value);
  gtk_adjustment_set_upper (gsef->value_adjustment, gsef->max_value);

  if (gsef->stop_recursion) /* this is a hack (but useful ;-) */
    {
      g_object_thaw_notify (G_OBJECT (gsef->value_adjustment));
      return;
    }

  gsef->stop_recursion++;
  switch (priv->update_policy)
    {
    case LIGMA_SIZE_ENTRY_UPDATE_NONE:
      break;

    case LIGMA_SIZE_ENTRY_UPDATE_SIZE:
      switch (priv->unit)
        {
        case LIGMA_UNIT_PIXEL:
          ligma_size_entry_set_refval_boundaries (gse, field,
                                                 gsef->min_value,
                                                 gsef->max_value);
          break;
        case LIGMA_UNIT_PERCENT:
          ligma_size_entry_set_refval_boundaries (gse, field,
                                                 gsef->lower +
                                                 (gsef->upper - gsef->lower) *
                                                 gsef->min_value / 100,
                                                 gsef->lower +
                                                 (gsef->upper - gsef->lower) *
                                                 gsef->max_value / 100);
          break;
        default:
          ligma_size_entry_set_refval_boundaries (gse, field,
                                                 gsef->min_value *
                                                 gsef->resolution /
                                                 ligma_unit_get_factor (priv->unit),
                                                 gsef->max_value *
                                                 gsef->resolution /
                                                 ligma_unit_get_factor (priv->unit));
          break;
        }
      break;

    case LIGMA_SIZE_ENTRY_UPDATE_RESOLUTION:
      ligma_size_entry_set_refval_boundaries (gse, field,
                                             gsef->min_value *
                                             ligma_unit_get_factor (priv->unit),
                                             gsef->max_value *
                                             ligma_unit_get_factor (priv->unit));
      break;

    default:
      break;
    }
  gsef->stop_recursion--;

  ligma_size_entry_set_value (gse, field, gsef->value);

  g_object_thaw_notify (G_OBJECT (gsef->value_adjustment));
}

/**
 * ligma_size_entry_get_value:
 * @gse:   The sizeentry you want to know a value of.
 * @field: The index of the field you want to know the value of.
 *
 * Returns the value of field # @field of the #LigmaSizeEntry.
 *
 * The @value returned is a distance or resolution
 * in the #LigmaUnit the user has selected in the #LigmaSizeEntry's
 * #LigmaUnitComboBox.
 *
 * NOTE: In most cases you won't be interested in this value because the
 *       #LigmaSizeEntry's purpose is to shield the programmer from unit
 *       calculations. Use ligma_size_entry_get_refval() instead.
 *
 * Returns: The value of the chosen @field.
 **/
gdouble
ligma_size_entry_get_value (LigmaSizeEntry *gse,
                           gint           field)
{
  LigmaSizeEntryPrivate *priv;
  LigmaSizeEntryField   *gsef;

  g_return_val_if_fail (LIGMA_IS_SIZE_ENTRY (gse), 0);

  priv = GET_PRIVATE (gse);

  g_return_val_if_fail ((field >= 0) && (field < priv->number_of_fields), 0);

  gsef = (LigmaSizeEntryField *) g_slist_nth_data (priv->fields, field);

  return gsef->value;
}

static void
ligma_size_entry_update_value (LigmaSizeEntryField *gsef,
                              gdouble             value)
{
  LigmaSizeEntryPrivate *priv = gsef->gse->priv;

  if (gsef->stop_recursion > 1)
    return;

  gsef->value = value;

  switch (priv->update_policy)
    {
    case LIGMA_SIZE_ENTRY_UPDATE_NONE:
      break;

    case LIGMA_SIZE_ENTRY_UPDATE_SIZE:
      switch (priv->unit)
        {
        case LIGMA_UNIT_PIXEL:
          gsef->refval = value;
          break;
        case LIGMA_UNIT_PERCENT:
          gsef->refval =
            CLAMP (gsef->lower + (gsef->upper - gsef->lower) * value / 100,
                   gsef->min_refval, gsef->max_refval);
          break;
        default:
          gsef->refval =
            CLAMP (value * gsef->resolution /
                   ligma_unit_get_factor (priv->unit),
                   gsef->min_refval, gsef->max_refval);
          break;
        }
      if (priv->show_refval)
        gtk_adjustment_set_value (gsef->refval_adjustment, gsef->refval);
      break;

    case LIGMA_SIZE_ENTRY_UPDATE_RESOLUTION:
      gsef->refval =
        CLAMP (value * ligma_unit_get_factor (priv->unit),
               gsef->min_refval, gsef->max_refval);
      if (priv->show_refval)
        gtk_adjustment_set_value (gsef->refval_adjustment, gsef->refval);
      break;

    default:
      break;
    }

  g_signal_emit (gsef->gse, ligma_size_entry_signals[VALUE_CHANGED], 0);
}

/**
 * ligma_size_entry_set_value:
 * @gse:   The sizeentry you want to set a value for.
 * @field: The index of the field you want to set a value for.
 * @value: The new value for @field.
 *
 * Sets the value for field # @field of the #LigmaSizeEntry.
 *
 * The @value passed is treated to be a distance or resolution
 * in the #LigmaUnit the user has selected in the #LigmaSizeEntry's
 * #LigmaUnitComboBox.
 *
 * NOTE: In most cases you won't be interested in this value because the
 *       #LigmaSizeEntry's purpose is to shield the programmer from unit
 *       calculations. Use ligma_size_entry_set_refval() instead.
 **/
void
ligma_size_entry_set_value (LigmaSizeEntry *gse,
                           gint           field,
                           gdouble        value)
{
  LigmaSizeEntryPrivate *priv;
  LigmaSizeEntryField   *gsef;

  g_return_if_fail (LIGMA_IS_SIZE_ENTRY (gse));

  priv = GET_PRIVATE (gse);

  g_return_if_fail ((field >= 0) && (field < priv->number_of_fields));

  gsef = (LigmaSizeEntryField *) g_slist_nth_data (priv->fields, field);

  value = CLAMP (value, gsef->min_value, gsef->max_value);
  gtk_adjustment_set_value (gsef->value_adjustment, value);
  ligma_size_entry_update_value (gsef, value);
}


static void
ligma_size_entry_value_callback (GtkAdjustment *adjustment,
                                gpointer       data)
{
  LigmaSizeEntryField *gsef;
  gdouble             new_value;

  gsef = (LigmaSizeEntryField *) data;

  new_value = gtk_adjustment_get_value (adjustment);

  if (gsef->value != new_value)
    ligma_size_entry_update_value (gsef, new_value);
}


/**
 * ligma_size_entry_set_refval_boundaries:
 * @gse:   The sizeentry you want to set the reference value boundaries for.
 * @field: The index of the field you want to set the reference value
 *         boundaries for.
 * @lower: The new lower boundary of the reference value of the chosen @field.
 * @upper: The new upper boundary of the reference value of the chosen @field.
 *
 * Limits the range of possible reference values which can be entered in
 * field # @field of the #LigmaSizeEntry.
 *
 * The current reference value of the @field will be clamped to fit in the
 * @field's new boundaries.
 **/
void
ligma_size_entry_set_refval_boundaries (LigmaSizeEntry *gse,
                                       gint           field,
                                       gdouble        lower,
                                       gdouble        upper)
{
  LigmaSizeEntryPrivate *priv;
  LigmaSizeEntryField   *gsef;

  g_return_if_fail (LIGMA_IS_SIZE_ENTRY (gse));

  priv = GET_PRIVATE (gse);

  g_return_if_fail ((field >= 0) && (field < priv->number_of_fields));
  g_return_if_fail (lower <= upper);

  gsef = (LigmaSizeEntryField *) g_slist_nth_data (priv->fields, field);
  gsef->min_refval = lower;
  gsef->max_refval = upper;

  if (priv->show_refval)
    {
      g_object_freeze_notify (G_OBJECT (gsef->refval_adjustment));

      gtk_adjustment_set_lower (gsef->refval_adjustment, gsef->min_refval);
      gtk_adjustment_set_upper (gsef->refval_adjustment, gsef->max_refval);
    }

  if (gsef->stop_recursion) /* this is a hack (but useful ;-) */
    {
      if (priv->show_refval)
        g_object_thaw_notify (G_OBJECT (gsef->refval_adjustment));

      return;
    }

  gsef->stop_recursion++;
  switch (priv->update_policy)
    {
    case LIGMA_SIZE_ENTRY_UPDATE_NONE:
      break;

    case LIGMA_SIZE_ENTRY_UPDATE_SIZE:
      switch (priv->unit)
        {
        case LIGMA_UNIT_PIXEL:
          ligma_size_entry_set_value_boundaries (gse, field,
                                                gsef->min_refval,
                                                gsef->max_refval);
          break;
        case LIGMA_UNIT_PERCENT:
          ligma_size_entry_set_value_boundaries (gse, field,
                                                100 * (gsef->min_refval -
                                                       gsef->lower) /
                                                (gsef->upper - gsef->lower),
                                                100 * (gsef->max_refval -
                                                       gsef->lower) /
                                                (gsef->upper - gsef->lower));
          break;
        default:
          ligma_size_entry_set_value_boundaries (gse, field,
                                                gsef->min_refval *
                                                ligma_unit_get_factor (priv->unit) /
                                                gsef->resolution,
                                                gsef->max_refval *
                                                ligma_unit_get_factor (priv->unit) /
                                                gsef->resolution);
          break;
        }
      break;

    case LIGMA_SIZE_ENTRY_UPDATE_RESOLUTION:
      ligma_size_entry_set_value_boundaries (gse, field,
                                            gsef->min_refval /
                                            ligma_unit_get_factor (priv->unit),
                                            gsef->max_refval /
                                            ligma_unit_get_factor (priv->unit));
      break;

    default:
      break;
    }
  gsef->stop_recursion--;

  ligma_size_entry_set_refval (gse, field, gsef->refval);

  if (priv->show_refval)
    g_object_thaw_notify (G_OBJECT (gsef->refval_adjustment));
}

/**
 * ligma_size_entry_set_refval_digits:
 * @gse:    The sizeentry you want to set the reference value digits for.
 * @field:  The index of the field you want to set the reference value for.
 * @digits: The new number of decimal digits for the #GtkSpinButton which
 *          displays @field's reference value.
 *
 * Sets the decimal digits of field # @field of the #LigmaSizeEntry to
 * @digits.
 *
 * If you don't specify this value explicitly, the reference value's number
 * of digits will equal to 0 for #LIGMA_SIZE_ENTRY_UPDATE_SIZE and to 2 for
 * #LIGMA_SIZE_ENTRY_UPDATE_RESOLUTION.
 **/
void
ligma_size_entry_set_refval_digits (LigmaSizeEntry *gse,
                                   gint           field,
                                   gint           digits)
{
  LigmaSizeEntryPrivate *priv;
  LigmaSizeEntryField   *gsef;

  g_return_if_fail (LIGMA_IS_SIZE_ENTRY (gse));

  priv = GET_PRIVATE (gse);

  g_return_if_fail ((field >= 0) && (field < priv->number_of_fields));
  g_return_if_fail ((digits >= 0) && (digits <= 6));

  gsef = (LigmaSizeEntryField*) g_slist_nth_data (priv->fields, field);
  gsef->refval_digits = digits;

  if (priv->update_policy == LIGMA_SIZE_ENTRY_UPDATE_SIZE)
    {
      if (priv->show_refval)
        gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->refval_spinbutton),
                                    gsef->refval_digits);
      else if (priv->unit == LIGMA_UNIT_PIXEL)
        gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                    gsef->refval_digits);
    }
}

/**
 * ligma_size_entry_get_refval:
 * @gse:   The sizeentry you want to know a reference value of.
 * @field: The index of the field you want to know the reference value of.
 *
 * Returns the reference value for field # @field of the #LigmaSizeEntry.
 *
 * The reference value is either a distance in pixels or a resolution
 * in dpi, depending on which #LigmaSizeEntryUpdatePolicy you chose in
 * ligma_size_entry_new().
 *
 * Returns: The reference value of the chosen @field.
 **/
gdouble
ligma_size_entry_get_refval (LigmaSizeEntry *gse,
                            gint           field)
{
  LigmaSizeEntryPrivate *priv;
  LigmaSizeEntryField   *gsef;

  /*  return 1.0 to avoid division by zero  */
  g_return_val_if_fail (LIGMA_IS_SIZE_ENTRY (gse), 1.0);

  priv = GET_PRIVATE (gse);

  g_return_val_if_fail ((field >= 0) && (field < priv->number_of_fields), 1.0);

  gsef = (LigmaSizeEntryField*) g_slist_nth_data (priv->fields, field);

  return gsef->refval;
}

static void
ligma_size_entry_update_refval (LigmaSizeEntryField *gsef,
                               gdouble             refval)
{
  LigmaSizeEntryPrivate *priv = GET_PRIVATE (gsef->gse);

  if (gsef->stop_recursion > 1)
    return;

  gsef->refval = refval;

  switch (priv->update_policy)
    {
    case LIGMA_SIZE_ENTRY_UPDATE_NONE:
      break;

    case LIGMA_SIZE_ENTRY_UPDATE_SIZE:
      switch (priv->unit)
        {
        case LIGMA_UNIT_PIXEL:
          gsef->value = refval;
          break;
        case LIGMA_UNIT_PERCENT:
          gsef->value =
            CLAMP (100 * (refval - gsef->lower) / (gsef->upper - gsef->lower),
                   gsef->min_value, gsef->max_value);
          break;
        default:
          gsef->value =
            CLAMP (refval * ligma_unit_get_factor (priv->unit) /
                   gsef->resolution,
                   gsef->min_value, gsef->max_value);
          break;
        }
      gtk_adjustment_set_value (gsef->value_adjustment, gsef->value);
      break;

    case LIGMA_SIZE_ENTRY_UPDATE_RESOLUTION:
      gsef->value =
        CLAMP (refval / ligma_unit_get_factor (priv->unit),
               gsef->min_value, gsef->max_value);
      gtk_adjustment_set_value (gsef->value_adjustment, gsef->value);
      break;

    default:
      break;
    }

  g_signal_emit (gsef->gse, ligma_size_entry_signals[REFVAL_CHANGED], 0);
}

/**
 * ligma_size_entry_set_refval:
 * @gse:    The sizeentry you want to set a reference value for.
 * @field:  The index of the field you want to set the reference value for.
 * @refval: The new reference value for @field.
 *
 * Sets the reference value for field # @field of the #LigmaSizeEntry.
 *
 * The @refval passed is either a distance in pixels or a resolution in dpi,
 * depending on which #LigmaSizeEntryUpdatePolicy you chose in
 * ligma_size_entry_new().
 **/
void
ligma_size_entry_set_refval (LigmaSizeEntry *gse,
                            gint           field,
                            gdouble        refval)
{
  LigmaSizeEntryPrivate *priv;
  LigmaSizeEntryField   *gsef;

  g_return_if_fail (LIGMA_IS_SIZE_ENTRY (gse));

  priv = GET_PRIVATE (gse);

  g_return_if_fail ((field >= 0) && (field < priv->number_of_fields));

  gsef = (LigmaSizeEntryField *) g_slist_nth_data (priv->fields, field);

  refval = CLAMP (refval, gsef->min_refval, gsef->max_refval);

  if (priv->show_refval)
    gtk_adjustment_set_value (gsef->refval_adjustment, refval);

  ligma_size_entry_update_refval (gsef, refval);
}

static void
ligma_size_entry_refval_callback (GtkAdjustment *adjustment,
                                 gpointer       data)
{
  LigmaSizeEntryField *gsef;
  gdouble             new_refval;

  gsef = (LigmaSizeEntryField *) data;

  new_refval = gtk_adjustment_get_value (adjustment);

  if (gsef->refval != new_refval)
    ligma_size_entry_update_refval (gsef, new_refval);
}


/**
 * ligma_size_entry_get_unit:
 * @gse: The sizeentry you want to know the unit of.
 *
 * Returns the #LigmaUnit the user has selected in the #LigmaSizeEntry's
 * #LigmaUnitComboBox.
 *
 * Returns: (transfer none): The sizeentry's unit.
 **/
LigmaUnit
ligma_size_entry_get_unit (LigmaSizeEntry *gse)
{
  g_return_val_if_fail (LIGMA_IS_SIZE_ENTRY (gse), LIGMA_UNIT_INCH);

  return GET_PRIVATE (gse)->unit;
}

static void
ligma_size_entry_update_unit (LigmaSizeEntry *gse,
                             LigmaUnit       unit)
{
  LigmaSizeEntryPrivate *priv = GET_PRIVATE (gse);
  LigmaSizeEntryField   *gsef;
  gint                  i;
  gint                  digits;

  priv->unit = unit;

  digits = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (gse),
                                               "ligma-pixel-digits"));

  for (i = 0; i < priv->number_of_fields; i++)
    {
      gsef = (LigmaSizeEntryField *) g_slist_nth_data (priv->fields, i);

      if (priv->update_policy == LIGMA_SIZE_ENTRY_UPDATE_SIZE)
        {
          if (unit == LIGMA_UNIT_PIXEL)
            gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                        gsef->refval_digits + digits);
          else if (unit == LIGMA_UNIT_PERCENT)
            gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                        2 + digits);
          else
            gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                        LIGMA_SIZE_ENTRY_DIGITS (unit) + digits);
        }
      else if (priv->update_policy == LIGMA_SIZE_ENTRY_UPDATE_RESOLUTION)
        {
          digits = (ligma_unit_get_digits (LIGMA_UNIT_INCH) -
                    ligma_unit_get_digits (unit));
          gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                      MAX (3 + digits, 3));
        }

      gsef->stop_recursion = 0; /* hack !!! */

      ligma_size_entry_set_refval_boundaries (gse, i,
                                             gsef->min_refval,
                                             gsef->max_refval);
    }

  g_signal_emit (gse, ligma_size_entry_signals[UNIT_CHANGED], 0);
}


/**
 * ligma_size_entry_set_unit:
 * @gse:  The sizeentry you want to change the unit for.
 * @unit: The new unit.
 *
 * Sets the #LigmaSizeEntry's unit. The reference value for all fields will
 * stay the same but the value in units or pixels per unit will change
 * according to which #LigmaSizeEntryUpdatePolicy you chose in
 * ligma_size_entry_new().
 **/
void
ligma_size_entry_set_unit (LigmaSizeEntry *gse,
                          LigmaUnit       unit)
{
  LigmaSizeEntryPrivate *priv;

  g_return_if_fail (LIGMA_IS_SIZE_ENTRY (gse));

  priv = GET_PRIVATE (gse);

  g_return_if_fail (priv->menu_show_pixels || (unit != LIGMA_UNIT_PIXEL));
  g_return_if_fail (priv->menu_show_percent || (unit != LIGMA_UNIT_PERCENT));

  ligma_unit_combo_box_set_active (LIGMA_UNIT_COMBO_BOX (priv->unit_combo), unit);
  ligma_size_entry_update_unit (gse, unit);
}

static void
ligma_size_entry_unit_callback (GtkWidget     *widget,
                               LigmaSizeEntry *gse)
{
  LigmaUnit new_unit;

  new_unit = ligma_unit_combo_box_get_active (LIGMA_UNIT_COMBO_BOX (widget));

  if (GET_PRIVATE (gse)->unit != new_unit)
    ligma_size_entry_update_unit (gse, new_unit);
}

/**
 * ligma_size_entry_attach_eevl:
 * @spin_button: one of the size_entry's spinbuttons.
 * @gsef:        a size entry field.
 *
 * Hooks in the LigmaEevl unit expression parser into the
 * #GtkSpinButton of the #LigmaSizeEntryField.
 **/
static void
ligma_size_entry_attach_eevl (GtkSpinButton      *spin_button,
                             LigmaSizeEntryField *gsef)
{
  gtk_spin_button_set_numeric (spin_button, FALSE);
  gtk_spin_button_set_update_policy (spin_button, GTK_UPDATE_IF_VALID);

  g_signal_connect_after (spin_button, "input",
                          G_CALLBACK (ligma_size_entry_eevl_input_callback),
                          gsef);
}

static gint
ligma_size_entry_eevl_input_callback (GtkSpinButton *spinner,
                                     gdouble       *return_val,
                                     gpointer      *data)
{
  LigmaSizeEntryField   *gsef      = (LigmaSizeEntryField *) data;
  LigmaSizeEntryPrivate *priv      = GET_PRIVATE (gsef->gse);
  LigmaEevlOptions       options   = LIGMA_EEVL_OPTIONS_INIT;
  gboolean              success   = FALSE;
  const gchar          *error_pos = NULL;
  GError               *error     = NULL;
  LigmaEevlQuantity      result;

  g_return_val_if_fail (GTK_IS_SPIN_BUTTON (spinner), FALSE);
  g_return_val_if_fail (LIGMA_IS_SIZE_ENTRY (gsef->gse), FALSE);

  options.unit_resolver_proc = ligma_size_entry_eevl_unit_resolver;
  options.data               = data;

  /* enable ratio expressions when there are two fields */
  if (priv->number_of_fields == 2)
    {
      LigmaSizeEntryField *other_gsef;
      LigmaEevlQuantity    default_unit_factor;
      gdouble             default_unit_offset;

      options.ratio_expressions = TRUE;

      if (gsef == priv->fields->data)
        {
          other_gsef = priv->fields->next->data;

          options.ratio_invert = FALSE;
        }
      else
        {
          other_gsef = priv->fields->data;

          options.ratio_invert = TRUE;
        }

      options.unit_resolver_proc (NULL,
                                  &default_unit_factor, &default_unit_offset,
                                  options.data);

      options.ratio_quantity.value     = other_gsef->value /
                                         default_unit_factor.value;
      options.ratio_quantity.dimension = default_unit_factor.dimension;
    }

  success = ligma_eevl_evaluate (gtk_entry_get_text (GTK_ENTRY (spinner)),
                                &options,
                                &result,
                                &error_pos,
                                &error);
  if (! success)
    {
      if (error && error_pos)
        {
          g_printerr ("ERROR: %s at '%s'\n",
                      error->message,
                      *error_pos ? error_pos : "<End of input>");
        }
      else
        {
          g_printerr ("ERROR: Expression evaluation failed without error.\n");
        }
      g_clear_error (&error);

      gtk_widget_error_bell (GTK_WIDGET (spinner));
      return GTK_INPUT_ERROR;
    }
  else if (result.dimension != 1 && priv->unit != LIGMA_UNIT_PERCENT)
    {
      g_printerr ("ERROR: result has wrong dimension (expected 1, got %d)\n", result.dimension);

      gtk_widget_error_bell (GTK_WIDGET (spinner));
      return GTK_INPUT_ERROR;
    }
  else if (result.dimension != 0 && priv->unit == LIGMA_UNIT_PERCENT)
    {
      g_printerr ("ERROR: result has wrong dimension (expected 0, got %d)\n", result.dimension);

      gtk_widget_error_bell (GTK_WIDGET (spinner));
      return GTK_INPUT_ERROR;
    }
  else
    {
      /* transform back to UI-unit */
      LigmaEevlQuantity  ui_unit;
      GtkAdjustment    *adj;
      gdouble           val;

      switch (priv->unit)
        {
        case LIGMA_UNIT_PIXEL:
          ui_unit.value     = gsef->resolution;
          ui_unit.dimension = 1;
          break;
        case LIGMA_UNIT_PERCENT:
          ui_unit.value     = 1.0;
          ui_unit.dimension = 0;
          break;
        default:
          ui_unit.value     = ligma_unit_get_factor(priv->unit);
          ui_unit.dimension = 1;
          break;
        }

      *return_val = result.value * ui_unit.value;

      /*  CLAMP() to adjustment bounds, or too large/small values
       *  will make the validation machinery revert to the old value.
       *  See bug #694477.
       */
      adj = gtk_spin_button_get_adjustment (spinner);

      val = CLAMP (*return_val,
                   gtk_adjustment_get_lower (adj),
                   gtk_adjustment_get_upper (adj));

      if (val != *return_val)
        {
          gtk_widget_error_bell (GTK_WIDGET (spinner));
          *return_val = val;
        }

      return TRUE;
    }
}

static gboolean
ligma_size_entry_eevl_unit_resolver (const gchar      *identifier,
                                    LigmaEevlQuantity *factor,
                                    gdouble          *offset,
                                    gpointer          data)
{
  LigmaSizeEntryField   *gsef                 = (LigmaSizeEntryField *) data;
  LigmaSizeEntryPrivate *priv                 = GET_PRIVATE (gsef->gse);
  gboolean              resolve_default_unit = (identifier == NULL);
  LigmaUnit              unit;

  g_return_val_if_fail (gsef, FALSE);
  g_return_val_if_fail (factor != NULL, FALSE);
  g_return_val_if_fail (offset != NULL, FALSE);
  g_return_val_if_fail (LIGMA_IS_SIZE_ENTRY (gsef->gse), FALSE);

  *offset = 0.0;

  for (unit = 0;
       unit <= ligma_unit_get_number_of_units ();
       unit++)
    {
      /* Hack to handle percent within the loop */
      if (unit == ligma_unit_get_number_of_units ())
        unit = LIGMA_UNIT_PERCENT;

      if ((resolve_default_unit && unit == priv->unit) ||
          (identifier &&
           (strcmp (ligma_unit_get_symbol (unit),       identifier) == 0 ||
            strcmp (ligma_unit_get_abbreviation (unit), identifier) == 0)))
        {
          switch (unit)
            {
            case LIGMA_UNIT_PERCENT:
              if (priv->unit == LIGMA_UNIT_PERCENT)
                {
                  factor->value = 1;
                  factor->dimension = 0;
                }
              else
                {
                  /* gsef->upper contains the '100%'-value */
                  factor->value = 100*gsef->resolution/(gsef->upper - gsef->lower);
                  /* gsef->lower contains the '0%'-value */
                  *offset       = gsef->lower/gsef->resolution;
                  factor->dimension = 1;
                }
              /* return here, don't perform percentage conversion */
              return TRUE;
            case LIGMA_UNIT_PIXEL:
              factor->value     = gsef->resolution;
              break;
            default:
              factor->value     = ligma_unit_get_factor (unit);
              break;
            }

          if (priv->unit == LIGMA_UNIT_PERCENT)
            {
              /* map non-percentages onto percent */
              factor->value = gsef->upper/(100*gsef->resolution);
              factor->dimension = 0;
            }
          else
            {
              factor->dimension = 1;
            }

          /* We are done */
          return TRUE;
        }
    }

  return FALSE;
}

/**
 * ligma_size_entry_show_unit_menu:
 * @gse: a #LigmaSizeEntry
 * @show: Boolean
 *
 * Controls whether a unit menu is shown in the size entry.  If
 * @show is %TRUE, the menu is shown; otherwise it is hidden.
 *
 * Since: 2.4
 **/
void
ligma_size_entry_show_unit_menu (LigmaSizeEntry *gse,
                                gboolean       show)
{
  g_return_if_fail (LIGMA_IS_SIZE_ENTRY (gse));

  gtk_widget_set_visible (GET_PRIVATE (gse)->unit_combo, show);
}


/**
 * ligma_size_entry_set_pixel_digits:
 * @gse: a #LigmaSizeEntry
 * @digits: the number of digits to display for a pixel size
 *
 * This function allows you set up a #LigmaSizeEntry so that sub-pixel
 * sizes can be entered.
 **/
void
ligma_size_entry_set_pixel_digits (LigmaSizeEntry *gse,
                                  gint           digits)
{
  LigmaUnitComboBox *combo;

  g_return_if_fail (LIGMA_IS_SIZE_ENTRY (gse));

  combo = LIGMA_UNIT_COMBO_BOX (GET_PRIVATE (gse)->unit_combo);

  g_object_set_data (G_OBJECT (gse), "ligma-pixel-digits",
                     GINT_TO_POINTER (digits));
  ligma_size_entry_update_unit (gse, ligma_unit_combo_box_get_active (combo));
}


/**
 * ligma_size_entry_grab_focus:
 * @gse: The sizeentry you want to grab the keyboard focus.
 *
 * This function is rather ugly and just a workaround for the fact that
 * it's impossible to implement gtk_widget_grab_focus() for a #GtkGrid (is this actually true after the Table->Grid conversion?).
 **/
void
ligma_size_entry_grab_focus (LigmaSizeEntry *gse)
{
  LigmaSizeEntryPrivate *priv;
  LigmaSizeEntryField   *gsef;

  g_return_if_fail (LIGMA_IS_SIZE_ENTRY (gse));

  priv = GET_PRIVATE (gse);

  gsef = priv->fields->data;
  if (gsef)
    gtk_widget_grab_focus (priv->show_refval ?
                           gsef->refval_spinbutton : gsef->value_spinbutton);
}

/**
 * ligma_size_entry_set_activates_default:
 * @gse:     A #LigmaSizeEntry
 * @setting: %TRUE to activate window's default widget on Enter keypress
 *
 * Iterates over all entries in the #LigmaSizeEntry and calls
 * gtk_entry_set_activates_default() on them.
 *
 * Since: 2.4
 **/
void
ligma_size_entry_set_activates_default (LigmaSizeEntry *gse,
                                       gboolean       setting)
{
  LigmaSizeEntryPrivate *priv;
  GSList               *list;

  g_return_if_fail (LIGMA_IS_SIZE_ENTRY (gse));

  priv = GET_PRIVATE (gse);

  for (list = priv->fields; list; list = g_slist_next (list))
    {
      LigmaSizeEntryField *gsef = list->data;

      if (gsef->value_spinbutton)
        gtk_entry_set_activates_default (GTK_ENTRY (gsef->value_spinbutton),
                                         setting);

      if (gsef->refval_spinbutton)
        gtk_entry_set_activates_default (GTK_ENTRY (gsef->refval_spinbutton),
                                         setting);
    }
}

/**
 * ligma_size_entry_get_help_widget:
 * @gse: a #LigmaSizeEntry
 * @field: the index of the widget you want to get a pointer to
 *
 * You shouldn't fiddle with the internals of a #LigmaSizeEntry but
 * if you want to set tooltips using ligma_help_set_help_data() you
 * can use this function to get a pointer to the spinbuttons.
 *
 * Returns: (transfer none): a #GtkWidget pointer that you can attach a tooltip to.
 **/
GtkWidget *
ligma_size_entry_get_help_widget (LigmaSizeEntry *gse,
                                 gint           field)
{
  LigmaSizeEntryPrivate *priv;
  LigmaSizeEntryField   *gsef;

  g_return_val_if_fail (LIGMA_IS_SIZE_ENTRY (gse), NULL);

  priv = GET_PRIVATE (gse);

  g_return_val_if_fail ((field >= 0) && (field < priv->number_of_fields), NULL);

  gsef = g_slist_nth_data (priv->fields, field);
  if (!gsef)
    return NULL;

  return (gsef->refval_spinbutton ?
          gsef->refval_spinbutton : gsef->value_spinbutton);
}
