/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_CONFIG_ERROR_H__
#define __GIMP_CONFIG_ERROR_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


typedef enum
{
  GIMP_CONFIG_ERROR_OPEN,         /*  open failed                            */
  GIMP_CONFIG_ERROR_OPEN_ENOENT,  /*  file does not exist                    */
  GIMP_CONFIG_ERROR_WRITE,        /*  write failed                           */
  GIMP_CONFIG_ERROR_PARSE,        /*  parser error                           */
  GIMP_CONFIG_ERROR_VERSION       /*  parser failed due to version mismatch  */
} GimpConfigError;

#define GIMP_CONFIG_ERROR (gimp_config_error_quark ())

GQuark        gimp_config_error_quark   (void) G_GNUC_CONST;


G_END_DECLS

#endif  /* __GIMP_CONFIG_ERROR_H__ */
