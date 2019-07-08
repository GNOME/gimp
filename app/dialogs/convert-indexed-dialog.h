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

#ifndef __CONVERT_INDEXED_DIALOG_H__
#define __CONVERT_INDEXED_DIALOG_H__


typedef void (* GimpConvertIndexedCallback) (GtkWidget              *dialog,
                                             GimpImage              *image,
                                             GimpConvertPaletteType  palette_type,
                                             gint                    max_colors,
                                             gboolean                remove_duplicates,
                                             GimpConvertDitherType   dither_type,
                                             gboolean                dither_alpha,
                                             gboolean                dither_text_layers,
                                             GimpPalette            *custom_palette,
                                             gpointer                user_data);


GtkWidget * convert_indexed_dialog_new (GimpImage                  *image,
                                        GimpContext                *context,
                                        GtkWidget                  *parent,
                                        GimpConvertPaletteType      palette_type,
                                        gint                        max_colors,
                                        gboolean                    remove_duplicates,
                                        GimpConvertDitherType       dither_type,
                                        gboolean                    dither_alpha,
                                        gboolean                    dither_text_layers,
                                        GimpPalette                *custom_palette,
                                        GimpConvertIndexedCallback  callback,
                                        gpointer                    user_data);


#endif  /*  __CONVERT_INDEXED_DIALOG_H__  */
