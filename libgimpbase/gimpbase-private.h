/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpbase-private.h
 * Copyright (C) 2003 Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_BASE_PRIVATE_H__
#define __GIMP_BASE_PRIVATE_H__


#include <libgimpbase/gimpcpuaccel-private.h>
#include <libgimpbase/gimpenv-private.h>

#ifndef G_OS_WIN32
#include <libgimpbase/gimpsignal.h>
#endif


typedef struct _GimpUnitVtable GimpUnitVtable;

struct _GimpUnitVtable
{
  /* These methods MUST only be set on libgimp, not in core app. */
  gboolean      (* get_deletion_flag)                 (GimpUnit  *unit);
  gboolean      (* set_deletion_flag)                 (GimpUnit  *unit,
                                                       gboolean   deletion_flag);
  gchar       * (* get_data)                          (gint       unit_id,
                                                       gdouble   *factor,
                                                       gint      *digits,
                                                       gchar    **symbol,
                                                       gchar    **abbreviation);

  /* These methods MUST only be set on app, not in libgimp. */
  GimpUnit    * (* get_user_unit)                     (gint       unit_id);


  /* Reserved methods. */
  void          (* _reserved_0)                       (void);
  void          (* _reserved_1)                       (void);
  void          (* _reserved_2)                       (void);
  void          (* _reserved_3)                       (void);
  void          (* _reserved_4)                       (void);
  void          (* _reserved_5)                       (void);
  void          (* _reserved_6)                       (void);
  void          (* _reserved_7)                       (void);
  void          (* _reserved_8)                       (void);
  void          (* _reserved_9)                       (void);
};


extern GimpUnitVtable  _gimp_unit_vtable;
extern GHashTable     *_gimp_units;


G_BEGIN_DECLS

void   gimp_base_init              (GimpUnitVtable *vtable);
void   gimp_base_exit              (void);
void   gimp_base_compat_enums_init (void);

G_END_DECLS

#endif /* __GIMP_BASE_PRIVATE_H__ */
