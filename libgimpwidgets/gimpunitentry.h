/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunitentry.h
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

#ifndef __GIMP_UNIT_ENTRY_H__
#define __GIMP_UNIT_ENTRY_H__

#include <gtk/gtkspinbutton.h>

#include "gimpunitadjustment.h"

G_BEGIN_DECLS

/**
 * boiler-plate
 **/
#define GIMP_TYPE_UNIT_ENTRY            (gimp_unit_entry_get_type ())
#define GIMP_UNIT_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_UNIT_ENTRY, GimpUnitEntry))
#define GIMP_UNIT_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_UNIT_ENTRY, GimpUnitEntryClass))
#define GIMP_IS_UNIT_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_UNIT_ENTRY))
#define GIMP_IS_UNIT_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_UNIT_ENTRY))
#define GIMP_UNIT_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_UNIT_ENTRY, GimpUnitEntryClass))

typedef struct _GimpUnitEntry       GimpUnitEntry;
typedef struct _GimpUnitEntryClass  GimpUnitEntryClass;

struct _GimpUnitEntry
{
  GtkSpinButton parent_instance;

  /* private */
  GimpUnitAdjustment *unitAdjustment; /* for convinience */

  /* set TRUE while up/down buttons/keys pressed or scrolling so that we can disable
     our parsing and display the changed value */
  gboolean          buttonPressed;    
  gboolean          scrolling;

  const gchar *id; /* identifier string of unit entry (used by GimpUnitEntryTable) */
};

struct _GimpUnitEntryClass
{
  GtkSpinButtonClass parent_class;
};

/**
 * prototypes
 **/
GType     gimp_unit_entry_get_type (void);
GtkWidget *gimp_unit_entry_new (const gchar *id);
GimpUnitAdjustment *gimp_unit_entry_get_adjustment (GimpUnitEntry *entry);
/* connect to another entry */
void gimp_unit_entry_connect (GimpUnitEntry *entry, GimpUnitEntry *target);
/* convenience getters/setters */
const gchar* gimp_unit_entry_get_id (GimpUnitEntry *entry);
void gimp_unit_entry_set_unit (GimpUnitEntry *entry, GimpUnit unit);
void gimp_unit_entry_set_resolution (GimpUnitEntry *entry, gdouble resolution);
void gimp_unit_entry_set_value (GimpUnitEntry *entry, gdouble resolution);
gdouble gimp_unit_entry_get_value (GimpUnitEntry *entry);
gdouble gimp_unit_entry_get_value_in_unit (GimpUnitEntry *entry, GimpUnit unit);
GimpUnit gimp_unit_entry_get_unit (GimpUnitEntry *entry);
void gimp_unit_entry_set_bounds (GimpUnitEntry *entry, GimpUnit unit, gdouble upper, gdouble lower);

G_END_DECLS

#endif /*__GIMP_UNIT_ENTRY_H__*/
