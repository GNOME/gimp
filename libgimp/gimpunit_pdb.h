/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball 
 *
 * gimpunit_pdb.h
 * Copyright (C) 1999-2000 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_UNIT_PDB_H__
#define __GIMP_UNIT_PDB_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */


gint       _gimp_unit_get_number_of_units          (void);
gint       _gimp_unit_get_number_of_built_in_units (void);

GimpUnit   _gimp_unit_new                 (gchar    *identifier,
					   gdouble   factor,
					   gint      digits,
					   gchar    *symbol,
					   gchar    *abbreviation,
					   gchar    *singular,
					   gchar    *plural);

gboolean   _gimp_unit_get_deletion_flag   (GimpUnit  unit);
void       _gimp_unit_set_deletion_flag   (GimpUnit  unit,
					   gboolean  deletion_flag);

gdouble    _gimp_unit_get_factor          (GimpUnit  unit);

gint       _gimp_unit_get_digits          (GimpUnit  unit);

gchar    * _gimp_unit_get_identifier      (GimpUnit  unit);

gchar    * _gimp_unit_get_symbol          (GimpUnit  unit);
gchar    * _gimp_unit_get_abbreviation    (GimpUnit  unit);
gchar    * _gimp_unit_get_singular        (GimpUnit  unit);
gchar    * _gimp_unit_get_plural          (GimpUnit  unit);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_UNIT_PDB_H__ */
