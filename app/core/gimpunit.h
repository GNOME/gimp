/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __APP_GIMP_UNIT_H__
#define __APP_GIMP_UNIT_H__


gint          _gimp_unit_get_number_of_units          (Gimp        *gimp);
gint          _gimp_unit_get_number_of_built_in_units (Gimp        *gimp) G_GNUC_CONST;

GimpUnit      _gimp_unit_new                          (Gimp        *gimp,
                                                       const gchar *identifier,
                                                       gdouble      factor,
                                                       gint         digits,
                                                       const gchar *symbol,
                                                       const gchar *abbreviation,
                                                       const gchar *singular,
                                                       const gchar *plural);

gboolean      _gimp_unit_get_deletion_flag            (Gimp        *gimp,
                                                       GimpUnit     unit);
void          _gimp_unit_set_deletion_flag            (Gimp        *gimp,
                                                       GimpUnit     unit,
                                                       gboolean     deletion_flag);

gdouble       _gimp_unit_get_factor                   (Gimp        *gimp,
                                                       GimpUnit     unit);

gint          _gimp_unit_get_digits                   (Gimp        *gimp,
                                                       GimpUnit     unit);

const gchar * _gimp_unit_get_identifier               (Gimp        *gimp,
                                                       GimpUnit     unit);

const gchar * _gimp_unit_get_symbol                   (Gimp        *gimp,
                                                       GimpUnit     unit);
const gchar * _gimp_unit_get_abbreviation             (Gimp        *gimp,
                                                       GimpUnit     unit);
const gchar * _gimp_unit_get_singular                 (Gimp        *gimp,
                                                       GimpUnit     unit);
const gchar * _gimp_unit_get_plural                   (Gimp        *gimp,
                                                       GimpUnit     unit);

void           gimp_user_units_free                   (Gimp        *gimp);


#endif  /*  __APP_GIMP_UNIT_H__  */
