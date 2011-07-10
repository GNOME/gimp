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

#include "gimpunitparser.h"
#include "gimpunitentry.h"
#include "gimpunitadjustment.h"

/* debug macro */
//#define UNITENTRY_DEBUG
#ifdef  UNITENTRY_DEBUG
#define DEBUG(x) g_debug x 
#else
#define DEBUG(x) /* nothing */
#endif

G_DEFINE_TYPE (GimpUnitEntry, gimp_unit_entry, GTK_TYPE_SPIN_BUTTON);

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
static gboolean on_output     (GtkSpinButton      *spin, 
                               gpointer           data);
/* parse and process entered text (signal emmited from GtkEntry) */
static void on_text_changed   (GtkEditable        *editable,
                               gpointer           user_data);
static void on_insert_text    (GtkEditable *editable,
                               gchar *new_text,
                               gint new_text_length,
                               gint *position,
                               gpointer user_data);
static gint on_input          (GtkSpinButton *spinbutton,
                               gpointer       arg1,
                               gpointer       user_data);
static void on_populate_popup (GtkEntry *entry,
                               GtkMenu  *menu,
                               gpointer  user_data); 
static void on_menu_item      (GtkWidget *menuItem,
                               gpointer *user_data);

static void
gimp_unit_entry_init (GimpUnitEntry *unitEntry)
{ 
  /* create and set our adjustment subclass */
  GObject *adjustment = gimp_unit_adjustment_new ();

  unitEntry->unitAdjustment = GIMP_UNIT_ADJUSTMENT (adjustment);
  gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (unitEntry), 
                                  GTK_ADJUSTMENT (adjustment));

  /* some default values */
  unitEntry->dontUpdateText = FALSE;  
  unitEntry->mode           = GIMP_UNIT_ENTRY_MODE_UNIT;                               

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
  g_signal_connect (&unitEntry->parent_instance.entry,
                    "populate-popup",
                    G_CALLBACK(on_populate_popup), 
                    NULL);
}

static void
gimp_unit_entry_class_init (GimpUnitEntryClass *class)
{
  GtkWidgetClass   *widgetClass = GTK_WIDGET_CLASS (class);

  /* some events we need to catch before our parent */
  /* FIXME: hopefully we don't need them anymore with the latest changes... */
  widgetClass->button_press_event   = gimp_unit_entry_button_press;
  widgetClass->button_release_event = gimp_unit_entry_button_release;
  widgetClass->scroll_event         = gimp_unit_entry_scroll;
  widgetClass->key_press_event      = gimp_unit_entry_key_press;
  widgetClass->key_release_event    = gimp_unit_entry_key_release;
}

GtkWidget*
gimp_unit_entry_new (void)
{
  GtkWidget *entry = g_object_new (GIMP_TYPE_UNIT_ENTRY, NULL);

  return entry;
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
  GimpUnitParserResult result; 
  gboolean             success;
  const gchar          *str = gtk_entry_get_text (GTK_ENTRY (entry));

  /* set resolution (important for correct calculation of px) */
  result.resolution = entry->unitAdjustment->resolution;
  /* set unit (we want to use current unit if the user didn't enter one) */
  result.unit = entry->unitAdjustment->unit;

  /* parse string of entry */
  success = gimp_unit_parser_parse (str, &result);

  if (!success)
  {
    /* paint entry red */
    GdkColor color;
    gdk_color_parse ("LightSalmon", &color);
    gtk_widget_modify_base (GTK_WIDGET (entry), GTK_STATE_NORMAL, &color);

    return FALSE;
  }
  else
  {
    /* reset color */
    gtk_widget_modify_base (GTK_WIDGET (entry), GTK_STATE_NORMAL, NULL);

    /* set new unit */  
    if (result.unit != entry->unitAdjustment->unit)
    {
      gimp_unit_adjustment_set_unit (entry->unitAdjustment, result.unit);
    }

    /* set new value */
    if (gimp_unit_adjustment_get_value (entry->unitAdjustment) != result.value)
    {
      /* result from parser is in inch, so convert to desired unit */
      result.value = gimp_units_to_pixels (result.value,
                                            GIMP_UNIT_INCH,
                                            entry->unitAdjustment->resolution);
      result.value = gimp_pixels_to_units (result.value,
                                            entry->unitAdjustment->unit, 
                                            entry->unitAdjustment->resolution);

      gimp_unit_adjustment_set_value (entry->unitAdjustment, result.value);

      //g_object_notify (G_OBJECT ( GTK_SPIN_BUTTON (entry)), "value");
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

  /* if updating disabled => return (user input must not be overwritten) */
  if (entry->dontUpdateText)
  {
    return TRUE;
  }
  
  /* set text of the entry */
  if (entry->mode == GIMP_UNIT_ENTRY_MODE_UNIT)
  {
    text = gimp_unit_adjustment_to_string (adj);
  }
  else
  {
    text = g_malloc (30 * sizeof (gchar));
    sprintf (text, "%.1f px/%s", 
             gimp_unit_adjustment_get_value (adj),
             gimp_unit_get_abbreviation (gimp_unit_adjustment_get_unit (adj)));
  }

  DEBUG (("on_output: %s\n", text);)

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
  DEBUG (("on_insert_text\n");)
}

/* parse and process entered text (signal emmited from GtkEntry) */
static 
void on_text_changed (GtkEditable *editable, gpointer user_data)
{
  GimpUnitEntry *entry = GIMP_UNIT_ENTRY (user_data);

  DEBUG (("on_text_changed\n");)

  if (!entry->mode == GIMP_UNIT_ENTRY_MODE_RESOLUTION)
  {
    /* disable updating the displayed text (user input must not be overwriten) */
    entry->dontUpdateText = TRUE;
    /* parse input */
    gimp_unit_entry_parse (entry);
    /* reenable updating */
    entry->dontUpdateText = FALSE;
  }
}

static 
gint on_input        (GtkSpinButton *spinButton,
                      gpointer       arg1,
                      gpointer       user_data)
{
  if (!GIMP_UNIT_ENTRY (spinButton)->mode == GIMP_UNIT_ENTRY_MODE_RESOLUTION)
  {
    /* parse and set value ourselves before GtkSpinButton does so, because
       GtkSpinButton would truncate our input and ignore parts of it */
    gimp_unit_entry_parse (GIMP_UNIT_ENTRY (spinButton));
    on_output (spinButton, (gpointer)GIMP_UNIT_ENTRY(spinButton)->unitAdjustment);
  }

  /* we want GtkSpinButton to handle the input nontheless (there is no problem anymore
     since we done the parsing), so we return FALSE */
  return FALSE;
}

static 
void on_populate_popup (GtkEntry *entry,
                        GtkMenu  *menu,
                        gpointer  user_data)
{
  GtkWidget *menuItem;
  int       i = 0;

  /* populate popup with item for each available unit */
  menuItem = gtk_separator_menu_item_new ();
  gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuItem);

  /* ignore PIXEL when in resolution mode */
  (GIMP_UNIT_ENTRY (entry)->mode == GIMP_UNIT_ENTRY_MODE_RESOLUTION) ? (i = 1) : (i = 0); 

  for (; i < gimp_unit_get_number_of_units(); i++)
  {
    menuItem = gtk_menu_item_new_with_label (gimp_unit_get_singular (i));
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuItem);

    /* save corresponding unit in menu item */
    g_object_set_data (G_OBJECT (menuItem), "unit", GINT_TO_POINTER (i));

    g_signal_connect(menuItem, "activate",
                     (GCallback) on_menu_item, 
                     gimp_unit_entry_get_adjustment (GIMP_UNIT_ENTRY (entry)));
  }

  menuItem = gtk_menu_item_new_with_label ("Set Unit:");
  gtk_widget_set_sensitive (menuItem, FALSE);
  gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menuItem);

  gtk_widget_show_all (GTK_WIDGET (menu));
}

static 
void on_menu_item      (GtkWidget *menuItem,
                        gpointer *user_data)
{
  GimpUnitAdjustment *adj   = GIMP_UNIT_ADJUSTMENT (user_data);
  GimpUnit            unit;
  const gchar        *label = gtk_menu_item_get_label (GTK_MENU_ITEM (menuItem));
              
  /* get selected unit */
  unit = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menuItem), "unit"));

  /* change unit according to selected unit */
  gimp_unit_adjustment_set_unit (adj, unit);
}                        


static gint 
gimp_unit_entry_focus_out (GtkWidget          *widget,
                           GdkEventFocus      *event)
{
  GtkEntryClass *class = GTK_ENTRY_CLASS (gimp_unit_entry_parent_class);

  return GTK_WIDGET_CLASS (class)->focus_out_event (widget, event);
}

static gint
gimp_unit_entry_button_press (GtkWidget          *widget,
                              GdkEventButton     *event)
{
  GtkSpinButtonClass *class = GTK_SPIN_BUTTON_CLASS (gimp_unit_entry_parent_class);
  GimpUnitEntry      *entry = GIMP_UNIT_ENTRY (widget);

  return GTK_WIDGET_CLASS(class)->button_press_event (widget, event);
}
static gint
gimp_unit_entry_button_release (GtkWidget          *widget,
                                GdkEventButton     *event)
{
  GtkSpinButtonClass *class = GTK_SPIN_BUTTON_CLASS (gimp_unit_entry_parent_class);
  GimpUnitEntry      *entry = GIMP_UNIT_ENTRY (widget);
   
  return GTK_WIDGET_CLASS(class)->button_release_event (widget, event);
}

static gint
gimp_unit_entry_scroll (GtkWidget          *widget,
                        GdkEventScroll     *event)
{
  GtkSpinButtonClass *class = GTK_SPIN_BUTTON_CLASS (gimp_unit_entry_parent_class);
  GimpUnitEntry      *entry = GIMP_UNIT_ENTRY (widget);

  return GTK_WIDGET_CLASS(class)->scroll_event (widget, event);
}

static gint 
gimp_unit_entry_key_press (GtkWidget          *widget,
                           GdkEventKey        *event)
{
  GtkSpinButtonClass *class = GTK_SPIN_BUTTON_CLASS (gimp_unit_entry_parent_class);
  GimpUnitEntry      *entry = GIMP_UNIT_ENTRY (widget);
   
  return GTK_WIDGET_CLASS(class)->key_press_event (widget, event);
}
static gint 
gimp_unit_entry_key_release (GtkWidget          *widget,
                             GdkEventKey        *event)
{
  GtkSpinButtonClass *class = GTK_SPIN_BUTTON_CLASS (gimp_unit_entry_parent_class);
  GimpUnitEntry      *entry = GIMP_UNIT_ENTRY (widget);
   
  return GTK_WIDGET_CLASS(class)->key_release_event (widget, event);
}

/* convenience getters/setters */
void 
gimp_unit_entry_set_unit (GimpUnitEntry *entry, GimpUnit unit)
{
  GimpUnitAdjustment *adj = gimp_unit_entry_get_adjustment (entry);
  gimp_unit_adjustment_set_unit (adj, unit);
}
void 
gimp_unit_entry_set_resolution (GimpUnitEntry *entry, gdouble resolution)
{
  GimpUnitAdjustment *adj = gimp_unit_entry_get_adjustment (entry);
  gimp_unit_adjustment_set_resolution (adj, resolution);
}
void 
gimp_unit_entry_set_value (GimpUnitEntry *entry, gdouble value)
{
  GimpUnitAdjustment *adj = gimp_unit_entry_get_adjustment (entry);
  gimp_unit_adjustment_set_value (adj, value);
}
gdouble 
gimp_unit_entry_get_value (GimpUnitEntry *entry)
{
  GimpUnitAdjustment *adj = gimp_unit_entry_get_adjustment (entry);
  return gimp_unit_adjustment_get_value (adj);
}
gdouble
gimp_unit_entry_get_value_in_unit (GimpUnitEntry *entry, GimpUnit unit)
{
  GimpUnitAdjustment *adj = gimp_unit_entry_get_adjustment (entry);
  return gimp_unit_adjustment_get_value_in_unit (adj, unit);
}
void 
gimp_unit_entry_set_value_in_unit (GimpUnitEntry *entry,
                                   gdouble value, 
                                   GimpUnit unit)
{
  GimpUnitAdjustment *adj = gimp_unit_entry_get_adjustment (entry);
  gimp_unit_adjustment_set_value_in_unit (adj, value, unit);
}
GimpUnit 
gimp_unit_entry_get_unit (GimpUnitEntry *entry)
{
  GimpUnitAdjustment *adj = gimp_unit_entry_get_adjustment (entry);
  return gimp_unit_adjustment_get_unit (adj);
}
void
gimp_unit_entry_set_bounds (GimpUnitEntry *entry, GimpUnit unit, gdouble upper, gdouble lower)
{
  GimpUnitAdjustment *adj = gimp_unit_entry_get_adjustment (entry);
  gimp_unit_adjustment_set_bounds (adj, unit, upper, lower);
}

void
gimp_unit_entry_set_mode (GimpUnitEntry     *entry,
                          GimpUnitEntryMode  mode)
{
  entry->mode = mode;
}
