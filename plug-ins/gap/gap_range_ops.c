/* gap_range_ops.c
 * 1997.11.06 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains implementation of range based frame operations.
 * - gap_range_to_multilayer 
 * - gap_range_flatten
 * - gap_range_layer_del
 * - gap_range_conv
 * - gap_anim_scale
 * - gap_anim_resize
 * - gap_anim_crop
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
 * 0.97.00; 1998/10/19   hof: gap_range_to_multilayer: extended layer selection
 * 0.96.03; 1998/08/31   hof: gap_range_to_multilayer: all params available
 *                            in non-interactive runmode
 * 0.96.02; 1998/08/05   hof: - p_frames_to_multilayer added framerate support
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
#include "gap_match.h"
#include "gap_arr_dialog.h"
#include "gap_resi_dialog.h"
#include "gap_mod_layer.h"
#include "gap_range_ops.h"

extern      int gap_debug; /* ==0  ... dont print debug infos */


/* ============================================================================
 * p_anim_sizechange_dialog
 *   dialog window with 2 (or 4) entry fields
 *   where the user can select the new Anim Frame (Image)-Size
 *   (if cnt == 4 additional Inputfields for offests are available)
 * return -1  in case of cancel or any error
 *            (include check for change of current frame)
 * return positve (0 or layerstack position) if everythig OK
 * ============================================================================
 */
static int 
p_anim_sizechange_dialog(t_anim_info *ainfo_ptr, t_gap_asiz asiz_mode,
               long *size_x, long *size_y, 
               long *offs_x, long *offs_y)
{
  static t_arr_arg  argv[4];
  gint  cnt;
  char *title;
  char  hline[100];
  gint   l_width;
  gint   l_height;
  gint   l_rc;

  /* get info about the image (size is common to all frames) */
  l_width  = gimp_image_width(ainfo_ptr->image_id);
  l_height = gimp_image_height(ainfo_ptr->image_id);
 
  p_init_arr_arg(&argv[0], WGT_INT_PAIR);
  argv[0].label_txt = "New Width :";
  argv[0].constraint = FALSE;
  argv[0].int_min   = 1;
  argv[0].int_max   = 1024;
  argv[0].int_ret   = l_width;
  
  p_init_arr_arg(&argv[1], WGT_INT_PAIR);
  argv[1].label_txt = "New Height :";
  argv[1].constraint = FALSE;
  argv[1].int_min    = 1;
  argv[1].int_max    = 1024;
  argv[1].int_ret   = l_height;
  
  p_init_arr_arg(&argv[2], WGT_INT_PAIR);
  argv[2].label_txt = "Offest X :";
  argv[2].constraint = FALSE;
  argv[2].int_min    = 0;
  argv[2].int_max    = l_width;
  argv[2].int_ret   = 0;
  
  p_init_arr_arg(&argv[3], WGT_INT_PAIR);
  argv[3].label_txt = "Offest Y :";
  argv[3].constraint = FALSE;
  argv[3].int_min    = 0;
  argv[3].int_max    = l_height;
  argv[3].int_ret   = 0;

  switch(asiz_mode)
  {
    case ASIZ_CROP:
      title = "Crop AnimFrames (all)";
      sprintf(hline, "Crop (original %dx%d)", l_width, l_height);
      argv[0].int_max   = l_width;
      argv[0].constraint = TRUE;
      argv[1].int_max   = l_height;
      argv[1].constraint = TRUE;
      argv[2].constraint = TRUE;
      argv[3].constraint = TRUE;
      cnt = 4;
      break;
    case ASIZ_RESIZE:
      title = "Resize AnimFrames (all)";
      sprintf(hline, "Resize (original %dx%d)", l_width, l_height);
      argv[2].int_min    = -l_width;
      argv[3].int_min    = -l_height;
     cnt = 4;
      break;
    default:
      title = "Scale AnimFrames (all)";
      sprintf(hline, "Scale (original %dx%d)", l_width, l_height);
      cnt = 2;
      break;
  }
  
  if(0 != p_chk_framerange(ainfo_ptr))   return -1;

  /* array dialog can handle all asiz_mode type (and is already prepared for)
   * BUT: RESIZE and SCALE should use the same dialogs as used in gimp
   *      on single Images.
   *      Therfore I made a procedure p_resi_dialog
   */
  if(asiz_mode == ASIZ_CROP)
  {
    l_rc = p_array_dialog(title, hline, cnt, argv);

      *size_x = (long)(argv[0].int_ret);
      *size_y = (long)(argv[1].int_ret);
      *offs_x = (long)(argv[2].int_ret);
      *offs_y = (long)(argv[3].int_ret);

      /* Clip size down to image borders */      
      if((*size_x + *offs_x) > l_width)
      {
        *size_x = l_width - *offs_x;
      }
      if((*size_y + *offs_y) > l_height)
      {
        *size_y = l_height - *offs_y;
      }
  }
  else
  {
    l_rc = p_resi_dialog(ainfo_ptr->image_id, asiz_mode, title,
                         size_x, size_y, offs_x, offs_y);
  }
  

  if(l_rc == TRUE)
  {
       if(0 != p_chk_framechange(ainfo_ptr))
       {
           return -1;
       }
       return 0;        /* OK */
  }
  else
  {
     return -1;
  }
   
}	/* end p_anim_sizechange_dialog */


/* ============================================================================
 * p_range_dialog
 *   dialog window with 2 (or 3) entry fields
 *   where the user can select a frame range (FROM TO)
 *   (if cnt == 3 additional Layerstackposition)
 * return -1  in case of cancel or any error
 *            (include check for change of current frame)
 * return positve (0 or layerstack position) if everythig OK
 * ============================================================================
 */
static int 
p_range_dialog(t_anim_info *ainfo_ptr,
               long *range_from, long *range_to, 
               char *title, char *hline, gint cnt)
{
  static t_arr_arg  argv[3];


  if(cnt != 3)  cnt = 2;

  p_init_arr_arg(&argv[0], WGT_INT_PAIR);
  argv[0].label_txt = "From :";
  argv[0].constraint = TRUE;
  argv[0].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[0].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[0].int_ret   = (gint)ainfo_ptr->curr_frame_nr;
  
  p_init_arr_arg(&argv[1], WGT_INT_PAIR);
  argv[1].label_txt = "To :";
  argv[1].constraint = TRUE;
  argv[1].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[1].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[1].int_ret   = (gint)ainfo_ptr->last_frame_nr;
  
  p_init_arr_arg(&argv[2], WGT_INT_PAIR);
  argv[2].label_txt = "Layerstack :";
  argv[2].constraint = FALSE;
  argv[2].int_min   = 0;
  argv[2].int_max   = 99;
  argv[2].int_ret   = 0;
  
  if(0 != p_chk_framerange(ainfo_ptr))   return -1;
  if(TRUE == p_array_dialog(title, hline, cnt, argv))
  {   *range_from = (long)(argv[0].int_ret);
      *range_to   = (long)(argv[1].int_ret);

       if(0 != p_chk_framechange(ainfo_ptr))
       {
           return -1;
       }
       return (int)(argv[2].int_ret);
  }
  else
  {
     return -1;
  }
   
}	/* end p_range_dialog */

/* ============================================================================
 * p_convert_dialog
 *
 *   return  0 .. OK 
 *          -1 .. in case of Error or cancel
 * ============================================================================
 */
static long
p_convert_dialog(t_anim_info *ainfo_ptr,
                      long *range_from, long *range_to, long *flatten,
                      GImageType *dest_type, gint32 *dest_colors, gint32 *dest_dither,
                      char *basename, gint len_base,
                      char *extension, gint len_ext)
{
  static t_arr_arg  argv[9];
  static char *radio_args[4]  = {"KEEP_TYPE", "Conv to RGB", "Conv to GRAY", "Conv to INDEXED" };
  
  p_init_arr_arg(&argv[0], WGT_INT_PAIR);
  argv[0].constraint = TRUE;
  argv[0].label_txt = "From Frame:";
  argv[0].help_txt  = "first handled frame";
  argv[0].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[0].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[0].int_ret   = (gint)ainfo_ptr->curr_frame_nr;
  
  p_init_arr_arg(&argv[1], WGT_INT_PAIR);
  argv[1].constraint = TRUE;
  argv[1].label_txt = "To   Frame:";
  argv[1].help_txt  = "last handled frame";
  argv[1].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[1].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[1].int_ret   = (gint)ainfo_ptr->last_frame_nr;

  p_init_arr_arg(&argv[2], WGT_LABEL);
  argv[2].label_txt ="\nSelect destination fileformat by extension\noptionally convert imagetype\n";

  p_init_arr_arg(&argv[3], WGT_FILESEL);
  argv[3].label_txt ="Basename:";
  argv[3].help_txt  ="basename of the resulting frames       \n(_0001.ext is added)";
  argv[3].text_buf_len = len_base;
  argv[3].text_buf_ret = basename;

  p_init_arr_arg(&argv[4], WGT_TEXT);
  argv[4].label_txt ="Extension:";
  argv[4].help_txt  ="extension of resulting frames       \n(is also used to define Fileformat)";
  argv[4].text_buf_len = len_ext;
  argv[4].text_buf_ret = extension;
  

  p_init_arr_arg(&argv[5], WGT_OPTIONMENU);
  argv[5].label_txt ="Imagetype :";
  argv[5].help_txt  ="Convert to, or keep imagetype           \n(most fileformats cant handle all types)";
  argv[5].radio_argc  = 4;
  argv[5].radio_argv = radio_args;
  argv[5].radio_ret  = 0;

  p_init_arr_arg(&argv[6], WGT_TOGGLE);
  argv[6].label_txt = "Flatten  :";
  argv[6].help_txt  ="Flatten all resulting frames               \n(most fileformats need flattened frames)";
  argv[6].int_ret   = 1;

  p_init_arr_arg(&argv[7], WGT_INT_PAIR);
  argv[7].constraint = TRUE;
  argv[7].label_txt = "Colors  :";
  argv[7].help_txt  = "Number of resulting Colors               \n(ignored if not converted to indexed)";
  argv[7].int_min   = 2;
  argv[7].int_max   = 256;
  argv[7].int_ret   = 255;

  p_init_arr_arg(&argv[8], WGT_TOGGLE);
  argv[8].label_txt = "Dither  :";
  argv[8].help_txt  = "Enable Floyd-Steinberg dithering      \n(ignored if not converted to indexed)";
  argv[8].int_ret   = 1;

  if(0 != p_chk_framerange(ainfo_ptr))   return -1;

  if(TRUE == p_array_dialog("Convert Frames to other Formats",
                                 "Convert Settings :", 
                                  9, argv))
  {
      *range_from  = (long)(argv[0].int_ret);
      *range_to    = (long)(argv[1].int_ret);
      switch(argv[5].radio_ret)
      {
        case 1: 
           *dest_type = RGB;
           break;
        case 2: 
           *dest_type = GRAY;
           break;
        case 3: 
           *dest_type = INDEXED;
           break;
        default:
           *dest_type = 9444;
           break;
      }
      *flatten     = (long)(argv[6].int_ret);
      *dest_colors = (gint32)(argv[7].int_ret);
      *dest_dither = (long)(argv[8].int_ret);

       if(0 != p_chk_framechange(ainfo_ptr))
       {
           return -1;
       }
       return 0;
  }
  else
  {
     return -1;
  }
}		/* end p_convert_dialog */

/* ============================================================================
 * p_range_to_multilayer_dialog
 *   dialog window with 4 entry fields
 *   where the user can select a frame range (FROM TO)
 * return -1  in case of cancel or any error
 *            (include check for change of current frame)
 * return positve (0 or layerstack position) if everythig OK
 * ============================================================================
 */
static int 
p_range_to_multilayer_dialog(t_anim_info *ainfo_ptr,
               long *range_from, long *range_to, 
               long *flatten_mode, long *bg_visible,
               long *framrate, char *frame_basename, gint len_frame_basename,
               char *title, char *hline,

	       gint32 *sel_mode, gint32 *sel_case,
	       gint32 *sel_invert, char *sel_pattern)
{
  static t_arr_arg  argv[10];
  
  static char *radio_args[4]  = {"Expand as necessary", 
                                 "Clipped to image",
                                 "Clipped to bottom layer", 
                                 "Flattened image" };
  static char *radio_help[4]  = {"Resulting Layer Size is made of the outline-rectangle \nof all visible layers (may differ from frame to frame)", 
                                 "Resulting Layer Size is the frame size",
                                 "Resulting Layer Size is the size of the bottom layer\n(may differ from frame to frame)", 
                                 "Resulting Layer Size is the frame size     \ntransparent parts are filled with BG color" };
  /* Layer select modes */
  static char *sel_args[7]    = {"Pattern is equal to LayerName",
                                  "Pattern is Start of LayerName",
                                  "Pattern is End of Layername",
                                  "Pattern is a Part of LayerName",
                                  "Pattern is LayerstackNumber List",
                                  "Pattern is REVERSE-stack List",
                                  "All Visible (ignore Pattern)"
                                  };
  static char *sel_help[7]    = {"select all Layers where Layername is equal to Pattern",
                                  "select all Layers where Layername starts with Pattern",
                                  "select all Layers where Layername ends up with Pattern",
                                  "select all Layers where Layername contains Pattern",
                                  "select Layerstack positions.\n0, 4-5, 8\nwhere 0 == Top-layer",
                                  "select Layerstack positions.\n0, 4-5, 8\nwhere 0 == BG-layer",
                                  "select all visible Layers"
                                  };


  p_init_arr_arg(&argv[0], WGT_INT_PAIR);
  argv[0].constraint = TRUE;
  argv[0].label_txt = "From :";
  argv[0].help_txt  = "first handled frame";
  argv[0].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[0].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[0].int_ret   = (gint)ainfo_ptr->curr_frame_nr;
  
  p_init_arr_arg(&argv[1], WGT_INT_PAIR);
  argv[1].constraint = TRUE;
  argv[1].label_txt = "To :";
  argv[1].help_txt  = "last handled frame";
  argv[1].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[1].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[1].int_ret   = (gint)ainfo_ptr->last_frame_nr;

  p_init_arr_arg(&argv[2], WGT_TEXT);
  argv[2].label_txt ="Layer Basename:";
  argv[2].help_txt  ="Basename for all Layers    \n[####] is replaced by frame number";
  argv[2].text_buf_len = len_frame_basename;
  argv[2].text_buf_ret = frame_basename;

  /* Framerate is not used any longer */
/*  
  p_init_arr_arg(&argv[3], WGT_INT_PAIR);
  argv[3].constraint = FALSE;
  argv[3].label_txt = "Framerate :";
  argv[3].help_txt  = "Framedelay in ms";
  argv[3].int_min   = (gint)0;
  argv[3].int_max   = (gint)300;
  argv[3].int_ret   = (gint)50;
 */
  p_init_arr_arg(&argv[3], WGT_LABEL);
  argv[3].label_txt = " ";
  
  p_init_arr_arg(&argv[4], WGT_RADIO);
  argv[4].label_txt = "Layer Mergemode :";
  argv[4].radio_argc = 4;
  argv[4].radio_argv = radio_args;
  argv[4].radio_help_argv = radio_help;
  argv[4].radio_ret  = 1;
  
  p_init_arr_arg(&argv[5], WGT_TOGGLE);
  argv[5].label_txt = "Exclude BG-Layer";
  argv[5].help_txt  = "Exclude the BG-Layers    \nin all handled frames\nregardless to selection";
  argv[5].int_ret   = 0;   /* 1: exclude BG Layer from all selections */


  /* Layer select mode RADIO buttons */
  p_init_arr_arg(&argv[6], WGT_RADIO);
  argv[6].label_txt = "Select Layer(s):";
  argv[6].radio_argc = 7;
  argv[6].radio_argv = sel_args;
  argv[6].radio_help_argv = sel_help;
  argv[6].radio_ret  = 6;

  /* Layer select pattern string */
  sprintf (sel_pattern, "0");
  p_init_arr_arg(&argv[7], WGT_TEXT);
  argv[7].label_txt ="Select Pattern:";
  argv[7].entry_width = 140;       /* pixel */
  argv[7].help_txt  ="String to identify layer names    \nor layerstack position numbers\n0,3-5";
  argv[7].text_buf_len = MAX_LAYERNAME;
  argv[7].text_buf_ret = sel_pattern;

  /* case sensitive checkbutton */
  p_init_arr_arg(&argv[8], WGT_TOGGLE);
  argv[8].label_txt = "Case sensitive";
  argv[8].help_txt  = "Lowercase and UPPERCASE letters are considered as different";
  argv[8].int_ret   = 1;

  /* invert selection checkbutton */
  p_init_arr_arg(&argv[9], WGT_TOGGLE);
  argv[9].label_txt = "Invert Selection";
  argv[9].help_txt  = "Use all unselected Layers";
  argv[9].int_ret   = 0;



  
  if(0 != p_chk_framerange(ainfo_ptr))   return -1;

  if(TRUE == p_array_dialog(title, hline, 10, argv))
  {   *range_from   = (long)(argv[0].int_ret);
      *range_to     = (long)(argv[1].int_ret);
      *framrate     = (long)(argv[3].int_ret);
      *flatten_mode = (long)(argv[4].int_ret);
      if (argv[5].int_ret == 0)  *bg_visible = 1; /* 1: use BG like any Layer */
      else                       *bg_visible = 0; /* 0: exclude (ignore) BG Layer */

      *sel_mode     = argv[6].int_ret;
                   /*     [7] sel_pattern  */
      *sel_case     = argv[8].int_ret;
      *sel_invert   = argv[9].int_ret;

       if(0 != p_chk_framechange(ainfo_ptr))
       {
           return -1;
       }
       return 0;
  }
  else
  {
     return -1;
  }
   
}	/* end p_range_to_multilayer_dialog */


/* ============================================================================
 * p_frames_to_multilayer
 * returns   image_id of the new created multilayer image
 *           (or -1 on error)
 * ============================================================================
 */
static gint32
p_frames_to_multilayer(t_anim_info *ainfo_ptr,
                      long range_from, long range_to,
                      long flatten_mode, long bg_visible,
                      long framerate, char *frame_basename,
		      gint32 sel_mode, gint32 sel_case,
		      gint32 sel_invert, char *sel_pattern)
{
  GImageType l_type;
  guint   l_width, l_height;
  long    l_cur_frame_nr;
  long    l_step, l_begin, l_end;
  long    l_vidx;
  gint32  l_tmp_image_id;
  gint32  l_new_image_id;
  gint32  l_cp_layer_id;
  gint32  l_tmp_layer_id;
  gint    l_src_offset_x, l_src_offset_y;    /* layeroffsets as they were in src_image */
  gint       l_nlayers;
  gint32    *l_layers_list;
  gint       l_visible;
  gint       l_nvisible;
  gint       l_nlayers_result;
  gdouble    l_percentage, l_percentage_step;  
  static  char l_layername[256];
  t_LayliElem *l_layli_ptr;
  gint32     l_sel_cnt;


  l_percentage = 0.0;
  l_nlayers_result = 0;
  if(ainfo_ptr->run_mode == RUN_INTERACTIVE)
  { 
    gimp_progress_init("Creating Layer-Animated Image ..");
  }
 
  l_tmp_layer_id = -1;
  
  /* get info about the image (size and type is common to all frames) */
  l_width  = gimp_image_width(ainfo_ptr->image_id);
  l_height = gimp_image_height(ainfo_ptr->image_id);
  l_type   = gimp_image_base_type(ainfo_ptr->image_id);
  
  l_new_image_id = gimp_image_new(l_width, l_height,l_type);
  l_visible = TRUE;   /* only the 1.st layer should be visible */

  l_begin = range_from;
  l_end   = range_to;
  
  if(range_from > range_to)
  {
    l_step  = -1;     /* operate in descending (reverse) order */
    l_percentage_step = 1.0 / ((1.0 + range_from) - range_to);

    if(range_to < ainfo_ptr->first_frame_nr)
    { l_begin = ainfo_ptr->first_frame_nr;
    }
    if(range_from > ainfo_ptr->last_frame_nr)
    { l_end = ainfo_ptr->last_frame_nr;
    }
  }
  else
  {
    l_step  = 1;      /* operate in ascending order */
    l_percentage_step = 1.0 / ((1.0 + range_to) - range_from);

    if(range_from < ainfo_ptr->first_frame_nr)
    { l_begin = ainfo_ptr->first_frame_nr;
    }
    if(range_to > ainfo_ptr->last_frame_nr)
    { l_end = ainfo_ptr->last_frame_nr;
    }
  }
  
  
  l_cur_frame_nr = l_begin;
  while(1)
  {
    /* build the frame name */
    if(ainfo_ptr->new_filename != NULL) free(ainfo_ptr->new_filename);
    ainfo_ptr->new_filename = p_alloc_fname(ainfo_ptr->basename,
                                        l_cur_frame_nr,
                                        ainfo_ptr->extension);
    if(ainfo_ptr->new_filename == NULL)
       goto error;

    /* load current frame into temporary image */
    l_tmp_image_id = p_load_image(ainfo_ptr->new_filename);
    if(l_tmp_image_id < 0)
       goto error;
       
    /* get informations (id, visible, selected) about all layers */
    l_layli_ptr = p_alloc_layli(l_tmp_image_id, &l_sel_cnt, &l_nlayers,
                               sel_mode, sel_case, sel_invert, sel_pattern);
    if(l_layli_ptr == NULL)
        goto error;

    l_nvisible  = l_sel_cnt;  /* count visible Layers == all selected layers */
    for(l_vidx=0; l_vidx < l_nlayers; l_vidx++)
    {
      /* set all selected layers visible, all others invisible */
      l_tmp_layer_id = l_layli_ptr[l_vidx].layer_id;
      gimp_layer_set_visible(l_tmp_layer_id,
                             l_layli_ptr[l_vidx].selected);

      if((bg_visible == 0) && (l_vidx == (l_nlayers -1)))
      {
         /* set BG_Layer invisible */
         gimp_layer_set_visible(l_tmp_layer_id, FALSE);
	 if(l_layli_ptr[l_vidx].selected)
	 {
	   l_nvisible--;
	 }
      }
    }

    free(l_layli_ptr);

    if((flatten_mode >= FLAM_MERG_EXPAND) && (flatten_mode <= FLAM_MERG_CLIP_BG))
    {
       if(gap_debug) fprintf(stderr, "p_frames_to_multilayer: %d MERGE visible layers=%d\n", (int)flatten_mode, (int)l_nvisible);

       /* merge all visible Layers */
       if(l_nvisible > 1)  gimp_image_merge_visible_layers  (l_tmp_image_id, flatten_mode);
    }
    else
    {
        if(gap_debug) fprintf(stderr, "p_frames_to_multilayer: %d FLATTEN\n", (int)flatten_mode);
        /* flatten temporary image (reduce to single layer) */
        gimp_image_flatten (l_tmp_image_id);
    }

    /* copy (the only visible) layer from temporary image */
    l_layers_list = gimp_image_get_layers(l_tmp_image_id, &l_nlayers);
    if(l_layers_list != NULL)
    {
      for(l_vidx=0; l_vidx < l_nlayers; l_vidx++)
      {
        l_tmp_layer_id = l_layers_list[l_vidx];

        /* stop at 1.st visible layer (this should be the only visible layer) */        
        if(gimp_layer_get_visible(l_tmp_layer_id)) break;

        /* stop at 1.st layer if image was flattened */
        if((flatten_mode < FLAM_MERG_EXPAND) || (flatten_mode > FLAM_MERG_CLIP_BG))  break;
      }
      g_free (l_layers_list);
      
      if(l_vidx < l_nlayers)
      {
         l_cp_layer_id = p_my_layer_copy(l_new_image_id,
                                     l_tmp_layer_id,
                                     100.0,   /* Opacity */
                                     0,       /* NORMAL */
                                     &l_src_offset_x,
                                     &l_src_offset_y);

        /* add the copied layer to current destination image */
        gimp_image_add_layer(l_new_image_id, l_cp_layer_id, 0);
        gimp_layer_set_offsets(l_cp_layer_id, l_src_offset_x, l_src_offset_y);

        l_nlayers_result++;


        /* add aplha channel to all layers
         * (without alpha raise and lower would not work on that layers)
         */
        gimp_layer_add_alpha(l_cp_layer_id);

        /* set name and visibility */
        if (frame_basename == NULL)  frame_basename = "frame_[####]";
        if (*frame_basename == '\0') frame_basename = "frame_[####]";
        
	p_substitute_framenr(&l_layername[0], sizeof(l_layername),
	                      frame_basename, (long)l_cur_frame_nr);
        gimp_layer_set_name(l_cp_layer_id, &l_layername[0]);

        gimp_layer_set_visible(l_cp_layer_id, l_visible);
        l_visible = FALSE;   /* all further layers are set invisible */
      }
      /* else: tmp image has no visible layers, ignore that frame */
    }

    /* destroy the tmp image */
    gimp_image_delete(l_tmp_image_id);
    
    if(ainfo_ptr->run_mode == RUN_INTERACTIVE)
    { 
      l_percentage += l_percentage_step;
      gimp_progress_update (l_percentage);
    }

    /* advance to next frame */
    if(l_cur_frame_nr == l_end)
       break;
    l_cur_frame_nr += l_step;

  }

  p_prevent_empty_image(l_new_image_id);
  
  return l_new_image_id;

error:
  gimp_image_delete(l_new_image_id);
  return -1;
  
}	/* end p_frames_to_multilayer */



/* ============================================================================
 * gap_range_to_multilayer
 * ============================================================================
 */
gint32 gap_range_to_multilayer(GRunModeType run_mode, gint32 image_id,
                             long range_from, long range_to,
                             long flatten_mode, long bg_visible,
                             long   framerate,
			     char  *frame_basename, int frame_basename_len,
			     gint32 sel_mode, gint32 sel_case,
			     gint32 sel_invert, char *sel_pattern)
{
  gint32  new_image_id;
  gint32  l_rc;
  long   l_from, l_to;
  t_anim_info *ainfo_ptr;
  gint32    l_sel_mode;
  gint32    l_sel_case;
  gint32    l_sel_invert;
  
  char      l_sel_pattern[MAX_LAYERNAME];

  l_rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      if(run_mode == RUN_INTERACTIVE)
      {

         strcpy(frame_basename, "frame_[####]");
         framerate = 0;
         l_rc = p_range_to_multilayer_dialog (ainfo_ptr, &l_from, &l_to,
                                &flatten_mode, &bg_visible,
                                &framerate, frame_basename, frame_basename_len, 
                                "Frames to Image",
                                "Create Multilayer-Image from Frames",
				&l_sel_mode, &sel_case,
				&sel_invert, &l_sel_pattern[0]
				);
      }
      else
      {
         l_from = range_from;
         l_to   = range_to;
         l_rc = 0;
	 l_sel_mode    = sel_mode;
	 l_sel_case    = sel_case;
	 l_sel_invert  = sel_invert;
  
         strncpy(&l_sel_pattern[0], sel_pattern, sizeof(l_sel_pattern) -1);
	 l_sel_pattern[sizeof(l_sel_pattern) -1] = '\0';
      }

      if(l_rc >= 0)
      {
         new_image_id = p_frames_to_multilayer(ainfo_ptr, l_from, l_to, 
                                               flatten_mode, bg_visible,
                                               framerate, frame_basename,
					       l_sel_mode, sel_case,
					       sel_invert, &l_sel_pattern[0]);
         gimp_display_new(new_image_id);
         l_rc = new_image_id;
      }
    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(l_rc);    
}	/* end gap_range_to_multilayer */

/* ============================================================================
 * p_type_convert
 *   convert image to desired type (reduce to dest_colors for INDEXED type)
 * ============================================================================
 */
static int
p_type_convert(gint32 image_id, GImageType dest_type, gint32 dest_colors, gint32 dest_dither)
{
  GParam     *l_params;
  gint        l_retvals;
  int         l_rc;

  l_rc = 0;
  l_params = NULL;

  switch(dest_type)
  {
    case INDEXED:
      if(gap_debug) fprintf(stderr, "DEBUG: p_type_convert to INDEXED ncolors=%d'\n", (int)dest_colors);
      l_params = gimp_run_procedure ("gimp_convert_indexed",
			           &l_retvals,
			           PARAM_IMAGE,    image_id,
			           PARAM_INT32,    dest_dither,      /* value 1== floyd-steinberg */
			           PARAM_INT32,    dest_colors,
			           PARAM_END);
      break;
    case GRAY:
      if(gap_debug) fprintf(stderr, "DEBUG: p_type_convert to GRAY'\n");
      l_params = gimp_run_procedure ("gimp_convert_grayscale",
			           &l_retvals,
			           PARAM_IMAGE,    image_id,
			           PARAM_END);
      break;
    case RGB:
      if(gap_debug) fprintf(stderr, "DEBUG: p_type_convert to RGB'\n");
      l_params = gimp_run_procedure ("gimp_convert_rgb",
			           &l_retvals,
			           PARAM_IMAGE,    image_id,
			           PARAM_END);
      break;
    default:
      if(gap_debug) fprintf(stderr, "DEBUG: p_type_convert AS_IT_IS (dont convert)'\n");
      return 0;
      break;
  }


  if (l_params[0].data.d_status == FALSE) 
  {  l_rc = -1;
  }
  
  g_free(l_params);
  return (l_rc);
}	/* end p_type_convert */

/* ============================================================================
 * p_frames_convert
 *    convert frames (multiple images) into desired fileformat and type
 *    (flatten the images if desired)
 *
 *   if save_proc_name == NULL
 *   then   use xcf save (and flatten image)
 *          and new_basename and new_extension
 *
 * returns   value >= 0 if all is ok 
 *           (or -1 on error)
 * ============================================================================
 */
static int
p_frames_convert(t_anim_info *ainfo_ptr,
                 long range_from, long range_to,
                 char *save_proc_name, char *new_basename, char *new_extension,
                 int flatten,
                 GImageType dest_type, gint32 dest_colors, gint32 dest_dither)
{
  GRunModeType l_run_mode;
  gint32  l_tmp_image_id;
  long    l_cur_frame_nr;
  long    l_step, l_begin, l_end;
  gint    l_nlayers;
  gint32 *l_layers_list;
  gdouble l_percentage, l_percentage_step;
  char   *l_sav_name;
  int     l_rc;
  gint    l_overwrite_mode;
  static  t_but_arg  l_argv[3];
 
  
  l_rc = 0;
  l_overwrite_mode = 0;
  l_percentage = 0.0;
  l_run_mode  = ainfo_ptr->run_mode;
  if(ainfo_ptr->run_mode == RUN_INTERACTIVE)
  { 
    if(save_proc_name == NULL) gimp_progress_init("Flattening Frames ..");
    else                       gimp_progress_init("Converting Frames ..");
  }
 

  l_begin = range_from;
  l_end   = range_to;
  
  if(range_from > range_to)
  {
    l_step  = -1;     /* operate in descending (reverse) order */
    l_percentage_step = 1.0 / ((1.0 + range_from) - range_to);

    if(range_to < ainfo_ptr->first_frame_nr)
    { l_begin = ainfo_ptr->first_frame_nr;
    }
    if(range_from > ainfo_ptr->last_frame_nr)
    { l_end = ainfo_ptr->last_frame_nr;
    }
  }
  else
  {
    l_step  = 1;      /* operate in ascending order */
    l_percentage_step = 1.0 / ((1.0 + range_to) - range_from);

    if(range_from < ainfo_ptr->first_frame_nr)
    { l_begin = ainfo_ptr->first_frame_nr;
    }
    if(range_to > ainfo_ptr->last_frame_nr)
    { l_end = ainfo_ptr->last_frame_nr;
    }
  }
  
  
  l_cur_frame_nr = l_begin;
  while(l_rc >= 0)
  {
    /* build the frame name */
    if(ainfo_ptr->new_filename != NULL) free(ainfo_ptr->new_filename);
    ainfo_ptr->new_filename = p_alloc_fname(ainfo_ptr->basename,
                                        l_cur_frame_nr,
                                        ainfo_ptr->extension);
    if(ainfo_ptr->new_filename == NULL)
       return -1;

    /* load current frame */
    l_tmp_image_id = p_load_image(ainfo_ptr->new_filename);
    if(l_tmp_image_id < 0)
       return -1;


    l_layers_list = gimp_image_get_layers(l_tmp_image_id, &l_nlayers);
    if(l_layers_list != NULL)
    {
      g_free (l_layers_list);
    }

    if((l_nlayers > 1 ) &&  (flatten != 0))
    {
       if(gap_debug) fprintf(stderr, "DEBUG: p_frames_convert flatten tmp image'\n");
       
       /* flatten current frame image (reduce to single layer) */
       gimp_image_flatten (l_tmp_image_id);

       /* save back the current frame with same name */
       if(save_proc_name == NULL)
       {
          l_rc = p_save_named_frame(l_tmp_image_id, ainfo_ptr->new_filename);
       }
    }

    if(save_proc_name != NULL)
    {
       if(dest_type != gimp_image_base_type(l_tmp_image_id))
       {
          /* have to convert to desired type (RGB, INDEXED, GRAYSCALE) */
          p_type_convert(l_tmp_image_id, dest_type, dest_colors, dest_dither);
       }
    
    
       /* build the name for output image */
       l_sav_name = p_alloc_fname(new_basename,
                                  l_cur_frame_nr,
                                  new_extension);
       if(l_sav_name != NULL)
       {
          if(1 == p_file_exists(l_sav_name))
          {
            if (l_overwrite_mode < 1)
            {
               l_argv[0].but_txt  = "OVERWRITE frame";
               l_argv[0].but_val  = 0;
               l_argv[1].but_txt  = "OVERWRITE all";
               l_argv[1].but_val  = 1;
               l_argv[2].but_txt  = "CANCEL";
               l_argv[2].but_val  = -1;

               l_overwrite_mode =  p_buttons_dialog  ("GAP Question", l_sav_name, 3, l_argv, -1);
            }
          
          }
 
          if(l_overwrite_mode < 0)  { l_rc = -1; }
          else
          {
            /* save with selected save procedure
             * (regardless if image was flattened or not)
             */
             l_rc = p_save_named_image(l_tmp_image_id, l_sav_name, l_run_mode);
             if(l_rc < 0)
             {
               p_msg_win(ainfo_ptr->run_mode, "Convert Frames: SAVE operation FAILED\n- desired save plugin cant handle type\n- or desired save plugin not available\n");
             }
          }
       
          l_run_mode  = RUN_NONINTERACTIVE;  /* for all further calls */

          free(l_sav_name);
       }
    }

    /* destroy the tmp image */
    gimp_image_delete(l_tmp_image_id);

    if(ainfo_ptr->run_mode == RUN_INTERACTIVE)
    { 
      l_percentage += l_percentage_step;
      gimp_progress_update (l_percentage);
    }

    /* advance to next frame */
    if((l_cur_frame_nr == l_end) || (l_rc < 0))
       break;
    l_cur_frame_nr += l_step;

  }


  return l_rc;  
}	/* end p_frames_convert */



/* ============================================================================
 * p_image_sizechange
 *     scale, resize or crop one image
 * ============================================================================
 */
static
int p_image_sizechange(gint32 image_id,
               t_gap_asiz asiz_mode,
               long size_x, long size_y,
               long offs_x, long offs_y
)
{
  GParam     *l_params;
  gint        l_retvals;

  switch(asiz_mode)
  {
    case ASIZ_CROP:
      l_params = gimp_run_procedure ("gimp_crop",
			         &l_retvals,
			         PARAM_IMAGE,    image_id,
			         PARAM_INT32,    size_x,
			         PARAM_INT32,    size_y,
			         PARAM_INT32,    offs_x,
			         PARAM_INT32,    offs_y,
			         PARAM_END);
      break;
    case ASIZ_RESIZE:
      gimp_image_resize(image_id, (guint)size_x, (guint)size_y, (gint)offs_x, (gint)offs_y);
      break;
    default:
      l_params = gimp_run_procedure ("gimp_image_scale",
			         &l_retvals,
			         PARAM_IMAGE,    image_id,
			         PARAM_INT32,    size_x,
			         PARAM_INT32,    size_y,
			         PARAM_END);
      break;
  }

  return 0;
}	/* end p_image_sizechange */

/* ============================================================================
 * p_anim_sizechange
 *     scale, resize or crop all frames in the animation
 * ============================================================================
 */
static
gint32 p_anim_sizechange(t_anim_info *ainfo_ptr,
               t_gap_asiz asiz_mode,
               long size_x, long size_y,
               long offs_x, long offs_y
)
{
  guint   l_width, l_height;
  long    l_cur_frame_nr;
  long    l_step, l_begin, l_end;
  gint32  l_tmp_image_id;
  gdouble    l_percentage, l_percentage_step;  
  GParam     *l_params;
  int         l_rc;

  l_rc = 0;
  l_params = NULL;


  l_percentage = 0.0;  
  if(ainfo_ptr->run_mode == RUN_INTERACTIVE)
  {
    switch(asiz_mode)
    {
      case ASIZ_CROP:
        gimp_progress_init("Cropping all Animation Frames ..");
        break;
      case ASIZ_RESIZE:
        gimp_progress_init("Resizing all Animation Frames ..");
        break;
      default:
        gimp_progress_init("Scaling all Animation Frames ..");
        break;
    }
  }
 
  
  /* get info about the image (size and type is common to all frames) */
  l_width  = gimp_image_width(ainfo_ptr->image_id);
  l_height = gimp_image_height(ainfo_ptr->image_id);
  

  l_begin = ainfo_ptr->first_frame_nr;
  l_end   = ainfo_ptr->last_frame_nr;
  
  l_step  = 1;      /* operate in ascending order */
  l_percentage_step = 1.0 / ((1.0 + l_end) - l_begin);

  
  l_cur_frame_nr = l_begin;
  
  while(1)
  {
    /* build the frame name */
    if(ainfo_ptr->new_filename != NULL) free(ainfo_ptr->new_filename);
    ainfo_ptr->new_filename = p_alloc_fname(ainfo_ptr->basename,
                                        l_cur_frame_nr,
                                        ainfo_ptr->extension);
    if(ainfo_ptr->new_filename == NULL)
       return -1;

    /* load current frame into temporary image */
    l_tmp_image_id = p_load_image(ainfo_ptr->new_filename);
    if(l_tmp_image_id < 0)
       return -1;

    l_rc = p_image_sizechange(l_tmp_image_id, asiz_mode,
                              size_x, size_y, offs_x, offs_y);
    if(l_rc < 0) break;

    /* save back the current frame with same name */
    l_rc = p_save_named_frame(l_tmp_image_id, ainfo_ptr->new_filename);
    if(l_rc < 0) break;

    /* destroy the tmp image */
    gimp_image_delete(l_tmp_image_id);
    
    if(ainfo_ptr->run_mode == RUN_INTERACTIVE)
    { 
      l_percentage += l_percentage_step;
      gimp_progress_update (l_percentage);
    }

    /* advance to next frame */
    if(l_cur_frame_nr == l_end)
       break;
    l_cur_frame_nr += l_step;

  }   /* end while loop over all frames*/

  return l_rc;
}	/* end  p_anim_sizechange */





/* ============================================================================
 * gap_range_flatten
 * ============================================================================
 */
int    gap_range_flatten(GRunModeType run_mode, gint32 image_id,
                         long range_from, long range_to)
{
  int    l_rc;
  long   l_from, l_to;
  t_anim_info *ainfo_ptr;

  l_rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      if(run_mode == RUN_INTERACTIVE)
      {
         l_rc = p_range_dialog (ainfo_ptr, &l_from, &l_to,
                                "Flatten Frames",
                                "Select Frame Range", 2);

      }
      else
      {
         l_from = range_from;
         l_to   = range_to;
      }

      if(l_rc >= 0)
      {
         l_rc = p_save_named_frame(ainfo_ptr->image_id, ainfo_ptr->old_filename);
         if(l_rc >= 0)
         {
           l_rc = p_frames_convert(ainfo_ptr, l_from, l_to, NULL, NULL, NULL, 1, 0,0,0); 
           p_load_named_frame(ainfo_ptr->image_id, ainfo_ptr->old_filename);
         }
      }
    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(l_rc);    
}	/* end gap_range_flatten */




/* ============================================================================
 * p_frames_layer_del
 * returns   image_id of the new created multilayer image
 *           (or -1 on error)
 * ============================================================================
 */
static int
p_frames_layer_del(t_anim_info *ainfo_ptr,
                   long range_from, long range_to, long position)
{
  gint32  l_tmp_image_id;

  long    l_cur_frame_nr;
  long    l_step, l_begin, l_end;
  gint32  l_tmp_layer_id;
  gint       l_nlayers;
  gint32    *l_layers_list;
  gdouble    l_percentage, l_percentage_step;  
  static  char l_buff[50];
  int          l_rc;
  

  l_rc = 0;
  l_percentage = 0.0;  
  if(ainfo_ptr->run_mode == RUN_INTERACTIVE)
  {
    sprintf(l_buff, "Removing Layer (pos:%ld) from Frames ..", position); 
    gimp_progress_init(l_buff);
  }
 

  /* get info about the image (size and type is common to all frames) */

  l_begin = range_from;
  l_end   = range_to;
  
  if(range_from > range_to)
  {
    l_step  = -1;     /* operate in descending (reverse) order */
    l_percentage_step = 1.0 / ((1.0 + range_from) - range_to);

    if(range_to < ainfo_ptr->first_frame_nr)
    { l_begin = ainfo_ptr->first_frame_nr;
    }
    if(range_from > ainfo_ptr->last_frame_nr)
    { l_end = ainfo_ptr->last_frame_nr;
    }
  }
  else
  {
    l_step  = 1;      /* operate in ascending order */
    l_percentage_step = 1.0 / ((1.0 + range_to) - range_from);

    if(range_from < ainfo_ptr->first_frame_nr)
    { l_begin = ainfo_ptr->first_frame_nr;
    }
    if(range_to > ainfo_ptr->last_frame_nr)
    { l_end = ainfo_ptr->last_frame_nr;
    }
  }
  
 
  l_cur_frame_nr = l_begin;
  while(1)
  {
    /* build the frame name */
    if(ainfo_ptr->new_filename != NULL) free(ainfo_ptr->new_filename);
    ainfo_ptr->new_filename = p_alloc_fname(ainfo_ptr->basename,
                                        l_cur_frame_nr,
                                        ainfo_ptr->extension);
    if(ainfo_ptr->new_filename == NULL)
       return -1;

    /* load current frame */
    l_tmp_image_id = p_load_image(ainfo_ptr->new_filename);
    if(l_tmp_image_id < 0)
       return -1;

    /* remove layer[position] */
    l_layers_list = gimp_image_get_layers(l_tmp_image_id, &l_nlayers);
    if(l_layers_list != NULL)
    {
      /* findout layer id of the requestetd position within layerstack */
      if(position < l_nlayers) l_tmp_layer_id = l_layers_list[position];
      else                     l_tmp_layer_id = l_layers_list[l_nlayers -1];

      g_free (l_layers_list);
   
      /* check for last layer (MUST NOT be deleted !) */ 
      if(l_nlayers > 1)
      {
        /* remove and delete requested layer */
        gimp_image_remove_layer(l_tmp_image_id, l_tmp_layer_id);

        /*  gimp_layer_delete(l_tmp_layer_id); */ /* gives sigsegv ERROR */
        /* A: gimp_image_remove_layer does automatic delete 
         *    if layer is not attched to any image
         * B: gimp_layer_delete has a BUG
         */


        /* save current frame */
        l_rc = p_save_named_frame(l_tmp_image_id, ainfo_ptr->new_filename);
      }
    }

    /* destroy the tmp image */
    gimp_image_delete(l_tmp_image_id);

    if(ainfo_ptr->run_mode == RUN_INTERACTIVE)
    { 
      l_percentage += l_percentage_step;
      gimp_progress_update (l_percentage);
    }

    /* advance to next frame */
    if((l_cur_frame_nr == l_end) || (l_rc < 0))
       break;
    l_cur_frame_nr += l_step;

  }


  return l_rc;  
}	/* end p_frames_layer_del */


/* ============================================================================
 * gap_range_layer_del
 * ============================================================================
 */
int gap_range_layer_del(GRunModeType run_mode, gint32 image_id,
                         long range_from, long range_to, long position)
{
  int    l_rc;
  long   l_position;
  long   l_from, l_to;
  t_anim_info *ainfo_ptr;

  l_rc = -1;
  l_position = 0;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      if(run_mode == RUN_INTERACTIVE)
      {
         l_rc = p_range_dialog (ainfo_ptr, &l_from, &l_to,
                                "Delete Layers in Frames",
                                "Select Frame Range & Position", 3);
         l_position = l_rc;

      }
      else
      {
         l_from = range_from;
         l_to   = range_to;
         l_position = position;
      }

      if(l_rc >= 0)
      {
         l_rc = p_save_named_frame(ainfo_ptr->image_id, ainfo_ptr->old_filename);
         if(l_rc >= 0)
         {
           l_rc = p_frames_layer_del(ainfo_ptr, l_from, l_to, l_position);
           p_load_named_frame(ainfo_ptr->image_id, ainfo_ptr->old_filename);
         }
      }
    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(l_rc);    
}	/* end gap_range_layer_del */


/* ============================================================================
 * gap_range_conv
 *   convert frame range to any gimp supported fileformat
 * ============================================================================
 */
gint32 gap_range_conv(GRunModeType run_mode, gint32 image_id,
                      long range_from, long range_to,
                      long       flatten,
                      GImageType dest_type,
                      gint32     dest_colors,
                      gint32     dest_dither,
                      char      *basename,
                      char      *extension)
{
  gint32  l_rc;
  long   l_from, l_to;
  long   l_flatten;
  gint32     l_dest_colors;
  gint32     l_dest_dither;
  GImageType l_dest_type;
  
  t_anim_info *ainfo_ptr;
  char  l_save_proc_name[128];
  char  l_basename[256];
  char *l_basename_ptr;
  long  l_number;
  char  l_extension[32];

  strcpy(l_save_proc_name, "gimp_file_save");
  strcpy(l_extension, ".tif");

  l_rc = -1;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      strncpy(l_basename, ainfo_ptr->basename, sizeof(l_basename) -1);
      l_basename[sizeof(l_basename) -1] = '\0';

      if(run_mode == RUN_INTERACTIVE)
      {
         
         l_flatten = 1;
         /* p_convert_dialog : select destination type
          * to find out extension
          */
         l_rc = p_convert_dialog (ainfo_ptr, &l_from, &l_to, &l_flatten,
                                  &l_dest_type, &l_dest_colors, &l_dest_dither,
                                  &l_basename[0], sizeof(l_basename),
                                  &l_extension[0], sizeof(l_extension));

      }
      else
      {
         l_from    = range_from;
         l_to      = range_to;
         l_flatten = flatten;
         l_dest_type   = dest_type;
         l_dest_colors = dest_colors;
         l_dest_dither = dest_dither;
         if(basename != NULL)
         {
            strncpy(l_basename, basename, sizeof(l_basename) -1);
            l_basename[sizeof(l_basename) -1] = '\0';
         }
         strncpy(l_extension, extension, sizeof(l_extension) -1);
         l_extension[sizeof(l_extension) -1] = '\0';
         
      }

      if(l_rc >= 0)
      {
         /* cut off extension and trailing frame number "_0001" */
         l_basename_ptr = p_alloc_basename(&l_basename[0], &l_number);
         if(l_basename_ptr == NULL)  { l_rc = -1; }
         else
         {
            l_rc = p_frames_convert(ainfo_ptr, l_from, l_to,
                                    l_save_proc_name,
                                    l_basename_ptr,
                                    l_extension,
                                    l_flatten,
                                    l_dest_type,
                                    l_dest_colors,
                                    l_dest_dither);
            free(l_basename_ptr);
         }
      }
    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(l_rc);    
}	/* end gap_range_conv */



/* ============================================================================
 * gap_anim_sizechange
 *    scale, resize or crop all anim_frame images of the animation
 *    (depending on asiz_mode)
 * ============================================================================
 */
int gap_anim_sizechange(GRunModeType run_mode, t_gap_asiz asiz_mode, gint32 image_id,
                  long size_x, long size_y, long offs_x, long offs_y)
{
  int    l_rc;
  long   l_size_x, l_size_y;
  long   l_offs_x, l_offs_y;
  t_anim_info *ainfo_ptr;
  
  l_rc = 0;
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {
    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      if(run_mode == RUN_INTERACTIVE)
      {
         l_rc = p_anim_sizechange_dialog (ainfo_ptr, asiz_mode,
                                          &l_size_x, &l_size_y,
                                          &l_offs_x, &l_offs_y);
      }
      else
      {
         l_size_x = size_x;
         l_size_y = size_y;
         l_offs_x = offs_x;
         l_offs_y = offs_y;
      }

      if(l_rc >= 0)
      {
         l_rc = p_save_named_frame(ainfo_ptr->image_id, ainfo_ptr->old_filename);
         if(l_rc >= 0)
         {
           /* we have to resize the current anim frame image in gimp's ram
            *(from where we were invoked)
            * Note: All anim frames on disc and the current one in ram
            *       must fit in size and type, to allow further animation operations.
            *       (Restriction of duplicate_into)
            */
           l_rc = p_image_sizechange(ainfo_ptr->image_id, asiz_mode,
                                     l_size_x, l_size_y, l_offs_x, l_offs_y);

           if(l_rc == 0)
           {
              /* sizechange for all anim frames on disk */
              l_rc = p_anim_sizechange(ainfo_ptr, asiz_mode,
                                       l_size_x, l_size_y,
                                       l_offs_x, l_offs_y );
           }
           /* p_load_named_frame(ainfo_ptr->image_id, ainfo_ptr->old_filename); */
           /* dont need to reload, because the same sizechange operation was
            * applied both to ram-image and dikfile
            *
            * But we must clear all undo steps. 
            * (If the user could undo the sizechange on the current image,
            *  it would not fit to the other frames on disk.)
            */
           gimp_image_enable_undo(ainfo_ptr->image_id); /* clear undo stack */
         }
      }
    }
    p_free_ainfo(&ainfo_ptr);
  }
  
  return(l_rc);    
}	/* end gap_anim_sizechange */
