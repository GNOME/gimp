/* gap_pdb_calls.c
 *
 * this module contains calls of procedures in the GIMPs Procedural Database
 * 
 * IMPORTANT Notes:
 *   some Procedures have changed their Interface from GIMP 1.0.2 to GIMP 1.1
 *   in that cases the procedure parameters are checked and the call is done
 *   in 1.0.2 or 1.1 style repectivly.
 *
 *   some of these procedures are not available in the official GIMP 1.0.2 releases
 *   (and prior releases)
 *
 *   The missing procedures (except guides) are available as patches to the gimp core.
 *   If you dont install the patches GAP will NOT work on GIMP 1.0.2 and older versions.
 *
 * GIMP 1.1 will provide all the procedures to run GAP at full fuctionality.
 * There are no Patches required to run GAP in the latest GIMP 1.1
 * development version
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
 * version 0.98.00; 1998/11/28  hof: 1.st (pre) release (GAP port to GIMP 1.1)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* GIMP includes */
#include "libgimp/gimp.h"

/* GAP includes */
#include "gap_pdb_calls.h"

extern int gap_debug;

/* ============================================================================
 * p_pdb_procedure_available
 *   if requested procedure is available in the PDB return the number of args
 *      (0 upto n) that are needed to call the procedure.
 *   if not available return -1
 * ============================================================================
 */

gint p_pdb_procedure_available(char *proc_name)
{
   /* Note: It would be nice to call "gimp_layer_get_linked" direct,
    *       but there is not such an Interface in gimp 0.99.16
    * Workaround:
    *   I did a patch to implement the "gimp_layer_get_linked"
    *   procedure, and call it via PDB call if available.
    *   if not available FALSE is returned.
    */
    
  int             l_nparams;
  int             l_nreturn_vals;
  int             l_proc_type;
  char            *l_proc_blurb;
  char            *l_proc_help;
  char            *l_proc_author;
  char            *l_proc_copyright;
  char            *l_proc_date;
  GParamDef       *l_params;
  GParamDef       *l_return_vals;
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
}	/* end p_pdb_procedure_available */

/* ---------------------- PDB procedure calls  -------------------------- */

/* ============================================================================
 * p_get_gimp_selection_bounds
 *   
 * ============================================================================
 */
gint
p_get_gimp_selection_bounds (gint32 image_id, gint32 *x1, gint32 *y1, gint32 *x2, gint32 *y2)
{
   static char     *l_get_sel_bounds_proc = "gimp_selection_bounds";
   GParam          *return_vals;
   int              nreturn_vals;

   if (p_pdb_procedure_available(l_get_sel_bounds_proc) >= 0)
   {
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
      printf("GAP: Error: PDB call of %s failed staus=%d\n", 
             l_get_sel_bounds_proc, (int)return_vals[0].data.d_status);
   }
   else
   {
      printf("GAP: Error: Procedure %s not found.\n",l_get_sel_bounds_proc);
   }
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
   static char     *l_sel_load = "gimp_selection_load";
   GParam          *return_vals;
   int              nreturn_vals;
   int              l_nparams;

   l_nparams = p_pdb_procedure_available(l_sel_load);
   if (l_nparams >= 0)
   {
     /* check if it can take exactly one channel_id as input (gimp1.1) */
      if(l_nparams  == 1)
      {
         /* use the new Interface (Gimp 1.1 style)
          * (1.1 knows the image_id where the channel belongs to)
          */
         return_vals = gimp_run_procedure (l_sel_load,
                                    &nreturn_vals,
                                    PARAM_CHANNEL, channel_id,
                                    PARAM_END);
      }
      else
      {
         /* use the old Interface (Gimp 1.0.2 style) */
         return_vals = gimp_run_procedure (l_sel_load,
                                    &nreturn_vals,
                                    PARAM_IMAGE, image_id,
                                    PARAM_CHANNEL, channel_id,
                                    PARAM_END);
       }
                                   
      if (return_vals[0].data.d_status == STATUS_SUCCESS)
      {
         return(TRUE);
      }
      printf("GAP: Error: PDB call of %s failed status=%d\n", 
             l_sel_load, (int)return_vals[0].data.d_status);
   }
   else
   {
      printf("GAP: Error: Procedure %s not found.\n",l_sel_load);
   }
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
   static char     *l_set_linked_proc = "gimp_layer_set_linked";
   GParam          *return_vals;
   int              nreturn_vals;

   if (p_pdb_procedure_available(l_set_linked_proc) >= 0)
   {
      return_vals = gimp_run_procedure (l_set_linked_proc,
                                    &nreturn_vals,
                                    PARAM_LAYER, layer_id,
                                    PARAM_INT32, new_state,  /* TRUE or FALSE */
                                    PARAM_END);
                                    
      if (return_vals[0].data.d_status == STATUS_SUCCESS)
      {
         return (0);
      }
      printf("GAP: Error: p_layer_set_linked to state %d failed\n",(int)new_state);
   }
   else
   {
      printf("GAP: Warning: Procedure %s not found. (Layer Can not be set to linked)\n",l_set_linked_proc);
   }
   return(-1);
}	/* end p_layer_set_linked */

/* ============================================================================
 * p_layer_get_linked
 *   
 * ============================================================================
 */

gint p_layer_get_linked(gint32 layer_id)
{
   /* Note: The Procedure "gimp_layer_get_linked" is part of GIMP 1.1
    *       but there is not such an Interface in gimp 1.0.2
    */
  static char     *l_get_linked_proc = "gimp_layer_get_linked";
    
  int             l_nparams;
  int             l_nreturn_vals;
  int             l_proc_type;
  char            *l_proc_blurb;
  char            *l_proc_help;
  char            *l_proc_author;
  char            *l_proc_copyright;
  char            *l_proc_date;
  GParamDef       *l_params;
  GParamDef       *l_return_vals;
  gint             l_rc;

  GParam *return_vals;
  int nreturn_vals;
  gint32 is_linked;

  l_rc = 0;
  
  /* Query the gimp application's procedural database
   *  regarding a particular procedure.
   */
  if(gimp_query_procedure  (l_get_linked_proc,
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
     /* check if it can take exactly one layerid as input
      * and give one result int32 parameter (TRUE/FALSE)
      */
     if (l_nparams  != 1)                       { l_rc = -1;  }
     if (l_params[0].type != PARAM_LAYER)       { l_rc = -1;  }
     if (l_nreturn_vals  != 1)                  { l_rc = -1;  }
     if (l_return_vals[0].type != PARAM_INT32)  { l_rc = -1;  }

     /*  free the query information  */
     g_free (l_proc_blurb);
     g_free (l_proc_help);
     g_free (l_proc_author);
     g_free (l_proc_copyright);
     g_free (l_proc_date);
     g_free (l_params);
     g_free (l_return_vals);
     
     
     if(l_rc != 0)
     {
        printf("Warning: Procedure %s has unexpected Interface. (Can not operate on linked layers)\n",l_get_linked_proc);
        printf("expected: 1, 1, PARAM_LAYER = %d  PARAM_INT32 = %d\n", (int)PARAM_LAYER, (int)PARAM_INT32);
        printf("l_nparams = %d\n", (int)l_nparams);
        printf("l_nreturn_vals = %d\n", (int)l_nreturn_vals);
        printf("l_params[0].type = %d\n", (int)l_params[0].type);
        printf("l_return_vals[0].type = %d\n", (int)l_return_vals[0].type);
        return FALSE;
     }
  }
  else
  {
     printf("Warning: Procedure %s not found. (Can not operate on linked layers)\n",l_get_linked_proc);
     return FALSE;
  }
  
  /* run the procedure */
  is_linked = FALSE;

  return_vals = gimp_run_procedure (l_get_linked_proc,
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
   static char     *l_fsel_attached_to_proc = "gimp_image_floating_sel_attached_to";
   GParam          *return_vals;
   int              nreturn_vals;

   if (p_pdb_procedure_available(l_fsel_attached_to_proc) >= 0)
   {
      return_vals = gimp_run_procedure (l_fsel_attached_to_proc,
                                    &nreturn_vals,
                                    PARAM_IMAGE, image_id,
                                    PARAM_END);
                                    
      if (return_vals[0].data.d_status == STATUS_SUCCESS)
      {
         return(return_vals[1].data.d_drawable);
      }
      printf("GAP: Error: PDB call of %s failed\n", l_fsel_attached_to_proc);
   }
   else
   {
      printf("GAP: Warning: Procedure %s not found. %s\n",
             l_fsel_attached_to_proc,
             "(cant find out drawable where f-sel is attached to)");
   }
   return(-1);
}	/* end p_gimp_image_floating_sel_attached_to */

/* ============================================================================
 * p_gimp_floating_sel_attach
 *   
 * ============================================================================
 */

gint   p_gimp_floating_sel_attach(gint32 layer_id, gint32 drawable_id)
{
   static char     *l_fsel_attach_proc = "gimp_floating_sel_attach";
   GParam          *return_vals;
   int              nreturn_vals;

   if (p_pdb_procedure_available(l_fsel_attach_proc) >= 0)
   {
      return_vals = gimp_run_procedure (l_fsel_attach_proc,
                                    &nreturn_vals,
                                    PARAM_LAYER,    layer_id,
                                    PARAM_DRAWABLE, drawable_id,
                                    PARAM_END);
                                    
      if (return_vals[0].data.d_status == STATUS_SUCCESS)
      {
         return (0);
      }
      printf("GAP: Error: PDB call of %s failed\n", l_fsel_attach_proc);
   }
   else
   {
      printf("GAP: Warning: Procedure %s not found. %s\n",
                          l_fsel_attach_proc,
			  "(cannot attach floating selection)");
   }
   return(-1);
}	/* end p_gimp_floating_sel_attach */

/* ============================================================================
 * p_gimp_floating_sel_rigor
 *   
 * ============================================================================
 */

gint   p_gimp_floating_sel_rigor(gint32 layer_id, gint32 undo)
{
   static char     *l_fsel_rigor_proc = "gimp_floating_sel_rigor";
   GParam          *return_vals;
   int              nreturn_vals;

   if (p_pdb_procedure_available(l_fsel_rigor_proc) >= 0)
   {
      return_vals = gimp_run_procedure (l_fsel_rigor_proc,
                                    &nreturn_vals,
                                    PARAM_LAYER, layer_id,
                                    PARAM_INT32, undo,
                                    PARAM_END);
                                    
      if (return_vals[0].data.d_status == STATUS_SUCCESS)
      {
         return (0);
      }
      printf("GAP: Error: PDB call of %s failed\n", l_fsel_rigor_proc);
   }
   else
   {
      printf("GAP: Warning: Procedure %s not found. %s\n",
                          l_fsel_rigor_proc,
			  "(cannot attach floating selection)");
   }
   return(-1);
}	/* end p_gimp_floating_sel_rigor */

/* ============================================================================
 * p_gimp_floating_sel_relax
 *   
 * ============================================================================
 */

gint   p_gimp_floating_sel_relax(gint32 layer_id, gint32 undo)
{
   static char     *l_fsel_relax_proc = "gimp_floating_sel_relax";
   GParam          *return_vals;
   int              nreturn_vals;

   if (p_pdb_procedure_available(l_fsel_relax_proc) >= 0)
   {
      return_vals = gimp_run_procedure (l_fsel_relax_proc,
                                    &nreturn_vals,
                                    PARAM_LAYER, layer_id,
                                    PARAM_INT32, undo,
                                    PARAM_END);
                                    
      if (return_vals[0].data.d_status == STATUS_SUCCESS)
      {
         return (0);
      }
      printf("GAP: Error: PDB call of %s failed\n", l_fsel_relax_proc);
   }
   else
   {
      printf("GAP: Warning: Procedure %s not found. %s\n",
                          l_fsel_relax_proc,
			  "(cannot attach floating selection)");
   }
   return(-1);
}	/* end p_gimp_floating_sel_relax */




/* ============================================================================
 * p_gimp_image_add_guide
 *   
 * ============================================================================
 */

gint32   p_gimp_image_add_guide(gint32 image_id, gint32 position, gint32 orientation)
{
   static char     *l_add_guide_proc;
   GParam          *return_vals;
   int              nreturn_vals;

   if(orientation == 0 )  /* in GIMP 1.1 we could use (orientation == ORIENTATION_VERTICAL) */
   {
      l_add_guide_proc = "gimp_image_add_vguide";
   }
   else
   {
      l_add_guide_proc = "gimp_image_add_hguide";
   }



   if (p_pdb_procedure_available(l_add_guide_proc) >= 0)
   {
      return_vals = gimp_run_procedure (l_add_guide_proc,
                                    &nreturn_vals,
                                    PARAM_IMAGE, image_id,
                                    PARAM_INT32, position,
                                    PARAM_END);
                                    
      if (return_vals[0].data.d_status == STATUS_SUCCESS)
      {
         return(return_vals[1].data.d_int32);  /* return the guide ID */
      }
      printf("GAP: Error: PDB call of %s failed\n", l_add_guide_proc);
   }
   else
   {
      printf("GAP: Warning: Procedure %s not found. %s\n",
                          l_add_guide_proc,
			  "(cannot add guide)");
   }
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
   static char     *l_findnext_guide_proc = "gimp_image_findnext_guide";
   GParam          *return_vals;
   int              nreturn_vals;

   if (p_pdb_procedure_available(l_findnext_guide_proc) >= 0)
   {
      return_vals = gimp_run_procedure (l_findnext_guide_proc,
                                    &nreturn_vals,
                                    PARAM_IMAGE, image_id,
                                    PARAM_INT32, guide_id,
                                    PARAM_END);
                                    
      if (return_vals[0].data.d_status == STATUS_SUCCESS)
      {
         return(return_vals[1].data.d_int32);  /* return the next guide ID */
      }
      printf("GAP: Error: PDB call of %s failed\n", l_findnext_guide_proc);
   }
   else
   {
      printf("GAP: Warning: Procedure %s not found. %s\n",
                          l_findnext_guide_proc,
			  "(if image has guides they are not saved)");
   }
   return(-1);
}	/* end p_gimp_image_findnext_guide */



/* ============================================================================
 * p_gimp_image_get_guide_position
 *
 * ============================================================================
 */

gint32   p_gimp_image_get_guide_position(gint32 image_id, gint32 guide_id)
{
   static char     *l_get_guide_pos_proc = "gimp_image_get_guide_position";
   GParam          *return_vals;
   int              nreturn_vals;

   if (p_pdb_procedure_available(l_get_guide_pos_proc) >= 0)
   {
      return_vals = gimp_run_procedure (l_get_guide_pos_proc,
                                    &nreturn_vals,
                                    PARAM_IMAGE, image_id,
                                    PARAM_INT32, guide_id,
                                    PARAM_END);
                                    
      if (return_vals[0].data.d_status == STATUS_SUCCESS)
      {
         return(return_vals[1].data.d_int32);  /* return the guide position */
      }
      printf("GAP: Error: PDB call of %s failed\n", l_get_guide_pos_proc);
   }
   else
   {
      printf("GAP: Warning: Procedure %s not found. %s\n",
                          l_get_guide_pos_proc,
			  "(cannot save guides)");
   }
   return(-1);
}	/* end p_gimp_image_get_guide_position */


/* ============================================================================
 * p_gimp_image_get_guide_orientation
 *
 * ============================================================================
 */

gint32   p_gimp_image_get_guide_orientation(gint32 image_id, gint32 guide_id)
{
   static char     *l_get_guide_pos_orient = "gimp_image_get_guide_orientation";
   GParam          *return_vals;
   int              nreturn_vals;

   if (p_pdb_procedure_available(l_get_guide_pos_orient) >= 0)
   {
      return_vals = gimp_run_procedure (l_get_guide_pos_orient,
                                    &nreturn_vals,
                                    PARAM_IMAGE, image_id,
                                    PARAM_INT32, guide_id,
                                    PARAM_END);
                                    
      if (return_vals[0].data.d_status == STATUS_SUCCESS)
      {
         return(return_vals[1].data.d_int32);  /* return the guide orientation */
      }
      printf("GAP: Error: PDB call of %s failed\n", l_get_guide_pos_orient);
   }
   else
   {
      printf("GAP: Warning: Procedure %s not found. %s\n",
                          l_get_guide_pos_orient,
			  "(cannot save guides)");
   }
   return(-1);
}	/* end p_gimp_image_get_guide_orientation */


/* ============================================================================
 * p_gimp_image_delete_guide
 *   
 * ============================================================================
 */

gint32   p_gimp_image_delete_guide(gint32 image_id, gint32 guide_id)
{
   static char     *l_delete_guide_proc = "gimp_image_delete_guide";
   GParam          *return_vals;
   int              nreturn_vals;

   if (p_pdb_procedure_available(l_delete_guide_proc) >= 0)
   {
      return_vals = gimp_run_procedure (l_delete_guide_proc,
                                    &nreturn_vals,
                                    PARAM_IMAGE, image_id,
                                    PARAM_INT32, guide_id,
                                    PARAM_END);
                                    
      if (return_vals[0].data.d_status == STATUS_SUCCESS)
      {
         return(return_vals[1].data.d_int32);  /* return the next guide ID */
      }
      printf("GAP: Error: PDB call of %s failed\n", l_delete_guide_proc);
   }
   else
   {
      printf("GAP: Warning: Procedure %s not found. %s\n",
                          l_delete_guide_proc,
			  "(cant remove old guides)");
   }
   return(-1);
}	/* end p_gimp_image_delete_guide */


/* ============================================================================
 * p_gimp_selection_none
 *   
 * ============================================================================
 */

gint   p_gimp_selection_none(gint32 image_id)
{
   static char     *l_sel_none_proc = "gimp_selection_none";
   GParam          *return_vals;
   int              nreturn_vals;

   if (p_pdb_procedure_available(l_sel_none_proc) >= 0)
   {
      return_vals = gimp_run_procedure (l_sel_none_proc,
                                    &nreturn_vals,
                                    PARAM_IMAGE,    image_id,
                                    PARAM_END);
                                    
      if (return_vals[0].data.d_status == STATUS_SUCCESS)
      {
         return (0);
      }
      printf("GAP: Error: PDB call of %s failed\n", l_sel_none_proc);
   }
   else
   {
      printf("GAP: Warning: Procedure %s not found. %s\n",
                          l_sel_none_proc,
			  "(cannot remove selection)");
   }
   return(-1);
}	/* end p_gimp_selection_none */



/* ============================================================================
 * p_gimp_rotate
 *  PDB call of 'gimp_rotate'
 * ============================================================================
 */

gint p_gimp_rotate(gint32 image_id, gint32 drawable_id, gint32 interpolation, gdouble angle_deg)
{
   static char     *l_rotate_proc = "gimp_rotate";
   GParam          *return_vals;
   int              nreturn_vals;
   gdouble          l_angle_rad;
   int              l_nparams;
   int              l_rc;

   l_rc = -1;
   l_angle_rad = (angle_deg * 3.14159) / 180.0;

   l_nparams = p_pdb_procedure_available(l_rotate_proc);
   if (l_nparams >= 0)
   {
     if (l_nparams == 3)
     {
         /* use the new Interface (Gimp 1.1 style)
          * (1.1 knows the image_id where the drawable belongs to)
          */
        return_vals = gimp_run_procedure (l_rotate_proc,
                                          &nreturn_vals,
                                          PARAM_DRAWABLE, drawable_id,
			                  PARAM_INT32, interpolation,
			                  PARAM_FLOAT, l_angle_rad,
                                          PARAM_END);
     }
     else
     {
         /* use the old Interface (Gimp 1.0.2 style) */
        return_vals = gimp_run_procedure (l_rotate_proc,
                                          &nreturn_vals,
			                  PARAM_IMAGE, image_id,
                                          PARAM_DRAWABLE, drawable_id,
			                  PARAM_INT32, interpolation,
			                  PARAM_FLOAT, l_angle_rad,
                                          PARAM_END);
     }
     if (return_vals[0].data.d_status == STATUS_SUCCESS)
     {
        l_rc = 0;
     }
     else
     {
       printf("gap: %s call failed %d\n", l_rotate_proc, (int)return_vals[0].data.d_status);
     }

     gimp_destroy_params (return_vals, nreturn_vals);
   }
   else
   {
      printf("GAP: Error: Procedure %s not found.\n",l_rotate_proc);
   }
   return (l_rc);

}  /* end p_gimp_rotate */


/* ============================================================================
 * p_gimp_channel_ops_duplicate
 *   call gimp_channel_ops_duplicate via procedural database
 *   (i could not find a direct call interface in gimp0.99.15)
 * ============================================================================
 */
gint32      p_gimp_channel_ops_duplicate  (gint32     image_ID)
{
  GParam *return_vals;
  int nreturn_vals;
  gint32 new_image_ID;

  return_vals = gimp_run_procedure ("gimp_channel_ops_duplicate",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);

  new_image_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    new_image_ID = return_vals[1].data.d_image;

  gimp_destroy_params (return_vals, nreturn_vals);

  return new_image_ID;
}	/* end p_gimp_channel_ops_duplicate */





/* ============================================================================
 * p_gimp_drawable_set_image
 *   
 * ============================================================================
 */

gint   p_gimp_drawable_set_image(gint32 drawable_id, gint32 image_id)
{
   static char     *l_drawable_set_img_proc = "gimp_drawable_set_image";
   GParam          *return_vals;
   int              nreturn_vals;

   if (p_pdb_procedure_available(l_drawable_set_img_proc) >= 0)
   {
      return_vals = gimp_run_procedure (l_drawable_set_img_proc,
                                    &nreturn_vals,
                                    PARAM_DRAWABLE,  drawable_id,
                                    PARAM_IMAGE,     image_id,
                                    PARAM_END);
                                    
      if (return_vals[0].data.d_status == STATUS_SUCCESS)
      {
         return (0);
      }
      printf("GAP: Error: PDB call of %s failed\n", l_drawable_set_img_proc);
   }
   else
   {
      printf("GAP: Warning: Procedure %s not found. %s\n",
                          l_drawable_set_img_proc,
			  "(cannot attach floating selection)");
   }
   return(-1);
}	/* end p_gimp_drawable_set_image */

