/* gap_mov_exec.c
 * 1997.11.06 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * Move : procedures for copying source layer(s) to multiple frames
 * (varying Koordinates, opacity, size ...)
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
 * gimp    1.1.29b; 2000/11/20  hof: FRAME based Stepmodes, bugfixes for path calculation
 * gimp    1.1.23a; 2000/06/03  hof: bugfix anim_preview < 100% did not work
 *                                   (the layer tattoos in a duplicated image may differ from the original !!)
 * gimp    1.1.20a; 2000/04/25  hof: support for keyframes, anim_preview
 * version 0.93.04              hof: Window with Info Message if no Source Image was selected in MovePath
 * version 0.90.00;             hof: 1.st (pre) release 14.Dec.1997
 */
#include "config.h"

/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* GIMP includes */
#include "gtk/gtk.h"
#include "config.h"
#include "libgimp/stdplugins-intl.h"
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap_layer_copy.h"
#include "gap_lib.h"
#include "gap_mov_dialog.h"
#include "gap_mov_exec.h"
#include "gap_pdb_calls.h"

extern      int gap_debug; /* ==0  ... dont print debug infos */

static gint p_mov_call_render(t_mov_data *mov_ptr, t_mov_current *cur_ptr, gint apv_layerstack);
static void p_mov_advance_src_layer(t_mov_current *cur_ptr, int src_stepmode);
static void p_mov_advance_src_frame(t_mov_current *cur_ptr, t_mov_values  *pvals);
static long   p_mov_execute(t_mov_data *mov_ptr);
static gdouble  p_calc_angle(gint p1x, gint p1y, gint p2x, gint p2y);
static gdouble  p_rotatate_less_than_180(gdouble angle, gdouble angle_new, gint *turns);

/* ============================================================================
 * p_mov_call_render
 *  load current frame, render and save back to disk
 *  for animted_preview
 * ============================================================================
 */

gint
p_mov_call_render(t_mov_data *mov_ptr, t_mov_current *cur_ptr, gint apv_layerstack)
{
  t_anim_info *ainfo_ptr;
  gint32  l_tmp_image_id;
  gint32  l_layer_id;
  int     l_rc;
  char    *l_fname;
  char    *l_name;

  l_rc = 0;
  ainfo_ptr = mov_ptr->dst_ainfo_ptr;


  if(mov_ptr->val_ptr->apv_mlayer_image < 0)
  {
    /* We are generating the Animation on the ORIGINAL FRAMES */
    if(ainfo_ptr->new_filename != NULL) g_free(ainfo_ptr->new_filename);
    ainfo_ptr->new_filename = p_alloc_fname(ainfo_ptr->basename,
                                	cur_ptr->dst_frame_nr,
                                	ainfo_ptr->extension);
    if(ainfo_ptr->new_filename == NULL)
       return -1;

    /* load next frame to render */
    l_tmp_image_id = p_load_image(ainfo_ptr->new_filename);
    if(l_tmp_image_id < 0)
      return -1;

    /* call render procedure for current image */
    if(0 == p_mov_render(l_tmp_image_id, mov_ptr->val_ptr, cur_ptr))
    {
      /* if OK: save the rendered frame back to disk */
      if(p_save_named_frame(l_tmp_image_id, ainfo_ptr->new_filename) < 0)
	l_rc = -1;
    }
    else l_rc = -1;
  }
  else
  {
    /* We are generating an ANIMATED PREVIEW multilayer image */
    if(mov_ptr->val_ptr->apv_src_frame >= 0)
    {
       /* anim preview uses one constant (prescaled) frame */
       l_tmp_image_id = p_gimp_channel_ops_duplicate(mov_ptr->val_ptr->apv_src_frame);
    }
    else
    {
       /* anim preview exact mode uses original frames */
       if(ainfo_ptr->new_filename != NULL) g_free(ainfo_ptr->new_filename);
       ainfo_ptr->new_filename = p_alloc_fname(ainfo_ptr->basename,
                                	   cur_ptr->dst_frame_nr,
                                	   ainfo_ptr->extension);
       l_tmp_image_id = p_load_image(ainfo_ptr->new_filename);
       if(l_tmp_image_id < 0)
	 return -1;

       if((mov_ptr->val_ptr->apv_scalex != 100.0) || (mov_ptr->val_ptr->apv_scaley != 100.0))
       {
         GimpParam     *l_params;
         gint        l_retvals;
	 gint32      l_size_x, l_size_y;
       
         l_size_x = (gimp_image_width(l_tmp_image_id) * mov_ptr->val_ptr->apv_scalex) / 100;
         l_size_y = (gimp_image_height(l_tmp_image_id) * mov_ptr->val_ptr->apv_scaley) / 100;
       
         l_params = gimp_run_procedure ("gimp_image_scale",
			         &l_retvals,
			         GIMP_PDB_IMAGE,    l_tmp_image_id,
			         GIMP_PDB_INT32,    l_size_x,
			         GIMP_PDB_INT32,    l_size_y,
			         GIMP_PDB_END);
       }
    }
    
    /* call render procedure for current image */
    if(0 == p_mov_render(l_tmp_image_id, mov_ptr->val_ptr, cur_ptr))
    {
      /* if OK and optional save to gap_paste-buffer */
      if(mov_ptr->val_ptr->apv_gap_paste_buff != NULL)
      {
         l_fname = p_alloc_fname(mov_ptr->val_ptr->apv_gap_paste_buff,
                                 cur_ptr->dst_frame_nr,
                                 ".xcf");
         p_save_named_frame(l_tmp_image_id, l_fname);
      }
      
      /* flatten the rendered frame */
      l_layer_id = gimp_image_flatten(l_tmp_image_id);
      if(l_layer_id < 0)
      {
        if(gap_debug) printf("p_mov_call_render: flattened layer_id:%d\n", (int)l_layer_id);
        /* hof:
	 * if invisible layers are flattened on an empty image
	 * we do not get a resulting layer (returned l_layer_id == -1)
	 *
	 *  I'm not sure if is this a bug, but here is a workaround:
	 *
	 * In that case I add a dummy layer 1x1 pixel (at offest -1,-1)
	 * and flatten again, and it works (tested with gimp-1.1.19)
	 */
        l_layer_id = gimp_layer_new(l_tmp_image_id, "dummy",
                                 1, 
				 1,
				 ((gint)(gimp_image_base_type(l_tmp_image_id)) * 2),
                                 100.0,     /* Opacity full opaque */     
                                 0);        /* NORMAL */
         gimp_image_add_layer(l_tmp_image_id, l_layer_id, 0);
	 gimp_layer_set_offsets(l_layer_id, -1, -1);
        l_layer_id = gimp_image_flatten(l_tmp_image_id);
	
      }
      gimp_layer_add_alpha(l_layer_id);
      
      if(gap_debug)
      {
        printf("p_mov_call_render: flattened layer_id:%d\n", (int)l_layer_id);
        printf("p_mov_call_render: tmp_image_id:%d  apv_mlayer_image:%d\n",
	        (int)l_tmp_image_id, (int)mov_ptr->val_ptr->apv_mlayer_image);
      }

      /* set layername (including delay for the framerate) */
      l_name = g_strdup_printf("frame_%04d (%dms)"
                              , (int) cur_ptr->dst_frame_nr
                              , (int)(1000/mov_ptr->val_ptr->apv_framerate));
      gimp_layer_set_name(l_layer_id, l_name);
      g_free(l_name);
      
      /* remove (its only) layer from source */
      gimp_image_remove_layer(l_tmp_image_id, l_layer_id);

      /* and set the dst_image as it's new Master */
      p_gimp_drawable_set_image(l_layer_id, mov_ptr->val_ptr->apv_mlayer_image);

      /* add the layer to the anim preview multilayer image */
      gimp_image_add_layer (mov_ptr->val_ptr->apv_mlayer_image, l_layer_id, apv_layerstack);
    }
    else l_rc = -1;
  }
    

  /* destroy the tmp image */
  gimp_image_delete(l_tmp_image_id);

  return l_rc;
}	/* end p_mov_call_render */
 

/* ============================================================================
 * p_mov_advance_src_layer
 * advance layer index according to stepmode
 * ============================================================================
 */
void p_mov_advance_src_layer(t_mov_current *cur_ptr, int src_stepmode)
{
  static int l_ping = -1;

  if(gap_debug) printf("p_mov_advance_src_layer: stepmode=%d last_layer=%d idx=%d\n",
                       (int)src_stepmode,
                       (int)cur_ptr->src_last_layer,
                       (int)cur_ptr->src_layer_idx
                      ); 

  /* note: top layer has index 0
   *       therfore reverse loops have to count up
   */
  if((cur_ptr->src_last_layer > 0 ) && (src_stepmode != GAP_STEP_NONE))
  {
    switch(src_stepmode)
    {
      case GAP_STEP_ONCE_REV:
        cur_ptr->src_layer_idx++;
        if(cur_ptr->src_layer_idx > cur_ptr->src_last_layer)
        {
           cur_ptr->src_layer_idx = cur_ptr->src_last_layer;
        }
        break;
      case GAP_STEP_ONCE:
        cur_ptr->src_layer_idx--;
        if(cur_ptr->src_layer_idx < 0)
        {
           cur_ptr->src_layer_idx = 0;
        }
        break;
      case GAP_STEP_PING_PONG:
        cur_ptr->src_layer_idx += l_ping;
        if(l_ping < 0)
        {
          if(cur_ptr->src_layer_idx < 0)
          {
             cur_ptr->src_layer_idx = 1;
             l_ping = 1;
          }
        }
        else
        {
          if(cur_ptr->src_layer_idx > cur_ptr->src_last_layer)
          {
             cur_ptr->src_layer_idx = cur_ptr->src_last_layer - 1;
             l_ping = -1;
          }
        }

        break;
      case GAP_STEP_LOOP_REV:
        cur_ptr->src_layer_idx++;
        if(cur_ptr->src_layer_idx > cur_ptr->src_last_layer)
        {
           cur_ptr->src_layer_idx = 0;
        }
        break;
      case GAP_STEP_LOOP:
      default:
        cur_ptr->src_layer_idx--;
        if(cur_ptr->src_layer_idx < 0)
        {
           cur_ptr->src_layer_idx = cur_ptr->src_last_layer;
        }
        break;

    }
  }
}	/* end  p_advance_src_layer */



/* ============================================================================
 * p_mov_advance_src_frame
 *   advance chached image to next source frame according to FRAME based pvals->stepmode
 * ============================================================================
 */
static void
p_mov_advance_src_frame(t_mov_current *cur_ptr, t_mov_values  *pvals)
{
  static int l_ping = 1;

  if(pvals->src_stepmode != GAP_STEP_FRAME_NONE)
  {
    if(pvals->cache_ainfo_ptr == NULL )
    {
      pvals->cache_ainfo_ptr =  p_alloc_ainfo(pvals->src_image_id, GIMP_RUN_NONINTERACTIVE);
    }

    if(pvals->cache_ainfo_ptr->first_frame_nr < 0)
    {
       p_dir_ainfo(pvals->cache_ainfo_ptr);
    }
  }

  if(gap_debug) printf("p_mov_advance_src_frame: stepmode=%d frame_cnt=%d first_frame=%d last_frame=%d idx=%d\n",
                       (int)pvals->src_stepmode,
                       (int)pvals->cache_ainfo_ptr->frame_cnt,
                       (int)pvals->cache_ainfo_ptr->first_frame_nr,
                       (int)pvals->cache_ainfo_ptr->last_frame_nr,
                       (int)cur_ptr->src_frame_idx
                      ); 

  if((pvals->cache_ainfo_ptr->frame_cnt > 1 ) && (pvals->src_stepmode != GAP_STEP_FRAME_NONE))
  {
    switch(pvals->src_stepmode)
    {
      case GAP_STEP_FRAME_ONCE_REV:
        cur_ptr->src_frame_idx--;
        if(cur_ptr->src_frame_idx < pvals->cache_ainfo_ptr->first_frame_nr)
        {
           cur_ptr->src_frame_idx = pvals->cache_ainfo_ptr->first_frame_nr;
        }
        break;
      case GAP_STEP_FRAME_ONCE:
        cur_ptr->src_frame_idx++;
        if(cur_ptr->src_frame_idx > pvals->cache_ainfo_ptr->last_frame_nr)
        {
           cur_ptr->src_frame_idx = pvals->cache_ainfo_ptr->last_frame_nr;
        }
        break;
      case GAP_STEP_FRAME_PING_PONG:
        cur_ptr->src_frame_idx += l_ping;
        if(l_ping < 0)
        {
          if(cur_ptr->src_frame_idx < pvals->cache_ainfo_ptr->first_frame_nr)
          {
             cur_ptr->src_frame_idx = pvals->cache_ainfo_ptr->first_frame_nr + 1;
             l_ping = 1;
          }
        }
        else
        {
          if(cur_ptr->src_frame_idx > pvals->cache_ainfo_ptr->last_frame_nr)
          {
             cur_ptr->src_frame_idx = pvals->cache_ainfo_ptr->last_frame_nr - 1;
             l_ping = -1;
          }
        }
        break;
      case GAP_STEP_FRAME_LOOP_REV:
        cur_ptr->src_frame_idx--;
        if(cur_ptr->src_frame_idx < pvals->cache_ainfo_ptr->first_frame_nr)
        {
           cur_ptr->src_frame_idx = pvals->cache_ainfo_ptr->last_frame_nr;
        }
        break;
      case GAP_STEP_FRAME_LOOP:
      default:
        cur_ptr->src_frame_idx++;
        if(cur_ptr->src_frame_idx > pvals->cache_ainfo_ptr->last_frame_nr)
        {
           cur_ptr->src_frame_idx = pvals->cache_ainfo_ptr->first_frame_nr;
        }
        break;

    }
    
    p_fetch_src_frame(pvals, cur_ptr->src_frame_idx);
  }
}	/* end  p_advance_src_frame */


/* ============================================================================
 * p_mov_execute
 * Copy layer(s) from Sourceimage to given destination frame range,
 * varying koordinates and opacity of the copied layer.
 * To each affected destination frame exactly one copy of a source layer is added.
 * The source layer is iterated through all layers of the sourceimage
 * according to stemmode parameter.
 * For the placement the layers act as if their size is equal to their
 * Sourceimages size.
 * ============================================================================
 */

/* TODO: add keyframe support */

long
p_mov_execute(t_mov_data *mov_ptr)
{
/* MIX_VALUE  0.0 <= factor <= 1.0
 *  result is a  for factor 0.0
 *            b  for factor 1.0
 *            mix for factors inbetween
 */
#define MIX_VALUE(factor, a, b) ((a * (1.0 - factor)) +  (b * factor))
  gint l_idx;
   t_mov_current l_current_data;
   t_mov_current *cur_ptr;
   t_mov_values  *val_ptr;

   gdouble  l_percentage;  
   gdouble  l_fpl;             /* frames_per_line */
   gdouble  l_flt_posfactor;
   long     l_frame_step;
   gdouble  l_frames;
   long     l_cnt;
   long     l_points;
   long     l_ptidx;
   long     l_prev_keyptidx;
   long     l_fridx;
   gdouble  l_flt_count;
   gint     l_rc;
   gint     l_nlayers;
   gint     l_idk;
   gint     l_prev_keyframe;
   gint     l_apv_layerstack;
   gdouble  l_flt_timing[GAP_MOV_MAX_POINT];   /* timing table in relative frame numbers (0.0 == the first handled frame) */
   
   
   if(mov_ptr->val_ptr->src_image_id < 0)
   {
      p_msg_win(mov_ptr->dst_ainfo_ptr->run_mode,
        _("No Source Image was selected.\n"
	  "Please open a 2nd Image of the same type before opening Move Path."));
      return -1;
   }

  l_apv_layerstack = 0;
  l_percentage = 0.0;  
  if(mov_ptr->dst_ainfo_ptr->run_mode == GIMP_RUN_INTERACTIVE)
  { 
    if(mov_ptr->val_ptr->apv_mlayer_image < 0)
    {
      gimp_progress_init( _("Copying Layers into Frames..."));
    }
    else
    {
      gimp_progress_init( _("Generating Animated Preview..."));
    }
  }

  if(gap_debug)
  {
    printf("p_mov_execute: values got from dialog:\n");
    printf("apv_mlayer_image: %ld\n", (long)mov_ptr->val_ptr->apv_mlayer_image);
    printf("apv_mode: %ld\n", (long)mov_ptr->val_ptr->apv_mode);
    printf("apv_scale x: %f y:%f\n", (float)mov_ptr->val_ptr->apv_scalex, (float)mov_ptr->val_ptr->apv_scaley);
    if(mov_ptr->val_ptr->apv_gap_paste_buff)
    {
      printf("apv_gap_paste_buf: %s\n", mov_ptr->val_ptr->apv_gap_paste_buff);
    }
    else
    {
      printf("apv_gap_paste_buf: ** IS NULL ** (do not copy to paste buffer)\n");
    }
    printf("src_image_id :%ld\n", (long)mov_ptr->val_ptr->src_image_id);
    printf("src_layer_id :%ld\n", (long)mov_ptr->val_ptr->src_layer_id);
    printf("src_handle :%d\n", mov_ptr->val_ptr->src_handle);
    printf("src_stepmode :%d\n", mov_ptr->val_ptr->src_stepmode);
    printf("src_paintmode :%d\n", mov_ptr->val_ptr->src_paintmode);
    printf("clip_to_img :%d\n", mov_ptr->val_ptr->clip_to_img);
    printf("dst_range_start :%d\n", (int)mov_ptr->val_ptr->dst_range_start);
    printf("dst_range_end :%d\n", (int)mov_ptr->val_ptr->dst_range_end);
    printf("dst_layerstack :%d\n", (int)mov_ptr->val_ptr->dst_layerstack);
    for(l_idx = 0; l_idx <= mov_ptr->val_ptr->point_idx_max; l_idx++)
    {
      printf("p_x[%d] :%d\n", l_idx, mov_ptr->val_ptr->point[l_idx].p_x);
      printf("p_y[%d] : :%d\n", l_idx, mov_ptr->val_ptr->point[l_idx].p_y);
      printf("opacity[%d] :%d\n", l_idx, mov_ptr->val_ptr->point[l_idx].opacity);
      printf("w_resize[%d] :%d\n", l_idx, mov_ptr->val_ptr->point[l_idx].w_resize);
      printf("h_resize[%d] :%d\n", l_idx, mov_ptr->val_ptr->point[l_idx].h_resize);
      printf("rotation[%d] :%d\n", l_idx, mov_ptr->val_ptr->point[l_idx].rotation);
      printf("keyframe[%d] :%d\n", l_idx, mov_ptr->val_ptr->point[l_idx].keyframe);
      printf("keyframe_abs[%d] :%d\n", l_idx, mov_ptr->val_ptr->point[l_idx].keyframe_abs);
    }
    printf("\n");
  }

   l_rc    = 0;
   cur_ptr = &l_current_data;
   val_ptr = mov_ptr->val_ptr;

   /* set offsets (in cur_ptr)  according to handle mode and src_img dimension */
   p_set_handle_offsets(val_ptr, cur_ptr);
    
   
   /* test for invers range */
   if(val_ptr->dst_range_start > val_ptr->dst_range_end)
   {
      /* step down */
      l_frame_step = -1;
      l_cnt = 1 + (val_ptr->dst_range_start - val_ptr->dst_range_end);
   }
   else
   {
      l_frame_step = 1;
      l_cnt = 1 + (val_ptr->dst_range_end - val_ptr->dst_range_start);
   }

   l_frames = (gdouble)l_cnt;              /* nr. of affected frames */
   l_points = val_ptr->point_idx_max +1;   /* nr. of available points */
   
   if(l_points > l_frames)
   {
      /* cut off some points if we got more than frames */
      l_points = l_cnt;
   }

   if(l_points < 2)
   {
      /* copy point[0] to point [1] because we need at least 2
       * points for the algorithms below to work. 
       * (simulates a line with lenght 0, to move along)
       */
      if(gap_debug) printf("p_mov_execute: added a 2nd Point\n");
      val_ptr->point[1].p_x = val_ptr->point[0].p_x;
      val_ptr->point[1].p_y = val_ptr->point[0].p_y;
      val_ptr->point[1].opacity = val_ptr->point[0].opacity;
      val_ptr->point[1].w_resize = val_ptr->point[0].w_resize;
      val_ptr->point[1].h_resize = val_ptr->point[0].h_resize;
      val_ptr->point[1].rotation = val_ptr->point[0].rotation;
      l_points = 2;
   }
   
   
   cur_ptr->dst_frame_nr = val_ptr->dst_range_start;
   cur_ptr->src_layers = NULL;

   if(mov_ptr->val_ptr->src_stepmode < GAP_STEP_FRAME)
   {   
     cur_ptr->src_layers = gimp_image_get_layers (val_ptr->src_image_id, &l_nlayers);
     if(cur_ptr->src_layers == NULL)
     {
       printf("ERROR (in p_mov_execute): Got no layers from SrcImage\n");
       return -1;
     }
     if(l_nlayers < 1)
     {
       printf("ERROR (in p_mov_execute): Source Image has no layers\n");
       return -1;
     }
     cur_ptr->src_last_layer = l_nlayers -1;

     /* findout index of src_layer_id */
     for(cur_ptr->src_layer_idx = 0; 
	 cur_ptr->src_layer_idx  < l_nlayers;
	 cur_ptr->src_layer_idx++)
     {
	if(cur_ptr->src_layers[cur_ptr->src_layer_idx] == val_ptr->src_layer_id)
           break;
     }
     cur_ptr->src_last_layer = l_nlayers -1;   /* index of last layer */
     }
   else
   {
     /* for FRAME stepmodes we use flattened Sorce frames
      * (instead of one multilayer source image )
      */
     p_fetch_src_frame (val_ptr, -1);  /* negative value fetches the selected frame number */
     cur_ptr->src_frame_idx = val_ptr->cache_ainfo_ptr->curr_frame_nr;
     
     if((val_ptr->cache_ainfo_ptr->first_frame_nr < 0)
     && (val_ptr->src_stepmode != GAP_STEP_FRAME_NONE))
     {
        p_dir_ainfo(val_ptr->cache_ainfo_ptr);
     }

     /* set offsets (in cur_ptr)  according to handle mode and cache_tmp_img dimension */
     p_set_handle_offsets(val_ptr, cur_ptr);
   }
   
   cur_ptr->currX   = (gdouble)val_ptr->point[0].p_x;
   cur_ptr->currY   = (gdouble)val_ptr->point[0].p_y;
   cur_ptr->currOpacity  = (gdouble)val_ptr->point[0].opacity;
   cur_ptr->currWidth    = (gdouble)val_ptr->point[0].w_resize;
   cur_ptr->currHeight   = (gdouble)val_ptr->point[0].h_resize;
   cur_ptr->currRotation = (gdouble)val_ptr->point[0].rotation;

   /* RENDER add current src_layer to current frame */
   l_rc = p_mov_call_render(mov_ptr, cur_ptr, l_apv_layerstack);

   if(gap_debug) printf("p_mov_execute: l_points=%d\n", (int)l_points);
   
   /* how many frames are affected from one line of the moving path */   
   l_fpl = ((gdouble)l_frames - 1.0) / ((gdouble)(l_points -1));
   if(gap_debug) printf("p_mov_execute: initial l_fpl=%f\n", l_fpl);

   /* calculate l_flt_timing controlpoint timing table considering keyframes */
   l_prev_keyptidx = 0;
   l_prev_keyframe = 0;
   l_flt_timing[0] = 0.0;
   l_flt_timing[l_points -1] = l_frames -1;
   l_flt_count = 0.0;
   for(l_ptidx=1;  l_ptidx < l_points - 1; l_ptidx++)
   {
     /* search for keyframes */
     if(l_ptidx > l_prev_keyptidx)
     {
       for(l_idk = l_ptidx; l_idk < l_points; l_idk++)
       {
          if(l_idk == l_points -1)
	  {
	    /* last point is always an implicite  keyframe */
            l_fpl = ((gdouble)((l_frames -1) - l_prev_keyframe)) / ((gdouble)((l_idk -  l_ptidx) +1));
            l_prev_keyframe = l_frames -1;

            l_prev_keyptidx = l_idk;
            if(gap_debug) printf("p_mov_execute: last point is implicite keyframe l_fpl=%f\n", l_fpl);
            break;
	  }
	  else
	  {
	    if (val_ptr->point[l_idk].keyframe > 0)
	    {
	      /* found a keyframe, have to recalculate frames_per_line */
              l_fpl = ((gdouble)(val_ptr->point[l_idk].keyframe - l_prev_keyframe)) / ((gdouble)((l_idk -  l_ptidx) +1));
              l_prev_keyframe = val_ptr->point[l_idk].keyframe;

              l_prev_keyptidx = l_idk;
              if(gap_debug) printf("p_mov_execute: keyframe l_fpl=%f\n", l_fpl);
              break;
	    }
	  }
       }
     }
     l_flt_count += l_fpl;
     l_flt_timing[l_ptidx] = l_flt_count;

     if(l_fpl < 1.0)
     {
	printf("p_mov_execute: ** Error frames per line at point[%d] = %f  (is less than 1.0 !!)\n",
	  (int)l_ptidx, (float)l_fpl);
     }
   }

   if(gap_debug)
   {
     printf("p_mov_execute: --- CONTROLPOINT relative frametiming TABLE -----\n");
     for(l_ptidx=0;  l_ptidx < l_points; l_ptidx++)
     {
       printf("p_mov_execute: l_flt_timing[%02d] = %f\n", (int)l_ptidx, (float)l_flt_timing[l_ptidx]);
     }
   }

    
  /* loop for each frame within the range (may step up or down) */
  l_ptidx = 1;
  cur_ptr->dst_frame_nr = val_ptr->dst_range_start;
  for(l_fridx = 1; l_fridx < l_cnt; l_fridx++)
  {
     
     if(gap_debug) printf("\np_mov_execute: l_fridx=%ld, l_flt_timing[l_ptidx]=%f, l_rc=%d l_ptidx=%d, l_prev_keyptidx=%d\n",
                           l_fridx, (float)l_flt_timing[l_ptidx], (int)l_rc, (int)l_ptidx, (int)l_prev_keyptidx);

     if(l_rc != 0) break;

      /* advance frame_nr, (1st frame was done outside this loop) */
      cur_ptr->dst_frame_nr += l_frame_step;  /* +1  or -1 */

      if((gdouble)l_fridx > l_flt_timing[l_ptidx])
      {
         /* change deltas for next line of the move path */
	 if(l_ptidx < l_points-1)
	 {
           l_ptidx++;
	   if(gap_debug)
	   {
	      printf("p_mov_execute: advance to controlpoint l_ptidx=%d, l_flt_timing[l_ptidx]=%f\n"
	             , (int)l_ptidx, (float)l_flt_timing[l_ptidx]);
	   }
	 }
	 else
	 {
	   if(gap_debug) printf("p_mov_execute: ** ERROR overflow l_ptidx=%d\n", (int)l_ptidx);
	 }
      }
      
      l_fpl = (l_flt_timing[l_ptidx] - l_flt_timing[l_ptidx -1]); /* float frames per line */
      if(l_fpl != 0.0)
      {
        l_flt_posfactor  = ((gdouble)l_fridx - l_flt_timing[l_ptidx -1]) / l_fpl;
      }
      else
      {
         l_flt_posfactor = 1.0;
         if(gap_debug) printf("p_mov_execute: ** ERROR l_fpl is 0.0 frames per line\n");
      }
      
 
      if(gap_debug) printf("p_mov_execute: l_fpl=%f, l_flt_posfactor=%f\n", (float)l_fpl, (float)l_flt_posfactor);

      l_flt_posfactor = CLAMP (l_flt_posfactor, 0.0, 1.0);
     
      cur_ptr->currX  =       MIX_VALUE(l_flt_posfactor, (gdouble)val_ptr->point[l_ptidx -1].p_x,      (gdouble)val_ptr->point[l_ptidx].p_x);
      cur_ptr->currY  =       MIX_VALUE(l_flt_posfactor, (gdouble)val_ptr->point[l_ptidx -1].p_y,      (gdouble)val_ptr->point[l_ptidx].p_y);
      cur_ptr->currOpacity =  MIX_VALUE(l_flt_posfactor, (gdouble)val_ptr->point[l_ptidx -1].opacity,  (gdouble)val_ptr->point[l_ptidx].opacity);
      cur_ptr->currWidth =    MIX_VALUE(l_flt_posfactor, (gdouble)val_ptr->point[l_ptidx -1].w_resize, (gdouble)val_ptr->point[l_ptidx].w_resize);
      cur_ptr->currHeight =   MIX_VALUE(l_flt_posfactor, (gdouble)val_ptr->point[l_ptidx -1].h_resize, (gdouble)val_ptr->point[l_ptidx].h_resize);
      cur_ptr->currRotation = MIX_VALUE(l_flt_posfactor, (gdouble)val_ptr->point[l_ptidx -1].rotation, (gdouble)val_ptr->point[l_ptidx].rotation);

      if(gap_debug)
      {
          printf("ROTATE [%02d] %d    [%02d] %d       MIX: %f\n"
          , (int)l_ptidx-1,  (int)val_ptr->point[l_ptidx -1].rotation
          , (int)l_ptidx,    (int)val_ptr->point[l_ptidx].rotation
	  , (float)cur_ptr->currRotation);
      }

       if(val_ptr->src_stepmode < GAP_STEP_FRAME )
       {
         /* advance settings for next src layer */
         p_mov_advance_src_layer(cur_ptr, val_ptr->src_stepmode);
       }

       else
       {
         /* advance settings for next source frame */
         p_mov_advance_src_frame(cur_ptr, val_ptr);
       }

       if(l_frame_step < 0)
       {
         /* if we step down, we have to insert the layer
	  * as lowest layer in the existing layerstack
	  * of the animated preview multilayer image.
	  * (if we step up, we always use 0 as l_apv_layerstack,
	  *  that means always insert on top of the layerstack)
	  */
         l_apv_layerstack++;
       }
       /* RENDER add current src_layer to current frame */
       l_rc = p_mov_call_render(mov_ptr, cur_ptr, l_apv_layerstack);

       /* show progress */
       if(mov_ptr->dst_ainfo_ptr->run_mode == GIMP_RUN_INTERACTIVE)
       { 
         l_percentage = (gdouble)l_fridx / (gdouble)(l_cnt -1);
         gimp_progress_update (l_percentage);
       }
           
   }

   if(cur_ptr->src_layers != NULL) g_free(cur_ptr->src_layers);
   
   cur_ptr->src_layers = NULL;
   
   
   return l_rc;  

}	/* end p_mov_execute */


/* ============================================================================
 * p_mov_anim_preview
 *   Generate an animate preview for the move path
 * ============================================================================
 */
gint32 
p_mov_anim_preview(t_mov_values *pvals_orig, t_anim_info *ainfo_ptr, gint preview_frame_nr)
{
  t_mov_data    apv_mov_data;
  t_mov_values  apv_mov_vals;
  t_mov_data    *l_mov_ptr;
  t_mov_values  *l_pvals;
  gint        l_idx;
  gint32      l_size_x, l_size_y;
  gint32      l_tmp_image_id;
  gint32      l_tmp_frame_id;
  gint32      l_mlayer_image_id;
  GimpParam     *l_params;
  gint        l_retvals;
  GimpImageBaseType  l_type;
  guint       l_width, l_height;
  gint32      l_stackpos;
  gint        l_nlayers;
  gint32     *l_src_layers;
  gint        l_rc;
  
  l_mov_ptr = &apv_mov_data;
  l_pvals = &apv_mov_vals;
  
  /* copy settings */
  memcpy(l_pvals, pvals_orig, sizeof(t_mov_values));
  l_mov_ptr->val_ptr = l_pvals;
  l_mov_ptr->dst_ainfo_ptr = ainfo_ptr;
  
  /* init local cached src image for anim preview generation.
   * (never mix cached src image for normal and anim preview
   *  because anim previews are often scaled down)
   */
  l_pvals->cache_src_image_id  = -1;  
  l_pvals->cache_tmp_image_id  = -1;
  l_pvals->cache_tmp_layer_id  = -1;
  l_pvals->cache_frame_number  = -1;
  l_pvals->cache_ainfo_ptr = NULL;

  /* -1 assume no tmp_image (use unscaled original source) */
  l_tmp_image_id = -1;
  l_stackpos = 0;

  /* Scale (down) needed ? */
  if((l_pvals->apv_scalex != 100.0) || (l_pvals->apv_scaley != 100.0))
  {
    /* scale the controlpoint koords */
    for(l_idx = 0; l_idx <= l_pvals->point_idx_max; l_idx++)
    {
      l_pvals->point[l_idx].p_x  = (l_pvals->point[l_idx].p_x * l_pvals->apv_scalex) / 100;
      l_pvals->point[l_idx].p_y  = (l_pvals->point[l_idx].p_y * l_pvals->apv_scaley) / 100;
    }

    /* for the FRAME based step modes we cant Scale here,
     * we have to scale later (at fetch time of the frame)
     */
    if(l_pvals->src_stepmode < GAP_STEP_FRAME)
    {
      /* copy and scale the source object image */
      l_tmp_image_id = p_gimp_channel_ops_duplicate(pvals_orig->src_image_id);
      l_pvals->src_image_id = l_tmp_image_id;

      l_size_x = MAX(1, (gimp_image_width(l_tmp_image_id) * l_pvals->apv_scalex) / 100);
      l_size_y = MAX(1, (gimp_image_height(l_tmp_image_id) * l_pvals->apv_scaley) / 100);
      l_params = gimp_run_procedure ("gimp_image_scale",
  		                   &l_retvals,
			           GIMP_PDB_IMAGE,    l_tmp_image_id,
			           GIMP_PDB_INT32,    l_size_x,
			           GIMP_PDB_INT32,    l_size_y,
			           GIMP_PDB_END);

       /* findout the src_layer id in the scaled copy by stackpos index */
       l_pvals->src_layer_id = -1;
       l_src_layers = gimp_image_get_layers (pvals_orig->src_image_id, &l_nlayers);
       if(l_src_layers == NULL)
       {
	 printf("ERROR: p_mov_anim_preview GOT no src_layers (original image_id %d)\n",
        	 (int)pvals_orig->src_image_id);
       }
       else
       {
	 for(l_stackpos = 0; 
	     l_stackpos  < l_nlayers;
	     l_stackpos++)
	 {
	    if(l_src_layers[l_stackpos] == pvals_orig->src_layer_id)
               break;
	 }
	 g_free(l_src_layers);

	 l_src_layers = gimp_image_get_layers (l_tmp_image_id, &l_nlayers);
	 if(l_src_layers == NULL)
	 {
           printf("ERROR: p_mov_anim_preview GOT no src_layers (scaled copy image_id %d)\n",
        	  (int)l_tmp_image_id);
	 }
	 else
	 {
            l_pvals->src_layer_id = l_src_layers[l_stackpos];
            g_free(l_src_layers);
	 }

       }

      if(gap_debug)
      {
	printf("p_mov_anim_preview: orig  src_image_id:%d src_layer:%d, stackpos:%d\n"
               ,(int)pvals_orig->src_image_id
	       ,(int)pvals_orig->src_layer_id
	       ,(int)l_stackpos);
	printf("   Scaled src_image_id:%d scaled_src_layer:%d\n"
               ,(int)l_tmp_image_id
	       ,(int)l_pvals->src_layer_id );
      }
    }

    
  }


  if(gap_debug)
  {
    printf("p_mov_anim_preview: src_image_id %d (orig:%d)\n"
           , (int) l_pvals->src_image_id
	   , (int) pvals_orig->src_image_id);
  }
  
  /* create the animated preview multilayer image in (scaled) framesize */
  l_width  = (gimp_image_width(ainfo_ptr->image_id) * l_pvals->apv_scalex) / 100;
  l_height = (gimp_image_height(ainfo_ptr->image_id) * l_pvals->apv_scaley) / 100;
  l_type   = gimp_image_base_type(ainfo_ptr->image_id);
  
  l_mlayer_image_id = gimp_image_new(l_width, l_height,l_type);
  l_pvals->apv_mlayer_image = l_mlayer_image_id;

  if(gap_debug)
  {
    printf("p_mov_anim_preview: apv_mlayer_image %d\n"
           , (int) l_pvals->apv_mlayer_image);
  }
  
  /* APV_MODE (Wich frames to use in the preview?)  */
  switch(l_pvals->apv_mode)
  {
    gchar *l_filename;
    
    case GAP_APV_QUICK:
      /* use an empty dummy frame for all frames */    
      l_tmp_frame_id = gimp_image_new(l_width, l_height,l_type);
      break;
    case GAP_APV_ONE_FRAME:
      /* use only one frame in the preview */    
      l_filename = p_alloc_fname(ainfo_ptr->basename,
                                 preview_frame_nr,
                                 ainfo_ptr->extension);
      l_tmp_frame_id =  p_load_image(l_filename);
      if((l_pvals->apv_scalex != 100.0) || (l_pvals->apv_scaley != 100.0))
      {
	l_size_x = (gimp_image_width(l_tmp_frame_id) * l_pvals->apv_scalex) / 100;
	l_size_y = (gimp_image_height(l_tmp_frame_id) * l_pvals->apv_scaley) / 100;
	l_params = gimp_run_procedure ("gimp_image_scale",
  		                   &l_retvals,
			           GIMP_PDB_IMAGE,    l_tmp_frame_id,
			           GIMP_PDB_INT32,    l_size_x,
			           GIMP_PDB_INT32,    l_size_y,
			           GIMP_PDB_END);
      }
      g_free(l_filename);
      break;
    default:  /* GAP_APV_EXACT */
      /* read the original frames for the preview (slow) */
      l_tmp_frame_id = -1; 
      break;
  }
  l_pvals->apv_src_frame = l_tmp_frame_id;

  if(gap_debug)
  {
    printf("p_mov_anim_preview: apv_src_frame %d\n"
           , (int) l_pvals->apv_src_frame);
  }


  /* EXECUTE move path in preview Mode */ 
  /* --------------------------------- */ 
  l_rc = p_mov_execute(l_mov_ptr);

  if(l_pvals->cache_tmp_image_id >= 0)
  {
     if(gap_debug)
     {
	printf("p_mov_anim_preview: DELETE cache_tmp_image_id:%d\n",
	         (int)l_pvals->cache_tmp_image_id);
     }
     /* destroy the cached frame image */
     gimp_image_delete(l_pvals->cache_tmp_image_id);
     l_pvals->cache_tmp_image_id = -1;
  }
  
  if(l_rc < 0)
  {
    return(-1);
  }
  
  /* add a display for the animated preview multilayer image */
  gimp_display_new(l_mlayer_image_id);
 
  /* delete the scaled copy of the src image (if there is one) */
  if(l_tmp_image_id >= 0)
  { 
    gimp_image_delete(l_tmp_image_id);
  }
  /* delete the (scaled) dummy frames (if there is one) */
  if(l_tmp_frame_id >= 0)
  { 
    gimp_image_delete(l_tmp_frame_id);
  }

  return(l_mlayer_image_id);
}	/* end p_mov_anim_preview */

/* ============================================================================
 * p_con_keyframe
 * ============================================================================
 */

gint
p_conv_keyframe_to_rel(gint abs_keyframe, t_mov_values *pvals)
{
    if(pvals->dst_range_start <= pvals->dst_range_end)
    {
       return (abs_keyframe -  pvals->dst_range_start);
    }
    return (pvals->dst_range_start - abs_keyframe);
}

gint
p_conv_keyframe_to_abs(gint rel_keyframe, t_mov_values *pvals)
{
    if(pvals->dst_range_start <= pvals->dst_range_end)
    {
      return(rel_keyframe + pvals->dst_range_start);
    }
    return(pvals->dst_range_start - rel_keyframe);
}

/* ============================================================================
 * p_gap_save_pointfile
 * ============================================================================
 */
gint
p_gap_save_pointfile(char *filename, t_mov_values *pvals)
{
  FILE *l_fp;
  gint l_idx;
  
  if(filename == NULL) return -1;
  
  l_fp = fopen(filename, "w+");
  if(l_fp != NULL)
  {
    fprintf(l_fp, "# GAP file contains saved Move Path Point Table\n");
    fprintf(l_fp, "%d  %d  # current_point  points\n",
                  (int)pvals->point_idx,
                  (int)pvals->point_idx_max + 1);
    fprintf(l_fp, "# x  y   width height opacity rotation [rel_keyframe]\n");
    for(l_idx = 0; l_idx <= pvals->point_idx_max; l_idx++)
    {
      if((l_idx > 0) 
      && (l_idx < pvals->point_idx_max)
      && ((int)pvals->point[l_idx].keyframe > 0))
      {
	fprintf(l_fp, "%04d %04d  %03d %03d  %03d %d %d\n",
                     (int)pvals->point[l_idx].p_x,
                     (int)pvals->point[l_idx].p_y,
                     (int)pvals->point[l_idx].w_resize,
                     (int)pvals->point[l_idx].h_resize,
                     (int)pvals->point[l_idx].opacity,
                     (int)pvals->point[l_idx].rotation,
                     (int)p_conv_keyframe_to_rel(pvals->point[l_idx].keyframe_abs, pvals));
      }
      else
      {
	fprintf(l_fp, "%04d %04d  %03d %03d  %03d %d\n",
                     (int)pvals->point[l_idx].p_x,
                     (int)pvals->point[l_idx].p_y,
                     (int)pvals->point[l_idx].w_resize,
                     (int)pvals->point[l_idx].h_resize,
                     (int)pvals->point[l_idx].opacity,
                     (int)pvals->point[l_idx].rotation);
      }
    }
     
    fclose(l_fp);
    return 0;
  }
  return -1;
}

/* ============================================================================
 * p_gap_load_pointfile
 * ============================================================================
 */
gint
p_gap_load_pointfile(char *filename, t_mov_values *pvals)
{
#define POINT_REC_MAX 128

  FILE *l_fp;
  gint   l_idx;
  char  l_buff[POINT_REC_MAX +1 ];
  char *l_ptr;
  gint   l_cnt;
  gint   l_rc;
  gint   l_v1, l_v2, l_v3, l_v4, l_v5, l_v6, l_v7;

  l_rc = -1;
  if(filename == NULL) return(l_rc);
  
  l_fp = fopen(filename, "r");
  if(l_fp != NULL)
  {
    l_idx = -1;
    while (NULL != fgets (l_buff, POINT_REC_MAX, l_fp))
    {
       /* skip leading blanks */
       l_ptr = l_buff;
       while(*l_ptr == ' ') { l_ptr++; }
       
       /* check if line empty or comment only (starts with '#') */
       if((*l_ptr != '#') && (*l_ptr != '\n') && (*l_ptr != '\0'))
       {
         l_cnt = sscanf(l_ptr, "%d%d%d%d%d%d%d", &l_v1, &l_v2, &l_v3, &l_v4, &l_v5, &l_v6, &l_v7);
         if(l_idx == -1)
         {
           if((l_cnt < 2) || (l_v2 > GAP_MOV_MAX_POINT) || (l_v1 > l_v2))
           {
             break;
            }
           pvals->point_idx     = l_v1;
           pvals->point_idx_max = l_v2 -1;
           l_idx = 0;
         }
         else
         {
           if((l_cnt != 6) && (l_cnt != 7))
           {
             l_rc = -2;  /* have to call p_reset_points() when called from dialog window */
             break;
           }
           pvals->point[l_idx].p_x      = l_v1;
           pvals->point[l_idx].p_y      = l_v2;
           pvals->point[l_idx].w_resize = l_v3;
           pvals->point[l_idx].h_resize = l_v4;
           pvals->point[l_idx].opacity  = l_v5;
           pvals->point[l_idx].rotation = l_v6;
           if((l_cnt == 7) && (l_idx > 0))
	   {
             pvals->point[l_idx].keyframe = l_v7;
             pvals->point[l_idx].keyframe_abs = p_conv_keyframe_to_abs(l_v7, pvals);
	   }
	   else
	   {
             pvals->point[l_idx].keyframe_abs = 0;
             pvals->point[l_idx].keyframe = 0;
	   }
           l_idx ++;
         }

         if(l_idx > pvals->point_idx_max) break;
       }
    }
     
    fclose(l_fp);
    if(l_idx >= 0)
    {
       l_rc = 0;  /* OK if we found at least one valid Controlpoint in the file */
    }
  }
  return (l_rc);
}


/* ============================================================================
 * procedured for calculating angels
 *   (used in rotate_follow option)
 * ============================================================================
 */

static gdouble
p_calc_angle(gint p1x, gint p1y, gint p2x, gint p2y)
{
  /* calculate angle in degree
   * how to rotate an object that follows the line between p1 and p2
   */
  gdouble l_a;
  gdouble l_b;
  gdouble l_angle_rad;
  gdouble l_angle;
  
  l_a = p2x - p1x;
  l_b = (p2y - p1y) * (-1.0);
  
  if(l_a == 0)
  {
    if(l_b < 0)  { l_angle = 90.0; }
    else         { l_angle = 270.0; }
  }
  else
  {
    l_angle_rad = atan(l_b/l_a);
    l_angle = (l_angle_rad * 180.0) / 3.14159;

    if(l_a < 0)
    {
      l_angle = 180 - l_angle;
    }
    else
    {
      l_angle = l_angle * (-1.0);
    }
  }  

  if(gap_debug)
  {
     printf("p_calc_angle: p1(%d/%d) p2(%d/%d)  a=%f, b=%f, angle=%f\n"
         , (int)p1x, (int)p1y, (int)p2x, (int)p2y
         , (float)l_a, (float)l_b, (float)l_angle);
  }
  return(l_angle);
}

static gdouble
p_rotatate_less_than_180(gdouble angle, gdouble angle_new, gint *turns)
{
  /* if an object  follows a circular path and does more than one turn
   * there comes a point where it flips from say 265 degree to -85 degree.
   *
   * if there are more (say 3) frames between the controlpoints,
   * the object performs an unexpected rotation effect because the iteration
   * from 265 to -85  is done  in a sequence like this: 265.0, 148.6, 32.3, -85.0
   *
   * we can avoid this by preventing angle changes of more than 180 degree.
   * in such a case this procedure adjusts the new_angle from -85 to 275
   * that results in oterations like this: 265.0, 268.3, 271.6, 275.0
   */
  gint l_diff;
  gint l_turns;

  l_diff = angle - (angle_new + (*turns * 360));
  if((l_diff >= -180) && (l_diff < 180))
  {
      return(angle_new + (*turns * 360));
  }

  l_diff = (angle - angle_new);
  if(l_diff < 0)
  {
     l_turns = (l_diff / 360) -1;
  }
  else
  {
     l_turns = (l_diff / 360) +1;
  }

  *turns = l_turns;

  if(gap_debug)
  {
     printf("p_rotatate_less_than_180: turns %d angle_new:%f\n"
             , (int)l_turns, (float)angle_new);
  }

  return( angle_new + (l_turns * 360));
}


/* ============================================================================
 * p_calculate_rotate_follow
 * ============================================================================
 */
void
p_calculate_rotate_follow(t_mov_values *pvals, gint32 startangle)
{
  gint l_idx;
  gdouble l_startangle;
  gdouble l_angle_1;
  gdouble l_angle_2;
  gdouble l_angle_new;
  gdouble l_angle;
  gint    l_turns;

  l_startangle = startangle;
  
  if(pvals->point_idx_max > 1)
  {
    l_angle = 0.0;
    l_turns = 0;
 
    for(l_idx = 0; l_idx <= pvals->point_idx_max; l_idx++)
    {
      if(l_idx == 0)
      {
        l_angle = p_calc_angle(pvals->point[l_idx].p_x,
	                       pvals->point[l_idx].p_y,
	                       pvals->point[l_idx +1].p_x,
	                       pvals->point[l_idx +1].p_y);
      }
      else
      {
        if(l_idx == pvals->point_idx_max)
	{
          l_angle_new = p_calc_angle(pvals->point[l_idx -1].p_x,
	                	 pvals->point[l_idx -1].p_y,
	                	 pvals->point[l_idx].p_x,
	                	 pvals->point[l_idx].p_y);
	}
	else
	{
           l_angle_1 = p_calc_angle(pvals->point[l_idx -1].p_x,
	                	  pvals->point[l_idx -1].p_y,
	                	  pvals->point[l_idx].p_x,
	                	  pvals->point[l_idx].p_y);
				  
           l_angle_2 = p_calc_angle(pvals->point[l_idx].p_x,
	                          pvals->point[l_idx].p_y,
	                          pvals->point[l_idx +1].p_x,
	                          pvals->point[l_idx +1].p_y);

           if((l_angle_1 == 0) && (l_angle_2 == 180))
	   {
               l_angle_new = 270;
	   }
	   else
	   {
             if((l_angle_1 == 90) && (l_angle_2 == 270))
	     {
               l_angle_new = 0;
	     }
	     else
	     {
               l_angle_new = (l_angle_1 + l_angle_2) / 2;
	     }
	   }
	   if(((l_angle_1 < 0) && (l_angle_2 >= 180))
	   || ((l_angle_2 < 0) && (l_angle_1 >= 180)))
	   {
	      l_angle_new += 180;
	   }
	}
	l_angle = p_rotatate_less_than_180(l_angle, l_angle_new, &l_turns);
      }

      if(gap_debug)
      {
        printf("ROT Follow [%03d] angle = %f\n", (int)l_idx, (float)l_angle);
      }

      pvals->point[l_idx].rotation = l_startangle + l_angle;
    }
  }
}	/* end p_calculate_rotate_follow */


/* ============================================================================
 * p_gap_chk_keyframes
 *   check if controlpoints and keyframe settings are OK
 *   return pointer to errormessage on Errors
 *      contains "\0" if no errors are found
 * ============================================================================
 */
gchar *p_gap_chk_keyframes(t_mov_values *pvals)
{
  gint   l_affected_frames;
  gint   l_idx;
  gint   l_errcount;
  gint   l_prev_keyframe;
  gint   l_prev_frame;
  gchar *l_err;
  gchar *l_err_lbltext;

  l_affected_frames = 1 + MAX(pvals->dst_range_start, pvals->dst_range_end)
                    - MIN(pvals->dst_range_start, pvals->dst_range_end);
  
  l_errcount = 0;
  l_prev_keyframe = 0;
  l_prev_frame = 0;
  l_err_lbltext = g_strdup("\0");
  
  for(l_idx = 0; l_idx < pvals->point_idx_max; l_idx++ )
  {
     if(pvals->point[l_idx].keyframe_abs != 0)
     {
         pvals->point[l_idx].keyframe = p_conv_keyframe_to_rel(pvals->point[l_idx].keyframe_abs, pvals);

         if(pvals->point[l_idx].keyframe > l_affected_frames - 2)
	 {
	    l_err = g_strdup_printf(_("\nError: Keyframe %d at point [%d] higher or equal than last handled frame")
	                              , pvals->point[l_idx].keyframe_abs,  l_idx+1);
	    l_err_lbltext = g_strdup_printf("%s%s", l_err_lbltext, l_err);
	    g_free(l_err);
	    l_errcount++;
	 }
         if(pvals->point[l_idx].keyframe < l_prev_frame)
	 {
	    l_err = g_strdup_printf(_("\nError: Keyframe %d at point [%d] leaves not enough space (frames)"
	                              "\nfor the previous controlpoints")
				      , pvals->point[l_idx].keyframe_abs, l_idx+1);
	    l_err_lbltext = g_strdup_printf("%s%s", l_err_lbltext, l_err);
	    g_free(l_err);
	    l_errcount++;
	 }
	 
         if(pvals->point[l_idx].keyframe <= l_prev_keyframe)
	 {
	    l_err = g_strdup_printf(_("\nError: Keyframe %d is not in sequence at point [%d]")
	                             , pvals->point[l_idx].keyframe_abs, l_idx+1);
	    l_err_lbltext = g_strdup_printf("%s%s", l_err_lbltext, l_err);
	    g_free(l_err);
	    l_errcount++;
	 }

         l_prev_keyframe = pvals->point[l_idx].keyframe;
	 if(l_prev_keyframe > l_prev_frame)
	 {
	   l_prev_frame = l_prev_keyframe +1;
	 }
     }
     else
     {
	l_prev_frame++;
	if(l_prev_frame +1 > l_affected_frames)
	{
	    l_err = g_strdup_printf(_("\nError: controlpoint [%d] is out of handled framerange"), l_idx+1);
	    l_err_lbltext = g_strdup_printf("%s%s", l_err_lbltext, l_err);
	    g_free(l_err);
	    l_errcount++;
	}
     }
     if(l_errcount > 10)
     {
       break;
     }
  }

  if(pvals->point_idx_max + 1 > l_affected_frames)
  {
	l_err = g_strdup_printf(_("\nError: more controlpoints (%d) than handled frames (%d)"
	                          "\nplease reduce controlpoints or select more frames"),
	                          (int)pvals->point_idx_max+1, (int)l_affected_frames);
	l_err_lbltext = g_strdup_printf("%s%s", l_err_lbltext, l_err);
	g_free(l_err);
  }

  return(l_err_lbltext);
}	/* end p_gap_chk_keyframes */


/* ============================================================================
 * p_check_move_path_params
 *   check the parameters for noninteractive call of MovePath
 *   return 0 (OK)  or -1 (Error)
 * ============================================================================
 */
static gint
p_check_move_path_params(t_mov_data *mov_data)
{
  gchar *l_err_lbltext;
  gint   l_rc;

  l_rc = 0;  /* assume OK */

  /* range params valid ? */
  if(MIN(mov_data->val_ptr->dst_range_start, mov_data->val_ptr->dst_range_end)
      < mov_data->dst_ainfo_ptr->first_frame_nr)
  {
     printf("Error: Range starts before first frame number %d\n",
             (int)mov_data->dst_ainfo_ptr->first_frame_nr);
     l_rc = -1;
  }

  if(MAX(mov_data->val_ptr->dst_range_start, mov_data->val_ptr->dst_range_end)
      > mov_data->dst_ainfo_ptr->last_frame_nr)
  {
     printf("Error: Range ends after last frame number %d\n",
             (int)mov_data->dst_ainfo_ptr->last_frame_nr);
     l_rc = -1;
  }
  
  /* is there a valid source object ? */
  if(mov_data->val_ptr->src_layer_id < 0)
  {
     printf("Error: the passed src_layer_id %d  is invalid\n",
             (int)mov_data->val_ptr->src_layer_id);
     l_rc = -1;
  }
  else if(gimp_drawable_is_layer(mov_data->val_ptr->src_layer_id))
  {
     mov_data->val_ptr->src_image_id = gimp_layer_get_image_id(mov_data->val_ptr->src_layer_id);
  }
  else
  {
     printf("Error: the passed src_layer_id %d  is no Layer\n",
             (int)mov_data->val_ptr->src_layer_id);
     l_rc = -1;
  }
  
  /* keyframes OK ? */
  l_err_lbltext = p_gap_chk_keyframes(mov_data->val_ptr);
  if (*l_err_lbltext != '\0')
  {
     printf("Error in Keyframe settings: %s\n", l_err_lbltext);
     l_rc = -1;
  }
  g_free(l_err_lbltext);
  
  return (l_rc);
}	/* end p_check_move_path_params */


/* ============================================================================
 * gap_move_path
 * ============================================================================
 */
int
gap_move_path(GimpRunModeType run_mode, gint32 image_id, t_mov_values *pvals, gchar *pointfile
             , gint rotation_follow , gint32 startangle)
{
  int l_rc;
  t_anim_info *ainfo_ptr;
  t_mov_data  l_mov_data;

  l_rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    l_mov_data.val_ptr = pvals;
    if(NULL != l_mov_data.val_ptr)
    {
      if (0 == p_dir_ainfo(ainfo_ptr))
      {
        if(0 != p_chk_framerange(ainfo_ptr))   return -1;

        l_mov_data.val_ptr->cache_src_image_id = -1;
        l_mov_data.dst_ainfo_ptr = ainfo_ptr;
        if(run_mode == GIMP_RUN_NONINTERACTIVE)
        {
           l_rc = 0;

	   /* get controlpoints from pointfile */
	   if (pointfile != NULL)
	   {
	     l_rc = p_gap_load_pointfile(pointfile, pvals);
	     if (l_rc < 0)
	     {
	       printf("Execution Error: could not load MovePath controlpoints from file: %s\n",
	               pointfile);
	     }
	   }
	   
	   if(l_rc >= 0)
	   {
	      l_rc = p_check_move_path_params(&l_mov_data);
	   }
	   
	   /* Automatic calculation of rotation values */
	   if((rotation_follow > 0) && (l_rc == 0))
	   {
              p_calculate_rotate_follow(pvals, startangle);
	   }
        }
        else
        {
	   /* Dialog for GIMP_RUN_INTERACTIVE 
	    * (and for GIMP_RUN_WITH_LAST_VALS that is not really supported here)
	    */
           l_rc = p_move_dialog (&l_mov_data);
           if(0 != p_chk_framechange(ainfo_ptr))
           {
              l_rc = -1;
           }
        }

        if(l_rc >= 0)
        {
           l_rc = p_save_named_frame(ainfo_ptr->image_id, ainfo_ptr->old_filename);
           if(l_rc >= 0)
           {
              l_rc = p_mov_execute(&l_mov_data);
           
              /* go back to the frame_nr where move operation was started from */
              p_load_named_frame(ainfo_ptr->image_id, ainfo_ptr->old_filename);
           }
        }

        if(l_mov_data.val_ptr->cache_tmp_image_id >= 0)
        {
	   if(gap_debug)
	   {
	      printf("gap_move: DELETE cache_tmp_image_id:%d\n",
	               (int)l_mov_data.val_ptr->cache_tmp_image_id);
           }
           /* destroy the cached frame image */
           gimp_image_delete(l_mov_data.val_ptr->cache_tmp_image_id);
        }
      }

    }

    p_free_ainfo(&ainfo_ptr);
  }
  
  return(l_rc);    
}	/* end gap_move_path */
