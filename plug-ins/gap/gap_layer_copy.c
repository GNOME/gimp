/* gap_layer_copy.c
 *    by hof (Wolfgang Hofer)
 *
 */
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

/* revision history:
 * version 0.99.00 1999.03.03   hof: use the regular gimp_layer_copy and gimp_channel_copy
 *                                   (removed private variant)
 * version 0.98.00 1998.11.26   hof: added channel copy
 * version         1998.11.26   hof: bugfix have to copy the layer's layer_mask too.
 *                                          type check of destination image
 * version 0.96.00              hof: bugfix memory leak (must set src_tile to unref after use) 
 * version 0.93.01              hof: when creating the destination layer
 *                                   add alpha channel if needed in extra call 
 * version 0.90.00;             hof: 1.st (pre) release
 */
 
/* SYTEM (UNIX) includes */ 
/* GIMP includes */
/* GAP includes */
#include "gap_layer_copy.h"
#include "gap_pdb_calls.h"

extern      int gap_debug; /* ==0  ... dont print debug infos */

/* ============================================================================
 * p_my_layer_copy
 *    copy src_layer to the dst_image,
 *    return the id of the new created layer (the copy)
 * NOTE: source layer MUST have same type (bpp) for now
 *       it would be fine to extend the code to convert between any type
 * ============================================================================
 */
gint32 p_my_layer_copy (gint32 dst_image_id,
                        gint32 src_layer_id,
                        gdouble    opacity, /* 0.0 upto 100.0 */
                        GimpLayerModeEffects mode,
                        gint *src_offset_x,
                        gint *src_offset_y )
{
  gint32 l_new_layer_id;
  gint32 l_ret_id;     
  char  *l_name;
  GimpImageType  l_src_type;

  if(gap_debug) printf("GAP p_my_layer_copy: START\n");

  l_ret_id = -1;       /* prepare error retcode -1 */
  l_name = NULL;
  
  if(opacity > 99.99) opacity = 100.0;
  if(opacity < 0.0)   opacity = 0.0;
  
  l_name = gimp_layer_get_name(src_layer_id);
  l_src_type   = gimp_drawable_type(src_layer_id);

  switch(l_src_type)
  {
    case GIMP_RGB_IMAGE:         /* 0 */
    case GIMP_RGBA_IMAGE:        /* 1 */
       if(gimp_image_base_type(dst_image_id) != GIMP_RGB) { return -1; }
       break;
    case GIMP_GRAY_IMAGE:        /* 2 */
    case GIMP_GRAYA_IMAGE:       /* 3 */
       if(gimp_image_base_type(dst_image_id) != GIMP_GRAY) { return -1; }
       break;
    case GIMP_INDEXED_IMAGE:     /* 4 */
    case GIMP_INDEXEDA_IMAGE:    /* 5 */
       if(gimp_image_base_type(dst_image_id) != GIMP_INDEXED) { return -1; }
       break;
  }


  /* copy the layer */  
  l_new_layer_id = gimp_layer_copy(src_layer_id);

  if(l_new_layer_id >= 0)
  {
    if(p_gimp_drawable_set_image(l_new_layer_id, dst_image_id) >= 0)
    {
        if(! gimp_drawable_has_alpha(l_new_layer_id))
        {
           /* have to add alpha channel */
           gimp_layer_add_alpha(l_new_layer_id);
        }

        /* findout the offsets of the original layer within the source Image */
        gimp_drawable_offsets(src_layer_id, src_offset_x, src_offset_y );

        gimp_layer_set_name(l_new_layer_id, l_name);
        gimp_layer_set_opacity(l_new_layer_id, opacity);
        gimp_layer_set_mode(l_new_layer_id, mode);


        l_ret_id = l_new_layer_id;  /* all done OK */
    }

  }
    
  if(l_name != NULL) { g_free (l_name); }

  if(gap_debug) printf("GAP p_my_layer_copy: ret %d\n", (int)l_ret_id);

  return l_ret_id;
}	/* end p_my_layer_copy */


/* ============================================================================
 * p_my_channel_copy
 *   copy a channel to dst_IMAGE
 * ============================================================================
 */
gint32 p_my_channel_copy (gint32 dst_image_id,
                          gint32 src_channel_id)
{
  gint32 l_new_channel_id;
  gint32 l_ret_id;     
  char  *l_name;

  if(gap_debug) printf("GAP :p_my_channel_copy START\n");

  l_ret_id = -1;       /* prepare error retcode -1 */
  l_name = NULL;
  
  /* create new channel in destination image */
  l_name = gimp_channel_get_name(src_channel_id);

  /* copy the channel */
  l_new_channel_id = gimp_channel_copy(src_channel_id);
  if(l_new_channel_id >= 0)
  {
    if(p_gimp_drawable_set_image(l_new_channel_id, dst_image_id) >= 0)
    {
        gimp_channel_set_name(l_new_channel_id, l_name);
        l_ret_id = l_new_channel_id;  /* all done OK */  
    }
  }
  
  if(l_name != NULL) { g_free (l_name); }

  if(gap_debug) printf("GAP :p_my_channel_copy id=%d\n", (int)l_ret_id);

  return l_ret_id;
}	/* end p_my_channel_copy */
