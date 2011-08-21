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

/**
 * SECTION: gimpunitentries
 * @title: GimpUnitEntries
 * @short_description: A convenience class for managing a set of #GimpUnitEntry widgets
 * @see_also: #GimpUnitEntry
 *
 * #GimpUnitEntries is a convenience class that creates and holds one or more 
 * #GimpUnitEntry widgets. It provides functions for easily adding and accessing the widgets
 * and also holds a #GtkTable containing the widgets for easy display. 
 *
 * It has been designed to cover the most common use-cases of #GimpUnitEntry so that you
 * don't have to deal with setting the entries up manually. Such a use-case would be the
 * input of width and height of an image, for example. It is also a replacement for
 * the old GimpSizeEntry and aims to provide the same functionaly.
 *
 * Whenever you add a GimpUnitEntry, #GimpUnitEntries stores it in a hash table and aligns
 * it vertically in its #GtkTable. You can also add a #GimpChainButton connecting two entries
 * or a preview label showing the value of two entries in a specific unit.
 *
 **/

/* debug macro */
//#define UNITENTRY_DEBUG
#ifdef  UNITENTRY_DEBUG
#define DEBUG(x) g_debug x 
#else
#define DEBUG(x) /* nothing */
#endif

#define UNIT_ENTRIES_ENTRY_COLUMN  2

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

typedef struct
{
  GtkWidget     *label;
  GimpUnitEntry *entry1;
  GimpUnitEntry *entry2;
  GimpUnit       unit;
} PreviewLabelData;

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

  private->table          = gtk_table_new     (0, 0, FALSE);
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

/**
 * gimp_unit_entries_new:
 *
 * Creates a new #GimpUnitEntries object. Note that the (empty) #GtkTable is also created
 * now so you can already access it even if you didn't add any #GimpUnitEntry widgets. 
 *
 * Returns: A Pointer to the new #GimpUnitEntries object.
 **/
GimpUnitEntries*
gimp_unit_entries_new (void)
{
  GObject *entries;

  entries = g_object_new (GIMP_TYPE_UNIT_ENTRIES, NULL);

  return GIMP_UNIT_ENTRIES (entries);
}

/**
 * gimp_unit_entries_add_entry:
 * @entries:   The #GimpUnitEntries object you want to add the entry to.
 * @id:        String used as an identifier for accessing the entry later.
 * @label_str: String for the label left to the entry or NULL.
 *
 * Creates a new #GimpUnitEntry and its #GimpUnitAdjustment, stores it internally and
 * adds it to the #GtkTable of the #GimpUnitEntries object.
 *
 * The entry is later identified by the id string. You can use any string for an id,
 * but there are a couple of common #defined strings to catch typos at compile time:
 * GIMP_UNIT_ENTRIES_HEIGHT,
 * GIMP_UNIT_ENTRIES_WIDTH,
 * GIMP_UNIT_ENTRIES_OFFSET_X,
 * GIMP_UNIT_ENTRIES_OFFSET_Y,
 * GIMP_UNIT_ENTRIES_RESOLUTION_X,
 * GIMP_UNIT_ENTRIES_RESOLUTION_Y        
 * You should use those whenever applicable or define you own strings.
 *
 * If label_str is not NULL, a #GtkLabel is automatically created and added to the table
 * to the left of the entry displaying said string.
 *
 * The entry is automatically connected to all other entries so that they all have the
 * same unit at all times.
 *
 * Returns: A Pointer to the newly created #GimpUnitEntry widget.
 **/
GtkWidget* 
gimp_unit_entries_add_entry (GimpUnitEntries *entries,
                             const gchar     *id,
                             const gchar     *label_str)
{
  GimpUnitEntriesPrivate  *private    = GIMP_UNIT_ENTRIES_GET_PRIVATE (entries);
  GimpUnitAdjustment      *adjustment = gimp_unit_adjustment_new ();
  GimpUnitEntry           *entry      = GIMP_UNIT_ENTRY (gimp_unit_entry_new ()); 
  GimpUnitEntry           *other_entry;
  GtkWidget               *label;
  guint                   bottom;
  gint                    i;
  gint                    left_attach,
                          right_attach,
                          top_attach,
                          bottom_attach;

  gimp_unit_entry_set_adjustment (entry, adjustment);                        

  /* retrieve bottom row of table */
  gtk_table_get_size (GTK_TABLE (gimp_unit_entries_get_table (entries)), &bottom, NULL);

  /* position of the entry (leave one row/column empty for labels etc) */
  left_attach   = UNIT_ENTRIES_ENTRY_COLUMN,
  right_attach  = left_attach + 1,
  top_attach    = bottom,
  bottom_attach = bottom + 1;

  /* remember position in table so that we later can place other widgets around it */
  g_object_set_data (G_OBJECT (entry), "row", GINT_TO_POINTER (bottom_attach));

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

/**
 * gimp_unit_entry_add_preview_label:
 * @entries:  The #GimpUnitEntries you want to add the label to.
 * @unit:     The unit you want to display the values in.
 * @id1:      The first entry you want to display the value of.
 * @id2:      The second entry you want to display the value of.
 *  
 * Adds an #GtkLabel showing the values of the #GimpUnitEntry widgets with id1 and id2.
 * The label is added to the table below the second entry and its string is automatically
 * updated when either entry changes its value.
 **/
void 
gimp_unit_entries_add_preview_label (GimpUnitEntries *entries,
                                     GimpUnit         unit,
                                     const gchar     *id1,
                                     const gchar     *id2)
{
  GtkWidget         *label  = gtk_label_new ("preview");
  GimpUnitEntry     *entry1 = gimp_unit_entries_get_entry (entries, id1);
  GimpUnitEntry     *entry2 = gimp_unit_entries_get_entry (entries, id2);
  gint               left_attach, 
                     right_attach, 
                     top_attach, 
                     bottom_attach;

  /* save unit */
  g_object_set_data (G_OBJECT (label), "unit", GINT_TO_POINTER (unit));
  /* save the two entries */
  g_object_set_data (G_OBJECT (label), "entry1", (gpointer) entry1);
  g_object_set_data (G_OBJECT (label), "entry2", (gpointer) entry2);

  /* place label below second entry */
  left_attach   = UNIT_ENTRIES_ENTRY_COLUMN;
  right_attach  = left_attach + 1;
  top_attach    = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry2), "row"));
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

/**
 * gimp_unit_entry_add_chain_button:
 * @entries:  The #GimpUnitEntries you want to add the chain button to.
 * @id1:      The first entry connected to the chain button.
 * @id2:      The second entry connected to the chain button.
 *
 * Adds a #GimpChainButton to the #GimpUnitEntries' table. The chain button is attached
 * to the right of both entries. Note that the state of chain button is not managed
 * automatically so you have to update it manually.
 **/
GtkWidget* 
gimp_unit_entries_add_chain_button  (GimpUnitEntries *entries,
                                     const gchar     *id1,
                                     const gchar     *id2)
{
  GimpUnitEntriesPrivate  *private        = GIMP_UNIT_ENTRIES_GET_PRIVATE (entries);
  GtkWidget               *chain_button;
  GimpUnitEntry           *entry1         = gimp_unit_entries_get_entry (entries, id1);
  GimpUnitEntry           *entry2         = gimp_unit_entries_get_entry (entries, id2);

  /* place chain button right of entries */
  gint left_attach   = UNIT_ENTRIES_ENTRY_COLUMN + 1;
  gint right_attach  = left_attach + 1;
  gint top_attach    = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry1), "row")) - 1;
  gint bottom_attach = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry2), "row"));

  chain_button = gimp_chain_button_new (GIMP_CHAIN_RIGHT);

  /* add chain_button to right of entries, spanning from the first to the second */
  gtk_table_attach (GTK_TABLE (gimp_unit_entries_get_table (entries)),
                    GTK_WIDGET (chain_button),
                    left_attach, right_attach,
                    top_attach, bottom_attach,
                    GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chain_button), TRUE);

  gtk_widget_show (chain_button);  
  
  private->chain_button = chain_button;                        

  return chain_button;
}

/**
 * gimp_unit_entries_get_entry:
 * @entries:  The #GimpUnitEntries object you want to get the entry from.
 * @id:       The id string of the entry you want to get.
 *
 * Returns: A Pointer to the #GimpUnitEntry with the given id.
 **/
GimpUnitEntry* 
gimp_unit_entries_get_entry (GimpUnitEntries *entries,
                             const gchar     *id)
{
  GimpUnitEntriesPrivate  *private        = GIMP_UNIT_ENTRIES_GET_PRIVATE (entries);
  gpointer                *entry_pointer;

  entry_pointer = g_hash_table_lookup (private->entries_store, id);

  if (!GIMP_IS_UNIT_ENTRY (entry_pointer))
  {
    g_warning ("gimp_unit_entries_get_entry: entry with id '%s' does not exist", id);
    return NULL;
  }

  return GIMP_UNIT_ENTRY (entry_pointer);
}

/**
 * gimp_unit_entries_get_nth_entry:
 * @entries: The #GimpUnitEntries object you want to get the entry from.
 * @index:   The index of the entry you want to get.
 *
 * This function is only provided for easy porting from the old GimpSizeEntry widget. 
 * You should always identifiy the entries by their respective id strings, as the internally
 * used hash table does not guarantee the right order of the stored entries. Use
 * #gimp_unit_entries_get_entry instead.
 *
 * Returns: A Pointer to the #GimpUnitEntry with the given index.
 **/
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

/**
 * gimp_unit_entries_get_entry_count:
 * @entries:  The #GimpUnitEntries object you want to get the entry count from.
 *
 * Returns: The count of #GimpUnitEntry widgets currently stored.
 **/
gint
gimp_unit_entries_get_entry_count (GimpUnitEntries *entries)
{
  GimpUnitEntriesPrivate  *private = GIMP_UNIT_ENTRIES_GET_PRIVATE (entries);

  return g_hash_table_size (private->entries_store);
}

/**
 * gimp_unit_entries_get_pixels:
 * @entries:  The #GimpUnitEntries object containing the entry.
 * @id     :  The id of the entry you want to get the value of.
 *
 * Returns: The value in pixels of the entry with the given id.
 **/
gdouble 
gimp_unit_entries_get_pixels (GimpUnitEntries *entries, 
                              const gchar     *id)
{
  GimpUnitEntry *entry;

  entry = gimp_unit_entries_get_entry (entries, id);

  return gimp_unit_entry_get_pixels (entry);
}

/**
 * gimp_unit_entries_get_nth_pixels:
 * @entries: The #GimpUnitEntries object containing the entry.
 * @index:   The index of the entry you want to get the value of.
 *
 * This function is only provided for easy porting from the old GimpSizeEntry widget. 
 * You should always identifiy the entries by their respective id strings, as the internally
 * used hash table does not guarantee the right order of the stored entries. Use
 * #gimp_unit_entries_get_pixels instead.
 *
 * Returns: The value in pixels of the entry with the given index.
 **/
gdouble         
gimp_unit_entries_get_nth_pixels (GimpUnitEntries *entries, 
                                  gint             index)
{
  GimpUnitEntry *entry;

  entry = gimp_unit_entries_get_nth_entry (entries, index);

  return gimp_unit_entry_get_pixels (entry);
}

/**
 * gimp_unit_entries_set_unit:
 * @entries:    The #GimpUnitEntries object containing your entries.
 * @adjustment: The unit you want to set the entries to.
 *
 * Sets the unit of all entries of the GimpUnitEntries object. Note that all entries have
 * the same unit so you cannot set one entry to a different unit.
 **/
void 
gimp_unit_entries_set_unit (GimpUnitEntries *entries, 
                            GimpUnit         unit)
{
  GimpUnitAdjustment *adj;
  gint                i, count = gimp_unit_entries_get_entry_count (entries);

  /* iterate over list of entries */
  for (i = 0; i < count; i++) 
  {
    adj = gimp_unit_entries_get_nth_adjustment (entries, i);
    gimp_unit_adjustment_set_unit (adj, unit);
  }
}

/**
 * gimp_unit_entries_set_resolution:
 * @entries:  The #GimpUnitEntries object containing your entries.
 * @...:      A NULL-terminated list of string/double pairs, the string being the
 *            id of an #GimpUnitEntry and the double being the resolution to set.
 *
 * Sets the resolution of one or more entries of the #GimpUnitEntries object.
 *
 * This is a variable arguments method. Pass a NULL-terminated list of string/double pairs,
 * the string being the id of an #GimpUnitEntry and the double being the resolution to set.
 *
 * Be cautious: this function is not type-safe. The resolution value must be a double. When
 * you are not sure the value you are passing is a double, always use an explicit type-cast!
 * Always end the argument list with NULL!
 *
 *<informalexample>
 *  <programlisting>
 *  gimp_unit_entry_set_resolution (unit_entries,
 *                                  "id_of_entry_1",  100.0,
 *                                  "id_of_entry_2",  100.0,
 *                                  NULL);
 *  </programlisting>
 *</informalexample>
 **/
void 
gimp_unit_entries_set_resolution (GimpUnitEntries *entries,
                                  ...)
{
  GimpUnitAdjustment *adj;
  va_list             args;
  gchar              *entry_name;
  gdouble             resolution;

  va_start (args, entries);

  while ((entry_name = va_arg (args, gchar*)) != NULL)
  {
    adj = gimp_unit_entries_get_adjustment (entries, entry_name);

    if (adj == NULL)
      break;

    resolution = va_arg (args, gdouble);
    gimp_unit_adjustment_set_resolution (adj, resolution);
  }

  va_end (args);
}

/**
 * gimp_unit_entries_set_mode:
 * @entries:  The #GimpUnitEntries object containing your entries.
 * @mode:     The mode you want to set the unit of.
 *
 * Sets all entries of the #GimpUnitEntries object to the given mode. See 
 * #gimp_unit_entry_set_mode for details. Note that all entries are in the mode so
 * you cannot set one entry to a different mode.
 **/
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
  }
}

/**
 * gimp_unit_entries_set_activates_default:
 * @entries: The #GimpUnitEntries object containing your entries.
 * @setting: activate or deactivate the activates default setting
 *
 * Calls gtk_entry_set_activates_default on all #GimpUnitEntry widgets. See
 * #gtk_entry_set_activates_default for details.
 **/
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

/**
 * gimp_unit_entries_set_bounds:
 * @entries:  The #GimpUnitEntries object containing your entries.
 * @unit:     The unit your bounds are in.
 * @...:      A NULL-terminated list of string/double/double triples, the string being the
 *            id of an #GimpUnitEntry, the first double the lower bound and the second
 *            double being the upper bound.
 *
 * Sets the bounds of one or more entries of the #GimpUnitEntries object.
 *
 * This is a variable arguments method. list of string/double/double triples, the string being the
 * id of an #GimpUnitEntry, the first double the lower bound and the second
 * double being the upper bound.
 *
 * See #gimp_unit_adjustment_set_bounds for more information.
 *
 * Be cautious: this function is not type-safe. The bound values must be doubles. When
 * you are not sure the value you are passing is a double, always use an explicit type-cast!
 * Always end the argument list with NULL!
 *
 *<informalexample>
 *  <programlisting>
 *  gimp_unit_entry_set_bounds (unit_entries, GIMP_UNIT_PIXEL,
 *                              "id_of_entry_1",  2000.0,
 *                              "id_of_entry_2",  1000.0,
 *                              NULL);
 *  </programlisting>
 *</informalexample>
 **/
void            
gimp_unit_entries_set_bounds (GimpUnitEntries *entries, 
                              GimpUnit         unit, 
                              ...)
{
  GimpUnitAdjustment *adj;
  va_list             args;
  gchar              *entry_name;
  gdouble             lower;
  gdouble             upper;

  va_start (args, unit);

  while ((entry_name = va_arg (args, gchar*)))
  {
    adj = gimp_unit_entries_get_adjustment (entries, entry_name);

    if (adj == NULL)
      break;

    lower = va_arg (args, gdouble);
    upper = va_arg (args, gdouble);
    gimp_unit_adjustment_set_bounds (adj, unit, lower, upper);
  }

  va_end (args);
}                                  

/**
 * gimp_unit_entries_grab_focus:
 * @entries: The #GimpUnitEntries object containing your entries.
 *
 * Calls gtk_widget_grab_focus on the first #GimpUnitEntry widget. See
 * #gtk_widget_grab_focus for details.
 **/
void 
gimp_unit_entries_grab_focus (GimpUnitEntries *entries)
{
  if (gimp_unit_entries_get_entry_count > 0)
    gtk_widget_grab_focus (GTK_WIDGET (gimp_unit_entries_get_nth_entry (entries, 0)));
}

/**
 * gimp_unit_entries_get_table:
 * @entry:  The #GimpUnitEntries object you want to get the table of.
 *
 * Returns: A pointer to the #GtkTable containing the entries.
 **/
GtkWidget*      
gimp_unit_entries_get_table  (GimpUnitEntries *entries)
{
  GimpUnitEntriesPrivate  *private = GIMP_UNIT_ENTRIES_GET_PRIVATE (entries);

  return private->table;
}

/**
 * gimp_unit_entries_get_chain_button:
 * @entry:  The #GimpUnitEntries object you want to get the chain button of.
 *
 * Returns: A pointer to the #GimpChainButton if there is any.
 **/
GtkWidget*      
gimp_unit_entries_get_chain_button  (GimpUnitEntries *entries)
{
  GimpUnitEntriesPrivate  *private = GIMP_UNIT_ENTRIES_GET_PRIVATE (entries);

  return private->chain_button;
}

/**
 * gimp_unit_entries_set_pixels:
 * @entries:  The #GimpUnitEntries object containing your entries.
 * @...:      A NULL-terminated list of string/double pairs, the string being the
 *            id of an #GimpUnitEntry and the double being the value in pixels to set.
 *
 * Sets the pixel values of one or more entries of the #GimpUnitEntries object.
 *
 * This is a variable arguments method. Pass a NULL-terminated list of string/double pairs,
 * the string being the id of an #GimpUnitEntry and the double being the pixel value to set.
 *
 * Be cautious: this function is not type-safe. The pixel value must be a double. When
 * you are not sure the value you are passing is a double, always use an explicit type-cast!
 * Always end the argument list with NULL!
 *
 *<informalexample>
 *  <programlisting>
 *  gimp_unit_entry_set_pixels (unit_entries,
 *                              "id_of_entry_1",  1280.0,
 *                              "id_of_entry_2",  1024.0,
 *                              NULL);
 *  </programlisting>
 *</informalexample>
 **/
void            
gimp_unit_entries_set_pixels (GimpUnitEntries *entries, 
                              ...)
{
  GimpUnitAdjustment *adj;
  va_list             args;
  gchar              *entry_name;
  gdouble             pixels;

  va_start (args, entries);

  while ((entry_name = va_arg (args, gchar*)) != NULL)
  {
    adj = gimp_unit_entries_get_adjustment (entries, entry_name);

    if (adj == NULL)
      break;

    pixels = va_arg (args, gdouble);
    gimp_unit_adjustment_set_value_in_unit (adj, pixels, GIMP_UNIT_PIXEL);
  }

  va_end (args);
}               

/**
 * gimp_unit_entries_set_nth_pixels:
 * @entries: The #GimpUnitEntries object containing the entry.
 * @index:   The index of the entry you want to set the value of.
 * @value:   The value to set in pixels.
 *
 * This function is only provided for easy porting from the old GimpSizeEntry widget. 
 * You should always identifiy the entries by their respective id strings, as the internally
 * used hash table does not guarantee the right order of the stored entries. Use
 * #gimp_unit_entries_set_pixels instead.
 **/                   
void            
gimp_unit_entries_set_nth_pixels (GimpUnitEntries *entries, 
                                  gint             index,
                                  gdouble          value)
{
  GimpUnitAdjustment *adj;

  adj = gimp_unit_entries_get_nth_adjustment (entries, index);

  gimp_unit_adjustment_set_value_in_unit (adj, value, GIMP_UNIT_PIXEL);
}      

/**
 * gimp_unit_entries_get_adjustment:
 * @entries:  The #GimpUnitEntries object containing the entry.
 * @id     :  The id of the entry you want to get the adjustment of.
 *
 * Returns: The #GimpUnitAdjustment of the entry with the given id.
 **/
GimpUnitAdjustment*  
gimp_unit_entries_get_adjustment (GimpUnitEntries *entries,
                                  const gchar     *id)
{
  GimpUnitEntry *entry;

  entry = gimp_unit_entries_get_entry (entries, id);

  if (entry == NULL)
    return NULL;

  return gimp_unit_entry_get_adjustment (entry);
}

/**
 * gimp_unit_entries_get_nth_adjustment:
 * @entries: The #GimpUnitEntries object containing the entry.
 * @index:   The index of the entry you want to get the adjustment of.
 *
 * This function is only provided for easy porting from the old GimpSizeEntry widget. 
 * You should always identifiy the entries by their respective id strings, as the internally
 * used hash table does not guarantee the right order of the stored entries. Use
 * #gimp_unit_entries_get_adjustment instead.
 *
 * Returns: The #GimpUnitAdjustment of the entry with the given index.
 **/
GimpUnitAdjustment*  
gimp_unit_entries_get_nth_adjustment  (GimpUnitEntries *entries,
                                       gint             index)
{
  GimpUnitEntry *entry;
  
  entry = gimp_unit_entries_get_nth_entry (entries, index);
  
  return gimp_unit_entry_get_adjustment (entry);  
}       
