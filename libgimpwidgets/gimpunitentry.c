/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunitentry.c
 * Copyright (C) 2011 Enrico Schröder <enni.schroeder@gmail.com>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkspinbutton.h>
#include <gdk/gdkkeysyms.h>

#include <glib/gprintf.h>

#include "libgimpbase/gimpbase.h"

#include "gimpwidgets.h"

#include "gimpeevl.h"
#include "gimpunitentry.h"
#include "gimpunitadjustment.h"

G_DEFINE_TYPE (GimpUnitEntry, gimp_unit_entry, GTK_TYPE_SPIN_BUTTON);

/* unit resolver for GimpEevl */
static gboolean unit_resolver (const gchar      *ident,
                               GimpEevlQuantity *result,
                               gpointer          data);

/* read and parse entered text */
static gboolean gimp_unit_entry_parse (GimpUnitEntry *unitEntry);

/**
 * event handlers
 **/
static gint gimp_unit_entry_focus_out      (GtkWidget          *widget,
                                            GdkEventFocus      *event);
static gint gimp_unit_entry_button_press   (GtkWidget          *widget,
                                            GdkEventButton     *event);
static gint gimp_unit_entry_button_release (GtkWidget          *widget,
                                            GdkEventButton     *event);
static gint gimp_unit_entry_scroll         (GtkWidget          *widget,
                                            GdkEventScroll     *event);
static gint gimp_unit_entry_key_press      (GtkWidget          *widget,
                                            GdkEventKey        *event);
static gint gimp_unit_entry_key_release    (GtkWidget          *widget,
                                            GdkEventKey        *event);


/**
 *  signal handlers
 **/

/* format displayed text (signal emmitted by GtkSpinButton before text is displayed) */
static gboolean on_output   (GtkSpinButton      *spin, 
                             gpointer           data);
/* parse and process entered text (signal emmited from GtkEntry) */
static void on_text_changed (GtkEditable        *editable,
                             gpointer           user_data);
static void on_insert_text  (GtkEditable *editable,
                             gchar *new_text,
                             gint new_text_length,
                             gint *position,
                             gpointer user_data);
static gint on_input        (GtkSpinButton *spinbutton,
                             gpointer       arg1,
                             gpointer       user_data);

static void
gimp_unit_entry_init (GimpUnitEntry *unitEntry)
{
  GimpUnitEntryClass *class = GIMP_UNIT_ENTRY_GET_CLASS (unitEntry);
  
  /* create and set our adjustment subclass */
  GObject *adjustment = gimp_unit_adjustment_new ();

  unitEntry->unitAdjustment = GIMP_UNIT_ADJUSTMENT (adjustment);
  gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (unitEntry), 
                                  GTK_ADJUSTMENT (adjustment));

  /* some default values */
  unitEntry->buttonPressed = FALSE;        
  unitEntry->scrolling = FALSE;                                   

  /* connect signals */
  /* we don't need all of them... */
  g_signal_connect (&unitEntry->parent_instance, 
                    "output",
                    G_CALLBACK(on_output), 
                    (gpointer) adjustment);
  g_signal_connect (&unitEntry->parent_instance.entry, 
                    "insert-text",
                    G_CALLBACK(on_insert_text), 
                    (gpointer) unitEntry);
  g_signal_connect (&unitEntry->parent_instance, 
                    "input",
                    G_CALLBACK(on_input), 
                    (gpointer) unitEntry);
  g_signal_connect (&unitEntry->parent_instance.entry,
                    "changed",
                    G_CALLBACK(on_text_changed), 
                    (gpointer) unitEntry);

  unitEntry->id = class->id;
  class->id++;
}

static void
gimp_unit_entry_class_init (GimpUnitEntryClass *class)
{
  GtkWidgetClass   *widgetClass = GTK_WIDGET_CLASS (class);

  /* some events we need to catch before our parent */
  widgetClass->button_press_event   = gimp_unit_entry_button_press;
  widgetClass->button_release_event = gimp_unit_entry_button_release;
  widgetClass->scroll_event         = gimp_unit_entry_scroll;
  widgetClass->key_press_event      = gimp_unit_entry_key_press;
  widgetClass->key_release_event    = gimp_unit_entry_key_release;

  class->id = 0;
}

GtkWidget*
gimp_unit_entry_new (void)
{
  return g_object_new (GIMP_TYPE_UNIT_ENTRY, NULL);
}

GimpUnitAdjustment*
gimp_unit_entry_get_adjustment (GimpUnitEntry *entry)
{
  return entry->unitAdjustment;
}

/* connect to another entry */
void 
gimp_unit_entry_connect (GimpUnitEntry *entry, GimpUnitEntry *target)
{
  gimp_unit_adjustment_connect (entry->unitAdjustment, target->unitAdjustment);
}

/* read and parse entered text */
static gboolean
gimp_unit_entry_parse (GimpUnitEntry *entry)
{
  gdouble           newValue;
  /* GimpEevl related stuff */
  GimpEevlQuantity  result;
  GError            *error    = NULL;
  const gchar       *errorpos = 0;
  /* text to parse */
  const gchar       *str      = gtk_entry_get_text (GTK_ENTRY (entry));

  if (strlen (str) <= 0)
    return FALSE;
  
  /** 
   * purpose of enteredUnit: use first unit unit_resolver will be called with  
   * as entered unit. enteredUnit is reset now to know which unit was the first one
   **/
  entry->enteredUnit = -1;

  g_debug ("%i parsing: %s", entry->id, str);

  /* parse text via GimpEevl */
  gimp_eevl_evaluate (str,
                      unit_resolver,
                      &result,
                      (gpointer) entry,
                      &errorpos,
                      &error);

  if (error || errorpos)
  {
    GdkColor color;
    gdk_color_parse ("LightSalmon", &color);
    gtk_widget_modify_base (GTK_WIDGET (entry), GTK_STATE_NORMAL, &color);

    g_debug ("gimpeevl parsing error \n");
    return FALSE;
  }
  else
  {
    gtk_widget_modify_base (GTK_WIDGET (entry), GTK_STATE_NORMAL, NULL);
    g_debug ("%i gimpeevl parser result: %s = %lg (%d)", entry->id, str, result.value, result.dimension);
    g_debug ("%i determined unit: %s\n", entry->id, gimp_unit_get_abbreviation (entry->enteredUnit));

    /* set new unit */  
    if (entry->enteredUnit != entry->unitAdjustment->unit)
    {
      gimp_unit_adjustment_set_unit (entry->unitAdjustment, entry->enteredUnit);
    }

    /* set new value */
    if (gimp_unit_adjustment_get_value (entry->unitAdjustment) != result.value)
    {
      /* result from parser is in inch, so convert to desired unit */
      newValue = gimp_units_to_pixels (result.value,
                                       GIMP_UNIT_INCH,
                                       entry->unitAdjustment->resolution);
      newValue = gimp_pixels_to_units (newValue,
                                       entry->unitAdjustment->unit, 
                                       entry->unitAdjustment->resolution);

      gimp_unit_adjustment_set_value (entry->unitAdjustment, newValue);

      g_object_notify (G_OBJECT ( GTK_SPIN_BUTTON (entry)), "value");
    }
  }

  return TRUE;
}

/**
 * signal handlers
 **/

/* format displayed text, displays "[value] [unit]" (gets called by GtkSpinButton) */
static gboolean 
on_output (GtkSpinButton *spin, gpointer data)
{
  gchar *text;
  GimpUnitAdjustment *adj   = GIMP_UNIT_ADJUSTMENT (data);
  GimpUnitEntry      *entry = GIMP_UNIT_ENTRY (spin); 

  /* return if widget still has focus => user input must not be overwritten */
  if (gtk_widget_has_focus (GTK_WIDGET (spin)) && 
      !entry->buttonPressed &&
      !entry->scrolling)
  {
    return TRUE;
  }

  /* parse once more to prevent value from being overwritten somewhere in GtkSpinButton or 
     GtkEntry. If we don't do that, the entered text is truncated at the first space.
     TODO: find out where and why
     not very elegant, because we have do deactivate parsing in case the value was modified
     due to a unit change or up/down buttons or keys
  */
  if(adj->unitChanged || entry->buttonPressed || entry->scrolling)
  {
    /* reset certain flags */
    adj->unitChanged = FALSE;
    entry->scrolling = FALSE;
  }
  else
    gimp_unit_entry_parse (GIMP_UNIT_ENTRY (spin));
  
  text = gimp_unit_adjustment_to_string (adj);

  g_debug ("on_output: %s\n", text);

  gtk_entry_set_text (GTK_ENTRY (spin), text);

  g_free (text);

  return TRUE;
}

static 
void on_insert_text (GtkEditable *editable,
                     gchar *new_text,
                     gint new_text_length,
                     gint *position,
                     gpointer user_data)
{
  g_debug ("on_insert_text\n");
}

/* parse and process entered text (signal emmited from GtkEntry) */
static 
void on_text_changed (GtkEditable *editable, gpointer user_data)
{
  GimpUnitEntry *entry = GIMP_UNIT_ENTRY (user_data);

  g_debug ("on_text_changed\n"); 

  gimp_unit_entry_parse (entry);
}

/* unit resolver for GimpEevl */
static gboolean
unit_resolver (const gchar      *ident,
               GimpEevlQuantity *result,
               gpointer          user_data)
{
  GimpUnitEntry   *entry       = GIMP_UNIT_ENTRY (user_data);
  GimpUnit        *unit        = &(entry->enteredUnit);
  gboolean        resolved     = FALSE;
  gboolean        default_unit = (ident == NULL);
  gint            numUnits     = gimp_unit_get_number_of_units ();
  const gchar     *abbr; 
  gint            i            = 0;

  result->dimension = 1;

  /* if no unit is specified, use default unit */
  if (default_unit)
  {
    /* if default hasn't been set before, set to inch*/
    if (*unit == -1)
      *unit = GIMP_UNIT_INCH;

    result->dimension = 1;

    if (*unit == GIMP_UNIT_PIXEL) /* handle case that unit is px */
      result->value = gimp_unit_entry_get_adjustment (entry)->resolution;
    else                          /* otherwise use factor */
      result->value = gimp_unit_get_factor (*unit);

    resolved          = TRUE; 
    return resolved;
  }

  /* find matching unit */
  for (i = 0; i < numUnits; i++)
  {
    abbr = gimp_unit_get_abbreviation (i);

    if (strcmp (abbr, ident) == 0)
    {
      /* handle case that unit is px */
      if (i == GIMP_UNIT_PIXEL)
        result->value = gimp_unit_entry_get_adjustment (entry)->resolution;
      else
        result->value = gimp_unit_get_factor (i);

      if (*unit == -1)
        *unit = i;

      i = numUnits;
      resolved = TRUE;
    }
  }

  return resolved;
}

static 
gint on_input        (GtkSpinButton *spinButton,
                      gpointer       arg1,
                      gpointer       user_data)
{
  return FALSE;
}

static gint 
gimp_unit_entry_focus_out (GtkWidget          *widget,
                           GdkEventFocus      *event)
{
  GtkEntryClass *class = GTK_ENTRY_CLASS (gimp_unit_entry_parent_class);

  g_debug ("focus_out\n");

  return GTK_WIDGET_CLASS (class)->focus_out_event (widget, event);
}

static 
gint gimp_unit_entry_button_press          (GtkWidget          *widget,
                                            GdkEventButton     *event)
{
  GtkSpinButtonClass *class = GTK_SPIN_BUTTON_CLASS (gimp_unit_entry_parent_class);
  GimpUnitEntry      *entry = GIMP_UNIT_ENTRY (widget);

  /* disable output (i.e. parsing and overwriting of our value) while button pressed */
  entry->buttonPressed = TRUE;
   
  return GTK_WIDGET_CLASS(class)->button_press_event (widget, event);
}
static 
gint gimp_unit_entry_button_release          (GtkWidget          *widget,
                                              GdkEventButton     *event)
{
  GtkSpinButtonClass *class = GTK_SPIN_BUTTON_CLASS (gimp_unit_entry_parent_class);
  GimpUnitEntry      *entry = GIMP_UNIT_ENTRY (widget);

  /* reenable output */
  entry->buttonPressed = FALSE;
   
  return GTK_WIDGET_CLASS(class)->button_release_event (widget, event);
}

static 
gint gimp_unit_entry_scroll                (GtkWidget          *widget,
                                            GdkEventScroll     *event)
{
  GtkSpinButtonClass *class = GTK_SPIN_BUTTON_CLASS (gimp_unit_entry_parent_class);
  GimpUnitEntry      *entry = GIMP_UNIT_ENTRY (widget);

  entry->scrolling = TRUE;  

  return GTK_WIDGET_CLASS(class)->scroll_event (widget, event);
}

static 
gint gimp_unit_entry_key_press      (GtkWidget          *widget,
                                     GdkEventKey        *event)
{
  GtkSpinButtonClass *class = GTK_SPIN_BUTTON_CLASS (gimp_unit_entry_parent_class);
  GimpUnitEntry      *entry = GIMP_UNIT_ENTRY (widget);

  /* disable output for up/down keys */
  switch (event->keyval)
  {
    case GDK_Up:
    case GDK_Down:
      entry->buttonPressed = TRUE;
      break;

    default:
      break;
  }
   
  return GTK_WIDGET_CLASS(class)->key_press_event (widget, event);
}
static 
gint gimp_unit_entry_key_release    (GtkWidget          *widget,
                                     GdkEventKey        *event)
{
  GtkSpinButtonClass *class = GTK_SPIN_BUTTON_CLASS (gimp_unit_entry_parent_class);
  GimpUnitEntry      *entry = GIMP_UNIT_ENTRY (widget);

  /* reenable output */
  switch (event->keyval)
  {
    case GDK_Up:
    case GDK_Down:
      entry->buttonPressed = FALSE;
      break;
      
    default:
      break;
  }
   
  return GTK_WIDGET_CLASS(class)->key_release_event (widget, event);
}