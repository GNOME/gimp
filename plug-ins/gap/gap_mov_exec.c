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

extern      int gap_debug; /* ==0  ... dont print debug infos */

static int p_mov_call_render(t_mov_data *mov_ptr, t_mov_current *cur_ptr);
static void p_mov_advance_src_layer(t_mov_current *cur_ptr, int src_stepmode);
static long   p_mov_execute(t_mov_data *mov_ptr);

/* ============================================================================
 * p_mov_call_render
 *  load current frame, render and save back to disk
 * ============================================================================
 */

int p_mov_call_render(t_mov_data *mov_ptr, t_mov_current *cur_ptr)
{
  t_anim_info *ainfo_ptr;
  gint32  l_tmp_image_id;
  int     l_rc;

  l_rc = 0;
  ainfo_ptr = mov_ptr->dst_ainfo_ptr;
  

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
 * To each affected destination frame exactly one is added.
 * The source layer is iterated through all layers of the sourceimage
 * according to stemmode parameter.
 * For the placement the layers act as if their size is equal to their
 * Sourceimages size.
 * ============================================================================
 */

long   p_mov_execute(t_mov_data *mov_ptr)
{
  int l_idx;
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
   long     l_fridx;
   gdouble  l_flt_count;
   int      l_rc;
   int      l_nlayers;

   if(mov_ptr->val_ptr->src_image_id < 0)
   {
      p_msg_win(mov_ptr->dst_ainfo_ptr->run_mode,
        _("No Source Image was selected\n(Please open a 2nd Image of the same type before opening Move Path)"));
      return -1;
   }

    
  l_percentage = 0.0;  
  if(mov_ptr->dst_ainfo_ptr->run_mode == RUN_INTERACTIVE)
  { 
    gimp_progress_init( _("Copying Layers into Frames .."));
  }

  if(gap_debug)
  {
    printf("p_mov_execute: values got from dialog:\n");
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
   l_rc = p_mov_call_render(mov_ptr, cur_ptr);


   /* how many frames are affected from one line of the moving path */   
   l_fpl = ((gdouble)l_frames - 1.0) / ((gdouble)(l_points -1));
   l_fpl2 = (l_fpl + 0.5);
   
   l_ptidx = 1;
   l_flt_count =  0.0;
   
   
  /* loop for each frame within the range (may step up or down) */
  cur_ptr->dst_frame_nr = val_ptr->dst_range_start;
  for(l_fridx = 1; l_fridx < l_cnt; l_fridx++)
  {
     
     if(gap_debug) printf("p_mov_execute: l_fridx=%ld, l_flt_count=%f, l_rc=%d\n",
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

       /* RENDER add current src_layer to current frame */
       l_rc = p_mov_call_render(mov_ptr, cur_ptr);

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

