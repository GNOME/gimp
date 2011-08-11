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

#define UNIT_ENTRY_STRING_LENGTH  30
#define UNIT_ENTRY_ERROR_TIMEOUT  2
#define UNIT_ENTRY_ERROR_COLOR    "LightSalmon"
#define UNIT_ENTRY_SIZE_REQUEST   200

typedef struct
{
  GimpUnitAdjustment *unit_adjustment; 

  /* flag set TRUE when entry's text should not be overwritten
     (i.e. during user input) */
  /* FIXME: maybe it's possible to block signals that trigger updates instead */
  gboolean            dont_update_text;

  /* input mode */
  GimpUnitEntryMode   mode;

  /* is our input valid? (for error indication) */
  gboolean            input_valid;
  /* the timer source which handles the error indication */
  GSource             *timer;
} GimpUnitEntryPrivate;

#define GIMP_UNIT_ENTRY_GET_PRIVATE(obj) \
  ((GimpUnitEntryPrivate *) ((GimpUnitEntry *) (obj))->private)

G_DEFINE_TYPE (GimpUnitEntry, gimp_unit_entry, GTK_TYPE_SPIN_BUTTON);

static gboolean gimp_unit_entry_parse           (GimpUnitEntry *unitEntry);

static gboolean gimp_unit_entry_output          (GtkSpinButton          *spin, 
                                                 gpointer                data);
static void     gimp_unit_entry_text_changed    (GtkEditable            *editable,
                                                 gpointer                user_data);
static gint     gimp_unit_entry_input           (GtkSpinButton          *spinbutton,
                                                 gpointer                arg1,
                                                 gpointer                user_data);
static void     gimp_unit_entry_populate_popup  (GtkEntry               *entry,
                                                 GtkMenu                *menu,
                                                 gpointer                user_data); 
static void     gimp_unit_entry_menu_item       (GtkWidget              *menu_item,
                                                 gpointer               *user_data);
static gboolean gimp_unit_entry_timer_callback  (GtkWidget              *entry);                                               

static void
gimp_unit_entry_init (GimpUnitEntry *unit_entry)
{ 
  GimpUnitEntryPrivate *private;

  unit_entry->private = G_TYPE_INSTANCE_GET_PRIVATE (unit_entry,
                                                     GIMP_TYPE_UNIT_ENTRY,
                                                     GimpUnitEntryPrivate);

  private = GIMP_UNIT_ENTRY_GET_PRIVATE (unit_entry);                                                   

  /* create and set our adjustment subclass */
  /*private->unit_adjustment = gimp_unit_adjustment_new ();
  gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (unit_entry), 
                                  GTK_ADJUSTMENT  (private->unit_adjustment));*/

  /* some default values */
  private->dont_update_text = FALSE;  
  private->mode             = GIMP_UNIT_ENTRY_MODE_UNIT;  
  private->input_valid      = TRUE; 
  private->timer            = NULL;                            

  /* connect signals */
  g_signal_connect (&unit_entry->parent_instance, 
                    "input",
                    G_CALLBACK(gimp_unit_entry_input), 
                    (gpointer) unit_entry);
  g_signal_connect (&unit_entry->parent_instance.entry,
                    "changed",
                    G_CALLBACK(gimp_unit_entry_text_changed), 
                    (gpointer) unit_entry);
  g_signal_connect (&unit_entry->parent_instance.entry,
                    "populate-popup",
                    G_CALLBACK(gimp_unit_entry_populate_popup), 
                    NULL);

  gtk_widget_set_size_request (GTK_WIDGET (unit_entry), UNIT_ENTRY_SIZE_REQUEST, -1);
}

static void
gimp_unit_entry_class_init (GimpUnitEntryClass *class)
{
  g_type_class_add_private (class, sizeof (GimpUnitEntryPrivate));
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
  GtkAdjustment *adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (entry));
  
  if (!GIMP_IS_UNIT_ADJUSTMENT (adjustment))
  {
    g_warning ("gimp_unit_entry_get_adjustment: GimpUnitEntry has no or invalid adjustment. "
               "Create GimpUnitAdjustment instance and set via gimp_unit_entry_set_adjustment() first.");
    return NULL;
  }

  return GIMP_UNIT_ADJUSTMENT (adjustment);
}

void                  
gimp_unit_entry_set_adjustment  (GimpUnitEntry      *entry,
                                 GimpUnitAdjustment *adjustment)
{
  gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (entry), 
                                  GTK_ADJUSTMENT  (adjustment));

  g_signal_connect (GTK_SPIN_BUTTON (entry), 
                    "output",
                    G_CALLBACK(gimp_unit_entry_output), 
                    (gpointer) adjustment);                                
}        

/* connect to another entry */
void 
gimp_unit_entry_connect (GimpUnitEntry *entry, 
                         GimpUnitEntry *target)
{
  gimp_unit_adjustment_follow_unit_of (gimp_unit_entry_get_adjustment (entry),
                                       gimp_unit_entry_get_adjustment (target));
}

/* read and parse entered text */
static gboolean
gimp_unit_entry_parse (GimpUnitEntry *entry)
{
  GimpUnitEntryPrivate *private         = GIMP_UNIT_ENTRY_GET_PRIVATE (entry);
  GimpUnitParserResult result; 
  gboolean             success;
  const gchar          *str             = gtk_entry_get_text (GTK_ENTRY (entry));
  GimpUnitAdjustment   *unit_adjustment = gimp_unit_entry_get_adjustment (entry);

  /* set resolution (important for correct calculation of px) */
  result.resolution = gimp_unit_adjustment_get_resolution (unit_adjustment);
  /* set unit (we want to use current unit if the user didn't enter one) */
  result.unit = gimp_unit_adjustment_get_unit (unit_adjustment);

  /* parse string of entry */
  success = gimp_unit_parser_parse (str, &result);

  if (success)
  {
    /* reset color */
    gtk_widget_modify_base (GTK_WIDGET (entry), GTK_STATE_NORMAL, NULL);
    private->input_valid = TRUE;

    /* set new unit */  
    if (result.unit != gimp_unit_adjustment_get_unit (unit_adjustment))
    {
      gimp_unit_adjustment_set_unit (gimp_unit_entry_get_adjustment (entry), result.unit);
    }

    /* set new value */
    if (result.value != gimp_unit_adjustment_get_value (gimp_unit_entry_get_adjustment (entry)))
    {
      /* result from parser is in inch, so convert to desired unit */
      result.value = gimp_units_to_pixels (result.value,
                                           GIMP_UNIT_INCH,
                                           gimp_unit_adjustment_get_resolution (unit_adjustment));
      result.value = gimp_pixels_to_units (result.value,
                                           gimp_unit_adjustment_get_unit (unit_adjustment), 
                                           gimp_unit_adjustment_get_resolution (unit_adjustment));

      gimp_unit_adjustment_set_value (gimp_unit_entry_get_adjustment (entry), result.value);
    }
  }
  else
  {
    private->input_valid = FALSE;
    return FALSE;
  }

  return TRUE;
}

/* format displayed text, displays "[value] [unit]" (gets called by GtkSpinButton) */
static gboolean 
gimp_unit_entry_output (GtkSpinButton *spin,
                        gpointer       data)
{
  GimpUnitAdjustment   *adj         = GIMP_UNIT_ADJUSTMENT (data);
  GimpUnitEntry        *entry       = GIMP_UNIT_ENTRY (spin); 
  GimpUnitEntryPrivate *private     = GIMP_UNIT_ENTRY_GET_PRIVATE (entry);
  gchar                *output      = NULL;
  gchar                 resolution_mode_output [UNIT_ENTRY_STRING_LENGTH];

  /* if updating disabled => return (user input must not be overwritten) */
  if (private->dont_update_text)
  {
    return TRUE;
  }
  
  /* set text of the entry */
  if (private->mode == GIMP_UNIT_ENTRY_MODE_UNIT)
  {
    output = gimp_unit_entry_to_string (entry);
    gtk_entry_set_text (GTK_ENTRY (entry), output);
    g_free (output);
  }
  else
  {
    snprintf (resolution_mode_output, UNIT_ENTRY_STRING_LENGTH, "%.1f px/%s", 
              gimp_unit_adjustment_get_value (adj),
              gimp_unit_get_abbreviation (gimp_unit_adjustment_get_unit (adj)));
    gtk_entry_set_text (GTK_ENTRY (entry), resolution_mode_output);
  }

  DEBUG (("on_output: %s\n", output);)

  return TRUE;
}

/* parse and process entered text (signal emmited from GtkEntry) */
static void 
gimp_unit_entry_text_changed (GtkEditable *editable, 
                              gpointer     user_data)
{
  GimpUnitEntry        *entry   = GIMP_UNIT_ENTRY (user_data);
  GimpUnitEntryPrivate *private = GIMP_UNIT_ENTRY_GET_PRIVATE (entry);

  DEBUG (("on_text_changed\n");)

  /* timer for error indication */
  if (gtk_widget_has_focus (GTK_WIDGET (entry)))
  {
    /* if timer exists, reset */
    if (private->timer != NULL)
    {
      g_source_destroy (private->timer);
    }
    /* create timer */
    private->timer = g_timeout_source_new_seconds (UNIT_ENTRY_ERROR_TIMEOUT);
    g_source_set_callback (private->timer, 
                           (GSourceFunc) gimp_unit_entry_timer_callback,
                           (gpointer) entry,
                           NULL);
    g_source_attach (private->timer, NULL);
  }

  if (!private->mode == GIMP_UNIT_ENTRY_MODE_RESOLUTION)
  {
    /* disable updating the displayed text (user input must not be overwriten) */
    private->dont_update_text = TRUE;
    /* parse input */
    gimp_unit_entry_parse (entry);
    /* reenable updating */
    private->dont_update_text = FALSE;
  }
}

static gint
gimp_unit_entry_input (GtkSpinButton *spin_button,
                       gpointer       arg1,
                       gpointer       user_data)
{
  GimpUnitEntry        *entry   = GIMP_UNIT_ENTRY (spin_button);
  GimpUnitEntryPrivate *private = GIMP_UNIT_ENTRY_GET_PRIVATE (entry);
  
  if (!private->mode == GIMP_UNIT_ENTRY_MODE_RESOLUTION)
  {
    /* parse and set value ourselves before GtkSpinButton does so, because
       GtkSpinButton would truncate our input and ignore everything which 
       is not a numbner */
    gimp_unit_entry_parse (GIMP_UNIT_ENTRY (spin_button));
    gimp_unit_entry_output (spin_button, (gpointer) gimp_unit_entry_get_adjustment (entry));
  }

  /* we want GtkSpinButton to handle the input nontheless (there is no problem anymore
     since we done the parsing), so we return FALSE */
  return FALSE;
}

static gboolean 
gimp_unit_entry_timer_callback  (GtkWidget *entry)                                   
{
  GimpUnitEntryPrivate *private = GIMP_UNIT_ENTRY_GET_PRIVATE (GIMP_UNIT_ENTRY (entry));

  /* paint entry red if input is invalid */
  if (!private->input_valid)
  {
    GdkColor color;
    gdk_color_parse (UNIT_ENTRY_ERROR_COLOR, &color);
    gtk_widget_modify_base (entry, GTK_STATE_NORMAL, &color);
  }

  /* delete timer */
  private->timer = NULL;

  return FALSE;
}

static void 
gimp_unit_entry_populate_popup (GtkEntry *entry,
                                GtkMenu  *menu,
                                gpointer  user_data)
{
  GimpUnitEntryPrivate *private = GIMP_UNIT_ENTRY_GET_PRIVATE (GIMP_UNIT_ENTRY (entry));
  GtkWidget *menu_item;
  gint       i                  = 0;
  gint       first_unit;

  menu_item = gtk_separator_menu_item_new ();
  gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);

  /* ignore PIXEL when in resolution mode */
  (private->mode == GIMP_UNIT_ENTRY_MODE_RESOLUTION) 
      ? (first_unit = 1) : (first_unit = 0); 

  /* populate popup with item for each available unit 
     (reversing iteration order to display commonly used units first) */
  for (i = gimp_unit_get_number_of_units() - 1; i >= first_unit; i--)
  {
    menu_item = gtk_menu_item_new_with_label (gimp_unit_get_singular (i));
    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);

    /* save corresponding unit in menu item */
    g_object_set_data (G_OBJECT (menu_item), "unit", GINT_TO_POINTER (i));

    g_signal_connect(menu_item, "activate",
                     (GCallback) gimp_unit_entry_menu_item, 
                     gimp_unit_entry_get_adjustment (GIMP_UNIT_ENTRY (entry)));
  }

  gtk_widget_show_all (GTK_WIDGET (menu));
}

static void
gimp_unit_entry_menu_item      (GtkWidget *menu_item,
                                gpointer  *user_data)
{
  GimpUnitAdjustment *adj   = GIMP_UNIT_ADJUSTMENT (user_data);
  GimpUnit            unit;
              
  /* get selected unit */
  unit = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menu_item), "unit"));

  /* change unit according to selected unit */
  gimp_unit_adjustment_set_unit (adj, unit);
}                        

/* convenience getters/setters */                         
gdouble
gimp_unit_entry_get_pixels (GimpUnitEntry *entry)
{
  GimpUnitAdjustment *adj = gimp_unit_entry_get_adjustment (entry);

  return gimp_unit_adjustment_get_value_in_unit (adj, GIMP_UNIT_PIXEL);
}

void
gimp_unit_entry_set_bounds (GimpUnitEntry *entry, 
                            GimpUnit       unit, 
                            gdouble        lower,
                            gdouble        upper)
{
  GimpUnitAdjustment *adj = gimp_unit_entry_get_adjustment (entry);
  gimp_unit_adjustment_set_bounds (adj, unit, lower, upper);
}

void
gimp_unit_entry_set_pixels (GimpUnitEntry      *entry,
                            gdouble             value)
{
  GimpUnitAdjustment *adj = gimp_unit_entry_get_adjustment (entry);
  gimp_unit_adjustment_set_value_in_unit (adj, value, GIMP_UNIT_PIXEL);
}                           

void
gimp_unit_entry_set_mode (GimpUnitEntry     *entry,
                          GimpUnitEntryMode  mode)
{
  GimpUnitEntryPrivate *private = GIMP_UNIT_ENTRY_GET_PRIVATE (entry);
  GimpUnitAdjustment   *adj     = gimp_unit_entry_get_adjustment (entry);

  private->mode = mode;

  /* set resolution in resolution mode to 1, otherwise calculation is not correct */
  if (mode == GIMP_UNIT_ENTRY_MODE_RESOLUTION)
  {
    gimp_unit_adjustment_set_resolution (adj, 1.0);
  }
}

/* get string in format "value unit" */
gchar* 
gimp_unit_entry_to_string (GimpUnitEntry *entry)
{
  return gimp_unit_entry_to_string_in_unit (
           entry, 
           gimp_unit_adjustment_get_unit (gimp_unit_entry_get_adjustment (entry)));
}

gchar*  
gimp_unit_entry_to_string_in_unit (GimpUnitEntry *entry, 
                                   GimpUnit       unit)
{
  gdouble value;
  gchar *text = g_malloc (sizeof (gchar) * UNIT_ENTRY_STRING_LENGTH);

  value = gimp_unit_adjustment_get_value_in_unit (gimp_unit_entry_get_adjustment (entry),
                                                  unit);

  g_snprintf (text, UNIT_ENTRY_STRING_LENGTH, "%.*f %s", 
             gimp_unit_get_digits (unit),
             value,
             gimp_unit_get_abbreviation (unit));

  return text;
}       
