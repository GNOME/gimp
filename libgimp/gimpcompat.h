/* 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */                                                                             
#ifndef __GIMPCOMPAT_H__
#define __GIMPCOMPAT_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* some compatibility defines for older plug-ins */

#ifndef GIMP_DISABLE_COMPAT_H

#define gimp_attach_parasite		gimp_parasite_attach
#define gimp_detach_parasite		gimp_parasite_detach
#define gimp_find_parasite		gimp_parasite_find
#define gimp_image_attach_parasite	gimp_image_parasite_attach
#define gimp_image_detach_parasite	gimp_image_parasite_detach
#define gimp_image_find_parasite	gimp_image_parasite_find
#define gimp_drawable_attach_parasite	gimp_drawable_parasite_attach
#define gimp_drawable_detach_parasite	gimp_drawable_parasite_detach
#define gimp_drawable_find_parasite	gimp_drawable_parasite_find
#define gimp_drawable_channel		gimp_drawable_is_channel
#define gimp_drawable_gray		gimp_drawable_is_gray
#define gimp_drawable_color		gimp_drawable_is_rgb
#define gimp_drawable_indexed		gimp_drawable_is_indexed
#define gimp_drawable_layer		gimp_drawable_is_layer
#define gimp_drawable_layer_mask	gimp_drawable_is_layer_mask
#define gimp_image_disable_undo		gimp_image_undo_disable
#define gimp_image_enable_undo		gimp_image_undo_enable
#define gimp_image_freeze_undo		gimp_image_undo_freeze
#define gimp_image_thaw_undo		gimp_image_undo_thaw

/*
#define gimp_get_data 			gimp_procedural_db_get_data
#define gimp_get_data_size		gimp_procedural_db_get_data_size
 
#define gimp_set_data(id, data, bytes) \
	(gimp_procedural_db_set_data (id, bytes, data))

#define gimp_query_images		gimp_image_list
*/

#endif /* GIMP_DISABLE_COMPAT_H */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMPCOMPAT_H__ */
