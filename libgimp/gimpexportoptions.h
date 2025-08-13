/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * gimpexportoptions.h
 * Copyright (C) 2024 Alx Sa.
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

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __LIBGIMP_GIMP_EXPORT_OPTIONS_H__
#define __LIBGIMP_GIMP_EXPORT_OPTIONS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * GimpExportReturn:
 * @GIMP_EXPORT_IGNORE: The image is unmodified but export shall continue anyway
 * @GIMP_EXPORT_EXPORT: The chosen transforms were applied to a new image
 *
 * Possible return values of [method@ExportOptions.get_image].
 **/
typedef enum
{
  GIMP_EXPORT_IGNORE,
  GIMP_EXPORT_EXPORT
} GimpExportReturn;


GimpExportReturn    gimp_export_options_get_image     (GimpExportOptions  *options,
                                                       GimpImage         **image) G_GNUC_WARN_UNUSED_RESULT;


G_END_DECLS


#endif /* __LIBGIMP_GIMP_EXPORT_OPTIONS_H__ */
