/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpunit.h
 * Copyright (C) 1999-2003 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_UNIT_H__
#define __GIMP_UNIT_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

#define GIMP_TYPE_UNIT               (gimp_unit_get_type ())
#define GIMP_VALUE_HOLDS_UNIT(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GIMP_TYPE_UNIT))

GType         gimp_unit_get_type                     (void) G_GNUC_CONST;


gint          gimp_unit_get_number_of_units          (void);
gint          gimp_unit_get_number_of_built_in_units (void) G_GNUC_CONST;

GimpUnit      gimp_unit_new                 (gchar    *identifier,
                                             gdouble   factor,
                                             gint      digits,
                                             gchar    *symbol,
                                             gchar    *abbreviation,
                                             gchar    *singular,
                                             gchar    *plural);

gboolean      gimp_unit_get_deletion_flag   (GimpUnit  unit);
void          gimp_unit_set_deletion_flag   (GimpUnit  unit,
                                             gboolean  deletion_flag);

gdouble       gimp_unit_get_factor          (GimpUnit  unit);

gint          gimp_unit_get_digits          (GimpUnit  unit);

const gchar * gimp_unit_get_identifier      (GimpUnit  unit);

const gchar * gimp_unit_get_symbol          (GimpUnit  unit);
const gchar * gimp_unit_get_abbreviation    (GimpUnit  unit);
const gchar * gimp_unit_get_singular        (GimpUnit  unit);
const gchar * gimp_unit_get_plural          (GimpUnit  unit);


G_END_DECLS

#endif /* __GIMP_UNIT_H__ */
