/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * file-icns-export.h
 * Copyright (C) 2004 Brion Vibber <brion@pobox.com>
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

#ifndef __ICNS_EXPORT_H__
#define __ICNS_EXPORT_H__


GimpPDBStatusType icns_save_image (GFile                *file,
                                   GimpImage            *image,
                                   GimpProcedure        *procedure,
                                   GimpProcedureConfig  *config,
                                   gint32                run_mode,
                                   GError              **error);

#endif /* __ICNS_EXPORT_H__ */
