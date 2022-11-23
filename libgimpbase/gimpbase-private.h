/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmabase-private.h
 * Copyright (C) 2003 Sven Neumann <sven@ligma.org>
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
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_BASE_PRIVATE_H__
#define __LIGMA_BASE_PRIVATE_H__


typedef struct _LigmaUnitVtable LigmaUnitVtable;

struct _LigmaUnitVtable
{
  gint          (* unit_get_number_of_units)          (void);
  gint          (* unit_get_number_of_built_in_units) (void);

  LigmaUnit      (* unit_new)                          (gchar    *identifier,
                                                       gdouble   factor,
                                                       gint      digits,
                                                       gchar    *symbol,
                                                       gchar    *abbreviation,
                                                       gchar    *singular,
                                                       gchar    *plural);
  gboolean      (* unit_get_deletion_flag)            (LigmaUnit  unit);
  void          (* unit_set_deletion_flag)            (LigmaUnit  unit,
                                                       gboolean  deletion_flag);

  gdouble       (* unit_get_factor)                   (LigmaUnit  unit);
  gint          (* unit_get_digits)                   (LigmaUnit  unit);
  const gchar * (* unit_get_identifier)               (LigmaUnit  unit);
  const gchar * (* unit_get_symbol)                   (LigmaUnit  unit);
  const gchar * (* unit_get_abbreviation)             (LigmaUnit  unit);
  const gchar * (* unit_get_singular)                 (LigmaUnit  unit);
  const gchar * (* unit_get_plural)                   (LigmaUnit  unit);

  void          (* _reserved_1)                       (void);
  void          (* _reserved_2)                       (void);
  void          (* _reserved_3)                       (void);
  void          (* _reserved_4)                       (void);
};


extern LigmaUnitVtable _ligma_unit_vtable;


G_BEGIN_DECLS

void   ligma_base_init              (LigmaUnitVtable *vtable);
void   ligma_base_compat_enums_init (void);

G_END_DECLS

#endif /* __LIGMA_BASE_PRIVATE_H__ */
