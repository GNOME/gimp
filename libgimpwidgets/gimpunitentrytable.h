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

#include <gtk/gtktable.h>
#include <gtk/gtklabel.h>
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

/* enum for standard 'set-ups' */
typedef enum 
{
  GIMP_UNIT_ENTRY_TABLE_EMPTY,                /* empty table */
  GIMP_UNIT_ENTRY_TABLE_DEFAULT,              /* just two entries */
  GIMP_UNIT_ENTRY_TABLE_DEFAULT_WITH_PREVIEW  /* ... with label showing value */
} GimpUnitEntryTableSetup;

struct _GimpUnitEntryTable
{
  GObject parent_instance;

  /* private */
  GtkWidget  *table;
  GList      *entries;        /* list of entries */
  GtkLabel   *previewLabel;   /* (optional) preview label automatically showing value */
  GimpUnit   previewUnit;    /* unit in which the preview is shown */
};

struct _GimpUnitEntryTableClass
{
  GObjectClass parent_class;
};

/**
 * prototypes
 **/
GType     gimp_unit_entry_table_get_type (void);
GObject   *gimp_unit_entry_table_new (void);

/* add UnitEntry */
GtkWidget* gimp_unit_entry_table_add_entry (GimpUnitEntryTable *table, const gchar* id, const gchar *label);
//void gimp_unit_entry_table_add_entries ()
/* add preview label showing the current value in given unit */
void gimp_unit_entry_table_add_label (GimpUnitEntryTable *table, GimpUnit unit);
/* get UnitEntry by id */
GimpUnitEntry* gimp_unit_entry_table_get_entry (GimpUnitEntryTable *table, const gchar* id);
/* get UnitEntry by index */
GimpUnitEntry* gimp_unit_entry_table_get_nth_entry (GimpUnitEntryTable *table, gint index);

G_END_DECLS

#endif /*__GIMP_UNIT_ENTRY_TABLE_H__*/
