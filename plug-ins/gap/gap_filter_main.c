/* gap_filter_main.c
 * 1997.12.18 hof (Wolfgang Hofer)
 *
 * GAP ... Gimp Animation Plugins
 *
 * This Module contains:
 * - MAIN of GAP_filter  foreach: call any Filter (==Plugin Proc)
 *                                with varying settings for all
 *                                layers within one Image.
 * - query   registration of gap_foreach Procedure
 *                        and for all Iterator_ALT Procedures
 * - run     invoke the gap_foreach procedure by its PDB name
 * 
 *
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

/* SYTEM (UNIX) includes */ 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* GIMP includes */
#include "gtk/gtk.h"
#include "config.h"
#include "libgimp/stdplugins-intl.h"
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap_filter.h"

static char *gap_filter_version = "0.92.00; 1998/01/16";

/* revision history:
 * version 0.92.00              hof: set gap_debug from environment 
 * version 0.91.01; Tue Dec 23  hof: 1.st (pre) release
 */

/* ------------------------
 * global gap DEBUG switch
 * ------------------------
 */

/* int gap_debug = 1; */    /* print debug infos */
/* int gap_debug = 0; */    /* 0: dont print debug infos */

int gap_debug = 0;



static void query(void);
static void run(char *name, int nparam, GParam *param,
                int *nretvals, GParam **retvals);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query_proc */
  run,   /* run_proc */
};

MAIN ()

static void
query ()
{
  static GParamDef args_foreach[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image"},
    {PARAM_DRAWABLE, "drawable", "Input drawable (unused)"},
    {PARAM_STRING, "proc_name", "name of plugin procedure to run for each layer)"},
  };
  static int nargs_foreach = sizeof(args_foreach) / sizeof(args_foreach[0]);

  static GParamDef *return_vals = NULL;
  static int nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure("plug_in_gap_layers_run_animfilter",
			 _("This plugin calls another plugin for each layer of an image, varying its settings (to produce animated effects). The called plugin must work on a single drawable and must be able to RUN_WITH_LAST_VALS"),
			 "",
			 "Wolfgang Hofer (hof@hotbot.com)",
			 "Wolfgang Hofer",
			 gap_filter_version,
			 _("<Image>/Filters/Filter all Layers..."),
			 "RGB*, INDEXED*, GRAY*",
			 PROC_PLUG_IN,
			 nargs_foreach, nreturn_vals,
			 args_foreach, return_vals);

  /* ------------------ ALTernative Iterators ------------------------------ */

  gap_query_iterators_ALT();

			 
}	/* end query */


static void
run (char    *name,
     int      n_params,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
#define  MAX_PLUGIN_NAME_LEN  256

  char l_plugin_name[MAX_PLUGIN_NAME_LEN];
  static GParam values[1];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  gint32     image_id;
  gint32  len_struct;
  gint32  total_steps;
  gdouble current_step;
  
  gint32     l_rc;
  char       *l_env;

  *nreturn_vals = 1;
  *return_vals = values;
  l_rc = 0;

  l_env = g_getenv("GAP_DEBUG");
  if(l_env != NULL)
  {
    if((*l_env != 'n') && (*l_env != 'N')) gap_debug = 1;
  }


  run_mode = param[0].data.d_int32;


  if(gap_debug) fprintf(stderr, "\n\ngap_filter_main: debug name = %s\n", name);
  
  if (strcmp (name, "plug_in_gap_layers_run_animfilter") == 0)
  {
      if (run_mode == RUN_NONINTERACTIVE)
      {
        if (n_params != 4)
        {
          status = STATUS_CALLING_ERROR;
        }
        else
        {
          strncpy(l_plugin_name, param[3].data.d_string, MAX_PLUGIN_NAME_LEN -1);
          l_plugin_name[MAX_PLUGIN_NAME_LEN -1] = '\0';
        }
        INIT_I18N();
      }
      else if(run_mode == RUN_WITH_LAST_VALS)
      {
        /* probably get last values (name of last plugin) */
        gimp_get_data("plug_in_gap_layers_run_animfilter", l_plugin_name);
      } else {
        INIT_I18N_UI();
      }

      if (status == STATUS_SUCCESS)
      {

        image_id    = param[1].data.d_image;

        l_rc = gap_proc_anim_apply(run_mode, image_id, l_plugin_name);
        gimp_set_data("plug_in_gap_layers_run_animfilter", l_plugin_name, sizeof(l_plugin_name));
      }
  }
  else
  {
      if ((run_mode == RUN_NONINTERACTIVE) && (n_params == 4))
      {
        total_steps  =  param[1].data.d_int32;
        current_step =  param[2].data.d_float;
        len_struct   =  param[3].data.d_int32;
        l_rc =  gap_run_iterators_ALT(name, run_mode, total_steps, current_step, len_struct);
      }
      else status = STATUS_CALLING_ERROR;
  }

 if(l_rc < 0)
 {
    status = STATUS_EXECUTION_ERROR;
 }
 
  
 if (run_mode != RUN_NONINTERACTIVE)
    gimp_displays_flush();

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

}
