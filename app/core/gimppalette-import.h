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

#ifndef __GIMP_PALETTE_IMPORT__
#define __GIMP_PALETTE_IMPORT__


GimpPalette * gimp_palette_import_from_gradient      (GimpGradient *gradient,
                                                      GimpContext  *context,
                                                      gboolean      reverse,
                                                      const gchar  *palette_name,
                                                      gint          n_colors);
GimpPalette * gimp_palette_import_from_image         (GimpImage    *image,
                                                      const gchar  *palette_name,
                                                      gint          n_colors,
                                                      gint          treshold,
                                                      gboolean      selection_only);
GimpPalette * gimp_palette_import_from_indexed_image (GimpImage    *image,
                                                      const gchar  *palette_name);
GimpPalette * gimp_palette_import_from_drawable      (GimpDrawable *drawable,
                                                      const gchar  *palette_name,
                                                      gint          n_colors,
                                                      gint          threshold,
                                                      gboolean      selection_only);
GimpPalette * gimp_palette_import_from_file          (const gchar  *filename,
                                                      const gchar  *palette_name,
                                                      GError      **error);

#endif  /* __GIMP_PALETTE_IMPORT_H__ */
