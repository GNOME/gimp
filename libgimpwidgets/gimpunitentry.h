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

#define GIMP_TYPE_UNIT_ENTRY            (gimp_unit_entry_get_type ())
#define GIMP_UNIT_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_UNIT_ENTRY, GimpUnitEntry))
#define GIMP_UNIT_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_UNIT_ENTRY, GimpUnitEntryClass))
#define GIMP_IS_UNIT_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_UNIT_ENTRY))
#define GIMP_IS_UNIT_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_UNIT_ENTRY))
#define GIMP_UNIT_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_UNIT_ENTRY, GimpUnitEntryClass))

typedef struct _GimpUnitEntry       GimpUnitEntry;
typedef struct _GimpUnitEntryClass  GimpUnitEntryClass;

typedef enum
{
  GIMP_UNIT_ENTRY_MODE_UNIT = 0, 
  GIMP_UNIT_ENTRY_MODE_RESOLUTION
} GimpUnitEntryMode;

struct _GimpUnitEntry
{
  GtkSpinButton parent_instance;

  gpointer      private;
};

struct _GimpUnitEntryClass
{
  GtkSpinButtonClass parent_class;
};

GType                 gimp_unit_entry_get_type          (void);
GtkWidget *           gimp_unit_entry_new               (void);

GimpUnitAdjustment *  gimp_unit_entry_get_adjustment    (GimpUnitEntry      *entry);
gdouble               gimp_unit_entry_get_pixels        (GimpUnitEntry      *entry);

void                  gimp_unit_entry_set_adjustment    (GimpUnitEntry      *entry,
                                                         GimpUnitAdjustment *adjustment);
void                  gimp_unit_entry_set_bounds        (GimpUnitEntry *entry, 
                                                         GimpUnit       unit, 
                                                         gdouble        lower,
                                                         gdouble        upper);
void                  gimp_unit_entry_set_mode          (GimpUnitEntry      *entry, 
                                                         GimpUnitEntryMode   mode);
void                  gimp_unit_entry_set_pixels        (GimpUnitEntry      *entry,
                                                         gdouble             value);
                                                         
gchar*                gimp_unit_entry_to_string         (GimpUnitEntry      *entry);
gchar*                gimp_unit_entry_to_string_in_unit (GimpUnitEntry      *entry, 
                                                         GimpUnit            unit);                                                                                                                    
                                                         
void                  gimp_unit_entry_connect           (GimpUnitEntry      *entry, 
                                                         GimpUnitEntry      *target);                                                                                                               

G_END_DECLS

#endif /*__GIMP_UNIT_ENTRY_H__*/
