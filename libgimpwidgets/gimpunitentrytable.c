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

#include <gtk/gtk.h>
#include <glib/gprintf.h>

#include "gimpunitentrytable.h"
#include "gimpwidgets.h"

/* debug macro */
//#define UNITENTRY_DEBUG
#ifdef  UNITENTRY_DEBUG
#define DEBUG(x) g_debug x 
#else
#define DEBUG(x) /* nothing */
#endif

G_DEFINE_TYPE (GimpUnitEntryTable, gimp_unit_entry_table, G_TYPE_OBJECT);

/**
 * signal handler
 **/
static void label_updater     (GtkAdjustment *adj, gpointer userData);
static void on_value_changed  (GtkAdjustment *adj, gpointer userData);

static void
gimp_unit_entry_table_init (GimpUnitEntryTable *table)
{
   /* initialize our fields */
  table->table        = gtk_table_new (1, 1, FALSE);
  table->entries      = NULL;
  table->bottom       = 0;
}

static void
gimp_unit_entry_table_class_init (GimpUnitEntryTableClass *klass)
{
  klass->sig_value_changed_id = g_signal_new ("value-changed",
                                              GIMP_TYPE_UNIT_ENTRY_TABLE,
                                              G_SIGNAL_RUN_LAST,
                                              0,
                                              NULL, 
                                              NULL,
                                              g_cclosure_marshal_VOID__OBJECT,
                                              G_TYPE_NONE, 
                                              1, 
                                              G_TYPE_OBJECT);
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
  gint i;
  /* position of the entry */
  gint leftAttach   = 1,
       rightAttach  = 2,
       topAttach    = table->bottom,
       bottomAttach = table->bottom+1;

  /* remember position in table so that we later can place other widgets around it */
  g_object_set_data (G_OBJECT (entry), "leftAttach", GINT_TO_POINTER (leftAttach));
  g_object_set_data (G_OBJECT (entry), "rightAttach", GINT_TO_POINTER (rightAttach));
  g_object_set_data (G_OBJECT (entry), "topAttach", GINT_TO_POINTER (topAttach));
  g_object_set_data (G_OBJECT (entry), "bottomAttach", GINT_TO_POINTER (bottomAttach));

  /* add entry to table at position (1, count) */
  gtk_table_attach_defaults (GTK_TABLE (table->table),
                             GTK_WIDGET (entry), 
                             leftAttach,
                             rightAttach,
                             topAttach,
                             bottomAttach);

  table->bottom++;

  /* if label string is given, create label and attach to the left of entry */
  if (labelStr != NULL)
  {
    label = gtk_label_new (labelStr);
    gtk_table_attach (GTK_TABLE (table->table),
                      label,
                      leftAttach-1 , leftAttach, topAttach, bottomAttach,
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
  
  /* connect to ourselves */
  g_signal_connect (gimp_unit_entry_get_adjustment (entry), "value-changed",
                    G_CALLBACK (on_value_changed), (gpointer) table);

  gtk_widget_show_all (table->table); 

  table->entries = g_list_append (table->entries, (gpointer) entry);

  return GTK_WIDGET (entry);
}

/* add preview label showing value of the two given entries in given unit */
void 
gimp_unit_entry_table_add_label (GimpUnitEntryTable *table,
                                 GimpUnit unit,
                                 const char* id1,
                                 const char* id2)
{
  GtkWidget     *label = gtk_label_new("preview");
  GimpUnitEntry *entry1 = gimp_unit_entry_table_get_entry (table, id1);
  GimpUnitEntry *entry2 = gimp_unit_entry_table_get_entry (table, id2);
  gint          leftAttach, rightAttach, topAttach, bottomAttach;

  /* save unit */
  g_object_set_data (G_OBJECT (label), "unit", GINT_TO_POINTER (unit));
  /* save the two entries */
  g_object_set_data (G_OBJECT (label), "entry1", (gpointer) entry1);
  g_object_set_data (G_OBJECT (label), "entry2", (gpointer) entry2);

  /* get the position of the entries and set label accordingly */
  leftAttach   = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry1), "leftAttach"));
  rightAttach  = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry1), "rightAttach"));
  topAttach    = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry2), "bottomAttach"));
  bottomAttach = topAttach + 1;

  /* add label below unit entries */
  gtk_table_attach (GTK_TABLE (table->table),
                    label,
                    leftAttach, rightAttach, 
                    topAttach, bottomAttach,
                    GTK_FILL, GTK_SHRINK,
                    10, 0);

  /* set alignment */
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  /*gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                             -1);*/

  table->bottom++;

  gtk_widget_show (GTK_WIDGET (label));

  /* connect label updater to changed signal */
  g_signal_connect (G_OBJECT (gimp_unit_entry_get_adjustment (entry1)), "value-changed",
                    G_CALLBACK (label_updater), (gpointer) label);

  g_signal_connect (G_OBJECT (gimp_unit_entry_get_adjustment (entry2)), "value-changed",
                    G_CALLBACK (label_updater), (gpointer) label);

  label_updater (NULL, (gpointer) label);
}

/* add chain button connecting the two given UnitEntries */
GtkWidget* 
gimp_unit_entry_table_add_chain_button (GimpUnitEntryTable *table,
                                        const char* id1,
                                        const char* id2)
{
  GtkWidget        *chainButton = gimp_chain_button_new(GIMP_CHAIN_RIGHT);
  GimpUnitEntry    *entry1      = gimp_unit_entry_table_get_entry (table, id1);
  GimpUnitEntry    *entry2      = gimp_unit_entry_table_get_entry (table, id2);

  /* retrieve position of entries */
  gint rightAttach  = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry1), "rightAttach"));
  gint topAttach    = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry1), "topAttach"));
  gint bottomAttach = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry2), "bottomAttach"));

  /* add chainbutton to right of entries, spanning from the first to the second */
  gtk_table_attach (GTK_TABLE (table->table),
                             GTK_WIDGET (chainButton),
                             rightAttach,
                             rightAttach + 1,
                             topAttach,
                             bottomAttach,
                             GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chainButton), TRUE);

  gtk_widget_show (chainButton);                          

  return chainButton;
}

/* get UnitEntry by identifier */
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
static 
void label_updater (GtkAdjustment *adj, gpointer userData)
{
  gchar               str[40];
  GtkLabel           *label       = GTK_LABEL (userData);
  GimpUnitEntry      *entry1      = GIMP_UNIT_ENTRY (g_object_get_data (G_OBJECT (label), "entry1"));
  GimpUnitEntry      *entry2      = GIMP_UNIT_ENTRY (g_object_get_data (G_OBJECT (label), "entry2"));
  GimpUnitAdjustment *adjustment;   
  GimpUnit           unit;

  /* retrieve preview unit */
  unit = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (label), "unit"));

  /* retrieve values of the two entries */
  adjustment = gimp_unit_entry_get_adjustment (entry1);
  g_sprintf (str, "%s", gimp_unit_adjustment_to_string_in_unit (adjustment, unit));

  adjustment = gimp_unit_entry_get_adjustment (entry2);
  g_sprintf (str, "%s x %s ", str, gimp_unit_adjustment_to_string_in_unit (adjustment, unit));

  gtk_label_set_text (label, str);
}

/* signal handler for value changed signal of one of our entries/adjustments */
static 
void on_value_changed  (GtkAdjustment *adj, gpointer userData)
{
  GimpUnitEntryTable *table = GIMP_UNIT_ENTRY_TABLE (userData);
  GimpUnitEntry *entry;
  gint i, count = g_list_length (table->entries);

  /* find corresponding entry */
  for (i = 0; i < count; i++) 
  {
    entry = gimp_unit_entry_table_get_nth_entry (table, i);
    if (gimp_unit_entry_get_adjustment (entry) == GIMP_UNIT_ADJUSTMENT (adj))
      i = count;
  }

  /* emit "value-changed" */
  g_signal_emit(table, GIMP_UNIT_ENTRY_TABLE_GET_CLASS(table)->sig_value_changed_id, 0, entry);
}

/* get count of attached unit entries */
gint
gimp_unit_entry_table_get_entry_count (GimpUnitEntryTable *table)
{
  return g_list_length (table->entries);
}

/* sets the unit of all entries */
void 
gimp_unit_entry_table_set_unit (GimpUnitEntryTable *table, GimpUnit unit)
{
  GimpUnitEntry *entry;
  gint i, count = g_list_length (table->entries);

  /* iterate over list of entries */
  for (i = 0; i < count; i++) 
  {
    entry = gimp_unit_entry_table_get_nth_entry (table, i);
    gimp_unit_entry_set_unit (entry, unit);
  }
}

/* sets the resolution of all entries */
void 
gimp_unit_entry_table_set_resolution (GimpUnitEntryTable *table, gdouble res)
{
  GimpUnitEntry *entry;
  gint i, count = g_list_length (table->entries);

  /* iterate over list of entries */
  for (i = 0; i < count; i++) 
  {
    entry = gimp_unit_entry_table_get_nth_entry (table, i);
    gimp_unit_entry_set_resolution (entry, res);
  }
}
