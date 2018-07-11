/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpextension-error.h
 * Copyright (C) 2018 Jehan <jehan@gimp.org>
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

#ifndef __GIMP_EXTENSION_ERROR_H__
#define __GIMP_EXTENSION_ERROR_H__


typedef enum
{
  /* Generic error condition. */
  GIMP_EXTENSION_FAILED,
  GIMP_EXTENSION_BAD_APPDATA,
  GIMP_EXTENSION_BAD_ID,
  GIMP_EXTENSION_NO_VERSION,
  GIMP_EXTENSION_BAD_PATH
} GimpExtensionErrorCode;


#define GIMP_EXTENSION_ERROR (gimp_extension_error_quark ())

GQuark  gimp_extension_error_quark (void) G_GNUC_CONST;


#endif /* __GIMP_EXTENSION_ERROR_H__ */

