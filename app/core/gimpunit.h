/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __APP_LIGMA_UNIT_H__
#define __APP_LIGMA_UNIT_H__


gint          _ligma_unit_get_number_of_units          (Ligma        *ligma);
gint          _ligma_unit_get_number_of_built_in_units (Ligma        *ligma) G_GNUC_CONST;

LigmaUnit      _ligma_unit_new                          (Ligma        *ligma,
                                                       const gchar *identifier,
                                                       gdouble      factor,
                                                       gint         digits,
                                                       const gchar *symbol,
                                                       const gchar *abbreviation,
                                                       const gchar *singular,
                                                       const gchar *plural);

gboolean      _ligma_unit_get_deletion_flag            (Ligma        *ligma,
                                                       LigmaUnit     unit);
void          _ligma_unit_set_deletion_flag            (Ligma        *ligma,
                                                       LigmaUnit     unit,
                                                       gboolean     deletion_flag);

gdouble       _ligma_unit_get_factor                   (Ligma        *ligma,
                                                       LigmaUnit     unit);

gint          _ligma_unit_get_digits                   (Ligma        *ligma,
                                                       LigmaUnit     unit);

const gchar * _ligma_unit_get_identifier               (Ligma        *ligma,
                                                       LigmaUnit     unit);

const gchar * _ligma_unit_get_symbol                   (Ligma        *ligma,
                                                       LigmaUnit     unit);
const gchar * _ligma_unit_get_abbreviation             (Ligma        *ligma,
                                                       LigmaUnit     unit);
const gchar * _ligma_unit_get_singular                 (Ligma        *ligma,
                                                       LigmaUnit     unit);
const gchar * _ligma_unit_get_plural                   (Ligma        *ligma,
                                                       LigmaUnit     unit);

void           ligma_user_units_free                   (Ligma        *ligma);


#endif  /*  __APP_LIGMA_UNIT_H__  */
