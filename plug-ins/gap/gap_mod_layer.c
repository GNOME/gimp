/* gap_mod_layer.c
 * 1998.10.14 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains:
 * modify Layer (perform actions (like raise, set visible, apply filter)
 *               - foreach selected layer
 *               - in each frame of the selected framerange)
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
 * gimp   1.1.6;     1999/06/21  hof: bugix: wrong iterator total_steps and direction
 * gimp   1.1.15.1;  1999/05/08  hof: bugix (dont mix GDrawableType with GImageType)
 * version 0.98.00   1998.11.27  hof: - use new module gap_pdb_calls.h
 * version 0.97.00   1998.10.19  hof: - created module
 */
 
/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap_arr_dialog.h"
#include "gap_filter.h"
#include "gap_filter_pdb.h"
#include "gap_pdb_calls.h"
#include "gap_match.h"
#include "gap_lib.h"
#include "gap_range_ops.h"
#include "gap_mod_layer.h"


extern      int gap_debug; /* ==0  ... dont print debug infos */


/* ============================================================================
 * p_layer_modify_dialog
 *   retcode    
 *               0   ... Generate Paramfile
 *               1   ... Generate Paramfile and start mpeg_encode
 * ============================================================================
 */
static
int p_layer_modify_dialog(t_anim_info *ainfo_ptr,
                   gint32 *range_from,  gint32 *range_to,
                   gint32 *action_mode, gint32 *sel_mode,
                   gint32 *sel_case,    gint32 *sel_invert,
                   char *sel_pattern,   char   *new_layername)
{
  static t_arr_arg  argv[9];
  static t_but_arg  b_argv[2];
  gint   l_rc;

  static char   l_buf[MAX_LAYERNAME];

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

  /* action items what to do with the selected layer(s) */
  static char *action_args[13]  = {"set Layer(s) visible",
                                  "set Layer(s) invisible",
                                  "set Layer(s) linked",
                                  "set Layer(s) unlinked",
                                  "raise Layer(s)",
                                  "lower Layer(s)",
                                  "merge Layer(s) expand as necessary",
                                  "merge Layer(s) clipped to image",
                                  "merge Layer(s) clipped to bg-layer",
                                  "apply filter on Layer(s)",
                                  "duplicate Layer(s)",
                                  "delete Layer(s)",
                                  "rename Layer(s)"
                                  };
                                  

/*                                                                   
  static char *action_help[13]  = {"set all selected layers visible",
                                  "set all selected layers  invisible",
                                  "set all selected layers  linked",
                                  "set all selected layers  unlinked",
                                  "raise all selected layers",
                                  "lower all selected layers",
                                  "merge expand as necessary",
                                  "merge clipped to image",
                                  "merge clipped to bg-layer",
                                  "APPLY FILTER to all selected layers",
                                  "duplicate all selected layers",
                                  "delete REMOVES all selected layers",
                                  "rename all selected layers\nto NewLayername."
                                  };
*/

  l_rc = -1;
  
  sprintf(l_buf, "Perform function on one or more Layer(s)\nin all frames of the selected framerange\n");
  
  /* the 3 Action Buttons */
    b_argv[0].but_txt  = "OK";
    b_argv[0].but_val  = 0;
    b_argv[1].but_txt  = "Cancel";
    b_argv[1].but_val  = -1;
  
  p_init_arr_arg(&argv[0], WGT_LABEL);
  argv[0].label_txt = &l_buf[0];

  p_init_arr_arg(&argv[1], WGT_INT_PAIR);
  argv[1].constraint = TRUE;
  argv[1].label_txt = "From Frame:";
  argv[1].help_txt  = "first handled frame";
  argv[1].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[1].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[1].int_ret   = (gint)ainfo_ptr->curr_frame_nr;

  p_init_arr_arg(&argv[2], WGT_INT_PAIR);
  argv[2].constraint = TRUE;
  argv[2].label_txt = "To   Frame:";
  argv[2].help_txt  = "last handled frame";
  argv[2].int_min   = (gint)ainfo_ptr->first_frame_nr;
  argv[2].int_max   = (gint)ainfo_ptr->last_frame_nr;
  argv[2].int_ret   = (gint)ainfo_ptr->last_frame_nr;


  /* Layer select mode RADIO buttons */
  p_init_arr_arg(&argv[3], WGT_RADIO);
  argv[3].label_txt = "Select Layer(s):";
  argv[3].radio_argc = 7;
  argv[3].radio_argv = sel_args;
  argv[3].radio_help_argv = sel_help;
  argv[3].radio_ret  = 4;

  /* Layer select pattern string */
  sprintf (sel_pattern, "0");
  p_init_arr_arg(&argv[4], WGT_TEXT);
  argv[4].label_txt ="Select Pattern:";
  argv[4].entry_width = 140;       /* pixel */
  argv[4].help_txt  ="String to identify layer names    \nor layerstack position numbers\n0,3-5";
  argv[4].text_buf_len = MAX_LAYERNAME;
  argv[4].text_buf_ret = sel_pattern;

  /* case sensitive checkbutton */
  p_init_arr_arg(&argv[5], WGT_TOGGLE);
  argv[5].label_txt = "Case sensitive";
  argv[5].help_txt  = "Lowercase and UPPERCASE letters are considered as different";
  argv[5].int_ret   = 1;

  /* invert selection checkbutton */
  p_init_arr_arg(&argv[6], WGT_TOGGLE);
  argv[6].label_txt = "Invert Selection";
  argv[6].help_txt  = "Perform actions on all unselected Layers";
  argv[6].int_ret   = 0;

  /* desired action to perform OPTIONMENU  */
  p_init_arr_arg(&argv[7], WGT_OPTIONMENU);
  argv[7].label_txt = "Function :";
  argv[7].radio_argc = 13;
  argv[7].radio_argv = action_args;
  /* argv[7].radio_help_argv = action_help */
  argv[7].help_txt = "Function to be performed on all selected layers";
  argv[7].radio_ret  = 0;

  /* a new name for the handled Layer(s) */
  *new_layername = '\0';
  p_init_arr_arg(&argv[8], WGT_TEXT);
  argv[8].label_txt ="New Layername:";
  argv[8].entry_width = 140;       /* pixel */
  argv[8].help_txt  ="New Layername for all handled layers \n[####] is replaced by frame number\n(is used on function rename only)";
  argv[8].text_buf_len = MAX_LAYERNAME;
  argv[8].text_buf_ret = new_layername;


  l_rc =  p_array_std_dialog("Frames Modify",
                                "Settings",
                                 9,   argv,      /* widget array */
                                 2,   b_argv,    /* button array */
                                 0);

  /* return the entered values */
  *range_from           = argv[1].int_ret;
  *range_to             = argv[2].int_ret;
  *sel_mode             = argv[3].int_ret;
                       /*     [4] sel_pattern  */
  *sel_case             = argv[5].int_ret;
  *sel_invert           = argv[6].int_ret;
  *action_mode          = argv[7].int_ret;
                       /*     [8] l_new_layername */
       
  
  return (l_rc);   
}	/* end p_layer_modify_dialog */


/* ============================================================================
 * p_pitstop_dialog
 *   return -1  on CANCEL
 *           0  on Continue (OK)
 * ============================================================================
 */
static gint
p_pitstop_dialog(gint text_flag, char *filter_procname)
{
  char *l_env;
  char  l_msg[512];
  static t_but_arg  l_but_argv[2];
  gint              l_but_argc;
  gint              l_argc;
  static t_arr_arg  l_argv[1];
  int               l_continue;
  
    


  l_but_argv[0].but_txt  = "Continue";
  l_but_argv[0].but_val  = 0;
  l_but_argv[1].but_txt  = "Cancel";
  l_but_argv[1].but_val  = -1;

  l_but_argc = 2;
  l_argc = 0;
 
  /* optional dialog between both calls (to see the effect of 1.call) */
  l_env = g_getenv("GAP_FILTER_PITSTOP");
  if(l_env != NULL)
  {
     if((*l_env == 'N') || (*l_env == 'n'))
     {
       return 0;  /* continue without question */
     }
  }
  if(text_flag == 0)
  {
     sprintf(l_msg, "2.nd call of %s\n(define end-settings)", filter_procname);
  }
  else
  {
     sprintf(l_msg, "Non-Interactive call of %s\n(for all selected layers)", filter_procname);
  }
  l_continue = p_array_std_dialog ("Animated Filter apply", l_msg,
  				   l_argc,     l_argv, 
  				   l_but_argc, l_but_argv, 0);

  return (l_continue);
    
}	/* end p_pitstop_dialog */


/* ============================================================================
 * p_get_1st_selected
 *   return index of the 1.st selected layer
 *   or -1 if no selection was found
 * ============================================================================
 */
int
p_get_1st_selected (t_LayliElem * layli_ptr, gint nlayers)
{
  int  l_idx;

  for(l_idx = 0; l_idx < nlayers; l_idx++)
  {
    if(layli_ptr[l_idx].selected != FALSE)
    {
      return (l_idx);
    }
  }
  return(-1);
}	/* end p_get_1st_selected */


/* ============================================================================
 * p_alloc_layli
 * returns   pointer to a new allocated image_id of the new created multilayer image
 *           (or NULL on error)
 * ============================================================================
 */

t_LayliElem *
p_alloc_layli(gint32 image_id, gint32 *l_sel_cnt, gint *nlayers,
              gint32 sel_mode,
              gint32 sel_case,
	      gint32 sel_invert,
              char *sel_pattern )
{
  gint32 *l_layers_list;
  gint32  l_layer_id;
  gint32  l_idx;
  t_LayliElem *l_layli_ptr;
  char      *l_layername;

  *l_sel_cnt = 0;

  l_layers_list = gimp_image_get_layers(image_id, nlayers);
  if(l_layers_list == NULL)
  {
    return(NULL);
  }
  
  l_layli_ptr = g_new0(t_LayliElem, (*nlayers));
  if(l_layli_ptr == NULL)
  {
     g_free (l_layers_list);
     return(NULL); 
  }

  for(l_idx = 0; l_idx < (*nlayers); l_idx++)
  {
    l_layer_id = l_layers_list[l_idx];
    l_layername = gimp_layer_get_name(l_layer_id); 
    l_layli_ptr[l_idx].layer_id  = l_layer_id;
    l_layli_ptr[l_idx].visible   = gimp_layer_get_visible(l_layer_id);
    l_layli_ptr[l_idx].selected  = p_match_layer(l_idx,
                                                 l_layername,
                                                 sel_pattern,
						 sel_mode,
						 sel_case,
						 sel_invert,
						 *nlayers, l_layer_id);
    if(l_layli_ptr[l_idx].selected != FALSE)
    {
      (*l_sel_cnt)++;  /* count all selected layers */
    }
    if(gap_debug) printf("gap: p_alloc_layli [%d] id:%d, sel:%d %s\n",
                         (int)l_idx, (int)l_layer_id,
			 (int)l_layli_ptr[l_idx].selected, l_layername);
    g_free (l_layername);
  }

  g_free (l_layers_list);

  return( l_layli_ptr );
}		/* end p_alloc_layli */

/* ============================================================================
 * p_prevent_empty_image
 *    check if the resulting image has at least one layer
 *    (gimp 1.0.0 tends to crash on layerless images)
 * ============================================================================
 */

void p_prevent_empty_image(gint32 image_id)
{
  GImageType l_type;
  guint   l_width, l_height;
  gint32  l_layer_id;
  gint    l_nlayers;
  gint32 *l_layers_list;

  l_layers_list = gimp_image_get_layers(image_id, &l_nlayers);
  if(l_layers_list != NULL)
  {
     g_free (l_layers_list);
  }
  else l_nlayers = 0;
  
  if(l_nlayers == 0)
  {
     /* the resulting image has no layer, add a transparent dummy layer */

     /* get info about the image */
     l_width  = gimp_image_width(image_id);
     l_height = gimp_image_height(image_id);
     l_type   = gimp_image_base_type(image_id);

     l_type   = (l_type * 2); /* convert from GImageType to GDrawableType */

     /* add a transparent dummy layer */
     l_layer_id = gimp_layer_new(image_id, "dummy",
                                    l_width, l_height,  l_type,
                                    0.0,       /* Opacity full transparent */     
                                    0);        /* NORMAL */
     gimp_image_add_layer(image_id, l_layer_id, 0);
  }

}	/* end p_prevent_empty_image */


/* ============================================================================
 * p_raise_layer
 *   raise layer (check if possible before)
 *   (without the check each failed attempt would open an inf window)
 * ============================================================================
 */
static void
p_raise_layer (gint32 image_id, gint32 layer_id, t_LayliElem * layli_ptr, gint nlayers)
{
  if(! gimp_drawable_has_alpha (layer_id)) return; /* has no alpha channel */

  if(layli_ptr[0].layer_id == layer_id)  return;   /* is already on top */

  gimp_image_raise_layer(image_id, layer_id);
}	/* end p_raise_layer */

static void
p_lower_layer (gint32 image_id, gint32 layer_id, t_LayliElem * layli_ptr, gint nlayers)
{
  if(! gimp_drawable_has_alpha (layer_id)) return; /* has no alpha channel */

  if(layli_ptr[nlayers-1].layer_id == layer_id)  return;   /* is already on bottom */

  if(nlayers > 1)
  {
    if((layli_ptr[nlayers-2].layer_id == layer_id)
    && (! gimp_drawable_has_alpha (layli_ptr[nlayers-1].layer_id)))
    {
      /* the layer is one step above a "bottom-layer without alpha" */
      return;
    }
  }

  gimp_image_lower_layer(image_id, layer_id);
}	/* end p_lower_layer */

/* ============================================================================
 * p_apply_action
 *
 *    perform function (defined by action_mode)
 *    on all selcted layer(s)
 *   
 * returns   0 if all done OK
 *           (or -1 on error)
 * ============================================================================
 */
 
static int 
p_apply_action(gint32 image_id,
	      gint32 action_mode,
	      t_LayliElem *layli_ptr,
	      gint nlayers,
	      gint32 sel_cnt,
	      
	      long  from,
	      long  to,
	      long  curr,
	      char *new_layername,
	      char *filter_procname
	      )
{
  int   l_idx;
  int   l_rc;
  gint32  l_layer_id;
  gint32  l_new_layer_id;
  gint    l_merge_mode;
  gint    l_vis_result;
  char    l_name_buff[MAX_LAYERNAME];
  
  if(gap_debug) fprintf(stderr, "gap: p_apply_action START\n");      

  l_rc = 0; 
  l_merge_mode = -44; /* none of the flatten modes */

  if(action_mode == ACM_MERGE_EXPAND) l_merge_mode = FLAM_MERG_EXPAND;
  if(action_mode == ACM_MERGE_IMG)    l_merge_mode = FLAM_MERG_CLIP_IMG;
  if(action_mode == ACM_MERGE_BG)     l_merge_mode = FLAM_MERG_CLIP_BG;

  /* merge actions require one call per image */  
  if(l_merge_mode != (-44))
  {
      if(sel_cnt < 2)
      {
        return(0);  /* OK, nothing to merge */
      }
     
     l_vis_result = FALSE;
     
     /* set selected layers visible, all others invisible for merge */
     for(l_idx = 0; l_idx < nlayers; l_idx++)
     {
       if(layli_ptr[l_idx].selected == FALSE)
       {
          gimp_layer_set_visible(layli_ptr[l_idx].layer_id, FALSE);
       }
       else
       {
          if(gimp_layer_get_visible(layli_ptr[l_idx].layer_id))
	  {
	    /* result will we visible if at least one of the
	     * selected layers was visible before
	     */
	    l_vis_result = TRUE;
	  }
          gimp_layer_set_visible(layli_ptr[l_idx].layer_id, TRUE);
       }
     }

     /* merge all visible layers (i.e. all selected layers) */
     l_layer_id = gimp_image_merge_visible_layers (image_id, l_merge_mode);
     if(l_vis_result == FALSE)
     {
        gimp_layer_set_visible(l_layer_id, FALSE);
     }
     
     /* if new_layername is available use that name
      * for the new merged layer
      */
     if (!p_is_empty (new_layername))
     {
	 p_substitute_framenr(&l_name_buff[0], sizeof(l_name_buff),
	                      new_layername, curr);
	 gimp_layer_set_name(l_layer_id, &l_name_buff[0]);
     }

     /* restore visibility flags after merge */
     for(l_idx = 0; l_idx < nlayers; l_idx++)
     {
       if(layli_ptr[l_idx].selected == FALSE)
       {
         gimp_layer_set_visible(layli_ptr[l_idx].layer_id, 
	                        layli_ptr[l_idx].visible);
       }
     }
     
     return(0);
  }

  /* -----------------------------*/  
  /* non-merge actions require calls foreach selected layer */  
  for(l_idx = 0; (l_idx < nlayers) && (l_rc == 0); l_idx++)
  {
    l_layer_id = layli_ptr[l_idx].layer_id;

    /* apply function defined by action_mode */
    if(layli_ptr[l_idx].selected != FALSE)
    {
      if(gap_debug) fprintf(stderr, "gap: p_apply_action on selected LayerID:%d layerstack:%d\n",
                           (int)l_layer_id, (int)l_idx);      
      switch(action_mode)
      {
        case ACM_SET_VISIBLE:
          gimp_layer_set_visible(l_layer_id, TRUE);
	  break;
        case ACM_SET_INVISIBLE:
          gimp_layer_set_visible(l_layer_id, FALSE);
	  break;
        case ACM_SET_LINKED:
          l_rc = p_layer_set_linked (l_layer_id, TRUE);
	  break;
        case ACM_SET_UNLINKED:
          l_rc = p_layer_set_linked (l_layer_id, FALSE);
	  break;
        case ACM_RAISE:
	  p_raise_layer(image_id, l_layer_id, layli_ptr, nlayers);
	  break;
        case ACM_LOWER:
	  p_lower_layer(image_id, l_layer_id, layli_ptr, nlayers);
	  break;
        case ACM_APPLY_FILTER:
	  l_rc = p_call_plugin(filter_procname,
	                       image_id,
			       l_layer_id,
			       RUN_WITH_LAST_VALS);
          if(gap_debug) fprintf(stderr, "gap: p_apply_action FILTER:%s rc =%d\n",
                                filter_procname, (int)l_rc);      
	  break;
        case ACM_DUPLICATE:
	  l_new_layer_id = gimp_layer_copy(l_layer_id);
	  gimp_image_add_layer (image_id, l_new_layer_id, -1); 
	  if (!p_is_empty (new_layername))
	  {
	      p_substitute_framenr(&l_name_buff[0], sizeof(l_name_buff),
	                           new_layername, curr);
	      gimp_layer_set_name(l_new_layer_id, &l_name_buff[0]);
	  }
	  break;
        case ACM_DELETE:
	  gimp_image_remove_layer(image_id, l_layer_id);
	  break;
        case ACM_RENAME:
	  p_substitute_framenr(&l_name_buff[0], sizeof(l_name_buff),
	                        new_layername, curr);
	  gimp_layer_set_name(l_layer_id, &l_name_buff[0]);
	  break;
        default:
	  break;
      }
    }
  }

  return (l_rc); 
}	/* end p_apply_action */


/* ============================================================================
 * p_do_filter_dialogs
 *    additional dialog steps
 *    a) gap_pdb_browser (select the filter)
 *    b) 1st interactive filtercall
 *    c) 1st pitstop dialog
 * ============================================================================
 */
static int
p_do_filter_dialogs(t_anim_info *ainfo_ptr, 
                    gint32 image_id, gint32 *dpy_id, 
                    t_LayliElem * layli_ptr, gint nlayers ,
                    char *filter_procname, int filt_len,
		    gint *plugin_data_len,
		    t_apply_mode *apply_mode
		    )
{
  t_gap_db_browse_result  l_browser_result;
  gint32   l_layer_id;
  int      l_rc;
  int      l_idx;
  static char l_key_from[512];
  static gint l_gtk_init = TRUE;   /* gkt_init at 1.st call */

  if(ainfo_ptr->run_mode == RUN_INTERACTIVE)
  { 
    l_gtk_init = FALSE;  /* gtk_init was done in 1.st modify dialog before */
  }


  /* GAP-PDB-Browser Dialog */
  /* ---------------------- */
  if(gap_db_browser_dialog("Select Filter for Animated frames-apply",
                                 "Apply Constant",
                                 "Apply Varying",
                                 p_constraint_proc,
                                 p_constraint_proc_sel1,
                                 p_constraint_proc_sel2,
                                 &l_browser_result,
				 l_gtk_init   /* do not call gtk_init */
			  ) 
    < 0)
  {
      if(gap_debug) fprintf(stderr, "DEBUG: gap_db_browser_dialog cancelled\n");
      return -1;
  }
    
  p_arr_gtk_init(FALSE); /* disable the initial gtk_init in gap_arr_dialog's
                          * (gtk_init was done before)
                          */

  strncpy(filter_procname, l_browser_result.selected_proc_name, filt_len-1);
  filter_procname[filt_len-1] = '\0';
  if(l_browser_result.button_nr == 1) *apply_mode = PTYP_VARYING_LINEAR;
  else                                *apply_mode = PAPP_CONSTANT;

  /* 1.st INTERACTIV Filtercall dialog */
  /* --------------------------------- */
  /* check for the Plugin */

  l_rc = p_procedure_available(filter_procname, PTYP_CAN_OPERATE_ON_DRAWABLE);
  if(l_rc < 0)
  {
     fprintf(stderr, "ERROR: Plugin not available or wrong type %s\n", filter_procname);
     return -1;
  }

  /* get 1.st selected layer (of 1.st handled frame in range ) */
  l_idx = p_get_1st_selected(layli_ptr, nlayers);
  if(l_idx < 0)
  {
     fprintf(stderr, "ERROR: No layer selected in 1.st handled frame\n");
     return (-1);
  }
  l_layer_id = layli_ptr[l_idx].layer_id;
  
  /* open a view for the 1.st handled frame */
  *dpy_id = gimp_display_new (image_id);

  l_rc = p_call_plugin(filter_procname, image_id, l_layer_id, RUN_INTERACTIVE);

  /* OOPS: cant delete the display here, because
   *       closing the last display seems to free up
   *       at least parts of the image,
   *       and causes crashes if the image_id is used
   *       in further gimp procedures
   */
  /* gimp_display_delete(*dpy_id); */

  /* get values, then store with suffix "_ITER_FROM" */
  *plugin_data_len = p_get_data(filter_procname);
  if(*plugin_data_len > 0)
  {
     sprintf(l_key_from, "%s_ITER_FROM", filter_procname);
     p_set_data(l_key_from, *plugin_data_len);
  }
  else
  {
    return (-1);
  }

  if(*apply_mode != PTYP_VARYING_LINEAR)
  {
    return (p_pitstop_dialog(1, filter_procname));
  }

  return(0);
}	/* end p_do_filter_dialogs */


/* ============================================================================
 * p_do_2nd_filter_dialogs
 *    d) [ 2nd interactive filtercall
 *    e)   2nd pitstop dialog ]
 *
 *   (temporary) open the last frame of the range
 *   get its 1.st selected laye
 *   and do the Interctive Filtercall (to get the end-values)
 *
 * then close everything (without save).
 * (the last frame will be processed later, with all its selected layers)
 * ============================================================================
 */
static gint
p_do_2nd_filter_dialogs(char *filter_procname,
                        t_apply_mode  l_apply_mode,
			char *last_frame_filename,
			gint32 sel_mode, gint32 sel_case,
			gint32 sel_invert, char *sel_pattern
                       )
{
  gint32   l_layer_id;
  gint32   l_dpy_id;
  int      l_rc;
  int      l_idx;
  static char l_key_to[512];
  static char l_key_from[512];
  gint32  l_last_image_id;
  t_LayliElem *l_layli_ptr;
  gint       l_nlayers;
  gint32     l_sel_cnt;
  gint       l_plugin_data_len;

  l_layli_ptr = NULL;
  l_rc = -1;       /* assume cancel or error */
  l_last_image_id = -1;
  l_dpy_id = -1;
  
  /* 2.nd INTERACTIV Filtercall dialog */
  /* --------------------------------- */
  if(last_frame_filename == NULL)
  {
    return (-1);  /* there is no 2.nd frame for 2.nd filter call */
  }

  if(p_pitstop_dialog(0, filter_procname) < 0)
     goto cleanup;

  /* load last frame into temporary image */
  l_last_image_id = p_load_image(last_frame_filename);
  if (l_last_image_id < 0)
     goto cleanup;

  /* get informations (id, visible, selected) about all layers */
  l_layli_ptr = p_alloc_layli(l_last_image_id, &l_sel_cnt, &l_nlayers,
                               sel_mode, sel_case, sel_invert, sel_pattern);

  if (l_layli_ptr == NULL)
     goto cleanup;

  /* get 1.st selected layer (of last handled frame in range ) */
  l_idx = p_get_1st_selected(l_layli_ptr, l_nlayers);
  if(l_idx < 0)
  {
     p_msg_win (RUN_INTERACTIVE, "GAP Modify: No layer selected in last handled frame\n");
     goto cleanup;
  }
  l_layer_id = l_layli_ptr[l_idx].layer_id;

  /* open a view for the last handled frame */
  l_dpy_id = gimp_display_new (l_last_image_id);

  /* 2.nd INTERACTIV Filtercall dialog */
  /* --------------------------------- */
  l_rc = p_call_plugin(filter_procname, l_last_image_id, l_layer_id, RUN_INTERACTIVE);

  /* get values, then store with suffix "_ITER_TO" */
  l_plugin_data_len = p_get_data(filter_procname);
  if(l_plugin_data_len <= 0)
     goto cleanup;

   sprintf(l_key_to, "%s_ITER_TO", filter_procname);
   p_set_data(l_key_to, l_plugin_data_len);

   /* get FROM values */
   sprintf(l_key_from, "%s_ITER_FROM", filter_procname);
   l_plugin_data_len = p_get_data(l_key_from);
   p_set_data(filter_procname, l_plugin_data_len);

  l_rc = p_pitstop_dialog(1, filter_procname);
  
cleanup:
  if(l_dpy_id >= 0)        gimp_display_delete(l_dpy_id);
  if(l_last_image_id >= 0) gimp_image_delete(l_last_image_id);
  if(l_layli_ptr != NULL)  g_free(l_layli_ptr);

  return (l_rc);

    
}	/* end p_do_2nd_filter_dialogs */


/* ============================================================================
 * p_frames_modify
 *
 *   foreach frame of the range (given by range_from and range_to)
 *   perform function defined by action_mode
 *   on all selected layer(s) described by sel_mode, sel_case
 *                                         sel_invert and sel_pattern
 * returns   0 if all done OK
 *           (or -1 on error or cancel)
 * ============================================================================
 */
static gint32
p_frames_modify(t_anim_info *ainfo_ptr,
                   long range_from, long range_to,
                   gint32 action_mode, gint32 sel_mode,
                   gint32 sel_case, gint32 sel_invert,
                   char *sel_pattern, char *new_layername)
{
  long    l_cur_frame_nr;
  long    l_step, l_begin, l_end;
  gint32  l_tmp_image_id;
  gint32  l_dpy_id;
  gint       l_nlayers;
  gdouble    l_percentage, l_percentage_step;  
  int        l_rc;
  int        l_idx;
  gint32     l_sel_cnt;
  t_LayliElem *l_layli_ptr;

  GParam     *l_params;
  gint        l_retvals;
  gint        l_plugin_data_len;
  char       l_filter_procname[256];
  char      *l_plugin_iterator;
  gdouble    l_cur_step;
  gint       l_total_steps;
  t_apply_mode  l_apply_mode;
  char         *l_last_frame_filename;

  


  if(gap_debug) fprintf(stderr, "gap: p_frames_modify START, action_mode=%d  sel_mode=%d case=%d, invert=%d patt:%s:\n",
        (int)action_mode, (int)sel_mode, (int)sel_case, (int)sel_invert, sel_pattern);

  l_percentage = 0.0;
  if(ainfo_ptr->run_mode == RUN_INTERACTIVE)
  { 
    gimp_progress_init("Modifying Frames/Layer(s) ..");
  }
 

  l_begin = range_from;
  l_end   = range_to;
  l_tmp_image_id = -1;
  l_layli_ptr = NULL;
  l_rc = 0;
  l_plugin_iterator = NULL;
  l_plugin_data_len = 0;
  l_apply_mode = PAPP_CONSTANT;
  l_dpy_id = -1;
  l_last_frame_filename = NULL;

  /* init step direction */  
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

    l_total_steps = l_begin - l_end;
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

    l_total_steps = l_end - l_begin;
  }
  
  l_cur_step = l_total_steps;

  l_cur_frame_nr = l_begin;
  while(1)              /* loop foreach frame in range */
  {
    if(gap_debug) fprintf(stderr, "p_frames_modify While l_cur_frame_nr = %d\n", (int)l_cur_frame_nr);

    /* build the frame name */
    if(ainfo_ptr->new_filename != NULL) g_free(ainfo_ptr->new_filename);
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
    {
       printf("gap: p_frames_modify: cant alloc layer info list\n");
       goto error;
    }

    if((l_cur_frame_nr == l_begin) && (action_mode == ACM_APPLY_FILTER))
    {
      /* ------------- 1.st frame: extra dialogs for APPLY_FILTER ---------- */

      if(l_sel_cnt < 1)
      {
         p_msg_win(RUN_INTERACTIVE, "No selected Layer in start frame\n");
         goto error;
      }
      
      if(l_begin != l_end)
      {
        l_last_frame_filename = p_alloc_fname(ainfo_ptr->basename,
                                        l_end,
                                        ainfo_ptr->extension);
      }
      
      /* additional dialog steps  a) gap_pdb_browser (select the filter)
       *                          b) 1st interactive filtercall
       *                          c) 1st pitstop dialog
       *                          d) [ 2nd interactive filtercall
       *                          e)   2nd pitstop dialog ]
       */

      l_rc = p_do_filter_dialogs(ainfo_ptr,
                                 l_tmp_image_id, &l_dpy_id,
                                 l_layli_ptr, l_nlayers,
                                &l_filter_procname[0], sizeof(l_filter_procname),
                                &l_plugin_data_len,
				&l_apply_mode
				 );

      if(l_last_frame_filename != NULL)
      {
        if((l_rc == 0) && (l_apply_mode == PTYP_VARYING_LINEAR))
	{
          l_rc = p_do_2nd_filter_dialogs(&l_filter_procname[0],
				   l_apply_mode,
				   l_last_frame_filename,
				   sel_mode, sel_case, sel_invert, sel_pattern
				  );
        }

        g_free(l_last_frame_filename);
        l_last_frame_filename = NULL;
      }

      /* the 1st selected layer has been filtered
       * in the INTERACTIVE call b)
       * therefore we unselect this layer, to avoid
       * a 2nd processing
       */
      l_idx = p_get_1st_selected(l_layli_ptr, l_nlayers);
      if(l_idx >= 0)
      {
        l_layli_ptr[l_idx].selected = FALSE;
        l_sel_cnt--;
      }

      /* check for matching Iterator PluginProcedures */
      if(l_apply_mode == PTYP_VARYING_LINEAR )
      {
        l_plugin_iterator =  p_get_iterator_proc(&l_filter_procname[0]);
      }
    }

    if(l_rc != 0)
      goto error;

    /* perform function (defined by action_mode) on selcted layer(s) */
    l_rc = p_apply_action(l_tmp_image_id,
		   action_mode,
		   l_layli_ptr,
		   l_nlayers,
		   l_sel_cnt,
		   l_begin, l_end, l_cur_frame_nr,
		   new_layername,
		   &l_filter_procname[0]
		   );
    if(l_rc != 0)
    {
      if(gap_debug) fprintf(stderr, "gap: p_frames_modify p_apply-action failed. rc=%d\n", (int)l_rc);
      goto error;
    }

    /* free layli info table for the current frame */ 
    if(l_layli_ptr != NULL)
    {
      g_free(l_layli_ptr);
      l_layli_ptr = NULL;
    }

    /* check if the resulting image has at least one layer */
    p_prevent_empty_image(l_tmp_image_id);
  
    /* save current frame with same name */
    l_rc = p_save_named_frame(l_tmp_image_id, ainfo_ptr->new_filename);
    if(l_rc < 0)
    {
      printf("gap: p_frames_modify save frame %d failed.\n", (int)l_cur_frame_nr);
      goto error;
    }
    else l_rc = 0;

    /* iterator call (for filter apply with varying values) */
    if((action_mode == ACM_APPLY_FILTER)
    && (l_plugin_iterator != NULL) && (l_apply_mode == PTYP_VARYING_LINEAR ))
    {
       l_cur_step -= 1.0;
        /* call plugin-specific iterator, to modify
         * the plugin's last_values
         */
       if(gap_debug) fprintf(stderr, "DEBUG: calling iterator %s  current frame:%d\n",
        		       l_plugin_iterator, (int)l_cur_frame_nr);
       l_params = gimp_run_procedure (l_plugin_iterator,
	  		     &l_retvals,
	  		     PARAM_INT32,   RUN_NONINTERACTIVE,
	  		     PARAM_INT32,   l_total_steps,          /* total steps  */
	  		     PARAM_FLOAT,   (gdouble)l_cur_step,    /* current step */
	  		     PARAM_INT32,   l_plugin_data_len, /* length of stored data struct */
	  		     PARAM_END);
       if (l_params[0].data.d_status == FALSE) 
       { 
         fprintf(stderr, "ERROR: iterator %s  failed\n", l_plugin_iterator);
         l_rc = -1;
       }

       g_free(l_params);
    }


    /* close display (if open) */
    if (l_dpy_id >= 0)
    {
      gimp_display_delete(l_dpy_id);
      l_dpy_id = -1;
    }

    /* destroy the tmp image */
    gimp_image_delete(l_tmp_image_id);

    if(l_rc != 0)
      goto error;
 

    
    if(ainfo_ptr->run_mode == RUN_INTERACTIVE)
    { 
      l_percentage += l_percentage_step;
      gimp_progress_update (l_percentage);
    }

    /* advance to next frame */
    if(l_cur_frame_nr == l_end)
       break;
    l_cur_frame_nr += l_step;

  }		/* end while(1)  loop foreach frame in range */

  if(gap_debug) fprintf(stderr, "p_frames_modify End OK\n");

  return 0;

error:
  if(gap_debug) fprintf(stderr, "gap: p_frames_modify exit with Error\n");

  if (l_dpy_id >= 0)
  {
      gimp_display_delete(l_dpy_id);
      l_dpy_id = -1;
  }
  if(l_tmp_image_id >= 0) gimp_image_delete(l_tmp_image_id);
  if(l_layli_ptr != NULL) g_free(l_layli_ptr);
  if(l_plugin_iterator != NULL)  g_free(l_plugin_iterator);
  return -1;
  
}		/* end p_frames_modify */


/* ============================================================================
 * gap_mod_layer
 * ============================================================================
 */

gint gap_mod_layer(GRunModeType run_mode, gint32 image_id,
                   gint32 range_from,  gint32 range_to,
                   gint32 action_mode, gint32 sel_mode,
                   gint32 sel_case, gint32 sel_invert,
                   char *sel_pattern, char *new_layername)
{
  int    l_rc;
  t_anim_info *ainfo_ptr;
  gint32    l_from;
  gint32    l_to;
  gint32    l_action_mode;
  gint32    l_sel_mode;
  gint32    l_sel_case;
  gint32    l_sel_invert;
  
  char      l_sel_pattern[MAX_LAYERNAME];
  char      l_new_layername[MAX_LAYERNAME];
  
  l_rc = 0;
  
  
  ainfo_ptr = p_alloc_ainfo(image_id, run_mode);
  if(ainfo_ptr != NULL)
  {

    if (0 == p_dir_ainfo(ainfo_ptr))
    {
      if(run_mode == RUN_INTERACTIVE)
      {
         l_rc = p_layer_modify_dialog (ainfo_ptr, &l_from, &l_to,
	                               &l_action_mode,
				       &l_sel_mode, &sel_case, &sel_invert,
				       &l_sel_pattern[0], &l_new_layername[0]);

      }
      else
      {
         l_from        = range_from;
         l_to          = range_to;
	 l_action_mode = action_mode;
	 l_sel_mode    = sel_mode;
	 l_sel_case    = sel_case;
	 l_sel_invert  = sel_invert;
  
         strncpy(&l_sel_pattern[0], sel_pattern, sizeof(l_sel_pattern) -1);
	 l_sel_pattern[sizeof(l_sel_pattern) -1] = '\0';
         strncpy(&l_new_layername[0], new_layername, sizeof(l_new_layername) -1);
	 l_new_layername[sizeof(l_new_layername) -1] = '\0';
      }

      if(l_rc >= 0)
      {
         l_rc = p_save_named_frame(ainfo_ptr->image_id, ainfo_ptr->old_filename);
         if(l_rc >= 0)
         {
           l_rc = p_frames_modify(ainfo_ptr, l_from, l_to,
	                          l_action_mode, 
				  l_sel_mode, sel_case, sel_invert,
				  &l_sel_pattern[0], &l_new_layername[0]
				 );
           p_load_named_frame(ainfo_ptr->image_id, ainfo_ptr->old_filename);
         }
      }


    }
    p_free_ainfo(&ainfo_ptr);
  }
  

  return(l_rc);   
}


