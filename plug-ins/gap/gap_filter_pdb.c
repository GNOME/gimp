/* gap_filter_pdb.c
 * 1998.10.14 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains:
 * - GAP_filter  pdb: functions for calling any Filter (==Plugin Proc)
 *                    that operates on a drawable
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
 * version gimp 1.1.17b  2000.02.22  hof: - removed limit PLUGIN_DATA_SIZE
 *                                        - removed support for old gimp 1.0.x PDB-interface.
 * version 0.97.00                   hof: - created module (as extract gap_filter_foreach)
 */
#include "config.h"

/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* GIMP includes */
#include "gtk/gtk.h"
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

static char *global_plugin_data = NULL;

static gint32 g_current_image_id;



gint p_call_plugin(char *plugin_name, gint32 image_id, gint32 layer_id, GimpRunModeType run_mode)
{
  GimpDrawable    *l_drawable;
  GimpParam       *l_ret_params;
  GimpParam       *l_argv;
  gint             l_retvals;
  gint             l_idx;
  gint             l_nparams;
  gint             l_nreturn_vals;
  GimpPDBProcType  l_proc_type;
  gchar           *l_proc_blurb;
  gchar           *l_proc_help;
  gchar           *l_proc_author;
  gchar           *l_proc_copyright;
  gchar           *l_proc_date;
  GimpParamDef    *l_params;
  GimpParamDef    *l_return_vals;

  l_drawable =  gimp_drawable_get(layer_id);  /* use the background layer */

  /* query for plugin_name to get its argument types */
  if (!gimp_procedural_db_proc_info (plugin_name,
				     &l_proc_blurb,
				     &l_proc_help,
				     &l_proc_author,
				     &l_proc_copyright,
				     &l_proc_date,
				     &l_proc_type,
				     &l_nparams,
				     &l_nreturn_vals,
				     &l_params,
				     &l_return_vals))
  {
    fprintf(stderr, "ERROR: Plugin not available, Name was %s\n", plugin_name);
    return -1;
  }			    

  /* construct the procedures arguments */
  l_argv = g_new (GimpParam, l_nparams);
  memset (l_argv, 0, (sizeof (GimpParam) * l_nparams));

  /* initialize the argument types */
  for (l_idx = 0; l_idx < l_nparams; l_idx++)
  {
    l_argv[l_idx].type = l_params[l_idx].type;
    switch(l_params[l_idx].type)
    {
      case GIMP_PDB_DISPLAY:
        l_argv[l_idx].data.d_display  = -1;
        break;
      case GIMP_PDB_DRAWABLE:
      case GIMP_PDB_LAYER:
      case GIMP_PDB_CHANNEL:
        l_argv[l_idx].data.d_drawable  = -1;
        break;
      case GIMP_PDB_IMAGE:
        l_argv[l_idx].data.d_image  = -1;
        break;
      case GIMP_PDB_INT32:
      case GIMP_PDB_INT16:
      case GIMP_PDB_INT8:
        l_argv[l_idx].data.d_int32  = 0;
        break;
      case GIMP_PDB_FLOAT:
        l_argv[l_idx].data.d_float  = 0.0;
        break;
      case GIMP_PDB_STRING:
        l_argv[l_idx].data.d_string  =  NULL;
        break;
      default:
        l_argv[l_idx].data.d_int32  = 0;
        break;
      
    }
  }

  /* init the standard parameters, that should be common to all plugins */
  l_argv[0].data.d_int32     = run_mode;
  l_argv[1].data.d_image     = image_id;
  l_argv[2].data.d_drawable  = l_drawable->id;

  /* run the plug-in procedure */
  l_ret_params = gimp_run_procedure2 (plugin_name, &l_retvals, l_nparams, l_argv);
  /*  free up arguments and values  */
  g_free (l_argv);


  /*  free the query information  */
  g_free (l_proc_blurb);
  g_free (l_proc_help);
  g_free (l_proc_author);
  g_free (l_proc_copyright);
  g_free (l_proc_date);
  g_free (l_params);
  g_free (l_return_vals);



  if (l_ret_params[0].data.d_status == FALSE)
  {
    fprintf(stderr, "ERROR: p_call_plugin %s failed.\n", plugin_name);
    g_free(l_ret_params);
    return -1;
  }
  else
  {
    if(gap_debug) fprintf(stderr, "DEBUG: p_call_plugin: %s successful.\n", plugin_name);
    g_free(l_ret_params);
    return 0;
  }
}


int
p_save_xcf(gint32 image_id, char *sav_name)
{
  GimpParam* l_params;
  gint   l_retvals;

    /* save current image as xcf file
     * xcf_save does operate on the complete image,
     * the drawable is ignored. (we can supply a dummy value)
     */
    l_params = gimp_run_procedure ("gimp_xcf_save",
			         &l_retvals,
			         GIMP_PDB_INT32,    GIMP_RUN_NONINTERACTIVE,
			         GIMP_PDB_IMAGE,    image_id,
			         GIMP_PDB_DRAWABLE, 0,
			         GIMP_PDB_STRING, sav_name,
			         GIMP_PDB_STRING, sav_name, /* raw name ? */
			         GIMP_PDB_END);

    if (l_params[0].data.d_status == FALSE) return(-1);
    
    return 0;
}


/* ============================================================================
 * p_get_data
 *    try to get the plugin's data (key is usually the name of the plugin)
 *    and check for the length of the retrieved data.
 * if all done OK return the length of the retrieved data,
 * return -1 in case of errors.
 * ============================================================================
 */
gint p_get_data(char *key)
{
   int l_len;

   l_len = gimp_get_data_size (key);
   if(l_len < 1)
   {
      fprintf(stderr, "ERROR: no stored data found for Key %s\n", key);
      return -1;
   }
   if(global_plugin_data)
   {
     g_free(global_plugin_data);
   }
   global_plugin_data = g_malloc0(l_len+1);
   gimp_get_data(key, global_plugin_data);

   if(gap_debug) printf("DEBUG p_get_data Key:%s  retrieved bytes %d\n", key, (int)l_len);
   return (l_len);
}

/* ============================================================================
 * p_set_data
 *
 *    set global_plugin_data
 * ============================================================================
 */
void p_set_data(char *key, gint plugin_data_len)
{
  if(global_plugin_data)
  {
    gimp_set_data(key, global_plugin_data, plugin_data_len);
  }
}

/* ============================================================================
 * p_procedure_available
 * ============================================================================
 */


gint p_procedure_available(char  *proc_name, t_proc_type ptype)
{
  gint             l_nparams;
  gint             l_nreturn_vals;
  GimpPDBProcType  l_proc_type;
  gchar           *l_proc_blurb;
  gchar           *l_proc_help;
  gchar           *l_proc_author;
  gchar           *l_proc_copyright;
  gchar           *l_proc_date;
  GimpParamDef    *l_params;
  GimpParamDef    *l_return_vals;
  gint             l_rc;

  l_rc = 0;
  
  /* Query the gimp application's procedural database
   *  regarding a particular procedure.
   */
  if (gimp_procedural_db_proc_info (proc_name,
				    &l_proc_blurb,
				    &l_proc_help,
				    &l_proc_author,
				    &l_proc_copyright,
				    &l_proc_date,
				    &l_proc_type,
				    &l_nparams,
				    &l_nreturn_vals,
				    &l_params,
				    &l_return_vals))
    {
     /* procedure found in PDB */
     if(gap_debug) fprintf(stderr, "DEBUG: found in PDB %s\n", proc_name);

     switch(ptype)
     {
        case PTYP_ITERATOR:
           /* check exactly for Input Parametertypes (common to all Iterators) */
           if (l_proc_type != GIMP_EXTENSION )     { l_rc = -1; break; }
           if (l_nparams  != 4)                    { l_rc = -1; break; }
           if (l_params[0].type != GIMP_PDB_INT32)    { l_rc = -1; break; }
           if (l_params[1].type != GIMP_PDB_INT32)    { l_rc = -1; break; }
           if (l_params[2].type != GIMP_PDB_FLOAT)    { l_rc = -1; break; }
           if (l_params[3].type != GIMP_PDB_INT32)    { l_rc = -1; break; }
           break;
        case PTYP_CAN_OPERATE_ON_DRAWABLE:
           /* check if plugin can be a typical one, that works on one drawable */
           if (l_proc_type != GIMP_PLUGIN)         { l_rc = -1; break; }
           if (l_nparams  < 3)                      { l_rc = -1; break; }
	   if (l_params[0].type !=  GIMP_PDB_INT32)    { l_rc = -1; break; }
	   if (l_params[1].type !=  GIMP_PDB_IMAGE)    { l_rc = -1; break; }
	   if (l_params[2].type !=  GIMP_PDB_DRAWABLE) { l_rc = -1; break; }
           break;
        default:
           break;
     }
     /*  free the query information  */
     g_free (l_proc_blurb);
     g_free (l_proc_help);
     g_free (l_proc_author);
     g_free (l_proc_copyright);
     g_free (l_proc_date);
     g_free (l_params);
     g_free (l_return_vals);
  }
  else
  {
     return -1;
  }
  

  return l_rc;
}	/* end p_procedure_available */


/* ============================================================================
 * p_get_iterator_proc
 *   check the PDB for Iterator Procedures (suffix "_Iterator" or "_Iterator_ALT"
 * return Pointer to the name of the Iterator Procedure
 *        or NULL if not found
 * ============================================================================
 */

char * p_get_iterator_proc(char *plugin_name)
{
  char      *l_plugin_iterator;

  /* check for matching Iterator PluginProcedures */
  l_plugin_iterator = g_strdup_printf("%s_Iterator", plugin_name);
     
  /* check if iterator is available in PDB */
  if(p_procedure_available(l_plugin_iterator, PTYP_ITERATOR) < 0)
  {
     g_free(l_plugin_iterator);
     l_plugin_iterator = g_strdup_printf("%s_Iterator_ALT", plugin_name);
     
     /* check for alternative Iterator   _Iterator_ALT
      * for now i made some Iterator Plugins using the ending _ALT,
      * If New plugins were added or existing ones were updated
      * the Authors should supply original _Iterator Procedures
      * to be used instead of my Hacked versions without name conflicts.
      */
     if(p_procedure_available(l_plugin_iterator, PTYP_ITERATOR) < 0)
     {
        /* both iterator names are not available */
        g_free(l_plugin_iterator);
        l_plugin_iterator = NULL;
     }
  }
  
  return (l_plugin_iterator);
}	/* end p_get_iterator_proc */


/* ============================================================================
 * constraint procedures
 *
 * aer responsible for:
 * - sensitivity of the dbbrowser's Apply Buttons
 * - filter for dbbrowser's listbox
 * ============================================================================
 */

int p_constraint_proc_sel1(gchar *proc_name)
{
  int        l_rc;
  GimpImageBaseType l_base_type;

  /* here we should check, if proc_name
   * can operate on the current Imagetype (RGB, INDEXED, GRAY)
   * if not, 0 should be returned.
   *
   * I did not find a way to do this with the PDB Interface of gimp 0.99.16
   */
  return 1;

  l_rc = 0;    /* 0 .. set Apply Button in_sensitive */

  l_base_type =gimp_image_base_type(g_current_image_id);
  switch(l_base_type)
  {
    case GIMP_RGB:
    case GIMP_GRAY:
    case GIMP_INDEXED:
      l_rc = 1;
      break;
  }
  return l_rc;
}

int p_constraint_proc_sel2(gchar *proc_name)
{
  char *l_plugin_iterator;
  int l_rc;
  
  l_rc = p_constraint_proc_sel1(proc_name);
  if(l_rc != 0)
  {
    l_plugin_iterator =  p_get_iterator_proc(proc_name);
    if(l_plugin_iterator != NULL)
    {
       g_free(l_plugin_iterator);
       return 1;    /* 1 .. set "Apply Varying" Button sensitive */
    }
  }
  
  return 0;         /* 0 .. set "Apply Varying" Button in_sensitive */
}

int p_constraint_proc(gchar *proc_name)
{
  int l_rc;

  if(strncmp(proc_name, "file", 4) == 0)
  {
     /* Do not add file Plugins (check if name starts with "file") */
     return 0;
  }

  if(strncmp(proc_name, "plug_in_gap_", 12) == 0)
  {
     /* Do not add GAP Plugins (check if name starts with "plug_in_gap_") */
     return 0;
  }
  
  l_rc = p_procedure_available(proc_name, PTYP_CAN_OPERATE_ON_DRAWABLE);

  if(l_rc < 0)
  {
     /* Do not add, Plugin not available or wrong type */
     return 0;
  }

  return 1;    /* 1 add the plugin procedure */
}
