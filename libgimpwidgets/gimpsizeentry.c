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
#include <stdio.h>
#include <math.h>

#include "gimpsizeentry.h"
#include "gimpunitmenu.h"


#define SIZE_MAX_VALUE 500000.0     /* is that enough ?? */


static void gimp_size_entry_unit_callback (GtkWidget *widget, gpointer data);
static void gimp_size_entry_value_callback (GtkWidget *widget, gpointer data);

enum {
  GSE_VALUE_CHANGED_SIGNAL,
  GSE_UNIT_CHANGED_SIGNAL,
  GSE_RESOLUTION_CHANGED_SIGNAL,
  LAST_SIGNAL
};

static gint gimp_size_entry_signals[LAST_SIGNAL] = { 0 };

static void
gimp_size_entry_class_init (GimpSizeEntryClass *class)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) class;

  gimp_size_entry_signals[GSE_VALUE_CHANGED_SIGNAL] = 
              gtk_signal_new ("value_changed",
			      GTK_RUN_FIRST,
			      object_class->type,
			      GTK_SIGNAL_OFFSET (GimpSizeEntryClass, gimp_size_entry),
			      gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
  gimp_size_entry_signals[GSE_UNIT_CHANGED_SIGNAL] = 
              gtk_signal_new ("unit_changed",
			      GTK_RUN_FIRST,
			      object_class->type,
			      GTK_SIGNAL_OFFSET (GimpSizeEntryClass, gimp_size_entry),
			      gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
  gimp_size_entry_signals[GSE_RESOLUTION_CHANGED_SIGNAL] = 
              gtk_signal_new ("resolution_changed",
			      GTK_RUN_FIRST,
			      object_class->type,
			      GTK_SIGNAL_OFFSET (GimpSizeEntryClass, gimp_size_entry),
			      gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
  gtk_object_class_add_signals (object_class, gimp_size_entry_signals, 
				LAST_SIGNAL);
  class->gimp_size_entry = NULL;
}

static void
gimp_size_entry_init (GimpSizeEntry *gse)
{
  
  GtkWidget     *spinbutton;
  GtkWidget     *unitmenu;
  GtkAdjustment *adj;

  adj = (GtkAdjustment *) gtk_adjustment_new (0.0, -SIZE_MAX_VALUE, SIZE_MAX_VALUE,
                                              1.0, 50.0, 0.0);
  spinbutton = gtk_spin_button_new (adj, 1.0, 0);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON(spinbutton), GTK_SHADOW_NONE);
  gtk_widget_set_usize (spinbutton, 75, 0);
  gtk_signal_connect (GTK_OBJECT (spinbutton), "changed",
		      (GtkSignalFunc) gimp_size_entry_value_callback, gse);
  gtk_box_pack_start (GTK_BOX(gse), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  unitmenu = gimp_unit_menu_new("%a", UNIT_PIXEL, TRUE, FALSE);
  gtk_signal_connect (GTK_OBJECT (unitmenu), "unit_changed",
		      (GtkSignalFunc) gimp_size_entry_unit_callback, gse);
  gtk_box_pack_start (GTK_BOX(gse), unitmenu, FALSE, FALSE, 0);
  gtk_widget_show(unitmenu);

  gse->spinbutton = spinbutton;
  gse->unitmenu = unitmenu;
  gse->resolution = 72;
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

      gse_type = gtk_type_unique (gtk_hbox_get_type (), &gse_info);
    }
  
  return gse_type;
}


GtkWidget*
gimp_size_entry_new (gfloat value, 
		     GUnit  unit, 
		     gfloat resolution,
		     guint  positive_only)
{
  GimpSizeEntry *gse;
  float          max;
  GtkAdjustment *adj;

  if (resolution <= 0.0)
    return (NULL);

  gse = gtk_type_new (gimp_size_entry_get_type ());
  
  gse->resolution = resolution;
  gse->positive_only = positive_only;

  gimp_unit_menu_set_unit (GIMP_UNIT_MENU (gse->unitmenu), unit);
  
  if ((positive_only == TRUE) && (value <= 0.0))
    value = 0.0;
 
  if (unit) /* unit != UNIT_PIXEL */
    {
      max = SIZE_MAX_VALUE / gse->resolution * gimp_unit_get_factor(unit);
      adj = (GtkAdjustment *) gtk_adjustment_new (0.0, 
						  ((positive_only ==TRUE) ? 0.0 : -max),
						  max, 0.1, 5.0, 0.0);
      gtk_spin_button_configure (GTK_SPIN_BUTTON (gse->spinbutton), adj, 0.01, 2);
    }
  else 
    {
      if (positive_only == TRUE)
	{
	  adj = (GtkAdjustment *) gtk_adjustment_new (0.0, 0, SIZE_MAX_VALUE,
						      1.0, 50.0, 0.0);
	  gtk_spin_button_configure (GTK_SPIN_BUTTON (gse->spinbutton), adj, 1.0, 0);
	}
    }

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (gse->spinbutton), value);

  return GTK_WIDGET (gse);
}

void
gimp_size_entry_set_value (GimpSizeEntry *gse,
			   gfloat value)
{
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (gse->spinbutton), value);
}


void
gimp_size_entry_set_value_as_pixels (GimpSizeEntry *gse,
				     gint pixels)
{
  gfloat value;

  if (gse->unit) /* unit != UNIT_PIXEL */
    value = (float)pixels / gse->resolution * gimp_unit_get_factor(gse->unit); 
  else
    value = (float)pixels;

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (gse->spinbutton), value);
}


gfloat
gimp_size_entry_get_value (GimpSizeEntry *gse)
{
  return (gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (gse->spinbutton)));
}

gint
gimp_size_entry_get_value_as_pixels (GimpSizeEntry *gse)
{
  gfloat value;

  value = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (gse->spinbutton));
  if (gse->unit) /* unit != UNIT_PIXEL */
    value = value * gse->resolution / gimp_unit_get_factor(gse->unit); 

  if (value - floor (value) < ceil (value) - value)
    return floor (value);
  else
    return ceil (value);
}

void
gimp_size_entry_set_unit (GimpSizeEntry *gse,
			  GUnit new_unit)
{
  gfloat         value;
  gfloat         max;
  gfloat         new_factor;
  GtkAdjustment *adj;

  if (new_unit == gse->unit) return;
  
  value = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (gse->spinbutton));

  gtk_signal_handler_block_by_data (GTK_OBJECT(gse->spinbutton), gse);
  
  if (new_unit) /* unit != UNIT_PIXEL */
    {
      new_factor = gimp_unit_get_factor (new_unit);
      max = SIZE_MAX_VALUE / gse->resolution * new_factor;
      adj = (GtkAdjustment *)gtk_adjustment_new (0.0, 
						 ((gse->positive_only == TRUE) ? 0.0 : -max),
						 max, 0.1, 5.0, 0.0);
      gtk_spin_button_configure (GTK_SPIN_BUTTON (gse->spinbutton), adj, 0.01, 2);
      value = value * new_factor / gimp_unit_get_factor(gse->unit);
      if (gse->unit == UNIT_PIXEL)
	value = value / gse->resolution;
    }
  else
    {
      max = SIZE_MAX_VALUE;
      adj = (GtkAdjustment *)gtk_adjustment_new (0.0, 
						 ((gse->positive_only == TRUE) ? 0.0 : -max),
						 max, 0.1, 5.0, 0.0);
      gtk_spin_button_configure (GTK_SPIN_BUTTON (gse->spinbutton), adj, 1.0, 0);
      value = value * gse->resolution / gimp_unit_get_factor(gse->unit);
    }

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (gse->spinbutton), value);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT(gse->spinbutton), gse);

  gimp_unit_menu_set_unit (GIMP_UNIT_MENU (gse->unitmenu), new_unit);
  gse->unit = new_unit;
}

GUnit
gimp_size_entry_get_unit (GimpSizeEntry *gse)
{
  return (gse->unit);
}

void
gimp_size_entry_set_resolution (GimpSizeEntry *gse,
				gfloat resolution)
{
  if (resolution <= 0.0)
    return;

  /* This function does NOT change the value of the size_entry
     for you! You have to take care of that yourself.           */
  gse->resolution = resolution;
  gtk_signal_emit (GTK_OBJECT (gse), gimp_size_entry_signals[GSE_RESOLUTION_CHANGED_SIGNAL]);
}


static void
gimp_size_entry_unit_callback (GtkWidget *widget,
				gpointer   data)
{
  GimpSizeEntry *gse;

  gse = data;
  
  gse->unit = gimp_unit_menu_get_unit (GIMP_UNIT_MENU(gse->unitmenu));
  
  gtk_signal_emit (GTK_OBJECT (gse), gimp_size_entry_signals[GSE_UNIT_CHANGED_SIGNAL]);
}

static void
gimp_size_entry_value_callback (GtkWidget *widget,
				gpointer   data)
{
  GimpSizeEntry *gse;

  gse = data;
  gtk_signal_emit (GTK_OBJECT (gse), gimp_size_entry_signals[GSE_VALUE_CHANGED_SIGNAL]);
}
