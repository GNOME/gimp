/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_PALETTE_LOAD_H__
#define __LIGMA_PALETTE_LOAD_H__


#define LIGMA_PALETTE_FILE_EXTENSION ".gpl"


typedef enum
{
  LIGMA_PALETTE_FILE_FORMAT_UNKNOWN,
  LIGMA_PALETTE_FILE_FORMAT_GPL,      /* LIGMA palette                        */
  LIGMA_PALETTE_FILE_FORMAT_RIFF_PAL, /* RIFF palette                        */
  LIGMA_PALETTE_FILE_FORMAT_ACT,      /* Photoshop binary color palette      */
  LIGMA_PALETTE_FILE_FORMAT_PSP_PAL,  /* JASC's Paint Shop Pro color palette */
  LIGMA_PALETTE_FILE_FORMAT_ACO,      /* Photoshop ACO color file            */
  LIGMA_PALETTE_FILE_FORMAT_CSS       /* Cascaded Stylesheet file (CSS)      */
} LigmaPaletteFileFormat;


GList               * ligma_palette_load               (LigmaContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * ligma_palette_load_act           (LigmaContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * ligma_palette_load_riff          (LigmaContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * ligma_palette_load_psp           (LigmaContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * ligma_palette_load_aco           (LigmaContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);
GList               * ligma_palette_load_css           (LigmaContext   *context,
                                                       GFile         *file,
                                                       GInputStream  *input,
                                                       GError       **error);

LigmaPaletteFileFormat ligma_palette_load_detect_format (GFile         *file,
                                                       GInputStream  *input);


#endif /* __LIGMA_PALETTE_H__ */
