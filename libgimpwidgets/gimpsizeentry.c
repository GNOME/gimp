/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "gimpsizeentry.h"
#include "gimpunitmenu.h"

#define SIZE_MAX_VALUE 500000.0     /* is that enough ?? */

static void gimp_size_entry_unit_callback             (GtkWidget *widget,
						       gpointer   data);

static void gimp_size_entry_value_callback            (GtkWidget *widget,
						       gpointer   data);
static void gimp_size_entry_value_focus_out_callback  (GtkWidget *widget,
						       GdkEvent  *event,
						       gpointer   data);

static void gimp_size_entry_refval_callback           (GtkWidget *widget,
						       gpointer   data);
static void gimp_size_entry_refval_focus_out_callback (GtkWidget *widget,
						       GdkEvent  *event,
						       gpointer   data);

enum {
  GSE_VALUE_CHANGED_SIGNAL,
  GSE_REFVAL_CHANGED_SIGNAL,
  GSE_UNIT_CHANGED_SIGNAL,
  LAST_SIGNAL
};

struct _GimpSizeEntryField
{
  GimpSizeEntry *gse;
  gint           index;

  gfloat         resolution;

  GtkWidget     *value_spinbutton;
  gfloat         value;
  gfloat         min_value;
  gfloat         max_value;

  GtkWidget     *refval_spinbutton;
  gfloat         refval;
  gfloat         min_refval;
  gfloat         max_refval;
  gint           refval_digits;
};


static gint gimp_size_entry_signals[LAST_SIGNAL] = { 0 };

static GtkTableClass *parent_class = NULL;

/*  this is a hack to avoid infinite recursion when calling the
 *  boundary functions
 */
static gint gimp_size_entry_stop_recursion = 0;

static void
gimp_size_entry_class_destroy (GtkObject *object)
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

  gimp_size_entry_signals[GSE_VALUE_CHANGED_SIGNAL] = 
              gtk_signal_new ("value_changed",
			      GTK_RUN_FIRST,
			      object_class->type,
			      GTK_SIGNAL_OFFSET (GimpSizeEntryClass,
						 gimp_size_entry),
			      gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
  gimp_size_entry_signals[GSE_REFVAL_CHANGED_SIGNAL] = 
              gtk_signal_new ("refval_changed",
			      GTK_RUN_FIRST,
			      object_class->type,
			      GTK_SIGNAL_OFFSET (GimpSizeEntryClass,
						 gimp_size_entry),
			      gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
  gimp_size_entry_signals[GSE_UNIT_CHANGED_SIGNAL] = 
              gtk_signal_new ("unit_changed",
			      GTK_RUN_FIRST,
			      object_class->type,
			      GTK_SIGNAL_OFFSET (GimpSizeEntryClass,
						 gimp_size_entry),
			      gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
  gtk_object_class_add_signals (object_class, gimp_size_entry_signals, 
				LAST_SIGNAL);

  object_class->destroy = gimp_size_entry_class_destroy;
  class->gimp_size_entry = NULL;
}

static void
gimp_size_entry_init (GimpSizeEntry *gse)
{
  gse->fields = NULL;
  gse->number_of_fields = 0;
  gse->unitmenu = NULL;
  gse->unit = UNIT_PIXEL;
  gse->menu_show_pixels = TRUE;
  gse->show_refval = FALSE;
  gse->update_policy = GIMP_SIZE_ENTRY_UPDATE_NONE;
}

guint
gimp_size_entry_get_type ()
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


GtkWidget*
gimp_size_entry_new (gint             number_of_fields,
		     GUnit            unit,
		     gchar           *unit_format,
		     guint            menu_show_pixels,
		     guint            show_refval,
		     guint            spinbutton_usize,
		     GimpSizeEntryUP  update_policy)
{
  GimpSizeEntry *gse;
  GtkAdjustment *adjustment;
  gint           i;

  g_return_val_if_fail ((number_of_fields > 0) && (number_of_fields <= 16),
			NULL);

  gse = gtk_type_new (gimp_size_entry_get_type ());

  gse->number_of_fields = number_of_fields;
  gse->unit = unit;
  gse->show_refval = show_refval ? 1 : 0;
  gse->update_policy = update_policy;

  gtk_table_resize (GTK_TABLE (gse),
		    1 + gse->show_refval + 2,
		    number_of_fields + 1 + 3);

  /*  show the 'pixels' menu entry only if we are a 'size' sizeentry and
   *  don't have the reference value spinbutton
   */
  if ((update_policy == GIMP_SIZE_ENTRY_UPDATE_SIZE) && menu_show_pixels)
    gse->menu_show_pixels = TRUE;
  else if (update_policy == GIMP_SIZE_ENTRY_UPDATE_RESOLUTION)
    gse->menu_show_pixels = FALSE;
  else
    gse->menu_show_pixels = menu_show_pixels;

  for (i = 0; i < number_of_fields; i++)
    {
      GimpSizeEntryField *gsef;

      gsef = g_malloc (sizeof (GimpSizeEntryField));
      gse->fields = g_slist_append (gse->fields, gsef);

      gsef->gse = gse;
      gsef->index = i;
      gsef->resolution = 1.0; /*  just to avoid division by zero  */
      gsef->value = 0;
      gsef->min_value = 0;
      gsef->max_value = SIZE_MAX_VALUE;
      gsef->refval = 0;
      gsef->min_refval = 0;
      gsef->max_refval = SIZE_MAX_VALUE;
      gsef->refval_digits =
	(update_policy == GIMP_SIZE_ENTRY_UPDATE_SIZE) ? 0 : 3;

      adjustment = 
	GTK_ADJUSTMENT (gtk_adjustment_new (gsef->value,
					    gsef->min_value, 
					    gsef->max_value,
					    1.0, 10.0, 0.0));
      gsef->value_spinbutton = gtk_spin_button_new (adjustment, 1.0, 3);
      gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(gsef->value_spinbutton),
				       GTK_SHADOW_NONE);
      gtk_widget_set_usize (gsef->value_spinbutton, spinbutton_usize, 0);
      gtk_table_attach_defaults (GTK_TABLE (gse), gsef->value_spinbutton,
				 i+1, i+2,
				 gse->show_refval+1, gse->show_refval+2);
      gtk_signal_connect (GTK_OBJECT (gsef->value_spinbutton), "changed",
			  (GtkSignalFunc) gimp_size_entry_value_callback, gsef);
      gtk_signal_connect (GTK_OBJECT (gsef->value_spinbutton),
			  "focus_out_event",
			  (GdkEventFunc) gimp_size_entry_value_focus_out_callback,
			  gsef);
      gtk_widget_show (gsef->value_spinbutton);

      if (gse->show_refval)
	{
	  adjustment =
	    GTK_ADJUSTMENT (gtk_adjustment_new (gsef->refval,
						gsef->min_refval,
						gsef->max_refval,
						1.0, 10.0, 0.0));
	  gsef->refval_spinbutton = gtk_spin_button_new (adjustment,
							 1.0,
							 gsef->refval_digits);
	  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (gsef->refval_spinbutton),
					   GTK_SHADOW_NONE);
	  gtk_widget_set_usize (gsef->refval_spinbutton, spinbutton_usize, 0);
	  gtk_table_attach_defaults (GTK_TABLE (gse), gsef->refval_spinbutton,
				     i+1, i+2, 1, 2);
	  gtk_signal_connect (GTK_OBJECT (gsef->refval_spinbutton), "changed",
			      (GtkSignalFunc) gimp_size_entry_refval_callback,
			      gsef);
	  gtk_signal_connect (GTK_OBJECT (gsef->refval_spinbutton),
			      "focus_out_event",
			      (GdkEventFunc) gimp_size_entry_refval_focus_out_callback,
			      gsef);
	  gtk_widget_show (gsef->refval_spinbutton);
	}			

      if (gse->menu_show_pixels && !gse->show_refval && (unit == UNIT_PIXEL))
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
				    gsef->refval_digits);
    }

  gse->unitmenu = gimp_unit_menu_new (unit_format, unit,
				      gse->menu_show_pixels, TRUE);
  gtk_table_attach_defaults (GTK_TABLE (gse), gse->unitmenu,
			     i+2, i+3,
			     gse->show_refval+1, gse->show_refval+2);
  gtk_signal_connect (GTK_OBJECT (gse->unitmenu), "unit_changed",
		      (GtkSignalFunc) gimp_size_entry_unit_callback, gse);
  gtk_widget_show (gse->unitmenu);
  
  return GTK_WIDGET (gse);
}


/*  convenience function for labeling the widget  ***********/

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

  gtk_table_attach_defaults (GTK_TABLE (gse), label,
			     column, column+1, row, row+1);
  gtk_widget_show (label);
}


/*  resolution stuff  ***********/

void
gimp_size_entry_set_resolution (GimpSizeEntry *gse,
				gint           field,
				gfloat         resolution,
				guint          keep_size)
{
  GimpSizeEntryField *gsef;

  g_return_if_fail (gse != NULL);
  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));
  g_return_if_fail (resolution > 0.0);

  gsef = (GimpSizeEntryField*)g_slist_nth_data (gse->fields, field);
  gsef->resolution = resolution;

  if (keep_size)
    gimp_size_entry_set_refval (gse, field, gsef->refval);
  else
    gimp_size_entry_set_value (gse, field, gsef->value);
}


/*  value stuff  ***********/

void
gimp_size_entry_set_value_boundaries  (GimpSizeEntry *gse,
				       gint           field,
				       gfloat         lower,
				       gfloat         upper)
{
  GimpSizeEntryField *gsef;
  GtkAdjustment      *adjustment;

  g_return_if_fail (gse != NULL);
  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));
  g_return_if_fail (lower < upper);

  gsef = (GimpSizeEntryField*)g_slist_nth_data (gse->fields, field);
  gsef->min_value = lower;
  gsef->max_value = upper;

  adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0,
						   gsef->min_value, 
						   gsef->max_value,
						   1.0, 10.0, 10.0));
  gtk_signal_handler_block_by_data (GTK_OBJECT (gsef->value_spinbutton), gsef);
  gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (gsef->value_spinbutton),
				  adjustment);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (gsef->value_spinbutton), gsef);
  gimp_size_entry_set_value (gse, field, gsef->value);

  if (gimp_size_entry_stop_recursion) /* this is a hack (but useful ;-) */
    return;

  gimp_size_entry_stop_recursion++;
  switch (gsef->gse->update_policy)
    {
    case GIMP_SIZE_ENTRY_UPDATE_NONE:
      break;
    case GIMP_SIZE_ENTRY_UPDATE_SIZE:
      if (gse->unit) /* unit != UNIT_PIXEL */
	gimp_size_entry_set_refval_boundaries (gse, field,
					       gsef->min_value *
					       gsef->resolution /
					       gimp_unit_get_factor (gse->unit),
					       gsef->max_value *
					       gsef->resolution /
					       gimp_unit_get_factor (gse->unit));
      else /* unit == UNIT_PIXEL */
	gimp_size_entry_set_refval_boundaries (gse, field,
					       gsef->min_value,
					       gsef->max_value);
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
  gimp_size_entry_stop_recursion--;
}

gfloat
gimp_size_entry_get_value (GimpSizeEntry *gse,
			   gint           field)
{
  GimpSizeEntryField *gsef;

  g_return_val_if_fail (gse != NULL, 0);
  g_return_val_if_fail (GIMP_IS_SIZE_ENTRY (gse), 0);
  g_return_val_if_fail ((field >= 0) && (field < gse->number_of_fields), 0);

  gsef = (GimpSizeEntryField*)g_slist_nth_data (gse->fields, field);
  return gsef->value;
}

void
gimp_size_entry_update_value (GimpSizeEntryField *gsef,
			      gfloat              value)
{
  if (gimp_size_entry_stop_recursion > 1)
    return;

  gsef->value = value;

  if (gsef->gse->show_refval)
    gtk_signal_handler_block_by_data (GTK_OBJECT (gsef->refval_spinbutton),
				      gsef);

  switch (gsef->gse->update_policy)
    {
    case GIMP_SIZE_ENTRY_UPDATE_NONE:
      break;

    case GIMP_SIZE_ENTRY_UPDATE_SIZE:
      if (gsef->gse->unit) /* unit != UNIT_PIXEL */
	gsef->refval = gsef->value * gsef->resolution /
	  gimp_unit_get_factor (gsef->gse->unit);
      else /* unit == UNIT_PIXEL */
	gsef->refval = value;
      if (gsef->gse->show_refval)
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (gsef->refval_spinbutton),
				   gsef->refval);
      break;

    case GIMP_SIZE_ENTRY_UPDATE_RESOLUTION:
      gsef->refval = value * gimp_unit_get_factor (gsef->gse->unit);
      if (gsef->gse->show_refval)
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (gsef->refval_spinbutton),
				   gsef->refval);
      break;

    default:
      break;
    }

  if (gsef->gse->show_refval)
    gtk_signal_handler_unblock_by_data (GTK_OBJECT (gsef->refval_spinbutton),
					gsef);
}

void
gimp_size_entry_set_value (GimpSizeEntry *gse,
			   gint           field,
			   gfloat         value)
{
  GimpSizeEntryField *gsef;

  g_return_if_fail (gse != NULL);
  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));

  gsef = (GimpSizeEntryField*)g_slist_nth_data (gse->fields, field);

  if (value < gsef->min_value)
    value = gsef->min_value;
  if (value > gsef->max_value)
    value = gsef->max_value;

  gtk_signal_handler_block_by_data (GTK_OBJECT (gsef->value_spinbutton), gsef);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (gsef->value_spinbutton), value);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (gsef->value_spinbutton), gsef);
  gimp_size_entry_update_value (gsef, value);
}


static void
gimp_size_entry_value_callback (GtkWidget *widget,
				gpointer   data)
{
  GimpSizeEntryField *gsef;
  gfloat              new_value;

  gsef = (GimpSizeEntryField*)data;

  new_value =
    gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON(gsef->value_spinbutton));

  if (gsef->value != new_value)
    {
      gimp_size_entry_update_value (gsef, new_value);
      gtk_signal_emit (GTK_OBJECT (gsef->gse),
		       gimp_size_entry_signals[GSE_VALUE_CHANGED_SIGNAL]);
    }
}

static void
gimp_size_entry_value_focus_out_callback (GtkWidget *widget,
					  GdkEvent  *event,
					  gpointer   data)
{
  GimpSizeEntryField *gsef;

  gsef = (GimpSizeEntryField*)data;

  gtk_spin_button_update (GTK_SPIN_BUTTON (gsef->value_spinbutton));
  gimp_size_entry_value_callback (widget, data);
}


/*  refval stuff  ***********/

void
gimp_size_entry_set_refval_boundaries  (GimpSizeEntry *gse,
					gint           field,
					gfloat         lower,
					gfloat         upper)
{
  GimpSizeEntryField *gsef;
  GtkAdjustment      *adjustment;

  g_return_if_fail (gse != NULL);
  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));
  g_return_if_fail (lower < upper);

  gsef = (GimpSizeEntryField*)g_slist_nth_data (gse->fields, field);
  gsef->min_refval = lower;
  gsef->max_refval = upper;

  if (gse->show_refval)
    {
      adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0,
						       gsef->min_refval, 
						       gsef->max_refval,
						       1.0, 10.0, 10.0));
      gtk_signal_handler_block_by_data (GTK_OBJECT (gsef->refval_spinbutton),
					gsef);
      gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (gsef->refval_spinbutton),
				      adjustment);
      gtk_signal_handler_unblock_by_data (GTK_OBJECT (gsef->refval_spinbutton),
					  gsef);
    }
  gimp_size_entry_set_refval (gse, field, gsef->refval);

  if (gimp_size_entry_stop_recursion) /* this is a hack (but useful ;-) */
    return;

  gimp_size_entry_stop_recursion++;
  switch (gsef->gse->update_policy)
    {
    case GIMP_SIZE_ENTRY_UPDATE_NONE:
      break;

    case GIMP_SIZE_ENTRY_UPDATE_SIZE:
      if (gse->unit) /* unit != UNIT_PIXEL */
	gimp_size_entry_set_value_boundaries (gse, field,
					      gsef->min_refval *
					      gimp_unit_get_factor (gse->unit) /
					      gsef->resolution,
					      gsef->max_refval *
					      gimp_unit_get_factor (gse->unit) /
					      gsef->resolution);
      else /* unit == UNIT_PIXEL */
	gimp_size_entry_set_value_boundaries (gse, field,
					      gsef->min_refval,
					      gsef->max_refval);
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
  gimp_size_entry_stop_recursion--;
}

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

  gsef = (GimpSizeEntryField*)g_slist_nth_data (gse->fields, field);
  gsef->refval_digits = digits;

  if (gse->update_policy == GIMP_SIZE_ENTRY_UPDATE_SIZE)
    {
      if (gse->show_refval)
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->refval_spinbutton),
				    gsef->refval_digits);
      else if (gse->unit == UNIT_PIXEL)
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
				    gsef->refval_digits);
    }
}

gfloat
gimp_size_entry_get_refval (GimpSizeEntry *gse,
			    gint           field)
{
  GimpSizeEntryField *gsef;

  /*  return 1.0 to avoid division by zero  */
  g_return_val_if_fail (gse != NULL, 1.0);
  g_return_val_if_fail (GIMP_IS_SIZE_ENTRY (gse), 1.0);
  g_return_val_if_fail ((field >= 0) && (field < gse->number_of_fields), 1.0);

  gsef = (GimpSizeEntryField*)g_slist_nth_data (gse->fields, field);
  return gsef->refval;
}

void
gimp_size_entry_update_refval (GimpSizeEntryField *gsef,
			       gfloat              refval)
{
  if (gimp_size_entry_stop_recursion > 1)
    return;

  gsef->refval = refval;

  gtk_signal_handler_block_by_data (GTK_OBJECT (gsef->value_spinbutton), gsef);

  switch (gsef->gse->update_policy)
    {
    case GIMP_SIZE_ENTRY_UPDATE_NONE:
      break;

    case GIMP_SIZE_ENTRY_UPDATE_SIZE:
      if (gsef->gse->unit) /* unit != UNIT_PIXEL */
	gsef->value = gsef->refval * gimp_unit_get_factor (gsef->gse->unit) /
	  gsef->resolution;
      else /* unit == UNIT_PIXEL */
	gsef->value = refval;
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (gsef->value_spinbutton),
				 gsef->value);
      break;

    case GIMP_SIZE_ENTRY_UPDATE_RESOLUTION:
      gsef->value = refval / gimp_unit_get_factor (gsef->gse->unit);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (gsef->value_spinbutton),
				 gsef->value);
      break;

    default:
      break;
    }

  gtk_signal_handler_unblock_by_data (GTK_OBJECT (gsef->value_spinbutton), gsef);
}

void
gimp_size_entry_set_refval (GimpSizeEntry *gse,
			    gint           field,
			    gfloat         refval)
{
  GimpSizeEntryField *gsef;

  g_return_if_fail (gse != NULL);
  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));

  gsef = (GimpSizeEntryField*)g_slist_nth_data (gse->fields, field);

  if (refval < gsef->min_refval)
    refval = gsef->min_refval;
  if (refval > gsef->max_refval)
    refval = gsef->max_refval;

  if (gse->show_refval)
    {
      gtk_signal_handler_block_by_data (GTK_OBJECT (gsef->refval_spinbutton),
					gsef);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (gsef->refval_spinbutton),
				 refval);
      gtk_signal_handler_unblock_by_data (GTK_OBJECT (gsef->refval_spinbutton),
					  gsef);
    }
  gimp_size_entry_update_refval (gsef, refval);
}

static void
gimp_size_entry_refval_callback (GtkWidget *widget,
				 gpointer data)
{
  GimpSizeEntryField *gsef;
  gfloat              new_refval;

  gsef = (GimpSizeEntryField*)data;

  new_refval =
    gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (gsef->refval_spinbutton));

  if (gsef->refval != new_refval)
    {
      gimp_size_entry_update_refval (gsef, new_refval);
      gtk_signal_emit (GTK_OBJECT (gsef->gse),
		       gimp_size_entry_signals[GSE_REFVAL_CHANGED_SIGNAL]);
    }
}

static void
gimp_size_entry_refval_focus_out_callback (GtkWidget *widget,
					   GdkEvent  *event,
					   gpointer   data)
{
  GimpSizeEntryField *gsef;

  gsef = (GimpSizeEntryField*)data;

  gtk_spin_button_update (GTK_SPIN_BUTTON (gsef->refval_spinbutton));
  gimp_size_entry_refval_callback (widget, data);
}


/*  unit stuff  ***********/

GUnit
gimp_size_entry_get_unit (GimpSizeEntry *gse)
{
  g_return_val_if_fail (gse != NULL, UNIT_INCH);
  g_return_val_if_fail (GIMP_IS_SIZE_ENTRY (gse), UNIT_INCH);

  return gse->unit;
}

void
gimp_size_entry_update_unit (GimpSizeEntry *gse,
			     GUnit          unit)
{
  GimpSizeEntryField *gsef;
  gint                i;
  gint                digits;

  gse->unit = unit;

  for (i = 0; i < gse->number_of_fields; i++)
    {
      gsef = (GimpSizeEntryField*)g_slist_nth_data(gse->fields, i);

      gtk_signal_handler_block_by_data (GTK_OBJECT (gsef->value_spinbutton),
					gsef);
      if (gse->update_policy == GIMP_SIZE_ENTRY_UPDATE_SIZE)
	{
	  if (unit == UNIT_PIXEL)
	    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
					gsef->refval_digits);
	  else
	    gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
					MAX(gimp_unit_get_digits (unit) + 1, 3));
	}
      else if (gse->update_policy == GIMP_SIZE_ENTRY_UPDATE_RESOLUTION)
	{
	  digits =
	    -(gimp_unit_get_digits (unit) - gimp_unit_get_digits (UNIT_INCH));
	  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
				      MAX(3 + digits, 3));
	}
      gtk_signal_handler_unblock_by_data (GTK_OBJECT (gsef->value_spinbutton),
					  gsef);

      gimp_size_entry_stop_recursion = 0; /* hack !!! */
      gimp_size_entry_set_refval_boundaries (gse, i,
					     gsef->min_refval,
					     gsef->max_refval);
      gimp_size_entry_set_refval (gse, i, gsef->refval);
    }
}

void
gimp_size_entry_set_unit (GimpSizeEntry *gse, 
			  GUnit          unit)
{
  g_return_if_fail (gse != NULL);
  g_return_if_fail (GIMP_IS_SIZE_ENTRY (gse));
  g_return_if_fail (gse->menu_show_pixels || (unit != UNIT_PIXEL));

  gimp_unit_menu_set_unit (GIMP_UNIT_MENU (gse->unitmenu), unit);
  gimp_size_entry_update_unit (gse, unit);
}

static void
gimp_size_entry_unit_callback (GtkWidget *widget,
			       gpointer   data)
{
  gimp_size_entry_update_unit (GIMP_SIZE_ENTRY (data),
			       gimp_unit_menu_get_unit(GIMP_UNIT_MENU(widget)));
  gtk_signal_emit (GTK_OBJECT (data),
		   gimp_size_entry_signals[GSE_UNIT_CHANGED_SIGNAL]);
}
