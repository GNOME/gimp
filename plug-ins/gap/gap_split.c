/* gap_range_ops.c
 * 1997.11.06 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains 
 * - gap_split_image
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

/* revision history
 * gimp   1.1.15.1;  1999/05/08  hof: bugix (dont mix GDrawableType with GImageType)
 * 0.96.00; 1998/07/01   hof: - added scale, resize and crop 
 *                              (affects full range == all anim frames)
 *                            - now using gap_arr_dialog.h
 * 0.94.01; 1998/04/28   hof: added flatten_mode to plugin: gap_range_to_multilayer
 * 0.92.00  1998.01.10   hof: bugfix in p_frames_to_multilayer
 *                            layers need alpha (to be raise/lower able) 
 * 0.90.00               first development release
 */
 
/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap_layer_copy.h"
#include "gap_lib.h"
#include "gap_arr_dialog.h"

extern      int gap_debug; /* ==0  ... dont print debug infos */

/* ============================================================================
 * p_split_image
 *
 * returns   value >= 0 if all is ok  return the image_id of 
 *                      the new created image (the last handled anim frame)
 *           (or -1 on error)
 * ============================================================================
 */
static int
p_split_image(t_anim_info *ainfo_ptr,
              char *new_extension,
              gint invers, gint no_alpha)
{
  GImageType l_type;
  guint   l_width, l_height;
  GRunModeType l_run_mode;
  gint32  l_new_image_id;
  gint    l_nlayers;
  gint32 *l_layers_list;
  gint32  l_src_layer_id;
  gint32  l_cp_layer_id;
  gint    l_src_offset_x, l_src_offset_y;    /* layeroffsets as they were in src_image */
  gdouble l_percentage, l_percentage_step;
  char   *l_sav_name;
  gint32  l_rc;
  int     l_idx;
  long    l_layer_idx;

  l_rc = -1;
  l_percentage = 0.0;
  l_run_mode  = ainfo_ptr->run_mode;
  if(ainfo_ptr->run_mode == RUN_INTERACTIVE)
  { 
    gimp_progress_init("Splitting into Frames ..");
  }
 
  
  l_new_image_id = -1;
  /* get info about the image  */
  l_width  = gimp_image_width(ainfo_ptr->image_id);
  l_height = gimp_image_height(ainfo_ptr->image_id);
  l_type = gimp_image_base_type(ainfo_ptr->image_id);

  l_layers_list = gimp_image_get_layers(ainfo_ptr->image_id, &l_nlayers);
  if(l_layers_list != NULL)
  {
    l_percentage_step = 1.0 / (l_nlayers);

    for(l_idx = 0; l_idx < l_nlayers; l_idx++)
    {
       if(l_new_image_id >= 0)
       {
          /* destroy the tmp image (it was saved to disk before) */
          gimp_image_delete(l_new_image_id);
      }
       
       if(invers == TRUE) l_layer_idx = l_idx;
       else               l_layer_idx = (l_nlayers - 1 ) - l_idx;

       l_src_layer_id = l_layers_list[l_layer_idx];

       /* create new image */
       l_new_image_id =  gimp_image_new(l_width, l_height,l_type);
       if(l_new_image_id < 0)
       {
         l_rc = -1;
         break;
       }

       /* copy the layer */
       l_cp_layer_id = p_my_layer_copy(l_new_image_id,
                                     l_src_layer_id,
                                     100.0,   /* Opacity */
                                     0,       /* NORMAL */
                                     &l_src_offset_x,
                                     &l_src_offset_y);
       /* add the copied layer to current destination image */
        gimp_image_add_layer(l_new_image_id, l_cp_layer_id, 0);
        gimp_layer_set_offsets(l_cp_layer_id, l_src_offset_x, l_src_offset_y);
     
       /* delete alpha channel ? */
       if (no_alpha == TRUE)
       {
           /* add a dummy layer (flatten needs at least 2 layers) */
           l_cp_layer_id = gimp_layer_new(l_new_image_id, "dummy",
                                          4, 4,         /* width, height */
                                          ((l_type * 2 ) + 1),  /* convert from GImageType to GDrawableType, and add alpha */
                                          0.0,          /* Opacity full transparent */     
                                          0);           /* NORMAL */
           gimp_image_add_layer(l_new_image_id, l_cp_layer_id, 0);
           gimp_image_flatten (l_new_image_id);
         
       }
       
       
       /* build the name for output image */
       l_sav_name = p_alloc_fname(ainfo_ptr->basename,
                                  (l_idx +1),       /* start at 1 (not at 0) */
                                  new_extension);
       if(l_sav_name != NULL)
       {
         /* save with selected save procedure
          * (regardless if image was flattened or not)
          */
          l_rc = p_save_named_image(l_new_image_id, l_sav_name, l_run_mode);
          if(l_rc < 0)
          {
            p_msg_win(ainfo_ptr->run_mode, "Split Frames: SAVE operation FAILED\n- desired save plugin cant handle type\n- or desired save plugin not available\n");
            break;
          }

          l_run_mode  = RUN_NONINTERACTIVE;  /* for all further calls */

          /* set image name */
          gimp_image_set_filename (l_new_image_id, l_sav_name);
          
          /* prepare return value */
          l_rc = l_new_image_id;

          g_free(l_sav_name);
       }
       
       /* save as frame */
       
 
       /* show progress bar */
       if(ainfo_ptr->run_mode == RUN_INTERACTIVE)
       { 
         l_percentage += l_percentage_step;
         gimp_progress_update (l_percentage);
       }

      
    }
    g_free (l_layers_list);
  }
  

  return l_rc;  
}	/* end p_split_image */


/* ============================================================================
 * p_split_dialog
 *
 *   return  0 (OK) 
 *          or  -1 in case of Error or cancel
 * ============================================================================
 */
static long
p_split_dialog(t_anim_info *ainfo_ptr, gint *inverse_order, gint *no_alpha, char *extension, gint len_ext)
{
  static t_arr_arg  argv[4];
  char   buf[128];
  
  sprintf(buf, "%s\n%s\n(%s_0001.%s)\n",
          "Make a frame (diskfile) from each Layer",
          "frames are named: base_nr.extension",
           ainfo_ptr->basename, extension);
 
  p_init_arr_arg(&argv[0], WGT_LABEL);
  argv[0].label_txt = &buf[0];

  p_init_arr_arg(&argv[1], WGT_TEXT);
  argv[1].label_txt ="Extension:";
  argv[1].help_txt  ="extension of resulting frames       \n(is also used to define Fileformat)";
  argv[1].text_buf_len = len_ext;
  argv[1].text_buf_ret = extension;

  p_init_arr_arg(&argv[2], WGT_TOGGLE);
  argv[2].label_txt = "Inverse Order :";
  argv[2].help_txt  = "Start frame 0001 at Top Layer";
  argv[2].int_ret   = 0;

  p_init_arr_arg(&argv[3], WGT_TOGGLE);
  argv[3].label_txt = "Flatten :";
  argv[3].help_txt  = "Remove Alpha Channel in resulting Frames,    \ntransparent parts are filled with BG color";
  argv[3].int_ret   = 0;

  if(TRUE == p_array_dialog("Split Image into Frames",
                                 "Split Settings :", 
                                  4, argv))
  {
       *inverse_order = argv[2].int_ret;
       *no_alpha      = argv[3].int_ret;
       return 0;
  }
  else
  {
     return -1;
  }
}		/* end p_split_dialog */

/* ============================================================================
 * gap_split_image
 *    Split one (multilayer) image into anim-frames
 *    one frame per layer.
 * ============================================================================
 */
int gap_split_image(GRunModeType run_mode,
                      gint32     image_id,
                      gint32     inverse_order,
                      gint32     no_alpha,
                      char      *extension)

{
  gint32  l_new_image_id;
  gint32  l_rc;
  gint32  l_inverse_order;
  gint32  l_no_alpha;
  
  t_anim_info *ainfo_ptr;
  char l_extension[32];

  strcpy(l_extension, ".xcf");

  l_rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      if(ainfo_ptr->frame_cnt != 0)
      {
         p_msg_win(run_mode,
           "OPERATION CANCELLED\nThis image is already an AnimFrame\nTry again on a Duplicate\n(image/channel ops/duplicate)");
         return -1;
      }
      else
      {
        if(run_mode == RUN_INTERACTIVE)
        {
           l_rc = p_split_dialog (ainfo_ptr, &l_inverse_order, &l_no_alpha, &l_extension[0], sizeof(l_extension));
        }
        else
        {
           l_inverse_order  =  inverse_order;
           l_no_alpha       =  no_alpha;
           strncpy(l_extension, extension, sizeof(l_extension) -1);
           l_extension[sizeof(l_extension) -1] = '\0';

        }

        if(l_rc >= 0)
        {
           l_new_image_id = p_split_image(ainfo_ptr,
                               l_extension,
                               l_inverse_order,
                               l_no_alpha);
           
           /* create a display for the new created image
            * (it is the first or the last frame of the 
            *  new created animation sequence)
            */                    
           gimp_display_new(l_new_image_id);
           l_rc = l_new_image_id;
        }
      }
    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(l_rc);    
}	/* end   gap_split_image */

