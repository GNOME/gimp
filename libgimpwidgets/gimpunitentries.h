/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunitentries.h
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

#ifndef __GIMP_UNIT_ENTRIES_H__
#define __GIMP_UNIT_ENTRIES_H__

#include <stdarg.h>

#include <glib.h>

#include "gimpunitentry.h"

G_BEGIN_DECLS

#define GIMP_TYPE_UNIT_ENTRIES            (gimp_unit_entries_get_type ())
#define GIMP_UNIT_ENTRIES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_UNIT_ENTRIES, GimpUnitEntries))
#define GIMP_UNIT_ENTRIES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_UNIT_ENTRIES, GimpUnitEntriesClass))
#define GIMP_IS_UNIT_ENTRIES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_UNIT_ENTRIES))
#define GIMP_IS_UNIT_ENTRIES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_UNIT_ENTRIES))
#define GIMP_UNIT_ENTRIES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_UNIT_ENTRIES, GimpUnitEntriesClass))

typedef struct _GimpUnitEntries       GimpUnitEntries;
typedef struct _GimpUnitEntriesClass  GimpUnitEntriesClass;

struct _GimpUnitEntries
{
  GObject parent_instance;

  /* private */
  GtkWidget  *table;
  GtkWidget  *chain_button;
  GHashTable *entries_store;

  /* dimensions of "sub-table" containing the actual entries */ 
  gint       bottom, right;
};

struct _GimpUnitEntriesClass
{
  GObjectClass parent_class;
};

GType           gimp_unit_entries_get_type              (void);
GObject*        gimp_unit_entries_new                   (void);

GtkWidget*      gimp_unit_entries_add_entry             (GimpUnitEntries *entries, 
                                                         const gchar     *id,
                                                         const gchar     *label,
                                                         gint             x, 
                                                         gint             y);
GtkWidget*      gimp_unit_entries_add_entry_defaults    (GimpUnitEntries *entries, 
                                                         const gchar     *id, 
                                                         const gchar     *label);
void            gimp_unit_entries_add_label             (GimpUnitEntries *entries, 
                                                         GimpUnit         unit, 
                                                         const char      *id1, 
                                                         const char      *id2);
GtkWidget*      gimp_unit_entries_add_chain_button      (GimpUnitEntries *entries,
                                                         const char      *id1, 
                                                         const char      *id2);

GimpUnitEntry*  gimp_unit_entries_get_entry             (GimpUnitEntries *entries, 
                                                         const gchar     *id);
GimpUnitEntry*  gimp_unit_entries_get_nth_entry         (GimpUnitEntries *entries, 
                                                         gint             index);
gint            gimp_unit_entries_get_entry_count       (GimpUnitEntries *entries);
gdouble         gimp_unit_entries_get_pixels            (GimpUnitEntries *entries, 
                                                         const gchar     *id);
gdouble         gimp_unit_entries_get_nth_pixels        (GimpUnitEntries *entries, 
                                                         gint             index);                                                             
GtkWidget*      gimp_unit_entries_get_table             (GimpUnitEntries *entries);                                                             
gdouble         gimp_unit_entries_get_value_in_unit     (GimpUnitEntries *entries,
                                                         const gchar     *id,
                                                         GimpUnit         unit);
GtkWidget*      gimp_unit_entries_get_chain_button      (GimpUnitEntries *entries);                                                              

void            gimp_unit_entries_set_unit              (GimpUnitEntries *entries, 
                                                         GimpUnit         unit);
void            gimp_unit_entries_set_resolution        (GimpUnitEntries *entries, 
                                                         gdouble          res);
void            gimp_unit_entries_set_mode              (GimpUnitEntries *entries, 
                                                         GimpUnitEntryMode mode);
void            gimp_unit_entries_set_activates_default (GimpUnitEntries *entries, 
                                                         gboolean         setting);
void            gimp_unit_entries_set_bounds            (GimpUnitEntries *entries, 
                                                         GimpUnit         unit, 
                                                         gdouble          upper, 
                                                         gdouble          lower);
void            gimp_unit_entries_set_pixels            (GimpUnitEntries *entries, 
                                                         const gchar     *id,
                                                         gdouble          value);
void            gimp_unit_entries_set_nth_pixels        (GimpUnitEntries *entries, 
                                                         gint             index,
                                                         gdouble          value);                                                                                                                          

void            gimp_unit_entries_grab_focus            (GimpUnitEntries *entries);

G_END_DECLS

#endif /*__GIMP_UNIT_ENTRIES_H__*/
