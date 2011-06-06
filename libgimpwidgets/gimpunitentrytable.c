/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunitentrytable.c
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

#include "gimpunitentrytable.h"

#include <gtk/gtklabel.h>
#include <glib/gprintf.h>

/* debug macro */
//#define UNITENTRY_DEBUG
#ifdef  UNITENTRY_DEBUG
#define DEBUG(x) g_debug x 
#else
#define DEBUG(x) /* nothing */
#endif

#define DEFAULT_TABLE_SIZE_X 3
#define DEFAULT_TABLE_SIZE_Y 3

G_DEFINE_TYPE (GimpUnitEntryTable, gimp_unit_entry_table, G_TYPE_OBJECT);

/**
 * signal handler
 **/
static void label_updater (GtkAdjustment *adj, gpointer userData);

static void
gimp_unit_entry_table_init (GimpUnitEntryTable *table)
{
   /* initialize our fields */
  table->table        = gtk_table_new (DEFAULT_TABLE_SIZE_X, DEFAULT_TABLE_SIZE_Y, FALSE);
  table->entries      = NULL;
  table->previewLabel = NULL;
}

static void
gimp_unit_entry_table_class_init (GimpUnitEntryTableClass *class)
{
}

GObject*
gimp_unit_entry_table_new (void)
{
  GObject *table;

  table = g_object_new (GIMP_TYPE_UNIT_ENTRY_TABLE, NULL);

  return table;
}

/* add an UnitEntry */
/* TODO: have options for default and custom layout */
GtkWidget* 
gimp_unit_entry_table_add_entry (GimpUnitEntryTable *table,
                                 const gchar *id,
                                 const gchar *labelStr)
{
  GimpUnitEntry *entry = GIMP_UNIT_ENTRY (gimp_unit_entry_new (id)) , *entry2; 
  gint          count  = g_list_length (table->entries); 
  GtkWidget     *label;
  int i;
   
  /* add entry to table at position (1, count) */
  gtk_table_attach_defaults (GTK_TABLE (table->table),
                             GTK_WIDGET (entry), 
                             1, 2, count, count + 1);

  /* if label is given, create label and attach to the left of entry */
  if (labelStr != NULL)
  {
    label = gtk_label_new (labelStr);
    gtk_table_attach (GTK_TABLE (table->table),
                      label,
                      0, 1, count, count + 1,
                      GTK_SHRINK, GTK_EXPAND | GTK_FILL,
                      10, 0);
  }

  /* connect entry to others */
  for (i = 0; i < count; i++)
  {
    entry2 = gimp_unit_entry_table_get_nth_entry (table, i);
    gimp_unit_entry_connect (GIMP_UNIT_ENTRY (entry), GIMP_UNIT_ENTRY (entry2));
    gimp_unit_entry_connect (GIMP_UNIT_ENTRY (entry2), GIMP_UNIT_ENTRY (entry));
  }
  
  gtk_widget_show_all (table->table); 

  table->entries = g_list_append (table->entries, (gpointer) entry);

  return GTK_WIDGET (entry);
}

/* add preview label showing the current value in given unit */
/** TODO: The whole label thing is subject to change. 
 *        Just a quick'n dirty solution for now 
 **/
void 
gimp_unit_entry_table_add_label (GimpUnitEntryTable *table, GimpUnit unit)
{
  GtkWidget     *label = gtk_label_new("preview");
  gint           count  = g_list_length (table->entries);
  gint           i;
  GimpUnitEntry *entry;

  /* save unit */
  table->previewUnit = unit;

  /* add label below unit entries */
  gtk_table_attach (GTK_TABLE (table->table),
                    label,
                    1, 2, 
                    count, count+1,
                    GTK_SHRINK, GTK_EXPAND | GTK_FILL,
                    10, 0);

  table->previewLabel = GTK_LABEL (label);
  gtk_widget_show (GTK_WIDGET (table->previewLabel));

  /* connect label updater to changed signal */
  for (i = 0; i < count; i++)
  {
    entry = gimp_unit_entry_table_get_nth_entry (table, i);
    g_signal_connect (G_OBJECT (gimp_unit_entry_get_adjustment (entry)), "value-changed",
                      G_CALLBACK (label_updater), (gpointer) table);
  }

  label_updater (NULL, (gpointer) table);
}

/* get UnitEntry by label */
GimpUnitEntry* 
gimp_unit_entry_table_get_entry (GimpUnitEntryTable *table,
                                 const gchar *id)
{
  GimpUnitEntry *entry;
  gint i, count = g_list_length (table->entries);

  /* iterate over list to find our entry */
  for (i = 0; i < count; i++) 
  {
    entry = gimp_unit_entry_table_get_nth_entry (table, i);
    if (g_strcmp0 (gimp_unit_entry_get_id (entry), id) == 0)
      return entry;
  }
  g_warning ("gimp_unit_entry_table_get_entry: entry with id '%s' does not exist", id);
  return NULL;
}

/* get UnitEntry by index */
GimpUnitEntry* 
gimp_unit_entry_table_get_nth_entry (GimpUnitEntryTable *table, 
                                     gint index)
{
  if (g_list_length (table->entries) <= index)
  {
    return NULL;
  }

  return GIMP_UNIT_ENTRY (g_list_nth (table->entries, index)->data);
}

/* updates the text of the preview label */
/** TODO: The whole label thing is subject to change. 
 *        Just a quick'n dirty solution for now 
 **/
static 
void label_updater (GtkAdjustment *adj, gpointer userData)
{
  gchar               str[40];
  GimpUnitEntryTable *table       = GIMP_UNIT_ENTRY_TABLE (userData);
  GimpUnitAdjustment *adjustment;
  gint                count       = g_list_length (table->entries);
  gint                i           = 0;
  GimpUnit            *unit;       

  if (table->previewLabel == NULL || count <= 0)
    return;

  adjustment = gimp_unit_entry_get_adjustment (gimp_unit_entry_table_get_nth_entry (table, 0));
  g_sprintf (str, "%s", gimp_unit_adjustment_to_string_in_unit (adjustment, table->previewUnit));

  for (i = 1; i < count; i++)
  {
    adjustment = gimp_unit_entry_get_adjustment (gimp_unit_entry_table_get_nth_entry (table, i));
    g_sprintf (str, "%s x %s ", str, gimp_unit_adjustment_to_string_in_unit (adjustment, table->previewUnit));
  }

  gtk_label_set_text (table->previewLabel, str); 
}
