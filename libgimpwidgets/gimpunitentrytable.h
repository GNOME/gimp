/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunitentrytable.h
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

#ifndef __GIMP_UNIT_ENTRY_TABLE_H__
#define __GIMP_UNIT_ENTRY_TABLE_H__

#include <stdarg.h>

#include <glib.h>

#include "gimpunitentry.h"

G_BEGIN_DECLS

/**
 * boiler-plate
 **/
#define GIMP_TYPE_UNIT_ENTRY_TABLE            (gimp_unit_entry_table_get_type ())
#define GIMP_UNIT_ENTRY_TABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_UNIT_ENTRY_TABLE, GimpUnitEntryTable))
#define GIMP_UNIT_ENTRY_TABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_UNIT_ENTRY_TABLE, GimpUnitEntryTableClass))
#define GIMP_IS_UNIT_ENTRY_TABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_UNIT_ENTRY_TABLE))
#define GIMP_IS_UNIT_ENTRY_TABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_UNIT_ENTRY_TABLE))
#define GIMP_UNIT_ENTRY_TABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_UNIT_ENTRY_TABLE, GimpUnitEntryTableClass))

typedef struct _GimpUnitEntryTable       GimpUnitEntryTable;
typedef struct _GimpUnitEntryTableClass  GimpUnitEntryTableClass;

struct _GimpUnitEntryTable
{
  GObject parent_instance;

  /* private */
  GtkWidget  *table;
  GList      *entries;        /* list of entries */

  gint       bottom;          /* bottom row of our table */
};

struct _GimpUnitEntryTableClass
{
  GObjectClass parent_class;

  /* signals */
  guint sig_value_changed_id;
};

/**
 * prototypes
 **/
GType     gimp_unit_entry_table_get_type (void);
GObject   *gimp_unit_entry_table_new (void);

/* add UnitEntry */
GtkWidget* gimp_unit_entry_table_add_entry (GimpUnitEntryTable *table, const gchar* id, const gchar *label);
//void gimp_unit_entry_table_add_entries ()
/* add preview label showing the current value of two entries in given unit */
void gimp_unit_entry_table_add_label (GimpUnitEntryTable *table, GimpUnit unit, const char* id1, const char* id2);
/* add chain button connecting the two given UnitEntries */
GtkWidget* gimp_unit_entry_table_add_chainbutton (GimpUnitEntryTable *table, const char* id1, const char*id2);
/* get UnitEntry by id */
GimpUnitEntry* gimp_unit_entry_table_get_entry (GimpUnitEntryTable *table, const gchar* id);
/* get UnitEntry by index */
GimpUnitEntry* gimp_unit_entry_table_get_nth_entry (GimpUnitEntryTable *table, gint index);
/* get count of attached unit entries */
gint gimp_unit_entry_table_get_entry_count (GimpUnitEntryTable *table);
/* sets the unit of all entries */
void gimp_unit_entry_table_set_unit (GimpUnitEntryTable *table, GimpUnit unit);
/* sets the resolution of all entries */
void gimp_unit_entry_table_set_resolution (GimpUnitEntryTable *table, gdouble res);

G_END_DECLS

#endif /*__GIMP_UNIT_ENTRY_TABLE_H__*/
