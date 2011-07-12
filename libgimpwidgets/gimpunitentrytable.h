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
  GtkWidget  *chain_button;
  GHashTable *entries;

  /* dimensions of "sub-table" containing the actual entries */ 
  gint       bottom, right;
};

struct _GimpUnitEntryTableClass
{
  GObjectClass parent_class;
};

GType           gimp_unit_entry_table_get_type              (void);
GObject*        gimp_unit_entry_table_new                   (void);

GtkWidget*      gimp_unit_entry_table_add_entry             (GimpUnitEntryTable *table, 
                                                             const gchar        *id,
                                                             const gchar        *label,
                                                             gint                x, 
                                                             gint                y);
GtkWidget*      gimp_unit_entry_table_add_entry_defaults    (GimpUnitEntryTable *table, 
                                                             const gchar        *id, 
                                                             const gchar        *label);
void            gimp_unit_entry_table_add_label             (GimpUnitEntryTable *table, 
                                                             GimpUnit            unit, 
                                                             const char         *id1, 
                                                             const char         *id2);
GtkWidget*      gimp_unit_entry_table_add_chain_button      (GimpUnitEntryTable *table,
                                                             const char         *id1, 
                                                             const char         *id2);

GimpUnitEntry*  gimp_unit_entry_table_get_entry             (GimpUnitEntryTable *table, 
                                                             const gchar        *id);
GimpUnitEntry*  gimp_unit_entry_table_get_nth_entry         (GimpUnitEntryTable *table, 
                                                             gint                index);
gint            gimp_unit_entry_table_get_entry_count       (GimpUnitEntryTable *table);
gdouble         gimp_unit_entry_table_get_pixels            (GimpUnitEntryTable *table, 
                                                             const gchar        *id);
gdouble         gimp_unit_entry_table_get_nth_pixels        (GimpUnitEntryTable *table, 
                                                             gint               index);                                                             
GtkWidget*      gimp_unit_entry_table_get_table             (GimpUnitEntryTable *table);                                                             
gdouble         gimp_unit_entry_table_get_value_in_unit     (GimpUnitEntryTable *table,
                                                             const gchar        *id,
                                                             GimpUnit            unit);
GtkWidget*      gimp_unit_entry_table_get_chain_button      (GimpUnitEntryTable *table);                                                              

void            gimp_unit_entry_table_set_unit              (GimpUnitEntryTable *table, 
                                                             GimpUnit            unit);
void            gimp_unit_entry_table_set_resolution        (GimpUnitEntryTable *table, 
                                                             gdouble             res);
void            gimp_unit_entry_table_set_mode              (GimpUnitEntryTable *table, 
                                                             GimpUnitEntryMode   mode);
void            gimp_unit_entry_table_set_activates_default (GimpUnitEntryTable *table, 
                                                             gboolean            setting);
void            gimp_unit_entry_table_set_bounds            (GimpUnitEntryTable *table, 
                                                             GimpUnit            unit, 
                                                             gdouble             upper, 
                                                             gdouble             lower);
void            gimp_unit_entry_table_set_pixels            (GimpUnitEntryTable *table, 
                                                             const gchar        *id,
                                                             gdouble             value);
void            gimp_unit_entry_table_set_nth_pixels        (GimpUnitEntryTable *table, 
                                                             gint                index,
                                                             gdouble             value);                                                                                                                          

void            gimp_unit_entry_table_grab_focus            (GimpUnitEntryTable *table);

G_END_DECLS

#endif /*__GIMP_UNIT_ENTRY_TABLE_H__*/
