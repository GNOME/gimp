/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpwidgets-private.h
 * Copyright (C) 2003 Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_WIDGETS_PRIVATE_H__
#define __GIMP_WIDGETS_PRIVATE_H__


typedef gboolean (* GimpGetColorFunc)      (GimpRGB *color);
typedef void     (* GimpEnsureModulesFunc) (void);


typedef struct _GimpWidgetsVTable GimpWidgetsVTable;

struct _GimpWidgetsVTable
{
  gint          (* unit_get_number_of_units)          (void);
  gint          (* unit_get_number_of_built_in_units) (void);
  gdouble       (* unit_get_factor)                   (GimpUnit  unit);
  gint          (* unit_get_digits)                   (GimpUnit  unit);
  const gchar * (* unit_get_identifier)               (GimpUnit  unit);
  const gchar * (* unit_get_symbol)                   (GimpUnit  unit);
  const gchar * (* unit_get_abbreviation)             (GimpUnit  unit);
  const gchar * (* unit_get_singular)                 (GimpUnit  unit);
  const gchar * (* unit_get_plural)                   (GimpUnit  unit);

  void          (* _reserved_1)                       (void);
  void          (* _reserved_2)                       (void);
  void          (* _reserved_3)                       (void);
  void          (* _reserved_4)                       (void);
};


extern GimpWidgetsVTable     _gimp_eek;
extern GimpHelpFunc          _gimp_standard_help_func;
extern GimpGetColorFunc      _gimp_get_foreground_func;
extern GimpGetColorFunc      _gimp_get_background_func;
extern GimpEnsureModulesFunc _gimp_ensure_modules_func;


G_BEGIN_DECLS


void  gimp_widgets_init (GimpWidgetsVTable    *vtable,
                         GimpHelpFunc          standard_help_func,
                         GimpGetColorFunc      get_foreground_func,
                         GimpGetColorFunc      get_background_func,
                         GimpEnsureModulesFunc ensure_modules_func);


G_END_DECLS

#endif /* __GIMP_WIDGETS_PRIVATE_H__ */
