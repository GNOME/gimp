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

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpprotocol.h"

#include "plug-in-types.h"

#include "pdb/procedural_db.h"

#include "plug-in.h"
#include "plug-in-params.h"


Argument *
plug_in_params_to_args (GPParam  *params,
			gint      nparams,
			gboolean  full_copy)
{
  Argument  *args;
  gchar    **stringarray;
  gint       count;
  gint       i, j;

  if (! (params && nparams))
    return NULL;

  args = g_new0 (Argument, nparams);

  for (i = 0; i < nparams; i++)
    {
      args[i].arg_type = params[i].type;

      switch (args[i].arg_type)
	{
	case GIMP_PDB_INT32:
	  args[i].value.pdb_int = params[i].data.d_int32;
	  break;
	case GIMP_PDB_INT16:
	  args[i].value.pdb_int = params[i].data.d_int16;
	  break;
	case GIMP_PDB_INT8:
	  args[i].value.pdb_int = params[i].data.d_int8;
	  break;
	case GIMP_PDB_FLOAT:
	  args[i].value.pdb_float = params[i].data.d_float;
	  break;
	case GIMP_PDB_STRING:
	  if (full_copy)
	    args[i].value.pdb_pointer = g_strdup (params[i].data.d_string);
	  else
	    args[i].value.pdb_pointer = params[i].data.d_string;
	  break;
	case GIMP_PDB_INT32ARRAY:
	  if (full_copy)
	    {
	      count = args[i-1].value.pdb_int;
	      args[i].value.pdb_pointer = g_new (gint32, count);
	      memcpy (args[i].value.pdb_pointer,
		      params[i].data.d_int32array, count * 4);
	    }
	  else
	    {
	      args[i].value.pdb_pointer = params[i].data.d_int32array;
	    }
	  break;
	case GIMP_PDB_INT16ARRAY:
	  if (full_copy)
	    {
	      count = args[i-1].value.pdb_int;
	      args[i].value.pdb_pointer = g_new (gint16, count);
	      memcpy (args[i].value.pdb_pointer,
		      params[i].data.d_int16array, count * 2);
	    }
	  else
	    {
	      args[i].value.pdb_pointer = params[i].data.d_int16array;
	    }
	  break;
	case GIMP_PDB_INT8ARRAY:
	  if (full_copy)
	    {
	      count = args[i-1].value.pdb_int;
	      args[i].value.pdb_pointer = g_new (gint8, count);
	      memcpy (args[i].value.pdb_pointer,
		      params[i].data.d_int8array, count);
	    }
	  else
	    {
	      args[i].value.pdb_pointer = params[i].data.d_int8array;
	    }
	  break;
	case GIMP_PDB_FLOATARRAY:
	  if (full_copy)
	    {
	      count = args[i-1].value.pdb_int;
	      args[i].value.pdb_pointer = g_new (gdouble, count);
	      memcpy (args[i].value.pdb_pointer,
		      params[i].data.d_floatarray, count * 8);
	    }
	  else
	    {
	      args[i].value.pdb_pointer = params[i].data.d_floatarray;
	    }
	  break;
	case GIMP_PDB_STRINGARRAY:
	  if (full_copy)
	    {
	      args[i].value.pdb_pointer = g_new (gchar *,
						 args[i-1].value.pdb_int);
	      stringarray = args[i].value.pdb_pointer;

	      for (j = 0; j < args[i-1].value.pdb_int; j++)
		stringarray[j] = g_strdup (params[i].data.d_stringarray[j]);
	    }
	  else
	    {
	      args[i].value.pdb_pointer = params[i].data.d_stringarray;
	    }
	  break;
	case GIMP_PDB_COLOR:
	  args[i].value.pdb_color = params[i].data.d_color;
	  break;
	case GIMP_PDB_REGION:
	  g_message ("the \"region\" arg type is not currently supported");
	  break;
	case GIMP_PDB_DISPLAY:
	  args[i].value.pdb_int = params[i].data.d_display;
	  break;
	case GIMP_PDB_IMAGE:
	  args[i].value.pdb_int = params[i].data.d_image;
	  break;
	case GIMP_PDB_LAYER:
	  args[i].value.pdb_int = params[i].data.d_layer;
	  break;
	case GIMP_PDB_CHANNEL:
	  args[i].value.pdb_int = params[i].data.d_channel;
	  break;
	case GIMP_PDB_DRAWABLE:
	  args[i].value.pdb_int = params[i].data.d_drawable;
	  break;
	case GIMP_PDB_SELECTION:
	  args[i].value.pdb_int = params[i].data.d_selection;
	  break;
	case GIMP_PDB_BOUNDARY:
	  args[i].value.pdb_int = params[i].data.d_boundary;
	  break;
	case GIMP_PDB_PATH:
	  args[i].value.pdb_int = params[i].data.d_path;
	  break;
	case GIMP_PDB_PARASITE:
	  if (full_copy)
	    args[i].value.pdb_pointer =
	      gimp_parasite_copy ((GimpParasite *) &(params[i].data.d_parasite));
	  else
	    args[i].value.pdb_pointer = (gpointer) &(params[i].data.d_parasite);
	  break;
	case GIMP_PDB_STATUS:
	  args[i].value.pdb_int = params[i].data.d_status;
	  break;
	case GIMP_PDB_END:
	  break;
	}
    }

  return args;
}

GPParam *
plug_in_args_to_params (Argument *args,
			gint      nargs,
			gboolean  full_copy)
{
  GPParam  *params;
  gchar   **stringarray;
  gint      i, j;

  if (! (args && nargs))
    return NULL;

  params = g_new0 (GPParam, nargs);

  for (i = 0; i < nargs; i++)
    {
      params[i].type = args[i].arg_type;

      switch (args[i].arg_type)
	{
	case GIMP_PDB_INT32:
	  params[i].data.d_int32 = args[i].value.pdb_int;
	  break;
	case GIMP_PDB_INT16:
	  params[i].data.d_int16 = args[i].value.pdb_int;
	  break;
	case GIMP_PDB_INT8:
	  params[i].data.d_int8 = args[i].value.pdb_int;
	  break;
	case GIMP_PDB_FLOAT:
	  params[i].data.d_float = args[i].value.pdb_float;
	  break;
	case GIMP_PDB_STRING:
	  if (full_copy)
	    params[i].data.d_string = g_strdup (args[i].value.pdb_pointer);
	  else
	    params[i].data.d_string = args[i].value.pdb_pointer;
	  break;
	case GIMP_PDB_INT32ARRAY:
	  if (full_copy)
	    {
	      params[i].data.d_int32array = g_new (gint32, params[i-1].data.d_int32);
	      memcpy (params[i].data.d_int32array,
		      args[i].value.pdb_pointer,
		      params[i-1].data.d_int32 * 4);
	    }
	  else
	    {
	      params[i].data.d_int32array = args[i].value.pdb_pointer;
	    }
	  break;
	case GIMP_PDB_INT16ARRAY:
	  if (full_copy)
	    {
	      params[i].data.d_int16array = g_new (gint16, params[i-1].data.d_int32);
	      memcpy (params[i].data.d_int16array,
		      args[i].value.pdb_pointer,
		      params[i-1].data.d_int32 * 2);
	    }
	  else
	    {
	      params[i].data.d_int16array = args[i].value.pdb_pointer;
	    }
	  break;
	case GIMP_PDB_INT8ARRAY:
	  if (full_copy)
	    {
	      params[i].data.d_int8array = g_new (gint8, params[i-1].data.d_int32);
	      memcpy (params[i].data.d_int8array,
		      args[i].value.pdb_pointer,
		      params[i-1].data.d_int32);
	    }
	  else
	    {
	      params[i].data.d_int8array = args[i].value.pdb_pointer;
	    }
	  break;
	case GIMP_PDB_FLOATARRAY:
	  if (full_copy)
	    {
	      params[i].data.d_floatarray = g_new (gdouble, params[i-1].data.d_int32);
	      memcpy (params[i].data.d_floatarray,
		      args[i].value.pdb_pointer,
		      params[i-1].data.d_int32 * 8);
	    }
	  else
	    {
	      params[i].data.d_floatarray = args[i].value.pdb_pointer;
	    }
	  break;
	case GIMP_PDB_STRINGARRAY:
	  if (full_copy)
	    {
	      params[i].data.d_stringarray = g_new (gchar*, params[i-1].data.d_int32);
	      stringarray = args[i].value.pdb_pointer;

	      for (j = 0; j < params[i-1].data.d_int32; j++)
		params[i].data.d_stringarray[j] = g_strdup (stringarray[j]);
	    }
	  else
	    {
	      params[i].data.d_stringarray = args[i].value.pdb_pointer;
	    }
	  break;
	case GIMP_PDB_COLOR:
	  params[i].data.d_color = args[i].value.pdb_color;
	  break;
	case GIMP_PDB_REGION:
	  g_message ("the \"region\" arg type is not currently supported");
	  break;
	case GIMP_PDB_DISPLAY:
	  params[i].data.d_display = args[i].value.pdb_int;
	  break;
	case GIMP_PDB_IMAGE:
	  params[i].data.d_image = args[i].value.pdb_int;
	  break;
	case GIMP_PDB_LAYER:
	  params[i].data.d_layer = args[i].value.pdb_int;
	  break;
	case GIMP_PDB_CHANNEL:
	  params[i].data.d_channel = args[i].value.pdb_int;
	  break;
	case GIMP_PDB_DRAWABLE:
	  params[i].data.d_drawable = args[i].value.pdb_int;
	  break;
	case GIMP_PDB_SELECTION:
	  params[i].data.d_selection = args[i].value.pdb_int;
	  break;
	case GIMP_PDB_BOUNDARY:
	  params[i].data.d_boundary = args[i].value.pdb_int;
	  break;
	case GIMP_PDB_PATH:
	  params[i].data.d_path = args[i].value.pdb_int;
	  break;
	case GIMP_PDB_PARASITE:
	  if (full_copy)
	    {
	      GimpParasite *tmp;

	      tmp = gimp_parasite_copy (args[i].value.pdb_pointer);
	      if (tmp == NULL)
		{
		  params[i].data.d_parasite.name  = NULL;
		  params[i].data.d_parasite.flags = 0;
		  params[i].data.d_parasite.size  = 0;
		  params[i].data.d_parasite.data  = NULL;
		}
	      else
		{
		  memcpy (&params[i].data.d_parasite, tmp,
			  sizeof (GimpParasite));
		  g_free (tmp);
		}
	    }
	  else
	    {
	      if (args[i].value.pdb_pointer == NULL)
		{
		  params[i].data.d_parasite.name  = NULL;
		  params[i].data.d_parasite.flags = 0;
		  params[i].data.d_parasite.size  = 0;
		  params[i].data.d_parasite.data  = NULL;
		}
	      else
		memcpy (&params[i].data.d_parasite,
			(GimpParasite *) (args[i].value.pdb_pointer),
			sizeof (GimpParasite));
	    }
	  break;
	case GIMP_PDB_STATUS:
	  params[i].data.d_status = args[i].value.pdb_int;
	  break;
	case GIMP_PDB_END:
	  break;
	}
    }

  return params;
}

void
plug_in_params_destroy (GPParam  *params,
			gint      nparams,
			gboolean  full_destroy)
{
  gint i, j;

  if (full_destroy)
    {
      for (i = 0; i < nparams; i++)
        {
          switch (params[i].type)
            {
            case GIMP_PDB_INT32:
            case GIMP_PDB_INT16:
            case GIMP_PDB_INT8:
            case GIMP_PDB_FLOAT:
              break;

            case GIMP_PDB_STRING:
              g_free (params[i].data.d_string);
              break;
            case GIMP_PDB_INT32ARRAY:
              g_free (params[i].data.d_int32array);
              break;
            case GIMP_PDB_INT16ARRAY:
              g_free (params[i].data.d_int16array);
              break;
            case GIMP_PDB_INT8ARRAY:
              g_free (params[i].data.d_int8array);
              break;
            case GIMP_PDB_FLOATARRAY:
              g_free (params[i].data.d_floatarray);
              break;
            case GIMP_PDB_STRINGARRAY:
              for (j = 0; j < params[i-1].data.d_int32; j++)
                g_free (params[i].data.d_stringarray[j]);
              g_free (params[i].data.d_stringarray);
              break;
            case GIMP_PDB_COLOR:
              break;
            case GIMP_PDB_REGION:
              g_message ("the \"region\" arg type is not currently supported");
              break;
            case GIMP_PDB_DISPLAY:
            case GIMP_PDB_IMAGE:
            case GIMP_PDB_LAYER:
            case GIMP_PDB_CHANNEL:
            case GIMP_PDB_DRAWABLE:
            case GIMP_PDB_SELECTION:
            case GIMP_PDB_BOUNDARY:
            case GIMP_PDB_PATH:
              break;
            case GIMP_PDB_PARASITE:
              if (params[i].data.d_parasite.data)
                {
                  g_free (params[i].data.d_parasite.name);
                  g_free (params[i].data.d_parasite.data);
                  params[i].data.d_parasite.name = NULL;
                  params[i].data.d_parasite.data = NULL;
                }
	  break;
            case GIMP_PDB_STATUS:
              break;
            case GIMP_PDB_END:
              break;
            }
        }
    }

  g_free (params);
}

void
plug_in_args_destroy (Argument *args,
		      gint      nargs,
		      gboolean  full_destroy)
{
  if (full_destroy)
    procedural_db_destroy_args (args, nargs);
  else
    g_free (args);
}

gboolean
plug_in_param_defs_check (const gchar *plug_in_name,
                          const gchar *plug_in_prog,
                          const gchar *procedure_name,
                          const gchar *menu_path,
                          GPParamDef  *params,
                          guint32      n_args,
                          GPParamDef  *return_vals,
                          guint32      n_return_vals,
                          GError     **error)
{
  return plug_in_proc_args_check (plug_in_name,
                                  plug_in_prog,
                                  procedure_name,
                                  menu_path,
                                  (ProcArg *) params,
                                  n_args,
                                  (ProcArg *) return_vals,
                                  n_return_vals,
                                  error);
}

gboolean
plug_in_proc_args_check (const gchar *plug_in_name,
                         const gchar *plug_in_prog,
                         const gchar *procedure_name,
                         const gchar *menu_path,
                         ProcArg     *args,
                         guint32      n_args,
                         ProcArg     *return_vals,
                         guint32      n_return_vals,
                         GError     **error)
{
  gchar *p;

  g_return_val_if_fail (plug_in_name != NULL, FALSE);
  g_return_val_if_fail (plug_in_prog != NULL, FALSE);
  g_return_val_if_fail (procedure_name != NULL, FALSE);
  g_return_val_if_fail (menu_path != NULL, FALSE);
  g_return_val_if_fail (args == NULL || n_args > 0, FALSE);
  g_return_val_if_fail (return_vals == NULL || n_return_vals > 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (strncmp (menu_path, "<Toolbox>", 9) == 0)
    {
      if ((n_args < 1) ||
          (args[0].arg_type != GIMP_PDB_INT32))
        {
          g_set_error (error, 0, 0,
                       "Plug-In \"%s\"\n(%s)\n\n"
                       "attempted to install <Toolbox> procedure \"%s\" "
                       "which does not take the standard <Toolbox> Plug-In "
                       "args.\n"
                       "(INT32)",
                       gimp_filename_to_utf8 (plug_in_name),
                       gimp_filename_to_utf8 (plug_in_prog),
                       procedure_name);
          return FALSE;
        }
    }
  else if (strncmp (menu_path, "<Image>", 7) == 0)
    {
      if ((n_args < 3) ||
          (args[0].arg_type != GIMP_PDB_INT32) ||
          (args[1].arg_type != GIMP_PDB_IMAGE) ||
          (args[2].arg_type != GIMP_PDB_DRAWABLE))
        {
          g_set_error (error, 0, 0,
                       "Plug-In \"%s\"\n(%s)\n\n"
                       "attempted to install <Image> procedure \"%s\" "
                       "which does not take the standard <Image> Plug-In "
                       "args.\n"
                       "(INT32, IMAGE, DRAWABLE)",
                       gimp_filename_to_utf8 (plug_in_name),
                       gimp_filename_to_utf8 (plug_in_prog),
                       procedure_name);
          return FALSE;
        }
    }
  else if (strncmp (menu_path, "<Load>", 6) == 0)
    {
      if ((n_args < 3) ||
          (args[0].arg_type != GIMP_PDB_INT32) ||
          (args[1].arg_type != GIMP_PDB_STRING) ||
          (args[2].arg_type != GIMP_PDB_STRING))
        {
          g_set_error (error, 0, 0,
                       "Plug-In \"%s\"\n(%s)\n\n"
                       "attempted to install <Load> procedure \"%s\" "
                       "which does not take the standard <Load> Plug-In "
                       "args.\n"
                       "(INT32, STRING, STRING)",
                       gimp_filename_to_utf8 (plug_in_name),
                       gimp_filename_to_utf8 (plug_in_prog),
                       procedure_name);
          return FALSE;
        }
    }
  else if (strncmp (menu_path, "<Save>", 6) == 0)
    {
      if ((n_args < 5) ||
          (args[0].arg_type != GIMP_PDB_INT32)    ||
          (args[1].arg_type != GIMP_PDB_IMAGE)    ||
          (args[2].arg_type != GIMP_PDB_DRAWABLE) ||
          (args[3].arg_type != GIMP_PDB_STRING)   ||
          (args[4].arg_type != GIMP_PDB_STRING))
        {
          g_set_error (error, 0, 0,
                       "Plug-In \"%s\"\n(%s)\n\n"
                       "attempted to install <Save> procedure \"%s\" "
                       "which does not take the standard <Save> Plug-In "
                       "args.\n"
                       "(INT32, IMAGE, DRAWABLE, STRING, STRING)",
                       gimp_filename_to_utf8 (plug_in_name),
                       gimp_filename_to_utf8 (plug_in_prog),
                       procedure_name);
          return FALSE;
        }
    }
  else
    {
      g_set_error (error, 0, 0,
                   "Plug-In \"%s\"\n(%s)\n"
                   "attempted to install procedure \"%s\" "
                   "in the invalid menu location \"%s\".\n"
                   "Use either \"<Toolbox>\", \"<Image>\", "
                   "\"<Load>\", or \"<Save>\".",
                   gimp_filename_to_utf8 (plug_in_name),
                   gimp_filename_to_utf8 (plug_in_prog),
                   procedure_name,
                   menu_path);
      return FALSE;
    }

  p = strchr (menu_path, '>') + 1;

  if (*p != '/' && *p != '\0')
    {
      g_set_error (error, 0, 0,
                   "Plug-In \"%s\"\n(%s)\n"
                   "attempted to install procedure \"%s\"\n"
                   "in the invalid menu location \"%s\".\n"
                   "The menu path must look like either \"<Prefix>\" "
                   "or \"<Prefix>/path/to/item\".",
                   gimp_filename_to_utf8 (plug_in_name),
                   gimp_filename_to_utf8 (plug_in_prog),
                   procedure_name,
                   menu_path);
      return FALSE;
    }

  return TRUE;
}
