/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpwidgets-error.h
 * Copyright (C) 2008  Martin Nordholts <martinn@svn.gnome.org>
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

#ifndef __GIMP_WIDGETS_ERROR_H__
#define __GIMP_WIDGETS_ERROR_H__

G_BEGIN_DECLS


typedef enum
{
  GIMP_WIDGETS_PARSE_ERROR,
} GimpWidgetsError;


#define GIMP_WIDGETS_ERROR (gimp_widgets_error_quark ())

GQuark  gimp_widgets_error_quark (void) G_GNUC_CONST;


G_END_DECLS

#endif  /* __GIMP_WIDGETS_ERROR_H__ */
