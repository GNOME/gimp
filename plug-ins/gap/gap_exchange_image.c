/* gap_exchange_image.c
 *   by hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * basic anim functions:
 *   This Plugin drops the content of the destination
 *   image (all layers,channels, paths, parasites & guides)
 *   and then moves (steal) the content of a source image to dst. image
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
 * 1.1.20a  2000/04/29   hof: bugfix in p_steal_content:  we must copy cmap before removing layers
 *                            from src_image to avoid crash on indexed frames.
 * 1.1.16a  2000/02/05   hof: handle path lockedstaus and image unit
 * 1.1.15b  2000/01/30   hof: handle image specific parasites
 * 1.1.15a  2000/01/25   hof: stopped gimp 1.0.x support (removed p_copy_content)
 *                            handle pathes
 * 0.98.00; 1998/11/30   hof: 1.st release
 *                             (substitute for the procedure "gimp_duplicate_into"
 *                              that was never part of the GIMP core)
 */
 
/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap_layer_copy.h"
#include "gap_pdb_calls.h"
#include "gap_exchange_image.h"
#include "gap_lib.h"


extern      int gap_debug; /* ==0  ... dont print debug infos */


/* ============================================================================
 * p_steal_content
 *   
 *   steal or copy all elements
 *     - layers, 
 *     - channels, selections, 
 *     - pathes,
 *     - guides, 
 *     - colormap
 *     from src_image and add them to dst_image.
 * ============================================================================
 */

static int
p_steal_content(gint32 dst_image_id, gint32 src_image_id)
{
   int     l_rc;
   int     l_idx;
   
   gint    l_nlayers;
   gint    l_nchannels;
   gint32 *l_layers_list;
   gint32 *l_channels_list;
   gint32  l_layer_id;
   gint32  l_channel_id;
   gint32  l_new_channel_id;
   gint32  l_layer_mask_id;
   gint32  l_guide_id;
   gint32  l_src_fsel_id;                 /* floating selection (in the src_image) */
   gint32  l_src_fsel_attached_to_id;     /* the drawable where floating selection is attached to (in the src_image) */
   gint32  l_fsel_attached_to_id;         /* the drawable id where to attach the floating selection (dst) */
   gint32  l_fsel_id;                     /* the drawable id of the floating selection itself (dst)  */
   gint32  l_active_layer_id;
   gint32  l_active_channel_id;
   gint32  l_x1, l_x2, l_y1, l_y2;
   guchar *l_cmap;
   gint    l_ncolors;
   gchar   **l_path_names;
   gchar    *l_current_pathname;
   gint32   l_num_paths;
   gdouble *l_path_points;
   gint32   l_path_type;
   gint32   l_path_closed;
   gint32   l_num_points;
   GimpParasite  *l_parasite;
   gchar    **l_parasite_names = NULL;
   gint32     l_num_parasites = 0;
 
   l_rc = -1;  /* init retcode to Errorstate */
   l_layers_list = NULL;
   l_channels_list = NULL;
   l_active_layer_id = -1;
   l_active_channel_id = -1;
   l_fsel_attached_to_id = -1;    /* -1  assume fsel is not available (and not attached to any drawable) */
   l_fsel_id = -1;                /* -1  assume there is no floating selection */


   if(gap_debug) printf("GAP-DEBUG: START p_steal_content dst_id=%d src_id=%d\n", (int)dst_image_id, (int)src_image_id);

   /* gimp_image_undo_disable (src_image_id); */ /* does not work !! if active we can not steal layers */

   /*  Copy the colormap if necessary  */
   if(gimp_image_base_type(src_image_id) == GIMP_INDEXED)
   {
       l_cmap = gimp_image_get_cmap (src_image_id, &l_ncolors);

       if(gap_debug) printf("GAP-DEBUG: copy colormap ncolors %d\n", (int)l_ncolors);
       gimp_image_set_cmap(dst_image_id, l_cmap, l_ncolors);
   }

   
   /* check for floating selection */
   l_src_fsel_attached_to_id = -1;
   l_src_fsel_id = gimp_image_floating_selection(src_image_id);
   if(l_src_fsel_id >= 0)
   {
       if(gap_debug) printf("GAP-DEBUG: call floating_sel_relax fsel_id=%d\n",
                           (int)l_src_fsel_id);
       
       p_gimp_floating_sel_relax (l_src_fsel_id, FALSE);
       l_src_fsel_attached_to_id = p_gimp_image_floating_sel_attached_to(src_image_id);
   }
  
   /* steal all layers */
   l_layers_list = gimp_image_get_layers(src_image_id, &l_nlayers);

   /* foreach layer do */
   for(l_idx = l_nlayers -1; l_idx >= 0; l_idx--)
   {
     l_layer_id = l_layers_list[l_idx];
     if(gap_debug) printf("GAP-DEBUG: START p_steal_content layer_id[%d]=%d\n", (int)l_idx, (int)l_layer_id);


     if(l_layer_id  == gimp_image_get_active_layer(src_image_id))
     {
        l_active_layer_id = l_layer_id;
     }

     l_layer_mask_id = gimp_layer_get_mask_id(l_layer_id);
     if(l_layer_mask_id >= 0)
     {
        /* layer has layermask */
        if(gap_debug) printf("GAP-DEBUG: START p_steal_content layer_mask_id=%d\n", (int)l_layer_mask_id);
       
        /* check for floating selection */
        if(l_layer_mask_id == l_src_fsel_attached_to_id)
        {
           l_fsel_attached_to_id = l_layer_mask_id;   /* the floating selection is attached to this layer_mask */
        }

        p_gimp_drawable_set_image(l_layer_mask_id, dst_image_id);
     }

     /* remove layer from source */
     gimp_image_remove_layer(src_image_id, l_layer_id);

     /* and set the dst_image as it's new Master */
     p_gimp_drawable_set_image(l_layer_id, dst_image_id);

     if(l_layer_id == l_src_fsel_id)
     {
        l_fsel_id = l_layer_id;    /* this layer is the floating selection */
     }
     else
     {
        if(gap_debug) printf("GAP-DEBUG: START p_steal_content add_layer_id=%d\n", (int)l_layer_id);
        /* add the layer on top of the images layerstak */
        gimp_image_add_layer (dst_image_id, l_layer_id, 0);
        
        if(l_layer_id == l_src_fsel_attached_to_id)
        {
           l_fsel_attached_to_id = l_layer_id;    /* the floating selection is attached to this layer */
        }
     }

   }  /* end foreach layer */
   
   /* steal all channels */ 
 
   l_channels_list = gimp_image_get_channels(src_image_id, &l_nchannels);
   
   /* foreach channel do */
   for(l_idx = l_nchannels -1; l_idx >= 0; l_idx--)
   {
      l_channel_id = l_channels_list[l_idx];
      if(gap_debug) printf("GAP-DEBUG: START p_steal_content channel_id=%d\n", (int)l_channel_id);

      l_new_channel_id = p_my_channel_copy(dst_image_id, l_channel_id);
                       
      if (l_new_channel_id < 0)  { goto cleanup; }
                      

       /* add channel on top of the channelstack */
       gimp_image_add_channel (dst_image_id, l_new_channel_id, 0);

       /* adjust channelproperties */
       gimp_channel_set_visible (l_new_channel_id, gimp_channel_get_visible(l_channel_id));
       gimp_channel_set_show_masked (l_new_channel_id, gimp_channel_get_show_masked(l_channel_id));

       if(l_channel_id == l_src_fsel_attached_to_id)
       {
          l_fsel_attached_to_id = l_channel_id;    /* the floating_selection is attached to this channel */
       }

       if(l_channel_id == gimp_image_get_active_channel(src_image_id))
       {
          l_active_channel_id = l_channel_id;
       }

       /* remove channel from source */
       gimp_image_remove_channel(src_image_id, l_channel_id);

       /* and set the dst_image as it's new Master */
       p_gimp_drawable_set_image(l_channel_id, dst_image_id);
       
   }      /* end foreach channel */

   /* check and see if we have to copy the selection */
   l_channel_id = gimp_image_get_selection(src_image_id);
   if((p_get_gimp_selection_bounds(src_image_id, &l_x1, &l_y1, &l_x2, &l_y2))
   && (l_channel_id >= 0))
   {
       if(gap_debug) printf("GAP-DEBUG: START p_steal_content selection_channel_id=%d\n", (int)l_channel_id);
              
       p_gimp_drawable_set_image(l_channel_id, dst_image_id);
       p_gimp_selection_load (l_channel_id);
	
   }


   /* attach the floating selection... */
   if((l_fsel_id >= 0) && (l_fsel_attached_to_id >= 0))
   {
      if(gap_debug) printf("GAP-DEBUG: attaching floating_selection id=%d to id %d\n",
                    (int)l_fsel_id, (int)l_fsel_attached_to_id);
      if(p_gimp_floating_sel_attach (l_fsel_id, l_fsel_attached_to_id) < 0)
      {
         /* in case of error add floating_selection like an ordinary layer
	  * (if patches are not installed you'll get the error for sure)
	  */
         printf("GAP: floating_selection is added as top-layer (attach failed)\n");
         gimp_image_add_layer (dst_image_id, l_fsel_id, 0);

      }
   }

   /* set active layer/channel */
   if(l_active_channel_id >= 0)
   {
       if(gap_debug) printf("GAP-DEBUG: SET active channel %d\n", (int)l_active_channel_id);     
       gimp_image_set_active_channel(dst_image_id, l_active_channel_id);
   }
   if(l_active_layer_id >= 0)
   {
       if(gap_debug) printf("GAP-DEBUG: SET active layer %d\n", (int)l_active_layer_id);     
       gimp_image_set_active_layer(dst_image_id, l_active_layer_id);
   }

   /* copy guides */
   l_guide_id = p_gimp_image_findnext_guide(src_image_id, 0);  /* get 1.st guide */
   while(l_guide_id > 0)
   {
      /* get position and orientation for the current guide ID */
      p_gimp_image_add_guide(dst_image_id, 
                             p_gimp_image_get_guide_position(src_image_id, l_guide_id),
                             p_gimp_image_get_guide_orientation(src_image_id, l_guide_id)
                             );
      l_guide_id = p_gimp_image_findnext_guide(src_image_id, l_guide_id);
   }

   /* copy paths */
   l_path_names = p_gimp_path_list(src_image_id, &l_num_paths);
   if((l_path_names != NULL) && (l_num_paths > 0))
   {
      l_current_pathname = p_gimp_path_get_current(src_image_id);
      for(l_idx=l_num_paths-1; l_idx >= 0; l_idx--)
      {
	 l_path_points = p_gimp_path_get_points(src_image_id, l_path_names[l_idx],
	                           &l_path_type, &l_path_closed, &l_num_points);
         if((l_path_points != NULL) && (l_num_points > 0))
	 {
	    p_gimp_path_set_points(dst_image_id, l_path_names[l_idx],
	                           l_path_type, l_num_points, l_path_points);
            p_gimp_path_set_locked(dst_image_id, l_path_names[l_idx],
	                           p_gimp_path_get_locked(src_image_id, l_path_names[l_idx]));
 	 }
                  
         if(l_path_points)  g_free(l_path_points);         
	 
	 g_free(l_path_names[l_idx]);
      }
      if(l_current_pathname)
      {
         p_gimp_path_set_current(dst_image_id, l_current_pathname);
         g_free(l_current_pathname);
      }
   }
   if(l_path_names) g_free(l_path_names);

   /* copy image specific parasites
    *  (drawable specific parasites are handled implicite
    *   by stealing layers and channels)
    */
   l_parasite_names = p_gimp_image_parasite_list (src_image_id, &l_num_parasites);

   for(l_idx = 0; l_idx < l_num_parasites; l_idx++)
   {
     l_parasite = gimp_image_parasite_find(src_image_id, l_parasite_names[l_idx]);
     if(l_parasite)
     {
	if(gap_debug) printf("copy image_parasite NAME:%s:\n",  l_parasite_names[l_idx]);

	gimp_image_attach_new_parasite(dst_image_id,
                                	l_parasite->name,
					l_parasite->flags,
					l_parasite->size,
					l_parasite->data);
	if(l_parasite->data) g_free(l_parasite->data);
	if(l_parasite->name) g_free(l_parasite->name);
	g_free(l_parasite);
     }
     g_free(l_parasite_names[l_idx]);
   }
   g_free(l_parasite_names);

   /* copy the image unit */ 
   gimp_image_set_unit(dst_image_id, 
                       gimp_image_get_unit(src_image_id));

   l_rc = 0;
   
cleanup:
   if(l_layers_list) g_free (l_layers_list);
   if(l_channels_list) g_free (l_channels_list);
   
   if(gap_debug) printf("GAP-DEBUG: END p_steal_content dst_id=%d src_id=%d  rc=%d\n",
                 (int)dst_image_id, (int)src_image_id, l_rc);
   return (l_rc);            /* 0 .. OK, or -1 on error */
}   /* end p_steal_content */


/* ============================================================================
 * p_replace_img
 *   
 *   This procedure replaces the content of image_id 
 *     with the content from src_image_id.
 *     By stealing all its layers and channels.
 * ============================================================================
 */

static int
p_replace_img(gint32 image_id, gint32 src_image_id)
{
   int     l_rc;
   int     l_idx;
   gint    l_nlayers;
   gint    l_nchannels;
   gint32 *l_layers_list;
   gint32 *l_channels_list;
   gint32  l_layer_id;
   gint32  l_channel_id;
   gint32  l_guide_id;
   gint32  l_old_bg_layer_id;
   gchar  **l_path_names;
   gint32   l_num_paths;
   gchar    **l_parasite_names = NULL;
   gint32     l_num_parasites = 0;

   if(gap_debug) printf("\nGAP-DEBUG: START p_replace_img img_id=%d \n", (int)image_id);

   l_old_bg_layer_id = -1;

   gimp_image_undo_disable (image_id);
   
   /* remove selection (if there is any) */
   p_gimp_selection_none(image_id);

   l_channels_list = gimp_image_get_channels(image_id, &l_nchannels);
   
   /* foreach channel do */
   for(l_idx = 0; l_idx < l_nchannels; l_idx++)
   {
      l_channel_id = l_channels_list[l_idx];
      gimp_image_remove_channel(image_id, l_channel_id);
      gimp_channel_delete(l_channel_id);
   }
   if(l_channels_list) { g_free (l_channels_list); }


   /* delete guides */
   l_guide_id = p_gimp_image_findnext_guide(image_id, 0);  /* get 1.st guide */
   while(l_guide_id > 0)
   {
      /* delete guide ID */
      p_gimp_image_delete_guide(image_id, l_guide_id);
      
      /* get 1.st (of the remaining) guides */
      l_guide_id = p_gimp_image_findnext_guide(image_id, 0); 
   }

   /* delete paths */
   l_path_names = p_gimp_path_list(image_id, &l_num_paths);
   if((l_path_names != NULL) && (l_num_paths > 0))
   {
      for(l_idx=0; l_idx < l_num_paths; l_idx++)
      {
	 p_gimp_path_delete(image_id, l_path_names[l_idx]);
	 g_free(l_path_names[l_idx]);
      }
   }
   if(l_path_names) g_free(l_path_names);

   /* remove image specific parasites */   
   l_parasite_names = p_gimp_image_parasite_list (image_id, &l_num_parasites);
   if(l_parasite_names)
   {
     for(l_idx = 0; l_idx < l_num_parasites; l_idx++)
     {
       if(gap_debug) printf("detach image_parasite NAME:%s:\n",  l_parasite_names[l_idx]);
       gimp_image_parasite_detach(image_id, l_parasite_names[l_idx]);
       g_free(l_parasite_names[l_idx]);
     }
     g_free(l_parasite_names);
   }

   /* get list of all (old) dst_layers to delete */
   l_layers_list = gimp_image_get_layers(image_id, &l_nlayers);

   /* foreach (old) layer of the do */
   for(l_idx = 0; l_idx < l_nlayers; l_idx++)
   {
     l_layer_id = l_layers_list[l_idx];
     
     if(l_idx == l_nlayers -1)
     {
        /* the background layer is renamed
         * (if not GIMP 1.1 will rename the new imported 
         *  Background Layer as "background#2")
         * and deleted later.
         * Important: the dst_image_id should not be completely empty.
         *            anytime it may happen, that its display(s) is (are) updated
         *            and this may crash on empty images.
         */
        gimp_layer_set_name(l_layer_id, "--old-bg-layer-will-die-soon");
        l_old_bg_layer_id = l_layer_id;
     }
     else
     {
        if(gap_debug) printf("GAP-DEBUG: p_replace_img del layer_id=%d \n", (int)l_layer_id);
        gimp_image_remove_layer(image_id, l_layer_id);
        /* gimp_layer_delete(l_layer_id); */      /* did crash in gimp 1.0.2 ?? and in 1.1 too */
     }
   }
   
   /* steal all layers, channels and guides from src_image */
   l_rc = p_steal_content(image_id, src_image_id);

   if(l_old_bg_layer_id >= 0)
   {
      /* now delete the (old) background layer */
      if(gap_debug) printf("GAP-DEBUG: p_replace_img del (old bg) layer_id=%d \n", (int)l_old_bg_layer_id);
      gimp_image_remove_layer(image_id, l_old_bg_layer_id);
      /* gimp_layer_delete(l_old_bg_layer_id); */      /* did crash in gimp 1.0.2 ?? and in 1.1 too */
   }

   if (l_layers_list) { g_free (l_layers_list); }

   gimp_image_undo_enable (image_id);
   
   if(gap_debug) printf("GAP-DEBUG: END p_replace_img img_id=%d  rc=%d\n", (int)image_id, l_rc);

   return l_rc;  /* OK */
 
}   /* end p_replace_img */



/* ============================================================================
 * p_exchange_image
 *   
 *   
 * ============================================================================
 */

int p_exchange_image(gint32 dst_image_id, gint32 src_image_id)
{
   int l_rc;

   /* check for equal image type */
   if(gimp_image_base_type(src_image_id) != gimp_image_base_type(dst_image_id))
   {
      printf("GAP: p_exchange_image Image Types are not equal\n");
      return -1;
   }

   /* check for equal width/height */
   if((gimp_image_height(src_image_id) != gimp_image_height(dst_image_id))
   || (gimp_image_width(src_image_id)  != gimp_image_width(dst_image_id)))
   {
      printf("GAP: p_exchange_image Image Size is not equal\n");
      return -1;
   }

   /* Now we drop content and copy (or steal) from src_image
    * step by step, using many other PDB-Interfaces.
    */
   l_rc = p_replace_img(dst_image_id, src_image_id);

   return (l_rc);
   
}   /* end p_exchange_image */
