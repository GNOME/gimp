/* $Id$
 * dialog_f.c -- functions for creating a GTK gdouble slider/value input.
 *
 * Copyright (C) 1999 Winston Chang
 *                    <wchang3@students.wisc.edu>
 *                    <winston@stdout.org>
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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dialog_f.h"

/*
 * 'dialog_create_value_f()' - Create an gdouble value control...
 */
void
dialog_create_value_f(char    *title,	  /* Label for control */
	                   GtkTable *table,	  /* Table container to use */
	                   int      row,	    /* Row # for container */
	                   gdouble  *value,	  /* Value holder */
                     gdouble  increment,/* Size of mouse-click and
                                            keyboard increment */
                     gint     precision,/* Number of digits after decimal point */
	                   int      left,	    /* Minimum value for slider */
	                   int      right)	  /* Maximum value for slider */
{
	GtkWidget	*label;       /* Control label */
	GtkWidget *scale;       /* Scale widget */
	GtkWidget *entry;       /* Text widget */
	GtkObject	*scale_data;	/* Scale data */
	char      buf[256];     /* String buffer */

 /* Create label... */

	label = gtk_label_new(title);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 1.0);
	gtk_table_attach(table, label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, 4, 0);
	gtk_widget_show(label);

 /*
	* Scale...
	*/
	/* the "right+increment" is necessary to make it stop on 5.0 instead
	   of 4.9.   I think this is a shortcoming of GTK's adjustments */
	scale_data = gtk_adjustment_new(*value, left, right+increment,
	                                 increment, increment, increment);

	gtk_signal_connect(GTK_OBJECT(scale_data), "value_changed",
		     (GtkSignalFunc) dialog_fscale_update, value);

	scale = gtk_hscale_new(GTK_ADJUSTMENT(scale_data));
	gtk_scale_set_digits( GTK_SCALE(scale), precision);
	gtk_widget_set_usize(scale, SCALE_WIDTH, 0);
	gtk_table_attach( table, scale, 1, 2, row, row + 1,
	                  GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
	gtk_scale_set_draw_value( GTK_SCALE(scale), FALSE);
	gtk_range_set_update_policy( GTK_RANGE(scale), GTK_UPDATE_CONTINUOUS);
	gtk_widget_show(scale);

 /*
	* Text entry...
	*/

	entry = gtk_entry_new();
	gtk_object_set_user_data(GTK_OBJECT(entry), scale_data);
	gtk_object_set_user_data(scale_data, entry);
	gtk_widget_set_usize(entry, ENTRY_WIDTH, 0);
	sprintf(buf, "%g", *value);
	gtk_entry_set_text(GTK_ENTRY(entry), buf);
	gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc) dialog_fentry_update,
		     value);
	gtk_table_attach( GTK_TABLE(table), entry, 2, 3, row, row + 1,
	                  GTK_FILL, GTK_FILL, 4, 0);
	gtk_widget_show(entry);
}


/*
 * 'dialog_fscale_update()' - Update the value field using the scale.
 */

void
dialog_fscale_update(GtkAdjustment *adjustment,	/* I - New value */
	                   gdouble       *value)	/* I - Current value */
{
	GtkWidget	*entry;		/* Text entry widget */
	char		buf[256];	/* Text buffer */


	if (*value != adjustment->value)
	{
	  *value = adjustment->value;

	  entry = gtk_object_get_user_data(GTK_OBJECT(adjustment));
		
		/* UGLY HACK ALERT */
		/* use precision of 5 */
	  sprintf(buf, "%.5g", *value);
		/* This is to round the number to a reasonable value.  For some
		   some reason it wants to increment by about 0.1000000105 instead
		   of 0.1.  That annoys me. */
		*value = atof(buf);

		/* assign the text value to the entry */
		gtk_signal_handler_block_by_data(GTK_OBJECT(entry), value);
	  gtk_entry_set_text(GTK_ENTRY(entry), buf);
	  gtk_signal_handler_unblock_by_data(GTK_OBJECT(entry), value);
		
		/* assign the rounded value back the adjustment */
		adjustment->value = *value;

		
	}
}


/*
 * 'dialog_fentry_update()' - Update the value field using the text entry.
 */

void
dialog_fentry_update(GtkWidget *widget,   /* I - Entry widget */
	                   gdouble   *value) /* I - Current value */
{
	GtkAdjustment	*adjustment;
	gdouble       new_value;
	/* these three are for the string cleaner */
	gint          shift;
	gboolean      periodfound;
	gint          digits_after_period;
	gchar*        textvalue;
	gchar         newtextvalue[MAX_ENTRY_LENGTH+1];
	gint          oldtextlength;
	gint          i;

/*	new_value = atod(gtk_entry_get_text(GTK_ENTRY(widget))); */

	textvalue = gtk_entry_get_text( GTK_ENTRY(widget));
	strncpy(newtextvalue, textvalue, MAX_ENTRY_LENGTH);
	/* clean up new value so that it only has one digit after decimal
	   and no weird chars */
	newtextvalue[MAX_ENTRY_LENGTH] ='\0';
	oldtextlength = strlen(newtextvalue);

	/* this stuff cleans up non-numeric chars */
	shift=0;
	periodfound = FALSE;
	digits_after_period = 0;
	for (i=0; i+shift < oldtextlength; i++) {
		if (newtextvalue[i] == '.') {
			if (periodfound) shift++;  /* if not first period, ignore */
			else periodfound = TRUE;   /* if first period, mark periodfound */
		}
		else if (newtextvalue[i]>='0' && newtextvalue[i]<='9') {
			if (periodfound) {
				digits_after_period++;
				/* this is to ignore a certain number of digits after period */
				if (digits_after_period > ENTRY_PRECISION) shift++;
			}
		}
		else shift++; /* ignore non-numeric chars */

		/* copy shifted value back, making sure we don't run off end */
		if (i+shift >= MAX_ENTRY_LENGTH) newtextvalue[i] = '\0';
		else newtextvalue[i] = newtextvalue[i+shift];
	}
	newtextvalue[i]='\0';



	/* write the cleaned-up value into the entry box */
	/* block the signal handler so we don't go into an infinite recursion */
	gtk_signal_handler_block_by_data(GTK_OBJECT(widget), value);
	/* update the entry text */
	gtk_entry_set_text( GTK_ENTRY(widget), newtextvalue);
	/* gtk_entry_set_position( GTK_ENTRY(widget), i); */ /* doesn't seem to work */
	/* unblock signal handler */
	gtk_signal_handler_unblock_by_data(GTK_OBJECT(widget), value);
							

/*	g_print(newtextvalue); */
/*	g_print("\n"); */
	/* set the adjustment thingy */
	new_value = atof(newtextvalue);
	
/*	g_print("%e.", new_value); */

	/* set the new value */
	if (*value != new_value) {
	  adjustment = gtk_object_get_user_data(GTK_OBJECT(widget));

	  if ((new_value >= adjustment->lower) &&
		    (new_value <= adjustment->upper))    {
	    *value            = new_value;
	    adjustment->value = new_value;

	    gtk_signal_emit_by_name(GTK_OBJECT(adjustment), "value_changed");

	  }
	}
}

