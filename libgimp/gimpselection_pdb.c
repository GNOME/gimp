/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpselection_pdb.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "gimp.h"


gint32
gimp_selection_bounds (gint32  image_ID,
		       gint   *non_empty,
		       gint   *x1,
		       gint   *y1,
		       gint   *x2,
		       gint   *y2)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint result;

  return_vals = gimp_run_procedure ("gimp_selection_bounds",
                                    &nreturn_vals,
                                    PARAM_IMAGE, image_ID,
                                    PARAM_END);

  result = FALSE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      result = TRUE;
      *non_empty = return_vals[1].data.d_int32; 
      *x1 = return_vals[2].data.d_int32;
      *y1 = return_vals[3].data.d_int32;
      *x2 = return_vals[4].data.d_int32;
      *y2 = return_vals[5].data.d_int32;
    }
  
  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}

gint32
gimp_selection_float (gint32 image_ID, 
		      gint32 drawable_ID,
		      gint32 x_offset,
		      gint32 y_offset)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 layer_ID;

  return_vals = gimp_run_procedure ("gimp_selection_float",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_DRAWABLE, drawable_ID, 
				    PARAM_INT32, x_offset, 
				    PARAM_INT32, y_offset,
				    PARAM_END);

  layer_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    layer_ID = return_vals[1].data.d_layer;

  gimp_destroy_params (return_vals, nreturn_vals);

  return layer_ID;
}

gboolean
gimp_selection_is_empty (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gboolean is_empty;
  
  return_vals = gimp_run_procedure ("gimp_selection_is_empty",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_END);
  is_empty = TRUE;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    is_empty = return_vals[1].data.d_int32 ? TRUE : FALSE;

  gimp_destroy_params (return_vals, nreturn_vals);

  return is_empty;
}

void
gimp_selection_none (gint32 image_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_selection_none",
                                    &nreturn_vals,
                                    PARAM_IMAGE, image_ID,
                                    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}
