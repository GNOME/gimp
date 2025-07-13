/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#pragma once


#define GIMP_PALETTE_FILE_EXTENSION ".gpl"


typedef enum
{
  GIMP_PALETTE_FILE_FORMAT_UNKNOWN,
  GIMP_PALETTE_FILE_FORMAT_GPL,      /* GIMP palette                        */
  GIMP_PALETTE_FILE_FORMAT_RIFF_PAL, /* RIFF palette                        */
  GIMP_PALETTE_FILE_FORMAT_ACT,      /* Photoshop binary color palette      */
  GIMP_PALETTE_FILE_FORMAT_PSP_PAL,  /* JASC's Paint Shop Pro color palette */
  GIMP_PALETTE_FILE_FORMAT_ACO,      /* Photoshop ACO color file            */
  GIMP_PALETTE_FILE_FORMAT_ACB,      /* Photoshop ACB color book            */
  GIMP_PALETTE_FILE_FORMAT_ASE,      /* Photoshop ASE color palette         */
  GIMP_PALETTE_FILE_FORMAT_CSS,      /* Cascaded Stylesheet file (CSS)      */
  GIMP_PALETTE_FILE_FORMAT_SBZ       /* Swatchbooker SBZ file               */
} GimpPaletteFileFormat;


GList               * gimp_palette_load               (GimpContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * gimp_palette_load_act           (GimpContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * gimp_palette_load_riff          (GimpContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * gimp_palette_load_psp           (GimpContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * gimp_palette_load_aco           (GimpContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * gimp_palette_load_acb           (GimpContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * gimp_palette_load_ase           (GimpContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * gimp_palette_load_css           (GimpContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * gimp_palette_load_sbz           (GimpContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);

GimpPaletteFileFormat gimp_palette_load_detect_format (GFile         *file,
                                                       GInputStream  *input);
