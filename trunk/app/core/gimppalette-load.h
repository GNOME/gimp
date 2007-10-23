/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_PALETTE_LOAD_H__
#define __GIMP_PALETTE_LOAD_H__


#define GIMP_PALETTE_FILE_EXTENSION ".gpl"


typedef enum
{
  GIMP_PALETTE_FILE_FORMAT_UNKNOWN,
  GIMP_PALETTE_FILE_FORMAT_GPL,      /* GIMP palette                        */
  GIMP_PALETTE_FILE_FORMAT_RIFF_PAL, /* RIFF palette                        */
  GIMP_PALETTE_FILE_FORMAT_ACT,      /* Photoshop binary color palette      */
  GIMP_PALETTE_FILE_FORMAT_PSP_PAL,  /* JASC's Paint Shop Pro color palette */
  GIMP_PALETTE_FILE_FORMAT_ACO       /* Photoshop ACO color file            */
} GimpPaletteFileFormat;


GList               * gimp_palette_load               (const gchar  *filename,
                                                       GError      **error);
GList               * gimp_palette_load_act           (const gchar  *filename,
                                                       GError      **error);
GList               * gimp_palette_load_riff          (const gchar  *filename,
                                                       GError      **error);
GList               * gimp_palette_load_psp           (const gchar  *filename,
                                                       GError      **error);
GList               * gimp_palette_load_aco           (const gchar  *filename,
                                                       GError      **error);

GimpPaletteFileFormat gimp_palette_load_detect_format (const gchar  *filename);


#endif /* __GIMP_PALETTE_H__ */
