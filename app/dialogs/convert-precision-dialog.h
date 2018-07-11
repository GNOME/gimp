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

#ifndef __CONVERT_PRECISION_DIALOG_H__
#define __CONVERT_PRECISION_DIALOG_H__


/*  Don't offer dithering when converting down to more than this
 *  number of bits per component. Note that gegl:dither would
 *  do 16 bit, so this is a limitation of the GUI to values that make
 *  sense. See bug #735895.
 */
#define CONVERT_PRECISION_DIALOG_MAX_DITHER_BITS 8


typedef void (* GimpConvertPrecisionCallback) (GtkWidget        *dialog,
                                               GimpImage        *image,
                                               GimpPrecision     precision,
                                               GeglDitherMethod  layer_dither_method,
                                               GeglDitherMethod  text_layer_dither_method,
                                               GeglDitherMethod  channel_dither_method,
                                               gpointer          user_data);


GtkWidget * convert_precision_dialog_new (GimpImage                    *image,
                                          GimpContext                  *context,
                                          GtkWidget                    *parent,
                                          GimpComponentType             component_type,
                                          GeglDitherMethod              layer_dither_method,
                                          GeglDitherMethod              text_layer_dither_method,
                                          GeglDitherMethod              channel_dither_method,
                                          GimpConvertPrecisionCallback  callback,
                                          gpointer                      user_data);


#endif  /*  __CONVERT_PRECISION_DIALOG_H__  */
