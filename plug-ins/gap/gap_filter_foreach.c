/* gap_filter_foreach.c
 * 1997.12.23 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains:
 * - GAP_filter  foreach: call any Filter (==Plugin Proc)
 *                        with varying settings for all
 *                        layers within one Image.
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
 * version 0.97.00              hof: - modul splitted (2.nd part is now gap_filter_pdb.c)
 * version 0.96.03              hof: - pitstop dialog provides optional backup on each step
 *                                     (and skip option)
 * version 0.96.00              hof: - now using gap_arr_dialog.h
 * version 0.92.00              hof: - pitstop dialog 
 *                                     give user a chance to stop after interactive plugin calls
 *                                     if you dont want the dialog export GAP_FILTER_PITSTOP="N"
 *                                   - fixed bug in restore of layervisibility
 *                                   - codegen via explicite button (in gap_debug mode)
 * version 0.91.01; Tue Dec 23  hof: 1.st (pre) release
 */
#include "config.h"

/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <string.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* GIMP includes */
#include "gtk/gtk.h"
#include "config.h"
#include "libgimp/stdplugins-intl.h"
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap_arr_dialog.h"
#include "gap_filter.h"
#include "gap_filter_pdb.h"
#include "gap_lib.h"


/* ------------------------
 * global gap DEBUG switch
 * ------------------------
 */

/* int gap_debug = 1; */    /* print debug infos */
/* int gap_debug = 0; */    /* 0: dont print debug infos */

extern int gap_debug;

static gint32 g_current_image_id;

void p_gdisplays_update_full(gint32 image_id)
{
  GimpParam* l_params;
  gint   l_retvals;

  if(p_procedure_available("gimp_image_update_full", PTYP_ANY) >= 0)
  {

    l_params = gimp_run_procedure ("gimp_image_update_full",
			         &l_retvals,
			         GIMP_PDB_IMAGE,  image_id,
			         GIMP_PDB_END);

     /* Note: gimp_displays_update_full is not available in the official release gimp 0.99.16
      *       (dont care if procedure is not there,
      *        --> the user may not see the current layer, because
      *            gimp_displays_flush() does not update on changes
      *            of the visibility.
      */

     g_free(l_params);
   }
   gimp_displays_flush();

}

/* pitstop dialog
 *   return -1 on cancel, 0 .. on continue, 1 .. on skip
 */
static gint p_pitstop(GimpRunModeType run_mode, char *plugin_name, gint text_flag,
                      char *step_backup_file, gint len_step_backup_file,
		      gint32 layer_idx)
{
  gchar *l_env;
  gchar *l_msg;
  static t_but_arg  l_but_argv[3];
  gint              l_but_argc;
  gint              l_argc;
  static t_arr_arg  l_argv[1];
  int               l_continue;
  char              l_skip_txt[20];
  
    

    p_init_arr_arg(&l_argv[0], WGT_FILESEL);
    l_argv[0].label_txt = _("backup to file");
    l_argv[0].entry_width = 140;	/* pixel */
    l_argv[0].help_txt  = _("Make backup of the image after each step");
    l_argv[0].text_buf_len = len_step_backup_file;
    l_argv[0].text_buf_ret = step_backup_file;

    l_but_argv[0].but_txt  = _("Continue");
    l_but_argv[0].but_val  = 0;
    l_but_argv[1].but_txt  = _("Cancel");
    l_but_argv[1].but_val  = -1;
    sprintf(l_skip_txt, "Skip %d", (int)layer_idx);
    l_but_argv[2].but_txt  = l_skip_txt;
    l_but_argv[2].but_val  = 1;

   l_but_argc = 2;
   l_argc = 0;
   /* optional dialog between both calls (to see the effect of 1.call) */
   if(run_mode == GIMP_RUN_INTERACTIVE)
   {
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
         l_msg = g_strdup_printf (_("2nd call of %s\n(define end-settings)"), plugin_name);
      }
      else
      {
         l_msg = g_strdup_printf (_("Non-Interactive call of %s\n(for all layers in between)"), plugin_name);
         l_but_argc = 3;
         l_argc = 1;
      }
      l_continue = p_array_std_dialog (_("Animated Filter apply"), l_msg,
                                       l_argc,     l_argv, 
                                       l_but_argc, l_but_argv, 0);
      g_free (l_msg);
      
      if(l_continue < 0) return -1;
      else               return l_continue;
      
   }
   
   return 0;  /* continue without question */
}	/* end p_pitstop */


static void p_visibilty_restore(gint32 image_id, gint nlayers, int *visible_tab, char *plugin_name)
{
  gint32    *l_layers_list;
  gint       l_nlayers2;
  gint32     l_idx;
  
      l_layers_list = gimp_image_get_layers(image_id, &l_nlayers2);
      if(l_nlayers2 == nlayers)
      {
        for(l_idx = 0; l_idx < nlayers; l_idx++)
        {
          gimp_layer_set_visible(l_layers_list[l_idx], visible_tab[l_idx]);
          if(gap_debug) printf("visibilty restore [%d] %d\n", (int)l_idx, (int)visible_tab[l_idx]);
        }
        p_gdisplays_update_full(image_id);
      }
      else
      {
        printf ("Error: Plugin %s has changed Nr. of layers from %d to %d\ncould not restore Layer visibilty.\n",
                plugin_name, (int)nlayers, (int)l_nlayers2);
      }

      g_free (l_layers_list);
}

static gint32 p_get_indexed_layerid(gint32 image_id, gint *nlayers, gint32 idx, char *plugin_name)
{
  gint32    *l_layers_list;
  gint32     l_layer_id;
  gint       l_nlayers2;

  l_layers_list = gimp_image_get_layers(image_id, &l_nlayers2);
  if(l_layers_list == NULL)
  {
      printf ("Warning: cant get layers (maybe the image was closed)\n");
      return -1;
  }
  if((l_nlayers2 != *nlayers) && (*nlayers > 0))
  {
     printf ("Error: Plugin %s has changed Nr. of layers from %d to %d\nAnim Filter apply stopped.\n", 
            plugin_name, (int)*nlayers, (int)l_nlayers2);
      return -1;
  }
  
  *nlayers = l_nlayers2;
  l_layer_id = l_layers_list[idx];
  g_free (l_layers_list);
  return (l_layer_id);
}

/* ============================================================================
 * p_foreach_multilayer
 *    apply the given plugin to each layer of the  image.
 * returns   image_id of the new created multilayer image
 *           (or -1 on error)
 * ============================================================================
 */

int p_foreach_multilayer(GimpRunModeType run_mode, gint32 image_id,
                         char *plugin_name, t_apply_mode apply_mode)
{
  static char l_key_from[512];
  static char l_key_to[512];
  char      *l_plugin_iterator;
  gint32     l_layer_id;
  gint32     l_top_layer;
  gint32     l_idx;
  gint       l_nlayers;
  gdouble    l_percentage, l_percentage_step;  
  GimpParam     *l_params;
  gint        l_retvals;
  int         l_rc;
  gint        l_plugin_data_len;
  long        l_child_pid;
  /* int         l_status; */
  int         *l_visible_tab;
  char         l_step_backup_file[120];
  gint         l_pit_rc;
  
  l_rc = 0;
  l_plugin_data_len = 0;
  l_nlayers = 0;
  l_visible_tab = NULL;
  l_step_backup_file[0] = '\0';
  
  /* check for the Plugin */

  l_rc = p_procedure_available(plugin_name, PTYP_CAN_OPERATE_ON_DRAWABLE);
  if(l_rc < 0)
  {
     fprintf(stderr, "ERROR: Plugin not available or wrong type %s\n", plugin_name);
     return -1;
  }


  /* check for matching Iterator PluginProcedures */
  l_plugin_iterator =  p_get_iterator_proc(plugin_name);

  l_percentage = 0.0;  
  if(run_mode == GIMP_RUN_INTERACTIVE)
  { 
    gimp_progress_init( _("Applying Filter to all Layers..."));
  }

  l_layer_id = p_get_indexed_layerid(image_id, &l_nlayers, 0,  plugin_name);
  if(l_layer_id >= 0)
  {
    if(l_nlayers < 1)
    {
       fprintf(stderr, "ERROR: need at least 1 Layers to apply plugin !\n");
    }
    else
    {
      /* allocate a table to store the visibility attributes for each layer */
      l_visible_tab = (gint*) g_malloc((l_nlayers +1) * sizeof (gint));
      if(l_visible_tab == NULL)
         return -1;

      /* save the visibility of all layers */         
      for(l_idx = 0; l_idx < l_nlayers; l_idx++)
      {
        l_layer_id = p_get_indexed_layerid(image_id, &l_nlayers, l_idx,  plugin_name);
        l_visible_tab[l_idx] = gimp_layer_get_visible(l_layer_id);
        
        /* make the backround visible, all others invisible 
         * (so the user can see the effect of the 1.st applied _FROM filter)
         */
        if(l_idx == (l_nlayers -1)) gimp_layer_set_visible(l_layer_id, TRUE);
        else                        gimp_layer_set_visible(l_layer_id, FALSE);
      }
      p_gdisplays_update_full(image_id);
  
      l_percentage_step = 1.0 / l_nlayers;
      
      if((l_plugin_iterator != NULL)  && (l_nlayers > 1) && (apply_mode == PTYP_VARYING_LINEAR ))
      {
        l_child_pid = 0; /* fork(); */
        if(l_child_pid < 0)
        {
           fprintf(stderr, "ERROR: fork failed !\n");
           return -1;
        }

        /* if(l_child_pid != 0) */
        {
          /* parent process: call plugin Interactive for background layer[n] */
          /* if(gap_debug) fprintf(stderr, "forked child process pid=%ld\n", l_child_pid); */
          if(gap_debug) fprintf(stderr, "DEBUG start 1.st Interactive call (_FROM values)\n");
          
          l_idx = l_nlayers -1;
          l_layer_id = p_get_indexed_layerid(image_id, &l_nlayers, l_idx,  plugin_name);
          if(l_layer_id < 0)
          {
            l_rc = -1;
          }
          else
          {
            if(gap_debug) fprintf(stderr, "DEBUG: apllying %s on Layerstack %d id=%d\n", plugin_name, (int)l_idx, (int)l_layer_id);
            l_rc = p_call_plugin(plugin_name, image_id, l_layer_id, GIMP_RUN_INTERACTIVE);

            /* get values, then store with suffix "_ITER_FROM" */
            l_plugin_data_len = p_get_data(plugin_name);
            if(l_plugin_data_len > 0)
            {
               sprintf(l_key_from, "%s_ITER_FROM", plugin_name);
               p_set_data(l_key_from, l_plugin_data_len);
            }
            else l_rc = -1;

            if(run_mode == GIMP_RUN_INTERACTIVE)
            { 
              l_percentage += l_percentage_step;
              gimp_progress_update (l_percentage);
            }
          }
        }
        /* else */
        if((l_rc >= 0) && (l_nlayers > 1))
        {
          /* child process: call plugin Interactive for top layer [0] */
          if(gap_debug) fprintf(stderr, "DEBUG start 2.nd Interactive call (_TO values)\n");

          /* optional dialog between both calls (to see the effect of 1.call) */
          if(p_pitstop(run_mode, plugin_name, 0,
	               l_step_backup_file, sizeof(l_step_backup_file), 0
		      )
	     < 0)
          {
              if(gap_debug) fprintf(stderr, "TERMINATED: by pitstop dialog\n");
              /* restore the visibility of all layers */         
              p_visibilty_restore(image_id, l_nlayers, l_visible_tab, plugin_name);
              g_free(l_visible_tab);
              return -1;
          }
          else
          {
             l_layer_id = p_get_indexed_layerid(image_id, &l_nlayers, 0,  plugin_name);
             if(l_layer_id < 0)
             {
               l_rc = -1;
             }
             else
             {
                /* make _TO layer visible */
                gimp_layer_set_visible(l_layer_id, TRUE);
                p_gdisplays_update_full(image_id);

                if(gap_debug) fprintf(stderr, "DEBUG: apllying %s on Layerstack 0  id=%d\n", plugin_name, (int)l_layer_id);
                l_rc = p_call_plugin(plugin_name, image_id, l_layer_id, GIMP_RUN_INTERACTIVE);

                /* get values, then store with suffix "_ITER_TO" */
                l_plugin_data_len = p_get_data(plugin_name);
                if(l_plugin_data_len > 0)
                {
                   sprintf(l_key_to, "%s_ITER_TO", plugin_name);
                   p_set_data(l_key_to, l_plugin_data_len);
                }
                else l_rc = -1;

                if(run_mode == GIMP_RUN_INTERACTIVE)
                { 
                  l_percentage += l_percentage_step;
                  gimp_progress_update (l_percentage);
                }

                /* if(gap_debug) fprintf(stderr, "DEBUG child process exit %d\n", (int)l_rc); */
                /* exit(l_rc); */                               /* end of childprocess */
             }

         }
          
        }
        l_top_layer = 1;
        
        /* wait until exit of childprocess */
        /* waitpid(l_child_pid, &l_status, 0); */
      }
      else
      {
          /* no iterator available, call plugin with constant values
           * for each layer
           */ 
          /* call plugin only ONCE Interactive for background layer[n] */
          l_layer_id = p_get_indexed_layerid(image_id, &l_nlayers, l_nlayers -1,  plugin_name);
          if(l_layer_id < 0)
          {
             l_rc = -1;
          }
          else
          {
            if(gap_debug) fprintf(stderr, "DEBUG: NO Varying, apllying %s on Layer id=%d\n", plugin_name, (int)l_layer_id);
            l_rc = p_call_plugin(plugin_name, image_id, l_layer_id, GIMP_RUN_INTERACTIVE);
            l_top_layer = 0;
            if(run_mode == GIMP_RUN_INTERACTIVE)
            { 
              l_percentage += l_percentage_step;
              gimp_progress_update (l_percentage);
            }
          }
      }

      if((l_rc >= 0) &&  (l_nlayers > 2))
      {

        /* call plugin foreach layer inbetween 
         * with runmode GIMP_RUN_WITH_LAST_VALS
         * and modify the last values
         */
        l_pit_rc = 1;
        for(l_idx = l_nlayers - 2; l_idx >= l_top_layer; l_idx--)
        {
          if(l_rc < 0) break;

          if(l_pit_rc > 0)   /* last pit_rc was a skip, so ask again for the next layer */
	  {
            l_pit_rc = p_pitstop(run_mode, plugin_name, 1,
	  	               l_step_backup_file, sizeof(l_step_backup_file),
		               l_idx );
	  }
          if(l_pit_rc < 0)
          {
              if(gap_debug) fprintf(stderr, "TERMINATED: by pitstop dialog\n");
              l_rc = -1;
          }

          l_layer_id = p_get_indexed_layerid(image_id, &l_nlayers, l_idx,  plugin_name);
          if(l_layer_id < 0)
          {
             l_rc = -1;
             break;
          }

          if(gap_debug) fprintf(stderr, "DEBUG: apllying %s on Layerstack %d id=%d\n", plugin_name, (int)l_idx, (int)l_layer_id);
          

          if((l_plugin_iterator != NULL) && (apply_mode == PTYP_VARYING_LINEAR ))
          {
              /* call plugin-specific iterator, to modify
               * the plugin's last_values
               */
             if(gap_debug) fprintf(stderr, "DEBUG: calling iterator %s  current step:%d\n",
	                             l_plugin_iterator, (int)l_idx);
             l_params = gimp_run_procedure (l_plugin_iterator,
			           &l_retvals,
			           GIMP_PDB_INT32,   GIMP_RUN_NONINTERACTIVE,
			           GIMP_PDB_INT32,   l_nlayers -1,      /* total steps  */
			           GIMP_PDB_FLOAT,   (gdouble)l_idx,    /* current step */
			           GIMP_PDB_INT32,   l_plugin_data_len, /* length of stored data struct */
			           GIMP_PDB_END);
             if (l_params[0].data.d_status == FALSE) 
             { 
               fprintf(stderr, "ERROR: iterator %s  failed\n", l_plugin_iterator);
               l_rc = -1;
             }

             g_free(l_params);
          }
 
          if(l_rc < 0) break;

          if(l_pit_rc == 0)  /* 0 == continue without further dialogs */
	  {
             /* call the plugin itself with runmode RUN_WITH_LAST_VALUES */
             l_rc = p_call_plugin(plugin_name, image_id, l_layer_id, GIMP_RUN_WITH_LAST_VALS);
             /* check if to save each step to backup file */	  
	     if((l_step_backup_file[0] != '\0') && (l_step_backup_file[0] != ' '))
	     {
	       printf ("Saving image to backupfile:%s step = %d\n",
	               l_step_backup_file, (int)l_idx);
	       p_save_xcf(image_id, l_step_backup_file);
	     }
	  }

          if(run_mode == GIMP_RUN_INTERACTIVE)
          { 
            l_percentage += l_percentage_step;
            gimp_progress_update (l_percentage);
          }

        }	/* end for */

      }

      /* restore the visibility of all layers */         
      p_visibilty_restore(image_id, l_nlayers, l_visible_tab, plugin_name);
      g_free(l_visible_tab);
    
    }
    
  }


  if(l_plugin_iterator != NULL)      g_free(l_plugin_iterator);

  return l_rc;  
}	/* end p_foreach_multilayer */



/* ============================================================================
 * gap_proc_anim_apply
 * ============================================================================
 */

gint gap_proc_anim_apply(GimpRunModeType run_mode, gint32 image_id, char *plugin_name)
{
  t_gap_db_browse_result  l_browser_result;
  t_apply_mode            l_apply_mode;

  l_apply_mode = PAPP_CONSTANT;
  g_current_image_id = image_id;
  
  if(run_mode == GIMP_RUN_INTERACTIVE)
  {

    if(gap_db_browser_dialog( _("Select Filter for Animated apply"),
                                 _("Apply Constant"),
                                 _("Apply Varying"),
                                 p_constraint_proc,
                                 p_constraint_proc_sel1,
                                 p_constraint_proc_sel2,
                                 &l_browser_result,
				 TRUE   /* call gtk_init */
			  ) 
      < 0)
    {
      if(gap_debug) fprintf(stderr, "DEBUG: gap_db_browser_dialog cancelled\n");
      return -1;
    }
    
    p_arr_gtk_init(FALSE); /* disable the initial gtk_init in gap_arr_dialog's
                            * (gtk_init was done by the browser dialog)
			    */
    
    strcpy(plugin_name, l_browser_result.selected_proc_name);
    if(l_browser_result.button_nr == 1) l_apply_mode = PTYP_VARYING_LINEAR;
    
    if(gap_debug) fprintf(stderr, "DEBUG: gap_db_browser_dialog SELECTED:%s\n", plugin_name);

  }

  return(p_foreach_multilayer(run_mode, 
                              image_id,
			      plugin_name,
			      l_apply_mode ));

}
