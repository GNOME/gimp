/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpguides_pdb.c
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
gimp_image_add_hguide (gint32 image_ID,
		       gint32 yposition)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 guide_ID;

  return_vals = gimp_run_procedure ("gimp_image_add_hguide",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_INT32, yposition,
				    PARAM_END);

  guide_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    guide_ID = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return guide_ID;
}

gint32
gimp_image_add_vguide (gint32 image_ID,
		       gint32 xposition)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 guide_ID;

  return_vals = gimp_run_procedure ("gimp_image_add_vguide",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_INT32, xposition,
				    PARAM_END);

  guide_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    guide_ID = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return guide_ID;
}

void
gimp_image_delete_guide (gint32 image_ID,
			 gint32 guide_ID)
{
  GParam *return_vals;
  gint nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_image_delete_guide",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_INT32, guide_ID,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

gint32
gimp_image_find_next_guide (gint32 image_ID,
			    gint32 guide_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 rtn_guide_ID;

  return_vals = gimp_run_procedure ("gimp_image_find_next_guide",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_INT32, guide_ID,
				    PARAM_END);

  rtn_guide_ID = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    rtn_guide_ID = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return rtn_guide_ID;
}

GOrientation
gimp_image_get_guide_orientation (gint32 image_ID,
				  gint32 guide_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  GOrientation guide_orientation;

  return_vals = gimp_run_procedure ("gimp_image_get_guide_orientation",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_INT32, guide_ID,
				    PARAM_END);

  guide_orientation = ORIENTATION_UNKNOWN;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    guide_orientation = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return guide_orientation;
}

gint32
gimp_image_get_guide_position (gint32 image_ID,
			       gint32 guide_ID)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint32 guide_position;

  return_vals = gimp_run_procedure ("gimp_image_get_guide_position",
				    &nreturn_vals,
				    PARAM_IMAGE, image_ID,
				    PARAM_INT32, guide_ID,
				    PARAM_END);

  guide_position = -1;
  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    guide_position = return_vals[1].data.d_int32;

  gimp_destroy_params (return_vals, nreturn_vals);

  return guide_position;
}
