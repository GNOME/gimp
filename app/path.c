/* The GIMP -- an image manipulation program
 * Copyright (C) 1999 Andy Thomas alt@picnic.demon.co.uk
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

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "core/core-types.h"
#include "tools/tools-types.h"

#include "core/gimpimage.h"

#include "tools/gimpbezierselecttool.h"

#include "gdisplay.h"
#include "gui/paths-dialog.h"

#include "path.h"
#include "pathP.h"


static gchar * unique_name (GimpImage *gimage,
			    gchar     *cstr);


Path *
path_new (GimpImage *gimage,
	  PathType   ptype,
	  GSList    *path_details,
	  gint       closed,
	  gint       state,
	  gint       locked,
	  gint       tattoo,  
	  gchar     *name)
{
  gchar *suniq;
  Path  *path = g_new0 (Path, 1);

  suniq = unique_name (gimage, name);
  if (suniq)
    path->name = suniq;
  else
    path->name = g_strdup (name);

  path->path_details = path_details;
  path->closed       = closed;
  path->state        = state;
  path->locked       = locked;
  path->pathtype     = ptype;

  if(tattoo)
    path->tattoo = tattoo;
  else
    path->tattoo = gimp_image_get_new_tattoo (gimage);

  return path;
}

Path *
path_copy (GimpImage *gimage,
	   Path      *path)
{
  Path  *p_copy = g_new0 (Path, 1);
  gchar *name;
  
  name = unique_name (gimage, path->name);

  if (name)
    p_copy->name = name;
  else
    p_copy->name = g_strdup (path->name);

  p_copy->closed       = path->closed;
  p_copy->state        = path->state;
  p_copy->pathtype     = path->pathtype;
  p_copy->path_details = pathpoints_copy (path->path_details);

  if (gimage)
    p_copy->tattoo = gimp_image_get_new_tattoo (gimage);
  else
    p_copy->tattoo = path->tattoo;

  return p_copy;
}

void 
path_free (Path *path)
{
  g_return_if_fail (path != NULL);

  g_free (path->name);
  pathpoints_free (path->path_details);
  g_free (path);
}


PathPoint * 
path_point_new (guint    type,
		gdouble  x, 
		gdouble  y)
{
  PathPoint *pathpoint = g_new0 (PathPoint,1);

  pathpoint->type = type;
  pathpoint->x    = x;
  pathpoint->y    = y;

  return pathpoint;
}

void
path_point_free (PathPoint *pathpoint)
{
  g_free (pathpoint);
}

void 
path_stroke (GimpImage *gimage,
	     PathList  *pl,
	     Path      *bzp)
{
  GimpBezierSelectTool *bezier_sel;
  GDisplay     *gdisp;

  gdisp = gdisplays_check_valid (pl->gdisp, gimage);
  bezier_sel = path_to_beziersel (bzp);
  bezier_stroke (bezier_sel, gdisp, SUBDIVIDE, !bzp->closed);
  bezier_select_free (bezier_sel);
}

gint 
path_distance (Path    *bzp,
	       gdouble  dist,
	       gint    *x,
	       gint    *y, 
	       gdouble *grad)
{
  gint          ret;
  GimpBezierSelectTool *bezier_sel;

  bezier_sel = path_to_beziersel (bzp);
  ret = bezier_distance_along (bezier_sel, !bzp->closed, dist, x, y, grad);
  bezier_select_free (bezier_sel);

  return ret;
}

Tattoo
path_get_tattoo (Path* p)
{
  if (!p)
    {
      g_warning ("path_get_tattoo: invalid path");
      return 0;
    }

  return p->tattoo;
}

Path*
path_get_path_by_tattoo (GimpImage *gimage,
			 Tattoo     tattoo)
{
  GSList   *tlist;
  PathList *plp;

  if (!gimage || !tattoo)
    return NULL;

  /* Go around the list and check all tattoos. */

  /* Get path structure  */
  plp = (PathList*) gimp_image_get_paths (gimage);

  if (!plp)
    return (NULL);
  
  for (tlist = plp->bz_paths; tlist; tlist = g_slist_next(tlist))
    {
      Path* p = (Path*)(tlist->data);

      if (p->tattoo == tattoo)
	return (p);
    }

  return (NULL);
}

PathList *
path_list_new (GimpImage *gimage,
	       gint       last_selected_row,
	       GSList    *bz_paths)
{
  PathList *pip = g_new0 (PathList, 1);

  pip->gimage            = gimage;
  pip->last_selected_row = last_selected_row;
  
  /* add connector to image delete/destroy */
  pip->sig_id = g_signal_connect (G_OBJECT (gimage), "destroy",
                                  G_CALLBACK (paths_dialog_destroy_cb),
                                  pip);
  
  pip->bz_paths = bz_paths;

  return (PathList *) pip;
}

void
path_list_free (PathList* iml)
{
  g_return_if_fail (iml != NULL);

  if (iml->bz_paths)
    {
      g_slist_foreach (iml->bz_paths, (GFunc) path_free, NULL);
      g_slist_free (iml->bz_paths);
    }

  g_free (iml);
}


/* Always return a copy that must be freed later */

static gchar *
strip_off_cnumber (gchar *str)
{
  gchar *hashptr;
  gint   num;
  gchar *copy;

  if (!str)
    return str;

  copy = g_strdup (str);

  if ((hashptr = strrchr (copy,'#')) &&              /* have a hash */
      strlen(hashptr) > 0 &&                         /* followed by something */
      (num = atoi(hashptr+1)) > 0 &&                 /* which is a number */
      ((int) log10 (num) + 1) == strlen (hashptr+1)) /* which is at the end */
    {
      gchar *tstr;

      /* Has a #<number> */
      *hashptr = '\0';
      tstr = g_strdup (copy);
      g_free (copy);
      copy = tstr;
    }

  return copy;
}

/* Return NULL if already unique else a unique string */

static gchar *
unique_name (GimpImage *gimage,
	     gchar     *cstr)
{
  GSList   *tlist;
  PathList *plp;
  gboolean  unique = TRUE;
  gchar    *copy_cstr;
  gchar    *copy_test;
  gchar    *stripped_copy;
  gint      counter = 1;

  /* Get bzpath structure  */
  if (!gimage || !(plp = (PathList*) gimp_image_get_paths(gimage)))
    return NULL;

  tlist = plp->bz_paths;

  while (tlist)
    {
      gchar *test_str;

      test_str = ((Path *) (tlist->data))->name;

      if (strcmp (cstr, test_str) == 0)
	{
	    unique = FALSE;
	    break;
	}
      tlist = g_slist_next(tlist);
    }

  if (unique)
    return NULL;

  /* OK Clashes with something */
  /* restart scan to find unique name */

  stripped_copy = strip_off_cnumber (cstr);
  copy_cstr = g_strdup_printf ("%s#%d", stripped_copy, counter++);

  tlist = plp->bz_paths;

  while (tlist)
    {
      copy_test = ((Path*)(tlist->data))->name;

      if (strcmp (copy_cstr,copy_test) == 0)
	{
	  g_free (copy_cstr);
	  copy_cstr = g_strdup_printf ("%s#%d", stripped_copy, counter++);
	  tlist = plp->bz_paths;
	  continue;
	}

      tlist = g_slist_next(tlist);
    }

  g_free (stripped_copy);

  return copy_cstr;
}
