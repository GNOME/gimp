/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpproceduraldb_pdb.c
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

void
gimp_procedural_db_set_data (gchar    *id,
			     gpointer  data,
			     guint32   length)
{
  GimpParam *return_vals;
  int nreturn_vals;

  return_vals = gimp_run_procedure ("gimp_procedural_db_set_data",
				    &nreturn_vals,
				    PARAM_STRING, id,
				    PARAM_INT32, length,
				    PARAM_INT8ARRAY, data,
				    PARAM_END);

  gimp_destroy_params (return_vals, nreturn_vals);
}

guint32
gimp_procedural_db_get_data_size (gchar *id)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  guint32 length;

  return_vals = gimp_run_procedure ("gimp_procedural_db_get_data_size",
                                    &nreturn_vals,
                                    PARAM_STRING, id,
                                    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
      length= return_vals[1].data.d_int32;
  else
      length= 0;

  gimp_destroy_params (return_vals, nreturn_vals);
  return length;
}

void
gimp_procedural_db_get_data (gchar    *id,
			     gpointer  data)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gint length;
  gchar *returned_data;

  return_vals = gimp_run_procedure ("gimp_procedural_db_get_data",
				    &nreturn_vals,
				    PARAM_STRING, id,
				    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      length = return_vals[1].data.d_int32;
      returned_data = (gchar *) return_vals[2].data.d_int8array;

      memcpy (data, returned_data, length);
    }

  gimp_destroy_params (return_vals, nreturn_vals);
}

void
gimp_procedural_db_query (gchar   *name_regexp,
			  gchar   *blurb_regexp,
			  gchar   *help_regexp,
			  gchar   *author_regexp,
			  gchar   *copyright_regexp,
			  gchar   *date_regexp,
			  gchar   *proc_type_regexp,
			  gint    *nprocs,
			  gchar ***proc_names)
{
  GimpParam *return_vals;
  gint nreturn_vals;
  gint i;

  return_vals = gimp_run_procedure ("gimp_procedural_db_query",
				    &nreturn_vals,
				    PARAM_STRING, name_regexp,
				    PARAM_STRING, blurb_regexp,
				    PARAM_STRING, help_regexp,
				    PARAM_STRING, author_regexp,
				    PARAM_STRING, copyright_regexp,
				    PARAM_STRING, date_regexp,
				    PARAM_STRING, proc_type_regexp,
				    PARAM_END);

  *nprocs = 0;
  *proc_names = NULL;

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *nprocs = return_vals[1].data.d_int32;
      *proc_names = g_new (gchar*, *nprocs);

      for (i = 0; i < *nprocs; i++)
	(*proc_names)[i] = g_strdup (return_vals[2].data.d_stringarray[i]);
    }

  gimp_destroy_params (return_vals, nreturn_vals);
}

gboolean
gimp_procedural_db_proc_info (gchar         *proc_name,
			      gchar        **proc_blurb,
			      gchar        **proc_help,
			      gchar        **proc_author,
			      gchar        **proc_copyright,
			      gchar        **proc_date,
			      gint          *proc_type,
			      gint          *nparams,
			      gint          *nreturn_vals,
			      GimpParamDef **params,
			      GimpParamDef **return_vals)
{
  GimpParam *ret_vals;
  gint nret_vals;
  gint i;
  gboolean success = TRUE;

  ret_vals = gimp_run_procedure ("gimp_procedural_db_proc_info",
				 &nret_vals,
				 PARAM_STRING, proc_name,
				 PARAM_END);

  if (ret_vals[0].data.d_status == STATUS_SUCCESS)
    {
      *proc_blurb     = g_strdup (ret_vals[1].data.d_string);
      *proc_help      = g_strdup (ret_vals[2].data.d_string);
      *proc_author    = g_strdup (ret_vals[3].data.d_string);
      *proc_copyright = g_strdup (ret_vals[4].data.d_string);
      *proc_date      = g_strdup (ret_vals[5].data.d_string);
      *proc_type      = ret_vals[6].data.d_int32;
      *nparams        = ret_vals[7].data.d_int32;
      *nreturn_vals   = ret_vals[8].data.d_int32;
      *params         = g_new (GimpParamDef, *nparams);
      *return_vals    = g_new (GimpParamDef, *nreturn_vals);

      for (i = 0; i < *nparams; i++)
	{
	  GimpParam *rvals;
	  gint nrvals;

	  rvals = gimp_run_procedure ("gimp_procedural_db_proc_arg",
				      &nrvals,
				      PARAM_STRING, proc_name,
				      PARAM_INT32, i,
				      PARAM_END);

	  if (rvals[0].data.d_status == STATUS_SUCCESS)
	    {
	      (* params) [i].type        = rvals[1].data.d_int32;
	      (* params) [i].name        = g_strdup (rvals[2].data.d_string);
	      (* params) [i].description = g_strdup (rvals[3].data.d_string);
	    }
	  else
	    {
	      g_free (*params);
	      g_free (*return_vals);
	      gimp_destroy_params (rvals, nrvals);
	      return FALSE;
	    }

	  gimp_destroy_params (rvals, nrvals);
	}

      for (i = 0; i < *nreturn_vals; i++)
	{
	  GimpParam *rvals;
	  gint nrvals;

	  rvals = gimp_run_procedure ("gimp_procedural_db_proc_val",
				      &nrvals,
				      PARAM_STRING, proc_name,
				      PARAM_INT32, i,
				      PARAM_END);

	  if (rvals[0].data.d_status == STATUS_SUCCESS)
	    {
	      (* return_vals)[i].type        = rvals[1].data.d_int32;
	      (* return_vals)[i].name        = g_strdup (rvals[2].data.d_string);
	      (* return_vals)[i].description = g_strdup (rvals[3].data.d_string);
	    }
	  else
	    {
	      g_free (*params);
	      g_free (*return_vals);
	      gimp_destroy_params (rvals, nrvals);
	      return FALSE;
	    }

	  gimp_destroy_params (rvals, nrvals);
	}
    }
  else
    success = FALSE;

  gimp_destroy_params (ret_vals, nret_vals);

  return success;
}
