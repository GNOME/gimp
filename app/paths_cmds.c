/* The GIMP -- an image manipulation program
 * Copyright (C) 1999 Andy Thomas alt@gimp.org
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
 * Some of this code is based on the layers_dialog box code.
 */

#include <stdio.h>
#include <stdlib.h>
#include "appenv.h"
#include "general.h"
#include "gimage.h"
#include "pathsP.h"
#include "paths_cmds.h"

#include "libgimp/gimpintl.h"

static int success;

/* These are the PDB functions that interact with the PATHS structures. */

/* List the paths that an image has */

static Argument *
path_list_invoker (Argument *args)
{
  Argument *return_args;
  gchar   **paths_list;
  gint      num_paths;
  GImage   *gimage;
  gint      int_value;

  gimage = NULL;

  success = TRUE;
  if (success)
    {
      int_value = args[0].value.pdb_int;
      if ((gimage = gimage_get_ID (int_value)) == NULL)
	success = FALSE;
    }

  return_args = procedural_db_return_args (&path_list_proc, success);

  if (success)
    {
      PathsList *plist = gimage->paths;

      if(plist && plist->bz_paths)
	{
	  gint    count = 0;
	  GSList *pl    = plist->bz_paths;

	  num_paths = g_slist_length(plist->bz_paths);
	  return_args[1].value.pdb_int = num_paths;

	  paths_list = g_malloc(sizeof(gchar *) * num_paths);
	  while(pl)
	    {
	      PATHP pptr = pl->data;
	      paths_list[count++] = g_strdup(pptr->name->str);
	      pl = g_slist_next(pl);
	    }
	  return_args[2].value.pdb_pointer = paths_list;
	}
      else
	{
	  return_args[1].value.pdb_int = 0;
	  return_args[2].value.pdb_pointer = NULL;
	}
    }

  return return_args;
}

/*  The procedure definition  */
ProcArg path_list_in_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image to list the paths from"
  },
};

ProcArg path_list_out_args[] =
{
  { PDB_INT32,
    "num_paths",
    "The number of paths returned"
  },
  { PDB_STRINGARRAY,
    "paths_list",
    "list of the paths belonging to this image"
  }
};

ProcRecord path_list_proc =
{
  "gimp_path_list",
  "List the paths associated with the passed image",
  "List the paths associated with the passed image",
  "Andy Thomas",
  "Andy Thomas",
  "1999",
  PDB_INTERNAL,

  /*  Input arguments  */
  sizeof(path_list_in_args)/sizeof(path_list_in_args[0]),
  path_list_in_args,

  /*  Output arguments  */
  sizeof(path_list_out_args)/sizeof(path_list_out_args[0]),
  path_list_out_args,

  /*  Exec method  */
  { { path_list_invoker } },
};

/* Get the points the named path is composed of */

static Argument *
path_get_points_invoker (Argument *args)
{
  Argument *return_args = NULL;
  GImage   *gimage;
  gchar    *pname;
  gint      int_value;

  gimage = NULL;

  success = TRUE;
  int_value = args[0].value.pdb_int;
  if ((gimage = gimage_get_ID (int_value)) == NULL)
    success = FALSE;

  if (success)
    {
      /* Get the path with the given name */
      PathsList *plist = gimage->paths;
      pname = args[1].value.pdb_pointer;
      
      if(pname && plist && plist->bz_paths)
	{
	  GSList *pl    = plist->bz_paths;
	  PATHP   pptr  = NULL;

	  while(pl)
	    {
	      pptr = pl->data;
	      if(strcmp(pname,pptr->name->str) == 0)
		{
		  /* Found the path */
		  break;
		}
	      pl = g_slist_next(pl);
	      pptr = NULL;
	    }

	  if(pl && pptr)
	    {
	      gint     num_pdetails;
	      gdouble *pnts;
	      GSList  *points_list;
	      gint     pcount = 0;

	      return_args = procedural_db_return_args (&path_get_points_proc, success);
	      /* Get the details for this path */
	      return_args[1].value.pdb_int = pptr->pathtype;
	      return_args[2].value.pdb_int = pptr->closed;

	      points_list = pptr->path_details;
	      if(points_list)
		{
		  num_pdetails = g_slist_length(points_list);
		  return_args[3].value.pdb_int = num_pdetails*3; /* 3 floats for each point */

		  pnts = g_malloc(sizeof(gdouble)*3*num_pdetails);

		  /* fill points and types in */
		  while(points_list)
		    {
		      PATHPOINTP ppoint = points_list->data;
		      pnts[pcount] = ppoint->x;
		      pnts[pcount+1] = ppoint->y;
		      pnts[pcount+2] = (gfloat)ppoint->type; /* Bit of fiddle but should be understandable why it was done */
		      pcount += 3;
		      points_list = g_slist_next(points_list);
		    }
		}
	      else
		{
		  return_args[3].value.pdb_int = 0;
		  pnts = NULL;
		}

	      return_args[4].value.pdb_pointer = pnts;
	    }
	  else
	    {
	      success = FALSE;
	    }
	}
      else
	{
	  success = FALSE;
	}
    }

  if(!success)
    {
      return_args = procedural_db_return_args (&path_get_points_proc, success);
    }

  return return_args;
}

/*  The procedure definition  */
ProcArg path_get_points_in_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image to list the paths from"
  },
  { PDB_STRING,
    "pathname",
    "the name of the path whose points should be listed"
  },
};

ProcArg path_get_points_out_args[] =
{
  { PDB_INT32,
    "paths_type",
    "The type of the path. Currently only one type (1 = Bezier) is supported"
  },
  { PDB_INT32,
    "pathclosed",
    "Return if the path is closed. {0=path open, 1= path closed}"
  },
  { PDB_INT32,
    "num_path_point_details",
    "The number of point returned. Each point is made up of (x,y,pnt_type) of floats"
  },
  { PDB_FLOATARRAY,
    "points_pairs",
    "The points in the path represented as 3 floats. The first is the x pos, next is the y pos, last is the type of the pnt. The type field is dependant on the path type. For beziers (type 1 paths) the type can either be {1.0= BEZIER_ANCHOR, 2.0= BEZIER_CONTROL}. Note all points are returned in pixel resolution"
  }
};

ProcRecord path_get_points_proc =
{
  "gimp_path_get_points",
  "List the points associated with the named path",
  "List the points associated with the named path",
  "Andy Thomas",
  "Andy Thomas",
  "1999",
  PDB_INTERNAL,

  /*  Input arguments  */
  sizeof(path_get_points_in_args)/sizeof(path_get_points_in_args[0]),
  path_get_points_in_args,

  /*  Output arguments  */
  sizeof(path_get_points_out_args)/sizeof(path_get_points_out_args[0]),
  path_get_points_out_args,

  /*  Exec method  */
  { { path_get_points_invoker } },
};

/* Get the name of the current path (of the passed image) */

static Argument *
path_get_current_invoker (Argument *args)
{
  Argument *return_args = NULL;
  GImage   *gimage;
  gint      int_value;

  gimage = NULL;

  success = TRUE;
  int_value = args[0].value.pdb_int;

  if ((gimage = gimage_get_ID (int_value)) == NULL)
    success = FALSE;

  if (success)
    {
      /* Get the path with the given name */
      PathsList *plist = gimage->paths;

      if(plist && plist->bz_paths)
	{
	  PATHP          pptr    = NULL;

	  if(plist->last_selected_row >= 0)
	    {
	      pptr = (PATHP)g_slist_nth_data(plist->bz_paths,plist->last_selected_row);
  	      return_args = procedural_db_return_args (&path_get_current_proc, success);
	      return_args[1].value.pdb_pointer = g_strdup(pptr->name->str);
	    }
	  else
	    {
	      success = FALSE;
	    }
	}
      else
	{
	  success = FALSE;
	}
    }

  if(!success)
    {
      return_args = procedural_db_return_args (&path_get_points_proc, success);
    }

  return return_args;
}

/*  The procedure definition  */
ProcArg path_get_current_in_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image to get the current paths from"
  }
};

ProcArg path_get_current_out_args[] =
{
  { PDB_STRING,
    "current_path_name",
    "The name of the current path"
  }
};

ProcRecord path_get_current_proc =
{
  "gimp_path_get_current",
  "The name of the current path. Error if no paths",
  "The name of the current path. Error if no paths",
  "Andy Thomas",
  "Andy Thomas",
  "1999",
  PDB_INTERNAL,

  /*  Input arguments  */
  sizeof(path_get_current_in_args)/sizeof(path_get_current_in_args[0]),
  path_get_current_in_args,

  /*  Output arguments  */
  sizeof(path_get_current_out_args)/sizeof(path_get_current_out_args[0]),
  path_get_current_out_args,

  /*  Exec method  */
  { { path_get_current_invoker } },
};


/* Set the name of the current path (of the passed image) */

static Argument *
path_set_current_invoker (Argument *args)
{
  Argument *return_args = NULL;
  GImage   *gimage;
  gint      int_value;
  gchar    *pname;

  gimage = NULL;

  success = TRUE;
  int_value = args[0].value.pdb_int;

  if ((gimage = gimage_get_ID (int_value)) == NULL)
    success = FALSE;

  if (success)
    {
      /* Set the current path to the given name */
      pname = args[1].value.pdb_pointer;

      if(pname && paths_set_path(gimage,pname))
	{
	  success = TRUE;
	}
      else
	{
	  success = FALSE;
	}
    }
      
  return_args = procedural_db_return_args (&path_set_current_proc, success);

  return return_args;
}

/*  The procedure definition  */
ProcArg path_set_current_in_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image to set the current paths"
  },
  { PDB_STRING,
    "set_current_path_name",
    "The name of the path to set the current path to"
  }
};

/* NO out args.. Either works or errors. */

ProcRecord path_set_current_proc =
{
  "gimp_path_set_current",
  "The name path to set to the current. Error if path does not exist.",
  "The name path to set to the current. Error if path does not exist.",
  "Andy Thomas",
  "Andy Thomas",
  "1999",
  PDB_INTERNAL,

  /*  Input arguments  */
  sizeof(path_set_current_in_args)/sizeof(path_set_current_in_args[0]),
  path_set_current_in_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { path_set_current_invoker } },
};


/* Set a path up from the given points */
/* Currently only the creation of bezier curves is allowed. 
 * The type parameter must be set to (1) to indicate a BEZIER type curve.
 */
/* For BEZIERS..
 * 
 * Note the that points must be given in the following order...
 * ACCACCACC ...
 * The last control point is missing for non closed curves.
 */

static Argument *
path_set_points_invoker (Argument *args)
{
  Argument *return_args = NULL;
  GImage   *gimage;
  gchar    *pname;
  gint      int_value;
  gint      ptype;
  gint      numpoints;
  gint      pclosed = FALSE;
  gdouble *pnts;

  gimage = NULL;

  success = TRUE;
  int_value = args[0].value.pdb_int;
  if ((gimage = gimage_get_ID (int_value)) == NULL)
    success = FALSE;

  if (success)
    {
      pname = args[1].value.pdb_pointer;
      ptype = args[2].value.pdb_int;
      numpoints = args[3].value.pdb_int;

      if((numpoints/2)%3 == 0)
	pclosed = TRUE;
      else if ((numpoints/2)%3 != 2)
	success = FALSE;
  
      pnts = args[4].value.pdb_pointer;
    }

  if(success)
    {

      if(paths_set_path_points(gimage,pname,ptype,pclosed,numpoints,pnts))
	return_args = procedural_db_return_args (&path_get_points_proc, success);
      else
	success = FALSE;
    }
  else
    {
      success = FALSE;
    }

  if(!success)
    {
      return_args = procedural_db_return_args (&path_get_points_proc, success);
    }

  return return_args;
}

/*  The procedure definition  */
ProcArg path_set_points_in_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image to list the paths from"
  },
  { PDB_STRING,
    "pathname",
    "the name of the path to create (if it exists then all current points are removed). This will not be set as the current path.You will have to do a gimp_set_current_path after creating the path to make it current."
  },
  { PDB_INT32,
    "path_type",
    "The type of the path. Currently only one type (1 = Bezier) is supported"
  },
  { PDB_INT32,
    "num_path_points",
    "The number of points in the path. Each point is made up of (x,y) of floats. "
    "Currently only the creation of bezier curves is allowed. "
    "The type parameter must be set to (1) to indicate a BEZIER type curve.\n"
    "For BEZIERS.\n"
    "Note the that points must be given in the following order... "
    "ACCACCAC ... "
    "If the path is not closed the last control point is missed off. "
    "Points consist of three control points (control/anchor/control) "
    "so for a curve that is not closed there must be at least two points passed (2 x,y pairs). "
    "If num_path_pnts%3 = 0 then the path is assumed to be closed and the points are ACCACCACCACC."
  },
  { PDB_FLOATARRAY,
    "points_pairs",
    "The points in the path represented as 2 floats. The first is the x pos, next is the y pos. The order of points define there types (see num_path_points comment)."
  }
  
};

/* No Out args */

ProcRecord path_set_points_proc =
{
  "gimp_path_set_points",
  "Set the points associated with the named path",
  "Set the points associated with the named path",
  "Andy Thomas",
  "Andy Thomas",
  "1999",
  PDB_INTERNAL,

  /*  Input arguments  */
  sizeof(path_set_points_in_args)/sizeof(path_set_points_in_args[0]),
  path_set_points_in_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { path_set_points_invoker } },
};


/* Stroke the current path */
/* Note it will ONLY do the current path.. */

static Argument *
path_stroke_current_invoker (Argument *args)
{
  Argument *return_args = NULL;
  GImage   *gimage;
  gint      int_value;

  gimage = NULL;
  success = TRUE;
  int_value = args[0].value.pdb_int;

  if ((gimage = gimage_get_ID (int_value)) == NULL)
    success = FALSE;

  if (success)
    {
      /* Get the path with the given name */
      PathsList *plist = gimage->paths;

      if(plist && plist->bz_paths)
	{
	  GSList *pl    = plist->bz_paths;
	  PATHP   pptr  = NULL;

	  if(plist->last_selected_row >= 0 &&
	     (pptr = (PATHP)g_slist_nth_data(plist->bz_paths,plist->last_selected_row)))
	    {
	      /* Found the path to stroke.. */
	      paths_stroke(gimage,plist,pptr);
	    }
	  else
	    {
	      success = FALSE;
	    }
	}
      else
	{
	  success = FALSE;
	}
    }
      
  return_args = procedural_db_return_args (&path_set_current_proc, success);

  return return_args;
}

/*  The procedure definition  */
ProcArg path_stroke_current_in_args[] =
{
  { PDB_IMAGE,
    "image",
    "the image which contains the path to stroke"
  },
};

/* NO out args.. Either works or errors. */

ProcRecord path_stroke_current_proc =
{
  "gimp_path_stroke_current",
  "Stroke the current path in the passed image",
  "Stroke the current path in the passed image",
  "Andy Thomas",
  "Andy Thomas",
  "1999",
  PDB_INTERNAL,

  /*  Input arguments  */
  sizeof(path_stroke_current_in_args)/sizeof(path_stroke_current_in_args[0]),
  path_stroke_current_in_args,

  /*  Output arguments  */
  0,
  NULL,

  /*  Exec method  */
  { { path_stroke_current_invoker } },
};
