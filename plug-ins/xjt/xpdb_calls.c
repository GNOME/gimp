/* xpdb_calls.c
 *
 * this module contains calls of procedures in the GIMPs Procedural Database
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
 * version 1.1.18a; 2000/03/07  hof: tattoo_state
 * version 1.1.16a; 2000/02/04  hof: path lockedstaus, tattoo set procedures
 * version 1.1.15b; 2000/01/28  hof: parasites
 *                                   removed old gimp 1.0.x PDB Interfaces
 * version 1.1.15;  2000/01/20  hof: parasites
 * version 1.02.00; 1999/02/01  hof: PDB-calls to load/save resolution tattoos and parasites
 *                                   busy_cursors (needs GIMP 1.1.1)
 * version 1.01.00; 1998/11/22  hof: PDB-calls to load/save guides under GIMP 1.1
 * version 1.00.00; 1998/10/31  hof: 1.st (pre) release
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* GIMP includes */
#include "libgimp/gimp.h"
#include "xjpeg.h"

/* #include "cursorutil.h" */

/* XJT includes */
#include "xpdb_calls.h"

extern gint xjt_debug;

/* ============================================================================
 * p_procedure_available
 *   if requested procedure is available in the PDB return the number of args
 *      (0 upto n) that are needed to call the procedure.
 *   if not available return -1
 * ============================================================================
 */

gint p_procedure_available(gchar *proc_name)
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
  if(gimp_query_procedure  (proc_name,
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
     return (l_nparams);
  }

  printf("Warning: Procedure %s not found.\n", proc_name);
  return -1;
}	/* end p_procedure_available */

/* ---------------------- PDB procedure calls  -------------------------- */

/* ============================================================================
 * p_get_gimp_selection_bounds
 *   
 * ============================================================================
 */
gint
p_get_gimp_selection_bounds (gint32 image_id, gint32 *x1, gint32 *y1, gint32 *x2, gint32 *y2)
{
   static gchar    *l_get_sel_bounds_proc = "gimp_selection_bounds";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_get_sel_bounds_proc,
                                 &nreturn_vals,
                                 PARAM_IMAGE, image_id,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      *x1 = return_vals[2].data.d_int32;
      *y1 = return_vals[3].data.d_int32;
      *x2 = return_vals[4].data.d_int32;
      *y2 = return_vals[5].data.d_int32;
      return(return_vals[1].data.d_int32);
   }
   printf("XJT: Error: PDB call of %s failed staus=%d\n", 
          l_get_sel_bounds_proc, (int)return_vals[0].data.d_status);
   return(FALSE);
}	/* end p_get_gimp_selection_bounds */

/* ============================================================================
 * p_gimp_selection_load
 *   
 * ============================================================================
 */
gint
p_gimp_selection_load (gint32 image_id, gint32 channel_id)
{
   static gchar    *l_sel_load = "gimp_selection_load";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_sel_load,
                                 &nreturn_vals,
                                 PARAM_CHANNEL, channel_id,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return(TRUE);
   }
   printf("XJT: Error: PDB call of %s failed status=%d\n", 
          l_sel_load, (int)return_vals[0].data.d_status);
   return(FALSE);
}	/* end p_gimp_selection_load */


/* ============================================================================
 * p_layer_set_linked
 *   set linked state of the layer
 * ============================================================================
 */
int
p_layer_set_linked (gint32 layer_id, gint32 new_state)
{
   static gchar    *l_set_linked_proc = "gimp_layer_set_linked";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_set_linked_proc,
                                 &nreturn_vals,
                                 PARAM_LAYER, layer_id,
                                 PARAM_INT32, new_state,  /* TRUE or FALSE */
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return (0);
   }
   printf("XJT: Error: p_set_linked to state %d failed\n",(int)new_state);
   return(-1);
}	/* end p_layer_set_linked */

/* ============================================================================
 * p_layer_get_linked
 *   
 * ============================================================================
 */

gint p_layer_get_linked(gint32 layer_id)
{
  GParam *return_vals;
  int nreturn_vals;
  gint32 is_linked;

  is_linked = FALSE;

  return_vals = gimp_run_procedure ("gimp_layer_get_linked",
                                    &nreturn_vals,
                                    PARAM_LAYER, layer_id,
                                    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
  {
    is_linked = return_vals[1].data.d_int32;
  }
  gimp_destroy_params (return_vals, nreturn_vals);

  return is_linked;
}

/* ============================================================================
 * p_gimp_image_floating_sel_attached_to
 *   
 * ============================================================================
 */

gint32 p_gimp_image_floating_sel_attached_to(gint32 image_id)
{
   static gchar    *l_fsel_attached_to_proc = "gimp_image_floating_sel_attached_to";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_fsel_attached_to_proc,
                                 &nreturn_vals,
                                 PARAM_IMAGE, image_id,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return(return_vals[1].data.d_drawable);
   }
   printf("XJT: Error: PDB call of %s failed\n", l_fsel_attached_to_proc);
   return(-1);
}	/* end p_gimp_image_floating_sel_attached_to */

/* ============================================================================
 * p_gimp_floating_sel_attach
 *   
 * ============================================================================
 */

gint   p_gimp_floating_sel_attach(gint32 layer_id, gint32 drawable_id)
{
   static gchar    *l_fsel_attach_proc = "gimp_floating_sel_attach";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_fsel_attach_proc,
                                 &nreturn_vals,
                                 PARAM_LAYER,    layer_id,
                                 PARAM_DRAWABLE, drawable_id,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return (0);
   }
   printf("XJT: Error: PDB call of %s failed\n", l_fsel_attach_proc);
   return(-1);
}	/* end p_gimp_floating_sel_attach */

/* ============================================================================
 * p_gimp_floating_sel_rigor
 *   
 * ============================================================================
 */

gint   p_gimp_floating_sel_rigor(gint32 layer_id, gint32 undo)
{
   static gchar    *l_fsel_rigor_proc = "gimp_floating_sel_rigor";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_fsel_rigor_proc,
                                 &nreturn_vals,
                                 PARAM_LAYER, layer_id,
                                 PARAM_INT32, undo,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return (0);
   }
   printf("XJT: Error: PDB call of %s failed\n", l_fsel_rigor_proc);
   return(-1);
}	/* end p_gimp_floating_sel_rigor */

/* ============================================================================
 * p_gimp_floating_sel_relax
 *   
 * ============================================================================
 */

gint   p_gimp_floating_sel_relax(gint32 layer_id, gint32 undo)
{
   static gchar    *l_fsel_relax_proc = "gimp_floating_sel_relax";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_fsel_relax_proc,
                                 &nreturn_vals,
                                 PARAM_LAYER, layer_id,
                                 PARAM_INT32, undo,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return (0);
   }
   printf("XJT: Error: PDB call of %s failed\n", l_fsel_relax_proc);
   return(-1);
}	/* end p_gimp_floating_sel_relax */




/* ============================================================================
 * p_gimp_image_add_guide
 *   
 * ============================================================================
 */

gint32   p_gimp_image_add_guide(gint32 image_id, gint32 position, gint32 orientation)
{
   static gchar    *l_add_guide_proc;
   GParam          *return_vals;
   int              nreturn_vals;

   if(orientation == ORIENTATION_VERTICAL)
   {
      l_add_guide_proc = "gimp_image_add_vguide";
   }
   else
   {
      l_add_guide_proc = "gimp_image_add_hguide";
   }

   return_vals = gimp_run_procedure (l_add_guide_proc,
                                 &nreturn_vals,
                                 PARAM_IMAGE, image_id,
                                 PARAM_INT32, position,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return(return_vals[1].data.d_int32);  /* return the guide ID */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_add_guide_proc);
   return(-1);
}	/* end p_gimp_image_add_guide */

/* ============================================================================
 * p_gimp_image_findnext_guide
 *
 * This procedure takes an image and a guide_id as input and finds the guide_id
 * of the successor of the given guide_id in the image's Guide list.
 * If the supplied guide_id is 0, the procedure will return the first Guide.
 * The procedure will return 0 if given the final guide_id as an argument 
 * or the image has no guides.
 *   
 * ============================================================================
 */

gint32   p_gimp_image_findnext_guide(gint32 image_id, gint32 guide_id)
{
   static gchar    *l_findnext_guide_proc = "gimp_image_find_next_guide";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_findnext_guide_proc,
                                 &nreturn_vals,
                                 PARAM_IMAGE, image_id,
                                 PARAM_INT32, guide_id,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return(return_vals[1].data.d_int32);  /* return the next guide ID */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_findnext_guide_proc);
   return(-1);
}	/* end p_gimp_image_findnext_guide */



/* ============================================================================
 * p_gimp_image_get_guide_position
 *
 * ============================================================================
 */

gint32   p_gimp_image_get_guide_position(gint32 image_id, gint32 guide_id)
{
   static gchar    *l_get_guide_pos_proc = "gimp_image_get_guide_position";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_get_guide_pos_proc,
                                 &nreturn_vals,
                                 PARAM_IMAGE, image_id,
                                 PARAM_INT32, guide_id,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return(return_vals[1].data.d_int32);  /* return the guide position */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_get_guide_pos_proc);
   return(-1);
}	/* end p_gimp_image_get_guide_position */


/* ============================================================================
 * p_gimp_image_get_guide_orientation
 *
 * ============================================================================
 */

gint  p_gimp_image_get_guide_orientation(gint32 image_id, gint32 guide_id)
{
   static gchar    *l_get_guide_pos_orient = "gimp_image_get_guide_orientation";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_get_guide_pos_orient,
                                 &nreturn_vals,
                                 PARAM_IMAGE, image_id,
                                 PARAM_INT32, guide_id,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return(return_vals[1].data.d_int32); /* return the guide orientation */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_get_guide_pos_orient);
   return(-1);
}	/* end p_gimp_image_get_guide_orientation */

/* ============================================================================
 * p_gimp_image_get_resolution
 *
 * ============================================================================
 */

gint32  p_gimp_image_get_resolution (gint32 image_id, float *xresolution, float *yresolution)
{
   static gchar    *l_procname = "gimp_image_get_resolution";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_procname,
                                 &nreturn_vals,
                                 PARAM_IMAGE, image_id,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      *xresolution = return_vals[1].data.d_float;
      *yresolution = return_vals[2].data.d_float;
      return (0); /* OK */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_procname);
   *xresolution = 72.0;
   *yresolution = 72.0;
   return(-1);
}	/* end p_gimp_image_get_resolution */

/* ============================================================================
 * p_gimp_image_set_resolution
 *
 * ============================================================================
 */

gint  p_gimp_image_set_resolution (gint32 image_id, float xresolution, float yresolution)
{
   static gchar    *l_procname = "gimp_image_set_resolution";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_procname,
                                 &nreturn_vals,
                                 PARAM_IMAGE, image_id,
                                 PARAM_FLOAT, xresolution,
                                 PARAM_FLOAT, yresolution,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return (0); /* OK */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_procname);
   return(-1);
}	/* end p_gimp_image_set_resolution */

/* ============================================================================
 * p_gimp_layer_get_tattoo
 *
 * ============================================================================
 */

gint32  p_gimp_layer_get_tattoo (gint32 layer_id)
{
   static gchar    *l_procname = "gimp_layer_get_tattoo";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_procname,
                                 &nreturn_vals,
                                 PARAM_LAYER, layer_id,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return(return_vals[1].data.d_int32);  /* OK, return Tattoo Id */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_procname);
   return(-1);
}	/* end p_gimp_layer_get_tattoo */

/* ============================================================================
 * p_gimp_channel_get_tattoo
 *
 * ============================================================================
 */

gint32  p_gimp_channel_get_tattoo (gint32 channel_id)
{
   static gchar    *l_procname = "gimp_channel_get_tattoo";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_procname,
                                 &nreturn_vals,
                                 PARAM_CHANNEL, channel_id,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return(return_vals[1].data.d_int32);  /* OK, return Tattoo Id */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_procname);
   return(-1);
}	/* end p_gimp_channel_get_tattoo */

/* ============================================================================
 * p_gimp_drawable_parasite_list
 * ============================================================================
 */

gchar **
p_gimp_drawable_parasite_list (gint32 drawable_id, gint32 *num_parasites)
{
   static gchar    *l_procname = "gimp_drawable_parasite_list";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_procname,
                                    &nreturn_vals,
                                    PARAM_DRAWABLE, drawable_id,
                                    PARAM_END);
                                    
   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      *num_parasites = return_vals[1].data.d_int32;
      return(return_vals[2].data.d_stringarray);    /* OK, return name list */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_procname);

   *num_parasites = 0;
   return(NULL);
}	/* end p_gimp_drawable_parasite_list */

/* ============================================================================
 * p_gimp_image_parasite_list
 * ============================================================================
 */

gchar **
p_gimp_image_parasite_list (gint32 image_id, gint32 *num_parasites)
{
   static gchar    *l_procname = "gimp_image_parasite_list";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_procname,
                                    &nreturn_vals,
                                    PARAM_IMAGE,  image_id,
                                    PARAM_END);
                                    
   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      *num_parasites = return_vals[1].data.d_int32;
      return(return_vals[2].data.d_stringarray);    /* OK, return name list */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_procname);

   *num_parasites = 0;
   return(NULL);
}	/* end p_gimp_image_parasite_list */

/* ============================================================================
 * p_gimp_path_set_points
 *   
 * ============================================================================
 */

gint
p_gimp_path_set_points(gint32 image_id, gchar *name,
                       gint32 path_type, gint32 num_points, gdouble *path_points)
{
   static gchar    *l_called_proc = "gimp_path_set_points";
   GParam          *return_vals;
   int              nreturn_vals;

   gint32 l_idx;

   if(xjt_debug)
   {
       printf("XJT: PDB p_gimp_path_set_points image_id: %d PATH name:%s\n", (int)image_id, name);
       printf("     path_type: %d  num_points: %d\n", (int)path_type, (int)num_points);

       for(l_idx=0; l_idx < num_points; l_idx++)
       {
	  printf("     point[%03d] : %f\n", (int)l_idx, (float)path_points[l_idx]);
       }
   }

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 PARAM_IMAGE,  image_id,
                                 PARAM_STRING, name,
                                 PARAM_INT32,  path_type,
                                 PARAM_INT32,  num_points,
                                 PARAM_FLOATARRAY, path_points,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return(0);  /* OK */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_called_proc);
   return(-1);
}	/* end p_gimp_path_set_points */



/* ============================================================================
 * p_gimp_path_get_points
 *   
 * ============================================================================
 */
gdouble *
p_gimp_path_get_points(gint32 image_id, gchar *name,
                       gint32 *path_type, gint32 *path_closed, gint32 *num_points)
{
   static gchar    *l_called_proc = "gimp_path_get_points";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 PARAM_IMAGE, image_id,
                                 PARAM_STRING, name,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      *path_type = return_vals[1].data.d_int32;
      *path_closed = return_vals[2].data.d_int32;
      *num_points = return_vals[3].data.d_int32;

      if(xjt_debug)
      {
	gint32 l_idx;

	printf("XJT: PDB p_gimp_path_get_points image_id: %d PATH name:%s\n", (int)image_id, name);
	printf("     path_type: %d  num_points: %d closed:%d\n", (int)*path_type, (int)*num_points, (int)*path_closed);

	for(l_idx=0; l_idx < *num_points; l_idx++)
	{
	   printf("  G  point[%03d] : %f\n", (int)l_idx, (float)return_vals[4].data.d_floatarray[l_idx]);
	}
      }

      return(return_vals[4].data.d_floatarray); /* OK return path points */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_called_proc);
   *num_points = 0;
   return(NULL);
}	/* end p_gimp_path_get_points */

/* ============================================================================
 * p_gimp_path_list
 *   
 * ============================================================================
 */

gchar **
p_gimp_path_list(gint32 image_id, gint32 *num_paths)
{
   static gchar    *l_called_proc = "gimp_path_list";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 PARAM_IMAGE, image_id,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      *num_paths = return_vals[1].data.d_int32;
      return(return_vals[2].data.d_stringarray);  /* OK, return path names */
   }
   printf("GAP: Error: PDB call of %s failed\n", l_called_proc);
   *num_paths = 0;
   return(NULL);
}	/* end p_gimp_path_list */

/* ============================================================================
 * p_gimp_path_get_current
 *   
 * ============================================================================
 */

gchar *
p_gimp_path_get_current(gint32 image_id)
{
   static gchar    *l_called_proc = "gimp_path_get_current";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 PARAM_IMAGE, image_id,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return(g_strdup(return_vals[1].data.d_string)); /* OK */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_called_proc);
   return(NULL);
}	/* end p_gimp_path_get_current */

/* ============================================================================
 * p_gimp_path_set_current
 *   
 * ============================================================================
 */

gint
p_gimp_path_set_current(gint32 image_id, gchar *name)
{
   static gchar    *l_called_proc = "gimp_path_set_current";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 PARAM_IMAGE, image_id,
                                 PARAM_STRING, name,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return(0); /* OK */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_called_proc);
   return(-1);
}	/* end p_gimp_path_set_current */

/* ============================================================================
 * p_gimp_path_get_locked
 *   
 * ============================================================================
 */

gint32
p_gimp_path_get_locked(gint32 image_id, gchar *name)
{
   static gchar    *l_called_proc = "gimp_path_get_locked";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 PARAM_IMAGE, image_id,
                                 PARAM_STRING, name,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return(return_vals[1].data.d_int32); /* OK */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_called_proc);
   return(FALSE);
}	/* end p_gimp_path_get_locked */

/* ============================================================================
 * p_gimp_path_set_locked
 *   
 * ============================================================================
 */

gint
p_gimp_path_set_locked(gint32 image_id, gchar *name, gint32 lockstatus)
{
   static gchar    *l_called_proc = "gimp_path_set_locked";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 PARAM_IMAGE, image_id,
                                 PARAM_STRING, name,
                                 PARAM_INT32, lockstatus,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return(0); /* OK */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_called_proc);
   return(-1);
}	/* end p_gimp_path_set_locked */

/* ============================================================================
 * p_gimp_path_get_tattoo
 *   
 * ============================================================================
 */

gint32
p_gimp_path_get_tattoo(gint32 image_id, gchar *name)
{
   static gchar    *l_called_proc = "gimp_path_get_tattoo";
   GParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 PARAM_IMAGE, image_id,
                                 PARAM_STRING, name,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return(return_vals[1].data.d_int32); /* OK */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_called_proc);
   return(-1);
}	/* end p_gimp_path_get_tattoo */

/* ============================================================================
 * p_gimp_path_set_tattoo
 *   
 * ============================================================================
 */

gint
p_gimp_path_set_tattoo(gint32 image_id, gchar *name, gint32 tattoovalue)
{
   static gchar    *l_called_proc = "gimp_path_set_tattoo";
   GParam          *return_vals;
   int              nreturn_vals;

   if(p_procedure_available(l_called_proc) < 0)
   {
     return(-1);
   }
   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 PARAM_IMAGE, image_id,
                                 PARAM_STRING, name,
                                 PARAM_INT32, tattoovalue,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return(0); /* OK */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_called_proc);
   return(-1);
}	/* end p_gimp_path_set_tattoo */


/* ============================================================================
 * p_gimp_layer_set_tattoo
 *   
 * ============================================================================
 */

gint
p_gimp_layer_set_tattoo(gint32 layer_id, gint32 tattoovalue)
{
   static gchar    *l_called_proc = "gimp_layer_set_tattoo";
   GParam          *return_vals;
   int              nreturn_vals;

   if(p_procedure_available(l_called_proc) < 0)
   {
     return(-1);
   }
   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 PARAM_LAYER, layer_id,
                                 PARAM_INT32, tattoovalue,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return(0); /* OK */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_called_proc);
   return(-1);
}	/* end p_gimp_layer_set_tattoo */


/* ============================================================================
 * p_gimp_channel_set_tattoo
 *   
 * ============================================================================
 */

gint
p_gimp_channel_set_tattoo(gint32 channel_id, gint32 tattoovalue)
{
   static gchar    *l_called_proc = "gimp_channel_set_tattoo";
   GParam          *return_vals;
   int              nreturn_vals;

   if(p_procedure_available(l_called_proc) < 0)
   {
     return(-1);
   }
   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 PARAM_CHANNEL, channel_id,
                                 PARAM_INT32, tattoovalue,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return(0); /* OK */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_called_proc);
   return(-1);
}	/* end p_gimp_channel_set_tattoo */


/* ============================================================================
 * p_gimp_image_set_tattoo_state
 *   
 * ============================================================================
 */

gint
p_gimp_image_set_tattoo_state(gint32 image_id, gint32 tattoo_state)
{
   static gchar    *l_called_proc = "gimp_image_set_tattoo_state";
   GParam          *return_vals;
   int              nreturn_vals;

   if(p_procedure_available(l_called_proc) < 0)
   {
     return(-1);
   }
   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 PARAM_IMAGE, image_id,
                                 PARAM_INT32, tattoo_state,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return(0); /* OK */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_called_proc);
   return(-1);
}	/* end p_gimp_image_set_tattoo_state */

/* ============================================================================
 * p_gimp_image_get_tattoo_state
 *   
 * ============================================================================
 */

gint32
p_gimp_image_get_tattoo_state(gint32 image_id)
{
   static gchar    *l_called_proc = "gimp_image_get_tattoo_state";
   GParam          *return_vals;
   int              nreturn_vals;

   if(p_procedure_available(l_called_proc) < 0)
   {
     return(-1);
   }
   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 PARAM_IMAGE, image_id,
                                 PARAM_END);

   if (return_vals[0].data.d_status == STATUS_SUCCESS)
   {
      return(return_vals[1].data.d_int32); /* OK */
   }
   printf("XJT: Error: PDB call of %s failed\n", l_called_proc);
   return(-1);
}	/* end p_gimp_image_get_tattoo_state */
