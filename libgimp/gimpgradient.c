/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */                                                                             
#include "gimp.h"


char **
gimp_gradients_get_list(gint *num_gradients)
{
  GParam  *return_vals;
  int      nreturn_vals;
  char   **gradient_names;
  int      i;

  return_vals = gimp_run_procedure ("gimp_gradients_get_list",
				    &nreturn_vals,
				    PARAM_END);
  *num_gradients = 0;
  gradient_names = NULL;

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *num_gradients = return_vals[1].data.d_int32;
      gradient_names = g_new (char *, *num_gradients);
      for (i = 0; i < *num_gradients; i++)
	gradient_names[i] = g_strdup (return_vals[2].data.d_stringarray[i]);
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return gradient_names;
}


char *
gimp_gradients_get_active(void)
{
  GParam *return_vals;
  int     nreturn_vals;
  char   *result;

  return_vals = gimp_run_procedure ("gimp_gradients_get_active",
				    &nreturn_vals,
				    PARAM_END);

  result = NULL;

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    result = g_strdup (return_vals[1].data.d_string);

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}


void
gimp_gradients_set_active(char *name)
{
  GParam *return_vals;
  int     nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_gradients_set_active",
				    &nreturn_vals,
				    PARAM_STRING, name,
				    PARAM_END);
  gimp_destroy_params (return_vals, nreturn_vals);
}


gdouble *
gimp_gradients_sample_uniform(gint num_samples)
{
  GParam  *return_vals;
  int      nreturn_vals;
  gdouble *result;
  int      nresult;
  int      i;

  return_vals = gimp_run_procedure ("gimp_gradients_sample_uniform",
				    &nreturn_vals,
				    PARAM_INT32, num_samples,
				    PARAM_END);
  
  result = NULL;

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      nresult = return_vals[1].data.d_int32;
      result  = g_new(gdouble, nresult);
      for (i = 0; i < nresult; i++)
	result[i] = return_vals[2].data.d_floatarray[i];
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}


gdouble *
gimp_gradients_sample_custom(gint num_samples, gdouble *positions)
{
  GParam  *return_vals;
  int      nreturn_vals;
  gdouble *result;
  int      nresult;
  int      i;

  return_vals = gimp_run_procedure ("gimp_gradients_sample_custom",
				    &nreturn_vals,
				    PARAM_INT32, num_samples,
				    PARAM_FLOATARRAY, positions,
				    PARAM_END);

  result = NULL;

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      nresult = return_vals[1].data.d_int32;
      result  = g_new (gdouble, nresult);
      for (i = 0; i < nresult; i++)
	result[i] = return_vals[2].data.d_floatarray[i];
    }

  gimp_destroy_params (return_vals, nreturn_vals);

  return result;
}
