/* gap_pdb_calls.c
 *
 * this module contains wraper calls of procedures in the GIMPs Procedural Database
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
 * version 1.1.16a; 2000/02/05  hof: path lockedstaus
 * version 1.1.15b; 2000/01/30  hof: image parasites
 * version 1.1.15a; 2000/01/26  hof: pathes
 *                                   removed old gimp 1.0.x PDB Interfaces
 * version 1.1.14a; 2000/01/09  hof: thumbnail save/load,
 *                              Procedures for video_info file
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
    
  gint             l_nparams;
  gint             l_nreturn_vals;
  GimpPDBProcType   l_proc_type;
  gchar            *l_proc_blurb;
  gchar            *l_proc_help;
  gchar            *l_proc_author;
  gchar            *l_proc_copyright;
  gchar            *l_proc_date;
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
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_get_sel_bounds_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_IMAGE, image_id,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      *x1 = return_vals[2].data.d_int32;
      *y1 = return_vals[3].data.d_int32;
      *x2 = return_vals[4].data.d_int32;
      *y2 = return_vals[5].data.d_int32;
      return(return_vals[1].data.d_int32);
   }
   printf("GAP: Error: PDB call of %s failed staus=%d\n", 
          l_get_sel_bounds_proc, (int)return_vals[0].data.d_status);
   return(FALSE);
}	/* end p_get_gimp_selection_bounds */

/* ============================================================================
 * p_gimp_selection_load
 *   
 * ============================================================================
 */
gint
p_gimp_selection_load (gint32 channel_id)
{
   static char     *l_sel_load = "gimp_selection_load";
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_sel_load,
                                 &nreturn_vals,
                                 GIMP_PDB_CHANNEL, channel_id,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return(TRUE);
   }
   printf("GAP: Error: PDB call of %s failed status=%d\n", 
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
   static char     *l_set_linked_proc = "gimp_layer_set_linked";
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_set_linked_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_LAYER, layer_id,
                                 GIMP_PDB_INT32, new_state,  /* TRUE or FALSE */
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return (0);
   }
   printf("GAP: Error: p_layer_set_linked to state %d failed\n",(int)new_state);
   return(-1);
}	/* end p_layer_set_linked */

/* ============================================================================
 * p_layer_get_linked
 *   
 * ============================================================================
 */

gint p_layer_get_linked(gint32 layer_id)
{
  static char     *l_get_linked_proc = "gimp_layer_get_linked";
  GimpParam *return_vals;
  int nreturn_vals;
  gint32 is_linked;

  is_linked = FALSE;
  return_vals = gimp_run_procedure (l_get_linked_proc,
                                    &nreturn_vals,
                                    GIMP_PDB_LAYER, layer_id,
                                    GIMP_PDB_END);

  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
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
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_fsel_attached_to_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_IMAGE, image_id,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return(return_vals[1].data.d_drawable);
   }
   printf("GAP: Error: PDB call of %s failed\n", l_fsel_attached_to_proc);
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
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_fsel_attach_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_LAYER,    layer_id,
                                 GIMP_PDB_DRAWABLE, drawable_id,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return (0);
   }
   printf("GAP: Error: PDB call of %s failed\n", l_fsel_attach_proc);
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
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_fsel_rigor_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_LAYER, layer_id,
                                 GIMP_PDB_INT32, undo,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return (0);
   }
   printf("GAP: Error: PDB call of %s failed\n", l_fsel_rigor_proc);
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
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_fsel_relax_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_LAYER, layer_id,
                                 GIMP_PDB_INT32, undo,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return (0);
   }
   printf("GAP: Error: PDB call of %s failed\n", l_fsel_relax_proc);
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
   GimpParam          *return_vals;
   int              nreturn_vals;

   if (orientation == GIMP_VERTICAL)
   {
     l_add_guide_proc = "gimp_image_add_vguide";
   }
   else
   {
     l_add_guide_proc = "gimp_image_add_hguide";
   }

   return_vals = gimp_run_procedure (l_add_guide_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_IMAGE, image_id,
                                 GIMP_PDB_INT32, position,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return(return_vals[1].data.d_int32);  /* return the guide ID */
   }
   printf("GAP: Error: PDB call of %s failed\n", l_add_guide_proc);
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
   static char     *l_findnext_guide_proc = "gimp_image_find_next_guide";
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_findnext_guide_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_IMAGE, image_id,
                                 GIMP_PDB_INT32, guide_id,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return(return_vals[1].data.d_int32);  /* return the next guide ID */
   }
   printf("GAP: Error: PDB call of %s failed\n", l_findnext_guide_proc);
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
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_get_guide_pos_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_IMAGE, image_id,
                                 GIMP_PDB_INT32, guide_id,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return(return_vals[1].data.d_int32);  /* return the guide position */
   }
   printf("GAP: Error: PDB call of %s failed\n", l_get_guide_pos_proc);
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
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_get_guide_pos_orient,
                                 &nreturn_vals,
                                 GIMP_PDB_IMAGE, image_id,
                                 GIMP_PDB_INT32, guide_id,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return(return_vals[1].data.d_int32);  /* return the guide orientation */
   }
   printf("GAP: Error: PDB call of %s failed\n", l_get_guide_pos_orient);
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
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_delete_guide_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_IMAGE, image_id,
                                 GIMP_PDB_INT32, guide_id,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return(return_vals[1].data.d_int32);  /* return the next guide ID */
   }
   printf("GAP: Error: PDB call of %s failed\n", l_delete_guide_proc);
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
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_sel_none_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_IMAGE,    image_id,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return (0);
   }
   printf("GAP: Error: PDB call of %s failed\n", l_sel_none_proc);
   return(-1);
}	/* end p_gimp_selection_none */


/* ============================================================================
 * p_gimp_rotate
 *  PDB call of 'gimp_rotate'
 * ============================================================================
 */

gint p_gimp_rotate(gint32 drawable_id, gint32 interpolation, gdouble angle_deg)
{
   static char     *l_rotate_proc = "gimp_rotate";
   GimpParam          *return_vals;
   int              nreturn_vals;
   gdouble          l_angle_rad;

   l_angle_rad = (angle_deg * 3.14159) / 180.0;

   return_vals = gimp_run_procedure (l_rotate_proc,
                                          &nreturn_vals,
                                          GIMP_PDB_DRAWABLE, drawable_id,
			                  GIMP_PDB_INT32, interpolation,
			                  GIMP_PDB_FLOAT, l_angle_rad,
                                          GIMP_PDB_END);
   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return(0);
   }
   return (-1);

}  /* end p_gimp_rotate */


/* ============================================================================
 * p_gimp_channel_ops_duplicate
 * ============================================================================
 */
gint32      p_gimp_channel_ops_duplicate  (gint32     image_ID)
{
  GimpParam *return_vals;
  int nreturn_vals;
  gint32 new_image_ID;

  return_vals = gimp_run_procedure ("gimp_channel_ops_duplicate",
				    &nreturn_vals,
				    GIMP_PDB_IMAGE, image_ID,
				    GIMP_PDB_END);

  new_image_ID = -1;
  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
  {
    new_image_ID = return_vals[1].data.d_image;
  }
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
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_drawable_set_img_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_DRAWABLE,  drawable_id,
                                 GIMP_PDB_IMAGE,     image_id,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return (0);
   }
   printf("GAP: Error: PDB call of %s failed\n", l_drawable_set_img_proc);
   return(-1);
}	/* end p_gimp_drawable_set_image */

/* ============================================================================
 * p_gimp_gimprc_query
 *   
 * ============================================================================
 */
char*
p_gimp_gimprc_query(char *key)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  char *value;

  return_vals = gimp_run_procedure ("gimp_gimprc_query",
				    &nreturn_vals,
				    GIMP_PDB_STRING, key,
				    GIMP_PDB_END);

  value = NULL;
  if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
  {
      if(return_vals[1].data.d_string != NULL)
      {
        value = g_strdup(return_vals[1].data.d_string);
      }
  }
  gimp_destroy_params (return_vals, nreturn_vals);

  return(value);
}


/* ============================================================================
 * p_gimp_file_save_thumbnail
 *   
 * ============================================================================
 */

gint
p_gimp_file_save_thumbnail(gint32 image_id, char* filename)
{
   static char     *l_called_proc = "gimp_file_save_thumbnail";
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_IMAGE,     image_id,
				 GIMP_PDB_STRING,    filename,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return (0);
   }
   printf("GAP: Error: PDB call of %s failed\n", l_called_proc);
   return(-1);
}	/* end p_gimp_file_save_thumbnail */

/* ============================================================================
 * p_gimp_file_load_thumbnail
 *   
 * ============================================================================
 */

gint
p_gimp_file_load_thumbnail(char* filename, gint32 *th_width, gint32 *th_height,
                           gint32 *th_data_count,  unsigned char **th_data)
{
   static char     *l_called_proc = "gimp_file_load_thumbnail";
   GimpParam          *return_vals;
   int              nreturn_vals;

   *th_data = NULL;
   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
 				 GIMP_PDB_STRING,    filename,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      *th_width = return_vals[1].data.d_int32;
      *th_height = return_vals[2].data.d_int32;
      *th_data_count = return_vals[3].data.d_int32;
      *th_data = (unsigned char *)return_vals[4].data.d_int8array;
      return (0); /* OK */
   }
   printf("GAP: Error: PDB call of %s failed\n", l_called_proc);
   return(-1);
}	/* end p_gimp_file_load_thumbnail */



gint   p_gimp_image_thumbnail(gint32 image_id, gint32 width, gint32 height,
                              gint32 *th_width, gint32 *th_height, gint32 *th_bpp,
			      gint32 *th_data_count, unsigned char **th_data)
{
   static char     *l_called_proc = "gimp_image_thumbnail";
   GimpParam          *return_vals;
   int              nreturn_vals;

   *th_data = NULL;
   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
 				 GIMP_PDB_IMAGE,    image_id,
 				 GIMP_PDB_INT32,    width,
 				 GIMP_PDB_INT32,    height,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      *th_width  = return_vals[1].data.d_int32;
      *th_height = return_vals[2].data.d_int32;
      *th_bpp    = return_vals[3].data.d_int32;
      *th_data_count = return_vals[4].data.d_int32;
      *th_data = (unsigned char *)return_vals[5].data.d_int8array;
      return(0); /* OK */
   }
   printf("GAP: Error: PDB call of %s failed\n", l_called_proc);
   return(-1);
}	/* end p_gimp_image_thumbnail */


/* ============================================================================
 * p_gimp_path_set_points
 *   
 * ============================================================================
 */

gint
p_gimp_path_set_points(gint32 image_id, char *name,
                       gint32 path_type, gint32 num_points, gdouble *path_points)
{
   static char     *l_called_proc = "gimp_path_set_points";
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_IMAGE,  image_id,
                                 GIMP_PDB_STRING, name,
                                 GIMP_PDB_INT32,  path_type,
                                 GIMP_PDB_INT32,  num_points,
                                 GIMP_PDB_FLOATARRAY, path_points,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return(0);  /* OK */
   }
   printf("GAP: Error: PDB call of %s failed\n", l_called_proc);
   return(-1);
}	/* end p_gimp_path_set_points */



/* ============================================================================
 * p_gimp_path_get_points
 *   
 * ============================================================================
 */
gdouble *
p_gimp_path_get_points(gint32 image_id, char *name,
                       gint32 *path_type, gint32 *path_closed, gint32 *num_points)
{
   static char     *l_called_proc = "gimp_path_get_points";
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_IMAGE, image_id,
                                 GIMP_PDB_STRING, name,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      *path_type = return_vals[1].data.d_int32;
      *path_closed = return_vals[2].data.d_int32;
      *num_points = return_vals[3].data.d_int32;
      return(return_vals[4].data.d_floatarray); /* OK return path points */
   }
   printf("GAP: Error: PDB call of %s failed\n", l_called_proc);
   *num_points = 0;
   return(NULL);
}	/* end p_gimp_path_get_points */

/* ============================================================================
 * p_gimp_path_delete
 *   
 * ============================================================================
 */

gint
p_gimp_path_delete(gint32 image_id, char *name)
{
   static char     *l_called_proc = "gimp_path_delete";
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_IMAGE, image_id,
                                 GIMP_PDB_STRING, name,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return (0);
   }
   printf("GAP: Error: PDB call of %s failed\n", l_called_proc);
   return(-1);
}	/* end p_gimp_path_delete */


/* ============================================================================
 * p_gimp_path_list
 *   
 * ============================================================================
 */

char **
p_gimp_path_list(gint32 image_id, gint32 *num_paths)
{
   static char     *l_called_proc = "gimp_path_list";
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_IMAGE, image_id,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
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

char *
p_gimp_path_get_current(gint32 image_id)
{
   static char     *l_called_proc = "gimp_path_get_current";
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_IMAGE, image_id,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return(g_strdup(return_vals[1].data.d_string)); /* OK */
   }
   printf("GAP: Error: PDB call of %s failed\n", l_called_proc);
   return(NULL);
}	/* end p_gimp_path_get_current */

/* ============================================================================
 * p_gimp_path_set_current
 *   
 * ============================================================================
 */

gint
p_gimp_path_set_current(gint32 image_id, char *name)
{
   static char     *l_called_proc = "gimp_path_set_current";
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_IMAGE, image_id,
                                 GIMP_PDB_STRING, name,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return(0); /* OK */
   }
   printf("GAP: Error: PDB call of %s failed\n", l_called_proc);
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
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_IMAGE, image_id,
                                 GIMP_PDB_STRING, name,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return(return_vals[1].data.d_int32); /* OK */
   }
   printf("GAP: Error: PDB call of %s failed\n", l_called_proc);
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
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_called_proc,
                                 &nreturn_vals,
                                 GIMP_PDB_IMAGE, image_id,
                                 GIMP_PDB_STRING, name,
                                 GIMP_PDB_INT32, lockstatus,
                                 GIMP_PDB_END);

   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      return(0); /* OK */
   }
   printf("GAP: Error: PDB call of %s failed\n", l_called_proc);
   return(-1);
}	/* end p_gimp_path_set_locked */

/* ============================================================================
 * p_gimp_image_parasite_list
 * ============================================================================
 */

gchar **
p_gimp_image_parasite_list (gint32 image_id, gint32 *num_parasites)
{
   static gchar    *l_procname = "gimp_image_parasite_list";
   GimpParam          *return_vals;
   int              nreturn_vals;

   return_vals = gimp_run_procedure (l_procname,
                                    &nreturn_vals,
                                    GIMP_PDB_IMAGE,  image_id,
                                    GIMP_PDB_END);
                                    
   if (return_vals[0].data.d_status == GIMP_PDB_SUCCESS)
   {
      *num_parasites = return_vals[1].data.d_int32;
      return(return_vals[2].data.d_stringarray);    /* OK, return name list */
   }
   printf("GAP: Error: PDB call of %s failed\n", l_procname);

   *num_parasites = 0;
   return(NULL);
}	/* end p_gimp_image_parasite_list */


/* ============================================================================
 * Procedures to get/set the video_info_file 
 * ============================================================================
 */

char *
p_alloc_video_info_name(char *basename)
{
  int   l_len;
  char *l_str;

  if(basename == NULL)
  {
    return(NULL);
  }

  l_str = g_strdup_printf("%svin.gap", basename);
  return(l_str);
}

int
p_set_video_info(t_video_info *vin_ptr, char *basename)
{
  FILE *l_fp;
  char  *l_vin_filename;
  int   l_rc;
    
  l_rc = -1;
  l_vin_filename = p_alloc_video_info_name(basename);
  if(l_vin_filename)
  {
      l_fp = fopen(l_vin_filename, "w");
      if(l_fp)
      {
         fprintf(l_fp, "# GIMP / GAP Videoinfo file\n");
         fprintf(l_fp, "(framerate %f) # 1.0 upto 100.0 frames per sec\n", (float)vin_ptr->framerate);
         fprintf(l_fp, "(timezoom %d)  # 1 upto 100 frames\n", (int)vin_ptr->timezoom );
         fclose(l_fp);
         l_rc = 0;
       }
       g_free(l_vin_filename);
  }
  return(l_rc);
}

t_video_info *
p_get_video_info(char *basename)
{
  FILE *l_fp;
  char  *l_vin_filename;
  t_video_info *l_vin_ptr;
  char         l_buf[4000];
  int   l_len;
  
  l_vin_ptr = g_malloc(sizeof(t_video_info));
  l_vin_ptr->timezoom = 1;
  l_vin_ptr->framerate = 24.0;
  
  l_vin_filename = p_alloc_video_info_name(basename);
  if(l_vin_filename)
  {
     l_fp = fopen(l_vin_filename, "r");
     if(l_fp)
     {
       while(NULL != fgets(l_buf, 4000-1, l_fp))
       {
 	  l_len = strlen("(framerate ");
	  if(strncmp(l_buf, "(framerate ", l_len) == 0)
	  {
	     l_vin_ptr->framerate = atof(&l_buf[l_len]);
	  }
	  l_len = strlen("(timezoom ");
	  if (strncmp(l_buf, "(timezoom ", l_len ) == 0)
	  {
	     l_vin_ptr->timezoom = atol(&l_buf[l_len]);
	  }
       }
       fclose(l_fp);
     }
     g_free(l_vin_filename);
  }
  return(l_vin_ptr);
}
