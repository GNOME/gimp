/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpVectorLayer-xcf
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
 * Copyright (C) 2006  Hendrik Boom
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

#ifndef __GIMP_VECTOR_LAYER_XCF_H__
#define __GIMP_VECTOR_LAYER_XCF_H__


const gchar * gimp_vector_layer_vector_parasite_name (void) G_GNUC_CONST;
const gchar * gimp_vector_layer_fill_parasite_name   (void) G_GNUC_CONST;
const gchar * gimp_vector_layer_stroke_parasite_name (void) G_GNUC_CONST;
gboolean      gimp_vector_layer_xcf_load_hack        (GimpLayer       **layer);
void          gimp_vector_layer_xcf_save_prepare     (GimpVectorLayer  *layer);


#endif /* __GIMP_VECTOR_LAYER_XCF_H__ */
