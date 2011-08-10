/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunitentries.c
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

#include "gimpunitentries.h"
#include "gimpwidgets.h"

/* debug macro */
//#define UNITENTRY_DEBUG
#ifdef  UNITENTRY_DEBUG
#define DEBUG(x) g_debug x 
#else
#define DEBUG(x) /* nothing */
#endif

#define UNIT_ENTRIES_LEFT_ATTACH  2
#define UNIT_ENTRIES_RIGHT_ATTACH 3

enum
{
  CHANGED,
  LAST_SIGNAL
};

typedef struct
{
  GtkWidget  *table;
  GtkWidget  *chain_button;
  GHashTable *entries_store;
} GimpUnitEntriesPrivate;

#define GIMP_UNIT_ENTRIES_GET_PRIVATE(obj) \
  ((GimpUnitEntriesPrivate *) ((GimpUnitEntries *) (obj))->private)

G_DEFINE_TYPE (GimpUnitEntries, gimp_unit_entries, G_TYPE_OBJECT);

static void gimp_unit_entries_label_updater  (GtkAdjustment *adj, gpointer user_data);
static void gimp_unit_entries_entry_changed  (GtkAdjustment *adj, gpointer user_data);

static guint unit_entries_signals[LAST_SIGNAL] = {0};

static void
gimp_unit_entries_init (GimpUnitEntries *entries)
{
  GimpUnitEntriesPrivate *private;

  entries->private = G_TYPE_INSTANCE_GET_PRIVATE (entries,
                                                  GIMP_TYPE_UNIT_ENTRIES,
                                                  GimpUnitEntriesPrivate);

  private = GIMP_UNIT_ENTRIES_GET_PRIVATE (entries);   

  private->table          = gtk_table_new     (1, 1, FALSE);
  private->entries_store  = g_hash_table_new  (g_str_hash, g_str_equal);
  private->chain_button   = NULL;
}

static void
gimp_unit_entries_class_init (GimpUnitEntriesClass *klass)
{
  g_type_class_add_private (klass, sizeof (GimpUnitEntriesPrivate));

  unit_entries_signals[CHANGED] = 
    g_signal_new ("changed",
                  GIMP_TYPE_UNIT_ENTRIES,
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 
                  1, 
                  G_TYPE_OBJECT);
}

GimpUnitEntries*
gimp_unit_entries_new (void)
{
  GObject *entries;

  entries = g_object_new (GIMP_TYPE_UNIT_ENTRIES, NULL);

  return GIMP_UNIT_ENTRIES (entries);
}

/* add an UnitEntry */
GtkWidget* 
gimp_unit_entries_add_entry (GimpUnitEntries *entries,
                             const gchar     *id,
                             const gchar     *label_str)
{
  GimpUnitEntriesPrivate  *private = GIMP_UNIT_ENTRIES_GET_PRIVATE (entries);
  GimpUnitEntry           *entry   = GIMP_UNIT_ENTRY (gimp_unit_entry_new ()); 
  GimpUnitEntry           *other_entry;
  GtkWidget               *label;
  guint                   bottom;
  gint                    i;
  gint                    left_attach,
                          right_attach,
                          top_attach,
                          bottom_attach;

  /* retrieve bottom row of table */
  gtk_table_get_size (GTK_TABLE (gimp_unit_entries_get_table (entries)), &bottom, NULL);

  /* position of the entry (leave one row/column empty for labels etc) */
  left_attach   = UNIT_ENTRIES_LEFT_ATTACH,
  right_attach  = UNIT_ENTRIES_RIGHT_ATTACH,
  top_attach    = bottom,
  bottom_attach = bottom + 1;

  /* remember position in table so that we later can place other widgets around it */
  g_object_set_data (G_OBJECT (entry), "left_attach", GINT_TO_POINTER (left_attach));
  g_object_set_data (G_OBJECT (entry), "right_attach", GINT_TO_POINTER (right_attach));
  g_object_set_data (G_OBJECT (entry), "top_attach", GINT_TO_POINTER (top_attach));
  g_object_set_data (G_OBJECT (entry), "bottom_attach", GINT_TO_POINTER (bottom_attach));

  /* add entry to table at position (1, count) */
  gtk_table_attach_defaults (GTK_TABLE (gimp_unit_entries_get_table (entries)),
                             GTK_WIDGET (entry), 
                             left_attach,
                             right_attach,
                             top_attach,
                             bottom_attach);

  /* if label string is given, create label and attach to the left of entry */
  if (label_str != NULL)
  {
    label = gtk_label_new (label_str);
    gtk_table_attach (GTK_TABLE (gimp_unit_entries_get_table (entries)),
                      label,
                      left_attach-1 , left_attach, top_attach, bottom_attach,
                      GTK_SHRINK, GTK_EXPAND | GTK_FILL,
                      10, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  }
  
  /* connect to ourselves */
  g_signal_connect (gimp_unit_entry_get_adjustment (entry), "value-changed",
                    G_CALLBACK (gimp_unit_entries_entry_changed), (gpointer) entries);
  g_signal_connect (gimp_unit_entry_get_adjustment (entry), "resolution-changed",
                    G_CALLBACK (gimp_unit_entries_entry_changed), (gpointer) entries); 
                    
  /* connect entry to others */
  for (i = 0; i < g_hash_table_size (private->entries_store); i++)
  {
    other_entry = gimp_unit_entries_get_nth_entry (entries, i);
    gimp_unit_entry_connect (GIMP_UNIT_ENTRY (entry), GIMP_UNIT_ENTRY (other_entry));
    gimp_unit_entry_connect (GIMP_UNIT_ENTRY (other_entry), GIMP_UNIT_ENTRY (entry));
  }                      

  gtk_widget_show_all (gimp_unit_entries_get_table (entries)); 

  g_hash_table_insert (private->entries_store, (gpointer) id, (gpointer) entry);

  return GTK_WIDGET (entry);
}

/* add preview label showing value of the two given entries in given unit */
void 
gimp_unit_entries_add_preview_label (GimpUnitEntries *entries,
                                     GimpUnit         unit,
                                     const gchar     *id1,
                                     const gchar     *id2)
{
  GtkWidget     *label  = gtk_label_new("preview");
  GimpUnitEntry *entry1 = gimp_unit_entries_get_entry (entries, id1);
  GimpUnitEntry *entry2 = gimp_unit_entries_get_entry (entries, id2);
  gint          left_attach, 
                right_attach, 
                top_attach, 
                bottom_attach;

  /* save unit */
  g_object_set_data (G_OBJECT (label), "unit", GINT_TO_POINTER (unit));
  /* save the two entries */
  g_object_set_data (G_OBJECT (label), "entry1", (gpointer) entry1);
  g_object_set_data (G_OBJECT (label), "entry2", (gpointer) entry2);

  /* get the position of the entries and set label accordingly */
  left_attach   = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry1), "left_attach"));
  right_attach  = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry1), "right_attach"));
  top_attach    = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry2), "bottom_attach"));
  bottom_attach = top_attach + 1;

  /* add label below unit entries */
  gtk_table_attach (GTK_TABLE (gimp_unit_entries_get_table (entries)),
                    label,
                    left_attach, right_attach, 
                    top_attach, bottom_attach,
                    GTK_FILL, GTK_SHRINK,
                    10, 0);

  /* set alignment */
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  /*gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                             -1);*/

  gtk_widget_show (GTK_WIDGET (label));

  /* connect label updater to changed signal */
  g_signal_connect (G_OBJECT (gimp_unit_entry_get_adjustment (entry1)), "value-changed",
                    G_CALLBACK (gimp_unit_entries_label_updater), (gpointer) label);

  g_signal_connect (G_OBJECT (gimp_unit_entry_get_adjustment (entry2)), "value-changed",
                    G_CALLBACK (gimp_unit_entries_label_updater), (gpointer) label);

  gimp_unit_entries_label_updater (NULL, (gpointer) label);
}

/* add chain button connecting the two given UnitEntries */
GtkWidget* 
gimp_unit_entries_add_chain_button  (GimpUnitEntries *entries,
                                     const gchar     *id1,
                                     const gchar     *id2)
{
  GimpUnitEntriesPrivate  *private        = GIMP_UNIT_ENTRIES_GET_PRIVATE (entries);
  GtkWidget               *chain_button;
  GimpUnitEntry           *entry1         = gimp_unit_entries_get_entry (entries, id1);
  GimpUnitEntry           *entry2         = gimp_unit_entries_get_entry (entries, id2);

  /* retrieve position of entries */
  gint right_attach  = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry1), "right_attach"));
  gint top_attach    = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry1), "top_attach"));
  gint bottom_attach = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry2), "bottom_attach"));

  chain_button = gimp_chain_button_new (GIMP_CHAIN_RIGHT);

  /* add chain_button to right of entries, spanning from the first to the second */
  gtk_table_attach (GTK_TABLE (gimp_unit_entries_get_table (entries)),
                    GTK_WIDGET (chain_button),
                    right_attach, right_attach + 1,
                    top_attach, bottom_attach,
                    GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chain_button), TRUE);

  gtk_widget_show (chain_button);  
  
  private->chain_button = chain_button;                        

  return chain_button;
}

/* get UnitEntry by identifier */
GimpUnitEntry* 
gimp_unit_entries_get_entry (GimpUnitEntries *entries,
                             const gchar     *id)
{
  GimpUnitEntriesPrivate  *private = GIMP_UNIT_ENTRIES_GET_PRIVATE (entries);
  GimpUnitEntry           *entry;

  entry = GIMP_UNIT_ENTRY (g_hash_table_lookup (private->entries_store, id));

  if (entry == NULL)
  {
    g_warning ("gimp_unit_entries_get_entry: entry with id '%s' does not exist", id);
  }

  return entry;
}

/* get UnitEntry by index */
GimpUnitEntry* 
gimp_unit_entries_get_nth_entry (GimpUnitEntries *entries, 
                                 gint             index)
{
  GimpUnitEntriesPrivate  *private = GIMP_UNIT_ENTRIES_GET_PRIVATE (entries);
  GHashTableIter           iter;
  gpointer                 key,
                           value;
  gint                     i;

  if (g_hash_table_size (private->entries_store) <= index || index < 0)
  {
    g_warning ("gimp_unit_entries_get_nth_entry: index < 0 or hash table size smaller than index");
    return NULL;
  }

  i = 0;

  g_hash_table_iter_init (&iter, private->entries_store);

  while (g_hash_table_iter_next (&iter, &key, &value))
  {
    if (i == index)
      return GIMP_UNIT_ENTRY (value);
    i++;
  }

  return NULL;
}

/* updates the text of the preview label */
static void 
gimp_unit_entries_label_updater (GtkAdjustment *adj,
                                 gpointer       user_data)
{
  gchar               str[40];
  GtkLabel           *label       = GTK_LABEL (user_data);
  GimpUnitEntry      *entry1      = GIMP_UNIT_ENTRY (g_object_get_data (G_OBJECT (label), "entry1"));
  GimpUnitEntry      *entry2      = GIMP_UNIT_ENTRY (g_object_get_data (G_OBJECT (label), "entry2"));
  GimpUnit            unit;

  /* retrieve preview unit */
  unit = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (label), "unit"));

  /* retrieve values of the two entries */
  g_sprintf (str, "%s", gimp_unit_entry_to_string_in_unit (entry1, unit));
  g_sprintf (str, "%s x %s ", str, gimp_unit_entry_to_string_in_unit (entry2, unit));

  gtk_label_set_text (label, str);
}

/* signal handler for signals of one of our entries/adjustments */
static void 
gimp_unit_entries_entry_changed  (GtkAdjustment *adj, 
                                  gpointer       user_data)
{
  GimpUnitEntries *entries  = GIMP_UNIT_ENTRIES (user_data);
  GimpUnitEntry   *entry;
  gint            i, count  = gimp_unit_entries_get_entry_count (entries);

  /* find corresponding entry */
  for (i = 0; i < count; i++) 
  {
    entry = gimp_unit_entries_get_nth_entry (entries, i);
    if (gimp_unit_entry_get_adjustment (entry) == GIMP_UNIT_ADJUSTMENT (adj))
      i = count;
  }

  /* emit "changed" */
  g_signal_emit (entries, unit_entries_signals[CHANGED], 0, entry);
}

/* get count of attached unit entries */
gint
gimp_unit_entries_get_entry_count (GimpUnitEntries *entries)
{
  GimpUnitEntriesPrivate  *private = GIMP_UNIT_ENTRIES_GET_PRIVATE (entries);

  return g_hash_table_size (private->entries_store);
}

/* get value of given entry in pixels */
gdouble 
gimp_unit_entries_get_pixels (GimpUnitEntries *entries, 
                              const gchar     *id)
{
  return gimp_unit_entries_get_value_in_unit (entries, id, GIMP_UNIT_PIXEL);
}

gdouble         
gimp_unit_entries_get_nth_pixels (GimpUnitEntries *entries, 
                                  gint             index)
{
  return gimp_unit_entry_get_pixels (gimp_unit_entries_get_nth_entry (entries, index));
}

gdouble 
gimp_unit_entries_get_value_in_unit (GimpUnitEntries *entries,
                                     const gchar     *id, 
                                     GimpUnit        unit)
{
  GimpUnitEntry *entry = gimp_unit_entries_get_entry (entries, id);

  if (entry != NULL)
    return gimp_unit_entry_get_value_in_unit (entry, unit);
  else
    return -1;
}

/* sets the unit of all entries */
void 
gimp_unit_entries_set_unit (GimpUnitEntries *entries, 
                            GimpUnit         unit)
{
  GimpUnitEntry *entry;
  gint           i, count = gimp_unit_entries_get_entry_count (entries);

  /* iterate over list of entries */
  for (i = 0; i < count; i++) 
  {
    entry = gimp_unit_entries_get_nth_entry (entries, i);
    gimp_unit_entry_set_unit (entry, unit);
  }
}

/* sets the resolution of all entries */
void 
gimp_unit_entries_set_resolution (GimpUnitEntries *entries,
                                  gdouble          res)
{
  GimpUnitEntry *entry;
  gint           i, count = gimp_unit_entries_get_entry_count (entries);

  /* iterate over list of entries */
  for (i = 0; i < count; i++) 
  {
    entry = gimp_unit_entries_get_nth_entry (entries, i);
    gimp_unit_entry_set_resolution (entry, res);
  }
}

/* sets resolution mode for all entries */
void 
gimp_unit_entries_set_mode (GimpUnitEntries   *entries,
                            GimpUnitEntryMode  mode)
{
  GimpUnitEntry *entry;
  gint           i, count = gimp_unit_entries_get_entry_count (entries);

  /* iterate over list of entries */
  for (i = 0; i < count; i++) 
  {
    entry = gimp_unit_entries_get_nth_entry (entries, i);
    gimp_unit_entry_set_mode (entry, mode);
    gimp_unit_entry_set_resolution (entry, 1.0);  /* otherwise calculation is not correct */
  }
}

/* calls gtk_entry_set_activates_default for all UnitEntries */
void 
gimp_unit_entries_set_activates_default (GimpUnitEntries *entries, 
                                         gboolean         setting)
{
  GimpUnitEntry *entry;
  gint           i, count = gimp_unit_entries_get_entry_count (entries);

  /* iterate over list of entries */
  for (i = 0; i < count; i++) 
  {
    entry = gimp_unit_entries_get_nth_entry (entries, i);
    gtk_entry_set_activates_default (GTK_ENTRY (entry), setting);
  }
}

void            
gimp_unit_entries_set_bounds (GimpUnitEntries *entries, 
                              GimpUnit         unit, 
                              gdouble          lower, 
                              gdouble          upper)
{
  GimpUnitEntry *entry;
  gint           i, count = gimp_unit_entries_get_entry_count (entries);

  /* iterate over list of entries */
  for (i = 0; i < count; i++) 
  {
    entry = gimp_unit_entries_get_nth_entry (entries, i);
    gimp_unit_entry_set_bounds (entry, unit, lower, upper);
  }
}                                  

void 
gimp_unit_entries_grab_focus (GimpUnitEntries *entries)
{
  if (gimp_unit_entries_get_entry_count > 0)
    gtk_widget_grab_focus (GTK_WIDGET (gimp_unit_entries_get_nth_entry (entries, 0)));
}

GtkWidget*      
gimp_unit_entries_get_table  (GimpUnitEntries *entries)
{
  GimpUnitEntriesPrivate  *private = GIMP_UNIT_ENTRIES_GET_PRIVATE (entries);

  return private->table;
}

GtkWidget*      
gimp_unit_entries_get_chain_button  (GimpUnitEntries *entries)
{
  GimpUnitEntriesPrivate  *private = GIMP_UNIT_ENTRIES_GET_PRIVATE (entries);

  return private->chain_button;
}

void            
gimp_unit_entries_set_pixels (GimpUnitEntries *entries, 
                              const gchar     *id,
                              gdouble          value)
{
  GimpUnitEntry *entry;

  entry = gimp_unit_entries_get_entry (entries, id);

  gimp_unit_entry_set_value_in_unit (entry, value, GIMP_UNIT_PIXEL);
}               
                   
void            
gimp_unit_entries_set_nth_pixels (GimpUnitEntries *entries, 
                                  gint             index,
                                  gdouble          value)
{
  GimpUnitEntry *entry;

  entry = gimp_unit_entries_get_nth_entry (entries, index);

  gimp_unit_entry_set_value_in_unit (entry, value, GIMP_UNIT_PIXEL);
}                                
