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
static long   p_mov_execute(t_mov_data *mov_ptr);

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
         GParam     *l_params;
         gint        l_retvals;
	 gint32      l_size_x, l_size_y;
       
         l_size_x = (gimp_image_width(l_tmp_image_id) * mov_ptr->val_ptr->apv_scalex) / 100;
         l_size_y = (gimp_image_height(l_tmp_image_id) * mov_ptr->val_ptr->apv_scaley) / 100;
       
         l_params = gimp_run_procedure ("gimp_image_scale",
			         &l_retvals,
			         PARAM_IMAGE,    l_tmp_image_id,
			         PARAM_INT32,    l_size_x,
			         PARAM_INT32,    l_size_y,
			         PARAM_END);
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
  gint l_idx;
   t_mov_current l_current_data;
   t_mov_current *cur_ptr;
   t_mov_values  *val_ptr;

   gdouble  l_percentage;  
   gdouble  l_fpl;             /* frames_per_line */
   long     l_fpl2;            /* frames_per_line  rounded to integer*/
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
   
   if(mov_ptr->val_ptr->src_image_id < 0)
   {
      p_msg_win(mov_ptr->dst_ainfo_ptr->run_mode,
        _("No Source Image was selected.\n"
	  "Please open a 2nd Image of the same type before opening Move Path."));
      return -1;
   }

  l_apv_layerstack = 0;
  l_percentage = 0.0;  
  if(mov_ptr->dst_ainfo_ptr->run_mode == RUN_INTERACTIVE)
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
      val_ptr->point[l_cnt].p_x = val_ptr->point[l_cnt].p_x;
      val_ptr->point[l_cnt].p_y = val_ptr->point[l_cnt].p_y;
      val_ptr->point[l_cnt].opacity = val_ptr->point[l_cnt].opacity;
      val_ptr->point[l_cnt].w_resize = val_ptr->point[l_cnt].w_resize;
      val_ptr->point[l_cnt].h_resize = val_ptr->point[l_cnt].h_resize;
      val_ptr->point[l_cnt].rotation = val_ptr->point[l_cnt].rotation;
      l_points = l_cnt;
   }
   
   
   cur_ptr->dst_frame_nr = val_ptr->dst_range_start;
   
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
   
   cur_ptr->currX   = (gdouble)val_ptr->point[0].p_x;
   cur_ptr->currY   = (gdouble)val_ptr->point[0].p_y;
   cur_ptr->currOpacity  = (gdouble)val_ptr->point[0].opacity;
   cur_ptr->currWidth    = (gdouble)val_ptr->point[0].w_resize;
   cur_ptr->currHeight   = (gdouble)val_ptr->point[0].h_resize;
   cur_ptr->currRotation = (gdouble)val_ptr->point[0].rotation;

   /* RENDER add current src_layer to current frame */
   l_rc = p_mov_call_render(mov_ptr, cur_ptr, l_apv_layerstack);


   /* how many frames are affected from one line of the moving path */   
   l_fpl = ((gdouble)l_frames - 1.0) / ((gdouble)(l_points -1));
   l_fpl2 = (l_fpl + 0.5);
   
   l_ptidx = 1;
   l_prev_keyptidx = 0;
   l_flt_count =  0.0;
   l_prev_keyframe = 0;

   if(gap_debug) printf("p_mov_execute: initial l_fpl=%f, l_fpl2=%d\n", l_fpl, (int)l_fpl2);
   
   
  /* loop for each frame within the range (may step up or down) */
  cur_ptr->dst_frame_nr = val_ptr->dst_range_start;
  for(l_fridx = 1; l_fridx < l_cnt; l_fridx++)
  {
     
     if(gap_debug) printf("\np_mov_execute: l_fridx=%ld, l_flt_count=%f, l_rc=%d\n",
                          l_fridx, l_flt_count, (int)l_rc);

     if(l_rc != 0) break;

      /* advance frame_nr, (1st frame was done outside this loop) */
      cur_ptr->dst_frame_nr += l_frame_step;  /* +1  or -1 */

      if((gdouble)l_fridx > l_flt_count)
      {
         /* re-adjust current values. (only to avoid rounding errors)
          * current values should already contain the point values[l_ptidx]
          */
         cur_ptr->currX   = (gdouble)val_ptr->point[l_ptidx -1].p_x;
         cur_ptr->currY   = (gdouble)val_ptr->point[l_ptidx -1].p_y;
         cur_ptr->currOpacity  = (gdouble)val_ptr->point[l_ptidx -1].opacity;
         cur_ptr->currWidth  = (gdouble)val_ptr->point[l_ptidx -1].w_resize;
         cur_ptr->currHeight  = (gdouble)val_ptr->point[l_ptidx -1].h_resize;
         cur_ptr->currRotation  = (gdouble)val_ptr->point[l_ptidx -1].rotation;

	 /* check if there are controlpoints fixed to keyframes */
	 if(l_ptidx > l_prev_keyptidx)
	 {
	   for(l_idk = l_ptidx; l_idk < l_points; l_idk++)
	   {
	      if (val_ptr->point[l_idk].keyframe > 0)
	      {
		/* found a keyframe, have to recalculate frames_per_line */
        	l_fpl = ((gdouble)(val_ptr->point[l_idk].keyframe - l_prev_keyframe)) / ((gdouble)((l_idk -  l_ptidx) +1));
        	l_fpl2 = (l_fpl + 0.5);
        	l_prev_keyframe = val_ptr->point[l_idk].keyframe;

                l_prev_keyptidx = l_idk;
		/* l_flt_count = (gdouble)l_fridx - l_fpl; */
        	if(gap_debug) printf("p_mov_execute: keyframe l_fpl=%f, l_fpl2=%d\n", l_fpl, (int)l_fpl2);
        	break;
	      }
	      if ((l_idk == l_points -1) && (l_prev_keyframe != 0))
	      {
		/* last point is an implicite keyframe (if any keyframe was used ) */
        	l_fpl = ((gdouble)(l_frames - l_prev_keyframe -1)) / ((gdouble)((l_idk -  l_ptidx) +1));
        	l_fpl2 = (l_fpl + 0.5);
        	l_prev_keyframe = val_ptr->point[l_idk].keyframe;

        	if(gap_debug) printf("p_mov_execute: last frame l_fpl=%f, l_fpl2=%d\n", l_fpl, (int)l_fpl2);
        	break;
	      }
	   }
	 }
	 
         /* change deltas for next line of the move path */
         cur_ptr->deltaX  = ((gdouble)val_ptr->point[l_ptidx].p_x  - (gdouble)val_ptr->point[l_ptidx -1].p_x) / (gdouble)l_fpl2;
         cur_ptr->deltaY  = ((gdouble)val_ptr->point[l_ptidx].p_y  - (gdouble)val_ptr->point[l_ptidx -1].p_y) / (gdouble)l_fpl2;
         cur_ptr->deltaOpacity = ((gdouble)val_ptr->point[l_ptidx].opacity  - (gdouble)val_ptr->point[l_ptidx -1].opacity) / (gdouble)l_fpl2;
         cur_ptr->deltaWidth = ((gdouble)val_ptr->point[l_ptidx].w_resize  - (gdouble)val_ptr->point[l_ptidx -1].w_resize) / (gdouble)l_fpl2;
         cur_ptr->deltaHeight = ((gdouble)val_ptr->point[l_ptidx].h_resize  - (gdouble)val_ptr->point[l_ptidx -1].h_resize) / (gdouble)l_fpl2;
         cur_ptr->deltaRotation = ((gdouble)val_ptr->point[l_ptidx].rotation  - (gdouble)val_ptr->point[l_ptidx -1].rotation) / (gdouble)l_fpl2;

         l_ptidx++;
         l_flt_count += l_fpl;
      }

       /* advance settings for next frame */
       p_mov_advance_src_layer(cur_ptr, val_ptr->src_stepmode);

       cur_ptr->currX        += cur_ptr->deltaX;
       cur_ptr->currY        += cur_ptr->deltaY;
       cur_ptr->currOpacity  += cur_ptr->deltaOpacity;
       cur_ptr->currWidth    += cur_ptr->deltaWidth;
       cur_ptr->currHeight   += cur_ptr->deltaHeight;
       cur_ptr->currRotation += cur_ptr->deltaRotation;

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
       if(mov_ptr->dst_ainfo_ptr->run_mode == RUN_INTERACTIVE)
       { 
         l_percentage = (gdouble)l_fridx / (gdouble)(l_cnt -1);
         gimp_progress_update (l_percentage);
       }
           
   }

   if(cur_ptr->src_layers != NULL) g_free(cur_ptr->src_layers);
   
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
  GParam     *l_params;
  gint        l_retvals;
  GImageType  l_type;
  guint       l_width, l_height;
  gint32      l_tattoo;
  gint        l_rc;
  
  l_mov_ptr = &apv_mov_data;
  l_pvals = &apv_mov_vals;
  
  /* copy settings */
  memcpy(l_pvals, pvals_orig, sizeof(t_mov_values));
  l_mov_ptr->val_ptr = l_pvals;
  l_mov_ptr->dst_ainfo_ptr = ainfo_ptr;

  /* -1 assume no tmp_image (use unscaled original source) */
  l_tmp_image_id = -1;

  /* Scale (down) needed ? */
  if((l_pvals->apv_scalex != 100.0) || (l_pvals->apv_scaley != 100.0))
  {
    /* scale the controlpoint koords */
    for(l_idx = 0; l_idx <= l_pvals->point_idx_max; l_idx++)
    {
      l_pvals->point[l_idx].p_x  = (l_pvals->point[l_idx].p_x * l_pvals->apv_scalex) / 100;
      l_pvals->point[l_idx].p_y  = (l_pvals->point[l_idx].p_y * l_pvals->apv_scaley) / 100;
    }

    /* copy and scale the source object image */
    l_tmp_image_id = p_gimp_channel_ops_duplicate(pvals_orig->src_image_id);
    l_pvals->src_image_id = l_tmp_image_id;

    l_size_x = MAX(1, (gimp_image_width(l_tmp_image_id) * l_pvals->apv_scalex) / 100);
    l_size_y = MAX(1, (gimp_image_height(l_tmp_image_id) * l_pvals->apv_scaley) / 100);
    l_params = gimp_run_procedure ("gimp_image_scale",
  		                 &l_retvals,
			         PARAM_IMAGE,    l_tmp_image_id,
			         PARAM_INT32,    l_size_x,
			         PARAM_INT32,    l_size_y,
			         PARAM_END);

    /* find the selected src_layer by tattoo in the copy of src_image */
    l_tattoo = gimp_layer_get_tattoo(pvals_orig->src_layer_id);
    l_pvals->src_layer_id = gimp_image_get_layer_by_tattoo(l_tmp_image_id, l_tattoo);
    
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
			           PARAM_IMAGE,    l_tmp_frame_id,
			           PARAM_INT32,    l_size_x,
			           PARAM_INT32,    l_size_y,
			           PARAM_END);
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
 * gap_move
 * ============================================================================
 */
int gap_move(GRunModeType run_mode, gint32 image_id)
{
  int l_rc;
  t_anim_info *ainfo_ptr;
  t_mov_data  l_mov_data;

  l_rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    l_mov_data.val_ptr = g_malloc(sizeof(t_mov_values));
    if(NULL != l_mov_data.val_ptr)
    {
      if (0 == p_dir_ainfo(ainfo_ptr))
      {
        if(0 != p_chk_framerange(ainfo_ptr))   return -1;

        l_mov_data.dst_ainfo_ptr = ainfo_ptr;
        if(run_mode == RUN_INTERACTIVE)
        {
           l_rc = p_move_dialog (&l_mov_data);
           if(0 != p_chk_framechange(ainfo_ptr))
           {
              l_rc = -1;
           }
        }
        else
        {
           l_rc = -1;  /* run NON_INTERACTIVE not implemented (define args) */
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
      }
      g_free(l_mov_data.val_ptr);
    }

    p_free_ainfo(&ainfo_ptr);
  }
  
  return(l_rc);    
}	/* end gap_move */

