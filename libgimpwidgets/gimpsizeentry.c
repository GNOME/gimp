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

#include "gimplimits.h"
#include "gimpsizeentry.h"
#include "gimpunitmenu.h"

#define SIZE_MAX_VALUE 500000.0

static void gimp_size_entry_unit_callback      (GtkWidget *widget,
						gpointer   data);
static void gimp_size_entry_value_callback     (GtkWidget *widget,
						gpointer   data);
static void gimp_size_entry_refval_callback    (GtkWidget *widget,
						gpointer   data);

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

static guint gimp_size_entry_signals[LAST_SIGNAL] = { 0 };

static GtkTableClass *parent_class = NULL;

static void
gimp_size_entry_destroy (GtkObject *object)
{
  GimpSizeEntry *gse;
  GSList        *list;        

  g_return_if_fail (object != NULL);
  g_return_if_fail (GIMP_IS_SIZE_ENTRY (object));

  gse = GIMP_SIZE_ENTRY (object);

  for (list = gse->fields; list; list = g_slist_next(list))
    g_free (list->data);

  g_slist_free (gse->fields);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gimp_size_entry_class_init (GimpSizeEntryClass *class)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) class;

  parent_class = gtk_type_class (gtk_table_get_type ());

  gimp_size_entry_signals[VALUE_CHANGED] = 
              gtk_signal_new ("value_changed",
			      GTK_RUN_FIRST,
			      object_class->type,
			      GTK_SIGNAL_OFFSET (GimpSizeEntryClass,
						 value_changed),
			      gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

  gimp_size_entry_signals[REFVAL_CHANGED] = 
              gtk_signal_new ("refval_changed",
			      GTK_RUN_FIRST,
			      object_class->type,
			      GTK_SIGNAL_OFFSET (GimpSizeEntryClass,
						 refval_changed),
			      gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

  gimp_size_entry_signals[UNIT_CHANGED] = 
              gtk_signal_new ("unit_changed",
			      GTK_RUN_FIRST,
			      object_class->type,
			      GTK_SIGNAL_OFFSET (GimpSizeEntryClass,
						 unit_changed),
			      gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, gimp_size_entry_signals, 
				LAST_SIGNAL);

  class->value_changed = NULL;
  class->refval_changed = NULL;
  class->unit_changed = NULL;

  object_class->destroy = gimp_size_entry_destroy;
}

static void
gimp_size_entry_init (GimpSizeEntry *gse)
{
  gse->fields = NULL;
  gse->number_of_fields = 0;
  gse->unitmenu = NULL;
  gse->unit = GIMP_UNIT_PIXEL;
  gse->menu_show_pixels = TRUE;
  gse->menu_show_percent = TRUE;
  gse->show_refval = FALSE;
  gse->update_policy = GIMP_SIZE_ENTRY_UPDATE_NONE;
}

GtkType
gimp_size_entry_get_type (void)
{
  static guint gse_type = 0;

  if (!gse_type)
    {
      GtkTypeInfo gse_info =
      {
	"GimpSizeEntry",
	sizeof (GimpSizeEntry),
	sizeof (GimpSizeEntryClass),
	(GtkClassInitFunc) gimp_size_entry_class_init,
	(GtkObjectInitFunc) gimp_size_entry_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      gse_type = gtk_type_unique (gtk_table_get_type (), &gse_info);
    }
  
  return gse_type;
}

/**
 * gimp_size_entry_new:
 * @number_of_fields: The number of input fields.
 * @unit: The initial unit.
 * @unit_format: A printf-like unit-format string (see #GimpUnitMenu).
 * @menu_show_pixels: #TRUE if the unit menu shold contain an item for
 *                    GIMP_UNIT_PIXEL (ignored if the @update_policy is not
 *                    GIMP_SIZE_ENTRY_UPDATE_NONE).
 * @menu_show_percent: #TRUE if the unit menu shold contain an item for
 *                     GIMP_UNIT_PERCENT.
 * @show_refval: #TRUE if you want an extra "refenence value" spinbutton per
                 input field.
 * @spinbutton_usize: The minimal horizontal size of the #GtkSpinButton's.
 * @update_policy: How the automatic pixel <-> real-world-unit calculations
 *                 should be performed.
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
 * of the #GimpUnitMenu to allow the caller to add labels or a #GimpChainButton.
 *
 * Returns: A Pointer to the new #GimpSizeEntry widget.
 *
 */
GtkWidget *
gimp_size_entry_new (gint                       number_of_fields,
		     GimpUnit                   unit,
		     gchar                     *unit_format,
		     gboolean                   menu_show_pixels,
		     gboolean                   menu_show_percent,
		     gboolean                   show_refval,
		     gint                       spinbutton_usize,
		     GimpSizeEntryUpdatePolicy  update_policy)
{
  GimpSizeEntry *gse;
  gint           i;

  g_return_val_if_fail ((number_of_fields >= 0) && (number_of_fields <= 16),
			NULL);

  gse = gtk_type_new (gimp_size_entry_get_type ());

  gse->number_of_fields = number_of_fields;
  gse->unit = unit;
  gse->show_refval = show_refval;
  gse->update_policy = update_policy;

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
      GimpSizeEntryField *gsef;

      gsef = g_malloc (sizeof (GimpSizeEntryField));
      gse->fields = g_slist_append (gse->fields, gsef);

      gsef->gse = gse;
      gsef->resolution = 1.0; /*  just to avoid division by zero  */
      gsef->lower = 0.0;
      gsef->upper = 100.0;
      gsef->value = 0;
      gsef->min_value = 0;
      gsef->max_value = SIZE_MAX_VALUE;
      gsef->refval_adjustment = NULL;
      gsef->value_adjustment = NULL;
      gsef->refval = 0;
      gsef->min_refval = 0;
      gsef->max_refval = SIZE_MAX_VALUE;
      gsef->refval_digits =
	(update_policy == GIMP_SIZE_ENTRY_UPDATE_SIZE) ? 0 : 3;
      gsef->stop_recursion = 0;

      gsef->value_adjustment = gtk_adjustment_new (gsef->value,
						   gsef->min_value,
						   gsef->max_value,
						   1.0, 10.0, 0.0);
      gsef->value_spinbutton =
	gtk_spin_button_new (GTK_ADJUSTMENT (gsef->value_adjustment), 1.0,
			     (unit == GIMP_UNIT_PERCENT) ? 2 :
			     (MIN (gimp_unit_get_digits (unit), 5) + 1));
      gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(gsef->value_spinbutton),
				       GTK_SHADOW_NONE);
      gtk_widget_set_usize (gsef->value_spinbutton, spinbutton_usize, 0);
      gtk_table_attach_defaults (GTK_TABLE (gse), gsef->value_spinbutton,
				 i+1, i+2,
				 gse->show_refval+1, gse->show_refval+2);
      gtk_signal_connect (GTK_OBJECT (gsef->value_adjustment), "value_changed",
			  (GtkSignalFunc) gimp_size_entry_value_callback, gsef);

      gtk_widget_show (gsef->value_spinbutton);

      if (gse->show_refval)
	{
	  gsef->refval_adjustment = gtk_adjustment_new (gsef->refval,
							gsef->min_refval,
							gsef->max_refval,
							1.0, 10.0, 0.0);
	  gsef->refval_spinbutton =
	    gtk_spin_button_new (GTK_ADJUSTMENT (gsef->refval_adjustment),
				 1.0,
				 gsef->refval_digits);
	  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (gsef->refval_spinbutton),
					   GTK_SHADOW_NONE);
	  gtk_widget_set_usize (gsef->refval_spinbutton, spinbutton_usize, 0);
	  gtk_table_attach_defaults (GTK_TABLE (gse), gsef->refval_spinbutton,
				     i+1, i+2, 1, 2);
	  gtk_signal_connect (GTK_OBJECT (gsef->refval_adjustment),
			      "value_changed",
			      (GtkSignalFunc) gimp_size_entry_refval_callback,
			      gsef);

	  gtk_widget_show (gsef->refval_spinbutton);
	}			

      if (gse->menu_show_pixels && !gse->show_refval && (unit == GIMP_UNIT_PIXEL))
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
  gtk_signal_connect (GTK_OBJECT (gse->unitmenu), "unit_changed",
		      (GtkSignalFunc) gimp_size_entry_unit_callback, gse);
  gtk_widget_show (gse->unitmenu);
  
  return GTK_WIDGET (gse);
}


/**
 * gimp_size_entry_add_field:
 * @gse: The sizeentry you want to add a field to.
 * @value_spinbutton: The spinbutton to display the field's value.
 * @refval_spinbutton: The spinbutton to display the field's reference value.
 *
 * Adds an input field to the #GimpSizeEntry.
 *
 * The new input field will have the index 0. If you specified @show_refval
 * as #TRUE in gimp_size_entry_new() you have to pass an additional
 * #GtkSpinButton to hold the reference value. If @show_refval was #FALSE,
 * @refval_spinbutton will be ignored.
 *
 */
void
gimp_size_entry_add_field  (GimpSizeEntry *gse,
			    GtkSpinButton *value_spinbutton,
			    GtkSpinButton *refval_spinbutton)
{
  GimpSizeEntryField *gsef;

  g_return_if_fail (gse != NULL);
  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail (value_spinbutton != NULL);
  g_return_if_fail (GTK_IS_SPIN_BUTTON (value_spinbutton));
  if (gse->show_refval)
    {
      g_return_if_fail (refval_spinbutton != NULL);
      g_return_if_fail (GTK_IS_SPIN_BUTTON (refval_spinbutton));
    }

  gsef = g_malloc (sizeof (GimpSizeEntryField));
  gse->fields = g_slist_prepend (gse->fields, gsef);
  gse->number_of_fields++;

  gsef->gse = gse;
  gsef->resolution = 1.0; /*  just to avoid division by zero  */
  gsef->lower = 0.0;
  gsef->upper = 100.0;
  gsef->value = 0;
  gsef->min_value = 0;
  gsef->max_value = SIZE_MAX_VALUE;
  gsef->refval = 0;
  gsef->min_refval = 0;
  gsef->max_refval = SIZE_MAX_VALUE;
  gsef->refval_digits =
    (gse->update_policy == GIMP_SIZE_ENTRY_UPDATE_SIZE) ? 0 : 3;
  gsef->stop_recursion = 0;

  gsef->value_adjustment =
    GTK_OBJECT (gtk_spin_button_get_adjustment (value_spinbutton));
  gsef->value_spinbutton = GTK_WIDGET (value_spinbutton);
  gtk_signal_connect (GTK_OBJECT (gsef->value_adjustment), "value_changed",
		      (GtkSignalFunc) gimp_size_entry_value_callback, gsef);

  if (gse->show_refval)
    {
      gsef->refval_adjustment =
	GTK_OBJECT (gtk_spin_button_get_adjustment (refval_spinbutton));
      gsef->refval_spinbutton = GTK_WIDGET (refval_spinbutton);
      gtk_signal_connect (GTK_OBJECT (gsef->refval_adjustment), "value_changed",
			  (GtkSignalFunc) gimp_size_entry_refval_callback,
			  gsef);
    }

  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (value_spinbutton),
			      MIN (gimp_unit_get_digits (gse->unit), 5) + 1);

  if (gse->menu_show_pixels && !gse->show_refval && (gse->unit == GIMP_UNIT_PIXEL))
    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
				gsef->refval_digits);
}

/**
 * gimp_size_entry_attach_label:
 * @gse: The sizeentry you want to add a label to.
 * @text: The text of the label.
 * @row: The row where the label will be attached.
 * @column: The column where the label will be attached.
 * @alignment: The horizontal alignment of the label.
 *
 * Attaches a #GtkLabel to the #GimpSizeEntry (which is a #GtkTable).
 */
void
gimp_size_entry_attach_label (GimpSizeEntry *gse,
			      gchar         *text,
			      gint           row,
			      gint           column,
			      gfloat         alignment)
{
  GtkWidget* label;

  g_return_if_fail (gse != NULL);
  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));

  label = gtk_label_new (text);
  gtk_misc_set_alignment (GTK_MISC (label), alignment, 0.5);

  gtk_table_attach (GTK_TABLE (gse), label, column, column+1, row, row+1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);
}


/**
 * gimp_size_entry_set_resolution:
 * @gse: The sizeentry you want to set a resolution for.
 * @field: The index of the field you want to set the resolution for.
 * @resolution: The new resolution (in dpi) for the chosen @field.
 * @keep_size: #TRUE if the @field's size in pixels should stay the same.
 *             #FALSE if the @field's size in units should stay the same.
 *
 * Sets the resolution (in dpi) for field # @field of the #GimpSizeEntry.
 *
 * The @resolution passed will be clamped to fit in
 * [#GIMP_MIN_RESOLUTION..#GIMP_MAX_RESOLUTION].
 *
 * This function does nothing if the #GimpSizeEntryUpdatePolicy specified in
 * gimp_size_entry_new() doesn't equal to GIMP_SIZE_ENTRY_UPDATE_SIZE.
 *
 */
void
gimp_size_entry_set_resolution (GimpSizeEntry *gse,
				gint           field,
				gdouble        resolution,
				gboolean       keep_size)
{
  GimpSizeEntryField *gsef;
  gfloat              val;

  g_return_if_fail (gse != NULL);
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
 * @gse: The sizeentry you want to set a size for.
 * @field: The index of the field you want to set the size for.
 * @lower: The reference value which will be treated as 0%.
 * @upper: The reference value which will be treated as 100%.
 *
 * Sets the pixel values for field # @field of the #GimpSizeEntry
 * which will be treated as 0% and 100%.
 *
 * These values will be used if you specified @menu_show_percent as #TRUE
 * in gimp_size_entry_new() and the user has selected GIMP_UNIT_PERCENT in
 * the #GimpSizeEntry's #GimpUnitMenu.
 *
 * This function does nothing if the #GimpSizeEntryUpdatePolicy specified in
 * gimp_size_entry_new() doesn't equal to GIMP_SIZE_ENTRY_UPDATE_SIZE.
 *
 */
void
gimp_size_entry_set_size (GimpSizeEntry *gse,
			  gint           field,
			  gdouble        lower,
			  gdouble        upper)
{
  GimpSizeEntryField *gsef;

  g_return_if_fail (gse != NULL);
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
 * @gse: The sizeentry you want to set value boundaries for.
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
 * NOTE: In most cases you won't be interested in these values because the
 *       #GimpSizeEntry's purpose is to shield the programmer from unit
 *       calculations. Use gimp_size_entry_set_refval_boundaries() instead.
 *
 */
void
gimp_size_entry_set_value_boundaries  (GimpSizeEntry *gse,
				       gint           field,
				       gdouble        lower,
				       gdouble        upper)
{
  GimpSizeEntryField *gsef;

  g_return_if_fail (gse != NULL);
  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));
  g_return_if_fail (lower <= upper);

  gsef = (GimpSizeEntryField*) g_slist_nth_data (gse->fields, field);
  gsef->min_value = lower;
  gsef->max_value = upper;

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
 * @gse: The sizeentry you want to know a value of.
 * @field: The index of the filed you want to know the value of.
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
 *
 */
gdouble
gimp_size_entry_get_value (GimpSizeEntry *gse,
			   gint           field)
{
  GimpSizeEntryField *gsef;

  g_return_val_if_fail (gse != NULL, 0);
  g_return_val_if_fail (GIMP_IS_SIZE_ENTRY (gse), 0);
  g_return_val_if_fail ((field >= 0) && (field < gse->number_of_fields), 0);

  gsef = (GimpSizeEntryField*) g_slist_nth_data (gse->fields, field);
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
}

/**
 * gimp_size_entry_set_value;
 * @gse: The sizeentry you want to set a value for.
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
 *
 */
void
gimp_size_entry_set_value (GimpSizeEntry *gse,
			   gint           field,
			   gdouble        value)
{
  GimpSizeEntryField *gsef;

  g_return_if_fail (gse != NULL);
  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));

  gsef = (GimpSizeEntryField*) g_slist_nth_data (gse->fields, field);

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

  gsef = (GimpSizeEntryField*) data;

  new_value = GTK_ADJUSTMENT (widget)->value;

  if (gsef->value != new_value)
    {
      gimp_size_entry_update_value (gsef, new_value);
      gtk_signal_emit (GTK_OBJECT (gsef->gse),
		       gimp_size_entry_signals[VALUE_CHANGED]);
    }
}


/**
 * gimp_size_entry_set_refval_boundaries:
 * @gse: The sizeentry you want to set the reference value boundaries for.
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
 *
 */
void
gimp_size_entry_set_refval_boundaries  (GimpSizeEntry *gse,
					gint           field,
					gdouble        lower,
					gdouble        upper)
{
  GimpSizeEntryField *gsef;

  g_return_if_fail (gse != NULL);
  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));
  g_return_if_fail (lower <= upper);

  gsef = (GimpSizeEntryField*) g_slist_nth_data (gse->fields, field);
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
						gimp_unit_get_factor(gse->unit) /
						gsef->resolution,
						gsef->max_refval *
						gimp_unit_get_factor(gse->unit) /
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
 * @gse: The sizeentry you want to set the reference value digits for.
 * @field: The index of the field you want to set the reference value for.
 * @digits: The new number of decimal digits for the #GtkSpinButton which
 *          displays @field's reference value.
 *
 * Sets the decimal digits of field # @field of the #GimpSizeEntry to
 * @digits.
 *
 * If you don't specify this value explicitly, the reference value's number
 * of digits will equal to 0 for GIMP_SIZE_ENTRY_UPDATE_SIZE and to 2 for
 * GIMP_SIZE_ENTRY_UPDATE_RESOLUTION.
 *
 */
void
gimp_size_entry_set_refval_digits (GimpSizeEntry *gse,
				   gint           field,
				   gint           digits)
{
  GimpSizeEntryField *gsef;

  g_return_if_fail (gse != NULL);
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
 * @gse: The sizeentry you want to know a reference value of.
 * @field: The index of the field you want to know the reference value of.
 *
 * Returns the reference value for field # @field of the #GimpSizeEntry.
 *
 * The reference value is either a distance in pixels or a resolution
 * in dpi, depending on which #GimpSizeEntryUpdatePolicy you chose in
 * gimp_size_entry_new().
 *
 * Returns: The reference value of the chosen @field.
 *
 */
gdouble
gimp_size_entry_get_refval (GimpSizeEntry *gse,
			    gint           field)
{
  GimpSizeEntryField *gsef;

  /*  return 1.0 to avoid division by zero  */
  g_return_val_if_fail (gse != NULL, 1.0);
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
}

/**
 * gimp_size_entry_set_refval;
 * @gse: The sizeentry you want to set a reference value for.
 * @field: The index of the field you want to set the reference value for.
 * @refval: The new reference value for @field.
 *
 * Sets the reference value for field # @field of the #GimpSizeEntry.
 *
 * The @refval passed is either a distance in pixels or a resolution in dpi,
 * depending on which #GimpSizeEntryUpdatePolicy you chose in
 * gimp_size_entry_new().
 *
 */
void
gimp_size_entry_set_refval (GimpSizeEntry *gse,
			    gint           field,
			    gdouble        refval)
{
  GimpSizeEntryField *gsef;

  g_return_if_fail (gse != NULL);
  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));

  gsef = (GimpSizeEntryField*) g_slist_nth_data (gse->fields, field);

  refval = CLAMP (refval, gsef->min_refval, gsef->max_refval);

  if (gse->show_refval)
    {
      gtk_adjustment_set_value (GTK_ADJUSTMENT (gsef->refval_adjustment),
				refval);
    }

  gimp_size_entry_update_refval (gsef, refval);
}

static void
gimp_size_entry_refval_callback (GtkWidget *widget,
				 gpointer   data)
{
  GimpSizeEntryField *gsef;
  gdouble             new_refval;

  gsef = (GimpSizeEntryField*) data;

  new_refval = GTK_ADJUSTMENT (widget)->value;

  if (gsef->refval != new_refval)
    {
      gimp_size_entry_update_refval (gsef, new_refval);
      gtk_signal_emit (GTK_OBJECT (gsef->gse),
		       gimp_size_entry_signals[REFVAL_CHANGED]);
    }
}


/**
 * gimp_size_entry_get_unit:
 * @gse: The sizeentry you want to know the unit of.
 *
 * Returns the #GimpUnit the user has selected in the #GimpSizeEntry's
 * #GimpUnitMenu. 
 *
 * Returns: The sizeentry's unit.
 *
 */
GimpUnit
gimp_size_entry_get_unit (GimpSizeEntry *gse)
{
  g_return_val_if_fail (gse != NULL, GIMP_UNIT_INCH);
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

  for (i = 0; i < gse->number_of_fields; i++)
    {
      gsef = (GimpSizeEntryField*) g_slist_nth_data (gse->fields, i);

      if (gse->update_policy == GIMP_SIZE_ENTRY_UPDATE_SIZE)
	{
	  if (unit == GIMP_UNIT_PIXEL)
	    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
					gsef->refval_digits);
	  else if (unit == GIMP_UNIT_PERCENT)
	    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
					2);
	  else
	    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
					MIN(gimp_unit_get_digits (unit), 5) + 1);
	}
      else if (gse->update_policy == GIMP_SIZE_ENTRY_UPDATE_RESOLUTION)
	{
	  digits =
	    -(gimp_unit_get_digits (unit) - gimp_unit_get_digits (GIMP_UNIT_INCH));
	  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
				      MAX(3 + digits, 3));
	}

      gsef->stop_recursion = 0; /* hack !!! */

      gtk_signal_handler_block_by_data (GTK_OBJECT (gsef->value_adjustment),
					gsef);

      gimp_size_entry_set_refval_boundaries (gse, i,
					     gsef->min_refval, gsef->max_refval);

      gtk_signal_handler_unblock_by_data (GTK_OBJECT (gsef->value_adjustment),
					  gsef);
    }

  gtk_signal_emit (GTK_OBJECT (gse),
		   gimp_size_entry_signals[VALUE_CHANGED]);
}

/**
 * gimp_size_entry_set_unit:
 * @gse: The sizeentry you want to change the unit for.
 * @unit: The new unit.
 *
 * Sets the #GimpSizeEntry's unit. The reference value for all fields will
 * stay the same but the value in units or pixels per unit will change
 * according to which #GimpSizeEntryUpdatePolicy you chose in
 * gimp_size_entry_new().
 *
 */
void
gimp_size_entry_set_unit (GimpSizeEntry *gse, 
			  GimpUnit       unit)
{
  g_return_if_fail (gse != NULL);
  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail (gse->menu_show_pixels || (unit != GIMP_UNIT_PIXEL));
  g_return_if_fail (gse->menu_show_percent || (unit != GIMP_UNIT_PERCENT));

  gimp_unit_menu_set_unit (GIMP_UNIT_MENU (gse->unitmenu), unit);
  gimp_size_entry_update_unit (gse, unit);
}

static void
gimp_size_entry_unit_callback (GtkWidget *widget,
			       gpointer   data)
{
  gimp_size_entry_update_unit (GIMP_SIZE_ENTRY (data),
			       gimp_unit_menu_get_unit (GIMP_UNIT_MENU(widget)));
  gtk_signal_emit (GTK_OBJECT (data),
		   gimp_size_entry_signals[UNIT_CHANGED]);
}

/**
 * gimp_size_entry_grab_focus:
 * @gse: The sizeentry you want to grab the keyboard focus.
 *
 * This function is rather ugly and just a workaround for the fact that
 * it's impossible to implement gtk_widget_grab_focus() for a #GtkTable.
 *
 */
void
gimp_size_entry_grab_focus (GimpSizeEntry *gse)
{
  GimpSizeEntryField *gsef;

  g_return_if_fail (gse != NULL);
  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));

  gsef = (GimpSizeEntryField*) gse->fields->data;

  gtk_widget_grab_focus (gse->show_refval ?
			 gsef->refval_spinbutton : gsef->value_spinbutton);
}
