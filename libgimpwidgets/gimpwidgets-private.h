/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpwidgets-private.h
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

#ifndef __GIMP_WIDGETS_PRIVATE_H__
#define __GIMP_WIDGETS_PRIVATE_H__


typedef gboolean (* GimpGetColorFunc)      (GimpRGB *color);
typedef void     (* GimpEnsureModulesFunc) (void);


extern GimpHelpFunc          _gimp_standard_help_func;
extern GimpGetColorFunc      _gimp_get_foreground_func;
extern GimpGetColorFunc      _gimp_get_background_func;
extern GimpEnsureModulesFunc _gimp_ensure_modules_func;


G_BEGIN_DECLS


void  gimp_widgets_init (GimpHelpFunc          standard_help_func,
                         GimpGetColorFunc      get_foreground_func,
                         GimpGetColorFunc      get_background_func,
                         GimpEnsureModulesFunc ensure_modules_func);


G_END_DECLS

#endif /* __GIMP_WIDGETS_PRIVATE_H__ */
