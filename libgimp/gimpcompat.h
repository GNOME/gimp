/* LIBGIMP - The GIMP Library 
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

#define gimp_channel_width              gimp_drawable_width
#define gimp_channel_height             gimp_drawable_height
#define gimp_channel_get_image_ID       gimp_drawable_get_image_ID
#define gimp_channel_get_layer_ID       -1

#define gimp_layer_width                gimp_drawable_width
#define gimp_layer_height               gimp_drawable_height
#define gimp_layer_bpp                  gimp_drawable_bpp
#define gimp_layer_type                 gimp_drawable_type

#define gimp_plugin_help_func           gimp_standard_help_func

#define Parasite                        GimpParasite
#define PARASITE_PERSISTENT             GIMP_PARASITE_PERSISTENT
#define PARASITE_UNDOABLE               GIMP_PARASITE_UNDOABLE
#define PARASITE_ATTACH_PARENT          GIMP_PARASITE_ATTACH_PARENT
#define PARASITE_PARENT_PERSISTENT      GIMP_PARASITE_PARENT_PERSISTENT
#define PARASITE_PARENT_UNDOABLE        GIMP_PARASITE_PARENT_UNDOABLE
#define PARASITE_ATTACH_GRANDPARENT     GIMP_PARASITE_ATTACH_GRANDPARENT
#define PARASITE_GRANDPARENT_PERSISTENT GIMP_PARASITE_GRANDPARENT_PERSISTENT
#define PARASITE_GRANDPARENT_UNDOABLE   GIMP_PARASITE_GRANDPARENT_UNDOABLE
#define parasite_new                    gimp_parasite_new
#define parasite_free                   gimp_parasite_free
#define parasite_copy                   gimp_parasite_copy
#define parasite_compare                gimp_parasite_compare
#define parasite_is_type                gimp_parasite_is_type
#define parasite_is_persistent          gimp_parasite_is_persistent
#define parasite_is_undoable            gimp_parasite_is_undoable
#define parasite_has_flag               gimp_parasite_has_flag
#define parasite_flags                  gimp_parasite_flags
#define parasite_name                   gimp_parasite_name
#define parasite_data                   gimp_parasite_data
#define parasite_data_size              gimp_parasite_data_size
#define PIXPIPE_MAXDIM                  GIMP_PIXPIPE_MAXDIM
#define PixPipeParams                   GimpPixPipeParams
#define pixpipeparams_init              gimp_pixpipe_params_init
#define pixpipeparams_parse             gimp_pixpipe_params_parse
#define pixpipeparams_build             gimp_pixpipe_params_build

#define GPlugInInfo  GimpPlugInInfo
#define GTile        GimpTile
#define GDrawable    GimpDrawable
#define GPixelRgn    GimpPixelRgn
#define GParamColor  GimpParamColor
#define GParamRegion GimpParamRegion
#define GParamData   GimpParamData
#define GParamDef    GimpParamDef
#define GParam       GimpParam

#endif /* GIMP_DISABLE_COMPAT_H */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMPCOMPAT_H__ */
