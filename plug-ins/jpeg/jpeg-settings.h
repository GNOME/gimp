/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * jpeg-settings.h
 * Copyright (C) 2007 Raphaël Quinet <raphael@gimp.org>
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

gboolean  jpeg_detect_original_settings  (struct jpeg_decompress_struct *cinfo,
                                          gint32    image_ID);

gboolean  jpeg_restore_original_settings (gint32    image_ID,
                                          gint     *quality,
                                          gint     *subsmp,
                                          gint     *num_quant_tables);

guint   **jpeg_restore_original_tables   (gint32    image_ID,
                                          gint      num_quant_tables);
