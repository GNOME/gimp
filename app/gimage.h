/* The GIMP -- an image manipulation program
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

#ifndef __GIMAGE_H__
#define __GIMAGE_H__


#include "gimpimage.h"


GImage * gimage_new                  (gint               width,
				      gint               height,
				      GimpImageBaseType  base_type);
void     gimage_delete               (GImage            *gimage);
void     gimage_invalidate_previews  (void);
void     gimage_set_layer_mask_apply (GImage            *gimage,
				      GimpLayer         *layer);
void     gimage_set_layer_mask_edit  (GImage            *gimage,
				      GimpLayer         *layer,
				      gboolean           edit);
void     gimage_set_layer_mask_show  (GImage            *gimage,
				      GimpLayer         *layer);
void     gimage_foreach              (GFunc              func,
				      gpointer           user_data);


extern guint32 next_guide_id;


#endif /* __GIMAGE_H__ */
